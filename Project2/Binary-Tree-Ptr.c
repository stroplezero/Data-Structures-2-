#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define MAX_INPUT         1024                  /* 트리 입력 최대 길이(널 포함) */
#ifndef REJECT_DUPLICATES
#define REJECT_DUPLICATES 1                     /* 1: 같은 문자 노드 중복 금지, 0: 허용 */
#endif
#define IS_NODE(c)        ((c) >= 'A' && (c) <= 'Z')

typedef struct Node {
    char         data;
    struct Node *left;
    struct Node *right;
} Node;

typedef struct {
    int nodes, leaves, nonleaves, height, degree;
} TreeInfo;

typedef enum { READ_TOO_LONG = -1, READ_EOF = 0, READ_OK = 1 } ReadResult;

/* ---------------- 트리 해제 ---------------- */
static void free_tree(Node *n)
{
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}

/* ---------------- 입력 읽기 ---------------- */

/* 버퍼보다 긴 줄은 나머지를 버리고 READ_TOO_LONG 반환 */
static ReadResult read_line(char *buf, size_t size)
{
    size_t len;

    if (!fgets(buf, (int)size, stdin)) return READ_EOF;

    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
        if (len > 0 && buf[len - 1] == '\r') buf[--len] = '\0';
        return READ_OK;
    }
    if (len == size - 1) {               /* 버퍼가 가득 찼는데 줄바꿈이 없음 */
        int c, over = 0;
        while ((c = getchar()) != '\n' && c != EOF) over = 1;
        if (over) return READ_TOO_LONG;
    }
    return READ_OK;                      /* 마지막 줄에 개행이 없는 경우 */
}

/* ---------------- 파싱 ---------------- */
static const char   *src;               /* 입력 문자열 */
static int           pos;               /* 현재 읽는 위치 */
static char          g_errmsg[160];     /* 오류 메시지 */
static int           err_pos;           /* 오류 위치 */
static unsigned char seen[26];          /* 노드 중복 검사용 */

/* 오류를 기록하고, 만들던 노드(있으면)를 해제한 뒤 NULL 반환 */
static Node *fail(Node *partial, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_errmsg, sizeof g_errmsg, fmt, ap);
    va_end(ap);
    err_pos = pos;
    free_tree(partial);
    return NULL;
}

/* 하위에서 이미 오류가 기록된 경우: 메시지는 그대로 두고 노드만 해제 */
static Node *discard(Node *partial)
{
    free_tree(partial);
    return NULL;
}

/* 현재 위치의 문자를 오류 메시지용 문자열로 */
static const char *cur_desc(void)
{
    static char d[16];
    unsigned char c = (unsigned char)src[pos];

    if (c >= 0x80)   return "ASCII가 아닌 문자";
    if (isprint(c))  snprintf(d, sizeof d, "'%c'", c);
    else             snprintf(d, sizeof d, "제어 문자");
    return d;
}

static void skip_ws(void)
{
    while (isspace((unsigned char)src[pos])) pos++;
}

/*
 * 노드 하나를 읽어 만들고, 자식이 있으면 재귀로 연결함.
 * 성공하면 노드 포인터, 실패하면 NULL (이미 만든 노드는 모두 해제됨)
 */
static Node *parse_node(void)
{
    Node *n;
    char c;

    skip_ws();
    c = src[pos];
    if (c == '\0')
        return fail(NULL, "노드가 필요한데 입력이 끝났습니다");
    if (!IS_NODE(c))
        return fail(NULL, "노드는 영문 대문자여야 합니다 (입력된 문자: %s)", cur_desc());

    if (REJECT_DUPLICATES) {
        if (seen[c - 'A'])
            return fail(NULL, "노드 '%c'가 중복되었습니다", c);
        seen[c - 'A'] = 1;
    }

    n = (Node *)calloc(1, sizeof *n);   /* left, right는 NULL로 초기화 */
    if (!n)
        return fail(NULL, "메모리가 부족합니다");
    n->data = c;
    pos++;
    skip_ws();
    if (src[pos] != '(') return n;     
    pos++;
    skip_ws();

    if (src[pos] == ')')
        return fail(n, "빈 괄호는 사용할 수 없습니다");

    if (src[pos] != ',') {              
        n->left = parse_node();
        if (!n->left) return discard(n);
        skip_ws();
    }
    if (src[pos] == ',') {            
        pos++;
        skip_ws();
        if (src[pos] != ')') {
            n->right = parse_node();
            if (!n->right) return discard(n);
            skip_ws();
        }
    }

    if (!n->left && !n->right)
        return fail(n, "자식이 하나도 없는 괄호입니다");

    if (src[pos] != ')') {
        if (src[pos] == ',')
            return fail(n, "이진트리는 자식을 최대 2개까지만 가질 수 있습니다");
        if (src[pos] == '\0')
            return fail(n, "닫는 괄호 ')'가 빠졌습니다");
        if (IS_NODE(src[pos]))
            return fail(n, "자식 사이에 ','가 빠졌습니다");
        return fail(n, "')'가 필요합니다 (입력된 문자: %s)", cur_desc());
    }
    pos++;
    return n;
}

/* 문자열 s를 파싱해 루트 포인터를 반환. 실패하면 NULL */
static Node *parse_tree(const char *s)
{
    Node *root;

    memset(seen, 0, sizeof seen);
    src = s;
    pos = 0;
    g_errmsg[0] = '\0';

    skip_ws();
    if (src[pos] == '\0')
        return fail(NULL, "입력이 비어 있습니다");

    root = parse_node();
    if (!root) return NULL;

    skip_ws();
    if (src[pos] != '\0') {
        if (src[pos] == ')')
            return fail(root, "짝이 맞지 않는 ')'가 있습니다");
        return fail(root, "트리가 끝난 뒤에 불필요한 문자가 있습니다 (입력된 문자: %s)", cur_desc());
    }
    return root;
}

static void print_parse_error(const char *input)
{
    printf("입력 오류: %s (위치 %d)\n  %s\n  ", g_errmsg, err_pos + 1, input);
    for (int i = 0; i < err_pos; i++) putchar(' ');
    puts("^");
}

/* 올바른 트리가 입력될 때까지 반복. 성공하면 루트, 입력 종료(EOF)면 NULL */
static Node *input_tree(void)
{
    char buf[MAX_INPUT];

    for (;;) {
        printf("괄호 표기법으로 이진트리를 입력하세요 (예: A(B(E,F),C(,G))): ");
        fflush(stdout);

        ReadResult r = read_line(buf, sizeof buf);
        if (r == READ_EOF) {
            printf("\n");
            return NULL;
        }
        if (r == READ_TOO_LONG) {
            printf("입력 오류: 입력이 너무 깁니다 (최대 %d자)\n", MAX_INPUT - 1);
            continue;
        }

        Node *root = parse_tree(buf);
        if (root) return root;
        print_parse_error(buf);
    }
}

/* ---------------- [2] 트리 정보 ---------------- */
/* 전위 순회하며 개수, 높이(레벨 수), 차수를 구한다. 루트의 depth = 1 */
static void collect(const Node *n, int depth, TreeInfo *info)
{
    int children = (n->left != NULL) + (n->right != NULL);

    info->nodes++;
    if (children == 0) info->leaves++;
    if (children > info->degree) info->degree = children;
    if (depth > info->height) info->height = depth;

    if (n->left)  collect(n->left,  depth + 1, info);
    if (n->right) collect(n->right, depth + 1, info);
}

static TreeInfo get_info(const Node *root)
{
    TreeInfo info = {0};

    collect(root, 1, &info);
    info.nonleaves = info.nodes - info.leaves;
    return info;
}

static void print_info(const TreeInfo *info)
{
    printf("1. 전체 노드의 수     : %d\n", info->nodes);
    printf("2. 단말 노드의 수     : %d\n", info->leaves);
    printf("3. 비단말 노드의 수   : %d\n", info->nonleaves);
    printf("4. 트리의 높이        : %d\n", info->height);
    printf("5. 트리의 차수        : %d\n", info->degree);
}

/* ---------------- [1] 트리 출력 ---------------- */
/*
 * prefix: 현재 줄 앞에 붙일 "|   " / "    " 들이 쌓인 버퍼 (len = 현재 길이)
 */
static void print_children(const Node *n, char *prefix, int len)
{
    const Node *kids[2];
    int k = 0;

    if (n->left)  kids[k++] = n->left;
    if (n->right) kids[k++] = n->right;

    for (int i = 0; i < k; i++) {
        printf("%s+---%c\n", prefix, kids[i]->data);
        memcpy(prefix + len, (i == k - 1) ? "    " : "|   ", 5);   
        print_children(kids[i], prefix, len + 4);
        prefix[len] = '\0';
    }
}

static void print_tree(const Node *root, int height)
{
    char *prefix = (char *)malloc((size_t)height * 4 + 1);

    if (!prefix) {
        printf("메모리가 부족하여 트리를 출력할 수 없습니다.\n");
        return;
    }
    prefix[0] = '\0';
    printf("%c\n", root->data);
    print_children(root, prefix, 0);
    free(prefix);
}

/* ---------------- [3] 형태 판별 ---------------- */
/*
 * 완전 이진트리: 각 노드에 "루트=1, 왼쪽=2i, 오른쪽=2i+1" 번호를 매겼을 때
 * 모든 번호가 1 ~ n 안에 들어오면 완전 이진트리.
 */
static int is_complete_rec(const Node *n, int idx, int count)
{
    if (!n) return 1;
    if (idx > count) return 0;
    return is_complete_rec(n->left,  2 * idx,     count)
        && is_complete_rec(n->right, 2 * idx + 1, count);
}

/* 포화 이진트리: 모든 단말이 마지막 레벨에 있고, 모든 비단말이 자식을 2개 가짐 */
static int is_full_rec(const Node *n, int depth, int height)
{
    if (!n->left && !n->right) return depth == height;
    if (!n->left || !n->right) return 0;
    return is_full_rec(n->left,  depth + 1, height)
        && is_full_rec(n->right, depth + 1, height);
}

/* 편향 이진트리: 노드가 2개 이상이고 모든 노드가 왼쪽 자식만(또는 오른쪽 자식만) 가짐
 * 반환: 1 = 왼쪽 편향, 2 = 오른쪽 편향, 0 = 아님 */
static int skew_type(const Node *root, int n)
{
    const Node *p;
    int left_only = 1, right_only = 1;

    if (n < 2) return 0;
    for (p = root; p; p = p->left)      /* 왼쪽 가지를 따라가며 오른쪽 자식이 있는지 */
        if (p->right) { left_only = 0; break; }
    for (p = root; p; p = p->right)     /* 오른쪽 가지를 따라가며 왼쪽 자식이 있는지 */
        if (p->left) { right_only = 0; break; }
    return left_only ? 1 : (right_only ? 2 : 0);
}

static void print_shape(const Node *root, const TreeInfo *info)
{
    int skew = skew_type(root, info->nodes);

    printf("완전 이진트리 : %s\n", is_complete_rec(root, 1, info->nodes) ? "예" : "아니오");
    printf("포화 이진트리 : %s\n", is_full_rec(root, 1, info->height) ? "예" : "아니오");
    printf("편향 이진트리 : %s\n",
           skew == 1 ? "예 (왼쪽 편향)" :
           skew == 2 ? "예 (오른쪽 편향)" : "아니오");
}

/* ---------------- main ---------------- */
int main(void)
{
    Node *root = input_tree();
    TreeInfo info;

    if (!root) {
        printf("입력이 없어 프로그램을 종료합니다.\n");
        return 0;
    }
    info = get_info(root);

    printf("\n[1] 이진트리 출력\n");
    print_tree(root, info.height);

    printf("\n[2] 트리 정보\n");
    print_info(&info);

    printf("\n[3] 이진트리 형태 판별\n");
    print_shape(root, &info);

    free_tree(root);
    return 0;
}
