// CATASTROPHE MIXER - 3DS homebrew (libctru + citro2d)
// Top screen: stereoscopic 3D city (parallax layers follow the 3D slider)
// Bottom screen: touch panel to pick & mix two disasters
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define SW 400
#define SH 240
#define GROUND 205
#define NB 20
#define BW 20
#define NPART 220
#define DURATION 7.0f

enum { E_QUAKE, E_TORNADO, E_TSUNAMI, E_VOLCANO, E_METEOR, E_LIGHT, E_COUNT };
enum { P_ROCK, P_LAVA, P_DEBRIS, P_BOLT, P_BOOM, P_SMOKE, P_WATER };

static const char *ENAME[E_COUNT] = {"Earthquake","Tornado","Tsunami","Volcano","Meteor","Lightning"};
static const char *PAIR[E_COUNT][E_COUNT];
static u32 ECOL[E_COUNT];

typedef struct { float x, y, vx, vy; int life, maxlife, type; } Part;
typedef struct { float hp, maxhp; int h; int tint; } Bld;

static Bld city[NB];
static Part parts[NPART];
static int order[2], nsel = 0;
static bool active = false;
static int mask = 0;
static float t = 0, cx = 200, tx = 200, wx = -40, gmult = 1;
static float shake = 0, shx = 0, shy = 0, gloom = 0, volc = 0;
static u32 frame = 0;
static char curname[48] = "";

static float slider = 0, g_off = 0, g_z = 0.5f;
static int g_sgn = 1;
static C2D_TextBuf tbuf;

static float frand(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }
static u32 lerpc(int r1,int g1,int b1,int r2,int g2,int b2,float k) {
    return C2D_Color32(r1+(r2-r1)*k, g1+(g2-g1)*k, b1+(b2-b1)*k, 255);
}

// ---------- world ----------
static void reset_city(void) {
    for (int i = 0; i < NB; i++) {
        city[i].h = 36 + rand() % 70; city[i].hp = city[i].maxhp = 100; city[i].tint = rand() % 4;
    }
    memset(parts, 0, sizeof parts);
    active = false; mask = 0; t = 0; shake = 0; volc = 0;
}
static float cityPct(void) {
    float s = 0; for (int i = 0; i < NB; i++) s += city[i].hp; return s / (NB * 100.0f) * 100.0f;
}
static float bheight(int i) {
    Bld *b = &city[i]; if (b->hp <= 0) return b->h * 0.08f;
    return b->h * (0.15f + 0.85f * b->hp / b->maxhp);
}
static void spawn(int type, float x, float y, float vx, float vy, int life) {
    for (int i = 0; i < NPART; i++) if (parts[i].life <= 0) {
        parts[i] = (Part){x, y, vx, vy, life, life, type}; return; }
}
static void hit(int i, float frac) {
    if (i < 0 || i >= NB) return;
    Bld *b = &city[i]; if (b->hp <= 0) return;
    b->hp -= frac * b->maxhp;
    if (b->hp <= 0) {
        b->hp = 0;
        for (int k = 0; k < 12; k++) spawn(P_DEBRIS, i*BW+BW/2, GROUND-b->h/2, frand(-2,2), frand(-4,-1), 70);
        for (int k = 0; k < 4; k++) spawn(P_SMOKE, i*BW+BW/2, GROUND-10, frand(-.3f,.3f), frand(-1,-.4f), 90);
    }
}
static void blast(float x, float r, float frac) {
    for (int i = 0; i < NB; i++) {
        float d = fabsf(i*BW + BW/2 - x);
        if (d < r) hit(i, frac * (1.0f - d / (r * 1.3f)));
    }
}
static void unleash(void) {
    if (active || nsel == 0) return;
    mask = 0; for (int i = 0; i < nsel; i++) mask |= 1 << order[i];
    gmult = nsel > 1 ? 1.6f : 1.0f;
    if (nsel > 1) snprintf(curname, sizeof curname, "%s", PAIR[order[0]][order[1]]);
    else snprintf(curname, sizeof curname, "Pure %s", ENAME[order[0]]);
    active = true; t = 0; tx = cx; wx = -40;
}

static void update(void) {
    const float dt = 1.0f / 60.0f;
    frame++;
    gloom += ((active ? 1.0f : 0.0f) - gloom) * 0.03f;
    float vt = (active && (mask & (1 << E_VOLCANO))) ? 1.0f : 0.0f;
    volc += (vt - volc) * 0.04f;
    if (shake > 0) { shake -= 0.25f; if (shake < 0) shake = 0; }
    shx = shake > 0 ? frand(-shake, shake) : 0; shy = shake > 0 ? frand(-shake, shake) : 0;

    if (active) {
        t += dt;
        float m = gmult;
        if (mask & (1 << E_QUAKE)) {
            shake = 4.5f;
            for (int i = 0; i < NB; i++) hit(i, 0.06f * (city[i].h / 60.0f) * m * dt);
            if (frame % 8 == 0) { int i = rand() % NB; if (city[i].hp > 0) spawn(P_DEBRIS, i*BW+10, GROUND-bheight(i), frand(-1,1), frand(-2,0), 60); }
        }
        if (mask & (1 << E_TORNADO)) {
            float target = cx + sinf(t * 1.3f) * 110.0f;
            tx += (target - tx) * 0.03f;
            for (int i = 0; i < NB; i++) if (fabsf(i*BW + 10 - tx) < 30) hit(i, 0.35f * m * dt);
            if (frame % 4 == 0) spawn(P_DEBRIS, tx + frand(-15,15), GROUND-5, frand(-1,1), frand(-4,-2), 80);
        }
        if (mask & (1 << E_TSUNAMI)) {
            wx = -40 + (t / DURATION) * (SW + 120);
            for (int i = 0; i < NB; i++) { float bx = i*BW + 10; if (bx < wx && bx > wx - 70) hit(i, 0.55f * m * dt); }
            if (frame % 3 == 0) spawn(P_WATER, wx + frand(-6,6), GROUND-48, frand(-1,2), frand(-3,-1), 40);
        }
        if (mask & (1 << E_VOLCANO)) {
            if (frame % 3 == 0) spawn(P_LAVA, cx, GROUND-85*volc, frand(-1.8f,1.8f), frand(-6.5f,-3.5f), 220);
            if (frame % 10 == 0) spawn(P_SMOKE, cx, GROUND-85*volc, frand(-.4f,.4f), -1.0f, 100);
        }
        if (mask & (1 << E_METEOR)) {
            if (frame % 12 == 0) spawn(P_ROCK, frand(cx-140, cx+140), -10, frand(.5f,2.0f), frand(4,6), 400);
        }
        if (mask & (1 << E_LIGHT)) {
            if (frame % 18 == 0) {
                float x = frand(cx-110, cx+110); if (x < 0) x = 0; if (x > SW-1) x = SW-1;
                float f = 0.4f * m;
                if ((mask & (1 << E_TSUNAMI)) && x < wx && x > wx - 70) f *= 2.0f; // wet = shocking
                hit((int)(x / BW), f);
                spawn(P_BOLT, x, 0, 0, 0, 8);
                spawn(P_BOOM, x, GROUND - bheight((int)(x/BW)), 0, 0, 14);
            }
        }
        if (t >= DURATION) active = false;
    }

    for (int i = 0; i < NPART; i++) {
        Part *p = &parts[i]; if (p->life <= 0) continue;
        p->life--;
        if (p->type == P_BOLT || p->type == P_BOOM) continue;
        // tornado carries stuff around (mixes with meteor/volcano)
        if (active && (mask & (1 << E_TORNADO)) && (p->type == P_ROCK || p->type == P_LAVA || p->type == P_DEBRIS)) {
            float dx = tx - p->x;
            if (fabsf(dx) < 70 && p->y > GROUND - 160) { p->vx += dx * 0.012f + sinf(t*8) * 0.1f; p->vy -= 0.16f; }
        }
        float g = (p->type == P_ROCK) ? .15f : (p->type == P_SMOKE ? -.02f : .12f);
        p->vy += g; p->x += p->vx; p->y += p->vy;
        if (p->x < -20 || p->x > SW + 20 || p->y > SH) { p->life = 0; continue; }
        if (p->type == P_ROCK || p->type == P_LAVA) {
            int bi = (int)(p->x / BW);
            float top = (bi >= 0 && bi < NB) ? GROUND - bheight(bi) : GROUND;
            if (p->y >= top) {
                if (p->type == P_ROCK) { blast(p->x, 32, 0.5f * gmult); spawn(P_BOOM, p->x, p->y, 0, 0, 20); shake = 6; for (int k=0;k<5;k++) spawn(P_DEBRIS,p->x,p->y,frand(-2,2),frand(-4,-1),50); }
                else { hit(bi, 0.14f * gmult); spawn(P_SMOKE, p->x, p->y, 0, -.5f, 40); }
                p->life = 0;
            }
        }
    }
}

// ---------- drawing helpers (parallax aware) ----------
static void layer(float depth, float z) { g_off = g_sgn * slider * depth; g_z = z; }
static void R(float x, float y, float w, float h, u32 c) { C2D_DrawRectSolid(x + g_off + shx, y + shy, g_z, w, h, c); }
static void Ci(float x, float y, float r, u32 c) { C2D_DrawCircleSolid(x + g_off + shx, y + shy, g_z, r, c); }
static void Tri(float x1,float y1,float x2,float y2,float x3,float y3,u32 c) {
    float o = g_off + shx;
    C2D_DrawTriangle(x1+o,y1+shy,c, x2+o,y2+shy,c, x3+o,y3+shy,c, g_z);
}
static void text(const char *s, float x, float y, float sc, u32 col, u32 flags) {
    C2D_Text tx_; C2D_TextParse(&tx_, tbuf, s); C2D_TextOptimize(&tx_);
    C2D_DrawText(&tx_, C2D_WithColor | flags, x + g_off, y, g_z, sc, sc, col);
}

static void draw_top(int sgn) {
    g_sgn = sgn;
    u32 white = C2D_Color32(255,255,255,255);

    // sky (far, behind the screen)
    layer(-8, 0.10f);
    u32 sT = lerpc(100,160,230, 35,25,45, gloom), sB = lerpc(190,225,255, 120,60,60, gloom);
    C2D_DrawRectangle(-30 + g_off, -10, g_z, SW + 60, SH + 20, sT, sT, sB, sB);
    // clouds
    layer(-6, 0.15f);
    u32 cc = lerpc(240,240,250, 70,60,80, gloom);
    for (int i = 0; i < 6; i++) {
        float x = fmodf(frame * 0.12f + i * 90, SW + 120) - 60, y = 28 + (i % 3) * 20;
        Ci(x, y, 14, cc); Ci(x+16, y+4, 11, cc); Ci(x-16, y+5, 10, cc);
    }
    // volcano mountain
    if (volc > 0.02f) {
        layer(-2, 0.18f);
        Tri(cx-75, GROUND, cx+75, GROUND, cx, GROUND - 90*volc, C2D_Color32(90,60,45,255));
        Tri(cx-12, GROUND-80*volc, cx+12, GROUND-80*volc, cx, GROUND-92*volc, C2D_Color32(255,110,20,255));
    }
    // far skyline
    layer(-3, 0.20f);
    u32 fs = lerpc(120,140,170, 50,40,60, gloom);
    for (int i = 0; i < 25; i++) { int h = 30 + (i * 37) % 35; R(i*17, GROUND - h, 15, h, fs); }

    // ground
    layer(0, 0.30f);
    R(-30, GROUND, SW + 60, SH - GROUND + 30, lerpc(70,140,70, 45,60,50, gloom));
    if (active && (mask & (1 << E_QUAKE)))
        for (int k = 0; k < 14; k++) R(cx - 56 + k*8, GROUND + 2 + (k % 3) * 5, 6, 2, C2D_Color32(10,10,10,255));

    // buildings (main plane, pops out of the screen)
    layer(2, 0.40f);
    static const int pal[4][3] = {{150,160,185},{190,150,130},{130,170,160},{175,175,120}};
    for (int i = 0; i < NB; i++) {
        float bh = bheight(i); float x = i * BW + 1;
        u32 body = city[i].hp > 0 ? lerpc(pal[city[i].tint][0],pal[city[i].tint][1],pal[city[i].tint][2], 60,50,60, gloom*0.5f)
                                  : C2D_Color32(80,70,65,255);
        R(x, GROUND - bh, BW - 2, bh, body);
        if (city[i].hp > 0)
            for (float wy = GROUND - bh + 6; wy < GROUND - 8; wy += 10)
                for (int c = 0; c < 2; c++) {
                    bool lit = ((i * 7 + (int)wy + c) % 3) != 0;
                    R(x + 3 + c*7, wy, 4, 5, lit ? C2D_Color32(255,230,140,255) : C2D_Color32(50,60,80,255));
                }
    }

    // tsunami
    if (active && (mask & (1 << E_TSUNAMI))) {
        layer(5, 0.50f);
        u32 w = C2D_Color32(40,110,220,200);
        R(wx - 70, GROUND - 48, 70, 48, w);
        for (int k = 0; k < 6; k++) Ci(wx - k*3, GROUND - 48 + k*7, 14 - k, C2D_Color32(190,230,255,220));
    }

    // particles
    layer(7, 0.60f);
    for (int i = 0; i < NPART; i++) {
        Part *p = &parts[i]; if (p->life <= 0) continue;
        float k = p->life / (float)p->maxlife; u8 a = (u8)(255 * k);
        switch (p->type) {
            case P_ROCK:  Ci(p->x, p->y, 4, C2D_Color32(110,85,70,255)); Ci(p->x-1, p->y-1, 2, C2D_Color32(255,120,30,255)); break;
            case P_LAVA:  Ci(p->x, p->y, 2.6f, C2D_Color32(255,120,20,255)); break;
            case P_DEBRIS:R(p->x, p->y, 3, 3, C2D_Color32(90,85,80,255)); break;
            case P_WATER: Ci(p->x, p->y, 2, C2D_Color32(170,215,255,255)); break;
            case P_SMOKE: Ci(p->x, p->y, 3 + (1-k)*6, C2D_Color32(70,70,75,(u8)(a*0.7f))); break;
            case P_BOOM:  Ci(p->x, p->y, 4 + (1-k)*28, C2D_Color32(255,170,40,a)); Ci(p->x, p->y, 2 + (1-k)*14, C2D_Color32(255,240,160,a)); break;
            case P_BOLT:
                for (int s = 0; s < 10; s++) R(p->x + ((s*53 + p->life*17) % 11) - 5, s*20, 3, 22, C2D_Color32(255,255,200,255));
                break;
        }
    }

    // tornado (closest world object)
    if (active && (mask & (1 << E_TORNADO))) {
        layer(9, 0.70f);
        for (int k = 0; k < 14; k++) {
            float yk = GROUND - k * 13, wk = 10 + k * 5, xk = tx + sinf(t * 9 + k * 0.6f) * (4 + k * 0.6f);
            u32 col = (mask & (1 << E_VOLCANO)) ? C2D_Color32(210,90+k*6,30,225)
                    : (mask & (1 << E_TSUNAMI)) ? C2D_Color32(80,130+k*5,210,225)
                    : C2D_Color32(105+k*5,105+k*5,115+k*5,225);
            R(xk - wk/2, yk - 12, wk, 13, col);
        }
    }

    // HUD, floats in front of the screen
    layer(12, 0.90f);
    Tri(cx - 6, 4, cx + 6, 4, cx, 15, C2D_Color32(255,220,40,255));
    char buf[64];
    if (active) {
        text(curname, 8, 8, 0.6f, white, 0);
        R(8, 28, 120 * (1 - t / DURATION), 4, C2D_Color32(255,90,60,255));
    } else if (cityPct() < 3.0f) {
        text("CITY DESTROYED!", SW/2, 90, 0.9f, C2D_Color32(255,80,60,255), C2D_AlignCenter);
        text("Press Y to rebuild", SW/2, 125, 0.5f, white, C2D_AlignCenter);
    } else {
        text("Aim with D-Pad / Circle Pad", SW/2, 8, 0.45f, white, C2D_AlignCenter);
    }
    snprintf(buf, sizeof buf, "Destroyed: %d%%", (int)(100 - cityPct()));
    text(buf, SW - 8, 8, 0.5f, white, C2D_AlignRight);
}

// ---------- bottom screen ----------
static bool inside(int px, int py, int x, int y, int w, int h) { return px >= x && px < x+w && py >= y && py < y+h; }
static void draw_bottom(void) {
    g_off = 0; shx = shy = 0; g_z = 0.5f;
    u32 white = C2D_Color32(255,255,255,255);
    C2D_DrawRectSolid(0, 0, 0.1f, 320, 240, C2D_Color32(24,26,40,255));
    text("CATASTROPHE MIXER", 160, 4, 0.6f, C2D_Color32(255,200,60,255), C2D_AlignCenter);
    text("D-Pad aim | A unleash | Y reset | START exit", 160, 22, 0.38f, C2D_Color32(170,170,190,255), C2D_AlignCenter);
    for (int i = 0; i < E_COUNT; i++) {
        int x = 10 + (i % 3) * 104, y = 38 + (i / 3) * 46;
        bool sel = (nsel > 0 && order[0] == i) || (nsel > 1 && order[1] == i);
        if (sel) C2D_DrawRectSolid(x-3, y-3, 0.3f, 102, 46, white);
        u32 c = ECOL[i];
        if (!sel) { c = C2D_Color32(((c)&0xFF)/2, ((c>>8)&0xFF)/2, ((c>>16)&0xFF)/2, 255); }
        C2D_DrawRectSolid(x, y, 0.4f, 96, 40, c);
        text(ENAME[i], x + 48, y + 11, 0.5f, white, C2D_AlignCenter);
    }
    C2D_DrawRectSolid(10, 130, 0.3f, 300, 36, C2D_Color32(44,48,70,255));
    char buf[64];
    if (nsel == 0) text("Pick 1 or 2 disasters to mix", 160, 142, 0.5f, white, C2D_AlignCenter);
    else if (nsel == 1) { snprintf(buf, sizeof buf, "Pure %s", ENAME[order[0]]); text(buf, 160, 142, 0.55f, white, C2D_AlignCenter); }
    else {
        text(PAIR[order[0]][order[1]], 160, 133, 0.6f, C2D_Color32(255,200,60,255), C2D_AlignCenter);
        text("MIX BONUS: x1.6 damage", 160, 151, 0.4f, white, C2D_AlignCenter);
    }
    float pct = cityPct();
    snprintf(buf, sizeof buf, "City integrity: %d%%", (int)pct);
    text(buf, 10, 170, 0.42f, white, 0);
    C2D_DrawRectSolid(10, 184, 0.3f, 300, 8, C2D_Color32(60,60,70,255));
    C2D_DrawRectSolid(10, 184, 0.4f, 3.0f * pct, 8, lerpc(255,60,50, 70,220,90, pct/100.0f));
    bool can = !active && nsel > 0;
    C2D_DrawRectSolid(10, 198, 0.3f, 190, 36, can ? C2D_Color32(210,40,40,255) : C2D_Color32(80,50,50,255));
    text(active ? "DISASTER IN PROGRESS" : "UNLEASH!", 105, 207, 0.55f, white, C2D_AlignCenter);
    C2D_DrawRectSolid(210, 198, 0.3f, 100, 36, C2D_Color32(50,90,170,255));
    text("REBUILD", 260, 207, 0.55f, white, C2D_AlignCenter);
}

static void touch_press(int px, int py) {
    for (int i = 0; i < E_COUNT; i++) {
        int x = 10 + (i % 3) * 104, y = 38 + (i / 3) * 46;
        if (inside(px, py, x, y, 96, 40)) {
            for (int k = 0; k < nsel; k++) if (order[k] == i) { // deselect
                if (k == 0) order[0] = order[1];
                nsel--; return; }
            if (nsel < 2) order[nsel++] = i; else { order[0] = order[1]; order[1] = i; }
            return;
        }
    }
    if (inside(px, py, 10, 198, 190, 36)) unleash();
    if (inside(px, py, 210, 198, 100, 36)) reset_city();
}

#define P(a,b,n) PAIR[a][b] = PAIR[b][a] = n
static void init_tables(void) {
    ECOL[E_QUAKE]=C2D_Color32(150,100,60,255);   ECOL[E_TORNADO]=C2D_Color32(120,130,150,255);
    ECOL[E_TSUNAMI]=C2D_Color32(50,110,220,255); ECOL[E_VOLCANO]=C2D_Color32(220,80,30,255);
    ECOL[E_METEOR]=C2D_Color32(140,70,170,255);  ECOL[E_LIGHT]=C2D_Color32(220,200,40,255);
    P(E_QUAKE,E_TORNADO,"Cyclonic Fault");   P(E_QUAKE,E_TSUNAMI,"Seaquake Surge");
    P(E_QUAKE,E_VOLCANO,"Magma Rift");       P(E_QUAKE,E_METEOR,"Impact Fault");
    P(E_QUAKE,E_LIGHT,"Shockquake");
    P(E_TORNADO,E_TSUNAMI,"Waterspout Apocalypse"); P(E_TORNADO,E_VOLCANO,"Lavanado");
    P(E_TORNADO,E_METEOR,"Meteornado");      P(E_TORNADO,E_LIGHT,"Stormfury Vortex");
    P(E_TSUNAMI,E_VOLCANO,"Steam Cataclysm"); P(E_TSUNAMI,E_METEOR,"Impact Megatsunami");
    P(E_TSUNAMI,E_LIGHT,"Thunder Tide");
    P(E_VOLCANO,E_METEOR,"Hellfire Rain");   P(E_VOLCANO,E_LIGHT,"Volcanic Tempest");
    P(E_METEOR,E_LIGHT,"Skyfall Storm");
}

int main(void) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    gfxSet3D(true);   // enable stereoscopic 3D on the top screen
    C3D_RenderTarget *topL = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *topR = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    C3D_RenderTarget *bot  = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    tbuf = C2D_TextBufNew(4096);
    srand(time(NULL));
    init_tables(); reset_city();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kd = hidKeysDown(), kh = hidKeysHeld();
        if (kd & KEY_START) break;
        if (kh & KEY_DLEFT)  cx -= 2.5f;
        if (kh & KEY_DRIGHT) cx += 2.5f;
        circlePosition cp; hidCircleRead(&cp);
        if (abs(cp.dx) > 15) cx += cp.dx / 50.0f;
        if (cx < 10) cx = 10; if (cx > SW - 10) cx = SW - 10;
        if (kd & KEY_A) unleash();
        if (kd & KEY_Y) reset_city();
        if (kd & KEY_TOUCH) { touchPosition tp; hidTouchRead(&tp); touch_press(tp.px, tp.py); }

        slider = osGet3DSliderState();
        update();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TextBufClear(tbuf);
        C2D_TargetClear(topL, C2D_Color32(0,0,0,255)); C2D_SceneBegin(topL); draw_top(+1);
        C2D_TargetClear(topR, C2D_Color32(0,0,0,255)); C2D_SceneBegin(topR); draw_top(-1);
        C2D_TargetClear(bot,  C2D_Color32(0,0,0,255)); C2D_SceneBegin(bot);  draw_bottom();
        C3D_FrameEnd(0);
    }
    C2D_TextBufDelete(tbuf);
    C2D_Fini(); C3D_Fini(); gfxExit();
    return 0;
}
