/* [Windows 전용] MST 비교: (인접 리스트 + Kruskal) vs (인접 행렬 + Prim)
 * 컴파일: MinGW  gcc -O2 mst_compare_win.c -o mst_compare.exe
 *         MSVC   cl /O2 /utf-8 mst_compare_win.c
 * 실행  : mst_compare.exe [noedges]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <windows.h>   /* QueryPerformanceCounter */

#define V 50
#define INF INT_MAX

typedef struct { int u, v, w; } Edge;

/* ---------- 난수 ---------- */
static int rnd(int lo, int hi) { return lo + rand() % (hi - lo + 1); }

/* ---------- 1. 그래프 생성 (단순·연결 그래프) ---------- */
static void generateGraph(Edge *edges, int m) {
    static char used[V][V];
    int perm[V], cnt = 0, i;
    memset(used, 0, sizeof(used));
    for (i = 0; i < V; i++) perm[i] = i;
    for (i = V - 1; i > 0; i--) {                 /* 셔플 */
        int j = rnd(0, i), t = perm[i]; perm[i] = perm[j]; perm[j] = t;
    }
    /* 임의 신장 트리로 연결성 보장 */
    for (i = 1; i < V; i++) {
        int a = perm[i], b = perm[rnd(0, i - 1)];
        used[a][b] = used[b][a] = 1;
        edges[cnt].u = a; edges[cnt].v = b; edges[cnt].w = rnd(1, 100); cnt++;
    }
    /* 나머지 간선: 자기 루프/중복 금지 */
    while (cnt < m) {
        int a = rnd(0, V - 1), b = rnd(0, V - 1);
        if (a == b || used[a][b]) continue;
        used[a][b] = used[b][a] = 1;
        edges[cnt].u = a; edges[cnt].v = b; edges[cnt].w = rnd(1, 100); cnt++;
    }
}

/* 연결성 검사 (DFS, 스택) */
static int isConnected(const Edge *edges, int m) {
    int i;
    int stack[V], sp = 0, visited = 0;
    char vis[V] = {0};
    stack[sp++] = 0; vis[0] = 1;
    while (sp) {
        int x = stack[--sp];
        visited++;
        for (i = 0; i < m; i++) {
            int y = -1;
            if (edges[i].u == x) y = edges[i].v;
            else if (edges[i].v == x) y = edges[i].u;
            if (y >= 0 && !vis[y]) { vis[y] = 1; stack[sp++] = y; }
        }
    }
    return visited == V;
}

/* ---------- 인접 리스트 ---------- */
typedef struct Node { int to, w; struct Node *next; } Node;
typedef struct { Node *head[V]; size_t mem; } AdjList;

static void addNode(AdjList *g, int a, int b, int w) {
    Node *n = (Node *)malloc(sizeof(Node));
    n->to = b; n->w = w; n->next = g->head[a]; g->head[a] = n;
    g->mem += sizeof(Node);
}
static void buildList(AdjList *g, const Edge *e, int m) {
    int i;
    memset(g->head, 0, sizeof(g->head));
    g->mem = sizeof(g->head);                     /* 헤드 포인터 배열 */
    for (i = 0; i < m; i++) { addNode(g, e[i].u, e[i].v, e[i].w); addNode(g, e[i].v, e[i].u, e[i].w); }
}
static void freeList(AdjList *g) {
    int i;
    for (i = 0; i < V; i++) { Node *p = g->head[i]; while (p) { Node *t = p->next; free(p); p = t; } }
}

/* ---------- 인접 행렬 (0 = 간선 없음) ---------- */
typedef struct { int w[V][V]; } AdjMatrix;
static void buildMatrix(AdjMatrix *m, const Edge *e, int cnt) {
    int i;
    memset(m, 0, sizeof(*m));
    for (i = 0; i < cnt; i++) m->w[e[i].u][e[i].v] = m->w[e[i].v][e[i].u] = e[i].w;
}

/* ---------- 결과 ---------- */
typedef struct { Edge mst[V - 1]; int count; long cost; size_t mem; double ms; } Result;

/* Windows 고해상도 타이머 (ms) */
static double nowMs(void) {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart * 1000.0 / (double)freq.QuadPart;
}

/* ---------- 3. Kruskal ---------- */
static int uf_parent[V], uf_rank[V];
static int uf_find(int x) { return uf_parent[x] == x ? x : (uf_parent[x] = uf_find(uf_parent[x])); }
static int uf_union(int a, int b) {
    a = uf_find(a); b = uf_find(b);
    if (a == b) return 0;
    if (uf_rank[a] < uf_rank[b]) { int t = a; a = b; b = t; }
    uf_parent[b] = a;
    if (uf_rank[a] == uf_rank[b]) uf_rank[a]++;
    return 1;
}
static int cmpEdge(const void *a, const void *b) { return ((const Edge *)a)->w - ((const Edge *)b)->w; }

static void kruskal(const AdjList *g, int m, Result *r) {
    double t0 = nowMs();
    int i, k = 0;
    Edge *all = (Edge *)malloc(sizeof(Edge) * m);      /* 간선 배열 */
    r->count = 0; r->cost = 0;
    for (i = 0; i < V; i++)
        for (Node *p = g->head[i]; p; p = p->next)
            if (i < p->to) { all[k].u = i; all[k].v = p->to; all[k].w = p->w; k++; }
    qsort(all, k, sizeof(Edge), cmpEdge);
    for (i = 0; i < V; i++) { uf_parent[i] = i; uf_rank[i] = 0; }
    for (i = 0; i < k && r->count < V - 1; i++)
        if (uf_union(all[i].u, all[i].v)) { r->mst[r->count++] = all[i]; r->cost += all[i].w; }
    r->ms = nowMs() - t0;
    r->mem = g->mem + sizeof(Edge) * m + sizeof(uf_parent) + sizeof(uf_rank) + sizeof(r->mst);
    free(all);
}

/* ---------- 4. Prim ---------- */
static void prim(const AdjMatrix *m, Result *r) {
    double t0 = nowMs();
    int key[V], parent[V], inMST[V], i, it, v;
    for (i = 0; i < V; i++) { key[i] = INF; parent[i] = -1; inMST[i] = 0; }
    key[0] = 0; r->count = 0; r->cost = 0;
    for (it = 0; it < V; it++) {
        int u = -1;
        for (i = 0; i < V; i++) if (!inMST[i] && (u == -1 || key[i] < key[u])) u = i;
        inMST[u] = 1;
        if (parent[u] != -1) {
            r->mst[r->count].u = parent[u]; r->mst[r->count].v = u; r->mst[r->count].w = key[u];
            r->count++; r->cost += key[u];
        }
        for (v = 0; v < V; v++)
            if (m->w[u][v] && !inMST[v] && m->w[u][v] < key[v]) { key[v] = m->w[u][v]; parent[v] = u; }
    }
    r->ms = nowMs() - t0;
    r->mem = sizeof(AdjMatrix) + sizeof(key) + sizeof(parent) + sizeof(inMST) + sizeof(r->mst);
}

/* ---------- 출력 ---------- */
static void report(int no, const char *title, int edges, const Result *r, int pe) {
    int i;
    printf("Case %d : %s\n", no, title);
    printf("Vertices      : %d\n", V);
    printf("Edges         : %d\n", edges);
    printf("MST Edges     : %d\n", r->count);
    printf("MST Cost      : %ld\n", r->cost);
    printf("Memory        : %lu Bytes\n", (unsigned long)r->mem);
    printf("Execution Time: %.6f ms\n", r->ms);
    if (pe) {
        printf("MST Edge List (u - v : w)\n");
        for (i = 0; i < r->count; i++) printf("  %d - %d : %d\n", r->mst[i].u, r->mst[i].v, r->mst[i].w);
    }
    printf("\n");
}

int main(int argc, char **argv) {
    int pe = !(argc > 1 && strcmp(argv[1], "noedges") == 0);
    const int SE = 100, DE = 600;
    static Edge sparse[100], dense[600];
    static AdjList sL, dL;
    static AdjMatrix sM, dM;
    static Result r1, r2, r3, r4;

    srand((unsigned)time(NULL));
    do generateGraph(sparse, SE); while (!isConnected(sparse, SE));
    do generateGraph(dense, DE);  while (!isConnected(dense, DE));

    buildList(&sL, sparse, SE);  buildList(&dL, dense, DE);
    buildMatrix(&sM, sparse, SE); buildMatrix(&dM, dense, DE);

    kruskal(&sL, SE, &r1); kruskal(&dL, DE, &r2);
    prim(&sM, &r3);        prim(&dM, &r4);

    report(1, "Sparse Graph - Adjacency List - Kruskal",  SE, &r1, pe);
    report(2, "Dense Graph - Adjacency List - Kruskal",   DE, &r2, pe);
    report(3, "Sparse Graph - Adjacency Matrix - Prim",   SE, &r3, pe);
    report(4, "Dense Graph - Adjacency Matrix - Prim",    DE, &r4, pe);

    printf("[검증] 모든 MST 간선 수 49 : %s\n",
           (r1.count == 49 && r2.count == 49 && r3.count == 49 && r4.count == 49) ? "OK" : "FAIL");
    printf("[검증] Sparse 비용 일치 : %s\n", r1.cost == r3.cost ? "OK" : "FAIL");
    printf("[검증] Dense  비용 일치 : %s\n", r2.cost == r4.cost ? "OK" : "FAIL");

    freeList(&sL); freeList(&dL);
    return 0;
}
