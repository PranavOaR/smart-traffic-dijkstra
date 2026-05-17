#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "traffic.h"

int compute_weight(int dist, int cong, int speed)
{
    int penalty = (80 - speed) / 10;
    if (penalty < 0) penalty = 0;
    return dist + cong * 5 + penalty;
}

void init_graph(Graph *g)
{
    g->num_nodes = 0;
    g->num_edges = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        g->nodes[i].id      = i;
        g->nodes[i].name[0] = '\0';
        g->nodes[i].head    = NULL;
    }
}

int add_node(Graph *g, const char *name)
{
    if (g->num_nodes >= MAX_NODES) return -1;
    int id = g->num_nodes++;
    g->nodes[id].id   = id;
    g->nodes[id].head = NULL;
    strncpy(g->nodes[id].name, name, MAX_NAME_LEN - 1);
    g->nodes[id].name[MAX_NAME_LEN - 1] = '\0';
    return id;
}

int add_edge(Graph *g, int src, int dest, int dist, int cong, int speed)
{
    if (src < 0 || src >= g->num_nodes)   return -1;
    if (dest < 0 || dest >= g->num_nodes) return -1;
    if (g->num_edges >= MAX_EDGES)         return -1;

    Edge *e = malloc(sizeof(Edge));
    if (!e) return -1;
    e->dest          = dest;
    e->base_distance = dist;
    e->congestion    = cong;
    e->speed_limit   = speed;
    e->weight        = compute_weight(dist, cong, speed);
    e->next          = g->nodes[src].head;
    g->nodes[src].head = e;
    g->num_edges++;
    return 0;
}

int update_edge_weight(Graph *g, int src, int dest, int cong, int speed)
{
    if (src < 0 || src >= g->num_nodes) return -1;
    for (Edge *e = g->nodes[src].head; e; e = e->next) {
        if (e->dest == dest) {
            e->congestion  = cong;
            e->speed_limit = speed;
            e->weight      = compute_weight(e->base_distance, cong, speed);
            return 0;
        }
    }
    return -1;
}

int find_node_by_name(const Graph *g, const char *name)
{
    for (int i = 0; i < g->num_nodes; i++)
        if (strcmp(g->nodes[i].name, name) == 0) return i;
    return -1;
}

void free_graph(Graph *g)
{
    for (int i = 0; i < g->num_nodes; i++) {
        Edge *e = g->nodes[i].head;
        while (e) { Edge *tmp = e->next; free(e); e = tmp; }
        g->nodes[i].head = NULL;
    }
    g->num_nodes = 0;
    g->num_edges = 0;
}

void load_demo_map(Graph *g)
{
    free_graph(g);
    init_graph(g);

    add_node(g, "City Hall");        /* 0 */
    add_node(g, "Market Street");    /* 1 */
    add_node(g, "Central Park");     /* 2 */
    add_node(g, "Train Station");    /* 3 */
    add_node(g, "Bus Terminal");     /* 4 */
    add_node(g, "Airport");          /* 5 */
    add_node(g, "University");       /* 6 */
    add_node(g, "Hospital");         /* 7 */
    add_node(g, "Shopping Mall");    /* 8 */
    add_node(g, "Residential Zone"); /* 9 */

    add_edge(g, 0, 1, 3,  7, 50);
    add_edge(g, 0, 2, 5,  3, 80);
    add_edge(g, 1, 3, 4,  9, 40);
    add_edge(g, 1, 4, 2,  6, 60);
    add_edge(g, 2, 1, 2,  4, 70);
    add_edge(g, 2, 6, 7,  2, 90);
    add_edge(g, 3, 5, 10, 5, 80);
    add_edge(g, 3, 7, 3,  8, 50);
    add_edge(g, 4, 3, 3,  5, 60);
    add_edge(g, 4, 9, 6,  3, 70);
    add_edge(g, 5, 8, 8,  2, 100);
    add_edge(g, 6, 7, 4,  3, 80);
    add_edge(g, 6, 8, 5,  4, 70);
    add_edge(g, 7, 9, 4,  6, 50);
    add_edge(g, 7, 8, 6,  7, 60);
    add_edge(g, 8, 9, 3,  5, 70);
    add_edge(g, 9, 0, 8,  2, 80);
    add_edge(g, 9, 4, 4,  3, 60);
}
