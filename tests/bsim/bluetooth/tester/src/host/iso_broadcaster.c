/*
 * Copyright 2025 NXP
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

LOG_MODULE_REGISTER(bsim_iso_broadcaster, CONFIG_BSIM_BTTESTER_LOG_LEVEL);

uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
						      0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
						      0x0d, 0x0e, 0x0f, 0x10};

#define BIS_DATA_LEN 10
#define BIG_INTERVAL 10000 /* 10ms */

static void test_iso_broadcaster(void)
{
	uint8_t bis_id;
	uint32_t count = 100; /* 100 BIS data packets */
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t ev_addr;

	NET_BUF_SIMPLE_DEFINE(data, BIS_DATA_LEN);

	bsim_btp_uart_init();

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, NULL);

	bsim_btp_core_register(BTP_SERVICE_ID_GAP);

	bsim_btp_gap_set_connectable(false);
	bsim_btp_gap_set_extended_advertising(true);
	bsim_btp_gap_set_discoverable(BTP_GAP_GENERAL_DISCOVERABLE);
	bsim_btp_gap_start_advertising(0U, 0U, NULL, BT_HCI_OWN_ADDR_PUBLIC);
	bsim_btp_gap_padv_configure(BTP_GAP_PADV_INCLUDE_TX_POWER, 150, 200);
	bsim_btp_gap_padv_start();
	bsim_btp_gap_create_big(1, BIG_INTERVAL, 20, true, broadcast_code);

	bsim_btp_wait_for_gap_bis_data_path_setup(&ev_addr, &bis_id);
	bt_addr_le_to_str(&ev_addr, addr_str, sizeof(addr_str));
	LOG_INF("Device %s: Data path of BIS %u is setup", addr_str, bis_id);
	while (count > 0) {
		net_buf_simple_reset(&data);
		while (net_buf_simple_tailroom(&data) > 0) {
			net_buf_simple_add_u8(&data, (uint8_t)count);
		}
		bsim_btp_gap_bis_broadcast(bis_id, &data);

		k_sleep(K_USEC(BIG_INTERVAL));
		count--;
	}

	k_sleep(K_SECONDS(1));
	bsim_btp_gap_padv_stop();
	k_sleep(K_SECONDS(1));

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "iso_broadcaster",
		.test_descr = "Smoketest for the GAP ISO Broadcaster BT Tester behavior",
		.test_main_f = test_iso_broadcaster,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_iso_broadcaster_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
