/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bs_tracing.h>
#include <bs_types.h>
#include <bstests.h>

#define BS_SECONDS(dur_sec)    ((bs_time_t)dur_sec * 1000000)
#define SIMULATED_TEST_TIMEOUT BS_SECONDS(60)

void the_test(void);
extern enum bst_result_t bst_result;

void test_init(void)
{
	bst_result = In_progress;
	bst_ticker_set_next_tick_absolute(SIMULATED_TEST_TIMEOUT);
}

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result == In_progress) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended. Consider increasing "
			       "simulation length.\n");
	}
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "the_test",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = the_test,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();

	return 0;
}
