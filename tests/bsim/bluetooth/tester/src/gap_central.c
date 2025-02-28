/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <zephyr/sys/util_macro.h>

#include "babblekit/testcase.h"
#include "bstests.h"

#include "btp.h"

static void test_gap_central(void)
{
	uint8_t evt_buffer[BTP_MTU];

	btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, evt_buffer);

	btp_core_register(BTP_SERVICE_ID_GAP);
	btp_gap_start_discovery(BTP_GAP_DISCOVERY_FLAG_LE);
	btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_DEVICE_FOUND, evt_buffer);

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "gap_central",
		.test_descr = "Smoketest for the GAP central BT Tester behavior",
		.test_main_f = test_gap_central,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gap_central_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
