#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT_SIZE 10000

char input[MAX_INPUT_SIZE];

/* ===================== 공통 유틸 ===================== */

/*malloc 실패를 공통으로 처리하는 안전한 할당 함수.*/

void* safeMalloc(size_t size) {
    void* p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "오류: 메모리 할당에 실패했습니다.\n");
        exit(1);
    }
    return p;
}

/* 공백 문자 모두 제거 */
void removeSpaces(char* str) {
    char* src = str;
    char* dst = str;
    while (*src) {
        if (!isspace((unsigned char)*src)) {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

/* ===================== 입력 유효성 검사 ===================== */

/* 재귀적으로 한 노드(및 그 하위 구조)가 문법적으로 올바른지 검사하고,
 * 올바르면 pos를 그 노드가 끝나는 위치까지 이동시킴.
 * 반환값: 1이면 유효, 0이면 오류. errMsg에 오류 원인을 기록함.
 * (errMsg는 snprintf로 기록하여 버퍼 오버플로를 방지함.) */
int validateNode(const char* str, int* pos, char* errMsg, size_t errMsgSize) {
    if (!isupper((unsigned char)str[*pos])) {
        snprintf(errMsg, errMsgSize, "위치 %d: 노드는 영문 대문자여야 합니다 ('%c')", *pos,
            str[*pos] == '\0' ? '?' : str[*pos]);
        return 0;
    }
    (*pos)++;

    if (str[*pos] == '(') {
        (*pos)++; 

        if (str[*pos] == ')') {
            snprintf(errMsg, errMsgSize, "위치 %d: 빈 자식 목록 \"()\" 은 허용되지 않습니다", *pos);
            return 0;
        }
        if (str[*pos] == ',') {
            snprintf(errMsg, errMsgSize, "위치 %d: 자식 목록이 콤마로 시작할 수 없습니다", *pos);
            return 0;
        }

        if (!validateNode(str, pos, errMsg, errMsgSize)) {
            return 0;
        }

        while (str[*pos] == ',') {
            (*pos)++; 
            if (str[*pos] == ',' || str[*pos] == ')') {
                snprintf(errMsg, errMsgSize, "위치 %d: 콤마 뒤에는 노드가 와야 합니다", *pos);
                return 0;
            }
            if (!validateNode(str, pos, errMsg, errMsgSize)) {
                return 0;
            }
        }

        if (str[*pos] != ')') {
            snprintf(errMsg, errMsgSize, "위치 %d: 괄호가 올바르게 닫히지 않았습니다", *pos);
            return 0;
        }
        (*pos)++; 
    }

    return 1;
}

/* 문자열 전체가 올바른 괄호 트리 표기인지 검사. */
int isValidTree(const char* str, char* errMsg, size_t errMsgSize) {
    if (str[0] == '\0') {
        snprintf(errMsg, errMsgSize, "입력이 비어 있습니다");
        return 0;
    }

    int pos = 0;
    if (!validateNode(str, &pos, errMsg, errMsgSize)) {
        return 0;
    }

    if (str[pos] != '\0') {
        snprintf(errMsg, errMsgSize, "위치 %d: 트리 뒤에 불필요한 문자('%c')가 있습니다", pos, str[pos]);
        return 0;
    }

    return 1;
}

/* 노드 문자가 중복 사용되었는지 검사. 중복은 오류 취급. */
int hasDuplicateNode(const char* str, char* dupOut) {
    int seen[26] = { 0 };
    for (int i = 0; str[i] != '\0'; i++) {
        if (isupper((unsigned char)str[i])) {
            int idx = str[i] - 'A';
            if (seen[idx]) {
                *dupOut = str[i];
                return 1;
            }
            seen[idx] = 1;
        }
    }
    return 0;
}

/* ===================== (1) 전체 노드 수 ===================== */
/* 문자열에서 대문자 개수를 세아림. */
int countNodes(const char* str) {
    int count = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (isupper((unsigned char)str[i])) {
            count++;
        }
    }
    return count;
}

/* ===================== (2) 단말 노드 수 ===================== */
/* 대문자 바로 다음 문자가 '(' 가 아니면 단말 노드 */
int countLeaves(const char* str) {
    int count = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (isupper((unsigned char)str[i]) && str[i + 1] != '(') {
            count++;
        }
    }
    return count;
}

/* ===================== (3) 비단말 노드 수 ===================== */
/* 대문자 바로 다음 문자가 '(' 이면 비단말 노드 */
int countNonLeaves(const char* str) {
    int count = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (isupper((unsigned char)str[i]) && str[i + 1] == '(') {
            count++;
        }
    }
    return count;
}

/* ===================== (4) 트리의 높이(정점 기준) ===================== */
/* 괄호 중첩 깊이의 최댓값 + 1 */
int getHeight(const char* str) {
    int depth = 0;
    int maxDepth = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '(') {
            depth++;
            if (depth > maxDepth) maxDepth = depth;
        }
        else if (str[i] == ')') {
            depth--;
        }
    }

    return maxDepth + 1; 
}

/* ===================== (5) 트리의 차수 ===================== */
/* 모든 노드의 차수(자식 수) 중 최댓값.
 * (입력은 이미 isValidTree()로 검증되지만, 방어적으로
 * 스택 하한(top < 0)도 함께 검사함.) */
int getMaxDegree(const char* str) {
    int len = (int)strlen(str);
    int* counterStack = (int*)safeMalloc(sizeof(int) * (size_t)(len + 1));
    int top = -1;
    int maxDegree = 0;

    for (int i = 0; i < len; i++) {
        char c = str[i];
        if (c == '(') {
            counterStack[++top] = 1;
        }
        else if (c == ',') {
            if (top >= 0) counterStack[top]++;
        }
        else if (c == ')') {
            if (top >= 0) {
                if (counterStack[top] > maxDegree) {
                    maxDegree = counterStack[top];
                }
                top--;
            }
        }
    }

    free(counterStack);
    return maxDegree;
}

/* ===================== (6) 노드 C의 부모 노드 ===================== */
/* "노드 스택": 현재까지 열려 있는 조상 노드 문자를 담는 스택. 
 * 반환값: 1이면 target을 찾음(부모 문자를 parentOut에 저장, 없으면 '\0'),
 *         0이면 문자열에 target 자체가 없음. */
int findParent(const char* str, char target, char* parentOut) {
    int len = (int)strlen(str);
    char* nodeStack = (char*)safeMalloc(sizeof(char) * (size_t)(len + 1));
    int top = -1;
    int found = 0;
    *parentOut = '\0';

    for (int i = 0; i < len; i++) {
        char c = str[i];
        if (isupper((unsigned char)c)) {
            if (c == target) {
                *parentOut = (top >= 0) ? nodeStack[top] : '\0';
                found = 1;
            }
            if (str[i + 1] == '(') {
                nodeStack[++top] = c;
            }
        }
        else if (c == ')') {
            if (top >= 0) top--;
        }
    }

    free(nodeStack);
    return found;
}

/* ===================== (7) 노드 C의 자식 노드 ===================== */
/* "카운터 스택": target 노드의 '(' 부터 시작해서, 괄호 깊이를 추적하는
 *               카운터를 스택에 쌓음. 
 * 반환값: 자식 수 (0이면 단말 노드, -1이면 target이 문자열에 없음). */
int findChildren(const char* str, char target, char result[], int maxResult) {
    int len = (int)strlen(str);

    int i = 0;
    while (str[i] != '\0' && str[i] != target) i++;
    if (str[i] == '\0') return -1; /* target이 없음 */

    if (str[i + 1] != '(') return 0; /* 단말 노드: 자식 없음 */

    int* counterStack = (int*)safeMalloc(sizeof(int) * (size_t)(len + 1));
    int top = 0;
    counterStack[0] = 0; /* target 자신의 자식 목록 깊이를 0으로 시작 */
    int count = 0;

    i += 2; /* 자식 목록 시작 위치로 이동 */

    while (top >= 0 && str[i] != '\0') {
        char c = str[i];
        if (isupper((unsigned char)c)) {
            if (top == 0) {
                if (count < maxResult) {
                    result[count++] = c; /* 직계 자식(깊이 0)만 기록 */
                }
                /* 버퍼 오버플로 방지 */
            }
        }
        else if (c == '(') {
            top++;
            counterStack[top] = top; /* 더 깊은 자식 목록으로 진입 */
        }
        else if (c == ')') {
            top--; /* 자식 목록 종료 */
        }
        i++;
    }

    free(counterStack);
    return count;
}

/* ===================== (8) 트리를 왼쪽으로 눕힌 형태로 출력 ===================== */
/* 어떤 노드가 "형제 목록에서 마지막이 아닌지"를
 * 그 노드의 부분 문자열만 건너뛰어 보고 판단함. */
int hasNextSibling(const char* str, int pos) {
    pos++;

    if (str[pos] == '(') {
        int depth = 1;
        pos++; 
        while (depth > 0 && str[pos] != '\0') {
            if (str[pos] == '(') depth++;
            else if (str[pos] == ')') depth--;
            pos++;
        }
    }

    return (str[pos] == ',');
}

/* hasMore[d]: 깊이 d에 있는(현재 경로상의) 조상이 
 *             형제를 더 가지고 있는지를 기록해 두는 배열.
 * maxDepth: hasMore 배열의 크기, 
 *           방어적으로 depth가 이를 넘어서면 그 지점에서 멈춤.
 */
void printTreeSideways(const char* str, int* pos, int depth, int hasMore[], int maxDepth) {
    if (!isupper((unsigned char)str[*pos])) return;

    if (depth >= maxDepth) {
        fprintf(stderr, "오류: 트리 깊이가 너무 깊어 출력을 중단합니다.\n");
        return;
    }

    int nodeStart = *pos;
    char nodeChar = str[*pos];
    (*pos)++;

    int hasChildren = (str[*pos] == '(');

    if (depth == 0) {
        printf("%c\n", nodeChar);
    }
    else {
        for (int d = 1; d < depth; d++) {
            printf(hasMore[d] ? "|   " : "    ");
        }
        printf("+---%c\n", nodeChar);
    }

    hasMore[depth] = hasNextSibling(str, nodeStart);

    if (hasChildren) {
        (*pos)++; 
        printTreeSideways(str, pos, depth + 1, hasMore, maxDepth);
        while (str[*pos] == ',') {
            (*pos)++; 
            printTreeSideways(str, pos, depth + 1, hasMore, maxDepth);
        }
        if (str[*pos] == ')') {
            (*pos)++; 
        }
    }
}

/* ===================== main ===================== */
int main() {
    printf("괄호 형태로 트리를 입력하세요 (예: A(B(C,D),E)): ");

    /* 1) 입력 자체를 읽지 못한 경우(EOF, 스트림 오류 등) */
    if (fgets(input, sizeof(input), stdin) == NULL) {
        fprintf(stderr, "오류: 입력을 읽지 못했습니다.\n");
        return 1;
    }

    int len = (int)strlen(input);

    /* 2) 입력이 버퍼 크기를 초과해 잘렸는지 확인 */
    if (len > 0 && input[len - 1] != '\n' && !feof(stdin)) {
        fprintf(stderr, "오류: 입력이 너무 깁니다 (최대 %d자). 입력을 줄여서 다시 시도하세요.\n",
            MAX_INPUT_SIZE - 1);
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { /* 남은 입력 비우기 */ }
        return 1;
    }

    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
    }

    removeSpaces(input);

    /* 3) 공백을 제거하고 나니 아무것도 남지 않은 경우 */
    if (strlen(input) == 0) {
        fprintf(stderr, "오류: 입력이 비어 있습니다.\n");
        return 1;
    }

    /* 4) 괄호 트리 문법 검증 (짝이 맞지 않는 괄호, 빈/연속 콤마,
     *    알 수 없는 문자, 트리 뒤 불필요한 문자 등) */
    char errMsg[256];
    if (!isValidTree(input, errMsg, sizeof(errMsg))) {
        fprintf(stderr, "오류: 잘못된 트리 표현입니다. (%s)\n", errMsg);
        return 1;
    }

    /* 5) 중복된 노드 문자 검사 */
    char dupChar;
    if (hasDuplicateNode(input, &dupChar)) {
        fprintf(stderr, "오류: 노드 문자 '%c' 가 중복 사용되었습니다. 모든 노드는 서로 다른 문자를 가져야 합니다.\n",
            dupChar);
        return 1;
    }

    int totalNodes = countNodes(input);
    int leafNodes = countLeaves(input);
    int nonLeafNodes = countNonLeaves(input);
    int height = getHeight(input);
    int degree = getMaxDegree(input);

    printf("\n=== 트리 특성 ===\n");
    printf("(1) 전체 노드 수   : %d\n", totalNodes);
    printf("(2) 단말 노드 수   : %d\n", leafNodes);
    printf("(3) 비단말 노드 수 : %d\n", nonLeafNodes);
    printf("(4) 트리의 높이    : %d\n", height);
    printf("(5) 트리의 차수    : %d\n", degree);

    /* 부모/자식 조회 C로 설정 */
    char target = 'C';

    char parent;
    int hasNode = findParent(input, target, &parent);
    printf("(6) 노드 %c의 부모 노드 : ", target);
    if (!hasNode) {
        printf("트리에 해당 노드가 없습니다.\n");
    }
    else if (parent == '\0') {
        printf("없음 (루트 노드)\n");
    }
    else {
        printf("%c\n", parent);
    }

    /* 자식 목록을 담을 배열은 이론상 최대 노드 수만큼만 필요함.
     * 동적으로 크기를 맞춰 배열 오버플로 방지. */
    int childBufSize = totalNodes + 1;
    char* children = (char*)safeMalloc(sizeof(char) * (size_t)childBufSize);

    int childCount = findChildren(input, target, children, childBufSize);
    printf("(7) 노드 %c의 자식 노드 : ", target);
    if (childCount < 0) {
        printf("트리에 해당 노드가 없습니다.\n");
    }
    else if (childCount == 0) {
        printf("없음 (단말 노드)\n");
    }
    else {
        for (int i = 0; i < childCount; i++) {
            printf("%c ", children[i]);
        }
        printf("\n");
    }
    free(children);

    printf("\n(8) 트리를 왼쪽으로 눕힌 형태로 출력\n");
    int pos = 0;
    /* hasMore 배열: 실제 트리 높이에 맞춰 동적으로 할당. */
    int hasMoreSize = height + 2;
    int* hasMore = (int*)safeMalloc(sizeof(int) * (size_t)hasMoreSize);
    for (int i = 0; i < hasMoreSize; i++) hasMore[i] = 0;

    printTreeSideways(input, &pos, 0, hasMore, hasMoreSize);
    free(hasMore);

    return 0;
}
