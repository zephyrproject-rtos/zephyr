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

#include "bap_endpoint.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#define LOG_MODULE_NAME bttester_bap
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);
#include "btp/btp.h"

#define DEFAULT_CONTEXT BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA
#define SUPPORTED_SINK_CONTEXT	(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | \
				 BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | \
				 BT_AUDIO_CONTEXT_TYPE_MEDIA | \
				 BT_AUDIO_CONTEXT_TYPE_GAME | \
				 BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL)

#define SUPPORTED_SOURCE_CONTEXT (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | \
				  BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | \
				  BT_AUDIO_CONTEXT_TYPE_MEDIA | \
				  BT_AUDIO_CONTEXT_TYPE_GAME)

#define AVAILABLE_SINK_CONTEXT SUPPORTED_SINK_CONTEXT
#define AVAILABLE_SOURCE_CONTEXT SUPPORTED_SOURCE_CONTEXT

static struct bt_audio_codec_cap default_codec_cap =
	BT_AUDIO_CODEC_LC3(BT_AUDIO_CODEC_LC3_FREQ_ANY, BT_AUDIO_CODEC_LC3_DURATION_10,
			   BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40u, 120u, 1u,
			   (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

struct audio_stream {
	struct bt_bap_stream stream;
	uint8_t ase_id;
	uint8_t conn_id;
	atomic_t seq_num;
	uint16_t last_req_seq_num;
	uint16_t last_sent_seq_num;
	uint16_t max_sdu;
	size_t len_to_send;
	struct k_work_delayable audio_clock_work;
	struct k_work_delayable audio_send_work;
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
	struct bt_bap_unicast_group *unicast_group;
	struct bt_bap_ep *end_points[MAX_END_POINTS_COUNT];
	size_t end_points_count;
} connections[CONFIG_BT_MAX_CONN];

static struct bt_audio_codec_qos_pref qos_pref =
	BT_AUDIO_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, 0x02, 10, 10000, 40000, 10000, 40000);

NET_BUF_POOL_FIXED_DEFINE(tx_pool, MAX(CONFIG_BT_ASCS_ASE_SRC_COUNT,
				       CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT),
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);

static struct net_buf_simple *rx_ev_buf = NET_BUF_SIMPLE(CONFIG_BT_ISO_RX_MTU +
							 sizeof(struct btp_bap_stream_received_ev));

static bool already_sent;

RING_BUF_DECLARE(audio_ring_buf, CONFIG_BT_ISO_TX_MTU);
static void audio_clock_timeout(struct k_work *work);
static void audio_send_timeout(struct k_work *work);

#define ISO_DATA_THREAD_STACK_SIZE 512
#define ISO_DATA_THREAD_PRIORITY -7
K_THREAD_STACK_DEFINE(iso_data_thread_stack_area, ISO_DATA_THREAD_STACK_SIZE);
static struct k_work_q iso_data_work_q;

static void print_codec_cfg(const struct bt_audio_codec_cfg *codec_cfg)
{
	LOG_DBG("codec_cfg 0x%02x cid 0x%04x vid 0x%04x count %zu", codec_cfg->id, codec_cfg->cid,
		codec_cfg->vid, codec_cfg->data_count);

	for (size_t i = 0; i < codec_cfg->data_count; i++) {
		LOG_DBG("data #%zu: type 0x%02x len %u", i, codec_cfg->data[i].data.type,
			codec_cfg->data[i].data.data_len);
		LOG_HEXDUMP_DBG(codec_cfg->data[i].data.data,
				codec_cfg->data[i].data.data_len -
					sizeof(codec_cfg->data[i].data.type),
				"");
	}

	if (codec_cfg->id == BT_AUDIO_CODEC_LC3_ID) {
		/* LC3 uses the generic LTV format - other codecs might do as well */

		enum bt_audio_location chan_allocation;

		LOG_DBG("  Frequency: %d Hz", bt_audio_codec_cfg_get_freq(codec_cfg));
		LOG_DBG("  Frame Duration: %d us",
			bt_audio_codec_cfg_get_frame_duration_us(codec_cfg));
		if (bt_audio_codec_cfg_get_chan_allocation_val(codec_cfg, &chan_allocation) == 0) {
			LOG_DBG("  Channel allocation: 0x%x", chan_allocation);
		}

		LOG_DBG("  Octets per frame: %d (negative means value not pressent)",
			bt_audio_codec_cfg_get_octets_per_frame(codec_cfg));
		LOG_DBG("  Frames per SDU: %d",
			bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, true));
	}

	for (size_t i = 0; i < codec_cfg->meta_count; i++) {
		LOG_DBG("meta #%zu: type 0x%02x len %u", i, codec_cfg->meta[i].data.type,
			codec_cfg->meta[i].data.data_len);
		LOG_HEXDUMP_DBG(codec_cfg->meta[i].data.data,
				codec_cfg->meta[i].data.data_len -
					sizeof(codec_cfg->meta[i].data.type),
				"");
	}
}

static void print_codec_cap(const struct bt_audio_codec_cap *codec_cap)
{
	LOG_DBG("codec_cap 0x%02x cid 0x%04x vid 0x%04x count %zu", codec_cap->id, codec_cap->cid,
		codec_cap->vid, codec_cap->data_count);

	for (size_t i = 0; i < codec_cap->data_count; i++) {
		LOG_DBG("data #%zu: type 0x%02x len %u", i, codec_cap->data[i].data.type,
			codec_cap->data[i].data.data_len);
		LOG_HEXDUMP_DBG(codec_cap->data[i].data.data,
				codec_cap->data[i].data.data_len -
					sizeof(codec_cap->data[i].data.type),
				"");
	}

	for (size_t i = 0; i < codec_cap->meta_count; i++) {
		LOG_DBG("meta #%zu: type 0x%02x len %u", i, codec_cap->meta[i].data.type,
			codec_cap->meta[i].data.data_len);
		LOG_HEXDUMP_DBG(codec_cap->meta[i].data.data,
				codec_cap->meta[i].data.data_len -
					sizeof(codec_cap->meta[i].data.type),
				"");
	}
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

static int validate_codec_parameters(const struct bt_audio_codec_cfg *codec_cfg)
{
	int freq_hz;
	int frame_duration_us;
	int frames_per_sdu;
	int octets_per_frame;
	int chan_allocation_err;
	enum bt_audio_location chan_allocation;

	freq_hz = bt_audio_codec_cfg_get_freq(codec_cfg);
	frame_duration_us = bt_audio_codec_cfg_get_frame_duration_us(codec_cfg);
	chan_allocation_err =
		bt_audio_codec_cfg_get_chan_allocation_val(codec_cfg, &chan_allocation);
	octets_per_frame = bt_audio_codec_cfg_get_octets_per_frame(codec_cfg);
	frames_per_sdu = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, true);

	if (freq_hz < 0) {
		LOG_DBG("Error: Invalid codec frequency.");
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
		LOG_DBG("Error: Invalid frames per sdu.");
		return -EINVAL;
	}

	if (chan_allocation_err < 0) {
		LOG_DBG("Error: Invalid channel allocation.");
		return -EINVAL;
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

static int lc3_enable(struct bt_bap_stream *stream, const struct bt_audio_codec_data *meta,
		      size_t meta_count, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Enable: stream %p meta_count %zu", stream, meta_count);

	return 0;
}

static int lc3_start(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Start: stream %p", stream);

	return 0;
}

static bool valid_metadata_type(uint8_t type, uint8_t len, const uint8_t *data)
{
	/* PTS checks if we are able to reject unsupported metadata type or RFU vale.
	 * The only RFU value PTS seems to check for now is the streaming context.
	 */
	switch (type) {
	case BT_AUDIO_METADATA_TYPE_PREF_CONTEXT:
	case BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT:
		if (len != 2) {
			return false;
		}

		/* PTS wants us to reject the parameter if reserved bits are set */
		if ((sys_get_le16(data) & ~(uint16_t)(BT_AUDIO_CONTEXT_TYPE_ANY)) > 0) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_STREAM_LANG:
		if (len != 3) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_PARENTAL_RATING:
		if (len != 1) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_EXTENDED: /* 2 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_VENDOR: /* 2 - 255 octets */
		/* At least Extended Metadata Type / Company_ID should be there */
		if (len < 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_CCID_LIST:
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO: /* 0 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI: /* 0 - 255 octets */
		return true;
	default:
		return false;
	}
}

static int lc3_metadata(struct bt_bap_stream *stream, const struct bt_audio_codec_data *meta,
			size_t meta_count, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Metadata: stream %p meta_count %zu", stream, meta_count);

	for (size_t i = 0; i < meta_count; i++) {
		const struct bt_audio_codec_data *data = data = &meta[i];

		if (!valid_metadata_type(data->data.type, data->data.data_len, data->data.data)) {
			LOG_DBG("Invalid metadata type %u or length %u",
			       data->data.type, data->data.data_len);

			*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED,
					       data->data.type);

			btp_send_ascs_operation_completed_ev(stream->conn, stream->ep->status.id,
							     BT_ASCS_METADATA_OP,
							     BTP_ASCS_STATUS_FAILED);

			return -EINVAL;
		}
	}

	return 0;
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

	LOG_DBG("Enabled stream %p", stream);
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
	btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
					     BT_ASCS_DISABLE_OP, BTP_ASCS_STATUS_SUCCESS);
}

static void stream_released(struct bt_bap_stream *stream)
{
	struct audio_connection *audio_conn;
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	LOG_DBG("Released stream %p", stream);

	audio_conn = &connections[a_stream->conn_id];

	if (audio_conn->unicast_group != NULL) {
		LOG_DBG("Deleting unicast group");

		int err = bt_bap_unicast_group_delete(audio_conn->unicast_group);

		if (err != 0) {
			LOG_DBG("Unable to delete unicast group: %d", err);

			return;
		}

		audio_conn->unicast_group = NULL;
	}

	a_stream->ase_id = 0;
}

static void stream_started(struct bt_bap_stream *stream)
{
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	LOG_DBG("Started stream %p", stream);

	if (stream->dir == BT_AUDIO_DIR_SINK) {
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

	btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
					     BT_ASCS_START_OP, BTP_STATUS_SUCCESS);
}

static void stream_stopped(struct bt_bap_stream *stream, uint8_t reason)
{
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	LOG_DBG("Stopped stream %p with reason 0x%02X", stream, reason);

	if (stream->dir == BT_AUDIO_DIR_SINK) {
		/* Stop send timer */
		k_work_cancel_delayable(&a_stream->audio_clock_work);
		k_work_cancel_delayable(&a_stream->audio_send_work);
	}

	btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
					     BT_ASCS_STOP_OP, BTP_STATUS_SUCCESS);
}

static void stream_recv(struct bt_bap_stream *stream,
			const struct bt_iso_recv_info *info,
			struct net_buf *buf)
{
	if (already_sent == false) {
		/* For now, send just a first packet, to limit the number
		 * of logs and not unnecessarily spam through btp.
		 */
		LOG_DBG("Incoming audio on stream %p len %u", stream, buf->len);
		already_sent = true;
		btp_send_stream_received_ev(stream->conn, stream->ep, buf->len, buf->data);
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

static bool codec_cap_get_val(const struct bt_audio_codec_cap *codec_cap, uint8_t type,
			      const struct bt_audio_codec_data **data)
{
	if (codec_cap == NULL) {
		LOG_DBG("codec is NULL");
		return false;
	}

	for (size_t i = 0; i < codec_cap->data_count; i++) {
		if (codec_cap->data[i].data.type == type) {
			*data = &codec_cap->data[i];

			return true;
		}
	}

	return false;
}

static void btp_send_pac_codec_found_ev(struct bt_conn *conn,
					const struct bt_audio_codec_cap *codec_cap,
					enum bt_audio_dir dir)
{
	struct btp_bap_codec_cap_found_ev ev;
	struct bt_conn_info info;
	const struct bt_audio_codec_data *data;

	(void)bt_conn_get_info(conn, &info);
	bt_addr_le_copy(&ev.address, info.le.dst);

	ev.dir = dir;
	ev.coding_format = codec_cap->id;

	codec_cap_get_val(codec_cap, BT_AUDIO_CODEC_LC3_FREQ, &data);
	memcpy(&ev.frequencies, data->data.data, sizeof(ev.frequencies));

	codec_cap_get_val(codec_cap, BT_AUDIO_CODEC_LC3_DURATION, &data);
	memcpy(&ev.frame_durations, data->data.data, sizeof(ev.frame_durations));

	codec_cap_get_val(codec_cap, BT_AUDIO_CODEC_LC3_FRAME_LEN, &data);
	memcpy(&ev.octets_per_frame, data->data.data, sizeof(ev.octets_per_frame));

	codec_cap_get_val(codec_cap, BT_AUDIO_CODEC_LC3_CHAN_COUNT, &data);
	memcpy(&ev.channel_counts, data->data.data, sizeof(ev.channel_counts));

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
	LOG_DBG("stream %p start operation rsp_code %u reason %u", stream, rsp_code, reason);
	already_sent = false;
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
	struct bt_iso_chan *iso_chan;
	struct net_buf *buf;
	uint32_t size;
	uint8_t *data;
	int err;

	dwork = k_work_delayable_from_work(work);
	stream = CONTAINER_OF(dwork, struct audio_stream, audio_send_work);

	if (stream->last_req_seq_num % 201 == 200) {
		iso_chan = bt_bap_stream_iso_chan_get(&stream->stream);
		err = bt_iso_chan_get_tx_sync(iso_chan, &info);
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
	}

	/* Because the seq_num field of the audio_stream struct is atomic_val_t (4 bytes),
	 * let's allow an overflow and just cast it to uint16_t.
	 */
	stream->last_req_seq_num = (uint16_t)atomic_get(&stream->seq_num);

	LOG_DBG("Sending data to ASE: ase_id %d len %d seq %d", stream->stream.ep->status.id,
		size, stream->last_req_seq_num);

	err = bt_bap_stream_send(&stream->stream, buf, stream->last_req_seq_num,
				 BT_ISO_TIMESTAMP_NONE);
	if (err != 0) {
		LOG_ERR("Failed to send audio data to stream: ase_id %d dir seq %d %d err %d",
			stream->ase_id, stream->stream.dir, stream->last_req_seq_num, err);
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
	struct bt_conn_info conn_info;
	uint32_t ret;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];
	(void)bt_conn_get_info(conn, &conn_info);

	stream = stream_find(audio_conn, cp->ase_id);
	if (stream == NULL || stream->stream.dir != BT_AUDIO_DIR_SINK) {
		return BTP_STATUS_FAILED;
	}

	ret = ring_buf_put(&audio_ring_buf, cp->data, cp->data_len);

	rp->data_len = ret;
	*rsp_len = sizeof(*rp) + 1;

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
	struct audio_connection *audio_conn;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Disconnected: %s (reason 0x%02x)", addr, reason);

	audio_conn = &connections[bt_conn_index(conn)];
	memset(audio_conn, 0, sizeof(*audio_conn));
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

static int client_create_unicast_group(struct audio_connection *audio_conn, uint8_t ase_id)
{
	int err;
	struct bt_bap_unicast_group_stream_pair_param pair_params[MAX_STREAMS_COUNT];
	struct bt_bap_unicast_group_stream_param stream_params[MAX_STREAMS_COUNT];
	struct bt_bap_unicast_group_param param;
	size_t stream_cnt = 0;
	size_t src_cnt = 0;
	size_t sink_cnt = 0;
	(void)memset(pair_params, 0, sizeof(pair_params));
	(void)memset(stream_params, 0, sizeof(stream_params));

	for (size_t i = 0; i < MAX_STREAMS_COUNT; i++) {
		struct bt_bap_stream *stream = &audio_conn->streams[i].stream;

		if (stream == NULL || stream->ep == NULL) {
			continue;
		}

		stream_params[stream_cnt].stream = stream;
		stream_params[stream_cnt].qos = &audio_conn->qos;

		if (stream->ep->dir == BT_AUDIO_DIR_SOURCE) {
			pair_params[src_cnt].rx_param = &stream_params[stream_cnt];
			src_cnt++;
		} else {
			pair_params[sink_cnt].tx_param = &stream_params[stream_cnt];
			sink_cnt++;
		}

		stream_cnt++;
	}

	if (stream_cnt == 0) {
		return -EINVAL;
	}

	param.params = pair_params;
	param.params_count = MAX(sink_cnt, src_cnt);
	param.packing = BT_ISO_PACKING_SEQUENTIAL;

	LOG_DBG("Creating unicast group");
	err = bt_bap_unicast_group_create(&param, &audio_conn->unicast_group);
	if (err != 0) {
		LOG_DBG("Could not create unicast group (err %d)", err);
		return -EINVAL;
	}

	return 0;
}

static bool codec_config_store(struct bt_data *data, void *user_data)
{
	struct bt_audio_codec_cfg *codec_cfg = user_data;
	struct bt_audio_codec_data *cdata;

	if (codec_cfg->data_count >= ARRAY_SIZE(codec_cfg->data)) {
		LOG_ERR("No slot available for Codec Config");
		return false;
	}

	cdata = &codec_cfg->data[codec_cfg->data_count];

	if (data->data_len > sizeof(cdata->value)) {
		LOG_ERR("Not enough space for Codec Config: %u > %zu", data->data_len,
			sizeof(cdata->value));
		return false;
	}

	LOG_DBG("#%u type 0x%02x len %u", codec_cfg->data_count, data->type, data->data_len);

	cdata->data.type = data->type;
	cdata->data.data_len = data->data_len;

	/* Deep copy data contents */
	cdata->data.data = cdata->value;
	(void)memcpy(cdata->value, data->data, data->data_len);

	LOG_HEXDUMP_DBG(cdata->value, data->data_len, "data");

	codec_cfg->data_count++;

	return true;
}

static uint8_t ascs_configure_codec(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_configure_codec_cmd *cp = cmd;
	struct bt_conn *conn;
	struct bt_conn_info conn_info;
	struct audio_connection *audio_conn;
	struct audio_stream *stream;
	struct bt_audio_codec_cfg *codec_cfg;
	struct bt_bap_ep *ep;
	struct net_buf_simple buf;

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

	if (cp->ltvs_len != 0) {
		net_buf_simple_init_with_data(&buf, (uint8_t *)cp->ltvs, cp->ltvs_len);

		/* Parse LTV entries */
		bt_data_parse(&buf, codec_config_store, codec_cfg);

		/* Check if all entries could be parsed */
		if (buf.len) {
			LOG_DBG("Unable to parse Codec Config: len %u", buf.len);
			bt_conn_unref(conn);

			return BTP_STATUS_FAILED;
		}
	}

	stream = stream_find(audio_conn, cp->ase_id);

	if (stream == NULL) {
		/* Configure a new stream */
		stream = stream_alloc(audio_conn);
		if (stream == NULL) {
			LOG_DBG("No streams available");
			bt_conn_unref(conn);

			return BTP_STATUS_FAILED;
		}

		if (conn_info.role == BT_HCI_ROLE_CENTRAL) {
			if (audio_conn->end_points_count == 0) {
				bt_conn_unref(conn);

				return BTP_STATUS_FAILED;
			}

			ep = end_point_find(audio_conn, cp->ase_id);

			if (ep == NULL) {
				bt_conn_unref(conn);

				return BTP_STATUS_FAILED;
			}

			err = bt_bap_stream_config(conn, &stream->stream, ep, codec_cfg);
		} else {
			err = server_stream_config(conn, &stream->stream, codec_cfg, &qos_pref);
		}
	} else {
		/* Reconfigure a stream */
		err = bt_bap_stream_reconfig(&stream->stream, codec_cfg);
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

	if (audio_conn->unicast_group != NULL) {
		err = bt_bap_unicast_group_delete(audio_conn->unicast_group);
		if (err != 0) {
			LOG_DBG("Failed to delete the unicast group, err %d", err);
			bt_conn_unref(conn);

			return BTP_STATUS_FAILED;
		}
		audio_conn->unicast_group = NULL;
	}

	qos = &audio_conn->qos;
	qos->phy = BT_AUDIO_CODEC_QOS_2M;
	qos->framing = cp->framing;
	qos->rtn = cp->retransmission_num;
	qos->sdu = cp->max_sdu;
	qos->latency = cp->max_transport_latency;
	qos->interval = sys_get_le24(cp->sdu_interval);
	qos->pd = sys_get_le24(cp->presentation_delay);

	err = client_create_unicast_group(audio_conn, cp->ase_id);
	if (err != 0) {
		LOG_DBG("Unable to create unicast group, err %d", err);
		bt_conn_unref(conn);

		return BTP_STATUS_FAILED;
	}

	LOG_DBG("QoS configuring streams");
	err = bt_bap_stream_qos(conn, audio_conn->unicast_group);
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

	err = bt_bap_stream_start(&stream->stream);
	if (err != 0) {
		LOG_DBG("Could not start stream: %d", err);
		return BTP_STATUS_FAILED;
	}

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
		LOG_DBG("Could not start stream: %d", err);
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
	int err;
	const struct btp_ascs_update_metadata_cmd *cp = cmd;
	struct audio_connection *audio_conn;
	struct audio_stream *stream;
	struct bt_audio_codec_data meta;
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

	meta.data.data = meta.value;
	meta.data.type = BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT;
	meta.data.data_len = 2;
	meta.value[0] = BT_AUDIO_CONTEXT_TYPE_ANY & 0xFF;
	meta.value[1] = (BT_AUDIO_CONTEXT_TYPE_ANY >> 8) & 0xFF;

	LOG_DBG("Updating stream metadata");
	err = bt_bap_stream_metadata(&stream->stream, &meta, 1);
	if (err != 0) {
		LOG_DBG("Failed to update stream metadata, err %d", err);
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
};

static int set_location(void)
{
	int err;

	err = bt_pacs_set_location(BT_AUDIO_DIR_SINK,
				   BT_AUDIO_LOCATION_FRONT_CENTER);
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
};

uint8_t tester_init_pacs(void)
{
	int err;

	bt_bap_unicast_server_register_cb(&unicast_server_cb);

	bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap_sink);
	bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &cap_source);

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
