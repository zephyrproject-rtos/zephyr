/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include "cap_stream_tx.h"

LOG_MODULE_REGISTER(cap_handover, LOG_LEVEL_INF);

BUILD_ASSERT(CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT == 1,
	     "Application only supports a single subgroup");
BUILD_ASSERT(CONFIG_BT_MAX_CONN == 1, "Application only supports a single connection");
BUILD_ASSERT(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT == CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT,
	     "Must support the same number of unicast sink stream and broadcast source streams");

#define SEM_TIMEOUT K_SECONDS(5)

/* For simplicity we use the mandatory configuration 16_2_1 */
static struct bt_bap_lc3_preset unicast_preset_16_2_1 = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_bap_lc3_preset broadcast_preset_16_2_1 = BT_BAP_LC3_BROADCAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

static struct bt_cap_broadcast_source *broadcast_source;
static struct bt_cap_unicast_group *unicast_group;
static struct bt_le_ext_adv *ext_adv;
static uint32_t broadcast_id;

uint64_t total_tx_iso_packet_count; /* This value is exposed to test code */

/* Store the parameters globally so we can reuse the parameters when switching between unicast and
 * broadcast
 */
static struct bt_cap_initiator_broadcast_create_param broadcast_create_param;
static struct bt_cap_unicast_audio_start_param unicast_audio_start_param;
static struct bt_cap_unicast_group_param unicast_group_param;

/** Struct to contain information for a specific peer (CAP) device */
struct peer_config {
	/** Streams - Can either be unicast or broadcast */
	struct bt_cap_stream streams[CAP_STREAM_TX_MAX];

	/** Reference to the unicast endpoints */
	struct bt_bap_ep *unicast_eps[CAP_STREAM_TX_MAX];

	/** ACL connection object for the peer device */
	struct bt_conn *conn;

	/** Remote BIS sync states
	 *
	 * Bitfield where BIT(n) == 0b1 indicates that BIS index n-1 has been synced
	 */
	uint32_t bis_sync;

	/** src_id that represents the recv_state - Needed as recv_state is a pointer that may be
	 * updated by the stack
	 */
	uint8_t src_id;
} peer;

static K_SEM_DEFINE(sem_proc, 0, 1);
static K_SEM_DEFINE(sem_state_change, 0, 1);
static K_SEM_DEFINE(sem_mtu_exchanged, 0, 1);
static K_SEM_DEFINE(sem_security_changed, 0, 1);
static K_SEM_DEFINE(sem_broadcast_stopped, 1, 1);
static K_SEM_DEFINE(sem_receive_state_updated, 0, 1);
static K_SEM_DEFINE(sem_streams, 0, CAP_STREAM_TX_MAX);

static void stream_configured_cb(struct bt_bap_stream *stream,
				 const struct bt_bap_qos_cfg_pref *pref)
{
	LOG_INF("Configured stream %p", stream);

	LOG_INF("Remote preferences: unframed %s, phy %u, rtn %u, latency %u, pd_min %u, pd_max "
		"%u, pref_pd_min %u, pref_pd_max %u",
		pref->unframed_supported ? "supported" : "not supported", pref->phy, pref->rtn,
		pref->latency, pref->pd_min, pref->pd_max, pref->pref_pd_min, pref->pref_pd_max);
}

static void stream_qos_set_cb(struct bt_bap_stream *stream)
{
	LOG_INF("QoS set stream %p", stream);
}

static void stream_enabled_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Enabled stream %p", stream);
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(stream, struct bt_cap_stream, bap_stream);
	int err;

	LOG_INF("Started stream %p", stream);

	err = cap_stream_tx_register_stream(cap_stream);
	if (err != 0) {
		LOG_ERR("Failed to register %p for TX: %d", stream, err);
	}
}

static void stream_metadata_updated_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Metadata updated stream %p", stream);
}

static void stream_disabled_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Disabled stream %p", stream);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(stream, struct bt_cap_stream, bap_stream);
	int err;

	LOG_INF("Stopped stream %p with reason 0x%02X", stream, reason);

	err = cap_stream_tx_unregister_stream(cap_stream);
	if (err != 0) {
		LOG_ERR("Failed to unregister %p for TX: %d", stream, err);
	}
}

static void stream_released_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Released stream %p", stream);

	k_sem_give(&sem_streams);
}

static void stream_sent_cb(struct bt_bap_stream *stream)
{
	/* Triggered every time we have sent an HCI data packet to the controller */

	if ((total_tx_iso_packet_count % 100U) == 0U) {
		LOG_INF("Sent %llu HCI ISO data packets", total_tx_iso_packet_count);
	}

	total_tx_iso_packet_count++;
}

static void
bap_broadcast_assistant_recv_state_cb(struct bt_conn *conn, int err,
				      const struct bt_bap_scan_delegator_recv_state *state)
{
	struct bt_le_ext_adv_info adv_info;

	if (err != 0) {
		LOG_ERR("Unexpected error: %d", err);
		return;
	}

	if (state == NULL) {
		/* Ignore */
		return;
	}

	err = bt_le_ext_adv_get_info(ext_adv, &adv_info);
	__ASSERT(err == 0, "Failed to get adv info: %d", err);

	/* The broadcast_id, adv_sid and address type is the combination that makes a receive state
	 * unique as defined by the BAP spec
	 */
	if (state->broadcast_id == broadcast_id && state->adv_sid == adv_info.sid &&
	    state->addr.type == adv_info.addr->type) {
		peer.bis_sync = state->num_subgroups > 0 ? state->subgroups[0].bis_sync : 0U;
		peer.src_id = state->src_id;

		LOG_DBG("Received receive state notification from %p with src_id 0x%02X, pa_state "
			"%d and bis_state 0x%08X",
			(void *)conn, state->src_id, state->pa_sync_state, peer.bis_sync);

		k_sem_give(&sem_receive_state_updated);
	}
}

static void bap_broadcast_assistant_recv_state_removed_cb(struct bt_conn *conn, uint8_t src_id)
{
	if (src_id == peer.src_id) {
		LOG_DBG("Receive state removed");
		peer.bis_sync = 0U;
		peer.src_id = 0U;

		k_sem_give(&sem_receive_state_updated);
	}
}

static void bap_broadcast_assistant_discover_cb(struct bt_conn *conn, int err,
						uint8_t recv_state_count)
{
	if (err == 0) {
		LOG_DBG("BASS discover done with %u recv states", recv_state_count);
	} else {
		LOG_DBG("BASS discover failed (%d)", err);
	}

	k_sem_give(&sem_proc);
}

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
	ARRAY_FOR_EACH(peer.unicast_eps, i) {
		if (peer.unicast_eps[i] == NULL) {
			LOG_INF("Sink ep[%zu]: %p", i, (void *)ep);
			peer.unicast_eps[i] = ep;
			return;
		}
	}
}

static void discover_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	if (err != 0) {
		LOG_ERR("Discovering sinks failed: %d", err);
	} else {
		LOG_INF("Discover sinks complete");
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
	add_remote_sink(ep);
}

static int discover_sinks(void)
{
	int err;

	LOG_INF("Discovering sink ASEs");
	k_sem_reset(&sem_proc);

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

	return 0;
}

static int discover_bass(void)
{
	int err;

	k_sem_reset(&sem_proc);

	err = bt_bap_broadcast_assistant_discover(peer.conn);
	if (err != 0) {
		LOG_ERR("Failed to discover BASS on the sink: %d", err);
		return err;
	}

	err = k_sem_take(&sem_proc, SEM_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Timeout on BASS discover: %d", err);
		return err;
	}

	return 0;
}

static int unicast_group_create(void)
{
	/* Keep the parameters static as that is a requirement when doing broadcast to unicast */
	static struct bt_cap_unicast_group_stream_param sink_stream_params[CAP_STREAM_TX_MAX] = {0};
	static struct bt_cap_unicast_group_stream_pair_param pair_params[CAP_STREAM_TX_MAX] = {0};
	int err;

	(void)memset(sink_stream_params, 0, sizeof(sink_stream_params));
	(void)memset(pair_params, 0, sizeof(pair_params));
	(void)memset(&unicast_group_param, 0, sizeof(unicast_group_param));

	ARRAY_FOR_EACH(peer.streams, i) {
		if (peer.unicast_eps[i] == NULL) {
			break;
		}

		sink_stream_params[i].stream = &peer.streams[i];
		sink_stream_params[i].qos_cfg = &unicast_preset_16_2_1.qos;
		pair_params[i].tx_param = &sink_stream_params[i];

		unicast_group_param.params_count++;
	}

	unicast_group_param.params = pair_params;
	unicast_group_param.packing = BT_ISO_PACKING_SEQUENTIAL;

	err = bt_cap_unicast_group_create(&unicast_group_param, &unicast_group);
	if (err != 0) {
		LOG_ERR("Failed to create group: %d", err);
		return err;
	}

	LOG_INF("Created group");

	return 0;
}

static int unicast_group_delete(void)
{
	int err;

	err = bt_cap_unicast_group_delete(unicast_group);
	if (err != 0) {
		LOG_ERR("Failed to delete group: %d", err);
		return err;
	}
	unicast_group = NULL;

	LOG_INF("Deleted group");

	return 0;
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

	return 0;
}

static void unicast_to_broadcast_complete_cb(int err, struct bt_conn *conn,
					     struct bt_cap_unicast_group *group,
					     struct bt_cap_broadcast_source *source)
{
	if (err != 0) {
		LOG_ERR("Failed to handover unicast to broadcast (failing conn %p): %d", conn, err);

		err = bt_conn_disconnect(peer.conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err != 0) {
			LOG_ERR("Failed to disconnect connection: %d", err);
		}
	} else {
		LOG_DBG("Unicast to broadcast handover completed with new source %p", source);
		err = k_sem_take(&sem_broadcast_stopped, K_NO_WAIT);
		__ASSERT(err == 0, "Failed to take sem_broadcast_stopped");
	}

	/* Update the unicast group and broadcast source as they may have been created or deleted */
	unicast_group = group;
	broadcast_source = source;

	k_sem_give(&sem_proc);
}

static void broadcast_to_unicast_complete_cb(int err, struct bt_conn *conn,
					     struct bt_cap_broadcast_source *source,
					     struct bt_cap_unicast_group *group)
{
	if (err != 0) {
		LOG_ERR("Failed to handover broadcast to unicast (failing conn %p): %d", conn, err);

		err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err != 0) {
			LOG_ERR("Failed to disconnect connection: %d", err);
		}
	} else {
		LOG_DBG("Broadcast to unicast handover completed with new unicast group %p", group);
		k_sem_give(&sem_broadcast_stopped);
	}

	/* Update the unicast group and broadcast source as they may have been created or deleted */
	unicast_group = group;
	broadcast_source = source;

	k_sem_give(&sem_proc);
}

static int unicast_audio_start(void)
{
	static struct bt_cap_unicast_audio_start_stream_param stream_param[CAP_STREAM_TX_MAX];
	int err;

	LOG_INF("Starting streams");
	(void)memset(stream_param, 0, sizeof(stream_param));
	(void)memset(&unicast_audio_start_param, 0, sizeof(unicast_audio_start_param));

	ARRAY_FOR_EACH(peer.streams, i) {
		if (peer.unicast_eps[i] == NULL) {
			break;
		}

		stream_param[i].member.member = peer.conn;
		stream_param[i].stream = &peer.streams[i];
		stream_param[i].ep = peer.unicast_eps[i];
		stream_param[i].codec_cfg = &unicast_preset_16_2_1.codec_cfg;
		unicast_audio_start_param.count++;
	}

	unicast_audio_start_param.type = BT_CAP_SET_TYPE_AD_HOC;
	unicast_audio_start_param.stream_params = stream_param;

	k_sem_reset(&sem_proc);

	err = bt_cap_initiator_unicast_audio_start(&unicast_audio_start_param);
	if (err != 0) {
		LOG_ERR("Failed to start unicast audio: %d", err);
		return err;
	}

	err = k_sem_take(&sem_proc, SEM_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Timeout on unicast audio start: %d", err);
		return err;
	}

	return 0;
}

static void broadcast_stopped_cb(struct bt_cap_broadcast_source *source, uint8_t reason)
{
	k_sem_give(&sem_broadcast_stopped);
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
		(void)memset(peer.unicast_eps, 0, sizeof(peer.unicast_eps));

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
		LOG_ERR("Failed to set security level: %s(%d)", bt_security_err_to_str(sec_err),
			sec_err);

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
	const bt_addr_le_t *addr = user_data;
	char addr_str[BT_ADDR_LE_STR_LEN];
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

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_BAP_CONN_PARAM_RELAXED,
				&peer.conn);
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
		/* Check for CAS support in advertising data */
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
			return err;
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

static int setup_broadcast_adv(void)
{
	struct bt_le_adv_param ext_adv_param = *BT_BAP_ADV_PARAM_BROADCAST_FAST;
	int err;

	/* Zephyr Controller works best while Extended Advertising interval is a multiple
	 * of the ISO Interval minus 10 ms (max. advertising random delay). This is
	 * required to place the AUX_ADV_IND PDUs in a non-overlapping interval with the
	 * Broadcast ISO radio events.
	 */

	ext_adv_param.interval_min -= BT_GAP_MS_TO_ADV_INTERVAL(10U);
	ext_adv_param.interval_max -= BT_GAP_MS_TO_ADV_INTERVAL(10U);

	/* Create a non-connectable advertising set */
	err = bt_le_ext_adv_create(&ext_adv_param, NULL, &ext_adv);
	if (err != 0) {
		LOG_ERR("Unable to create extended advertising set: %d", err);
		return err;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(ext_adv, BT_BAP_PER_ADV_PARAM_BROADCAST_FAST);
	if (err != 0) {
		LOG_ERR("Failed to set periodic advertising parameters: %d", err);

		__maybe_unused const int del_err = bt_le_ext_adv_delete(ext_adv);

		__ASSERT(del_err == 0, "Failed to delete ext_adv: %d", del_err);

		return err;
	}

	return 0;
}

static int set_base_data(void)
{
	struct bt_data per_ad;
	int err;

	NET_BUF_SIMPLE_DEFINE(base_buf, BT_BASE_MAX_SIZE);

	err = bt_cap_initiator_broadcast_get_base(broadcast_source, &base_buf);
	if (err != 0) {
		LOG_ERR("Failed to get encoded BASE: %d", err);
		return err;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = bt_le_per_adv_set_data(ext_adv, &per_ad, 1);
	if (err != 0) {
		LOG_ERR("Failed to set periodic advertising data: %d", err);
		return err;
	}

	return 0;
}

static int handover_unicast_to_broadcast(void)
{
	static struct bt_cap_initiator_broadcast_stream_param stream_params[CAP_STREAM_TX_MAX];
	static struct bt_cap_initiator_broadcast_subgroup_param subgroup_param = {0};
	/* Struct containing the converted unicast group configuration */
	struct bt_cap_handover_unicast_to_broadcast_param param = {0};
	size_t stream_cnt = 0U;
	int err;

	ARRAY_FOR_EACH(stream_params, i) {
		struct bt_cap_stream *cap_stream = &peer.streams[i];
		const struct bt_bap_ep *ep = cap_stream->bap_stream.ep;
		struct bt_bap_ep_info ep_info;

		if (ep == NULL) {
			/* Not configured */
			continue;
		}

		err = bt_bap_ep_get_info(ep, &ep_info);
		__ASSERT(err == 0, "Failed to get endpoint info: %d", err);

		if (ep_info.state != BT_BAP_EP_STATE_STREAMING) {
			/* Not streaming - Handover is only applied to streaming streams */
			continue;
		}

		stream_params[stream_cnt].stream = cap_stream;
		stream_params[stream_cnt].data_len = 0U;
		stream_params[stream_cnt].data = NULL;

		stream_cnt++;
	}

	if (stream_cnt == 0U) {
		LOG_ERR("No streams can be handed over");
		return -ENODEV;
	}

	subgroup_param.stream_count = stream_cnt;
	subgroup_param.stream_params = stream_params;
	subgroup_param.codec_cfg = &broadcast_preset_16_2_1.codec_cfg;

	broadcast_create_param.subgroup_count = 1U;
	broadcast_create_param.subgroup_params = &subgroup_param;
	broadcast_create_param.qos = &broadcast_preset_16_2_1.qos;
	broadcast_create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	broadcast_create_param.encryption = false;

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.unicast_group = unicast_group;
	param.broadcast_create_param = &broadcast_create_param;
	param.ext_adv = ext_adv;
	param.pa_interval = BT_BAP_PA_INTERVAL_UNKNOWN;
	param.broadcast_id = broadcast_id;

	k_sem_reset(&sem_proc);

	LOG_INF("Handing over %u streams from unicast to broadcast", stream_cnt);

	err = bt_cap_handover_unicast_to_broadcast(&param);
	if (err != 0) {
		LOG_ERR("Failed to handover unicast audio to broadcast: %d", err);
		return err;
	}

	err = k_sem_take(&sem_proc, SEM_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Timeout on unicast to broadcast: %d", err);
		return err;
	}

	if (broadcast_source == NULL) {
		/* Handover failed */
		return -EBADMSG;
	}

	/* Enable Periodic Advertising -
	 * Extended advertising is started by bt_cap_handover_unicast_to_broadcast
	 */
	err = bt_le_per_adv_start(ext_adv);
	if (err != 0) {
		LOG_ERR("Failed to enable periodic advertising: %d", err);
		return err;
	}

	/* Update the Periodic Advertising data with the BASE from the new broadcast source */
	err = set_base_data();
	if (err != 0) {
		LOG_ERR("Failed to set BASE data: %d", err);
		return err;
	}

	LOG_INF("Waiting for remote device to synchronize to the PA and BIG");
	while (true) {
		size_t bis_sync_cnt;

		/* Use a high timeout here as syncing to the Periodic Advertising, receiving
		 * the BASE and the BIGInfo and finally syncing to the BIG may take a while
		 */
		err = k_sem_take(&sem_receive_state_updated, K_SECONDS(300));
		if (err != 0) {
			LOG_ERR("Timeout on receive state update: %d", err);
			return err;
		}

		/* Break once the receive state indicates a non-empty BIS */

		bis_sync_cnt = sys_count_bits(&peer.bis_sync, sizeof(peer.bis_sync));
		if (bis_sync_cnt > 0) {
			const uint8_t expected_bis_synced =
				broadcast_create_param.subgroup_params[0].stream_count;
			if (bis_sync_cnt != expected_bis_synced) {
				LOG_ERR("Remote synced to subgroup_params[0]: 0x%08X, "
					"but we expected %zu BIS syncs",
					peer.bis_sync,
					broadcast_create_param.subgroup_params[0].stream_count);

				return -EBADMSG;
			}

			break;
		}
	}

	return 0;
}

static int handover_broadcast_to_unicast(void)
{
	struct bt_cap_commander_broadcast_reception_stop_param reception_stop_param = {0};
	struct bt_cap_commander_broadcast_reception_stop_member_param member_param = {0};
	struct bt_cap_handover_broadcast_to_unicast_param param = {0};
	struct bt_le_ext_adv_info adv_info;
	int err;

	err = bt_le_ext_adv_get_info(ext_adv, &adv_info);
	__ASSERT(err == 0, "Failed to get adv info: %d", err);

	if (peer.bis_sync == 0U) {
		LOG_ERR("Remote device does not have receive state for our broadcast, cannot "
			"handover");
		return -ENODEV;
	}

	reception_stop_param.type = BT_CAP_SET_TYPE_AD_HOC;
	reception_stop_param.param = &member_param;
	reception_stop_param.count = 1;

	member_param.member.member = peer.conn;
	member_param.src_id = peer.src_id;
	member_param.num_subgroups = broadcast_create_param.subgroup_count;

	param.adv_type = adv_info.addr->type;
	param.adv_sid = adv_info.sid;
	param.broadcast_id = broadcast_id;
	param.broadcast_source = broadcast_source;
	param.unicast_group_param = &unicast_group_param;
	/* unicast_audio_start_param will still be valid here, so we can just reuse them */
	param.unicast_start_param = &unicast_audio_start_param;
	param.reception_stop_param = &reception_stop_param;

	k_sem_reset(&sem_proc);

	LOG_INF("Handing over %u streams from broadcast to unicast",
		unicast_audio_start_param.count);

	err = bt_cap_handover_broadcast_to_unicast(&param);
	if (err != 0) {
		LOG_ERR("Failed to handover broadcast audio to unicast: %d", err);
		return err;
	}

	err = k_sem_take(&sem_proc, SEM_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Timeout on broadcast to unicast: %d", err);
		return err;
	}

	/* Disable Periodic Advertising as broadcast has now stopped */
	err = bt_le_per_adv_stop(ext_adv);
	if (err != 0) {
		LOG_ERR("Failed to stop periodic advertising: %d", err);
		return err;
	}

	/* Disable Extended Advertising as broadcast has now stopped */
	err = bt_le_ext_adv_stop(ext_adv);
	if (err != 0) {
		LOG_ERR("Failed to stop extended advertising: %d", err);
		return err;
	}

	return 0;
}

static int init_cap_handover(void)
{
	static struct bt_bap_broadcast_assistant_cb broadcast_assistant_cbs = {
		.discover = bap_broadcast_assistant_discover_cb,
		.recv_state = bap_broadcast_assistant_recv_state_cb,
		.recv_state_removed = bap_broadcast_assistant_recv_state_removed_cb,
	};
	static struct bt_bap_unicast_client_cb unicast_client_cbs = {
		.discover = discover_cb,
		.pac_record = pac_record_cb,
		.endpoint = endpoint_cb,
	};
	static struct bt_cap_initiator_cb cap_initiator_cb = {
		.unicast_discovery_complete = cap_discovery_complete_cb,
		.unicast_start_complete = unicast_start_complete_cb,
		.broadcast_stopped = broadcast_stopped_cb,
	};
	static struct bt_cap_handover_cb cap_handover_cb = {
		.unicast_to_broadcast_complete = unicast_to_broadcast_complete_cb,
		.broadcast_to_unicast_complete = broadcast_to_unicast_complete_cb,
	};
	static struct bt_gatt_cb gatt_callbacks = {
		.att_mtu_updated = att_mtu_updated_cb,
	};
	static struct bt_le_scan_cb scan_callbacks = {
		.recv = scan_recv_cb,
	};
	static struct bt_bap_stream_ops stream_ops = {
		.configured = stream_configured_cb,
		.qos_set = stream_qos_set_cb,
		.enabled = stream_enabled_cb,
		.started = stream_started_cb,
		.metadata_updated = stream_metadata_updated_cb,
		.disabled = stream_disabled_cb,
		.stopped = stream_stopped_cb,
		.released = stream_released_cb,
		.sent = stream_sent_cb,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("Bluetooth enable failed: %d", err);

		return err;
	}

	err = bt_cap_initiator_register_cb(&cap_initiator_cb);
	if (err != 0) {
		LOG_ERR("Failed to register CAP initiator callbacks: %d", err);

		return err;
	}

	err = bt_cap_handover_register_cb(&cap_handover_cb);
	if (err != 0) {
		LOG_ERR("Failed to register CAP handover callbacks: %d", err);

		return err;
	}

	err = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
	if (err != 0) {
		LOG_ERR("Failed to register BAP unicast client callbacks: %d", err);

		return err;
	}

	err = bt_bap_broadcast_assistant_register_cb(&broadcast_assistant_cbs);
	if (err != 0) {
		LOG_ERR("Failed to register broadcast assistant callbacks: %d", err);
		return err;
	}

	ARRAY_FOR_EACH_PTR(peer.streams, stream) {
		bt_cap_stream_ops_register(stream, &stream_ops);
	}

	bt_gatt_cb_register(&gatt_callbacks);
	bt_le_scan_cb_register(&scan_callbacks);

	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK)) {
		cap_stream_tx_init();
	}

#if defined(CONFIG_SAMPLE_STATIC_BROADCAST_ID)
	broadcast_id = CONFIG_SAMPLE_BROADCAST_ID;
#else
	broadcast_id = 0U;
	err = bt_rand(&broadcast_id, BT_AUDIO_BROADCAST_ID_SIZE);
	if (err != 0) {
		LOG_ERR("Failed to generate broadcast ID: %d", err);
		return err;
	}
	LOG_INF("Generated broadcast_id: %06X", broadcast_id);
#endif /* CONFIG_SAMPLE_STATIC_BROADCAST_ID */

	err = setup_broadcast_adv();
	if (err != 0) {
		LOG_ERR("Failed to setup broadcast adv: %d", err);
		return err;
	}

	return 0;
}

static int reset_cap_handover(void)
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

	/* If there are streams with active endpoints, we wait until they have been released */
	ARRAY_FOR_EACH_PTR(peer.streams, stream) {
		if (stream->bap_stream.ep != NULL) {
			err = k_sem_take(&sem_streams, SEM_TIMEOUT);
			if (err != 0) {
				LOG_ERR("Timeout on sem_streams: %d", err);
				return err;
			}
		}
	}

	if (unicast_group != NULL) {
		err = unicast_group_delete();
		if (err != 0) {
			return err;
		}
	}

	if (broadcast_source != NULL) {
		/* Stop broadcast if active - Since this is an asynchronous operation we have to
		 * wait for the broadcast stop to complete via sem_broadcast_stopped.
		 */
		if (k_sem_count_get(&sem_broadcast_stopped) == 0U) {
			err = bt_cap_initiator_broadcast_audio_stop(broadcast_source);
			if (err != 0) {
				LOG_ERR("Could not stop broadcast source: %d", err);
				return err;
			}

			err = k_sem_take(&sem_broadcast_stopped, SEM_TIMEOUT);
			if (err != 0) {
				LOG_ERR("Timeout on broadcast stop: %d", err);
				return err;
			}
		}

		err = bt_cap_initiator_broadcast_audio_delete(broadcast_source);
		if (err != 0) {
			LOG_ERR("Could not delete broadcast source: %d", err);
			return err;
		}
		broadcast_source = NULL;
	}

	k_sem_reset(&sem_proc);
	k_sem_reset(&sem_state_change);
	k_sem_reset(&sem_mtu_exchanged);
	k_sem_reset(&sem_security_changed);
	k_sem_reset(&sem_receive_state_updated);
	k_sem_reset(&sem_streams);

	/* Since sem_broadcast_stopped needs to be reset to 1, we use k_sem_give instead of
	 * k_sem_reset
	 */
	k_sem_give(&sem_broadcast_stopped);

	total_tx_iso_packet_count = 0U;

	return 0;
}

int main(void)
{
	int err;

	err = init_cap_handover();
	if (err != 0) {
		LOG_ERR("Initialization failed: %d", err);

		return 0;
	}

	LOG_INF("Bluetooth initialized");

	while (true) {
		err = reset_cap_handover();
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

		/* Discover the BASS so that we can add our broadcast source to the acceptor */
		err = discover_bass();
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

		while (true) {
			/* Switch between unicast and broadcast */

			/* Wait between switches */
			k_sleep(K_SECONDS(10));

			err = handover_unicast_to_broadcast();
			if (err != 0) {
				break;
			}

			/* Wait between switches */
			k_sleep(K_SECONDS(10));

			err = handover_broadcast_to_unicast();
			if (err != 0) {
				break;
			}
		}

		/* If handover failed, we attempt to disconnect if not already */
		if (peer.conn != NULL) {
			err = bt_conn_disconnect(peer.conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			if (err != 0) {
				LOG_ERR("Failed to disconnect: %d", err);
				continue;
			}
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
