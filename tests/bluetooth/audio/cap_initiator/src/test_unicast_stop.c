/* test_unicast_stop.c - unit test for unicast stop procedure */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
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
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#include <sys/errno.h>

#include "audio/bap_endpoint.h"
#include "cap_initiator.h"
#include "conn.h"
#include "expects_util.h"
#include "test_common.h"

BUILD_ASSERT(CONFIG_BT_MAX_CONN * (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +
				   CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT) >=
	     CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT);

/* Either BT_AUDIO_DIR_SINK or BT_AUDIO_DIR_SOURCE */
#define INDEX_TO_DIR(_idx) (((_idx) & 1U) + 1U)

struct cap_initiator_test_unicast_stop_fixture {
	struct bt_bap_ep *snk_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	struct bt_bap_ep *src_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
	struct bt_cap_stream *audio_stop_streams[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_cap_stream cap_streams[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_cap_unicast_audio_stop_param audio_stop_param;
	struct bt_cap_unicast_group *unicast_group;
	struct bt_conn conns[CONFIG_BT_MAX_CONN];
	struct bt_bap_lc3_preset preset;
};

static void cap_initiator_test_unicast_stop_fixture_init(
	struct cap_initiator_test_unicast_stop_fixture *fixture)
{
	struct bt_cap_unicast_group_stream_pair_param
		group_pair_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	struct bt_cap_unicast_group_stream_param
		group_stream_param[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	struct bt_cap_unicast_group_param group_param = {0};
	size_t stream_cnt = 0U;
	size_t pair_idx = 0U;
	int err;

	fixture->preset = (struct bt_bap_lc3_preset)BT_BAP_LC3_UNICAST_PRESET_16_2_1(
		BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

	for (size_t i = 0U; i < ARRAY_SIZE(fixture->conns); i++) {
		test_conn_init(&fixture->conns[i], i);
	}

	while (stream_cnt < ARRAY_SIZE(group_stream_param)) {
		const enum bt_audio_dir dir = INDEX_TO_DIR(stream_cnt);

		pair_idx = stream_cnt / 2U;

		group_stream_param[stream_cnt].stream = &fixture->cap_streams[stream_cnt];
		group_stream_param[stream_cnt].qos_cfg = &fixture->preset.qos;

		/* Switch between sink and source depending on index*/
		if (dir == BT_AUDIO_DIR_SINK) {
			group_pair_params[pair_idx].tx_param = &group_stream_param[stream_cnt];
		} else {
			group_pair_params[pair_idx].rx_param = &group_stream_param[stream_cnt];
		}

		stream_cnt++;
	}

	group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param.params_count = pair_idx + 1U;
	group_param.params = group_pair_params;

	err = bt_cap_unicast_group_create(&group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);
}

static struct bt_conn *get_conn_from_index(struct cap_initiator_test_unicast_stop_fixture *fixture,
					   size_t index)
{
	const size_t conn_index = (index / 2U) % ARRAY_SIZE(fixture->conns);

	return &fixture->conns[conn_index];
}

static struct bt_bap_ep *get_ep_from_index(struct cap_initiator_test_unicast_stop_fixture *fixture,
					   size_t index)
{
	const size_t conn_index = (index / 2U) % ARRAY_SIZE(fixture->conns);
	const size_t ep_index = index / (ARRAY_SIZE(fixture->conns) * 2U);
	const enum bt_audio_dir dir = INDEX_TO_DIR(index);
	struct bt_bap_ep *ep;

	/* Distribute the streams like
	 * [0]: conn[0] snk[0]
	 * [1]: conn[0] src[0]
	 * [2]: conn[1] snk[0]
	 * [3]: conn[1] src[0]
	 * [4]: conn[0] snk[1]
	 * [5]: conn[0] src[1]
	 * [6]: conn[1] snk[1]
	 * [7]: conn[1] src[1]
	 */
	if (dir == BT_AUDIO_DIR_SINK) {
		ep = fixture->snk_eps[conn_index][ep_index];
	} else {
		ep = fixture->src_eps[conn_index][ep_index];
	}

	return ep;
}

static void init_default_params(struct cap_initiator_test_unicast_stop_fixture *fixture)
{
	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		fixture->audio_stop_streams[i] = &fixture->cap_streams[i];
	}

	fixture->audio_stop_param.type = BT_CAP_SET_TYPE_AD_HOC;
	fixture->audio_stop_param.count = ARRAY_SIZE(fixture->cap_streams);
	fixture->audio_stop_param.streams = fixture->audio_stop_streams;
	fixture->audio_stop_param.release = false;
}

static void *cap_initiator_test_unicast_stop_setup(void)
{
	struct cap_initiator_test_unicast_stop_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void cap_initiator_test_unicast_stop_before(void *f)
{
	struct cap_initiator_test_unicast_stop_fixture *fixture = f;
	int err;

	memset(fixture, 0, sizeof(*fixture));
	cap_initiator_test_unicast_stop_fixture_init(f);

	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	mock_discover(fixture->conns, fixture->snk_eps, fixture->src_eps);
	init_default_params(fixture);
}

static void cap_initiator_test_unicast_stop_after(void *f)
{
	struct cap_initiator_test_unicast_stop_fixture *fixture = f;

	bt_cap_initiator_unregister_cb(&mock_cap_initiator_cb);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		mock_bt_conn_disconnected(&fixture->conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	/* In the case of a test failing, we cancel the procedure so that subsequent won't fail */
	bt_cap_initiator_unicast_audio_cancel();

	if (fixture->unicast_group != NULL) {
		const struct bt_cap_unicast_audio_stop_param param = {
			.type = BT_CAP_SET_TYPE_AD_HOC,
			.count = ARRAY_SIZE(fixture->cap_streams),
			.streams = fixture->audio_stop_streams,
			.release = true,
		};

		(void)bt_cap_initiator_unicast_audio_stop(&param);
		(void)bt_cap_unicast_group_delete(fixture->unicast_group);
	}
}

static void cap_initiator_test_unicast_stop_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(cap_initiator_test_unicast_stop, NULL, cap_initiator_test_unicast_stop_setup,
	    cap_initiator_test_unicast_stop_before, cap_initiator_test_unicast_stop_after,
	    cap_initiator_test_unicast_stop_teardown);

static ZTEST_F(cap_initiator_test_unicast_stop,
	       test_initiator_unicast_stop_disable_state_codec_configured)
{
	int err;

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		test_unicast_set_state(&fixture->cap_streams[i], get_conn_from_index(fixture, i),
				       get_ep_from_index(fixture, i), &fixture->preset,
				       BT_BAP_EP_STATE_CODEC_CONFIGURED);
	}

	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	zassert_equal(err, -EALREADY, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 0,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->state;

		zassert_equal(state, BT_BAP_EP_STATE_CODEC_CONFIGURED,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}

static ZTEST_F(cap_initiator_test_unicast_stop,
	       test_initiator_unicast_stop_disable_state_qos_configured)
{
	int err;

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		test_unicast_set_state(&fixture->cap_streams[i], get_conn_from_index(fixture, i),
				       get_ep_from_index(fixture, i), &fixture->preset,
				       BT_BAP_EP_STATE_QOS_CONFIGURED);
	}

	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	zassert_equal(err, -EALREADY, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 0,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->state;

		zassert_equal(state, BT_BAP_EP_STATE_QOS_CONFIGURED,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}

static ZTEST_F(cap_initiator_test_unicast_stop, test_initiator_unicast_stop_disable_state_enabling)
{
	int err;

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		test_unicast_set_state(&fixture->cap_streams[i], get_conn_from_index(fixture, i),
				       get_ep_from_index(fixture, i), &fixture->preset,
				       BT_BAP_EP_STATE_ENABLING);
	}

	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 1,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->state;

		zassert_equal(state, BT_BAP_EP_STATE_QOS_CONFIGURED,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}

static ZTEST_F(cap_initiator_test_unicast_stop, test_initiator_unicast_stop_disable_state_streaming)
{
	int err;

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		test_unicast_set_state(&fixture->cap_streams[i], get_conn_from_index(fixture, i),
				       get_ep_from_index(fixture, i), &fixture->preset,
				       BT_BAP_EP_STATE_STREAMING);
	}

	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 1,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->state;

		zassert_equal(state, BT_BAP_EP_STATE_QOS_CONFIGURED,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}

static ZTEST_F(cap_initiator_test_unicast_stop,
	       test_initiator_unicast_stop_release_state_codec_configured)
{
	int err;

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		test_unicast_set_state(&fixture->cap_streams[i], get_conn_from_index(fixture, i),
				       get_ep_from_index(fixture, i), &fixture->preset,
				       BT_BAP_EP_STATE_CODEC_CONFIGURED);
	}

	fixture->audio_stop_param.release = true;

	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 1,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;

		zassert_equal(bap_stream->ep, NULL, "[%zu]: Stream %p not released (%p)", i,
			      bap_stream, bap_stream->ep);
	}
}

static ZTEST_F(cap_initiator_test_unicast_stop,
	       test_initiator_unicast_stop_release_state_qos_configured)
{
	int err;

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		test_unicast_set_state(&fixture->cap_streams[i], get_conn_from_index(fixture, i),
				       get_ep_from_index(fixture, i), &fixture->preset,
				       BT_BAP_EP_STATE_QOS_CONFIGURED);
	}

	fixture->audio_stop_param.release = true;

	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 1,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;

		zassert_equal(bap_stream->ep, NULL, "[%zu]: Stream %p not released (%p)", i,
			      bap_stream, bap_stream->ep);
	}
}

static ZTEST_F(cap_initiator_test_unicast_stop, test_initiator_unicast_stop_release_state_enabling)
{
	int err;

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		test_unicast_set_state(&fixture->cap_streams[i], get_conn_from_index(fixture, i),
				       get_ep_from_index(fixture, i), &fixture->preset,
				       BT_BAP_EP_STATE_ENABLING);
	}

	fixture->audio_stop_param.release = true;

	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 1,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;

		zassert_equal(bap_stream->ep, NULL, "[%zu]: Stream %p not released (%p)", i,
			      bap_stream, bap_stream->ep);
	}
}

static ZTEST_F(cap_initiator_test_unicast_stop, test_initiator_unicast_stop_release_state_streaming)
{
	int err;

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		test_unicast_set_state(&fixture->cap_streams[i], get_conn_from_index(fixture, i),
				       get_ep_from_index(fixture, i), &fixture->preset,
				       BT_BAP_EP_STATE_STREAMING);
	}

	fixture->audio_stop_param.release = true;

	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 1,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);
	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;

		zassert_equal(bap_stream->ep, NULL, "[%zu]: Stream %p not released (%p)", i,
			      bap_stream, bap_stream->ep);
	}
}

static ZTEST_F(cap_initiator_test_unicast_stop, test_initiator_unicast_stop_inval_param_null)
{
	int err;

	err = bt_cap_initiator_unicast_audio_stop(NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 0,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_stop,
	       test_initiator_unicast_stop_inval_param_null_streams)
{
	int err;

	fixture->audio_stop_param.streams = NULL;

	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 0,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_stop, test_initiator_unicast_stop_inval_missing_cas)
{
	int err;

	fixture->audio_stop_param.type = BT_CAP_SET_TYPE_CSIP;

	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		test_unicast_set_state(&fixture->cap_streams[i], get_conn_from_index(fixture, i),
				       get_ep_from_index(fixture, i), &fixture->preset,
				       BT_BAP_EP_STATE_STREAMING);
	}

	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 0,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_stop, test_initiator_unicast_stop_inval_param_zero_count)
{
	int err;

	fixture->audio_stop_param.count = 0U;

	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 0,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_stop, test_initiator_unicast_stop_inval_param_inval_count)
{
	int err;

	fixture->audio_stop_param.count = CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT + 1U;

	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", 0,
			   mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);
}
