#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef enum { FIRST, RANDOM, MEDIAN3 } PivotType;

static long long cmp_count;

static void swap(int *a, int *b) { int t = *a; *a = *b; *b = t; }

/* Windows의 rand()는 최대 32767이므로 두 번 조합 (30비트, 음수 없음) */
static int rand_int(void) {
    return ((rand() & 0x7FFF) << 15) | (rand() & 0x7FFF);
}

/* 구간 [lo, hi]에서 피벗 위치를 고르고 a[lo]로 옮긴다 */
static void choose_pivot(int a[], int lo, int hi, PivotType type) {
    int p = lo;
    if (type == RANDOM) {
        p = lo + rand_int() % (hi - lo + 1);   /* 비교 아님 */
    } else if (type == MEDIAN3) {
        int mid = lo + (hi - lo) / 2;
        int x = a[lo], y = a[mid], z = a[hi];
        cmp_count++;                           /* 중앙값 결정 비교 (2~3회) */
        if (x < y) {
            cmp_count++;
            if (y < z) p = mid;                /* x<y<z */
            else {
                cmp_count++;
                p = (x < z) ? hi : lo;
            }
        } else {
            cmp_count++;
            if (x < z) p = lo;                 /* y<=x<z */
            else {
                cmp_count++;
                p = (y < z) ? hi : mid;
            }
        }
    }
    if (p != lo) swap(&a[lo], &a[p]);
}

/* Hoare 방식 분할: 피벗은 a[lo], 세 방법 모두 동일 */
static int partition(int a[], int lo, int hi, PivotType type) {
    choose_pivot(a, lo, hi, type);
    int pivot = a[lo];
    int i = lo, j = hi + 1;
    while (1) {
        /* 범위 검사(i<=hi)는 세지 않고, 값 비교 a[i]<pivot만 센다 */
        while (++i <= hi) {
            cmp_count++;
            if (!(a[i] < pivot)) break;
        }
        /* a[lo]==pivot 이므로 j는 lo에서 반드시 멈춘다 */
        do { --j; cmp_count++; } while (a[j] > pivot);
        if (i >= j) break;
        swap(&a[i], &a[j]);
    }
    swap(&a[lo], &a[j]);
    return j;
}

/* 순수 재귀 퀵 정렬 */
static void quick_sort(int a[], int lo, int hi, PivotType type) {
    if (lo >= hi) return;
    int p = partition(a, lo, hi, type);
    quick_sort(a, lo, p - 1, type);
    quick_sort(a, p + 1, hi, type);
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
