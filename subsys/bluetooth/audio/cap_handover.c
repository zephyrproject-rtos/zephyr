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
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#include "cap_internal.h"
#include "csip_internal.h"

LOG_MODULE_REGISTER(bt_cap_handover, CONFIG_BT_CAP_HANDOVER_LOG_LEVEL);

static const struct bt_cap_handover_cb *cap_cb;

bool bt_cap_handover_is_handover_broadcast_source(
	const struct bt_cap_broadcast_source *cap_broadcast_source)
{
	const struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	const struct bt_cap_handover_proc_param *proc_param = &active_proc->proc_param.handover;

	return cap_broadcast_source == proc_param->unicast_to_broadcast.broadcast_source;
}

struct cap_unicast_group_stream_lookup {
	struct bt_cap_stream *active_sink_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	struct bt_cap_stream *streams[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	size_t active_sink_streams_cnt;
	size_t cnt;
};

static bool unicast_group_foreach_stream_cb(struct bt_cap_stream *cap_stream, void *user_data)
{
	struct cap_unicast_group_stream_lookup *data = user_data;
	const struct bt_bap_stream *bap_stream = &cap_stream->bap_stream;

	__ASSERT_NO_MSG(data->cnt < ARRAY_SIZE(data->streams));
	__ASSERT_NO_MSG(data->active_sink_streams_cnt < ARRAY_SIZE(data->active_sink_streams));

	if (bap_stream->ep != NULL) {
		struct bt_bap_ep_info ep_info;
		int err;

		err = bt_bap_ep_get_info(bap_stream->ep, &ep_info);
		__ASSERT_NO_MSG(err == 0);

		/* Only consider sink streams for handover to broadcast */
		if (ep_info.state == BT_BAP_EP_STATE_STREAMING &&
		    ep_info.dir == BT_AUDIO_DIR_SINK) {
			data->active_sink_streams[data->active_sink_streams_cnt++] = cap_stream;
		}
	}

	data->streams[data->cnt++] = cap_stream;

	return false;
}

void bt_cap_handover_proc_complete(void)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	struct bt_cap_handover_proc_param *proc_param = &active_proc->proc_param.handover;
	struct bt_cap_broadcast_source *broadcast_source =
		proc_param->unicast_to_broadcast.broadcast_source;
	struct bt_cap_unicast_group *unicast_group = proc_param->unicast_to_broadcast.unicast_group;
	struct bt_conn *failed_conn = active_proc->failed_conn;
	const int err = active_proc->err;

	bt_cap_common_clear_active_proc();

	if (cap_cb != NULL && cap_cb->unicast_to_broadcast_complete != NULL) {
		cap_cb->unicast_to_broadcast_complete(err, failed_conn, unicast_group,
						      broadcast_source);
	}
}

void bt_cap_handover_unicast_audio_stopped(void)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();

	__ASSERT_NO_MSG(active_proc->proc_type == BT_CAP_COMMON_PROC_TYPE_STOP);

	if (active_proc->err != 0) {
		bt_cap_handover_proc_complete();
	} else {
		/* continue */
		bt_cap_handover_unicast_to_broadcast_setup_broadcast();
	}
}

void bt_cap_handover_broadcast_source_stopped(uint8_t reason)
{
	if (reason == BT_HCI_ERR_LOCALHOST_TERM_CONN) {
		/* TODO: This will be the case when we do broadcast to unicast handover and
		 * it successfully stops the broadcast
		 */
	} else {
		struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
		struct bt_cap_handover_proc_param *proc_param = &active_proc->proc_param.handover;
		int err;

		err = bt_cap_initiator_broadcast_audio_delete(
			proc_param->unicast_to_broadcast.broadcast_source);
		__ASSERT_NO_MSG(err == 0);
		proc_param->unicast_to_broadcast.broadcast_source = NULL;

		active_proc->err = reason;
		active_proc->failed_conn = NULL;
		bt_cap_handover_proc_complete();
	}
}

void bt_cap_handover_unicast_to_broadcast_reception_start(void)
{
	struct bt_cap_commander_broadcast_reception_start_param param = {0};
	struct bt_cap_initiator_broadcast_create_param *create_param;
	struct bt_cap_handover_proc_param *proc_param;
	struct bt_cap_common_proc *active_proc;
	struct bt_le_ext_adv_info adv_info;
	int err;

	active_proc = bt_cap_common_get_active_proc();
	proc_param = &active_proc->proc_param.handover;
	create_param = proc_param->unicast_to_broadcast.broadcast_create_param;
	param.type = proc_param->unicast_to_broadcast.type;
	param.param = proc_param->unicast_to_broadcast.reception_start_member_params;

	err = bt_le_ext_adv_get_info(proc_param->unicast_to_broadcast.ext_adv, &adv_info);
	__ASSERT_NO_MSG(err != -EINVAL);
	if (err != 0) {
		/* May happen if the advertising set was deleted while in this procedure */
		LOG_DBG("Failed to get adv info: %d", err);
		active_proc->err = err;
		active_proc->failed_conn = NULL;

		bt_cap_handover_proc_complete();
		return;
	}

	if (adv_info.ext_adv_state != BT_LE_EXT_ADV_STATE_ENABLED) {
		/* Start advertising to get the actual adv addr */
		err = bt_le_ext_adv_start(proc_param->unicast_to_broadcast.ext_adv,
					  BT_LE_EXT_ADV_START_DEFAULT);
		if (err != 0) {
			LOG_DBG("Failed to start advertising set (err %d)\n", err);

			active_proc->err = err;
			active_proc->failed_conn = NULL;

			bt_cap_handover_proc_complete();

			return;
		}

		/* Since bt_le_ext_adv_get_info returns the pointer to actual advertising address we
		 * do not need to call it again to get the address
		 */
	}
	/* else if we are already in the paused or active state and do not need to anything */

	ARRAY_FOR_EACH(proc_param->unicast_to_broadcast.reception_start_member_params, i) {
		struct bt_cap_commander_broadcast_reception_start_member_param *member_param =
			&proc_param->unicast_to_broadcast.reception_start_member_params[i];
		const struct bt_conn *member_conn = bt_cap_common_get_member_conn(
			proc_param->unicast_to_broadcast.type, &member_param->member);

		/* The member_conns are populated from index and upwards, thus once we
		 * reach a NULL pointer in the array, there are no more acceptors to send the
		 * reception start request to.
		 */
		if (member_conn == NULL) {
			break;
		}

		member_param->addr = bt_addr_le_any;
		member_param->adv_sid = adv_info.sid;
		member_param->pa_interval = proc_param->unicast_to_broadcast.pa_interval;
		member_param->broadcast_id = proc_param->unicast_to_broadcast.broadcast_id;
		bt_addr_le_copy(&member_param->addr, adv_info.addr);

		member_param->num_subgroups = create_param->subgroup_count;
		for (size_t j = 0U; j < member_param->num_subgroups; j++) {
			const struct bt_cap_initiator_broadcast_subgroup_param *subgroup_param =
				&create_param->subgroup_params[j];
			struct bt_bap_bass_subgroup *subgroup = &member_param->subgroups[j];

			subgroup->metadata_len = subgroup_param->codec_cfg->meta_len;
			(void)memcpy(subgroup->metadata, subgroup_param->codec_cfg->meta,
				     subgroup->metadata_len);

			/* The bis_sync value has been set up earlier in
			 * bt_cap_handover_unicast_to_broadcast while we still had the ACL
			 * references
			 */
			__ASSERT(subgroup->bis_sync != 0U, "BIS sync was not properly setup");
		}

		param.count++;
	}

	err = cap_commander_broadcast_reception_start(&param);
	if (err != 0) {
		LOG_DBG("Failed to start reception start: %d", err);
		active_proc->err = err;
		active_proc->failed_conn = NULL;

		bt_cap_handover_proc_complete();
	}
}

void bt_cap_handover_unicast_to_broadcast_setup_broadcast(void)
{
	struct bt_cap_initiator_broadcast_create_param *broadcast_create_param;
	struct bt_cap_broadcast_source **broadcast_source;
	struct bt_cap_handover_proc_param *proc_param;
	struct bt_cap_common_proc *active_proc;
	struct bt_le_ext_adv *ext_adv;
	int err;

	active_proc = bt_cap_common_get_active_proc();
	proc_param = &active_proc->proc_param.handover;
	broadcast_create_param = proc_param->unicast_to_broadcast.broadcast_create_param;

	broadcast_source = &proc_param->unicast_to_broadcast.broadcast_source;

	err = bt_cap_unicast_group_delete(proc_param->unicast_to_broadcast.unicast_group);
	__ASSERT_NO_MSG(err == 0);
	proc_param->unicast_to_broadcast.unicast_group = NULL;

	err = bt_cap_initiator_broadcast_audio_create(broadcast_create_param, broadcast_source);
	if (err != 0) {
		LOG_DBG("Failed to create broadcast source: %d", err);
		active_proc->err = err;
		active_proc->failed_conn = NULL;

		bt_cap_handover_proc_complete();

		return;
	}

	ext_adv = active_proc->proc_param.handover.unicast_to_broadcast.ext_adv;
	err = bt_cap_initiator_broadcast_audio_start(*broadcast_source, ext_adv);
	if (err != 0) {
		LOG_DBG("Failed to start broadcast source: %d", err);
		active_proc->err = err;
		active_proc->failed_conn = NULL;

		err = bt_cap_initiator_broadcast_audio_delete(*broadcast_source);
		__ASSERT_NO_MSG(err == 0);

		bt_cap_handover_proc_complete();

		return;
	}
}

static bool valid_unicast_to_broadcast_stream_metadata_param(
	const struct bt_cap_initiator_broadcast_subgroup_param *subgroup_param,
	const struct bt_cap_stream *stream,
	const struct cap_unicast_group_stream_lookup *lookup_data)
{
	const uint8_t *broadcast_ccid_list;
	const uint8_t *unicast_ccid_list;
	int broadcast_ret;
	int unicast_ret;

	/* Compare existing unicast metadata with the subgroup param meteadata. It
	 * is mandatory that the CCID list and the context type remain the same
	 */
	if (stream->bap_stream.codec_cfg == subgroup_param->codec_cfg) {
		return true;
	}

	/* Verify CCID lists */
	unicast_ret = bt_audio_codec_cfg_meta_get_ccid_list(stream->bap_stream.codec_cfg,
							    &unicast_ccid_list);

	/* CCID list is not mandatory, so it is OK if it is missing, as long
	 * as it is missing for both unicast and broadcast
	 */

	if (unicast_ret < 0 && unicast_ret != -ENODATA) {
		return false;
	}

	broadcast_ret = bt_audio_codec_cfg_meta_get_ccid_list(subgroup_param->codec_cfg,
							      &broadcast_ccid_list);

	if (unicast_ret != broadcast_ret) {
		return false;
	}

	/* we only need to compare if the list exists and is non-empty */
	if (unicast_ret > 0 && !util_memeq(unicast_ccid_list, broadcast_ccid_list, unicast_ret)) {
		return false;
	}

	/* Verify streaming contexts (mandatory to be in the metadata )*/
	unicast_ret = bt_audio_codec_cfg_meta_get_stream_context(stream->bap_stream.codec_cfg);
	if (unicast_ret <= 0) { /* mandatory to have a streaming context */
		return false;
	}

	broadcast_ret = bt_audio_codec_cfg_meta_get_stream_context(subgroup_param->codec_cfg);
	if (unicast_ret != broadcast_ret) {
		return false;
	}

	return true;
}

static bool
valid_unicast_to_broadcast_metadata(const struct bt_cap_handover_unicast_to_broadcast_param *param,
				    const struct cap_unicast_group_stream_lookup *lookup_data)
{
	size_t unique_metadata_cnt = 0U;

	/* Count unique metadata from the unicast streams to verify the correct number of
	 * subgroups exist. Each unique set of metadata needs to be in its own group
	 */
	for (size_t i = 0U; i < lookup_data->active_sink_streams_cnt; i++) {
		const struct bt_bap_stream *bap_stream_i =
			&lookup_data->active_sink_streams[i]->bap_stream;
		const struct bt_audio_codec_cfg *codec_cfg_i = bap_stream_i->codec_cfg;
		bool unique_metadata = true;

		for (size_t j = 0U; j < i; j++) {
			const struct bt_bap_stream *bap_stream_j =
				&lookup_data->active_sink_streams[j]->bap_stream;
			const struct bt_audio_codec_cfg *codec_cfg_j = bap_stream_j->codec_cfg;

			if (codec_cfg_i == codec_cfg_j ||
			    util_eq(codec_cfg_i->meta, codec_cfg_i->meta_len, codec_cfg_j->meta,
				    codec_cfg_j->meta_len)) {
				unique_metadata = false;
				break;
			}
		}

		if (unique_metadata) {
			unique_metadata_cnt++;
		}
	}

	if (unique_metadata_cnt > CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT) {
		LOG_DBG("Cannot create broadcast source with %zu subgroups (max %d)",
			unique_metadata_cnt, CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT);

		return false;
	}

	if (unique_metadata_cnt > param->broadcast_create_param->subgroup_count) {
		LOG_DBG("Mismatch between unique metadata from unicast (%zu) and number of "
			"subgroups (%zu)",
			unique_metadata_cnt, param->broadcast_create_param->subgroup_count);

		return false;
	}

	return true;
}

static bool valid_unicast_to_broadcast_create_param(
	const struct bt_cap_handover_unicast_to_broadcast_param *param,
	const struct cap_unicast_group_stream_lookup *lookup_data)
{
	size_t total_broadcast_streams = 0U;

	for (size_t i = 0U; i < param->broadcast_create_param->subgroup_count; i++) {
		const struct bt_cap_initiator_broadcast_subgroup_param *subgroup_param =
			&param->broadcast_create_param->subgroup_params[i];

		for (size_t j = 0U; j < subgroup_param->stream_count; j++) {
			const struct bt_cap_stream *stream =
				subgroup_param->stream_params[j].stream;
			bool stream_is_handed_over = false;

			if (stream == NULL) {
				LOG_DBG("subgroup_param[%zu].stream_params[%zu].stream is NULL", i,
					j);
				return false;
			}

			if (stream->bap_stream.group != param->unicast_group->bap_unicast_group) {
				LOG_DBG("Stream %p is not part of the unicast group %p", stream,
					param->unicast_group->bap_unicast_group);
				return false;
			}

			for (size_t k = 0U; k < lookup_data->active_sink_streams_cnt; k++) {
				if (stream == lookup_data->active_sink_streams[k]) {
					stream_is_handed_over = true;
					break;
				}
			}

			if (!stream_is_handed_over) {
				LOG_DBG("Stream %p was not in unicast group %p", stream,
					param->unicast_group);
				return false;
			}

			if (!valid_unicast_to_broadcast_stream_metadata_param(
				    subgroup_param, stream, lookup_data)) {
				return false;
			}

			total_broadcast_streams++;
		}
	}

	if (total_broadcast_streams != lookup_data->active_sink_streams_cnt) {
		LOG_DBG("Not all unicast sink streams are being handed over to broadcast (%zu != "
			"%zu)",
			total_broadcast_streams, lookup_data->active_sink_streams_cnt);
		return false;
	}

	return true;
}

static bool
valid_unicast_to_broadcast_param(const struct bt_cap_handover_unicast_to_broadcast_param *param,
				 struct cap_unicast_group_stream_lookup *lookup_data)
{
	struct bt_le_ext_adv_info adv_info;
	int err;

	if (param == NULL) {
		LOG_DBG("param is NULL");
		return false;
	}

	if (param->unicast_group == NULL) {
		LOG_DBG("param->unicast_group is NULL");
		return false;
	}

	if (!bt_cap_initiator_broadcast_audio_start_valid_param(param->broadcast_create_param)) {
		LOG_DBG("param->broadcast_create_param is invalid");
		return false;
	}

	if (param->ext_adv == NULL) {
		LOG_DBG("param->ext_adv is NULL");
		return false;
	}

	err = bt_le_ext_adv_get_info(param->ext_adv, &adv_info);
	__ASSERT_NO_MSG(err == 0);

	if (adv_info.per_adv_state == BT_LE_PER_ADV_STATE_NONE) {
		LOG_DBG("Advertising set %p not configured for periodic advertising",
			param->ext_adv);
		return false;
	}

	if (param->broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		LOG_DBG("param->broadcast_id is invalid: 0x%08X", param->broadcast_id);
		return false;
	}

	if (!IN_RANGE(param->pa_interval, BT_GAP_PER_ADV_MIN_INTERVAL,
		      BT_GAP_PER_ADV_MAX_INTERVAL)) {
		LOG_DBG("param->pa_interval is invalid: %u", param->pa_interval);
		return false;
	}

	err = bt_cap_unicast_group_foreach_stream(param->unicast_group,
						  unicast_group_foreach_stream_cb, lookup_data);
	__ASSERT_NO_MSG(err == 0);
	if (lookup_data->active_sink_streams_cnt == 0U) {
		LOG_DBG("param->unicast_group does not contain any active streams");

		return false;
	}

	if (!valid_unicast_to_broadcast_create_param(param, lookup_data)) {
		return false;
	}

	if (!valid_unicast_to_broadcast_metadata(param, lookup_data)) {
		return false;
	}

	return true;
}

int bt_cap_handover_unicast_to_broadcast(
	const struct bt_cap_handover_unicast_to_broadcast_param *param)
{
	struct cap_unicast_group_stream_lookup lookup_data = {0};
	struct bt_cap_unicast_audio_stop_param stop_param = {0};
	struct bt_cap_handover_proc_param *proc_param;
	struct bt_cap_common_proc *active_proc;
	uint8_t bis_index;
	int err;

	if (!valid_unicast_to_broadcast_param(param, &lookup_data)) {
		return -EINVAL;
	}

	if (bt_cap_common_test_and_set_proc_active()) {
		LOG_DBG("A CAP procedure is already in progress");

		return -EBUSY;
	}

	/* TBD: How do we prevent the unicast group from being modified or deleted while we are
	 * doing this procedure?
	 * TBD: How do we check for BASS?
	 */

	if (lookup_data.cnt == 0U) {
		LOG_DBG("param->unicast_group does not contain any streams");

		bt_cap_common_clear_active_proc();

		return -EINVAL;
	}

	active_proc = bt_cap_common_get_active_proc();
	proc_param = &active_proc->proc_param.handover;

	/* Populate an array of unique connection pointers to determine which acceptors to add the
	 * broadcast source to
	 */
	for (size_t i = 0U; i < lookup_data.active_sink_streams_cnt; i++) {
		const struct bt_cap_stream *stream = lookup_data.active_sink_streams[i];
		bool conn_added;

		/* Add stream's conn to conns if not already there */
		conn_added = false;
		ARRAY_FOR_EACH(proc_param->unicast_to_broadcast.reception_start_member_params, j) {
			struct bt_cap_commander_broadcast_reception_start_member_param
				*member_param = &proc_param->unicast_to_broadcast
							 .reception_start_member_params[j];
			const struct bt_conn *member_conn =
				bt_cap_common_get_member_conn(param->type, &member_param->member);

			if (member_conn == stream->bap_stream.conn) {
				conn_added = true;
				break;
			}

			if (member_conn == NULL) {
				if (param->type == BT_CAP_SET_TYPE_CSIP) {
					struct bt_cap_common_client *client =
						bt_cap_common_get_client_by_acl(
							stream->bap_stream.conn);

					member_param->member.csip =
						bt_csip_set_coordinator_csis_inst_by_handle(
							stream->bap_stream.conn,
							client->csis_start_handle);
				} else {
					member_param->member.member = stream->bap_stream.conn;
					conn_added = true;
					break;
				}
			}
		}

		if (!conn_added) {
			LOG_DBG("Stream %p connection could not be added. "
				"Some streams contain an invalid conn pointer",
				stream);

			bt_cap_common_clear_active_proc();

			return -EINVAL;
		}
	}

	/* We need to set up the expected BIS index fields for the reception start now, as once the
	 * unicast group has been stopped, the reference to the ACL connection from the stream will
	 * be lost
	 */

	bis_index = 1U; /* BIS indexes start from 1 */
	for (size_t i = 0U; i < param->broadcast_create_param->subgroup_count; i++) {
		const struct bt_cap_initiator_broadcast_subgroup_param *subgroup_param =
			&param->broadcast_create_param->subgroup_params[i];

		for (size_t j = 0U; j < subgroup_param->stream_count; j++) {
			const struct bt_cap_stream *stream =
				subgroup_param->stream_params[j].stream;

			ARRAY_FOR_EACH_PTR(
				proc_param->unicast_to_broadcast.reception_start_member_params,
				member_param) {
				const struct bt_conn *member_conn = bt_cap_common_get_member_conn(
					param->type, &member_param->member);

				/* Once we reach a NULL connection pointer, we've handled all
				 * acceptors from the unicast group
				 */
				if (member_conn == NULL) {
					break;
				}

				if (stream->bap_stream.conn == member_conn) {
					member_param->subgroups[i].bis_sync |=
						BT_ISO_BIS_INDEX_BIT(bis_index);
				}
			}

			bis_index++;
		}
	}

	/* Store the broadcast parameters for later */
	proc_param->unicast_to_broadcast.broadcast_create_param = param->broadcast_create_param;
	proc_param->unicast_to_broadcast.unicast_group = param->unicast_group;
	proc_param->unicast_to_broadcast.ext_adv = param->ext_adv;
	proc_param->unicast_to_broadcast.type = param->type;
	proc_param->unicast_to_broadcast.pa_interval = param->pa_interval;
	proc_param->unicast_to_broadcast.broadcast_id = param->broadcast_id;

	stop_param.type = param->type;
	stop_param.release = true;
	stop_param.streams = lookup_data.streams;
	stop_param.count = lookup_data.cnt;

	bt_cap_common_set_handover_active();

	__ASSERT_NO_MSG(bt_cap_initiator_valid_unicast_audio_stop_param(&stop_param));

	err = cap_initiator_unicast_audio_stop(&stop_param);
	if (err != 0) {
		LOG_DBG("Failed to stop unicast audio: %d", err);

		bt_cap_common_clear_active_proc();

		return -ENOEXEC;
	}

	return 0;
}

int bt_cap_handover_broadcast_to_unicast(
	const struct bt_cap_handover_broadcast_to_unicast_param *param,
	struct bt_bap_unicast_group **unicast_group)
{
	return -ENOSYS;
}

int bt_cap_handover_register_cb(const struct bt_cap_handover_cb *cb)
{
	if (cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	if (cap_cb != NULL) {
		LOG_DBG("callbacks already registered");
		return -EALREADY;
	}

	cap_cb = cb;

	return 0;
}

int bt_cap_handover_unregister_cb(const struct bt_cap_handover_cb *cb)
{
	if (cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	if (cap_cb != cb) {
		LOG_DBG("cb is not registered");
		return -EINVAL;
	}

	cap_cb = NULL;

	return 0;
}
