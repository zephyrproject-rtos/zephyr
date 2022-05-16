/*
 * Copyright (c) 2021 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <zephyr/sys/util.h>

/*
 * Normally parent is defined parent(k) = floor((k-1) / 2) but we can avoid a
 * divide by noticing that floor((k-1) / 2) = ((k - 1) >> 1).
 */

#define parent(k) (((k) - 1) >> 1)
/*
 * Normally left is defined left(k) = (2 * k + 1) but we can avoid a
 * multiply by noticing that (2 * k + 1) = ((k << 1) + 1).
 */

#define left(k) (((k) << 1) + 1)

/*
 * Normally right is defined right(k) = (2 * k + 2) but we can avoid a
 * multiply by noticing that right(k) = left(k) + 1
 */
#define right(k) (left(k) + 1)

#define A(k) ((uint8_t *)base + size * (k))

struct qsort_comp {
	bool has3;
	void *arg;
	union {
		int (*comp2)(const void *a, const void *b);
		int (*comp3)(const void *a, const void *b, void *arg);
	};
};

static inline int compare(struct qsort_comp *cmp, void *a, void *b)
{
	if (cmp->has3) {
		return cmp->comp3(a, b, cmp->arg);
	}

	return cmp->comp2(a, b);
}

static void sift_down(void *base, int start, int end, size_t size, struct qsort_comp *cmp)
{
	int root;
	int child;
	int swap;

	for (swap = start, root = swap; left(root) < end; root = swap) {
		child = left(root);

		/* if root < left */
		if (compare(cmp, A(swap), A(child)) < 0) {
			swap = child;
		}

		/* right exists and min(A(root),A(left)) < A(right) */
		if (right(root) < end && compare(cmp, A(swap), A(right(root))) < 0) {
			swap = right(root);
		}

		if (swap == root) {
			return;
		}

		byteswp(A(root), A(swap), size);
	}
}

static void heapify(void *base, int nmemb, size_t size, struct qsort_comp *cmp)
{
	int start;

	for (start = parent(nmemb - 1); start >= 0; --start) {
		sift_down(base, start, nmemb, size, cmp);
	}
}

static void heap_sort(void *base, int nmemb, size_t size, struct qsort_comp *cmp)
{
	int end;

	heapify(base, nmemb, size, cmp);

	for (end = nmemb - 1; end > 0; --end) {
		byteswp(A(end), A(0), size);
		sift_down(base, 0, end, size, cmp);
	}
}

void qsort_r(void *base, size_t nmemb, size_t size,
	     int (*comp3)(const void *a, const void *b, void *arg), void *arg)
{
	struct qsort_comp cmp = {
		.has3 = true,
		.arg = arg,
		{
			.comp3 = comp3
		}
	};

	heap_sort(base, nmemb, size, &cmp);
}

void qsort(void *base, size_t nmemb, size_t size,
	   int (*comp2)(const void *a, const void *b))
{
	struct qsort_comp cmp = {
		.has3 = false,
		.arg = NULL,
		{
			.comp2 = comp2
		}
	};

	heap_sort(base, nmemb, size, &cmp);
}
