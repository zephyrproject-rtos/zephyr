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

LOG_MODULE_REGISTER(bsim_mcp_peripheral, CONFIG_BSIM_BTTESTER_LOG_LEVEL);

static void test_mcp_peripheral(void)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t remote_addr;

	bsim_btp_uart_init();

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, NULL);

	bsim_btp_core_register(BTP_SERVICE_ID_GAP);
	bsim_btp_core_register(BTP_SERVICE_ID_GMCS);

	bsim_btp_gap_set_discoverable(BTP_GAP_GENERAL_DISCOVERABLE);
	bsim_btp_gap_start_advertising(0U, 0U, NULL, BT_HCI_OWN_ADDR_PUBLIC);
	bsim_btp_wait_for_gap_device_connected(&remote_addr);
	bt_addr_le_to_str(&remote_addr, addr_str, sizeof(addr_str));
	LOG_INF("Device %s connected", addr_str);
	bsim_btp_wait_for_gap_device_disconnected(NULL);
	LOG_INF("Device %s disconnected", addr_str);

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "mcp_peripheral",
		.test_descr = "Smoketest for the MCP peripheral BT Tester behavior",
		.test_main_f = test_mcp_peripheral,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_mcp_peripheral_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
