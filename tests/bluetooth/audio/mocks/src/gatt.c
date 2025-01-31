/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/fff.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/ztest_test.h>
#include <zephyr/ztest_assert.h>

#include "gatt.h"
#include "conn.h"
#include "common/bt_str.h"

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

static uint16_t last_static_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
static sys_slist_t db;

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

/* Exact copy of subsys/bluetooth/host/gatt.c:foreach_attr_type_dyndb() */
static void foreach_attr_type_dyndb(uint16_t start_handle, uint16_t end_handle,
				    const struct bt_uuid *uuid, const void *attr_data,
				    uint16_t num_matches, bt_gatt_attr_func_t func, void *user_data)
{
	size_t i;
	struct bt_gatt_service *svc;

	SYS_SLIST_FOR_EACH_CONTAINER(&db, svc, node) {
		struct bt_gatt_service *next;

		next = SYS_SLIST_PEEK_NEXT_CONTAINER(svc, node);
		if (next) {
			/* Skip ahead if start is not within service handles */
			if (next->attrs[0].handle <= start_handle) {
				continue;
			}
		}

		for (i = 0; i < svc->attr_count; i++) {
			struct bt_gatt_attr *attr = &svc->attrs[i];

			if (gatt_foreach_iter(attr, attr->handle, start_handle, end_handle, uuid,
					      attr_data, &num_matches, func,
					      user_data) == BT_GATT_ITER_STOP) {
				return;
			}
		}
	}
}

/* Exact copy of subsys/bluetooth/host/gatt.c:bt_gatt_foreach_attr_type() */
void bt_gatt_foreach_attr_type(uint16_t start_handle, uint16_t end_handle,
			       const struct bt_uuid *uuid,
			       const void *attr_data, uint16_t num_matches,
			       bt_gatt_attr_func_t func, void *user_data)
{
	size_t i;

	LOG_DBG("bt_gatt_foreach_attr_type");

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
					LOG_DBG("Returning after searching static DB");
					return;
				}
			}
		}
	}

	LOG_DBG("foreach_attr_type_dyndb");
	/* Iterate over dynamic db */
	foreach_attr_type_dyndb(start_handle, end_handle, uuid, attr_data,
				num_matches, func, user_data);
}

static void bt_gatt_service_init(void)
{
	last_static_handle = 0U;

	STRUCT_SECTION_FOREACH(bt_gatt_service_static, svc) {
		last_static_handle += svc->attr_count;
	}
}

/* Exact copy of subsys/bluetooth/host/gatt.c:found_attr() */
static uint8_t found_attr(const struct bt_gatt_attr *attr, uint16_t handle, void *user_data)
{
	const struct bt_gatt_attr **found = user_data;

	*found = attr;

	return BT_GATT_ITER_STOP;
}

/* Exact copy of subsys/bluetooth/host/gatt.c:find_attr() */
static const struct bt_gatt_attr *find_attr(uint16_t handle)
{
	const struct bt_gatt_attr *attr = NULL;

	bt_gatt_foreach_attr(handle, handle, found_attr, &attr);

	return attr;
}

/* Exact copy of subsys/bluetooth/host/gatt.c:gatt_insert() */
static void gatt_insert(struct bt_gatt_service *svc, uint16_t last_handle)
{
	struct bt_gatt_service *tmp, *prev = NULL;

	if (last_handle == 0 || svc->attrs[0].handle > last_handle) {
		sys_slist_append(&db, &svc->node);
		return;
	}

	/* DB shall always have its service in ascending order */
	SYS_SLIST_FOR_EACH_CONTAINER(&db, tmp, node) {
		if (tmp->attrs[0].handle > svc->attrs[0].handle) {
			if (prev) {
				sys_slist_insert(&db, &prev->node, &svc->node);
			} else {
				sys_slist_prepend(&db, &svc->node);
			}
			return;
		}

		prev = tmp;
	}
}

/* Exact copy of subsys/bluetooth/host/gatt.c:gatt_register() */
static int gatt_register(struct bt_gatt_service *svc)
{
	struct bt_gatt_service *last;
	uint16_t handle, last_handle;
	struct bt_gatt_attr *attrs = svc->attrs;
	uint16_t count = svc->attr_count;

	if (sys_slist_is_empty(&db)) {
		handle = last_static_handle;
		last_handle = 0;
		goto populate;
	}

	last = SYS_SLIST_PEEK_TAIL_CONTAINER(&db, last, node);
	handle = last->attrs[last->attr_count - 1].handle;
	last_handle = handle;

populate:
	/* Populate the handles and append them to the list */
	for (; attrs && count; attrs++, count--) {
		if (!attrs->handle) {
			/* Allocate handle if not set already */
			attrs->handle = ++handle;
		} else if (attrs->handle > handle) {
			/* Use existing handle if valid */
			handle = attrs->handle;
		} else if (find_attr(attrs->handle)) {
			/* Service has conflicting handles */
			LOG_ERR("Mock: Unable to register handle 0x%04x", attrs->handle);
			return -EINVAL;
		}

		LOG_DBG("attr %p handle 0x%04x uuid %s perm 0x%02x", attrs, attrs->handle,
			bt_uuid_str(attrs->uuid), attrs->perm);
	}

	gatt_insert(svc, last_handle);

	return 0;
}

static int gatt_unregister(struct bt_gatt_service *svc)
{
	if (!sys_slist_find_and_remove(&db, &svc->node)) {
		return -ENOENT;
	}

	return 0;
}

int bt_gatt_service_register(struct bt_gatt_service *svc)
{
	int err;

	__ASSERT(svc, "invalid parameters\n");
	__ASSERT(svc->attrs, "invalid parameters\n");
	__ASSERT(svc->attr_count, "invalid parameters\n");

	/* Init GATT core services */
	bt_gatt_service_init();

	/* Do no allow to register mandatory services twice */
	if (!bt_uuid_cmp(svc->attrs[0].uuid, BT_UUID_GAP) ||
	    !bt_uuid_cmp(svc->attrs[0].uuid, BT_UUID_GATT)) {
		return -EALREADY;
	}

	err = gatt_register(svc);
	if (err < 0) {
		return err;
	}

	return 0;
}

int bt_gatt_service_unregister(struct bt_gatt_service *svc)
{
	int err;

	__ASSERT(svc, "invalid parameters\n");

	err = gatt_unregister(svc);
	if (err) {
		return err;
	}

	return 0;
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

uint16_t bt_gatt_attr_get_handle(const struct bt_gatt_attr *attr)
{
	uint16_t handle = 1;

	if (!attr) {
		return 0;
	}

	if (attr->handle) {
		return attr->handle;
	}

	STRUCT_SECTION_FOREACH(bt_gatt_service_static, static_svc) {
		/* Skip ahead if start is not within service attributes array */
		if ((attr < &static_svc->attrs[0]) ||
		    (attr > &static_svc->attrs[static_svc->attr_count - 1])) {
			handle += static_svc->attr_count;
			continue;
		}

		for (size_t i = 0; i < static_svc->attr_count; i++, handle++) {
			if (attr == &static_svc->attrs[i]) {
				return handle;
			}
		}
	}

	return 0;
}

static uint8_t find_next(const struct bt_gatt_attr *attr, uint16_t handle, void *user_data)
{
	struct bt_gatt_attr **next = user_data;

	*next = (struct bt_gatt_attr *)attr;

	return BT_GATT_ITER_STOP;
}

struct bt_gatt_attr *bt_gatt_find_by_uuid(const struct bt_gatt_attr *attr, uint16_t attr_count,
					  const struct bt_uuid *uuid)
{
	struct bt_gatt_attr *found = NULL;
	uint16_t start_handle = bt_gatt_attr_get_handle(attr);
	uint16_t end_handle = start_handle && attr_count
				      ? MIN(start_handle + attr_count, BT_ATT_LAST_ATTRIBUTE_HANDLE)
				      : BT_ATT_LAST_ATTRIBUTE_HANDLE;

	if (attr != NULL && start_handle == 0U) {
		/* If start_handle is 0 then `attr` is not in our database, and should not be used
		 * as a starting point for the search
		 */
		LOG_DBG("Could not find handle of attr %p", attr);
		return NULL;
	}

	bt_gatt_foreach_attr_type(start_handle, end_handle, uuid, NULL, 1, find_next, &found);

	return found;
}
