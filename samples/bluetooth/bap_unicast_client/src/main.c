/*
 * Copyright (c) 2021-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

#include "stream_tx.h"

static void start_scan(void);

uint64_t unicast_audio_recv_ctr; /* This value is exposed to test code */

static struct bt_bap_unicast_client_cb unicast_client_cbs;
static struct bt_conn *default_conn;
static struct bt_bap_unicast_group *unicast_group;
static struct audio_sink {
	struct bt_bap_ep *ep;
	uint16_t seq_num;
} sinks[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_bap_ep *sources[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];

static struct bt_bap_stream streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +
				      CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
static size_t configured_sink_stream_count;
static size_t configured_source_stream_count;

#define configured_stream_count (configured_sink_stream_count + \
				 configured_source_stream_count)

/* Select a codec configuration to apply that is mandatory to support by both client and server.
 * Allows this sample application to work without logic to parse the codec capabilities of the
 * server and selection of an appropriate codec configuration.
 */
static struct bt_bap_lc3_preset codec_configuration = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_disconnected, 0, 1);
static K_SEM_DEFINE(sem_mtu_exchanged, 0, 1);
static K_SEM_DEFINE(sem_security_updated, 0, 1);
static K_SEM_DEFINE(sem_sinks_discovered, 0, 1);
static K_SEM_DEFINE(sem_sources_discovered, 0, 1);
static K_SEM_DEFINE(sem_stream_configured, 0, 1);
static K_SEM_DEFINE(sem_stream_qos, 0, ARRAY_SIZE(sinks) + ARRAY_SIZE(sources));
static K_SEM_DEFINE(sem_stream_enabled, 0, 1);
static K_SEM_DEFINE(sem_stream_started, 0, 1);
static K_SEM_DEFINE(sem_stream_connected, 0, 1);

static void print_hex(const uint8_t *ptr, size_t len)
{
	while (len-- != 0) {
		printk("%02x", *ptr++);
	}
}

static bool print_cb(struct bt_data *data, void *user_data)
{
	const char *str = (const char *)user_data;

	printk("%s: type 0x%02x value_len %u\n", str, data->type, data->data_len);
	print_hex(data->data, data->data_len);
	printk("\n");

	return true;
}

static void print_codec_cap(const struct bt_audio_codec_cap *codec_cap)
{
	printk("codec id 0x%02x cid 0x%04x vid 0x%04x count %u\n", codec_cap->id, codec_cap->cid,
	       codec_cap->vid, codec_cap->data_len);

	if (codec_cap->id == BT_HCI_CODING_FORMAT_LC3) {
		bt_audio_data_parse(codec_cap->data, codec_cap->data_len, print_cb, "data");
	} else { /* If not LC3, we cannot assume it's LTV */
		printk("data: ");
		print_hex(codec_cap->data, codec_cap->data_len);
		printk("\n");
	}

	bt_audio_data_parse(codec_cap->meta, codec_cap->meta_len, print_cb, "meta");
}

static bool check_audio_support_and_connect(struct bt_data *data,
					    void *user_data)
{
	struct net_buf_simple ascs_svc_data;
	bt_addr_le_t *addr = user_data;
	uint8_t announcement_type;
	uint32_t audio_contexts;
	const struct bt_uuid *uuid;
	uint16_t uuid_val;
	uint8_t meta_len;
	size_t min_size;
	int err;

	printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	if (data->type != BT_DATA_SVC_DATA16) {
		return true; /* Continue parsing to next AD data type */
	}

	if (data->data_len < sizeof(uuid_val)) {
		printk("AD invalid size %u\n", data->data_len);
		return true; /* Continue parsing to next AD data type */
	}

	net_buf_simple_init_with_data(&ascs_svc_data, (void *)data->data,
				      data->data_len);

	uuid_val = net_buf_simple_pull_le16(&ascs_svc_data);
	uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(uuid_val));
	if (bt_uuid_cmp(uuid, BT_UUID_ASCS) != 0) {
		/* We are looking for the ASCS service data */
		return true; /* Continue parsing to next AD data type */
	}

	min_size = sizeof(announcement_type) + sizeof(audio_contexts) + sizeof(meta_len);
	if (ascs_svc_data.len < min_size) {
		printk("AD invalid size %u\n", data->data_len);
		return false; /* Stop parsing */
	}

	announcement_type = net_buf_simple_pull_u8(&ascs_svc_data);
	audio_contexts = net_buf_simple_pull_le32(&ascs_svc_data);
	meta_len = net_buf_simple_pull_u8(&ascs_svc_data);

	err = bt_le_scan_stop();
	if (err != 0) {
		printk("Failed to stop scan: %d\n", err);
		return false; /* Stop parsing */
	}

	printk("Audio server found with type %u, contexts 0x%08x and meta_len %u; connecting\n",
	       announcement_type, audio_contexts, meta_len);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_BAP_CONN_PARAM_RELAXED,
				&default_conn);
	if (err != 0) {
		printk("Create conn to failed (%u)\n", err);
		start_scan();
	}

	return false; /* Stop parsing */
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (default_conn != NULL) {
		/* Already connected */
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND &&
	    type != BT_GAP_ADV_TYPE_EXT_ADV) {
		return;
	}

	(void)bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	/* connect only to devices in close proximity */
	if (rssi < -50) {
		return;
	}

	bt_data_parse(ad, check_audio_support_and_connect, (void *)addr);
}

static void start_scan(void)
{
	int err;

	/* This demo doesn't require active scan */
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static void stream_configured(struct bt_bap_stream *stream, const struct bt_bap_qos_cfg_pref *pref)
{
	printk("Audio Stream %p configured\n", stream);

	k_sem_give(&sem_stream_configured);
}

static void stream_qos_set(struct bt_bap_stream *stream)
{
	printk("Audio Stream %p QoS set\n", stream);

	k_sem_give(&sem_stream_qos);
}

static void stream_enabled(struct bt_bap_stream *stream)
{
	printk("Audio Stream %p enabled\n", stream);

	k_sem_give(&sem_stream_enabled);
}

static bool stream_tx_can_send(const struct bt_bap_stream *stream)
{
	struct bt_bap_ep_info info;
	int err;

	if (stream == NULL || stream->ep == NULL) {
		return false;
	}

	err = bt_bap_ep_get_info(stream->ep, &info);
	if (err != 0) {
		return false;
	}

	return info.can_send;
}

static void stream_connected_cb(struct bt_bap_stream *stream)
{
	printk("Audio Stream %p connected\n", stream);

	/* Reset sequence number for sinks */
	for (size_t i = 0U; i < configured_sink_stream_count; i++) {
		if (stream->ep == sinks[i].ep) {
			sinks[i].seq_num = 0U;
			break;
		}
	}

	k_sem_give(&sem_stream_connected);
}

static void stream_started(struct bt_bap_stream *stream)
{
	printk("Audio Stream %p started\n", stream);
	/* Register the stream for TX if it can send */
	if (IS_ENABLED(CONFIG_BT_AUDIO_TX) && stream_tx_can_send(stream)) {
		const int err = stream_tx_register(stream);

		if (err != 0) {
			printk("Failed to register stream %p for TX: %d\n", stream, err);
		}
	}

	k_sem_give(&sem_stream_started);
}

static void stream_metadata_updated(struct bt_bap_stream *stream)
{
	printk("Audio Stream %p metadata updated\n", stream);
}

static void stream_disabled(struct bt_bap_stream *stream)
{
	printk("Audio Stream %p disabled\n", stream);
}

static void stream_stopped(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Audio Stream %p stopped with reason 0x%02X\n", stream, reason);

	/* Unregister the stream for TX if it can send */
	if (IS_ENABLED(CONFIG_BT_AUDIO_TX) && stream_tx_can_send(stream)) {
		const int err = stream_tx_unregister(stream);

		if (err != 0) {
			printk("Failed to unregister stream %p for TX: %d", stream, err);
		}
	}
}

static void stream_released(struct bt_bap_stream *stream)
{
	printk("Audio Stream %p released\n", stream);
}

static void stream_recv(struct bt_bap_stream *stream,
			const struct bt_iso_recv_info *info,
			struct net_buf *buf)
{
	if (info->flags & BT_ISO_FLAGS_VALID) {
		unicast_audio_recv_ctr++;
		printk("Incoming audio on stream %p len %u (%"PRIu64")\n", stream, buf->len,
			unicast_audio_recv_ctr);
	}
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
	.recv = stream_recv,
	.connected = stream_connected_cb,
};

static void add_remote_source(struct bt_bap_ep *ep)
{
	for (size_t i = 0U; i < ARRAY_SIZE(sources); i++) {
		if (sources[i] == NULL) {
			printk("Source #%zu: ep %p\n", i, ep);
			sources[i] = ep;
			return;
		}
	}

	printk("Could not add source ep\n");
}

static void add_remote_sink(struct bt_bap_ep *ep)
{
	for (size_t i = 0U; i < ARRAY_SIZE(sinks); i++) {
		if (sinks[i].ep == NULL) {
			printk("Sink #%zu: ep %p\n", i, ep);
			sinks[i].ep = ep;
			return;
		}
	}

	printk("Could not add sink ep\n");
}

static void print_remote_codec_cap(const struct bt_audio_codec_cap *codec_cap,
				   enum bt_audio_dir dir)
{
	printk("codec_cap %p dir 0x%02x\n", codec_cap, dir);

	print_codec_cap(codec_cap);
}

static void discover_sinks_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	if (err != 0 && err != BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		printk("Discovery failed: %d\n", err);
		return;
	}

	if (err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		printk("Discover sinks completed without finding any sink ASEs\n");
	} else {
		printk("Discover sinks complete: err %d\n", err);
	}

	k_sem_give(&sem_sinks_discovered);
}

static void discover_sources_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	if (err != 0 && err != BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		printk("Discovery failed: %d\n", err);
		return;
	}

	if (err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		printk("Discover sinks completed without finding any source ASEs\n");
	} else {
		printk("Discover sources complete: err %d\n", err);
	}

	k_sem_give(&sem_sources_discovered);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		printk("Failed to connect to %s %u %s\n", addr, err, bt_hci_err_to_str(err));

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	if (conn != default_conn) {
		return;
	}

	printk("Connected: %s\n", addr);
	k_sem_give(&sem_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	bt_conn_unref(default_conn);
	default_conn = NULL;

	k_sem_give(&sem_disconnected);
}

static void security_changed_cb(struct bt_conn *conn, bt_security_t level,
				enum bt_security_err err)
{
	if (err == 0) {
		k_sem_give(&sem_security_updated);
	} else {
		printk("Failed to set security level: %s(%u)", bt_security_err_to_str(err), err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed_cb
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU exchanged: %u/%u\n", tx, rx);
	k_sem_give(&sem_mtu_exchanged);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated,
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

static void pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir,
			  const struct bt_audio_codec_cap *codec_cap)
{
	print_remote_codec_cap(codec_cap, dir);
}

static void endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	if (dir == BT_AUDIO_DIR_SOURCE) {
		add_remote_source(ep);
	} else if (dir == BT_AUDIO_DIR_SINK) {
		add_remote_sink(ep);
	}
}

static struct bt_bap_unicast_client_cb unicast_client_cbs = {
	.location = unicast_client_location_cb,
	.available_contexts = available_contexts_cb,
	.pac_record = pac_record_cb,
	.endpoint = endpoint_cb,
};

static int init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth enable failed (err %d)\n", err);
		return err;
	}

	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		streams[i].ops = &stream_ops;
	}

	bt_gatt_cb_register(&gatt_callbacks);

	if (IS_ENABLED(CONFIG_BT_AUDIO_TX)) {
		stream_tx_init();
	}

	return 0;
}

static int scan_and_connect(void)
{
	int err;

	start_scan();

	err = k_sem_take(&sem_connected, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_connected (err %d)\n", err);
		return err;
	}

	err = k_sem_take(&sem_mtu_exchanged, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_mtu_exchanged (err %d)\n", err);
		return err;
	}

	err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
	if (err != 0) {
		printk("failed to set security (err %d)\n", err);
		return err;
	}

	err = k_sem_take(&sem_security_updated, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_security_updated (err %d)\n", err);
		return err;
	}

	return 0;
}

static int discover_sinks(void)
{
	int err;

	unicast_client_cbs.discover = discover_sinks_cb;

	err = bt_bap_unicast_client_discover(default_conn, BT_AUDIO_DIR_SINK);
	if (err != 0) {
		printk("Failed to discover sinks: %d\n", err);
		return err;
	}

	err = k_sem_take(&sem_sinks_discovered, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_sinks_discovered (err %d)\n", err);
		return err;
	}

	return 0;
}

static int discover_sources(void)
{
	int err;

	unicast_client_cbs.discover = discover_sources_cb;

	err = bt_bap_unicast_client_discover(default_conn, BT_AUDIO_DIR_SOURCE);
	if (err != 0) {
		printk("Failed to discover sources: %d\n", err);
		return err;
	}

	err = k_sem_take(&sem_sources_discovered, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_sources_discovered (err %d)\n", err);
		return err;
	}

	return 0;
}

static int configure_stream(struct bt_bap_stream *stream, struct bt_bap_ep *ep)
{
	int err;

	err = bt_bap_stream_config(default_conn, stream, ep, &codec_configuration.codec_cfg);
	if (err != 0) {
		return err;
	}

	err = k_sem_take(&sem_stream_configured, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_stream_configured (err %d)\n", err);
		return err;
	}

	return 0;
}

static int configure_streams(void)
{
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(sinks); i++) {
		struct bt_bap_ep *ep = sinks[i].ep;
		struct bt_bap_stream *stream = &streams[i];

		if (ep == NULL) {
			continue;
		}

		err = configure_stream(stream, ep);
		if (err != 0) {
			printk("Could not configure sink stream[%zu]: %d\n",
			       i, err);
			return err;
		}

		printk("Configured sink stream[%zu]\n", i);
		configured_sink_stream_count++;
	}

	for (size_t i = 0; i < ARRAY_SIZE(sources); i++) {
		struct bt_bap_ep *ep = sources[i];
		struct bt_bap_stream *stream = &streams[i + configured_sink_stream_count];

		if (ep == NULL) {
			continue;
		}

		err = configure_stream(stream, ep);
		if (err != 0) {
			printk("Could not configure source stream[%zu]: %d\n",
			       i, err);
			return err;
		}

		printk("Configured source stream[%zu]\n", i);
		configured_source_stream_count++;
	}

	return 0;
}

static int create_group(void)
{
	const size_t params_count = MAX(configured_sink_stream_count,
					configured_source_stream_count);
	struct bt_bap_unicast_group_stream_pair_param pair_params[params_count];
	struct bt_bap_unicast_group_stream_param stream_params[configured_stream_count];
	struct bt_bap_unicast_group_param param;
	int err;

	for (size_t i = 0U; i < configured_stream_count; i++) {
		stream_params[i].stream = &streams[i];
		stream_params[i].qos = &codec_configuration.qos;
	}

	for (size_t i = 0U; i < params_count; i++) {
		if (i < configured_sink_stream_count) {
			pair_params[i].tx_param = &stream_params[i];
		} else {
			pair_params[i].tx_param = NULL;
		}

		if (i < configured_source_stream_count) {
			pair_params[i].rx_param = &stream_params[i + configured_sink_stream_count];
		} else {
			pair_params[i].rx_param = NULL;
		}
	}

	param.params = pair_params;
	param.params_count = params_count;
	param.packing = BT_ISO_PACKING_SEQUENTIAL;

	err = bt_bap_unicast_group_create(&param, &unicast_group);
	if (err != 0) {
		printk("Could not create unicast group (err %d)\n", err);
		return err;
	}

	return 0;
}

static int delete_group(void)
{
	int err;

	err = bt_bap_unicast_group_delete(unicast_group);
	if (err != 0) {
		printk("Could not create unicast group (err %d)\n", err);
		return err;
	}

	return 0;
}

static int set_stream_qos(void)
{
	int err;

	err = bt_bap_stream_qos(default_conn, unicast_group);
	if (err != 0) {
		printk("Unable to setup QoS: %d\n", err);
		return err;
	}

	for (size_t i = 0U; i < configured_stream_count; i++) {
		printk("QoS: waiting for %zu streams\n", configured_stream_count);
		err = k_sem_take(&sem_stream_qos, K_FOREVER);
		if (err != 0) {
			printk("failed to take sem_stream_qos (err %d)\n", err);
			return err;
		}
	}

	return 0;
}

static int enable_streams(void)
{
	for (size_t i = 0U; i < configured_stream_count; i++) {
		int err;

		err = bt_bap_stream_enable(&streams[i], codec_configuration.codec_cfg.meta,
					   codec_configuration.codec_cfg.meta_len);
		if (err != 0) {
			printk("Unable to enable stream: %d\n", err);
			return err;
		}

		err = k_sem_take(&sem_stream_enabled, K_FOREVER);
		if (err != 0) {
			printk("failed to take sem_stream_enabled (err %d)\n", err);
			return err;
		}
	}

	return 0;
}

static int connect_streams(void)
{
	for (size_t i = 0U; i < configured_stream_count; i++) {
		int err;

		k_sem_reset(&sem_stream_connected);

		err = bt_bap_stream_connect(&streams[i]);
		if (err == -EALREADY) {
			/* We have already connected a paired stream */
			continue;
		} else if (err != 0) {
			printk("Unable to start stream: %d\n", err);
			return err;
		}

		err = k_sem_take(&sem_stream_connected, K_FOREVER);
		if (err != 0) {
			printk("failed to take sem_stream_connected (err %d)\n", err);
			return err;
		}
	}

	return 0;
}

static enum bt_audio_dir stream_dir(const struct bt_bap_stream *stream)
{
	struct bt_bap_ep_info ep_info;
	int err;

	err = bt_bap_ep_get_info(stream->ep, &ep_info);
	if (err != 0) {
		printk("Failed to get ep info for %p: %d\n", stream, err);
		__ASSERT_NO_MSG(false);

		return 0;
	}

	return ep_info.dir;
}

static int start_streams(void)
{
	for (size_t i = 0U; i < configured_stream_count; i++) {
		struct bt_bap_stream *stream = &streams[i];
		int err;

		if (stream_dir(stream) == BT_AUDIO_DIR_SOURCE) {
			err = bt_bap_stream_start(&streams[i]);
			if (err != 0) {
				printk("Unable to start stream: %d\n", err);
				return err;
			}
		} /* Sink streams are started by the unicast server */

		err = k_sem_take(&sem_stream_started, K_FOREVER);
		if (err != 0) {
			printk("failed to take sem_stream_started (err %d)\n", err);
			return err;
		}
	}

	return 0;
}

static void reset_data(void)
{
	k_sem_reset(&sem_connected);
	k_sem_reset(&sem_disconnected);
	k_sem_reset(&sem_mtu_exchanged);
	k_sem_reset(&sem_security_updated);
	k_sem_reset(&sem_sinks_discovered);
	k_sem_reset(&sem_sources_discovered);
	k_sem_reset(&sem_stream_configured);
	k_sem_reset(&sem_stream_qos);
	k_sem_reset(&sem_stream_enabled);
	k_sem_reset(&sem_stream_started);
	k_sem_reset(&sem_stream_connected);

	configured_sink_stream_count = 0;
	configured_source_stream_count = 0;
	memset(sinks, 0, sizeof(sinks));
	memset(sources, 0, sizeof(sources));
}

int main(void)
{
	int err;

	printk("Initializing\n");
	err = init();
	if (err != 0) {
		return 0;
	}
	printk("Initialized\n");

	err = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
	if (err != 0) {
		printk("Failed to register client callbacks: %d", err);
		return 0;
	}

	while (true) {
		reset_data();

		printk("Waiting for connection\n");
		err = scan_and_connect();
		if (err != 0) {
			return 0;
		}
		printk("Connected\n");

		printk("Discovering sinks\n");
		err = discover_sinks();
		if (err != 0) {
			return 0;
		}
		printk("Sinks discovered\n");

		printk("Discovering sources\n");
		err = discover_sources();
		if (err != 0) {
			return 0;
		}
		printk("Sources discovered\n");

		printk("Configuring streams\n");
		err = configure_streams();
		if (err != 0) {
			return 0;
		}

		if (configured_stream_count == 0U) {
			printk("No streams were configured\n");
			return 0;
		}

		printk("Creating unicast group\n");
		err = create_group();
		if (err != 0) {
			return 0;
		}
		printk("Unicast group created\n");

		printk("Setting stream QoS\n");
		err = set_stream_qos();
		if (err != 0) {
			return 0;
		}
		printk("Stream QoS Set\n");

		printk("Enabling streams\n");
		err = enable_streams();
		if (err != 0) {
			return 0;
		}
		printk("Streams enabled\n");

		printk("Connecting streams\n");
		err = connect_streams();
		if (err != 0) {
			return 0;
		}
		printk("Streams connected\n");

		printk("Starting streams\n");
		err = start_streams();
		if (err != 0) {
			return 0;
		}
		printk("Streams started\n");

		/* Wait for disconnect */
		err = k_sem_take(&sem_disconnected, K_FOREVER);
		if (err != 0) {
			printk("failed to take sem_disconnected (err %d)\n", err);
			return 0;
		}

		printk("Deleting group\n");
		err = delete_group();
		if (err != 0) {
			return 0;
		}
		printk("Group deleted\n");
	}
}
