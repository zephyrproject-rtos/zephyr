/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <malloc.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static inline size_t mi_usage(const struct mallinfo *mi)
{
	return mi->uordblks;
}

static void print_mallinfo(const struct mallinfo *mi)
{
	printk("%p:\n"
	       "arena: %zu\n"
	       "ordblks: %zu\n"
	       "smblks: %zu\n"
	       "hblks: %zu\n"
	       "hblkhd: %zu\n"
	       "usmblks: %zu\n"
	       "fsmblks: %zu\n"
	       "uordblks: %zu\n"
	       "fordblks: %zu\n"
	       "keepcost: %zu\n",
	       mi, mi->arena, mi->ordblks, mi->smblks, mi->hblks, mi->hblkhd, mi->usmblks,
	       mi->fsmblks, mi->uordblks, mi->fordblks, mi->keepcost);
}

static bool mi_gt(const struct mallinfo *a, const struct mallinfo *b)
{
	if (mi_usage(a) <= mi_usage(b)) {
		printk("mallinfo %p is <= mallinfo %p\n", a, b);
		print_mallinfo(a);
		printk("\n");
		print_mallinfo(b);
		return false;
	}

	return true;
}

ZTEST(mallinfo, test_mallinfo)
{
	uint8_t *data;
	struct mallinfo mi_then;
	struct mallinfo mi_now;

	mi_then = mallinfo();
	data = malloc(42);
	mi_now = mallinfo();
	zassert_true(mi_gt(&mi_now, &mi_then));

	mi_then = mi_now;
	free(data);
	mi_now = mallinfo();
	zassert_true(mi_gt(&mi_then, &mi_now));
}

ZTEST_SUITE(mallinfo, NULL, NULL, NULL, NULL, NULL);
