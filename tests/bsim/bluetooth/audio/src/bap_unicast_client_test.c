/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/atomic_types.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "bap_stream_rx.h"
#include "bap_stream_tx.h"
#include "bstests.h"
#include "common.h"
#include "bap_common.h"

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)

#define BAP_STREAM_RETRY_WAIT K_MSEC(100)

extern enum bst_result_t bst_result;

static struct audio_test_stream test_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_bap_ep *g_sinks[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_bap_ep *g_sources[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];

static struct bt_bap_unicast_group_stream_pair_param pair_params[ARRAY_SIZE(test_streams)];
static struct bt_bap_unicast_group_stream_param stream_params[ARRAY_SIZE(test_streams)];

/*Mandatory support preset by both client and server */
static struct bt_bap_lc3_preset preset_16_2_1 = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_sink_discovered);
CREATE_FLAG(flag_source_discovered);
CREATE_FLAG(flag_codec_cap_found);
CREATE_FLAG(flag_endpoint_found);
CREATE_FLAG(flag_stream_codec_configured);
static atomic_t flag_stream_qos_configured;
CREATE_FLAG(flag_stream_enabled);
CREATE_FLAG(flag_stream_metadata);
CREATE_FLAG(flag_stream_started);
CREATE_FLAG(flag_stream_connected);
CREATE_FLAG(flag_stream_disconnected);
CREATE_FLAG(flag_stream_disabled);
CREATE_FLAG(flag_stream_stopped);
CREATE_FLAG(flag_stream_released);
CREATE_FLAG(flag_operation_success);

static void stream_configured(struct bt_bap_stream *stream, const struct bt_bap_qos_cfg_pref *pref)
{
	printk("Configured stream %p\n", stream);

	/* TODO: The preference should be used/taken into account when
	 * setting the QoS
	 */

	SET_FLAG(flag_stream_codec_configured);
}

static void stream_qos_set(struct bt_bap_stream *stream)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	printk("QoS set stream %p\n", stream);

	test_stream->tx_sdu_size = stream->qos->sdu;

	atomic_inc(&flag_stream_qos_configured);
}

static void stream_enabled(struct bt_bap_stream *stream)
{
	printk("Enabled stream %p\n", stream);

	SET_FLAG(flag_stream_enabled);
}

static void stream_started(struct bt_bap_stream *stream)
{
	printk("Started stream %p\n", stream);

	if (bap_stream_tx_can_send(stream)) {
		int err;

		err = bap_stream_tx_register(stream);
		if (err != 0) {
			FAIL("Failed to register stream %p for TX: %d\n", stream, err);
			return;
		}
	}

	SET_FLAG(flag_stream_started);
}

static void stream_connected(struct bt_bap_stream *stream)
{
	printk("Connected stream %p\n", stream);

	SET_FLAG(flag_stream_connected);
}

static void stream_disconnected(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Disconnected stream %p with reason %u\n", stream, reason);

	SET_FLAG(flag_stream_disconnected);
}

static void stream_metadata_updated(struct bt_bap_stream *stream)
{
	printk("Metadata updated stream %p\n", stream);

	SET_FLAG(flag_stream_metadata);
}

static void stream_disabled(struct bt_bap_stream *stream)
{
	printk("Disabled stream %p\n", stream);

	SET_FLAG(flag_stream_disabled);
}

static void stream_stopped(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stopped stream %p with reason 0x%02X\n", stream, reason);

	if (bap_stream_tx_can_send(stream)) {
		int err;

		err = bap_stream_tx_unregister(stream);
		if (err != 0) {
			FAIL("Failed to unregister stream %p for TX: %d\n", stream, err);
			return;
		}
	}

	SET_FLAG(flag_stream_stopped);
}

static void stream_released(struct bt_bap_stream *stream)
{
	printk("Released stream %p\n", stream);

	SET_FLAG(flag_stream_released);
}

static struct bt_bap_stream_ops stream_ops = {
	.configured = stream_configured,
	.qos_set = stream_qos_set,
	.enabled = stream_enabled,
	.started = stream_started,
	.metadata_updated = stream_metadata_updated,
	.disabled = stream_disabled,
	.stopped = stream_stopped,
	.released = stream_released,
	.recv = bap_stream_rx_recv_cb,
	.sent = bap_stream_tx_sent_cb,
	.connected = stream_connected,
	.disconnected = stream_disconnected,
};

static void unicast_client_location_cb(struct bt_conn *conn,
				       enum bt_audio_dir dir,
				       enum bt_audio_location loc)
{
	printk("dir %u loc %X\n", dir, loc);
}

static void available_contexts_cb(struct bt_conn *conn,
				  enum bt_audio_context snk_ctx,
				  enum bt_audio_context src_ctx)
{
	printk("snk ctx %u src ctx %u\n", snk_ctx, src_ctx);
}


static void config_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		      enum bt_bap_ascs_reason reason)
{
	printk("stream %p config operation rsp_code %u reason %u\n", stream, rsp_code, reason);

	if (rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
		SET_FLAG(flag_operation_success);
	}
}

static void qos_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		   enum bt_bap_ascs_reason reason)
{
	printk("stream %p qos operation rsp_code %u reason %u\n", stream, rsp_code, reason);

	if (rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
		SET_FLAG(flag_operation_success);
	}
}

static void enable_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		      enum bt_bap_ascs_reason reason)
{
	printk("stream %p enable operation rsp_code %u reason %u\n", stream, rsp_code, reason);

	if (rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
		SET_FLAG(flag_operation_success);
	}
}

static void start_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		     enum bt_bap_ascs_reason reason)
{
	printk("stream %p start operation rsp_code %u reason %u\n", stream, rsp_code, reason);

	if (rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
		SET_FLAG(flag_operation_success);
	}
}

static void stop_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		    enum bt_bap_ascs_reason reason)
{
	printk("stream %p stop operation rsp_code %u reason %u\n", stream, rsp_code, reason);

	if (rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
		SET_FLAG(flag_operation_success);
	}
}

static void disable_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		       enum bt_bap_ascs_reason reason)
{
	printk("stream %p disable operation rsp_code %u reason %u\n", stream, rsp_code, reason);

	if (rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
		SET_FLAG(flag_operation_success);
	}
}

static void metadata_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
			enum bt_bap_ascs_reason reason)
{
	printk("stream %p metadata operation rsp_code %u reason %u\n", stream, rsp_code, reason);

	if (rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
		SET_FLAG(flag_operation_success);
	}
}

static void release_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		       enum bt_bap_ascs_reason reason)
{
	printk("stream %p release operation rsp_code %u reason %u\n", stream, rsp_code, reason);

	if (rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
		SET_FLAG(flag_operation_success);
	}
}

static void add_remote_sink(struct bt_bap_ep *ep)
{
	for (size_t i = 0U; i < ARRAY_SIZE(g_sinks); i++) {
		if (g_sinks[i] == NULL) {
			printk("Sink #%zu: ep %p\n", i, ep);
			g_sinks[i] = ep;
			return;
		}
	}

	FAIL("Could not add sink ep\n");
}

static void add_remote_source(struct bt_bap_ep *ep)
{
	for (size_t i = 0U; i < ARRAY_SIZE(g_sources); i++) {
		if (g_sources[i] == NULL) {
			printk("Source #%u: ep %p\n", i, ep);
			g_sources[i] = ep;
			return;
		}
	}

	FAIL("Could not add source ep\n");
}

static void print_remote_codec_cap(const struct bt_audio_codec_cap *codec_cap,
				   enum bt_audio_dir dir)
{
	printk("codec %p dir 0x%02x\n", codec_cap, dir);

	print_codec_cap(codec_cap);
}

static void discover_sinks_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	if (err != 0) {
		FAIL("Discovery failed: %d\n", err);
		return;
	}

	printk("Discover complete\n");

	SET_FLAG(flag_sink_discovered);
}

static void discover_sources_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	if (err != 0) {
		FAIL("Discovery failed: %d\n", err);
		return;
	}

	printk("Sources discover complete\n");

	SET_FLAG(flag_source_discovered);
}

static void pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir,
			  const struct bt_audio_codec_cap *codec_cap)
{
	print_remote_codec_cap(codec_cap, dir);
	SET_FLAG(flag_codec_cap_found);
}

static void endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	if (dir == BT_AUDIO_DIR_SINK) {
		add_remote_sink(ep);
	} else {
		add_remote_source(ep);
	}

	SET_FLAG(flag_endpoint_found);
}

static struct bt_bap_unicast_client_cb unicast_client_cbs = {
	.location = unicast_client_location_cb,
	.available_contexts = available_contexts_cb,
	.config = config_cb,
	.qos = qos_cb,
	.enable = enable_cb,
	.start = start_cb,
	.stop = stop_cb,
	.disable = disable_cb,
	.metadata = metadata_cb,
	.release = release_cb,
	.pac_record = pac_record_cb,
	.endpoint = endpoint_cb,
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU exchanged\n");
	SET_FLAG(flag_mtu_exchanged);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated,
};

static bool parse_ascs_ad_data(struct bt_data *data, void *user_data)
{
	const struct bt_le_scan_recv_info *info = user_data;
	uint16_t available_source_context;
	uint16_t available_sink_context;
	struct net_buf_simple net_buf;
	struct bt_uuid_16 adv_uuid;
	uint8_t announcement_type;
	void *uuid;
	int err;

	const size_t min_data_len = BT_UUID_SIZE_16 + sizeof(announcement_type) +
				    sizeof(available_sink_context) +
				    sizeof(available_source_context);

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < min_data_len) {

		return true;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)data->data, data->data_len);

	uuid = net_buf_simple_pull_mem(&net_buf, BT_UUID_SIZE_16);
	if (!bt_uuid_create(&adv_uuid.uuid, uuid, BT_UUID_SIZE_16)) {
		return true;
	}

	if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_ASCS)) {
		return true;
	}

	announcement_type = net_buf_simple_pull_u8(&net_buf);
	available_sink_context = net_buf_simple_pull_le16(&net_buf);
	available_source_context = net_buf_simple_pull_le16(&net_buf);

	printk("Found ASCS with announcement type 0x%02X, sink ctx 0x%04X, source ctx 0x%04X\n",
	       announcement_type, available_sink_context, available_source_context);

	printk("Stopping scan\n");
	if (bt_le_scan_stop()) {
		FAIL("Could not stop scan");
		return false;
	}

	err = bt_conn_le_create(info->addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err) {
		FAIL("Could not connect to peer: %d", err);
		return false;
	}

	/* Stop parsing */
	return false;
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (default_conn) {
		return;
	}

	/* We're only interested in connectable events */
	if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) == 0) {
		return;
	}
	/* connect only to devices in close proximity */
	if (info->rssi < -70) {
		return;
	}

	bt_addr_le_to_str(info->addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, info->rssi);

	bt_data_parse(ad, parse_ascs_ad_data, (void *)info);
}

static struct bt_le_scan_cb bap_scan_cb = {
	.recv = broadcast_scan_recv,
};

static void init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	bap_stream_tx_init();

	for (size_t i = 0; i < ARRAY_SIZE(test_streams); i++) {
		struct bt_bap_stream *bap_stream =
			bap_stream_from_audio_test_stream(&test_streams[i]);

		bap_stream->ops = &stream_ops;
	}

	bt_le_scan_cb_register(&bap_scan_cb);
	bt_gatt_cb_register(&gatt_callbacks);

	err = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
	if (err != 0) {
		FAIL("Failed to register client callbacks: %d", err);
		return;
	}
}

static void scan_and_connect(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
	WAIT_FOR_FLAG(flag_connected);
}

static void disconnect_acl(void)
{
	int err;

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		FAIL("Failed to disconnect (err %d)\n", err);
		return;
	}

	WAIT_FOR_UNSET_FLAG(flag_connected);
}

static void exchange_mtu(void)
{
	WAIT_FOR_FLAG(flag_mtu_exchanged);
}

static void discover_sinks(void)
{
	int err;

	unicast_client_cbs.discover = discover_sinks_cb;

	UNSET_FLAG(flag_codec_cap_found);
	UNSET_FLAG(flag_sink_discovered);
	UNSET_FLAG(flag_endpoint_found);

	err = bt_bap_unicast_client_discover(default_conn, BT_AUDIO_DIR_SINK);
	if (err != 0) {
		printk("Failed to discover sink: %d\n", err);
		return;
	}

	memset(g_sinks, 0, sizeof(g_sinks));

	WAIT_FOR_FLAG(flag_codec_cap_found);
	WAIT_FOR_FLAG(flag_endpoint_found);
	WAIT_FOR_FLAG(flag_sink_discovered);
}

static void discover_sources(void)
{
	int err;

	unicast_client_cbs.discover = discover_sources_cb;

	UNSET_FLAG(flag_codec_cap_found);
	UNSET_FLAG(flag_source_discovered);

	err = bt_bap_unicast_client_discover(default_conn, BT_AUDIO_DIR_SOURCE);
	if (err != 0) {
		printk("Failed to discover sink: %d\n", err);
		return;
	}

	memset(g_sources, 0, sizeof(g_sources));

	WAIT_FOR_FLAG(flag_codec_cap_found);
	WAIT_FOR_FLAG(flag_source_discovered);
}

static int codec_configure_stream(struct bt_bap_stream *stream, struct bt_bap_ep *ep,
				  struct bt_audio_codec_cfg *codec_cfg)
{
	int err;

	UNSET_FLAG(flag_stream_codec_configured);
	UNSET_FLAG(flag_operation_success);

	do {

		err = bt_bap_stream_config(default_conn, stream, ep, codec_cfg);
		if (err == -EBUSY) {
			k_sleep(BAP_STREAM_RETRY_WAIT);
		} else if (err != 0) {
			FAIL("Could not configure stream %p: %d\n", stream, err);
			return err;
		}
	} while (err == -EBUSY);

	WAIT_FOR_FLAG(flag_stream_codec_configured);
	WAIT_FOR_FLAG(flag_operation_success);

	return 0;
}

static void codec_configure_streams(size_t stream_cnt)
{
	for (size_t i = 0U; i < ARRAY_SIZE(pair_params); i++) {
		if (pair_params[i].rx_param != NULL && g_sources[i] != NULL) {
			struct bt_bap_stream *stream = pair_params[i].rx_param->stream;
			const int err = codec_configure_stream(stream, g_sources[i],
							       &preset_16_2_1.codec_cfg);

			if (err != 0) {
				FAIL("Unable to configure source stream[%zu]: %d", i, err);
				return;
			}
		}

		if (pair_params[i].tx_param != NULL && g_sinks[i] != NULL) {
			struct bt_bap_stream *stream = pair_params[i].tx_param->stream;
			const int err = codec_configure_stream(stream, g_sinks[i],
							       &preset_16_2_1.codec_cfg);

			if (err != 0) {
				FAIL("Unable to configure sink stream[%zu]: %d", i, err);
				return;
			}
		}
	}
}

static void qos_configure_streams(struct bt_bap_unicast_group *unicast_group,
				  size_t stream_cnt)
{
	int err;

	UNSET_FLAG(flag_stream_qos_configured);

	do {
		err = bt_bap_stream_qos(default_conn, unicast_group);
		if (err == -EBUSY) {
			k_sleep(BAP_STREAM_RETRY_WAIT);
		} else if (err != 0) {
			FAIL("Unable to QoS configure streams: %d\n", err);
			return;
		}
	} while (err == -EBUSY);

	while (atomic_get(&flag_stream_qos_configured) != stream_cnt) {
		(void)k_sleep(K_MSEC(1));
	}
}

static int enable_stream(struct bt_bap_stream *stream)
{
	int err;

	UNSET_FLAG(flag_stream_enabled);

	do {
		err = bt_bap_stream_enable(stream, NULL, 0);
		if (err == -EBUSY) {
			k_sleep(BAP_STREAM_RETRY_WAIT);
		} else if (err != 0) {
			FAIL("Could not enable stream %p: %d\n", stream, err);
			return err;
		}
	} while (err == -EBUSY);

	WAIT_FOR_FLAG(flag_stream_enabled);

	return 0;
}

static void enable_streams(size_t stream_cnt)
{
	for (size_t i = 0U; i < stream_cnt; i++) {
		struct bt_bap_stream *stream = bap_stream_from_audio_test_stream(&test_streams[i]);
		int err;

		err = enable_stream(stream);
		if (err != 0) {
			FAIL("Unable to enable stream[%zu]: %d", i, err);

			return;
		}
	}
}

static int metadata_update_stream(struct bt_bap_stream *stream)
{
	const uint8_t new_meta[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_VENDOR, LONG_META),
	};
	int err;

	UNSET_FLAG(flag_stream_metadata);

	do {
		err = bt_bap_stream_metadata(stream, new_meta, ARRAY_SIZE(new_meta));
		if (err == -EBUSY) {
			k_sleep(BAP_STREAM_RETRY_WAIT);
		} else if (err != 0) {
			FAIL("Could not metadata update stream %p: %d\n", stream, err);
			return err;
		}
	} while (err == -EBUSY);

	WAIT_FOR_FLAG(flag_stream_metadata);

	return 0;
}

static void metadata_update_streams(size_t stream_cnt)
{
	for (size_t i = 0U; i < stream_cnt; i++) {
		struct bt_bap_stream *stream = bap_stream_from_audio_test_stream(&test_streams[i]);
		int err;

		err = metadata_update_stream(stream);
		if (err != 0) {
			FAIL("Unable to metadata update stream[%zu]: %d", i, err);

			return;
		}
	}
}

static int connect_stream(struct bt_bap_stream *stream)
{
	int err;

	UNSET_FLAG(flag_stream_started);

	do {
		err = bt_bap_stream_connect(stream);
		if (err == -EALREADY) {
			SET_FLAG(flag_stream_started);
		} else if (err != 0) {
			FAIL("Could not start stream %p: %d\n", stream, err);
			return err;
		}
	} while (err == -EBUSY);

	WAIT_FOR_FLAG(flag_stream_started);

	return 0;
}

static void connect_streams(void)
{
	struct bt_bap_stream *source_stream;
	struct bt_bap_stream *sink_stream;

	/* We only support a single CIS so far, so only start one. We can use the group pair
	 * params to start both a sink and source stream that use the same CIS
	 */

	source_stream = pair_params[0].rx_param == NULL ? NULL : pair_params[0].rx_param->stream;
	sink_stream = pair_params[0].tx_param == NULL ? NULL : pair_params[0].tx_param->stream;

	UNSET_FLAG(flag_stream_connected);

	if (sink_stream != NULL) {
		const int err = connect_stream(sink_stream);

		if (err != 0) {
			FAIL("Unable to connect sink: %d", err);

			return;
		}
	}

	if (source_stream != NULL) {
		const int err = connect_stream(source_stream);

		if (err != 0) {
			FAIL("Unable to connect source stream: %d", err);

			return;
		}
	}

	WAIT_FOR_FLAG(flag_stream_connected);
}

static int start_stream(struct bt_bap_stream *stream)
{
	int err;

	UNSET_FLAG(flag_stream_started);

	do {
		err = bt_bap_stream_start(stream);
		if (err == -EBUSY) {
			k_sleep(BAP_STREAM_RETRY_WAIT);
		} else if (err != 0) {
			FAIL("Could not start stream %p: %d\n", stream, err);
			return err;
		}
	} while (err == -EBUSY);

	WAIT_FOR_FLAG(flag_stream_started);

	return 0;
}

static void start_streams(void)
{
	struct bt_bap_stream *source_stream;

	source_stream = pair_params[0].rx_param == NULL ? NULL : pair_params[0].rx_param->stream;

	if (source_stream != NULL) {
		const int err = start_stream(source_stream);

		if (err != 0) {
			FAIL("Unable to start source stream: %d", err);

			return;
		}
	}
}

static void transceive_streams(void)
{
	struct bt_bap_stream *source_stream;
	struct bt_bap_stream *sink_stream;

	source_stream = pair_params[0].rx_param == NULL ? NULL : pair_params[0].rx_param->stream;
	sink_stream = pair_params[0].tx_param == NULL ? NULL : pair_params[0].tx_param->stream;

	if (sink_stream != NULL) {
		struct audio_test_stream *test_stream =
			audio_test_stream_from_bap_stream(sink_stream);

		/* Keep sending until we reach the minimum expected */
		while (test_stream->tx_cnt < MIN_SEND_COUNT) {
			k_sleep(K_MSEC(100));
		}
	}

	if (source_stream != NULL) {
		printk("Waiting for data\n");
		WAIT_FOR_FLAG(flag_audio_received);
	}
}

static void disable_streams(size_t stream_cnt)
{
	for (size_t i = 0; i < stream_cnt; i++) {
		int err;

		UNSET_FLAG(flag_operation_success);
		UNSET_FLAG(flag_stream_disabled);

		do {
			err = bt_bap_stream_disable(
				bap_stream_from_audio_test_stream(&test_streams[i]));
			if (err == -EBUSY) {
				k_sleep(BAP_STREAM_RETRY_WAIT);
			} else if (err != 0) {
				FAIL("Could not disable stream: %d\n", err);
				return;
			}
		} while (err == -EBUSY);

		WAIT_FOR_FLAG(flag_operation_success);
		WAIT_FOR_FLAG(flag_stream_disabled);
	}
}

static void stop_streams(size_t stream_cnt)
{
	UNSET_FLAG(flag_stream_disconnected);

	for (size_t i = 0; i < stream_cnt; i++) {
		struct bt_bap_stream *source_stream;
		int err;

		/* We can only stop source streams */
		source_stream =
			pair_params[i].rx_param == NULL ? NULL : pair_params[i].rx_param->stream;

		if (source_stream == NULL) {
			continue;
		}

		UNSET_FLAG(flag_operation_success);
		UNSET_FLAG(flag_stream_stopped);

		do {
			err = bt_bap_stream_stop(source_stream);
			if (err == -EBUSY) {
				k_sleep(BAP_STREAM_RETRY_WAIT);
			} else if (err != 0) {
				FAIL("Could not stop stream: %d\n", err);
				return;
			}
		} while (err == -EBUSY);

		WAIT_FOR_FLAG(flag_operation_success);
		WAIT_FOR_FLAG(flag_stream_stopped);
	}

	WAIT_FOR_FLAG(flag_stream_disconnected);
}

static void release_streams(size_t stream_cnt)
{
	for (size_t i = 0; i < stream_cnt; i++) {
		int err;

		UNSET_FLAG(flag_operation_success);
		UNSET_FLAG(flag_stream_released);

		do {
			err = bt_bap_stream_release(
				bap_stream_from_audio_test_stream(&test_streams[i]));
			if (err == -EBUSY) {
				k_sleep(BAP_STREAM_RETRY_WAIT);
			} else if (err != 0) {
				FAIL("Could not release stream: %d\n", err);
				return;
			}
		} while (err == -EBUSY);

		WAIT_FOR_FLAG(flag_operation_success);
		WAIT_FOR_FLAG(flag_stream_released);
	}
}

static size_t create_unicast_group(struct bt_bap_unicast_group **unicast_group)
{
	struct bt_bap_unicast_group_param param;
	size_t stream_cnt = 0;
	size_t pair_cnt = 0;
	int err;

	memset(stream_params, 0, sizeof(stream_params));
	memset(pair_params, 0, sizeof(pair_params));

	for (size_t i = 0U; i < MIN(ARRAY_SIZE(g_sinks), ARRAY_SIZE(test_streams)); i++) {
		if (g_sinks[i] == NULL) {
			break;
		}

		stream_params[stream_cnt].stream =
			bap_stream_from_audio_test_stream(&test_streams[stream_cnt]);
		stream_params[stream_cnt].qos = &preset_16_2_1.qos;
		pair_params[i].tx_param = &stream_params[stream_cnt];

		stream_cnt++;

		break;
	}

	for (size_t i = 0U; i < MIN(ARRAY_SIZE(g_sources), ARRAY_SIZE(test_streams)); i++) {
		if (g_sources[i] == NULL) {
			break;
		}

		stream_params[stream_cnt].stream =
			bap_stream_from_audio_test_stream(&test_streams[stream_cnt]);
		stream_params[stream_cnt].qos = &preset_16_2_1.qos;
		pair_params[i].rx_param = &stream_params[stream_cnt];

		stream_cnt++;

		break;
	}

	for (pair_cnt = 0U; pair_cnt < ARRAY_SIZE(pair_params); pair_cnt++) {
		if (pair_params[pair_cnt].rx_param == NULL &&
		    pair_params[pair_cnt].tx_param == NULL) {
			break;
		}
	}

	if (stream_cnt == 0U) {
		FAIL("No streams added to group");

		return 0;
	}

	param.params = pair_params;
	param.params_count = pair_cnt;
	param.packing = BT_ISO_PACKING_SEQUENTIAL;

	/* Require controller support for CIGs */
	err = bt_bap_unicast_group_create(&param, unicast_group);
	if (err != 0) {
		FAIL("Unable to create unicast group: %d", err);

		return 0;
	}

	return stream_cnt;
}

static void delete_unicast_group(struct bt_bap_unicast_group *unicast_group)
{
	int err;

	/* Require controller support for CIGs */
	err = bt_bap_unicast_group_delete(unicast_group);
	if (err != 0) {
		FAIL("Unable to delete unicast group: %d", err);
		return;
	}
}

static void test_main(void)
{
	/* TODO: Temporarily reduce to 1 due to bug in controller. Set to > 1 value again when
	 * https://github.com/zephyrproject-rtos/zephyr/issues/57904 has been resolved.
	 */
	const unsigned int iterations = 1;

	init();

	scan_and_connect();

	exchange_mtu();

	discover_sinks();
	discover_sinks(); /* test that we can discover twice */

	discover_sources();
	discover_sources(); /* test that we can discover twice */

	/* Run the stream setup multiple time to ensure states are properly
	 * set and reset
	 */
	for (unsigned int i = 0U; i < iterations; i++) {
		struct bt_bap_unicast_group *unicast_group;
		size_t stream_cnt;

		printk("\n########### Running iteration #%u\n\n", i);

		printk("Creating unicast group\n");
		stream_cnt = create_unicast_group(&unicast_group);

		printk("Codec configuring streams\n");
		codec_configure_streams(stream_cnt);

		printk("QoS configuring streams\n");
		qos_configure_streams(unicast_group, stream_cnt);

		printk("Enabling streams\n");
		enable_streams(stream_cnt);

		printk("Metadata update streams\n");
		metadata_update_streams(stream_cnt);

		printk("Connecting streams\n");
		connect_streams();

		printk("Starting streams\n");
		start_streams();

		printk("Starting transceiving\n");
		transceive_streams();

		printk("Disabling streams\n");
		disable_streams(stream_cnt);

		printk("Stopping streams\n");
		stop_streams(stream_cnt);

		printk("Releasing streams\n");
		release_streams(stream_cnt);

		/* Test removing streams from group after creation */
		printk("Deleting unicast group\n");
		delete_unicast_group(unicast_group);
		unicast_group = NULL;
	}

	disconnect_acl();

	PASS("Unicast client passed\n");
}

static void test_main_acl_disconnect(void)
{
	struct bt_bap_unicast_group *unicast_group;
	size_t stream_cnt;

	init();

	stream_ops.recv = NULL; /* We do not care about data in this test */

	scan_and_connect();

	exchange_mtu();

	discover_sinks();

	discover_sources();

	printk("Creating unicast group\n");
	stream_cnt = create_unicast_group(&unicast_group);

	printk("Codec configuring streams\n");
	codec_configure_streams(stream_cnt);

	printk("QoS configuring streams\n");
	qos_configure_streams(unicast_group, stream_cnt);

	printk("Enabling streams\n");
	enable_streams(stream_cnt);

	printk("Metadata update streams\n");
	metadata_update_streams(stream_cnt);

	printk("Connecting streams\n");
	connect_streams();

	printk("Starting streams\n");
	start_streams();

	disconnect_acl();

	printk("Deleting unicast group\n");
	delete_unicast_group(unicast_group);
	unicast_group = NULL;

	/* Reconnect */
	scan_and_connect();

	disconnect_acl();

	PASS("Unicast client ACL disconnect passed\n");
}

static void test_main_async_group(void)
{
	struct bt_bap_stream rx_stream = {0};
	struct bt_bap_stream tx_stream = {0};
	struct bt_bap_qos_cfg rx_qos = BT_BAP_QOS_CFG_UNFRAMED(7500U, 30U, 2U, 75U, 40000U);
	struct bt_bap_qos_cfg tx_qos = BT_BAP_QOS_CFG_UNFRAMED(10000U, 40U, 2U, 100U, 40000U);
	struct bt_bap_unicast_group_stream_param rx_param = {
		.qos = &rx_qos,
		.stream = &rx_stream,
	};
	struct bt_bap_unicast_group_stream_param tx_param = {
		.qos = &tx_qos,
		.stream = &tx_stream,
	};
	struct bt_bap_unicast_group_stream_pair_param pair_param = {
		.rx_param = &rx_param,
		.tx_param = &tx_param,
	};
	struct bt_bap_unicast_group_param param = {
		.params = &pair_param,
		.params_count = 1U,
		.packing = BT_ISO_PACKING_SEQUENTIAL,
	};
	struct bt_bap_unicast_group *unicast_group;
	int err;

	init();

	err = bt_bap_unicast_group_create(&param, &unicast_group);
	if (err != 0) {
		FAIL("Unable to create unicast group: %d", err);

		return;
	}

	PASS("Unicast client async group parameters passed\n");
}

static void test_main_reconf_group(void)
{
	static struct bt_bap_lc3_preset preset_16_2_2 = BT_BAP_LC3_UNICAST_PRESET_16_2_2(
		BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	struct bt_bap_stream rx_stream = {0};
	struct bt_bap_stream tx_stream = {0};
	struct bt_bap_unicast_group_stream_param rx_param = {
		.qos = &preset_16_2_1.qos,
		.stream = &rx_stream,
	};
	struct bt_bap_unicast_group_stream_param tx_param = {
		.qos = &preset_16_2_1.qos,
		.stream = &tx_stream,
	};
	struct bt_bap_unicast_group_stream_pair_param pair_param = {
		.rx_param = &rx_param,
		.tx_param = &tx_param,
	};
	struct bt_bap_unicast_group_param param = {
		.params = &pair_param,
		.params_count = 1U,
		.packing = BT_ISO_PACKING_SEQUENTIAL,
	};
	struct bt_bap_unicast_group *unicast_group;
	int err;

	init();

	err = bt_bap_unicast_group_create(&param, &unicast_group);
	if (err != 0) {
		FAIL("Unable to create unicast group: %d", err);

		return;
	}

	rx_param.qos = &preset_16_2_2.qos;
	tx_param.qos = &preset_16_2_2.qos;
	err = bt_bap_unicast_group_reconfig(unicast_group, &param);
	if (err != 0) {
		FAIL("Unable to reconfigure unicast group: %d", err);

		return;
	}

	PASS("Unicast client async group parameters passed\n");
}

static const struct bst_test_instance test_unicast_client[] = {
	{
		.test_id = "unicast_client",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	{
		.test_id = "unicast_client_acl_disconnect",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_acl_disconnect,
	},
	{
		.test_id = "unicast_client_async_group",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_async_group,
		.test_descr = "Tests that a unicast group (CIG) can be created with different "
			      "values in each direction, such as 10000us SDU interval in C to P "
			      "and 7500us for P to C",
	},
	{
		.test_id = "unicast_client_reconf_group",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_reconf_group,
		.test_descr = "Tests that a unicast group (CIG) can be reconfigred with new values",
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_unicast_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_unicast_client);
}

#else /* !(CONFIG_BT_BAP_UNICAST_CLIENT) */

struct bst_test_list *test_unicast_client_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */
