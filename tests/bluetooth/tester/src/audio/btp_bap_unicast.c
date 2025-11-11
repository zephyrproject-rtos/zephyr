/* btp_bap_unicast.c - Bluetooth BAP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/hci_types.h>
#include <hci_core.h>

#include "ascs_internal.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#define LOG_MODULE_NAME bttester_bap_unicast
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);
#include "btp/btp.h"
#include "btp_bap_audio_stream.h"
#include "btp_bap_unicast.h"

static struct bt_bap_qos_cfg_pref qos_pref =
	BT_BAP_QOS_CFG_PREF(true, BT_GAP_LE_PHY_2M, 0x02, 10, 10000, 40000, 10000, 40000);

static struct btp_bap_unicast_connection connections[CONFIG_BT_MAX_CONN];
static struct btp_bap_unicast_group cigs[CONFIG_BT_ISO_MAX_CIG];

static struct btp_bap_unicast_stream *stream_bap_to_unicast(const struct bt_bap_stream *stream)
{
	return CONTAINER_OF(CONTAINER_OF(CONTAINER_OF(stream, struct bt_cap_stream, bap_stream),
		struct btp_bap_audio_stream, cap_stream),
		struct btp_bap_unicast_stream, audio_stream);
}

static inline struct bt_bap_stream *stream_unicast_to_bap(struct btp_bap_unicast_stream *stream)
{
	return &stream->audio_stream.cap_stream.bap_stream;
}

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

		ret = bt_audio_codec_cfg_get_frame_dur(codec_cfg);
		if (ret > 0) {
			LOG_DBG("  Frame Duration: %d us",
				bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret));
		}

		ret = bt_audio_codec_cfg_get_chan_allocation(codec_cfg, &chan_allocation, false);
		if (ret == 0) {
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

void btp_bap_unicast_stream_free(struct btp_bap_unicast_stream *stream)
{
	(void)memset(stream, 0, sizeof(*stream));
}

struct btp_bap_unicast_stream *btp_bap_unicast_stream_find(
	struct btp_bap_unicast_connection *conn, uint8_t ase_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn->streams); i++) {
		struct bt_bap_stream *stream = stream_unicast_to_bap(&conn->streams[i]);
		struct bt_bap_ep_info info;

		if (stream->ep == NULL) {
			continue;
		}

		(void)bt_bap_ep_get_info(stream->ep, &info);
		if (info.id == ase_id) {
			return &conn->streams[i];
		}
	}

	return NULL;
}

struct bt_bap_ep *btp_bap_unicast_end_point_find(struct btp_bap_unicast_connection *conn,
						 uint8_t ase_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn->end_points); i++) {
		struct bt_bap_ep_info info;
		struct bt_bap_ep *ep = conn->end_points[i];

		if (ep == NULL) {
			continue;
		}

		(void)bt_bap_ep_get_info(ep, &info);
		if (info.id == ase_id) {
			return ep;
		}
	}

	return NULL;
}

static void btp_send_ascs_ase_state_changed_ev(struct bt_conn *conn, uint8_t ase_id,
					       enum bt_bap_ep_state state)
{
	struct btp_ascs_ase_state_changed_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));
	ev.ase_id = ase_id;
	ev.state = (uint8_t)state;

	tester_event(BTP_SERVICE_ID_ASCS, BTP_ASCS_EV_ASE_STATE_CHANGED, &ev, sizeof(ev));
}

static void btp_send_ascs_operation_completed_ev(struct bt_conn *conn, uint8_t ase_id,
						 uint8_t opcode, uint8_t status)
{
	struct btp_ascs_operation_completed_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));
	ev.ase_id = ase_id;
	ev.opcode = opcode;
	ev.status = status;
	ev.flags = 0;

	tester_event(BTP_SERVICE_ID_ASCS, BTP_ASCS_EV_OPERATION_COMPLETED, &ev, sizeof(ev));
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
	const uint8_t *data;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.dir = dir;
	ev.coding_format = codec_cap->id;

	if (codec_cap_get_val(codec_cap, BT_AUDIO_CODEC_CAP_TYPE_FREQ, &data)) {
		memcpy(&ev.frequencies, data, sizeof(ev.frequencies));
	}

	if (codec_cap_get_val(codec_cap, BT_AUDIO_CODEC_CAP_TYPE_DURATION, &data)) {
		memcpy(&ev.frame_durations, data, sizeof(ev.frame_durations));
	}

	if (codec_cap_get_val(codec_cap, BT_AUDIO_CODEC_CAP_TYPE_FRAME_LEN, &data)) {
		memcpy(&ev.octets_per_frame, data, sizeof(ev.octets_per_frame));
	}

	if (codec_cap_get_val(codec_cap, BT_AUDIO_CODEC_CAP_TYPE_CHAN_COUNT, &data)) {
		memcpy(&ev.channel_counts, data, sizeof(ev.channel_counts));
	}

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_CODEC_CAP_FOUND, &ev, sizeof(ev));
}

static void btp_send_ase_found_ev(struct bt_conn *conn, struct bt_bap_ep *ep)
{
	struct bt_bap_ep_info info;
	struct btp_bap_ase_found_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	(void)bt_bap_ep_get_info(ep, &info);
	ev.ase_id = info.id;
	ev.dir = info.dir;

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_ASE_FOUND, &ev, sizeof(ev));
}

static inline void print_qos(const struct bt_bap_qos_cfg *qos)
{
	LOG_DBG("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u rtn %u latency %u pd %u",
		qos->interval, qos->framing, qos->phy, qos->sdu, qos->rtn, qos->latency, qos->pd);
}

static int validate_codec_parameters(const struct bt_audio_codec_cfg *codec_cfg)
{
	int frames_per_sdu;
	int octets_per_frame;
	int chan_allocation_err;
	enum bt_audio_location chan_allocation;
	int ret;

	chan_allocation_err =
		bt_audio_codec_cfg_get_chan_allocation(codec_cfg, &chan_allocation, false);
	octets_per_frame = bt_audio_codec_cfg_get_octets_per_frame(codec_cfg);
	frames_per_sdu = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, true);

	ret = bt_audio_codec_cfg_get_freq(codec_cfg);
	if (ret < 0) {
		LOG_DBG("Error: Invalid codec frequency: %d", ret);
		return -EINVAL;
	}

	ret = bt_audio_codec_cfg_get_frame_dur(codec_cfg);
	if (ret < 0) {
		LOG_DBG("Error: Invalid frame duration: %d", ret);
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
		      struct bt_bap_qos_cfg_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	struct bt_bap_ep_info info;
	struct btp_bap_unicast_connection *u_conn;
	struct btp_bap_unicast_stream *u_stream;

	LOG_DBG("ASE Codec Config: ep %p dir %u", ep, dir);

	print_codec_cfg(codec_cfg);
	(void)bt_bap_ep_get_info(ep, &info);

	if (validate_codec_parameters(codec_cfg)) {
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_CONF_REJECTED,
				       BT_BAP_ASCS_REASON_CODEC_DATA);

		return -ENOTSUP;
	}

	u_conn = &connections[bt_conn_index(conn)];
	u_stream = btp_bap_unicast_stream_alloc(u_conn);
	if (u_stream == NULL) {
		LOG_DBG("No free stream available");
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_NO_MEM, BT_BAP_ASCS_REASON_NONE);

		return -ENOMEM;
	}

	*stream = stream_unicast_to_bap(u_stream);
	LOG_DBG("ASE Codec Config stream %p", *stream);

	if (dir == BT_AUDIO_DIR_SOURCE) {
		u_conn->configured_source_stream_count++;
	} else {
		u_conn->configured_sink_stream_count++;
	}

	*pref = qos_pref;

	return 0;
}

static int lc3_reconfig(struct bt_bap_stream *stream, enum bt_audio_dir dir,
			const struct bt_audio_codec_cfg *codec_cfg,
			struct bt_bap_qos_cfg_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("ASE Codec Reconfig: stream %p", stream);

	print_codec_cfg(codec_cfg);

	return 0;
}

static int lc3_qos(struct bt_bap_stream *stream, const struct bt_bap_qos_cfg *qos,
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
	LOG_DBG("Metadata: stream %p meta_len %zu", stream, meta_len);

	return bt_audio_data_parse(meta, meta_len, data_func_cb, rsp);
}

static int lc3_start(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Start: stream %p", stream);

	return 0;
}

static int lc3_metadata(struct bt_bap_stream *stream, const uint8_t meta[], size_t meta_len,
			struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Metadata: stream %p meta_count %zu", stream, meta_len);

	return bt_audio_data_parse(meta, meta_len, data_func_cb, rsp);
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

static void stream_state_changed(struct bt_bap_stream *stream)
{
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);
	struct bt_bap_ep_info info;
	int err;

	if (stream->ep == NULL) {
		info.state = BT_BAP_EP_STATE_IDLE;
	} else {
		err = bt_bap_ep_get_info(stream->ep, &info);
		if (err != 0) {
			LOG_ERR("Failed to get info: %d", err);

			return;
		}
	}

	btp_send_ascs_ase_state_changed_ev(stream->conn, u_stream->ase_id, info.state);
}

static void stream_configured_cb(struct bt_bap_stream *stream,
				 const struct bt_bap_qos_cfg_pref *pref)
{
	struct bt_bap_ep_info info;
	struct btp_bap_unicast_connection *u_conn;
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);

	(void)bt_bap_ep_get_info(stream->ep, &info);
	LOG_DBG("Configured stream %p, ep %u, dir %u", stream, info.id, info.dir);

	u_stream->conn_id = bt_conn_index(stream->conn);
	u_conn = &connections[u_stream->conn_id];
	u_stream->ase_id = info.id;

	stream_state_changed(stream);
}

static void stream_qos_set_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("QoS set stream %p", stream);

	stream_state_changed(stream);
}

static void stream_enabled_cb(struct bt_bap_stream *stream)
{
	struct bt_bap_ep_info info;
	struct bt_conn_info conn_info;
	int err;

	LOG_DBG("Enabled stream %p", stream);

	(void)bt_bap_ep_get_info(stream->ep, &info);
	(void)bt_conn_get_info(stream->conn, &conn_info);
	if (conn_info.role == BT_HCI_ROLE_PERIPHERAL && info.dir == BT_AUDIO_DIR_SINK) {
		/* Automatically do the receiver start ready operation */
		/* TODO: This should ideally be done by the upper tester */
		err = bt_bap_stream_start(stream);
		if (err != 0) {
			LOG_DBG("Failed to start stream %p", stream);

			return;
		}
	}

	stream_state_changed(stream);
}

static void stream_metadata_updated_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("Metadata updated stream %p", stream);

	stream_state_changed(stream);
}

static void stream_disabled_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("Disabled stream %p", stream);

	stream_state_changed(stream);
}

static void stream_released_cb(struct bt_bap_stream *stream)
{
	uint8_t cig_id;
	struct bt_bap_ep_info info;
	struct btp_bap_unicast_connection *u_conn;
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);
	struct bt_conn *conn;

	LOG_DBG("Released stream %p", stream);

	u_conn = &connections[u_stream->conn_id];

	/* TODO: Fix this as stream->ep is always NULL in the released callback */
	if (stream->ep != NULL) {
		(void)bt_bap_ep_get_info(stream->ep, &info);
		if (info.dir == BT_AUDIO_DIR_SINK) {
			u_conn->configured_sink_stream_count--;
		} else {
			u_conn->configured_source_stream_count--;
		}
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &u_conn->address);
	if (conn == NULL) {
		LOG_ERR("Unknown connection");
		return;
	}

	btp_send_ascs_ase_state_changed_ev(conn, u_stream->ase_id, BT_BAP_EP_STATE_IDLE);
	bt_conn_unref(conn);

	cig_id = u_stream->cig_id;
	btp_bap_unicast_stream_free(u_stream);

	if (cigs[cig_id].in_use == true &&
	    u_conn->configured_sink_stream_count == 0 &&
	    u_conn->configured_source_stream_count == 0) {
		struct btp_bap_unicast_group *u_cig = &cigs[cig_id];

		/* The unicast group will be deleted only at release of the last stream */
		LOG_DBG("Deleting unicast group");

		int err = bt_bap_unicast_group_delete(u_cig->cig);

		if (err != 0) {
			LOG_DBG("Unable to delete unicast group: %d", err);

			return;
		}

		memset(u_cig, 0, sizeof(*u_cig));
	}
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);

	/* Callback called on transition to Streaming state */

	LOG_DBG("Started stream %p", stream);

	/* Start TX */
	if (btp_bap_audio_stream_can_send(&u_stream->audio_stream)) {
		int err;

		err = btp_bap_audio_stream_tx_register(&u_stream->audio_stream);
		if (err != 0) {
			LOG_ERR("Failed to register stream: %d", err);
		}
	}

	stream_state_changed(stream);
}

static void stream_connected_cb(struct bt_bap_stream *stream)
{
	struct bt_conn_info conn_info;

	LOG_DBG("Connected stream %p", stream);

	(void)bt_conn_get_info(stream->conn, &conn_info);
	if (conn_info.role == BT_HCI_ROLE_CENTRAL) {
		struct bt_bap_ep_info ep_info;
		int err;

		err = bt_bap_ep_get_info(stream->ep, &ep_info);
		if (err != 0) {
			LOG_ERR("Failed to get info: %d", err);

			return;
		}

		if (ep_info.dir == BT_AUDIO_DIR_SOURCE) {
			/* Automatically do the receiver start ready operation for source ASEs as
			 * the client
			 */
			err = bt_bap_stream_start(stream);
			if (err != 0) {
				LOG_ERR("Failed to start stream %p", stream);
			}
		} else {
			struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);

			btp_send_ascs_operation_completed_ev(stream->conn, u_stream->ase_id,
							     BT_ASCS_START_OP,
							     BTP_ASCS_STATUS_SUCCESS);
		}
	}
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);

	LOG_DBG("Stopped stream %p with reason 0x%02X", stream, reason);

	if (btp_bap_audio_stream_can_send(&u_stream->audio_stream)) {
		int err;

		err = btp_bap_audio_stream_tx_unregister(&u_stream->audio_stream);
		if (err != 0) {
			LOG_ERR("Failed to unregister stream: %d", err);
		}
	}

	btp_send_ascs_operation_completed_ev(stream->conn, u_stream->ase_id,
					     BT_ASCS_STOP_OP, BTP_STATUS_SUCCESS);
	stream_state_changed(stream);
}

static void send_stream_received_ev(struct bt_conn *conn, struct bt_bap_ep *ep,
				    uint8_t data_len, uint8_t *data)
{
	struct btp_bap_stream_received_ev *ev;
	struct bt_bap_ep_info ep_info;
	int err;

	err = bt_bap_ep_get_info(ep, &ep_info);
	__ASSERT_NO_MSG(err == 0);

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + data_len, (uint8_t **)&ev);

	LOG_DBG("Stream received, ep %d, dir %d, len %d", ep_info.id, ep_info.dir, data_len);

	bt_addr_le_copy(&ev->address, bt_conn_get_dst(conn));

	ev->ase_id = ep_info.id;
	ev->data_len = data_len;
	memcpy(ev->data, data, data_len);

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_STREAM_RECEIVED, ev, sizeof(*ev) + data_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void stream_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);

	if (u_stream->already_sent == false) {
		/* For now, send just a first packet, to limit the number
		 * of logs and not unnecessarily spam through btp.
		 */
		LOG_DBG("Incoming audio on stream %p len %u flags 0x%02X seq_num %u and ts %u",
			stream, buf->len, info->flags, info->seq_num, info->ts);

		if ((info->flags & BT_ISO_FLAGS_VALID) != 0) {
			u_stream->already_sent = true;
			send_stream_received_ev(stream->conn, stream->ep, buf->len, buf->data);
		}
	}
}

static struct bt_bap_stream_ops stream_ops = {
	.configured = stream_configured_cb,
	.qos_set = stream_qos_set_cb,
	.enabled = stream_enabled_cb,
	.metadata_updated = stream_metadata_updated_cb,
	.disabled = stream_disabled_cb,
	.released = stream_released_cb,
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
	.recv = stream_recv_cb,
	.sent = btp_bap_audio_stream_sent_cb,
	.connected = stream_connected_cb,
};

struct btp_bap_unicast_stream *btp_bap_unicast_stream_alloc(
	struct btp_bap_unicast_connection *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn->streams); i++) {
		struct btp_bap_unicast_stream *stream = &conn->streams[i];

		if (stream->in_use == false) {
			bt_bap_stream_cb_register(stream_unicast_to_bap(stream), &stream_ops);
			stream->in_use = true;

			return stream;
		}
	}

	return NULL;
}

static void unicast_client_location_cb(struct bt_conn *conn,
				       enum bt_audio_dir dir,
				       enum bt_audio_location loc)
{
	LOG_DBG("dir %u loc %X", dir, loc);
}

static void unicast_client_available_contexts_cb(struct bt_conn *conn,
						 enum bt_audio_context snk_ctx,
						 enum bt_audio_context src_ctx)
{
	LOG_DBG("snk ctx %u src ctx %u", snk_ctx, src_ctx);
}

static void unicast_client_config_cb(struct bt_bap_stream *stream,
				     enum bt_bap_ascs_rsp_code rsp_code,
				     enum bt_bap_ascs_reason reason)
{
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);

	LOG_DBG("stream %p config operation rsp_code %u reason %u", stream, rsp_code, reason);

	btp_send_ascs_operation_completed_ev(stream->conn, u_stream->ase_id, BT_ASCS_CONFIG_OP,
					     rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS
						     ? BTP_ASCS_STATUS_SUCCESS
						     : BTP_ASCS_STATUS_FAILED);
}

static void unicast_client_qos_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
				  enum bt_bap_ascs_reason reason)
{
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);

	LOG_DBG("stream %p qos operation rsp_code %u reason %u", stream, rsp_code, reason);

	btp_send_ascs_operation_completed_ev(stream->conn, u_stream->ase_id, BT_ASCS_QOS_OP,
					     rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS
						     ? BTP_ASCS_STATUS_SUCCESS
						     : BTP_ASCS_STATUS_FAILED);
}

static void unicast_client_enable_cb(struct bt_bap_stream *stream,
				     enum bt_bap_ascs_rsp_code rsp_code,
				     enum bt_bap_ascs_reason reason)
{
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);

	LOG_DBG("stream %p enable operation rsp_code %u reason %u", stream, rsp_code, reason);

	btp_send_ascs_operation_completed_ev(stream->conn, u_stream->ase_id, BT_ASCS_ENABLE_OP,
					     rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS
						     ? BTP_ASCS_STATUS_SUCCESS
						     : BTP_ASCS_STATUS_FAILED);
}

static void unicast_client_start_cb(struct bt_bap_stream *stream,
				    enum bt_bap_ascs_rsp_code rsp_code,
				    enum bt_bap_ascs_reason reason)
{
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);
	struct bt_bap_ep_info ep_info;
	int err;

	/* Callback called on Receiver Start Ready notification from ASE Control Point */

	LOG_DBG("stream %p start operation rsp_code %u reason %u", stream, rsp_code, reason);
	u_stream->already_sent = false;

	err = bt_bap_ep_get_info(stream->ep, &ep_info);
	if (err != 0) {
		LOG_ERR("Failed to get ep info: %d", err);

		return;
	}

	if (ep_info.dir == BT_AUDIO_DIR_SOURCE) {
		btp_send_ascs_operation_completed_ev(stream->conn, u_stream->ase_id,
						     BT_ASCS_START_OP, BTP_ASCS_STATUS_SUCCESS);
	}
}

static void unicast_client_stop_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
				   enum bt_bap_ascs_reason reason)
{
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);

	LOG_DBG("stream %p stop operation rsp_code %u reason %u", stream, rsp_code, reason);

	btp_send_ascs_operation_completed_ev(stream->conn, u_stream->ase_id, BT_ASCS_STOP_OP,
					     rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS
						     ? BTP_ASCS_STATUS_SUCCESS
						     : BTP_ASCS_STATUS_FAILED);
}

static void unicast_client_disable_cb(struct bt_bap_stream *stream,
				      enum bt_bap_ascs_rsp_code rsp_code,
				      enum bt_bap_ascs_reason reason)
{
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);

	LOG_DBG("stream %p disable operation rsp_code %u reason %u", stream, rsp_code, reason);

	btp_send_ascs_operation_completed_ev(stream->conn, u_stream->ase_id, BT_ASCS_DISABLE_OP,
					     rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS
						     ? BTP_ASCS_STATUS_SUCCESS
						     : BTP_ASCS_STATUS_FAILED);
}

static void unicast_client_metadata_cb(struct bt_bap_stream *stream,
				       enum bt_bap_ascs_rsp_code rsp_code,
				       enum bt_bap_ascs_reason reason)
{
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);

	LOG_DBG("stream %p metadata operation rsp_code %u reason %u", stream, rsp_code, reason);

	btp_send_ascs_operation_completed_ev(stream->conn, u_stream->ase_id, BT_ASCS_METADATA_OP,
					     rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS
						     ? BTP_ASCS_STATUS_SUCCESS
						     : BTP_ASCS_STATUS_FAILED);
}

static void unicast_client_release_cb(struct bt_bap_stream *stream,
				      enum bt_bap_ascs_rsp_code rsp_code,
				      enum bt_bap_ascs_reason reason)
{
	struct btp_bap_unicast_stream *u_stream = stream_bap_to_unicast(stream);

	LOG_DBG("stream %p release operation rsp_code %u reason %u", stream, rsp_code, reason);

	btp_send_ascs_operation_completed_ev(stream->conn, u_stream->ase_id, BT_ASCS_RELEASE_OP,
					     rsp_code == BT_BAP_ASCS_RSP_CODE_SUCCESS
						     ? BTP_ASCS_STATUS_SUCCESS
						     : BTP_ASCS_STATUS_FAILED);
}

static void unicast_client_pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir,
					 const struct bt_audio_codec_cap *codec_cap)
{
	LOG_DBG("");

	if (codec_cap != NULL) {
		LOG_DBG("Discovered codec capabilities %p", codec_cap);
		print_codec_cap(codec_cap);
		btp_send_pac_codec_found_ev(conn, codec_cap, dir);
	}
}

static void btp_send_discovery_completed_ev(struct bt_conn *conn, uint8_t status)
{
	struct btp_bap_discovery_completed_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));
	ev.status = status;

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_DISCOVERY_COMPLETED, &ev, sizeof(ev));
}

static void unicast_client_endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir,
				       struct bt_bap_ep *ep)
{
	struct btp_bap_unicast_connection *u_conn;

	LOG_DBG("");

	if (ep != NULL) {
		struct bt_bap_ep_info ep_info;
		int err;

		err = bt_bap_ep_get_info(ep, &ep_info);
		__ASSERT_NO_MSG(err == 0);

		LOG_DBG("Discovered ASE %p, id %u, dir 0x%02x", ep, ep_info.id, ep_info.dir);

		u_conn = &connections[bt_conn_index(conn)];

		if (u_conn->end_points_count >= CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +
						    CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT) {
			LOG_DBG("Failed to cache ep %p due to configured limit: %zu", ep,
				u_conn->end_points_count);

			btp_send_discovery_completed_ev(conn, BTP_BAP_DISCOVERY_STATUS_FAILED);

			return;
		}

		u_conn->end_points[u_conn->end_points_count++] = ep;
		btp_send_ase_found_ev(conn, ep);

		return;
	}
}

static void unicast_client_discover_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
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

static struct bt_bap_unicast_server_register_param param = {
	CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
	CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT
};

static struct bt_bap_unicast_client_cb unicast_client_cbs = {
	.location = unicast_client_location_cb,
	.available_contexts = unicast_client_available_contexts_cb,
	.config = unicast_client_config_cb,
	.qos = unicast_client_qos_cb,
	.enable = unicast_client_enable_cb,
	.start = unicast_client_start_cb,
	.stop = unicast_client_stop_cb,
	.disable = unicast_client_disable_cb,
	.metadata = unicast_client_metadata_cb,
	.release = unicast_client_release_cb,
	.pac_record = unicast_client_pac_record_cb,
	.endpoint = unicast_client_endpoint_cb,
	.discover = unicast_client_discover_cb,
};

uint8_t btp_bap_discover(const void *cmd, uint16_t cmd_len,
			 void *rsp, uint16_t *rsp_len)
{
	const struct btp_bap_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	struct btp_bap_unicast_connection *u_conn;
	struct bt_conn_info conn_info;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	u_conn = &connections[bt_conn_index(conn)];
	(void)bt_conn_get_info(conn, &conn_info);

	if (u_conn->end_points_count > 0 || conn_info.role != BT_HCI_ROLE_CENTRAL) {
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

static int server_stream_config(struct bt_conn *conn, struct bt_bap_stream *stream,
				struct bt_audio_codec_cfg *codec_cfg,
				struct bt_bap_qos_cfg_pref *qos)
{
	int err;
	struct bt_bap_ep_info info;

	err = bt_bap_unicast_server_config_ase(conn, stream, codec_cfg, qos);
	if (err != 0) {
		return err;
	}

	print_codec_cfg(codec_cfg);
	(void)bt_bap_ep_get_info(stream->ep, &info);

	LOG_DBG("ASE Codec Config: ase_id %u dir %u", info.id, info.dir);
	LOG_DBG("ASE Codec Config stream %p", stream);

	return 0;
}

static uint8_t client_add_ase_to_cis(struct btp_bap_unicast_connection *u_conn, uint8_t ase_id,
				     uint8_t cis_id, uint8_t cig_id)
{
	struct btp_bap_unicast_stream *stream;

	if (cig_id >= CONFIG_BT_ISO_MAX_CIG ||
	    cis_id >= CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT) {
		return -EINVAL;
	}

	stream = btp_bap_unicast_stream_find(u_conn, ase_id);
	if (stream == NULL) {
		return -EINVAL;
	}

	LOG_DBG("Added ASE %u to CIS %u at CIG %u", ase_id, cis_id, cig_id);

	stream->cig_id = cig_id;
	stream->cis_id = cis_id;

	return 0;
}

static int client_unicast_group_param_set(struct btp_bap_unicast_connection *u_conn, uint8_t cig_id,
	struct bt_bap_unicast_group_stream_pair_param *pair_params,
	struct bt_bap_unicast_group_stream_param **stream_param_ptr)
{
	struct bt_bap_unicast_group_stream_param *stream_params = *stream_param_ptr;

	for (size_t i = 0; i < ARRAY_SIZE(u_conn->streams); i++) {
		struct bt_bap_ep_info info;
		struct bt_bap_ep *ep;
		struct btp_bap_unicast_stream *u_stream = &u_conn->streams[i];
		struct bt_bap_stream *stream = stream_unicast_to_bap(u_stream);

		if (u_stream->in_use == false || u_stream->cig_id != cig_id) {
			continue;
		}

		ep = btp_bap_unicast_end_point_find(u_conn, u_stream->ase_id);
		if (ep == NULL) {
			return -EINVAL;
		}

		stream_params->stream = stream;
		stream_params->qos = &cigs[u_stream->cig_id].qos[u_stream->cis_id];

		(void)bt_bap_ep_get_info(ep, &info);
		if (info.dir == BT_AUDIO_DIR_SOURCE) {
			if (pair_params[u_stream->cis_id].rx_param != NULL) {
				return -EINVAL;
			}

			pair_params[u_stream->cis_id].rx_param = stream_params;
		} else {
			if (pair_params[u_stream->cis_id].tx_param != NULL) {
				return -EINVAL;
			}

			pair_params[u_stream->cis_id].tx_param = stream_params;
		}

		stream_params++;
	}

	*stream_param_ptr = stream_params;

	return 0;
}

int btp_bap_unicast_group_create(uint8_t cig_id,
					   struct btp_bap_unicast_group **out_unicast_group)
{
	int err;
	struct bt_bap_unicast_group_stream_pair_param
		pair_params[BTP_BAP_UNICAST_MAX_STREAMS_COUNT];
	struct bt_bap_unicast_group_stream_param stream_params[BTP_BAP_UNICAST_MAX_STREAMS_COUNT];
	struct bt_bap_unicast_group_stream_param *stream_param_ptr;
	struct bt_bap_unicast_group_param param;
	size_t cis_cnt = 0;

	(void)memset(pair_params, 0, sizeof(pair_params));
	(void)memset(stream_params, 0, sizeof(stream_params));
	*out_unicast_group = NULL;

	if (cig_id >= CONFIG_BT_ISO_MAX_CIG) {
		return -EINVAL;
	}

	/* API does not allow to assign a CIG ID freely, so ensure we create groups
	 * in the right order.
	 */
	for (uint8_t i = 0; i < cig_id; i++) {
		if (cigs[i].in_use == false) {
			return -EINVAL;
		}
	}

	if (cigs[cig_id].in_use == true) {
		struct btp_bap_unicast_group *u_cig = &cigs[cig_id];

		err = bt_bap_unicast_group_delete(u_cig->cig);
		if (err != 0) {
			LOG_DBG("Failed to delete the unicast group, err %d", err);

			return BTP_STATUS_FAILED;
		}

		memset(u_cig, 0, sizeof(*u_cig));
	}

	stream_param_ptr = stream_params;
	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct btp_bap_unicast_connection *unicast_conn = &connections[i];

		if (unicast_conn->end_points_count == 0) {
			continue;
		}

		/* CISes have been assigned earlier to CIGs with client_add_ase_to_cis() */
		err = client_unicast_group_param_set(unicast_conn, cig_id, pair_params,
						     &stream_param_ptr);
		if (err != 0) {
			return err;
		}
	}

	/* Count CISes to be established */
	for (size_t count = ARRAY_SIZE(pair_params); count > 0; --count) {
		size_t i = count - 1;

		if (pair_params[i].tx_param != NULL || pair_params[i].rx_param != NULL) {
			cis_cnt++;

			continue;
		}

		if (cis_cnt > 0) {
			/* No gaps allowed */
			return -EINVAL;
		}
	}

	param.params = pair_params;
	param.params_count = cis_cnt;
	param.packing = BT_ISO_PACKING_SEQUENTIAL;

	LOG_DBG("Creating unicast group");
	err = bt_bap_unicast_group_create(&param, &cigs[cig_id].cig);
	if (err != 0) {
		LOG_DBG("Could not create unicast group (err %d)", err);
		return -EINVAL;
	}

	cigs[cig_id].in_use = true;
	cigs[cig_id].cig_id = cig_id;
	*out_unicast_group = &cigs[cig_id];

	return 0;
}

struct btp_bap_unicast_group *btp_bap_unicast_group_find(uint8_t cig_id)
{
	if (cig_id >= CONFIG_BT_ISO_MAX_CIG) {
		return NULL;
	}

	return &cigs[cig_id];
}

static int client_configure_codec(struct btp_bap_unicast_connection *u_conn, struct bt_conn *conn,
				  uint8_t ase_id, struct bt_audio_codec_cfg *codec_cfg)
{
	int err;
	struct bt_bap_ep *ep;
	struct btp_bap_unicast_stream *stream;

	stream = btp_bap_unicast_stream_find(u_conn, ase_id);
	if (stream == NULL) {
		/* Configure a new stream */
		stream = btp_bap_unicast_stream_alloc(u_conn);
		if (stream == NULL) {
			LOG_DBG("No streams available");

			return -ENOMEM;
		}

		if (u_conn->end_points_count == 0) {
			return -EINVAL;
		}

		ep = btp_bap_unicast_end_point_find(u_conn, ase_id);
		if (ep == NULL) {
			return -EINVAL;
		}

		memcpy(&stream->codec_cfg, codec_cfg, sizeof(*codec_cfg));
		err = bt_bap_stream_config(conn, stream_unicast_to_bap(stream), ep,
					   &stream->codec_cfg);
	} else {
		/* Reconfigure a stream */
		memcpy(&stream->codec_cfg, codec_cfg, sizeof(*codec_cfg));
		err = bt_bap_stream_reconfig(stream_unicast_to_bap(stream),
					     &stream->codec_cfg);
	}

	return err;
}

static int server_configure_codec(struct btp_bap_unicast_connection *u_conn, struct bt_conn *conn,
				  uint8_t ase_id, struct bt_audio_codec_cfg *codec_cfg)
{
	struct btp_bap_unicast_stream *stream;
	int err = 0;

	stream = btp_bap_unicast_stream_find(u_conn, ase_id);
	if (stream == NULL) {
		/* Zephyr allocates ASE instances for remote clients dynamically.
		 * To initiate Codec Config operation autonomously in server the role,
		 * we have to initialize all ASEs with a smaller ID first.
		 * Fortunately, the PTS has nothing against such behavior.
		 */
		for (uint8_t i = 1; i <= ase_id; i++) {
			stream = btp_bap_unicast_stream_find(u_conn, i);
			if (stream != NULL) {
				continue;
			}

			/* Configure a new stream */
			stream = btp_bap_unicast_stream_alloc(u_conn);
			if (stream == NULL) {
				LOG_DBG("No streams available");

				return -ENOMEM;
			}

			memcpy(&stream->codec_cfg, codec_cfg, sizeof(*codec_cfg));
			err = server_stream_config(conn, stream_unicast_to_bap(stream),
						   &stream->codec_cfg, &qos_pref);
		}
	} else {
		/* Reconfigure a stream */
		memcpy(&stream->codec_cfg, codec_cfg, sizeof(*codec_cfg));
		err = bt_bap_stream_reconfig(stream_unicast_to_bap(stream), &stream->codec_cfg);
	}

	if (err == 0) {
		btp_send_ascs_operation_completed_ev(conn, stream->ase_id, BT_ASCS_CONFIG_OP,
						     BT_BAP_ASCS_RSP_CODE_SUCCESS);
	}

	return err;
}

uint8_t btp_ascs_configure_codec(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_configure_codec_cmd *cp = cmd;
	struct bt_conn *conn;
	struct bt_conn_info conn_info;
	struct btp_bap_unicast_connection *u_conn;
	struct bt_audio_codec_cfg codec_cfg;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	u_conn = &connections[bt_conn_index(conn)];

	(void)bt_conn_get_info(conn, &conn_info);

	memset(&codec_cfg, 0, sizeof(codec_cfg));

	codec_cfg.target_latency = BT_AUDIO_CODEC_CFG_TARGET_LATENCY_BALANCED;
	codec_cfg.target_phy = BT_AUDIO_CODEC_CFG_TARGET_PHY_2M;
	codec_cfg.id = cp->coding_format;
	codec_cfg.vid = cp->vid;
	codec_cfg.cid = cp->cid;

	if (cp->cc_ltvs_len != 0) {
		codec_cfg.data_len = cp->cc_ltvs_len;
		memcpy(codec_cfg.data, cp->cc_ltvs, cp->cc_ltvs_len);
	}

	if (conn_info.role == BT_HCI_ROLE_CENTRAL) {
		err = client_configure_codec(u_conn, conn, cp->ase_id, &codec_cfg);
	} else {
		err = server_configure_codec(u_conn, conn, cp->ase_id, &codec_cfg);
	}

	bt_conn_unref(conn);

	if (err) {
		LOG_DBG("Failed to configure stream (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_ascs_preconfigure_qos(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_ascs_preconfigure_qos_cmd *cp = cmd;
	struct bt_bap_qos_cfg *qos;

	LOG_DBG("");

	qos = &cigs[cp->cig_id].qos[cp->cis_id];
	memset(qos, 0, sizeof(*qos));

	qos->phy = BT_BAP_QOS_CFG_2M;
	qos->framing = cp->framing;
	qos->rtn = cp->retransmission_num;
	qos->sdu = sys_le16_to_cpu(cp->max_sdu);
	qos->latency = sys_le16_to_cpu(cp->max_transport_latency);
	qos->interval = sys_get_le24(cp->sdu_interval);
	qos->pd = sys_get_le24(cp->presentation_delay);

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_ascs_configure_qos(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_configure_qos_cmd *cp = cmd;
	struct bt_conn_info conn_info;
	struct bt_conn *conn;
	struct btp_bap_unicast_group *out_unicast_group;

	LOG_DBG("");

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

	if (cigs[cp->cig_id].in_use == false) {
		err = btp_bap_unicast_group_create(cp->cig_id, &out_unicast_group);
		if (err != 0) {
			LOG_DBG("Unable to create unicast group, err %d", err);
			bt_conn_unref(conn);

			return BTP_STATUS_FAILED;
		}
	}

	LOG_DBG("QoS configuring streams");
	err = bt_bap_stream_qos(conn, cigs[cp->cig_id].cig);
	bt_conn_unref(conn);

	if (err != 0) {
		LOG_DBG("Unable to QoS configure streams: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_ascs_enable(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_enable_cmd *cp = cmd;
	struct btp_bap_unicast_connection *u_conn;
	struct btp_bap_unicast_stream *stream;
	struct bt_conn *conn;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	u_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	stream = btp_bap_unicast_stream_find(u_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Enabling stream");
	err = bt_bap_stream_enable(stream_unicast_to_bap(stream), NULL, 0);
	if (err != 0) {
		LOG_DBG("Could not enable stream: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_ascs_disable(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_disable_cmd *cp = cmd;
	struct btp_bap_unicast_connection *u_conn;
	struct btp_bap_unicast_stream *stream;
	struct bt_conn *conn;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	u_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	stream = btp_bap_unicast_stream_find(u_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Disabling stream");

	err = bt_bap_stream_disable(stream_unicast_to_bap(stream));
	if (err != 0) {
		LOG_DBG("Could not disable stream: %d", err);
		return BTP_STATUS_FAILED;
	}

	if (err == 0) {
		struct bt_conn_info conn_info;

		err = bt_conn_get_info(conn, &conn_info);
		if (err != 0) {
			LOG_ERR("Failed to get conn info: %d", err);
			return BTP_STATUS_FAILED;
		}

		if (conn_info.role == BT_HCI_ROLE_PERIPHERAL) {
			/* The server the operation completes immediately */
			btp_send_ascs_operation_completed_ev(conn, stream->ase_id,
							     BT_ASCS_DISABLE_OP,
							     BT_BAP_ASCS_RSP_CODE_SUCCESS);
		}
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_ascs_receiver_start_ready(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_receiver_start_ready_cmd *cp = cmd;
	struct btp_bap_unicast_connection *u_conn;
	struct btp_bap_unicast_stream *stream;
	struct bt_bap_stream *bap_stream;
	struct bt_conn_info conn_info;
	struct bt_bap_ep_info info;
	struct bt_conn *conn;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_conn_get_info(conn, &conn_info);
	if (err != 0) {
		LOG_ERR("Failed to get conn info: %d", err);
		return BTP_STATUS_FAILED;
	}

	if (conn_info.role == BT_HCI_ROLE_PERIPHERAL) {
		/* Cannot connect the CIS as the peripheral */
		LOG_DBG("Cannot connect the CIS as the peripheral");
		return BTP_STATUS_FAILED;
	}

	u_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	stream = btp_bap_unicast_stream_find(u_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	bap_stream = stream_unicast_to_bap(stream);
	(void)bt_bap_ep_get_info(bap_stream->ep, &info);
	if (info.state == BT_BAP_EP_STATE_STREAMING) {
		/* Already started */
		return BTP_STATUS_SUCCESS;
	}

	LOG_DBG("Starting stream %p, ep %u, dir %u", bap_stream, cp->ase_id, info.dir);

	/* TODO: This function should not do the BAP stream connect, and should instead just the
	 * operation that function is named after. Connecting the BAP stream should be its own BTP
	 * command
	 */
	while (true) {
		err = bt_bap_stream_connect(bap_stream);
		if (err == -EBUSY) {
			/* TODO: How to determine if a controller is ready again after
			 * bt_bap_stream_start? In AC 6(i) tests the PTS sends Receiver Start Ready
			 * only after all CISes are established.
			 */
			k_sleep(K_MSEC(1000));
			continue;
		} else if (err != 0 && err != -EALREADY) {
			LOG_DBG("Could not connect stream: %d", err);
			return BTP_STATUS_FAILED;
		}

		break;
	};

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_ascs_receiver_stop_ready(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_receiver_stop_ready_cmd *cp = cmd;
	struct btp_bap_unicast_connection *u_conn;
	struct btp_bap_unicast_stream *stream;
	struct bt_conn *conn;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	u_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	stream = btp_bap_unicast_stream_find(u_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Stopping stream");
	err = bt_bap_stream_stop(stream_unicast_to_bap(stream));
	if (err != 0) {
		LOG_DBG("Could not stop stream: %d", err);
		return BTP_STATUS_FAILED;
	}

	if (err == 0) {
		struct bt_conn_info conn_info;

		err = bt_conn_get_info(conn, &conn_info);
		if (err != 0) {
			LOG_ERR("Failed to get conn info: %d", err);
			return BTP_STATUS_FAILED;
		}

		if (conn_info.role == BT_HCI_ROLE_PERIPHERAL) {
			/* The server the operation completes immediately */
			btp_send_ascs_operation_completed_ev(conn, stream->ase_id, BT_ASCS_STOP_OP,
							     BT_BAP_ASCS_RSP_CODE_SUCCESS);
		}
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_ascs_release(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_ascs_release_cmd *cp = cmd;
	struct btp_bap_unicast_connection *u_conn;
	struct btp_bap_unicast_stream *stream;
	struct bt_conn *conn;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	u_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	stream = btp_bap_unicast_stream_find(u_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Releasing stream");
	err = bt_bap_stream_release(stream_unicast_to_bap(stream));
	if (err != 0) {
		LOG_DBG("Unable to release stream, err %d", err);
		return BTP_STATUS_FAILED;
	}

	if (err == 0) {
		struct bt_conn_info conn_info;

		err = bt_conn_get_info(conn, &conn_info);
		if (err != 0) {
			LOG_ERR("Failed to get conn info: %d", err);
			return BTP_STATUS_FAILED;
		}

		if (conn_info.role == BT_HCI_ROLE_PERIPHERAL) {
			/* The server the operation completes immediately */
			btp_send_ascs_operation_completed_ev(conn, stream->ase_id,
							     BT_ASCS_RELEASE_OP,
							     BT_BAP_ASCS_RSP_CODE_SUCCESS);
		}
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_ascs_update_metadata(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const uint8_t meta[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_ANY)),
	};
	const struct btp_ascs_update_metadata_cmd *cp = cmd;
	struct btp_bap_unicast_connection *u_conn;
	struct btp_bap_unicast_stream *stream;
	struct bt_conn *conn;
	int err;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	u_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	stream = btp_bap_unicast_stream_find(u_conn, cp->ase_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Updating stream metadata");
	err = bt_bap_stream_metadata(stream_unicast_to_bap(stream), meta, ARRAY_SIZE(meta));
	if (err != 0) {
		LOG_DBG("Failed to update stream metadata, err %d", err);
		return BTP_STATUS_FAILED;
	}

	if (err == 0) {
		struct bt_conn_info conn_info;

		err = bt_conn_get_info(conn, &conn_info);
		if (err != 0) {
			LOG_ERR("Failed to get conn info: %d", err);
			return BTP_STATUS_FAILED;
		}

		if (conn_info.role == BT_HCI_ROLE_PERIPHERAL) {
			/* The server the operation completes immediately */
			btp_send_ascs_operation_completed_ev(conn, stream->ase_id,
							     BT_ASCS_METADATA_OP,
							     BT_BAP_ASCS_RSP_CODE_SUCCESS);
		}
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_ascs_add_ase_to_cis(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_conn *conn;
	struct btp_bap_unicast_connection *u_conn;
	struct bt_conn_info conn_info;
	const struct btp_ascs_add_ase_to_cis *cp = cmd;

	LOG_DBG("");

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

	u_conn = &connections[bt_conn_index(conn)];
	bt_conn_unref(conn);

	err = client_add_ase_to_cis(u_conn, cp->ase_id, cp->cis_id, cp->cig_id);

	return BTP_STATUS_VAL(err);
}

struct btp_bap_unicast_connection *btp_bap_unicast_conn_get(size_t conn_index)
{
	return &connections[conn_index];
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct btp_bap_unicast_connection *u_conn;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		LOG_DBG("Failed to connect to %s (%u)", addr, err);
		return;
	}

	LOG_DBG("Connected: %s", addr);

	u_conn = &connections[bt_conn_index(conn)];
	memset(u_conn, 0, sizeof(*u_conn));
	bt_addr_le_copy(&u_conn->address, bt_conn_get_dst(conn));
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

static bool unicast_inited;

int btp_bap_unicast_init(void)
{
	int err;

	if (unicast_inited) {
		return 0;
	}

	(void)memset(connections, 0, sizeof(connections));

	err = bt_bap_unicast_server_register(&param);
	if (err != 0) {
		LOG_DBG("Failed to register unicast server (err %d)\n", err);

		return err;
	}

	err = bt_bap_unicast_server_register_cb(&unicast_server_cb);
	if (err != 0) {
		LOG_DBG("Failed to register client callbacks: %d", err);
		return err;
	}

	err = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
	if (err != 0) {
		LOG_DBG("Failed to register client callbacks: %d", err);
		return err;
	}

	bt_conn_cb_register(&conn_callbacks);

	unicast_inited = true;

	return 0;
}
