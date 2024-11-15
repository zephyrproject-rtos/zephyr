/* btp_cap.c - Bluetooth CAP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/audio/cap.h>

#include "btp/btp.h"
#include "btp_bap_audio_stream.h"
#include "bap_endpoint.h"
#include "zephyr/sys/byteorder.h"
#include <stdint.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_cap
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp_bap_unicast.h"
#include "btp_bap_broadcast.h"

static struct btp_bap_unicast_group *u_group;

extern struct bt_csip_set_coordinator_set_member *btp_csip_set_members[CONFIG_BT_MAX_CONN];

static struct bt_bap_stream *stream_unicast_to_bap(struct btp_bap_unicast_stream *stream)
{
	return &stream->audio_stream.cap_stream.bap_stream;
}

static struct bt_cap_stream *stream_unicast_to_cap(struct btp_bap_unicast_stream *stream)
{
	return &stream->audio_stream.cap_stream;
}

static struct bt_cap_stream *stream_broadcast_to_cap(struct btp_bap_broadcast_stream *stream)
{
	return &stream->audio_stream.cap_stream;
}

static void btp_send_discovery_completed_ev(struct bt_conn *conn, uint8_t status)
{
	struct btp_cap_discovery_completed_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));
	ev.status = status;

	tester_event(BTP_SERVICE_ID_CAP, BTP_CAP_EV_DISCOVERY_COMPLETED, &ev, sizeof(ev));
}

static void cap_discovery_complete_cb(struct bt_conn *conn, int err,
				      const struct bt_csip_set_coordinator_set_member *member,
				      const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	LOG_DBG("");

	if (err != 0) {
		LOG_DBG("Failed to discover CAS: %d", err);
		btp_send_discovery_completed_ev(conn, BTP_CAP_DISCOVERY_STATUS_FAILED);

		return;
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)) {
		if (csis_inst == NULL) {
			LOG_DBG("Failed to discover CAS CSIS");
			btp_send_discovery_completed_ev(conn, BTP_CAP_DISCOVERY_STATUS_FAILED);

			return;
		}

		LOG_DBG("Found CAS with CSIS %p", csis_inst);
	} else {
		LOG_DBG("Found CAS");
	}

	btp_send_discovery_completed_ev(conn, BTP_CAP_DISCOVERY_STATUS_SUCCESS);
}

static void btp_send_cap_unicast_start_completed_ev(uint8_t cig_id, uint8_t status)
{
	struct btp_cap_unicast_start_completed_ev ev;

	ev.cig_id = cig_id;
	ev.status = status;

	tester_event(BTP_SERVICE_ID_CAP, BTP_CAP_EV_UNICAST_START_COMPLETED, &ev, sizeof(ev));
}

static void btp_send_cap_unicast_stop_completed_ev(uint8_t cig_id, uint8_t status)
{
	struct btp_cap_unicast_stop_completed_ev ev;

	ev.cig_id = cig_id;
	ev.status = status;

	tester_event(BTP_SERVICE_ID_CAP, BTP_CAP_EV_UNICAST_STOP_COMPLETED, &ev, sizeof(ev));
}

static void unicast_start_complete_cb(int err, struct bt_conn *conn)
{
	LOG_DBG("");

	if (err != 0) {
		LOG_DBG("Failed to unicast-start, err %d", err);
		btp_send_cap_unicast_start_completed_ev(u_group->cig_id,
							BTP_CAP_UNICAST_START_STATUS_FAILED);

		return;
	}

	btp_send_cap_unicast_start_completed_ev(u_group->cig_id,
						BTP_CAP_UNICAST_START_STATUS_SUCCESS);
}

static void unicast_update_complete_cb(int err, struct bt_conn *conn)
{
	LOG_DBG("");

	if (err != 0) {
		LOG_DBG("Failed to unicast-update, err %d", err);
	}
}

static void unicast_stop_complete_cb(int err, struct bt_conn *conn)
{
	LOG_DBG("");

	if (err != 0) {
		LOG_DBG("Failed to unicast-stop, err %d", err);
		btp_send_cap_unicast_stop_completed_ev(u_group->cig_id,
						       BTP_CAP_UNICAST_START_STATUS_FAILED);

		return;
	}

	btp_send_cap_unicast_stop_completed_ev(u_group->cig_id,
					       BTP_CAP_UNICAST_START_STATUS_SUCCESS);
}

static struct bt_cap_initiator_cb cap_cb = {
	.unicast_discovery_complete = cap_discovery_complete_cb,
	.unicast_start_complete = unicast_start_complete_cb,
	.unicast_update_complete = unicast_update_complete_cb,
	.unicast_stop_complete = unicast_stop_complete_cb,
};

static uint8_t btp_cap_supported_commands(const void *cmd, uint16_t cmd_len,
					  void *rsp, uint16_t *rsp_len)
{
	struct btp_cap_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_CAP_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_CAP_DISCOVER);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_cap_discover(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_cap_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_cap_initiator_unicast_discover(conn);
	if (err != 0) {
		LOG_DBG("Failed to discover remote ASEs: %d", err);
		bt_conn_unref(conn);

		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);

	return BTP_STATUS_SUCCESS;
}

static int cap_unicast_setup_ase(struct bt_conn *conn, uint8_t ase_id, uint8_t cis_id,
				 uint8_t cig_id, struct bt_audio_codec_cfg *codec_cfg,
				 struct bt_bap_qos_cfg *qos)
{
	struct btp_bap_unicast_group *group;
	struct btp_bap_unicast_stream *u_stream;
	struct bt_bap_stream *stream;
	struct btp_bap_unicast_connection *u_conn = btp_bap_unicast_conn_get(bt_conn_index(conn));

	u_stream = btp_bap_unicast_stream_find(u_conn, ase_id);
	if (u_stream == NULL) {
		/* Configure a new u_stream */
		u_stream = btp_bap_unicast_stream_alloc(u_conn);
		if (u_stream == NULL) {
			LOG_DBG("No streams available");

			return -ENOMEM;
		}
	}

	stream = stream_unicast_to_bap(u_stream);
	bt_cap_stream_ops_register(&u_stream->audio_stream.cap_stream, stream->ops);

	u_stream->conn_id = bt_conn_index(conn);
	u_stream->ase_id = ase_id;
	u_stream->cig_id = cig_id;
	u_stream->cis_id = cis_id;
	memcpy(&u_stream->codec_cfg, codec_cfg, sizeof(*codec_cfg));

	group = btp_bap_unicast_group_find(cig_id);
	if (group == NULL) {
		return -EINVAL;
	}

	memcpy(&group->qos[cis_id], qos, sizeof(*qos));

	return 0;
}

static uint8_t btp_cap_unicast_setup_ase(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len)
{
	const struct btp_cap_unicast_setup_ase_cmd *cp = cmd;
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_bap_qos_cfg qos;
	struct bt_conn *conn;
	const uint8_t *ltv_ptr;
	int err;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");

		return BTP_STATUS_FAILED;
	}

	memset(&qos, 0, sizeof(qos));
	qos.phy = BT_BAP_QOS_CFG_2M;
	qos.framing = cp->framing;
	qos.rtn = cp->retransmission_num;
	qos.sdu = sys_le16_to_cpu(cp->max_sdu);
	qos.latency = sys_le16_to_cpu(cp->max_transport_latency);
	qos.interval = sys_get_le24(cp->sdu_interval);
	qos.pd = sys_get_le24(cp->presentation_delay);

	memset(&codec_cfg, 0, sizeof(codec_cfg));
	codec_cfg.id = cp->coding_format;
	codec_cfg.vid = cp->vid;
	codec_cfg.cid = cp->cid;

	ltv_ptr = cp->ltvs;
	if (cp->cc_ltvs_len != 0) {
		codec_cfg.data_len = cp->cc_ltvs_len;
		memcpy(codec_cfg.data, ltv_ptr, cp->cc_ltvs_len);
		ltv_ptr += cp->cc_ltvs_len;
	}

	if (cp->metadata_ltvs_len != 0) {
		codec_cfg.meta_len = cp->metadata_ltvs_len;
		memcpy(codec_cfg.meta, ltv_ptr, cp->metadata_ltvs_len);
	}

	err = cap_unicast_setup_ase(conn, cp->ase_id, cp->cis_id, cp->cig_id, &codec_cfg, &qos);
	bt_conn_unref(conn);

	return BTP_STATUS_VAL(err);
}

static uint8_t btp_cap_unicast_audio_start(const void *cmd, uint16_t cmd_len,
					   void *rsp, uint16_t *rsp_len)
{
	int err;
	size_t stream_count = 0;
	const struct btp_cap_unicast_audio_start_cmd *cp = cmd;
	struct bt_cap_unicast_audio_start_param start_param;
	struct bt_cap_unicast_audio_start_stream_param stream_params[
		ARRAY_SIZE(btp_csip_set_members) * BTP_BAP_UNICAST_MAX_STREAMS_COUNT];

	LOG_DBG("");

	err = btp_bap_unicast_group_create(cp->cig_id, &u_group);
	if (err != 0) {
		LOG_ERR("Failed to create unicast group");

		return BTP_STATUS_FAILED;
	}

	for (size_t conn_index = 0; conn_index < ARRAY_SIZE(btp_csip_set_members); conn_index++) {
		struct btp_bap_unicast_connection *u_conn = btp_bap_unicast_conn_get(conn_index);

		if (u_conn->end_points_count == 0) {
			/* Connection not initialized */
			continue;
		}

		for (size_t i = 0; i < ARRAY_SIZE(u_conn->streams); i++) {
			struct bt_cap_unicast_audio_start_stream_param *stream_param;
			struct btp_bap_unicast_stream *u_stream = &u_conn->streams[i];

			if (!u_stream->in_use || u_stream->cig_id != cp->cig_id) {
				continue;
			}

			stream_param = &stream_params[stream_count++];
			stream_param->stream = stream_unicast_to_cap(u_stream);
			stream_param->codec_cfg = &u_stream->codec_cfg;
			stream_param->member.member = bt_conn_lookup_addr_le(BT_ID_DEFAULT,
									     &u_conn->address);
			stream_param->ep = btp_bap_unicast_end_point_find(u_conn, u_stream->ase_id);
		}
	}

	start_param.type = cp->set_type;
	start_param.count = stream_count;
	start_param.stream_params = stream_params;

	err = bt_cap_initiator_unicast_audio_start(&start_param);
	if (err != 0) {
		LOG_ERR("Failed to start unicast audio: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_cap_unicast_audio_update(const void *cmd, uint16_t cmd_len,
					    void *rsp, uint16_t *rsp_len)
{
	int err;
	const uint8_t *data_ptr;
	const struct btp_cap_unicast_audio_update_cmd *cp = cmd;
	struct bt_cap_unicast_audio_update_stream_param
		stream_params[ARRAY_SIZE(btp_csip_set_members) * BTP_BAP_UNICAST_MAX_STREAMS_COUNT];
	struct bt_cap_unicast_audio_update_param param = {0};

	LOG_DBG("");

	if (cp->stream_count == 0) {
		return BTP_STATUS_FAILED;
	}

	data_ptr = cp->update_data;
	for (size_t i = 0; i < cp->stream_count; i++) {
		struct btp_bap_unicast_connection *u_conn;
		struct btp_bap_unicast_stream *u_stream;
		struct bt_conn *conn;
		struct btp_cap_unicast_audio_update_data *update_data =
			(struct btp_cap_unicast_audio_update_data *)data_ptr;
		struct bt_cap_unicast_audio_update_stream_param *stream_param = &stream_params[i];

		conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &update_data->address);
		if (!conn) {
			LOG_ERR("Unknown connection");

			return BTP_STATUS_FAILED;
		}

		u_conn = btp_bap_unicast_conn_get(bt_conn_index(conn));
		bt_conn_unref(conn);
		if (u_conn->end_points_count == 0) {
			/* Connection not initialized */

			return BTP_STATUS_FAILED;
		}

		u_stream = btp_bap_unicast_stream_find(u_conn, update_data->ase_id);
		if (u_stream == NULL) {
			return BTP_STATUS_FAILED;
		}

		stream_param->stream = &u_stream->audio_stream.cap_stream;
		stream_param->meta_len = update_data->metadata_ltvs_len;
		stream_param->meta = update_data->metadata_ltvs;

		data_ptr = ((uint8_t *)update_data) + stream_param->meta_len +
			   sizeof(struct btp_cap_unicast_audio_update_data);
	}

	param.count = cp->stream_count;
	param.stream_params = stream_params;
	param.type = BT_CAP_SET_TYPE_AD_HOC;

	err = bt_cap_initiator_unicast_audio_update(&param);
	if (err != 0) {
		LOG_ERR("Failed to start unicast audio: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_cap_unicast_audio_stop(const void *cmd, uint16_t cmd_len,
					  void *rsp, uint16_t *rsp_len)
{
	struct bt_cap_stream
		*streams[ARRAY_SIZE(btp_csip_set_members) * BTP_BAP_UNICAST_MAX_STREAMS_COUNT];
	struct bt_cap_unicast_audio_stop_param param = {0};
	int err;
	const struct btp_cap_unicast_audio_stop_cmd *cp = cmd;
	size_t stream_cnt = 0U;

	LOG_DBG("");

	/* Get generate the same stream list as used by btp_cap_unicast_audio_start */
	for (size_t conn_index = 0; conn_index < ARRAY_SIZE(btp_csip_set_members); conn_index++) {
		struct btp_bap_unicast_connection *u_conn = btp_bap_unicast_conn_get(conn_index);

		if (u_conn->end_points_count == 0) {
			/* Connection not initialized */
			continue;
		}

		for (size_t i = 0; i < ARRAY_SIZE(u_conn->streams); i++) {
			struct btp_bap_unicast_stream *u_stream = &u_conn->streams[i];

			if (!u_stream->in_use || u_stream->cig_id != cp->cig_id) {
				continue;
			}

			streams[stream_cnt++] = stream_unicast_to_cap(u_stream);
		}
	}

	param.streams = streams;
	param.count = stream_cnt;
	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.release = true;

	err = bt_cap_initiator_unicast_audio_stop(&param);
	if (err != 0) {
		LOG_ERR("Failed to start unicast audio: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static struct bt_cap_initiator_broadcast_subgroup_param
	cap_subgroup_params[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
static struct bt_cap_initiator_broadcast_stream_param
	cap_stream_params[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT]
		     [CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];

static uint8_t btp_cap_broadcast_source_setup_stream(const void *cmd, uint16_t cmd_len,
						     void *rsp, uint16_t *rsp_len)
{
	const uint8_t *ltv_ptr;
	struct btp_bap_broadcast_stream *stream;
	const struct btp_cap_broadcast_source_setup_stream_cmd *cp = cmd;
	struct btp_bap_broadcast_local_source *source =
		btp_bap_broadcast_local_source_get(cp->source_id);
	struct bt_audio_codec_cfg *codec_cfg;

	stream = btp_bap_broadcast_stream_alloc(source);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	stream->subgroup_id = cp->subgroup_id;
	codec_cfg = &stream->codec_cfg;
	memset(codec_cfg, 0, sizeof(*codec_cfg));
	codec_cfg->id = cp->coding_format;
	codec_cfg->vid = cp->vid;
	codec_cfg->cid = cp->cid;

	ltv_ptr = cp->ltvs;
	if (cp->cc_ltvs_len != 0) {
		codec_cfg->data_len = cp->cc_ltvs_len;
		memcpy(codec_cfg->data, ltv_ptr, cp->cc_ltvs_len);
		ltv_ptr += cp->cc_ltvs_len;
	}

	if (cp->metadata_ltvs_len != 0) {
		codec_cfg->meta_len = cp->metadata_ltvs_len;
		memcpy(codec_cfg->meta, ltv_ptr, cp->metadata_ltvs_len);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_cap_broadcast_source_setup_subgroup(const void *cmd, uint16_t cmd_len,
						       void *rsp, uint16_t *rsp_len)
{
	const uint8_t *ltv_ptr;
	struct bt_audio_codec_cfg *codec_cfg;
	const struct btp_cap_broadcast_source_setup_subgroup_cmd *cp = cmd;
	struct btp_bap_broadcast_local_source *source =
		btp_bap_broadcast_local_source_get(cp->source_id);

	if (cp->subgroup_id >= sizeof(cap_subgroup_params)) {
		return BTP_STATUS_FAILED;
	}

	cap_subgroup_params[cp->subgroup_id].codec_cfg =
		&source->subgroup_codec_cfg[cp->subgroup_id];
	codec_cfg = cap_subgroup_params[cp->subgroup_id].codec_cfg;
	memset(codec_cfg, 0, sizeof(*codec_cfg));
	codec_cfg->id = cp->coding_format;
	codec_cfg->vid = cp->vid;
	codec_cfg->cid = cp->cid;

	ltv_ptr = cp->ltvs;
	if (cp->cc_ltvs_len != 0) {
		codec_cfg->data_len = cp->cc_ltvs_len;
		memcpy(codec_cfg->data, ltv_ptr, cp->cc_ltvs_len);
		ltv_ptr += cp->cc_ltvs_len;
	}

	if (cp->metadata_ltvs_len != 0) {
		codec_cfg->meta_len = cp->metadata_ltvs_len;
		memcpy(codec_cfg->meta, ltv_ptr, cp->metadata_ltvs_len);
	}

	return BTP_STATUS_SUCCESS;
}

static int cap_broadcast_source_adv_setup(struct btp_bap_broadcast_local_source *source,
					  uint32_t *gap_settings)
{
	int err;
	struct bt_le_adv_param param = *BT_LE_EXT_ADV_NCONN;
	uint32_t broadcast_id;

	NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);

	/* Broadcast Audio Streaming Endpoint advertising data */
	struct bt_data base_ad[2];
	struct bt_data per_ad;

	err = bt_rand(&broadcast_id, BT_AUDIO_BROADCAST_ID_SIZE);
	if (err) {
		printk("Unable to generate broadcast ID: %d\n", err);

		return -EINVAL;
	}

	*gap_settings = BIT(BTP_GAP_SETTINGS_DISCOVERABLE) |
			BIT(BTP_GAP_SETTINGS_EXTENDED_ADVERTISING);
	/* Setup extended advertising data */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, source->broadcast_id);
	base_ad[0].type = BT_DATA_SVC_DATA16;
	base_ad[0].data_len = ad_buf.len;
	base_ad[0].data = ad_buf.data;
	base_ad[1].type = BT_DATA_NAME_COMPLETE;
	base_ad[1].data_len = sizeof(CONFIG_BT_DEVICE_NAME) - 1;
	base_ad[1].data = CONFIG_BT_DEVICE_NAME;
	err = tester_gap_create_adv_instance(&param, BTP_GAP_ADDR_TYPE_IDENTITY, base_ad, 2, NULL,
					     0, gap_settings);
	if (err != 0) {
		LOG_DBG("Failed to create extended advertising instance: %d", err);

		return -EINVAL;
	}

	err = tester_gap_padv_configure(BT_LE_PER_ADV_PARAM(BT_GAP_PER_ADV_FAST_INT_MIN_2,
							    BT_GAP_PER_ADV_FAST_INT_MAX_2,
							    BT_LE_PER_ADV_OPT_NONE));
	if (err != 0) {
		LOG_DBG("Failed to configure periodic advertising: %d", err);

		return -EINVAL;
	}

	err = bt_cap_initiator_broadcast_get_base(source->cap_broadcast, &base_buf);
	if (err != 0) {
		LOG_DBG("Failed to get encoded BASE: %d\n", err);

		return -EINVAL;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = tester_gap_padv_set_data(&per_ad, 1);
	if (err != 0) {
		return -EINVAL;
	}

	return 0;
}

static uint8_t btp_cap_broadcast_source_setup(const void *cmd, uint16_t cmd_len,
					      void *rsp, uint16_t *rsp_len)
{
	int err;
	uint32_t gap_settings;
	static struct bt_cap_initiator_broadcast_create_param create_param;
	const struct btp_cap_broadcast_source_setup_cmd *cp = cmd;
	struct btp_cap_broadcast_source_setup_rp *rp = rsp;
	struct btp_bap_broadcast_local_source *source =
		btp_bap_broadcast_local_source_get(cp->source_id);
	struct bt_bap_qos_cfg *qos = &source->qos;

	LOG_DBG("");

	memset(&create_param, 0, sizeof(create_param));

	for (size_t i = 0; i < ARRAY_SIZE(source->streams); i++) {
		struct btp_bap_broadcast_stream *stream = &source->streams[i];
		struct bt_cap_initiator_broadcast_stream_param *stream_param;
		struct bt_cap_initiator_broadcast_subgroup_param *subgroup_param;
		uint8_t bis_id;

		if (!stream->in_use) {
			/* No more streams set up */
			break;
		}

		subgroup_param = &cap_subgroup_params[stream->subgroup_id];
		bis_id = subgroup_param->stream_count++;
		stream_param = &cap_stream_params[stream->subgroup_id][bis_id];
		stream_param->stream = stream_broadcast_to_cap(stream);

		if (cp->flags & BTP_CAP_BROADCAST_SOURCE_SETUP_FLAG_SUBGROUP_CODEC) {
			stream_param->data_len = 0;
			stream_param->data = NULL;
		} else {
			stream_param->data_len = stream->codec_cfg.data_len;
			stream_param->data = stream->codec_cfg.data;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(cap_subgroup_params); i++) {
		if (cap_subgroup_params[i].stream_count == 0) {
			/* No gaps allowed */
			break;
		}

		cap_subgroup_params[i].stream_params = cap_stream_params[i];
		create_param.subgroup_count++;
	}

	if (create_param.subgroup_count == 0) {
		return BTP_STATUS_FAILED;
	}

	memset(qos, 0, sizeof(*qos));
	qos->phy = BT_BAP_QOS_CFG_2M;
	qos->framing = cp->framing;
	qos->rtn = cp->retransmission_num;
	qos->sdu = sys_le16_to_cpu(cp->max_sdu);
	qos->latency = sys_le16_to_cpu(cp->max_transport_latency);
	qos->interval = sys_get_le24(cp->sdu_interval);
	qos->pd = sys_get_le24(cp->presentation_delay);

	create_param.subgroup_params = cap_subgroup_params;
	create_param.qos = qos;
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	create_param.encryption = cp->flags & BTP_CAP_BROADCAST_SOURCE_SETUP_FLAG_ENCRYPTION;
	memcpy(create_param.broadcast_code, cp->broadcast_code, sizeof(cp->broadcast_code));

	err = bt_cap_initiator_broadcast_audio_create(&create_param, &source->cap_broadcast);
	memset(cap_subgroup_params, 0, sizeof(cap_subgroup_params));
	memset(&create_param, 0, sizeof(create_param));
	if (err != 0) {
		LOG_ERR("Failed to create audio source: %d", err);

		return BTP_STATUS_FAILED;
	}

	err = cap_broadcast_source_adv_setup(source, &gap_settings);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	rp->gap_settings = gap_settings;
	sys_put_le24(source->broadcast_id, rp->broadcast_id);
	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_cap_broadcast_source_release(const void *cmd, uint16_t cmd_len,
						void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_cap_broadcast_source_release_cmd *cp = cmd;
	struct btp_bap_broadcast_local_source *source =
		btp_bap_broadcast_local_source_get(cp->source_id);

	LOG_DBG("");

	err = bt_cap_initiator_broadcast_audio_delete(source->cap_broadcast);
	if (err != 0) {
		LOG_DBG("Unable to delete broadcast source: %d", err);

		return BTP_STATUS_FAILED;
	}

	memset(source, 0, sizeof(*source));

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_cap_broadcast_adv_start(const void *cmd, uint16_t cmd_len,
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

static uint8_t btp_cap_broadcast_adv_stop(const void *cmd, uint16_t cmd_len,
					  void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = tester_gap_padv_stop();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	err = tester_gap_stop_ext_adv();

	return BTP_STATUS_VAL(err);
}

static uint8_t btp_cap_broadcast_source_start(const void *cmd, uint16_t cmd_len,
					      void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_cap_broadcast_source_start_cmd *cp = cmd;
	struct btp_bap_broadcast_local_source *source =
		btp_bap_broadcast_local_source_get(cp->source_id);
	struct bt_le_ext_adv *ext_adv = tester_gap_ext_adv_get();

	LOG_DBG("");

	if (ext_adv == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_cap_initiator_broadcast_audio_start(source->cap_broadcast, ext_adv);
	if (err != 0) {
		LOG_ERR("Failed to start audio source: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_cap_broadcast_source_stop(const void *cmd, uint16_t cmd_len,
					     void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_cap_broadcast_source_stop_cmd *cp = cmd;
	struct btp_bap_broadcast_local_source *source =
		btp_bap_broadcast_local_source_get(cp->source_id);

	err = bt_cap_initiator_broadcast_audio_stop(source->cap_broadcast);
	if (err != 0) {
		LOG_ERR("Failed to stop audio source: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_cap_broadcast_source_update(const void *cmd, uint16_t cmd_len,
					       void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_data per_ad;
	const struct btp_cap_broadcast_source_update_cmd *cp = cmd;
	struct btp_bap_broadcast_local_source *source =
		btp_bap_broadcast_local_source_get(cp->source_id);
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);

	LOG_DBG("");

	if (cp->metadata_ltvs_len == 0) {
		return BTP_STATUS_FAILED;
	}

	err = bt_cap_initiator_broadcast_audio_update(source->cap_broadcast, cp->metadata_ltvs,
						      cp->metadata_ltvs_len);
	if (err != 0) {
		LOG_ERR("Failed to update audio source: %d", err);

		return BTP_STATUS_FAILED;
	}

	err = bt_cap_initiator_broadcast_get_base(source->cap_broadcast, &base_buf);
	if (err != 0) {
		LOG_DBG("Failed to get encoded BASE: %d\n", err);

		return -EINVAL;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = tester_gap_padv_set_data(&per_ad, 1);
	if (err != 0) {
		return -EINVAL;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler cap_handlers[] = {
	{
		.opcode = BTP_CAP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = btp_cap_supported_commands
	},
	{
		.opcode = BTP_CAP_DISCOVER,
		.expect_len = sizeof(struct btp_cap_discover_cmd),
		.func = btp_cap_discover
	},
	{
		.opcode = BTP_CAP_UNICAST_SETUP_ASE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = btp_cap_unicast_setup_ase
	},
	{
		.opcode = BTP_CAP_UNICAST_AUDIO_START,
		.expect_len = sizeof(struct btp_cap_unicast_audio_start_cmd),
		.func = btp_cap_unicast_audio_start
	},
	{
		.opcode = BTP_CAP_UNICAST_AUDIO_UPDATE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = btp_cap_unicast_audio_update
	},
	{
		.opcode = BTP_CAP_UNICAST_AUDIO_STOP,
		.expect_len = sizeof(struct btp_cap_unicast_audio_stop_cmd),
		.func = btp_cap_unicast_audio_stop
	},
	{
		.opcode = BTP_CAP_BROADCAST_SOURCE_SETUP_STREAM,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = btp_cap_broadcast_source_setup_stream
	},
	{
		.opcode = BTP_CAP_BROADCAST_SOURCE_SETUP_SUBGROUP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = btp_cap_broadcast_source_setup_subgroup
	},
	{
		.opcode = BTP_CAP_BROADCAST_SOURCE_SETUP,
		.expect_len = sizeof(struct btp_cap_broadcast_source_setup_cmd),
		.func = btp_cap_broadcast_source_setup
	},
	{
		.opcode = BTP_CAP_BROADCAST_SOURCE_RELEASE,
		.expect_len = sizeof(struct btp_cap_broadcast_source_release_cmd),
		.func = btp_cap_broadcast_source_release
	},
	{
		.opcode = BTP_CAP_BROADCAST_ADV_START,
		.expect_len = sizeof(struct btp_cap_broadcast_adv_start_cmd),
		.func = btp_cap_broadcast_adv_start
	},
	{
		.opcode = BTP_CAP_BROADCAST_ADV_STOP,
		.expect_len = sizeof(struct btp_cap_broadcast_adv_stop_cmd),
		.func = btp_cap_broadcast_adv_stop
	},
	{
		.opcode = BTP_CAP_BROADCAST_SOURCE_START,
		.expect_len = sizeof(struct btp_cap_broadcast_source_start_cmd),
		.func = btp_cap_broadcast_source_start
	},
	{
		.opcode = BTP_CAP_BROADCAST_SOURCE_STOP,
		.expect_len = sizeof(struct btp_cap_broadcast_source_stop_cmd),
		.func = btp_cap_broadcast_source_stop
	},
	{
		.opcode = BTP_CAP_BROADCAST_SOURCE_UPDATE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = btp_cap_broadcast_source_update
	},
};

uint8_t tester_init_cap(void)
{
	int err;

	err = bt_cap_initiator_register_cb(&cap_cb);
	if (err != 0) {
		LOG_DBG("Failed to register CAP callbacks (err %d)", err);
		return err;
	}

	tester_register_command_handlers(BTP_SERVICE_ID_CAP, cap_handlers,
					 ARRAY_SIZE(cap_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_cap(void)
{
	return BTP_STATUS_SUCCESS;
}
