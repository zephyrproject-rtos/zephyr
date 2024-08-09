/* test_unicast_start.c - unit test for unicast start procedure */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/fff.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <sys/errno.h>

#include "bap_endpoint.h"
#include "cap_initiator.h"
#include "conn.h"
#include "expects_util.h"
#include "test_common.h"
#include "ztest_assert.h"
#include "ztest_test.h"

struct cap_initiator_test_unicast_start_fixture {
	struct bt_cap_stream cap_streams[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_bap_ep eps[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_bap_unicast_group unicast_group;
	struct bt_conn conns[CONFIG_BT_MAX_CONN];
	struct bt_bap_lc3_preset preset;
};

static void cap_initiator_test_unicast_start_fixture_init(
	struct cap_initiator_test_unicast_start_fixture *fixture)
{
	fixture->preset = (struct bt_bap_lc3_preset)BT_BAP_LC3_UNICAST_PRESET_16_2_1(
		BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

	for (size_t i = 0U; i < ARRAY_SIZE(fixture->conns); i++) {
		test_conn_init(&fixture->conns[i]);
	}

	for (size_t i = 0U; i < ARRAY_SIZE(fixture->cap_streams); i++) {
		struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;

		sys_slist_append(&fixture->unicast_group.streams, &bap_stream->_node);
		bap_stream->group = &fixture->unicast_group;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(fixture->eps); i++) {
		fixture->eps[i].dir = (i & 1) + 1; /* Makes it either 1 or 2 (sink or source)*/
	}
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
	int err;

	memset(f, 0, sizeof(struct cap_initiator_test_unicast_start_fixture));
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
	struct bt_cap_unicast_audio_start_stream_param
		stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = ARRAY_SIZE(stream_params),
		.stream_params = stream_params,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &fixture->cap_streams[i];
		stream_params[i].codec_cfg = &fixture->preset.codec_cfg;
		/* Distribute the streams equally among the connections */
		stream_params[i].member.member = &fixture->conns[i % ARRAY_SIZE(fixture->conns)];
		stream_params[i].ep = &fixture->eps[i];
	}

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 1,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->status.state;

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
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT,
		.stream_params = NULL,
	};
	int err;

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_null_member)
{
	struct bt_cap_unicast_audio_start_stream_param
		stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = ARRAY_SIZE(stream_params),
		.stream_params = stream_params,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &fixture->cap_streams[i];
		stream_params[i].codec_cfg = &fixture->preset.codec_cfg;
		stream_params[i].ep = &fixture->eps[i];
	}

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start, test_initiator_unicast_start_inval_missing_cas)
{
	struct bt_cap_unicast_audio_start_stream_param
		stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_CSIP, /* CSIP requires CAS */
		.count = ARRAY_SIZE(stream_params),
		.stream_params = stream_params,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &fixture->cap_streams[i];
		stream_params[i].codec_cfg = &fixture->preset.codec_cfg;
		stream_params[i].member.member = &fixture->conns[i % ARRAY_SIZE(fixture->conns)];
		stream_params[i].ep = &fixture->eps[i];
	}

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_zero_count)
{
	struct bt_cap_unicast_audio_start_stream_param
		stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = 0U,
		.stream_params = stream_params,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &fixture->cap_streams[i];
		stream_params[i].codec_cfg = &fixture->preset.codec_cfg;
		stream_params[i].member.member = &fixture->conns[i % ARRAY_SIZE(fixture->conns)];
		stream_params[i].ep = &fixture->eps[i];
	}

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_inval_count)
{
	struct bt_cap_unicast_audio_start_stream_param
		stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT + 1U,
		.stream_params = stream_params,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &fixture->cap_streams[i];
		stream_params[i].codec_cfg = &fixture->preset.codec_cfg;
		stream_params[i].member.member = &fixture->conns[i % ARRAY_SIZE(fixture->conns)];
		stream_params[i].ep = &fixture->eps[i];
	}

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_inval_stream_param_null_stream)
{
	struct bt_cap_unicast_audio_start_stream_param stream_param = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = 1,
		.stream_params = &stream_param,
	};
	int err;

	stream_param.stream = NULL;
	stream_param.codec_cfg = &fixture->preset.codec_cfg;
	stream_param.member.member = &fixture->conns[0];
	stream_param.ep = &fixture->eps[0];

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_inval_stream_param_null_codec_cfg)
{
	struct bt_cap_unicast_audio_start_stream_param stream_param = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = 1,
		.stream_params = &stream_param,
	};
	int err;

	stream_param.stream = &fixture->cap_streams[0];
	stream_param.codec_cfg = NULL;
	stream_param.member.member = &fixture->conns[0];
	stream_param.ep = &fixture->eps[0];

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_inval_stream_param_null_member)
{
	struct bt_cap_unicast_audio_start_stream_param stream_param = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = 1,
		.stream_params = &stream_param,
	};
	int err;

	stream_param.stream = &fixture->cap_streams[0];
	stream_param.codec_cfg = &fixture->preset.codec_cfg;
	stream_param.member.member = NULL;
	stream_param.ep = &fixture->eps[0];

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_inval_stream_param_null_ep)
{
	struct bt_cap_unicast_audio_start_stream_param stream_param = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = 1,
		.stream_params = &stream_param,
	};
	int err;

	stream_param.stream = &fixture->cap_streams[0];
	stream_param.codec_cfg = &fixture->preset.codec_cfg;
	stream_param.member.member = &fixture->conns[0];
	stream_param.ep = NULL;

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_inval_param_inval_stream_param_invalid_meta)
{
	struct bt_cap_unicast_audio_start_stream_param stream_param = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = 1,
		.stream_params = &stream_param,
	};
	int err;

	stream_param.stream = &fixture->cap_streams[0];
	stream_param.codec_cfg = &fixture->preset.codec_cfg;
	stream_param.member.member = &fixture->conns[0];
	stream_param.ep = &fixture->eps[0];

	/* CAP requires stream context - Let's remove it */
	memset(stream_param.codec_cfg->meta, 0, sizeof(stream_param.codec_cfg->meta));
	stream_param.codec_cfg->meta_len = 0U;

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static ZTEST_F(cap_initiator_test_unicast_start,
	       test_initiator_unicast_start_state_codec_configured)
{
	struct bt_cap_unicast_audio_start_stream_param
		stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = ARRAY_SIZE(stream_params),
		.stream_params = stream_params,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &fixture->cap_streams[i];
		stream_params[i].codec_cfg = &fixture->preset.codec_cfg;
		stream_params[i].member.member = &fixture->conns[i % ARRAY_SIZE(fixture->conns)];
		stream_params[i].ep = &fixture->eps[i];

		test_unicast_set_state(stream_params[i].stream, stream_params[i].member.member,
				       stream_params[i].ep, &fixture->preset,
				       BT_BAP_EP_STATE_CODEC_CONFIGURED);
	}

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 1,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->status.state;

		zassert_equal(state, BT_BAP_EP_STATE_STREAMING,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}

static ZTEST_F(cap_initiator_test_unicast_start, test_initiator_unicast_start_state_qos_configured)
{
	struct bt_cap_unicast_audio_start_stream_param
		stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = ARRAY_SIZE(stream_params),
		.stream_params = stream_params,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &fixture->cap_streams[i];
		stream_params[i].codec_cfg = &fixture->preset.codec_cfg;
		stream_params[i].member.member = &fixture->conns[i % ARRAY_SIZE(fixture->conns)];
		stream_params[i].ep = &fixture->eps[i];

		test_unicast_set_state(stream_params[i].stream, stream_params[i].member.member,
				       stream_params[i].ep, &fixture->preset,
				       BT_BAP_EP_STATE_QOS_CONFIGURED);
	}

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 1,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->status.state;

		zassert_equal(state, BT_BAP_EP_STATE_STREAMING,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}

static ZTEST_F(cap_initiator_test_unicast_start, test_initiator_unicast_start_state_enabling)
{
	struct bt_cap_unicast_audio_start_stream_param
		stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = ARRAY_SIZE(stream_params),
		.stream_params = stream_params,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &fixture->cap_streams[i];
		stream_params[i].codec_cfg = &fixture->preset.codec_cfg;
		stream_params[i].member.member = &fixture->conns[i % ARRAY_SIZE(fixture->conns)];
		stream_params[i].ep = &fixture->eps[i];

		test_unicast_set_state(stream_params[i].stream, stream_params[i].member.member,
				       stream_params[i].ep, &fixture->preset,
				       BT_BAP_EP_STATE_ENABLING);
	}

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 1,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->status.state;

		zassert_equal(state, BT_BAP_EP_STATE_STREAMING,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}

static ZTEST_F(cap_initiator_test_unicast_start, test_initiator_unicast_start_state_streaming)
{
	struct bt_cap_unicast_audio_start_stream_param
		stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	const struct bt_cap_unicast_audio_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.count = ARRAY_SIZE(stream_params),
		.stream_params = stream_params,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &fixture->cap_streams[i];
		stream_params[i].codec_cfg = &fixture->preset.codec_cfg;
		stream_params[i].member.member = &fixture->conns[i % ARRAY_SIZE(fixture->conns)];
		stream_params[i].ep = &fixture->eps[i];

		test_unicast_set_state(stream_params[i].stream, stream_params[i].member.member,
				       stream_params[i].ep, &fixture->preset,
				       BT_BAP_EP_STATE_STREAMING);
	}

	err = bt_cap_initiator_unicast_audio_start(&param);
	zassert_equal(err, -EALREADY, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", 0,
			   mock_cap_initiator_unicast_start_complete_cb_fake.call_count);

	for (size_t i = 0U; i < ARRAY_SIZE(stream_params); i++) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;
		const enum bt_bap_ep_state state = bap_stream->ep->status.state;

		zassert_equal(state, BT_BAP_EP_STATE_STREAMING,
			      "[%zu]: Stream %p unexpected state: %d", i, bap_stream, state);
	}
}
