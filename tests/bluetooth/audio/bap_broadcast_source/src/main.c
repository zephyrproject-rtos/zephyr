/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#include "bluetooth.h"
#include "bap_stream_expects.h"

DEFINE_FFF_GLOBALS;

static void mock_init_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	mock_bap_stream_init();
}

static void mock_destroy_rule_after(const struct ztest_unit_test *test, void *fixture)
{
	mock_bap_stream_cleanup();
}

ZTEST_RULE(mock_rule, mock_init_rule_before, mock_destroy_rule_after);

struct bap_broadcast_source_test_suite_fixture {
	struct bt_bap_broadcast_source_param *param;
	size_t stream_cnt;
	struct bt_bap_broadcast_source *source;
};

static void bap_broadcast_source_test_suite_fixture_init(
	struct bap_broadcast_source_test_suite_fixture *fixture)
{
	const uint8_t bis_cfg_data[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
				    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT),
	};
	const size_t streams_per_subgroup = CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT /
					    CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT;
	const enum bt_audio_context ctx = BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED;
	const enum bt_audio_location loc = BT_AUDIO_LOCATION_FRONT_LEFT;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_param;
	struct bt_bap_broadcast_source_stream_param *stream_params;
	struct bt_audio_codec_cfg *codec_cfg;
	struct bt_audio_codec_qos *codec_qos;
	struct bt_bap_stream *streams;
	const uint16_t latency = 10U; /* ms*/
	const uint32_t pd = 40000U;   /* us */
	const uint16_t sdu = 40U;     /* octets */
	const uint8_t rtn = 2U;
	uint8_t *bis_data;

	zassert_true(streams_per_subgroup > 0U);
	zassert_true(sizeof(bis_cfg_data) <= CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE);

	/* Allocate memory for everything */
	fixture->param = malloc(sizeof(struct bt_bap_broadcast_source_param));
	subgroup_param = malloc(sizeof(struct bt_bap_broadcast_source_subgroup_param) *
				CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT);
	zassert_not_null(subgroup_param);
	stream_params = malloc(sizeof(struct bt_bap_broadcast_source_stream_param) *
			       CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT);
	zassert_not_null(stream_params);
	codec_cfg = malloc(sizeof(struct bt_audio_codec_cfg));
	zassert_not_null(codec_cfg);
	codec_qos = malloc(sizeof(struct bt_audio_codec_qos));
	zassert_not_null(codec_qos);
	streams = malloc(sizeof(struct bt_bap_stream) * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT);
	zassert_not_null(streams);
	bis_data = malloc(CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE);
	zassert_not_null(bis_data);

	/* Memset everything to 0 */
	memset(fixture->param, 0, sizeof(*fixture->param));
	memset(subgroup_param, 0,
	       sizeof(struct bt_bap_broadcast_source_subgroup_param) *
		       CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT);
	memset(stream_params, 0,
	       sizeof(struct bt_bap_broadcast_source_stream_param) *
		       CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT);
	memset(codec_cfg, 0, sizeof(struct bt_audio_codec_cfg));
	memset(codec_qos, 0, sizeof(struct bt_audio_codec_qos));
	memset(streams, 0, sizeof(struct bt_bap_stream) * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT);
	memset(bis_data, 0, CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE);

	/* Initialize default values*/
	*codec_cfg = BT_AUDIO_CODEC_LC3_CONFIG_16_2(loc, ctx);
	*codec_qos = BT_AUDIO_CODEC_LC3_QOS_10_UNFRAMED(sdu, rtn, latency, pd);
	memcpy(bis_data, bis_cfg_data, sizeof(bis_cfg_data));

	for (size_t i = 0U; i < CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT; i++) {
		subgroup_param[i].params_count = streams_per_subgroup;
		subgroup_param[i].params = stream_params + i * streams_per_subgroup;
		subgroup_param[i].codec_cfg = codec_cfg;
	}

	for (size_t i = 0U; i < CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT; i++) {
		stream_params[i].stream = &streams[i];
		stream_params[i].data = bis_data;
		stream_params[i].data_len = sizeof(bis_cfg_data);
		bt_bap_stream_cb_register(stream_params[i].stream, &mock_bap_stream_ops);
	}

	fixture->param->params_count = CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT;
	fixture->param->params = subgroup_param;
	fixture->param->qos = codec_qos;
	fixture->param->encryption = false;
	memset(fixture->param->broadcast_code, 0, sizeof(fixture->param->broadcast_code));
	fixture->param->packing = BT_ISO_PACKING_SEQUENTIAL;

	fixture->stream_cnt = fixture->param->params_count * streams_per_subgroup;
}

static void *bap_broadcast_source_test_suite_setup(void)
{
	struct bap_broadcast_source_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void bap_broadcast_source_test_suite_before(void *f)
{
	memset(f, 0, sizeof(struct bap_broadcast_source_test_suite_fixture));
	bap_broadcast_source_test_suite_fixture_init(f);
}

static void bap_broadcast_source_test_suite_after(void *f)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = f;
	struct bt_bap_broadcast_source_param *param;

	if (fixture->source != NULL) {
		int err;

		(void)bt_bap_broadcast_source_stop(fixture->source);

		err = bt_bap_broadcast_source_delete(fixture->source);
		zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
		fixture->source = NULL;
	}

	param = fixture->param;

	free(param->params[0].params[0].data);
	free(param->params[0].params[0].stream);
	free(param->params[0].params);
	free(param->params[0].codec_cfg);
	free(param->params);
	free(param->qos);
	free(param);
}

static void bap_broadcast_source_test_suite_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(bap_broadcast_source_test_suite, NULL, bap_broadcast_source_test_suite_setup,
	    bap_broadcast_source_test_suite_before, bap_broadcast_source_test_suite_after,
	    bap_broadcast_source_test_suite_teardown);

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_create_delete)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	for (size_t i = 0u; i < create_param->params_count; i++) {
		for (size_t j = 0u; j < create_param->params[i].params_count; j++) {
			const struct bt_bap_stream *stream =
				create_param->params[i].params[j].stream;

			zassert_equal(create_param->qos->sdu, stream->qos->sdu,
				      "Unexpected stream SDU");
			zassert_equal(create_param->qos->rtn, stream->qos->rtn,
				      "Unexpected stream RTN");
			zassert_equal(create_param->qos->phy, stream->qos->phy,
				      "Unexpected stream PHY");
		}
	}

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_create_start_send_stop_delete)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_le_ext_adv ext_adv = {0};
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(fixture->source, &ext_adv);
	zassert_equal(0, err, "Unable to start broadcast source: err %d", err);

	zexpect_call_count("bt_bap_stream_ops.connected", fixture->stream_cnt,
			   mock_bap_stream_connected_cb_fake.call_count);
	zexpect_call_count("bt_bap_stream_ops.started", fixture->stream_cnt,
			   mock_bap_stream_started_cb_fake.call_count);

	for (size_t i = 0U; i < create_param->params_count; i++) {
		for (size_t j = 0U; j < create_param->params[i].params_count; j++) {
			struct bt_bap_stream *bap_stream = create_param->params[i].params[j].stream;

			/* Since BAP doesn't care about the `buf` we can just provide NULL */
			err = bt_bap_stream_send(bap_stream, NULL, 0, BT_ISO_TIMESTAMP_NONE);
			zassert_equal(0, err,
				      "Unable to send on broadcast stream[%zu][%zu]: err %d", i, j,
				      err);
		}
	}

	zexpect_call_count("bt_bap_stream_ops.sent", fixture->stream_cnt,
			   mock_bap_stream_sent_cb_fake.call_count);

	err = bt_bap_broadcast_source_stop(fixture->source);
	zassert_equal(0, err, "Unable to stop broadcast source: err %d", err);

	zexpect_call_count("bt_bap_stream_ops.disconnected", fixture->stream_cnt,
			   mock_bap_stream_disconnected_cb_fake.call_count);
	zexpect_call_count("bt_bap_stream_ops.stopped", fixture->stream_cnt,
			   mock_bap_stream_stopped_cb_fake.call_count);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_create_inval_param_null)
{
	int err;

	err = bt_bap_broadcast_source_create(NULL, &fixture->source);
	zassert_not_equal(0, err, "Did not fail with null params");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_create_inval_source_null)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	err = bt_bap_broadcast_source_create(create_param, NULL);
	zassert_not_equal(0, err, "Did not fail with null source");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_create_inval_subgroup_params_count_0)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	create_param->params_count = 0U;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_not_equal(0, err, "Did not fail with params_count %u", create_param->params_count);
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_create_inval_subgroup_params_count_above_max)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	create_param->params_count = CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT + 1;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_not_equal(0, err, "Did not fail with params_count %u", create_param->params_count);
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_create_inval_subgroup_params_null)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	int err;

	create_param->params = NULL;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	/* Restore the params for the cleanup after function */
	create_param->params = subgroup_params;
	zassert_not_equal(0, err, "Did not fail with NULL subgroup params");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_create_inval_qos_null)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_audio_codec_qos *qos = create_param->qos;
	int err;

	create_param->qos = NULL;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	/* Restore the params for the cleanup after function */
	create_param->qos = qos;
	zassert_not_equal(0, err, "Did not fail with NULL qos");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_create_inval_packing)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	create_param->packing = 0x02;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_not_equal(0, err, "Did not fail with packing %u", create_param->packing);
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_create_inval_subgroup_params_params_count_0)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	int err;

	subgroup_params->params_count = 0U;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_not_equal(0, err, "Did not fail with %u stream params",
			  subgroup_params->params_count);
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_create_inval_subgroup_params_params_count_above_max)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	int err;

	subgroup_params->params_count = CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT + 1;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_not_equal(0, err, "Did not fail with %u stream params",
			  subgroup_params->params_count);
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_create_inval_subgroup_params_stream_params_null)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	int err;

	subgroup_params->params = NULL;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	/* Restore the params for the cleanup after function */
	subgroup_params->params = stream_params;
	zassert_not_equal(0, err, "Did not fail with NULL stream params");
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_create_inval_subgroup_params_codec_cfg_null)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	subgroup_params->codec_cfg = NULL;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	/* Restore the params for the cleanup after function */
	subgroup_params->codec_cfg = codec_cfg;
	zassert_not_equal(0, err, "Did not fail with NULL codec_cfg");
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_create_inval_subgroup_params_codec_cfg_data_len)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	codec_cfg->data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_not_equal(0, err, "Did not fail with codec_cfg->data_len %zu", codec_cfg->data_len);
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_create_inval_subgroup_params_codec_cfg_meta_len)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	codec_cfg->meta_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE + 1;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_not_equal(0, err, "Did not fail with codec_cfg->meta_len %zu", codec_cfg->meta_len);
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_create_inval_subgroup_params_codec_cfg_cid)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	codec_cfg->id = BT_HCI_CODING_FORMAT_LC3;
	codec_cfg->cid = 0x01; /* Shall be 0 if id == 0x06 (LC3)*/
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_not_equal(0, err, "Did not fail with codec_cfg->cid %u", codec_cfg->cid);
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_create_inval_subgroup_params_codec_cfg_vid)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	codec_cfg->id = BT_HCI_CODING_FORMAT_LC3;
	codec_cfg->vid = 0x01; /* Shall be 0 if id == 0x06 (LC3)*/
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_not_equal(0, err, "Did not fail with codec_cfg->vid %u", codec_cfg->vid);
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_create_inval_stream_params_stream_null)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	struct bt_bap_stream *stream = stream_params->stream;
	int err;

	stream_params->stream = NULL;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	/* Restore the params for the cleanup after function */
	stream_params->stream = stream;
	zassert_not_equal(0, err, "Did not fail with NULL stream_params->stream");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_create_inval_stream_params_data_null)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	uint8_t *data = stream_params->data;
	int err;

	stream_params->data = NULL;
	stream_params->data_len = 1;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	/* Restore the params for the cleanup after function */
	stream_params->data = data;
	zassert_not_equal(
		0, err,
		"Did not fail with NULL stream_params->data and stream_params_>data_len %zu",
		stream_params->data_len);
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_create_inval_stream_params_data_len)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	int err;

	stream_params->data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_not_equal(0, err, "Did not fail with stream_params_>data_len %zu",
			  stream_params->data_len);
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_start_inval_source_null)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_le_ext_adv ext_adv = {0};
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(NULL, &ext_adv);
	zassert_not_equal(0, err, "Did not fail with null source");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_start_inval_ext_adv_null)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(fixture->source, NULL);
	zassert_not_equal(0, err, "Did not fail with null ext_adv");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_start_inval_double_start)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_le_ext_adv ext_adv = {0};
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(fixture->source, &ext_adv);
	zassert_equal(0, err, "Unable to start broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(fixture->source, &ext_adv);
	zassert_not_equal(0, err, "Did not fail with starting already started source");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_reconfigure_single_subgroup)
{
	struct bt_bap_broadcast_source_param *reconf_param = fixture->param;
	const size_t subgroup_cnt = reconf_param->params_count;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       reconf_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(reconf_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	for (size_t i = 0u; i < reconf_param->params_count; i++) {
		for (size_t j = 0u; j < reconf_param->params[i].params_count; j++) {
			const struct bt_bap_stream *stream =
				reconf_param->params[i].params[j].stream;

			zassert_equal(reconf_param->qos->sdu, stream->qos->sdu,
				      "Unexpected stream SDU");
			zassert_equal(reconf_param->qos->rtn, stream->qos->rtn,
				      "Unexpected stream RTN");
			zassert_equal(reconf_param->qos->phy, stream->qos->phy,
				      "Unexpected stream PHY");
		}
	}

	reconf_param->params_count = 1U;
	reconf_param->qos->sdu = 100U;
	reconf_param->qos->rtn = 3U;
	reconf_param->qos->phy = 1U;

	err = bt_bap_broadcast_source_reconfig(fixture->source, reconf_param);
	zassert_equal(0, err, "Unable to reconfigure broadcast source: err %d", err);

	for (size_t i = 0u; i < subgroup_cnt; i++) {
		for (size_t j = 0u; j < reconf_param->params[i].params_count; j++) {
			const struct bt_bap_stream *stream =
				reconf_param->params[i].params[j].stream;

			zassert_equal(reconf_param->qos->sdu, stream->qos->sdu,
				      "Unexpected stream SDU");
			zassert_equal(reconf_param->qos->rtn, stream->qos->rtn,
				      "Unexpected stream RTN");
			zassert_equal(reconf_param->qos->phy, stream->qos->phy,
				      "Unexpected stream PHY");
		}
	}

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_reconfigure_all)
{
	struct bt_bap_broadcast_source_param *reconf_param = fixture->param;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       reconf_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(reconf_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	for (size_t i = 0u; i < reconf_param->params_count; i++) {
		for (size_t j = 0u; j < reconf_param->params[i].params_count; j++) {
			const struct bt_bap_stream *stream =
				reconf_param->params[i].params[j].stream;

			zassert_equal(reconf_param->qos->sdu, stream->qos->sdu,
				      "Unexpected stream SDU");
			zassert_equal(reconf_param->qos->rtn, stream->qos->rtn,
				      "Unexpected stream RTN");
			zassert_equal(reconf_param->qos->phy, stream->qos->phy,
				      "Unexpected stream PHY");
		}
	}

	reconf_param->qos->sdu = 100U;
	reconf_param->qos->rtn = 3U;
	reconf_param->qos->phy = 1U;

	err = bt_bap_broadcast_source_reconfig(fixture->source, reconf_param);
	zassert_equal(0, err, "Unable to reconfigure broadcast source: err %d", err);

	for (size_t i = 0u; i < reconf_param->params_count; i++) {
		for (size_t j = 0u; j < reconf_param->params[i].params_count; j++) {
			const struct bt_bap_stream *stream =
				reconf_param->params[i].params[j].stream;

			zassert_equal(reconf_param->qos->sdu, stream->qos->sdu,
				      "Unexpected stream SDU");
			zassert_equal(reconf_param->qos->rtn, stream->qos->rtn,
				      "Unexpected stream RTN");
			zassert_equal(reconf_param->qos->phy, stream->qos->phy,
				      "Unexpected stream PHY");
		}
	}

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_reconfigure_inval_param_null)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_reconfig(fixture->source, NULL);
	zassert_not_equal(0, err, "Did not fail with null params");

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_reconfigure_inval_source_null)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_reconfig(NULL, param);
	zassert_not_equal(0, err, "Did not fail with null source");

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_subgroup_params_count_0)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	param->params_count = 0U;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	zassert_not_equal(0, err, "Did not fail with params_count %u", param->params_count);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_subgroup_params_count_above_max)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	param->params_count = CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT + 1;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	zassert_not_equal(0, err, "Did not fail with params_count %u", param->params_count);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_subgroup_params_null)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	param->params = NULL;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	/* Restore the params for the cleanup after function */
	param->params = subgroup_params;
	zassert_not_equal(0, err, "Did not fail with NULL subgroup params");

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_reconfigure_inval_qos_null)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_audio_codec_qos *qos = param->qos;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	param->qos = NULL;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	/* Restore the params for the cleanup after function */
	param->qos = qos;
	zassert_not_equal(0, err, "Did not fail with NULL qos");

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_reconfigure_inval_packing)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	param->packing = 0x02;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	zassert_not_equal(0, err, "Did not fail with packing %u", param->packing);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_subgroup_params_params_count_0)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	subgroup_params->params_count = 0U;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	zassert_not_equal(0, err, "Did not fail with %u stream params",
			  subgroup_params->params_count);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_subgroup_params_params_count_above_max)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	subgroup_params->params_count = CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT + 1;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	zassert_not_equal(0, err, "Did not fail with %u stream params",
			  subgroup_params->params_count);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_subgroup_params_stream_params_null)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	subgroup_params->params = NULL;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	/* Restore the params for the cleanup after function */
	subgroup_params->params = stream_params;
	zassert_not_equal(0, err, "Did not fail with NULL stream params");

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_null)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	subgroup_params->codec_cfg = NULL;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	/* Restore the params for the cleanup after function */
	subgroup_params->codec_cfg = codec_cfg;
	zassert_not_equal(0, err, "Did not fail with NULL codec_cfg");

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_data_len)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	codec_cfg->data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	zassert_not_equal(0, err, "Did not fail with codec_cfg->data_len %zu", codec_cfg->data_len);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_meta_len)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	codec_cfg->meta_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE + 1;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	zassert_not_equal(0, err, "Did not fail with codec_cfg->meta_len %zu", codec_cfg->meta_len);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_cid)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	codec_cfg->id = 0x06;
	codec_cfg->cid = 0x01; /* Shall be 0 if id == 0x06 (LC3)*/
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	zassert_not_equal(0, err, "Did not fail with codec_cfg->cid %u", codec_cfg->cid);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_vid)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	codec_cfg->id = 0x06;
	codec_cfg->vid = 0x01; /* Shall be 0 if id == 0x06 (LC3)*/
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	zassert_not_equal(0, err, "Did not fail with codec_cfg->vid %u", codec_cfg->vid);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_stream_params_stream_null)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	struct bt_bap_stream *stream = stream_params->stream;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	stream_params->stream = NULL;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	/* Restore the params for the cleanup after function */
	stream_params->stream = stream;
	zassert_not_equal(0, err, "Did not fail with NULL stream_params->stream");

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_stream_params_data_null)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	uint8_t *data = stream_params->data;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	stream_params->data = NULL;
	stream_params->data_len = 1;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	/* Restore the params for the cleanup after function */
	stream_params->data = data;
	zassert_not_equal(
		0, err,
		"Did not fail with NULL stream_params->data and stream_params_>data_len %zu",
		stream_params->data_len);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite,
	test_broadcast_source_reconfigure_inval_stream_params_data_len)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	stream_params->data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	zassert_not_equal(0, err, "Did not fail with stream_params_>data_len %zu",
			  stream_params->data_len);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_reconfigure_inval_state)
{
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source *source;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);
	source = fixture->source;

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;

	err = bt_bap_broadcast_source_reconfig(source, param);
	zassert_not_equal(0, err, "Did not fail with deleted broadcast source");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_stop_inval_source_null)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_le_ext_adv ext_adv = {0};
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(fixture->source, &ext_adv);
	zassert_equal(0, err, "Unable to start broadcast source: err %d", err);

	err = bt_bap_broadcast_source_stop(NULL);
	zassert_not_equal(0, err, "Did not fail with null source");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_stop_inval_state)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_le_ext_adv ext_adv = {0};
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(fixture->source, &ext_adv);
	zassert_equal(0, err, "Unable to start broadcast source: err %d", err);

	err = bt_bap_broadcast_source_stop(fixture->source);
	zassert_equal(0, err, "Unable to stop broadcast source: err %d", err);

	err = bt_bap_broadcast_source_stop(NULL);
	zassert_not_equal(0, err, "Did not fail with stopping already stopped source");
}


ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_delete_inval_source_null)
{
	int err;

	err = bt_bap_broadcast_source_delete(NULL);
	zassert_not_equal(0, err, "Did not fail with null source");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_delete_inval_double_start)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source *source;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to start broadcast source: err %d", err);

	source = fixture->source;
	/* Set to NULL to avoid deleting it in bap_broadcast_source_test_suite_after */
	fixture->source = NULL;

	err = bt_bap_broadcast_source_delete(source);
	zassert_not_equal(0, err, "Did not fail with deleting already deleting source");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_get_id)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	uint32_t broadcast_id;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_get_id(fixture->source, &broadcast_id);
	zassert_equal(0, err, "Unable to get broadcast ID: err %d", err);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_get_id_inval_source_null)
{
	uint32_t broadcast_id;
	int err;

	err = bt_bap_broadcast_source_get_id(NULL, &broadcast_id);
	zassert_not_equal(0, err, "Did not fail with null source");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_get_id_inval_id_null)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_get_id(fixture->source, NULL);
	zassert_not_equal(0, err, "Did not fail with null ID");

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_get_id_inval_state)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source *source;
	uint32_t broadcast_id;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	source = fixture->source;

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;

	err = bt_bap_broadcast_source_get_id(source, &broadcast_id);
	zassert_not_equal(0, err, "Did not fail with deleted broadcast source");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_get_base_single_bis)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	const uint8_t expected_base[] = {
		0x51, 0x18,                   /* uuid */
		0x40, 0x9C, 0x00,             /* pd */
		0x01,                         /* subgroup count */
		0x01,                         /* bis count */
		0x06, 0x00, 0x00, 0x00, 0x00, /* LC3 codec_id*/
		0x10,                         /* cc length */
		0x02, 0x01, 0x03, 0x02, 0x02, 0x01, 0x05, 0x03,
		0x01, 0x00, 0x00, 0x00, 0x03, 0x04, 0x28, 0x00, /* cc */
		0x04,                                           /* meta length */
		0x03, 0x02, 0x01, 0x00,                         /* meta */
		0x01,                                           /* bis index */
		0x03,                                           /* bis cc length */
		0x02, 0x03, 0x03                                /* bis cc length */
	};

	NET_BUF_SIMPLE_DEFINE(base_buf, 64);

	/* Make the create param simpler for verification */
	create_param->params_count = 1U;
	create_param->params[0].params_count = 1U;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_get_base(fixture->source, &base_buf);
	zassert_equal(0, err, "Unable to get broadcast source BASE: err %d", err);

	zassert_equal(sizeof(expected_base), base_buf.len, "Incorrect base_buf.len %u, expected %u",
		      base_buf.len, sizeof(expected_base));

	/* Use memcmp to print the buffers if they are not identical as zassert_mem_equal does not
	 * do that
	 */
	if (memcmp(expected_base, base_buf.data, base_buf.len) != 0) {
		for (size_t i = 0U; i < base_buf.len; i++) {
			printk("[%zu]: 0x%02X %s 0x%02X\n", i, expected_base[i],
			       expected_base[i] == base_buf.data[i] ? "==" : "!=",
			       base_buf.data[i]);
		}

		zassert_mem_equal(expected_base, base_buf.data, base_buf.len);
	}

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_get_base)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	const uint8_t expected_base[] = {
		0x51, 0x18,                   /* uuid */
		0x40, 0x9C, 0x00,             /* pd */
		0x02,                         /* subgroup count */
		0x01,                         /* Subgroup 1: bis count */
		0x06, 0x00, 0x00, 0x00, 0x00, /* LC3 codec_id*/
		0x10,                         /* cc length */
		0x02, 0x01, 0x03, 0x02, 0x02, 0x01, 0x05, 0x03,
		0x01, 0x00, 0x00, 0x00, 0x03, 0x04, 0x28, 0x00, /* cc */
		0x04,                                           /* meta length */
		0x03, 0x02, 0x01, 0x00,                         /* meta */
		0x01,                                           /* bis index */
		0x03,                                           /* bis cc length */
		0x02, 0x03, 0x03,                               /* bis cc length */
		0x01,                                           /* Subgroup 1: bis count */
		0x06, 0x00, 0x00, 0x00, 0x00,                   /* LC3 codec_id*/
		0x10,                                           /* cc length */
		0x02, 0x01, 0x03, 0x02, 0x02, 0x01, 0x05, 0x03,
		0x01, 0x00, 0x00, 0x00, 0x03, 0x04, 0x28, 0x00, /* cc */
		0x04,                                           /* meta length */
		0x03, 0x02, 0x01, 0x00,                         /* meta */
		0x02,                                           /* bis index */
		0x03,                                           /* bis cc length */
		0x02, 0x03, 0x03                                /* bis cc length */
	};

	NET_BUF_SIMPLE_DEFINE(base_buf, 128);

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_get_base(fixture->source, &base_buf);
	zassert_equal(0, err, "Unable to get broadcast source BASE: err %d", err);

	zassert_equal(sizeof(expected_base), base_buf.len, "Incorrect base_buf.len %u, expected %u",
		      base_buf.len, sizeof(expected_base));

	/* Use memcmp to print the buffers if they are not identical as zassert_mem_equal does not
	 * do that
	 */
	if (memcmp(expected_base, base_buf.data, base_buf.len) != 0) {
		for (size_t i = 0U; i < base_buf.len; i++) {
			printk("[%zu]: 0x%02X %s 0x%02X\n", i, expected_base[i],
			       expected_base[i] == base_buf.data[i] ? "==" : "!=",
			       base_buf.data[i]);
		}

		zassert_mem_equal(expected_base, base_buf.data, base_buf.len);
	}

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_get_base_inval_source_null)
{
	int err;

	NET_BUF_SIMPLE_DEFINE(base_buf, 64);

	err = bt_bap_broadcast_source_get_base(NULL, &base_buf);
	zassert_not_equal(0, err, "Did not fail with null source");
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_get_base_inval_base_buf_null)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_get_base(fixture->source, NULL);
	zassert_not_equal(0, err, "Did not fail with null BASE buffer");

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_get_base_inval_state)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source *source;
	int err;

	NET_BUF_SIMPLE_DEFINE(base_buf, 64);

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	source = fixture->source;

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;

	err = bt_bap_broadcast_source_get_base(source, &base_buf);
	zassert_not_equal(0, err, "Did not fail with deleted broadcast source");
}

/** This tests that providing a buffer too small for _any_ BASE fails correctly */
ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_get_base_inval_very_small_buf)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	NET_BUF_SIMPLE_DEFINE(base_buf, 15); /* Too small to hold any BASE */

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_get_base(fixture->source, &base_buf);
	zassert_not_equal(0, err, "Did not fail with too small base_buf (%u)", base_buf.size);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

/** This tests that providing a buffer too small for the BASE we want to setup fails correctly */
ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_get_base_inval_small_buf)
{
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	/* Can hold a base, but not large enough for this configuration */
	NET_BUF_SIMPLE_DEFINE(base_buf, 64);

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_get_base(fixture->source, &base_buf);
	zassert_not_equal(0, err, "Did not fail with too small base_buf (%u)", base_buf.size);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}
