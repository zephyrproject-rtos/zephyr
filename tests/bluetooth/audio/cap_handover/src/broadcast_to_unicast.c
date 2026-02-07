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
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/fff.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#include "audio/bap_endpoint.h"
#include "audio/bap_iso.h"
#include "bluetooth.h"
#include "cap_initiator.h"
#include "cap_handover.h"
#include "conn.h"
#include "expects_util.h"
#include "test_common.h"

static void mock_init_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	test_mocks_init();
}

static void mock_destroy_rule_after(const struct ztest_unit_test *test, void *fixture)
{
	test_mocks_cleanup();
}

ZTEST_RULE(mock_rule, mock_init_rule_before, mock_destroy_rule_after);

#define MAX_STREAMS 2
BUILD_ASSERT(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT >= MAX_STREAMS);
BUILD_ASSERT(CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT >= MAX_STREAMS);
BUILD_ASSERT(CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT >= MAX_STREAMS);

struct cap_handover_broadcast_to_unicast_test_suite_fixture {
	struct bt_cap_unicast_audio_start_stream_param
		unicast_audio_start_stream_params[MAX_STREAMS];
	struct bt_cap_commander_broadcast_reception_stop_member_param
		stop_member_params[CONFIG_BT_MAX_CONN];
	struct bt_bap_ep *snk_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	struct bt_cap_unicast_group_stream_pair_param unicast_group_stream_pair_params[MAX_STREAMS];
	struct bt_cap_initiator_broadcast_stream_param broadcast_stream_params[MAX_STREAMS];
	struct bt_cap_unicast_group_stream_param unicast_group_stream_params[MAX_STREAMS];
	struct bt_cap_handover_broadcast_to_unicast_param broadcast_to_unicast_param;
	struct bt_cap_commander_broadcast_reception_stop_param reception_stop_param;
	struct bt_cap_initiator_broadcast_create_param broadcast_create_param;
	struct bt_cap_unicast_audio_start_param unicast_audio_start_param;
	struct bt_cap_initiator_broadcast_subgroup_param subgroup_params;
	struct bt_bap_lc3_preset unicast_presets[MAX_STREAMS];
	struct bt_cap_unicast_group_param unicast_group_param;
	struct bt_cap_broadcast_source *broadcast_source;
	struct bt_cap_stream cap_streams[MAX_STREAMS];
	struct bt_bap_lc3_preset broadcast_preset;
	struct bt_conn conns[CONFIG_BT_MAX_CONN];
	struct bt_le_ext_adv ext_adv;
};

static void *cap_handover_broadcast_to_unicast_test_suite_setup(void)
{
	struct cap_handover_broadcast_to_unicast_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void cap_handover_broadcast_to_unicast_test_suite_before(void *f)
{
	struct cap_handover_broadcast_to_unicast_test_suite_fixture *fixture = f;
	int err;

	memset(fixture, 0, sizeof(*fixture));

	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_handover_register_cb(&mock_cap_handover_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	ARRAY_FOR_EACH(fixture->conns, i) {
		test_conn_init(&fixture->conns[i], i);
		err = bt_bap_broadcast_assistant_discover(&fixture->conns[i]);
		zassert_equal(err, 0, "Unexpected return value %d", err);

		mock_unicast_client_discover(&fixture->conns[i], fixture->snk_eps[i], NULL);
	}

	/* Create advertising set */
	fixture->ext_adv.ext_adv_state = BT_LE_EXT_ADV_STATE_ENABLED;
	fixture->ext_adv.per_adv_state = BT_LE_PER_ADV_STATE_ENABLED;

	fixture->broadcast_preset = (struct bt_bap_lc3_preset)BT_BAP_LC3_BROADCAST_PRESET_16_2_1(
		BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

	fixture->broadcast_create_param.subgroup_count = 1U;
	fixture->broadcast_create_param.subgroup_params = &fixture->subgroup_params;
	fixture->broadcast_create_param.qos = &fixture->broadcast_preset.qos;
	fixture->broadcast_create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	fixture->broadcast_create_param.encryption = false;

	fixture->subgroup_params.stream_count = ARRAY_SIZE(fixture->cap_streams);
	fixture->subgroup_params.stream_params = fixture->broadcast_stream_params;
	fixture->subgroup_params.codec_cfg = &fixture->broadcast_preset.codec_cfg;

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		fixture->broadcast_stream_params[i].stream = &fixture->cap_streams[i];
	}

	/* Start broadcast source */
	err = bt_cap_initiator_broadcast_audio_create(&fixture->broadcast_create_param,
						      &fixture->broadcast_source);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	err = bt_cap_initiator_broadcast_audio_start(fixture->broadcast_source, &fixture->ext_adv);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	zexpect_call_count("bt_cap_initiator_cb.broadcast_start_cb", 1,
			   mock_broadcast_start_cb_fake.call_count);

	/* Prepare default handover parameters including unicast group create parameters */
	ARRAY_FOR_EACH(fixture->unicast_presets, i) {
		fixture->unicast_presets[i] =
			(struct bt_bap_lc3_preset)BT_BAP_LC3_UNICAST_PRESET_16_2_1(
				BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	}

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		struct bt_cap_unicast_audio_start_stream_param *start_stream_param =
			&fixture->unicast_audio_start_stream_params[i];
		struct bt_cap_unicast_group_stream_param *group_stream_param =
			&fixture->unicast_group_stream_params[i];
		const size_t conn_index = i % ARRAY_SIZE(fixture->conns);
		const size_t ep_index = i / ARRAY_SIZE(fixture->conns);

		start_stream_param->stream = &fixture->cap_streams[i];
		start_stream_param->codec_cfg = &fixture->unicast_presets[i].codec_cfg;

		/* Distribute the streams like
		 * [0]: conn[0] snk[0]
		 * [1]: conn[1] snk[0]
		 * [2]: conn[0] snk[1]
		 * [3]: conn[1] snk[1]
		 */
		start_stream_param->member.member = &fixture->conns[conn_index];
		start_stream_param->ep = fixture->snk_eps[conn_index][ep_index];
		group_stream_param->stream = &fixture->cap_streams[i];
		group_stream_param->qos_cfg = &fixture->unicast_presets[i].qos;

		fixture->unicast_group_stream_pair_params[i].tx_param = group_stream_param;
	}

	fixture->unicast_audio_start_param.type = BT_CAP_SET_TYPE_AD_HOC;
	fixture->unicast_audio_start_param.count = ARRAY_SIZE(fixture->cap_streams);
	fixture->unicast_audio_start_param.stream_params =
		fixture->unicast_audio_start_stream_params;

	fixture->unicast_group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	fixture->unicast_group_param.params_count = ARRAY_SIZE(fixture->cap_streams);
	fixture->unicast_group_param.params = fixture->unicast_group_stream_pair_params;

	fixture->broadcast_to_unicast_param.broadcast_id = TEST_COMMON_BROADCAST_ID;
	fixture->broadcast_to_unicast_param.adv_sid = TEST_COMMON_ADV_SID;
	fixture->broadcast_to_unicast_param.adv_type = TEST_COMMON_ADV_TYPE;
	fixture->broadcast_to_unicast_param.broadcast_source = fixture->broadcast_source;
	fixture->broadcast_to_unicast_param.unicast_group_param = &fixture->unicast_group_param;
	fixture->broadcast_to_unicast_param.unicast_start_param =
		&fixture->unicast_audio_start_param;

	/* Prepare reception start parameters */
	fixture->reception_stop_param.type = fixture->unicast_audio_start_param.type;
	fixture->reception_stop_param.param = fixture->stop_member_params;
	fixture->reception_stop_param.count = ARRAY_SIZE(fixture->stop_member_params);

	ARRAY_FOR_EACH(fixture->stop_member_params, i) {
		fixture->stop_member_params[i].member.member = &fixture->conns[i];
		fixture->stop_member_params[i].src_id = TEST_COMMON_SRC_ID;
		fixture->stop_member_params[i].num_subgroups =
			fixture->broadcast_create_param.subgroup_count;
	}
}

static void cap_handover_broadcast_to_unicast_test_suite_after(void *f)
{
	struct cap_handover_broadcast_to_unicast_test_suite_fixture *fixture = f;

	bt_cap_initiator_unregister_cb(&mock_cap_initiator_cb);
	bt_cap_handover_unregister_cb(&mock_cap_handover_cb);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		mock_bt_conn_disconnected(&fixture->conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	/* In the case of a test failing, we cancel the procedure so that subsequent won't fail */
	bt_cap_initiator_unicast_audio_cancel();

	/* In the case of a test failing, we delete the source so that subsequent tests won't fail
	 */
	if (fixture->broadcast_source != NULL) {
		(void)bt_cap_initiator_broadcast_audio_stop(fixture->broadcast_source);
		(void)bt_cap_initiator_broadcast_audio_delete(fixture->broadcast_source);
	}

	/* If a unicast group was created it exists as the 4th parameter in the callback */
	if (mock_broadcast_to_unicast_complete_cb_fake.arg3_history[0] != NULL) {
		struct bt_cap_unicast_group *unicast_group =
			mock_broadcast_to_unicast_complete_cb_fake.arg3_history[0];
		struct bt_cap_stream *cap_stream_ptrs[MAX_STREAMS];

		const struct bt_cap_unicast_audio_stop_param param = {
			.type = BT_CAP_SET_TYPE_AD_HOC,
			.count = ARRAY_SIZE(fixture->cap_streams),
			.streams = cap_stream_ptrs,
			.release = true,
		};

		ARRAY_FOR_EACH(cap_stream_ptrs, idx) {
			cap_stream_ptrs[idx] = &fixture->cap_streams[idx];
		}

		(void)bt_cap_initiator_unicast_audio_stop(&param);
		(void)bt_cap_unicast_group_delete(unicast_group);
	}
}

static void cap_handover_broadcast_to_unicast_test_suite_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(cap_handover_broadcast_to_unicast_test_suite, NULL,
	    cap_handover_broadcast_to_unicast_test_suite_setup,
	    cap_handover_broadcast_to_unicast_test_suite_before,
	    cap_handover_broadcast_to_unicast_test_suite_after,
	    cap_handover_broadcast_to_unicast_test_suite_teardown);

static void validate_handover_callback(void)
{
	zexpect_call_count("bt_cap_initiator_cb.broadcast_to_unicast_complete_cb", 1,
			   mock_broadcast_to_unicast_complete_cb_fake.call_count);
	zassert_equal(0, mock_broadcast_to_unicast_complete_cb_fake.arg0_history[0]);
	zassert_equal_ptr(NULL, mock_broadcast_to_unicast_complete_cb_fake.arg1_history[0]);
	zassert_equal_ptr(NULL, mock_broadcast_to_unicast_complete_cb_fake.arg2_history[0]);
	zassert_not_equal(NULL, mock_broadcast_to_unicast_complete_cb_fake.arg3_history[0]);
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite, test_handover_broadcast_to_unicast)
{
	int err;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	validate_handover_callback();
	fixture->broadcast_source = NULL;
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_reception_stop)
{
	int err;

	fixture->broadcast_to_unicast_param.reception_stop_param = &fixture->reception_stop_param;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	validate_handover_callback();
	fixture->broadcast_source = NULL;
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_null_param)
{
	int err;

	err = bt_cap_handover_broadcast_to_unicast(NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_reception_stop_param_type)
{
	int err;

	/* Mismatch between unicast_start_param and this */
	fixture->reception_stop_param.type = BT_CAP_SET_TYPE_CSIP;

	fixture->broadcast_to_unicast_param.reception_stop_param = &fixture->reception_stop_param;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_reception_stop_missing_conn)
{
	int err;

	if (fixture->reception_stop_param.count == 1) {
		ztest_test_skip();
	}

	fixture->reception_stop_param.count--;
	fixture->broadcast_to_unicast_param.reception_stop_param = &fixture->reception_stop_param;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_broadcast_id)
{
	int err;

	fixture->broadcast_to_unicast_param.broadcast_id = 0xFFFFFFFFU;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_adv_sid)
{
	int err;

	fixture->broadcast_to_unicast_param.adv_sid = 0xFFU;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_adv_type)
{
	int err;

	fixture->broadcast_to_unicast_param.adv_type = 0xFFU;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_null_broadcast_source)
{
	int err;

	fixture->broadcast_to_unicast_param.broadcast_source = NULL;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_null_unicast_group_param)
{
	int err;

	fixture->broadcast_to_unicast_param.unicast_group_param = NULL;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_null_unicast_start_param)
{
	int err;

	fixture->broadcast_to_unicast_param.unicast_start_param = NULL;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_unicast_stream)
{
	struct bt_cap_stream cap_stream = {0};
	int err;

	/* Attempt to use a stream not in the broadcast source */
	fixture->unicast_audio_start_stream_params[0].stream = &cap_stream;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_stream_state)
{
	int err;

	/* Attempt to use a stream not in the broadcast source */
	fixture->unicast_audio_start_stream_params[0].stream->bap_stream.ep->state =
		BT_BAP_EP_STATE_QOS_CONFIGURED;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_stream_group)
{
	int err;
	void *group = fixture->unicast_audio_start_stream_params[0].stream->bap_stream.group;

	/* Attempt to use a stream not in the broadcast source */
	fixture->unicast_audio_start_stream_params[0].stream->bap_stream.group =
		UINT_TO_POINTER(0x12345678);

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	/* Restore group to support proper cleanup after the test */
	fixture->unicast_audio_start_stream_params[0].stream->bap_stream.group = group;
}

static ZTEST_F(cap_handover_broadcast_to_unicast_test_suite,
	       test_handover_broadcast_to_unicast_inval_unicast_start_stream_cnt)
{
	int err;

	if (fixture->unicast_audio_start_param.count == 1) {
		ztest_test_skip();
	}

	/* Attempt to use a stream not in the broadcast source */
	fixture->unicast_audio_start_param.count -= 1;

	err = bt_cap_handover_broadcast_to_unicast(&fixture->broadcast_to_unicast_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}
