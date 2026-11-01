#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef long long ll;

/* 삽입 정렬: 값 비교 1회당 count 증가 */
ll insertion_sort(int a[], int n)
{
    ll cnt = 0;
    for (int i = 1; i < n; i++) {
        int key = a[i];
        int j = i - 1;
        while (j >= 0) {
            cnt++;                  /* 데이터 값 비교 (a[j] > key) */
            if (a[j] > key) {
                a[j + 1] = a[j];
                j--;
            }
            else break;
        }
        a[j + 1] = key;
    }
    return cnt;
}

/* 간격 h를 사용한 gap 삽입 정렬 */
static void gap_insertion(int a[], int n, int h, ll* cnt)
{
    for (int i = h; i < n; i++) {
        int key = a[i];
        int j = i - h;
        while (j >= 0) {
            (*cnt)++;               /* 데이터 값 비교 */
            if (a[j] > key) {
                a[j + h] = a[j];
                j -= h;
            }
            else break;
        }
        a[j + h] = key;
    }
}

/* 쉘 정렬 - 기본 간격: n/2, n/4, ..., 1 */
ll shell_sort_original(int a[], int n)
{
    ll cnt = 0;
    for (int h = n / 2; h >= 1; h /= 2)
        gap_insertion(a, n, h, &cnt);
    return cnt;
}

/* 쉘 정렬 - Knuth 간격: h(k+1)=3h(k)+1, n보다 작은 가장 큰 값부터 역순 */
ll shell_sort_knuth(int a[], int n)
{
    ll cnt = 0;
    int h = 1;
    while (3 * h + 1 < n)
        h = 3 * h + 1;
    for (; h >= 1; h = (h - 1) / 3) {
        gap_insertion(a, n, h, &cnt);
        if (h == 1) break;
    }
    return cnt;
}

/* 오름차순 검증 */
int is_sorted(const int a[], int n)
{
    for (int i = 1; i < n; i++)
        if (a[i - 1] > a[i]) return 0;
    return 1;
}

int main(void)
{
    int sizes[] = { 1000, 2000, 4000, 8000, 16000, 20000 };
    int num = sizeof(sizes) / sizeof(sizes[0]);

    srand((unsigned int)time(NULL));

    for (int s = 0; s < num; s++) {
        int n = sizes[s];
        int* orig = (int*)malloc(sizeof(int) * n);
        int* work = (int*)malloc(sizeof(int) * n);
        if (!orig || !work) {
            printf("메모리 할당 실패\n");
            return 1;
        }

        /* 원본 데이터 생성 (측정 제외) */
        for (int i = 0; i < n; i++)
            orig[i] = (rand() << 15 | rand()) % 1000000;

        /* 삽입 정렬 */
        for (int i = 0; i < n; i++) work[i] = orig[i];
        ll c1 = insertion_sort(work, n);
        int ok1 = is_sorted(work, n);

        /* 쉘 정렬 (기본) */
        for (int i = 0; i < n; i++) work[i] = orig[i];
        ll c2 = shell_sort_original(work, n);
        int ok2 = is_sorted(work, n);

        /* 쉘 정렬 (Knuth) */
        for (int i = 0; i < n; i++) work[i] = orig[i];
        ll c3 = shell_sort_knuth(work, n);
        int ok3 = is_sorted(work, n);

        printf("Data Size : %d\n\n", n);
        printf("Insertion Sort       : %lld comparisons\n", c1);
        printf("Shell Sort (Original): %lld comparisons\n", c2);
        printf("Shell Sort (Knuth)   : %lld comparisons\n", c3);
        if (!(ok1 && ok2 && ok3))
            printf("[경고] 정렬 결과 오류 발생!\n");
        printf("\n");

        free(orig);
        free(work);
    }
    return 0;
}
