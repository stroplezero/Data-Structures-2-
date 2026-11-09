#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef enum { FIRST, RANDOM, MEDIAN3 } PivotType;

static long long cmp_count;

static void swap(int *a, int *b) { int t = *a; *a = *b; *b = t; }

/* 난수 생성: Windows의 rand()는 최대 32767이라 두 번 조합 */
static int rand_int(void) {
    return (rand() << 15) | rand();
}

/* 구간 [lo, hi]에서 피벗 위치를 고르고 lo로 옮긴다 */
static void choose_pivot(int a[], int lo, int hi, PivotType type) {
    int p = lo;
    if (type == RANDOM) {
        p = lo + rand_int() % (hi - lo + 1);   /* 비교 아님 */
    } else if (type == MEDIAN3) {
        int mid = lo + (hi - lo) / 2;
        int x = a[lo], y = a[mid], z = a[hi];
        /* 중앙값 결정을 위한 비교 (최대 3회) 포함 */
        cmp_count++;
        if (x < y) {
            cmp_count++;
            if (y < z) p = mid;            /* x<y<z */
            else {
                cmp_count++;
                p = (x < z) ? hi : lo;     /* x<z<=y 또는 z<=x<y */
            }
        } else {
            cmp_count++;
            if (x < z) p = lo;             /* y<=x<z */
            else {
                cmp_count++;
                p = (y < z) ? hi : mid;    /* y<z<=x 또는 z<=y<=x */
            }
        }
    }
    if (p != lo) swap(&a[lo], &a[p]);
}

/* Hoare 방식 분할: 피벗은 a[lo]에 위치 (모든 방법 동일) */
static int partition(int a[], int lo, int hi, PivotType type) {
    choose_pivot(a, lo, hi, type);
    int pivot = a[lo];
    int i = lo, j = hi + 1;
    while (1) {
        do { i++; if (i > hi) break; cmp_count++; } while (a[i] < pivot);
        do { j--; cmp_count++; } while (a[j] > pivot);   /* j==lo면 a[lo]==pivot에서 멈춤 */
        if (i >= j) break;
        swap(&a[i], &a[j]);
    }
    swap(&a[lo], &a[j]);
    return j;
}

/* 재귀 깊이 문제(First 피벗 최악의 경우)를 피하려고 작은 쪽만 재귀, 큰 쪽은 반복 */
static void quick_sort(int a[], int lo, int hi, PivotType type) {
    while (lo < hi) {
        int p = partition(a, lo, hi, type);
        if (p - lo < hi - p) {
            quick_sort(a, lo, p - 1, type);
            lo = p + 1;
        } else {
            quick_sort(a, p + 1, hi, type);
            hi = p - 1;
        }
    }
}

static int is_sorted(const int a[], int n) {
    for (int i = 1; i < n; i++)
        if (a[i - 1] > a[i]) return 0;
    return 1;
}

int main(void) {
    int sizes[] = { 1000, 2000, 4000, 8000, 16000, 20000 };
    int count = sizeof(sizes) / sizeof(sizes[0]);
    const char *names[] = { "First ", "Random", "Median-3" };

    srand((unsigned)time(NULL));

    for (int s = 0; s < count; s++) {
        int n = sizes[s];
        int *orig = (int *)malloc(sizeof(int) * n);
        int *work = (int *)malloc(sizeof(int) * n);
        if (!orig || !work) { printf("메모리 할당 실패\n"); return 1; }

        for (int i = 0; i < n; i++) orig[i] = rand_int() % 100000;  /* 중복 허용 */

        printf("Data Size : %d\n\n", n);
        for (int t = 0; t < 3; t++) {
            for (int i = 0; i < n; i++) work[i] = orig[i];   /* 동일 원본 복사 */
            cmp_count = 0;
            quick_sort(work, 0, n - 1, (PivotType)t);
            if (!is_sorted(work, n))
                printf("  [오류] 정렬 결과가 올바르지 않습니다!\n");
            printf("Quick Sort (%-8s) : %lld comparisons\n", names[t], cmp_count);
        }
        printf("\n");
        free(orig);
        free(work);
    }
    return 0;
}
