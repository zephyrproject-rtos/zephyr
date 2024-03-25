/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/check.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include "cap_internal.h"
#include "ccid_internal.h"
#include "csip_internal.h"
#include "bap_endpoint.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_cap_initiator, CONFIG_BT_CAP_INITIATOR_LOG_LEVEL);

#include "common/bt_str.h"

static const struct bt_cap_initiator_cb *cap_cb;

int bt_cap_initiator_register_cb(const struct bt_cap_initiator_cb *cb)
{
	CHECKIF(cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	CHECKIF(cap_cb != NULL) {
		LOG_DBG("callbacks already registered");
		return -EALREADY;
	}

	cap_cb = cb;

	return 0;
}

struct valid_metadata_param {
	bool stream_context_found;
	bool valid;
};

static bool data_func_cb(struct bt_data *data, void *user_data)
{
	struct valid_metadata_param *metadata_param = (struct valid_metadata_param *)user_data;

	LOG_DBG("type %u len %u data %s", data->type, data->data_len,
		bt_hex(data->data, data->data_len));

	if (data->type == BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT) {
		if (data->data_len != 2) { /* Stream context size */
			return false;
		}

		metadata_param->stream_context_found = true;
	} else if (IS_ENABLED(CONFIG_BT_CCID) && data->type == BT_AUDIO_METADATA_TYPE_CCID_LIST) {
		/* If the application supplies a CCID list, we verify that the CCIDs exist on our
		 * device
		 */
		for (uint8_t i = 0U; i < data->data_len; i++) {
			const uint8_t ccid = data->data[i];

			if (bt_ccid_find_attr(ccid) == NULL) {
				LOG_DBG("Unknown characterstic for CCID 0x%02X", ccid);
				metadata_param->valid = false;

				return false;
			}
		}
	}

	return true;
}

static bool cap_initiator_valid_metadata(const uint8_t meta[], size_t meta_len)
{
	struct valid_metadata_param metadata_param = {
		.stream_context_found = false,
		.valid = true,
	};
	int err;

	LOG_DBG("meta %p len %zu", meta, meta_len);

	err = bt_audio_data_parse(meta, meta_len, data_func_cb, &metadata_param);
	if (err != 0 && err != -ECANCELED) {
		return false;
	}

	if (!metadata_param.stream_context_found) {
		LOG_DBG("No streaming context supplied");
	}

	return metadata_param.stream_context_found && metadata_param.valid;
}

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
static struct bt_cap_broadcast_source {
	struct bt_bap_broadcast_source *bap_broadcast;
} broadcast_sources[CONFIG_BT_BAP_BROADCAST_SRC_COUNT];

static bool cap_initiator_broadcast_audio_start_valid_param(
	const struct bt_cap_initiator_broadcast_create_param *param)
{

	for (size_t i = 0U; i < param->subgroup_count; i++) {
		const struct bt_cap_initiator_broadcast_subgroup_param *subgroup_param;
		const struct bt_audio_codec_cfg *codec_cfg;
		bool valid_metadata;

		subgroup_param = &param->subgroup_params[i];
		codec_cfg = subgroup_param->codec_cfg;

		/* Streaming Audio Context shall be present in CAP */

		CHECKIF(codec_cfg == NULL) {
			LOG_DBG("subgroup[%zu]->codec_cfg is NULL", i);
			return false;
		}

		valid_metadata =
			cap_initiator_valid_metadata(codec_cfg->meta, codec_cfg->meta_len);

		CHECKIF(!valid_metadata) {
			LOG_DBG("Invalid metadata supplied for subgroup[%zu]", i);
			return false;
		}
	}

	return true;
}

static void cap_initiator_broadcast_to_bap_broadcast_param(
	const struct bt_cap_initiator_broadcast_create_param *cap_param,
	struct bt_bap_broadcast_source_param *bap_param,
	struct bt_bap_broadcast_source_subgroup_param
		bap_subgroup_params[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT],
	struct bt_bap_broadcast_source_stream_param
		bap_stream_params[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT])
{
	size_t stream_cnt = 0U;

	bap_param->params_count = cap_param->subgroup_count;
	bap_param->params = bap_subgroup_params;
	bap_param->qos = cap_param->qos;
	bap_param->packing = cap_param->packing;
	bap_param->encryption = cap_param->encryption;
	if (bap_param->encryption) {
		memcpy(bap_param->broadcast_code, cap_param->broadcast_code,
		       BT_AUDIO_BROADCAST_CODE_SIZE);
	} else {
		memset(bap_param->broadcast_code, 0, BT_AUDIO_BROADCAST_CODE_SIZE);
	}

	for (size_t i = 0U; i < bap_param->params_count; i++) {
		const struct bt_cap_initiator_broadcast_subgroup_param *cap_subgroup_param =
			&cap_param->subgroup_params[i];
		struct bt_bap_broadcast_source_subgroup_param *bap_subgroup_param =
			&bap_param->params[i];

		bap_subgroup_param->codec_cfg = cap_subgroup_param->codec_cfg;
		bap_subgroup_param->params_count = cap_subgroup_param->stream_count;
		bap_subgroup_param->params = &bap_stream_params[stream_cnt];

		for (size_t j = 0U; j < bap_subgroup_param->params_count; j++, stream_cnt++) {
			const struct bt_cap_initiator_broadcast_stream_param *cap_stream_param =
				&cap_subgroup_param->stream_params[j];
			struct bt_bap_broadcast_source_stream_param *bap_stream_param =
				&bap_subgroup_param->params[j];

			bap_stream_param->stream = &cap_stream_param->stream->bap_stream;
#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
			bap_stream_param->data_len = cap_stream_param->data_len;
			/* We do not need to copy the data, as that is the same type of struct, so
			 * we can just point to the CAP parameter data
			 */
			bap_stream_param->data = cap_stream_param->data;
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */
		}
	}
}

int bt_cap_initiator_broadcast_audio_create(
	const struct bt_cap_initiator_broadcast_create_param *param,
	struct bt_cap_broadcast_source **broadcast_source)
{
	struct bt_bap_broadcast_source_subgroup_param
		bap_subgroup_params[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
	struct bt_bap_broadcast_source_stream_param
		bap_stream_params[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
	struct bt_bap_broadcast_source_param bap_create_param = {0};

	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return -EINVAL;
	}

	CHECKIF(broadcast_source == NULL) {
		LOG_DBG("source is NULL");
		return -EINVAL;
	}

	if (!cap_initiator_broadcast_audio_start_valid_param(param)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(broadcast_sources); i++) {
		if (broadcast_sources[i].bap_broadcast == NULL) {
			*broadcast_source = &broadcast_sources[i];
			break;
		}
	}

	cap_initiator_broadcast_to_bap_broadcast_param(param, &bap_create_param,
						       bap_subgroup_params, bap_stream_params);

	return bt_bap_broadcast_source_create(&bap_create_param,
					      &(*broadcast_source)->bap_broadcast);
}

int bt_cap_initiator_broadcast_audio_start(struct bt_cap_broadcast_source *broadcast_source,
					   struct bt_le_ext_adv *adv)
{
	CHECKIF(adv == NULL) {
		LOG_DBG("adv is NULL");
		return -EINVAL;
	}

	CHECKIF(broadcast_source == NULL) {
		LOG_DBG("broadcast_source is NULL");
		return -EINVAL;
	}

	return bt_bap_broadcast_source_start(broadcast_source->bap_broadcast, adv);
}

int bt_cap_initiator_broadcast_audio_update(struct bt_cap_broadcast_source *broadcast_source,
					    const uint8_t meta[], size_t meta_len)
{
	CHECKIF(broadcast_source == NULL) {
		LOG_DBG("broadcast_source is NULL");
		return -EINVAL;
	}

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	if (!cap_initiator_valid_metadata(meta, meta_len)) {
		LOG_DBG("Invalid metadata");
		return -EINVAL;
	}

	return bt_bap_broadcast_source_update_metadata(broadcast_source->bap_broadcast, meta,
						       meta_len);
}

int bt_cap_initiator_broadcast_audio_stop(struct bt_cap_broadcast_source *broadcast_source)
{
	CHECKIF(broadcast_source == NULL) {
		LOG_DBG("broadcast_source is NULL");
		return -EINVAL;
	}

	return bt_bap_broadcast_source_stop(broadcast_source->bap_broadcast);
}

int bt_cap_initiator_broadcast_audio_delete(struct bt_cap_broadcast_source *broadcast_source)
{
	int err;

	CHECKIF(broadcast_source == NULL) {
		LOG_DBG("broadcast_source is NULL");
		return -EINVAL;
	}

	err = bt_bap_broadcast_source_delete(broadcast_source->bap_broadcast);
	if (err == 0) {
		broadcast_source->bap_broadcast = NULL;
	}

	return err;
}

int bt_cap_initiator_broadcast_get_id(const struct bt_cap_broadcast_source *broadcast_source,
				      uint32_t *const broadcast_id)
{
	CHECKIF(broadcast_source == NULL) {
		LOG_DBG("broadcast_source is NULL");
		return -EINVAL;
	}

	return bt_bap_broadcast_source_get_id(broadcast_source->bap_broadcast, broadcast_id);
}

int bt_cap_initiator_broadcast_get_base(struct bt_cap_broadcast_source *broadcast_source,
					struct net_buf_simple *base_buf)
{
	CHECKIF(broadcast_source == NULL) {
		LOG_DBG("broadcast_source is NULL");
		return -EINVAL;
	}

	return bt_bap_broadcast_source_get_base(broadcast_source->bap_broadcast, base_buf);
}

#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)

static void
bt_cap_initiator_discover_complete(struct bt_conn *conn, int err,
				   const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (cap_cb && cap_cb->unicast_discovery_complete) {
		cap_cb->unicast_discovery_complete(conn, err, csis_inst);
	}
}

int bt_cap_initiator_unicast_discover(struct bt_conn *conn)
{
	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	return bt_cap_common_discover(conn, bt_cap_initiator_discover_complete);
}

static bool valid_unicast_audio_start_param(const struct bt_cap_unicast_audio_start_param *param)
{
	struct bt_bap_unicast_group *unicast_group = NULL;

	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return false;
	}

	CHECKIF(param->count == 0) {
		LOG_DBG("Invalid param->count: %u", param->count);
		return false;
	}

	CHECKIF(param->stream_params == NULL) {
		LOG_DBG("param->stream_params is NULL");
		return false;
	}

	CHECKIF(param->count > CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT) {
		LOG_DBG("param->count (%zu) is larger than "
			"CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT (%d)",
			param->count,
			CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT);
		return false;
	}

	for (size_t i = 0U; i < param->count; i++) {
		const struct bt_cap_unicast_audio_start_stream_param *stream_param =
									&param->stream_params[i];
		const union bt_cap_set_member *member = &stream_param->member;
		const struct bt_cap_stream *cap_stream = stream_param->stream;
		const struct bt_audio_codec_cfg *codec_cfg = stream_param->codec_cfg;
		const struct bt_bap_stream *bap_stream;
		const struct bt_cap_common_client *client =
			bt_cap_common_get_client(param->type, member);

		if (client == NULL) {
			LOG_DBG("Invalid param->members[%zu]", i);
			return false;
		}

		CHECKIF(stream_param->codec_cfg == NULL) {
			LOG_DBG("param->stream_params[%zu].codec_cfg  is NULL", i);
			return false;
		}

		CHECKIF(!cap_initiator_valid_metadata(codec_cfg->meta, codec_cfg->meta_len)) {
			LOG_DBG("param->stream_params[%zu].codec_cfg  is invalid", i);
			return false;
		}

		CHECKIF(stream_param->ep == NULL) {
			LOG_DBG("param->stream_params[%zu].ep is NULL", i);
			return false;
		}

		CHECKIF(member == NULL) {
			LOG_DBG("param->stream_params[%zu].member is NULL", i);
			return false;
		}

		CHECKIF(cap_stream == NULL) {
			LOG_DBG("param->streams[%zu] is NULL", i);
			return false;
		}

		bap_stream = &cap_stream->bap_stream;

		CHECKIF(bap_stream->ep != NULL) {
			LOG_DBG("param->streams[%zu] is already started", i);
			return false;
		}

		CHECKIF(bap_stream->group == NULL) {
			LOG_DBG("param->streams[%zu] is not in a unicast group", i);
			return false;
		}

		/* Use the group of the first stream for comparison */
		if (unicast_group == NULL) {
			unicast_group = bap_stream->group;
		} else {
			CHECKIF(bap_stream->group != unicast_group) {
				LOG_DBG("param->streams[%zu] is not in this group %p", i,
					unicast_group);
				return false;
			}
		}

		for (size_t j = 0U; j < i; j++) {
			if (param->stream_params[j].stream == cap_stream) {
				LOG_DBG("param->stream_params[%zu] (%p) is "
					"duplicated by "
					"param->stream_params[%zu] (%p)",
					j, param->stream_params[j].stream,
					i, cap_stream);
				return false;
			}
		}
	}

	return true;
}

static void cap_initiator_unicast_audio_proc_complete(void)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	enum bt_cap_common_proc_type proc_type;
	struct bt_conn *failed_conn;
	int err;

	failed_conn = active_proc->failed_conn;
	err = active_proc->err;
	proc_type = active_proc->proc_type;
	bt_cap_common_clear_active_proc();

	if (cap_cb == NULL) {
		return;
	}

	switch (proc_type) {
	case BT_CAP_COMMON_PROC_TYPE_START:
		if (cap_cb->unicast_start_complete != NULL) {
			cap_cb->unicast_start_complete(err, failed_conn);
		}
		break;
	case BT_CAP_COMMON_PROC_TYPE_UPDATE:
		if (cap_cb->unicast_update_complete != NULL) {
			cap_cb->unicast_update_complete(err, failed_conn);
		}
		break;
	case BT_CAP_COMMON_PROC_TYPE_STOP:
		if (cap_cb->unicast_stop_complete != NULL) {
			cap_cb->unicast_stop_complete(err, failed_conn);
		}
		break;
	case BT_CAP_COMMON_PROC_TYPE_NONE:
	default:
		__ASSERT(false, "Invalid proc_type: %u", proc_type);
	}
}

static int cap_initiator_unicast_audio_configure(
	const struct bt_cap_unicast_audio_start_param *param)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	struct bt_cap_initiator_proc_param *proc_param;
	struct bt_audio_codec_cfg *codec_cfg;
	struct bt_bap_stream *bap_stream;
	struct bt_bap_ep *ep;
	struct bt_conn *conn;
	int err;
	/** TODO: If this is a CSIP set, then the order of the procedures may
	 * not match the order in the parameters, and the CSIP ordered access
	 * procedure should be used.
	 */

	for (size_t i = 0U; i < param->count; i++) {
		struct bt_cap_unicast_audio_start_stream_param *stream_param =
									&param->stream_params[i];
		union bt_cap_set_member *member = &stream_param->member;
		struct bt_cap_stream *cap_stream = stream_param->stream;

		conn = bt_cap_common_get_member_conn(param->type, member);

		/* Ensure that ops are registered before any procedures are started */
		bt_cap_stream_ops_register_bap(cap_stream);

		/* Store the necessary parameters as we cannot assume that the supplied parameters
		 * are kept valid
		 */
		active_proc->proc_param.initiator[i].stream = cap_stream;
		active_proc->proc_param.initiator[i].start.ep = stream_param->ep;
		active_proc->proc_param.initiator[i].start.conn = conn;
		active_proc->proc_param.initiator[i].start.codec_cfg = stream_param->codec_cfg;
	}

	/* Store the information about the active procedure so that we can
	 * continue the procedure after each step
	 */
	bt_cap_common_start_proc(BT_CAP_COMMON_PROC_TYPE_START, param->count);
	bt_cap_common_set_subproc(BT_CAP_COMMON_SUBPROC_TYPE_CODEC_CONFIG);

	proc_param = &active_proc->proc_param.initiator[0];
	bap_stream = &proc_param->stream->bap_stream;
	codec_cfg = proc_param->start.codec_cfg;
	conn = proc_param->start.conn;
	ep = proc_param->start.ep;
	active_proc->proc_initiated_cnt++;

	/* Since BAP operations may require a write long or a read long on the notification,
	 * we cannot assume that we can do multiple streams at once, thus do it one at a time.
	 * TODO: We should always be able to do one per ACL, so there is room for optimization.
	 */
	err = bt_bap_stream_config(conn, bap_stream, ep, codec_cfg);
	if (err != 0) {
		LOG_DBG("Failed to config stream %p: %d", proc_param->stream, err);

		bt_cap_common_clear_active_proc();
	}

	return err;
}

int bt_cap_initiator_unicast_audio_start(const struct bt_cap_unicast_audio_start_param *param)
{
	if (bt_cap_common_proc_is_active()) {
		LOG_DBG("A CAP procedure is already in progress");

		return -EBUSY;
	}

	if (!valid_unicast_audio_start_param(param)) {
		return -EINVAL;
	}

	return cap_initiator_unicast_audio_configure(param);
}

void bt_cap_initiator_codec_configured(struct bt_cap_stream *cap_stream)
{
	struct bt_conn
		*conns[MIN(CONFIG_BT_MAX_CONN, CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT)];
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	struct bt_cap_initiator_proc_param *proc_param;
	struct bt_bap_unicast_group *unicast_group;

	if (!bt_cap_common_stream_in_active_proc(cap_stream)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (bt_cap_common_subproc_is_type(BT_CAP_COMMON_SUBPROC_TYPE_RELEASE)) {
		/* When releasing a stream, it may go into the codec configured state if
		 * the unicast server caches the configuration - We treat it as a release
		 */
		bt_cap_initiator_released(cap_stream);
		return;
	} else if (!bt_cap_common_subproc_is_type(BT_CAP_COMMON_SUBPROC_TYPE_CODEC_CONFIG)) {
		/* Unexpected callback - Abort */
		bt_cap_common_abort_proc(cap_stream->bap_stream.conn, -EBADMSG);
	} else {
		active_proc->proc_done_cnt++;

		LOG_DBG("Stream %p configured (%zu/%zu streams done)", cap_stream,
			active_proc->proc_done_cnt, active_proc->proc_cnt);
	}

	if (bt_cap_common_proc_is_aborted()) {
		if (bt_cap_common_proc_all_handled()) {
			cap_initiator_unicast_audio_proc_complete();
		}

		return;
	}

	if (!bt_cap_common_proc_is_done()) {
		const size_t proc_done_cnt = active_proc->proc_done_cnt;
		struct bt_cap_stream *next_cap_stream;
		struct bt_audio_codec_cfg *codec_cfg;
		struct bt_bap_stream *bap_stream;
		struct bt_conn *conn;
		struct bt_bap_ep *ep;
		int err;

		proc_param = &active_proc->proc_param.initiator[proc_done_cnt];
		next_cap_stream = proc_param->stream;
		conn = proc_param->start.conn;
		ep = proc_param->start.ep;
		codec_cfg = proc_param->start.codec_cfg;
		bap_stream = &next_cap_stream->bap_stream;
		active_proc->proc_initiated_cnt++;

		/* Since BAP operations may require a write long or a read long on the notification,
		 * we cannot assume that we can do multiple streams at once, thus do it one at a
		 * time.
		 * TODO: We should always be able to do one per ACL, so there is room for
		 * optimization.
		 */
		err = bt_bap_stream_config(conn, bap_stream, ep, codec_cfg);
		if (err != 0) {
			LOG_DBG("Failed to config stream %p: %d", next_cap_stream, err);

			bt_cap_common_abort_proc(conn, err);
			cap_initiator_unicast_audio_proc_complete();
		}

		return;
	}

	/* The QoS Configure procedure works on a set of connections and a
	 * unicast group, so we generate a list of unique connection pointers
	 * for the procedure
	 */
	(void)memset(conns, 0, sizeof(conns));
	for (size_t i = 0U; i < active_proc->proc_cnt; i++) {
		struct bt_conn *stream_conn =
			active_proc->proc_param.initiator[i].stream->bap_stream.conn;
		struct bt_conn **free_conn = NULL;
		bool already_added = false;

		for (size_t j = 0U; j < ARRAY_SIZE(conns); j++) {
			if (stream_conn == conns[j]) {
				already_added = true;
				break;
			} else if (conns[j] == NULL && free_conn == NULL) {
				free_conn = &conns[j];
			}
		}

		if (already_added) {
			continue;
		}

		if (free_conn != NULL) {
			*free_conn = stream_conn;
		} else {
			__ASSERT_PRINT("No free conns");
		}
	}

	/* All streams in the procedure share the same unicast group, so we just
	 * use the reference from the first stream
	 */
	proc_param = &active_proc->proc_param.initiator[0];
	unicast_group = (struct bt_bap_unicast_group *)proc_param->stream->bap_stream.group;
	bt_cap_common_set_subproc(BT_CAP_COMMON_SUBPROC_TYPE_QOS_CONFIG);

	for (size_t i = 0U; i < ARRAY_SIZE(conns); i++) {
		int err;

		/* When conns[i] is NULL, we have QoS Configured all unique connections */
		if (conns[i] == NULL) {
			break;
		}

		active_proc->proc_initiated_cnt++;

		err = bt_bap_stream_qos(conns[i], unicast_group);
		if (err != 0) {
			LOG_DBG("Failed to set stream QoS for conn %p and group %p: %d",
				(void *)conns[i], unicast_group, err);

			/* End or mark procedure as aborted.
			 * If we have sent any requests over air, we will abort
			 * once all sent requests has completed
			 */
			bt_cap_common_abort_proc(conns[i], err);
			if (i == 0U) {
				cap_initiator_unicast_audio_proc_complete();
			}

			return;
		}
	}
}

void bt_cap_initiator_qos_configured(struct bt_cap_stream *cap_stream)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	struct bt_cap_initiator_proc_param *proc_param;
	struct bt_cap_stream *next_cap_stream;
	struct bt_bap_stream *bap_stream;
	int err;

	if (!bt_cap_common_stream_in_active_proc(cap_stream)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (!bt_cap_common_subproc_is_type(BT_CAP_COMMON_SUBPROC_TYPE_QOS_CONFIG)) {
		/* Unexpected callback - Abort */
		bt_cap_common_abort_proc(cap_stream->bap_stream.conn, -EBADMSG);
	} else {
		active_proc->proc_done_cnt++;

		LOG_DBG("Stream %p QoS configured (%zu/%zu streams done)", cap_stream,
			active_proc->proc_done_cnt, active_proc->proc_cnt);
	}

	if (bt_cap_common_proc_is_aborted()) {
		if (bt_cap_common_proc_all_handled()) {
			cap_initiator_unicast_audio_proc_complete();
		}

		return;
	}

	if (!bt_cap_common_proc_is_done()) {
		/* Not yet finished, wait for all */
		return;
	}

	bt_cap_common_set_subproc(BT_CAP_COMMON_SUBPROC_TYPE_ENABLE);
	proc_param = &active_proc->proc_param.initiator[0];
	next_cap_stream = proc_param->stream;
	bap_stream = &next_cap_stream->bap_stream;
	active_proc->proc_initiated_cnt++;

	/* Since BAP operations may require a write long or a read long on the notification, we
	 * cannot assume that we can do multiple streams at once, thus do it one at a time.
	 * TODO: We should always be able to do one per ACL, so there is room for optimization.
	 */
	err = bt_bap_stream_enable(bap_stream, bap_stream->codec_cfg->meta,
				   bap_stream->codec_cfg->meta_len);
	if (err != 0) {
		LOG_DBG("Failed to enable stream %p: %d", next_cap_stream, err);

		bt_cap_common_abort_proc(bap_stream->conn, err);
		cap_initiator_unicast_audio_proc_complete();
	}
}

void bt_cap_initiator_enabled(struct bt_cap_stream *cap_stream)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	struct bt_cap_initiator_proc_param *proc_param;
	struct bt_bap_stream *bap_stream;
	int err;

	if (!bt_cap_common_stream_in_active_proc(cap_stream)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (!bt_cap_common_subproc_is_type(BT_CAP_COMMON_SUBPROC_TYPE_ENABLE)) {
		/* Unexpected callback - Abort */
		bt_cap_common_abort_proc(cap_stream->bap_stream.conn, -EBADMSG);
	} else {
		active_proc->proc_done_cnt++;

		LOG_DBG("Stream %p enabled (%zu/%zu streams done)", cap_stream,
			active_proc->proc_done_cnt, active_proc->proc_cnt);
	}

	if (bt_cap_common_proc_is_aborted()) {
		if (bt_cap_common_proc_all_handled()) {
			cap_initiator_unicast_audio_proc_complete();
		}

		return;
	}

	if (!bt_cap_common_proc_is_done()) {
		struct bt_cap_stream *next_cap_stream =
			active_proc->proc_param.initiator[active_proc->proc_done_cnt].stream;
		struct bt_bap_stream *next_bap_stream = &next_cap_stream->bap_stream;

		active_proc->proc_initiated_cnt++;

		/* Since BAP operations may require a write long or a read long on the notification,
		 * we cannot assume that we can do multiple streams at once, thus do it one at a
		 * time.
		 * TODO: We should always be able to do one per ACL, so there is room for
		 * optimization.
		 */
		err = bt_bap_stream_enable(next_bap_stream, next_bap_stream->codec_cfg->meta,
					   next_bap_stream->codec_cfg->meta_len);
		if (err != 0) {
			LOG_DBG("Failed to enable stream %p: %d", next_cap_stream, err);

			bt_cap_common_abort_proc(next_bap_stream->conn, err);
			cap_initiator_unicast_audio_proc_complete();
		}

		return;
	}

	bt_cap_common_set_subproc(BT_CAP_COMMON_SUBPROC_TYPE_START);
	proc_param = &active_proc->proc_param.initiator[0];
	bap_stream = &proc_param->stream->bap_stream;

	/* Since BAP operations may require a write long or a read long on the notification, we
	 * cannot assume that we can do multiple streams at once, thus do it one at a time.
	 * TODO: We should always be able to do one per ACL, so there is room for optimization.
	 */
	err = bt_bap_stream_start(bap_stream);
	if (err != 0) {
		LOG_DBG("Failed to start stream %p: %d", proc_param->stream, err);

		/* End and mark procedure as aborted.
		 * If we have sent any requests over air, we will abort
		 * once all sent requests has completed
		 */
		bt_cap_common_abort_proc(bap_stream->conn, err);
		cap_initiator_unicast_audio_proc_complete();

		return;
	}
}

void bt_cap_initiator_started(struct bt_cap_stream *cap_stream)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();

	if (!bt_cap_common_stream_in_active_proc(cap_stream)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (!bt_cap_common_subproc_is_type(BT_CAP_COMMON_SUBPROC_TYPE_START)) {
		/* Unexpected callback - Abort */
		bt_cap_common_abort_proc(cap_stream->bap_stream.conn, -EBADMSG);
	} else {
		active_proc->proc_done_cnt++;

		LOG_DBG("Stream %p started (%zu/%zu streams done)", cap_stream,
			active_proc->proc_done_cnt, active_proc->proc_cnt);
	}

	/* Since bt_bap_stream_start connects the ISO, we can, at this point,
	 * only do this one by one due to a restriction in the ISO layer
	 * (maximum 1 outstanding ISO connection request at any one time).
	 */
	if (!bt_cap_common_proc_is_done()) {
		struct bt_cap_stream *next_cap_stream =
			active_proc->proc_param.initiator[active_proc->proc_done_cnt].stream;
		struct bt_bap_stream *bap_stream = &next_cap_stream->bap_stream;
		int err;

		/* Not yet finished, start next */
		err = bt_bap_stream_start(bap_stream);
		if (err != 0) {
			LOG_DBG("Failed to start stream %p: %d", next_cap_stream, err);

			/* End and mark procedure as aborted.
			 * If we have sent any requests over air, we will abort
			 * once all sent requests has completed
			 */
			bt_cap_common_abort_proc(bap_stream->conn, err);
			cap_initiator_unicast_audio_proc_complete();
		}
	} else {
		cap_initiator_unicast_audio_proc_complete();
	}
}

static bool can_update_metadata(const struct bt_bap_stream *bap_stream)
{
	struct bt_bap_ep_info ep_info;
	int err;

	if (bap_stream->conn == NULL) {
		return false;
	}

	err = bt_bap_ep_get_info(bap_stream->ep, &ep_info);
	if (err != 0) {
		LOG_DBG("Failed to get endpoint info %p: %d", bap_stream, err);

		return false;
	}

	return ep_info.state == BT_BAP_EP_STATE_ENABLING ||
	       ep_info.state == BT_BAP_EP_STATE_STREAMING;
}

static bool valid_unicast_audio_update_param(const struct bt_cap_unicast_audio_update_param *param)
{
	struct bt_bap_unicast_group *unicast_group = NULL;

	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return false;
	}

	CHECKIF(param->count == 0) {
		LOG_DBG("Invalid param->count: %u", param->count);
		return false;
	}

	CHECKIF(param->stream_params == NULL) {
		LOG_DBG("param->stream_params is NULL");
		return false;
	}

	CHECKIF(param->count > CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT) {
		LOG_DBG("param->count (%zu) is larger than "
			"CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT (%d)",
			param->count, CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT);
		return false;
	}

	for (size_t i = 0U; i < param->count; i++) {
		const struct bt_cap_unicast_audio_update_stream_param *stream_param =
			&param->stream_params[i];
		const struct bt_cap_stream *cap_stream = stream_param->stream;
		const struct bt_bap_stream *bap_stream;
		struct bt_cap_common_client *client;
		struct bt_conn *conn;

		CHECKIF(cap_stream == NULL) {
			LOG_DBG("param->stream_params[%zu] is NULL", i);
			return false;
		}

		bap_stream = &cap_stream->bap_stream;
		conn = bap_stream->conn;
		CHECKIF(conn == NULL) {
			LOG_DBG("param->stream_params[%zu].stream->bap_stream.conn is NULL", i);

			return -EINVAL;
		}

		client = bt_cap_common_get_client_by_acl(conn);
		if (!client->cas_found) {
			LOG_DBG("CAS was not found for param->stream_params[%zu].stream", i);
			return false;
		}

		CHECKIF(bap_stream->group == NULL) {
			LOG_DBG("param->stream_params[%zu] is not in a unicast group", i);
			return false;
		}

		/* Use the group of the first stream for comparison */
		if (unicast_group == NULL) {
			unicast_group = bap_stream->group;
		} else {
			CHECKIF(bap_stream->group != unicast_group) {
				LOG_DBG("param->stream_params[%zu] is not in this group %p", i,
					unicast_group);
				return false;
			}
		}

		if (!can_update_metadata(bap_stream)) {
			LOG_DBG("param->stream_params[%zu].stream is not in right state to be "
				"updated",
				i);

			return false;
		}

		if (!cap_initiator_valid_metadata(stream_param->meta, stream_param->meta_len)) {
			LOG_DBG("param->stream_params[%zu] invalid metadata", i);

			return false;
		}

		for (size_t j = 0U; j < i; j++) {
			if (param->stream_params[j].stream == cap_stream) {
				LOG_DBG("param->stream_params[%zu] (%p) is "
					"duplicated by "
					"param->stream_params[%zu] (%p)",
					j, param->stream_params[j].stream, i, cap_stream);
				return false;
			}
		}
	}

	return true;
}

int bt_cap_initiator_unicast_audio_update(const struct bt_cap_unicast_audio_update_param *param)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	struct bt_cap_initiator_proc_param *proc_param;
	struct bt_bap_stream *bap_stream;
	const uint8_t *meta;
	size_t meta_len;
	int err;

	if (bt_cap_common_proc_is_active()) {
		LOG_DBG("A CAP procedure is already in progress");

		return -EBUSY;
	}

	if (!valid_unicast_audio_update_param(param)) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < param->count; i++) {
		const struct bt_cap_unicast_audio_update_stream_param *stream_param =
			&param->stream_params[i];
		struct bt_cap_stream *cap_stream = stream_param->stream;

		active_proc->proc_param.initiator[i].stream = cap_stream;
		active_proc->proc_param.initiator[i].meta_update.meta_len = stream_param->meta_len;
		memcpy(&active_proc->proc_param.initiator[i].meta_update.meta, stream_param->meta,
		       stream_param->meta_len);
	}

	bt_cap_common_start_proc(BT_CAP_COMMON_PROC_TYPE_UPDATE, param->count);
	bt_cap_common_set_subproc(BT_CAP_COMMON_SUBPROC_TYPE_META_UPDATE);

	proc_param = &active_proc->proc_param.initiator[0];
	bap_stream = &proc_param->stream->bap_stream;
	meta_len = proc_param->meta_update.meta_len;
	meta = proc_param->meta_update.meta;
	active_proc->proc_initiated_cnt++;

	err = bt_bap_stream_metadata(bap_stream, meta, meta_len);
	if (err != 0) {
		LOG_DBG("Failed to update metadata for stream %p: %d", proc_param->stream, err);

		bt_cap_common_clear_active_proc();
	}

	return err;
}

int bt_cap_initiator_unicast_audio_cancel(void)
{
	if (!bt_cap_common_proc_is_active() && !bt_cap_common_proc_is_aborted()) {
		LOG_DBG("No CAP procedure is in progress");

		return -EALREADY;
	}

	bt_cap_common_abort_proc(NULL, -ECANCELED);
	cap_initiator_unicast_audio_proc_complete();

	return 0;
}

void bt_cap_initiator_metadata_updated(struct bt_cap_stream *cap_stream)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();

	if (!bt_cap_common_stream_in_active_proc(cap_stream)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (!bt_cap_common_subproc_is_type(BT_CAP_COMMON_SUBPROC_TYPE_META_UPDATE)) {
		/* Unexpected callback - Abort */
		bt_cap_common_abort_proc(cap_stream->bap_stream.conn, -EBADMSG);
	} else {
		active_proc->proc_done_cnt++;

		LOG_DBG("Stream %p QoS metadata updated (%zu/%zu streams done)", cap_stream,
			active_proc->proc_done_cnt, active_proc->proc_cnt);
	}

	if (bt_cap_common_proc_is_aborted()) {
		if (bt_cap_common_proc_all_handled()) {
			cap_initiator_unicast_audio_proc_complete();
		}

		return;
	}

	if (!bt_cap_common_proc_is_done()) {
		const size_t proc_done_cnt = active_proc->proc_done_cnt;
		struct bt_cap_initiator_proc_param *proc_param;
		struct bt_cap_stream *next_cap_stream;
		struct bt_bap_stream *bap_stream;
		const uint8_t *meta;
		size_t meta_len;
		int err;

		proc_param = &active_proc->proc_param.initiator[proc_done_cnt];
		meta_len = proc_param->meta_update.meta_len;
		meta = proc_param->meta_update.meta;
		next_cap_stream = proc_param->stream;
		bap_stream = &next_cap_stream->bap_stream;
		active_proc->proc_initiated_cnt++;

		/* Since BAP operations may require a write long or a read long on the notification,
		 * we cannot assume that we can do multiple streams at once, thus do it one at a
		 * time.
		 * TODO: We should always be able to do one per ACL, so there is room for
		 * optimization.
		 */

		err = bt_bap_stream_metadata(bap_stream, meta, meta_len);
		if (err != 0) {
			LOG_DBG("Failed to update metadata for stream %p: %d", next_cap_stream,
				err);

			bt_cap_common_abort_proc(bap_stream->conn, err);
			cap_initiator_unicast_audio_proc_complete();
		}

		return;
	}

	cap_initiator_unicast_audio_proc_complete();
}

static bool can_release(const struct bt_bap_stream *bap_stream)
{
	struct bt_bap_ep_info ep_info;
	int err;

	if (bap_stream->conn == NULL) {
		return false;
	}

	err = bt_bap_ep_get_info(bap_stream->ep, &ep_info);
	if (err != 0) {
		LOG_DBG("Failed to get endpoint info %p: %d", bap_stream, err);

		return false;
	}

	return ep_info.state != BT_BAP_EP_STATE_IDLE;
}

static bool valid_unicast_audio_stop_param(const struct bt_cap_unicast_audio_stop_param *param)
{
	struct bt_bap_unicast_group *unicast_group = NULL;

	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return false;
	}

	CHECKIF(param->count == 0) {
		LOG_DBG("Invalid param->count: %u", param->count);
		return false;
	}

	CHECKIF(param->streams == NULL) {
		LOG_DBG("param->streams is NULL");
		return false;
	}

	CHECKIF(param->count > CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT) {
		LOG_DBG("param->count (%zu) is larger than "
			"CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT (%d)",
			param->count, CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT);
		return false;
	}

	for (size_t i = 0U; i < param->count; i++) {
		const struct bt_cap_stream *cap_stream = param->streams[i];
		const struct bt_bap_stream *bap_stream;
		struct bt_cap_common_client *client;
		struct bt_conn *conn;

		CHECKIF(cap_stream == NULL) {
			LOG_DBG("param->streams[%zu] is NULL", i);
			return false;
		}

		bap_stream = &cap_stream->bap_stream;
		conn = bap_stream->conn;
		CHECKIF(conn == NULL) {
			LOG_DBG("param->streams[%zu]->bap_stream.conn is NULL", i);

			return -EINVAL;
		}

		client = bt_cap_common_get_client_by_acl(conn);
		if (!client->cas_found) {
			LOG_DBG("CAS was not found for param->streams[%zu]", i);
			return false;
		}

		CHECKIF(bap_stream->group == NULL) {
			LOG_DBG("param->streams[%zu] is not in a unicast group", i);
			return false;
		}

		/* Use the group of the first stream for comparison */
		if (unicast_group == NULL) {
			unicast_group = bap_stream->group;
		} else {
			CHECKIF(bap_stream->group != unicast_group) {
				LOG_DBG("param->streams[%zu] is not in this group %p", i,
					unicast_group);
				return false;
			}
		}

		if (!can_release(bap_stream)) {
			LOG_DBG("Cannot stop param->streams[%zu]", i);

			return false;
		}

		for (size_t j = 0U; j < i; j++) {
			if (param->streams[j] == cap_stream) {
				LOG_DBG("param->stream_params[%zu] (%p) is "
					"duplicated by "
					"param->stream_params[%zu] (%p)",
					j, param->streams[j], i, cap_stream);
				return false;
			}
		}
	}

	return true;
}

int bt_cap_initiator_unicast_audio_stop(const struct bt_cap_unicast_audio_stop_param *param)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	struct bt_cap_initiator_proc_param *proc_param;
	struct bt_bap_stream *bap_stream;
	int err;

	if (bt_cap_common_proc_is_active()) {
		LOG_DBG("A CAP procedure is already in progress");

		return -EBUSY;
	}

	if (!valid_unicast_audio_stop_param(param)) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < param->count; i++) {
		struct bt_cap_stream *cap_stream = param->streams[i];

		active_proc->proc_param.initiator[i].stream = cap_stream;
	}

	bt_cap_common_start_proc(BT_CAP_COMMON_PROC_TYPE_STOP, param->count);

	bt_cap_common_set_subproc(BT_CAP_COMMON_SUBPROC_TYPE_RELEASE);

	/** TODO: If this is a CSIP set, then the order of the procedures may
	 * not match the order in the parameters, and the CSIP ordered access
	 * procedure should be used.
	 */
	proc_param = &active_proc->proc_param.initiator[0];
	bap_stream = &proc_param->stream->bap_stream;
	active_proc->proc_initiated_cnt++;

	err = bt_bap_stream_release(bap_stream);
	if (err != 0) {
		LOG_DBG("Failed to stop bap_stream %p: %d", proc_param->stream, err);

		bt_cap_common_clear_active_proc();
	}

	return err;
}

void bt_cap_initiator_released(struct bt_cap_stream *cap_stream)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();

	if (!bt_cap_common_stream_in_active_proc(cap_stream)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (!bt_cap_common_subproc_is_type(BT_CAP_COMMON_SUBPROC_TYPE_RELEASE)) {
		/* Unexpected callback - Abort */
		bt_cap_common_abort_proc(cap_stream->bap_stream.conn, -EBADMSG);
	} else {
		active_proc->proc_done_cnt++;

		LOG_DBG("Stream %p released (%zu/%zu streams done)", cap_stream,
			active_proc->proc_done_cnt, active_proc->proc_cnt);
	}

	if (bt_cap_common_proc_is_aborted()) {
		if (bt_cap_common_proc_all_handled()) {
			cap_initiator_unicast_audio_proc_complete();
		}

		return;
	}

	if (!bt_cap_common_proc_is_done()) {
		struct bt_cap_stream *next_cap_stream =
			active_proc->proc_param.initiator[active_proc->proc_done_cnt].stream;
		struct bt_bap_stream *bap_stream = &next_cap_stream->bap_stream;
		int err;

		active_proc->proc_initiated_cnt++;
		/* Since BAP operations may require a write long or a read long on the notification,
		 * we cannot assume that we can do multiple streams at once, thus do it one at a
		 * time.
		 * TODO: We should always be able to do one per ACL, so there is room for
		 * optimization.
		 */
		err = bt_bap_stream_release(bap_stream);
		if (err != 0) {
			LOG_DBG("Failed to release stream %p: %d", next_cap_stream, err);

			bt_cap_common_abort_proc(bap_stream->conn, err);
			cap_initiator_unicast_audio_proc_complete();
		}
	} else {
		cap_initiator_unicast_audio_proc_complete();
	}
}

#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE) && defined(CONFIG_BT_BAP_UNICAST_CLIENT)

int bt_cap_initiator_unicast_to_broadcast(
	const struct bt_cap_unicast_to_broadcast_param *param,
	struct bt_cap_broadcast_source **source)
{
	return -ENOSYS;
}

int bt_cap_initiator_broadcast_to_unicast(const struct bt_cap_broadcast_to_unicast_param *param,
					  struct bt_bap_unicast_group **unicast_group)
{
	return -ENOSYS;
}

#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE && CONFIG_BT_BAP_UNICAST_CLIENT */
