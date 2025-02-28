/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/sys/util_macro.h>

#include "babblekit/testcase.h"
#include "bstests.h"

#include "btp.h"

static void test_gap_peripheral(void)
{
	uint8_t evt_buffer[BTP_MTU];

	btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, evt_buffer);

	btp_core_register(BTP_SERVICE_ID_GAP);
	btp_gap_set_discoverable(BTP_GAP_GENERAL_DISCOVERABLE);
	btp_gap_start_advertising(0U, 0U, NULL, 0xFFFFU, BT_HCI_OWN_ADDR_PUBLIC);

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "gap_peripheral",
		.test_descr = "Smoketest for the GAP central BT Tester behavior",
		.test_main_f = test_gap_peripheral,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gap_peripheral_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
