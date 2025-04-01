/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util_macro.h>

#include "babblekit/testcase.h"
#include "bstests.h"

#include "btp/btp.h"
#include "bsim_btp.h"

LOG_MODULE_REGISTER(bsim_csip_central, CONFIG_BSIM_BTTESTER_LOG_LEVEL);

static void test_csip_central(void)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t remote_addr;

	bsim_btp_uart_init();

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, NULL);

	bsim_btp_core_register(BTP_SERVICE_ID_GAP);
	bsim_btp_core_register(BTP_SERVICE_ID_CSIP);

	bsim_btp_gap_start_discovery(BTP_GAP_DISCOVERY_FLAG_LE);
	bsim_btp_wait_for_gap_device_found(&remote_addr);
	bt_addr_le_to_str(&remote_addr, addr_str, sizeof(addr_str));
	LOG_INF("Found remote device %s", addr_str);

	bsim_btp_gap_stop_discovery();
	bsim_btp_gap_connect(&remote_addr, BTP_GAP_ADDR_TYPE_IDENTITY);
	bsim_btp_wait_for_gap_device_connected(NULL);
	LOG_INF("Device %s connected", addr_str);

	bsim_btp_gap_pair(&remote_addr);
	bsim_btp_wait_for_gap_sec_level_changed(NULL, NULL);

	bsim_btp_csip_discover(&remote_addr);
	bsim_btp_wait_for_csip_discovered(NULL);

	bsim_btp_csip_set_coordinator_lock();
	bsim_btp_wait_for_lock();

	bsim_btp_gap_disconnect(&remote_addr);
	bsim_btp_wait_for_gap_device_disconnected(NULL);
	LOG_INF("Device %s disconnected", addr_str);

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "csip_central",
		.test_descr = "Smoketest for the CSIP central BT Tester behavior",
		.test_main_f = test_csip_central,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_csip_central_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
