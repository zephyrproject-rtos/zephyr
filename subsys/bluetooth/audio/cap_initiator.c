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
#include "csip_internal.h"
#include "bap_endpoint.h"

#include <zephyr/logging/log.h>

BUILD_ASSERT(sizeof(struct bt_bap_broadcast_source_create_param) ==
		     sizeof(struct bt_cap_initiator_broadcast_create_param),
	     "Size of struct bt_bap_broadcast_source_create_param must equal "
	     "to struct bt_cap_initiator_broadcast_create_param");

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

static bool cap_initiator_valid_metadata(const struct bt_codec_data meta[],
					 size_t meta_count)
{
	bool stream_context_found;

	LOG_DBG("meta %p count %zu", meta, meta_count);

	/* Streaming Audio Context shall be present in CAP */
	stream_context_found = false;
	for (size_t i = 0U; i < meta_count; i++) {
		const struct bt_data *metadata = &meta[i].data;

		LOG_DBG("metadata %p type %u len %u data %s",
			metadata, metadata->type, metadata->data_len,
			bt_hex(metadata->data, metadata->data_len));

		if (metadata->type == BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT) {
			if (metadata->data_len != 2) { /* Stream context size */
				return false;
			}

			stream_context_found = true;
			break;
		}
	}

	if (!stream_context_found) {
		LOG_DBG("No streaming context supplied");
	}

	return stream_context_found;
}

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)

static bool cap_initiator_broadcast_audio_start_valid_param(
	const struct bt_cap_initiator_broadcast_create_param *param)
{

	for (size_t i = 0U; i < param->subgroup_count; i++) {
		const struct bt_cap_initiator_broadcast_subgroup_param *subgroup_param;
		const struct bt_codec *codec;
		bool valid_metadata;

		subgroup_param = &param->subgroup_params[i];
		codec = subgroup_param->codec;

		/* Streaming Audio Context shall be present in CAP */

		CHECKIF(codec == NULL) {
			LOG_DBG("subgroup[%zu]->codec is NULL", i);
			return false;
		}

		valid_metadata = cap_initiator_valid_metadata(codec->meta,
							      codec->meta_count);

		CHECKIF(!valid_metadata) {
			LOG_DBG("Invalid metadata supplied for subgroup[%zu]", i);
			return false;
		}
	}

	return true;
}


int bt_cap_initiator_broadcast_audio_start(struct bt_cap_initiator_broadcast_create_param *param,
					   struct bt_le_ext_adv *adv,
					   struct bt_cap_broadcast_source **broadcast_source)
{
	/* TODO: For now the create param and broadcast sources are
	 * identical, so we can just cast them. This need to be updated and
	 * made resistant to changes in either the CAP or BAP APIs at some point
	 */
	struct bt_bap_broadcast_source_create_param *bap_create_param =
		(struct bt_bap_broadcast_source_create_param *)param;
	struct bt_bap_broadcast_source **bap_broadcast_source =
		(struct bt_bap_broadcast_source **)broadcast_source;
	int err;

	if (!cap_initiator_broadcast_audio_start_valid_param(param)) {
		return -EINVAL;
	}

	CHECKIF(broadcast_source == NULL) {
		LOG_DBG("source is NULL");
		return -EINVAL;
	}

	err = bt_bap_broadcast_source_create(bap_create_param, bap_broadcast_source);
	if (err != 0) {
		LOG_DBG("Failed to create broadcast source: %d", err);
		return err;
	}

	err = bt_bap_broadcast_source_start(*bap_broadcast_source, adv);
	if (err != 0) {
		int del_err;

		LOG_DBG("Failed to start broadcast source: %d\n", err);

		del_err = bt_bap_broadcast_source_delete(*bap_broadcast_source);
		if (del_err) {
			LOG_ERR("Failed to delete BAP broadcast source: %d",
				del_err);
		}
	}

	return err;
}

int bt_cap_initiator_broadcast_audio_update(struct bt_cap_broadcast_source *broadcast_source,
					    const struct bt_codec_data meta[],
					    size_t meta_count)
{
	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	if (!cap_initiator_valid_metadata(meta, meta_count)) {
		LOG_DBG("Invalid metadata");
		return -EINVAL;
	}

	return bt_bap_broadcast_source_update_metadata(
		(struct bt_bap_broadcast_source *)broadcast_source, meta, meta_count);
}

int bt_cap_initiator_broadcast_audio_stop(struct bt_cap_broadcast_source *broadcast_source)
{
	return bt_bap_broadcast_source_stop((struct bt_bap_broadcast_source *)broadcast_source);
}

int bt_cap_initiator_broadcast_audio_delete(struct bt_cap_broadcast_source *broadcast_source)
{
	return bt_bap_broadcast_source_delete((struct bt_bap_broadcast_source *)broadcast_source);
}

int bt_cap_initiator_broadcast_get_id(const struct bt_cap_broadcast_source *source,
				      uint32_t *const broadcast_id)
{
	return bt_bap_broadcast_source_get_id((struct bt_bap_broadcast_source *)source,
					      broadcast_id);
}

int bt_cap_initiator_broadcast_get_base(struct bt_cap_broadcast_source *source,
					struct net_buf_simple *base_buf)
{
	return bt_bap_broadcast_source_get_base((struct bt_bap_broadcast_source *)source, base_buf);
}

#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
enum {
	CAP_UNICAST_PROC_STATE_ACTIVE,
	CAP_UNICAST_PROC_STATE_ABORTED,

	CAP_UNICAST_PROC_STATE_FLAG_NUM,
} cap_unicast_proc_state;

enum cap_unicast_subproc_type {
	CAP_UNICAST_SUBPROC_TYPE_NONE,
	CAP_UNICAST_SUBPROC_TYPE_CODEC_CONFIG,
	CAP_UNICAST_SUBPROC_TYPE_QOS_CONFIG,
	CAP_UNICAST_SUBPROC_TYPE_ENABLE,
	CAP_UNICAST_SUBPROC_TYPE_START,
	CAP_UNICAST_SUBPROC_TYPE_META_UPDATE,
	CAP_UNICAST_SUBPROC_TYPE_RELEASE,
};

struct cap_unicast_proc {
	ATOMIC_DEFINE(proc_state_flags, CAP_UNICAST_PROC_STATE_FLAG_NUM);
	/* Total number of streams in the procedure*/
	size_t stream_cnt;
	/* Number of streams where a subprocedure have been started */
	size_t stream_initiated_cnt;
	/* Number of streams done with the procedure */
	size_t stream_done_cnt;
	enum cap_unicast_subproc_type subproc_type;
	int err;
	struct bt_conn *failed_conn;
	struct bt_cap_stream *streams[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_bap_unicast_group *unicast_group;
};

struct cap_unicast_client {
	struct bt_conn *conn;
	struct bt_gatt_discover_params param;
	uint16_t csis_start_handle;
	const struct bt_csip_set_coordinator_csis_inst *csis_inst;
	bool cas_found;
};

static struct cap_unicast_client bt_cap_unicast_clients[CONFIG_BT_MAX_CONN];
static const struct bt_uuid *cas_uuid = BT_UUID_CAS;
static struct cap_unicast_proc active_proc;

static void cap_set_subproc(enum cap_unicast_subproc_type subproc_type)
{
	active_proc.stream_done_cnt = 0U;
	active_proc.stream_initiated_cnt = 0U;
	active_proc.subproc_type = subproc_type;
}

static bool cap_proc_is_active(void)
{
	return atomic_test_bit(active_proc.proc_state_flags, CAP_UNICAST_PROC_STATE_ACTIVE);
}

static bool cap_proc_is_aborted(void)
{
	return atomic_test_bit(active_proc.proc_state_flags, CAP_UNICAST_PROC_STATE_ABORTED);
}

static bool cap_proc_all_streams_handled(void)
{
	return active_proc.stream_done_cnt == active_proc.stream_initiated_cnt;
}

static bool cap_proc_is_done(void)
{
	return active_proc.stream_done_cnt == active_proc.stream_cnt;
}

static void cap_abort_proc(struct bt_conn *conn, int err)
{
	if (cap_proc_is_aborted()) {
		/* no-op */
		return;
	}

	active_proc.err = err;
	active_proc.failed_conn = conn;
	atomic_set_bit(active_proc.proc_state_flags, CAP_UNICAST_PROC_STATE_ABORTED);
}

static bool cap_conn_in_active_proc(const struct bt_conn *conn)
{
	if (!cap_proc_is_active()) {
		return false;
	}

	for (size_t i = 0U; i < active_proc.stream_initiated_cnt; i++) {
		if (active_proc.streams[i]->bap_stream.conn == conn) {
			return true;
		}
	}

	return false;
}

static void cap_initiator_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct cap_unicast_client *client;

	client = &bt_cap_unicast_clients[bt_conn_index(conn)];

	if (client->conn != NULL) {
		bt_conn_unref(client->conn);
	}
	(void)memset(client, 0, sizeof(*client));

	if (cap_conn_in_active_proc(conn)) {
		cap_abort_proc(conn, -ENOTCONN);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = cap_initiator_disconnected,
};

static struct cap_unicast_client *lookup_unicast_client_by_csis(
	const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (csis_inst == NULL) {
		return NULL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(bt_cap_unicast_clients); i++) {
		struct cap_unicast_client *client = &bt_cap_unicast_clients[i];

		if (client->csis_inst == csis_inst) {
			return client;
		}
	}

	return NULL;
}

static void csis_client_discover_cb(struct bt_conn *conn,
				    const struct bt_csip_set_coordinator_set_member *member,
				    int err, size_t set_count)
{
	struct cap_unicast_client *client;

	if (err != 0) {
		LOG_DBG("CSIS client discover failed: %d", err);

		if (cap_cb && cap_cb->unicast_discovery_complete) {
			cap_cb->unicast_discovery_complete(conn, err, NULL);
		}

		return;
	}

	client = &bt_cap_unicast_clients[bt_conn_index(conn)];
	client->csis_inst = bt_csip_set_coordinator_csis_inst_by_handle(
					conn, client->csis_start_handle);

	if (member == NULL || set_count == 0 || client->csis_inst == NULL) {
		LOG_ERR("Unable to find CSIS for CAS");

		if (cap_cb && cap_cb->unicast_discovery_complete) {
			cap_cb->unicast_discovery_complete(conn, -ENODATA,
							   NULL);
		}
	} else {
		LOG_DBG("Found CAS with CSIS");
		if (cap_cb && cap_cb->unicast_discovery_complete) {
			cap_cb->unicast_discovery_complete(conn, 0,
							   client->csis_inst);
		}
	}
}

static uint8_t cap_unicast_discover_included_cb(struct bt_conn *conn,
						const struct bt_gatt_attr *attr,
						struct bt_gatt_discover_params *params)
{
	params->func = NULL;

	if (attr == NULL) {
		LOG_DBG("CAS CSIS include not found");

		if (cap_cb && cap_cb->unicast_discovery_complete) {
			cap_cb->unicast_discovery_complete(conn, 0, NULL);
		}
	} else {
		const struct bt_gatt_include *included_service = attr->user_data;
		struct cap_unicast_client *client = CONTAINER_OF(params,
								 struct cap_unicast_client,
								 param);

		/* If the remote CAS includes CSIS, we first check if we
		 * have already discovered it, and if so we can just retrieve it
		 * and forward it to the application. If not, then we start
		 * CSIS discovery
		 */
		client->csis_start_handle = included_service->start_handle;
		client->csis_inst = bt_csip_set_coordinator_csis_inst_by_handle(
					conn, client->csis_start_handle);

		if (client->csis_inst == NULL) {
			static struct bt_csip_set_coordinator_cb csis_client_cb = {
				.discover = csis_client_discover_cb
			};
			static bool csis_cbs_registered;
			int err;

			LOG_DBG("CAS CSIS not known, discovering");

			if (!csis_cbs_registered) {
				bt_csip_set_coordinator_register_cb(&csis_client_cb);
				csis_cbs_registered = true;
			}

			err = bt_csip_set_coordinator_discover(conn);
			if (err != 0) {
				LOG_DBG("Discover failed (err %d)", err);
				if (cap_cb && cap_cb->unicast_discovery_complete) {
					cap_cb->unicast_discovery_complete(conn,
									   err,
									   NULL);
				}
			}
		} else if (cap_cb && cap_cb->unicast_discovery_complete) {
			LOG_DBG("Found CAS with CSIS");
			cap_cb->unicast_discovery_complete(conn, 0,
							   client->csis_inst);
		}
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t cap_unicast_discover_cas_cb(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr,
				       struct bt_gatt_discover_params *params)
{
	params->func = NULL;

	if (attr == NULL) {
		if (cap_cb && cap_cb->unicast_discovery_complete) {
			cap_cb->unicast_discovery_complete(conn, -ENODATA,
							   NULL);
		}
	} else {
		const struct bt_gatt_service_val *prim_service = attr->user_data;
		struct cap_unicast_client *client = CONTAINER_OF(params,
								 struct cap_unicast_client,
								 param);
		int err;

		client->cas_found = true;
		client->conn = bt_conn_ref(conn);

		if (attr->handle == prim_service->end_handle) {
			LOG_DBG("Found CAS without CSIS");
			cap_cb->unicast_discovery_complete(conn, 0, NULL);

			return BT_GATT_ITER_STOP;
		}

		LOG_DBG("Found CAS, discovering included CSIS");

		params->uuid = NULL;
		params->start_handle = attr->handle + 1;
		params->end_handle = prim_service->end_handle;
		params->type = BT_GATT_DISCOVER_INCLUDE;
		params->func = cap_unicast_discover_included_cb;

		err = bt_gatt_discover(conn, params);
		if (err != 0) {
			LOG_DBG("Discover failed (err %d)", err);

			params->func = NULL;
			if (cap_cb && cap_cb->unicast_discovery_complete) {
				cap_cb->unicast_discovery_complete(conn, err,
								   NULL);
			}
		}
	}

	return BT_GATT_ITER_STOP;
}

int bt_cap_initiator_unicast_discover(struct bt_conn *conn)
{
	struct bt_gatt_discover_params *param;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	param = &bt_cap_unicast_clients[bt_conn_index(conn)].param;

	/* use param->func to tell if a client is "busy" */
	if (param->func != NULL) {
		return -EBUSY;
	}

	param->func = cap_unicast_discover_cas_cb;
	param->uuid = cas_uuid;
	param->type = BT_GATT_DISCOVER_PRIMARY;
	param->start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	param->end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	err = bt_gatt_discover(conn, param);
	if (err != 0) {
		param->func = NULL;
	}

	return err;
}

static bool cap_stream_in_active_proc(const struct bt_cap_stream *cap_stream)
{
	if (!cap_proc_is_active()) {
		return false;
	}

	for (size_t i = 0U; i < active_proc.stream_cnt; i++) {
		if (active_proc.streams[i] == cap_stream) {
			return true;
		}
	}

	return false;
}

static bool valid_unicast_audio_start_param(const struct bt_cap_unicast_audio_start_param *param,
					    struct bt_bap_unicast_group *unicast_group)
{
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
		const struct bt_codec *codec = stream_param->codec;
		const struct bt_bap_stream *bap_stream;

		CHECKIF(stream_param->codec == NULL) {
			LOG_DBG("param->stream_params[%zu].codec is NULL", i);
			return false;
		}

		CHECKIF(!cap_initiator_valid_metadata(codec->meta,
						      codec->meta_count)) {
			LOG_DBG("param->stream_params[%zu].codec is invalid",
				i);
			return false;
		}

		CHECKIF(stream_param->ep == NULL) {
			LOG_DBG("param->stream_params[%zu].ep is NULL", i);
			return false;
		}

		CHECKIF(stream_param->qos == NULL) {
			LOG_DBG("param->stream_params[%zu].qos is NULL", i);
			return false;
		}

		CHECKIF(member == NULL) {
			LOG_DBG("param->stream_params[%zu].member is NULL", i);
			return false;
		}

		if (param->type == BT_CAP_SET_TYPE_AD_HOC) {
			struct cap_unicast_client *client;

			CHECKIF(member->member == NULL) {
				LOG_DBG("param->members[%zu] is NULL", i);
				return false;
			}

			client = &bt_cap_unicast_clients[bt_conn_index(member->member)];

			if (!client->cas_found) {
				LOG_DBG("CAS was not found for param->members[%zu]", i);
				return false;
			}
		}

		if (param->type == BT_CAP_SET_TYPE_CSIP) {
			struct cap_unicast_client *client;

			CHECKIF(member->csip == NULL) {
				LOG_DBG("param->csip.set[%zu] is NULL", i);
				return false;
			}

			client = lookup_unicast_client_by_csis(member->csip);
			if (client == NULL) {
				LOG_DBG("CSIS was not found for param->members[%zu]", i);
				return false;
			}
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

		CHECKIF(bap_stream->group != unicast_group) {
			LOG_DBG("param->streams[%zu] is not in this group %p", i, unicast_group);
			return false;
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

static int cap_initiator_unicast_audio_configure(
	const struct bt_cap_unicast_audio_start_param *param)
{
	/** TODO: If this is a CSIP set, then the order of the procedures may
	 * not match the order in the parameters, and the CSIP ordered access
	 * procedure should be used.
	 */

	/* Store the information about the active procedure so that we can
	 * continue the procedure after each step
	 */
	atomic_set_bit(active_proc.proc_state_flags, CAP_UNICAST_PROC_STATE_ACTIVE);
	active_proc.stream_cnt = param->count;

	cap_set_subproc(CAP_UNICAST_SUBPROC_TYPE_CODEC_CONFIG);

	for (size_t i = 0U; i < param->count; i++) {
		struct bt_cap_unicast_audio_start_stream_param *stream_param =
									&param->stream_params[i];
		union bt_cap_set_member *member = &stream_param->member;
		struct bt_cap_stream *cap_stream = stream_param->stream;
		struct bt_bap_stream *bap_stream = &cap_stream->bap_stream;
		struct bt_bap_ep *ep = stream_param->ep;
		struct bt_codec *codec = stream_param->codec;
		struct bt_conn *conn;
		int err;

		if (param->type == BT_CAP_SET_TYPE_AD_HOC) {
			conn = member->member;
		} else {
			struct cap_unicast_client *client;

			/* We have verified that `client` wont be NULL in
			 * `valid_unicast_audio_start_param`.
			 */
			client = lookup_unicast_client_by_csis(member->csip);
			__ASSERT(client != NULL, "client is NULL");
			conn = client->conn;
		}

		/* Ensure that ops are registered before any procedures are started */
		bt_cap_stream_ops_register_bap(cap_stream);

		active_proc.streams[i] = cap_stream;

		err = bt_bap_stream_config(conn, bap_stream, ep, codec);
		if (err != 0) {
			LOG_DBG("bt_bap_stream_config failed for param->stream_params[%zu]: %d",
				i, err);

			if (i > 0U) {
				cap_abort_proc(bap_stream->conn, err);
			} else {
				(void)memset(&active_proc, 0, sizeof(active_proc));
			}

			return err;
		}

		active_proc.stream_initiated_cnt++;
	}

	return 0;
}

static void cap_initiator_unicast_audio_start_complete(void)
{
	struct bt_bap_unicast_group *unicast_group;
	struct bt_conn *failed_conn;
	int err;

	/* All streams in the procedure share the same unicast group, so we just
	 * use the reference from the first stream
	 */
	unicast_group = (struct bt_bap_unicast_group *)active_proc.streams[0]->bap_stream.group;
	failed_conn = active_proc.failed_conn;
	err = active_proc.err;

	(void)memset(&active_proc, 0, sizeof(active_proc));
	if (cap_cb != NULL && cap_cb->unicast_start_complete != NULL) {
		cap_cb->unicast_start_complete(unicast_group, err, failed_conn);
	}
}

int bt_cap_initiator_unicast_audio_start(const struct bt_cap_unicast_audio_start_param *param,
					 struct bt_bap_unicast_group *unicast_group)
{
	if (cap_proc_is_active()) {
		LOG_DBG("A CAP procedure is already in progress");

		return -EBUSY;
	}

	CHECKIF(unicast_group == NULL) {
		LOG_DBG("unicast_group is NULL");
		return -EINVAL;
	}

	if (!valid_unicast_audio_start_param(param, unicast_group)) {
		return -EINVAL;
	}

	active_proc.unicast_group = unicast_group;

	return cap_initiator_unicast_audio_configure(param);
}

void bt_cap_initiator_codec_configured(struct bt_cap_stream *cap_stream)
{
	struct bt_conn *conns[MIN(CONFIG_BT_MAX_CONN,
				  CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT)];
	struct bt_bap_unicast_group *unicast_group;

	if (!cap_stream_in_active_proc(cap_stream)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (active_proc.subproc_type == CAP_UNICAST_SUBPROC_TYPE_RELEASE) {
		/* When releasing a stream, it may go into the codec configured state if
		 * the unicast server caches the configuration - We treat it as a release
		 */
		bt_cap_initiator_released(cap_stream);
		return;
	} else if (active_proc.subproc_type != CAP_UNICAST_SUBPROC_TYPE_CODEC_CONFIG) {
		/* Unexpected callback - Abort */
		cap_abort_proc(cap_stream->bap_stream.conn, -EBADMSG);
	} else {
		active_proc.stream_done_cnt++;

		LOG_DBG("Stream %p configured (%zu/%zu streams done)",
			cap_stream, active_proc.stream_done_cnt,
			active_proc.stream_cnt);

		if (!cap_proc_is_done()) {
			/* Not yet finished, wait for all */
			return;
		}
	}

	if (cap_proc_is_aborted()) {
		if (cap_proc_all_streams_handled()) {
			cap_initiator_unicast_audio_start_complete();
		}

		return;
	}

	/* The QoS Configure procedure works on a set of connections and a
	 * unicast group, so we generate a list of unique connection pointers
	 * for the procedure
	 */
	(void)memset(conns, 0, sizeof(conns));
	for (size_t i = 0U; i < active_proc.stream_cnt; i++) {
		struct bt_conn *stream_conn = active_proc.streams[i]->bap_stream.conn;
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

		__ASSERT(free_conn, "No free conns");
		*free_conn = stream_conn;
	}

	/* All streams in the procedure share the same unicast group, so we just
	 * use the reference from the first stream
	 */
	unicast_group = (struct bt_bap_unicast_group *)active_proc.streams[0]->bap_stream.group;
	cap_set_subproc(CAP_UNICAST_SUBPROC_TYPE_QOS_CONFIG);

	for (size_t i = 0U; i < ARRAY_SIZE(conns); i++) {
		int err;

		/* When conns[i] is NULL, we have QoS Configured all unique connections */
		if (conns[i] == NULL) {
			break;
		}

		err = bt_bap_stream_qos(conns[i], unicast_group);
		if (err != 0) {
			LOG_DBG("Failed to set stream QoS for conn %p and group %p: %d",
				(void *)conns[i], unicast_group, err);

			/* End or mark procedure as aborted.
			 * If we have sent any requests over air, we will abort
			 * once all sent requests has completed
			 */
			cap_abort_proc(conns[i], err);
			if (i == 0U) {
				cap_initiator_unicast_audio_start_complete();
			}

			return;
		}

		active_proc.stream_initiated_cnt++;
	}
}

void bt_cap_initiator_qos_configured(struct bt_cap_stream *cap_stream)
{
	if (!cap_stream_in_active_proc(cap_stream)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (active_proc.subproc_type != CAP_UNICAST_SUBPROC_TYPE_QOS_CONFIG) {
		/* Unexpected callback - Abort */
		cap_abort_proc(cap_stream->bap_stream.conn, -EBADMSG);
	} else {
		active_proc.stream_done_cnt++;

		LOG_DBG("Stream %p QoS configured (%zu/%zu streams done)",
			cap_stream, active_proc.stream_done_cnt,
			active_proc.stream_cnt);

		if (!cap_proc_is_done()) {
			/* Not yet finished, wait for all */
			return;
		}
	}

	if (cap_proc_is_aborted()) {
		if (cap_proc_all_streams_handled()) {
			cap_initiator_unicast_audio_start_complete();
		}

		return;
	}

	cap_set_subproc(CAP_UNICAST_SUBPROC_TYPE_ENABLE);

	for (size_t i = 0U; i < active_proc.stream_cnt; i++) {
		struct bt_cap_stream *cap_stream = active_proc.streams[i];
		struct bt_bap_stream *bap_stream = &cap_stream->bap_stream;
		int err;

		/* TODO: Add metadata */
		err = bt_bap_stream_enable(bap_stream,
					     bap_stream->codec->meta,
					     bap_stream->codec->meta_count);
		if (err != 0) {
			LOG_DBG("Failed to enable stream %p: %d",
				cap_stream, err);

			/* End or mark procedure as aborted.
			 * If we have sent any requests over air, we will abort
			 * once all sent requests has completed
			 */
			cap_abort_proc(bap_stream->conn, err);
			if (i == 0U) {
				cap_initiator_unicast_audio_start_complete();
			}

			return;
		}
	}
}

void bt_cap_initiator_enabled(struct bt_cap_stream *cap_stream)
{
	struct bt_bap_stream *bap_stream;
	int err;

	if (!cap_stream_in_active_proc(cap_stream)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (active_proc.subproc_type != CAP_UNICAST_SUBPROC_TYPE_ENABLE) {
		/* Unexpected callback - Abort */
		cap_abort_proc(cap_stream->bap_stream.conn, -EBADMSG);
	} else {
		active_proc.stream_done_cnt++;

		LOG_DBG("Stream %p enabled (%zu/%zu streams done)",
			cap_stream, active_proc.stream_done_cnt,
			active_proc.stream_cnt);

		if (!cap_proc_is_done()) {
			/* Not yet finished, wait for all */
			return;
		}
	}

	if (cap_proc_is_aborted()) {
		if (cap_proc_all_streams_handled()) {
			cap_initiator_unicast_audio_start_complete();
		}

		return;
	}

	cap_set_subproc(CAP_UNICAST_SUBPROC_TYPE_START);

	bap_stream = &active_proc.streams[0]->bap_stream;

	/* Since bt_bap_stream_start connects the ISO, we can, at this point,
	 * only do this one by one due to a restriction in the ISO layer
	 * (maximum 1 outstanding ISO connection request at any one time).
	 */
	err = bt_bap_stream_start(bap_stream);
	if (err != 0) {
		LOG_DBG("Failed to start stream %p: %d", active_proc.streams[0], err);

		/* End and mark procedure as aborted.
		 * If we have sent any requests over air, we will abort
		 * once all sent requests has completed
		 */
		cap_abort_proc(bap_stream->conn, err);
		cap_initiator_unicast_audio_start_complete();

		return;
	}
}

void bt_cap_initiator_started(struct bt_cap_stream *cap_stream)
{
	if (!cap_stream_in_active_proc(cap_stream)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (active_proc.subproc_type != CAP_UNICAST_SUBPROC_TYPE_START) {
		/* Unexpected callback - Abort */
		cap_abort_proc(cap_stream->bap_stream.conn, -EBADMSG);
	} else {
		active_proc.stream_done_cnt++;

		LOG_DBG("Stream %p started (%zu/%zu streams done)",
			cap_stream, active_proc.stream_done_cnt,
			active_proc.stream_cnt);
	}

	/* Since bt_bap_stream_start connects the ISO, we can, at this point,
	 * only do this one by one due to a restriction in the ISO layer
	 * (maximum 1 outstanding ISO connection request at any one time).
	 */
	if (!cap_proc_is_done()) {
		struct bt_cap_stream *next = active_proc.streams[active_proc.stream_done_cnt];
		struct bt_bap_stream *bap_stream = &next->bap_stream;
		int err;

		/* Not yet finished, start next */
		err = bt_bap_stream_start(bap_stream);
		if (err != 0) {
			LOG_DBG("Failed to start stream %p: %d", next, err);

			/* End and mark procedure as aborted.
			 * If we have sent any requests over air, we will abort
			 * once all sent requests has completed
			 */
			cap_abort_proc(bap_stream->conn, err);
			cap_initiator_unicast_audio_start_complete();
		}
	} else {
		cap_initiator_unicast_audio_start_complete();
	}
}

static void cap_initiator_unicast_audio_update_complete(void)
{
	struct bt_conn *failed_conn;
	int err;

	failed_conn = active_proc.failed_conn;
	err = active_proc.err;

	(void)memset(&active_proc, 0, sizeof(active_proc));
	if (cap_cb != NULL && cap_cb->unicast_update_complete != NULL) {
		cap_cb->unicast_update_complete(err, failed_conn);
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

int bt_cap_initiator_unicast_audio_update(const struct bt_cap_unicast_audio_update_param params[],
					  size_t count)
{
	CHECKIF(params == NULL) {
		LOG_DBG("params is NULL");

		return -EINVAL;
	}

	CHECKIF(count == 0) {
		LOG_DBG("count is 0");

		return -EINVAL;
	}

	if (cap_proc_is_active()) {
		LOG_DBG("A CAP procedure is already in progress");

		return -EBUSY;
	}

	for (size_t i = 0U; i < count; i++) {
		CHECKIF(params[i].stream == NULL) {
			LOG_DBG("params[%zu].stream is NULL", i);

			return -EINVAL;
		}

		CHECKIF(params[i].stream->bap_stream.conn == NULL) {
			LOG_DBG("params[%zu].stream->bap_stream.conn is NULL", i);

			return -EINVAL;
		}

		CHECKIF(!cap_initiator_valid_metadata(params[i].meta,
						      params[i].meta_count)) {
			LOG_DBG("params[%zu].meta is invalid", i);

			return -EINVAL;
		}

		for (size_t j = 0U; j < i; j++) {
			if (params[j].stream == params[i].stream) {
				LOG_DBG("param.streams[%zu] is duplicated by param.streams[%zu]",
					j, i);
				return -EINVAL;
			}
		}

		if (!can_update_metadata(&params[i].stream->bap_stream)) {
			LOG_DBG("params[%zu].stream is not in right state to be updated", i);

			return -EINVAL;
		}
	}

	atomic_set_bit(active_proc.proc_state_flags,
		       CAP_UNICAST_PROC_STATE_ACTIVE);
	active_proc.stream_cnt = count;

	cap_set_subproc(CAP_UNICAST_SUBPROC_TYPE_META_UPDATE);

	/** TODO: If this is a CSIP set, then the order of the procedures may
	 * not match the order in the parameters, and the CSIP ordered access
	 * procedure should be used.
	 */
	for (size_t i = 0U; i < count; i++) {
		int err;

		active_proc.streams[i] = params[i].stream;

		err = bt_bap_stream_metadata(&params[i].stream->bap_stream, params[i].meta,
					     params[i].meta_count);
		if (err != 0) {
			LOG_DBG("Failed to update metadata for stream %p: %d",
				params[i].stream, err);

			if (i > 0U) {
				cap_abort_proc(params[i].stream->bap_stream.conn, err);
			} else {
				(void)memset(&active_proc, 0,
					     sizeof(active_proc));
			}

			return err;
		}

		active_proc.stream_initiated_cnt++;
	}

	return 0;
}

void bt_cap_initiator_metadata_updated(struct bt_cap_stream *cap_stream)
{
	if (!cap_stream_in_active_proc(cap_stream)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (active_proc.subproc_type != CAP_UNICAST_SUBPROC_TYPE_META_UPDATE) {
		/* Unexpected callback - Abort */
		cap_abort_proc(cap_stream->bap_stream.conn, -EBADMSG);
	} else {
		active_proc.stream_done_cnt++;

		LOG_DBG("Stream %p QoS metadata updated (%zu/%zu streams done)",
			cap_stream, active_proc.stream_done_cnt,
			active_proc.stream_cnt);
	}

	if (!cap_proc_is_done()) {
		/* Not yet finished, wait for all */
		return;
	} else if (cap_proc_is_aborted()) {
		if (cap_proc_all_streams_handled()) {
			cap_initiator_unicast_audio_update_complete();
		}

		return;
	}

	cap_initiator_unicast_audio_update_complete();
}

static void cap_initiator_unicast_audio_stop_complete(void)
{
	struct bt_bap_unicast_group *unicast_group;
	struct bt_conn *failed_conn;
	int err;

	unicast_group = active_proc.unicast_group;
	failed_conn = active_proc.failed_conn;
	err = active_proc.err;

	(void)memset(&active_proc, 0, sizeof(active_proc));

	if (cap_cb != NULL && cap_cb->unicast_stop_complete != NULL) {
		cap_cb->unicast_stop_complete(unicast_group, err, failed_conn);
	}
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

int bt_cap_initiator_unicast_audio_stop(struct bt_bap_unicast_group *unicast_group)
{
	struct bt_bap_stream *bap_stream;
	size_t stream_cnt;

	if (cap_proc_is_active()) {
		LOG_DBG("A CAP procedure is already in progress");

		return -EBUSY;
	}

	CHECKIF(unicast_group == NULL) {
		LOG_DBG("unicast_group is NULL");
		return -EINVAL;
	}

	stream_cnt = 0U;
	SYS_SLIST_FOR_EACH_CONTAINER(&unicast_group->streams, bap_stream, _node) {
		if (can_release(bap_stream)) {
			stream_cnt++;
		}
	}

	if (stream_cnt == 0U) {
		LOG_DBG("All streams are already stopped");

		return -EALREADY;
	}

	atomic_set_bit(active_proc.proc_state_flags,
		       CAP_UNICAST_PROC_STATE_ACTIVE);
	active_proc.stream_cnt = stream_cnt;
	active_proc.unicast_group = unicast_group;

	cap_set_subproc(CAP_UNICAST_SUBPROC_TYPE_RELEASE);

	/** TODO: If this is a CSIP set, then the order of the procedures may
	 * not match the order in the parameters, and the CSIP ordered access
	 * procedure should be used.
	 */
	SYS_SLIST_FOR_EACH_CONTAINER(&unicast_group->streams, bap_stream, _node) {
		struct bt_cap_stream *cap_stream =
			CONTAINER_OF(bap_stream, struct bt_cap_stream, bap_stream);
		int err;

		if (!can_release(bap_stream)) {
			continue;
		}

		active_proc.streams[active_proc.stream_initiated_cnt] = cap_stream;

		err = bt_bap_stream_release(bap_stream);
		if (err != 0) {
			LOG_DBG("Failed to stop bap_stream %p: %d", bap_stream, err);

			if (active_proc.stream_initiated_cnt > 0U) {
				cap_abort_proc(bap_stream->conn, err);
			} else {
				(void)memset(&active_proc, 0,
					     sizeof(active_proc));
			}

			return err;
		}

		active_proc.stream_initiated_cnt++;
	}

	return 0;
}

void bt_cap_initiator_released(struct bt_cap_stream *cap_stream)
{
	if (!cap_stream_in_active_proc(cap_stream)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (active_proc.subproc_type != CAP_UNICAST_SUBPROC_TYPE_RELEASE) {
		/* Unexpected callback - Abort */
		cap_abort_proc(cap_stream->bap_stream.conn, -EBADMSG);
	} else {
		active_proc.stream_done_cnt++;

		LOG_DBG("Stream %p released (%zu/%zu streams done)",
			cap_stream, active_proc.stream_done_cnt,
			active_proc.stream_cnt);
	}

	if (!cap_proc_is_done()) {
		/* Not yet finished, wait for all */
		return;
	} else if (cap_proc_is_aborted()) {
		if (cap_proc_all_streams_handled()) {
			cap_initiator_unicast_audio_stop_complete();
		}

		return;
	}

	cap_initiator_unicast_audio_stop_complete();
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
