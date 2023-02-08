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

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>

#include "bap_endpoint.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#define LOG_MODULE_NAME bttester_bap
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
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

static struct bt_codec default_codec =
	BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY, BT_CODEC_LC3_DURATION_10,
		     BT_CODEC_LC3_CHAN_COUNT_SUPPORT(1),
		     40u, 120u, 1u,
		     (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |
		      BT_AUDIO_CONTEXT_TYPE_MEDIA));

struct audio_stream {
	struct bt_bap_stream stream;
	uint8_t ase_id;
	uint8_t conn_id;
	uint16_t seq_num;
	uint16_t max_sdu;
	size_t len_to_send;
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
	struct bt_codec codec;
	struct bt_codec_qos qos;
	struct bt_bap_unicast_group *unicast_group;
	struct bt_bap_ep *end_points[MAX_END_POINTS_COUNT];
	size_t end_points_count;
} connections[CONFIG_BT_MAX_CONN];

static struct bt_bap_unicast_client_discover_params ase_discover_params;

static struct bt_codec_qos_pref qos_pref = BT_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, 0x02,
							     10, 10000, 40000, 10000, 40000);

static void print_codec(const struct bt_codec *codec)
{
	LOG_DBG("codec 0x%02x cid 0x%04x vid 0x%04x count %zu",
	       codec->id, codec->cid, codec->vid, codec->data_count);

	for (size_t i = 0; i < codec->data_count; i++) {
		LOG_DBG("data #%zu: type 0x%02x len %u", i, codec->data[i].data.type,
			codec->data[i].data.data_len);
		LOG_HEXDUMP_DBG(codec->data[i].data.data, codec->data[i].data.data_len -
				sizeof(codec->data[i].data.type), "");
	}

	if (codec->id == BT_CODEC_LC3_ID) {
		/* LC3 uses the generic LTV format - other codecs might do as well */

		uint32_t chan_allocation;

		LOG_DBG("  Frequency: %d Hz", bt_codec_cfg_get_freq(codec));
		LOG_DBG("  Frame Duration: %d us", bt_codec_cfg_get_frame_duration_us(codec));
		if (bt_codec_cfg_get_chan_allocation_val(codec, &chan_allocation) == 0) {
			LOG_DBG("  Channel allocation: 0x%x", chan_allocation);
		}

		LOG_DBG("  Octets per frame: %d (negative means value not pressent)",
			bt_codec_cfg_get_octets_per_frame(codec));
		LOG_DBG("  Frames per SDU: %d",
			bt_codec_cfg_get_frame_blocks_per_sdu(codec, true));
	}

	for (size_t i = 0; i < codec->meta_count; i++) {
		LOG_DBG("meta #%zu: type 0x%02x len %u", i, codec->meta[i].data.type,
			codec->meta[i].data.data_len);
		LOG_HEXDUMP_DBG(codec->meta[i].data.data, codec->meta[i].data.data_len -
				sizeof(codec->meta[i].data.type), "");
	}
}

static inline void print_qos(const struct bt_codec_qos *qos)
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

static int validate_codec_parameters(const struct bt_codec *codec)
{
	int freq_hz;
	int frame_duration_us;
	int frames_per_sdu;
	int octets_per_frame;
	int chan_allocation_err;
	enum bt_audio_location chan_allocation;

	freq_hz = bt_codec_cfg_get_freq(codec);
	frame_duration_us = bt_codec_cfg_get_frame_duration_us(codec);
	chan_allocation_err = bt_codec_cfg_get_chan_allocation_val(codec, &chan_allocation);
	octets_per_frame = bt_codec_cfg_get_octets_per_frame(codec);
	frames_per_sdu = bt_codec_cfg_get_frame_blocks_per_sdu(codec, true);

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
		      const struct bt_codec *codec, struct bt_bap_stream **stream,
		      struct bt_codec_qos_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	struct audio_connection *audio_conn;
	struct audio_stream *stream_wrap;

	LOG_DBG("ASE Codec Config: ep %p dir %u", ep, dir);

	print_codec(codec);

	if (validate_codec_parameters(codec)) {
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
			const struct bt_codec *codec, struct bt_codec_qos_pref *const pref,
			struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("ASE Codec Reconfig: stream %p", stream);

	print_codec(codec);

	return 0;
}

static int lc3_qos(struct bt_bap_stream *stream, const struct bt_codec_qos *qos,
		   struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("QoS: stream %p qos %p", stream, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_bap_stream *stream, const struct bt_codec_data *meta,
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
	case BT_AUDIO_METADATA_TYPE_EXTENDED: /* 1 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_VENDOR: /* 1 - 255 octets */
		if (len < 1) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_CCID_LIST: /* 2 - 254 octets */
		if (len < 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO: /* 0 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI: /* 0 - 255 octets */
		return true;
	default:
		return false;
	}
}

static int lc3_metadata(struct bt_bap_stream *stream,
			const struct bt_codec_data *meta,
			size_t meta_count, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Metadata: stream %p meta_count %zu", stream, meta_count);

	for (size_t i = 0; i < meta_count; i++) {
		const struct bt_codec_data *data = data = &meta[i];

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

static void stream_configured(struct bt_bap_stream *stream,
			      const struct bt_codec_qos_pref *pref)
{
	struct audio_connection *audio_conn;
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	LOG_DBG("Configured stream %p", stream);
	a_stream->conn_id = bt_conn_index(stream->conn);
	audio_conn = &connections[a_stream->conn_id];
	audio_conn->qos.pd = pref->pd_min;
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
	btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
					     BT_ASCS_START_OP, BTP_STATUS_SUCCESS);
}

static void stream_stopped(struct bt_bap_stream *stream, uint8_t reason)
{
	struct audio_stream *a_stream = CONTAINER_OF(stream, struct audio_stream, stream);

	LOG_DBG("Stopped stream %p with reason 0x%02X", stream, reason);
	btp_send_ascs_operation_completed_ev(stream->conn, a_stream->ase_id,
					     BT_ASCS_STOP_OP, BTP_STATUS_SUCCESS);
}

static void stream_recv(struct bt_bap_stream *stream,
			const struct bt_iso_recv_info *info,
			struct net_buf *buf)
{
	LOG_DBG("Incoming audio on stream %p len %u", stream, buf->len);
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

static void btp_send_pac_codec_found_ev(struct bt_conn *conn, const struct bt_codec *codec,
					enum bt_audio_dir dir)
{
	struct btp_bap_codec_cap_found_ev ev;
	struct bt_conn_info info;
	const struct bt_codec_data *data;

	(void)bt_conn_get_info(conn, &info);
	bt_addr_le_copy(&ev.address, info.le.dst);

	ev.dir = dir;
	ev.coding_format = codec->id;

	bt_codec_get_val(codec, BT_CODEC_LC3_FREQ, &data);
	memcpy(&ev.frequencies, data->data.data, sizeof(ev.frequencies));

	bt_codec_get_val(codec, BT_CODEC_LC3_DURATION, &data);
	memcpy(&ev.frame_durations, data->data.data, sizeof(ev.frame_durations));

	bt_codec_get_val(codec, BT_CODEC_LC3_FRAME_LEN, &data);
	memcpy(&ev.octets_per_frame, data->data.data, sizeof(ev.octets_per_frame));

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

static void discover_remote_ases_cb(struct bt_conn *conn, struct bt_codec *codec,
				    struct bt_bap_ep *ep,
				    struct bt_bap_unicast_client_discover_params *params)
{
	enum bt_audio_dir dir = params->dir;
	struct audio_connection *audio_conn;

	if (params->err != 0 && params->err != BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		LOG_DBG("Discover remote ASEs failed: %d", params->err);
		btp_send_discovery_completed_ev(conn, BTP_BAP_DISCOVERY_STATUS_FAILED);

		return;
	}

	if (codec != NULL) {
		LOG_DBG("Discovered codec capabilities %p", codec);
		print_codec(codec);
		btp_send_pac_codec_found_ev(conn, codec, dir);

		return;
	}

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

	LOG_DBG("Discover complete");

	if (params->err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		LOG_DBG("Discover remote ASEs completed without finding any source ASEs");
	} else {
		LOG_DBG("Discover remote ASEs complete: err %d", params->err);
	}

	(void)memset(params, 0, sizeof(*params));

	if (dir == BT_AUDIO_DIR_SINK) {
		int err;

		params->func = discover_remote_ases_cb;
		params->dir = BT_AUDIO_DIR_SOURCE;

		err = bt_bap_unicast_client_discover(conn, params);
		if (err != 0) {
			LOG_DBG("Failed to discover source ASEs: %d", err);
			btp_send_discovery_completed_ev(conn, BTP_BAP_DISCOVERY_STATUS_FAILED);
		}

		return;
	}

	btp_send_discovery_completed_ev(conn, BTP_BAP_DISCOVERY_STATUS_SUCCESS);
}

static uint8_t bap_discover(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	const struct btp_bap_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	struct audio_connection *audio_conn;
	struct bt_conn_info conn_info;
	static struct bt_bap_unicast_client_discover_params *params = &ase_discover_params;
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

	params->func = discover_remote_ases_cb;
	params->dir = BT_AUDIO_DIR_SINK;

	err = bt_bap_unicast_client_discover(conn, params);
	if (err != 0) {
		LOG_DBG("Failed to discover remote ASEs: %d", err);
		bt_conn_unref(conn);

		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);

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
	.codec = &default_codec,
};

static struct bt_pacs_cap cap_source = {
	.codec = &default_codec,
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
				struct bt_codec *codec, struct bt_codec_qos_pref *qos)
{
	int err;
	struct bt_bap_ep *ep;

	err = bt_bap_unicast_server_config_ase(conn, stream, codec, qos);
	if (err != 0) {
		return err;
	}

	print_codec(&default_codec);

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

	for (size_t i = 0; i < MAX_STREAMS_COUNT; i++) {
		struct bt_bap_stream *stream = &audio_conn->streams[i].stream;

		if (stream == NULL || stream->ep == NULL) {
			continue;
		}

		if (stream->ep->status.id != ase_id) {
			/* TODO: For now one stream per group is configured */
			continue;
		}

		stream_params[stream_cnt].stream = stream;
		stream_params[stream_cnt].qos = &audio_conn->qos;

		if (stream->ep->dir == BT_AUDIO_DIR_SOURCE) {
			pair_params[stream_cnt].tx_param = NULL;
			pair_params[stream_cnt].rx_param = &stream_params[stream_cnt];
		} else {
			pair_params[stream_cnt].tx_param = &stream_params[stream_cnt];
			pair_params[stream_cnt].rx_param = NULL;
		}

		stream_cnt++;
	}

	if (stream_cnt == 0) {
		return -EINVAL;
	}

	param.params = pair_params;
	param.params_count = stream_cnt;
	param.packing = BT_ISO_PACKING_SEQUENTIAL;

	LOG_DBG("Creating unicast group");
	err = bt_bap_unicast_group_create(&param, &audio_conn->unicast_group);
	if (err != 0) {
		LOG_DBG("Could not create unicast group (err %d)", err);
		return -EINVAL;
	}

	return 0;
}

void helper_create_codec_config(struct bt_codec *codec, uint8_t	freq, uint8_t frame_duration,
				uint32_t chan_loc, uint16_t octets_per_frame,
				uint8_t frames_per_sdu, uint16_t stream_context)
{
	struct bt_codec tmp_codec;

	memset(&tmp_codec, 0, sizeof(tmp_codec));

	tmp_codec = (struct bt_codec)BT_CODEC_LC3_CONFIG(freq, frame_duration, chan_loc,
		octets_per_frame, frames_per_sdu, stream_context);

	memcpy(codec, &tmp_codec, sizeof(*codec));

	/* Macro BT_CODEC_LC3_CONFIG inits .data field of struct bt_data with local memory.
	 * We have to init .value field of struct bt_codec_data separately.
	 */
	for (int i = 0; i < CONFIG_BT_CODEC_MAX_DATA_COUNT; i++) {
		struct bt_codec_data *codec_data = &codec->data[i];

		memcpy(codec_data->value, codec_data->data.data, sizeof(codec_data->value));
		codec_data->data.data = codec_data->value;
	}

	for (int i = 0; i < CONFIG_BT_CODEC_MAX_METADATA_COUNT; i++) {
		struct bt_codec_data *metadata = &codec->meta[i];

		memcpy(metadata->value, metadata->data.data, sizeof(metadata->value));
		metadata->data.data = metadata->value;
	}

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
	struct bt_codec *codec;
	struct bt_bap_ep *ep;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	audio_conn = &connections[bt_conn_index(conn)];

	(void)bt_conn_get_info(conn, &conn_info);

	codec = &audio_conn->codec;
	memset(codec, 0, sizeof(struct bt_codec));

	if (cp->coding_format != BT_CODEC_LC3_ID) {
		bt_conn_unref(conn);

		return BTP_STATUS_FAILED;
	}

	helper_create_codec_config(codec, cp->freq, cp->frame_duration,	cp->audio_locations,
				   cp->octets_per_frame, 1u, BT_AUDIO_CONTEXT_TYPE_ANY);

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

			err = bt_bap_stream_config(conn, &stream->stream, ep, codec);
		} else {
			err = server_stream_config(conn, &stream->stream, codec, &qos_pref);
		}
	} else {
		/* Reconfigure a stream */
		err = bt_bap_stream_reconfig(&stream->stream, codec);
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
	struct bt_codec_qos *qos;
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
	qos = &audio_conn->qos;
	qos->phy = BT_CODEC_QOS_2M;
	qos->framing = cp->framing;
	qos->rtn = cp->retransmission_num;
	qos->sdu = cp->max_sdu;
	qos->latency = cp->max_transport_latency;
	qos->interval = cp->sdu_interval;
	/* qos->pd set to minimum at codec config callback */

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

	LOG_DBG("Starting stream");
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
	struct bt_codec_data meta;
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
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
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
