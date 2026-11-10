#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define REPEAT 100   /* 실행시간은 REPEAT회 반복 측정의 평균 (복사 시간은 제외) */

typedef enum { FIRST, RANDOM, MEDIAN3 } PivotType;

static long long cmp_count;     /* 숫자 비교 횟수 */
static int cur_depth, max_depth; /* 재귀 깊이 (메모리 계산용) */

/* 재귀 호출 1회(스택 프레임)의 추정 크기: 인자·지역변수 + 반환 주소. */
#define QUICK_FRAME  (sizeof(int *) + 4 * sizeof(int) + sizeof(PivotType) + sizeof(void *))
#define MERGE_FRAME  (sizeof(int *) + 4 * sizeof(int) + sizeof(void *))

static void swap(int *a, int *b) { int t = *a; *a = *b; *b = t; }
static int rand_int(void) { return ((rand() & 0x7FFF) << 15) | (rand() & 0x7FFF); }

/* ---------------- 타이머 ---------------- */
static double now_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
#endif
}

/* ---------------- 퀵 정렬 ---------------- */
static void choose_pivot(int a[], int lo, int hi, PivotType type) {
    int p = lo;
    if (type == RANDOM) {
        p = lo + rand_int() % (hi - lo + 1);          /* 비교 아님 */
    } else if (type == MEDIAN3) {
        int mid = lo + (hi - lo) / 2;
        int x = a[lo], y = a[mid], z = a[hi];
        cmp_count++;
        if (x < y) {
            cmp_count++;
            if (y < z) p = mid;
            else { cmp_count++; p = (x < z) ? hi : lo; }
        } else {
            cmp_count++;
            if (x < z) p = lo;
            else { cmp_count++; p = (y < z) ? hi : mid; }
        }
    }
    if (p != lo) swap(&a[lo], &a[p]);
}

static int partition(int a[], int lo, int hi, PivotType type) {
    choose_pivot(a, lo, hi, type);
    int pivot = a[lo];
    int i = lo, j = hi + 1;
    while (1) {
        while (++i <= hi) {
            cmp_count++;
            if (!(a[i] < pivot)) break;
        }
        do { j--; cmp_count++; } while (a[j] > pivot);
        if (i >= j) break;
        swap(&a[i], &a[j]);
    }
    swap(&a[lo], &a[j]);
    return j;
}

static void quick_sort(int a[], int lo, int hi, PivotType type) {
    if (lo >= hi) return;
    if (++cur_depth > max_depth) max_depth = cur_depth;
    int p = partition(a, lo, hi, type);
    quick_sort(a, lo, p - 1, type);
    quick_sort(a, p + 1, hi, type);
    cur_depth--;
}

/* ---------------- 병합 정렬 ---------------- */
static void merge(int a[], int tmp[], int lo, int mid, int hi) {
    int i = lo, j = mid + 1, k = lo;
    while (i <= mid && j <= hi) {
        cmp_count++;                       /* 데이터 값끼리의 비교 */
        if (a[i] <= a[j]) tmp[k++] = a[i++];
        else              tmp[k++] = a[j++];
    }
    while (i <= mid) tmp[k++] = a[i++];
    while (j <= hi)  tmp[k++] = a[j++];
    for (k = lo; k <= hi; k++) a[k] = tmp[k];
}

static void merge_sort(int a[], int tmp[], int lo, int hi) {
    if (lo >= hi) return;
    if (++cur_depth > max_depth) max_depth = cur_depth;
    int mid = lo + (hi - lo) / 2;
    merge_sort(a, tmp, lo, mid);
    merge_sort(a, tmp, mid + 1, hi);
    merge(a, tmp, lo, mid, hi);
    cur_depth--;
}

static int is_sorted(const int a[], int n) {
    for (int i = 1; i < n; i++) if (a[i - 1] > a[i]) return 0;
    return 1;
}

/* alg: 0=Merge, 1=First, 2=Random, 3=Median3
   결과: 비교 횟수(첫 실행), 평균 시간(ms), 메모리(Byte) */
static void run(int alg, const int orig[], int work[], int tmp[], int n,
                long long *cmp, double *ms, size_t *mem) {
    double total = 0;
    for (int r = 0; r < REPEAT; r++) {
        memcpy(work, orig, sizeof(int) * n);        /* 복사: 측정 제외 */
        cmp_count = 0; cur_depth = 0; max_depth = 0;

        double s = now_ms();
        if (alg == 0) merge_sort(work, tmp, 0, n - 1);
        else          quick_sort(work, 0, n - 1, (PivotType)(alg - 1));
        double e = now_ms();

        total += e - s;
        if (!is_sorted(work, n)) printf("  [오류] 정렬 결과가 올바르지 않습니다!\n");
        if (r == 0) {
            *cmp = cmp_count;
            *mem = (alg == 0) ? sizeof(int) * (size_t)n + (size_t)max_depth * MERGE_FRAME
                              : (size_t)max_depth * QUICK_FRAME;
        }
    }
    *ms = total / REPEAT;
}

int main(void) {
    int sizes[] = { 1000, 2000, 4000, 8000, 16000, 20000 };
    const char *names[] = { "Merge Sort", "Quick (First)", "Quick (Random)", "Quick (Median3)" };

    srand((unsigned)time(NULL));

    for (int s = 0; s < 6; s++) {
        int n = sizes[s];
        int *orig = malloc(sizeof(int) * n);
        int *work = malloc(sizeof(int) * n);
        int *tmp  = malloc(sizeof(int) * n);   /* 병합 정렬 임시 배열 */
        if (!orig || !work || !tmp) { printf("메모리 할당 실패\n"); return 1; }

        for (int i = 0; i < n; i++) orig[i] = rand_int() % 100000;

        printf("Data Size : %d\n\n", n);
        printf("%-16s%-15s%-12s%s\n", "Algorithm", "Comparisons", "Time(ms)", "Memory(Bytes)");
        printf("--------------------------------------------------------\n");
        for (int alg = 0; alg < 4; alg++) {
            long long cmp; double ms; size_t mem;
            run(alg, orig, work, tmp, n, &cmp, &ms, &mem);
            printf("%-16s%-15lld%-12.4f%zu\n", names[alg], cmp, ms, mem);
        }
        printf("\n");
        free(orig); free(work); free(tmp);
    }
    return 0;
}
