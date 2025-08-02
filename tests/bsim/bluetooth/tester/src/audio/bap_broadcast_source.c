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

LOG_MODULE_REGISTER(bsim_bap_broadcast_source, CONFIG_BSIM_BTTESTER_LOG_LEVEL);

static void test_bap_broadcast_source(void)
{
	const uint8_t cc_data_16_2_1[] = BT_AUDIO_CODEC_CFG_LC3_DATA(
		BT_AUDIO_CODEC_CFG_FREQ_16KHZ, BT_AUDIO_CODEC_CFG_DURATION_10,
		BT_AUDIO_LOCATION_FRONT_LEFT, 40, 1);
	const uint8_t coding_format = BT_HCI_CODING_FORMAT_LC3;
	const uint8_t framing = BT_ISO_FRAMING_UNFRAMED;
	const uint32_t presentation_delay = 40000U;
	const uint32_t broadcast_id = 0x123456;
	const uint8_t streams_per_subgroup = 1U;
	const uint32_t sdu_interval = 10000U;
	const uint16_t max_latency = 10U;
	const uint16_t max_sdu = 40U;
	const uint8_t subgroups = 1U;
	const uint16_t cid = 0U; /* shall be 0 for LC3 */
	const uint16_t vid = 0U; /* shall be 0 for LC3 */
	const uint8_t rtn = 2U;

	bsim_btp_uart_init();

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, NULL);

	bsim_btp_core_register(BTP_SERVICE_ID_GAP);
	bsim_btp_core_register(BTP_SERVICE_ID_BAP);

	bsim_btp_bap_broadcast_source_setup_v2(broadcast_id, streams_per_subgroup, subgroups,
					       sdu_interval, framing, max_sdu, rtn, max_latency,
					       presentation_delay, coding_format, vid, cid,
					       ARRAY_SIZE(cc_data_16_2_1), cc_data_16_2_1);
	bsim_btp_bap_broadcast_adv_start(broadcast_id);
	bsim_btp_bap_broadcast_source_start(broadcast_id);

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "bap_broadcast_source",
		.test_descr = "Smoketest for the BAP broadcast source BT Tester behavior",
		.test_main_f = test_bap_broadcast_source,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_bap_broadcast_source_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
