/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

/*
 * This is just a demo of the test framework facilities
 */

extern enum bst_result_t bst_result;

static void test_empty_init(void)
{
	bst_ticker_set_next_tick_absolute(500e3);
	bst_result = In_progress;
}

static void test_empty_tick(bs_time_t HW_device_time)
{

	bst_result = Failed;
	bs_trace_error_line("test: empty demo test finished "
			    "(failed as it should be)\n");

}

static void test_empty_thread(void *p1, void *p2, void *p3)
{
	int i = 0;

	while (1) {
		printk("A silly demo thread. Iteration %i\n", i++);
		k_sleep(K_MSEC(100));
	}
}

static K_THREAD_STACK_DEFINE(stack_te,
			     CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
static struct k_thread test_thread_thread;

static void test_main(void)
{

	bs_trace_raw_time(3, "Empty test main called\n");

	k_thread_create(&test_thread_thread, stack_te,
			CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE,
			test_empty_thread, NULL, NULL, NULL,
			0, 0, K_NO_WAIT);
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "empty",
		.test_descr = "demo empty test (it just fails after 500ms)",
		.test_post_init_f = test_empty_init,
		.test_tick_f = test_empty_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_empty_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
