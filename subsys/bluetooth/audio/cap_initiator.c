/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/check.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio/tbs.h>
#include <bluetooth/audio/cap.h>
#include "cap_internal.h"
#include "csis_internal.h"
#include "endpoint.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_CAP_INITIATOR)
#define LOG_MODULE_NAME bt_cap_initiator
#include "common/log.h"

static const struct bt_cap_initiator_cb *cap_cb;

int bt_cap_initiator_register_cb(const struct bt_cap_initiator_cb *cb)
{
	CHECKIF(cb == NULL) {
		BT_DBG("cb is NULL");
		return -EINVAL;
	}

	CHECKIF(cap_cb != NULL) {
		BT_DBG("callbacks already registered");
		return -EALREADY;
	}

	cap_cb = cb;

	return 0;
}

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)

static bool cap_initiator_valid_metadata(const struct bt_codec_data meta[],
					 size_t meta_count)
{
	bool stream_context_found;

	/* Streaming Audio Context shall be present in CAP */
	stream_context_found = false;
	for (size_t i = 0U; i < meta_count; i++) {
		const struct bt_data *metadata = &meta[i].data;

		if (metadata->type == BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT) {
			if (metadata->data_len != 2) { /* Stream context size */
				return false;
			}

			stream_context_found = true;
			break;
		}
	}

	if (!stream_context_found) {
		BT_DBG("No streaming context supplied");
	}

	return stream_context_found;
}

static bool cap_initiator_broadcast_source_create_valid_param(
	const struct bt_cap_broadcast_audio_start_param *param)
{
	bool valid_metadata;

	CHECKIF(param == NULL) {
		BT_DBG("param is NULL");
		return false;
	}

	CHECKIF(!IN_RANGE(param->count, 0,
			  MIN(BT_ISO_MAX_GROUP_ISO_COUNT,
			      CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT))) {
		BT_DBG("Invalid stream count: %zu", param->count);
		return false;
	}

	CHECKIF(param->streams == NULL) {
		BT_DBG("param->streams is NULL");
		return false;
	}

	CHECKIF(param->codec == NULL) {
		BT_DBG("param->streams is NULL");
		return false;
	}

	CHECKIF(param->qos == NULL) {
		BT_DBG("param->streams is NULL");
		return false;
	}

	CHECKIF(param->codec->meta == NULL) {
		BT_DBG("param->codec->meta is NULL");
		return false;
	}

	valid_metadata = cap_initiator_valid_metadata(param->codec->meta,
						      param->codec->meta_count);

	if (!valid_metadata) {
		BT_DBG("Invalid metadata supplied");
	}

	return valid_metadata;
}

int bt_cap_initiator_broadcast_source_create(
	const struct bt_cap_broadcast_source_create_param *param,
	struct bt_cap_broadcast_source **broadcast_source)
{
	int err;

	if (!cap_initiator_broadcast_source_create_valid_param(param)) {
		return -EINVAL;
	}

	CHECKIF(broadcast_source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	err = bt_audio_broadcast_source_create(
		param->streams, param->count, param->codec, param->qos,
		(struct bt_audio_broadcast_source **)broadcast_source);
	if (err != 0) {
		BT_DBG("Failed to create broadcast source: %d", err);
	}

	return err;
}

int bt_cap_initiator_broadcast_audio_start(struct bt_cap_broadcast_source *broadcast_source)
{
	int err;

	err = bt_audio_broadcast_source_start((struct bt_audio_broadcast_source *)broadcast_source);
	if (err != 0) {
		BT_DBG("Failed to start broadcast source: %d\n", err);
	}

	return err;
}

int bt_cap_initiator_broadcast_audio_update(struct bt_cap_broadcast_source *broadcast_source,
					    const struct bt_codec_data meta[],
					    size_t meta_count)
{
	CHECKIF(meta == NULL) {
		BT_DBG("meta is NULL");
		return -EINVAL;
	}

	if (!cap_initiator_valid_metadata(meta, meta_count)) {
		BT_DBG("Invalid metadata");
		return -EINVAL;
	}

	return bt_audio_broadcast_source_metadata(
		(struct bt_audio_broadcast_source *)broadcast_source,
		meta, meta_count);
}

int bt_cap_initiator_broadcast_audio_stop(struct bt_cap_broadcast_source *broadcast_source)
{
	return -ENOSYS;
}

#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
static const struct bt_uuid *cas_uuid = BT_UUID_CAS;

struct cap_unicast_client {
	struct bt_gatt_discover_params param;
	uint16_t csis_start_handle;
	const struct bt_csis_client_csis_inst *csis_inst;
};

static struct cap_unicast_client bt_cap_unicast_clients[CONFIG_BT_MAX_CONN];

static void csis_client_discover_cb(struct bt_conn *conn,
				    const struct bt_csis_client_set_member *member,
				    int err, size_t set_count)
{
	struct cap_unicast_client *client;

	if (err != 0) {
		BT_DBG("CSIS client discover failed: %d", err);

		if (cap_cb && cap_cb->discovery_complete) {
			cap_cb->discovery_complete(conn, err, NULL);
		}

		return;
	}

	client = &bt_cap_unicast_clients[bt_conn_index(conn)];
	client->csis_inst = bt_csis_client_csis_inst_by_handle(
					conn, client->csis_start_handle);

	if (member == NULL || set_count == 0 || client->csis_inst == NULL) {
		BT_ERR("Unable to find CSIS for CAS");

		if (cap_cb && cap_cb->discovery_complete) {
			cap_cb->discovery_complete(conn, -ENODATA, NULL);
		}
	} else {
		BT_DBG("Found CAS with CSIS");
		if (cap_cb && cap_cb->discovery_complete) {
			cap_cb->discovery_complete(conn, 0, client->csis_inst);
		}
	}
}

static uint8_t cap_unicast_discover_included_cb(struct bt_conn *conn,
						const struct bt_gatt_attr *attr,
						struct bt_gatt_discover_params *params)
{

	params->func = NULL;

	if (attr == NULL) {
		BT_DBG("CAS CSIS include not found");

		if (cap_cb && cap_cb->discovery_complete) {
			cap_cb->discovery_complete(conn, 0, NULL);
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
		client->csis_inst = bt_csis_client_csis_inst_by_handle(
					conn, client->csis_start_handle);

		if (client->csis_inst == NULL) {
			static struct bt_csis_client_cb csis_client_cb = {
				.discover = csis_client_discover_cb
			};
			static bool csis_cbs_registered;
			int err;

			BT_DBG("CAS CSIS not known, discovering");

			if (!csis_cbs_registered) {
				bt_csis_client_register_cb(&csis_client_cb);
				csis_cbs_registered = true;
			}

			err = bt_csis_client_discover(conn);
			if (err != 0) {
				BT_DBG("Discover failed (err %d)", err);
				if (cap_cb && cap_cb->discovery_complete) {
					cap_cb->discovery_complete(conn, err,
								   NULL);
				}
			}
		} else if (cap_cb && cap_cb->discovery_complete) {
			BT_DBG("Found CAS with CSIS");
			cap_cb->discovery_complete(conn, 0, client->csis_inst);
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
		if (cap_cb && cap_cb->discovery_complete) {
			cap_cb->discovery_complete(conn, -ENODATA, NULL);
		}
	} else {
		const struct bt_gatt_service_val *prim_service = attr->user_data;
		int err;

		if (attr->handle == prim_service->end_handle) {
			BT_DBG("Found CAS without CSIS");
			cap_cb->discovery_complete(conn, 0, NULL);

			return BT_GATT_ITER_STOP;
		}

		BT_DBG("Found CAS, discovering included CSIS");

		params->uuid = NULL;
		params->start_handle = attr->handle + 1;
		params->end_handle = prim_service->end_handle;
		params->type = BT_GATT_DISCOVER_INCLUDE;
		params->func = cap_unicast_discover_included_cb;

		err = bt_gatt_discover(conn, params);
		if (err != 0) {
			BT_DBG("Discover failed (err %d)", err);

			params->func = NULL;
			if (cap_cb && cap_cb->discovery_complete) {
				cap_cb->discovery_complete(conn, err, NULL);
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
		BT_DBG("NULL conn");
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

int bt_cap_initiator_unicast_audio_start(const struct bt_cap_unicast_audio_start_param *param,
					 struct bt_audio_unicast_group **unicast_group)
{
	return -ENOSYS;
}

int bt_cap_initiator_unicast_audio_update(struct bt_audio_unicast_group *unicast_group,
					  uint8_t meta_count,
					  const struct bt_codec_data *meta)
{
	return -ENOSYS;
}

int bt_cap_initiator_unicast_audio_stop(struct bt_audio_unicast_group *unicast_group)
{
	return -ENOSYS;
}

#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE) && defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)

int bt_cap_initiator_unicast_to_broadcast(
	const struct bt_cap_unicast_to_broadcast_param *param,
	struct bt_cap_broadcast_source **source)
{
	return -ENOSYS;
}

int bt_cap_initiator_broadcast_to_unicast(const struct bt_cap_broadcast_to_unicast_param *param,
					  struct bt_audio_unicast_group **unicast_group)
{
	return -ENOSYS;
}

#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE && CONFIG_BT_AUDIO_UNICAST_CLIENT */
