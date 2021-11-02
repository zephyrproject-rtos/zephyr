/*
 * Copyright (c) 2021 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/util.h>

typedef int (*comp3_t)(const void *, const void *, void *);

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

static void sift_down(void *base, int start, int end, size_t size, comp3_t comp, void *comp_arg)
{
	int root;
	int child;
	int swap;

	for (swap = start, root = swap; left(root) < end; root = swap) {
		child = left(root);

		/* if root < left */
		if (comp(A(swap), A(child), comp_arg) < 0) {
			swap = child;
		}

		/* right exists and min(A(root),A(left)) < A(right) */
		if (right(root) < end && comp(A(swap), A(right(root)), comp_arg) < 0) {
			swap = right(root);
		}

		if (swap == root) {
			return;
		}

		byteswp(A(root), A(swap), size);
	}
}

static void heapify(void *base, int nmemb, size_t size, comp3_t comp, void *comp_arg)
{
	int start;

	for (start = parent(nmemb - 1); start >= 0; --start) {
		sift_down(base, start, nmemb, size, comp, comp_arg);
	}
}

static void heap_sort(void *base, int nmemb, size_t size, comp3_t comp, void *comp_arg)
{
	int end;

	heapify(base, nmemb, size, comp, comp_arg);

	for (end = nmemb - 1; end > 0; --end) {
		byteswp(A(end), A(0), size);
		sift_down(base, 0, end, size, comp, comp_arg);
	}
}

void qsort_r(void *base, size_t nmemb, size_t size, comp3_t comp, void *arg)
{
	heap_sort(base, nmemb, size, comp, arg);
}
