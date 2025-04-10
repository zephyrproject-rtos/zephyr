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
#include <zephyr/bluetooth/iso.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

#include "babblekit/testcase.h"
#include "bstests.h"

#include "btp/btp.h"
#include "bsim_btp.h"

LOG_MODULE_REGISTER(bsim_cap_broadcast_sink, CONFIG_BSIM_BTTESTER_LOG_LEVEL);

static void test_cap_broadcast_sink(void)
{
	const uint16_t pa_sync_timeout = BT_GAP_PER_ADV_MAX_TIMEOUT;
	char addr_str[BT_ADDR_LE_STR_LEN];
	const uint16_t pa_sync_skip = 0U;
	const uint8_t src_id = 0U; /* BASS receive state source ID */
	bt_addr_le_t remote_addr;
	uint32_t broadcast_id;
	uint8_t adv_sid;
	uint8_t bis_id;

	bsim_btp_uart_init();

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, NULL);

	bsim_btp_core_register(BTP_SERVICE_ID_GAP);
	bsim_btp_core_register(BTP_SERVICE_ID_BAP);
	bsim_btp_core_register(BTP_SERVICE_ID_CAS);
	bsim_btp_core_register(BTP_SERVICE_ID_PACS);

	bsim_btp_bap_broadcast_sink_setup();

	bsim_btp_bap_broadcast_scan_start();
	bsim_btp_wait_for_bap_baa_found(&remote_addr, &broadcast_id, &adv_sid);
	bt_addr_le_to_str(&remote_addr, addr_str, sizeof(addr_str));
	LOG_INF("Found remote device %s", addr_str);
	bsim_btp_bap_broadcast_scan_stop();

	bsim_btp_bap_broadcast_sink_sync(&remote_addr, broadcast_id, adv_sid, pa_sync_skip,
					 pa_sync_timeout, false, src_id);
	bsim_btp_wait_for_bap_bap_bis_found(&bis_id);
	bsim_btp_bap_broadcast_sink_bis_sync(&remote_addr, broadcast_id,
					     BT_ISO_BIS_INDEX_BIT(bis_id));
	bsim_btp_wait_for_btp_bap_bis_synced();
	bsim_btp_wait_for_bap_bis_stream_received();
	bsim_btp_bap_broadcast_sink_stop(&remote_addr, broadcast_id);
	bsim_btp_bap_broadcast_sink_release();

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "cap_broadcast_sink",
		.test_descr = "Smoketest for the CAP Broadcast Sink BT Tester behavior",
		.test_main_f = test_cap_broadcast_sink,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_cap_broadcast_sink_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
