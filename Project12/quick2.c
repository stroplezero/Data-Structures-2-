#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#define N        10000
#define REPEAT   200      /* 반복 측정 횟수 (평균 사용) */

static int rand_int(void) { return (rand() << 15) | rand(); }
static void swap(int *a, int *b) { int t = *a; *a = *b; *b = t; }

/* 세 원소의 중앙값 위치를 골라 a[lo]로 옮김 */
static void choose_median3(int a[], int lo, int hi) {
    int mid = lo + (hi - lo) / 2;
    int x = a[lo], y = a[mid], z = a[hi];
    int p;
    if (x < y) {
        if (y < z) p = mid;
        else       p = (x < z) ? hi : lo;
    } else {
        if (x < z) p = lo;
        else       p = (y < z) ? hi : mid;
    }
    if (p != lo) swap(&a[lo], &a[p]);
}

/* Hoare 방식 분할 (두 방법이 공통 사용), 피벗은 a[lo] */
static int partition(int a[], int lo, int hi) {
    choose_median3(a, lo, hi);
    int pivot = a[lo];
    int i = lo, j = hi + 1;
    while (1) {
        while (++i <= hi && a[i] < pivot) ;
        while (a[--j] > pivot) ;          /* j==lo에서 pivot과 같아 멈춤 */
        if (i >= j) break;
        swap(&a[i], &a[j]);
    }
    swap(&a[lo], &a[j]);
    return j;
}

/* 1) 재귀적 퀵 정렬 */
static void quick_recursive(int a[], int lo, int hi) {
    if (lo >= hi) return;
    int p = partition(a, lo, hi);
    quick_recursive(a, lo, p - 1);
    quick_recursive(a, p + 1, hi);
}

/* 2) 반복적 퀵 정렬 (명시적 스택) */
static void quick_iterative(int a[], int n) {
    int *stack = (int *)malloc(sizeof(int) * 2 * (n + 1));
    int top = -1;
    stack[++top] = 0;
    stack[++top] = n - 1;
    while (top >= 0) {
        int hi = stack[top--];
        int lo = stack[top--];
        if (lo >= hi) continue;
        int p = partition(a, lo, hi);
        stack[++top] = lo;     stack[++top] = p - 1;
        stack[++top] = p + 1;  stack[++top] = hi;
    }
    free(stack);
}

static int is_sorted(const int a[], int n) {
    for (int i = 1; i < n; i++) if (a[i - 1] > a[i]) return 0;
    return 1;
}

static double now_ms(void) {
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
}

int main(void) {
    int *orig = (int *)malloc(sizeof(int) * N);
    int *work = (int *)malloc(sizeof(int) * N);
    double t_rec = 0, t_iter = 0;

    srand((unsigned)time(NULL));
    for (int i = 0; i < N; i++) orig[i] = rand_int() % 100000;

    for (int r = 0; r < REPEAT; r++) {
        double s, e;

        memcpy(work, orig, sizeof(int) * N);          /* 복사: 측정 제외 */
        s = now_ms();
        quick_recursive(work, 0, N - 1);
        e = now_ms();
        t_rec += e - s;
        if (!is_sorted(work, N)) printf("[오류] 재귀 정렬 실패\n");

        memcpy(work, orig, sizeof(int) * N);
        s = now_ms();
        quick_iterative(work, N);
        e = now_ms();
        t_iter += e - s;
        if (!is_sorted(work, N)) printf("[오류] 반복 정렬 실패\n");
    }

    printf("Data Size : %d\n", N);
    printf("Pivot     : Median-of-Three\n");
    printf("(평균, %d회 반복)\n\n", REPEAT);
    printf("Recursive Quick Sort : %.4f ms\n", t_rec / REPEAT);
    printf("Iterative Quick Sort : %.4f ms\n", t_iter / REPEAT);

    free(orig);
    free(work);
    return 0;
}
