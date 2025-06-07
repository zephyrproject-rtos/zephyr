/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/audio.h>
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

LOG_MODULE_REGISTER(bsim_bap_central, CONFIG_BSIM_BTTESTER_LOG_LEVEL);

static void test_bap_central(void)
{
	uint8_t cc_data_16_2_1[] = BT_AUDIO_CODEC_CFG_LC3_DATA(BT_AUDIO_CODEC_CFG_FREQ_16KHZ,
							       BT_AUDIO_CODEC_CFG_DURATION_10,
							       BT_AUDIO_LOCATION_FRONT_LEFT, 40, 1);

	char addr_str[BT_ADDR_LE_STR_LEN];
	const uint8_t cig_id = 0U;
	const uint8_t cis_id = 0U;
	const uint32_t sdu_interval = 10000U;
	const uint8_t framing = BT_ISO_FRAMING_UNFRAMED;
	const uint16_t max_sdu = 40U;
	const uint8_t rtn = 2U;
	const uint16_t max_latency = 10U;
	const uint32_t presentation_delay = 40000U;
	bt_addr_le_t remote_addr;
	uint8_t ase_id;

	bsim_btp_uart_init();

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, NULL);

	bsim_btp_core_register(BTP_SERVICE_ID_GAP);
	bsim_btp_core_register(BTP_SERVICE_ID_BAP);
	bsim_btp_core_register(BTP_SERVICE_ID_ASCS);
	bsim_btp_core_register(BTP_SERVICE_ID_PACS);

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

	bsim_btp_ascs_configure_codec(&remote_addr, ase_id, BT_HCI_CODING_FORMAT_LC3, 0U, 0U,
				      ARRAY_SIZE(cc_data_16_2_1), cc_data_16_2_1);
	bsim_btp_wait_for_ascs_operation_complete();

	/* The CIS must be preconfigured before sending the request to the BAP unicast server */
	bsim_btp_ascs_add_ase_to_cis(&remote_addr, ase_id, cig_id, cis_id);
	btp_ascs_preconfigure_qos(cig_id, cis_id, sdu_interval, framing, max_sdu, rtn, max_latency,
				  presentation_delay);

	bsim_btp_ascs_configure_qos(&remote_addr, ase_id, cig_id, cis_id, sdu_interval, framing,
				    max_sdu, rtn, max_latency, presentation_delay);
	bsim_btp_wait_for_ascs_operation_complete();

	bsim_btp_ascs_enable(&remote_addr, ase_id);
	bsim_btp_wait_for_ascs_operation_complete();

	bsim_btp_ascs_receiver_start_ready(&remote_addr, ase_id);
	bsim_btp_wait_for_ascs_operation_complete();

	bsim_btp_ascs_release(&remote_addr, ase_id);
	bsim_btp_wait_for_ascs_operation_complete();

	bsim_btp_gap_disconnect(&remote_addr);
	bsim_btp_wait_for_gap_device_disconnected(NULL);
	LOG_INF("Device %s disconnected", addr_str);

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "bap_central",
		.test_descr = "Smoketest for the BAP central BT Tester behavior",
		.test_main_f = test_bap_central,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_bap_central_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
