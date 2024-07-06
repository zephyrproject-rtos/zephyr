/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/tmap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/types.h>

#include "audio_internal.h"

LOG_MODULE_REGISTER(bt_tmap, CONFIG_BT_TMAP_LOG_LEVEL);

/* Hex value if all TMAP role bits are set */
#define TMAP_ALL_ROLES		0x3F

static uint16_t tmap_role;
static const struct bt_tmap_cb *cb;
static bool tmas_found;

static struct bt_uuid_16 uuid[CONFIG_BT_MAX_CONN] = {BT_UUID_INIT_16(0)};
static struct bt_gatt_discover_params discover_params[CONFIG_BT_MAX_CONN];
static struct bt_gatt_read_params read_params[CONFIG_BT_MAX_CONN];

uint8_t tmap_char_read(struct bt_conn *conn, uint8_t err,
		       struct bt_gatt_read_params *params,
		       const void *data, uint16_t length)
{
	uint16_t peer_role;

	/* Check read request result */
	if (err != BT_ATT_ERR_SUCCESS) {
		if (cb->discovery_complete) {
			cb->discovery_complete(0, conn, err);
		}

		return BT_GATT_ITER_STOP;
	}

	/* Check data length */
	if (length != sizeof(peer_role)) {
		if (cb->discovery_complete) {
			cb->discovery_complete(0, conn, BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		return BT_GATT_ITER_STOP;
	}

	/* Extract the TMAP role of the peer and inform application of the value found */
	peer_role = sys_get_le16(data);

	if ((peer_role > 0U) && (peer_role <= TMAP_ALL_ROLES)) {
		if (cb->discovery_complete) {
			cb->discovery_complete((enum bt_tmap_role)peer_role, conn, 0);
		}
	} else {
		if (cb->discovery_complete) {
			cb->discovery_complete(0, conn, BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;
	uint8_t conn_id = bt_conn_index(conn);

	if (!attr) {
		(void)memset(params, 0, sizeof(*params));
		if (!tmas_found) {
			/* TMAS not found on peer */
			if (cb->discovery_complete) {
				cb->discovery_complete(0, conn, BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
			}
		}

		tmas_found = false;

		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(discover_params[conn_id].uuid, BT_UUID_TMAS)) {
		LOG_DBG("Discovered TMAS\n");
		tmas_found = true;
		memcpy(&uuid[conn_id], BT_UUID_GATT_TMAPR, sizeof(uuid[conn_id]));
		discover_params[conn_id].uuid = &uuid[conn_id].uuid;
		discover_params[conn_id].start_handle = attr->handle + 1;
		discover_params[conn_id].type = BT_GATT_DISCOVER_CHARACTERISTIC;

		/* Discovered TMAS - Search for TMAP Role characteristic */
		err = bt_gatt_discover(conn, &discover_params[conn_id]);
		if (err) {
			LOG_DBG("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(discover_params[conn_id].uuid, BT_UUID_GATT_TMAPR)) {
		/* Use 0 for now, will expand later */
		read_params[conn_id].func = tmap_char_read;
		read_params[conn_id].handle_count = 1u;
		read_params[conn_id].single.handle = bt_gatt_attr_value_handle(attr);
		read_params[conn_id].single.offset = 0;

		/* Discovered TMAP Role characteristic - read value */
		err = bt_gatt_read(conn, &read_params[0]);
		if (err != 0) {
			LOG_DBG("Could not read peer TMAP Role");
		}
	} else {
		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_STOP;
}

static k_ssize_t read_role(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	uint16_t role;

	role = sys_cpu_to_le16(tmap_role);
	LOG_DBG("TMAP: role 0x%04X", role);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &role, sizeof(role));
}

/* Telephony and Media Audio Service attributes */
#define BT_TMAS_SERVICE_DEFINITION \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_TMAS), \
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_TMAPR, \
			       BT_GATT_CHRC_READ, \
			       BT_GATT_PERM_READ, \
			       read_role, NULL, NULL)

static struct bt_gatt_attr svc_attrs[] = { BT_TMAS_SERVICE_DEFINITION };
static struct bt_gatt_service tmas;

int bt_tmap_register(enum bt_tmap_role role)
{
	int err;

	tmas = (struct bt_gatt_service)BT_GATT_SERVICE(svc_attrs);

	err = bt_gatt_service_register(&tmas);
	if (err) {
		LOG_DBG("Could not register the TMAS service");
		return -ENOEXEC;
	}

	tmap_role = role;
	tmas_found = false;

	return 0;
}

int bt_tmap_discover(struct bt_conn *conn, const struct bt_tmap_cb *tmap_cb)
{
	int err = 0;
	uint8_t conn_id = bt_conn_index(conn);

	cb = tmap_cb;

	memcpy(&uuid[conn_id], BT_UUID_TMAS, sizeof(uuid[conn_id]));
	discover_params[conn_id].uuid = &uuid[conn_id].uuid;
	discover_params[conn_id].func = discover_func;
	discover_params[conn_id].start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params[conn_id].end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params[conn_id].type = BT_GATT_DISCOVER_PRIMARY;

	err = bt_gatt_discover(conn, &discover_params[conn_id]);

	return err;
}

void bt_tmap_set_role(enum bt_tmap_role role)
{
	tmap_role = role;
}
