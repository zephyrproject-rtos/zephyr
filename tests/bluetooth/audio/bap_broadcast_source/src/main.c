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

	zassert_true(streams_per_subgroup > 0U);

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

	/* Initialize default values*/
	*codec_cfg = BT_AUDIO_CODEC_LC3_CONFIG_16_2(loc, ctx);
	*codec_qos = BT_AUDIO_CODEC_LC3_QOS_10_UNFRAMED(sdu, rtn, latency, pd);

	for (size_t i = 0U; i < CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT; i++) {
		subgroup_param[i].params_count = streams_per_subgroup;
		subgroup_param[i].params = stream_params + i * streams_per_subgroup;
		subgroup_param[i].codec_cfg = codec_cfg;
	}

	for (size_t i = 0U; i < CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT; i++) {
		stream_params[i].stream = &streams[i];
		stream_params[i].data = NULL;
		stream_params[i].data_len = 0U;
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

	if (fixture->source != NULL) {
		int err;

		(void)bt_bap_broadcast_source_stop(fixture->source);

		err = bt_bap_broadcast_source_delete(fixture->source);
		zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
		fixture->source = NULL;
	}

	free(fixture->param->params[0].params[0].stream);
	free(fixture->param->params[0].params);
	free(fixture->param->params[0].codec_cfg);
	free(fixture->param->params);
	free(fixture->param->qos);
	free(fixture->param);
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
	struct bt_bap_stream *stream = create_param->params[0].params[0].stream;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);

	zassert_equal(create_param->qos->sdu, stream->qos->sdu, "Unexpected stream SDU");
	zassert_equal(create_param->qos->rtn, stream->qos->rtn, "Unexpected stream RTN");
	zassert_equal(create_param->qos->phy, stream->qos->phy, "Unexpected stream PHY");

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

	zexpect_call_count("bt_bap_stream_ops.stopped", fixture->stream_cnt,
			   mock_bap_stream_stopped_cb_fake.call_count);

	err = bt_bap_broadcast_source_delete(fixture->source);
	zassert_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
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

ZTEST_F(bap_broadcast_source_test_suite, test_broadcast_source_reconfigure)
{
	struct bt_bap_broadcast_source_param *reconf_param = fixture->param;
	struct bt_bap_stream *stream = reconf_param->params[0].params[0].stream;
	int err;

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       reconf_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(reconf_param, &fixture->source);

	zassert_equal(reconf_param->qos->sdu, stream->qos->sdu, "Unexpected stream SDU");
	zassert_equal(reconf_param->qos->rtn, stream->qos->rtn, "Unexpected stream RTN");
	zassert_equal(reconf_param->qos->phy, stream->qos->phy, "Unexpected stream PHY");

	zassert_equal(0, err, "Unable to create broadcast source: err %d", err);
	reconf_param->qos->sdu = 100U;
	reconf_param->qos->rtn = 3U;
	reconf_param->qos->phy = 1U;

	err = bt_bap_broadcast_source_reconfig(fixture->source, reconf_param);
	zassert_equal(0, err, "Unable to reconfigure broadcast source: err %d", err);

	zassert_equal(reconf_param->qos->sdu, stream->qos->sdu, "Unexpected stream SDU");
	zassert_equal(reconf_param->qos->rtn, stream->qos->rtn, "Unexpected stream RTN");
	zassert_equal(reconf_param->qos->phy, stream->qos->phy, "Unexpected stream PHY");

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
