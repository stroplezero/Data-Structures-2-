#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define MAX_HEIGHT        24                    /* 배열로 표현 가능한 최대 높이 */
#define MAX_INDEX         ((1 << MAX_HEIGHT) - 1)
#define MAX_INPUT         1024                  /* 트리 입력 최대 길이(널 포함) */
#define REJECT_DUPLICATES 1                     /* 1: 같은 문자 노드 중복 금지, 0: 허용 */
#define IS_NODE(c)        ((c) >= 'A' && (c) <= 'Z')

typedef struct {
    char *a;        /* 트리 배열 (1-based, 빈 칸은 '\0') */
    int   cap;      /* 배열 크기 */
    int   max_idx;  /* 노드가 있는 가장 큰 인덱스 */
    int   n;        /* 전체 노드 수 */
} ArrTree;

typedef struct {
    int nodes, leaves, nonleaves, height, degree;
} TreeInfo;

typedef enum { READ_TOO_LONG = -1, READ_EOF = 0, READ_OK = 1 } ReadResult;

/* ---------------- 입력 읽기 ---------------- */

/* 한 줄을 읽는다. 버퍼보다 긴 줄은 나머지를 버리고 READ_TOO_LONG 반환 */
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

static int fail(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_errmsg, sizeof g_errmsg, fmt, ap);
    va_end(ap);
    err_pos = pos;
    return 0;
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

/* idx 위치까지 배열이 들어가도록 확장 (새 칸은 0으로 초기화) */
static int ensure(ArrTree *t, int idx)
{
    int ncap;
    char *na;

    if (idx > MAX_INDEX)
        return fail("트리가 너무 깊어 배열로 표현할 수 없습니다 (최대 높이 %d)", MAX_HEIGHT);
    if (idx < t->cap) return 1;

    ncap = t->cap ? t->cap : 16;
    while (ncap <= idx) ncap *= 2;

    na = (char *)realloc(t->a, (size_t)ncap);
    if (!na) return fail("메모리가 부족합니다");
    memset(na + t->cap, 0, (size_t)(ncap - t->cap));
    t->a = na;
    t->cap = ncap;
    return 1;
}

/*
 * 노드 하나를 읽어 idx 위치에 저장하고, 자식이 있으면 재귀 처리
 */
static int parse_node(ArrTree *t, int idx)
{
    int kids = 0;
    char c;

    skip_ws();
    c = src[pos];
    if (c == '\0')
        return fail("노드가 필요한데 입력이 끝났습니다");
    if (!IS_NODE(c))
        return fail("노드는 영문 대문자여야 합니다 (입력된 문자: %s)", cur_desc());
    if (!ensure(t, idx)) return 0;

    if (REJECT_DUPLICATES) {
        if (seen[c - 'A'])
            return fail("노드 '%c'가 중복되었습니다", c);
        seen[c - 'A'] = 1;
    }

    t->a[idx] = c;
    pos++;
    skip_ws();
    if (src[pos] != '(') return 1;     
    pos++;
    skip_ws();

    if (src[pos] == ')')
        return fail("빈 괄호는 사용할 수 없습니다");

    if (src[pos] != ',') {             
        if (!parse_node(t, 2 * idx)) return 0;
        kids++;
        skip_ws();
    }
    if (src[pos] == ',') {             
        pos++;
        skip_ws();
        if (src[pos] != ')') {
            if (!parse_node(t, 2 * idx + 1)) return 0;
            kids++;
            skip_ws();
        }
    }

    if (kids == 0)
        return fail("자식이 하나도 없는 괄호입니다");

    if (src[pos] != ')') {
        if (src[pos] == ',')
            return fail("이진트리는 자식을 최대 2개까지만 가질 수 있습니다");
        if (src[pos] == '\0')
            return fail("닫는 괄호 ')'가 빠졌습니다");
        if (IS_NODE(src[pos]))
            return fail("자식 사이에 ','가 빠졌습니다");
        return fail("')'가 필요합니다 (입력된 문자: %s)", cur_desc());
    }
    pos++;
    return 1;
}

/* 문자열 s를 파싱해 out(0으로 초기화된 상태)에 저장. 성공 시 1 */
static int parse_tree(ArrTree *out, const char *s)
{
    memset(seen, 0, sizeof seen);
    src = s;
    pos = 0;
    g_errmsg[0] = '\0';

    skip_ws();
    if (src[pos] == '\0')
        return fail("입력이 비어 있습니다");

    if (!parse_node(out, 1)) return 0;

    skip_ws();
    if (src[pos] != '\0') {
        if (src[pos] == ')')
            return fail("짝이 맞지 않는 ')'가 있습니다");
        return fail("트리가 끝난 뒤에 불필요한 문자가 있습니다 (입력된 문자: %s)", cur_desc());
    }

    for (int i = 1; i < out->cap; i++) {
        if (out->a[i]) {
            out->n++;
            out->max_idx = i;
        }
    }
    return 1;
}

static void print_parse_error(const char *input)
{
    printf("입력 오류: %s (위치 %d)\n  %s\n  ", g_errmsg, err_pos + 1, input);
    for (int i = 0; i < err_pos; i++) putchar(' ');
    puts("^");
}

/* 올바른 트리가 입력될 때까지 반복. 성공 1, 입력 종료(EOF) 0 */
static int input_tree(ArrTree *t)
{
    char buf[MAX_INPUT];

    for (;;) {
        printf("괄호 표기법으로 이진트리를 입력하세요 (예: A(B(E,F),C(,G))): ");
        fflush(stdout);

        ReadResult r = read_line(buf, sizeof buf);
        if (r == READ_EOF) {
            printf("\n");
            return 0;
        }
        if (r == READ_TOO_LONG) {
            printf("입력 오류: 입력이 너무 깁니다 (최대 %d자)\n", MAX_INPUT - 1);
            continue;
        }

        ArrTree tmp = {0};
        if (parse_tree(&tmp, buf)) {
            *t = tmp;
            return 1;
        }
        free(tmp.a);
        print_parse_error(buf);
    }
}

/* ---------------- 배열 접근 ---------------- */
static int has(const ArrTree *t, int i)
{
    return i >= 1 && i < t->cap && t->a[i] != '\0';
}

static int child_count(const ArrTree *t, int i)
{
    return has(t, 2 * i) + has(t, 2 * i + 1);
}

/* ---------------- [1] 트리 출력 ---------------- */
static void print_children(const ArrTree *t, int idx, const char *prefix)
{
    int kids[2], k = 0;
    if (has(t, 2 * idx))     kids[k++] = 2 * idx;
    if (has(t, 2 * idx + 1)) kids[k++] = 2 * idx + 1;

    for (int i = 0; i < k; i++) {
        char next[512];
        printf("%s+---%c\n", prefix, t->a[kids[i]]);
        snprintf(next, sizeof next, "%s%s", prefix, (i == k - 1) ? "    " : "|   ");
        print_children(t, kids[i], next);
    }
}

static void print_tree(const ArrTree *t)
{
    printf("%c\n", t->a[1]);
    print_children(t, 1, "");
}

/* ---------------- [2] 트리 정보 ---------------- */
static int tree_height(const ArrTree *t)     
{
    int h = 0;
    for (int x = t->max_idx; x > 0; x >>= 1) h++;
    return h;
}

static TreeInfo get_info(const ArrTree *t)
{
    TreeInfo info = {0};

    for (int i = 1; i <= t->max_idx; i++) {
        if (!has(t, i)) continue;
        int c = child_count(t, i);
        info.nodes++;
        if (c == 0) info.leaves++;
        if (c > info.degree) info.degree = c;
    }
    info.nonleaves = info.nodes - info.leaves;
    info.height = tree_height(t);
    return info;
}

static void print_info(const ArrTree *t)
{
    TreeInfo info = get_info(t);

    printf("1. 전체 노드의 수     : %d\n", info.nodes);
    printf("2. 단말 노드의 수     : %d\n", info.leaves);
    printf("3. 비단말 노드의 수   : %d\n", info.nonleaves);
    printf("4. 트리의 높이        : %d (루트의 레벨을 1로 계산)\n", info.height);
    printf("5. 트리의 차수        : %d\n", info.degree);
}

/* ---------------- [3] 형태 판별 ---------------- */
/* 완전 이진트리: 노드가 배열의 1 ~ n 번 칸을 빈틈없이 채움 */
static int is_complete(const ArrTree *t)
{
    return t->max_idx == t->n;
}

/* 포화 이진트리: 높이 h일 때 노드 수가 2^h - 1 */
static int is_full(const ArrTree *t)
{
    return t->n == (1 << tree_height(t)) - 1;
}

/* 편향 이진트리: 노드가 2개 이상이고 모든 노드가 왼쪽 자식만(또는 오른쪽 자식만) 가짐
 * 반환: 1 = 왼쪽 편향, 2 = 오른쪽 편향, 0 = 아님 */
static int skew_type(const ArrTree *t)
{
    int left_only = 1, right_only = 1;

    if (t->n < 2) return 0;
    for (int i = 1; i <= t->max_idx; i++) {
        if (!has(t, i)) continue;
        if (has(t, 2 * i + 1)) left_only = 0;   
        if (has(t, 2 * i))     right_only = 0; 
    }
    return left_only ? 1 : (right_only ? 2 : 0);
}

static void print_shape(const ArrTree *t)
{
    int skew = skew_type(t);

    printf("완전 이진트리 : %s\n", is_complete(t) ? "예" : "아니오");
    printf("포화 이진트리 : %s\n", is_full(t) ? "예" : "아니오");
    printf("편향 이진트리 : %s\n",
           skew == 1 ? "예 (왼쪽 편향)" :
           skew == 2 ? "예 (오른쪽 편향)" : "아니오");
}

/* ---------------- main ---------------- */
int main(void)
{
    ArrTree t = {0};

    if (!input_tree(&t)) {
        printf("입력이 없어 프로그램을 종료합니다.\n");
        return 0;
    }

    printf("\n[1] 이진트리 출력\n");
    print_tree(&t);

    printf("\n[2] 트리 정보\n");
    print_info(&t);

    printf("\n[3] 이진트리 형태 판별\n");
    print_shape(&t);

    free(t.a);
    return 0;
}
