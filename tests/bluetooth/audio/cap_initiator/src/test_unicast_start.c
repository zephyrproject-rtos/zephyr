/* test_unicast_start.c - unit test for unicast start procedure */

/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/fff.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#include <sys/errno.h>

#include "audio/bap_endpoint.h"
#include "audio/bap_iso.h"
#include "cap_initiator.h"
#include "conn.h"
#include "expects_util.h"
#include "test_common.h"

struct cap_initiator_test_unicast_start_fixture {
	struct bt_cap_stream cap_streams[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_bap_ep eps[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_cap_unicast_audio_start_stream_param
		audio_start_stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_cap_unicast_audio_start_param audio_start_param;
	struct bt_cap_unicast_group *unicast_group;
	struct bt_conn conns[CONFIG_BT_MAX_CONN];
	struct bt_bap_lc3_preset preset;
};

static void cap_initiator_test_unicast_start_fixture_init(
	struct cap_initiator_test_unicast_start_fixture *fixture)
{
	struct bt_cap_unicast_group_stream_pair_param
		pair_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	struct bt_cap_unicast_group_stream_param
		stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	struct bt_cap_unicast_group_param group_param = {0};
	size_t stream_cnt = 0U;
	size_t pair_cnt = 0U;
	int err;

	fixture->preset = (struct bt_bap_lc3_preset)BT_BAP_LC3_UNICAST_PRESET_16_2_1(
		BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

	for (size_t i = 0U; i < ARRAY_SIZE(fixture->conns); i++) {
		test_conn_init(&fixture->conns[i]);
	}

	for (size_t i = 0U; i < ARRAY_SIZE(fixture->eps); i++) {
		const uint8_t dir = (i & 1) + 1; /* Makes it either 1 or 2 (sink or source)*/

		fixture->eps[i].dir = dir;
	}

	while (stream_cnt < ARRAY_SIZE(stream_params)) {
		stream_params[stream_cnt].stream = &fixture->cap_streams[stream_cnt];
		stream_params[stream_cnt].qos_cfg = &fixture->preset.qos;

		/* Switch between sink and source depending on index*/
		if ((stream_cnt & 1) == 0) {
			pair_params[pair_cnt].tx_param = &stream_params[stream_cnt];
		} else {
			pair_params[pair_cnt].rx_param = &stream_params[stream_cnt];
		}

		pair_cnt = DIV_ROUND_UP(stream_cnt, 2U);
		stream_cnt++;
	}

	group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param.params_count = pair_cnt;
	group_param.params = pair_params;

	err = bt_cap_unicast_group_create(&group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	/* Setup default params */
	ARRAY_FOR_EACH(fixture->audio_start_stream_params, i) {
		struct bt_cap_unicast_audio_start_stream_param *stream_param =
			&fixture->audio_start_stream_params[i];
		/* We pair 2 streams, so only increase conn_index every 2nd stream and otherwise
		 * round robin on all conns
		 */
		const size_t conn_index = (i / 2) % ARRAY_SIZE(fixture->conns);

		stream_param->stream = &fixture->cap_streams[i];
		stream_param->codec_cfg = &fixture->preset.codec_cfg;
		/* Distribute the streams equally among the connections */
		stream_param->member.member = &fixture->conns[conn_index];
		stream_param->ep = &fixture->eps[i];
	}

	fixture->audio_start_param.type = BT_CAP_SET_TYPE_AD_HOC;
	fixture->audio_start_param.count = CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT;
	fixture->audio_start_param.stream_params = fixture->audio_start_stream_params;
}

static void *cap_initiator_test_unicast_start_setup(void)
{
	struct cap_initiator_test_unicast_start_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void cap_initiator_test_unicast_start_before(void *f)
{
	struct cap_initiator_test_unicast_start_fixture *fixture = f;
	int err;

	(void)memset(fixture, 0, sizeof(*fixture));
	cap_initiator_test_unicast_start_fixture_init(f);

	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

static void cap_initiator_test_unicast_start_after(void *f)
{
	struct cap_initiator_test_unicast_start_fixture *fixture = f;

	bt_cap_initiator_unregister_cb(&mock_cap_initiator_cb);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		mock_bt_conn_disconnected(&fixture->conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	/* In the case of a test failing, we cancel the procedure so that subsequent won't fail */
	bt_cap_initiator_unicast_audio_cancel();

	if (fixture->unicast_group != NULL) {
		struct bt_cap_stream
			*cap_stream_ptrs[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
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
		(void)bt_cap_unicast_group_delete(fixture->unicast_group);
	}
}

static void cap_initiator_test_unicast_start_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(cap_initiator_test_unicast_start, NULL, cap_initiator_test_unicast_start_setup,
	    cap_initiator_test_unicast_start_before, cap_initiator_test_unicast_start_after,
	    cap_initiator_test_unicast_start_teardown);

static ZTEST_F(cap_initiator_test_unicast_start, test_initiator_unicast_start)
{
	int err;

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 1,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);

	zassert_equal(0, mock_cap_initiator_unicast_start_complete_cb_fake.arg0_history[0], "%d",
		      mock_cap_initiator_unicast_start_complete_cb_fake.arg0_history[0]);
	zassert_equal_ptr(NULL, mock_cap_initiator_unicast_start_complete_cb_fake.arg1_history[0],
			  "%p", mock_cap_initiator_unicast_start_complete_cb_fake.arg1_history[0]);

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->state;

		zassert_equal(state, BT_BAP_EP_STATE_STREAMING,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}

static ZTEST_F(cap_initiator_test_unicast_start, test_initiator_unicast_start_inval_param_null)
{
	int err;

	err = bt_cap_initiator_unicast_audio_start(NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_null_param)
{
	int err;

	fixture->audio_start_param.stream_params = NULL;

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_null_member)
{
	int err;

	fixture->audio_start_stream_params[0].member.member = NULL;

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start, test_initiator_unicast_start_inval_missing_cas)
{
	int err;

	fixture->audio_start_param.type = BT_CAP_SET_TYPE_CSIP; /* CSIP requires CAS */

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_zero_count)
{
	int err;

	fixture->audio_start_param.count = 0U;

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_inval_count)
{
	int err;

	fixture->audio_start_param.count = CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT + 1U;

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_inval_stream_param_null_stream)
{
	int err;

	fixture->audio_start_stream_params[0].stream = NULL;

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_inval_stream_param_null_codec_cfg)
{
	int err;

	fixture->audio_start_stream_params[0].codec_cfg = NULL;

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_inval_stream_param_null_member)
{
	int err;

	fixture->audio_start_stream_params[0].member.member = NULL;

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_inval_stream_param_null_ep)
{
	int err;

	fixture->audio_start_stream_params[0].ep = NULL;

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_inval_stream_param_invalid_meta)
{
	int err;

	/* CAP requires stream context - Let's remove it */
	(void)memset(fixture->audio_start_stream_params[0].codec_cfg->meta, 0,
		     sizeof(fixture->audio_start_stream_params[0].codec_cfg->meta));
	fixture->audio_start_stream_params[0].codec_cfg->meta_len = 0U;

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_state_codec_configured)
{
	int err;

	ARRAY_FOR_EACH_PTR(fixture->audio_start_stream_params, stream_param) {
		test_unicast_set_state(stream_param->stream, stream_param->member.member,
				       stream_param->ep, &fixture->preset,
				       BT_BAP_EP_STATE_CODEC_CONFIGURED);
	}

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 1,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
	zassert_equal(0, mock_cap_initiator_unicast_start_complete_cb_fake.arg0_history[0], "%d",
		      mock_cap_initiator_unicast_start_complete_cb_fake.arg0_history[0]);
	zassert_equal_ptr(NULL, mock_cap_initiator_unicast_start_complete_cb_fake.arg1_history[0],
			  "%p", mock_cap_initiator_unicast_start_complete_cb_fake.arg1_history[0]);

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->state;

		zassert_equal(state, BT_BAP_EP_STATE_STREAMING,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}

static ZTEST_F(cap_initiator_test_unicast_start, test_initiator_unicast_start_state_qos_configured)
{
	int err;

	ARRAY_FOR_EACH_PTR(fixture->audio_start_stream_params, stream_param) {
		test_unicast_set_state(stream_param->stream, stream_param->member.member,
				       stream_param->ep, &fixture->preset,
				       BT_BAP_EP_STATE_QOS_CONFIGURED);
	}

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 1,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
	zassert_equal(0, mock_cap_initiator_unicast_start_complete_cb_fake.arg0_history[0], "%d",
		      mock_cap_initiator_unicast_start_complete_cb_fake.arg0_history[0]);
	zassert_equal_ptr(NULL, mock_cap_initiator_unicast_start_complete_cb_fake.arg1_history[0],
			  "%p", mock_cap_initiator_unicast_start_complete_cb_fake.arg1_history[0]);

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->state;

		zassert_equal(state, BT_BAP_EP_STATE_STREAMING,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}

static ZTEST_F(cap_initiator_test_unicast_start, test_initiator_unicast_start_state_enabling)
{
	int err;

	ARRAY_FOR_EACH_PTR(fixture->audio_start_stream_params, stream_param) {
		test_unicast_set_state(stream_param->stream, stream_param->member.member,
				       stream_param->ep, &fixture->preset,
				       BT_BAP_EP_STATE_ENABLING);
	}

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 1,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
	zassert_equal(0, mock_cap_initiator_unicast_start_complete_cb_fake.arg0_history[0], "%d",
		      mock_cap_initiator_unicast_start_complete_cb_fake.arg0_history[0]);
	zassert_equal_ptr(NULL, mock_cap_initiator_unicast_start_complete_cb_fake.arg1_history[0],
			  "%p", mock_cap_initiator_unicast_start_complete_cb_fake.arg1_history[0]);

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->state;

		zassert_equal(state, BT_BAP_EP_STATE_STREAMING,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}

static ZTEST_F(cap_initiator_test_unicast_start, test_initiator_unicast_start_state_streaming)
{
	int err;

	ARRAY_FOR_EACH_PTR(fixture->audio_start_stream_params, stream_param) {
		test_unicast_set_state(stream_param->stream, stream_param->member.member,
				       stream_param->ep, &fixture->preset,
				       BT_BAP_EP_STATE_STREAMING);
	}

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	zassert_equal(err, -EALREADY, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->state;

		zassert_equal(state, BT_BAP_EP_STATE_STREAMING,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}
