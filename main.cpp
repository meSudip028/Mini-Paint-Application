/*
 * ============================================================
 *  Mini Paint Application
 *  Using C++ + OpenGL (GLUT/freeglut)
 * ------------------------------------------------------------
 *  Features:
 *    - Draw Line  (DDA Algorithm)
 *    - Draw Circle (Midpoint Circle Algorithm)
 *    - Fill Color  (Flood Fill Algorithm)
 *    - Change Colors
 *    - Clear Screen / Undo
 * ============================================================
 */

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cmath>
#include <vector>
#include <stack>
#include <cstdio>

const int W     = 800;
const int H     = 600;
const int PANEL = 150;

struct Color3 { float r, g, b; };

// ── Palette ──────────────────────────────────────────────────
const int PALETTE_SIZE = 10;
Color3 palette[PALETTE_SIZE] = {
    {0.0f, 0.0f, 0.0f},   // Black
    {1.0f, 0.0f, 0.0f},   // Red
    {0.0f, 0.75f,0.0f},   // Green
    {0.1f, 0.4f, 0.9f},   // Blue
    {1.0f, 0.8f, 0.0f},   // Yellow
    {1.0f, 0.5f, 0.0f},   // Orange
    {0.6f, 0.0f, 0.8f},   // Purple
    {0.0f, 0.8f, 0.8f},   // Cyan
    {1.0f, 0.4f, 0.7f},   // Pink
    {1.0f, 1.0f, 1.0f},   // White / Eraser
};

// Palette grid layout (computed once, used in both draw & click)
const int   PAL_COLS = 4;
const float PAL_SW   = 26, PAL_SH = 22, PAL_GAP = 5;
const float PAL_OX   = 8;   // left offset inside panel
float       PAL_OY   = 0;   // top-left Y of first swatch (set in drawPanel)

Color3 currentColor = {0.0f, 0.0f, 0.0f};

// ── Pixel buffer (for flood fill source) ─────────────────────
// index: pixels[screenY][screenX],  screenY=0 is TOP of window
Color3 pixels[H][W];

// ── App state ────────────────────────────────────────────────
enum Mode { DRAW_LINE, DRAW_CIRCLE, FILL };
Mode  currentMode = DRAW_LINE;
bool  waiting     = false;
float startX, startY;
char  statusMsg[256] = "L=Line  C=Circle  F=Fill  |  Click canvas!";

struct Shape { int type; float x1,y1,x2,y2; Color3 color; };
std::vector<Shape> shapes;

// ── Helpers ──────────────────────────────────────────────────
void setStatus(const char* m){ snprintf(statusMsg,sizeof(statusMsg),"%s",m); }

// Convert window mouse-Y (top=0) to OpenGL Y (bottom=0)
float toGL(int y){ return (float)(H - y); }

bool onCanvas(int x,int y){ return x>=PANEL && x<W && y>=20 && y<H; }

bool colorEq(Color3 a,Color3 b){
    return fabsf(a.r-b.r)<0.015f &&
           fabsf(a.g-b.g)<0.015f &&
           fabsf(a.b-b.b)<0.015f;
}

// ── Pixel buffer access (screen coords: y=0 top) ─────────────
void bufSet(int x,int y,Color3 c){
    if(x<PANEL||x>=W||y<0||y>=H) return;
    pixels[y][x] = c;
}
Color3 bufGet(int x,int y){
    if(x<PANEL||x>=W||y<0||y>=H) return {-1,-1,-1};
    return pixels[y][x];
}

// ── DDA Line ─────────────────────────────────────────────────
// Works in OpenGL coords (y=0 bottom)
void ddaLine(float x1,float y1,float x2,float y2,Color3 c){
    float dx=x2-x1, dy=y2-y1;
    float steps=fmaxf(fabsf(dx),fabsf(dy));
    if(steps==0) return;
    float xi=dx/steps, yi=dy/steps;
    float x=x1, y=y1;
    glColor3f(c.r,c.g,c.b);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for(int i=0;i<=(int)steps;i++){
        int px=(int)roundf(x), pgl=(int)roundf(y);
        glVertex2f((float)px,(float)pgl);
        int sy = H - pgl;   // convert to screen-y for buffer
        bufSet(px, sy, c);
        x+=xi; y+=yi;
    }
    glEnd();
}

// ── Midpoint Circle ───────────────────────────────────────────
void plotOct(float cx,float cy,float ox,float oy,Color3 c){
    float pts[8][2]={
        {cx+ox,cy+oy},{cx-ox,cy+oy},{cx+ox,cy-oy},{cx-ox,cy-oy},
        {cx+oy,cy+ox},{cx-oy,cy+ox},{cx+oy,cy-ox},{cx-oy,cy-ox}
    };
    for(auto& p:pts){
        glVertex2f(p[0],p[1]);
        int sx=(int)roundf(p[0]), sgy=(int)roundf(p[1]);
        int sy=H-sgy;
        bufSet(sx,sy,c);
    }
}
void midpointCircle(float cx,float cy,float r,Color3 c){
    glColor3f(c.r,c.g,c.b);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    float x=0,y=r,p=1.0f-r;
    plotOct(cx,cy,x,y,c);
    while(x<y){
        x++;
        if(p<0) p+=2*x+1;
        else{ y--; p+=2*(x-y)+1; }
        plotOct(cx,cy,x,y,c);
    }
    glEnd();
}

// ── Flood Fill (screen coords) ────────────────────────────────
void floodFill(int sx,int sy,Color3 target,Color3 fill){
    if(colorEq(target,fill)) return;
    std::stack<std::pair<int,int>> st;
    st.push({sx,sy});
    while(!st.empty()){
        auto [cx,cy]=st.top(); st.pop();
        if(cx<PANEL||cx>=W||cy<0||cy>=H) continue;
        if(!colorEq(bufGet(cx,cy),target)) continue;
        bufSet(cx,cy,fill);
        // draw the pixel immediately in OpenGL coords
        int gly=H-cy;
        glColor3f(fill.r,fill.g,fill.b);
        glBegin(GL_POINTS); glVertex2f((float)cx,(float)gly); glEnd();
        st.push({cx+1,cy}); st.push({cx-1,cy});
        st.push({cx,cy+1}); st.push({cx,cy-1});
    }
    glFlush();
}

// ── Draw one shape ────────────────────────────────────────────
void drawShape(const Shape& s){
    if(s.type==0)
        ddaLine(s.x1,s.y1,s.x2,s.y2,s.color);
    else{
        float r=sqrtf((s.x2-s.x1)*(s.x2-s.x1)+(s.y2-s.y1)*(s.y2-s.y1));
        midpointCircle(s.x1,s.y1,r,s.color);
    }
}

// ── Text helpers ─────────────────────────────────────────────
void txt(float x,float y,const char* t,float r,float g,float b,void* font=GLUT_BITMAP_HELVETICA_12){
    glColor3f(r,g,b); glRasterPos2f(x,y);
    for(const char* c=t;*c;c++) glutBitmapCharacter(font,*c);
}

// ── Draw Panel ───────────────────────────────────────────────
void drawPanel(){
    // background
    glColor3f(0.13f,0.13f,0.16f);
    glBegin(GL_QUADS);
      glVertex2f(0,0); glVertex2f(PANEL,0);
      glVertex2f(PANEL,H); glVertex2f(0,H);
    glEnd();
    glColor3f(0.28f,0.28f,0.33f);
    glBegin(GL_LINES); glVertex2f(PANEL,0); glVertex2f(PANEL,H); glEnd();

    float y = (float)H - 18.0f;

    // Title
    txt(8,y,"Mini Paint",0.3f,0.7f,1.0f,GLUT_BITMAP_HELVETICA_18); y-=28;
    glColor3f(0.28f,0.28f,0.33f);
    glBegin(GL_LINES); glVertex2f(5,y); glVertex2f(PANEL-5,y); glEnd(); y-=16;

    // Tools
    txt(8,y,"TOOLS",0.55f,0.55f,0.6f); y-=20;

    auto btnRow=[&](const char* label,bool active){
        if(active){
            glColor3f(0.15f,0.38f,0.82f);
            glBegin(GL_QUADS);
              glVertex2f(5,y-2); glVertex2f(PANEL-5,y-2);
              glVertex2f(PANEL-5,y+14); glVertex2f(5,y+14);
            glEnd();
        }
        txt(10,y,label,active?1.0f:0.68f,active?1.0f:0.68f,active?1.0f:0.68f);
        y-=22;
    };
    btnRow("[L] Draw Line",   currentMode==DRAW_LINE);
    btnRow("[C] Draw Circle", currentMode==DRAW_CIRCLE);
    btnRow("[F] Fill Color",  currentMode==FILL);
    y-=6;

    glColor3f(0.28f,0.28f,0.33f);
    glBegin(GL_LINES); glVertex2f(5,y); glVertex2f(PANEL-5,y); glEnd(); y-=16;

    // Colors
    txt(8,y,"COLORS",0.55f,0.55f,0.6f); y-=20;

    // Store palette top-left Y for click detection
    PAL_OY = y;

    for(int i=0;i<PALETTE_SIZE;i++){
        float px = PAL_OX + (i%PAL_COLS)*(PAL_SW+PAL_GAP);
        float py = y       - (i/PAL_COLS)*(PAL_SH+PAL_GAP);
        glColor3f(palette[i].r,palette[i].g,palette[i].b);
        glBegin(GL_QUADS);
          glVertex2f(px,py-PAL_SH); glVertex2f(px+PAL_SW,py-PAL_SH);
          glVertex2f(px+PAL_SW,py); glVertex2f(px,py);
        glEnd();
        // white border if selected
        if(colorEq(palette[i],currentColor)){
            glColor3f(1,1,1); glLineWidth(2.0f);
            glBegin(GL_LINE_LOOP);
              glVertex2f(px-1,py-PAL_SH-1); glVertex2f(px+PAL_SW+1,py-PAL_SH-1);
              glVertex2f(px+PAL_SW+1,py+1); glVertex2f(px-1,py+1);
            glEnd();
            glLineWidth(1.0f);
        }
    }
    int rows=(PALETTE_SIZE+PAL_COLS-1)/PAL_COLS;
    y -= rows*(PAL_SH+PAL_GAP)+10;

    glColor3f(0.28f,0.28f,0.33f);
    glBegin(GL_LINES); glVertex2f(5,y); glVertex2f(PANEL-5,y); glEnd(); y-=16;

    // Current color preview
    txt(8,y,"SELECTED",0.55f,0.55f,0.6f); y-=18;
    glColor3f(currentColor.r,currentColor.g,currentColor.b);
    glBegin(GL_QUADS);
      glVertex2f(8,y-22); glVertex2f(PANEL-8,y-22);
      glVertex2f(PANEL-8,y); glVertex2f(8,y);
    glEnd();
    glColor3f(0.5f,0.5f,0.55f); glLineWidth(1);
    glBegin(GL_LINE_LOOP);
      glVertex2f(8,y-22); glVertex2f(PANEL-8,y-22);
      glVertex2f(PANEL-8,y); glVertex2f(8,y);
    glEnd();
    y-=32;

    glColor3f(0.28f,0.28f,0.33f);
    glBegin(GL_LINES); glVertex2f(5,y); glVertex2f(PANEL-5,y); glEnd(); y-=16;
    txt(8,y,"[U] Undo",0.7f,0.7f,0.7f); y-=18;
    txt(8,y,"[X] Clear",0.7f,0.7f,0.7f);
}

void drawStatusBar(){
    glColor3f(0.1f,0.1f,0.12f);
    glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(W,0); glVertex2f(W,20); glVertex2f(0,20); glEnd();
    txt(PANEL+8,5,statusMsg,0.82f,0.82f,0.82f);
}

// ── Full redraw of pixel buffer onto canvas ───────────────────
void redrawBuffer(){
    glPointSize(1.0f);
    glBegin(GL_POINTS);
    for(int sy=0;sy<H;sy++){
        for(int sx=PANEL;sx<W;sx++){
            Color3& c=pixels[sy][sx];
            glColor3f(c.r,c.g,c.b);
            glVertex2f((float)sx,(float)(H-sy));
        }
    }
    glEnd();
}

// ── Display ──────────────────────────────────────────────────
void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0,W,0,H);
    glMatrixMode(GL_MODELVIEW);  glLoadIdentity();

    // White canvas background
    glColor3f(1,1,1);
    glBegin(GL_QUADS);
      glVertex2f(PANEL,20); glVertex2f(W,20);
      glVertex2f(W,H);      glVertex2f(PANEL,H);
    glEnd();

    drawPanel();
    drawStatusBar();

    // Redraw pixel buffer (fills etc.)
    redrawBuffer();

    // Redraw vector shapes on top
    glPointSize(2.0f);
    for(auto& s:shapes) drawShape(s);

    // Preview start dot
    if(waiting){
        glColor3f(1.0f,0.2f,0.2f);
        glPointSize(7.0f);
        glBegin(GL_POINTS); glVertex2f(startX,startY); glEnd();
    }

    glutSwapBuffers();
}

void reshape(int w,int h){ glViewport(0,0,w,h); }

// ── Palette click (using same layout math as drawPanel) ───────
bool handlePaletteClick(int mx,int my){
    // my is mouse Y (top=0), convert to GL Y
    float gy = toGL(my);
    for(int i=0;i<PALETTE_SIZE;i++){
        float px = PAL_OX + (i%PAL_COLS)*(PAL_SW+PAL_GAP);
        float py = PAL_OY - (i/PAL_COLS)*(PAL_SH+PAL_GAP);
        if((float)mx>=px && (float)mx<=px+PAL_SW &&
           gy>=py-PAL_SH && gy<=py){
            currentColor = palette[i];
            return true;
        }
    }
    return false;
}

// ── Mouse ────────────────────────────────────────────────────
void mouse(int btn,int state,int x,int y){
    if(btn!=GLUT_LEFT_BUTTON||state!=GLUT_DOWN) return;

    // Panel area → check palette
    if(x<PANEL){
        if(handlePaletteClick(x,y))
            setStatus("Color changed! Now draw or fill.");
        glutPostRedisplay();
        return;
    }
    if(!onCanvas(x,y)) return;

    float gx=(float)x, gy=toGL(y);
    // screen coords for buffer
    int sx=x, sy=y;

    if(currentMode==FILL){
        Color3 target=bufGet(sx,sy);
        floodFill(sx,sy,target,currentColor);
        setStatus("Filled! Pick another tool or color.");
        glutPostRedisplay();
        return;
    }

    if(!waiting){
        startX=gx; startY=gy; waiting=true;
        setStatus(currentMode==DRAW_LINE ?
            "Start set — click end point." :
            "Center set — click for radius.");
    } else {
        Shape s; s.color=currentColor;
        if(currentMode==DRAW_LINE){
            s.type=0; s.x1=startX; s.y1=startY; s.x2=gx; s.y2=gy;
            setStatus("Line drawn!");
        } else {
            s.type=1; s.x1=startX; s.y1=startY; s.x2=gx; s.y2=gy;
            setStatus("Circle drawn!");
        }
        shapes.push_back(s);
        waiting=false;
        glutPostRedisplay();
    }
}

// ── Keyboard ─────────────────────────────────────────────────
void keyboard(unsigned char key,int /*x*/,int /*y*/){
    switch(key){
        case 'l': case 'L':
            currentMode=DRAW_LINE;   waiting=false;
            setStatus("Line mode (DDA). Click start point on canvas."); break;
        case 'c': case 'C':
            currentMode=DRAW_CIRCLE; waiting=false;
            setStatus("Circle mode (Midpoint). Click center on canvas."); break;
        case 'f': case 'F':
            currentMode=FILL;        waiting=false;
            setStatus("Fill mode (Flood Fill). Click inside a closed shape."); break;
        case 'u': case 'U':
            if(!shapes.empty()){
                shapes.pop_back();
                // reset buffer and redraw remaining
                for(int j=0;j<H;j++)
                    for(int i=PANEL;i<W;i++) pixels[j][i]={1,1,1};
                setStatus("Undo done.");
            } else setStatus("Nothing to undo.");
            break;
        case 'x': case 'X':
            shapes.clear(); waiting=false;
            for(int j=0;j<H;j++)
                for(int i=PANEL;i<W;i++) pixels[j][i]={1,1,1};
            setStatus("Canvas cleared.");
            break;
        case 27: exit(0);
    }
    glutPostRedisplay();
}

// ── Main ─────────────────────────────────────────────────────
int main(int argc,char** argv){
    // Init pixel buffer: canvas=white, panel=dark
    for(int j=0;j<H;j++)
        for(int i=0;i<W;i++)
            pixels[j][i]=(i>=PANEL)?Color3{1,1,1}:Color3{0.13f,0.13f,0.16f};

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(W,H);
    glutInitWindowPosition(100,80);
    glutCreateWindow("Mini Paint - C++ OpenGL");

    glClearColor(1,1,1,1);
    glPointSize(2.0f);
    glEnable(GL_POINT_SMOOTH);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}