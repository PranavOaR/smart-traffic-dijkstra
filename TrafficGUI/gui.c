#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include "traffic.h"

#define WIN_W   1100
#define WIN_H    720
#define PANEL_X  790
#define NODE_R    26

/* Fixed canvas positions for each of the 10 demo intersections. */
static const Vector2 POS[10] = {
    {390,300}, /* 0 City Hall        */
    {200,180}, /* 1 Market Street    */
    {580,160}, /* 2 Central Park     */
    {100,380}, /* 3 Train Station    */
    {310,500}, /* 4 Bus Terminal     */
    {130,560}, /* 5 Airport          */
    {660,300}, /* 6 University       */
    {510,470}, /* 7 Hospital         */
    {620,560}, /* 8 Shopping Mall    */
    {380,620}, /* 9 Residential Zone */
};

/* ── Colours ─────────────────────────────────────────── */
static const Color BG     = {15,  20,  35,  255};
static const Color PANEL  = {20,  28,  50,  255};
static const Color EDGE_C = {80, 100, 140,  255};
static const Color NODE_C = {40,  80, 160,  255};
static const Color SRC_C  = {30, 160,  60,  255};
static const Color DST_C  = {190, 40,  40,  255};
static const Color PATH_C = {200,130,  20,  255};
static const Color TEXT_C = {200,210, 230,  255};
static const Color LINE_C = { 60, 80, 130,  255};

/* ── Font helpers ─────────────────────────────────────── */
static Font F;    /* loaded in run_gui; used by all text calls */
static bool custom_font_loaded = false;

static void txt(const char *s, float x, float y, float sz, Color c)
{
    DrawTextEx(F, s, (Vector2){x, y}, sz, 1, c);
}

/* Centre-aligned text at (cx, cy). */
static void txt_c(const char *s, float cx, float cy, float sz, Color c)
{
    Vector2 dim = MeasureTextEx(F, s, sz, 1);
    DrawTextEx(F, s, (Vector2){cx - dim.x/2, cy - dim.y/2}, sz, 1, c);
}

static float tw(const char *s, float sz)
{
    return MeasureTextEx(F, s, sz, 1).x;
}

typedef enum {
    SELECT_SOURCE,
    SELECT_DESTINATION
} SelectMode;

/* ── UI helpers ──────────────────────────────────────── */

static bool ui_button_ex(Rectangle r, const char *label, bool active)
{
    bool hot = CheckCollisionPointRec(GetMousePosition(), r);
    Color fill = active ? (Color){70,120,210,255} :
                 hot    ? (Color){60,100,200,255} :
                          (Color){35,55,110,255};
    DrawRectangleRec(r, fill);
    DrawRectangleLinesEx(r, active ? 2 : 1, (Color){80,130,255,255});
    float lw = tw(label, 15);
    txt(label, r.x + (r.width - lw)/2, r.y + (r.height - 15)/2, 15, WHITE);
    return hot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool ui_button(Rectangle r, const char *label)
{
    return ui_button_ex(r, label, false);
}

/* Draw directed arrow between node circle edges. */
static void draw_arrow(Vector2 a, Vector2 b, Color col, float thick)
{
    float dx = b.x-a.x, dy = b.y-a.y;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 1.0f) return;
    float ux = dx/len, uy = dy/len;

    Vector2 p1 = {a.x + ux*NODE_R, a.y + uy*NODE_R};
    Vector2 p2 = {b.x - ux*NODE_R, b.y - uy*NODE_R};
    DrawLineEx(p1, p2, thick, col);

    /* Arrowhead wings at ±30° from reversed direction. */
    Vector2 w1 = {p2.x + (-ux*0.866f + uy*0.5f)*14,
                  p2.y + (-ux*0.5f   - uy*0.866f)*14};
    Vector2 w2 = {p2.x + (-ux*0.866f - uy*0.5f)*14,
                  p2.y + ( ux*0.5f   - uy*0.866f)*14};
    DrawLineEx(p2, w1, thick, col);
    DrawLineEx(p2, w2, thick, col);
}

/* Weight label: dark pill offset perpendicularly from the edge midpoint. */
static void draw_weight_label(Vector2 a, Vector2 b, int weight, bool on_path)
{
    float dx = b.x-a.x, dy = b.y-a.y;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 1.0f) return;

    /* Perpendicular direction (rotated 90°), offset 14px to the left of the edge. */
    float px = -(dy/len) * 14;
    float py =  (dx/len) * 14;
    float cx = (a.x + b.x)/2 + px;
    float cy = (a.y + b.y)/2 + py;

    char buf[8]; snprintf(buf, sizeof(buf), "%d", weight);
    float fs  = 13.0f;
    float lw  = tw(buf, fs);
    float pad = 5.0f;

    /* Dark pill background for readability over any edge/node. */
    DrawRectangleRounded(
        (Rectangle){cx - lw/2 - pad, cy - fs/2 - pad/2, lw + pad*2, fs + pad},
        0.6f, 6, (Color){8, 12, 25, 220});
    txt_c(buf, cx, cy + 1, fs, on_path ? YELLOW : (Color){180,200,235,255});
}

static int hit_node(Vector2 m, int n)
{
    for (int i = 0; i < n && i < 10; i++) {
        float dx = m.x - POS[i].x, dy = m.y - POS[i].y;
        if (dx*dx + dy*dy <= (float)(NODE_R * NODE_R)) return i;
    }
    return -1;
}

/* ── Main loop ────────────────────────────────────────── */
void run_gui(Graph *g)
{
    InitWindow(WIN_W, WIN_H, "Smart Traffic Analysis - Dijkstra");
    SetTargetFPS(60);

    F = GetFontDefault();
#ifdef __APPLE__
    /* Use the macOS system font when present; other platforms use raylib's default font. */
    Font mac_font = LoadFontEx("/System/Library/Fonts/SFNS.ttf", 48, NULL, 0);
    if (mac_font.texture.id != 0) {
        F = mac_font;
        custom_font_loaded = true;
        SetTextureFilter(F.texture, TEXTURE_FILTER_BILINEAR);
    }
#endif

    int  src = -1, dst = -1;
    int  path[MAX_NODES], path_len = 0;
    bool has_path = false;
    int  cached_src = -2;
    SelectMode select_mode = SELECT_SOURCE;
    DijkstraResult res, all_res;

    load_demo_map(g);

    while (!WindowShouldClose()) {

        /* ── Input ──────────────────────────────────────── */
        Vector2 m = GetMousePosition();
        if (m.x < PANEL_X) {
            int hit = hit_node(m, g->num_nodes);
            if (hit >= 0) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (select_mode == SELECT_SOURCE) {
                        src = hit;
                        if (dst == src) dst = -1;
                        select_mode = SELECT_DESTINATION;
                    } else {
                        dst = hit;
                        if (src == dst) src = -1;
                    }
                    has_path = false;
                    path_len = 0;
                }
            }
        }

        if (src >= 0 && dst >= 0 && src != dst && !has_path) {
            dijkstra(g, src, &res);
            has_path = reconstruct_path(&res, src, dst, path, &path_len);
        }
        if (src >= 0 && src != cached_src) {
            dijkstra(g, src, &all_res);
            cached_src = src;
        }

        /* ── Draw ───────────────────────────────────────── */
        BeginDrawing();
        ClearBackground(BG);
        DrawRectangle(PANEL_X, 0, WIN_W - PANEL_X, WIN_H, PANEL);
        DrawRectangle(PANEL_X, 0, 1, WIN_H, LINE_C);

        /* ─ Edges ─ */
        for (int u = 0; u < g->num_nodes && u < 10; u++) {
            for (Edge *e = g->nodes[u].head; e; e = e->next) {
                int v = e->dest;
                if (v >= 10) continue;

                bool on = false;
                for (int k = 0; k+1 < path_len; k++)
                    if (path[k]==u && path[k+1]==v) { on=true; break; }

                draw_arrow(POS[u], POS[v], on ? ORANGE : EDGE_C, on ? 3.5f : 1.5f);
                draw_weight_label(POS[u], POS[v], e->weight, on);
            }
        }

        /* ─ Nodes ─ */
        for (int i = 0; i < g->num_nodes && i < 10; i++) {
            bool on = false;
            for (int k = 0; k < path_len; k++) if (path[k]==i) { on=true; break; }

            Color fill = (i==src) ? SRC_C : (i==dst) ? DST_C :
                         on       ? PATH_C : NODE_C;
            DrawCircleV(POS[i], NODE_R, fill);
            DrawCircleLines((int)POS[i].x, (int)POS[i].y, NODE_R,
                            (Color){160,200,255,255});

            char id[4]; snprintf(id, sizeof(id), "%d", i);
            txt_c(id, POS[i].x, POS[i].y, 18, WHITE);
            txt_c(g->nodes[i].name,
                  POS[i].x, POS[i].y + NODE_R + 10, 12, TEXT_C);
        }

        /* ─ Side Panel ─ */
        float panX = PANEL_X + 14, panY = 18;

        txt("Traffic Analysis",        panX, panY,    20, (Color){120,170,255,255});
        txt("Dijkstra  |  Min-Heap",   panX, panY+26, 13, (Color){100,120,160,255});
        panY += 64;

        txt("Select Mode", panX, panY, 13, TEXT_C); panY += 20;
        if (ui_button_ex((Rectangle){panX,(float)panY, 132,34}, "Source",
                         select_mode == SELECT_SOURCE)) {
            select_mode = SELECT_SOURCE;
        }
        if (ui_button_ex((Rectangle){panX+146,(float)panY, 132,34}, "Destination",
                         select_mode == SELECT_DESTINATION)) {
            select_mode = SELECT_DESTINATION;
        }
        panY += 44;
        txt("Click any node to set the active selection.", panX, panY, 12,
            (Color){120,140,180,255}); panY += 24;
        DrawLine(PANEL_X+8, (int)panY, WIN_W-8, (int)panY, LINE_C); panY += 12;

        txt("Source",      panX, panY, 13, (Color){80,200,110,255}); panY += 20;
        txt(src>=0 ? g->nodes[src].name : "—", panX+8, panY, 15, WHITE); panY += 26;

        txt("Destination", panX, panY, 13, (Color){220,90,90,255});  panY += 20;
        txt(dst>=0 ? g->nodes[dst].name : "—", panX+8, panY, 15, WHITE); panY += 30;
        DrawLine(PANEL_X+8, (int)panY, WIN_W-8, (int)panY, LINE_C); panY += 12;

        if (has_path) {
            txt("Shortest Path", panX, panY, 14, ORANGE); panY += 22;
            for (int k = 0; k < path_len; k++) {
                char line[60];
                snprintf(line, sizeof(line), "%s%s",
                    g->nodes[path[k]].name, k < path_len-1 ? "  ->" : "");
                txt(line, panX+6, panY, 13, WHITE); panY += 18;
            }
            char wl[32]; snprintf(wl, sizeof(wl), "Total weight:  %d", res.dist[dst]);
            txt(wl, panX, panY, 15, YELLOW); panY += 32;
        } else if (src>=0 && dst>=0 && src!=dst) {
            txt("No path found.", panX, panY, 14, (Color){200,80,80,255}); panY += 32;
        } else {
            txt("Select source & destination.", panX, panY, 13,
                (Color){120,140,180,255}); panY += 32;
        }

        DrawLine(PANEL_X+8, (int)panY, WIN_W-8, (int)panY, LINE_C); panY += 12;

        if (ui_button((Rectangle){panX,(float)panY, 278,36}, "Load Demo Map")) {
            load_demo_map(g); src=dst=-1; has_path=false; path_len=0; cached_src=-2;
            select_mode = SELECT_SOURCE;
        }
        panY += 46;
        if (ui_button((Rectangle){panX,(float)panY, 278,36}, "Clear Selection")) {
            src=dst=-1; has_path=false; path_len=0;
            select_mode = SELECT_SOURCE;
        }
        panY += 48;

        /* All-distances table. */
        if (src >= 0) {
            DrawLine(PANEL_X+8, (int)panY, WIN_W-8, (int)panY, LINE_C); panY += 10;
            txt("All distances from source", panX, panY, 13,
                (Color){100,160,255,255}); panY += 20;
            for (int i = 0; i < g->num_nodes; i++) {
                if (i == src) continue;
                txt(g->nodes[i].name, panX+4, panY, 12, TEXT_C);
                if (all_res.dist[i] == INF) {
                    txt("INF", panX + 260 - tw("INF", 12), panY, 12,
                        (Color){150,100,100,255});
                } else {
                    char val[16]; snprintf(val, sizeof(val), "%d", all_res.dist[i]);
                    txt(val, panX + 260 - tw(val, 12), panY, 12, YELLOW);
                }
                panY += 16;
                if (panY > WIN_H - 16) break;
            }
        }

        EndDrawing();
    }

    if (custom_font_loaded) UnloadFont(F);
    free_graph(g);
    CloseWindow();
}
