/* btp_bap.c - Bluetooth BAP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stddef.h>
#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>

#include "bap_endpoint.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#define LOG_MODULE_NAME bttester_bap
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);
#include "btp/btp.h"

#define SUPPORTED_SINK_CONTEXT	BT_AUDIO_CONTEXT_TYPE_ANY
#define SUPPORTED_SOURCE_CONTEXT BT_AUDIO_CONTEXT_TYPE_ANY

#define AVAILABLE_SINK_CONTEXT SUPPORTED_SINK_CONTEXT
#define AVAILABLE_SOURCE_CONTEXT SUPPORTED_SOURCE_CONTEXT

static const struct bt_audio_codec_cap default_codec_cap = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_LC3_FREQ_ANY, BT_AUDIO_CODEC_LC3_DURATION_7_5 |
	BT_AUDIO_CODEC_LC3_DURATION_10, BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1, 2), 26u, 155u, 1u,
	BT_AUDIO_CONTEXT_TYPE_ANY);

static const struct bt_audio_codec_cap vendor_codec_cap = BT_AUDIO_CODEC_CAP(
	0xff, 0xffff, 0xffff, BT_AUDIO_CODEC_CAP_LC3_DATA(BT_AUDIO_CODEC_LC3_FREQ_ANY,
	BT_AUDIO_CODEC_LC3_DURATION_7_5 | BT_AUDIO_CODEC_LC3_DURATION_10,
	BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1, 2), 26u, 155, 1u),
	BT_AUDIO_CODEC_CAP_LC3_META(BT_AUDIO_CONTEXT_TYPE_ANY));

struct audio_stream {
	struct bt_bap_stream stream;
	atomic_t seq_num;
	uint16_t last_req_seq_num;
	uint16_t last_sent_seq_num;
	uint16_t max_sdu;
	size_t len_to_send;
	struct k_work_delayable audio_clock_work;
	struct k_work_delayable audio_send_work;
	bool already_sent;
	bool broadcast;

	union {
		/* Unicast */
		struct {
			uint8_t ase_id;
			uint8_t conn_id;
			uint8_t cig_id;
			uint8_t cis_id;
			struct bt_bap_unicast_group **cig;
		};

		/* Broadcast */
		struct {
			uint8_t bis_id;
			bool bis_synced;
		};
	};
};

#define MAX_STREAMS_COUNT MAX(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT, \
			     CONFIG_BT_ASCS_ASE_SNK_COUNT) + MAX(CONFIG_BT_ASCS_ASE_SRC_COUNT,\
			     CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT)
#define MAX_END_POINTS_COUNT CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT + \
			     CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT
struct audio_connection {
	struct audio_stream streams[MAX_STREAMS_COUNT];
	size_t configured_sink_stream_count;
	size_t configured_source_stream_count;
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_audio_codec_qos qos;
	struct bt_bap_ep *end_points[MAX_END_POINTS_COUNT];
	size_t end_points_count;
} connections[CONFIG_BT_MAX_CONN], broadcast_connection;

static struct bt_bap_unicast_group *cigs[CONFIG_BT_ISO_MAX_CIG];

static struct bt_audio_codec_qos_pref qos_pref =
	BT_AUDIO_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, 0x02, 10, 10000, 40000, 10000, 40000);

NET_BUF_POOL_FIXED_DEFINE(tx_pool, MAX(CONFIG_BT_ASCS_ASE_SRC_COUNT,
				       CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT),
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);

static struct net_buf_simple *rx_ev_buf = NET_BUF_SIMPLE(CONFIG_BT_ISO_RX_MTU +
							 sizeof(struct btp_bap_stream_received_ev));

RING_BUF_DECLARE(audio_ring_buf, CONFIG_BT_ISO_TX_MTU);
static void audio_clock_timeout(struct k_work *work);
static void audio_send_timeout(struct k_work *work);

#define ISO_DATA_THREAD_STACK_SIZE 512
#define ISO_DATA_THREAD_PRIORITY -7
K_THREAD_STACK_DEFINE(iso_data_thread_stack_area, ISO_DATA_THREAD_STACK_SIZE);
static struct k_work_q iso_data_work_q;

static struct bt_bap_broadcast_source *broadcast_source;
static struct audio_connection *broadcaster;
static struct bt_bap_broadcast_sink *broadcast_sink;
static bt_addr_le_t broadcaster_addr;
static uint32_t broadcaster_broadcast_id;
static struct bt_bap_stream *sink_streams[MAX_STREAMS_COUNT];
/* A mask for the maximum BIS we can sync to. +1 since the BIS indexes start from 1. */
static const uint32_t bis_index_mask = BIT_MASK(MAX_STREAMS_COUNT + 1);
static uint32_t bis_index_bitfield;
#define INVALID_BROADCAST_ID      (BT_AUDIO_BROADCAST_ID_MAX + 1)

static bool print_cb(struct bt_data *data, void *user_data)
{
	const char *str = (const char *)user_data;

	LOG_DBG("%s: type 0x%02x value_len %u", str, data->type, data->data_len);
	LOG_HEXDUMP_DBG(data->data, data->data_len, "");

	return true;
}

static void print_codec_cfg(const struct bt_audio_codec_cfg *codec_cfg)
{
	LOG_DBG("codec_cfg 0x%02x cid 0x%04x vid 0x%04x count %u", codec_cfg->id, codec_cfg->cid,
		codec_cfg->vid, codec_cfg->data_len);

	if (codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
		enum bt_audio_location chan_allocation;
		int ret;

		/* LC3 uses the generic LTV format - other codecs might do as well */

		bt_audio_data_parse(codec_cfg->data, codec_cfg->data_len, print_cb, "data");

		ret = bt_audio_codec_cfg_get_freq(codec_cfg);
		if (ret > 0) {
			LOG_DBG("  Frequency: %d Hz", bt_audio_codec_cfg_freq_to_freq_hz(ret));
		}

		LOG_DBG("  Frame Duration: %d us",
			bt_audio_codec_cfg_get_frame_duration_us(codec_cfg));
		if (bt_audio_codec_cfg_get_chan_allocation(codec_cfg, &chan_allocation) == 0) {
			LOG_DBG("  Channel allocation: 0x%x", chan_allocation);
		}

		LOG_DBG("  Octets per frame: %d (negative means value not pressent)",
			bt_audio_codec_cfg_get_octets_per_frame(codec_cfg));
		LOG_DBG("  Frames per SDU: %d",
			bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, true));
	} else {
		LOG_HEXDUMP_DBG(codec_cfg->data, codec_cfg->data_len, "data");
	}

	bt_audio_data_parse(codec_cfg->meta, codec_cfg->meta_len, print_cb, "meta");
}

static void print_codec_cap(const struct bt_audio_codec_cap *codec_cap)
{
	LOG_DBG("codec_cap 0x%02x cid 0x%04x vid 0x%04x count %zu", codec_cap->id, codec_cap->cid,
		codec_cap->vid, codec_cap->data_len);

	if (codec_cap->id == BT_HCI_CODING_FORMAT_LC3) {
		bt_audio_data_parse(codec_cap->data, codec_cap->data_len, print_cb, "data");
	} else {
		LOG_HEXDUMP_DBG(codec_cap->data, codec_cap->data_len, "data");
	}

	bt_audio_data_parse(codec_cap->meta, codec_cap->meta_len, print_cb, "meta");
}

static inline void print_qos(const struct bt_audio_codec_qos *qos)
{
	LOG_DBG("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u rtn %u latency %u pd %u",
		qos->interval, qos->framing, qos->phy, qos->sdu, qos->rtn, qos->latency, qos->pd);
}

static struct audio_stream *stream_alloc(struct audio_connection *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn->streams); i++) {
		struct audio_stream *stream = &conn->streams[i];

		if (!stream->stream.conn) {
			return stream;
		}
	}

	return NULL;
}

static void stream_free(struct audio_stream *stream)
{
	(void)memset(stream, 0, sizeof(*stream));
}

static struct audio_stream *stream_find(struct audio_connection *conn, uint8_t ase_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn->streams); i++) {
		struct bt_bap_stream *stream = &conn->streams[i].stream;

		if (stream->ep != NULL && stream->ep->status.id == ase_id) {
			return &conn->streams[i];
		}
	}

	return NULL;
}

static struct bt_bap_ep *end_point_find(struct audio_connection *conn, uint8_t ase_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn->end_points); i++) {
		struct bt_bap_ep *ep = conn->end_points[i];

		if (ep->status.id == ase_id) {
			return ep;
		}
	}

	return NULL;
}

static void btp_send_ascs_operation_completed_ev(struct bt_conn *conn, uint8_t ase_id,
						 uint8_t opcode, uint8_t status)
{
	struct btp_ascs_operation_completed_ev ev;
	struct bt_conn_info info;

	(void)bt_conn_get_info(conn, &info);
	bt_addr_le_copy(&ev.address, info.le.dst);
	ev.ase_id = ase_id;
	ev.opcode = opcode;
	ev.status = status;
	ev.flags = 0;

	tester_event(BTP_SERVICE_ID_ASCS, BTP_ASCS_EV_OPERATION_COMPLETED, &ev, sizeof(ev));
}

static void btp_send_ascs_ase_state_changed_ev(struct bt_conn *conn, uint8_t ase_id, uint8_t state)
{
	struct btp_ascs_ase_state_changed_ev ev;
	struct bt_conn_info info;

	(void)bt_conn_get_info(conn, &info);
	bt_addr_le_copy(&ev.address, info.le.dst);
	ev.ase_id = ase_id;
	ev.state = state;

	tester_event(BTP_SERVICE_ID_ASCS, BTP_ASCS_EV_ASE_STATE_CHANGED, &ev, sizeof(ev));
}

static int validate_codec_parameters(const struct bt_audio_codec_cfg *codec_cfg)
{
	int frame_duration_us;
	int frames_per_sdu;
	int octets_per_frame;
	int chan_allocation_err;
	enum bt_audio_location chan_allocation;
	int ret;

	frame_duration_us = bt_audio_codec_cfg_get_frame_duration_us(codec_cfg);
	chan_allocation_err =
		bt_audio_codec_cfg_get_chan_allocation(codec_cfg, &chan_allocation);
	octets_per_frame = bt_audio_codec_cfg_get_octets_per_frame(codec_cfg);
	frames_per_sdu = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, true);

	ret = bt_audio_codec_cfg_get_freq(codec_cfg);
	if (ret < 0) {
		LOG_DBG("Error: Invalid codec frequency: %d", ret);
		return -EINVAL;
	}

	if (frame_duration_us < 0) {
		LOG_DBG("Error: Invalid frame duration.");
		return -EINVAL;
	}

	if (octets_per_frame < 0) {
		LOG_DBG("Error: Invalid octets per frame.");
		return -EINVAL;
	}

	if (frames_per_sdu < 0) {
		/* The absence of the Codec_Frame_Blocks_Per_SDU LTV structure shall be
		 * interpreted as equivalent to a Codec_Frame_Blocks_Per_SDU value of 0x01
		 */
		LOG_DBG("Codec_Frame_Blocks_Per_SDU LTV structure not defined.");
	}

	if (chan_allocation_err < 0) {
		/* The absence of the Audio_Channel_Allocation LTV structure shall be
		 * interpreted as a single channel with no specified Audio Location.
		 */
		LOG_DBG("Audio_Channel_Allocation LTV structure not defined.");
	}

	return 0;
}

static int lc3_config(struct bt_conn *conn, const struct bt_bap_ep *ep, enum bt_audio_dir dir,
		      const struct bt_audio_codec_cfg *codec_cfg, struct bt_bap_stream **stream,
		      struct bt_audio_codec_qos_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	struct audio_connection *audio_conn;
	struct audio_stream *stream_wrap;

	LOG_DBG("ASE Codec Config: ep %p dir %u", ep, dir);

	print_codec_cfg(codec_cfg);

	if (validate_codec_parameters(codec_cfg)) {
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_CONF_REJECTED,
				       BT_BAP_ASCS_REASON_CODEC_DATA);

		btp_send_ascs_operation_completed_ev(conn, ep->status.id, BT_ASCS_CONFIG_OP,
						     BTP_ASCS_STATUS_FAILED);

		return -ENOTSUP;
	}

	audio_conn = &connections[bt_conn_index(conn)];
	stream_wrap = stream_alloc(audio_conn);

	if (stream_wrap == NULL) {
		LOG_DBG("No free stream available");
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_NO_MEM, BT_BAP_ASCS_REASON_NONE);

		btp_send_ascs_operation_completed_ev(conn, ep->status.id, BT_ASCS_CONFIG_OP,
						     BTP_ASCS_STATUS_FAILED);

		return -ENOMEM;
	}

	*stream = &stream_wrap->stream;
	LOG_DBG("ASE Codec Config stream %p", *stream);

	if (dir == BT_AUDIO_DIR_SOURCE) {
		audio_conn->configured_source_stream_count++;
	} else {
		audio_conn->configured_sink_stream_count++;
	}

	*pref = qos_pref;

	return 0;
}

static int lc3_reconfig(struct bt_bap_stream *stream, enum bt_audio_dir dir,
			const struct bt_audio_codec_cfg *codec_cfg,
			struct bt_audio_codec_qos_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("ASE Codec Reconfig: stream %p", stream);

	print_codec_cfg(codec_cfg);

	return 0;
}

static int lc3_qos(struct bt_bap_stream *stream, const struct bt_audio_codec_qos *qos,
		   struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("QoS: stream %p qos %p", stream, qos);

	print_qos(qos);

	return 0;
}

static bool valid_metadata_type(uint8_t type, uint8_t len, const uint8_t *data)
{
	/* PTS checks if we are able to reject unsupported metadata type or RFU vale.
	 * The only RFU value PTS seems to check for now is the streaming context.
	 */
	if (!BT_AUDIO_METADATA_TYPE_IS_KNOWN(type)) {
		return false;
	}

	if (type == BT_AUDIO_METADATA_TYPE_PREF_CONTEXT ||
	    type == BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT) {
		/* PTS wants us to reject the parameter if reserved bits are set */
		if ((sys_get_le16(data) & ~(uint16_t)(BT_AUDIO_CONTEXT_TYPE_ANY)) > 0) {
			return false;
		}
	}

	return true;
}

static bool data_func_cb(struct bt_data *data, void *user_data)
{
	struct bt_bap_ascs_rsp *rsp = (struct bt_bap_ascs_rsp *)user_data;

	if (!valid_metadata_type(data->type, data->data_len, data->data)) {
		LOG_DBG("Invalid metadata type %u or length %u", data->type, data->data_len);
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED, data->type);
		return false;
	}

	return true;
}

static int lc3_enable(struct bt_bap_stream *stream, const uint8_t meta[], size_t meta_len,
		      struct bt_bap_ascs_rsp *rsp)
{
	int err;

	LOG_DBG("Metadata: stream %p meta_len %zu", stream, meta_len);

	err = bt_audio_data_parse(meta, meta_len, data_func_cb, rsp);
	if (err != 0) {
		btp_send_ascs_operation_completed_ev(stream->conn, stream->ep->status.id,
						     BT_ASCS_ENABLE_OP, BTP_ASCS_STATUS_FAILED);
	}

	return err;
}

static int lc3_start(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Start: stream %p", stream);

	return 0;
}

static int lc3_metadata(struct bt_bap_stream *stream, const uint8_t meta[], size_t meta_len,
			struct bt_bap_ascs_rsp *rsp)
{
	int err;

	LOG_DBG("Metadata: stream %p meta_count %zu", stream, meta_len);

	err = bt_audio_data_parse(meta, meta_len, data_func_cb, rsp);
	if (err != 0) {
		btp_send_ascs_operation_completed_ev(stream->conn, stream->ep->status.id,
						     BT_ASCS_METADATA_OP, BTP_ASCS_STATUS_FAILED);
	}

	return err;
}

static int lc3_disable(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Disable: stream %p", stream);

	return 0;
}

static int lc3_stop(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Stop: stream %p", stream);

	return 0;
}

static int lc3_release(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Release: stream %p", stream);

	return 0;
}

static const struct bt_bap_unicast_server_cb unicast_server_cb = {
	.config = lc3_config,
	.reconfig = lc3_reconfig,
	.qos = lc3_qos,
	.enable = lc3_enable,
	.start = lc3_start,
	.metadata = lc3_metadata,
	.disable = lc3_disable,
	.stop = lc3_stop,
	.release = lc3_release,
};

static void btp_send_stream_received_ev(struct bt_conn *conn, struct bt_bap_ep *ep,
					uint8_t data_len, uint8_t *data)
{
	struct btp_bap_stream_received_ev *ev;
	struct bt_conn_info info;

	LOG_DBG("Stream received, ep %d, dir %d, len %d", ep->status.id, ep->dir,
		data_len);

	(void)bt_conn_get_info(conn, &info);

	net_buf_simple_init(rx_ev_buf, 0);

	ev = net_buf_simple_add(rx_ev_buf, sizeof(*ev));

	bt_addr_le_copy(&ev->address, info.le.dst);

	ev->ase_id = ep->status.id;
	ev->data_len = data_len;
	memcpy(ev->data, data, data_len);

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_STREAM_RECEIVED, ev, sizeof(*ev) + data_len);
}

static void btp_send_bis_syced_ev(const bt_addr_le_t *address, uint32_t broadcast_id,
				  uint8_t bis_id)
{
	struct btp_bap_bis_syned_ev ev;

	bt_addr_le_copy(&ev.address, address);
	sys_put_le24(broadcast_id, ev.broadcast_id);
	ev.bis_id = bis_id;

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BIS_SYNCED, &ev, sizeof(ev));
}

static void btp_send_bis_stream_received_ev(const bt_addr_le_t *address, uint32_t broadcast_id,
					    uint8_t bis_id, uint8_t data_len, uint8_t *data)
{
	struct btp_bap_bis_stream_received_ev *ev;

	LOG_DBG("Stream received, len %d", data_len);

	net_buf_simple_init(rx_ev_buf, 0);

	ev = net_buf_simple_add(rx_ev_buf, sizeof(*ev));

	bt_addr_le_copy(&ev->address, address);
	sys_put_le24(broadcast_id, ev->broadcast_id);
	ev->bis_id = bis_id;
	ev->data_len = data_len;
	memcpy(ev->data, data, data_len);

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BIS_STREAM_RECEIVED, ev,
		     sizeof(*ev) + data_len);
}

static void stream_configured(struct bt_bap_stream *stream,
			      const struct bt_audio_codec_qos_pref *pref)
{
	struct audio_connection *audio_conn;
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	LOG_DBG("Configured stream %p, ep %u, dir %u", stream, stream->ep->status.id,
		stream->ep->dir);
	a_stream->conn_id = bt_conn_index(stream->conn);
	audio_conn = &connections[a_stream->conn_id];
	a_stream->ase_id = stream->ep->status.id;

	btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
					     BT_ASCS_CONFIG_OP, BTP_ASCS_STATUS_SUCCESS);
}

static void stream_qos_set(struct bt_bap_stream *stream)
{
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	LOG_DBG("QoS set stream %p", stream);
	btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
					     BT_ASCS_QOS_OP, BTP_ASCS_STATUS_SUCCESS);
}

static void stream_enabled(struct bt_bap_stream *stream)
{
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);
	struct bt_bap_ep_info info;
	struct bt_conn_info conn_info;
	int err;

	LOG_DBG("Enabled stream %p", stream);

	(void)bt_bap_ep_get_info(stream->ep, &info);
	(void)bt_conn_get_info(stream->conn, &conn_info);
	if (conn_info.role == BT_HCI_ROLE_PERIPHERAL && info.dir == BT_AUDIO_DIR_SINK) {
		/* Automatically do the receiver start ready operation */
		err = bt_bap_stream_start(&a_stream->stream);
		if (err != 0) {
			LOG_DBG("Failed to start stream %p", stream);
			btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
							     BT_ASCS_ENABLE_OP,
							     BTP_ASCS_STATUS_FAILED);

			return;
		}
	}

	btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
					     BT_ASCS_ENABLE_OP, BTP_ASCS_STATUS_SUCCESS);
}

static void stream_metadata_updated(struct bt_bap_stream *stream)
{
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	LOG_DBG("Metadata updated stream %p", stream);
	btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
					     BT_ASCS_METADATA_OP, BTP_ASCS_STATUS_SUCCESS);
}

static void stream_disabled(struct bt_bap_stream *stream)
{
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	LOG_DBG("Disabled stream %p", stream);

	/* Stop send timer */
	k_work_cancel_delayable(&a_stream->audio_clock_work);
	k_work_cancel_delayable(&a_stream->audio_send_work);

	btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
					     BT_ASCS_DISABLE_OP, BTP_ASCS_STATUS_SUCCESS);
}

static void stream_released(struct bt_bap_stream *stream)
{
	struct audio_connection *audio_conn;
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	LOG_DBG("Released stream %p", stream);

	audio_conn = &connections[a_stream->conn_id];

	/* Stop send timer */
	k_work_cancel_delayable(&a_stream->audio_clock_work);
	k_work_cancel_delayable(&a_stream->audio_send_work);

	if (cigs[stream->ep->cig_id] != NULL) {
		/* The unicast group will be deleted only at release of the last stream */
		LOG_DBG("Deleting unicast group");

		int err = bt_bap_unicast_group_delete(cigs[stream->ep->cig_id]);

		if (err != 0) {
			LOG_DBG("Unable to delete unicast group: %d", err);

			return;
		}

		cigs[stream->ep->cig_id] = NULL;
	}

	if (stream->ep->dir == BT_AUDIO_DIR_SINK) {
		audio_conn->configured_sink_stream_count--;
	} else {
		audio_conn->configured_source_stream_count--;
	}

	stream_free(a_stream);
}

static void stream_started(struct bt_bap_stream *stream)
{
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);
	struct bt_bap_ep_info info;

	/* Callback called on transition to Streaming state */

	LOG_DBG("Started stream %p", stream);

	(void)bt_bap_ep_get_info(stream->ep, &info);
	if (info.can_send == true) {
		/* Schedule first TX ISO data at seq_num 1 instead of 0 to ensure
		 * we are in sync with the controller at start of streaming.
		 */
		a_stream->seq_num = 1;

		/* Run audio clock work in system work queue */
		k_work_init_delayable(&a_stream->audio_clock_work, audio_clock_timeout);
		k_work_schedule(&a_stream->audio_clock_work, K_NO_WAIT);

		/* Run audio send work in user defined work queue */
		k_work_init_delayable(&a_stream->audio_send_work, audio_send_timeout);
		k_work_schedule_for_queue(&iso_data_work_q, &a_stream->audio_send_work,
					  K_USEC(a_stream->stream.qos->interval));
	}

	if (a_stream->broadcast) {
		btp_send_bis_syced_ev(&broadcaster_addr, broadcaster_broadcast_id,
				      a_stream->bis_id);
	} else {
		btp_send_ascs_ase_state_changed_ev(stream->conn, a_stream->ase_id,
						   stream->ep->status.state);
	}
}

static void stream_stopped(struct bt_bap_stream *stream, uint8_t reason)
{
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	LOG_DBG("Stopped stream %p with reason 0x%02X", stream, reason);

	/* Stop send timer */
	k_work_cancel_delayable(&a_stream->audio_clock_work);
	k_work_cancel_delayable(&a_stream->audio_send_work);

	if (!a_stream->broadcast) {
		btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
						     BT_ASCS_STOP_OP, BTP_STATUS_SUCCESS);
	}
}

static void stream_recv(struct bt_bap_stream *stream,
			const struct bt_iso_recv_info *info,
			struct net_buf *buf)
{
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	if (a_stream->already_sent == false) {
		/* For now, send just a first packet, to limit the number
		 * of logs and not unnecessarily spam through btp.
		 */
		LOG_DBG("Incoming audio on stream %p len %u", stream, buf->len);
		a_stream->already_sent = true;

		if (a_stream->broadcast) {
			btp_send_bis_stream_received_ev(&broadcaster_addr, broadcaster_broadcast_id,
							a_stream->bis_id, buf->len, buf->data);
		} else {
			btp_send_stream_received_ev(stream->conn, stream->ep, buf->len, buf->data);
		}
	}
}

static void stream_sent(struct bt_bap_stream *stream)
{
	LOG_DBG("Stream %p sent", stream);
}

static struct bt_bap_stream_ops stream_ops = {
	.configured = stream_configured,
	.qos_set = stream_qos_set,
	.enabled = stream_enabled,
	.metadata_updated = stream_metadata_updated,
	.disabled = stream_disabled,
	.released = stream_released,
	.started = stream_started,
	.stopped = stream_stopped,
	.recv = stream_recv,
	.sent = stream_sent,
};

static void btp_send_discovery_completed_ev(struct bt_conn *conn, uint8_t status)
{
	struct btp_bap_discovery_completed_ev ev;
	struct bt_conn_info info;

	(void) bt_conn_get_info(conn, &info);
	bt_addr_le_copy(&ev.address, info.le.dst);
	ev.status = status;

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_DISCOVERY_COMPLETED, &ev, sizeof(ev));
}

struct search_type_param {
	uint8_t type;
	const uint8_t *data;
};

static bool data_type_search_cb(struct bt_data *data, void *user_data)
{
	struct search_type_param *param = (struct search_type_param *)user_data;

	if (param->type == data->type) {
		param->data = data->data;

		return false;
	}

	return true;
}

static bool codec_cap_get_val(const struct bt_audio_codec_cap *codec_cap, uint8_t type,
			      const uint8_t **data)
{
	struct search_type_param param = {
		.type = type,
		.data = NULL,
	};
	int err;

	err = bt_audio_data_parse(codec_cap->data, codec_cap->data_len, data_type_search_cb,
				  &param);
	if (err != 0 && err != -ECANCELED) {
		LOG_DBG("Could not parse the data: %d", err);

		return false;
	}

	if (param.data == NULL) {
		LOG_DBG("Could not find the type %u", type);

		return false;
	}

	*data = param.data;

	return true;
}

static void btp_send_pac_codec_found_ev(struct bt_conn *conn,
					const struct bt_audio_codec_cap *codec_cap,
					enum bt_audio_dir dir)
{
	struct btp_bap_codec_cap_found_ev ev;
	struct bt_conn_info info;
	const uint8_t *data;

	(void)bt_conn_get_info(conn, &info);
	bt_addr_le_copy(&ev.address, info.le.dst);

	ev.dir = dir;
	ev.coding_format = codec_cap->id;

	if (codec_cap_get_val(codec_cap, BT_AUDIO_CODEC_LC3_FREQ, &data)) {
		memcpy(&ev.frequencies, data, sizeof(ev.frequencies));
	}

	if (codec_cap_get_val(codec_cap, BT_AUDIO_CODEC_LC3_DURATION, &data)) {
		memcpy(&ev.frame_durations, data, sizeof(ev.frame_durations));
	}

	if (codec_cap_get_val(codec_cap, BT_AUDIO_CODEC_LC3_FRAME_LEN, &data)) {
		memcpy(&ev.octets_per_frame, data, sizeof(ev.octets_per_frame));
	}

	if (codec_cap_get_val(codec_cap, BT_AUDIO_CODEC_LC3_CHAN_COUNT, &data)) {
		memcpy(&ev.channel_counts, data, sizeof(ev.channel_counts));
	}

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_CODEC_CAP_FOUND, &ev, sizeof(ev));
}

static void btp_send_ase_found_ev(struct bt_conn *conn, struct bt_bap_ep *ep)
{
	struct btp_ascs_ase_found_ev ev;
	struct bt_conn_info info;

	(void)bt_conn_get_info(conn, &info);
	bt_addr_le_copy(&ev.address, info.le.dst);

	ev.ase_id = ep->status.id;
	ev.dir = ep->dir;

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_ASE_FOUND, &ev, sizeof(ev));
}

static void unicast_client_location_cb(struct bt_conn *conn,
				       enum bt_audio_dir dir,
				       enum bt_audio_location loc)
{
	LOG_DBG("dir %u loc %X", dir, loc);
}

static void available_contexts_cb(struct bt_conn *conn,
				  enum bt_audio_context snk_ctx,
				  enum bt_audio_context src_ctx)
{
	LOG_DBG("snk ctx %u src ctx %u", snk_ctx, src_ctx);
}


static void config_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		      enum bt_bap_ascs_reason reason)
{
	LOG_DBG("stream %p config operation rsp_code %u reason %u", stream, rsp_code, reason);
}

static void qos_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		   enum bt_bap_ascs_reason reason)
{
	LOG_DBG("stream %p qos operation rsp_code %u reason %u", stream, rsp_code, reason);
}

static void enable_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		      enum bt_bap_ascs_reason reason)
{
	LOG_DBG("stream %p enable operation rsp_code %u reason %u", stream, rsp_code, reason);
}

static void start_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		     enum bt_bap_ascs_reason reason)
{
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	/* Callback called on Receiver Start Ready notification from ASE Control Point */

	LOG_DBG("stream %p start operation rsp_code %u reason %u", stream, rsp_code, reason);
	a_stream->already_sent = false;

	btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
					     BT_ASCS_START_OP, BTP_STATUS_SUCCESS);
}

static void stop_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		    enum bt_bap_ascs_reason reason)
{
	LOG_DBG("stream %p stop operation rsp_code %u reason %u", stream, rsp_code, reason);
}

static void disable_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		       enum bt_bap_ascs_reason reason)
{
	LOG_DBG("stream %p disable operation rsp_code %u reason %u", stream, rsp_code, reason);
}

static void metadata_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
			enum bt_bap_ascs_reason reason)
{
	LOG_DBG("stream %p metadata operation rsp_code %u reason %u", stream, rsp_code, reason);
}

static void release_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		       enum bt_bap_ascs_reason reason)
{
	LOG_DBG("stream %p release operation rsp_code %u reason %u", stream, rsp_code, reason);
}

static void pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir,
			  const struct bt_audio_codec_cap *codec_cap)
{
	LOG_DBG("");

	if (codec_cap != NULL) {
		LOG_DBG("Discovered codec capabilities %p", codec_cap);
		print_codec_cap(codec_cap);
		btp_send_pac_codec_found_ev(conn, codec_cap, dir);
	}
}

static void endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	struct audio_connection *audio_conn;

	LOG_DBG("");

	if (ep != NULL) {
		LOG_DBG("Discovered ASE %p, id %u, dir 0x%02x", ep, ep->status.id, ep->dir);

		audio_conn = &connections[bt_conn_index(conn)];

		if (audio_conn->end_points_count >= CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +
						    CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT) {
			LOG_DBG("Failed to cache ep %p due to configured limit: %zu", ep,
				audio_conn->end_points_count);

			btp_send_discovery_completed_ev(conn, BTP_BAP_DISCOVERY_STATUS_FAILED);

			return;
		}

		audio_conn->end_points[audio_conn->end_points_count++] = ep;
		btp_send_ase_found_ev(conn, ep);

		return;
	}
}

static void discover_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	LOG_DBG("");

	if (err != 0 && err != BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		LOG_DBG("Discover remote ASEs failed: %d", err);
		btp_send_discovery_completed_ev(conn, BTP_BAP_DISCOVERY_STATUS_FAILED);
		return;
	}

	LOG_DBG("Discover complete");

	if (err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		LOG_DBG("Discover remote ASEs completed without finding any source ASEs");
	} else {
		LOG_DBG("Discover remote ASEs complete: err %d", err);
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		err = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SOURCE);

		if (err != 0) {
			LOG_DBG("Failed to discover source ASEs: %d", err);
			btp_send_discovery_completed_ev(conn, BTP_BAP_DISCOVERY_STATUS_FAILED);
		}

		return;
	}

	btp_send_discovery_completed_ev(conn, BTP_BAP_DISCOVERY_STATUS_SUCCESS);
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
	.discover = discover_cb,
};

static uint8_t bap_discover(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	const struct btp_bap_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	struct audio_connection *audio_conn;
	struct bt_conn_info conn_info;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];
	(void)bt_conn_get_info(conn, &conn_info);

	if (audio_conn->end_points_count > 0 || conn_info.role != BT_HCI_ROLE_CENTRAL) {
		bt_conn_unref(conn);

		return BTP_STATUS_FAILED;
	}

	err = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SINK);
	if (err != 0) {
		LOG_DBG("Failed to discover remote ASEs: %d", err);
		bt_conn_unref(conn);

		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);

	return BTP_STATUS_SUCCESS;
}

static void audio_clock_timeout(struct k_work *work)
{
	struct audio_stream *stream;
	struct k_work_delayable *dwork;

	dwork = k_work_delayable_from_work(work);
	stream = CONTAINER_OF(dwork, struct audio_stream, audio_clock_work);
	atomic_inc(&stream->seq_num);

	k_work_schedule(dwork, K_USEC(stream->stream.qos->interval));
}

static void audio_send_timeout(struct k_work *work)
{
	struct bt_iso_tx_info info;
	struct audio_stream *stream;
	struct k_work_delayable *dwork;
	struct net_buf *buf;
	uint32_t size;
	uint8_t *data;
	int err;

	dwork = k_work_delayable_from_work(work);
	stream = CONTAINER_OF(dwork, struct audio_stream, audio_send_work);

	if (stream->last_req_seq_num % 201 == 200) {
		err = bt_bap_stream_get_tx_sync(&stream->stream, &info);
		if (err != 0) {
			LOG_DBG("Failed to get last seq num: err %d", err);
		} else if (stream->last_req_seq_num > info.seq_num) {
			LOG_DBG("Previous TX request rejected by the controller: requested seq %u,"
				" last accepted seq %u", stream->last_req_seq_num, info.seq_num);
			stream->last_sent_seq_num = info.seq_num;
		} else {
			LOG_DBG("Host and Controller sequence number is in sync.");
			stream->last_sent_seq_num = info.seq_num;
		}
		/* TODO: Synchronize the Host clock with the Controller clock */
	}

	buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("Cannot allocate net_buf. Dropping data.");
		k_work_schedule_for_queue(&iso_data_work_q, dwork,
					  K_USEC(stream->stream.qos->interval));
		return;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	/* Get buffer within a ring buffer memory */
	size = ring_buf_get_claim(&audio_ring_buf, &data, stream->stream.qos->sdu);
	if (size != 0) {
		net_buf_add_mem(buf, data, size);
	} else {
		k_work_schedule_for_queue(&iso_data_work_q, dwork,
					  K_USEC(stream->stream.qos->interval));
		return;
	}

	/* Because the seq_num field of the audio_stream struct is atomic_val_t (4 bytes),
	 * let's allow an overflow and just cast it to uint16_t.
	 */
	stream->last_req_seq_num = (uint16_t)atomic_get(&stream->seq_num);

	LOG_DBG("Sending data to stream %p len %d seq %d", &stream->stream, size,
		stream->last_req_seq_num);

	err = bt_bap_stream_send(&stream->stream, buf, 0, BT_ISO_TIMESTAMP_NONE);
	if (err != 0) {
		LOG_ERR("Failed to send audio data to stream %p, err %d", &stream->stream, err);
		net_buf_unref(buf);
	}

	if (size != 0) {
		/* Free ring buffer memory */
		err = ring_buf_get_finish(&audio_ring_buf, size);
		if (err != 0) {
			LOG_ERR("Error freeing ring buffer memory: %d", err);
		}
	}

	k_work_schedule_for_queue(&iso_data_work_q, dwork,
				  K_USEC(stream->stream.qos->interval));
}

static uint8_t bap_send(const void *cmd, uint16_t cmd_len,
			void *rsp, uint16_t *rsp_len)
{
	struct btp_bap_send_rp *rp = rsp;
	const struct btp_bap_send_cmd *cp = cmd;
	struct audio_connection *audio_conn;
	struct audio_stream *stream;
	struct bt_conn *conn;
	struct bt_bap_ep_info info;
	uint32_t ret;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];

	stream = stream_find(audio_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	(void)bt_bap_ep_get_info(stream->stream.ep, &info);
	if (info.can_send == false) {
		return BTP_STATUS_FAILED;
	}

	ret = ring_buf_put(&audio_ring_buf, cp->data, cp->data_len);

	rp->data_len = ret;
	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static int setup_broadcast_source(uint8_t streams_per_subgroup,	uint8_t subgroups,
				  struct bt_bap_broadcast_source **source)
{
	int err;
	struct bt_bap_broadcast_source_stream_param
		stream_params[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
	struct bt_bap_broadcast_source_subgroup_param
		subgroup_param[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
	struct bt_bap_broadcast_source_param create_param;

	if (streams_per_subgroup * subgroups > CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT ||
	    subgroups >	CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < subgroups; i++) {
		subgroup_param[i].params_count = streams_per_subgroup;
		subgroup_param[i].params = stream_params + i * streams_per_subgroup;
		subgroup_param[i].codec_cfg = &broadcaster->codec_cfg;
	}

	for (size_t j = 0U; j < streams_per_subgroup; j++) {
		broadcaster->streams[j].broadcast = true;
		stream_params[j].stream = &broadcaster->streams[j].stream;
		stream_params[j].data = NULL;
		stream_params[j].data_len = 0U;
		bt_bap_stream_cb_register(stream_params[j].stream, &stream_ops);
	}

	create_param.params_count = subgroups;
	create_param.params = subgroup_param;
	create_param.qos = &broadcaster->qos;
	create_param.encryption = false;
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;

	LOG_DBG("Creating broadcast source with %zu subgroups with %zu streams",
		subgroups, subgroups * streams_per_subgroup);

	if (*source == NULL) {
		err = bt_bap_broadcast_source_create(&create_param, source);
		if (err != 0) {
			LOG_DBG("Unable to create broadcast source: %d", err);

			return err;
		}
	} else {
		err = bt_bap_broadcast_source_reconfig(*source, &create_param);
		if (err != 0) {
			LOG_DBG("Unable to reconfig broadcast source: %d", err);

			return err;
		}
	}

	return 0;
}

static uint8_t broadcast_source_setup(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_bap_broadcast_source_setup_cmd *cp = cmd;
	struct btp_bap_broadcast_source_setup_rp *rp = rsp;
	struct bt_le_adv_param *param = BT_LE_EXT_ADV_NCONN_NAME;
	uint32_t broadcast_id;
	uint32_t gap_settings = BIT(BTP_GAP_SETTINGS_DISCOVERABLE) |
				BIT(BTP_GAP_SETTINGS_EXTENDED_ADVERTISING);

	NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);

	/* Broadcast Audio Streaming Endpoint advertising data */
	struct bt_data base_ad;
	struct bt_data per_ad;

	LOG_DBG("");

	broadcaster->codec_cfg.id = cp->coding_format;
	broadcaster->codec_cfg.vid = cp->vid;
	broadcaster->codec_cfg.cid = cp->cid;
	broadcaster->codec_cfg.data_len = cp->cc_ltvs_len;
	memcpy(broadcaster->codec_cfg.data, cp->cc_ltvs, cp->cc_ltvs_len);

	broadcaster->qos.phy = BT_AUDIO_CODEC_QOS_2M;
	broadcaster->qos.framing = cp->framing;
	broadcaster->qos.rtn = cp->retransmission_num;
	broadcaster->qos.latency = sys_le16_to_cpu(cp->max_transport_latency);
	broadcaster->qos.interval = sys_get_le24(cp->sdu_interval);
	broadcaster->qos.pd = sys_get_le24(cp->presentation_delay);
	broadcaster->qos.sdu = sys_le16_to_cpu(cp->max_sdu);

	err = setup_broadcast_source(cp->streams_per_subgroup, cp->subgroups, &broadcast_source);
	if (err != 0) {
		LOG_DBG("Unable to setup broadcast source: %d", err);

		return BTP_STATUS_FAILED;
	}

	err = bt_bap_broadcast_source_get_id(broadcast_source, &broadcast_id);
	if (err != 0) {
		LOG_DBG("Unable to get broadcast ID: %d", err);

		return BTP_STATUS_FAILED;
	}

	/* Setup extended advertising data */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, broadcast_id);
	base_ad.type = BT_DATA_SVC_DATA16;
	base_ad.data_len = ad_buf.len;
	base_ad.data = ad_buf.data;
	err = tester_gap_create_adv_instance(param, BTP_GAP_ADDR_TYPE_IDENTITY, &base_ad, 1, NULL,
					     0, &gap_settings);
	if (err != 0) {
		LOG_DBG("Failed to create extended advertising instance: %d", err);

		return BTP_STATUS_FAILED;
	}

	err = tester_gap_padv_configure(BT_LE_PER_ADV_PARAM(BT_GAP_PER_ADV_FAST_INT_MIN_2,
							    BT_GAP_PER_ADV_FAST_INT_MAX_2,
							    BT_LE_PER_ADV_OPT_USE_TX_POWER));
	if (err != 0) {
		LOG_DBG("Failed to configure periodic advertising: %d", err);

		return BTP_STATUS_FAILED;
	}

	err = bt_bap_broadcast_source_get_base(broadcast_source, &base_buf);
	if (err != 0) {
		LOG_DBG("Failed to get encoded BASE: %d\n", err);

		return BTP_STATUS_FAILED;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = tester_gap_padv_set_data(&per_ad, 1);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	rp->gap_settings = gap_settings;
	sys_put_le24(broadcast_id, rp->broadcast_id);
	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t broadcast_source_release(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_bap_broadcast_source_delete(broadcast_source);
	if (err != 0) {
		LOG_DBG("Unable to delete broadcast source: %d", err);

		return BTP_STATUS_FAILED;
	}

	memset(broadcaster, 0, sizeof(*broadcaster));
	broadcast_source = NULL;

	return BTP_STATUS_SUCCESS;
}

static uint8_t broadcast_adv_start(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_le_ext_adv *ext_adv = tester_gap_ext_adv_get();

	LOG_DBG("");

	if (ext_adv == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = tester_gap_start_ext_adv();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	err = tester_gap_padv_start();
	if (err != 0) {
		LOG_DBG("Unable to start periodic advertising: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t broadcast_adv_stop(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = tester_gap_padv_stop();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	err = tester_gap_stop_ext_adv();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t broadcast_source_start(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_le_ext_adv *ext_adv = tester_gap_ext_adv_get();

	LOG_DBG("");

	if (ext_adv == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bap_broadcast_source_start(broadcast_source, ext_adv);
	if (err != 0) {
		LOG_DBG("Unable to start broadcast source: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t broadcast_source_stop(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_bap_broadcast_source_stop(broadcast_source);
	if (err != 0) {
		LOG_DBG("Unable to stop broadcast source: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static int broadcast_sink_reset(void)
{
	bis_index_bitfield = 0U;
	(void)memset(&broadcaster_addr, 0, sizeof(broadcaster_addr));
	(void)memset(broadcaster, 0, sizeof(*broadcaster));
	broadcaster_broadcast_id = INVALID_BROADCAST_ID;

	return 0;
}

static void btp_send_baa_found_ev(const bt_addr_le_t *address, uint32_t broadcast_id,
				  uint8_t sid, uint16_t interval)
{
	struct btp_bap_baa_found_ev ev;

	bt_addr_le_copy(&ev.address, address);
	sys_put_le24(broadcast_id, ev.broadcast_id);
	ev.advertiser_sid = sid;
	ev.padv_interval = interval;

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BAA_FOUND, &ev, sizeof(ev));
}

static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	const struct bt_le_scan_recv_info *info = user_data;
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct bt_uuid_16 adv_uuid;
	uint32_t broadcast_id;

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return true;
	}

	if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO)) {
		return true;
	}

	broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	LOG_DBG("Found broadcaster with ID 0x%06X and addr %s and sid 0x%02X", broadcast_id,
		le_addr, info->sid);

	btp_send_baa_found_ev(info->addr, broadcast_id, info->sid, info->interval);

	/* Stop parsing */
	return false;
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	/* If 0 there is no periodic advertising. */
	if (info->interval != 0U) {
		bt_data_parse(ad, scan_check_and_sync_broadcast, (void *)info);
	}
}

static struct bt_le_scan_cb bap_scan_cb = {
	.recv = broadcast_scan_recv,
};

static void btp_send_bis_found_ev(const bt_addr_le_t *address, uint32_t broadcast_id, uint32_t pd,
				  uint8_t subgroup_index, uint8_t bis_index,
				  const struct bt_audio_codec_cfg *codec_cfg)
{
	struct btp_bap_bis_found_ev *ev;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + codec_cfg->data_len, (uint8_t **)&ev);

	bt_addr_le_copy(&ev->address, address);
	sys_put_le24(broadcast_id, ev->broadcast_id);
	sys_put_le24(pd, ev->presentation_delay);
	ev->subgroup_id = subgroup_index;
	ev->bis_id = bis_index;
	ev->coding_format = codec_cfg->id;
	ev->vid = sys_cpu_to_le16(codec_cfg->vid);
	ev->cid = sys_cpu_to_le16(codec_cfg->cid);

	ev->cc_ltvs_len = codec_cfg->data_len;
	memcpy(ev->cc_ltvs, codec_cfg->data, ev->cc_ltvs_len);

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BIS_FOUND, ev,
		     sizeof(*ev) + ev->cc_ltvs_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base)
{
	size_t stream_count = 0U;
	uint32_t base_bis_index_bitfield = 0U;
	const struct bt_audio_codec_cfg *codec_cfg;

	LOG_DBG("");

	if (broadcaster_broadcast_id != sink->broadcast_id) {
		return;
	}

	LOG_DBG("Received BASE with %u subgroups from broadcast sink %p", base->subgroup_count,
		sink);

	for (size_t i = 0U; i < base->subgroup_count; i++) {
		for (size_t j = 0U; j < base->subgroups[i].bis_count; j++) {
			const uint8_t index = base->subgroups[i].bis_data[j].index;

			codec_cfg = &base->subgroups[i].codec_cfg;
			base_bis_index_bitfield |= BIT(index);

			if (stream_count < MAX_STREAMS_COUNT) {
				broadcaster->streams[stream_count++].bis_id = index;
			}

			btp_send_bis_found_ev(&broadcaster_addr, sink->broadcast_id,
					      sink->base.pd, i, index, codec_cfg);
		}
	}

	bis_index_bitfield = base_bis_index_bitfield & bis_index_mask;
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, bool encrypted)
{
	int err;

	LOG_DBG("");

	err = bt_bap_broadcast_sink_sync(broadcast_sink, bis_index_bitfield, sink_streams, NULL);
	if (err != 0) {
		LOG_DBG("Unable to sync to broadcast source: %d", err);
	}
}

static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
	.base_recv = base_recv_cb,
	.syncable = syncable_cb,
};

static void bap_pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
				  struct bt_le_per_adv_sync_synced_info *info)
{
	int err;
	struct bt_le_per_adv_sync *pa_sync;

	LOG_DBG("");

	pa_sync = tester_gap_padv_get();

	if (sync != pa_sync) {
		return;
	}

	err = bt_bap_broadcast_sink_create(pa_sync, broadcaster_broadcast_id, &broadcast_sink);
	if (err != 0) {
		LOG_DBG("Failed to create broadcast sink: ID 0x%06X, err %d",
			broadcaster_broadcast_id, err);
	}
}

static struct bt_le_per_adv_sync_cb bap_pa_sync_cb = {
	.synced = bap_pa_sync_synced_cb,
};

static uint8_t broadcast_sink_setup(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = broadcast_sink_reset();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	for (size_t i = 0U; i < MAX_STREAMS_COUNT; i++) {
		broadcaster->streams[i].broadcast = true;
		sink_streams[i] = &broadcaster->streams[i].stream;
		sink_streams[i]->ops = &stream_ops;
	}

	bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
	bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);
	bt_le_scan_cb_register(&bap_scan_cb);

	return BTP_STATUS_SUCCESS;
}

static uint8_t broadcast_sink_release(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = broadcast_sink_reset();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t broadcast_scan_start(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0 && err != -EALREADY) {
		LOG_DBG("Unable to start scan for broadcast sources: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t broadcast_scan_stop(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_le_scan_stop();
	if (err != 0) {
		LOG_DBG("Failed to stop scan, %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t broadcast_sink_sync(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_bap_broadcast_sink_sync_cmd *cp = cmd;
	struct bt_le_per_adv_sync_param create_params = {0};

	LOG_DBG("");

	/* Sink Sync steps:
	 * 1. bt_le_per_adv_sync_create()
	 * 2. bap_pa_sync_synced_cb()
	 * 3. bt_bap_broadcast_sink_create()
	 * 4. - base_recv_cb()
	 *    - syncable_cb()
	 *    - broadcast_code_cb() <- only with scan delegator
	 *    - bis_sync_req_cb() <- only for scan delegator
	 * 5. bt_bap_broadcast_sink_sync()
	 * 6. stream_started()
	 * 7. stream_recv_cb()
	 * 8. bap_pa_sync_terminated_cb()
	 * 9. stream_stopped_cb()
	 */

	broadcaster_broadcast_id = sys_get_le24(cp->broadcast_id);
	bt_addr_le_copy(&broadcaster_addr, &cp->address);
	bt_addr_le_copy(&create_params.addr, &cp->address);
	create_params.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	create_params.sid = cp->advertiser_sid;
	create_params.skip = cp->skip;
	create_params.timeout = cp->sync_timeout;

	err = tester_padv_create_sync(&create_params);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t broadcast_sink_stop(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_bap_broadcast_sink_stop(broadcast_sink);
	if (err != 0) {
		LOG_DBG("Unable to sync to broadcast source: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct audio_connection *audio_conn;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		LOG_DBG("Failed to connect to %s (%u)", addr, err);
		return;
	}

	LOG_DBG("Connected: %s", addr);

	audio_conn = &connections[bt_conn_index(conn)];
	memset(audio_conn, 0, sizeof(*audio_conn));

	for (size_t i = 0; i < ARRAY_SIZE(audio_conn->streams); i++) {
		bt_bap_stream_cb_register(&audio_conn->streams[i].stream, &stream_ops);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Disconnected: %s (reason 0x%02x)", addr, reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static struct bt_pacs_cap cap_sink = {
	.codec_cap = &default_codec_cap,
};

static struct bt_pacs_cap cap_source = {
	.codec_cap = &default_codec_cap,
};

static struct bt_pacs_cap vendor_cap_sink = {
	.codec_cap = &vendor_codec_cap,
};

static struct bt_pacs_cap vendor_cap_source = {
	.codec_cap = &vendor_codec_cap,
};

static uint8_t ascs_supported_commands(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	struct btp_ascs_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_ASCS_READ_SUPPORTED_COMMANDS);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static int server_stream_config(struct bt_conn *conn, struct bt_bap_stream *stream,
				struct bt_audio_codec_cfg *codec_cfg,
				struct bt_audio_codec_qos_pref *qos)
{
	int err;
	struct bt_bap_ep *ep;

	err = bt_bap_unicast_server_config_ase(conn, stream, codec_cfg, qos);
	if (err != 0) {
		return err;
	}

	print_codec_cfg(codec_cfg);

	ep = stream->ep;
	LOG_DBG("ASE Codec Config: ase_id %u dir %u", ep->status.id, ep->dir);
	LOG_DBG("ASE Codec Config stream %p", stream);

	return 0;
}

static uint8_t client_add_ase_to_cis(struct audio_connection *audio_conn, uint8_t ase_id,
				     uint8_t cis_id, uint8_t cig_id)
{
	struct audio_stream *stream;

	if (cig_id >= CONFIG_BT_ISO_MAX_CIG || cis_id >= UNICAST_GROUP_STREAM_CNT) {
		return BTP_STATUS_FAILED;
	}

	stream = stream_find(audio_conn, ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Added ASE %u to CIS %u at CIG %u", ase_id, cis_id, cig_id);

	stream->cig = &cigs[cig_id];
	stream->cig_id = cig_id;
	stream->cis_id = cis_id;

	return 0;
}

static int client_create_unicast_group(struct audio_connection *audio_conn, uint8_t ase_id,
				       uint8_t cig_id)
{
	int err;
	struct bt_bap_unicast_group_stream_pair_param pair_params[MAX_STREAMS_COUNT];
	struct bt_bap_unicast_group_stream_param stream_params[MAX_STREAMS_COUNT];
	struct bt_bap_unicast_group_param param;
	size_t stream_cnt = 0;
	size_t src_cnt = 0;
	size_t sink_cnt = 0;
	size_t cis_cnt = 0;

	(void)memset(pair_params, 0, sizeof(pair_params));
	(void)memset(stream_params, 0, sizeof(stream_params));

	if (cig_id >= CONFIG_BT_ISO_MAX_CIG) {
		return -EINVAL;
	}

	/* API does not allow to assign a CIG ID freely, so ensure we create groups
	 * in the right order.
	 */
	for (uint8_t i = 0; i < cig_id; i++) {
		if (cigs[i] == NULL) {
			return -EINVAL;
		}
	}

	/* Assign end points to CISes */
	for (size_t i = 0; i < MAX_STREAMS_COUNT; i++) {
		struct audio_stream *a_stream = &audio_conn->streams[i];
		struct bt_bap_stream *stream = &a_stream->stream;

		if (stream == NULL || stream->ep == NULL || a_stream->cig == NULL ||
			a_stream->cig_id != cig_id) {
			continue;
		}

		stream_params[stream_cnt].stream = stream;
		stream_params[stream_cnt].qos = &audio_conn->qos;

		if (stream->ep->dir == BT_AUDIO_DIR_SOURCE) {
			if (pair_params[a_stream->cis_id].rx_param != NULL) {
				return -EINVAL;
			}

			pair_params[a_stream->cis_id].rx_param = &stream_params[stream_cnt];
			src_cnt++;
		} else {
			if (pair_params[a_stream->cis_id].tx_param != NULL) {
				return -EINVAL;
			}

			pair_params[a_stream->cis_id].tx_param = &stream_params[stream_cnt];
			sink_cnt++;
		}

		stream_cnt++;
	}

	/* Count CISes to be established */
	for (size_t i = 0; i < MAX_STREAMS_COUNT; i++) {
		if (pair_params[i].tx_param == NULL && pair_params[i].rx_param == NULL) {
			/* No gaps allowed */
			break;
		}

		cis_cnt++;
	}

	/* Make sure there are no gaps in the pair_params */
	if (cis_cnt == 0 || cis_cnt < MAX(sink_cnt, src_cnt)) {
		return -EINVAL;
	}

	param.params = pair_params;
	param.params_count = cis_cnt;
	param.packing = BT_ISO_PACKING_SEQUENTIAL;

	LOG_DBG("Creating unicast group");
	err = bt_bap_unicast_group_create(&param, &cigs[cig_id]);
	if (err != 0) {
		LOG_DBG("Could not create unicast group (err %d)", err);
		return -EINVAL;
	}

	return 0;
}

static int client_configure_codec(struct audio_connection *audio_conn, struct bt_conn *conn,
				  uint8_t ase_id, struct bt_audio_codec_cfg *codec_cfg)
{
	int err;
	struct bt_bap_ep *ep;
	struct audio_stream *stream;

	stream = stream_find(audio_conn, ase_id);
	if (stream == NULL) {
		/* Configure a new stream */
		stream = stream_alloc(audio_conn);
		if (stream == NULL) {
			LOG_DBG("No streams available");

			return -ENOMEM;
		}

		if (audio_conn->end_points_count == 0) {
			return -EINVAL;
		}

		ep = end_point_find(audio_conn, ase_id);
		if (ep == NULL) {
			return -EINVAL;
		}

		err = bt_bap_stream_config(conn, &stream->stream, ep, codec_cfg);
	} else {
		/* Reconfigure a stream */
		err = bt_bap_stream_reconfig(&stream->stream, codec_cfg);
	}

	return err;
}

static int server_configure_codec(struct audio_connection *audio_conn, struct bt_conn *conn,
				  uint8_t ase_id, struct bt_audio_codec_cfg *codec_cfg)
{
	struct audio_stream *stream;
	int err = 0;

	stream = stream_find(audio_conn, ase_id);
	if (stream == NULL) {
		/* Zephyr allocates ASE instances for remote clients dynamically.
		 * To initiate Codec Config operation autonomously in server the role,
		 * we have to initialize all ASEs with a smaller ID first.
		 * Fortunately, the PTS has nothing against such behavior.
		 */
		for (uint8_t i = 1; i <= ase_id; i++) {
			stream = stream_find(audio_conn, i);
			if (stream != NULL) {
				continue;
			}

			/* Configure a new stream */
			stream = stream_alloc(audio_conn);
			if (stream == NULL) {
				LOG_DBG("No streams available");

				return -ENOMEM;
			}

			err = server_stream_config(conn, &stream->stream, codec_cfg, &qos_pref);
		}
	} else {
		/* Reconfigure a stream */
		err = bt_bap_stream_reconfig(&stream->stream, codec_cfg);
	}

	return err;
}

static uint8_t ascs_configure_codec(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_configure_codec_cmd *cp = cmd;
	struct bt_conn *conn;
	struct bt_conn_info conn_info;
	struct audio_connection *audio_conn;
	struct bt_audio_codec_cfg *codec_cfg;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];

	(void)bt_conn_get_info(conn, &conn_info);

	codec_cfg = &audio_conn->codec_cfg;
	memset(codec_cfg, 0, sizeof(*codec_cfg));

	codec_cfg->id = cp->coding_format;
	codec_cfg->vid = cp->vid;
	codec_cfg->cid = cp->cid;

	if (cp->cc_ltvs_len != 0) {
		codec_cfg->data_len = cp->cc_ltvs_len;
		memcpy(codec_cfg->data, cp->cc_ltvs, cp->cc_ltvs_len);
	}

	if (conn_info.role == BT_HCI_ROLE_CENTRAL) {
		err = client_configure_codec(audio_conn, conn, cp->ase_id, codec_cfg);
	} else {
		err = server_configure_codec(audio_conn, conn, cp->ase_id, codec_cfg);
	}

	bt_conn_unref(conn);

	if (err) {
		LOG_DBG("Failed to configure stream (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ascs_configure_qos(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_configure_qos_cmd *cp = cmd;
	struct bt_conn_info conn_info;
	struct audio_connection *audio_conn;
	struct bt_audio_codec_qos *qos;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	(void)bt_conn_get_info(conn, &conn_info);
	if (conn_info.role == BT_HCI_ROLE_PERIPHERAL) {
		bt_conn_unref(conn);

		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];

	if (cigs[cp->cig_id] != NULL) {
		err = bt_bap_unicast_group_delete(cigs[cp->cig_id]);
		if (err != 0) {
			LOG_DBG("Failed to delete the unicast group, err %d", err);
			bt_conn_unref(conn);

			return BTP_STATUS_FAILED;
		}
		cigs[cp->cig_id] = NULL;
	}

	err = client_add_ase_to_cis(audio_conn, cp->ase_id, cp->cis_id, cp->cig_id);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	qos = &audio_conn->qos;
	qos->phy = BT_AUDIO_CODEC_QOS_2M;
	qos->framing = cp->framing;
	qos->rtn = cp->retransmission_num;
	qos->sdu = cp->max_sdu;
	qos->latency = cp->max_transport_latency;
	qos->interval = sys_get_le24(cp->sdu_interval);
	qos->pd = sys_get_le24(cp->presentation_delay);

	err = client_create_unicast_group(audio_conn, cp->ase_id, cp->cig_id);
	if (err != 0) {
		LOG_DBG("Unable to create unicast group, err %d", err);
		bt_conn_unref(conn);

		return BTP_STATUS_FAILED;
	}

	LOG_DBG("QoS configuring streams");
	err = bt_bap_stream_qos(conn, cigs[cp->cig_id]);
	bt_conn_unref(conn);

	if (err != 0) {
		LOG_DBG("Unable to QoS configure streams: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ascs_enable(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_enable_cmd *cp = cmd;
	struct audio_connection *audio_conn;
	struct audio_stream *stream;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	stream = stream_find(audio_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Enabling stream");
	err = bt_bap_stream_enable(&stream->stream, NULL, 0);
	if (err != 0) {
		LOG_DBG("Could not enable stream: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ascs_disable(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_disable_cmd *cp = cmd;
	struct audio_connection *audio_conn;
	struct audio_stream *stream;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	stream = stream_find(audio_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Disabling stream");
	err = bt_bap_stream_disable(&stream->stream);

	if (err != 0) {
		LOG_DBG("Could not disable stream: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ascs_receiver_start_ready(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_receiver_start_ready_cmd *cp = cmd;
	struct audio_connection *audio_conn;
	struct audio_stream *stream;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	stream = stream_find(audio_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Starting stream %p, ep %u, dir %u", &stream->stream, cp->ase_id,
		stream->stream.ep->dir);

	while (true) {
		err = bt_bap_stream_start(&stream->stream);
		if (err == -EBUSY) {
			/* TODO: How to determine if a controller is ready again after
			 * bt_bap_stream_start? In AC 6(i) tests the PTS sends Receiver Start Ready
			 * only after all CISes are established.
			 */
			k_sleep(K_MSEC(1000));
			continue;
		} else if (err != 0) {
			LOG_DBG("Could not start stream: %d", err);
			return BTP_STATUS_FAILED;
		}

		break;
	};

	return BTP_STATUS_SUCCESS;
}

static uint8_t ascs_receiver_stop_ready(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_receiver_stop_ready_cmd *cp = cmd;
	struct audio_connection *audio_conn;
	struct audio_stream *stream;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	stream = stream_find(audio_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Stopping stream");
	err = bt_bap_stream_stop(&stream->stream);
	if (err != 0) {
		LOG_DBG("Could not stop stream: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ascs_release(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_release_cmd *cp = cmd;
	struct audio_connection *audio_conn;
	struct audio_stream *stream;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	stream = stream_find(audio_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Releasing stream");
	err = bt_bap_stream_release(&stream->stream);
	if (err != 0) {
		LOG_DBG("Unable to release stream %p, err %d", stream, err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ascs_update_metadata(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const uint8_t meta[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_ANY)),
	};
	const struct btp_ascs_update_metadata_cmd *cp = cmd;
	struct audio_connection *audio_conn;
	struct audio_stream *stream;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	stream = stream_find(audio_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Updating stream metadata");
	err = bt_bap_stream_metadata(&stream->stream, meta, ARRAY_SIZE(meta));
	if (err != 0) {
		LOG_DBG("Failed to update stream metadata, err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ascs_add_ase_to_cis(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_conn *conn;
	struct audio_connection *audio_conn;
	struct bt_conn_info conn_info;
	const struct btp_ascs_add_ase_to_cis *cp = cmd;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");

		return BTP_STATUS_FAILED;
	}

	(void)bt_conn_get_info(conn, &conn_info);
	if (conn_info.role == BT_HCI_ROLE_PERIPHERAL) {
		bt_conn_unref(conn);

		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	err = client_add_ase_to_cis(audio_conn, cp->ase_id, cp->cis_id, cp->cig_id);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler ascs_handlers[] = {
	{
		.opcode = BTP_ASCS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = ascs_supported_commands,
	},
	{
		.opcode = BTP_ASCS_CONFIGURE_CODEC,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = ascs_configure_codec,
	},
	{
		.opcode = BTP_ASCS_CONFIGURE_QOS,
		.expect_len = sizeof(struct btp_ascs_configure_qos_cmd),
		.func = ascs_configure_qos,
	},
	{
		.opcode = BTP_ASCS_ENABLE,
		.expect_len = sizeof(struct btp_ascs_enable_cmd),
		.func = ascs_enable,
	},
	{
		.opcode = BTP_ASCS_RECEIVER_START_READY,
		.expect_len = sizeof(struct btp_ascs_receiver_start_ready_cmd),
		.func = ascs_receiver_start_ready,
	},
	{
		.opcode = BTP_ASCS_RECEIVER_STOP_READY,
		.expect_len = sizeof(struct btp_ascs_receiver_stop_ready_cmd),
		.func = ascs_receiver_stop_ready,
	},
	{
		.opcode = BTP_ASCS_DISABLE,
		.expect_len = sizeof(struct btp_ascs_disable_cmd),
		.func = ascs_disable,
	},
	{
		.opcode = BTP_ASCS_RELEASE,
		.expect_len = sizeof(struct btp_ascs_release_cmd),
		.func = ascs_release,
	},
	{
		.opcode = BTP_ASCS_UPDATE_METADATA,
		.expect_len = sizeof(struct btp_ascs_update_metadata_cmd),
		.func = ascs_update_metadata,
	},
	{
		.opcode = BTP_ASCS_ADD_ASE_TO_CIS,
		.expect_len = sizeof(struct btp_ascs_add_ase_to_cis),
		.func = ascs_add_ase_to_cis,
	},
};

static int set_location(void)
{
	int err;

	err = bt_pacs_set_location(BT_AUDIO_DIR_SINK,
				   BT_AUDIO_LOCATION_FRONT_CENTER |
				   BT_AUDIO_LOCATION_FRONT_RIGHT);
	if (err != 0) {
		return err;
	}

	err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE,
				   (BT_AUDIO_LOCATION_FRONT_LEFT |
				    BT_AUDIO_LOCATION_FRONT_RIGHT));
	if (err != 0) {
		return err;
	}

	return 0;
}

static int set_available_contexts(void)
{
	int err;

	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
					     AVAILABLE_SOURCE_CONTEXT);
	if (err != 0) {
		return err;
	}

	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK,
					     AVAILABLE_SINK_CONTEXT);
	if (err != 0) {
		return err;
	}

	return 0;
}

static int set_supported_contexts(void)
{
	int err;

	err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE,
					     SUPPORTED_SOURCE_CONTEXT);
	if (err != 0) {
		return err;
	}

	err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK,
					     SUPPORTED_SINK_CONTEXT);
	if (err != 0) {
		return err;
	}

	return 0;
}

static uint8_t pacs_supported_commands(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	struct btp_pacs_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_PACS_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_PACS_UPDATE_CHARACTERISTIC);
	tester_set_bit(rp->data, BTP_PACS_SET_LOCATION);
	tester_set_bit(rp->data, BTP_PACS_SET_AVAILABLE_CONTEXTS);
	tester_set_bit(rp->data, BTP_PACS_SET_SUPPORTED_CONTEXTS);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t pacs_update_characteristic(const void *cmd, uint16_t cmd_len,
					  void *rsp, uint16_t *rsp_len)
{
	const struct btp_pacs_update_characteristic_cmd *cp = cmd;
	int err;

	switch (cp->characteristic) {
	case BTP_PACS_CHARACTERISTIC_SINK_PAC:
		err = bt_pacs_cap_unregister(BT_AUDIO_DIR_SINK,
					     &cap_sink);
		break;
	case BTP_PACS_CHARACTERISTIC_SOURCE_PAC:
		err = bt_pacs_cap_unregister(BT_AUDIO_DIR_SOURCE,
					     &cap_source);
		break;
	case BTP_PACS_CHARACTERISTIC_SINK_AUDIO_LOCATIONS:
		err = bt_pacs_set_location(BT_AUDIO_DIR_SINK,
					   BT_AUDIO_LOCATION_FRONT_CENTER |
					   BT_AUDIO_LOCATION_BACK_CENTER);
		break;
	case BTP_PACS_CHARACTERISTIC_SOURCE_AUDIO_LOCATIONS:
		err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE,
					   (BT_AUDIO_LOCATION_FRONT_LEFT |
					    BT_AUDIO_LOCATION_FRONT_RIGHT |
					    BT_AUDIO_LOCATION_FRONT_CENTER));
		break;
	case BTP_PACS_CHARACTERISTIC_AVAILABLE_AUDIO_CONTEXTS:
		err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
				BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
		break;
	case BTP_PACS_CHARACTERISTIC_SUPPORTED_AUDIO_CONTEXTS:
		err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE,
				SUPPORTED_SOURCE_CONTEXT |
				BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL);
		break;
	default:
		return BTP_STATUS_FAILED;
	}

	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pacs_set_location(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_pacs_set_location_cmd *cp = cmd;
	int err;

	err = bt_pacs_set_location((enum bt_audio_dir)cp->dir,
				   (enum bt_audio_location)cp->location);

	return (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

static uint8_t pacs_set_available_contexts(const void *cmd, uint16_t cmd_len,
					   void *rsp, uint16_t *rsp_len)
{
	const struct btp_pacs_set_available_contexts_cmd *cp = cmd;
	int err;

	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK,
					     (enum bt_audio_context)cp->sink_contexts);
	if (err) {
		return BTP_STATUS_FAILED;
	}
	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
					     (enum bt_audio_context)cp->source_contexts);

	return (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

static uint8_t pacs_set_supported_contexts(const void *cmd, uint16_t cmd_len,
					   void *rsp, uint16_t *rsp_len)
{
	const struct btp_pacs_set_supported_contexts_cmd *cp = cmd;
	int err;

	err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK,
					     (enum bt_audio_context)cp->sink_contexts);
	if (err) {
		return BTP_STATUS_FAILED;
	}
	err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE,
					     (enum bt_audio_context)cp->source_contexts);

	return (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

static const struct btp_handler pacs_handlers[] = {
	{
		.opcode = BTP_PACS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = pacs_supported_commands,
	},
	{
		.opcode = BTP_PACS_UPDATE_CHARACTERISTIC,
		.expect_len = sizeof(struct btp_pacs_update_characteristic_cmd),
		.func = pacs_update_characteristic,
	},
	{
		.opcode = BTP_PACS_SET_LOCATION,
		.expect_len = sizeof(struct btp_pacs_set_location_cmd),
		.func = pacs_set_location
	},
	{
		.opcode = BTP_PACS_SET_AVAILABLE_CONTEXTS,
		.expect_len = sizeof(struct btp_pacs_set_available_contexts_cmd),
		.func = pacs_set_available_contexts
	},
	{
		.opcode = BTP_PACS_SET_SUPPORTED_CONTEXTS,
		.expect_len = sizeof(struct btp_pacs_set_supported_contexts_cmd),
		.func = pacs_set_supported_contexts
	}
};

static uint8_t bap_supported_commands(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	struct btp_bap_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_BAP_READ_SUPPORTED_COMMANDS);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler bap_handlers[] = {
	{
		.opcode = BTP_BAP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = bap_supported_commands,
	},
	{
		.opcode = BTP_BAP_DISCOVER,
		.expect_len = sizeof(struct btp_bap_discover_cmd),
		.func = bap_discover,
	},
	{
		.opcode = BTP_BAP_SEND,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = bap_send,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SOURCE_SETUP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = broadcast_source_setup,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SOURCE_RELEASE,
		.expect_len = sizeof(struct btp_bap_broadcast_source_release_cmd),
		.func = broadcast_source_release,
	},
	{
		.opcode = BTP_BAP_BROADCAST_ADV_START,
		.expect_len = sizeof(struct btp_bap_broadcast_adv_start_cmd),
		.func = broadcast_adv_start,
	},
	{
		.opcode = BTP_BAP_BROADCAST_ADV_STOP,
		.expect_len = sizeof(struct btp_bap_broadcast_adv_stop_cmd),
		.func = broadcast_adv_stop,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SOURCE_START,
		.expect_len = sizeof(struct btp_bap_broadcast_source_start_cmd),
		.func = broadcast_source_start,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SOURCE_STOP,
		.expect_len = sizeof(struct btp_bap_broadcast_source_stop_cmd),
		.func = broadcast_source_stop,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SINK_SETUP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = broadcast_sink_setup,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SINK_RELEASE,
		.expect_len = sizeof(struct btp_bap_broadcast_sink_release_cmd),
		.func = broadcast_sink_release,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SCAN_START,
		.expect_len = sizeof(struct btp_bap_broadcast_scan_start_cmd),
		.func = broadcast_scan_start,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SCAN_STOP,
		.expect_len = sizeof(struct btp_bap_broadcast_scan_stop_cmd),
		.func = broadcast_scan_stop,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SINK_SYNC,
		.expect_len = sizeof(struct btp_bap_broadcast_sink_sync_cmd),
		.func = broadcast_sink_sync,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SINK_STOP,
		.expect_len = sizeof(struct btp_bap_broadcast_sink_stop_cmd),
		.func = broadcast_sink_stop,
	},
};

uint8_t tester_init_pacs(void)
{
	int err;

	bt_bap_unicast_server_register_cb(&unicast_server_cb);

	bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap_sink);
	bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &cap_source);
	bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &vendor_cap_sink);
	bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &vendor_cap_source);

	err = set_location();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	err = set_supported_contexts();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	err = set_available_contexts();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	tester_register_command_handlers(BTP_SERVICE_ID_PACS, pacs_handlers,
					 ARRAY_SIZE(pacs_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_pacs(void)
{
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_init_ascs(void)
{
	bt_conn_cb_register(&conn_callbacks);

	tester_register_command_handlers(BTP_SERVICE_ID_ASCS, ascs_handlers,
					 ARRAY_SIZE(ascs_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_ascs(void)
{
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_init_bap(void)
{
	int err;

	/* reset data */
	(void)memset(connections, 0, sizeof(connections));

	broadcaster = &broadcast_connection;

	err = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
	if (err != 0) {
		LOG_DBG("Failed to register client callbacks: %d", err);
		return BTP_STATUS_FAILED;
	}

	k_work_queue_init(&iso_data_work_q);
	k_work_queue_start(&iso_data_work_q, iso_data_thread_stack_area,
			   K_THREAD_STACK_SIZEOF(iso_data_thread_stack_area),
			   ISO_DATA_THREAD_PRIORITY, NULL);

	tester_register_command_handlers(BTP_SERVICE_ID_BAP, bap_handlers,
					 ARRAY_SIZE(bap_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_bap(void)
{
	/* reset data */
	(void)memset(connections, 0, sizeof(connections));

	return BTP_STATUS_SUCCESS;
}
