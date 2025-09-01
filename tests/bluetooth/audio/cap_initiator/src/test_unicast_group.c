/* test_unicast_group.c - unit test for unicast group functions */

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
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <sys/errno.h>

#include "bap_endpoint.h"
#include "test_common.h"
#include "ztest_assert.h"
#include "ztest_test.h"

struct cap_initiator_test_unicast_group_fixture {
	struct bt_cap_unicast_group_param *group_param;
	struct bt_cap_unicast_group *unicast_group;
	struct bt_bap_qos_cfg *qos_cfg;
};

static void *cap_initiator_test_unicast_group_setup(void)
{
	struct cap_initiator_test_unicast_group_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void cap_initiator_test_unicast_group_before(void *f)
{

	struct cap_initiator_test_unicast_group_fixture *fixture = f;
	struct bt_cap_unicast_group_stream_pair_param *pair_params;
	struct bt_cap_unicast_group_stream_param *stream_params;
	struct bt_cap_stream *cap_streams;
	size_t pair_cnt = 0U;
	size_t str_cnt = 0U;

	memset(f, 0, sizeof(struct cap_initiator_test_unicast_group_fixture));

	fixture->group_param = calloc(sizeof(struct bt_cap_unicast_group_param), 1);
	zassert_not_null(fixture->group_param);
	pair_params = calloc(sizeof(struct bt_cap_unicast_group_stream_pair_param),
			     DIV_ROUND_UP(CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT, 2U));
	zassert_not_null(pair_params);
	stream_params = calloc(sizeof(struct bt_cap_unicast_group_stream_param),
			       CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT);
	zassert_not_null(stream_params);
	cap_streams = calloc(sizeof(struct bt_cap_stream),
			     CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT);
	zassert_not_null(cap_streams);
	fixture->qos_cfg = calloc(sizeof(struct bt_bap_qos_cfg), 1);
	zassert_not_null(fixture->qos_cfg);

	*fixture->qos_cfg = BT_BAP_QOS_CFG_UNFRAMED(10000u, 40u, 2u, 10u, 40000u); /* 16_2_1 */

	while (str_cnt < CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT) {
		stream_params[str_cnt].stream = &cap_streams[str_cnt];
		stream_params[str_cnt].qos_cfg = fixture->qos_cfg;

		if (str_cnt & 1) {
			pair_params[pair_cnt].tx_param = &stream_params[str_cnt];
		} else {
			pair_params[pair_cnt].rx_param = &stream_params[str_cnt];
		}

		str_cnt++;
		pair_cnt = str_cnt / 2U;
	}

	fixture->group_param->packing = BT_ISO_PACKING_SEQUENTIAL;
	fixture->group_param->params_count = pair_cnt;
	fixture->group_param->params = pair_params;
}

static void cap_initiator_test_unicast_group_after(void *f)
{
	struct cap_initiator_test_unicast_group_fixture *fixture = f;
	struct bt_cap_unicast_group_param *group_param;

	/* In the case of a test failing, we delete the group so that subsequent tests won't fail */
	if (fixture->unicast_group != NULL) {
		bt_cap_unicast_group_delete(fixture->unicast_group);
	}

	group_param = fixture->group_param;

	free(group_param->params[0].rx_param->stream);
	free(group_param->params[0].rx_param);
	free(group_param->params);
	free(group_param);
	free(fixture->qos_cfg);
}

static void cap_initiator_test_unicast_group_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(cap_initiator_test_unicast_group, NULL, cap_initiator_test_unicast_group_setup,
	    cap_initiator_test_unicast_group_before, cap_initiator_test_unicast_group_after,
	    cap_initiator_test_unicast_group_teardown);

static ZTEST_F(cap_initiator_test_unicast_group, test_initiator_unicast_group_create)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_create_inval_null_param)
{
	int err = 0;

	err = bt_cap_unicast_group_create(NULL, &fixture->unicast_group);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_create_inval_null_rx_stream)
{
	int err = 0;

	if (fixture->group_param->params[0].rx_param->stream == NULL) {
		ztest_test_skip();
	}
	fixture->group_param->params[0].rx_param->stream = NULL;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_create_inval_null_tx_stream)
{
	int err = 0;

	if (fixture->group_param->params[0].tx_param->stream == NULL) {
		ztest_test_skip();
	}
	fixture->group_param->params[0].tx_param->stream = NULL;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_create_inval_too_many_streams)
{
	int err = 0;

	fixture->group_param->params_count = CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT + 1;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group, test_initiator_unicast_group_reconfig)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_reconfig(fixture->unicast_group, fixture->group_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_reconfig_inval_null_group)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_reconfig(NULL, fixture->group_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_reconfig_inval_null_param)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_reconfig(fixture->unicast_group, NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group, test_initiator_unicast_group_add_streams)
{
	struct bt_cap_stream stream = {};
	struct bt_cap_unicast_group_stream_param stream_param = {
		.stream = &stream,
		.qos_cfg = fixture->qos_cfg,
	};
	const struct bt_cap_unicast_group_stream_pair_param pair_param = {
		.rx_param = &stream_param,
	};
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_add_streams(fixture->unicast_group, &pair_param, 1);
	zassert_equal(err, 0, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_add_streams_inval_null_group)
{
	struct bt_cap_stream stream = {};
	struct bt_cap_unicast_group_stream_param stream_param = {
		.stream = &stream,
		.qos_cfg = fixture->qos_cfg,
	};
	const struct bt_cap_unicast_group_stream_pair_param pair_param = {
		.rx_param = &stream_param,
	};
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_add_streams(NULL, &pair_param, 1);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_add_streams_inval_null_param)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_add_streams(fixture->unicast_group, NULL, 1);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_add_streams_inval_0_param)
{
	struct bt_cap_stream stream = {};
	struct bt_cap_unicast_group_stream_param stream_param = {
		.stream = &stream,
		.qos_cfg = fixture->qos_cfg,
	};
	const struct bt_cap_unicast_group_stream_pair_param pair_param = {
		.rx_param = &stream_param,
	};
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_add_streams(fixture->unicast_group, &pair_param, 0);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group, test_initiator_unicast_group_delete)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_delete(fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	fixture->unicast_group = NULL;
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_delete_inval_null_group)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_delete(NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_delete_inval_double_delete)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_delete(fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_delete(fixture->unicast_group);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
	fixture->unicast_group = NULL;
}

static bool unicast_group_foreach_stream_cb(struct bt_cap_stream *cap_stream, void *user_data)
{
	size_t *cnt = user_data;

	(*cnt)++;

	return false;
}

static ZTEST_F(cap_initiator_test_unicast_group, test_initiator_unicast_group_foreach_stream)
{
	size_t expect_cnt = 0U;
	size_t cnt = 0U;
	int err;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_foreach_stream(fixture->unicast_group,
						  unicast_group_foreach_stream_cb, &cnt);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	for (size_t i = 0; i < fixture->group_param->params_count; i++) {
		if (fixture->group_param->params[i].rx_param != NULL) {
			expect_cnt++;
		}

		if (fixture->group_param->params[i].tx_param != NULL) {
			expect_cnt++;
		}
	}

	zassert_equal(cnt, expect_cnt, "Unexpected cnt (%zu != %zu)", cnt, expect_cnt);
}

static bool unicast_group_foreach_stream_return_early_cb(struct bt_cap_stream *stream,
							 void *user_data)
{
	size_t *cnt = user_data;

	(*cnt)++;

	return true;
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_foreach_stream_return_early)
{
	size_t cnt = 0U;
	int err;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_foreach_stream(
		fixture->unicast_group, unicast_group_foreach_stream_return_early_cb, &cnt);
	zassert_equal(err, -ECANCELED, "Unexpected return value: %d", err);
	zassert_equal(cnt, 1U, "Got %zu, expected %u", cnt, 1U);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_foreach_stream_inval_null_group)
{
	size_t expect_cnt = 0U;
	size_t cnt = 0U;
	int err;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_foreach_stream(NULL, unicast_group_foreach_stream_cb, &cnt);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zassert_equal(cnt, expect_cnt, "Unexpected cnt (%zu != %zu)", cnt, expect_cnt);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_foreach_stream_inval_null_func)
{
	size_t expect_cnt = 0U;
	size_t cnt = 0U;
	int err;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_foreach_stream(fixture->unicast_group, NULL, &cnt);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	zassert_equal(cnt, expect_cnt, "Unexpected cnt (%zu != %zu)", cnt, expect_cnt);
}

static ZTEST_F(cap_initiator_test_unicast_group, test_initiator_unicast_group_get_info)
{
	struct bt_cap_unicast_group_info cap_info;
	int err;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_get_info(fixture->unicast_group, &cap_info);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zassert_not_null(cap_info.unicast_group);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_get_info_inval_null_group)
{
	struct bt_cap_unicast_group_info cap_info;
	int err;

	err = bt_cap_unicast_group_get_info(NULL, &cap_info);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(cap_initiator_test_unicast_group,
	       test_initiator_unicast_group_get_info_inval_null_info)
{
	int err;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_cap_unicast_group_get_info(fixture->unicast_group, NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}
