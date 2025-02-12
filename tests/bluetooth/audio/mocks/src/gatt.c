/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/iterable_sections.h>

#include "gatt.h"
#include "conn.h"

#define LOG_LEVEL CONFIG_BT_GATT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_gatt);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(mock_bt_gatt_notify_cb)                                                               \
	FAKE(mock_bt_gatt_is_subscribed)                                                           \

DEFINE_FAKE_VALUE_FUNC(int, mock_bt_gatt_notify_cb, struct bt_conn *,
		       struct bt_gatt_notify_params *);
DEFINE_FAKE_VALUE_FUNC(bool, mock_bt_gatt_is_subscribed, struct bt_conn *,
		       const struct bt_gatt_attr *, uint16_t);

ssize_t bt_gatt_attr_read_service(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}

ssize_t bt_gatt_attr_read_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}

ssize_t bt_gatt_attr_read_ccc(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}

ssize_t bt_gatt_attr_write_ccc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			       const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}

void mock_bt_gatt_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);

	mock_bt_gatt_is_subscribed_fake.return_val = true;
}

static void notify_params_deep_copy_destroy(struct bt_gatt_notify_params *params)
{
	struct bt_gatt_notify_params *copy;

	for (unsigned int i = 0; i < mock_bt_gatt_notify_cb_fake.call_count; i++) {
		copy = mock_bt_gatt_notify_cb_fake.arg1_history[i];
		if (copy != params) {
			continue;
		}

		/* Free UUID deep copy */
		if (copy->uuid) {
			free((void *)copy->uuid);
		}

		free(copy);

		mock_bt_gatt_notify_cb_fake.arg1_history[i] = NULL;

		break;
	}
}

static void notify_params_deep_copy_destroy_all(void)
{
	struct bt_gatt_notify_params *copy;

	for (unsigned int i = 0; i < mock_bt_gatt_notify_cb_fake.call_count; i++) {
		copy = mock_bt_gatt_notify_cb_fake.arg1_history[i];
		if (copy == NULL) {
			continue;
		}

		/* Free UUID deep copy */
		if (copy->uuid) {
			free((void *)copy->uuid);
		}

		free(copy);
	}
}

void mock_bt_gatt_cleanup(void)
{
	notify_params_deep_copy_destroy_all();
}

static struct bt_uuid *uuid_deep_copy(const struct bt_uuid *uuid)
{
	struct bt_uuid *copy;

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		copy = malloc(sizeof(struct bt_uuid_16));
		zassert_not_null(copy);
		memcpy(copy, uuid, sizeof(struct bt_uuid_16));
		break;
	case BT_UUID_TYPE_32:
		copy = malloc(sizeof(struct bt_uuid_32));
		zassert_not_null(copy);
		memcpy(copy, uuid, sizeof(struct bt_uuid_32));
		break;
	case BT_UUID_TYPE_128:
		copy = malloc(sizeof(struct bt_uuid_128));
		zassert_not_null(copy);
		memcpy(copy, uuid, sizeof(struct bt_uuid_128));
		break;
	default:
		zassert_unreachable("Unexpected uuid->type 0x%02x", uuid->type);
	}

	return copy;
}

static struct bt_gatt_notify_params *notify_params_deep_copy(struct bt_gatt_notify_params *params)
{
	struct bt_gatt_notify_params *copy;

	copy = malloc(sizeof(*params));
	zassert_not_null(copy);

	memcpy(copy, params, sizeof(*params));

	if (params->uuid != NULL) {
		copy->uuid = uuid_deep_copy(params->uuid);
	}

	return copy;
}

int bt_gatt_notify_cb(struct bt_conn *conn, struct bt_gatt_notify_params *params)
{
	struct bt_gatt_notify_params *copy;
	int err;

	zassert_not_null(params, "'%s()' was called with incorrect '%s' value", __func__, "params");

	/* Either params->uuid, params->attr, or both has to be provided */
	zassert_true(params->uuid != NULL || params->attr != NULL,
		     "'%s()' was called with incorrect '%s' value", __func__,
		     "params->uuid or params->attr");

	copy = notify_params_deep_copy(params);

	err = mock_bt_gatt_notify_cb(conn, copy);
	if (err != 0) {
		notify_params_deep_copy_destroy(copy);
	}

	return err;
}

void bt_gatt_notify_cb_reset(void)
{
	notify_params_deep_copy_destroy_all();

	RESET_FAKE(mock_bt_gatt_notify_cb);
}

#define foreach_attr_type_dyndb(...)
#define last_static_handle BT_ATT_LAST_ATTRIBUTE_HANDLE

/* Exact copy of subsys/bluetooth/host/gatt.c:gatt_foreach_iter() */
static uint8_t gatt_foreach_iter(const struct bt_gatt_attr *attr,
				 uint16_t handle, uint16_t start_handle,
				 uint16_t end_handle,
				 const struct bt_uuid *uuid,
				 const void *attr_data, uint16_t *num_matches,
				 bt_gatt_attr_func_t func, void *user_data)
{
	uint8_t result;

	/* Stop if over the requested range */
	if (handle > end_handle) {
		return BT_GATT_ITER_STOP;
	}

	/* Check if attribute handle is within range */
	if (handle < start_handle) {
		return BT_GATT_ITER_CONTINUE;
	}

	/* Match attribute UUID if set */
	if (uuid && bt_uuid_cmp(uuid, attr->uuid)) {
		return BT_GATT_ITER_CONTINUE;
	}

	/* Match attribute user_data if set */
	if (attr_data && attr_data != attr->user_data) {
		return BT_GATT_ITER_CONTINUE;
	}

	*num_matches -= 1;

	result = func(attr, handle, user_data);

	if (!*num_matches) {
		return BT_GATT_ITER_STOP;
	}

	return result;
}

/* Exact copy of subsys/bluetooth/host/gatt.c:bt_gatt_foreach_attr_type() */
void bt_gatt_foreach_attr_type(uint16_t start_handle, uint16_t end_handle,
			       const struct bt_uuid *uuid,
			       const void *attr_data, uint16_t num_matches,
			       bt_gatt_attr_func_t func, void *user_data)
{
	size_t i;

	if (!num_matches) {
		num_matches = UINT16_MAX;
	}

	if (start_handle <= last_static_handle) {
		uint16_t handle = 1;

		STRUCT_SECTION_FOREACH(bt_gatt_service_static, static_svc) {
			/* Skip ahead if start is not within service handles */
			if (handle + static_svc->attr_count < start_handle) {
				handle += static_svc->attr_count;
				continue;
			}

			for (i = 0; i < static_svc->attr_count; i++, handle++) {
				if (gatt_foreach_iter(&static_svc->attrs[i],
						      handle, start_handle,
						      end_handle, uuid,
						      attr_data, &num_matches,
						      func, user_data) ==
				    BT_GATT_ITER_STOP) {
					return;
				}
			}
		}
	}

	/* Iterate over dynamic db */
	foreach_attr_type_dyndb(start_handle, end_handle, uuid, attr_data,
				num_matches, func, user_data);
}

/* Exact copy of subsys/bluetooth/host/gatt.c:bt_gatt_attr_read() */
ssize_t bt_gatt_attr_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  void *buf, uint16_t buf_len, uint16_t offset,
			  const void *value, uint16_t value_len)
{
	uint16_t len;

	if (offset > value_len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	len = MIN(buf_len, value_len - offset);

	LOG_DBG("handle 0x%04x offset %u length %u", attr->handle, offset, len);

	memcpy(buf, (uint8_t *)value + offset, len);

	return len;
}

int bt_gatt_discover(struct bt_conn *conn, struct bt_gatt_discover_params *params)
{
	zassert_not_null(conn, "'%s()' was called with incorrect '%s' value", __func__, "conn");
	zassert_not_null(params, "'%s()' was called with incorrect '%s' value", __func__, "params");
	zassert_not_null(params->func, "'%s()' was called with incorrect '%s' value", __func__,
			 "params->func");
	zassert_between_inclusive(
		params->start_handle, BT_ATT_FIRST_ATTRIBUTE_HANDLE, BT_ATT_LAST_ATTRIBUTE_HANDLE,
		"'%s()' was called with incorrect '%s' value", __func__, "params->start_handle");
	zassert_between_inclusive(
		params->end_handle, BT_ATT_FIRST_ATTRIBUTE_HANDLE, BT_ATT_LAST_ATTRIBUTE_HANDLE,
		"'%s()' was called with incorrect '%s' value", __func__, "params->end_handle");
	zassert_true(params->start_handle <= params->end_handle,
		     "'%s()' was called with incorrect '%s' value", __func__, "params->end_handle");

	struct bt_gatt_service_val value;
	struct bt_uuid_16 uuid;
	struct bt_gatt_attr attr;
	uint16_t start_handle;
	uint16_t end_handle;

	if (conn->info.state != BT_CONN_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	switch (params->type) {
	case BT_GATT_DISCOVER_PRIMARY:
	case BT_GATT_DISCOVER_SECONDARY:
	case BT_GATT_DISCOVER_STD_CHAR_DESC:
	case BT_GATT_DISCOVER_INCLUDE:
	case BT_GATT_DISCOVER_CHARACTERISTIC:
	case BT_GATT_DISCOVER_DESCRIPTOR:
	case BT_GATT_DISCOVER_ATTRIBUTE:
		break;
	default:
		LOG_ERR("Invalid discovery type: %u", params->type);
		return -EINVAL;
	}

	uuid.uuid.type = BT_UUID_TYPE_16;
	uuid.val = params->type;
	start_handle = params->start_handle;
	end_handle = params->end_handle;
	value.end_handle = end_handle;
	value.uuid = params->uuid;

	attr = (struct bt_gatt_attr){
		.uuid = &uuid.uuid,
		.user_data = &value,
		.handle = start_handle,
	};

	params->func(conn, &attr, params);

	return 0;
}

uint16_t bt_gatt_get_mtu(struct bt_conn *conn)
{
	return 64;
}

bool bt_gatt_is_subscribed(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, uint16_t ccc_type)
{
	return mock_bt_gatt_is_subscribed(conn, attr, ccc_type);
}
