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

LOG_MODULE_REGISTER(bsim_vcp_central, CONFIG_BSIM_BTTESTER_LOG_LEVEL);

static void test_vcp_central(void)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t remote_addr;

	bsim_btp_uart_init();

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, NULL);

	bsim_btp_core_register(BTP_SERVICE_ID_GAP);
	bsim_btp_core_register(BTP_SERVICE_ID_VCP);
	bsim_btp_core_register(BTP_SERVICE_ID_VOCS);
	bsim_btp_core_register(BTP_SERVICE_ID_AICS);

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

	bsim_btp_vcp_discover(&remote_addr);
	bsim_btp_wait_for_vcp_discovered(NULL);

	const uint8_t new_vol = 123;
	uint8_t ev_vol;

	bsim_btp_vcp_ctlr_set_vol(&remote_addr, new_vol);
	bsim_btp_wait_for_vcp_state(NULL, &ev_vol);
	TEST_ASSERT(ev_vol == new_vol, "%u != %u", ev_vol, new_vol);

	const int16_t new_offset = -5;
	int16_t ev_offset;

	bsim_btp_vocs_state_set(&remote_addr, new_offset);
	bsim_btp_wait_for_vocs_state(NULL, &ev_offset);
	TEST_ASSERT(ev_offset == new_offset, "%d != %d", ev_offset, new_offset);

	const int8_t new_gain = 5;
	int8_t ev_gain;

	bsim_btp_aics_set_gain(&remote_addr, new_gain);
	bsim_btp_wait_for_aics_state(NULL, &ev_gain);
	TEST_ASSERT(ev_gain == new_gain, "%d != %d", ev_gain, new_gain);

	bsim_btp_gap_disconnect(&remote_addr);
	bsim_btp_wait_for_gap_device_disconnected(NULL);
	LOG_INF("Device %s disconnected", addr_str);

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "vcp_central",
		.test_descr = "Smoketest for the VCP central BT Tester behavior",
		.test_main_f = test_vcp_central,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_vcp_central_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
