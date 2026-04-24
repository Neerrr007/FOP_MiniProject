#ifndef NODE_H
#define NODE_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    STATE_EMPTY=0, STATE_WALL, STATE_START, STATE_END,
    STATE_OPEN, STATE_CLOSED, STATE_PATH
} State;

typedef struct Node {
    int         x, y;
    int         nodeWidth, nodeHeight;
    int         gCost, hCost, fCost;
    struct Node *parent;
    State       state;
    bool        visited;
} Node;

typedef struct { unsigned char r,g,b; } NodeColor;

static inline Node node_create(int px, int py, int nw, int nh, State s) {
    Node n; memset(&n,0,sizeof(n));
    n.x=px/nw; n.y=py/nh;
    n.nodeWidth=nw; n.nodeHeight=nh;
    n.state=s;
    return n;
}

static inline NodeColor node_get_color(State s) {
    switch(s){
        case STATE_EMPTY:   return (NodeColor){242,242,242};
        case STATE_WALL:    return (NodeColor){30,33,36};
        case STATE_START:   return (NodeColor){99,249,0};
        case STATE_END:     return (NodeColor){254,41,27};
        case STATE_PATH:    return (NodeColor){255,199,0};
        case STATE_CLOSED:  return (NodeColor){180,180,220};
        case STATE_OPEN:    return (NodeColor){147,197,253};
        default:            return (NodeColor){200,200,200};
    }
}

static inline bool node_contains(Node *n, int px, int py) {
    int rx=n->x*n->nodeWidth, ry=n->y*n->nodeHeight;
    return px>=rx&&px<rx+n->nodeWidth&&py>=ry&&py<ry+n->nodeHeight;
}

#endif