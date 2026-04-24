#ifndef GRID_H
#define GRID_H

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "node.h"
#include "algorithm.h"

/* ── Split-screen: each side has its own animation state ── */
typedef struct {
    PathResult result;
    bool       ready;
    int        closedIdx;
    int        pathIdx;
    int        phase;       /* 0=idle 1=closed 2=path 3=done */
    AlgoType   algo;
    HeuristicType heuristic;
    int        nodesVisited;
    int        pathLength;
    double     timeMs;
    /* deep-copied grid for independent animation */
    Node      *grid;
} GridSide;

typedef struct {
    int        gridWidth, gridHeight;
    int        nodeWidth, nodeHeight;
    Node      *grid;        /* master grid (walls/start/end) */
    int      **maze;
    Node      *start;
    Node      *end;
    Node      *lastSelectedNode;
    State      selectedState;
    bool       isRunning;

    /* single-algo mode */
    PathResult singleResult;
    bool       singleReady;
    int        singleClosedIdx;
    int        singlePathIdx;
    int        singlePhase;
    AlgoType      currentAlgo;
    HeuristicType currentHeuristic;
    int        pathLength;
    int        nodesVisited;
    double     timeMs;

    /* split-screen mode */
    bool       splitMode;
    GridSide   left;
    GridSide   right;
} Grid;

#define grid_node(g,i,j) (&(g)->grid[(i)*(g)->gridHeight+(j)])

/* ── Helpers ── */
static Node *find_node_in(Node *grid, int gh, Node *master_node, int gw) {
    /* find equivalent node in a copied grid by coords */
    int x=master_node->x, y=master_node->y;
    if(x<0||x>=gw||y<0||y>=gh) return &grid[0];
    return &grid[x*gh+y];
}

static void copy_grid_walls(Node *dst, Node *src, int gw, int gh) {
    for(int i=0;i<gw*gh;i++){
        dst[i]=src[i];
        dst[i].parent=NULL;
        dst[i].visited=false;
        dst[i].gCost=dst[i].hCost=dst[i].fCost=0;
    }
}

static void grid_set_start_and_end(Grid *g){
    g->start=grid_node(g,1,1); g->start->state=STATE_START;
    g->end=grid_node(g,g->gridWidth-2,g->gridHeight-2); g->end->state=STATE_END;
}

static void gridside_free(GridSide *s){
    result_free(&s->result);
    free(s->grid); s->grid=NULL;
}

static void grid_init(Grid *g, int w, int h, int nw, int nh){
    memset(g,0,sizeof(Grid));
    g->gridWidth=w; g->gridHeight=h; g->nodeWidth=nw; g->nodeHeight=nh;
    g->selectedState=STATE_START;
    g->currentAlgo=ALGO_ASTAR; g->currentHeuristic=HEURISTIC_MANHATTAN;
    g->grid=(Node*)malloc(sizeof(Node)*w*h);
    g->maze=(int**)malloc(sizeof(int*)*w);
    for(int i=0;i<w;i++) g->maze[i]=(int*)calloc(h,sizeof(int));
    for(int i=0;i<w;i++)
        for(int j=0;j<h;j++)
            *grid_node(g,i,j)=node_create(i*nw,j*nh,nw,nh,STATE_EMPTY);
    grid_set_start_and_end(g);
    srand((unsigned int)time(NULL));
}

static void grid_clear_search(Grid *g){
    for(int i=0;i<g->gridWidth;i++)
        for(int j=0;j<g->gridHeight;j++){
            State s=grid_node(g,i,j)->state;
            if(s==STATE_CLOSED||s==STATE_PATH||s==STATE_OPEN)
                grid_node(g,i,j)->state=STATE_EMPTY;
        }
    if(g->singleReady){result_free(&g->singleResult);g->singleReady=false;}
    if(g->splitMode){
        gridside_free(&g->left); gridside_free(&g->right);
        g->splitMode=false;
    }
    g->singlePhase=0; g->isRunning=false;
}

static void grid_reset(Grid *g){
    for(int i=0;i<g->gridWidth;i++)
        for(int j=0;j<g->gridHeight;j++)
            grid_node(g,i,j)->state=STATE_EMPTY;
    grid_set_start_and_end(g);
    g->pathLength=0;g->nodesVisited=0;
    g->singlePhase=0;g->isRunning=false;g->splitMode=false;
    if(g->singleReady){result_free(&g->singleResult);g->singleReady=false;}
    gridside_free(&g->left); gridside_free(&g->right);
}

static void grid_handle_click(Grid *g, int mx, int my, int button){
    if(g->isRunning) return;
    for(int i=0;i<g->gridWidth;i++)
        for(int j=0;j<g->gridHeight;j++){
            Node *n=grid_node(g,i,j);
            if(node_contains(n,mx,my)){
                grid_clear_search(g);
                if(g->selectedState==STATE_START){g->start->state=STATE_EMPTY;g->start=n;n->state=STATE_START;}
                else if(g->selectedState==STATE_END){g->end->state=STATE_EMPTY;g->end=n;n->state=STATE_END;}
                else if(g->selectedState==STATE_WALL&&n!=g->lastSelectedNode){
                    if(button==SDL_BUTTON_LEFT&&n->state==STATE_EMPTY) n->state=STATE_WALL;
                    else if(n->state==STATE_WALL) n->state=STATE_EMPTY;
                    g->lastSelectedNode=n;
                }
                return;
            }
        }
}

static void grid_handle_drag(Grid *g,int mx,int my,Uint32 bs){
    if(g->isRunning)return;
    grid_handle_click(g,mx,my,(bs&SDL_BUTTON_LMASK)?SDL_BUTTON_LEFT:SDL_BUTTON_RIGHT);
}

/* ── Maze ── */
static void shuffle_dirs(int *d,int n){for(int i=n-1;i>0;i--){int j=rand()%(i+1),t=d[i];d[i]=d[j];d[j]=t;}}
typedef struct{int row,col;}MazeCell;

static void maze_carve(Grid *g,int sr,int sc){
    MazeCell *stack=(MazeCell*)malloc(sizeof(MazeCell)*g->gridWidth*g->gridHeight);
    int top=0; stack[top].row=sr;stack[top].col=sc;top++;
    while(top>0){
        int row=stack[top-1].row,col=stack[top-1].col;
        int dirs[]={1,2,3,4}; shuffle_dirs(dirs,4); bool moved=false;
        for(int d=0;d<4;d++){
            switch(dirs[d]){
                case 1:if(row-2<=0)continue;if(g->maze[row-2][col]!=0){g->maze[row-2][col]=0;g->maze[row-1][col]=0;stack[top].row=row-2;stack[top].col=col;top++;moved=true;}break;
                case 2:if(col+2>=g->gridHeight-1)continue;if(g->maze[row][col+2]!=0){g->maze[row][col+2]=0;g->maze[row][col+1]=0;stack[top].row=row;stack[top].col=col+2;top++;moved=true;}break;
                case 3:if(row+2>=g->gridWidth-1)continue;if(g->maze[row+2][col]!=0){g->maze[row+2][col]=0;g->maze[row+1][col]=0;stack[top].row=row+2;stack[top].col=col;top++;moved=true;}break;
                case 4:if(col-2<=0)continue;if(g->maze[row][col-2]!=0){g->maze[row][col-2]=0;g->maze[row][col-1]=0;stack[top].row=row;stack[top].col=col-2;top++;moved=true;}break;
            }
            if(moved)break;
        }
        if(!moved)top--;
    }
    free(stack);
}

static void grid_generate_maze(Grid *g){
    if(g->isRunning)return;
    g->isRunning=true; grid_reset(g);
    for(int i=0;i<g->gridWidth;i++) for(int j=0;j<g->gridHeight;j++) g->maze[i][j]=1;
    int rr=rand()%g->gridWidth; while(rr%2==0)rr=rand()%g->gridWidth;
    int rc=rand()%g->gridHeight; while(rc%2==0)rc=rand()%g->gridHeight;
    g->maze[rr][rc]=0; maze_carve(g,rr,rc);
    for(int i=0;i<g->gridWidth;i++)
        for(int j=0;j<g->gridHeight;j++)
            if(g->maze[i][j]==1) grid_node(g,i,j)->state=STATE_WALL;
    grid_set_start_and_end(g);
    g->isRunning=false;
}

/* ── Single-algo run ── */
static void grid_find_path(Grid *g){
    if(g->isRunning)return;
    g->isRunning=true; g->splitMode=false;
    grid_clear_search(g);
    g->singleResult=run_algorithm(g->grid,g->gridWidth,g->gridHeight,
                                   g->start,g->end,g->currentAlgo,g->currentHeuristic);
    g->singleReady=true;
    g->pathLength=g->singleResult.path.size;
    g->nodesVisited=g->singleResult.nodesVisited;
    g->timeMs=g->singleResult.timeMs;
    g->singleClosedIdx=0; g->singlePathIdx=0; g->singlePhase=1;
}

/* ── Split-screen run ── */
static void grid_find_path_split(Grid *g, AlgoType algoL, HeuristicType htL,
                                            AlgoType algoR, HeuristicType htR){
    if(g->isRunning)return;
    g->isRunning=true; g->splitMode=true;
    grid_clear_search(g);

    int sz=g->gridWidth*g->gridHeight;

    /* left side */
    gridside_free(&g->left);
    g->left.grid=(Node*)malloc(sizeof(Node)*sz);
    copy_grid_walls(g->left.grid,g->grid,g->gridWidth,g->gridHeight);
    g->left.algo=algoL; g->left.heuristic=htL;
    Node *ls=find_node_in(g->left.grid,g->gridHeight,g->start,g->gridWidth);
    Node *le=find_node_in(g->left.grid,g->gridHeight,g->end,g->gridWidth);
    g->left.result=run_algorithm(g->left.grid,g->gridWidth,g->gridHeight,ls,le,algoL,htL);
    g->left.ready=true; g->left.closedIdx=0; g->left.pathIdx=0; g->left.phase=1;
    g->left.nodesVisited=g->left.result.nodesVisited;
    g->left.pathLength=g->left.result.path.size;
    g->left.timeMs=g->left.result.timeMs;

    /* right side */
    gridside_free(&g->right);
    g->right.grid=(Node*)malloc(sizeof(Node)*sz);
    copy_grid_walls(g->right.grid,g->grid,g->gridWidth,g->gridHeight);
    g->right.algo=algoR; g->right.heuristic=htR;
    Node *rs=find_node_in(g->right.grid,g->gridHeight,g->start,g->gridWidth);
    Node *re=find_node_in(g->right.grid,g->gridHeight,g->end,g->gridWidth);
    g->right.result=run_algorithm(g->right.grid,g->gridWidth,g->gridHeight,rs,re,algoR,htR);
    g->right.ready=true; g->right.closedIdx=0; g->right.pathIdx=0; g->right.phase=1;
    g->right.nodesVisited=g->right.result.nodesVisited;
    g->right.pathLength=g->right.result.path.size;
    g->right.timeMs=g->right.result.timeMs;
}

/* ── Animation step for one side ── */
static bool step_side(GridSide *s, int speed){
    if(s->phase==1){
        for(int i=0;i<speed&&s->closedIdx<s->result.closed.size;i++){
            Node *n=s->result.closed.data[s->closedIdx++];
            if(n->state!=STATE_START&&n->state!=STATE_END) n->state=STATE_CLOSED;
        }
        if(s->closedIdx>=s->result.closed.size)
            s->phase=s->result.found?2:3;
    } else if(s->phase==2){
        if(s->pathIdx<s->result.path.size)
            s->result.path.data[s->pathIdx++]->state=STATE_PATH;
        else s->phase=3;
    }
    return s->phase==3;
}

/* ── Master step ── */
static void grid_step(Grid *g){
    if(!g->isRunning)return;
    if(g->splitMode){
        bool ld=(g->left.phase==3), rd=(g->right.phase==3);
        if(!ld) ld=step_side(&g->left,3);
        if(!rd) rd=step_side(&g->right,3);
        if(ld&&rd){g->isRunning=false;}
    } else {
        if(g->singlePhase==1){
            for(int i=0;i<3&&g->singleClosedIdx<g->singleResult.closed.size;i++){
                Node *n=g->singleResult.closed.data[g->singleClosedIdx++];
                if(n->state!=STATE_START&&n->state!=STATE_END) n->state=STATE_CLOSED;
            }
            if(g->singleClosedIdx>=g->singleResult.closed.size){
                /* instantly mark all path nodes so they're visible even if animation is fast */
                for(int i=0;i<g->singleResult.path.size;i++){
                    Node *n=g->singleResult.path.data[i];
                    if(n->state!=STATE_START&&n->state!=STATE_END) n->state=STATE_PATH;
                }
                g->singlePhase=g->singleResult.found?3:3;
            }
        } else if(g->singlePhase==2){
            for(int i=0;i<2&&g->singlePathIdx<g->singleResult.path.size;i++){
                Node *n=g->singleResult.path.data[g->singlePathIdx++];
                if(n->state!=STATE_START&&n->state!=STATE_END)
                    n->state=STATE_PATH;
            }
            if(g->singlePathIdx>=g->singleResult.path.size) g->singlePhase=3;
        } else if(g->singlePhase==3){
            g->isRunning=false; g->singlePhase=0;
        }
    }
}

/* ── Draw one grid panel ── */
static void draw_grid_panel(SDL_Renderer *ren, Node *grid, int gw, int gh,
                             int offX, int offY, int nw, int nh, int panelW){
    for(int i=0;i<gw;i++){
        for(int j=0;j<gh;j++){
            Node *n=&grid[i*gh+j];
            NodeColor c=node_get_color(n->state);
            SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,255);
            SDL_Rect r={offX+i*nw+1,offY+j*nh+1,nw-2,nh-2};
            SDL_RenderFillRect(ren,&r);
            if(n->state!=STATE_WALL){
                SDL_SetRenderDrawColor(ren,178,178,178,255);
                SDL_Rect b={offX+i*nw,offY+j*nh,nw,nh};
                SDL_RenderDrawRect(ren,&b);
            }
        }
    }
    /* panel border */
    SDL_SetRenderDrawColor(ren,80,80,80,255);
    SDL_Rect border={offX,offY,panelW,gh*nh};
    SDL_RenderDrawRect(ren,&border);
}

static void grid_draw(Grid *g, SDL_Renderer *ren, int gridAreaW){
    if(g->splitMode){
        int half=gridAreaW/2;
        int nw=half/g->gridWidth, nh=g->nodeHeight;
        draw_grid_panel(ren,g->left.grid,g->gridWidth,g->gridHeight,0,0,nw,nh,half);
        draw_grid_panel(ren,g->right.grid,g->gridWidth,g->gridHeight,half,0,nw,nh,half);
    } else {
        draw_grid_panel(ren,g->grid,g->gridWidth,g->gridHeight,0,0,
                        g->nodeWidth,g->nodeHeight,gridAreaW);
    }
}

static void grid_free(Grid *g){
    free(g->grid);
    for(int i=0;i<g->gridWidth;i++) free(g->maze[i]);
    free(g->maze);
    if(g->singleReady) result_free(&g->singleResult);
    gridside_free(&g->left); gridside_free(&g->right);
}

#endif
