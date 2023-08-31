/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include "common.h"

/* When BROADCAST_ENQUEUE_COUNT > 1 we can enqueue enough buffers to ensure that
 * the controller is never idle
 */
#define BROADCAST_ENQUEUE_COUNT 2U
#define TOTAL_BUF_NEEDED	(BROADCAST_ENQUEUE_COUNT * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT)

BUILD_ASSERT(CONFIG_BT_ISO_TX_BUF_COUNT >= TOTAL_BUF_NEEDED,
	     "CONFIG_BT_ISO_TX_BUF_COUNT should be at least "
	     "BROADCAST_ENQUEUE_COUNT * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT");

NET_BUF_POOL_FIXED_DEFINE(tx_pool,
			  TOTAL_BUF_NEEDED,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

extern enum bst_result_t bst_result;
static struct bt_bap_stream broadcast_source_streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
static struct bt_bap_stream *streams[ARRAY_SIZE(broadcast_source_streams)];
static struct bt_bap_lc3_preset preset_16_2_1 = BT_BAP_LC3_BROADCAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_bap_lc3_preset preset_16_2_2 = BT_BAP_LC3_BROADCAST_PRESET_16_2_2(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
CREATE_FLAG(flag_stopping);

static K_SEM_DEFINE(sem_started, 0U, ARRAY_SIZE(streams));
static K_SEM_DEFINE(sem_stopped, 0U, ARRAY_SIZE(streams));

static void started_cb(struct bt_bap_stream *stream)
{
	printk("Stream %p started\n", stream);
	k_sem_give(&sem_started);
}

static void stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);
	k_sem_give(&sem_stopped);
}

static void sent_cb(struct bt_bap_stream *stream)
{
	static uint8_t mock_data[CONFIG_BT_ISO_TX_MTU];
	static bool mock_data_initialized;
	static uint16_t seq_num;
	struct net_buf *buf;
	int ret;

	if (TEST_FLAG(flag_stopping)) {
		return;
	}

	if (!mock_data_initialized) {
		for (size_t i = 0U; i < ARRAY_SIZE(mock_data); i++) {
			/* Initialize mock data */
			mock_data[i] = (uint8_t)i;
		}
		mock_data_initialized = true;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		printk("Could not allocate buffer when sending on %p\n",
		       stream);
		return;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	/* Use preset_16_2_1 as that is the config we end up using */
	net_buf_add_mem(buf, mock_data, preset_16_2_1.qos.sdu);
	ret = bt_bap_stream_send(stream, buf, seq_num++,
				   BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		/* This will end broadcasting on this stream. */
		printk("Unable to broadcast data on %p: %d\n", stream, ret);
		net_buf_unref(buf);
		return;
	}
}

static struct bt_bap_stream_ops stream_ops = {
	.started = started_cb,
	.stopped = stopped_cb,
	.sent = sent_cb
};

static uint8_t valid_bis_codec_data[] = {BT_AUDIO_CODEC_DATA(
	BT_AUDIO_CODEC_CONFIG_LC3_FREQ, BT_BYTES_LIST_LE16(BT_AUDIO_CODEC_CONFIG_LC3_FREQ_16KHZ))};

static void broadcast_source_create_inval_reset_param(
	struct bt_bap_broadcast_source_create_param *param,
	struct bt_bap_broadcast_source_subgroup_param *subgroup_param,
	struct bt_bap_broadcast_source_stream_param *stream_param)
{
	struct bt_bap_broadcast_source_stream_param valid_stream_param;
	struct bt_bap_broadcast_source_subgroup_param valid_subgroup_param;
	struct bt_bap_broadcast_source_create_param create_param;

	valid_stream_param.stream = &broadcast_source_streams[0];
	valid_stream_param.data_len = ARRAY_SIZE(valid_bis_codec_data);
	valid_stream_param.data = valid_bis_codec_data;

	valid_subgroup_param.params_count = 1U;
	valid_subgroup_param.params = &valid_stream_param;
	valid_subgroup_param.codec_cfg = &preset_16_2_1.codec_cfg;

	create_param.params_count = 1U;
	create_param.params = &valid_subgroup_param;
	create_param.qos = &preset_16_2_1.qos;
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	create_param.encryption = false;

	memcpy(param, &create_param, sizeof(create_param));
	memcpy(subgroup_param, &valid_subgroup_param, sizeof(valid_subgroup_param));
	memcpy(stream_param, &valid_stream_param, sizeof(valid_stream_param));
	param->params = subgroup_param;
	subgroup_param->params = stream_param;
}

static void broadcast_source_create_inval_stream_param(void)
{
	struct bt_bap_broadcast_source_subgroup_param subgroup_param;
	struct bt_bap_broadcast_source_create_param create_param;
	struct bt_bap_broadcast_source_stream_param stream_param;
	struct bt_bap_broadcast_source *broadcast_source;
	int err;

	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);

	/* Set data NULL while count is 1 */
	stream_param.data = NULL;

	printk("Test bt_bap_broadcast_source_create with NULL stream_param\n");
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with NULL stream_param data did not "
		     "fail\n");
		return;
	}

	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);

	/* Initialize codec configuration data that is too large */
	stream_param.data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;

	printk("Test bt_bap_broadcast_source_create with stream_param.data_len %zu\n",
	       stream_param.data_len);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with stream_param data len %u "
		     "did not fail\n",
		     stream_param.data_len);
		return;
	}

	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);

	/* Set stream to NULL */
	stream_param.stream = NULL;

	printk("Test bt_bap_broadcast_source_create with NULL stream_param.stream\n");
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with NULL stream_param stream "
		     "did not fail\n");
		return;
	}

	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);

	if (CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE < 255) {
		uint8_t bis_codec_data[CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1] = {0};

		memcpy(bis_codec_data, valid_bis_codec_data, ARRAY_SIZE(valid_bis_codec_data));

		/* Set LTV data to invalid size */
		stream_param.data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;
		stream_param.data = bis_codec_data;

		printk("Test bt_bap_broadcast_source_create with CC LTV size %u\n",
		       stream_param.data_len);
		err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
		if (err == 0) {
			FAIL("bt_bap_broadcast_source_create with CC LTV size %u in stream_param "
			     "did not fail\n",
			     stream_param.data_len);
			return;
		}
	}
}

static void broadcast_source_create_inval_subgroup_codec_param(void)
{
	struct bt_bap_broadcast_source_subgroup_param subgroup_param;
	struct bt_bap_broadcast_source_create_param create_param;
	struct bt_bap_broadcast_source_stream_param stream_param;
	struct bt_bap_broadcast_source *broadcast_source;
	struct bt_audio_codec_cfg codec_cfg;
	int err;

	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);
	subgroup_param.codec_cfg =
		memcpy(&codec_cfg, &preset_16_2_1.codec_cfg, sizeof(preset_16_2_1.codec_cfg));

	codec_cfg.data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;

	printk("Test bt_bap_broadcast_source_create with codec.data_len %zu\n", codec_cfg.data_len);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with codec data len %zu did not fail\n",
		     codec_cfg.data_len);
		return;
	}

	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);
	subgroup_param.codec_cfg =
		memcpy(&codec_cfg, &preset_16_2_1.codec_cfg, sizeof(preset_16_2_1.codec_cfg));

	codec_cfg.meta_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE + 1;

	printk("Test bt_bap_broadcast_source_create with codec.meta_len %zu\n", codec_cfg.meta_len);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with codec meta len %zu did not fail\n",
		     codec_cfg.meta_len);
		return;
	}

	if (CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE < 255) {
		broadcast_source_create_inval_reset_param(&create_param, &subgroup_param,
							  &stream_param);
		subgroup_param.codec_cfg = memcpy(&codec_cfg, &preset_16_2_1.codec_cfg,
						  sizeof(preset_16_2_1.codec_cfg));

		/* Set LTV data to invalid size */
		codec_cfg.data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;

		printk("Test bt_bap_broadcast_source_create with CC LTV size %u\n",
		       codec_cfg.data_len);
		err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
		if (err == 0) {
			FAIL("bt_bap_broadcast_source_create with CC LTV size %zu in "
			     "subgroup_param did not fail\n",
			     codec_cfg.data_len);
			return;
		}
	}

	if (CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE < 255) {
		broadcast_source_create_inval_reset_param(&create_param, &subgroup_param,
							  &stream_param);
		subgroup_param.codec_cfg = memcpy(&codec_cfg, &preset_16_2_1.codec_cfg,
						  sizeof(preset_16_2_1.codec_cfg));

		/* Set LTV data to invalid size */
		codec_cfg.meta_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE + 1;

		printk("Test bt_bap_broadcast_source_create with Meta LTV size %u\n",
		       codec_cfg.meta_len);
		err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
		if (err == 0) {
			FAIL("bt_bap_broadcast_source_create with meta LTV size %zu in "
			     "subgroup_param did not fail\n",
			     codec_cfg.meta_len);
			return;
		}
	}
}

static void broadcast_source_create_inval_subgroup_param(void)
{
	struct bt_bap_broadcast_source_subgroup_param subgroup_param;
	struct bt_bap_broadcast_source_create_param create_param;
	struct bt_bap_broadcast_source_stream_param stream_param;
	struct bt_bap_broadcast_source *broadcast_source;
	int err;

	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);

	/* Set count to 0 */
	subgroup_param.params_count = 0;

	err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with 0 stream_param count did not fail\n");
		return;
	}

	/* Set count higher than max */
	subgroup_param.params_count = CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT + 1;

	err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with too high stream_param count did not "
		     "fail\n");
		return;
	}

	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);

	/* Set params to NULL */
	subgroup_param.params = NULL;

	err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with NULL stream_param did not fail\n");
		return;
	}

	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);

	/* Set codec to NULL */
	subgroup_param.codec_cfg = NULL;

	err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with NULL codec did not fail\n");
		return;
	}

	/* Invalid codec values */
	broadcast_source_create_inval_subgroup_codec_param();
}

static void broadcast_source_create_inval(void)
{
	struct bt_bap_broadcast_source_stream_param stream_param;
	struct bt_bap_broadcast_source_subgroup_param subgroup_param;
	struct bt_bap_broadcast_source_create_param create_param;
	struct bt_bap_broadcast_source *broadcast_sources[CONFIG_BT_BAP_BROADCAST_SRC_COUNT + 1U];
	struct bt_audio_codec_qos qos;
	int err;

	/* Test NULL parameters */
	printk("Test bt_bap_broadcast_source_create with NULL param\n");
	err = bt_bap_broadcast_source_create(NULL, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with NULL param did not fail\n");
		return;
	}

	printk("Test bt_bap_broadcast_source_create with NULL broadcast source\n");
	err = bt_bap_broadcast_source_create(&create_param, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with NULL broadcast source did not "
		     "fail\n");
		return;
	}

	/* Test stream_param values */
	broadcast_source_create_inval_stream_param();

	/* Test invalid subgroup_param values*/
	broadcast_source_create_inval_subgroup_param();

	/* Invalid create_param values */
	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);
	create_param.params_count = 0;

	printk("Test bt_bap_broadcast_source_create with 0 params_count\n");
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with 0 params_count did not fail\n");
		return;
	}

	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);
	create_param.params = NULL;

	printk("Test bt_bap_broadcast_source_create with NULL params\n");
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with NULL params did not fail\n");
		return;
	}

	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);
	create_param.packing = 0x35;

	printk("Test bt_bap_broadcast_source_create with packing 0x%02X\n", create_param.packing);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with invalid packing did not fail\n");
		return;
	}

	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);
	create_param.qos = NULL;

	printk("Test bt_bap_broadcast_source_create with NULL qos\n");
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with NULL qos did not fail\n");
		return;
	}

	/* Invalid QoS values */
	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);
	create_param.qos = memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));
	qos.phy = BT_AUDIO_CODEC_QOS_CODED + 1;

	printk("Test bt_bap_broadcast_source_create with qos.phy 0x%02X\n", qos.phy);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with invalid PHY did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.framing = BT_AUDIO_CODEC_QOS_FRAMING_FRAMED + 1;

	printk("Test bt_bap_broadcast_source_create with qos.framing 0x%02X\n", qos.framing);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with invalid framing did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.rtn = BT_ISO_BROADCAST_RTN_MAX + 1;

	printk("Test bt_bap_broadcast_source_create with qos.rtn 0x%02X\n", qos.rtn);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with invalid RTN did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.sdu = BT_ISO_MAX_SDU + 1;

	printk("Test bt_bap_broadcast_source_create with qos.sdu 0x%02X\n", qos.sdu);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with invalid SDU size did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.latency = BT_ISO_LATENCY_MIN - 1;

	printk("Test bt_bap_broadcast_source_create with qos.latency 0x%02X\n", qos.latency);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with too low latency did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.latency = BT_ISO_LATENCY_MAX + 1;

	printk("Test bt_bap_broadcast_source_create with qos.latency 0x%02X\n", qos.latency);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with too high latency did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.interval = BT_ISO_SDU_INTERVAL_MIN - 1;

	printk("Test bt_bap_broadcast_source_create with qos.interval 0x%02X\n", qos.interval);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with too low interval did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.interval = BT_ISO_SDU_INTERVAL_MAX + 1;

	printk("Test bt_bap_broadcast_source_create with qos.interval 0x%02X\n", qos.interval);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with too high interval did not fail\n");
		return;
	}

	/* Exceeding memory limits */
	broadcast_source_create_inval_reset_param(&create_param, &subgroup_param, &stream_param);

	printk("Test bt_bap_broadcast_source_create with %zu broadcast sources\n",
	       ARRAY_SIZE(broadcast_sources));
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_sources); i++) {
		err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[i]);

		if (i < CONFIG_BT_BAP_BROADCAST_SRC_COUNT) {
			if (err != 0) {
				FAIL("bt_bap_broadcast_source_create[%zu] failed: %d\n", i, err);
				return;
			}
		} else {
			if (err == 0) {
				FAIL("bt_bap_broadcast_source_create[%zu] did not fail\n", i);
				return;
			}
		}
	}

	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_sources) - 1; i++) {
		err = bt_bap_broadcast_source_delete(broadcast_sources[i]);
		if (err != 0) {
			FAIL("bt_bap_broadcast_source_delete[%zu] failed: %d\n", i, err);
			return;
		}
		broadcast_sources[i] = NULL;
	}

	create_param.params_count = CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT + 1;

	printk("Test bt_bap_broadcast_source_create with %zu subgroups\n",
	       create_param.params_count);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_sources[0]);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_create with %zu subgroups did not fail\n",
		     create_param.params_count);
		return;
	}
}

static int setup_broadcast_source(struct bt_bap_broadcast_source **source)
{
	uint8_t bis_codec_data[] = {3, BT_AUDIO_CODEC_CONFIG_LC3_FREQ,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CODEC_CONFIG_LC3_FREQ_16KHZ)};
	struct bt_bap_broadcast_source_stream_param
		stream_params[ARRAY_SIZE(broadcast_source_streams)];
	struct bt_bap_broadcast_source_subgroup_param
		subgroup_params[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
	struct bt_bap_broadcast_source_create_param create_param;
	int err;

	(void)memset(broadcast_source_streams, 0,
		     sizeof(broadcast_source_streams));

	for (size_t i = 0; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &broadcast_source_streams[i];
		bt_bap_stream_cb_register(stream_params[i].stream,
					    &stream_ops);
#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
		stream_params[i].data_len = ARRAY_SIZE(bis_codec_data);
		stream_params[i].data = bis_codec_data;
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */
	}

	for (size_t i = 0U; i < ARRAY_SIZE(subgroup_params); i++) {
		subgroup_params[i].params_count = 1U;
		subgroup_params[i].params = &stream_params[i];
		subgroup_params[i].codec_cfg = &preset_16_2_1.codec_cfg;
	}

	create_param.params_count = ARRAY_SIZE(subgroup_params);
	create_param.params = subgroup_params;
	create_param.qos = &preset_16_2_2.qos;
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	create_param.encryption = false;

	printk("Creating broadcast source with %zu subgroups and %zu streams\n",
	       ARRAY_SIZE(subgroup_params), ARRAY_SIZE(stream_params));
	err = bt_bap_broadcast_source_create(&create_param, source);
	if (err != 0) {
		printk("Unable to create broadcast source: %d\n", err);
		return err;
	}

	return 0;
}

static void test_broadcast_source_get_id_inval(struct bt_bap_broadcast_source *source,
					       uint32_t *broadcast_id_out)
{
	int err;

	printk("Test bt_bap_broadcast_source_get_id with NULL source\n");
	err = bt_bap_broadcast_source_get_id(NULL, broadcast_id_out);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_get_id with NULL source did not fail\n");
		return;
	}

	printk("Test bt_bap_broadcast_source_get_id with NULL broadcast_id\n");
	err = bt_bap_broadcast_source_get_id(source, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_get_id with NULL ID did not fail\n");
		return;
	}
}

static void test_broadcast_source_get_id(struct bt_bap_broadcast_source *source,
					 uint32_t *broadcast_id_out)
{
	int err;

	err = bt_bap_broadcast_source_get_id(source, broadcast_id_out);
	if (err != 0) {
		FAIL("Unable to get broadcast ID: %d\n", err);
		return;
	}
}

static void test_broadcast_source_get_base_inval(struct bt_bap_broadcast_source *source,
						 struct net_buf_simple *base_buf)
{
	/* Large enough for minimum, but not large enough for any CC or Meta data */
	NET_BUF_SIMPLE_DEFINE(small_base_buf, BT_BAP_BASE_MIN_SIZE + 2);
	NET_BUF_SIMPLE_DEFINE(very_small_base_buf, 4);
	int err;

	printk("Test bt_bap_broadcast_source_get_base with NULL source\n");
	err = bt_bap_broadcast_source_get_base(NULL, base_buf);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_get_base with NULL source did not fail\n");
		return;
	}

	printk("Test bt_bap_broadcast_source_get_base with NULL buf\n");
	err = bt_bap_broadcast_source_get_base(source, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_get_base with NULL buf did not fail\n");
		return;
	}

	printk("Test bt_bap_broadcast_source_get_base with very small buf\n");
	err = bt_bap_broadcast_source_get_base(source, &very_small_base_buf);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_get_base with very small buf did not fail\n");
		return;
	}

	printk("Test bt_bap_broadcast_source_get_base with small buf\n");
	err = bt_bap_broadcast_source_get_base(source, &small_base_buf);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_get_base with small buf did not fail\n");
		return;
	}
}

static void test_broadcast_source_get_base(struct bt_bap_broadcast_source *source,
					   struct net_buf_simple *base_buf)
{
	int err;

	err = bt_bap_broadcast_source_get_base(source, base_buf);
	if (err != 0) {
		FAIL("Failed to get encoded BASE: %d\n", err);
		return;
	}
}

static int setup_extended_adv(struct bt_bap_broadcast_source *source, struct bt_le_ext_adv **adv)
{
	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf,
			      BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);
	struct bt_data ext_ad;
	struct bt_data per_ad;
	uint32_t broadcast_id;
	int err;

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, adv);
	if (err != 0) {
		printk("Unable to create extended advertising set: %d\n", err);
		return err;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(*adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters: %d\n",
		       err);
		return err;
	}

	test_broadcast_source_get_id_inval(source, &broadcast_id);
	test_broadcast_source_get_id(source, &broadcast_id);

	/* Setup extended advertising data */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, broadcast_id);
	ext_ad.type = BT_DATA_SVC_DATA16;
	ext_ad.data_len = ad_buf.len;
	ext_ad.data = ad_buf.data;
	err = bt_le_ext_adv_set_data(*adv, &ext_ad, 1, NULL, 0);
	if (err != 0) {
		printk("Failed to set extended advertising data: %d\n", err);
		return err;
	}

	/* Setup periodic advertising data */
	test_broadcast_source_get_base_inval(source, &base_buf);
	test_broadcast_source_get_base(source, &base_buf);

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = bt_le_per_adv_set_data(*adv, &per_ad, 1);
	if (err != 0) {
		printk("Failed to set periodic advertising data: %d\n", err);
		return err;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(*adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising: %d\n", err);
		return err;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(*adv);
	if (err) {
		printk("Failed to enable periodic advertising: %d\n", err);
		return err;
	}

	return 0;
}

static void test_broadcast_source_reconfig_inval_state(struct bt_bap_broadcast_source *source)
{
	int err;

	printk("Test bt_bap_broadcast_source_reconfig in stopped state\n");
	err = bt_bap_broadcast_source_reconfig(source, &preset_16_2_1.codec_cfg,
					       &preset_16_2_1.qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig in stopped state did not fail\n");
		return;
	}
}

static void test_broadcast_source_reconfig_inval(struct bt_bap_broadcast_source *source)
{
	struct bt_audio_codec_qos qos;
	struct bt_audio_codec_cfg codec_cfg;
	int err;

	/* Test NULL values */
	printk("Test bt_bap_broadcast_source_reconfig with NULL source\n");
	err = bt_bap_broadcast_source_reconfig(NULL, &preset_16_2_1.codec_cfg, &preset_16_2_1.qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with NULL broadcast source did not fail\n");
		return;
	}

	printk("Test bt_bap_broadcast_source_reconfig with NULL codec\n");
	err = bt_bap_broadcast_source_reconfig(source, NULL, &preset_16_2_1.qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with NULL codec did not fail\n");
		return;
	}

	printk("Test bt_bap_broadcast_source_reconfig with NULL QoS\n");
	err = bt_bap_broadcast_source_reconfig(source, &preset_16_2_1.codec_cfg, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with NULL QoS did not fail\n");
		return;
	}

	/* Test invalid codec values */
	memcpy(&codec_cfg, &preset_16_2_1.codec_cfg, sizeof(preset_16_2_1.codec_cfg));

	codec_cfg.data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;

	printk("Test bt_bap_broadcast_source_reconfig with codec.data_len %zu\n",
	       codec_cfg.data_len);
	err = bt_bap_broadcast_source_reconfig(source, &codec_cfg, &preset_16_2_1.qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with too high codec data len did not "
		     "fail\n");
		return;
	}

	memcpy(&codec_cfg, &preset_16_2_1.codec_cfg, sizeof(preset_16_2_1.codec_cfg));

	codec_cfg.meta_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE + 1;

	printk("Test bt_bap_broadcast_source_reconfig with codec.meta_len %zu\n",
	       codec_cfg.meta_len);
	err = bt_bap_broadcast_source_reconfig(source, &codec_cfg, &preset_16_2_1.qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with too high codec meta len did not "
		     "fail\n");
		return;
	}

	memcpy(&codec_cfg, &preset_16_2_1.codec_cfg, sizeof(preset_16_2_1.codec_cfg));

	if (CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE < 255) {
		/* Set LTV data to invalid size */
		codec_cfg.data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;

		printk("Test bt_bap_broadcast_source_reconfig with CC LTV size %u\n",
		       codec_cfg.data_len);
		err = bt_bap_broadcast_source_reconfig(source, &codec_cfg, &preset_16_2_1.qos);
		if (err == 0) {
			FAIL("bt_bap_broadcast_source_reconfig with too large CC LTV did not "
			     "fail\n");
			return;
		}

		memcpy(&codec_cfg, &preset_16_2_1.codec_cfg, sizeof(preset_16_2_1.codec_cfg));
	}

	if (CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE < 255) {
		/* Set LTV data to invalid size */
		codec_cfg.meta_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE + 1;

		printk("Test bt_bap_broadcast_source_reconfig with meta LTV size %u\n",
		       codec_cfg.meta_len);
		err = bt_bap_broadcast_source_reconfig(source, &codec_cfg, &preset_16_2_1.qos);
		if (err == 0) {
			FAIL("bt_bap_broadcast_source_reconfig with too large meta LTV did not "
			     "fail\n");
			return;
		}

		memcpy(&codec_cfg, &preset_16_2_1.codec_cfg, sizeof(preset_16_2_1.codec_cfg));
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));
	qos.phy = BT_AUDIO_CODEC_QOS_CODED + 1;

	printk("Test bt_bap_broadcast_source_reconfig with qos.phy %u\n", qos.phy);
	err = bt_bap_broadcast_source_reconfig(source, &preset_16_2_1.codec_cfg, &qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with invalid PHY did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.framing = BT_AUDIO_CODEC_QOS_FRAMING_FRAMED + 1;

	printk("Test bt_bap_broadcast_source_reconfig with qos.framing %u\n", qos.framing);
	err = bt_bap_broadcast_source_reconfig(source, &preset_16_2_1.codec_cfg, &qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with invalid framing did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.rtn = BT_ISO_BROADCAST_RTN_MAX + 1;

	printk("Test bt_bap_broadcast_source_reconfig with qos.rtn %u\n", qos.rtn);
	err = bt_bap_broadcast_source_reconfig(source, &preset_16_2_1.codec_cfg, &qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with invalid RTN did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.sdu = BT_ISO_MAX_SDU + 1;

	printk("Test bt_bap_broadcast_source_reconfig with qos.sdu %u\n", qos.sdu);
	err = bt_bap_broadcast_source_reconfig(source, &preset_16_2_1.codec_cfg, &qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with invalid SDU size did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.latency = BT_ISO_LATENCY_MIN - 1;

	printk("Test bt_bap_broadcast_source_reconfig with qos.latency %u\n", qos.latency);
	err = bt_bap_broadcast_source_reconfig(source, &preset_16_2_1.codec_cfg, &qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with too low latency did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.latency = BT_ISO_LATENCY_MAX + 1;

	printk("Test bt_bap_broadcast_source_reconfig with qos.latency %u\n", qos.latency);
	err = bt_bap_broadcast_source_reconfig(source, &preset_16_2_1.codec_cfg, &qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with too high latency did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.interval = BT_ISO_SDU_INTERVAL_MIN - 1;

	printk("Test bt_bap_broadcast_source_reconfig with qos.interval %u\n", qos.interval);
	err = bt_bap_broadcast_source_reconfig(source, &preset_16_2_1.codec_cfg, &qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with too low interval did not fail\n");
		return;
	}

	memcpy(&qos, &preset_16_2_1.qos, sizeof(preset_16_2_1.qos));

	qos.interval = BT_ISO_SDU_INTERVAL_MAX + 1;

	printk("Test bt_bap_broadcast_source_reconfig with qos.interval %u\n", qos.interval);
	err = bt_bap_broadcast_source_reconfig(source, &preset_16_2_1.codec_cfg, &qos);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_reconfig with too high interval did not fail\n");
		return;
	}
}

static void test_broadcast_source_reconfig(struct bt_bap_broadcast_source *source)
{
	int err;

	printk("Reconfiguring broadcast source\n");
	err = bt_bap_broadcast_source_reconfig(source, &preset_16_2_1.codec_cfg,
					       &preset_16_2_1.qos);
	if (err != 0) {
		FAIL("Unable to reconfigure broadcast source: %d\n", err);
		return;
	}
}

static void test_broadcast_source_start_inval_state(struct bt_bap_broadcast_source *source,
						    struct bt_le_ext_adv *adv)
{
	int err;

	printk("Test bt_bap_broadcast_source_start in streaming state\n");
	err = bt_bap_broadcast_source_start(source, adv);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_start in streaming state did not fail\n");
		return;
	}
}

static void test_broadcast_source_start_inval(struct bt_bap_broadcast_source *source,
					      struct bt_le_ext_adv *adv)
{
	int err;

	printk("Test bt_bap_broadcast_source_start with NULL source\n");
	err = bt_bap_broadcast_source_start(NULL, adv);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_start with NULL source did not fail\n");
		return;
	}

	printk("Test bt_bap_broadcast_source_start with NULL adv\n");
	err = bt_bap_broadcast_source_start(source, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_start with NULL adv did not fail\n");
		return;
	}
}

static void test_broadcast_source_start(struct bt_bap_broadcast_source *source,
					struct bt_le_ext_adv *adv)
{
	int err;

	printk("Starting broadcast source\n");
	err = bt_bap_broadcast_source_start(source, adv);
	if (err != 0) {
		FAIL("Unable to start broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	printk("Waiting for streams to be started\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_started, K_FOREVER);
	}
}

static void test_broadcast_source_stop_inval_state(struct bt_bap_broadcast_source *source)
{
	int err;

	printk("Test bt_bap_broadcast_source_stop in stopped state\n");
	err = bt_bap_broadcast_source_stop(source);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_stop in stopped state did not fail\n");
		return;
	}
}

static void test_broadcast_source_stop_inval(void)
{
	int err;

	printk("Test bt_bap_broadcast_source_stop with NULL source\n");
	err = bt_bap_broadcast_source_stop(NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_stop with NULL source did not fail\n");
		return;
	}
}

static void test_broadcast_source_stop(struct bt_bap_broadcast_source *source)
{
	int err;

	SET_FLAG(flag_stopping);
	printk("Stopping broadcast source\n");

	err = bt_bap_broadcast_source_stop(source);
	if (err != 0) {
		FAIL("Unable to stop broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be stopped */
	printk("Waiting for streams to be stopped\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_stopped, K_FOREVER);
	}
}

static void test_broadcast_source_delete_inval_state(struct bt_bap_broadcast_source *source)
{
	int err;

	printk("Test bt_bap_broadcast_source_delete in streaming state\n");
	err = bt_bap_broadcast_source_delete(NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_delete in streaming state not fail\n");
		return;
	}
}

static void test_broadcast_source_delete_inval(void)
{
	int err;

	printk("Test bt_bap_broadcast_source_delete with NULL source\n");
	err = bt_bap_broadcast_source_delete(NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_source_delete with NULL source did not fail\n");
		return;
	}
}

static void test_broadcast_source_delete(struct bt_bap_broadcast_source *source)
{
	int err;

	SET_FLAG(flag_stopping);
	printk("Deleting broadcast source\n");

	err = bt_bap_broadcast_source_delete(source);
	if (err != 0) {
		FAIL("Unable to stop broadcast source: %d\n", err);
		return;
	}
}

static int stop_extended_adv(struct bt_le_ext_adv *adv)
{
	int err;

	err = bt_le_per_adv_stop(adv);
	if (err) {
		printk("Failed to stop periodic advertising: %d\n", err);
		return err;
	}

	err = bt_le_ext_adv_stop(adv);
	if (err) {
		printk("Failed to stop extended advertising: %d\n", err);
		return err;
	}

	err = bt_le_ext_adv_delete(adv);
	if (err) {
		printk("Failed to delete extended advertising: %d\n", err);
		return err;
	}

	return 0;
}

static void test_main(void)
{
	uint8_t new_metadata[] = BT_AUDIO_CODEC_CFG_LC3_META(BT_AUDIO_CONTEXT_TYPE_ALERTS);
	struct bt_bap_broadcast_source *source;
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	broadcast_source_create_inval();
	err = setup_broadcast_source(&source);
	if (err != 0) {
		FAIL("Unable to setup broadcast source: %d\n", err);
		return;
	}

	err = setup_extended_adv(source, &adv);
	if (err != 0) {
		FAIL("Failed to setup extended advertising: %d\n", err);
		return;
	}

	test_broadcast_source_reconfig_inval(source);
	test_broadcast_source_reconfig(source);

	test_broadcast_source_start_inval(source, adv);
	test_broadcast_source_start(source, adv);
	test_broadcast_source_reconfig_inval_state(source);
	test_broadcast_source_start_inval_state(source, adv);

	/* Initialize sending */
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		for (unsigned int j = 0U; j < BROADCAST_ENQUEUE_COUNT; j++) {
			sent_cb(streams[i]);
		}
	}

	/* Keeping running for a little while */
	k_sleep(K_SECONDS(15));

	/* Update metadata while streaming */
	printk("Updating metadata\n");
	err = bt_bap_broadcast_source_update_metadata(source, new_metadata,
						      ARRAY_SIZE(new_metadata));
	if (err != 0) {
		FAIL("Failed to update metadata broadcast source: %d", err);
		return;
	}

	/* Keeping running for a little while */
	k_sleep(K_SECONDS(5));

	test_broadcast_source_delete_inval_state(source);

	test_broadcast_source_stop_inval();
	test_broadcast_source_stop(source);
	test_broadcast_source_stop_inval_state(source);

	test_broadcast_source_delete_inval();
	test_broadcast_source_delete(source);
	source = NULL;

	err = stop_extended_adv(adv);
	if (err != 0) {
		FAIL("Unable to stop extended advertising: %d\n", err);
		return;
	}
	adv = NULL;

	/* Recreate broadcast source to verify that it's possible */
	printk("Recreating broadcast source\n");
	err = setup_broadcast_source(&source);
	if (err != 0) {
		FAIL("Unable to setup broadcast source: %d\n", err);
		return;
	}

	printk("Deleting broadcast source\n");
	test_broadcast_source_delete(source);
	source = NULL;

	PASS("Broadcast source passed\n");
}

static const struct bst_test_instance test_broadcast_source[] = {
	{
		.test_id = "broadcast_source",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_broadcast_source_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_broadcast_source);
}

#else /* CONFIG_BT_BAP_BROADCAST_SOURCE */

struct bst_test_list *test_broadcast_source_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */
