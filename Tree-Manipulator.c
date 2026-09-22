/*
 * tree_manipulator.c
 * 이진트리 조작 프로그램
 *
 * 메뉴: Insert(I), Delete(D), Update(U), Read(R), Print(P)
 * 경로 표기: /A/B/C  (루트부터 목표 노드까지의 데이터를 '/'로 구분하여 나열)
 *
 * 컴파일: gcc -o tree_manipulator tree_manipulator.c
 * 실행:   ./tree_manipulator
 */

#define _CRT_SECURE_NO_WARNINGS  /* MSVC: strtok/strncpy/sprintf 등을 "안전하지 않음" 경고 없이 사용 (표준 C 함수이므로 gcc 등 다른 컴파일러에는 영향 없음) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE      256
#define DEFAULT_SIZE  1000   /* create_btree(size)의 기본 용량 */
#define MAX_TOKENS    10

 /* ================= 이진트리 ADT ================= */

typedef struct Node {
    char data;
    struct Node* left;
    struct Node* right;
} Node;

typedef struct {
    Node* root;
    int   size;   /* 최대 노드 수 */
    int   count;  /* 현재 노드 수 */
} BTree;

/* 경로로 찾은 노드 정보: 노드 자신, 부모, 부모 기준 좌/우 여부 */
typedef struct {
    Node* node;     /* 찾은 노드 (없으면 NULL) */
    Node* parent;   /* 부모 노드 (루트이면 NULL) */
    int   is_left;  /* 부모의 왼쪽 자식=1, 오른쪽 자식=0, 루트=-1 */
} FindResult;

/* size개의 노드를 저장할 수 있는 빈 이진트리를 생성하여 반환 */
BTree* create_btree(int size) {
    BTree* tree = (BTree*)malloc(sizeof(BTree));
    if (!tree) { fprintf(stderr, "메모리 할당 실패\n"); exit(1); }
    tree->root = NULL;
    tree->size = size;
    tree->count = 0;
    return tree;
}

/* 빈 tree에 데이터가 value인 루트 노드 생성 */
BTree* insert_root(BTree* tree, char value) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->data = value;
    node->left = node->right = NULL;
    tree->root = node;
    tree->count++;
    return tree;
}

/* parent 아래 child('L'/'R') 위치에 새 노드 추가 */
BTree* insert_child(BTree* tree, Node* parent, char child, char value) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->data = value;
    node->left = node->right = NULL;
    if (child == 'L') parent->left = node;
    else parent->right = node;
    tree->count++;
    return tree;
}

/* leaf 노드를 부모로부터 분리하고 삭제 */
BTree* delete_node(BTree* tree, Node* parent, int is_left, Node* leaf) {
    if (is_left == -1)      tree->root = NULL;   /* 루트 삭제 -> 빈 트리 */
    else if (is_left == 1)  parent->left = NULL;
    else                    parent->right = NULL;
    free(leaf);
    tree->count--;
    return tree;
}

/* node의 데이터를 value로 변경 */
BTree* update_value(BTree* tree, Node* node, char value) {
    node->data = value;
    return tree;
}

/* parent의 왼쪽/오른쪽 자식 데이터를 반환 (없으면 '\0') */
void read_child(Node* parent, char* left_out, char* right_out) {
    *left_out = parent->left ? parent->left->data : '\0';
    *right_out = parent->right ? parent->right->data : '\0';
}

/* 트리를 왼쪽으로 눕힌 형태로 재귀 출력 */
static void print_node(Node* node, int level) {
    if (node == NULL) return;
    if (level > 0) {
        for (int i = 0; i < level - 1; i++) printf("    ");
        printf("+---");
    }
    printf("%c\n", node->data);
    print_node(node->left, level + 1);
    print_node(node->right, level + 1);
}

void print_btree(BTree* tree) {
    if (tree->root == NULL) { printf("트리가 비어 있습니다.\n"); return; }
    print_node(tree->root, 0);
}

static void destroy_node(Node* node) {
    if (node == NULL) return;
    destroy_node(node->left);
    destroy_node(node->right);
    free(node);
}

/* tree의 모든 노드를 제거하고 tree 자체도 해제 */
void destroy_btree(BTree* tree) {
    destroy_node(tree->root);
    free(tree);
}

/* ================= 경로 탐색 ================= */

/* path가 "/X/Y/Z" 형식(각 구간은 영문 대문자 한 글자)인지 검사 */
int is_valid_path(const char* path) {
    int len = (int)strlen(path);
    if (len < 2 || path[0] != '/') return 0;
    int seg_len = 0;
    for (int i = 1; i <= len; i++) {
        if (path[i] == '/' || path[i] == '\0') {
            if (seg_len != 1) return 0;
            if (path[i] == '\0') break;
            seg_len = 0;
        }
        else {
            if (!isupper((unsigned char)path[i])) return 0;
            seg_len++;
        }
    }
    return 1;
}

/* path로 노드를 탐색 (경로가 존재하지 않으면 node=NULL) */
FindResult find_path(BTree* tree, const char* path) {
    FindResult result = { NULL, NULL, -1 };
    if (tree->root == NULL) return result;
    if (!is_valid_path(path)) return result;

    char buf[MAX_LINE];
    strncpy(buf, path, MAX_LINE - 1);
    buf[MAX_LINE - 1] = '\0';

    char* token = strtok(buf + 1, "/");
    if (token == NULL || token[0] != tree->root->data) return result;

    Node* cur = tree->root;
    Node* parent = NULL;
    int is_left = -1;

    token = strtok(NULL, "/");
    while (token != NULL) {
        char c = token[0];
        Node* next = NULL;
        int next_is_left = -1;
        if (cur->left && cur->left->data == c) { next = cur->left;  next_is_left = 1; }
        else if (cur->right && cur->right->data == c) { next = cur->right; next_is_left = 0; }
        if (next == NULL) { result.node = NULL; return result; }
        parent = cur;
        cur = next;
        is_left = next_is_left;
        token = strtok(NULL, "/");
    }

    result.node = cur;
    result.parent = parent;
    result.is_left = is_left;
    return result;
}

/* ================= 명령어 처리 ================= */

/* 대소문자를 구분하지 않는 문자열 비교 (strcasecmp 대체: 표준 C만 사용) */
int str_equal_ignore_case(const char* a, const char* b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

int match_cmd(const char* token, const char* full, const char* abbr) {
    return str_equal_ignore_case(token, full) || str_equal_ignore_case(token, abbr);
}

/* "/ A" 처럼 '/' 바로 뒤에 공백이 들어간 입력을 "/A"로 정규화 */
void normalize_line(char* line) {
    char result[MAX_LINE];
    int j = 0;
    int len = (int)strlen(line);
    for (int i = 0; i < len; i++) {
        result[j++] = line[i];
        if (line[i] == '/') {
            while (i + 1 < len && line[i + 1] == ' ') i++;
        }
    }
    result[j] = '\0';
    strcpy(line, result);
}

void handle_insert(BTree* tree, char** tok, int n) {
    if (n == 2) {
        /* Insert /A : 루트 생성 */
        if (tree->root != NULL) {
            printf("오류: 이미 루트 노드가 존재합니다.\n");
            return;
        }
        char* path = tok[1];
        if (strlen(path) != 2 || path[0] != '/' || !isupper((unsigned char)path[1])) {
            printf("오류: 경로 형식이 올바르지 않습니다.\n");
            return;
        }
        insert_root(tree, path[1]);
        printf("루트 노드 %c가 생성되었습니다.\n", path[1]);
        return;
    }

    if (n == 4) {
        char* path = tok[1];
        char* child_tok = tok[2];
        char* value_tok = tok[3];

        if (tree->root == NULL) {
            printf("오류: 트리가 비어 있습니다. 먼저 루트 노드를 생성하세요.\n");
            return;
        }
        if (strlen(value_tok) != 1 || !isupper((unsigned char)value_tok[0])) {
            printf("오류: new-data는 영문 대문자 한 글자여야 합니다.\n");
            return;
        }

        int is_left_child;
        if (match_cmd(child_tok, "left", "l")) is_left_child = 1;
        else if (match_cmd(child_tok, "right", "r")) is_left_child = 0;
        else { printf("오류: child는 L 또는 R이어야 합니다.\n"); return; }

        FindResult fr = find_path(tree, path);
        if (fr.node == NULL) { printf("오류: parent-node 경로가 존재하지 않습니다.\n"); return; }

        Node* parent = fr.node;
        if (parent->left != NULL && parent->right != NULL) {
            printf("오류: 부모 노드의 자식이 이미 2개입니다.\n");
            return;
        }
        if (is_left_child && parent->left != NULL) {
            printf("오류: 왼쪽 자식이 이미 존재합니다.\n");
            return;
        }
        if (!is_left_child && parent->right != NULL) {
            printf("오류: 오른쪽 자식이 이미 존재합니다.\n");
            return;
        }

        char value = value_tok[0];
        char sibling = is_left_child ? (parent->right ? parent->right->data : '\0')
            : (parent->left ? parent->left->data : '\0');
        if (sibling == value) {
            printf("오류: 동일한 부모의 자식끼리 같은 데이터를 가질 수 없습니다.\n");
            return;
        }

        insert_child(tree, parent, is_left_child ? 'L' : 'R', value);
        printf("%s/%c 노드가 추가되었습니다.\n", path, value);
        return;
    }

    printf("오류: Insert 명령의 인자 개수가 올바르지 않습니다.\n");
}

void handle_delete(BTree* tree, char** tok, int n) {
    if (n != 2) { printf("오류: Delete 명령의 인자 개수가 올바르지 않습니다.\n"); return; }
    if (tree->root == NULL) { printf("오류: 트리가 비어 있습니다.\n"); return; }

    char* path = tok[1];
    FindResult fr = find_path(tree, path);
    if (fr.node == NULL) { printf("오류: 해당 경로의 노드가 존재하지 않습니다.\n"); return; }
    if (fr.node->left != NULL || fr.node->right != NULL) {
        printf("오류: 단말 노드가 아니므로 삭제할 수 없습니다.\n");
        return;
    }

    delete_node(tree, fr.parent, fr.is_left, fr.node);
    printf("%s 노드가 삭제되었습니다.\n", path);
}

void handle_update(BTree* tree, char** tok, int n) {
    if (n != 3) { printf("오류: Update 명령의 인자 개수가 올바르지 않습니다.\n"); return; }
    if (tree->root == NULL) { printf("오류: 트리가 비어 있습니다.\n"); return; }

    char* path = tok[1];
    char* value_tok = tok[2];
    if (strlen(value_tok) != 1 || !isupper((unsigned char)value_tok[0])) {
        printf("오류: new-data는 영문 대문자 한 글자여야 합니다.\n");
        return;
    }

    FindResult fr = find_path(tree, path);
    if (fr.node == NULL) { printf("오류: 해당 경로의 노드가 존재하지 않습니다.\n"); return; }

    char value = value_tok[0];
    char sibling = '\0';
    if (fr.parent != NULL) {
        if (fr.is_left == 1 && fr.parent->right) sibling = fr.parent->right->data;
        else if (fr.is_left == 0 && fr.parent->left) sibling = fr.parent->left->data;
    }
    if (sibling == value) {
        printf("오류: 동일한 부모의 자식끼리 같은 데이터를 가질 수 없습니다.\n");
        return;
    }

    update_value(tree, fr.node, value);

    char new_path[MAX_LINE];
    strncpy(new_path, path, MAX_LINE - 1);
    new_path[MAX_LINE - 1] = '\0';
    new_path[strlen(new_path) - 1] = value;   /* 마지막 한 글자를 새 데이터로 교체 */

    printf("%s의 데이터가 %c로 변경되었습니다. (새 경로: %s)\n", path, value, new_path);
}

void handle_read(BTree* tree, char** tok, int n) {
    if (n != 2) { printf("오류: Read 명령의 인자 개수가 올바르지 않습니다.\n"); return; }
    if (tree->root == NULL) { printf("오류: 트리가 비어 있습니다.\n"); return; }

    char* path = tok[1];
    FindResult fr = find_path(tree, path);
    if (fr.node == NULL) { printf("오류: 해당 경로의 노드가 존재하지 않습니다.\n"); return; }

    char l, r;
    read_child(fr.node, &l, &r);

    if (l == '\0' && r == '\0') {
        printf("자식 노드가 없습니다.\n");
        return;
    }

    char buf[32] = "";
    if (l != '\0') {
        char tmp[16];
        sprintf(tmp, "%c(L)", l);
        strcat(buf, tmp);
    }
    if (r != '\0') {
        char tmp[16];
        if (buf[0] != '\0') strcat(buf, ", ");
        sprintf(tmp, "%c(R)", r);
        strcat(buf, tmp);
    }
    printf("%s\n", buf);
}

/* ================= 메인 루프 ================= */

int main(void) {
    BTree* tree = create_btree(DEFAULT_SIZE);
    char line[MAX_LINE];

    printf("=== 이진트리 조작 프로그램 ===\n");
    printf("명령어: Insert(I), Delete(D), Update(U), Read(R), Print(P), Quit(Q)\n\n");

    while (1) {
        printf("> ");
        if (fgets(line, sizeof(line), stdin) == NULL) break;   /* EOF */

        line[strcspn(line, "\n")] = '\0';
        normalize_line(line);

        char* tok[MAX_TOKENS];
        int n = 0;
        char* p = strtok(line, " \t");
        while (p != NULL && n < MAX_TOKENS) { tok[n++] = p; p = strtok(NULL, " \t"); }
        if (n == 0) continue;

        if (match_cmd(tok[0], "insert", "i"))      handle_insert(tree, tok, n);
        else if (match_cmd(tok[0], "delete", "d"))  handle_delete(tree, tok, n);
        else if (match_cmd(tok[0], "update", "u"))  handle_update(tree, tok, n);
        else if (match_cmd(tok[0], "read", "r"))    handle_read(tree, tok, n);
        else if (match_cmd(tok[0], "print", "p")) {
            if (n != 1) printf("오류: Print 명령은 추가 인자를 받지 않습니다.\n");
            else print_btree(tree);
        }
        else if (match_cmd(tok[0], "quit", "q")) break;
        else printf("오류: 알 수 없는 명령어입니다.\n");
    }

    destroy_btree(tree);
    printf("프로그램을 종료합니다.\n");
    return 0;
}