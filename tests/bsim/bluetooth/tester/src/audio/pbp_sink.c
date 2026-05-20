/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

#include "babblekit/testcase.h"
#include "bstests.h"

#include "btp/btp.h"
#include "bsim_btp.h"

LOG_MODULE_REGISTER(bsim_pbp_sink, CONFIG_BSIM_BTTESTER_LOG_LEVEL);

static void test_pbp_sink(void)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t remote_addr;

	bsim_btp_uart_init();

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, NULL);

	bsim_btp_core_register(BTP_SERVICE_ID_GAP);
	bsim_btp_core_register(BTP_SERVICE_ID_PBP);
	bsim_btp_core_register(BTP_SERVICE_ID_BAP);
	bsim_btp_core_register(BTP_SERVICE_ID_CAS);
	bsim_btp_core_register(BTP_SERVICE_ID_PACS);

	bsim_btp_pbp_broadcast_scan_start();
	bsim_btp_wait_for_pbp_public_broadcast_announcement_found(&remote_addr);

	bt_addr_le_to_str(&remote_addr, addr_str, sizeof(addr_str));
	LOG_INF("Found remote device %s", addr_str);

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "pbp_sink",
		.test_descr = "Smoketest for the PBP broadcast sink BT Tester behavior",
		.test_main_f = test_pbp_sink,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_pbp_sink_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
