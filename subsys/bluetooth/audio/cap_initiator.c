/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/check.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/audio/cap.h>
#include "cap_internal.h"
#include "csip_internal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_cap_initiator, CONFIG_BT_CAP_INITIATOR_LOG_LEVEL);

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

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)

int bt_cap_initiator_broadcast_audio_start(const struct bt_cap_broadcast_audio_start_param *param,
					   struct bt_audio_broadcast_source **source)
{
	return -ENOSYS;
}

int bt_cap_initiator_broadcast_audio_update(struct bt_audio_broadcast_source *broadcast_source,
					    uint8_t meta_count,
					    const struct bt_codec_data *meta)
{
	return -ENOSYS;
}

int bt_cap_initiator_broadcast_audio_stop(struct bt_audio_broadcast_source *broadcast_source)
{
	return -ENOSYS;
}

#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
static const struct bt_uuid *cas_uuid = BT_UUID_CAS;

struct cap_unicast_client {
	struct bt_gatt_discover_params param;
	uint16_t csis_start_handle;
	const struct bt_csip_set_coordinator_csis_inst *csis_inst;
};

static struct cap_unicast_client bt_cap_unicast_clients[CONFIG_BT_MAX_CONN];

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
		int err;

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
	struct bt_audio_broadcast_source **source)
{
	return -ENOSYS;
}

int bt_cap_initiator_broadcast_to_unicast(const struct bt_cap_broadcast_to_unicast_param *param,
					  struct bt_audio_unicast_group **unicast_group)
{
	return -ENOSYS;
}

#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE && CONFIG_BT_AUDIO_UNICAST_CLIENT */
