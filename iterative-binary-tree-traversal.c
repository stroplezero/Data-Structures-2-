#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct TreeNode {
    char data;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

static TreeNode* makeNode(char data) {
    TreeNode *n = (TreeNode*)malloc(sizeof(TreeNode));
    if (!n) { fprintf(stderr, "메모리 할당 실패\n"); exit(1); }
    n->data = data;
    n->left = n->right = NULL;
    return n;
}

static char* dupStr(const char *s) {
    size_t len = strlen(s) + 1;
    char *r = (char*)malloc(len);
    memcpy(r, s, len);
    return r;
}

/* ================= 범용 노드 포인터 스택 ================= */
typedef struct {
    TreeNode **data;
    int top;
    int capacity;
} NodeStack;

static NodeStack* nsCreate(int capacity) {
    NodeStack *s = (NodeStack*)malloc(sizeof(NodeStack));
    s->data = (TreeNode**)malloc(sizeof(TreeNode*) * capacity);
    s->top = -1;
    s->capacity = capacity;
    return s;
}
static int nsIsEmpty(NodeStack *s) { return s->top == -1; }
static void nsPush(NodeStack *s, TreeNode *node) {
    if (s->top + 1 >= s->capacity) {
        s->capacity *= 2;
        s->data = (TreeNode**)realloc(s->data, sizeof(TreeNode*) * s->capacity);
    }
    s->data[++(s->top)] = node;
}
static TreeNode* nsPop(NodeStack *s) { return s->data[(s->top)--]; }
static void nsDestroy(NodeStack *s) { free(s->data); free(s); }

/* ================= 괄호 표기법 파서 ================= */

typedef struct {
    TreeNode *parent;
    int slot;   /* 0 = 왼쪽 자식 대기, 1 = 오른쪽 자식 대기 */
} ParseFrame;

typedef struct {
    ParseFrame *data;
    int top;
    int capacity;
} ParseStack;

static ParseStack* psCreate(int capacity) {
    ParseStack *s = (ParseStack*)malloc(sizeof(ParseStack));
    s->data = (ParseFrame*)malloc(sizeof(ParseFrame) * capacity);
    s->top = -1;
    s->capacity = capacity;
    return s;
}
static int psIsEmpty(ParseStack *s) { return s->top == -1; }
static void psPush(ParseStack *s, ParseFrame f) {
    if (s->top + 1 >= s->capacity) {
        s->capacity *= 2;
        s->data = (ParseFrame*)realloc(s->data, sizeof(ParseFrame) * s->capacity);
    }
    s->data[++(s->top)] = f;
}
static ParseFrame* psTop(ParseStack *s) { return &s->data[s->top]; }
static void psPop(ParseStack *s) { (s->top)--; }
static void psDestroy(ParseStack *s) { free(s->data); free(s); }

static void freeTree(TreeNode *root) {   
    if (!root) return;
    NodeStack *s = nsCreate(16);
    nsPush(s, root);
    while (!nsIsEmpty(s)) {
        TreeNode *node = nsPop(s);
        if (node->left)  nsPush(s, node->left);
        if (node->right) nsPush(s, node->right);
        free(node);
    }
    nsDestroy(s);
}

static TreeNode* buildTreeFromString(const char *str, int *ok, char *errmsgOut, size_t errmsgLen) {
    int len = (int)strlen(str);
    int pos = 0;
    TreeNode *root = NULL;
    TreeNode *lastNode = NULL;     
    TreeNode *unattachedNode = NULL; /* 오류 발생 시 아직 트리에 연결되지 않은 노드를 정리하기 위한 변수 */
    int errorFlag = 0;
    ParseStack *stack = psCreate(16);

    while (pos < len) {
        char c = str[pos];

        if (isspace((unsigned char)c)) { pos++; continue; }

        if (isupper((unsigned char)c)) {
            TreeNode *node = makeNode(c);
            if (psIsEmpty(stack)) {
                if (root != NULL) {
                    errorFlag = 1;
                    snprintf(errmsgOut, errmsgLen,
                             "노드 '%c' 앞에 연산자가 없습니다 (위치 %d).", c, pos);
                    unattachedNode = node;
                    break;
                }
                root = node;
            } else {
                ParseFrame *top = psTop(stack);
                if (top->slot == 0) {
                    if (top->parent->left != NULL) {
                        errorFlag = 1;
                        snprintf(errmsgOut, errmsgLen,
                                 "노드 '%c' 위치가 올바르지 않습니다 (위치 %d).", c, pos);
                        unattachedNode = node;
                        break;
                    }
                    top->parent->left = node;
                } else {
                    if (top->parent->right != NULL) {
                        errorFlag = 1;
                        snprintf(errmsgOut, errmsgLen,
                                 "노드 '%c' 위치가 올바르지 않습니다 (위치 %d).", c, pos);
                        unattachedNode = node;
                        break;
                    }
                    top->parent->right = node;
                }
            }
            lastNode = node;
            pos++;
        } else if (c == '(') {
            if (lastNode == NULL) {
                errorFlag = 1;
                snprintf(errmsgOut, errmsgLen, "'(' 앞에 노드가 없습니다 (위치 %d).", pos);
                break;
            }
            ParseFrame nf; nf.parent = lastNode; nf.slot = 0;
            psPush(stack, nf);
            lastNode = NULL;
            pos++;
        } else if (c == ',') {
            if (psIsEmpty(stack)) {
                errorFlag = 1;
                snprintf(errmsgOut, errmsgLen, "','가 괄호 밖에 있습니다 (위치 %d).", pos);
                break;
            }
            ParseFrame *top = psTop(stack);
            if (top->slot != 0) {
                errorFlag = 1;
                if (top->parent->right != NULL) {
                    int k = pos + 1;   /* 쉼표 바로 뒤(공백 제외)가 ')' 이면 자식이 아니라 쉼표만 남은 경우 */
                    while (k < len && isspace((unsigned char)str[k])) k++;
                    if (k < len && str[k] == ')')
                        snprintf(errmsgOut, errmsgLen, "불필요한 쉼표가 있습니다 (위치 %d).", pos);
                    else
                        snprintf(errmsgOut, errmsgLen,
                                 "자식이 3개 이상입니다. 이진트리는 자식을 최대 2개까지만 가질 수 있습니다 (위치 %d).", pos);
                } else
                    snprintf(errmsgOut, errmsgLen, "','가 중복되었습니다 (위치 %d).", pos);
                break;
            }
            top->slot = 1;
            lastNode = NULL;
            pos++;
        } else if (c == ')') {
            if (psIsEmpty(stack)) {
                errorFlag = 1;
                snprintf(errmsgOut, errmsgLen, "짝이 맞지 않는 ')' 입니다 (위치 %d).", pos);
                break;
            }
            ParseFrame *top = psTop(stack);
            /* A(B) 처럼 왼쪽 자식만 있는 경우는 허용, 괄호 안에 자식이 하나도 없으면 오류 */
            if (top->parent->left == NULL && top->parent->right == NULL) {
                errorFlag = 1;
                snprintf(errmsgOut, errmsgLen, "괄호 안에 자식 노드가 없습니다 (위치 %d).", pos);
                break;
            }
            /* A(B,) 처럼 쉼표 뒤에 오른쪽 자식이 없으면 오류 (왼쪽 자식만 있으면 A(B) 형태로 작성) */
            if (top->slot == 1 && top->parent->right == NULL) {
                errorFlag = 1;
                snprintf(errmsgOut, errmsgLen,
                         "쉼표 뒤에 오른쪽 자식이 없습니다 (위치 %d). 왼쪽 자식만 있으면 A(B) 형태로 씁니다.", pos);
                break;
            }
            psPop(stack);
            lastNode = NULL;
            pos++;
        } else {
            errorFlag = 1;
            if ((unsigned char)c >= 0x20 && (unsigned char)c < 0x7F)
                snprintf(errmsgOut, errmsgLen, "잘못된 문자 '%c' (위치 %d).", c, pos);
            else   /* 한글 등 여러 바이트 문자나 제어 문자는 그대로 출력하면 깨지므로 위치만 표시 */
                snprintf(errmsgOut, errmsgLen, "허용되지 않는 문자입니다 (위치 %d). 영문 대문자, '(', ')', ',' 만 사용할 수 있습니다.", pos);
            break;
        }
    }

    if (!errorFlag) {
        if (!psIsEmpty(stack)) {
            errorFlag = 1;
            snprintf(errmsgOut, errmsgLen, "닫히지 않은 '(' 가 있습니다.");
        } else if (root == NULL) {
            errorFlag = 1;
            snprintf(errmsgOut, errmsgLen, "입력이 비어 있습니다.");
        }
    }

    psDestroy(stack);

    if (errorFlag) {
        if (unattachedNode) free(unattachedNode);
        freeTree(root);
        *ok = 0;
        return NULL;
    }

    *ok = 1;
    return root;
}

/* ================= 반복적(iterative) 순회  ================= */

static void preorder(TreeNode *tree) {
    printf("Preorder  :");
    if (tree) {
        NodeStack *s = nsCreate(16);
        nsPush(s, tree);
        while (!nsIsEmpty(s)) {
            TreeNode *node = nsPop(s);
            printf(" %c", node->data);        
            if (node->right) nsPush(s, node->right);
            if (node->left)  nsPush(s, node->left);
        }
        nsDestroy(s);
    }
    printf("\n");
}

static void inorder(TreeNode *tree) {
    printf("Inorder   :");
    NodeStack *s = nsCreate(16);
    TreeNode *cur = tree;
    while (cur != NULL || !nsIsEmpty(s)) {
        while (cur != NULL) {                   
            nsPush(s, cur);
            cur = cur->left;
        }
        cur = nsPop(s);
        printf(" %c", cur->data);               
        cur = cur->right;
    }
    nsDestroy(s);
    printf("\n");
}

static void postorder(TreeNode *tree) {
    printf("Postorder :");
    if (tree) {
        NodeStack *s1 = nsCreate(16);
        NodeStack *s2 = nsCreate(16);
        nsPush(s1, tree);
        while (!nsIsEmpty(s1)) {                
            TreeNode *node = nsPop(s1);
            nsPush(s2, node);
            if (node->left)  nsPush(s1, node->left);
            if (node->right) nsPush(s1, node->right);
        }
        while (!nsIsEmpty(s2)) {              
            TreeNode *node = nsPop(s2);
            printf(" %c", node->data);
        }
        nsDestroy(s1);
        nsDestroy(s2);
    }
    printf("\n");
}

static int countNodes(TreeNode *root) {
    if (!root) return 0;
    int count = 0;
    NodeStack *s = nsCreate(16);
    nsPush(s, root);
    while (!nsIsEmpty(s)) {
        TreeNode *node = nsPop(s);
        count++;
        if (node->left)  nsPush(s, node->left);
        if (node->right) nsPush(s, node->right);
    }
    nsDestroy(s);
    return count;
}

/* ================= 트리 구조 출력 =================*/
typedef struct {
    TreeNode *node;
    char *prefix;  
    int isLast;    
    int isRoot;     
} PrintFrame;

typedef struct {
    PrintFrame *data;
    int top;
    int capacity;
} PrintStack;

static PrintStack* pfCreate(int capacity) {
    PrintStack *s = (PrintStack*)malloc(sizeof(PrintStack));
    s->data = (PrintFrame*)malloc(sizeof(PrintFrame) * capacity);
    s->top = -1;
    s->capacity = capacity;
    return s;
}
static int pfIsEmpty(PrintStack *s) { return s->top == -1; }
static void pfPush(PrintStack *s, PrintFrame f) {
    if (s->top + 1 >= s->capacity) {
        s->capacity *= 2;
        s->data = (PrintFrame*)realloc(s->data, sizeof(PrintFrame) * s->capacity);
    }
    s->data[++(s->top)] = f;
}
static PrintFrame pfPop(PrintStack *s) { return s->data[(s->top)--]; }
static void pfDestroy(PrintStack *s) { free(s->data); free(s); }

static void printStructure(TreeNode *root) {
    if (!root) return;
    PrintStack *s = pfCreate(16);
    PrintFrame rootFrame; rootFrame.node = root; rootFrame.prefix = dupStr(""); rootFrame.isLast = 0; rootFrame.isRoot = 1;
    pfPush(s, rootFrame);

    while (!pfIsEmpty(s)) {
        PrintFrame f = pfPop(s);
        char *childBasePrefix;

        if (f.isRoot) {
            printf("%c\n", f.node->data);
            childBasePrefix = dupStr("");
        } else {
            printf("%s+---%c\n", f.prefix, f.node->data);
            size_t plen = strlen(f.prefix);
            childBasePrefix = (char*)malloc(plen + 5);
            snprintf(childBasePrefix, plen + 5, "%s%s", f.prefix, f.isLast ? "    " : "|   ");
        }
        free(f.prefix);

        int hasLeft = (f.node->left != NULL);
        int hasRight = (f.node->right != NULL);

       
        if (hasLeft && hasRight) {
            PrintFrame rf; rf.node = f.node->right; rf.prefix = dupStr(childBasePrefix); rf.isLast = 1; rf.isRoot = 0;
            pfPush(s, rf);
            PrintFrame lf; lf.node = f.node->left; lf.prefix = dupStr(childBasePrefix); lf.isLast = 0; lf.isRoot = 0;
            pfPush(s, lf);
        } else if (hasLeft) {
            PrintFrame lf; lf.node = f.node->left; lf.prefix = dupStr(childBasePrefix); lf.isLast = 1; lf.isRoot = 0;
            pfPush(s, lf);
        } else if (hasRight) {
            PrintFrame rf; rf.node = f.node->right; rf.prefix = dupStr(childBasePrefix); rf.isLast = 1; rf.isRoot = 0;
            pfPush(s, rf);
        }
        free(childBasePrefix);
    }
    pfDestroy(s);
}

/* ================= 구조 + 순회 결과 출력 ================= */
static void runTraversalsAndPrint(const char *label, TreeNode *root) {
    printf("\n=== %s ===\n", label);
    printf("[트리 구조]\n");
    printStructure(root);
    printf("\n[순회 결과]\n");
    preorder(root);
    inorder(root);
    postorder(root);
}

/* ================= 내부 테스트 기능 ================= */
static void runSelfTest(void) {
    printf("\n##################################################\n");
    printf("# 내부 테스트 (노드 10개 이상, 좌/우 서브트리 모두 포함)\n");
    printf("##################################################\n");

    const char *testExpr = "A(B(D(H,I),E),C(F(,J),G(,K)))";
    printf("입력 괄호 표현식: %s\n", testExpr);

    int ok;
    char errmsg[256];
    TreeNode *root = buildTreeFromString(testExpr, &ok, errmsg, sizeof(errmsg));
    if (!ok) {
        printf("[오류] %s\n", errmsg);
        return;
    }

    printf("노드 개수: %d\n", countNodes(root));
    runTraversalsAndPrint("자체 테스트 트리", root);

    freeTree(root);
}

/* ================= main ================= */
int main(void) {
    char line[1024];

    printf("=== 괄호 표기법 이진트리 입력 및 순회 프로그램 (반복적 방식) ===\n");
    printf("입력 형식 예시: A(B(,D),C)  (자식이 없으면 비워둠)\n");
    printf("괄호 표기법으로 이진트리를 입력하세요: ");
    fflush(stdout);

    if (fgets(line, sizeof(line), stdin) == NULL) {
        printf("입력을 읽을 수 없습니다.\n");
    } else {
        size_t len = strlen(line);
        int tooLong = 0;
        if (len == sizeof(line) - 1 && line[len-1] != '\n' && line[len-1] != '\r') {
            int ch = getc(stdin);              /* 버퍼가 가득 찼는데 입력이 더 남아 있는지 확인 */
            if (ch != EOF && ch != '\n' && ch != '\r') tooLong = 1;
        }
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';

        int ok;
        char errmsg[256];
        TreeNode *root = NULL;
        if (tooLong) {
            ok = 0;
            snprintf(errmsg, sizeof(errmsg), "입력이 너무 깁니다 (최대 %d자).", (int)sizeof(line) - 1);
        } else {
            root = buildTreeFromString(line, &ok, errmsg, sizeof(errmsg));
        }

        if (!ok) {
            printf("\n[오류] 잘못된 괄호 표현식입니다: %s\n", errmsg);
        } else {
            runTraversalsAndPrint("입력한 트리", root);
            freeTree(root);
        }
    }

    runSelfTest();

    return 0;
}
