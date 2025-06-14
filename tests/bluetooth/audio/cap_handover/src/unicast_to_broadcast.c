/* main.c - Application main entry point */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "bap_endpoint.h"
#include "bap_iso.h"
#include "bluetooth.h"
#include "cap_initiator.h"
#include "conn.h"
#include "expects_util.h"
#include "test_common.h"
#include "ztest_assert.h"
#include "ztest_test.h"

static void mock_init_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	test_mocks_init();
}

static void mock_destroy_rule_after(const struct ztest_unit_test *test, void *fixture)
{
	test_mocks_cleanup();
}

ZTEST_RULE(mock_rule, mock_init_rule_before, mock_destroy_rule_after);

#define STREAMS_PER_DIRECTION 1
#define MAX_STREAMS           2
BUILD_ASSERT(CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT >= MAX_STREAMS);
BUILD_ASSERT(CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT >= STREAMS_PER_DIRECTION);

struct cap_handover_test_suite_fixture {
	struct bt_cap_unicast_group_stream_pair_param
		unicast_group_stream_pair_params[DIV_ROUND_UP(MAX_STREAMS, STREAMS_PER_DIRECTION)];
	struct bt_cap_unicast_audio_start_stream_param
		unicast_audio_start_stream_params[MAX_STREAMS];
	struct bt_cap_initiator_broadcast_stream_param broadcast_stream_params[MAX_STREAMS];
	struct bt_cap_unicast_group_stream_param unicast_group_stream_params[MAX_STREAMS];
	struct bt_cap_initiator_broadcast_create_param broadcast_create_param;
	struct bt_cap_unicast_to_broadcast_param unicast_to_broadcast_param;
	struct bt_cap_unicast_audio_start_param unicast_audio_start_param;
	struct bt_cap_initiator_broadcast_subgroup_param subgroup_params;
	struct bt_cap_unicast_group_param group_param;
	struct bt_cap_stream cap_streams[MAX_STREAMS];
	struct bt_cap_unicast_group *unicast_group;
	struct bt_conn conns[CONFIG_BT_MAX_CONN];
	struct bt_bap_ep eps[MAX_STREAMS];
	struct bt_bap_lc3_preset preset;
	struct bt_le_ext_adv ext_adv;
};

static void *cap_handover_test_suite_setup(void)
{
	struct cap_handover_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void cap_handover_test_suite_before(void *f)
{
	struct cap_handover_test_suite_fixture *fixture = f;
	size_t stream_cnt = 0U;
	size_t pair_cnt = 0U;
	int err;

	memset(fixture, 0, sizeof(*fixture));

	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	/* Create unicast group */
	fixture->preset = (struct bt_bap_lc3_preset)BT_BAP_LC3_UNICAST_PRESET_16_2_1(
		BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

	for (size_t i = 0U; i < ARRAY_SIZE(fixture->conns); i++) {
		test_conn_init(&fixture->conns[i]);
	}

	for (size_t i = 0U; i < ARRAY_SIZE(fixture->eps); i++) {
		if ((i & 1) == 0) {
			fixture->eps[i].dir = BT_AUDIO_DIR_SINK;
		} else {
			fixture->eps[i].dir = BT_AUDIO_DIR_SOURCE;
		}
	}

	while (stream_cnt < ARRAY_SIZE(fixture->unicast_group_stream_params)) {
		fixture->unicast_group_stream_params[stream_cnt].stream =
			&fixture->cap_streams[stream_cnt];
		fixture->unicast_group_stream_params[stream_cnt].qos_cfg = &fixture->preset.qos;

		/* Switch between sink and source depending on index*/
		if ((stream_cnt & 1) == 0) {
			fixture->unicast_group_stream_pair_params[pair_cnt].tx_param =
				&fixture->unicast_group_stream_params[stream_cnt];
		} else {
			fixture->unicast_group_stream_pair_params[pair_cnt].rx_param =
				&fixture->unicast_group_stream_params[stream_cnt];
		}

		stream_cnt++;
		pair_cnt = stream_cnt / 2U;
	}

	fixture->group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	fixture->group_param.params_count = pair_cnt;
	fixture->group_param.params = fixture->unicast_group_stream_pair_params;

	err = bt_cap_unicast_group_create(&fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	/* Start unicast group */
	fixture->unicast_audio_start_param.type = BT_CAP_SET_TYPE_AD_HOC;
	fixture->unicast_audio_start_param.count =
		ARRAY_SIZE(fixture->unicast_audio_start_stream_params);
	fixture->unicast_audio_start_param.stream_params =
		fixture->unicast_audio_start_stream_params;

	for (size_t i = 0U; i < ARRAY_SIZE(fixture->unicast_audio_start_stream_params); i++) {
		fixture->unicast_audio_start_stream_params[i].stream = &fixture->cap_streams[i];
		fixture->unicast_audio_start_stream_params[i].codec_cfg =
			&fixture->preset.codec_cfg;
		/* Distribute the streams equally among the connections */
		fixture->unicast_audio_start_stream_params[i].member.member =
			&fixture->conns[i % ARRAY_SIZE(fixture->conns)];
		fixture->unicast_audio_start_stream_params[i].ep = &fixture->eps[i];
	}

	err = bt_cap_initiator_unicast_audio_start(&fixture->unicast_audio_start_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 1,
			   mock_unicast_start_complete_cb_fake.call_count);
	zassert_equal(0, mock_unicast_start_complete_cb_fake.arg0_history[0]);
	zassert_equal_ptr(NULL, mock_unicast_start_complete_cb_fake.arg1_history[0]);

	/* Prepare default handover parameters including broadcast source create parameters */
	fixture->unicast_to_broadcast_param.type = BT_CAP_SET_TYPE_AD_HOC;
	fixture->unicast_to_broadcast_param.ext_adv = &fixture->ext_adv;
	fixture->unicast_to_broadcast_param.unicast_group = fixture->unicast_group;
	fixture->unicast_to_broadcast_param.sid = 0U;
	fixture->unicast_to_broadcast_param.pa_interval = 0x1234U;
	fixture->unicast_to_broadcast_param.broadcast_id = 0x123456U;
	fixture->unicast_to_broadcast_param.broadcast_create_param =
		&fixture->broadcast_create_param;

	fixture->broadcast_create_param.subgroup_count = 1U;
	fixture->broadcast_create_param.subgroup_params = &fixture->subgroup_params;
	fixture->broadcast_create_param.qos = &fixture->preset.qos;
	fixture->broadcast_create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	fixture->broadcast_create_param.encryption = false;

	/* We use pair_cnt as stream_count as it is equal to the number of sink streams */
	fixture->subgroup_params.stream_count = pair_cnt;
	fixture->subgroup_params.stream_params = fixture->broadcast_stream_params;
	fixture->subgroup_params.codec_cfg = &fixture->preset.codec_cfg;

	for (size_t i = 0U; i < pair_cnt; i++) {
		fixture->broadcast_stream_params[i].stream =
			fixture->unicast_group_stream_pair_params[i].tx_param->stream;
	}
}

static void cap_handover_test_suite_after(void *f)
{
	struct cap_handover_test_suite_fixture *fixture = f;

	bt_cap_initiator_unregister_cb(&mock_cap_initiator_cb);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		mock_bt_conn_disconnected(&fixture->conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

static void cap_handover_test_suite_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(cap_handover_test_suite, NULL, cap_handover_test_suite_setup,
	    cap_handover_test_suite_before, cap_handover_test_suite_after,
	    cap_handover_test_suite_teardown);

static void validate_handover_callback(void)
{
	zexpect_call_count("bt_cap_initiator_cb.unicast_to_broadcast_complete_cb", 1,
			   mock_unicast_to_broadcast_complete_cb_fake.call_count);
	zassert_equal(0, mock_unicast_to_broadcast_complete_cb_fake.arg0_history[0]);
	zassert_equal_ptr(NULL, mock_unicast_to_broadcast_complete_cb_fake.arg1_history[0]);
	zassert_equal_ptr(NULL, mock_unicast_to_broadcast_complete_cb_fake.arg2_history[0]);
	zassert_not_equal(NULL, mock_unicast_to_broadcast_complete_cb_fake.arg3_history[0]);
}

ZTEST_F(cap_handover_test_suite, test_handover_unicast_to_broadcast)
{
	int err = 0;

	err = bt_cap_initiator_unicast_to_broadcast(&fixture->unicast_to_broadcast_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	validate_handover_callback();
}
