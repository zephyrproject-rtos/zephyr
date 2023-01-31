/* @file
 * @brief Bluetooth Unicast Client
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>

#include "../host/hci_core.h"
#include "../host/conn_internal.h"
#include "../host/iso_internal.h"

#include "bap_iso.h"
#include "audio_internal.h"
#include "bap_endpoint.h"
#include "pacs_internal.h"
#include "bap_unicast_client_internal.h"

#include <zephyr/logging/log.h>

BUILD_ASSERT(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 ||
		     CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0,
	     "CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT or "
	     "CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT shall be non-zero");

LOG_MODULE_REGISTER(bt_bap_unicast_client, CONFIG_BT_BAP_UNICAST_CLIENT_LOG_LEVEL);

#define PAC_DIR_UNUSED(dir) ((dir) != BT_AUDIO_DIR_SINK && (dir) != BT_AUDIO_DIR_SOURCE)

struct bt_bap_unicast_client_ep {
	uint16_t handle;
	uint16_t cp_handle;
	struct bt_gatt_subscribe_params subscribe;
	struct bt_gatt_discover_params discover;
	struct bt_bap_ep ep;

	/* Bool to help handle different order of CP and ASE notification when releasing */
	bool release_requested;
};

static const struct bt_uuid *snk_uuid = BT_UUID_PACS_SNK;
static const struct bt_uuid *src_uuid = BT_UUID_PACS_SRC;
static const struct bt_uuid *pacs_context_uuid = BT_UUID_PACS_SUPPORTED_CONTEXT;
static const struct bt_uuid *pacs_snk_loc_uuid = BT_UUID_PACS_SNK_LOC;
static const struct bt_uuid *pacs_src_loc_uuid = BT_UUID_PACS_SRC_LOC;
static const struct bt_uuid *pacs_avail_ctx_uuid = BT_UUID_PACS_AVAILABLE_CONTEXT;
static const struct bt_uuid *ase_snk_uuid = BT_UUID_ASCS_ASE_SNK;
static const struct bt_uuid *ase_src_uuid = BT_UUID_ASCS_ASE_SRC;
static const struct bt_uuid *cp_uuid = BT_UUID_ASCS_ASE_CP;

static struct bt_bap_unicast_group unicast_groups[UNICAST_GROUP_CNT];

static struct unicast_client {
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	struct bt_bap_unicast_client_ep snks[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	struct bt_bap_unicast_client_ep srcs[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

	struct bt_gatt_subscribe_params cp_subscribe;
	struct bt_gatt_subscribe_params snk_loc_subscribe;
	struct bt_gatt_subscribe_params src_loc_subscribe;
	struct bt_gatt_subscribe_params avail_ctx_subscribe;

	/* TODO: We should be able to reduce the number of discover params for
	 * CCCD discovery, but requires additional work if it is to support an
	 * arbitrary number of EATT bearers, as control point and location CCCD
	 * discovery needs to be done in serial to avoid using the same discover
	 * parameters twice
	 */
	struct bt_gatt_discover_params loc_cc_disc;
	struct bt_gatt_discover_params avail_ctx_cc_disc;
} uni_cli_insts[CONFIG_BT_MAX_CONN];

static const struct bt_bap_unicast_client_cb *unicast_client_cbs;

/* TODO: Move the functions to avoid these prototypes */
static int unicast_client_ep_set_metadata(struct bt_bap_ep *ep, void *data, uint8_t len,
					  struct bt_codec *codec);

static int unicast_client_ep_set_codec(struct bt_bap_ep *ep, uint8_t id, uint16_t cid, uint16_t vid,
				       void *data, uint8_t len, struct bt_codec *codec);
static int unicast_client_ep_start(struct bt_bap_ep *ep,
				   struct net_buf_simple *buf);

static void unicast_client_reset(struct bt_bap_ep *ep);

static int unicast_client_send_start(struct bt_bap_ep *ep)
{
	if (ep->receiver_ready != true || ep->dir != BT_AUDIO_DIR_SOURCE) {
		LOG_DBG("Invalid ep %p %u %s",
			ep, ep->receiver_ready, bt_audio_dir_str(ep->dir));

		return -EINVAL;
	}

	struct bt_ascs_start_op *req;
	struct net_buf_simple *buf;
	int err;

	buf = bt_bap_unicast_client_ep_create_pdu(BT_ASCS_START_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 1U;

	err = unicast_client_ep_start(ep, buf);
	if (err != 0) {
		LOG_DBG("unicast_client_ep_start failed: %d",
			err);

		return err;
	}

	err = bt_bap_unicast_client_ep_send(ep->stream->conn, ep, buf);
	if (err != 0) {
		LOG_DBG("bt_bap_unicast_client_ep_send failed: %d", err);

		return err;
	}

	return 0;
}

static int unicast_client_ep_idle_state(struct bt_bap_ep *ep);

/** Checks if the stream can terminate the CIS
 *
 * If the CIS is used for another stream, or if the CIS is not in the connected
 * state it will return false.
 */
static bool unicast_client_can_disconnect_stream(const struct bt_bap_stream *stream)
{
	const struct bt_bap_ep *stream_ep;
	enum bt_iso_state iso_state;

	if (stream == NULL) {
		return false;
	}

	stream_ep = stream->ep;

	if (stream_ep == NULL || stream_ep->iso == NULL) {
		return false;
	}

	iso_state = stream_ep->iso->chan.state;

	if (iso_state == BT_ISO_STATE_CONNECTED || iso_state == BT_ISO_STATE_CONNECTING) {
		const struct bt_bap_ep *pair_ep;

		pair_ep = bt_bap_iso_get_paired_ep(stream_ep);

		/* If there are no paired endpoint, or the paired endpoint is
		 * not in the streaming state, we can disconnect the CIS
		 */
		if (pair_ep == NULL || pair_ep->status.state != BT_BAP_EP_STATE_STREAMING) {
			return true;
		}
	}

	return false;
}

static struct bt_bap_stream *audio_stream_by_ep_id(const struct bt_conn *conn,
						     uint8_t id)
{
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 || CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	const uint8_t conn_index = bt_conn_index(conn);
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 ||                                        \
	* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0                                           \
	*/

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(uni_cli_insts[conn_index].snks); i++) {
		const struct bt_bap_unicast_client_ep *client_ep =
			&uni_cli_insts[conn_index].snks[i];

		if (client_ep->ep.status.id == id) {
			return client_ep->ep.stream;
		}
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(uni_cli_insts[conn_index].srcs); i++) {
		const struct bt_bap_unicast_client_ep *client_ep =
			&uni_cli_insts[conn_index].srcs[i];

		if (client_ep->ep.status.id == id) {
			return client_ep->ep.stream;
		}
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

	return NULL;
}

#if defined(CONFIG_BT_AUDIO_RX)
static void unicast_client_ep_iso_recv(struct bt_iso_chan *chan,
				       const struct bt_iso_recv_info *info, struct net_buf *buf)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);
	const struct bt_bap_stream_ops *ops;
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep = iso->rx.ep;

	if (ep == NULL) {
		/* In the case that the CIS has been setup as bidirectional, and
		 * only one of the directions have an ASE configured yet,
		 * we should only care about valid ISO packets when doing this
		 * check. The reason is that some controllers send HCI ISO data
		 * packets to the host, even if no SDU was sent on the remote
		 * side. This basically means that empty PDUs are sent to the
		 * host as HCI ISO data packets, which we should just ignore
		 */
		if ((info->flags & BT_ISO_FLAGS_VALID) != 0) {
			LOG_ERR("iso %p not bound with ep", chan);
		}

		return;
	}

	if (IS_ENABLED(CONFIG_BT_BAP_DEBUG_STREAM_DATA) &&
	    ep->status.state != BT_BAP_EP_STATE_STREAMING) {
		LOG_DBG("ep %p is not in the streaming state: %s", ep,
			bt_bap_ep_state_str(ep->status.state));
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	ops = stream->ops;

	if (IS_ENABLED(CONFIG_BT_BAP_DEBUG_STREAM_DATA)) {
		LOG_DBG("stream %p ep %p len %zu", stream, ep, net_buf_frags_len(buf));
	}

	if (ops != NULL && ops->recv != NULL) {
		ops->recv(stream, info, buf);
	} else {
		LOG_WRN("No callback for recv set");
	}
}
#endif /* CONFIG_BT_AUDIO_RX */

#if defined(CONFIG_BT_AUDIO_TX)
static void unicast_client_ep_iso_sent(struct bt_iso_chan *chan)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep = iso->tx.ep;

	if (ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_BAP_DEBUG_STREAM_DATA)) {
		LOG_DBG("stream %p ep %p", stream, ep);
	}

	if (stream->ops != NULL && stream->ops->sent != NULL) {
		stream->ops->sent(stream);
	}
}
#endif /* CONFIG_BT_AUDIO_TX */

static void unicast_client_ep_iso_connected(struct bt_bap_ep *ep)
{
	struct bt_bap_stream *stream;

	if (ep->status.state != BT_BAP_EP_STATE_ENABLING) {
		LOG_DBG("endpoint not in enabling state: %s",
			bt_bap_ep_state_str(ep->status.state));
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	LOG_DBG("stream %p ep %p dir %s receiver_ready %u",
		stream, ep, bt_audio_dir_str(ep->dir), ep->receiver_ready);

	if (ep->receiver_ready && ep->dir == BT_AUDIO_DIR_SOURCE) {
		const int err = unicast_client_send_start(ep);

		if (err != 0) {
			LOG_DBG("Failed to send start: %d", err);

			/* TBD: Should we release the stream then? */
			ep->receiver_ready = false;
		}
	}
}

static void unicast_client_iso_connected(struct bt_iso_chan *chan)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);

	if (iso->rx.ep == NULL && iso->tx.ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	if (iso->rx.ep != NULL) {
		unicast_client_ep_iso_connected(iso->rx.ep);
	}

	if (iso->tx.ep != NULL) {
		unicast_client_ep_iso_connected(iso->tx.ep);
	}
}

static void unicast_client_ep_iso_disconnected(struct bt_bap_ep *ep, uint8_t reason)
{
	struct bt_bap_stream *stream;

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("Stream not associated with an ep");
		return;
	}

	LOG_DBG("stream %p ep %p reason 0x%02x", stream, ep, reason);

	/* If we were in the idle state when we started the ISO disconnection
	 * then we need to call unicast_client_ep_idle_state again when
	 * the ISO has finalized the disconnection
	 */
	if (ep->status.state == BT_BAP_EP_STATE_IDLE) {

		(void)unicast_client_ep_idle_state(ep);

		if (stream->conn != NULL) {
			struct bt_conn_info conn_info;
			int err;

			err = bt_conn_get_info(stream->conn, &conn_info);
			if (err != 0 || conn_info.state == BT_CONN_STATE_DISCONNECTED) {
				/* Retrigger the reset of the EP if the ACL is disconnected before
				 * the ISO is disconnected
				 */
				unicast_client_reset(ep);
			}
		}
	} else {
		if (stream->ops != NULL && stream->ops->stopped != NULL) {
			stream->ops->stopped(stream, reason);
		} else {
			LOG_WRN("No callback for stopped set");
		}
	}
}

static void unicast_client_iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);

	if (iso->rx.ep == NULL && iso->tx.ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	if (iso->rx.ep != NULL) {
		unicast_client_ep_iso_disconnected(iso->rx.ep, reason);
	}

	if (iso->tx.ep != NULL) {
		unicast_client_ep_iso_disconnected(iso->tx.ep, reason);
	}
}

static struct bt_iso_chan_ops unicast_client_iso_ops = {
#if defined(CONFIG_BT_AUDIO_RX)
	.recv = unicast_client_ep_iso_recv,
#endif /* CONFIG_BT_AUDIO_RX */
#if defined(CONFIG_BT_AUDIO_TX)
	.sent = unicast_client_ep_iso_sent,
#endif /* CONFIG_BT_AUDIO_TX */
	.connected = unicast_client_iso_connected,
	.disconnected = unicast_client_iso_disconnected,
};

bool bt_bap_ep_is_unicast_client(const struct bt_bap_ep *ep)
{
	for (size_t i = 0U; i < ARRAY_SIZE(uni_cli_insts); i++) {
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
		if (PART_OF_ARRAY(uni_cli_insts[i].snks, ep)) {
			return true;
		}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
		if (PART_OF_ARRAY(uni_cli_insts[i].srcs, ep)) {
			return true;
		}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
	}

	return false;
}

static void unicast_client_ep_init(struct bt_bap_ep *ep, uint16_t handle, uint8_t dir)
{
	struct bt_bap_unicast_client_ep *client_ep;

	LOG_DBG("ep %p dir %s handle 0x%04x", ep, bt_audio_dir_str(dir), handle);

	client_ep = CONTAINER_OF(ep, struct bt_bap_unicast_client_ep, ep);

	(void)memset(ep, 0, sizeof(*ep));
	client_ep->handle = handle;
	ep->status.id = 0U;
	ep->dir = dir;
}

static struct bt_bap_ep *unicast_client_ep_find(struct bt_conn *conn, uint16_t handle)
{
	uint8_t index;

	index = bt_conn_index(conn);

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(uni_cli_insts[index].snks); i++) {
		struct bt_bap_unicast_client_ep *client_ep = &uni_cli_insts[index].snks[i];

		if ((handle && client_ep->handle == handle) || (!handle && client_ep->handle)) {
			return &client_ep->ep;
		}
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(uni_cli_insts[index].srcs); i++) {
		struct bt_bap_unicast_client_ep *client_ep = &uni_cli_insts[index].srcs[i];

		if ((handle && client_ep->handle == handle) || (!handle && client_ep->handle)) {
			return &client_ep->ep;
		}
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

	return NULL;
}

struct bt_bap_iso *bt_bap_unicast_client_new_audio_iso(void)
{
	struct bt_bap_iso *bap_iso;

	bap_iso = bt_bap_iso_new();
	if (bap_iso == NULL) {
		return NULL;
	}

	bt_bap_iso_init(bap_iso, &unicast_client_iso_ops);

	LOG_DBG("New bap_iso %p", bap_iso);

	return bap_iso;
}

static struct bt_bap_ep *unicast_client_ep_new(struct bt_conn *conn, enum bt_audio_dir dir,
					       uint16_t handle)
{
	size_t i, size;
	uint8_t index;
	struct bt_bap_unicast_client_ep *cache;

	index = bt_conn_index(conn);

	switch (dir) {
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	case BT_AUDIO_DIR_SINK:
		cache = uni_cli_insts[index].snks;
		size = ARRAY_SIZE(uni_cli_insts[index].snks);
		break;
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	case BT_AUDIO_DIR_SOURCE:
		cache = uni_cli_insts[index].srcs;
		size = ARRAY_SIZE(uni_cli_insts[index].srcs);
		break;
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
	default:
		return NULL;
	}

	for (i = 0; i < size; i++) {
		struct bt_bap_unicast_client_ep *client_ep = &cache[i];

		if (!client_ep->handle) {
			unicast_client_ep_init(&client_ep->ep, handle, dir);
			return &client_ep->ep;
		}
	}

	return NULL;
}

static struct bt_bap_ep *unicast_client_ep_get(struct bt_conn *conn, enum bt_audio_dir dir,
					       uint16_t handle)
{
	struct bt_bap_ep *ep;

	ep = unicast_client_ep_find(conn, handle);
	if (ep || !handle) {
		return ep;
	}

	return unicast_client_ep_new(conn, dir, handle);
}

static int unicast_client_ep_idle_state(struct bt_bap_ep *ep)
{
	struct bt_bap_unicast_client_ep *client_ep =
		CONTAINER_OF(ep, struct bt_bap_unicast_client_ep, ep);
	struct bt_bap_stream *stream = ep->stream;
	const struct bt_bap_stream_ops *ops;

	if (stream == NULL) {
		return -EINVAL;
	}

	/* If CIS is connected, disconnect and wait for CIS disconnection */
	if (unicast_client_can_disconnect_stream(stream)) {
		int err;

		LOG_DBG("Disconnecting stream");
		err = bt_bap_stream_disconnect(stream);

		if (err != 0) {
			LOG_ERR("Failed to disconnect stream: %d", err);
		}

		return err;
	} else if (ep->iso != NULL && ep->iso->chan.state == BT_ISO_STATE_DISCONNECTING) {
		/* Wait */
		return -EBUSY;
	}

	bt_bap_stream_reset(stream);

	/* Notify upper layer */
	if (client_ep->release_requested) {
		/* In case that we get the idle state notification before the CP notification we
		 * trigger the CP callback now, as after this we won't be able to find the stream
		 * by the ASE ID
		 */
		client_ep->release_requested = false;

		if (unicast_client_cbs != NULL && unicast_client_cbs->release != NULL) {
			unicast_client_cbs->release(stream, BT_BAP_ASCS_RSP_CODE_SUCCESS,
						    BT_BAP_ASCS_REASON_NONE);
		}
	}

	ops = stream->ops;
	if (ops != NULL && ops->released != NULL) {
		ops->released(stream);
	} else {
		LOG_WRN("No callback for released set");
	}

	return 0;
}

static void unicast_client_ep_qos_update(struct bt_bap_ep *ep,
					 const struct bt_ascs_ase_status_qos *qos)
{
	struct bt_iso_chan_io_qos *iso_io_qos;

	LOG_DBG("ep %p dir %s bap_iso %p", ep, bt_audio_dir_str(ep->dir), ep->iso);

	if (ep->dir == BT_AUDIO_DIR_SOURCE) {
		/* If the endpoint is a source, then we need to
		 * reset our RX parameters
		 */
		iso_io_qos = &ep->iso->rx.qos;
	} else if (ep->dir == BT_AUDIO_DIR_SINK) {
		/* If the endpoint is a sink, then we need to
		 * reset our TX parameters
		 */
		iso_io_qos = &ep->iso->tx.qos;
	} else {
		__ASSERT(false, "Invalid ep->dir: %u", ep->dir);
		return;
	}

	iso_io_qos->phy = qos->phy;
	iso_io_qos->sdu = sys_le16_to_cpu(qos->sdu);
	iso_io_qos->rtn = qos->rtn;
}

static void unicast_client_ep_config_state(struct bt_bap_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_config *cfg;
	struct bt_codec_qos_pref *pref;
	struct bt_bap_stream *stream;
	void *cc;

	if (buf->len < sizeof(*cfg)) {
		LOG_ERR("Config status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_WRN("No stream active for endpoint");
		return;
	}

	cfg = net_buf_simple_pull_mem(buf, sizeof(*cfg));

	if (stream->codec == NULL) {
		LOG_ERR("Stream %p does not have a codec configured", stream);
		return;
	} else if (stream->codec->id != cfg->codec.id) {
		LOG_ERR("Codec configuration mismatched: %u, %u", stream->codec->id, cfg->codec.id);
		/* TODO: Release the stream? */
		return;
	}

	if (buf->len < cfg->cc_len) {
		LOG_ERR("Malformed ASE Config status: buf->len %u < %u cc_len", buf->len,
			cfg->cc_len);
		return;
	}

	cc = net_buf_simple_pull_mem(buf, cfg->cc_len);

	pref = &stream->ep->qos_pref;

	/* Convert to interval representation so they can be matched by QoS */
	pref->unframed_supported = cfg->framing == BT_ASCS_QOS_FRAMING_UNFRAMED;
	pref->phy = cfg->phy;
	pref->rtn = cfg->rtn;
	pref->latency = sys_le16_to_cpu(cfg->latency);
	pref->pd_min = sys_get_le24(cfg->pd_min);
	pref->pd_max = sys_get_le24(cfg->pd_max);
	pref->pref_pd_min = sys_get_le24(cfg->prefer_pd_min);
	pref->pref_pd_max = sys_get_le24(cfg->prefer_pd_max);

	LOG_DBG("dir %s unframed_supported 0x%02x phy 0x%02x rtn %u "
		"latency %u pd_min %u pd_max %u pref_pd_min %u pref_pd_max %u codec 0x%02x ",
		bt_audio_dir_str(ep->dir), pref->unframed_supported, pref->phy, pref->rtn,
		pref->latency, pref->pd_min, pref->pd_max, pref->pref_pd_min, pref->pref_pd_max,
		stream->codec->id);

	unicast_client_ep_set_codec(ep, cfg->codec.id, sys_le16_to_cpu(cfg->codec.cid),
				    sys_le16_to_cpu(cfg->codec.vid), cc, cfg->cc_len, NULL);

	/* Notify upper layer */
	if (stream->ops != NULL && stream->ops->configured != NULL) {
		stream->ops->configured(stream, pref);
	} else {
		LOG_WRN("No callback for configured set");
	}
}

static void unicast_client_ep_qos_state(struct bt_bap_ep *ep, struct net_buf_simple *buf,
					uint8_t old_state)
{
	struct bt_ascs_ase_status_qos *qos;
	struct bt_bap_stream *stream;

	if (buf->len < sizeof(*qos)) {
		LOG_ERR("QoS status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream active for endpoint");
		return;
	}

	if (ep->dir == BT_AUDIO_DIR_SINK && stream->ops != NULL && stream->ops->disabled != NULL) {
		/* If the old state was enabling or streaming, then the sink
		 * ASE has been disabled. Since the sink ASE does not have a
		 * disabling state, we can check if by comparing the old_state
		 */
		const bool disabled = old_state == BT_BAP_EP_STATE_ENABLING ||
				      old_state == BT_BAP_EP_STATE_STREAMING;

		if (disabled) {
			stream->ops->disabled(stream);
		}
	}

	qos = net_buf_simple_pull_mem(buf, sizeof(*qos));

	/* Update existing QoS configuration */
	unicast_client_ep_qos_update(ep, qos);

	ep->cig_id = qos->cig_id;
	ep->cis_id = qos->cis_id;
	(void)memcpy(&stream->qos->interval, sys_le24_to_cpu(qos->interval), sizeof(qos->interval));
	stream->qos->framing = qos->framing;
	stream->qos->phy = qos->phy;
	stream->qos->sdu = sys_le16_to_cpu(qos->sdu);
	stream->qos->rtn = qos->rtn;
	stream->qos->latency = sys_le16_to_cpu(qos->latency);
	(void)memcpy(&stream->qos->pd, sys_le24_to_cpu(qos->pd), sizeof(qos->pd));

	LOG_DBG("dir %s cig 0x%02x cis 0x%02x codec 0x%02x interval %u "
		"framing 0x%02x phy 0x%02x rtn %u latency %u pd %u",
		bt_audio_dir_str(ep->dir), ep->cig_id, ep->cis_id, stream->codec->id,
		stream->qos->interval, stream->qos->framing, stream->qos->phy, stream->qos->rtn,
		stream->qos->latency, stream->qos->pd);

	/* Disconnect ISO if connected */
	if (unicast_client_can_disconnect_stream(stream)) {
		const int err = bt_bap_stream_disconnect(stream);

		if (err != 0) {
			LOG_ERR("Failed to disconnect stream: %d", err);
		}
	} else {
		/* We setup the data path here, as this is the earliest where
		 * we have the ISO <-> EP coupling completed (due to setting
		 * the CIS ID in the QoS procedure).
		 */
		if (ep->dir == BT_AUDIO_DIR_SOURCE) {
			/* If the endpoint is a source, then we need to
			 * configure our RX parameters
			 */
			bt_audio_codec_to_iso_path(&ep->iso->rx.path, stream->codec);
		} else {
			/* If the endpoint is a sink, then we need to
			 * configure our TX parameters
			 */
			bt_audio_codec_to_iso_path(&ep->iso->tx.path, stream->codec);
		}
	}

	/* Notify upper layer */
	if (stream->ops != NULL && stream->ops->qos_set != NULL) {
		stream->ops->qos_set(stream);
	} else {
		LOG_WRN("No callback for qos_set set");
	}
}

static void unicast_client_ep_enabling_state(struct bt_bap_ep *ep, struct net_buf_simple *buf,
					     bool state_changed)
{
	struct bt_ascs_ase_status_enable *enable;
	struct bt_bap_stream *stream;
	void *metadata;

	if (buf->len < sizeof(*enable)) {
		LOG_ERR("Enabling status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream active for endpoint");
		return;
	}

	enable = net_buf_simple_pull_mem(buf, sizeof(*enable));

	if (buf->len < enable->metadata_len) {
		LOG_ERR("Malformed PDU: remaining len %u expected %u", buf->len,
			enable->metadata_len);
		return;
	}

	metadata = net_buf_simple_pull_mem(buf, enable->metadata_len);

	LOG_DBG("dir %s cig 0x%02x cis 0x%02x", bt_audio_dir_str(ep->dir), ep->cig_id, ep->cis_id);

	unicast_client_ep_set_metadata(ep, metadata, enable->metadata_len, NULL);

	/* Notify upper layer
	 *
	 * If the state did not change then only the metadata was changed
	 */
	if (state_changed) {
		if (stream->ops != NULL && stream->ops->enabled != NULL) {
			stream->ops->enabled(stream);
		} else {
			LOG_WRN("No callback for enabled set");
		}
	} else {
		if (stream->ops != NULL && stream->ops->metadata_updated != NULL) {
			stream->ops->metadata_updated(stream);
		} else {
			LOG_WRN("No callback for metadata_updated set");
		}
	}
}

static void unicast_client_ep_streaming_state(struct bt_bap_ep *ep, struct net_buf_simple *buf,
					      bool state_changed)
{
	struct bt_ascs_ase_status_stream *stream_status;
	struct bt_bap_stream *stream;

	if (buf->len < sizeof(*stream_status)) {
		LOG_ERR("Streaming status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream active for endpoint");
		return;
	}

	stream_status = net_buf_simple_pull_mem(buf, sizeof(*stream_status));

	LOG_DBG("dir %s cig 0x%02x cis 0x%02x", bt_audio_dir_str(ep->dir), ep->cig_id, ep->cis_id);

	/* Notify upper layer
	 *
	 * If the state did not change then only the metadata was changed
	 */
	if (state_changed) {
		if (stream->ops != NULL && stream->ops->started != NULL) {
			stream->ops->started(stream);
		} else {
			LOG_WRN("No callback for started set");
		}
	} else {
		if (stream->ops != NULL && stream->ops->metadata_updated != NULL) {
			stream->ops->metadata_updated(stream);
		} else {
			LOG_WRN("No callback for metadata_updated set");
		}
	}
}

static void unicast_client_ep_disabling_state(struct bt_bap_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_disable *disable;
	struct bt_bap_stream *stream;

	if (buf->len < sizeof(*disable)) {
		LOG_ERR("Disabling status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream active for endpoint");
		return;
	}

	disable = net_buf_simple_pull_mem(buf, sizeof(*disable));

	LOG_DBG("dir %s cig 0x%02x cis 0x%02x", bt_audio_dir_str(ep->dir), ep->cig_id, ep->cis_id);

	/* Notify upper layer */
	if (stream->ops != NULL && stream->ops->disabled != NULL) {
		stream->ops->disabled(stream);
	} else {
		LOG_WRN("No callback for disabled set");
	}
}

static void unicast_client_ep_releasing_state(struct bt_bap_ep *ep, struct net_buf_simple *buf)
{
	struct bt_bap_stream *stream;

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream active for endpoint");
		return;
	}

	LOG_DBG("dir %s", bt_audio_dir_str(ep->dir));

	if (unicast_client_can_disconnect_stream(stream)) {
		/* The Unicast Client shall terminate any CIS established for
		 * that ASE by following the Connected Isochronous Stream
		 * Terminate procedure defined in Volume 3, Part C,
		 * Section 9.3.15 in when the Unicast Client has determined
		 * that the ASE is in the Releasing state.
		 */
		const int err = bt_bap_stream_disconnect(stream);

		if (err != 0) {
			LOG_ERR("Failed to disconnect stream: %d", err);
		}
	}
}

static void unicast_client_ep_set_status(struct bt_bap_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status *status;
	struct bt_bap_unicast_client_ep *client_ep;
	bool state_changed;
	uint8_t old_state;

	if (!ep) {
		return;
	}

	client_ep = CONTAINER_OF(ep, struct bt_bap_unicast_client_ep, ep);

	status = net_buf_simple_pull_mem(buf, sizeof(*status));

	old_state = ep->status.state;
	ep->status = *status;
	state_changed = old_state != ep->status.state;

	LOG_DBG("ep %p handle 0x%04x id 0x%02x dir %s state %s -> %s", ep, client_ep->handle,
		status->id, bt_audio_dir_str(ep->dir), bt_bap_ep_state_str(old_state),
		bt_bap_ep_state_str(status->state));

	switch (status->state) {
	case BT_BAP_EP_STATE_IDLE:
		(void)unicast_client_ep_idle_state(ep);
		break;
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x00 (Idle) */
		case BT_BAP_EP_STATE_IDLE:
			/* or 0x01 (Codec Configured) */
		case BT_BAP_EP_STATE_CODEC_CONFIGURED:
			/* or 0x02 (QoS Configured) */
		case BT_BAP_EP_STATE_QOS_CONFIGURED:
			/* or 0x06 (Releasing) */
		case BT_BAP_EP_STATE_RELEASING:
			break;
		default:
			LOG_WRN("Invalid state transition: %s -> %s",
				bt_bap_ep_state_str(old_state),
				bt_bap_ep_state_str(ep->status.state));
			return;
		}

		unicast_client_ep_config_state(ep, buf);
		break;
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		/* QoS configured have different allowed states depending on the endpoint type */
		if (ep->dir == BT_AUDIO_DIR_SOURCE) {
			switch (old_state) {
			/* Valid only if ASE_State field = 0x01 (Codec Configured) */
			case BT_BAP_EP_STATE_CODEC_CONFIGURED:
			/* or 0x02 (QoS Configured) */
			case BT_BAP_EP_STATE_QOS_CONFIGURED:
			/* or 0x05 (Disabling) */
			case BT_BAP_EP_STATE_DISABLING:
				break;
			default:
				LOG_WRN("Invalid state transition: %s -> %s",
					bt_bap_ep_state_str(old_state),
					bt_bap_ep_state_str(ep->status.state));
				return;
			}
		} else {
			switch (old_state) {
			/* Valid only if ASE_State field = 0x01 (Codec Configured) */
			case BT_BAP_EP_STATE_CODEC_CONFIGURED:
			/* or 0x02 (QoS Configured) */
			case BT_BAP_EP_STATE_QOS_CONFIGURED:
			/* or 0x03 (Enabling) */
			case BT_BAP_EP_STATE_ENABLING:
			/* or 0x04 (Streaming)*/
			case BT_BAP_EP_STATE_STREAMING:
				break;
			default:
				LOG_WRN("Invalid state transition: %s -> %s",
					bt_bap_ep_state_str(old_state),
					bt_bap_ep_state_str(ep->status.state));
				return;
			}
		}

		unicast_client_ep_qos_state(ep, buf, old_state);
		break;
	case BT_BAP_EP_STATE_ENABLING:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x02 (QoS Configured) */
		case BT_BAP_EP_STATE_QOS_CONFIGURED:
			/* or 0x03 (Enabling) */
		case BT_BAP_EP_STATE_ENABLING:
			break;
		default:
			LOG_WRN("Invalid state transition: %s -> %s",
				bt_bap_ep_state_str(old_state),
				bt_bap_ep_state_str(ep->status.state));
			return;
		}

		unicast_client_ep_enabling_state(ep, buf, state_changed);
		break;
	case BT_BAP_EP_STATE_STREAMING:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x03 (Enabling)*/
		case BT_BAP_EP_STATE_ENABLING:
			/* or 0x04 (Streaming)*/
		case BT_BAP_EP_STATE_STREAMING:
			break;
		default:
			LOG_WRN("Invalid state transition: %s -> %s",
				bt_bap_ep_state_str(old_state),
				bt_bap_ep_state_str(ep->status.state));
			return;
		}

		unicast_client_ep_streaming_state(ep, buf, state_changed);
		break;
	case BT_BAP_EP_STATE_DISABLING:
		if (ep->dir == BT_AUDIO_DIR_SOURCE) {
			switch (old_state) {
			/* Valid only if ASE_State field = 0x03 (Enabling) */
			case BT_BAP_EP_STATE_ENABLING:
			/* or 0x04 (Streaming) */
			case BT_BAP_EP_STATE_STREAMING:
				break;
			default:
				LOG_WRN("Invalid state transition: %s -> %s",
					bt_bap_ep_state_str(old_state),
					bt_bap_ep_state_str(ep->status.state));
				return;
			}
		} else {
			/* Sinks cannot go into the disabling state */
			LOG_WRN("Invalid state transition: %s -> %s",
				bt_bap_ep_state_str(old_state),
				bt_bap_ep_state_str(ep->status.state));
			return;
		}

		unicast_client_ep_disabling_state(ep, buf);
		break;
	case BT_BAP_EP_STATE_RELEASING:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x01 (Codec Configured) */
		case BT_BAP_EP_STATE_CODEC_CONFIGURED:
			/* or 0x02 (QoS Configured) */
		case BT_BAP_EP_STATE_QOS_CONFIGURED:
			/* or 0x03 (Enabling) */
		case BT_BAP_EP_STATE_ENABLING:
			/* or 0x04 (Streaming) */
		case BT_BAP_EP_STATE_STREAMING:
			break;
			/* or 0x04 (Disabling) */
		case BT_BAP_EP_STATE_DISABLING:
			if (ep->dir == BT_AUDIO_DIR_SOURCE) {
				break;
			} /* else fall through for sink */

			/* fall through */
		default:
			LOG_WRN("Invalid state transition: %s -> %s",
				bt_bap_ep_state_str(old_state),
				bt_bap_ep_state_str(ep->status.state));
			return;
		}

		unicast_client_ep_releasing_state(ep, buf);
		break;
	}
}

static void unicast_client_codec_data_add(struct net_buf_simple *buf, const char *prefix,
					  size_t num, const struct bt_codec_data *data)
{
	for (size_t i = 0; i < num; i++) {
		const struct bt_data *d = &data[i].data;
		struct bt_ascs_codec_config *cc;

		LOG_DBG("#%u: %s type 0x%02x len %u", i, prefix, d->type, d->data_len);
		LOG_HEXDUMP_DBG(d->data, d->data_len, prefix);

		cc = net_buf_simple_add(buf, sizeof(*cc));
		cc->len = d->data_len + sizeof(cc->type);
		cc->type = d->type;
		net_buf_simple_add_mem(buf, d->data, d->data_len);
	}
}

static bool unicast_client_codec_config_store(struct bt_data *data, void *user_data)
{
	struct bt_codec *codec = user_data;
	struct bt_codec_data *cdata;

	if (codec->data_count >= ARRAY_SIZE(codec->data)) {
		LOG_ERR("No slot available for Codec Config");
		return false;
	}

	cdata = &codec->data[codec->data_count];

	if (data->data_len > sizeof(cdata->value)) {
		LOG_ERR("Not enough space for Codec Config: %u > %zu", data->data_len,
			sizeof(cdata->value));
		return false;
	}

	LOG_DBG("#%u type 0x%02x len %u", codec->data_count, data->type, data->data_len);

	cdata->data.type = data->type;
	cdata->data.data_len = data->data_len;

	/* Deep copy data contents */
	cdata->data.data = cdata->value;
	(void)memcpy(cdata->value, data->data, data->data_len);

	LOG_HEXDUMP_DBG(cdata->value, data->data_len, "data");

	codec->data_count++;

	return true;
}

static int unicast_client_ep_set_codec(struct bt_bap_ep *ep, uint8_t id, uint16_t cid, uint16_t vid,
				       void *data, uint8_t len, struct bt_codec *codec)
{
	struct net_buf_simple ad;

	if (!ep && !codec) {
		return -EINVAL;
	}

	LOG_DBG("ep %p codec id 0x%02x cid 0x%04x vid 0x%04x len %u", ep, id, cid, vid, len);

	if (!codec) {
		codec = &ep->codec;
	}

	codec->id = id;
	codec->cid = cid;
	codec->vid = vid;

	/* Reset current metadata */
	codec->data_count = 0;
	(void)memset(codec->data, 0, sizeof(codec->data));

	if (!len) {
		return 0;
	}

	net_buf_simple_init_with_data(&ad, data, len);

	/* Parse LTV entries */
	bt_data_parse(&ad, unicast_client_codec_config_store, codec);

	/* Check if all entries could be parsed */
	if (ad.len) {
		LOG_ERR("Unable to parse Codec Config: len %u", ad.len);
		goto fail;
	}

	return 0;

fail:
	(void)memset(codec, 0, sizeof(*codec));
	return -EINVAL;
}

static bool unicast_client_codec_metadata_store(struct bt_data *data, void *user_data)
{
	struct bt_codec *codec = user_data;
	struct bt_codec_data *meta;

	if (codec->meta_count >= ARRAY_SIZE(codec->meta)) {
		LOG_ERR("No slot available for Codec Config Metadata");
		return false;
	}

	meta = &codec->meta[codec->meta_count];

	if (data->data_len > sizeof(meta->value)) {
		LOG_ERR("Not enough space for Codec Config Metadata: %u > %zu", data->data_len,
			sizeof(meta->value));
		return false;
	}

	LOG_DBG("#%u type 0x%02x len %u", codec->meta_count, data->type, data->data_len);

	meta->data.type = data->type;
	meta->data.data_len = data->data_len;

	/* Deep copy data contents */
	meta->data.data = meta->value;
	(void)memcpy(meta->value, data->data, data->data_len);

	LOG_HEXDUMP_DBG(meta->value, data->data_len, "data");

	codec->meta_count++;

	return true;
}

static int unicast_client_ep_set_metadata(struct bt_bap_ep *ep, void *data, uint8_t len,
					  struct bt_codec *codec)
{
	struct net_buf_simple meta;
	int err;

	if (!ep && !codec) {
		return -EINVAL;
	}

	LOG_DBG("ep %p len %u codec %p", ep, len, codec);

	if (!codec) {
		codec = &ep->codec;
	}

	/* Reset current metadata */
	codec->meta_count = 0;
	(void)memset(codec->meta, 0, sizeof(codec->meta));

	if (!len) {
		return 0;
	}

	net_buf_simple_init_with_data(&meta, data, len);

	/* Parse LTV entries */
	bt_data_parse(&meta, unicast_client_codec_metadata_store, codec);

	/* Check if all entries could be parsed */
	if (meta.len) {
		LOG_ERR("Unable to parse Metadata: len %u", meta.len);
		err = -EINVAL;

		if (meta.len > 2) {
			/* Value of the Metadata Type field in error */
			err = meta.data[2];
		}

		goto fail;
	}

	return 0;

fail:
	codec->meta_count = 0;
	(void)memset(codec->meta, 0, sizeof(codec->meta));
	return err;
}

static uint8_t unicast_client_cp_notify(struct bt_conn *conn,
					struct bt_gatt_subscribe_params *params, const void *data,
					uint16_t length)
{
	struct bt_ascs_cp_rsp *rsp;
	struct net_buf_simple buf;

	LOG_DBG("conn %p len %u", conn, length);

	if (!data) {
		LOG_DBG("Unsubscribed");
		params->value_handle = 0x0000;
		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len < sizeof(*rsp)) {
		LOG_ERR("Control Point Notification too small");
		return BT_GATT_ITER_STOP;
	}

	rsp = net_buf_simple_pull_mem(&buf, sizeof(*rsp));

	for (uint8_t i = 0U; i < rsp->num_ase; i++) {
		struct bt_bap_unicast_client_ep *client_ep;
		struct bt_ascs_cp_ase_rsp *ase_rsp;
		struct bt_bap_stream *stream;

		if (buf.len < sizeof(*ase_rsp)) {
			LOG_ERR("Control Point Notification too small");
			return BT_GATT_ITER_STOP;
		}

		ase_rsp = net_buf_simple_pull_mem(&buf, sizeof(*ase_rsp));

		LOG_DBG("op %s (0x%02x) id 0x%02x code %s (0x%02x) "
			"reason %s (0x%02x)", bt_ascs_op_str(rsp->op), rsp->op,
			ase_rsp->id, bt_ascs_rsp_str(ase_rsp->code),
			ase_rsp->code, bt_ascs_reason_str(ase_rsp->reason),
			ase_rsp->reason);

		if (unicast_client_cbs == NULL) {
			continue;
		}

		stream = audio_stream_by_ep_id(conn, ase_rsp->id);
		if (stream == NULL) {
			LOG_DBG("Could not find stream by id %u", ase_rsp->id);
			continue;
		}

		client_ep = CONTAINER_OF(stream->ep, struct bt_bap_unicast_client_ep, ep);

		switch (rsp->op) {
		case BT_ASCS_CONFIG_OP:
			if (unicast_client_cbs->config != NULL) {
				unicast_client_cbs->config(stream, ase_rsp->code, ase_rsp->reason);
			}
			break;
		case BT_ASCS_QOS_OP:
			if (unicast_client_cbs->qos != NULL) {
				unicast_client_cbs->qos(stream, ase_rsp->code, ase_rsp->reason);
			}
			break;
		case BT_ASCS_ENABLE_OP:
			if (unicast_client_cbs->enable != NULL) {
				unicast_client_cbs->enable(stream, ase_rsp->code, ase_rsp->reason);
			}
			break;
		case BT_ASCS_START_OP:
			if (unicast_client_cbs->start != NULL) {
				unicast_client_cbs->start(stream, ase_rsp->code, ase_rsp->reason);
			}
			break;
		case BT_ASCS_DISABLE_OP:
			if (unicast_client_cbs->disable != NULL) {
				unicast_client_cbs->disable(stream, ase_rsp->code, ase_rsp->reason);
			}
			break;
		case BT_ASCS_STOP_OP:
			if (unicast_client_cbs->stop != NULL) {
				unicast_client_cbs->stop(stream, ase_rsp->code, ase_rsp->reason);
			}
			break;
		case BT_ASCS_METADATA_OP:
			if (unicast_client_cbs->metadata != NULL) {
				unicast_client_cbs->metadata(stream, ase_rsp->code,
							     ase_rsp->reason);
			}
			break;
		case BT_ASCS_RELEASE_OP:
			if (client_ep->release_requested) {
				/* Set to false to only handle the callback here */
				client_ep->release_requested = false;
			}

			if (unicast_client_cbs->release != NULL) {
				unicast_client_cbs->release(stream, ase_rsp->code, ase_rsp->reason);
			}
			break;
		default:
			break;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t unicast_client_ep_notify(struct bt_conn *conn,
					struct bt_gatt_subscribe_params *params, const void *data,
					uint16_t length)
{
	struct net_buf_simple buf;
	struct bt_bap_unicast_client_ep *client_ep;

	client_ep = CONTAINER_OF(params, struct bt_bap_unicast_client_ep, subscribe);

	LOG_DBG("conn %p ep %p len %u", conn, &client_ep->ep, length);

	if (!data) {
		LOG_DBG("Unsubscribed");
		params->value_handle = 0x0000;
		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len < sizeof(struct bt_ascs_ase_status)) {
		LOG_ERR("Notification too small");
		return BT_GATT_ITER_STOP;
	}

	unicast_client_ep_set_status(&client_ep->ep, &buf);

	return BT_GATT_ITER_CONTINUE;
}

static int unicast_client_ep_subscribe(struct bt_conn *conn, struct bt_bap_ep *ep)
{
	struct bt_bap_unicast_client_ep *client_ep;

	client_ep = CONTAINER_OF(ep, struct bt_bap_unicast_client_ep, ep);

	LOG_DBG("ep %p handle 0x%02x", ep, client_ep->handle);

	if (client_ep->subscribe.value_handle) {
		return 0;
	}

	client_ep->subscribe.value_handle = client_ep->handle;
	client_ep->subscribe.ccc_handle = 0x0000;
	client_ep->subscribe.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	client_ep->subscribe.disc_params = &client_ep->discover;
	client_ep->subscribe.notify = unicast_client_ep_notify;
	client_ep->subscribe.value = BT_GATT_CCC_NOTIFY;
	atomic_set_bit(client_ep->subscribe.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	return bt_gatt_subscribe(conn, &client_ep->subscribe);
}

static void unicast_client_cp_sub_cb(struct bt_conn *conn, uint8_t err,
				     struct bt_gatt_subscribe_params *sub_params)
{
	struct bt_bap_unicast_client_discover_params *params;

	LOG_DBG("conn %p err %u", conn, err);

	params = CONTAINER_OF(sub_params->disc_params, struct bt_bap_unicast_client_discover_params,
			      discover);

	params->err = err;
	params->func(conn, NULL, NULL, params);
}

static void unicast_client_ep_set_cp(struct bt_conn *conn,
				     struct bt_bap_unicast_client_discover_params *params,
				     uint16_t handle)
{
	uint8_t index;

	LOG_DBG("conn %p 0x%04x", conn, handle);

	index = bt_conn_index(conn);

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(uni_cli_insts[index].snks); i++) {
		struct bt_bap_unicast_client_ep *client_ep = &uni_cli_insts[index].snks[i];

		if (client_ep->handle) {
			client_ep->cp_handle = handle;
		}
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(uni_cli_insts[index].srcs); i++) {
		struct bt_bap_unicast_client_ep *client_ep = &uni_cli_insts[index].srcs[i];

		if (client_ep->handle) {
			client_ep->cp_handle = handle;
		}
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

	if (!uni_cli_insts[index].cp_subscribe.value_handle) {
		int err;

		uni_cli_insts[index].cp_subscribe.value_handle = handle;
		uni_cli_insts[index].cp_subscribe.ccc_handle = 0x0000;
		uni_cli_insts[index].cp_subscribe.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		uni_cli_insts[index].cp_subscribe.disc_params = &params->discover;
		uni_cli_insts[index].cp_subscribe.notify = unicast_client_cp_notify;
		uni_cli_insts[index].cp_subscribe.value = BT_GATT_CCC_NOTIFY;
		uni_cli_insts[index].cp_subscribe.subscribe = unicast_client_cp_sub_cb;
		atomic_set_bit(uni_cli_insts[index].cp_subscribe.flags,
			       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

		err = bt_gatt_subscribe(conn, &uni_cli_insts[index].cp_subscribe);
		if (err != 0) {
			LOG_DBG("Failed to subscribe: %d", err);

			params->err = BT_ATT_ERR_UNLIKELY;

			params->func(conn, NULL, NULL, params);

			return;
		}
	} else { /* already subscribed */
		params->err = 0;
		params->func(conn, NULL, NULL, params);
	}
}

NET_BUF_SIMPLE_DEFINE_STATIC(ep_buf, CONFIG_BT_L2CAP_TX_MTU);

struct net_buf_simple *bt_bap_unicast_client_ep_create_pdu(uint8_t op)
{
	struct bt_ascs_ase_cp *hdr;

	/* Reset buffer before using */
	net_buf_simple_reset(&ep_buf);

	hdr = net_buf_simple_add(&ep_buf, sizeof(*hdr));
	hdr->op = op;

	return &ep_buf;
}

static int unicast_client_ep_config(struct bt_bap_ep *ep, struct net_buf_simple *buf,
				    const struct bt_codec *codec)
{
	struct bt_ascs_config *req;
	uint8_t cc_len;

	LOG_DBG("ep %p buf %p codec %p", ep, buf, codec);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x00 (Idle) */
	case BT_BAP_EP_STATE_IDLE:
		/* or 0x01 (Codec Configured) */
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		/* or 0x02 (QoS Configured) */
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x dir %s codec 0x%02x", ep->status.id, bt_audio_dir_str(ep->dir),
		codec->id);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->ase = ep->status.id;
	req->latency = 0x02; /* TODO: Select target latency based on additional input? */
	req->phy = 0x02;     /* TODO: Select target PHY based on additional input? */
	req->codec.id = codec->id;
	req->codec.cid = codec->cid;
	req->codec.vid = codec->vid;

	cc_len = buf->len;
	unicast_client_codec_data_add(buf, "data", codec->data_count, codec->data);
	req->cc_len = buf->len - cc_len;

	return 0;
}

int bt_bap_unicast_client_ep_qos(struct bt_bap_ep *ep, struct net_buf_simple *buf,
				 struct bt_codec_qos *qos)
{
	struct bt_ascs_qos *req;
	struct bt_conn_iso *conn_iso;

	LOG_DBG("ep %p buf %p qos %p", ep, buf, qos);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x01 (Codec Configured) */
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		/* or 0x02 (QoS Configured) */
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	conn_iso = &ep->iso->chan.iso->iso;

	LOG_DBG("id 0x%02x cig 0x%02x cis 0x%02x interval %u framing 0x%02x "
		"phy 0x%02x sdu %u rtn %u latency %u pd %u",
		ep->status.id, conn_iso->cig_id, conn_iso->cis_id, qos->interval, qos->framing,
		qos->phy, qos->sdu, qos->rtn, qos->latency, qos->pd);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->ase = ep->status.id;
	/* TODO: don't hardcode CIG and CIS, they should come from ISO */
	req->cig = conn_iso->cig_id;
	req->cis = conn_iso->cis_id;
	sys_put_le24(qos->interval, req->interval);
	req->framing = qos->framing;
	req->phy = qos->phy;
	req->sdu = qos->sdu;
	req->rtn = qos->rtn;
	req->latency = sys_cpu_to_le16(qos->latency);
	sys_put_le24(qos->pd, req->pd);

	return 0;
}

static int unicast_client_ep_enable(struct bt_bap_ep *ep, struct net_buf_simple *buf,
				    struct bt_codec_data *meta, size_t meta_count)
{
	struct bt_ascs_metadata *req;

	LOG_DBG("ep %p buf %p metadata count %zu", ep, buf, meta_count);

	if (!ep) {
		return -EINVAL;
	}

	if (ep->status.state != BT_BAP_EP_STATE_QOS_CONFIGURED) {
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x", ep->status.id);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->ase = ep->status.id;

	req->len = buf->len;
	unicast_client_codec_data_add(buf, "meta", meta_count, meta);
	req->len = buf->len - req->len;

	return 0;
}

static int unicast_client_ep_metadata(struct bt_bap_ep *ep, struct net_buf_simple *buf,
				      struct bt_codec_data *meta, size_t meta_count)
{
	struct bt_ascs_metadata *req;

	LOG_DBG("ep %p buf %p metadata count %zu", ep, buf, meta_count);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid for an ASE only if ASE_State field = 0x03 (Enabling) */
	case BT_BAP_EP_STATE_ENABLING:
	/* or 0x04 (Streaming) */
	case BT_BAP_EP_STATE_STREAMING:
		break;
	default:
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x", ep->status.id);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->ase = ep->status.id;

	req->len = buf->len;
	unicast_client_codec_data_add(buf, "meta", meta_count, meta);
	req->len = buf->len - req->len;

	return 0;
}

static int unicast_client_ep_start(struct bt_bap_ep *ep, struct net_buf_simple *buf)
{
	LOG_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	if (ep->status.state != BT_BAP_EP_STATE_ENABLING &&
	    ep->status.state != BT_BAP_EP_STATE_DISABLING) {
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

static int unicast_client_ep_disable(struct bt_bap_ep *ep, struct net_buf_simple *buf)
{
	LOG_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Enabling) */
	case BT_BAP_EP_STATE_ENABLING:
		/* or 0x04 (Streaming) */
	case BT_BAP_EP_STATE_STREAMING:
		break;
	default:
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

static int unicast_client_ep_stop(struct bt_bap_ep *ep, struct net_buf_simple *buf)
{
	LOG_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	/* Valid only if ASE_State field value = 0x05 (Disabling). */
	if (ep->status.state != BT_BAP_EP_STATE_DISABLING) {
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

static int unicast_client_ep_release(struct bt_bap_ep *ep, struct net_buf_simple *buf)
{
	LOG_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x01 (Codec Configured) */
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		/* or 0x02 (QoS Configured) */
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		/* or 0x03 (Enabling) */
	case BT_BAP_EP_STATE_ENABLING:
		/* or 0x04 (Streaming) */
	case BT_BAP_EP_STATE_STREAMING:
		/* or 0x05 (Disabling) */
	case BT_BAP_EP_STATE_DISABLING:
		break;
	default:
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

int bt_bap_unicast_client_ep_send(struct bt_conn *conn, struct bt_bap_ep *ep,
				  struct net_buf_simple *buf)
{
	struct bt_bap_unicast_client_ep *client_ep =
		CONTAINER_OF(ep, struct bt_bap_unicast_client_ep, ep);

	LOG_DBG("conn %p ep %p buf %p len %u", conn, ep, buf, buf->len);

	return bt_gatt_write_without_response(conn, client_ep->cp_handle, buf->data, buf->len,
					      false);
}

static void unicast_client_reset(struct bt_bap_ep *ep)
{
	struct bt_bap_unicast_client_ep *client_ep =
		CONTAINER_OF(ep, struct bt_bap_unicast_client_ep, ep);
	int err;

	LOG_DBG("ep %p", ep);

	/* Pretend we received an idle state notification from the server to trigger all cleanup */
	ep->status.state = BT_BAP_EP_STATE_IDLE;
	err = unicast_client_ep_idle_state(ep);
	if (err != 0) {
		LOG_DBG("unicast_client_ep_idle_state returned %d", err);

		if (err == -EBUSY) {
			/* Wait for ISO disconnected event */
			return;
		}
	}

	(void)memset(ep, 0, sizeof(*ep));

	client_ep->cp_handle = 0U;
	client_ep->handle = 0U;
	(void)memset(&client_ep->discover, 0, sizeof(client_ep->discover));
	/* Need to keep the subscribe params intact for the callback */
}

static void unicast_client_ep_reset(struct bt_conn *conn)
{
	uint8_t index;

	LOG_DBG("conn %p", conn);

	index = bt_conn_index(conn);

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(uni_cli_insts[index].snks); i++) {
		struct bt_bap_ep *ep = &uni_cli_insts[index].snks[i].ep;

		unicast_client_reset(ep);
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(uni_cli_insts[index].srcs); i++) {
		struct bt_bap_ep *ep = &uni_cli_insts[index].srcs[i].ep;

		unicast_client_reset(ep);
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
}

static void bt_audio_codec_qos_to_cig_param(struct bt_iso_cig_param *cig_param,
					    const struct bt_codec_qos *qos)
{
	cig_param->framing = qos->framing;
	cig_param->packing = BT_ISO_PACKING_SEQUENTIAL; /*  TODO: Add to QoS struct */
	cig_param->interval = qos->interval;
	cig_param->latency = qos->latency;
	cig_param->sca = BT_GAP_SCA_UNKNOWN;
}

/* FIXME: Remove `qos` parameter. Some of the QoS related CIG can be different
 * between CIS'es. The implementation shall take the CIG parameters from
 * unicast_group instead.
 */
static int bt_audio_cig_create(struct bt_bap_unicast_group *group, const struct bt_codec_qos *qos)
{
	struct bt_iso_cig_param param;
	uint8_t cis_count;
	int err;

	LOG_DBG("group %p qos %p", group, qos);

	cis_count = 0U;
	for (size_t i = 0U; i < ARRAY_SIZE(group->cis); i++) {
		if (group->cis[i] == NULL) {
			/* A NULL CIS acts as a NULL terminater */
			break;
		}

		cis_count++;
	}

	param.num_cis = cis_count;
	param.cis_channels = group->cis;
	bt_audio_codec_qos_to_cig_param(&param, qos);

	err = bt_iso_cig_create(&param, &group->cig);
	if (err != 0) {
		LOG_ERR("bt_iso_cig_create failed: %d", err);
		return err;
	}

	group->qos = qos;

	return 0;
}

static int bt_audio_cig_reconfigure(struct bt_bap_unicast_group *group,
				    const struct bt_codec_qos *qos)
{
	struct bt_iso_cig_param param;
	uint8_t cis_count;
	int err;

	LOG_DBG("group %p qos %p", group, qos);

	cis_count = 0U;
	for (size_t i = 0U; i < ARRAY_SIZE(group->cis); i++) {
		if (group->cis[i] == NULL) {
			/* A NULL CIS acts as a NULL terminater */
			break;
		}

		cis_count++;
	}

	param.num_cis = cis_count;
	param.cis_channels = group->cis;
	bt_audio_codec_qos_to_cig_param(&param, qos);

	err = bt_iso_cig_reconfigure(group->cig, &param);
	if (err != 0) {
		LOG_ERR("bt_iso_cig_create failed: %d", err);
		return err;
	}

	group->qos = qos;

	return 0;
}

static void audio_stream_qos_cleanup(const struct bt_conn *conn,
				     struct bt_bap_unicast_group *group)
{
	struct bt_bap_stream *stream;

	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, _node) {
		if (stream->conn != conn && stream->ep != NULL) {
			/* Channel not part of this ACL, skip */
			continue;
		}

		bt_bap_iso_unbind_ep(stream->ep->iso, stream->ep);
	}
}

static int unicast_client_cig_terminate(struct bt_bap_unicast_group *group)
{
	LOG_DBG("group %p", group);

	return bt_iso_cig_terminate(group->cig);
}

static int unicast_client_stream_connect(struct bt_bap_stream *stream)
{
	struct bt_iso_connect_param param;
	struct bt_iso_chan *iso_chan;

	iso_chan = bt_bap_stream_iso_chan_get(stream);

	LOG_DBG("stream %p iso %p", stream, iso_chan);

	if (stream == NULL || iso_chan == NULL) {
		return -EINVAL;
	}

	param.acl = stream->conn;
	param.iso_chan = iso_chan;

	switch (iso_chan->state) {
	case BT_ISO_STATE_DISCONNECTED:
		return bt_iso_chan_connect(&param, 1);
	case BT_ISO_STATE_CONNECTING:
		return -EBUSY;
	case BT_ISO_STATE_CONNECTED:
		return -EALREADY;
	default:
		return bt_iso_chan_connect(&param, 1);
	}
}

static int unicast_group_add_iso(struct bt_bap_unicast_group *group, struct bt_bap_iso *iso)
{
	struct bt_iso_chan **chan_slot = NULL;

	__ASSERT_NO_MSG(group != NULL);
	__ASSERT_NO_MSG(iso != NULL);

	/* Append iso channel to the group->cis array */
	for (size_t i = 0U; i < ARRAY_SIZE(group->cis); i++) {
		/* Return if already there */
		if (group->cis[i] == &iso->chan) {
			return 0;
		}

		if (chan_slot == NULL && group->cis[i] == NULL) {
			chan_slot = &group->cis[i];
		}
	}

	if (chan_slot == NULL) {
		return -ENOMEM;
	}

	*chan_slot = &iso->chan;

	return 0;
}

static void unicast_group_del_iso(struct bt_bap_unicast_group *group, struct bt_bap_iso *iso)
{
	struct bt_bap_stream *stream;

	__ASSERT_NO_MSG(group != NULL);
	__ASSERT_NO_MSG(iso != NULL);

	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, _node) {
		if (stream->ep->iso == iso) {
			/* still in use by some other stream */
			return;
		}
	}

	for (size_t i = 0U; i < ARRAY_SIZE(group->cis); i++) {
		if (group->cis[i] == &iso->chan) {
			group->cis[i] = NULL;
			return;
		}
	}
}

static void unicast_client_codec_qos_to_iso_qos(struct bt_bap_iso *iso,
						const struct bt_codec_qos *qos,
						enum bt_audio_dir dir)
{
	struct bt_iso_chan_io_qos *io_qos;
	struct bt_iso_chan_io_qos *other_io_qos;

	if (dir == BT_AUDIO_DIR_SINK) {
		/* If the endpoint is a sink, then we need to
		 * configure our TX parameters
		 */
		io_qos = iso->chan.qos->tx;
		if (bt_bap_iso_get_ep(true, iso, BT_AUDIO_DIR_SOURCE) == NULL) {
			other_io_qos = iso->chan.qos->rx;
		} else {
			other_io_qos = NULL;
		}
	} else {
		/* If the endpoint is a source, then we need to
		 * configure our RX parameters
		 */
		io_qos = iso->chan.qos->rx;
		if (bt_bap_iso_get_ep(true, iso, BT_AUDIO_DIR_SINK) == NULL) {
			other_io_qos = iso->chan.qos->tx;
		} else {
			other_io_qos = NULL;
		}
	}

	bt_audio_codec_qos_to_iso_qos(io_qos, qos);

	if (other_io_qos != NULL) {
		/* If the opposing ASE of the CIS is not yet configured, we
		 * still need to set the PHY value when creating the CIG.
		 */
		other_io_qos->phy = io_qos->phy;
	}
}

static void unicast_group_add_stream(struct bt_bap_unicast_group *group,
				     struct bt_bap_unicast_group_stream_param *param,
				     struct bt_bap_iso *iso, enum bt_audio_dir dir)
{
	struct bt_bap_stream *stream = param->stream;
	struct bt_codec_qos *qos = param->qos;

	LOG_DBG("group %p stream %p qos %p iso %p dir %u", group, stream, qos, iso, dir);

	__ASSERT_NO_MSG(stream->ep == NULL || (stream->ep != NULL && stream->ep->iso == NULL));

	stream->qos = qos;
	stream->dir = dir;
	stream->group = group;

	/* iso initialized already */
	bt_bap_iso_bind_stream(iso, stream);
	if (stream->ep != NULL) {
		bt_bap_iso_bind_ep(iso, stream->ep);
	}

	/* Store the Codec QoS in the bap_iso */
	unicast_client_codec_qos_to_iso_qos(iso, qos, dir);

	sys_slist_append(&group->streams, &stream->_node);
}

static int unicast_group_add_stream_pair(struct bt_bap_unicast_group *group,
					 struct bt_bap_unicast_group_stream_pair_param *param)
{
	struct bt_bap_iso *iso;
	int err;

	__ASSERT_NO_MSG(group != NULL);
	__ASSERT_NO_MSG(param != NULL);
	__ASSERT_NO_MSG(param->rx_param != NULL || param->tx_param != NULL);

	iso = bt_bap_unicast_client_new_audio_iso();
	if (iso == NULL) {
		return -ENOMEM;
	}

	err = unicast_group_add_iso(group, iso);
	if (err < 0) {
		bt_bap_iso_unref(iso);
		return err;
	}

	if (param->rx_param != NULL) {
		unicast_group_add_stream(group, param->rx_param, iso, BT_AUDIO_DIR_SOURCE);
	}

	if (param->tx_param != NULL) {
		unicast_group_add_stream(group, param->tx_param, iso, BT_AUDIO_DIR_SINK);
	}

	bt_bap_iso_unref(iso);

	return 0;
}

static void unicast_group_del_stream(struct bt_bap_unicast_group *group,
				     struct bt_bap_stream *stream)
{
	__ASSERT_NO_MSG(group != NULL);
	__ASSERT_NO_MSG(stream != NULL);

	if (sys_slist_find_and_remove(&group->streams, &stream->_node)) {
		struct bt_bap_ep *ep = stream->ep;

		if (stream->bap_iso != NULL) {
			bt_bap_iso_unbind_stream(stream->bap_iso, stream);
		}

		if (ep != NULL && ep->iso != NULL) {
			unicast_group_del_iso(group, ep->iso);

			bt_bap_iso_unbind_ep(ep->iso, ep);
		}

		stream->group = NULL;
	}
}

static void unicast_group_del_stream_pair(struct bt_bap_unicast_group *group,
					  struct bt_bap_unicast_group_stream_pair_param *param)
{
	__ASSERT_NO_MSG(group != NULL);
	__ASSERT_NO_MSG(param != NULL);
	__ASSERT_NO_MSG(param->rx_param != NULL || param->tx_param != NULL);

	if (param->rx_param != NULL) {
		__ASSERT_NO_MSG(param->rx_param->stream);
		unicast_group_del_stream(group, param->rx_param->stream);
	}

	if (param->tx_param != NULL) {
		__ASSERT_NO_MSG(param->tx_param->stream);
		unicast_group_del_stream(group, param->tx_param->stream);
	}
}

static struct bt_bap_unicast_group *unicast_group_alloc(void)
{
	struct bt_bap_unicast_group *group = NULL;

	for (size_t i = 0U; i < ARRAY_SIZE(unicast_groups); i++) {
		if (!unicast_groups[i].allocated) {
			group = &unicast_groups[i];

			(void)memset(group, 0, sizeof(*group));

			group->allocated = true;
			group->index = i;

			break;
		}
	}

	return group;
}

static void unicast_group_free(struct bt_bap_unicast_group *group)
{
	struct bt_bap_stream *stream, *next;

	__ASSERT_NO_MSG(group != NULL);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&group->streams, stream, next, _node) {
		struct bt_bap_ep *ep = stream->ep;

		stream->group = NULL;
		if (stream->bap_iso != NULL) {
			bt_bap_iso_unbind_stream(stream->bap_iso, stream);
		}

		if (ep != NULL && ep->iso != NULL) {
			bt_bap_iso_unbind_ep(ep->iso, ep);
		}

		sys_slist_remove(&group->streams, NULL, &stream->_node);
	}

	group->allocated = false;
}

static int stream_param_check(const struct bt_bap_unicast_group_stream_param *param)
{
	CHECKIF(param->stream == NULL)
	{
		LOG_ERR("param->stream is NULL");
		return -EINVAL;
	}

	CHECKIF(param->qos == NULL)
	{
		LOG_ERR("param->qos is NULL");
		return -EINVAL;
	}

	if (param->stream != NULL && param->stream->group != NULL) {
		LOG_WRN("stream %p already part of group %p", param->stream, param->stream->group);
		return -EALREADY;
	}

	CHECKIF(bt_audio_verify_qos(param->qos) != BT_BAP_ASCS_REASON_NONE)
	{
		LOG_ERR("Invalid QoS");
		return -EINVAL;
	}

	return 0;
}

static int stream_pair_param_check(const struct bt_bap_unicast_group_stream_pair_param *param)
{
	int err;

	CHECKIF(param->rx_param == NULL && param->tx_param == NULL)
	{
		LOG_DBG("Invalid stream parameters");
		return -EINVAL;
	}

	if (param->rx_param != NULL) {
		err = stream_param_check(param->rx_param);
		if (err < 0) {
			return err;
		}
	}

	if (param->tx_param != NULL) {
		err = stream_param_check(param->tx_param);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

static int group_qos_common_set(const struct bt_codec_qos **group_qos,
				const struct bt_bap_unicast_group_stream_pair_param *param)
{
	if (param->rx_param != NULL && *group_qos == NULL) {
		*group_qos = param->rx_param->qos;
	}

	if (param->tx_param != NULL && *group_qos == NULL) {
		*group_qos = param->tx_param->qos;
	}

	return 0;
}

int bt_bap_unicast_group_create(struct bt_bap_unicast_group_param *param,
				  struct bt_bap_unicast_group **out_unicast_group)
{
	struct bt_bap_unicast_group *unicast_group;
	const struct bt_codec_qos *group_qos = NULL;
	int err;

	CHECKIF(out_unicast_group == NULL)
	{
		LOG_DBG("out_unicast_group is NULL");
		return -EINVAL;
	}
	/* Set out_unicast_group to NULL until the source has actually been created */
	*out_unicast_group = NULL;

	CHECKIF(param == NULL)
	{
		LOG_DBG("streams is NULL");
		return -EINVAL;
	}

	CHECKIF(param->params_count > UNICAST_GROUP_STREAM_CNT)
	{
		LOG_DBG("Too many streams provided: %u/%u", param->params_count,
			UNICAST_GROUP_STREAM_CNT);
		return -EINVAL;
	}

	unicast_group = unicast_group_alloc();
	if (unicast_group == NULL) {
		LOG_DBG("Could not allocate any more unicast groups");
		return -ENOMEM;
	}

	for (size_t i = 0U; i < param->params_count; i++) {
		struct bt_bap_unicast_group_stream_pair_param *stream_param;

		stream_param = &param->params[i];

		err = stream_pair_param_check(stream_param);
		if (err < 0) {
			return err;
		}

		err = group_qos_common_set(&group_qos, stream_param);
		if (err < 0) {
			return err;
		}

		err = unicast_group_add_stream_pair(unicast_group, stream_param);
		if (err < 0) {
			LOG_DBG("unicast_group_add_stream failed: %d", err);
			unicast_group_free(unicast_group);

			return err;
		}
	}

	err = bt_audio_cig_create(unicast_group, group_qos);
	if (err != 0) {
		LOG_DBG("bt_audio_cig_create failed: %d", err);
		unicast_group_free(unicast_group);

		return err;
	}

	*out_unicast_group = unicast_group;

	return 0;
}

int bt_bap_unicast_group_add_streams(struct bt_bap_unicast_group *unicast_group,
				       struct bt_bap_unicast_group_stream_pair_param params[],
				       size_t num_param)
{
	const struct bt_codec_qos *group_qos = unicast_group->qos;
	struct bt_bap_stream *tmp_stream;
	size_t total_stream_cnt;
	struct bt_iso_cig *cig;
	size_t num_added;
	int err;

	CHECKIF(unicast_group == NULL)
	{
		LOG_DBG("unicast_group is NULL");
		return -EINVAL;
	}

	CHECKIF(params == NULL)
	{
		LOG_DBG("params is NULL");
		return -EINVAL;
	}

	CHECKIF(num_param == 0)
	{
		LOG_DBG("num_param is 0");
		return -EINVAL;
	}

	total_stream_cnt = num_param;
	SYS_SLIST_FOR_EACH_CONTAINER(&unicast_group->streams, tmp_stream, _node) {
		total_stream_cnt++;
	}

	if (total_stream_cnt > UNICAST_GROUP_STREAM_CNT) {
		LOG_DBG("Too many streams provided: %u/%u", total_stream_cnt,
			UNICAST_GROUP_STREAM_CNT);
		return -EINVAL;
	}

	/* We can just check the CIG state to see if any streams have started as
	 * that would start the ISO connection procedure
	 */
	cig = unicast_group->cig;
	if (cig != NULL && cig->state != BT_ISO_CIG_STATE_CONFIGURED) {
		LOG_DBG("At least one unicast group stream is started");
		return -EBADMSG;
	}

	for (num_added = 0U; num_added < num_param; num_added++) {
		struct bt_bap_unicast_group_stream_pair_param *stream_param;

		stream_param = &params[num_added];

		err = stream_pair_param_check(stream_param);
		if (err < 0) {
			return err;
		}

		err = group_qos_common_set(&group_qos, stream_param);
		if (err < 0) {
			return err;
		}

		err = unicast_group_add_stream_pair(unicast_group, stream_param);
		if (err < 0) {
			LOG_DBG("unicast_group_add_stream failed: %d", err);
			goto fail;
		}
	}

	err = bt_audio_cig_reconfigure(unicast_group, group_qos);
	if (err != 0) {
		LOG_DBG("bt_audio_cig_reconfigure failed: %d", err);
		goto fail;
	}

	return 0;

fail:
	/* Restore group by removing the newly added streams */
	while (num_added--) {
		unicast_group_del_stream_pair(unicast_group, &params[num_added]);
	}

	return err;
}

int bt_bap_unicast_group_delete(struct bt_bap_unicast_group *unicast_group)
{
	struct bt_bap_stream *stream;

	CHECKIF(unicast_group == NULL)
	{
		LOG_DBG("unicast_group is NULL");
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&unicast_group->streams, stream, _node) {
		/* If a stream has an endpoint, it is not ready to be removed
		 * from a group, as it is not in an idle state
		 */
		if (stream->ep != NULL) {
			LOG_DBG("stream %p is not released", stream);
			return -EINVAL;
		}
	}

	if (unicast_group->cig != NULL) {
		const int err = unicast_client_cig_terminate(unicast_group);

		if (err != 0) {
			LOG_DBG("unicast_client_cig_terminate failed with err %d", err);

			return err;
		}
	}

	unicast_group_free(unicast_group);

	return 0;
}

int bt_bap_unicast_client_config(struct bt_bap_stream *stream, const struct bt_codec *codec)
{
	struct bt_bap_ep *ep = stream->ep;
	struct bt_ascs_config_op *op;
	struct net_buf_simple *buf;
	int err;

	buf = bt_bap_unicast_client_ep_create_pdu(BT_ASCS_CONFIG_OP);

	op = net_buf_simple_add(buf, sizeof(*op));
	op->num_ases = 0x01;

	err = unicast_client_ep_config(ep, buf, codec);
	if (err != 0) {
		return err;
	}

	err = bt_bap_unicast_client_ep_send(stream->conn, ep, buf);
	if (err != 0) {
		return err;
	}

	return 0;
}

int bt_bap_unicast_client_qos(struct bt_conn *conn, struct bt_bap_unicast_group *group)
{
	struct bt_bap_stream *stream;
	struct bt_ascs_config_op *op;
	struct net_buf_simple *buf;
	struct bt_bap_ep *ep;
	bool conn_stream_found;
	bool cig_connected;
	int err;

	/* Used to determine if a stream for the supplied connection pointer
	 * was actually found
	 */
	conn_stream_found = false;

	/* User to determine if any stream in the group is in
	 * the connected state
	 */
	cig_connected = false;

	/* Validate streams before starting the QoS execution */
	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, _node) {
		if (stream->conn != conn) {
			/* Channel not part of this ACL, skip */
			continue;
		}
		conn_stream_found = true;

		ep = stream->ep;
		if (ep == NULL) {
			LOG_DBG("stream->ep is NULL");
			return -EINVAL;
		}

		/* Can only be done if all the streams are in the codec
		 * configured state or the QoS configured state
		 */
		switch (ep->status.state) {
		case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		case BT_BAP_EP_STATE_QOS_CONFIGURED:
			break;
		default:
			LOG_DBG("Invalid state: %s", bt_bap_ep_state_str(stream->ep->status.state));
			return -EINVAL;
		}

		if (bt_bap_stream_verify_qos(stream, stream->qos) != BT_BAP_ASCS_REASON_NONE) {
			return -EINVAL;
		}

		/* Verify ep->dir */
		switch (ep->dir) {
		case BT_AUDIO_DIR_SINK:
		case BT_AUDIO_DIR_SOURCE:
			break;
		default:
			__ASSERT(false, "invalid endpoint dir: %u", ep->dir);
			return -EINVAL;
		}

		if (stream->bap_iso == NULL) {
			/* This can only happen if the stream was somehow added
			 * to a group without the bap_iso being bound to it
			 */
			LOG_ERR("Could not find bap_iso for stream %p", stream);
			return -EINVAL;
		}
	}

	if (!conn_stream_found) {
		LOG_DBG("No streams in the group %p for conn %p", group, conn);
		return -EINVAL;
	}

	/* Generate the control point write */
	buf = bt_bap_unicast_client_ep_create_pdu(BT_ASCS_QOS_OP);

	op = net_buf_simple_add(buf, sizeof(*op));

	(void)memset(op, 0, sizeof(*op));
	ep = NULL; /* Needed to find the control point handle */
	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, _node) {
		if (stream->conn != conn) {
			/* Channel not part of this ACL, skip */
			continue;
		}

		op->num_ases++;

		if (stream->ep->iso == NULL) {
			/* Not yet bound with the bap_iso */
			bt_bap_iso_bind_ep(stream->bap_iso, stream->ep);
		}

		err = bt_bap_unicast_client_ep_qos(stream->ep, buf, stream->qos);
		if (err != 0) {
			audio_stream_qos_cleanup(conn, group);

			return err;
		}

		if (ep == NULL) {
			ep = stream->ep;
		}
	}

	err = bt_bap_unicast_client_ep_send(conn, ep, buf);
	if (err != 0) {
		LOG_DBG("Could not send config QoS: %d", err);
		audio_stream_qos_cleanup(conn, group);

		return err;
	}

	return 0;
}

int bt_bap_unicast_client_enable(struct bt_bap_stream *stream, struct bt_codec_data *meta,
				 size_t meta_count)
{
	struct bt_bap_ep *ep = stream->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_enable_op *req;
	int err;

	LOG_DBG("stream %p", stream);

	buf = bt_bap_unicast_client_ep_create_pdu(BT_ASCS_ENABLE_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x01;

	err = unicast_client_ep_enable(ep, buf, meta, meta_count);
	if (err) {
		return err;
	}

	return bt_bap_unicast_client_ep_send(stream->conn, ep, buf);
}

int bt_bap_unicast_client_metadata(struct bt_bap_stream *stream, struct bt_codec_data *meta,
				   size_t meta_count)
{
	struct bt_bap_ep *ep = stream->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_enable_op *req;
	int err;

	LOG_DBG("stream %p", stream);

	buf = bt_bap_unicast_client_ep_create_pdu(BT_ASCS_METADATA_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x01;

	err = unicast_client_ep_metadata(ep, buf, meta, meta_count);
	if (err) {
		return err;
	}

	return bt_bap_unicast_client_ep_send(stream->conn, ep, buf);
}

int bt_bap_unicast_client_start(struct bt_bap_stream *stream)
{
	struct bt_bap_ep *ep = stream->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_start_op *req;
	int err;

	LOG_DBG("stream %p", stream);

	buf = bt_bap_unicast_client_ep_create_pdu(BT_ASCS_START_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x00;

	/* If an ASE is in the Enabling state, and if the Unicast Client has
	 * not yet established a CIS for that ASE, the Unicast Client shall
	 * attempt to establish a CIS by using the Connected Isochronous Stream
	 * Central Establishment procedure.
	 */
	err = unicast_client_stream_connect(stream);
	if (err == 0) {
		if (ep->dir == BT_AUDIO_DIR_SOURCE) {
			/* Set bool and wait for ISO to be connected to send the
			 * receiver start ready
			 */
			stream->ep->receiver_ready = true;
		}
	} else if (err == -EALREADY) {
		LOG_DBG("ISO %p already connected", bt_bap_stream_iso_chan_get(stream));

		if (ep->dir == BT_AUDIO_DIR_SOURCE) {
			stream->ep->receiver_ready = true;

			err = unicast_client_send_start(ep);
			if (err != 0) {
				LOG_DBG("Failed to send start: %d", err);

				/* TBD: Should we release the stream then? */
				stream->ep->receiver_ready = false;

				return err;
			}
		}
	} else {
		LOG_DBG("unicast_client_stream_connect failed: %d", err);

		return err;
	}

	return 0;
}

int bt_bap_unicast_client_disable(struct bt_bap_stream *stream)
{
	struct bt_bap_ep *ep = stream->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_disable_op *req;
	int err;

	LOG_DBG("stream %p", stream);

	buf = bt_bap_unicast_client_ep_create_pdu(BT_ASCS_DISABLE_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x01;

	err = unicast_client_ep_disable(ep, buf);
	if (err) {
		return err;
	}

	return bt_bap_unicast_client_ep_send(stream->conn, ep, buf);
}

int bt_bap_unicast_client_stop(struct bt_bap_stream *stream)
{
	struct bt_bap_ep *ep = stream->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_start_op *req;
	int err;

	LOG_DBG("stream %p", stream);

	buf = bt_bap_unicast_client_ep_create_pdu(BT_ASCS_STOP_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x00;

	/* When initiated by the client, valid only if Direction field
	 * parameter value = 0x02 (Server is Audio Source)
	 */
	if (ep->dir == BT_AUDIO_DIR_SOURCE) {
		err = unicast_client_ep_stop(ep, buf);
		if (err) {
			return err;
		}
		req->num_ases++;

		return bt_bap_unicast_client_ep_send(stream->conn, ep, buf);
	}

	return 0;
}

int bt_bap_unicast_client_release(struct bt_bap_stream *stream)
{
	struct bt_bap_unicast_client_ep *client_ep;
	struct bt_bap_ep *ep = stream->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_disable_op *req;
	int err, len;

	LOG_DBG("stream %p", stream);

	if (stream->conn == NULL || stream->conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	buf = bt_bap_unicast_client_ep_create_pdu(BT_ASCS_RELEASE_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x01;
	len = buf->len;

	/* Only attempt to release if not IDLE already */
	if (stream->ep->status.state == BT_BAP_EP_STATE_IDLE) {
		bt_bap_stream_reset(stream);
	} else {
		err = unicast_client_ep_release(ep, buf);
		if (err) {
			return err;
		}
	}

	/* Check if anything needs to be send */
	if (len == buf->len) {
		return 0;
	}

	err = bt_bap_unicast_client_ep_send(stream->conn, ep, buf);
	if (err != 0) {
		return err;
	}

	client_ep = CONTAINER_OF(ep, struct bt_bap_unicast_client_ep, ep);
	client_ep->release_requested = true;

	return 0;
}

static uint8_t unicast_client_cp_discover_func(struct bt_conn *conn,
					       const struct bt_gatt_attr *attr,
					       struct bt_gatt_discover_params *discover)
{
	struct bt_bap_unicast_client_discover_params *params;
	struct bt_gatt_chrc *chrc;

	params = CONTAINER_OF(discover, struct bt_bap_unicast_client_discover_params, discover);

	if (!attr) {
		if (params->err) {
			LOG_ERR("Unable to find ASE Control Point");
		}
		params->func(conn, NULL, NULL, params);
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	LOG_DBG("conn %p attr %p handle 0x%04x", conn, attr, chrc->value_handle);

	unicast_client_ep_set_cp(conn, params, chrc->value_handle);

	return BT_GATT_ITER_STOP;
}

static int unicast_client_ase_cp_discover(struct bt_conn *conn,
					  struct bt_bap_unicast_client_discover_params *params)
{
	LOG_DBG("conn %p params %p", conn, params);

	params->err = BT_ATT_ERR_ATTRIBUTE_NOT_FOUND;
	params->discover.uuid = cp_uuid;
	params->discover.func = unicast_client_cp_discover_func;
	params->discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	params->discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	return bt_gatt_discover(conn, &params->discover);
}

static uint8_t unicast_client_ase_read_func(struct bt_conn *conn, uint8_t err,
					    struct bt_gatt_read_params *read, const void *data,
					    uint16_t length)
{
	struct bt_bap_unicast_client_discover_params *params;
	struct net_buf_simple buf;
	struct bt_bap_ep *ep;

	params = CONTAINER_OF(read, struct bt_bap_unicast_client_discover_params, read);

	LOG_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err) {
		if (err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND && params->num_eps) {
			if (unicast_client_ase_cp_discover(conn, params) < 0) {
				LOG_ERR("Unable to discover ASE Control Point");
				err = BT_ATT_ERR_UNLIKELY;
				goto fail;
			}
			return BT_GATT_ITER_STOP;
		}
		params->err = err;
		params->func(conn, NULL, NULL, params);
		return BT_GATT_ITER_STOP;
	}

	if (!data) {
		if (params->num_eps && unicast_client_ase_cp_discover(conn, params) < 0) {
			LOG_ERR("Unable to discover ASE Control Point");
			err = BT_ATT_ERR_UNLIKELY;
			goto fail;
		}
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("handle 0x%04x", read->by_uuid.start_handle);

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len < sizeof(struct bt_ascs_ase_status)) {
		LOG_ERR("Read response too small");
		goto fail;
	}

	ep = unicast_client_ep_get(conn, params->dir, read->by_uuid.start_handle);
	if (!ep) {
		LOG_WRN("No space left to parse ASE");
		if (params->num_eps) {
			if (unicast_client_ase_cp_discover(conn, params) < 0) {
				LOG_ERR("Unable to discover ASE Control Point");
				err = BT_ATT_ERR_UNLIKELY;
				goto fail;
			}
			return BT_GATT_ITER_STOP;
		}
		goto fail;
	}

	unicast_client_ep_set_status(ep, &buf);
	unicast_client_ep_subscribe(conn, ep);

	params->func(conn, NULL, ep, params);

	params->num_eps++;

	return BT_GATT_ITER_CONTINUE;

fail:
	params->func(conn, NULL, NULL, params);
	return BT_GATT_ITER_STOP;
}

static int unicast_client_ase_discover(struct bt_conn *conn,
				       struct bt_bap_unicast_client_discover_params *params)
{
	LOG_DBG("conn %p params %p", conn, params);

	params->read.func = unicast_client_ase_read_func;
	params->read.handle_count = 0u;

	if (params->dir == BT_AUDIO_DIR_SINK) {
		params->read.by_uuid.uuid = ase_snk_uuid;
	} else if (params->dir == BT_AUDIO_DIR_SOURCE) {
		params->read.by_uuid.uuid = ase_src_uuid;
	} else {
		return -EINVAL;
	}

	params->read.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->read.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	return bt_gatt_read(conn, &params->read);
}

static uint8_t unicast_client_pacs_avail_ctx_read_func(struct bt_conn *conn, uint8_t err,
						       struct bt_gatt_read_params *read,
						       const void *data, uint16_t length)
{
	struct bt_bap_unicast_client_discover_params *params;
	struct bt_pacs_context context;
	struct net_buf_simple buf;

	params = CONTAINER_OF(read, struct bt_bap_unicast_client_discover_params, read);

	LOG_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err || data == NULL || length != sizeof(context)) {
		LOG_DBG("Could not read available context: %d, %p, %u", err, data, length);

		params->func(conn, NULL, NULL, params);

		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);
	context.snk = net_buf_simple_pull_le16(&buf);
	context.src = net_buf_simple_pull_le16(&buf);

	LOG_DBG("sink context %u, source context %u", context.snk, context.src);

	if (unicast_client_cbs != NULL && unicast_client_cbs->available_contexts != NULL) {
		unicast_client_cbs->available_contexts(conn, context.snk, context.src);
	}

	/* Read ASE instances */
	if (unicast_client_ase_discover(conn, params) < 0) {
		LOG_ERR("Unable to read ASE");

		params->func(conn, NULL, NULL, params);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t unicast_client_pacs_avail_ctx_notify_cb(struct bt_conn *conn,
						       struct bt_gatt_subscribe_params *params,
						       const void *data, uint16_t length)
{
	struct bt_pacs_context context;
	struct net_buf_simple buf;

	LOG_DBG("conn %p len %u", conn, length);

	if (!data) {
		LOG_DBG("Unsubscribed");
		params->value_handle = 0x0000;
		return BT_GATT_ITER_STOP;
	}

	/* Terminate early if there's no callbacks */
	if (unicast_client_cbs == NULL || unicast_client_cbs->available_contexts == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len != sizeof(context)) {
		LOG_ERR("Avail_ctx notification incorrect size: %u", length);
		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);
	context.snk = net_buf_simple_pull_le16(&buf);
	context.src = net_buf_simple_pull_le16(&buf);

	LOG_DBG("sink context %u, source context %u", context.snk, context.src);

	if (unicast_client_cbs != NULL && unicast_client_cbs->available_contexts != NULL) {
		unicast_client_cbs->available_contexts(conn, context.snk, context.src);
	}

	return BT_GATT_ITER_CONTINUE;
}

static int unicast_client_pacs_avail_ctx_read(struct bt_conn *conn,
					      struct bt_bap_unicast_client_discover_params *params,
					      uint16_t handle)
{
	LOG_DBG("conn %p params %p", conn, params);

	params->read.func = unicast_client_pacs_avail_ctx_read_func;
	params->read.handle_count = 1U;
	params->read.single.handle = handle;
	params->read.single.offset = 0U;

	return bt_gatt_read(conn, &params->read);
}

static uint8_t unicast_client_pacs_avail_ctx_discover_cb(struct bt_conn *conn,
							 const struct bt_gatt_attr *attr,
							 struct bt_gatt_discover_params *discover)
{
	struct bt_bap_unicast_client_discover_params *params;
	uint8_t index = bt_conn_index(conn);
	const struct bt_gatt_chrc *chrc;
	int err;

	params = CONTAINER_OF(discover, struct bt_bap_unicast_client_discover_params, discover);

	if (!attr) {
		/* If available_ctx is not found, we terminate the discovery as
		 * the characteristic is mandatory
		 */

		params->func(conn, NULL, NULL, params);

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	LOG_DBG("conn %p attr %p handle 0x%04x", conn, attr, chrc->value_handle);

	if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
		struct bt_gatt_subscribe_params *sub_params;

		sub_params = &uni_cli_insts[index].avail_ctx_subscribe;

		if (sub_params->value_handle == 0) {
			LOG_DBG("Subscribing to handle %u", chrc->value_handle);
			sub_params->value_handle = chrc->value_handle;
			sub_params->ccc_handle = 0x0000; /* auto discover ccc */
			sub_params->end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
			sub_params->disc_params = &uni_cli_insts[index].avail_ctx_cc_disc;
			sub_params->notify = unicast_client_pacs_avail_ctx_notify_cb;
			sub_params->value = BT_GATT_CCC_NOTIFY;

			err = bt_gatt_subscribe(conn, sub_params);
			if (err != 0 && err != -EALREADY) {
				LOG_ERR("Failed to subscribe to avail_ctx: %d", err);
			}
		} /* else already subscribed */
	} else {
		LOG_DBG("Invalid chrc->properties: %u", chrc->properties);
		/* If the characteristic is not subscribable we terminate the
		 * discovery as BT_GATT_CHRC_NOTIFY is mandatory
		 */
		params->func(conn, NULL, NULL, params);

		return BT_GATT_ITER_STOP;
	}

	err = unicast_client_pacs_avail_ctx_read(conn, params, chrc->value_handle);
	if (err != 0) {
		LOG_DBG("Failed to read PACS avail_ctx: %d", err);
		params->err = err;

		params->func(conn, NULL, NULL, params);
	}

	return BT_GATT_ITER_STOP;
}

static int
unicast_client_pacs_avail_ctx_discover(struct bt_conn *conn,
				       struct bt_bap_unicast_client_discover_params *params)
{
	LOG_DBG("conn %p params %p", conn, params);

	params->discover.uuid = pacs_avail_ctx_uuid;
	params->discover.func = unicast_client_pacs_avail_ctx_discover_cb;
	params->discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	params->discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	return bt_gatt_discover(conn, &params->discover);
}

static uint8_t unicast_client_pacs_location_read_func(struct bt_conn *conn, uint8_t err,
						      struct bt_gatt_read_params *read,
						      const void *data, uint16_t length)
{
	struct bt_bap_unicast_client_discover_params *params;
	struct net_buf_simple buf;
	uint32_t location;

	params = CONTAINER_OF(read, struct bt_bap_unicast_client_discover_params, read);

	LOG_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err || data == NULL || length != sizeof(location)) {
		LOG_DBG("Unable to read PACS location for dir %s: %u, %p, %u",
			bt_audio_dir_str(params->dir), err, data, length);

		params->func(conn, NULL, NULL, params);

		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);
	location = net_buf_simple_pull_le32(&buf);

	LOG_DBG("dir %s loc %X", bt_audio_dir_str(params->dir), location);

	if (unicast_client_cbs != NULL && unicast_client_cbs->location != NULL) {
		unicast_client_cbs->location(conn, params->dir, (enum bt_audio_location)location);
	}

	/* Read available contexts */
	if (unicast_client_pacs_avail_ctx_discover(conn, params) < 0) {
		LOG_ERR("Unable to read available contexts");

		params->func(conn, NULL, NULL, params);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t unicast_client_pacs_location_notify_cb(struct bt_conn *conn,
						      struct bt_gatt_subscribe_params *params,
						      const void *data, uint16_t length)
{
	struct net_buf_simple buf;
	enum bt_audio_dir dir;
	uint32_t location;

	LOG_DBG("conn %p len %u", conn, length);

	if (!data) {
		LOG_DBG("Unsubscribed");
		params->value_handle = 0x0000;
		return BT_GATT_ITER_STOP;
	}

	/* Terminate early if there's no callbacks */
	if (unicast_client_cbs == NULL || unicast_client_cbs->location == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len != sizeof(location)) {
		LOG_ERR("Location notification incorrect size: %u", length);
		return BT_GATT_ITER_STOP;
	}

	if (params == &uni_cli_insts[bt_conn_index(conn)].snk_loc_subscribe) {
		dir = BT_AUDIO_DIR_SINK;
	} else if (params == &uni_cli_insts[bt_conn_index(conn)].src_loc_subscribe) {
		dir = BT_AUDIO_DIR_SOURCE;
	} else {
		LOG_ERR("Invalid notification");

		return BT_GATT_ITER_CONTINUE;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);
	location = net_buf_simple_pull_le32(&buf);

	LOG_DBG("dir %s loc %X", bt_audio_dir_str(dir), location);

	if (unicast_client_cbs != NULL && unicast_client_cbs->location != NULL) {
		unicast_client_cbs->location(conn, dir, (enum bt_audio_location)location);
	}

	return BT_GATT_ITER_CONTINUE;
}

static int unicast_client_pacs_location_read(struct bt_conn *conn,
					     struct bt_bap_unicast_client_discover_params *params,
					     uint16_t handle)
{
	LOG_DBG("conn %p params %p", conn, params);

	params->read.func = unicast_client_pacs_location_read_func;
	params->read.handle_count = 1U;
	params->read.single.handle = handle;
	params->read.single.offset = 0U;

	return bt_gatt_read(conn, &params->read);
}

static uint8_t unicast_client_pacs_location_discover_cb(struct bt_conn *conn,
							const struct bt_gatt_attr *attr,
							struct bt_gatt_discover_params *discover)
{
	struct bt_bap_unicast_client_discover_params *params;
	uint8_t index = bt_conn_index(conn);
	const struct bt_gatt_chrc *chrc;
	int err;

	params = CONTAINER_OF(discover, struct bt_bap_unicast_client_discover_params, discover);

	if (!attr) {
		/* If location is not found, we just continue reading the
		 * available contexts, as location is optional.
		 */
		if (unicast_client_pacs_avail_ctx_discover(conn, params) < 0) {
			LOG_ERR("Unable to read available contexts");

			params->func(conn, NULL, NULL, params);
		}

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	LOG_DBG("conn %p attr %p handle 0x%04x", conn, attr, chrc->value_handle);

	if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
		struct bt_gatt_subscribe_params *sub_params;

		if (params->dir == BT_AUDIO_DIR_SINK) {
			sub_params = &uni_cli_insts[index].snk_loc_subscribe;
		} else {
			sub_params = &uni_cli_insts[index].src_loc_subscribe;
		}

		sub_params->value_handle = chrc->value_handle;
		sub_params->ccc_handle = 0x0000; /* auto discover ccc */
		sub_params->end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		sub_params->disc_params = &uni_cli_insts[index].loc_cc_disc;
		sub_params->notify = unicast_client_pacs_location_notify_cb;
		sub_params->value = BT_GATT_CCC_NOTIFY;

		err = bt_gatt_subscribe(conn, sub_params);
		if (err != 0 && err != -EALREADY) {
			LOG_ERR("Failed to subscribe to location: %d", err);
		}
	}

	err = unicast_client_pacs_location_read(conn, params, chrc->value_handle);
	if (err != 0) {
		LOG_DBG("Failed to read PACS location: %d", err);
		params->err = err;

		params->func(conn, NULL, NULL, params);
	}

	return BT_GATT_ITER_STOP;
}

static int
unicast_client_pacs_location_discover(struct bt_conn *conn,
				      struct bt_bap_unicast_client_discover_params *params)
{
	LOG_DBG("conn %p params %p", conn, params);

	if (params->dir == BT_AUDIO_DIR_SINK) {
		params->discover.uuid = pacs_snk_loc_uuid;
	} else if (params->dir == BT_AUDIO_DIR_SOURCE) {
		params->discover.uuid = pacs_src_loc_uuid;
	} else {
		return -EINVAL;
	}

	params->discover.func = unicast_client_pacs_location_discover_cb;
	params->discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	params->discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	return bt_gatt_discover(conn, &params->discover);
}

static uint8_t unicast_client_pacs_context_read_func(struct bt_conn *conn, uint8_t err,
						     struct bt_gatt_read_params *read,
						     const void *data, uint16_t length)
{
	struct bt_bap_unicast_client_discover_params *params;
	struct net_buf_simple buf;
	struct bt_pacs_context *context;
	int index;

	params = CONTAINER_OF(read, struct bt_bap_unicast_client_discover_params, read);

	LOG_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err || length < sizeof(uint16_t) * 2) {
		goto discover_loc;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);
	context = net_buf_simple_pull_mem(&buf, sizeof(*context));

	index = bt_conn_index(conn);

discover_loc:
	/* Read ASE instances */
	if (unicast_client_pacs_location_discover(conn, params) < 0) {
		LOG_ERR("Unable to read PACS location");

		params->func(conn, NULL, NULL, params);
	}

	return BT_GATT_ITER_STOP;
}

static int
unicast_client_pacs_context_discover(struct bt_conn *conn,
				     struct bt_bap_unicast_client_discover_params *params)
{
	LOG_DBG("conn %p params %p", conn, params);

	params->read.func = unicast_client_pacs_context_read_func;
	params->read.handle_count = 0u;
	params->read.by_uuid.uuid = pacs_context_uuid;
	params->read.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->read.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	return bt_gatt_read(conn, &params->read);
}

static uint8_t unicast_client_read_func(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *read, const void *data,
					uint16_t length)
{
	struct bt_bap_unicast_client_discover_params *params;
	struct net_buf_simple buf;
	struct bt_pacs_read_rsp *rsp;

	params = CONTAINER_OF(read, struct bt_bap_unicast_client_discover_params, read);

	LOG_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err || !data) {
		params->err = err;
		goto fail;
	}

	LOG_DBG("handle 0x%04x", read->by_uuid.start_handle);

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len < sizeof(*rsp)) {
		LOG_ERR("Read response too small");
		goto fail;
	}

	rsp = net_buf_simple_pull_mem(&buf, sizeof(*rsp));

	/* If no PAC was found don't bother discovering ASE and ASE CP */
	if (!rsp->num_pac) {
		goto fail;
	}

	while (rsp->num_pac) {
		struct bt_pac_codec *pac_codec;
		struct bt_pac_ltv_data *meta, *cc;
		void *cc_ltv, *meta_ltv;
		struct bt_codec codec;

		LOG_DBG("pac #%u", params->num_caps);

		if (buf.len < sizeof(*pac_codec)) {
			LOG_ERR("Malformed PAC: remaining len %u expected %zu", buf.len,
				sizeof(*pac_codec));
			break;
		}

		pac_codec = net_buf_simple_pull_mem(&buf, sizeof(*pac_codec));

		if (buf.len < sizeof(*cc)) {
			LOG_ERR("Malformed PAC: remaining len %u expected %zu", buf.len,
				sizeof(*cc));
			break;
		}

		cc = net_buf_simple_pull_mem(&buf, sizeof(*cc));
		if (buf.len < cc->len) {
			LOG_ERR("Malformed PAC: remaining len %u expected %zu", buf.len, cc->len);
			break;
		}

		cc_ltv = net_buf_simple_pull_mem(&buf, cc->len);

		if (buf.len < sizeof(*meta)) {
			LOG_ERR("Malformed PAC: remaining len %u expected %zu", buf.len,
				sizeof(*meta));
			break;
		}

		meta = net_buf_simple_pull_mem(&buf, sizeof(*meta));
		if (buf.len < meta->len) {
			LOG_ERR("Malformed PAC: remaining len %u expected %u", buf.len, meta->len);
			break;
		}

		meta_ltv = net_buf_simple_pull_mem(&buf, meta->len);

		if (unicast_client_ep_set_codec(
			    NULL, pac_codec->id, sys_le16_to_cpu(pac_codec->cid),
			    sys_le16_to_cpu(pac_codec->vid), cc_ltv, cc->len, &codec)) {
			LOG_ERR("Unable to parse Codec");
			break;
		}

		if (unicast_client_ep_set_metadata(NULL, meta_ltv, meta->len, &codec)) {
			LOG_ERR("Unable to parse Codec Metadata");
			break;
		}

		LOG_DBG("codec 0x%02x config count %u meta count %u ", codec.id, codec.data_count,
			codec.meta_count);

		params->func(conn, &codec, NULL, params);

		rsp->num_pac--;
		params->num_caps++;
	}

	if (!params->num_caps) {
		goto fail;
	}

	/* Read PACS contexts */
	if (unicast_client_pacs_context_discover(conn, params) < 0) {
		LOG_ERR("Unable to read PACS context");
		goto fail;
	}

	return BT_GATT_ITER_STOP;

fail:
	params->func(conn, NULL, NULL, params);
	return BT_GATT_ITER_STOP;
}

static void unicast_client_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("conn %p reason 0x%02x", conn, reason);

	unicast_client_ep_reset(conn);
}

static struct bt_conn_cb conn_cbs = {
	.disconnected = unicast_client_disconnected,
};

int bt_bap_unicast_client_discover(struct bt_conn *conn,
				   struct bt_bap_unicast_client_discover_params *params)
{
	static bool conn_cb_registered;
	uint8_t role;

	if (!conn || conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	role = conn->role;
	if (role != BT_HCI_ROLE_CENTRAL) {
		LOG_DBG("Invalid conn role: %u, shall be central", role);
		return -EINVAL;
	}

	if (params->dir == BT_AUDIO_DIR_SINK) {
		params->read.by_uuid.uuid = snk_uuid;
	} else if (params->dir == BT_AUDIO_DIR_SOURCE) {
		params->read.by_uuid.uuid = src_uuid;
	} else {
		return -EINVAL;
	}

	params->read.func = unicast_client_read_func;
	params->read.handle_count = 0u;
	params->read.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->read.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	if (!conn_cb_registered) {
		bt_conn_cb_register(&conn_cbs);
		conn_cb_registered = true;
	}

	params->num_caps = 0u;
	params->num_eps = 0u;

	return bt_gatt_read(conn, &params->read);
}

int bt_bap_unicast_client_register_cb(const struct bt_bap_unicast_client_cb *cbs)
{
	CHECKIF(cbs == NULL) {
		LOG_DBG("cbs is NULL");
		return -EINVAL;
	}

	if (unicast_client_cbs != NULL) {
		LOG_DBG("Callbacks already registered");
		return -EALREADY;
	}

	unicast_client_cbs = cbs;

	return 0;
}
