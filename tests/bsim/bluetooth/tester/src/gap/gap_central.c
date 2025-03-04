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

#include "bsim_btp.h"

LOG_MODULE_REGISTER(bsim_gap_central, CONFIG_BTTESTER_LOG_LEVEL);

static void test_gap_central(void)
{
	struct btp_gap_device_found_ev *device_found_ev;
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t remote_addr;
	struct net_buf *evt;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, NULL);

	bsim_btp_core_register(BTP_SERVICE_ID_GAP);
	bsim_btp_gap_start_discovery(BTP_GAP_DISCOVERY_FLAG_LE);
	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_DEVICE_FOUND, &evt);

	device_found_ev = net_buf_pull_mem(evt, sizeof(*device_found_ev));
	bt_addr_le_copy(&remote_addr, &device_found_ev->address);
	net_buf_unref(evt);
	bt_addr_le_to_str(&remote_addr, addr_str, sizeof(addr_str));
	LOG_INF("Found remote device %s", addr_str);

	bsim_btp_gap_stop_discovery();
	bsim_btp_gap_connect(&remote_addr, BTP_GAP_ADDR_TYPE_IDENTITY);
	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_DEVICE_CONNECTED, NULL);

	/* Keep alive for a little while */
	k_sleep(K_SECONDS(10));

	bsim_btp_gap_disconnect(&remote_addr);
	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_DEVICE_DISCONNECTED, NULL);

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
