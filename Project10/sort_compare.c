#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 10000
#define TRIALS 100
#define MAXKEY 1000000

typedef struct { int key1, key2, idx; } Data;   /\* idx: 원래 입력 순서 \*/

static long long cmpCnt, movCnt;
#define CNT\_RESET() (cmpCnt = movCnt = 0)

/\* 이식성 있는 난수 (rand()의 RAND\_MAX=32767 문제 회피) \*/
static unsigned long long rs = 88172645463325252ULL;
static unsigned int rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (unsigned int)(rs >> 11); }
static int rndKey(void) { return (int)(rnd() % (MAXKEY + 1)); }

static void gen(Data\* a, int n) { for (int i = 0; i < n; i++) { a[i].key1 = rndKey(); a[i].key2 = rndKey(); a[i].idx = i; } }

/\* ---------- 정렬 알고리즘 ---------- \*/
static void swapData(Data\* a, int i, int j) {
    Data t = a[i]; movCnt++;
    a[i] = a[j];   movCnt++;
    a[j] = t;      movCnt++;
}

static void selectionSort(Data\* a, int n) {
    for (int i = 0; i < n - 1; i++) {
        int m = i;
        for (int j = i + 1; j < n; j++) {
            cmpCnt++;
            if (a[j].key1 < a[m].key1) m = j;
        }
        if (m != i) swapData(a, i, m);
    }
}

static void bubbleSort(Data\* a, int n, int early) {
    for (int p = 0; p < n - 1; p++) {
        int swapped = 0;
        for (int j = 0; j < n - 1 - p; j++) {
            cmpCnt++;
            if (a[j].key1 > a[j + 1].key1) { swapData(a, j, j + 1); swapped = 1; }
        }
        if (early && !swapped) break;
    }
}

/\* useGE=0: Case A (>), useGE=1: Case B (>=) \*/
static void insertionSort(Data\* a, int n, int useGE) {
    for (int i = 1; i < n; i++) {
        Data cur = a[i]; movCnt++;               /\* 임시 변수 저장 \*/
        int j = i - 1;
        while (j >= 0) {
            cmpCnt++;
            int move = useGE ? (a[j].key1 >= cur.key1) : (a[j].key1 > cur.key1);
            if (!move) break;
            a[j + 1] = a[j]; movCnt++;           /\* 한 칸 이동 \*/
            j--;
        }
        a[j + 1] = cur; movCnt++;                /\* 최종 위치 대입 \*/
    }
}

/\* ---------- 검증 ---------- \*/
static int isSorted(const Data\* a, int n) { for (int i = 1; i < n; i++) if (a[i - 1].key1 > a[i].key1) return 0; return 1; }
/\* key1이 같은 인접 데이터의 idx가 증가해야 안정. 위반 쌍 개수 반환 \*/
static int stabViolations(const Data\* a, int n) {
    int v = 0;
    for (int i = 1; i < n; i++) if (a[i - 1].key1 == a[i].key1 && a[i - 1].idx > a[i].idx) v++;
    return v;
}

enum { SEL, BUB, BUBE, INS, NALG };
static const char\* names[NALG] = { "Selection Sort","Bubble Sort","Bubble Sort (Early Termination)","Insertion Sort" };

static void runAlg(int alg, Data\* a, int n) {
    switch (alg) {
    case SEL:  selectionSort(a, n); break;
    case BUB:  bubbleSort(a, n, 0); break;
    case BUBE: bubbleSort(a, n, 1); break;
    default:   insertionSort(a, n, 0); break;
    }
}

static int cmpSortedAsc(const void\* x, const void\* y) {            /\* 시험 입력 준비용 \*/
    const Data\* p = x, \* q = y;
    if (p->key1 != q->key1) return p->key1 < q->key1 ? -1 : 1;
    return p->idx - q->idx;
}
static int cmpSortedDesc(const void\* x, const void\* y) {
    const Data\* p = x, \* q = y;
    if (p->key1 != q->key1) return p->key1 > q->key1 ? -1 : 1;
    return p->idx - q->idx;
}

static void reportState(const char\* title, const Data\* src) {
    static Data work[N];
    printf("%s\\n\\n", title);
    for (int alg = 0; alg < NALG; alg++) {
        memcpy(work, src, sizeof(Data) \* N);          /\* 동일 입력 복사 (측정 제외) \*/
        CNT\_RESET();
        runAlg(alg, work, N);
        printf("%s\\nComparisons : %lld\\nMovements   : %lld\\nResult      : %s\\n\\n",
            names[alg], cmpCnt, movCnt, isSorted(work, N) ? "Sorted OK" : "NOT SORTED");
    }
    printf("------------------------------------\\n\\n");
}

/\* 안정성 검증 \*/
static void printSmall(const char\* label, const Data\* a, int n) {
    printf("  %-22s:", label);
    for (int i = 0; i < n; i++) printf(" (%d,%d)", a[i].key1, a[i].key2);
    printf("\\n");
}

static void stabilityTest(void) {
    static Data base[N], w[N];
    const char\* lab[4] = { "Selection Sort","Bubble Sort","Insertion Sort (>)","Insertion Sort (>=)" };
    int res[4];

    printf("Stability Verification\\n\\n");

    /\* 중복이 많은 대규모 데이터: key1 in [0,99], 10000개 \*/
    for (int i = 0; i < N; i++) { base[i].key1 = (int)(rnd() % 100); base[i].key2 = rndKey(); base[i].idx = i; }
    printf("[Large test] %d items, Key1 in 0..99 (many duplicates)\\n", N);
    for (int alg = 0; alg < 4; alg++) {
        memcpy(w, base, sizeof base);
        CNT\_RESET();
        if (alg == 0) selectionSort(w, N);
        else if (alg == 1) bubbleSort(w, N, 0);
        else insertionSort(w, N, alg == 3);
        int sorted = isSorted(w, N), v = stabViolations(w, N);
        res[alg] = sorted && v == 0;
        printf("  %-22s: sorted=%s, order violations=%d\\n", lab[alg], sorted ? "yes" : "NO", v);
    }
    printf("\\n");
    printf("Selection Sort       : %s\\n", res[0] ? "Stable" : "Unstable");
    printf("Bubble Sort          : %s\\n", res[1] ? "Stable" : "Unstable");
    printf("Insertion Sort (>)   : %s\\n", res[2] ? "Stable" : "Unstable");
    printf("Insertion Sort (>=)  : %s\\n", res[3] ? "Stable" : "Unstable");
}

int main(void) {
    static Data orig[N], work[N];
    static long long tc[NALG], tm[NALG];
    int allOk = 1;

    /\* 실행마다 다른 시드: 시간 + 주소(ASLR) + 클럭 \*/
    rs ^= (unsigned long long)time(NULL) \* 6364136223846793005ULL;
    rs ^= (unsigned long long)clock() \* 1442695040888963407ULL;
    rs ^= (unsigned long long)(size\_t) & allOk;
    if (rs == 0) rs = 88172645463325252ULL;
    for (int i = 0; i < 20; i++) rnd();               /\* 시드 섞기 \*/
    printf("Seed state : %llu\\n", rs);

    printf("Data Count : %d\\n\\nRandom Data\\nNumber of Trials : %d\\n\\n", N, TRIALS);
    for (int t = 0; t < TRIALS; t++) {
        gen(orig, N);                                  /\* 모든 알고리즘이 같은 난수 데이터 사용 \*/
        for (int alg = 0; alg < NALG; alg++) {
            memcpy(work, orig, sizeof orig);
            CNT\_RESET();
            runAlg(alg, work, N);
            tc[alg] += cmpCnt; tm[alg] += movCnt;
            if (!isSorted(work, N)) { allOk = 0; printf("!! %s failed in trial %d\\n", names[alg], t + 1); }
        }
    }
    for (int alg = 0; alg < NALG; alg++)
        printf("%s\\nTotal Comparisons   : %lld\\nAverage Comparisons : %.2f\\nAverage Movements   : %.2f\\n\\n",
            names[alg], tc[alg], (double)tc[alg] / TRIALS, (double)tm[alg] / TRIALS);
    printf("All %d trials sorted correctly: %s\\n\\n------------------------------------\\n\\n", TRIALS, allOk ? "YES" : "NO");

    /\* 동일한 원본으로 세 가지 입력 상태 생성 \*/
    static Data base[N], sorted[N], rev[N], part[N];
    gen(base, N);
    memcpy(sorted, base, sizeof base); qsort(sorted, N, sizeof(Data), cmpSortedAsc);
    memcpy(rev, base, sizeof base);    qsort(rev, N, sizeof(Data), cmpSortedDesc);
    /\* 일부 정렬: 정렬 데이터에서 임의의 두 위치를 N/100(=100)쌍 교환 \*/
    memcpy(part, sorted, sizeof sorted);
    for (int k = 0; k < N / 100; k++) {
        int i = rnd() % N, j = rnd() % N;
        Data t = part[i]; part[i] = part[j]; part[j] = t;
    }

    reportState("Sorted Data", sorted);
    reportState("Reverse-Sorted Data", rev);
    reportState("Partially Sorted Data\\n(sorted data + 100 random pair swaps)", part);
    stabilityTest();
    return 0;
}
