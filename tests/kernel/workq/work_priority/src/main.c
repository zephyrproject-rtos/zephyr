/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

/* Track execution order */
#define MAX_WORK_ITEMS 10
static int execution_order[MAX_WORK_ITEMS];
static int execution_index;
static struct k_sem completion_sem;

/* Work item wrapper to track ID */
struct test_work {
	struct k_work work;
	int id;
	int total_items;
};

/* Work handler that records execution order */
static void work_handler(struct k_work *work)
{
	struct test_work *tw = CONTAINER_OF(work, struct test_work, work);

	execution_order[execution_index++] = tw->id;
	printk("Work %d executed (priority: %d)\n", tw->id, work->priority);

	/* Signal completion when all items processed */
	if (execution_index >= tw->total_items) {
		k_sem_give(&completion_sem);
	}
}

/* Test basic priority ordering with system work queue */
ZTEST(work_priority, test_basic_priority_ordering)
{
	static struct test_work work[5];

	/* Initialize test state */
	execution_index = 0;
	memset(execution_order, -1, sizeof(execution_order));
	k_sem_init(&completion_sem, 0, 1);

	/* Initialize work items with different priorities
	 * Higher priority (lower number) should execute first
	 */
	work[0].id = 0;
	work[0].total_items = 5;
	k_work_init_priority(&work[0].work, work_handler, 5);  /* Lowest priority */

	work[1].id = 1;
	work[1].total_items = 5;
	k_work_init_priority(&work[1].work, work_handler, 1);  /* High priority */

	work[2].id = 2;
	work[2].total_items = 5;
	k_work_init_priority(&work[2].work, work_handler, 3);  /* Medium priority */

	work[3].id = 3;
	work[3].total_items = 5;
	k_work_init_priority(&work[3].work, work_handler, 0);  /* Highest priority */

	work[4].id = 4;
	work[4].total_items = 5;
	k_work_init_priority(&work[4].work, work_handler, 2);  /* Medium-high priority */

	/* Submit all work items quickly to system work queue */
	k_work_submit(&work[0].work);
	k_work_submit(&work[1].work);
	k_work_submit(&work[2].work);
	k_work_submit(&work[3].work);
	k_work_submit(&work[4].work);

	/* Wait for all work to complete */
	k_sem_take(&completion_sem, K_SECONDS(5));

	/* Verify execution order: should be 3, 1, 4, 2, 0 */
	printk("Execution order: ");
	for (int i = 0; i < 5; i++) {
		printk("%d ", execution_order[i]);
	}
	printk("\n");

	zassert_equal(execution_order[0], 3, "Work 3 (prio 0) should execute first");
	zassert_equal(execution_order[1], 1, "Work 1 (prio 1) should execute second");
	zassert_equal(execution_order[2], 4, "Work 4 (prio 2) should execute third");
	zassert_equal(execution_order[3], 2, "Work 2 (prio 3) should execute fourth");
	zassert_equal(execution_order[4], 0, "Work 0 (prio 5) should execute last");
}

/* Test FIFO ordering for equal priority items */
ZTEST(work_priority, test_equal_priority_fifo)
{
	static struct test_work work[4];

	/* Initialize test state */
	execution_index = 0;
	memset(execution_order, -1, sizeof(execution_order));
	k_sem_init(&completion_sem, 0, 1);

	/* Initialize all work items with same priority */
	for (int i = 0; i < 4; i++) {
		work[i].id = i;
		work[i].total_items = 4;
		k_work_init_priority(&work[i].work, work_handler, 5);
	}

	/* Submit in order 0, 1, 2, 3 */
	for (int i = 0; i < 4; i++) {
		k_work_submit(&work[i].work);
	}

	/* Wait for all work to complete */
	k_sem_take(&completion_sem, K_SECONDS(5));

	/* With equal priority, should maintain FIFO: 0, 1, 2, 3 */
	printk("FIFO execution order: ");
	for (int i = 0; i < 4; i++) {
		printk("%d ", execution_order[i]);
	}
	printk("\n");

	for (int i = 0; i < 4; i++) {
		zassert_equal(execution_order[i], i, "FIFO order not maintained");
	}
}

/* Test mixed priority and dynamic submission */
ZTEST(work_priority, test_dynamic_priority_insertion)
{
	static struct test_work work[6];

	/* Initialize test state */
	execution_index = 0;
	memset(execution_order, -1, sizeof(execution_order));
	k_sem_init(&completion_sem, 0, 1);

	/* Submit items with various priorities at different times */
	work[0].id = 0;
	work[0].total_items = 6;
	k_work_init_priority(&work[0].work, work_handler, 5);
	k_work_submit(&work[0].work);

	work[1].id = 1;
	work[1].total_items = 6;
	k_work_init_priority(&work[1].work, work_handler, 3);
	k_work_submit(&work[1].work);

	work[2].id = 2;
	work[2].total_items = 6;
	k_work_init_priority(&work[2].work, work_handler, 1);
	k_work_submit(&work[2].work);

	work[3].id = 3;
	work[3].total_items = 6;
	k_work_init_priority(&work[3].work, work_handler, 0);
	k_work_submit(&work[3].work);

	work[4].id = 4;
	work[4].total_items = 6;
	k_work_init_priority(&work[4].work, work_handler, 2);
	k_work_submit(&work[4].work);

	work[5].id = 5;
	work[5].total_items = 6;
	k_work_init_priority(&work[5].work, work_handler, 4);
	k_work_submit(&work[5].work);

	/* Wait for all work to complete */
	k_sem_take(&completion_sem, K_SECONDS(5));

	/* Should execute in priority order: 3, 2, 4, 1, 5, 0 */
	printk("Dynamic insertion order: ");
	for (int i = 0; i < 6; i++) {
		printk("%d ", execution_order[i]);
	}
	printk("\n");

	zassert_equal(execution_order[0], 3, "Highest priority first");
	zassert_equal(execution_order[1], 2, "Second highest priority");
	zassert_equal(execution_order[2], 4, "Third priority");
	zassert_equal(execution_order[3], 1, "Fourth priority");
	zassert_equal(execution_order[4], 5, "Fifth priority");
	zassert_equal(execution_order[5], 0, "Lowest priority last");
}

/* ISR context callback for testing ISR submission */
static void isr_submit_work(const void *param)
{
	struct test_work *tw = (struct test_work *)param;

	k_work_submit(&tw->work);
}

/* Test ISR context submission maintains priority ordering */
ZTEST(work_priority, test_isr_submit_priority)
{
	static struct test_work work[4];

	/* Initialize test state */
	execution_index = 0;
	memset(execution_order, -1, sizeof(execution_order));
	k_sem_init(&completion_sem, 0, 1);

	/* Initialize work items with different priorities */
	work[0].id = 0;
	work[0].total_items = 4;
	k_work_init_priority(&work[0].work, work_handler, 10);

	work[1].id = 1;
	work[1].total_items = 4;
	k_work_init_priority(&work[1].work, work_handler, 0);

	work[2].id = 2;
	work[2].total_items = 4;
	k_work_init_priority(&work[2].work, work_handler, 5);

	work[3].id = 3;
	work[3].total_items = 4;
	k_work_init_priority(&work[3].work, work_handler, 2);

	/* Submit all work items from ISR context */
	irq_offload(isr_submit_work, &work[0]);
	irq_offload(isr_submit_work, &work[1]);
	irq_offload(isr_submit_work, &work[2]);
	irq_offload(isr_submit_work, &work[3]);

	/* Wait for all work to complete */
	k_sem_take(&completion_sem, K_SECONDS(5));

	/* Verify execution order: should be 1(0), 3(2), 2(5), 0(10) */
	printk("ISR submission order: ");
	for (int i = 0; i < 4; i++) {
		printk("%d ", execution_order[i]);
	}
	printk("\n");

	zassert_equal(execution_order[0], 1, "Work 1 (prio 0) should execute first");
	zassert_equal(execution_order[1], 3, "Work 3 (prio 2) should execute second");
	zassert_equal(execution_order[2], 2, "Work 2 (prio 5) should execute third");
	zassert_equal(execution_order[3], 0, "Work 0 (prio 10) should execute last");
}

/* Test flush interaction with priority ordering */
ZTEST(work_priority, test_flush_with_priority)
{
	static struct test_work work[3];
	struct k_work_sync sync;

	/* Initialize test state */
	execution_index = 0;
	memset(execution_order, -1, sizeof(execution_order));
	k_sem_init(&completion_sem, 0, 1);

	/* Initialize work items with different priorities */
	work[0].id = 0;
	work[0].total_items = 3;
	k_work_init_priority(&work[0].work, work_handler, 10);

	work[1].id = 1;
	work[1].total_items = 3;
	k_work_init_priority(&work[1].work, work_handler, 5);

	work[2].id = 2;
	work[2].total_items = 3;
	k_work_init_priority(&work[2].work, work_handler, 0);

	/* Submit all work items */
	k_work_submit(&work[0].work);
	k_work_submit(&work[1].work);
	k_work_submit(&work[2].work);

	/* Flush work[1] - should not disrupt priority ordering */
	k_work_flush(&work[1].work, &sync);

	/* Wait for all work to complete */
	k_sem_take(&completion_sem, K_SECONDS(5));

	/* Verify execution order: should be 2(0), 1(5), 0(10) */
	printk("Flush interaction order: ");
	for (int i = 0; i < 3; i++) {
		printk("%d ", execution_order[i]);
	}
	printk("\n");

	zassert_equal(execution_order[0], 2, "Work 2 (prio 0) should execute first");
	zassert_equal(execution_order[1], 1, "Work 1 (prio 5) should execute second");
	zassert_equal(execution_order[2], 0, "Work 0 (prio 10) should execute last");
}

ZTEST_SUITE(work_priority, NULL, NULL, NULL, NULL, NULL);
