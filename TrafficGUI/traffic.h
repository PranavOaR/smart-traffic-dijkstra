#ifndef TRAFFIC_H
#define TRAFFIC_H

#define MAX_NODES    50
#define MAX_EDGES    200
#define MAX_NAME_LEN 50
#define INF          0x7FFFFFFF

typedef struct Edge {
    int dest;
    int weight;
    int base_distance;
    int congestion;
    int speed_limit;
    struct Edge *next;
} Edge;

typedef struct {
    int  id;
    char name[MAX_NAME_LEN];
    Edge *head;
} Node;

typedef struct {
    int  num_nodes;
    int  num_edges;
    Node nodes[MAX_NODES];
} Graph;

typedef struct {
    int dist[MAX_NODES];
    int prev[MAX_NODES];
    int visited[MAX_NODES];
} DijkstraResult;

/* graph.c */
void init_graph(Graph *g);
int  add_node(Graph *g, const char *name);
int  add_edge(Graph *g, int src, int dest, int dist, int cong, int speed);
int  update_edge_weight(Graph *g, int src, int dest, int cong, int speed);
int  find_node_by_name(const Graph *g, const char *name);
void free_graph(Graph *g);
void load_demo_map(Graph *g);
int  compute_weight(int dist, int cong, int speed);

/* dijkstra.c */
int  dijkstra(const Graph *g, int source, DijkstraResult *result);
int  reconstruct_path(const DijkstraResult *r, int src, int dest,
                      int path[], int *len);

/* gui.c */
void run_gui(Graph *g);

#endif /* TRAFFIC_H */
