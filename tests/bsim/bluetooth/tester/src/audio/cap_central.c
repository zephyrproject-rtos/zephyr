/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "babblekit/testcase.h"
#include "bstests.h"

#include "btp/btp.h"
#include "bsim_btp.h"

LOG_MODULE_REGISTER(bsim_cap_central, CONFIG_BSIM_BTTESTER_LOG_LEVEL);

static void test_cap_central(void)
{
	const uint8_t metadata[] = BT_AUDIO_CODEC_CFG_LC3_META(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	const uint8_t cc_data_16_2_1[] = BT_AUDIO_CODEC_CFG_LC3_DATA(
		BT_AUDIO_CODEC_CFG_FREQ_16KHZ, BT_AUDIO_CODEC_CFG_DURATION_10,
		BT_AUDIO_LOCATION_FRONT_LEFT, 40U, 1U);
	const enum bt_cap_set_type set_type = BT_CAP_SET_TYPE_AD_HOC;
	const uint8_t coding_format = BT_HCI_CODING_FORMAT_LC3;
	const uint8_t framing = BT_ISO_FRAMING_UNFRAMED;
	const uint32_t presentation_delay = 40000U;
	const uint32_t sdu_interval = 10000U;
	char addr_str[BT_ADDR_LE_STR_LEN];
	const uint16_t max_latency = 10U;
	const uint16_t max_sdu = 40U;
	const uint16_t vid = 0x0000; /* shall be 0x0000 for LC3 */
	const uint16_t cid = 0x0000; /* shall be 0x0000 for LC3 */
	const uint8_t cig_id = 0U;
	const uint8_t cis_id = 0U;
	bt_addr_le_t remote_addr;
	const uint8_t rtn = 2U;
	uint8_t ase_id;

	bsim_btp_uart_init();

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, NULL);

	bsim_btp_core_register(BTP_SERVICE_ID_GAP);
	bsim_btp_core_register(BTP_SERVICE_ID_BAP);
	bsim_btp_core_register(BTP_SERVICE_ID_CAP);

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

	bsim_btp_bap_discover(&remote_addr);
	bsim_btp_wait_for_bap_ase_found(&ase_id);
	bsim_btp_wait_for_bap_discovered();

	bsim_btp_cap_discover(&remote_addr);
	bsim_btp_wait_for_cap_discovered();

	bsim_btp_cap_unicast_setup_ase_cmd(&remote_addr, ase_id, cis_id, cig_id, coding_format, vid,
					   cid, sdu_interval, framing, max_sdu, rtn, max_latency,
					   presentation_delay, ARRAY_SIZE(cc_data_16_2_1),
					   cc_data_16_2_1, ARRAY_SIZE(metadata), metadata);

	bsim_btp_cap_unicast_audio_start(cig_id, (uint8_t)set_type);
	bsim_btp_wait_for_cap_unicast_start_completed();

	bsim_btp_gap_disconnect(&remote_addr);
	bsim_btp_wait_for_gap_device_disconnected(NULL);
	LOG_INF("Device %s disconnected", addr_str);

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "cap_central",
		.test_descr = "Smoketest for the CAP central BT Tester behavior",
		.test_main_f = test_cap_central,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_cap_central_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
