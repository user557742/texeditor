#include <bits/stdc++.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#define rei(i,a,b) for(int i=a;i<=b;i++)
using namespace std;

/* ================= EXIT ================= */
bool exit_check(SDL_Event e){
    if(e.type == SDL_EVENT_QUIT) return true;
    if(e.type == SDL_EVENT_KEY_DOWN)
        if(e.key.key == SDLK_Q && (e.key.mod & SDL_KMOD_CTRL)) return true;
    return false;
}

/* ================= ATLAS ================= */
SDL_Texture* CreateAsciiAtlas(
    SDL_Renderer* renderer,
    TTF_Font* font,
    SDL_Color color,
    unordered_map<char, SDL_Rect>& glyphRects
){
    const int L = 32, R = 126;
    int W = 0, H = 0;
    unordered_map<char, SDL_Surface*> surf;

    for(char c=L;c<=R;c++){
        char s[2]={c,0};
        SDL_Surface* sf = TTF_RenderText_Blended(font,s,1,color);
        if(!sf) continue;
        surf[c]=sf;
        glyphRects[c]={W,0,sf->w,sf->h};
        W+=sf->w;
        H=max(H,sf->h);
    }

    SDL_Texture* atlas = SDL_CreateTexture(
        renderer,SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,W,H
    );
    SDL_SetTextureBlendMode(atlas,SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer,atlas);
    SDL_SetRenderDrawColor(renderer,0,0,0,0);
    SDL_RenderClear(renderer);

    for(auto& [c,sf]:surf){
        SDL_Texture* t=SDL_CreateTextureFromSurface(renderer,sf);
        SDL_FRect dst={(float)glyphRects[c].x,0,
                       (float)glyphRects[c].w,
                       (float)glyphRects[c].h};
        SDL_RenderTexture(renderer,t,nullptr,&dst);
        SDL_DestroyTexture(t);
        SDL_DestroySurface(sf);
    }

    SDL_SetRenderTarget(renderer,nullptr);
    return atlas;
}

/* ================= TEXT ================= */
SDL_Texture* renderText(
    SDL_Renderer* renderer,
    SDL_Texture* atlas,
    unordered_map<char,SDL_Rect>& glyph,
    const string& s
){
    if(s.empty()) return nullptr;
    int w=0,h=0;
    for(char c:s){
        if(!glyph.count(c)) continue;
        w+=glyph[c].w;
        h=max(h,glyph[c].h);
    }
    if(w==0) return nullptr;

    SDL_Texture* tex=SDL_CreateTexture(
        renderer,SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,w,h
    );
    SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer,tex);
    SDL_SetRenderDrawColor(renderer,0,0,0,0);
    SDL_RenderClear(renderer);

    int x=0;
    for(char c:s){
        if(!glyph.count(c)) continue;
        SDL_Rect r=glyph[c];
        SDL_FRect src={(float)r.x,0,(float)r.w,(float)r.h};
        SDL_FRect dst={(float)x,0,(float)r.w,(float)r.h};
        SDL_RenderTexture(renderer,atlas,&src,&dst);
        x+=r.w;
    }
    SDL_SetRenderTarget(renderer,nullptr);
    return tex;
}

/* ================= MODEL ================= */
struct Line{
    string s;
    SDL_Texture* tex=nullptr;
    bool dirty=true;
};

vector<Line> lines;
int fontSize=30;
int cpx=0,cpy=0;
int chw=0,chh=0;

/* ================= FONT REBUILD ================= */
void rebuild_font(
    SDL_Renderer* renderer,
    TTF_Font*& font,
    SDL_Texture*& atlas,
    unordered_map<char,SDL_Rect>& glyph
){
    if(atlas) SDL_DestroyTexture(atlas);
    if(font) TTF_CloseFont(font);

    font=TTF_OpenFont(
        "D:/texeditor/font/JetBrainsMono-Bold.ttf",
        fontSize
    );
    if(!font) return;

    glyph.clear();
    atlas=CreateAsciiAtlas(renderer,font,{255,255,255,255},glyph);

    int minx,maxx,miny,maxy,adv;
    TTF_GetGlyphMetrics(font,'M',&minx,&maxx,&miny,&maxy,&adv);
    chw=adv;
    chh=TTF_GetFontHeight(font);

    for(auto& l:lines){
        if(l.tex) SDL_DestroyTexture(l.tex);
        l.tex=nullptr;
        l.dirty=true;
    }

    int row=cpy/chh;
    int col=cpx/chw;
    cpy=row*chh;
    cpx=col*chw;
}
int main(){
    freopen("log.txt","w",stdout);

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* win=SDL_CreateWindow("LIDE",800,600,SDL_WINDOW_RESIZABLE);
    SDL_Renderer* ren=SDL_CreateRenderer(win,nullptr);

    TTF_Font* font=nullptr;
    SDL_Texture* atlas=nullptr;
    unordered_map<char,SDL_Rect> glyph;

    rebuild_font(ren,font,atlas,glyph);

    lines.push_back(Line());

    SDL_StartTextInput(win);
    SDL_Event e;
    bool run=true;
    int fullmh=0;
    int borderlessss=0;
    while(run){
        while(SDL_PollEvent(&e)){
            if(exit_check(e)) run=false;

            int row=cpy/chh;
            row=clamp(row,0,(int)lines.size()-1);

            if(e.type==SDL_EVENT_TEXT_INPUT){
                char c=e.text.text[0];
                int pos=cpx/chw;
                pos=min(pos,(int)lines[row].s.size());
                lines[row].s.insert(lines[row].s.begin()+pos,c);
                cpx+=chw;
                lines[row].dirty=true;
            }

            if(e.type==SDL_EVENT_KEY_DOWN){
                if(e.key.key==SDLK_RETURN){
                    Line nl;
                    int pos=cpx/chw;
                    nl.s=lines[row].s.substr(pos);
                    lines[row].s.erase(pos);
                    lines[row].dirty=true;
                    lines.insert(lines.begin()+row+1,nl);
                    cpx=0;
                    cpy+=chh;
                }

                if(e.key.key==SDLK_BACKSPACE){
                    if(cpx>0){
                        int pos=cpx/chw-1;
                        lines[row].s.erase(pos,1);
                        cpx-=chw;
                        lines[row].dirty=true;
                    }else if(row>0){
                        int prev=lines[row-1].s.size();
                        lines[row-1].s+=lines[row].s;
                        lines[row-1].dirty=true;
                        if(lines[row].tex) SDL_DestroyTexture(lines[row].tex);
                        lines.erase(lines.begin()+row);
                        cpy-=chh;
                        cpx=prev*chw;
                    }
                }

                if(e.key.key==SDLK_UP){
                    if(cpy>=chh) cpy-=chh;
                }
                if(e.key.key==SDLK_DOWN){
                    if(cpy/chh<(int)lines.size()-1) cpy+=chh;
                }
                if(e.key.key==SDLK_LEFT){
                    if(cpx>=chw) cpx-=chw;
                }
                if(e.key.key==SDLK_RIGHT){
                    int maxx=lines[row].s.size()*chw;
                    cpx=min(cpx+chw,maxx);
                }
                if(e.key.key==SDLK_F11) fullmh=1-fullmh;
                if(e.key.key==SDLK_F10) borderlessss=1-borderlessss;
                
                if(fullmh) SDL_SetWindowFullscreen(win,SDL_WINDOW_FULLSCREEN);
                else SDL_SetWindowFullscreen(win,0);
                if(borderlessss) SDL_SetWindowBordered(win,SDL_WINDOW_BORDERLESS);
                else SDL_SetWindowBordered(win,0);
                if(
                    (e.key.key == SDLK_EQUALS || e.key.key == SDLK_KP_PLUS) &&
                    (e.key.mod & SDL_KMOD_CTRL)
                ){
                    fontSize++;
                    rebuild_font(ren,font,atlas,glyph);
                }

                if(
                    (e.key.key == SDLK_MINUS || e.key.key == SDLK_KP_MINUS) &&
                    (e.key.mod & SDL_KMOD_CTRL)
                ){
                    fontSize--;
                    rebuild_font(ren,font,atlas,glyph);
                }
                row=cpy/chh;
                row=clamp(row,0,(int)lines.size()-1);
                cpx=min(cpx,(int)lines[row].s.size()*chw);
            }
        }

        SDL_SetRenderDrawColor(ren,0,0,0,255);
        SDL_RenderClear(ren);

        int y=0;
        for(auto& l:lines){
            if(l.dirty){
                if(l.tex) SDL_DestroyTexture(l.tex);
                l.tex=renderText(ren,atlas,glyph,l.s);
                l.dirty=false;
            }
            if(l.tex){
                float w,h;
                SDL_GetTextureSize(l.tex,&w,&h);
                SDL_FRect d={0,(float)y,w,h};
                SDL_RenderTexture(ren,l.tex,nullptr,&d);
            }
            y+=chh;
        }

        SDL_FRect caret={(float)cpx,(float)cpy,(float)chw,(float)chh};
        SDL_SetRenderDrawColor(ren,255,255,255,80);
        SDL_RenderRect(ren,&caret);

        SDL_RenderPresent(ren);
    }

    for(auto& l:lines) if(l.tex) SDL_DestroyTexture(l.tex);
    SDL_DestroyTexture(atlas);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
