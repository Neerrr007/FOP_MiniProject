#ifndef MAIN_H
#define MAIN_H

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "node.h"
#include "grid.h"

/* ── Layout ── */
#define WINDOW_WIDTH    1000
#define WINDOW_HEIGHT    650
#define SIDE_W           200
#define GRID_AREA_W     (WINDOW_WIDTH - SIDE_W)
#define NODE_W            20
#define NODE_H            20
#define GRID_W          (GRID_AREA_W / NODE_W)
#define GRID_H          (WINDOW_HEIGHT / NODE_H)
#define BTN_X           (GRID_AREA_W + 10)
#define BTN_W           (SIDE_W - 20)
#define BTN_H            28
#define BTN_GAP           6

/* ── App modes ── */
typedef enum { MODE_NORMAL=0, MODE_COMPARE_SETUP, MODE_HEURISTIC_SETUP } AppMode;

/* ── Button ── */
typedef struct {
    SDL_Rect rect;
    char     label[48];
    bool     selected, hovered, toggled;
} Btn;

static void btn_init(Btn *b, int x, int y, int w, int h, const char *lbl){
    b->rect=(SDL_Rect){x,y,w,h};
    strncpy(b->label,lbl,47);
    b->selected=b->hovered=b->toggled=false;
}

static bool pt_in(int x,int y,SDL_Rect *r){return x>=r->x&&x<r->x+r->w&&y>=r->y&&y<r->y+r->h;}

/* ── Text helpers ── */
static void draw_text(SDL_Renderer *ren, TTF_Font *font, const char *txt,
                      int x, int y, SDL_Color col){
    SDL_Surface *s=TTF_RenderText_Blended(font,txt,col);
    if(!s)return;
    SDL_Texture *t=SDL_CreateTextureFromSurface(ren,s);
    if(t){SDL_Rect d={x,y,s->w,s->h};SDL_RenderCopy(ren,t,NULL,&d);SDL_DestroyTexture(t);}
    SDL_FreeSurface(s);
}

static void draw_text_centered(SDL_Renderer *ren, TTF_Font *font, const char *txt,
                                SDL_Rect *rect, SDL_Color col){
    SDL_Surface *s=TTF_RenderText_Blended(font,txt,col);
    if(!s)return;
    SDL_Texture *t=SDL_CreateTextureFromSurface(ren,s);
    if(t){
        SDL_Rect d={rect->x+(rect->w-s->w)/2,rect->y+(rect->h-s->h)/2,s->w,s->h};
        SDL_RenderCopy(ren,t,NULL,&d);SDL_DestroyTexture(t);
    }
    SDL_FreeSurface(s);
}

static void draw_btn(SDL_Renderer *ren, TTF_Font *font, Btn *b){
    SDL_Color fg={0,0,0,255};
    if(b->toggled)     SDL_SetRenderDrawColor(ren,99,249,0,255);
    else if(b->hovered)SDL_SetRenderDrawColor(ren,200,200,200,255);
    else               SDL_SetRenderDrawColor(ren,255,255,255,255);
    SDL_RenderFillRect(ren,&b->rect);
    if(b->selected){
        SDL_SetRenderDrawColor(ren,255,165,0,255);
        for(int t=0;t<2;t++){SDL_Rect br={b->rect.x-t,b->rect.y-t,b->rect.w+2*t,b->rect.h+2*t};SDL_RenderDrawRect(ren,&br);}
    }
    draw_text_centered(ren,font,b->label,&b->rect,fg);
}

static void draw_divider(SDL_Renderer *ren, TTF_Font *font, const char *lbl, int y){
    SDL_SetRenderDrawColor(ren,100,100,100,255);
    SDL_RenderDrawLine(ren,BTN_X,y,BTN_X+BTN_W,y);
    SDL_Color c={180,180,180,255};
    draw_text(ren,font,lbl,BTN_X,y+2,c);
}

/* ────────────────────────────────────────────────────────────────────────────
   POPUP SYSTEM
   ──────────────────────────────────────────────────────────────────────────── */
typedef struct {
    bool  visible;
    char  lines[20][128];
    int   lineCount;
    char  title[64];
} Popup;

static void popup_show(Popup *p, const char *title, const char **lines, int n){
    p->visible=true;
    strncpy(p->title,title,63);
    p->lineCount=n<20?n:20;
    for(int i=0;i<p->lineCount;i++) strncpy(p->lines[i],lines[i],127);
}

static void popup_draw(SDL_Renderer *ren, TTF_Font *font, TTF_Font *bigFont, Popup *p){
    if(!p->visible)return;
    int pw=560,ph=40+p->lineCount*22+20;
    int px=(WINDOW_WIDTH-pw)/2, py=(WINDOW_HEIGHT-ph)/2;

    /* dim background */
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren,0,0,0,160);
    SDL_Rect bg={0,0,WINDOW_WIDTH,WINDOW_HEIGHT};SDL_RenderFillRect(ren,&bg);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);

    /* box */
    SDL_SetRenderDrawColor(ren,30,33,36,255);
    SDL_Rect box={px,py,pw,ph};SDL_RenderFillRect(ren,&box);
    SDL_SetRenderDrawColor(ren,255,165,0,255);SDL_RenderDrawRect(ren,&box);

    /* title */
    SDL_Color white={255,255,255,255};
    draw_text(ren,bigFont,p->title,px+14,py+8,white);

    /* lines */
    SDL_Color grey={200,200,200,255};
    for(int i=0;i<p->lineCount;i++)
        draw_text(ren,font,p->lines[i],px+14,py+38+i*22,grey);

    /* close hint */
    SDL_Color dim={120,120,120,255};
    draw_text(ren,font,"[Click anywhere or press ESC to close]",px+14,py+ph-18,dim);
}

/* ── Stats popup ── */
static void show_stats_popup(Popup *p, Grid *g){
    char title[64]; snprintf(title,63,"Results — %s (%s)",
        algo_names[g->currentAlgo],
        g->currentHeuristic==HEURISTIC_MANHATTAN?"Manhattan":"Euclidean");
    const char *lines[8];
    char l0[128],l1[128],l2[128],l3[128],l4[128],l5[128],l6[128];
    snprintf(l0,127,"Algorithm  : %s",algo_names[g->currentAlgo]);
    snprintf(l1,127,"Heuristic  : %s",g->currentHeuristic==HEURISTIC_MANHATTAN?"Manhattan":"Euclidean");
    snprintf(l2,127,"Path found : %s",g->singleResult.found?"Yes":"No");
    snprintf(l3,127,"Path length: %d nodes",g->pathLength);
    snprintf(l4,127,"Nodes visited: %d",g->nodesVisited);
    snprintf(l5,127,"Time taken : %.3f ms",g->timeMs);
    snprintf(l6,127,"%s",algo_descriptions[g->currentAlgo]);
    lines[0]=l0;lines[1]=l1;lines[2]=l2;lines[3]=l3;lines[4]=l4;lines[5]=l5;
    lines[6]="";lines[7]=l6;
    popup_show(p,title,lines,8);
}

/* ── Compare stats popup ── */
static void show_compare_popup(Popup *p, Grid *g){
    static char ls[16][128];
    int n=0;
    snprintf(ls[n++],127,"%-18s %-12s %-12s %-10s","Algorithm","Path Len","Visited","Time(ms)");
    snprintf(ls[n++],127,"%-18s %-12s %-12s %-10s","------------------","--------","-------","-------");
    /* left */
    snprintf(ls[n++],127,"LEFT: %-12s [%s]",algo_names[g->left.algo],
             g->left.heuristic==HEURISTIC_MANHATTAN?"Manh":"Eucl");
    snprintf(ls[n++],127,"  Path: %-6d  Visited: %-6d  Time: %.3f ms",
             g->left.pathLength, g->left.nodesVisited, g->left.timeMs);
    snprintf(ls[n++],127,"  %s",g->left.result.found?"Path found":"No path found");
    ls[n][0]=0;n++;
    /* right */
    snprintf(ls[n++],127,"RIGHT: %-12s [%s]",algo_names[g->right.algo],
             g->right.heuristic==HEURISTIC_MANHATTAN?"Manh":"Eucl");
    snprintf(ls[n++],127,"  Path: %-6d  Visited: %-6d  Time: %.3f ms",
             g->right.pathLength, g->right.nodesVisited, g->right.timeMs);
    snprintf(ls[n++],127,"  %s",g->right.result.found?"Path found":"No path found");
    ls[n][0]=0;n++;
    /* verdict */
    if(g->left.result.found&&g->right.result.found){
        if(g->left.nodesVisited<g->right.nodesVisited)
            snprintf(ls[n++],127,"LEFT explored fewer nodes (%d vs %d)",g->left.nodesVisited,g->right.nodesVisited);
        else if(g->right.nodesVisited<g->left.nodesVisited)
            snprintf(ls[n++],127,"RIGHT explored fewer nodes (%d vs %d)",g->right.nodesVisited,g->left.nodesVisited);
        else
            snprintf(ls[n++],127,"Both explored same number of nodes");
    }
    const char *ptrs[16];
    for(int i=0;i<n;i++)ptrs[i]=ls[i];
    popup_show(p,"Split-Screen Comparison Results",ptrs,n);
}

/* ────────────────────────────────────────────────────────────────────────────
   SIDE PANEL UI
   ──────────────────────────────────────────────────────────────────────────── */
typedef struct {
    /* Node type */
    Btn nodeBtn[3];  /* Start End Wall */
    int selectedNode;

    /* Algorithm selector */
    Btn algoBtn[ALGO_COUNT];
    int selectedAlgo;

    /* Heuristic toggle */
    Btn heuristicBtn[2]; /* Manhattan Euclidean */
    int selectedHeuristic;

    /* Actions */
    Btn btnVisualize;
    Btn btnMaze;
    Btn btnClear;
    Btn btnReset;
    Btn btnStats;

    /* Split compare */
    Btn btnCompare;
    Btn btnHCompare;  /* heuristic compare */

    /* Compare setup: left/right picks */
    Btn leftAlgoBtn[ALGO_COUNT];
    Btn rightAlgoBtn[ALGO_COUNT];
    Btn leftHBtn[2];
    Btn rightHBtn[2];
    Btn btnRunCompare;
    int leftAlgo, rightAlgo;
    int leftH, rightH;

    AppMode mode;
} UI;

static void ui_build(UI *ui){
    memset(ui,0,sizeof(UI));
    ui->selectedNode=2; /* wall */
    ui->selectedAlgo=ALGO_ASTAR;
    ui->selectedHeuristic=HEURISTIC_MANHATTAN;
    ui->leftAlgo=ALGO_ASTAR; ui->rightAlgo=ALGO_BFS;
    ui->leftH=HEURISTIC_MANHATTAN; ui->rightH=HEURISTIC_MANHATTAN;
    ui->mode=MODE_NORMAL;

    int y=8;
    /* Node buttons */
    const char *nnames[]={"Start","End","Wall"};
    for(int i=0;i<3;i++){btn_init(&ui->nodeBtn[i],BTN_X,y,BTN_W/3-2,BTN_H,nnames[i]);ui->nodeBtn[i].rect.x=BTN_X+i*(BTN_W/3+1);y+=0;}
    ui->nodeBtn[ui->selectedNode].selected=true; y+=BTN_H+BTN_GAP+4;

    /* Algorithm buttons */
    y+=14;
    for(int i=0;i<ALGO_COUNT;i++){btn_init(&ui->algoBtn[i],BTN_X,y,BTN_W,BTN_H,algo_names[i]);y+=BTN_H+BTN_GAP;}
    ui->algoBtn[ui->selectedAlgo].selected=true; y+=4;

    /* Heuristic */
    y+=14;
    btn_init(&ui->heuristicBtn[0],BTN_X,y,BTN_W/2-2,BTN_H,"Manhattan");
    btn_init(&ui->heuristicBtn[1],BTN_X+BTN_W/2+2,y,BTN_W/2-2,BTN_H,"Euclidean");
    ui->heuristicBtn[0].selected=true; y+=BTN_H+BTN_GAP+4;

    /* Actions */
    y+=14;
    btn_init(&ui->btnVisualize,BTN_X,y,BTN_W,BTN_H,"Visualize"); y+=BTN_H+BTN_GAP;
    btn_init(&ui->btnMaze,BTN_X,y,BTN_W,BTN_H,"Generate Maze"); y+=BTN_H+BTN_GAP;
    btn_init(&ui->btnClear,BTN_X,y,BTN_W/2-2,BTN_H,"Clear");
    btn_init(&ui->btnReset,BTN_X+BTN_W/2+2,y,BTN_W/2-2,BTN_H,"Reset"); y+=BTN_H+BTN_GAP;
    btn_init(&ui->btnStats,BTN_X,y,BTN_W,BTN_H,"Show Stats"); y+=BTN_H+BTN_GAP+4;

    /* Compare */
    y+=14;
    btn_init(&ui->btnCompare,BTN_X,y,BTN_W,BTN_H,"Algo Compare (Split)"); y+=BTN_H+BTN_GAP;
    btn_init(&ui->btnHCompare,BTN_X,y,BTN_W,BTN_H,"Heuristic Compare"); y+=BTN_H+BTN_GAP;
}

/* Build compare-setup panel buttons */
static void build_compare_panel(UI *ui, bool heuristicMode){
    int y=8;
    const char *nnames[]={"Start","End","Wall"};
    for(int i=0;i<3;i++){btn_init(&ui->nodeBtn[i],BTN_X,y,BTN_W/3-2,BTN_H,nnames[i]);ui->nodeBtn[i].rect.x=BTN_X+i*(BTN_W/3+1);}
    y+=BTN_H+BTN_GAP+4;

    if(!heuristicMode){
        /* left algo */
        y+=14;
        for(int i=0;i<ALGO_COUNT;i++){
            char lbl[32]; snprintf(lbl,31,"L: %s",algo_names[i]);
            btn_init(&ui->leftAlgoBtn[i],BTN_X,y,BTN_W,BTN_H,lbl);y+=BTN_H+BTN_GAP;
        }
        ui->leftAlgoBtn[ui->leftAlgo].selected=true; y+=4;
        /* right algo */
        y+=14;
        for(int i=0;i<ALGO_COUNT;i++){
            char lbl[32]; snprintf(lbl,31,"R: %s",algo_names[i]);
            btn_init(&ui->rightAlgoBtn[i],BTN_X,y,BTN_W,BTN_H,lbl);y+=BTN_H+BTN_GAP;
        }
        ui->rightAlgoBtn[ui->rightAlgo].selected=true; y+=8;
    } else {
        /* Both sides use currentAlgo but different heuristics */
        y+=14;
        btn_init(&ui->leftHBtn[0],BTN_X,y,BTN_W/2-2,BTN_H,"L: Manhattan");
        btn_init(&ui->leftHBtn[1],BTN_X+BTN_W/2+2,y,BTN_W/2-2,BTN_H,"L: Euclidean");
        ui->leftHBtn[ui->leftH].selected=true; y+=BTN_H+BTN_GAP+4;
        btn_init(&ui->rightHBtn[0],BTN_X,y,BTN_W/2-2,BTN_H,"R: Manhattan");
        btn_init(&ui->rightHBtn[1],BTN_X+BTN_W/2+2,y,BTN_W/2-2,BTN_H,"R: Euclidean");
        ui->rightHBtn[ui->rightH].selected=true; y+=BTN_H+BTN_GAP+8;
    }

    btn_init(&ui->btnRunCompare,BTN_X,y,BTN_W,BTN_H,"Run Comparison"); y+=BTN_H+BTN_GAP;
    btn_init(&ui->btnReset,BTN_X,y,BTN_W/2-2,BTN_H,"Reset");
    btn_init(&ui->btnMaze,BTN_X+BTN_W/2+2,y,BTN_W/2-2,BTN_H,"Maze");
    /* back button reusing btnClear slot */
    y+=BTN_H+BTN_GAP;
    btn_init(&ui->btnClear,BTN_X,y,BTN_W,BTN_H,"<< Back");
}

static void ui_draw_panel(SDL_Renderer *ren, TTF_Font *font, TTF_Font *smallFont,
                           UI *ui, Grid *g){
    /* background */
    SDL_SetRenderDrawColor(ren,30,33,36,255);
    SDL_Rect panel={GRID_AREA_W,0,SIDE_W,WINDOW_HEIGHT};
    SDL_RenderFillRect(ren,&panel);

    SDL_Color white={255,255,255,255};
    SDL_Color orange={255,165,0,255};
    SDL_Color grey={150,150,150,255};

    if(ui->mode==MODE_NORMAL){
        draw_text(ren,smallFont,"Node Type",BTN_X,2,grey);
        for(int i=0;i<3;i++) draw_btn(ren,font,&ui->nodeBtn[i]);
        draw_divider(ren,smallFont,"Algorithm",ui->nodeBtn[0].rect.y+BTN_H+BTN_GAP+2);
        for(int i=0;i<ALGO_COUNT;i++) draw_btn(ren,font,&ui->algoBtn[i]);
        draw_divider(ren,smallFont,"Heuristic",ui->algoBtn[ALGO_COUNT-1].rect.y+BTN_H+BTN_GAP+2);
        for(int i=0;i<2;i++) draw_btn(ren,font,&ui->heuristicBtn[i]);
        draw_divider(ren,smallFont,"Actions",ui->heuristicBtn[0].rect.y+BTN_H+BTN_GAP+2);
        draw_btn(ren,font,&ui->btnVisualize);
        draw_btn(ren,font,&ui->btnMaze);
        draw_btn(ren,font,&ui->btnClear);
        draw_btn(ren,font,&ui->btnReset);
        draw_btn(ren,font,&ui->btnStats);
        draw_divider(ren,smallFont,"Compare",ui->btnStats.rect.y+BTN_H+BTN_GAP+2);
        draw_btn(ren,font,&ui->btnCompare);
        draw_btn(ren,font,&ui->btnHCompare);

        /* live stats */
        int sy=ui->btnHCompare.rect.y+BTN_H+BTN_GAP+12;
        char buf[48];
        snprintf(buf,47,"Path: %d",g->pathLength); draw_text(ren,font,buf,BTN_X,sy,white); sy+=18;
        snprintf(buf,47,"Visited: %d",g->nodesVisited); draw_text(ren,font,buf,BTN_X,sy,white); sy+=18;
        snprintf(buf,47,"%.3f ms",g->timeMs); draw_text(ren,font,buf,BTN_X,sy,orange);

    } else if(ui->mode==MODE_COMPARE_SETUP){
        draw_text(ren,font,"ALGO COMPARE",BTN_X,2,orange);
        draw_text(ren,smallFont,"Node Type",BTN_X,22,grey);
        for(int i=0;i<3;i++) draw_btn(ren,font,&ui->nodeBtn[i]);
        draw_divider(ren,smallFont,"Left Algorithm",ui->nodeBtn[0].rect.y+BTN_H+BTN_GAP+2);
        for(int i=0;i<ALGO_COUNT;i++) draw_btn(ren,font,&ui->leftAlgoBtn[i]);
        draw_divider(ren,smallFont,"Right Algorithm",ui->leftAlgoBtn[ALGO_COUNT-1].rect.y+BTN_H+BTN_GAP+2);
        for(int i=0;i<ALGO_COUNT;i++) draw_btn(ren,font,&ui->rightAlgoBtn[i]);
        draw_btn(ren,font,&ui->btnRunCompare);
        draw_btn(ren,font,&ui->btnReset);
        draw_btn(ren,font,&ui->btnMaze);
        draw_btn(ren,font,&ui->btnClear);

    } else if(ui->mode==MODE_HEURISTIC_SETUP){
        draw_text(ren,font,"HEURISTIC COMPARE",BTN_X,2,orange);
        draw_text(ren,smallFont,"Node Type",BTN_X,22,grey);
        for(int i=0;i<3;i++) draw_btn(ren,font,&ui->nodeBtn[i]);
        draw_divider(ren,smallFont,"Left Heuristic",ui->nodeBtn[0].rect.y+BTN_H+BTN_GAP+2);
        for(int i=0;i<2;i++) draw_btn(ren,font,&ui->leftHBtn[i]);
        draw_divider(ren,smallFont,"Right Heuristic",ui->leftHBtn[1].rect.y+BTN_H+BTN_GAP+2);
        for(int i=0;i<2;i++) draw_btn(ren,font,&ui->rightHBtn[i]);
        draw_btn(ren,font,&ui->btnRunCompare);
        draw_btn(ren,font,&ui->btnReset);
        draw_btn(ren,font,&ui->btnMaze);
        draw_btn(ren,font,&ui->btnClear);
    }
}

static void update_hovers(UI *ui, int mx, int my){
    for(int i=0;i<3;i++) ui->nodeBtn[i].hovered=pt_in(mx,my,&ui->nodeBtn[i].rect);
    for(int i=0;i<ALGO_COUNT;i++){
        ui->algoBtn[i].hovered=pt_in(mx,my,&ui->algoBtn[i].rect);
        ui->leftAlgoBtn[i].hovered=pt_in(mx,my,&ui->leftAlgoBtn[i].rect);
        ui->rightAlgoBtn[i].hovered=pt_in(mx,my,&ui->rightAlgoBtn[i].rect);
    }
    for(int i=0;i<2;i++){
        ui->heuristicBtn[i].hovered=pt_in(mx,my,&ui->heuristicBtn[i].rect);
        ui->leftHBtn[i].hovered=pt_in(mx,my,&ui->leftHBtn[i].rect);
        ui->rightHBtn[i].hovered=pt_in(mx,my,&ui->rightHBtn[i].rect);
    }
    ui->btnVisualize.hovered=pt_in(mx,my,&ui->btnVisualize.rect);
    ui->btnMaze.hovered=pt_in(mx,my,&ui->btnMaze.rect);
    ui->btnClear.hovered=pt_in(mx,my,&ui->btnClear.rect);
    ui->btnReset.hovered=pt_in(mx,my,&ui->btnReset.rect);
    ui->btnStats.hovered=pt_in(mx,my,&ui->btnStats.rect);
    ui->btnCompare.hovered=pt_in(mx,my,&ui->btnCompare.rect);
    ui->btnHCompare.hovered=pt_in(mx,my,&ui->btnHCompare.rect);
    ui->btnRunCompare.hovered=pt_in(mx,my,&ui->btnRunCompare.rect);
}

/* ── Draw split mode labels ── */
static void draw_split_labels(SDL_Renderer *ren, TTF_Font *font, Grid *g, UI *ui){
    if(!g->splitMode)return;
    int half=GRID_AREA_W/2;
    SDL_Color white={255,255,255,255};
    SDL_Color yellow={255,220,0,255};

    char lbl[64];
    const char *lh=(ui->mode==MODE_HEURISTIC_SETUP)?
        (ui->leftH==HEURISTIC_MANHATTAN?"Manhattan":"Euclidean"):
        (g->left.heuristic==HEURISTIC_MANHATTAN?"Manhattan":"Euclidean");
    const char *rh=(ui->mode==MODE_HEURISTIC_SETUP)?
        (ui->rightH==HEURISTIC_MANHATTAN?"Manhattan":"Euclidean"):
        (g->right.heuristic==HEURISTIC_MANHATTAN?"Manhattan":"Euclidean");

    snprintf(lbl,63,"%s [%s]",algo_names[g->left.algo],lh);
    SDL_SetRenderDrawColor(ren,0,0,0,160);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    SDL_Rect lb={0,0,half,20}; SDL_RenderFillRect(ren,&lb);
    SDL_Rect rb={half,0,half,20}; SDL_RenderFillRect(ren,&rb);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);

    draw_text(ren,font,lbl,4,2,yellow);
    snprintf(lbl,63,"%s [%s]",algo_names[g->right.algo],rh);
    draw_text(ren,font,lbl,half+4,2,yellow);

    /* live stats at bottom */
    char sl[80],sr[80];
    snprintf(sl,79,"Path:%d Visited:%d %.2fms",g->left.pathLength,g->left.nodesVisited,g->left.timeMs);
    snprintf(sr,79,"Path:%d Visited:%d %.2fms",g->right.pathLength,g->right.nodesVisited,g->right.timeMs);
    SDL_SetRenderDrawColor(ren,0,0,0,160);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    SDL_Rect lb2={0,WINDOW_HEIGHT-18,half,18};SDL_RenderFillRect(ren,&lb2);
    SDL_Rect rb2={half,WINDOW_HEIGHT-18,half,18};SDL_RenderFillRect(ren,&rb2);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    draw_text(ren,font,sl,4,WINDOW_HEIGHT-16,white);
    draw_text(ren,font,sr,half+4,WINDOW_HEIGHT-16,white);
}

/* ────────────────────────────────────────────────────────────────────────────
   MAIN
   ──────────────────────────────────────────────────────────────────────────── */
int main(void){
    if(SDL_Init(SDL_INIT_VIDEO)!=0){fprintf(stderr,"SDL: %s\n",SDL_GetError());return 1;}
    if(TTF_Init()!=0){fprintf(stderr,"TTF: %s\n",TTF_GetError());return 1;}

    TTF_Font *font=TTF_OpenFont("C:/Windows/Fonts/arial.ttf",11);
    TTF_Font *bigFont=TTF_OpenFont("C:/Windows/Fonts/arialbd.ttf",13);
    if(!font||!bigFont){fprintf(stderr,"Font: %s\n",TTF_GetError());return 1;}

    SDL_Window *win=SDL_CreateWindow("Path Finding Visualizer — Learning Tool",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WINDOW_WIDTH,WINDOW_HEIGHT,SDL_WINDOW_SHOWN);
    SDL_Renderer *ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);

    Grid g; grid_init(&g,GRID_W,GRID_H,NODE_W,NODE_H);
    UI ui; ui_build(&ui);
    Popup popup; memset(&popup,0,sizeof(popup));
    bool comparePopupShown=false;

    bool running=true;
    SDL_Event ev;

    while(running){
        while(SDL_PollEvent(&ev)){
            if(ev.type==SDL_QUIT){running=false;break;}

            if(ev.type==SDL_KEYDOWN){
                if(ev.key.keysym.sym==SDLK_ESCAPE){
                    if(popup.visible) popup.visible=false;
                    else running=false;
                }
            }

            if(ev.type==SDL_MOUSEBUTTONDOWN){
                int mx=ev.button.x,my=ev.button.y;

                /* close popup on click anywhere */
                if(popup.visible){popup.visible=false;continue;}

                if(mx>=GRID_AREA_W){
                    /* ── Side panel clicks ── */
                    if(ui.mode==MODE_NORMAL){
                        /* node type */
                        for(int i=0;i<3;i++) if(pt_in(mx,my,&ui.nodeBtn[i].rect)){
                            ui.nodeBtn[ui.selectedNode].selected=false;
                            ui.selectedNode=i; ui.nodeBtn[i].selected=true;
                            g.selectedState=(i==0)?STATE_START:(i==1)?STATE_END:STATE_WALL;
                        }
                        /* algo select */
                        for(int i=0;i<ALGO_COUNT;i++) if(pt_in(mx,my,&ui.algoBtn[i].rect)){
                            ui.algoBtn[ui.selectedAlgo].selected=false;
                            ui.selectedAlgo=i; ui.algoBtn[i].selected=true;
                            g.currentAlgo=(AlgoType)i;
                        }
                        /* heuristic */
                        for(int i=0;i<2;i++) if(pt_in(mx,my,&ui.heuristicBtn[i].rect)){
                            ui.heuristicBtn[ui.selectedHeuristic].selected=false;
                            ui.selectedHeuristic=i; ui.heuristicBtn[i].selected=true;
                            g.currentHeuristic=(HeuristicType)i;
                        }
                        /* actions */
                        if(pt_in(mx,my,&ui.btnVisualize.rect)&&!g.isRunning) grid_find_path(&g);
                        if(pt_in(mx,my,&ui.btnMaze.rect)&&!g.isRunning)      grid_generate_maze(&g);
                        if(pt_in(mx,my,&ui.btnClear.rect)&&!g.isRunning)     grid_clear_search(&g);
                        if(pt_in(mx,my,&ui.btnReset.rect)&&!g.isRunning)     grid_reset(&g);
                        if(pt_in(mx,my,&ui.btnStats.rect)&&g.singleReady)    show_stats_popup(&popup,&g);
                        /* enter compare modes */
                        if(pt_in(mx,my,&ui.btnCompare.rect)){
                            ui.mode=MODE_COMPARE_SETUP;
                            build_compare_panel(&ui,false);
                        }
                        if(pt_in(mx,my,&ui.btnHCompare.rect)){
                            ui.mode=MODE_HEURISTIC_SETUP;
                            build_compare_panel(&ui,true);
                        }

                    } else if(ui.mode==MODE_COMPARE_SETUP||ui.mode==MODE_HEURISTIC_SETUP){
                        /* node type */
                        for(int i=0;i<3;i++) if(pt_in(mx,my,&ui.nodeBtn[i].rect)){
                            ui.nodeBtn[ui.selectedNode].selected=false;
                            ui.selectedNode=i; ui.nodeBtn[i].selected=true;
                            g.selectedState=(i==0)?STATE_START:(i==1)?STATE_END:STATE_WALL;
                        }
                        if(ui.mode==MODE_COMPARE_SETUP){
                            for(int i=0;i<ALGO_COUNT;i++){
                                if(pt_in(mx,my,&ui.leftAlgoBtn[i].rect)){
                                    ui.leftAlgoBtn[ui.leftAlgo].selected=false;
                                    ui.leftAlgo=i; ui.leftAlgoBtn[i].selected=true;
                                }
                                if(pt_in(mx,my,&ui.rightAlgoBtn[i].rect)){
                                    ui.rightAlgoBtn[ui.rightAlgo].selected=false;
                                    ui.rightAlgo=i; ui.rightAlgoBtn[i].selected=true;
                                }
                            }
                        } else {
                            for(int i=0;i<2;i++){
                                if(pt_in(mx,my,&ui.leftHBtn[i].rect)){
                                    ui.leftHBtn[ui.leftH].selected=false;
                                    ui.leftH=i; ui.leftHBtn[i].selected=true;
                                }
                                if(pt_in(mx,my,&ui.rightHBtn[i].rect)){
                                    ui.rightHBtn[ui.rightH].selected=false;
                                    ui.rightH=i; ui.rightHBtn[i].selected=true;
                                }
                            }
                        }
                        if(pt_in(mx,my,&ui.btnMaze.rect)&&!g.isRunning)  grid_generate_maze(&g);
                        if(pt_in(mx,my,&ui.btnReset.rect)&&!g.isRunning) grid_reset(&g);
                        if(pt_in(mx,my,&ui.btnClear.rect)){ui.mode=MODE_NORMAL;ui_build(&ui);comparePopupShown=false;}
                        if(pt_in(mx,my,&ui.btnRunCompare.rect)&&!g.isRunning){
                            if(g.splitMode&&g.left.ready&&g.right.ready){
                                /* already ran — show results again */
                                show_compare_popup(&popup,&g);
                            } else {
                                AlgoType la=(AlgoType)ui.leftAlgo, ra=(AlgoType)ui.rightAlgo;
                                HeuristicType lh=(HeuristicType)ui.leftH, rh=(HeuristicType)ui.rightH;
                                if(ui.mode==MODE_HEURISTIC_SETUP){la=ra=g.currentAlgo;}
                                comparePopupShown=false;
                                grid_find_path_split(&g,la,lh,ra,rh);
                            }
                        }
                    }
                } else {
                    /* ── Grid clicks ── */
                    if(!g.isRunning&&!g.splitMode)
                        grid_handle_click(&g,mx,my,ev.button.button);
                }
            }

            if(ev.type==SDL_MOUSEMOTION){
                update_hovers(&ui,ev.motion.x,ev.motion.y);
                if((ev.motion.state&(SDL_BUTTON_LMASK|SDL_BUTTON_RMASK))
                    &&ev.motion.x<GRID_AREA_W&&!g.isRunning&&!g.splitMode)
                    grid_handle_drag(&g,ev.motion.x,ev.motion.y,ev.motion.state);
            }
        }

        if(g.isRunning) grid_step(&g);

        /* auto-show compare popup once when animation finishes */
        if(g.splitMode&&!g.isRunning&&g.left.ready&&g.right.ready&&!comparePopupShown){
            show_compare_popup(&popup,&g);
            comparePopupShown=true;
        }

        /* render */
        SDL_SetRenderDrawColor(ren,242,242,242,255);
        SDL_RenderClear(ren);
        grid_draw(&g,ren,GRID_AREA_W);
        if(g.splitMode) draw_split_labels(ren,font,&g,&ui);
        ui_draw_panel(ren,font,bigFont,&ui,&g);
        popup_draw(ren,font,bigFont,&popup);
        SDL_RenderPresent(ren);
    }

    grid_free(&g);
    TTF_CloseFont(font);TTF_CloseFont(bigFont);
    TTF_Quit();SDL_DestroyRenderer(ren);SDL_DestroyWindow(win);SDL_Quit();
    return 0;
}

#endif