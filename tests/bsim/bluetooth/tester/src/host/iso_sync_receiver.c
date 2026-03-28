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

LOG_MODULE_REGISTER(bsim_iso_sync_receiver, CONFIG_BSIM_BTTESTER_LOG_LEVEL);

extern uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE];
NET_BUF_SIMPLE_DEFINE_STATIC(bis_stream_rx, BTP_MTU);

static void test_iso_sync_receiver(void)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	char ev_addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t remote_addr;
	bt_addr_le_t ev_addr;
	uint16_t sync_handle;
	uint16_t lost_sync_handle;
	uint8_t status;
	uint8_t sid;
	uint8_t num_bis;
	uint8_t encryption;
	uint8_t bis_id;
	uint8_t reason;
	struct btp_gap_bis_stream_received_ev *ev;

	bsim_btp_uart_init();

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, NULL);

	bsim_btp_core_register(BTP_SERVICE_ID_GAP);
	bsim_btp_gap_start_discovery(BTP_GAP_DISCOVERY_FLAG_LE);
	bsim_btp_wait_for_gap_device_found(&remote_addr);
	bt_addr_le_to_str(&remote_addr, addr_str, sizeof(addr_str));
	LOG_INF("Found remote device %s", addr_str);

	bsim_btp_gap_padv_create_sync(&remote_addr, 0, 0, 0x200, 0);
	bsim_btp_wait_for_gap_periodic_sync_established(&ev_addr, &sync_handle, &status);
	bt_addr_le_to_str(&ev_addr, ev_addr_str, sizeof(ev_addr_str));
	TEST_ASSERT(bt_addr_le_eq(&remote_addr, &ev_addr), "%s != %s", addr_str, ev_addr_str);
	TEST_ASSERT(status == 0, "Sync failed with status %u", status);
	LOG_INF("Device %s: periodic synced %u status %u", addr_str, sync_handle, status);

	bsim_btp_wait_for_gap_periodic_biginfo(&ev_addr, &sid, &num_bis, &encryption);
	bt_addr_le_to_str(&ev_addr, ev_addr_str, sizeof(ev_addr_str));
	TEST_ASSERT(bt_addr_le_eq(&remote_addr, &ev_addr), "%s != %s", addr_str, ev_addr_str);
	LOG_INF("Device %s: BIGinfo sid %u num_bis %u enc %u", addr_str, sid, num_bis, encryption);

	bsim_btp_gap_big_create_sync(&remote_addr, sid, num_bis, BIT(0), 0x00, 0xFF, encryption,
				     broadcast_code);

	bsim_btp_wait_for_gap_bis_data_path_setup(&ev_addr, &bis_id);
	bt_addr_le_to_str(&ev_addr, ev_addr_str, sizeof(ev_addr_str));
	TEST_ASSERT(bt_addr_le_eq(&remote_addr, &ev_addr), "%s != %s", addr_str, ev_addr_str);
	LOG_INF("Device %s: Data path of BIS %u is setup", addr_str, bis_id);

	do {
		net_buf_simple_reset(&bis_stream_rx);
		bsim_btp_wait_for_gap_bis_stream_received(&bis_stream_rx);
		TEST_ASSERT(bis_stream_rx.len >= sizeof(*ev));

		ev = net_buf_simple_pull_mem(&bis_stream_rx, sizeof(*ev));
		bt_addr_le_to_str(&ev->address, ev_addr_str, sizeof(ev_addr_str));
		TEST_ASSERT(bt_addr_le_eq(&ev->address, &ev_addr));
		TEST_ASSERT(ev->bis_id == bis_id, "Invalid BIS %u != %u", ev->bis_id, bis_id);
		LOG_INF("Device %s: BIS Stream RX BIS %u len %u flags %u TS %u seq_num %u",
			addr_str, ev->bis_id, ev->data_len, ev->flags, ev->ts, ev->seq_num);
		LOG_HEXDUMP_INF(bis_stream_rx.data, bis_stream_rx.len, "BIS Stream RX data: ");
	} while (ev->data_len < 1);

	bsim_btp_wait_for_gap_periodic_sync_lost(&lost_sync_handle, &reason);
	TEST_ASSERT(lost_sync_handle == sync_handle, "Sync lost handle mismatch %u != %u",
		    lost_sync_handle, sync_handle);
	LOG_INF("Device %s: Periodic sync lost (reason %u)", addr_str, reason);

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "iso_sync_receiver",
		.test_descr = "Smoketest for the GAP ISO Sync receiver BT Tester behavior",
		.test_main_f = test_iso_sync_receiver,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_iso_sync_receiver_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
