#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define V 100

/* ---------- 인접 리스트 노드 ---------- */
typedef struct Node {
    int vertex;
    struct Node* next;
} Node;

typedef struct {
    Node* head[V];
} AdjList;

typedef struct {
    int m[V][V];
} AdjMatrix;

typedef struct { int u, v; } Edge;

/* ---------- 메모리 사용량 추적 (리스트 노드용) ---------- */
static size_t list_mem;

/* ================= 인접 행렬 ================= */
void mat_init(AdjMatrix* g) {
    for (int i = 0; i < V; i++)
        for (int j = 0; j < V; j++) g->m[i][j] = 0;
}
/* 생성 단계 (비교 횟수 측정 대상 아님) */
void mat_build(AdjMatrix* g, int u, int v) { g->m[u][v] = g->m[v][u] = 1; }

/* 삽입: 이미 존재하는지 확인 (1회) 후 두 위치 설정 */
long mat_insert(AdjMatrix* g, int u, int v) {
    long c = 0;
    c++;                       /* matrix[u][v] 확인 */
    if (g->m[u][v] == 0) {
        g->m[u][v] = 1;
        g->m[v][u] = 1;
    }
    return c;
}
/* 삭제: 존재 확인 (1회) 후 두 위치 해제 */
long mat_delete(AdjMatrix* g, int u, int v) {
    long c = 0;
    c++;
    if (g->m[u][v] == 1) {
        g->m[u][v] = 0;
        g->m[v][u] = 0;
    }
    return c;
}
long mat_connected(AdjMatrix* g, int u, int v, int* res) {
    *res = g->m[u][v];
    return 1;
}
long mat_adjacent(AdjMatrix* g, int u, int print) {
    long c = 0;
    for (int j = 0; j < V; j++) {
        c++;
        if (g->m[u][j] && print) printf("%d ", j);
    }
    return c;
}
size_t mat_memory(AdjMatrix* g) {
    (void)g;                 /* 크기가 고정이라 g는 사용하지 않음 */
    return sizeof(*g);
}

/* ================= 인접 리스트 ================= */
void list_init(AdjList* g) {
    for (int i = 0; i < V; i++) g->head[i] = NULL;
    list_mem = sizeof(AdjList);          /* 정점별 head 배열 (기본 부가 공간) */
}
void list_push_front(AdjList* g, int u, int v) {
    Node* n = (Node*)malloc(sizeof(Node));
    n->vertex = v;
    n->next = g->head[u];
    g->head[u] = n;
    list_mem += sizeof(Node);
}
/* 생성 단계 */
void list_build(AdjList* g, int u, int v) {
    list_push_front(g, u, v);
    list_push_front(g, v, u);
}
/* u의 리스트에서 v 탐색, 방문 노드 수를 cnt에 누적 */
static int list_find(AdjList* g, int u, int v, long* cnt) {
    for (Node* p = g->head[u]; p; p = p->next) {
        (*cnt)++;
        if (p->vertex == v) return 1;
    }
    return 0;
}
/* 삽입: 중복 확인(탐색) 후 양쪽 리스트 앞에 삽입 */
long list_insert(AdjList* g, int u, int v) {
    long c = 0;
    if (!list_find(g, u, v, &c)) {
        list_push_front(g, u, v);
        list_push_front(g, v, u);
    }
    return c;
}
static void list_remove_one(AdjList* g, int u, int v, long* cnt) {
    Node* prev = NULL;
    for (Node* p = g->head[u]; p; prev = p, p = p->next) {
        (*cnt)++;
        if (p->vertex == v) {
            if (prev) prev->next = p->next; else g->head[u] = p->next;
            free(p);
            list_mem -= sizeof(Node);
            return;
        }
    }
}
/* 삭제: 양쪽 리스트에서 각각 탐색 후 제거 (두 탐색 비교 횟수 합산) */
long list_delete(AdjList* g, int u, int v) {
    long c = 0;
    list_remove_one(g, u, v, &c);
    list_remove_one(g, v, u, &c);
    return c;
}
long list_connected(AdjList* g, int u, int v, int* res) {
    long c = 0;
    *res = list_find(g, u, v, &c);
    return c;
}
long list_adjacent(AdjList* g, int u, int print) {
    long c = 0;
    for (Node* p = g->head[u]; p; p = p->next) {
        c++;
        if (print) printf("%d ", p->vertex);
    }
    return c;
}
void list_free(AdjList* g) {
    for (int i = 0; i < V; i++) {
        Node* p = g->head[i];
        while (p) { Node* t = p; p = p->next; free(t); }
        g->head[i] = NULL;
    }
}

/* ================= 그래프 생성 (공통 간선 목록) ================= */
/* 중복/자기 자신 간선 없이 E개 생성. 임시 배열 chk는 측정에서 제외 */
void gen_edges(Edge* edges, int E) {
    static char chk[V][V];
    for (int i = 0; i < V; i++) for (int j = 0; j < V; j++) chk[i][j] = 0;
    int k = 0;
    while (k < E) {
        int u = rand() % V, v = rand() % V;
        if (u == v || chk[u][v]) continue;
        chk[u][v] = chk[v][u] = 1;
        edges[k].u = u; edges[k].v = v; k++;
    }
}

/* 현재 그래프에 존재하지 않는 정점 쌍 선택 (행렬 기준으로 판별) */
void pick_nonedge(AdjMatrix* g, int* u, int* v) {
    do { *u = rand() % V; *v = rand() % V; } while (*u == *v || g->m[*u][*v]);
}

typedef struct {
    size_t mem;
    long ins, del, conn, adj;
} Result;

void print_result(int no, const char* gname, const char* rep, Result r) {
    printf("Case %d : %s Graph - %s\n", no, gname, rep);
    printf("Memory                         : %zu Bytes\n", r.mem);
    printf("Edge insertion comparisons     : %ld\n", r.ins);
    printf("Edge deletion comparisons      : %ld\n", r.del);
    printf("Connectivity check comparisons : %ld\n", r.conn);
    printf("Adjacent vertices comparisons  : %ld\n\n", r.adj);
}

void run(const char* gname, int E, int caseM, int caseL) {
    Edge* edges = (Edge*)malloc(sizeof(Edge) * E);
    gen_edges(edges, E);

    AdjMatrix* M = (AdjMatrix*)malloc(sizeof(AdjMatrix));
    AdjList L;
    mat_init(M);
    list_init(&L);
    for (int i = 0; i < E; i++) {
        mat_build(M, edges[i].u, edges[i].v);
        list_build(&L, edges[i].u, edges[i].v);
    }

    /* 측정에 사용할 동일한 입력 선택 */
    int iu, iv;                       /* 삽입할 (존재하지 않는) 간선 */
    pick_nonedge(M, &iu, &iv);
    int di = rand() % E;              /* 삭제할 (존재하는) 간선 */
    int du = edges[di].u, dv = edges[di].v;
    int cu, cv;                       /* 연결 확인 정점 쌍 */
    do { cu = rand() % V; cv = rand() % V; } while (cu == cv);
    int au = rand() % V;              /* 인접 정점 출력 대상 */

    Result rm, rl;
    rm.mem = mat_memory(M);
    rl.mem = list_mem;                /* 삽입/삭제 전의 그래프 크기 */

    /* 인접 행렬 */
    int res;
    printf("[%s] 연결 확인 (%d,%d): ", gname, cu, cv);
    rm.conn = mat_connected(M, cu, cv, &res);
    printf("행렬=%s, ", res ? "연결됨" : "연결 안 됨");
    rl.conn = list_connected(&L, cu, cv, &res);
    printf("리스트=%s\n", res ? "연결됨" : "연결 안 됨");

    printf("[%s] 정점 %d의 인접 정점 (행렬): ", gname, au);
    rm.adj = mat_adjacent(M, au, 1);
    printf("\n[%s] 정점 %d의 인접 정점 (리스트): ", gname, au);
    rl.adj = list_adjacent(&L, au, 1);
    printf("\n[%s] 삽입 간선 (%d,%d), 삭제 간선 (%d,%d)\n\n",
        gname, iu, iv, du, dv);

    rm.ins = mat_insert(M, iu, iv);
    rl.ins = list_insert(&L, iu, iv);
    rm.del = mat_delete(M, du, dv);
    rl.del = list_delete(&L, du, dv);

    print_result(caseM, gname, "Adjacency Matrix", rm);
    print_result(caseL, gname, "Adjacency List", rl);

    list_free(&L);
    free(M);
    free(edges);
}

int main(void) {
    srand((unsigned)time(NULL));
    run("Sparse", 100, 1, 2);
    run("Dense", 4000, 3, 4);
    return 0;
}
