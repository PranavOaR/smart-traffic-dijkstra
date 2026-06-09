# Smart Traffic Analysis System

A city road network simulator built in C that applies **Dijkstra's Algorithm** to find
the shortest (least-congested) path between any two intersections.
Submitted as a mini-project for **Design and Analysis of Algorithms (DAA), 4th Semester — REVA University**.

---

## Problem Statement

Given a directed weighted graph representing a city's road network, find the
shortest path between any two intersections where edge weights reflect real
traffic conditions: road distance, congestion level, and speed limit.

---

## Algorithm: Dijkstra's Shortest Path

Dijkstra's algorithm finds the minimum-cost path from a single source node to all
other nodes in a graph with non-negative edge weights.

### How it works (step by step)

1. Set distance to source = 0; all other distances = ∞
2. Push source into a **min-heap** (priority queue)
3. Extract the node `u` with the smallest known distance
4. For each neighbour `v` of `u`: if `dist[u] + weight(u,v) < dist[v]`, update `dist[v]` and record `prev[v] = u`
5. Repeat until the heap is empty
6. Trace `prev[]` backwards from destination to source to reconstruct the path

### Optimisation: Lazy Deletion

Instead of decreasing keys in the heap (complex to implement), this project uses
**lazy deletion**: stale (already-visited) entries in the heap are simply skipped
when popped. This keeps the implementation simple while maintaining correctness.

### Time Complexity

| Operation | Complexity |
|-----------|-----------|
| Overall   | O((V + E) log V) |
| With binary min-heap | V extractions × O(log V) + E relaxations × O(log V) |

Where V = number of intersections, E = number of roads.

### Space Complexity

O(V + E) for the adjacency list + O(V) for the heap and distance arrays.

---

## Edge Weight Formula

Each road's weight is a composite value that captures traffic conditions:

```
weight = base_distance + (congestion × 5) + max(0, (80 - speed_limit) / 10)
```

| Parameter | Range | Effect |
|-----------|-------|--------|
| `base_distance` | 1–1000 km | Direct distance contribution |
| `congestion` | 1 (free) – 10 (gridlock) | Each level adds 5 to weight |
| `speed_limit` | 5–200 km/h | Roads slower than 80 km/h add a penalty |

**Example:** A 3 km road with congestion level 7 and 50 km/h limit:
```
weight = 3 + (7 × 5) + (80 - 50)/10 = 3 + 35 + 3 = 41
```

---

## Data Structures

### Adjacency List (Graph)

Each intersection is a `Node` with a linked list of outgoing `Edge`s.
Chosen over an adjacency matrix because the graph is sparse
(18 edges across 10 nodes — far fewer than the 10×10 = 100 possible).

```
Node 0 (City Hall)
  └─ Edge → Market Street   weight=41
  └─ Edge → Central Park    weight=20

Node 1 (Market Street)
  └─ Edge → Train Station   weight=53
  └─ Edge → Bus Terminal    weight=34
...
```

### Binary Min-Heap

A heap stored in a flat array. The node with the smallest distance is always at
index 0. Two operations maintain the heap property:

- **sift_up** — called after insertion; swaps a new element upward until its parent is smaller
- **sift_down** — called after extraction; moves the root downward until both children are larger

---

## Demo City Map

10 intersections, 18 directed roads:

```
City Hall (0) ──→ Market Street (1)   weight 41
City Hall (0) ──→ Central Park (2)    weight 20
Market Street (1) → Train Station (3) weight 53
Market Street (1) → Bus Terminal (4)  weight 34
Central Park (2) → Market Street (1)  weight 23
Central Park (2) → University (6)     weight 17
Train Station (3) → Airport (5)       weight 35
Train Station (3) → Hospital (7)      weight 46
Bus Terminal (4) → Train Station (3)  weight 30
Bus Terminal (4) → Residential Zone (9) weight 22
Airport (5) → Shopping Mall (8)       weight 18
University (6) → Hospital (7)         weight 19
University (6) → Shopping Mall (8)    weight 26
Hospital (7) → Residential Zone (9)   weight 37
Hospital (7) → Shopping Mall (8)      weight 43
Shopping Mall (8) → Residential Zone (9) weight 29
Residential Zone (9) → City Hall (0)  weight 18
Residential Zone (9) → Bus Terminal (4) weight 21
```

**Sample result — City Hall → Airport:**
```
City Hall  →  Market Street  →  Train Station  →  Airport
Total weight: 129
```

---

## Project Structure

```
MiniProject/
│
├── DijkstraAlgorithm/
│   └── traffic_analysis.c     # Self-contained CLI version (810 lines)
│
└── TrafficGUI/
    ├── Makefile
    ├── traffic.h               # Shared types and prototypes
    ├── main.c                  # Entry point (9 lines)
    ├── graph.c                 # Graph management + demo map loader
    ├── dijkstra.c              # Min-heap + Dijkstra + path reconstruction
    └── gui.c                   # Raylib visual interface
```

### File responsibilities

| File | Lines | Purpose |
|------|-------|---------|
| `traffic.h` | 54 | `Edge`, `Node`, `Graph`, `DijkstraResult` structs + all prototypes |
| `main.c` | 9 | Allocates graph on stack, calls `run_gui` |
| `graph.c` | 120 | `init_graph`, `add_node`, `add_edge`, `update_edge_weight`, `free_graph`, `load_demo_map` |
| `dijkstra.c` | 91 | `push`/`pop` heap, `dijkstra`, `reconstruct_path` |
| `gui.c` | 250 | Raylib window, click handling, arrow drawing, panel rendering |

---

## Features

- **Visual graph** — 10 nodes drawn as labelled circles, 18 directed edges as arrows
- **Shortest path** — left-click source (green), right-click destination (red); path highlights in orange instantly
- **Weight labels** — each edge shows its composite weight in a floating dark box
- **All-distances panel** — right sidebar lists distance from source to every node
- **Live update** — change source/destination at any time; result recalculates immediately
- **Load / Clear buttons** — reload the demo map or clear the current selection

---

## Build & Run

### macOS Prerequisites

- macOS (Apple Silicon or Intel)
- `gcc`
- Raylib 5.x — install with: `brew install raylib`

### macOS GUI version

```bash
cd TrafficGUI
make
./traffic_gui
```

### Windows Prerequisites

For Windows, use **MSYS2 UCRT64**. Do not use regular Command Prompt or
PowerShell unless the MSYS2 toolchain has been added to PATH.

1. Install MSYS2 from <https://www.msys2.org/>
2. Open the **MSYS2 UCRT64** terminal
3. Install GCC, Make, pkg-config, and Raylib:

```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-pkg-config mingw-w64-ucrt-x86_64-raylib
```

### Windows GUI version

From the **MSYS2 UCRT64** terminal:

```bash
cd /c/Users/YourName/path/to/MiniProject/TrafficGUI
mingw32-make
./traffic_gui.exe
```

If `mingw32-make` is unavailable, try:

```bash
make
```

Common Windows errors:

| Error | Fix |
|-------|-----|
| `raylib.h: No such file or directory` | Raylib is not installed, or the project is being built from the wrong terminal. Use **MSYS2 UCRT64** and install `mingw-w64-ucrt-x86_64-raylib`. |
| `cannot find -lraylib` | Raylib is missing for the active compiler environment. Install the UCRT64 Raylib package above. |
| `make: command not found` | Install `mingw-w64-ucrt-x86_64-make`, then run `mingw32-make`. |

### CLI version

The CLI version does not need Raylib.

macOS:

```bash
cd DijkstraAlgorithm
gcc -Wall -O2 -o traffic_analysis traffic_analysis.c
./traffic_analysis
```

Windows, from **MSYS2 UCRT64**:

```bash
cd /c/Users/YourName/path/to/MiniProject/DijkstraAlgorithm
gcc -Wall -O2 -o traffic_analysis.exe traffic_analysis.c
./traffic_analysis.exe
```

---

## CLI Menu Options

```
1. Load Demo City Map
2. Add Intersection (Node)
3. Add Road (Edge)
4. View Road Network
5. Find Shortest Path (Single Destination)
6. Find All Shortest Paths from Source
7. Update Road Congestion / Speed Limit
8. Exit
```

---

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C (C11 standard) |
| GUI library | Raylib 5.5 |
| Build system | GNU Make |
| Platform | macOS (arm64 / x86_64), Windows via MSYS2 UCRT64 |

---

## References

1. Dijkstra, E.W. (1959). *A note on two problems in connexion with graphs.* Numerische Mathematik, 1, 269–271.
2. Cormen, T.H. et al. *Introduction to Algorithms*, 4th ed. — Chapter 24 (Single-Source Shortest Paths)
3. [Raylib documentation](https://www.raylib.com)
