/*
 * Copyright (c) 2019 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This test demonstrates a serious bugs that exist in the mem-pool code
 * when the fix from PR #16966 is not applied.
 *
 * Expected output with the bug present:
 *
 *	allocating boxes
 *	init box 3
 *	show box 3: tic="tic" tac="tac" toe="toe"
 *	init box 4
 *	show box 4: tic="tic" tac="tac" toe="toe"
 *	show box 3: tic="tic" tac="tac" toe="tic"
 *	init box 3
 *	show box 3: tic="tic" tac="tac" toe="toe"
 *	show box 4: tic="toe" tac="tac" toe="toe"
 */

/**
 * @brief Memory Pool Bug Tests
 * @defgroup kernel_memory_pool_tests Memory Pool
 * @ingroup all_tests
 * @{
 * @}
 */

#include <ztest.h>
#include <zephyr.h>
#include <misc/printk.h>

K_MEM_POOL_DEFINE(my_pool, 8, 44, 9, 4);

struct tic_tac_toe { char *tic; char *tac; char *toe; };

static struct tic_tac_toe *boxes[9];

static bool spooked;

static void init_box(int n)
{
	printk("init box %d\n", n);
	boxes[n]->tic = "tic";
	boxes[n]->tac = "tac";
	boxes[n]->toe = "toe";
}

static void show_box(int n)
{
	printk("show box %d: tic=\"%s\" tac=\"%s\" toe=\"%s\"\n",
	       n, boxes[n]->tic, boxes[n]->tac, boxes[n]->toe);

	if (strcmp(boxes[n]->tic, "tic") != 0 ||
	    strcmp(boxes[n]->tac, "tac") != 0 ||
	    strcmp(boxes[n]->toe, "toe") != 0) {
		spooked = true;
	}
}

void test_mempool_spook(void)
{
	struct k_mem_block block[9];
	int i, ret;

	printk("allocating boxes\n");
	for (i = 0; i < 9; i++) {
		ret = k_mem_pool_alloc(&my_pool, &block[i],
				       sizeof(struct tic_tac_toe), 0);
		zassert_false(ret != 0 || block[i].data == NULL,
			      "memory allocation failure\n");
		boxes[i] = block[i].data;
	}

	init_box(3);
	show_box(3);
	init_box(4);
	show_box(4);
	show_box(3); /* ! */
	init_box(3);
	show_box(3);
	show_box(4); /* ! */

	zassert_false(spooked, "");
}
