/*
 * Copyright (c) 2019 Balaji Kulkarni
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>

/**
 * @brief Generic implementation of Binary Search
 *
 * @param key	pointer to first element to search
 * @param array	pointer to item being searched for
 * @param count	number of elements
 * @param size	size of each element
 * @param cmp	pointer to comparison function
 *
 * @return	pointer to key if present in the array, or NULL otherwise
 */

void *bsearch(const void *key, const void *array, size_t count, size_t size,
	      int (*cmp)(const void *key, const void *element))
{
	size_t low = 0;
	size_t high = count;
	size_t index;
	int result;
	const char *pivot;

	while (low < high) {
		index = low + (high - low) / 2;
		pivot = (const char *)array + index * size;
		result = cmp(key, pivot);

		if (result == 0) {
			return (void *)pivot;
		} else if (result > 0) {
			low = index + 1;
		} else {
			high = index;

		}
	}
	return NULL;
}
