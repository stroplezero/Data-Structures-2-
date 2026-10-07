#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TRY_COUNT    100
#define SEARCH_COUNT 50
#define MAX_VALUE    1000

/* ---------- BST 노드 ---------- */
typedef struct Node {
    int data;
    struct Node* left;
    struct Node* right;
} Node;

/* ---------- AVL 노드 ---------- */
typedef struct AVLNode {
    int data;
    int h;                          /* 노드 수 기준 높이 */
    struct AVLNode* left;
    struct AVLNode* right;
} AVLNode;

static long long arr_build_cmp = 0;   /* 배열 생성 비교 횟수 */
static long long bst_build_cmp = 0;   /* BST 생성 비교 횟수 */
static long long avl_build_cmp = 0;   /* AVL 생성 비교 횟수 */

/* ================= 배열 ================= */
/* 중복 확인용 순차 탐색: 비교 횟수를 *cmp 에 누적. 존재하면 1 */
int array_contains(const int arr[], int n, int key, long long* cmp) {
    for (int i = 0; i < n; i++) {
        (*cmp)++;
        if (arr[i] == key) return 1;
    }
    return 0;
}

/* 탐색용 순차 탐색: 원소와 비교할 때마다 1회 */
int sequential_search(const int arr[], int n, int key, int* comparisons) {
    *comparisons = 0;
    for (int i = 0; i < n; i++) {
        (*comparisons)++;
        if (arr[i] == key) return 1;
    }
    return 0;
}

/* ================= BST ================= */
Node* create_node(int value) {
    Node* n = (Node*)malloc(sizeof(Node));
    if (!n) { fprintf(stderr, "메모리 할당 실패\n"); exit(1); }
    n->data = value;
    n->left = n->right = NULL;
    return n;
}

/* 삽입: 기존 노드와 값을 비교할 때마다 1회로 계산
        같은 값이면 삽입하지 않음 (inserted = 0) */
Node* insert(Node* root, int value, int* inserted) {
    *inserted = 1;
    if (root == NULL) return create_node(value);

    Node* cur = root;
    while (1) {
        bst_build_cmp++;                    
        if (value == cur->data) {           /* 중복 */
            *inserted = 0;
            break;
        }
        if (value < cur->data) {
            if (cur->left == NULL) { cur->left = create_node(value); break; }
            cur = cur->left;
        }
        else {
            if (cur->right == NULL) { cur->right = create_node(value); break; }
            cur = cur->right;
        }
    }
    return root;
}

/* BST 탐색: 노드 방문마다 비교 1회 */
int bst_search(Node* root, int key, int* comparisons) {
    Node* cur = root;
    *comparisons = 0;
    while (cur != NULL) {
        (*comparisons)++;
        if (key == cur->data) return 1;
        else if (key < cur->data) cur = cur->left;
        else cur = cur->right;
    }
    return 0;
}

int height(Node* n) {
    if (!n) return 0;
    int l = height(n->left), r = height(n->right);
    return (l > r ? l : r) + 1;
}

void free_tree(Node* n) {
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}

/* ================= AVL ================= */
AVLNode* create_avl_node(int value) {
    AVLNode* n = (AVLNode*)malloc(sizeof(AVLNode));
    if (!n) { fprintf(stderr, "메모리 할당 실패\n"); exit(1); }
    n->data = value;
    n->h = 1;
    n->left = n->right = NULL;
    return n;
}

static int avl_h(AVLNode* n) { return n ? n->h : 0; }

static void avl_update(AVLNode* n) {
    int l = avl_h(n->left), r = avl_h(n->right);
    n->h = (l > r ? l : r) + 1;
}

static int balance_factor(AVLNode* n) {
    return n ? avl_h(n->left) - avl_h(n->right) : 0;
}

static AVLNode* rotate_right(AVLNode* y) {      /* 우회전 */
    AVLNode* x = y->left;
    y->left = x->right;
    x->right = y;
    avl_update(y);
    avl_update(x);
    return x;
}

static AVLNode* rotate_left(AVLNode* x) {       /* 좌회전 */
    AVLNode* y = x->right;
    x->right = y->left;
    y->left = x;
    avl_update(x);
    avl_update(y);
    return y;
}

static int avl_inserted;   /* 이번 삽입에서 실제로 삽입됐는지 */

static AVLNode* avl_insert_rec(AVLNode* node, int value) {
    if (node == NULL) {
        avl_inserted = 1;
        return create_avl_node(value);
    }
    avl_build_cmp++;                         
    if (value == node->data) {               /* 중복: 삽입 안 함 */
        avl_inserted = 0;
        return node;
    }
    if (value < node->data) node->left = avl_insert_rec(node->left, value);
    else                    node->right = avl_insert_rec(node->right, value);

    if (!avl_inserted) return node;          /* 중복이면 구조 변화 없음 */

    avl_update(node);
    int bf = balance_factor(node);           /* 비교 횟수에 포함하지 않음 */

    if (bf > 1) {
        if (balance_factor(node->left) < 0)  /* LR */
            node->left = rotate_left(node->left);
        return rotate_right(node);           /* LL: LR의 2단계 */
    }
    if (bf < -1) {
        if (balance_factor(node->right) > 0) /* RL */
            node->right = rotate_right(node->right);
        return rotate_left(node);            /* RR: RL의 2단계 */
    }
    return node;
}

AVLNode* avl_insert(AVLNode* root, int value, int* inserted) {
    avl_inserted = 0;
    root = avl_insert_rec(root, value);
    *inserted = avl_inserted;
    return root;
}

int avl_search(AVLNode* root, int key, int* comparisons) {
    AVLNode* cur = root;
    *comparisons = 0;
    while (cur != NULL) {
        (*comparisons)++;
        if (key == cur->data) return 1;
        else if (key < cur->data) cur = cur->left;
        else cur = cur->right;
    }
    return 0;
}

int avl_height(AVLNode* n) { return avl_h(n); }

void free_avl(AVLNode* n) {
    if (!n) return;
    free_avl(n->left);
    free_avl(n->right);
    free(n);
}

/* ================= main ================= */
int main(void) {
    srand((unsigned)time(NULL));

    int gen[TRY_COUNT];          /* 생성된 난수 100개 (중복 허용) */
    int arr[TRY_COUNT];          /* 서로 다른 값 저장 (발생 순서) */
    int arr_len = 0;
    Node* root = NULL;           /* BST */
    AVLNode* avl = NULL;         /* AVL */
    int dup_count = 0;

    /* 1~2. 난수 100번 생성 후 세 자료구조에 동일하게 삽입 시도 */
    for (int i = 0; i < TRY_COUNT; i++) {
        int v = rand() % (MAX_VALUE + 1);
        gen[i] = v;

        int dup = array_contains(arr, arr_len, v, &arr_build_cmp);
        if (!dup) arr[arr_len++] = v;
        else dup_count++;

        int b_ins, a_ins;
        root = insert(root, v, &b_ins);
        avl = avl_insert(avl, v, &a_ins);

        if (b_ins == dup || a_ins == dup) {   /* 중복 판정은 서로 일치해야 함 */
            fprintf(stderr, "오류: 중복 판정 불일치 (value=%d)\n", v);
            return 1;
        }
    }

    printf("===== 생성된 %d개의 정수 (발생 순서, 중복 포함) =====\n", TRY_COUNT);
    for (int i = 0; i < TRY_COUNT; i++)
        printf("%4d%s", gen[i], (i % 10 == 9) ? "\n" : " ");

    printf("\n실제로 저장된 서로 다른 값의 수 : %d\n", arr_len);
    printf("중복으로 삽입되지 않은 값의 수   : %d\n", dup_count);

    printf("\n===== 생성(삽입) 과정의 총 비교 횟수 =====\n");
    printf("배열 : %lld\n", arr_build_cmp);
    printf("BST  : %lld\n", bst_build_cmp);
    printf("AVL  : %lld\n", avl_build_cmp);

    printf("\n===== 자료구조의 크기 및 높이 =====\n");
    printf("배열의 길이     : %d\n", arr_len);
    printf("BST의 높이      : %d\n", height(root));
    printf("AVL 트리의 높이 : %d\n", avl_height(avl));

    /* 3. 탐색 대상 50개 생성 */
    int keys[SEARCH_COUNT];
    for (int i = 0; i < SEARCH_COUNT; i++)
        keys[i] = rand() % (MAX_VALUE + 1);

    printf("\n===== 생성된 %d개의 탐색 대상 =====\n", SEARCH_COUNT);
    for (int i = 0; i < SEARCH_COUNT; i++)
        printf("%4d%s", keys[i], (i % 10 == 9) ? "\n" : " ");

    /* 4. 탐색 및 비교 횟수 측정 */
    long long seq_total = 0, bst_total = 0, avl_total = 0;
    int found_count = 0;

    printf("\n===== 탐색 결과 =====\n");
    for (int i = 0; i < SEARCH_COUNT; i++) {
        int sc, bc, ac;
        int sf = sequential_search(arr, arr_len, keys[i], &sc);
        int bf = bst_search(root, keys[i], &bc);
        int af = avl_search(avl, keys[i], &ac);

        if (sf != bf || sf != af) {
            fprintf(stderr, "오류: 탐색 결과 불일치 (key=%d)\n", keys[i]);
            return 1;
        }
        if (sf) found_count++;
        seq_total += sc; bst_total += bc; avl_total += ac;

        const char* res = sf ? "Found" : "Not Found";
        printf("\nSearch Key : %d  (No. %d)\n", keys[i], i + 1);
        printf("Sequential Search\n  Result      : %s\n  Comparisons : %d\n", res, sc);
        printf("BST Search\n  Result      : %s\n  Comparisons : %d\n", res, bc);
        printf("AVL Search\n  Result      : %s\n  Comparisons : %d\n", res, ac);
    }

    /* 5. 요약 표 */
    printf("\n===== 탐색 결과 요약 표 =====\n");
    printf("%-4s %-10s %-10s %-12s %-12s %-12s\n",
        "No.", "SearchKey", "Result", "Sequential", "BST", "AVL");
    for (int i = 0; i < SEARCH_COUNT; i++) {
        int sc, bc, ac;
        int f = sequential_search(arr, arr_len, keys[i], &sc);
        bst_search(root, keys[i], &bc);
        avl_search(avl, keys[i], &ac);
        printf("%-4d %-10d %-10s %-12d %-12d %-12d\n",
            i + 1, keys[i], f ? "Found" : "Not Found", sc, bc, ac);
    }

    printf("\nNumber of searches: %d (Found: %d, Not Found: %d)\n",
        SEARCH_COUNT, found_count, SEARCH_COUNT - found_count);

    printf("\nSequential Search\n");
    printf("Total comparisons   : %lld\n", seq_total);
    printf("Average comparisons : %.2f\n", (double)seq_total / SEARCH_COUNT);

    printf("\nBST Search\n");
    printf("Total comparisons   : %lld\n", bst_total);
    printf("Average comparisons : %.2f\n", (double)bst_total / SEARCH_COUNT);

    printf("\nAVL Search\n");
    printf("Total comparisons   : %lld\n", avl_total);
    printf("Average comparisons : %.2f\n", (double)avl_total / SEARCH_COUNT);

    free_tree(root);
    free_avl(avl);
    return 0;
}
