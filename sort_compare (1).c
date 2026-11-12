/*
 * 과제 14: 힙 정렬, 퀵 정렬 및 병합 정렬의 성능 비교
 *
 * - 힙 정렬(Heap Sort), 병합 정렬(Merge Sort),
 *   퀵 정렬(Quick Sort: 첫 번째 원소 / 무작위 / 세 원소의 중앙값)을 구현한다.
 * - 데이터 크기 n = 1,000 / 2,000 / 4,000 / 8,000 / 16,000 / 20,000 에 대해
 *   숫자 비교 횟수, 실행시간(ms), 최대 추가 메모리(Byte)를 측정한다.
 * - 모든 알고리즘은 동일한 원본 데이터의 복사본을 사용한다.
 *
 * 컴파일: gcc -O2 -o sort_compare sort_compare.c
 * 실행  : ./sort_compare
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------------------
 * 메모리 계산 기준 (README 참고)
 *   int = 4 Byte, 포인터 = 8 Byte, 반환 주소 = 8 Byte (64비트 환경)
 * ---------------------------------------------------------------------- */
#define INT_SZ  ((long long)sizeof(int))
#define PTR_SZ  ((long long)sizeof(void *))
#define RET_SZ  8LL

/* 퀵 정렬 재귀 호출 1회의 스택 프레임 크기
 *   반환 주소 + 배열 포인터 + lo, hi + 지역변수 i, j, pivot = 8+8+8+12 = 36 */
#define QUICK_FRAME (RET_SZ + PTR_SZ + 2 * INT_SZ + 3 * INT_SZ)
/* 병합 정렬 재귀 호출 1회의 스택 프레임 크기
 *   반환 주소 + 배열 포인터 + 임시배열 포인터 + lo, mid, hi + i, j, k
 *   = 8+8+8+12+12 = 48 */
#define MERGE_FRAME (RET_SZ + 2 * PTR_SZ + 3 * INT_SZ + 3 * INT_SZ)
/* 힙 정렬 보조 변수(재귀 없음): n, parent, child, end, tmp = int 5개 = 20 */
#define HEAP_AUX    (5 * INT_SZ)

/* 숫자 비교 횟수 카운터 (알고리즘 실행 전에 0으로 초기화) */
static long long cmp_count = 0;
/* 재귀 최대 깊이 (병합/퀵 정렬의 스택 메모리 계산용) */
static int cur_depth = 0, max_depth = 0;

static void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

/* ========================================================================
 * 1. 힙 정렬 (최대 힙, 입력 배열 자체를 힙으로 사용)
 * ====================================================================== */

/* a[start]를 아래로 내려 힙 속성을 복구한다. (힙 범위: a[0..end-1]) */
static void sift_down(int *a, int start, int end)
{
    int parent = start;
    for (;;) {
        int child = 2 * parent + 1;
        if (child >= end)                    /* 인덱스 검사: 비교 횟수 제외 */
            break;
        if (child + 1 < end) {               /* 인덱스 검사: 비교 횟수 제외 */
            cmp_count++;                     /* 두 자식 값 비교 */
            if (a[child + 1] > a[child])
                child++;
        }
        cmp_count++;                         /* 부모와 큰 자식 값 비교 */
        if (a[child] > a[parent]) {
            swap(&a[parent], &a[child]);
            parent = child;
        } else {
            break;
        }
    }
}

static long long heap_sort(int *a, int n)
{
    int i, end;
    /* (1) 배열을 최대 힙으로 구성 */
    for (i = n / 2 - 1; i >= 0; i--)
        sift_down(a, i, n);
    /* (2) 루트(최댓값)를 힙의 마지막과 교환 -> 힙 크기 감소 -> 힙 속성 복구 */
    for (end = n - 1; end > 0; end--) {
        swap(&a[0], &a[end]);
        sift_down(a, 0, end);
    }
    return HEAP_AUX;                         /* 추가 메모리: 보조 변수뿐 */
}

/* ========================================================================
 * 2. 병합 정렬 (임시 배열 사용, 하향식)
 * ====================================================================== */
static void merge_rec(int *a, int *tmp, int lo, int hi)
{
    int mid, i, j, k;

    cur_depth++;
    if (cur_depth > max_depth)
        max_depth = cur_depth;

    if (lo < hi) {
        mid = (lo + hi) / 2;
        merge_rec(a, tmp, lo, mid);
        merge_rec(a, tmp, mid + 1, hi);

        /* 정렬된 두 부분을 병합 */
        i = lo; j = mid + 1; k = lo;
        while (i <= mid && j <= hi) {
            cmp_count++;                     /* 숫자 비교 */
            if (a[i] <= a[j])
                tmp[k++] = a[i++];
            else
                tmp[k++] = a[j++];
        }
        while (i <= mid) tmp[k++] = a[i++];
        while (j <= hi)  tmp[k++] = a[j++];
        for (k = lo; k <= hi; k++)
            a[k] = tmp[k];
    }
    cur_depth--;
}

static long long merge_sort(int *a, int n)
{
    int *tmp = (int *)malloc(sizeof(int) * n);   /* 임시 배열: n * 4 Byte */
    cur_depth = max_depth = 0;
    merge_rec(a, tmp, 0, n - 1);
    free(tmp);
    return (long long)n * INT_SZ + (long long)max_depth * MERGE_FRAME;
}

/* ========================================================================
 * 3. 퀵 정렬 (피벗 선택 방법 3가지)
 * ====================================================================== */
typedef int (*pivot_fn)(int *a, int lo, int hi);

/* (1) 첫 번째 원소 선택: 비교 없음 */
static int pivot_first(int *a, int lo, int hi)
{
    (void)a; (void)hi;
    return lo;
}

/* (2) 무작위 원소 선택: 비교 없음 */
static int pivot_random(int *a, int lo, int hi)
{
    (void)a;
    return lo + rand() % (hi - lo + 1);
}

/* (3) 세 원소(lo, mid, hi)의 중앙값 선택: 값 비교 2~3회를 카운트한다. */
static int pivot_median3(int *a, int lo, int hi)
{
    int mid = lo + (hi - lo) / 2;
    int x = a[lo], y = a[mid], z = a[hi];

    cmp_count++;                             /* x < y ? */
    if (x < y) {
        cmp_count++;                         /* y < z ? */
        if (y < z) return mid;               /* x < y < z */
        cmp_count++;                         /* x < z ? */
        return (x < z) ? hi : lo;
    } else {
        cmp_count++;                         /* x < z ? */
        if (x < z) return lo;                /* y <= x < z */
        cmp_count++;                         /* y < z ? */
        return (y < z) ? hi : mid;
    }
}

static void quick_rec(int *a, int lo, int hi, pivot_fn pf)
{
    int p, pivot, i, j;

    cur_depth++;
    if (cur_depth > max_depth)
        max_depth = cur_depth;

    if (lo < hi) {
        /* 피벗을 선택하여 맨 앞으로 옮긴다. */
        p = pf(a, lo, hi);
        swap(&a[lo], &a[p]);
        pivot = a[lo];

        i = lo + 1;
        j = hi;
        for (;;) {
            /* 인덱스 검사(i <= j)는 비교 횟수에서 제외, 값 비교만 카운트 */
            while (i <= j) {
                cmp_count++;
                if (a[i] < pivot) i++;
                else break;
            }
            while (i <= j) {
                cmp_count++;
                if (a[j] > pivot) j--;
                else break;
            }
            if (i >= j)
                break;
            swap(&a[i], &a[j]);
            i++;
            j--;
        }
        swap(&a[lo], &a[j]);                 /* 피벗을 최종 위치로 */

        quick_rec(a, lo, j - 1, pf);
        quick_rec(a, j + 1, hi, pf);
    }
    cur_depth--;
}

static long long quick_sort(int *a, int n, pivot_fn pf)
{
    cur_depth = max_depth = 0;
    quick_rec(a, 0, n - 1, pf);
    return (long long)max_depth * QUICK_FRAME;
}

/* ========================================================================
 * 측정 및 출력
 * ====================================================================== */
typedef enum { HEAP, MERGE, QUICK_FIRST, QUICK_RANDOM, QUICK_MEDIAN3 } algo_t;

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/* 정렬 알고리즘 하나를 실행하고 비교 횟수, 시간, 메모리를 반환한다. */
static void run(algo_t algo, const char *name, const int *orig, const int *ref,
                int n, long long *comps, double *ms, long long *mem)
{
    int *a = (int *)malloc(sizeof(int) * n);
    double t0, t1;
    int i;

    memcpy(a, orig, sizeof(int) * n);        /* 배열 복사 (시간 측정에서 제외) */
    cmp_count = 0;

    t0 = now_ms();                           /* ---- 시간 측정 시작 ---- */
    switch (algo) {
    case HEAP:          *mem = heap_sort(a, n);                      break;
    case MERGE:         *mem = merge_sort(a, n);                     break;
    case QUICK_FIRST:   *mem = quick_sort(a, n, pivot_first);        break;
    case QUICK_RANDOM:  *mem = quick_sort(a, n, pivot_random);       break;
    case QUICK_MEDIAN3: *mem = quick_sort(a, n, pivot_median3);      break;
    }
    t1 = now_ms();                           /* ---- 시간 측정 종료 ---- */

    *comps = cmp_count;
    *ms = t1 - t0;

    /* 정렬 결과 검증: 기준 배열(ref)과 모든 원소가 같아야 한다. */
    for (i = 0; i < n; i++) {
        if (a[i] != ref[i]) {
            printf("오류: %s 정렬 결과가 올바르지 않습니다!\n", name);
            exit(1);
        }
    }
    free(a);
}

int main(void)
{
    const int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    const int num_sizes = (int)(sizeof(sizes) / sizeof(sizes[0]));
    const char *names[] = {"Heap Sort", "Merge Sort", "Quick (First)",
                           "Quick (Random)", "Quick (Median3)"};
    const algo_t algos[] = {HEAP, MERGE, QUICK_FIRST, QUICK_RANDOM, QUICK_MEDIAN3};
    int s, k, i;

    srand(2026);                             /* 재현 가능하도록 시드 고정 */

    for (s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        int *orig = (int *)malloc(sizeof(int) * n);
        int *ref  = (int *)malloc(sizeof(int) * n);

        for (i = 0; i < n; i++)
            orig[i] = rand() % 1000001;      /* 무작위 정수 (중복 허용) */
        memcpy(ref, orig, sizeof(int) * n);
        qsort(ref, n, sizeof(int), cmp_int); /* 검증용 정답 배열 */

        printf("Data Size : %d\n\n", n);
        printf("%-16s%-15s%-12s%s\n", "Algorithm", "Comparisons", "Time(ms)", "Memory(Bytes)");
        printf("--------------------------------------------------------\n");
        for (k = 0; k < 5; k++) {
            long long comps = 0, mem = 0;
            double ms = 0.0;
            run(algos[k], names[k], orig, ref, n, &comps, &ms, &mem);
            printf("%-16s%-15lld%-12.3f%lld\n", names[k], comps, ms, mem);
        }
        printf("\n");
        free(orig);
        free(ref);
    }
    return 0;
}
