#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DATA_COUNT 100
#define SEARCH_COUNT 50
#define MAX_VALUE 1000

/* ---------- BST 노드 (연결 자료구조) ---------- */
typedef struct Node {
    int data;
    struct Node* left;
    struct Node* right;
} Node;

static long long build_comparisons = 0;  /* BST 생성 비교 횟수 */

Node* create_node(int value) {
    Node* n = (Node*)malloc(sizeof(Node));
    if (!n) { fprintf(stderr, "메모리 할당 실패\n"); exit(1); }
    n->data = value;
    n->left = n->right = NULL;
    return n;
}

/* 삽입: 기존 노드와 값을 비교할 때마다 1회로 계산 */
Node* insert(Node* root, int value) {
    if (root == NULL) return create_node(value);

    Node* cur = root;
    while (1) {
        build_comparisons++;               /* value vs cur->data */
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
/* 탐색 성공 시 1, 실패 시 0 반환 */
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

/* 순차 탐색: 원소와 비교할 때마다 1회 */
/* 탐색 성공 시 1, 실패 시 0 반환 */
int sequential_search(const int arr[], int n, int key, int* comparisons) {
    *comparisons = 0;
    for (int i = 0; i < n; i++) {
        (*comparisons)++;
        if (arr[i] == key) return 1;
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

int main(void) {
    srand((unsigned)time(NULL));

    int arr[DATA_COUNT];
    int used[MAX_VALUE + 1] = { 0 };

    /* 1. 서로 다른 100개 정수 생성 (발생 순서대로 배열 저장) */
    for (int i = 0; i < DATA_COUNT; ) {
        int v = rand() % (MAX_VALUE + 1);
        if (used[v]) continue;             /* 중복이면 재생성 */
        used[v] = 1;
        arr[i++] = v;
    }

    /* BST 삽입 (발생 순서대로) */
    Node* root = NULL;
    for (int i = 0; i < DATA_COUNT; i++)
        root = insert(root, arr[i]);

    printf("===== 생성된 %d개의 서로 다른 정수 (발생 순서) =====\n", DATA_COUNT);
    for (int i = 0; i < DATA_COUNT; i++) {
        printf("%4d%s", arr[i], (i % 10 == 9) ? "\n" : " ");
    }
    printf("\nBST 생성 과정의 총 비교 횟수 : %lld\n", build_comparisons);
    printf("BST 높이                     : %d\n", height(root));

    /* 2. 탐색 대상 50개 생성 */
    int keys[SEARCH_COUNT];
    for (int i = 0; i < SEARCH_COUNT; i++)
        keys[i] = rand() % (MAX_VALUE + 1);

    printf("\n===== 생성된 %d개의 탐색 대상 =====\n", SEARCH_COUNT);
    for (int i = 0; i < SEARCH_COUNT; i++)
        printf("%4d%s", keys[i], (i % 10 == 9) ? "\n" : " ");

    /* 3~4. 탐색 및 비교 횟수 측정 */
    long long seq_total = 0, bst_total = 0;
    int seq_min = 1 << 30, seq_max = 0, bst_min = 1 << 30, bst_max = 0;
    int found_count = 0;

    printf("\n===== 탐색 결과 =====\n");
    printf("%-4s %-10s %-10s %-18s %-18s\n",
        "No.", "SearchKey", "Result", "Sequential Comp.", "BST Comp.");

    for (int i = 0; i < SEARCH_COUNT; i++) {
        int sc, bc;
        int sf = sequential_search(arr, DATA_COUNT, keys[i], &sc);
        int bf = bst_search(root, keys[i], &bc);

        if (sf != bf) {   /* 두 방법의 결과(탐색 성공/실패)는 항상 같아야 함 */
            fprintf(stderr, "오류: 탐색 결과 불일치 (key=%d)\n", keys[i]);
            return 1;
        }
        if (sf) found_count++;

        seq_total += sc; bst_total += bc;
        if (sc < seq_min) seq_min = sc;
        if (sc > seq_max) seq_max = sc;
        if (bc < bst_min) bst_min = bc;
        if (bc > bst_max) bst_max = bc;

        printf("%-4d %-10d %-10s %-18d %-18d\n",
            i + 1, keys[i], sf ? "Found" : "Not Found", sc, bc);
    }

    /* 5~6. 요약 출력 */
    printf("\nNumber of searches: %d (Found: %d, Not Found: %d)\n",
        SEARCH_COUNT, found_count, SEARCH_COUNT - found_count);

    printf("\nSequential Search\n");
    printf("Total comparisons   : %lld\n", seq_total);
    printf("Average comparisons : %.2f\n", (double)seq_total / SEARCH_COUNT);
    printf("Min / Max           : %d / %d\n", seq_min, seq_max);

    printf("\nBST Search\n");
    printf("Total comparisons   : %lld\n", bst_total);
    printf("Average comparisons : %.2f\n", (double)bst_total / SEARCH_COUNT);
    printf("Min / Max           : %d / %d\n", bst_min, bst_max);

    printf("\n===== 비용 분석 (BST 생성 비용 포함) =====\n");
    printf("BST 생성 비교 횟수           : %lld\n", build_comparisons);
    printf("BST 생성 + 50회 탐색 합계    : %lld\n", build_comparisons + bst_total);
    printf("순차 탐색 50회 합계          : %lld\n", seq_total);
    long long diff = seq_total - (build_comparisons + bst_total);
    if (diff >= 0)
        printf("BST(생성 포함)가 %lld회 더 적음\n", diff);
    else
        printf("순차 탐색이 %lld회 더 적음\n", -diff);

    free_tree(root);
    return 0;
}
