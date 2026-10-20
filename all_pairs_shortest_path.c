#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define V 10          /* 정점 수 */
#define E 20          /* 간선 수 */
#define MAX_W 100     /* 최대 가중치 */
#define INF 1000000000

static int adj[V][V];            /* 가중치 인접행렬 (0이면 간선 없음) */
static int edgeU[E], edgeV[E];   /* 간선 목록 (u < v) */
static int dist[V][V];           /* 최단거리 */
static int nxt[V][V];            /* nxt[i][j] : i에서 j로 갈 때 다음 정점 */

/* ---------- 1. 그래프 생성 ---------- */
static void generateGraph(void)
{
    int perm[V], i, j, t, cnt = 0;

    /* 정점 순서를 무작위로 섞음 */
    for (i = 0; i < V; i++) perm[i] = i;
    for (i = V - 1; i > 0; i--) {
        j = rand() % (i + 1);
        t = perm[i]; perm[i] = perm[j]; perm[j] = t;
    }

    /* 신장트리(V-1개 간선)를 만들어 연결 그래프를 보장함 */
    for (i = 1; i < V; i++) {
        int a = perm[i];
        int b = perm[rand() % i];
        int wt = rand() % MAX_W + 1;
        adj[a][b] = adj[b][a] = wt;
        edgeU[cnt] = a < b ? a : b;
        edgeV[cnt] = a < b ? b : a;
        cnt++;
    }

    /* 나머지 간선은 자기 자신/중복이 아닌 것만 무작위로 추가 */
    while (cnt < E) {
        int a = rand() % V;
        int b = rand() % V;
        if (a == b || adj[a][b] != 0) continue;
        adj[a][b] = adj[b][a] = rand() % MAX_W + 1;
        edgeU[cnt] = a < b ? a : b;
        edgeV[cnt] = a < b ? b : a;
        cnt++;
    }

    /* 출력을 보기 좋게 (u, v) 순으로 정렬 */
    for (i = 0; i < E - 1; i++) {
        for (j = 0; j < E - 1 - i; j++) {
            if (edgeU[j] > edgeU[j + 1] ||
                (edgeU[j] == edgeU[j + 1] && edgeV[j] > edgeV[j + 1])) {
                t = edgeU[j]; edgeU[j] = edgeU[j + 1]; edgeU[j + 1] = t;
                t = edgeV[j]; edgeV[j] = edgeV[j + 1]; edgeV[j + 1] = t;
            }
        }
    }
}

/* ---------- 2. 그래프 출력 ---------- */
static void printGraph(void)
{
    int i;
    printf("Graph\n");
    printf("Vertices : %d\n", V);
    printf("Edges    : %d\n\n", E);
    printf("Edges\n");
    for (i = 0; i < E; i++)
        printf("(%d, %d) Weight: %d\n", edgeU[i], edgeV[i], adj[edgeU[i]][edgeV[i]]);
    printf("\n");
}

/* ---------- 3. Floyd-Warshall (경로 복원용 next 배열 포함) ---------- */
static void floydWarshall(void)
{
    int i, j, k;

    for (i = 0; i < V; i++) {
        for (j = 0; j < V; j++) {
            if (i == j) { dist[i][j] = 0;        nxt[i][j] = i; }
            else if (adj[i][j]) { dist[i][j] = adj[i][j]; nxt[i][j] = j; }
            else { dist[i][j] = INF;      nxt[i][j] = -1; }
        }
    }

    for (k = 0; k < V; k++)
        for (i = 0; i < V; i++)
            for (j = 0; j < V; j++)
                if (dist[i][k] != INF && dist[k][j] != INF &&
                    dist[i][k] + dist[k][j] < dist[i][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    nxt[i][j] = nxt[i][k];
                }
}

/* u에서 v까지의 경로를 path[]에 복원하고 정점 수를 반환 */
static int buildPath(int u, int v, int path[])
{
    int len = 0, cur = u;
    if (nxt[u][v] == -1) return 0;
    path[len++] = cur;
    while (cur != v) {
        cur = nxt[cur][v];
        path[len++] = cur;
    }
    return len;
}

/* ---------- 4. 확인용 검사 함수들 ---------- */
static int checkEdges(void)       /* 간선 20개, 자기루프/중복 없음, 가중치 범위 */
{
    int i, j, ok = 1;
    for (i = 0; i < E; i++) {
        if (edgeU[i] == edgeV[i]) ok = 0;
        if (adj[edgeU[i]][edgeV[i]] < 1 || adj[edgeU[i]][edgeV[i]] > MAX_W) ok = 0;
        for (j = i + 1; j < E; j++)
            if (edgeU[i] == edgeU[j] && edgeV[i] == edgeV[j]) ok = 0;
    }
    return ok;
}

static int checkConnected(void)   /* 모든 쌍에 경로가 존재하는가 */
{
    int i, j;
    for (i = 0; i < V; i++)
        for (j = 0; j < V; j++)
            if (dist[i][j] == INF) return 0;
    return 1;
}

/* ---------- 5. 최단경로 출력 ---------- */
int main(void)
{
    int u, v, i, pairs = 0, pathOk = 1;
    int path[V];

    srand((unsigned int)time(NULL));

    generateGraph();
    printGraph();
    floydWarshall();

    printf("Shortest Paths\n");
    for (u = 0; u < V; u++) {
        for (v = u + 1; v < V; v++) {
            int len = buildPath(u, v, path);
            int sum = 0;

            printf("\n%d -> %d\n", u, v);
            printf("Path     : ");
            for (i = 0; i < len; i++) {
                printf("%d", path[i]);
                if (i < len - 1) printf(" -> ");
            }
            printf("\nDistance : %d\n", dist[u][v]);

            /* 경로의 간선 가중치 합이 최단거리와 같은지 검증 */
            for (i = 0; i + 1 < len; i++) sum += adj[path[i]][path[i + 1]];
            if (len == 0 || sum != dist[u][v]) pathOk = 0;

            pairs++;
        }
    }

    printf("\nTotal vertex pairs : %d\n", pairs);

    /* 확인 사항 요약 */
    printf("\n[Verification]\n");
    printf("Vertices == %d          : %s\n", V, "OK");
    printf("Edges == %d             : %s\n", E, checkEdges() ? "OK" : "FAIL");
    printf("No self-loop/duplicate  : %s\n", checkEdges() ? "OK" : "FAIL");
    printf("Connected graph         : %s\n", checkConnected() ? "OK" : "FAIL");
    printf("Pairs == 45             : %s\n", pairs == 45 ? "OK" : "FAIL");
    printf("Path weight sum == dist : %s\n", pathOk ? "OK" : "FAIL");

    return 0;
}
