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

LOG_MODULE_REGISTER(bsim_cap_broadcast_source, CONFIG_BSIM_BTTESTER_LOG_LEVEL);

static void test_cap_broadcast_source(void)
{
	const uint8_t metadata[] = BT_AUDIO_CODEC_CFG_LC3_META(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	const uint8_t cc_data_16_2_1[] = BT_AUDIO_CODEC_CFG_LC3_DATA(
		BT_AUDIO_CODEC_CFG_FREQ_16KHZ, BT_AUDIO_CODEC_CFG_DURATION_10,
		BT_AUDIO_LOCATION_FRONT_LEFT, 40U, 1U);
	const uint8_t coding_format = BT_HCI_CODING_FORMAT_LC3;
	const uint8_t framing = BT_ISO_FRAMING_UNFRAMED;
	const uint32_t presentation_delay = 40000U;
	const uint32_t broadcast_id = 0x123456U;
	const uint32_t sdu_interval = 10000U;
	const uint16_t max_latency = 10U;
	const uint8_t subgroup_id = 0U;
	const uint16_t max_sdu = 40U;
	const uint8_t source_id = 0U;
	const uint16_t vid = 0x0000U; /* shall be 0x0000 for LC3 */
	const uint16_t cid = 0x0000U; /* shall be 0x0000 for LC3 */
	const uint8_t flags = 0U;
	const uint8_t rtn = 2U;

	bsim_btp_uart_init();

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY, NULL);

	bsim_btp_core_register(BTP_SERVICE_ID_GAP);
	bsim_btp_core_register(BTP_SERVICE_ID_BAP); /* required to start the TX thread */
	bsim_btp_core_register(BTP_SERVICE_ID_CAP);

	bsim_btp_cap_broadcast_source_setup_stream(source_id, subgroup_id, coding_format, vid, cid,
						   0, NULL, 0, NULL);
	bsim_btp_cap_broadcast_source_setup_subgroup(
		source_id, subgroup_id, coding_format, vid, cid, ARRAY_SIZE(cc_data_16_2_1),
		cc_data_16_2_1, ARRAY_SIZE(metadata), metadata);
	bsim_btp_cap_broadcast_source_setup(source_id, broadcast_id, sdu_interval, framing, max_sdu,
					    rtn, max_latency, presentation_delay, flags);
	bsim_btp_cap_broadcast_adv_start(source_id);
	bsim_btp_cap_broadcast_source_start(source_id);

	TEST_PASS("PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "cap_broadcast_source",
		.test_descr = "Smoketest for the CAP broadcast source BT Tester behavior",
		.test_main_f = test_cap_broadcast_source,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_cap_broadcast_source_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_sample);
}
