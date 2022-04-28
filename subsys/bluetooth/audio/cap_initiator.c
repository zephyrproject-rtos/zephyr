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

static struct {
	struct bt_gatt_discover_params param;
} bt_cap_unicast_clients[CONFIG_BT_MAX_CONN];

static uint8_t cap_unicast_discover_cb(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr,
				       struct bt_gatt_discover_params *params)
{
	int err;

	params->func = NULL;

	if (attr == NULL) {
		err = -ENODATA;
	} else {
		err = 0;
	}

	if (cap_cb && cap_cb->discovery_complete) {
		cap_cb->discovery_complete(conn, err);
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

	param->func = cap_unicast_discover_cb;
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
