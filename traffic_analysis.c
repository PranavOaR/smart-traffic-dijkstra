/*
 * =============================================================================
 * Project  : Smart Traffic Analysis System
 * Author   : [Author Placeholder]
 * Subject  : Design and Analysis of Algorithms (DAA)
 * Date     : 2026-04-27
 * =============================================================================
 * Description:
 *   Models a city road network as a weighted directed graph and applies
 *   Dijkstra's algorithm (binary min-heap, lazy deletion) to find shortest
 *   paths between intersections. Edge weights are composite values derived
 *   from base distance, congestion level, and speed limit.
 * =============================================================================
 */

/* =============================================================================
 * Section 2: Includes
 * ============================================================================= */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* =============================================================================
 * Section 3: Macros / Constants
 * ============================================================================= */
#define MAX_NODES    50
#define MAX_EDGES    200
#define MAX_NAME_LEN 50
#define INF          INT_MAX

/* =============================================================================
 * Section 4: Struct Typedefs
 * ============================================================================= */
typedef struct Edge {
    int dest;
    int weight;          /* composite traffic weight */
    int base_distance;
    int congestion;      /* 1 (free flow) to 10 (gridlock) */
    int speed_limit;     /* km/h */
    struct Edge *next;
} Edge;

typedef struct Node {
    int  id;
    char name[MAX_NAME_LEN];
    Edge *head;
} Node;

typedef struct Graph {
    int  num_nodes;
    int  num_edges;
    Node nodes[MAX_NODES];
} Graph;

typedef struct HeapNode {
    int node_id;
    int dist;
} HeapNode;

typedef struct MinHeap {
    int      size;
    HeapNode arr[MAX_NODES * 2];
} MinHeap;

typedef struct DijkstraResult {
    int dist[MAX_NODES];
    int prev[MAX_NODES];
    int visited[MAX_NODES];
} DijkstraResult;

/* =============================================================================
 * Section 5: Function Prototypes
 * ============================================================================= */

/* Utility */
int  compute_weight(int base_distance, int congestion, int speed_limit);
void print_separator(void);
int  read_int(const char *prompt, int min, int max);
void read_string(const char *prompt, char *buf, int max_len);
void flush_input(void);

/* Min-heap */
void     heap_init(MinHeap *h);
int      heap_is_empty(const MinHeap *h);
void     heap_sift_up(MinHeap *h, int i);
void     heap_sift_down(MinHeap *h, int i);
void     heap_insert(MinHeap *h, int node_id, int dist);
HeapNode heap_extract_min(MinHeap *h);

/* Graph management */
void init_graph(Graph *g);
int  add_node(Graph *g, const char *name);
int  add_edge(Graph *g, int src, int dest, int base_distance, int congestion, int speed_limit);
int  update_edge_weight(Graph *g, int src, int dest, int new_congestion, int new_speed_limit);
int  find_node_by_name(const Graph *g, const char *name);
void print_graph(const Graph *g);
void free_graph(Graph *g);

/* Dijkstra */
int dijkstra(const Graph *g, int source, DijkstraResult *result);

/* Path output */
int  reconstruct_path(const DijkstraResult *result, int source, int dest, int path[], int *path_len);
void print_shortest_path(const Graph *g, const DijkstraResult *result, int source, int dest);
void print_all_distances(const Graph *g, const DijkstraResult *result, int source);

/* Demo map and CLI */
void load_demo_map(Graph *g);
void run_menu(Graph *g);

/* =============================================================================
 * Section 6: Utility Functions
 * ============================================================================= */

/*
 * compute_weight — composite traffic weight formula:
 *   weight = base_distance + (congestion * 5) + max(0, (80 - speed_limit) / 10)
 */
int compute_weight(int base_distance, int congestion, int speed_limit)
{
    int speed_penalty = (80 - speed_limit) / 10;
    if (speed_penalty < 0) speed_penalty = 0;
    return base_distance + (congestion * 5) + speed_penalty;
}

void print_separator(void)
{
    printf("------------------------------------------------------------\n");
}

/*
 * read_int — prompt, read an integer in [min, max], loop until valid.
 */
int read_int(const char *prompt, int min, int max)
{
    char buf[64];
    int  val;

    for (;;) {
        printf("%s [%d-%d]: ", prompt, min, max);
        fflush(stdout);

        if (fgets(buf, (int)sizeof(buf), stdin) == NULL) {
            /* EOF: return min as fallback */
            return min;
        }

        if (sscanf(buf, "%d", &val) == 1 && val >= min && val <= max) {
            return val;
        }
        printf("  Invalid input. Please enter an integer between %d and %d.\n", min, max);
    }
}

/*
 * read_string — prompt, read a string (fgets-based, strips trailing newline).
 */
void read_string(const char *prompt, char *buf, int max_len)
{
    printf("%s: ", prompt);
    fflush(stdout);

    if (fgets(buf, max_len, stdin) == NULL) {
        buf[0] = '\0';
        return;
    }

    /* Strip trailing newline */
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }
}

/*
 * flush_input — discard characters until newline or EOF.
 */
void flush_input(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

/* =============================================================================
 * Section 7: Min-Heap Implementation
 * ============================================================================= */

void heap_init(MinHeap *h)
{
    h->size = 0;
}

int heap_is_empty(const MinHeap *h)
{
    return h->size == 0;
}

/*
 * heap_sift_up — restore heap property upward from index i.
 */
void heap_sift_up(MinHeap *h, int i)
{
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->arr[parent].dist <= h->arr[i].dist) break;

        /* Swap */
        HeapNode tmp   = h->arr[parent];
        h->arr[parent] = h->arr[i];
        h->arr[i]      = tmp;
        i = parent;
    }
}

/*
 * heap_sift_down — restore heap property downward from index i.
 */
void heap_sift_down(MinHeap *h, int i)
{
    int n = h->size;

    for (;;) {
        int smallest = i;
        int left     = 2 * i + 1;
        int right    = 2 * i + 2;

        if (left  < n && h->arr[left].dist  < h->arr[smallest].dist) smallest = left;
        if (right < n && h->arr[right].dist < h->arr[smallest].dist) smallest = right;

        if (smallest == i) break;

        HeapNode tmp        = h->arr[smallest];
        h->arr[smallest]    = h->arr[i];
        h->arr[i]           = tmp;
        i = smallest;
    }
}

void heap_insert(MinHeap *h, int node_id, int dist)
{
    if (h->size >= MAX_NODES * 2) {
        fprintf(stderr, "Heap overflow — cannot insert.\n");
        return;
    }
    h->arr[h->size].node_id = node_id;
    h->arr[h->size].dist    = dist;
    heap_sift_up(h, h->size);
    h->size++;
}

HeapNode heap_extract_min(MinHeap *h)
{
    HeapNode min_node = h->arr[0];
    h->size--;
    if (h->size > 0) {
        h->arr[0] = h->arr[h->size];
        heap_sift_down(h, 0);
    }
    return min_node;
}

/* =============================================================================
 * Section 8: Graph Management
 * ============================================================================= */

void init_graph(Graph *g)
{
    g->num_nodes = 0;
    g->num_edges = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        g->nodes[i].id   = i;
        g->nodes[i].name[0] = '\0';
        g->nodes[i].head = NULL;
    }
}

/*
 * add_node — append a named intersection; returns its ID or -1 if graph is full.
 */
int add_node(Graph *g, const char *name)
{
    if (g->num_nodes >= MAX_NODES) {
        fprintf(stderr, "Graph is full — cannot add more nodes.\n");
        return -1;
    }
    int id = g->num_nodes;
    g->nodes[id].id   = id;
    g->nodes[id].head = NULL;
    strncpy(g->nodes[id].name, name, MAX_NAME_LEN - 1);
    g->nodes[id].name[MAX_NAME_LEN - 1] = '\0';
    g->num_nodes++;
    return id;
}

/*
 * add_edge — allocate an Edge, compute composite weight, prepend to adjacency list.
 * Returns 0 on success, -1 on failure.
 */
int add_edge(Graph *g, int src, int dest, int base_distance, int congestion, int speed_limit)
{
    if (src < 0 || src >= g->num_nodes || dest < 0 || dest >= g->num_nodes) {
        fprintf(stderr, "add_edge: invalid node ID(s).\n");
        return -1;
    }
    if (g->num_edges >= MAX_EDGES) {
        fprintf(stderr, "add_edge: edge limit reached.\n");
        return -1;
    }

    Edge *e = (Edge *)malloc(sizeof(Edge));
    if (e == NULL) {
        fprintf(stderr, "add_edge: malloc failed.\n");
        return -1;
    }

    e->dest          = dest;
    e->base_distance = base_distance;
    e->congestion    = congestion;
    e->speed_limit   = speed_limit;
    e->weight        = compute_weight(base_distance, congestion, speed_limit);

    /* Prepend to adjacency list */
    e->next              = g->nodes[src].head;
    g->nodes[src].head   = e;
    g->num_edges++;
    return 0;
}

/*
 * update_edge_weight — find the directed edge src->dest, update congestion and
 * speed limit, recompute weight. Returns 0 on success, -1 if not found.
 */
int update_edge_weight(Graph *g, int src, int dest, int new_congestion, int new_speed_limit)
{
    if (src < 0 || src >= g->num_nodes) return -1;

    for (Edge *e = g->nodes[src].head; e != NULL; e = e->next) {
        if (e->dest == dest) {
            e->congestion  = new_congestion;
            e->speed_limit = new_speed_limit;
            e->weight      = compute_weight(e->base_distance, new_congestion, new_speed_limit);
            return 0;
        }
    }
    return -1;
}

/*
 * find_node_by_name — case-sensitive linear search; returns node ID or -1.
 */
int find_node_by_name(const Graph *g, const char *name)
{
    for (int i = 0; i < g->num_nodes; i++) {
        if (strcmp(g->nodes[i].name, name) == 0) return i;
    }
    return -1;
}

/*
 * print_graph — display full adjacency list with weights and node names.
 */
void print_graph(const Graph *g)
{
    print_separator();
    printf("Road Network Adjacency List (%d nodes, %d edges)\n",
           g->num_nodes, g->num_edges);
    print_separator();

    for (int i = 0; i < g->num_nodes; i++) {
        printf("[%d] %s\n", i, g->nodes[i].name);
        for (Edge *e = g->nodes[i].head; e != NULL; e = e->next) {
            printf("      --> %-20s  weight=%-4d  (dist=%d, cong=%d, speed=%d km/h)\n",
                   g->nodes[e->dest].name,
                   e->weight,
                   e->base_distance,
                   e->congestion,
                   e->speed_limit);
        }
        if (g->nodes[i].head == NULL) {
            printf("      (no outgoing roads)\n");
        }
    }
    print_separator();
}

/*
 * free_graph — walk every adjacency list and free all allocated Edge nodes.
 */
void free_graph(Graph *g)
{
    for (int i = 0; i < g->num_nodes; i++) {
        Edge *e = g->nodes[i].head;
        while (e != NULL) {
            Edge *tmp = e->next;
            free(e);
            e = tmp;
        }
        g->nodes[i].head = NULL;
    }
    g->num_nodes = 0;
    g->num_edges = 0;
}

/* =============================================================================
 * Section 9: Dijkstra's Algorithm
 * ============================================================================= */

/*
 * dijkstra — single-source shortest paths using a binary min-heap with lazy
 * deletion (stale entries are silently skipped). O((V+E) log V).
 *
 * Returns 0 on success, -1 if source is invalid.
 */
int dijkstra(const Graph *g, int source, DijkstraResult *result)
{
    if (source < 0 || source >= g->num_nodes) return -1;

    /* Initialise distances to INF, prev to -1, visited to 0 */
    for (int i = 0; i < g->num_nodes; i++) {
        result->dist[i]    = INF;
        result->prev[i]    = -1;
        result->visited[i] = 0;
    }
    result->dist[source] = 0;

    MinHeap heap;
    heap_init(&heap);
    heap_insert(&heap, source, 0);

    while (!heap_is_empty(&heap)) {
        HeapNode hn = heap_extract_min(&heap);
        int u       = hn.node_id;

        /* Lazy deletion: skip if already finalised */
        if (result->visited[u]) continue;
        result->visited[u] = 1;

        /* Relax neighbours */
        for (Edge *e = g->nodes[u].head; e != NULL; e = e->next) {
            int v = e->dest;
            if (result->visited[v]) continue;

            /* Guard against INF overflow */
            if (result->dist[u] != INF &&
                result->dist[u] + e->weight < result->dist[v])
            {
                result->dist[v] = result->dist[u] + e->weight;
                result->prev[v] = u;
                heap_insert(&heap, v, result->dist[v]);
            }
        }
    }
    return 0;
}

/* =============================================================================
 * Section 10: Path Output Functions
 * ============================================================================= */

/*
 * reconstruct_path — fills path[] with node IDs from source to dest.
 * Returns 1 if a path exists, 0 otherwise.
 */
int reconstruct_path(const DijkstraResult *result, int source, int dest,
                     int path[], int *path_len)
{
    if (result->dist[dest] == INF) {
        *path_len = 0;
        return 0;
    }

    /* Trace back from dest to source */
    int tmp[MAX_NODES];
    int len = 0;
    int cur = dest;

    while (cur != -1) {
        tmp[len++] = cur;
        if (cur == source) break;
        cur = result->prev[cur];
        if (len > MAX_NODES) {
            /* Cycle guard */
            *path_len = 0;
            return 0;
        }
    }

    /* Reverse into path[] */
    for (int i = 0; i < len; i++) {
        path[i] = tmp[len - 1 - i];
    }
    *path_len = len;
    return 1;
}

/*
 * print_shortest_path — pretty-print the shortest path from source to dest
 * using node names.
 */
void print_shortest_path(const Graph *g, const DijkstraResult *result,
                         int source, int dest)
{
    print_separator();
    printf("Shortest path: %s  -->  %s\n",
           g->nodes[source].name, g->nodes[dest].name);
    print_separator();

    int path[MAX_NODES];
    int path_len = 0;

    if (!reconstruct_path(result, source, dest, path, &path_len)) {
        printf("No path exists between %s and %s.\n",
               g->nodes[source].name, g->nodes[dest].name);
        print_separator();
        return;
    }

    printf("Route  : ");
    for (int i = 0; i < path_len; i++) {
        printf("%s", g->nodes[path[i]].name);
        if (i < path_len - 1) printf("  ->  ");
    }
    printf("\n");
    printf("Total weight : %d\n", result->dist[dest]);
    print_separator();
}

/*
 * print_all_distances — print distances from source to every reachable node.
 */
void print_all_distances(const Graph *g, const DijkstraResult *result, int source)
{
    print_separator();
    printf("All shortest distances from: %s\n", g->nodes[source].name);
    print_separator();

    for (int i = 0; i < g->num_nodes; i++) {
        if (i == source) continue;
        if (result->dist[i] == INF) {
            printf("  %-25s  -->  UNREACHABLE\n", g->nodes[i].name);
        } else {
            printf("  %-25s  -->  %d\n", g->nodes[i].name, result->dist[i]);
        }
    }
    print_separator();
}

/* =============================================================================
 * Section 11: Demo Map Loader
 * ============================================================================= */

/*
 * load_demo_map — populate the graph with 10 named intersections and 18
 * directed edges representing a sample city road network.
 */
void load_demo_map(Graph *g)
{
    /* Clear any existing graph first */
    free_graph(g);
    init_graph(g);

    /* Add nodes (IDs 0-9) */
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

    /* Add 18 directed edges: add_edge(g, src, dest, dist, cong, speed) */
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

/* =============================================================================
 * Section 12: CLI / Menu Functions
 * ============================================================================= */

void run_menu(Graph *g)
{
    int running = 1;

    while (running) {
        printf("\n");
        print_separator();
        printf("        SMART TRAFFIC ANALYSIS SYSTEM\n");
        print_separator();
        printf("  MAIN MENU:\n");
        printf("    1. Load Demo City Map\n");
        printf("    2. Add Intersection (Node)\n");
        printf("    3. Add Road (Edge)\n");
        printf("    4. View Road Network\n");
        printf("    5. Find Shortest Path (Single Destination)\n");
        printf("    6. Find All Shortest Paths from Source\n");
        printf("    7. Update Road Congestion / Speed Limit\n");
        printf("    8. Exit\n");
        print_separator();

        int choice = read_int("Enter choice", 1, 8);

        switch (choice) {

        /* ------------------------------------------------------------------ */
        case 1: {
            load_demo_map(g);
            printf("Demo city map loaded successfully.\n");
            printf("  %d intersections, %d roads added.\n",
                   g->num_nodes, g->num_edges);
            break;
        }

        /* ------------------------------------------------------------------ */
        case 2: {
            char name[MAX_NAME_LEN];
            read_string("Enter intersection name", name, MAX_NAME_LEN);
            if (strlen(name) == 0) {
                printf("Name cannot be empty.\n");
                break;
            }
            int id = add_node(g, name);
            if (id >= 0)
                printf("Intersection '%s' added with ID %d.\n", name, id);
            break;
        }

        /* ------------------------------------------------------------------ */
        case 3: {
            if (g->num_nodes < 2) {
                printf("Need at least 2 intersections. Please add nodes first.\n");
                break;
            }
            char src_name[MAX_NAME_LEN], dest_name[MAX_NAME_LEN];
            read_string("Enter source intersection name", src_name, MAX_NAME_LEN);
            read_string("Enter destination intersection name", dest_name, MAX_NAME_LEN);

            int src  = find_node_by_name(g, src_name);
            int dest = find_node_by_name(g, dest_name);

            if (src < 0) {
                printf("Intersection '%s' not found.\n", src_name);
                break;
            }
            if (dest < 0) {
                printf("Intersection '%s' not found.\n", dest_name);
                break;
            }

            int dist  = read_int("  Base distance (km)", 1, 1000);
            int cong  = read_int("  Congestion level", 1, 10);
            int speed = read_int("  Speed limit (km/h)", 5, 200);

            int rc = add_edge(g, src, dest, dist, cong, speed);
            if (rc == 0) {
                int w = compute_weight(dist, cong, speed);
                printf("Road added: %s --> %s  (weight=%d)\n",
                       src_name, dest_name, w);
            }
            break;
        }

        /* ------------------------------------------------------------------ */
        case 4: {
            if (g->num_nodes == 0) {
                printf("Graph is empty. Load a map or add nodes first.\n");
                break;
            }
            print_graph(g);
            break;
        }

        /* ------------------------------------------------------------------ */
        case 5: {
            if (g->num_nodes == 0) {
                printf("Graph is empty. Load a map or add nodes first.\n");
                break;
            }
            char src_name[MAX_NAME_LEN], dest_name[MAX_NAME_LEN];
            read_string("Enter source intersection name", src_name, MAX_NAME_LEN);
            read_string("Enter destination intersection name", dest_name, MAX_NAME_LEN);

            int src  = find_node_by_name(g, src_name);
            int dest = find_node_by_name(g, dest_name);

            if (src < 0) {
                printf("Intersection '%s' not found.\n", src_name);
                break;
            }
            if (dest < 0) {
                printf("Intersection '%s' not found.\n", dest_name);
                break;
            }

            DijkstraResult result;
            dijkstra(g, src, &result);
            print_shortest_path(g, &result, src, dest);
            break;
        }

        /* ------------------------------------------------------------------ */
        case 6: {
            if (g->num_nodes == 0) {
                printf("Graph is empty. Load a map or add nodes first.\n");
                break;
            }
            char src_name[MAX_NAME_LEN];
            read_string("Enter source intersection name", src_name, MAX_NAME_LEN);

            int src = find_node_by_name(g, src_name);
            if (src < 0) {
                printf("Intersection '%s' not found.\n", src_name);
                break;
            }

            DijkstraResult result;
            dijkstra(g, src, &result);
            print_all_distances(g, &result, src);
            break;
        }

        /* ------------------------------------------------------------------ */
        case 7: {
            if (g->num_nodes < 2) {
                printf("Need at least 2 intersections to update a road.\n");
                break;
            }
            char src_name[MAX_NAME_LEN], dest_name[MAX_NAME_LEN];
            read_string("Enter source intersection name", src_name, MAX_NAME_LEN);
            read_string("Enter destination intersection name", dest_name, MAX_NAME_LEN);

            int src  = find_node_by_name(g, src_name);
            int dest = find_node_by_name(g, dest_name);

            if (src < 0) {
                printf("Intersection '%s' not found.\n", src_name);
                break;
            }
            if (dest < 0) {
                printf("Intersection '%s' not found.\n", dest_name);
                break;
            }

            int cong  = read_int("  New congestion level", 1, 10);
            int speed = read_int("  New speed limit (km/h)", 5, 200);

            int rc = update_edge_weight(g, src, dest, cong, speed);
            if (rc == 0) {
                printf("Road %s --> %s updated successfully.\n", src_name, dest_name);
            } else {
                printf("Road %s --> %s not found.\n", src_name, dest_name);
            }
            break;
        }

        /* ------------------------------------------------------------------ */
        case 8: {
            printf("Freeing graph resources and exiting. Goodbye.\n");
            free_graph(g);
            running = 0;
            break;
        }

        default:
            printf("Unexpected choice.\n");
            break;
        }
    }
}

/* =============================================================================
 * Section 13: main()
 * ============================================================================= */

int main(void)
{
    Graph g;
    init_graph(&g);

    printf("\n");
    print_separator();
    printf("  Welcome to the Smart Traffic Analysis System\n");
    printf("  Powered by Dijkstra's Algorithm (Min-Heap, Lazy Deletion)\n");
    print_separator();

    run_menu(&g);

    return 0;
}
