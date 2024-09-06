/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "cap_initiator.h"

LOG_MODULE_REGISTER(cap_initiator_unicast, LOG_LEVEL_INF);

#define SEM_TIMEOUT K_SECONDS(5)

/* We use the same config for both sink and source streams
 * For simplicity we use the mandatory configuration 16_2_1
 */
static struct bt_bap_lc3_preset unicast_preset_16_2_1 = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_bap_unicast_group *unicast_group;
uint64_t total_rx_iso_packet_count; /* This value is exposed to test code */
uint64_t total_unicast_tx_iso_packet_count; /* This value is exposed to test code */

/** Struct to contain information for a specific peer (CAP) device */
struct peer_config {
	/** Stream for the source endpoint */
	struct bt_cap_stream source_stream;
	/** Stream for the sink endpoint */
	struct bt_cap_stream sink_stream;
	/** Semaphore to help wait for a release operation if the source stream is not idle */
	struct k_sem source_stream_sem;
	/** Semaphore to help wait for a release operation if the sink stream is not idle */
	struct k_sem sink_stream_sem;
	/** Reference to the endpoint for the source stream */
	struct bt_bap_ep *source_ep;
	/** Reference to the endpoint for the sink stream */
	struct bt_bap_ep *sink_ep;
	/** ACL connection object for the peer device */
	struct bt_conn *conn;
	/** Current sequence number for TX */
	uint16_t tx_seq_num;
};

/* TODO: Expand to multiple ACL connections */
static struct peer_config peer;

static K_SEM_DEFINE(sem_proc, 0, 1);
static K_SEM_DEFINE(sem_state_change, 0, 1);
static K_SEM_DEFINE(sem_mtu_exchanged, 0, 1);
static K_SEM_DEFINE(sem_security_changed, 0, 1);

static bool is_tx_stream(struct bt_bap_stream *stream)
{
	struct bt_bap_ep_info ep_info;
	int err;

	err = bt_bap_ep_get_info(stream->ep, &ep_info);
	if (err != 0) {
		LOG_ERR("Failed to get ep info: %d", err);

		return false;
	}

	return ep_info.dir == BT_AUDIO_DIR_SINK;
}

static void unicast_stream_configured_cb(struct bt_bap_stream *stream,
					 const struct bt_audio_codec_qos_pref *pref)
{
	LOG_INF("Configured stream %p", stream);

	/* TODO: The preference should be used/taken into account when
	 * setting the QoS
	 */

	LOG_INF("Remote preferences: unframed %s, phy %u, rtn %u, latency %u, pd_min %u, pd_max "
		"%u, pref_pd_min %u, pref_pd_max %u",
		pref->unframed_supported ? "supported" : "not supported", pref->phy, pref->rtn,
		pref->latency, pref->pd_min, pref->pd_max, pref->pref_pd_min, pref->pref_pd_max);
}

static void unicast_stream_qos_set_cb(struct bt_bap_stream *stream)
{
	LOG_INF("QoS set stream %p", stream);
}

static void unicast_stream_enabled_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Enabled stream %p", stream);
}

static void unicast_stream_started_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Started stream %p", stream);
	total_rx_iso_packet_count = 0U;
	total_unicast_tx_iso_packet_count = 0U;

	if (is_tx_stream(stream)) {
		struct bt_cap_stream *cap_stream =
			CONTAINER_OF(stream, struct bt_cap_stream, bap_stream);
		int err;

		err = cap_initiator_tx_register_stream(cap_stream);
		if (err != 0) {
			LOG_ERR("Failed to register %p for TX: %d", stream, err);
		}
	}
}

static void unicast_stream_metadata_updated_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Metadata updated stream %p", stream);
}

static void unicast_stream_disabled_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Disabled stream %p", stream);
}

static void unicast_stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	LOG_INF("Stopped stream %p with reason 0x%02X", stream, reason);

	if (is_tx_stream(stream)) {
		struct bt_cap_stream *cap_stream =
			CONTAINER_OF(stream, struct bt_cap_stream, bap_stream);
		int err;

		err = cap_initiator_tx_unregister_stream(cap_stream);
		if (err != 0) {
			LOG_ERR("Failed to unregister %p for TX: %d", stream, err);
		}
	}
}

static void unicast_stream_released_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Released stream %p", stream);

	if (stream == &peer.source_stream.bap_stream) {
		k_sem_give(&peer.source_stream_sem);
	} else if (stream == &peer.sink_stream.bap_stream) {
		k_sem_give(&peer.sink_stream_sem);
	}
}

static void unicast_stream_recv_cb(struct bt_bap_stream *stream,
				   const struct bt_iso_recv_info *info, struct net_buf *buf)
{
	/* Triggered every time we receive an HCI data packet from the controller.
	 * A call to this does not indicate valid data
	 * (see the `info->flags` for which flags to check),
	 */

	if ((total_rx_iso_packet_count % 100U) == 0U) {
		LOG_INF("Received %llu HCI ISO data packets", total_rx_iso_packet_count);
	}

	total_rx_iso_packet_count++;
}

static void unicast_stream_sent_cb(struct bt_bap_stream *stream)
{
	/* Triggered every time we have sent an HCI data packet to the controller */

	if ((total_unicast_tx_iso_packet_count % 100U) == 0U) {
		LOG_INF("Sent %llu HCI ISO data packets", total_unicast_tx_iso_packet_count);
	}

	total_unicast_tx_iso_packet_count++;
}

static struct bt_bap_stream_ops unicast_stream_ops = {
	.configured = unicast_stream_configured_cb,
	.qos_set = unicast_stream_qos_set_cb,
	.enabled = unicast_stream_enabled_cb,
	.started = unicast_stream_started_cb,
	.metadata_updated = unicast_stream_metadata_updated_cb,
	.disabled = unicast_stream_disabled_cb,
	.stopped = unicast_stream_stopped_cb,
	.released = unicast_stream_released_cb,
	.recv = unicast_stream_recv_cb,
	.sent = unicast_stream_sent_cb,
};

static bool log_codec_cb(struct bt_data *data, void *user_data)
{
	const char *str = (const char *)user_data;

	LOG_DBG("\t%s: type 0x%02x value_len %u", str, data->type, data->data_len);
	LOG_HEXDUMP_DBG(data->data, data->data_len, "\t\tdata");

	return true;
}

static void log_codec(const struct bt_audio_codec_cap *codec_cap, enum bt_audio_dir dir)
{
	LOG_INF("codec id 0x%02x cid 0x%04x vid 0x%04x count %u", codec_cap->id, codec_cap->cid,
		codec_cap->vid, codec_cap->data_len);

	if (codec_cap->id == BT_HCI_CODING_FORMAT_LC3) {
		bt_audio_data_parse(codec_cap->data, codec_cap->data_len, log_codec_cb, "data");
	} else { /* If not LC3, we cannot assume it's LTV */
		LOG_HEXDUMP_DBG(codec_cap->data, codec_cap->data_len, "data");
	}

	bt_audio_data_parse(codec_cap->meta, codec_cap->meta_len, log_codec_cb, "meta");
}

static void add_remote_sink(struct bt_bap_ep *ep)
{
	if (peer.sink_ep == NULL) {
		LOG_INF("Sink ep: %p", (void *)ep);
		peer.sink_ep = ep;
		return;
	}
}

static void add_remote_source(struct bt_bap_ep *ep)
{
	if (peer.source_ep == NULL) {
		LOG_INF("Source ep: %p", (void *)ep);
		peer.source_ep = ep;
		return;
	}
}

static void discover_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	if (dir == BT_AUDIO_DIR_SINK) {
		if (err != 0) {
			LOG_ERR("Discovery sinks failed: %d", err);
		} else {
			LOG_INF("Discover sinks complete");
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		if (err != 0) {
			LOG_ERR("Discovery sources failed: %d", err);
		} else {
			LOG_INF("Discover sources complete");
		}
	}

	k_sem_give(&sem_proc);
}

static void pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir,
			  const struct bt_audio_codec_cap *codec_cap)
{
	log_codec(codec_cap, dir);
}

static void endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	if (dir == BT_AUDIO_DIR_SOURCE) {
		add_remote_source(ep);
	} else if (dir == BT_AUDIO_DIR_SINK) {
		add_remote_sink(ep);
	}
}

static int discover_sinks(void)
{
	int err;

	LOG_INF("Discovering sink ASEs");
	k_sem_reset(&sem_proc);

	bt_cap_stream_ops_register(&peer.sink_stream, &unicast_stream_ops);

	err = bt_bap_unicast_client_discover(peer.conn, BT_AUDIO_DIR_SINK);
	if (err != 0) {
		LOG_ERR("Failed to discover sink: %d", err);
		return err;
	}

	err = k_sem_take(&sem_proc, SEM_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Timeout on sinks discover: %d", err);
		return err;
	}

	return err;
}

static int discover_sources(void)
{
	int err;

	LOG_INF("Discovering source ASEs");
	k_sem_reset(&sem_proc);

	bt_cap_stream_ops_register(&peer.source_stream, &unicast_stream_ops);

	err = bt_bap_unicast_client_discover(peer.conn, BT_AUDIO_DIR_SOURCE);
	if (err != 0) {
		LOG_ERR("Failed to discover sources: %d", err);
		return err;
	}

	err = k_sem_take(&sem_proc, SEM_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Timeout on sources discover: %d", err);
		return err;
	}

	return 0;
}

static int unicast_group_create(void)
{
	struct bt_bap_unicast_group_stream_param source_stream_param = {
		.qos = &unicast_preset_16_2_1.qos,
		.stream = &peer.source_stream.bap_stream,
	};
	struct bt_bap_unicast_group_stream_param sink_stream_param = {
		.qos = &unicast_preset_16_2_1.qos,
		.stream = &peer.sink_stream.bap_stream,
	};
	struct bt_bap_unicast_group_stream_pair_param pair_params = {0};
	struct bt_bap_unicast_group_param group_param = {0};
	int err;

	if (peer.source_ep != NULL) {
		pair_params.rx_param = &source_stream_param;
	}

	if (peer.sink_ep != NULL) {
		pair_params.tx_param = &sink_stream_param;
	}

	group_param.params_count = 1U;
	group_param.params = &pair_params;

	err = bt_bap_unicast_group_create(&group_param, &unicast_group);
	if (err != 0) {
		LOG_ERR("Failed to create group: %d", err);
		return err;
	}

	LOG_INF("Created group");

	return err;
}

static int unicast_group_delete(void)
{
	int err;

	err = bt_bap_unicast_group_delete(unicast_group);
	if (err != 0) {
		LOG_ERR("Failed to delete group: %d", err);
		return err;
	}
	unicast_group = NULL;

	LOG_INF("Deleted group");

	return err;
}

static void cap_discovery_complete_cb(struct bt_conn *conn, int err,
				      const struct bt_csip_set_coordinator_set_member *member,
				      const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		LOG_ERR("CAS discovery completed with error: %d", err);

		return;
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER) && csis_inst != NULL) {
		LOG_INF("Found CAS with CSIS %p", csis_inst);
		/* TODO: Do set member discovery */
	} else {
		LOG_INF("Found CAS");
	}

	k_sem_give(&sem_proc);
}

static void unicast_start_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		LOG_ERR("Failed to start (failing conn %p): %d", (void *)conn, err);
		return;
	}

	k_sem_give(&sem_proc);
}

static int discover_cas(void)
{
	int err;

	LOG_INF("Discovering CAS");
	k_sem_reset(&sem_proc);

	err = bt_cap_initiator_unicast_discover(peer.conn);
	if (err != 0) {
		LOG_ERR("Failed to discover CAS: %d", err);
		return err;
	}

	err = k_sem_take(&sem_proc, SEM_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Timeout on CAS discover: %d", err);
		return err;
	}

	return err;
}

static int unicast_audio_start(void)
{
	/* TODO: Expand to start multiple streams on multiple CAP acceptors */
	struct bt_cap_unicast_audio_start_stream_param stream_param[2] = {0};
	struct bt_cap_unicast_audio_start_param param = {0};
	int err;

	LOG_INF("Starting streams");

	if (peer.sink_ep != NULL) {
		stream_param[param.count].member.member = peer.conn;
		stream_param[param.count].stream = &peer.sink_stream;
		stream_param[param.count].ep = peer.sink_ep;
		stream_param[param.count].codec_cfg = &unicast_preset_16_2_1.codec_cfg;
		param.count++;
	}

	if (peer.source_ep != NULL) {
		stream_param[param.count].member.member = peer.conn;
		stream_param[param.count].stream = &peer.source_stream;
		stream_param[param.count].ep = peer.source_ep;
		stream_param[param.count].codec_cfg = &unicast_preset_16_2_1.codec_cfg;
		param.count++;
	}

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.stream_params = stream_param;

	err = bt_cap_initiator_unicast_audio_start(&param);
	if (err != 0) {
		LOG_ERR("Failed to start unicast audio: %d", err);
		return err;
	}

	return err;
}

static void att_mtu_updated_cb(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	LOG_INF("MTU exchanged: %u/%u", tx, rx);
	k_sem_give(&sem_mtu_exchanged);
}

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		LOG_ERR("Scanning failed to start: %d", err);
		return;
	}

	LOG_INF("Scanning successfully started");
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		LOG_ERR("Failed to connect to %s: %u", addr, err);

		bt_conn_unref(peer.conn);
		peer.conn = NULL;

		start_scan();
		return;
	}

	if (conn != peer.conn) {
		return;
	}

	LOG_INF("Connected: %s", addr);
	k_sem_give(&sem_state_change);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != peer.conn) {
		return;
	}

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s, reason 0x%02x %s", addr, reason, bt_hci_err_to_str(reason));

	bt_conn_unref(peer.conn);
	peer.conn = NULL;

	k_sem_give(&sem_state_change);
}

static void security_changed_cb(struct bt_conn *conn, bt_security_t level,
				enum bt_security_err sec_err)
{
	if (sec_err == 0) {
		LOG_INF("Security changed: %u", level);
		k_sem_give(&sem_security_changed);
	} else {
		LOG_ERR("Failed to set security level: %s(%d)",
			bt_security_err_to_str(sec_err), sec_err);

		if (sec_err == BT_SECURITY_ERR_PIN_OR_KEY_MISSING) {
			int err;

			LOG_INF("Removing old key");
			err = bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(conn));
			if (err != 0) {
				LOG_ERR("Failed to remove old key: %d", err);
			}
		}
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.security_changed = security_changed_cb,
};

static bool check_audio_support_and_connect_cb(struct bt_data *data, void *user_data)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t *addr = user_data;
	const struct bt_uuid *uuid;
	uint16_t uuid_val;
	int err;

	LOG_DBG("[AD]: %u data_len %u", data->type, data->data_len);

	if (data->type != BT_DATA_SVC_DATA16) {
		return true; /* Continue parsing to next AD data type */
	}

	if (data->data_len < sizeof(uuid_val)) {
		LOG_DBG("AD invalid size %u", data->data_len);
		return true; /* Continue parsing to next AD data type */
	}

	/* We are looking for the CAS service data */
	uuid_val = sys_get_le16(data->data);
	uuid = BT_UUID_DECLARE_16(uuid_val);
	if (bt_uuid_cmp(uuid, BT_UUID_CAS) != 0) {
		return true; /* Continue parsing to next AD data type */
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	LOG_INF("Attempt to connect to %s", addr_str);

	err = bt_le_scan_stop();
	if (err != 0) {
		LOG_ERR("Failed to stop scan: %d", err);
		return false;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &peer.conn);
	if (err != 0) {
		LOG_WRN("Create conn to failed: %d, restarting scan", err);
		start_scan();
	}

	return false; /* Stop parsing */
}

static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	if (peer.conn != NULL) {
		/* Already connected */
		return;
	}

	/* Check for connectable, extended advertising */
	if (((info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0) &&
	    ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE)) != 0) {
		/* Check for TMAS support in advertising data */
		bt_data_parse(buf, check_audio_support_and_connect_cb, (void *)info->addr);
	}
}

static int scan_and_connect(void)
{
	int err;

	start_scan();

	err = k_sem_take(&sem_state_change, K_FOREVER);
	if (err != 0) {
		LOG_ERR("Failed to take sem_state_change: %d", err);
		return err;
	}

	return 0;
}

static void exchange_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (err == BT_ATT_ERR_SUCCESS) {
		LOG_INF("MTU exchange done");
		k_sem_give(&sem_proc);
	} else {
		LOG_ERR("MTU exchange failed: err %u", err);
	}
}

static int exchange_mtu(void)
{
	int err;

	if (!IS_ENABLED(CONFIG_BT_GATT_AUTO_UPDATE_MTU)) {
		static struct bt_gatt_exchange_params exchange_params = {
			.func = exchange_cb,
		};

		LOG_INF("Exchanging MTU");

		k_sem_reset(&sem_proc);

		err = bt_gatt_exchange_mtu(peer.conn, &exchange_params);
		if (err != 0) {
			LOG_ERR("Failed to exchange MTU: %d", err);
		}

		err = k_sem_take(&sem_proc, SEM_TIMEOUT);
		if (err != 0) {
			LOG_ERR("Timeout on MTU exchange request: %d", err);
			return err;
		}
	}

	LOG_INF("Waiting for MTU exchange");
	err = k_sem_take(&sem_mtu_exchanged, SEM_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Timeout on MTU exchange: %d", err);
		return err;
	}

	return 0;
}

static int update_security(void)
{
	int err;

	err = bt_conn_set_security(peer.conn, BT_SECURITY_L2);
	if (err != 0) {
		LOG_ERR("Failed to set security: %d", err);
		return err;
	}

	LOG_INF("Waiting for security change");
	err = k_sem_take(&sem_security_changed, SEM_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Timeout on security: %d", err);
		return err;
	}

	return 0;
}

static int init_cap_initiator(void)
{
	static struct bt_bap_unicast_client_cb unicast_client_cbs = {
		.discover = discover_cb,
		.pac_record = pac_record_cb,
		.endpoint = endpoint_cb,
	};
	static struct bt_cap_initiator_cb cap_cb = {
		.unicast_discovery_complete = cap_discovery_complete_cb,
		.unicast_start_complete = unicast_start_complete_cb,
	};
	static struct bt_gatt_cb gatt_callbacks = {
		.att_mtu_updated = att_mtu_updated_cb,
	};
	static struct bt_le_scan_cb scan_callbacks = {
		.recv = scan_recv_cb,
	};
	int err;

	err = bt_cap_initiator_register_cb(&cap_cb);
	if (err != 0) {
		LOG_ERR("Failed to register CAP callbacks: %d", err);

		return err;
	}

	err = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
	if (err != 0) {
		LOG_ERR("Failed to register BAP unicast client callbacks: %d", err);

		return err;
	}

	bt_gatt_cb_register(&gatt_callbacks);
	bt_le_scan_cb_register(&scan_callbacks);

	k_sem_init(&peer.source_stream_sem, 0, 1);
	k_sem_init(&peer.sink_stream_sem, 0, 1);

	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK)) {
		cap_initiator_tx_init();
	}

	return 0;
}

static int reset_cap_initiator(void)
{
	int err;

	LOG_INF("Resetting");

	if (peer.conn != NULL) {
		err = bt_conn_disconnect(peer.conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err != 0) {
			return err;
		}

		err = k_sem_take(&sem_state_change, K_FOREVER);
		if (err != 0) {
			LOG_ERR("Timeout on disconnect: %d", err);
			return err;
		}
	}

	if (peer.source_stream.bap_stream.ep != NULL) {
		err = k_sem_take(&peer.source_stream_sem, SEM_TIMEOUT);
		if (err != 0) {
			LOG_ERR("Timeout on source_stream_sem: %d", err);
			return err;
		}
	}

	if (peer.sink_stream.bap_stream.ep != NULL) {
		err = k_sem_take(&peer.sink_stream_sem, SEM_TIMEOUT);
		if (err != 0) {
			LOG_ERR("Timeout on sink_stream_sem: %d", err);
			return err;
		}
	}

	if (unicast_group != NULL) {
		int err;

		err = unicast_group_delete();
		if (err != 0) {
			return err;
		}

		return err;
	}

	peer.source_ep = NULL;
	peer.sink_ep = NULL;
	k_sem_reset(&sem_proc);
	k_sem_reset(&sem_state_change);
	k_sem_reset(&sem_mtu_exchanged);

	return 0;
}

int cap_initiator_unicast(void)
{
	int err;

	err = init_cap_initiator();
	if (err != 0) {
		return err;
	}

	LOG_INF("CAP initiator unicast initialized");

	while (true) {
		err = reset_cap_initiator();
		if (err != 0) {
			LOG_ERR("Failed to reset");

			return err;
		}

		/* Start scanning for and connecting to CAP Acceptors. CAP Acceptors are identified
		 * by their advertising data
		 */
		err = scan_and_connect();
		if (err != 0) {
			continue;
		}

		/* BAP mandates support for an MTU of at least 65 octets. Because of that, we
		 * should always exchange the MTU before accessing BAP related services to ensure
		 * correctness
		 */
		err = exchange_mtu();
		if (err != 0) {
			continue;
		}

		/* LE Audio services require encryption with LE Secure Connections, so we increase
		 * security before attempting to do any LE Audio operations
		 */
		err = update_security();
		if (err != 0) {
			continue;
		}

		/* All remote CAP Acceptors shall have the Common Audio Service (CAS) so we start by
		 * discovering that on the remote device to determine if they are really CAP
		 * Acceptors. If they are only a BAP Unicast Server we ignore them.
		 */
		err = discover_cas();
		if (err != 0) {
			continue;
		}

		/* Discover sink ASEs and capabilities. This may not result in any endpoints if they
		 * remote device is only a source (e.g. a microphone)
		 */
		err = discover_sinks();
		if (err != 0) {
			continue;
		}

		/* Discover source ASEs and capabilities. This may not result in any endpoints if
		 * they remote device is only a sink (e.g. a speaker)
		 */
		err = discover_sources();
		if (err != 0) {
			continue;
		}

		/* Create a unicast group (Connected Isochronous Group (CIG)) based on what we have
		 * found on the remote device
		 */
		err = unicast_group_create();
		if (err != 0) {
			continue;
		}

		/* Execute the start operation which will take one or more streams into the
		 * streaming state
		 */
		err = unicast_audio_start();
		if (err != 0) {
			continue;
		}

		/* Reset if disconnected */
		err = k_sem_take(&sem_state_change, K_FOREVER);
		if (err != 0) {
			LOG_ERR("Failed to take sem_state_change: err %d", err);

			return err;
		}
	}

	return 0;
}
