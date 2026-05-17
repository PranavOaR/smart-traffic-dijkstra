#include "traffic.h"

/* Min-heap node and heap — internal to this file only. */
typedef struct { int id, dist; } HNode;
typedef struct { int size; HNode arr[MAX_NODES * 2]; } Heap;

static void sift_up(Heap *h, int i)
{
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->arr[p].dist <= h->arr[i].dist) break;
        HNode t = h->arr[p]; h->arr[p] = h->arr[i]; h->arr[i] = t;
        i = p;
    }
}

static void sift_down(Heap *h, int i)
{
    for (;;) {
        int s = i, l = 2*i+1, r = 2*i+2;
        if (l < h->size && h->arr[l].dist < h->arr[s].dist) s = l;
        if (r < h->size && h->arr[r].dist < h->arr[s].dist) s = r;
        if (s == i) break;
        HNode t = h->arr[s]; h->arr[s] = h->arr[i]; h->arr[i] = t;
        i = s;
    }
}

static void push(Heap *h, int id, int d)
{
    if (h->size >= MAX_NODES * 2) return;
    h->arr[h->size].id   = id;
    h->arr[h->size].dist = d;
    sift_up(h, h->size++);
}

static HNode pop(Heap *h)
{
    HNode m  = h->arr[0];
    h->arr[0] = h->arr[--h->size];
    if (h->size > 0) sift_down(h, 0);
    return m;
}

/* Single-source shortest paths. O((V+E) log V). */
int dijkstra(const Graph *g, int src, DijkstraResult *r)
{
    if (src < 0 || src >= g->num_nodes) return -1;

    for (int i = 0; i < g->num_nodes; i++) {
        r->dist[i] = INF; r->prev[i] = -1; r->visited[i] = 0;
    }
    r->dist[src] = 0;

    Heap h = {0};
    push(&h, src, 0);

    while (h.size > 0) {
        HNode hn = pop(&h);
        int u = hn.id;
        if (r->visited[u]) continue;   /* lazy-deletion: skip stale entry */
        r->visited[u] = 1;

        for (Edge *e = g->nodes[u].head; e; e = e->next) {
            int v = e->dest;
            if (!r->visited[v] && r->dist[u] != INF &&
                r->dist[u] + e->weight < r->dist[v]) {
                r->dist[v] = r->dist[u] + e->weight;
                r->prev[v] = u;
                push(&h, v, r->dist[v]);
            }
        }
    }
    return 0;
}

/* Walk prev[] backwards from dest to src, then reverse into path[]. */
int reconstruct_path(const DijkstraResult *r, int src, int dest,
                     int path[], int *len)
{
    if (r->dist[dest] == INF) { *len = 0; return 0; }
    int tmp[MAX_NODES], n = 0, cur = dest;
    while (cur != -1 && n < MAX_NODES) {
        tmp[n++] = cur;
        if (cur == src) break;
        cur = r->prev[cur];
    }
    for (int i = 0; i < n; i++) path[i] = tmp[n - 1 - i];
    *len = n;
    return 1;
}
