#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "node.h"
#include <math.h>
#include <time.h>
#include <string.h>

typedef enum {
    ALGO_ASTAR=0, ALGO_DIJKSTRA, ALGO_BFS, ALGO_DFS, ALGO_GREEDY, ALGO_COUNT
} AlgoType;

typedef enum { HEURISTIC_MANHATTAN=0, HEURISTIC_EUCLIDEAN } HeuristicType;

static const char *algo_names[ALGO_COUNT] = {
    "A* Search","Dijkstra's","BFS","DFS","Greedy Best-First"
};

static const char *algo_descriptions[ALGO_COUNT] = {
    "A* uses f=g+h. g=cost from start, h=estimated cost to end. Finds shortest path efficiently.",
    "Dijkstra explores all nodes by distance from start. No heuristic. Always finds shortest path.",
    "BFS explores layer by layer. Guaranteed shortest path on unweighted grids. Very thorough.",
    "DFS dives deep before backtracking. Does NOT guarantee shortest path. Explores like a thread.",
    "Greedy only uses h=distance to goal. Fast but NOT guaranteed shortest path. Rushes to goal."
};

typedef struct {
    int      nodesVisited;
    int      pathLength;
    double   timeMs;
    bool     pathFound;
    AlgoType algo;
    HeuristicType heuristic;
} AlgoStats;

/* ── Node list ── */
typedef struct { Node **data; int size,cap; } NodeList;
static void nl_init(NodeList *l){l->data=NULL;l->size=l->cap=0;}
static void nl_free(NodeList *l){free(l->data);l->data=NULL;l->size=l->cap=0;}
static void nl_push(NodeList *l,Node *n){
    if(l->size==l->cap){l->cap=l->cap?l->cap*2:8;l->data=(Node**)realloc(l->data,sizeof(Node*)*l->cap);}
    l->data[l->size++]=n;
}
static Node *nl_pop(NodeList *l){return l->size?l->data[--l->size]:NULL;}
static Node *nl_shift(NodeList *l){
    if(!l->size)return NULL;
    Node *n=l->data[0];
    memmove(l->data,l->data+1,sizeof(Node*)*--l->size);
    return n;
}
static bool nl_contains(NodeList *l,Node *n){for(int i=0;i<l->size;i++)if(l->data[i]==n)return true;return false;}
static Node *nl_pop_lowest_f(NodeList *l){
    if(!l->size)return NULL;
    int b=0;for(int i=1;i<l->size;i++)if(l->data[i]->fCost<l->data[b]->fCost)b=i;
    Node *n=l->data[b];l->data[b]=l->data[--l->size];return n;
}
static Node *nl_pop_lowest_g(NodeList *l){
    if(!l->size)return NULL;
    int b=0;for(int i=1;i<l->size;i++)if(l->data[i]->gCost<l->data[b]->gCost)b=i;
    Node *n=l->data[b];l->data[b]=l->data[--l->size];return n;
}

/* ── Heuristics ── */
static double h_manhattan(Node *a,Node *b){return abs(a->x-b->x)+abs(a->y-b->y);}
static double h_euclidean(Node *a,Node *b){double dx=a->x-b->x,dy=a->y-b->y;return sqrt(dx*dx+dy*dy);}
static double heuristic(Node *a,Node *b,HeuristicType ht){return ht==HEURISTIC_MANHATTAN?h_manhattan(a,b):h_euclidean(a,b);}

typedef struct {
    NodeList closed;
    NodeList path;
    bool     found;
    int      nodesVisited;
    double   timeMs;
} PathResult;

static void result_free(PathResult *r){nl_free(&r->closed);nl_free(&r->path);}

static void get_neighbors(Node *grid,int gw,int gh,Node *n,Node **out,int *count){
    *count=0;
    int dx[]={0,0,-1,1},dy[]={-1,1,0,0};
    for(int d=0;d<4;d++){
        int nx=n->x+dx[d],ny=n->y+dy[d];
        if(nx>=0&&nx<gw&&ny>=0&&ny<gh)out[(*count)++]=&grid[nx*gh+ny];
    }
}

static void reset_path_state(Node *grid,int gw,int gh){
    for(int i=0;i<gw*gh;i++){
        grid[i].gCost=0;grid[i].hCost=0;grid[i].fCost=0;
        grid[i].parent=NULL;grid[i].visited=false;
    }
}

static void trace_path(Node *end,Node *start,NodeList *path){
    Node *cur=end->parent;
    while(cur&&cur!=start){nl_push(path,cur);cur=cur->parent;}
    for(int i=0,j=path->size-1;i<j;i++,j--){Node *t=path->data[i];path->data[i]=path->data[j];path->data[j]=t;}
}

static PathResult run_astar(Node *grid,int gw,int gh,Node *start,Node *end,HeuristicType ht){
    PathResult r;memset(&r,0,sizeof(r));nl_init(&r.closed);nl_init(&r.path);
    reset_path_state(grid,gw,gh);
    clock_t t0=clock();
    NodeList open;nl_init(&open);
    start->gCost=0;start->hCost=(int)heuristic(start,end,ht);start->fCost=start->hCost;
    nl_push(&open,start);
    Node *nb[4];int nc;
    while(open.size){
        Node *cur=nl_pop_lowest_f(&open);
        if(cur==end){r.found=true;break;}
        if(cur->visited)continue;
        cur->visited=true;nl_push(&r.closed,cur);r.nodesVisited++;
        get_neighbors(grid,gw,gh,cur,nb,&nc);
        for(int i=0;i<nc;i++){
            if(nb[i]->state==STATE_WALL||nb[i]->visited)continue;
            int tg=cur->gCost+1;
            if(!nl_contains(&open,nb[i])||tg<nb[i]->gCost){
                nb[i]->parent=cur;nb[i]->gCost=tg;
                nb[i]->hCost=(int)heuristic(nb[i],end,ht);
                nb[i]->fCost=nb[i]->gCost+nb[i]->hCost;
                nl_push(&open,nb[i]);
            }
        }
    }
    r.timeMs=(double)(clock()-t0)/CLOCKS_PER_SEC*1000.0;
    if(r.found)trace_path(end,start,&r.path);
    nl_free(&open);return r;
}

static PathResult run_dijkstra(Node *grid,int gw,int gh,Node *start,Node *end){
    PathResult r;memset(&r,0,sizeof(r));nl_init(&r.closed);nl_init(&r.path);
    reset_path_state(grid,gw,gh);
    for(int i=0;i<gw*gh;i++)grid[i].gCost=999999;
    start->gCost=0;
    clock_t t0=clock();
    NodeList open;nl_init(&open);nl_push(&open,start);
    Node *nb[4];int nc;
    while(open.size){
        Node *cur=nl_pop_lowest_g(&open);
        if(cur->visited)continue;
        cur->visited=true;nl_push(&r.closed,cur);r.nodesVisited++;
        if(cur==end){r.found=true;break;}
        get_neighbors(grid,gw,gh,cur,nb,&nc);
        for(int i=0;i<nc;i++){
            if(nb[i]->state==STATE_WALL||nb[i]->visited)continue;
            int tg=cur->gCost+1;
            if(tg<nb[i]->gCost){nb[i]->gCost=tg;nb[i]->fCost=tg;nb[i]->parent=cur;nl_push(&open,nb[i]);}
        }
    }
    r.timeMs=(double)(clock()-t0)/CLOCKS_PER_SEC*1000.0;
    if(r.found)trace_path(end,start,&r.path);
    nl_free(&open);return r;
}

static PathResult run_bfs(Node *grid,int gw,int gh,Node *start,Node *end){
    PathResult r;memset(&r,0,sizeof(r));nl_init(&r.closed);nl_init(&r.path);
    reset_path_state(grid,gw,gh);
    clock_t t0=clock();
    NodeList queue;nl_init(&queue);start->visited=true;nl_push(&queue,start);
    Node *nb[4];int nc;
    while(queue.size){
        Node *cur=nl_shift(&queue);nl_push(&r.closed,cur);r.nodesVisited++;
        if(cur==end){r.found=true;break;}
        get_neighbors(grid,gw,gh,cur,nb,&nc);
        for(int i=0;i<nc;i++){
            if(nb[i]->state==STATE_WALL||nb[i]->visited)continue;
            nb[i]->visited=true;nb[i]->parent=cur;nl_push(&queue,nb[i]);
        }
    }
    r.timeMs=(double)(clock()-t0)/CLOCKS_PER_SEC*1000.0;
    if(r.found)trace_path(end,start,&r.path);
    nl_free(&queue);return r;
}

static PathResult run_dfs(Node *grid,int gw,int gh,Node *start,Node *end){
    PathResult r;memset(&r,0,sizeof(r));nl_init(&r.closed);nl_init(&r.path);
    reset_path_state(grid,gw,gh);
    clock_t t0=clock();
    NodeList stack;nl_init(&stack);nl_push(&stack,start);
    Node *nb[4];int nc;
    while(stack.size){
        Node *cur=nl_pop(&stack);
        if(cur->visited)continue;
        cur->visited=true;nl_push(&r.closed,cur);r.nodesVisited++;
        if(cur==end){r.found=true;break;}
        get_neighbors(grid,gw,gh,cur,nb,&nc);
        for(int i=0;i<nc;i++){
            if(nb[i]->state==STATE_WALL||nb[i]->visited)continue;
            nb[i]->parent=cur;nl_push(&stack,nb[i]);
        }
    }
    r.timeMs=(double)(clock()-t0)/CLOCKS_PER_SEC*1000.0;
    if(r.found)trace_path(end,start,&r.path);
    nl_free(&stack);return r;
}

static PathResult run_greedy(Node *grid,int gw,int gh,Node *start,Node *end,HeuristicType ht){
    PathResult r;memset(&r,0,sizeof(r));nl_init(&r.closed);nl_init(&r.path);
    reset_path_state(grid,gw,gh);
    clock_t t0=clock();
    NodeList open;nl_init(&open);
    start->fCost=(int)heuristic(start,end,ht);nl_push(&open,start);
    Node *nb[4];int nc;
    while(open.size){
        Node *cur=nl_pop_lowest_f(&open);
        if(cur->visited)continue;
        cur->visited=true;nl_push(&r.closed,cur);r.nodesVisited++;
        if(cur==end){r.found=true;break;}
        get_neighbors(grid,gw,gh,cur,nb,&nc);
        for(int i=0;i<nc;i++){
            if(nb[i]->state==STATE_WALL||nb[i]->visited)continue;
            nb[i]->fCost=(int)heuristic(nb[i],end,ht);nb[i]->parent=cur;nl_push(&open,nb[i]);
        }
    }
    r.timeMs=(double)(clock()-t0)/CLOCKS_PER_SEC*1000.0;
    if(r.found)trace_path(end,start,&r.path);
    nl_free(&open);return r;
}

static PathResult run_algorithm(Node *grid,int gw,int gh,Node *start,Node *end,AlgoType algo,HeuristicType ht){
    switch(algo){
        case ALGO_ASTAR:    return run_astar(grid,gw,gh,start,end,ht);
        case ALGO_DIJKSTRA: return run_dijkstra(grid,gw,gh,start,end);
        case ALGO_BFS:      return run_bfs(grid,gw,gh,start,end);
        case ALGO_DFS:      return run_dfs(grid,gw,gh,start,end);
        case ALGO_GREEDY:   return run_greedy(grid,gw,gh,start,end,ht);
        default:            {PathResult r;memset(&r,0,sizeof(r));return r;}
    }
}

#endif