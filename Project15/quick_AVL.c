#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* 퀵 정렬(피벗: Median-of-3) vs AVL 트리 정렬 : 정렬 + 삽입/삭제 전체 과정 비교 */

static long long cmp_q, mov_q;   /* 퀵(배열) 비교/이동 */
static long long cmp_a, mov_a;   /* AVL 비교/이동 */
static int cur_depth, max_depth; /* 재귀 깊이 */

static int rand_int(void) { return ((rand() & 0x7FFF) << 15) | (rand() & 0x7FFF); }

static double now_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
#else
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
#endif
}

/* ================= 퀵 정렬 (Median3) ================= */
#define QUICK_FRAME (sizeof(int *) + 3 * sizeof(int) + sizeof(int) + sizeof(void *))

static void swap_q(int* a, int* b) { int t = *a; *a = *b; *b = t; mov_q += 3; }

static void choose_pivot(int a[], int lo, int hi) {
    int p = lo, mid = lo + (hi - lo) / 2;
    int x = a[lo], y = a[mid], z = a[hi];
    cmp_q++;
    if (x < y) {
        cmp_q++;
        if (y < z) p = mid;
        else { cmp_q++; p = (x < z) ? hi : lo; }
    }
    else {
        cmp_q++;
        if (x < z) p = lo;
        else { cmp_q++; p = (y < z) ? hi : mid; }
    }
    if (p != lo) swap_q(&a[lo], &a[p]);
}

static int partition(int a[], int lo, int hi) {
    choose_pivot(a, lo, hi);
    int pivot = a[lo];
    int i = lo, j = hi + 1;
    while (1) {
        while (++i <= hi) { cmp_q++; if (!(a[i] < pivot)) break; }
        do { j--; cmp_q++; } while (a[j] > pivot);
        if (i >= j) break;
        swap_q(&a[i], &a[j]);
    }
    swap_q(&a[lo], &a[j]);
    return j;
}

static void quick_sort(int a[], int lo, int hi) {
    if (lo >= hi) return;
    if (++cur_depth > max_depth) max_depth = cur_depth;
    int p = partition(a, lo, hi);
    quick_sort(a, lo, p - 1);
    quick_sort(a, p + 1, hi);
    cur_depth--;
}

/* 정렬 배열 삽입: 선형 탐색(첫 a[i] > v 위치) 후 뒤로 이동 */
static void arr_insert(int a[], int* cnt, int v) {
    int i = 0;
    while (i < *cnt) { cmp_q++; if (a[i] > v) break; i++; }
    for (int k = *cnt; k > i; k--) { a[k] = a[k - 1]; mov_q++; }
    a[i] = v; mov_q++;
    (*cnt)++;
}
/* 삭제: 선형 탐색 후 제거, 뒤의 데이터를 앞으로 이동. 없으면 false */
static int arr_delete(int a[], int* cnt, int v) {
    int i = 0;
    while (i < *cnt) { cmp_q++; if (a[i] >= v) break; i++; }
    if (i >= *cnt) return 0;
    cmp_q++;
    if (a[i] != v) return 0;
    for (int k = i; k < *cnt - 1; k++) { a[k] = a[k + 1]; mov_q++; }
    (*cnt)--;
    return 1;
}

/* ================= AVL 트리 (중복은 cnt로 저장) ================= */
typedef struct Node { int key, cnt, height; struct Node* l, * r; } Node;
static long long nodes_now, nodes_peak;

#define AVL_FRAME (sizeof(Node *) * 2 + sizeof(int) + sizeof(void *))

static int h(Node* n) { return n ? n->height : 0; }
static void upd(Node* n) { int a = h(n->l), b = h(n->r); n->height = (a > b ? a : b) + 1; }
static Node* rot_r(Node* y) { Node* x = y->l; y->l = x->r; x->r = y; upd(y); upd(x); return x; }
static Node* rot_l(Node* x) { Node* y = x->r; x->r = y->l; y->l = x; upd(x); upd(y); return y; }
static Node* balance(Node* n) {
    upd(n);
    int bf = h(n->l) - h(n->r);          /* 균형 인수: 제어 비교(미집계) */
    if (bf > 1) { if (h(n->l->l) < h(n->l->r)) n->l = rot_l(n->l); return rot_r(n); }
    if (bf < -1) { if (h(n->r->r) < h(n->r->l)) n->r = rot_r(n->r); return rot_l(n); }
    return n;
}
static Node* avl_insert(Node* n, int v) {
    if (!n) {
        Node* t = malloc(sizeof(Node));
        if (!t) { printf("메모리 할당 실패\n"); exit(1); }
        t->key = v; t->cnt = 1; t->height = 1; t->l = t->r = NULL;
        mov_a++;                         /* key 대입 */
        if (++nodes_now > nodes_peak) nodes_peak = nodes_now;
        return t;
    }
    if (++cur_depth > max_depth) max_depth = cur_depth;
    cmp_a++;                             /* 3방향 비교 1회 = 1 비교 */
    if (v < n->key) n->l = avl_insert(n->l, v);
    else if (v > n->key) n->r = avl_insert(n->r, v);
    else { n->cnt++; mov_a++; cur_depth--; return n; }
    cur_depth--;
    return balance(n);
}
static Node* min_node(Node* n) { while (n->l) n = n->l; return n; }
static Node* avl_delete(Node* n, int v, int* ok) {
    if (!n) return NULL;
    if (++cur_depth > max_depth) max_depth = cur_depth;
    cmp_a++;
    if (v < n->key) n->l = avl_delete(n->l, v, ok);
    else if (v > n->key) n->r = avl_delete(n->r, v, ok);
    else {
        *ok = 1;
        if (n->cnt > 1) { n->cnt--; mov_a++; cur_depth--; return n; }
        if (!n->l || !n->r) {
            Node* c = n->l ? n->l : n->r;
            free(n); nodes_now--; cur_depth--;
            return c;
        }
        Node* s = min_node(n->r);        /* 후속자: 포인터 이동만 (비교 없음) */
        n->key = s->key; n->cnt = s->cnt; mov_a += 2;
        s->cnt = 1;                      /* 후속자 노드가 완전히 제거되도록 */
        int dummy = 0;
        n->r = avl_delete(n->r, n->key, &dummy);
    }
    cur_depth--;
    return balance(n);
}
static int inorder(Node* n, int* out, int pos) {
    if (!n) return pos;
    pos = inorder(n->l, out, pos);
    for (int i = 0; i < n->cnt; i++) out[pos++] = n->key;
    return inorder(n->r, out, pos);
}
static void free_tree(Node* n) { if (n) { free_tree(n->l); free_tree(n->r); free(n); } }

/* ================= main ================= */
int main(void) {
    int sizes[] = { 1000, 2000, 4000, 8000, 16000, 20000 };
    srand((unsigned)time(NULL));

    for (int s = 0; s < 6; s++) {
        int n = sizes[s], ops = n / 5;
        int* orig = malloc(sizeof(int) * n);
        int* arr = malloc(sizeof(int) * (n + ops));
        int* opt = malloc(sizeof(int) * ops), * opv = malloc(sizeof(int) * ops);
        int* chk = malloc(sizeof(int) * (n + ops));
        if (!orig || !arr || !opt || !opv || !chk) { printf("메모리 할당 실패\n"); return 1; }

        /* 난수 사전 생성 (시간 제외) */
        for (int i = 0; i < n; i++) orig[i] = rand_int() % 100000;
        for (int i = 0; i < ops; i++) { opt[i] = rand_int() % 2; opv[i] = rand_int() % 100000; }

        /* ---- 퀵 정렬 + 배열 삽입/삭제 ---- */
        cmp_q = mov_q = 0; cur_depth = max_depth = 0;
        memcpy(arr, orig, sizeof(int) * n);
        int cnt = n, peak = n;
        double t0 = now_ms();
        quick_sort(arr, 0, n - 1);
        for (int i = 0; i < ops; i++) {
            if (opt[i] == 0) arr_insert(arr, &cnt, opv[i]);
            else arr_delete(arr, &cnt, opv[i]);
            if (cnt > peak) peak = cnt;
        }
        double tq = now_ms() - t0;
        size_t mem_q = sizeof(int) * (size_t)peak + (size_t)max_depth * QUICK_FRAME;

        /* ---- AVL 트리 정렬 + 삽입/삭제 ---- */
        cmp_a = mov_a = 0; cur_depth = max_depth = 0; nodes_now = nodes_peak = 0;
        Node* root = NULL;
        t0 = now_ms();
        for (int i = 0; i < n; i++) root = avl_insert(root, orig[i]);
        int sorted_ok;
        {   /* 중위 순회 확인 (정렬 결과 확인 단계, 시간에 포함) */
            int total = inorder(root, chk, 0); (void)total;
        }
        for (int i = 0; i < ops; i++) {
            if (opt[i] == 0) root = avl_insert(root, opv[i]);
            else { int ok = 0; root = avl_delete(root, opv[i], &ok); }
        }
        double ta = now_ms() - t0;
        size_t mem_a = sizeof(Node) * (size_t)nodes_peak + (size_t)max_depth * AVL_FRAME;

        /* 결과 검증 (시간 제외) */
        int total = inorder(root, chk, 0);
        sorted_ok = (total == cnt);
        for (int i = 0; sorted_ok && i < total; i++) if (chk[i] != arr[i]) sorted_ok = 0;

        printf("Data Size : %d\nOperations: %d\n\n", n, ops);
        printf("%-18s%-16s%-16s\n", "", "Quick Sort", "AVL Tree");
        printf("------------------------------------------------\n");
        char b1[32], b2[32];
        snprintf(b1, sizeof b1, "%.4f ms", tq);
        snprintf(b2, sizeof b2, "%.4f ms", ta);
        printf("%-18s%-16s%-16s\n", "Total Time", b1, b2);
        printf("%-18s%-16lld%-16lld\n", "Comparisons", cmp_q, cmp_a);
        printf("%-18s%-16lld%-16lld\n", "Data Movements", mov_q, mov_a);
        snprintf(b1, sizeof b1, "%zu Bytes", mem_q);
        snprintf(b2, sizeof b2, "%zu Bytes", mem_a);
        printf("%-18s%-16s%-16s\n", "Memory Usage", b1, b2);
        printf("\n%-18s%-16d%-16d\n", "Final Data Count", cnt, total);
        printf("Result match (array == AVL inorder): %s\n\n", sorted_ok ? "OK" : "MISMATCH");

        free_tree(root); free(orig); free(arr); free(opt); free(opv); free(chk);
    }
    return 0;
}
