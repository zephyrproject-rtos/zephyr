/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/types.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#define FAIL(...)					\
	do {						\
		bst_result = Failed;			\
		bs_trace_error_time_line(__VA_ARGS__);	\
	} while (0)

#define PASS(...)					\
	do {						\
		bst_result = Passed;			\
		bs_trace_info_time(1, __VA_ARGS__);	\
	} while (0)

extern enum bst_result_t bst_result;

/* TODO: The BIG/BIS host API is not yet merged, so forcibly use the controller
 * interface instead of the relying on HCI. Change to use host API once merged.
 */
#define USE_HOST_API 0

#if !IS_ENABLED(USE_HOST_API)
#include "ll.h"
#endif /* !USE_HOST_API */

static void test_iso_main(void)
{
	struct bt_le_ext_adv *adv;
	struct bt_le_adv_param param = { 0 };
	int err;
	int index;
	uint8_t bis_count = 1; /* TODO: Add support for multiple BIS per BIG */

	printk("\n*ISO broadcast test*\n");

	printk("Bluetooth initializing...\n");
	err = bt_enable(NULL);
	if (err) {
		FAIL("Could not init BT: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Create advertising set...\n");

	param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
	param.options |= BT_LE_ADV_OPT_EXT_ADV;
	err = bt_le_ext_adv_create(&param, NULL, &adv);
	if (err) {
		FAIL("Could not create advertising set: %d\n", err);
		return;
	}
	printk("success.\n");


	printk("Setting PA parameters...\n");
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		FAIL("Could not set PA set parameters: %d\n", err);
		return;
	}
	printk("success.\n");


#if !IS_ENABLED(USE_HOST_API)
	uint8_t big_handle = 0;
	uint32_t sdu_interval = 0x10000; /*us */
	uint16_t max_sdu = 0x10;
	uint16_t max_latency = 0x0A;
	uint8_t rtn = 0;
	uint8_t phy = 0;
	uint8_t packing = 0;
	uint8_t framing = 0;
	uint8_t encryption = 0;
	uint8_t bcode[16] = { 0 };

	/* Assume that index == handle */
	index = bt_le_ext_adv_get_index(adv);

	printk("Creating BIG...\n");
	err = ll_big_create(big_handle, index, bis_count, sdu_interval, max_sdu,
			    max_latency, rtn, phy, packing, framing, encryption,
			    bcode);
	if (err) {
		FAIL("Could not create BIG: %d\n", err);
		return;
	}
	printk("success.\n");

	k_sleep(K_MSEC(5000));

	printk("Terminating BIG...\n");
	err = ll_big_terminate(big_handle, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	if (err) {
		FAIL("Could not terminate BIG: %d\n", err);
		return;
	}
	printk("success.\n");

	k_sleep(K_MSEC(5000));
#endif /* !USE_HOST_API */


	PASS("Iso tests Passed\n");
	bs_trace_silent_exit(0);

	return;
}


static void test_iso_init(void)
{
	bst_result = In_progress;
}

static void test_iso_tick(bs_time_t HW_device_time)
{
	bst_result = Failed;
	bs_trace_error_line("Test finished.\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "broadcast",
		.test_descr = "ISO broadcast",
		.test_post_init_f = test_iso_init,
		.test_tick_f = test_iso_tick,
		.test_main_f = test_iso_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_iso_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_iso_install,
	NULL
};

void main(void)
{
	bst_main();
}
