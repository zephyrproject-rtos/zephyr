/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>

#include "../bluetooth/host/settings.h"

#include "audio_internal.h"
#include "common/bt_str.h"
#include "has_internal.h"

LOG_MODULE_REGISTER(bt_has, CONFIG_BT_HAS_LOG_LEVEL);

/* The service allows operations with paired devices only.
 * The number of clients is set to maximum number of simultaneous connections to paired devices.
 */
#define MAX_INSTS MIN(CONFIG_BT_MAX_CONN, CONFIG_BT_MAX_PAIRED)

#define BITS_CHANGED(_new_value, _old_value) ((_new_value) ^ (_old_value))
#define FEATURE_DEVICE_TYPE_UNCHANGED(_new_value) \
	!BITS_CHANGED(_new_value, (has.features & BT_HAS_FEAT_HEARING_AID_TYPE_MASK))
#define FEATURE_SYNC_SUPPORT_UNCHANGED(_new_value) \
	!BITS_CHANGED(_new_value, ((has.features & BT_HAS_FEAT_PRESET_SYNC_SUPP) != 0 ? 1 : 0))
#define FEATURE_IND_PRESETS_UNCHANGED(_new_value) \
	!BITS_CHANGED(_new_value, ((has.features & BT_HAS_FEAT_INDEPENDENT_PRESETS) != 0 ? 1 : 0))
#define BONDED_CLIENT_INIT_FLAGS \
	(BIT(FLAG_ACTIVE_INDEX_CHANGED) | BIT(FLAG_NOTIFY_PRESET_LIST) | BIT(FLAG_FEATURES_CHANGED))

static struct bt_has has;

#if defined(CONFIG_BT_HAS_ACTIVE_PRESET_INDEX)
static void active_preset_index_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_HAS_ACTIVE_PRESET_INDEX */

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
struct has_client;

static int read_preset_response(struct has_client *client);
static int preset_list_changed(struct has_client *client);
static int preset_list_changed_generic_update_tail(struct has_client *client);
static int preset_list_changed_record_deleted_last(struct has_client *client);
static ssize_t write_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *data, uint16_t len, uint16_t offset, uint8_t flags);

static void preset_cp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

static ssize_t read_active_preset_index(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					void *buf, uint16_t len, uint16_t offset)
{
	uint8_t active_index;

	LOG_DBG("conn %p attr %p offset %d", (void *)conn, attr, offset);

	if (offset > sizeof(active_index)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	active_index = bt_has_preset_active_get();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &active_index, sizeof(active_index));
}
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT */

#if defined(CONFIG_BT_HAS_FEATURES_NOTIFIABLE)
static void features_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_HAS_FEATURES_NOTIFIABLE */

static ssize_t read_features(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	LOG_DBG("conn %p attr %p offset %d", (void *)conn, attr, offset);

	if (offset > sizeof(has.features)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &has.features,
				 sizeof(has.features));
}

#if defined(CONFIG_BT_HAS_FEATURES_NOTIFIABLE)
#define BT_HAS_CHR_FEATURES \
	BT_AUDIO_CHRC(BT_UUID_HAS_HEARING_AID_FEATURES, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_features, NULL, NULL), \
	BT_AUDIO_CCC(features_cfg_changed),
#else
#define BT_HAS_CHR_FEATURES \
	BT_AUDIO_CHRC(BT_UUID_HAS_HEARING_AID_FEATURES, \
		      BT_GATT_CHRC_READ, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_features, NULL, NULL),
#endif /* CONFIG_BT_HAS_FEATURES_NOTIFIABLE */

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
#if defined(CONFIG_BT_HAS_PRESET_CONTROL_POINT_NOTIFIABLE)
#define BT_HAS_CHR_PRESET_CONTROL_POINT \
	BT_AUDIO_CHRC(BT_UUID_HAS_PRESET_CONTROL_POINT, \
		      BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_WRITE_ENCRYPT, \
		      NULL, write_control_point, NULL), \
	BT_AUDIO_CCC(preset_cp_cfg_changed),
#else
#define BT_HAS_CHR_PRESET_CONTROL_POINT \
	BT_AUDIO_CHRC(BT_UUID_HAS_PRESET_CONTROL_POINT, \
		      BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE, \
		      BT_GATT_PERM_WRITE_ENCRYPT, \
		      NULL, write_control_point, NULL), \
	BT_AUDIO_CCC(preset_cp_cfg_changed),
#endif /* CONFIG_BT_HAS_PRESET_CONTROL_POINT_NOTIFIABLE */
#else
#define BT_HAS_CHR_PRESET_CONTROL_POINT
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT */

#if defined(CONFIG_BT_HAS_ACTIVE_PRESET_INDEX)
#define BT_HAS_CHR_ACTIVE_PRESET_INDEX \
	BT_AUDIO_CHRC(BT_UUID_HAS_ACTIVE_PRESET_INDEX, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_active_preset_index, NULL, NULL), \
	BT_AUDIO_CCC(active_preset_index_cfg_changed)
#else
#define BT_HAS_CHR_ACTIVE_PRESET_INDEX
#endif /* CONFIG_BT_HAS_ACTIVE_PRESET_INDEX */

/* Hearing Access Service GATT Attributes */
static struct bt_gatt_attr has_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HAS),
	BT_HAS_CHR_FEATURES
	BT_HAS_CHR_PRESET_CONTROL_POINT
	BT_HAS_CHR_ACTIVE_PRESET_INDEX
};

static struct bt_gatt_service has_svc;
static struct bt_gatt_attr *hearing_aid_features_attr;
static struct bt_gatt_attr *preset_control_point_attr;
static struct bt_gatt_attr *active_preset_index_attr;

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT) || defined(CONFIG_BT_HAS_FEATURES_NOTIFIABLE)
static void notify_work_handler(struct k_work *work);

enum flag_internal {
	FLAG_ACTIVE_INDEX_CHANGED,
	FLAG_PENDING_READ_PRESET_RESPONSE,
	FLAG_NOTIFY_PRESET_LIST,
	FLAG_NOTIFY_PRESET_LIST_GENERIC_UPDATE_TAIL,
	FLAG_NOTIFY_PRESET_LIST_RECORD_DELETED_LAST,
	FLAG_FEATURES_CHANGED,
	FLAG_NUM,
};

/* Stored client context */
static struct client_context {
	bt_addr_le_t addr;

	/* Pending notification flags */
	ATOMIC_DEFINE(flags, FLAG_NUM);

	/* Last notified preset index */
	uint8_t last_preset_index_known;
} contexts[CONFIG_BT_MAX_PAIRED];

/* Connected client instance */
static struct has_client {
	struct bt_conn *conn;
#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
	union {
		struct bt_gatt_indicate_params ind;
#if defined(CONFIG_BT_HAS_PRESET_CONTROL_POINT_NOTIFIABLE)
		struct bt_gatt_notify_params ntf;
#endif /* CONFIG_BT_HAS_PRESET_CONTROL_POINT_NOTIFIABLE */
	} params;

	uint8_t preset_changed_index_next;
	struct bt_has_cp_read_presets_req read_presets_req;
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT */
	struct k_work_delayable notify_work;
	struct client_context *context;
} has_client_list[MAX_INSTS];


static struct client_context *context_find(const bt_addr_le_t *addr)
{
	__ASSERT_NO_MSG(addr != NULL);

	for (size_t i = 0; i < ARRAY_SIZE(contexts); i++) {
		if (bt_addr_le_eq(&contexts[i].addr, addr)) {
			return &contexts[i];
		}
	}

	return NULL;
}

static struct client_context *context_alloc(const bt_addr_le_t *addr)
{
	struct client_context *context;

	__ASSERT_NO_MSG(addr != NULL);

	/* Free contexts has BT_ADDR_LE_ANY as the address */
	context = context_find(BT_ADDR_LE_ANY);
	if (context == NULL) {
		return NULL;
	}

	memset(context, 0, sizeof(*context));

	bt_addr_le_copy(&context->addr, addr);

	return context;
}

static void context_free(struct client_context *context)
{
	bt_addr_le_copy(&context->addr, BT_ADDR_LE_ANY);
}

static void client_free(struct has_client *client)
{
	struct bt_conn_info info = { 0 };
	int err;

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT) || defined(CONFIG_BT_HAS_FEATURES_NOTIFIABLE)
	(void)k_work_cancel_delayable(&client->notify_work);
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT || CONFIG_BT_HAS_FEATURES_NOTIFIABLE */

	err = bt_conn_get_info(client->conn, &info);
	__ASSERT_NO_MSG(err == 0);

	if (client->context != NULL && !bt_le_bond_exists(info.id, info.le.dst)) {
		/* Free stored context of non-bonded client */
		context_free(client->context);
		client->context = NULL;
	}

	bt_conn_unref(client->conn);
	client->conn = NULL;
}

static struct has_client *client_alloc(struct bt_conn *conn)
{
	struct bt_conn_info info = { 0 };
	struct has_client *client = NULL;
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(has_client_list); i++) {
		if (conn == has_client_list[i].conn) {
			return &has_client_list[i];
		}

		/* first free slot */
		if (!client && has_client_list[i].conn == NULL) {
			client = &has_client_list[i];
		}
	}

	__ASSERT(client, "failed to get client for conn %p", (void *)conn);

	memset(client, 0, sizeof(*client));

	client->conn = bt_conn_ref(conn);

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT) || defined(CONFIG_BT_HAS_FEATURES_NOTIFIABLE)
	k_work_init_delayable(&client->notify_work, notify_work_handler);
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT || CONFIG_BT_HAS_FEATURES_NOTIFIABLE */

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		LOG_DBG("Could not get conn info: %d", err);

		return NULL;
	}

	client->context = context_find(info.le.dst);
	if (client->context == NULL) {
		client->context = context_alloc(info.le.dst);
		if (client->context == NULL) {
			LOG_ERR("Failed to allocate client_context for %s",
				bt_addr_le_str(info.le.dst));

			client_free(client);

			return NULL;
		}

		LOG_DBG("New client_context for %s", bt_addr_le_str(info.le.dst));
	}

	return client;
}

static struct has_client *client_find_by_conn(struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(has_client_list); i++) {
		if (conn == has_client_list[i].conn) {
			return &has_client_list[i];
		}
	}

	return NULL;
}

static void notify_work_reschedule(struct has_client *client, k_timeout_t delay)
{
	int err;

	__ASSERT(client->conn, "Not connected");

	if (k_work_delayable_remaining_get(&client->notify_work) > 0) {
		return;
	}

	err = k_work_reschedule(&client->notify_work, delay);
	if (err < 0) {
		LOG_ERR("Failed to reschedule notification work err %d", err);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	struct has_client *client;
	struct bt_conn_info info;
	int ret;

	LOG_DBG("conn %p level %d err %d", (void *)conn, level, err);

	if (err != BT_SECURITY_ERR_SUCCESS) {
		return;
	}

	client = client_alloc(conn);
	if (unlikely(!client)) {
		LOG_ERR("Failed to allocate client");
		return;
	}

	ret = bt_conn_get_info(client->conn, &info);
	if (ret < 0) {
		LOG_ERR("bt_conn_get_info err %d", ret);
		return;
	}

	if (!bt_le_bond_exists(info.id, info.le.dst)) {
		return;
	}

	if (atomic_get(client->context->flags) != 0) {
		notify_work_reschedule(client, K_NO_WAIT);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct has_client *client;

	LOG_DBG("conn %p reason %d", (void *)conn, reason);

	client = client_find_by_conn(conn);
	if (client) {
		client_free(client);
	}
}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	struct has_client *client;

	LOG_DBG("conn %p %s -> %s", (void *)conn, bt_addr_le_str(rpa), bt_addr_le_str(identity));

	client = client_find_by_conn(conn);
	if (client == NULL) {
		return;
	}

	bt_addr_le_copy(&client->context->addr, identity);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.disconnected = disconnected,
	.security_changed = security_changed,
	.identity_resolved = identity_resolved,
};

static void notify_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct has_client *client = CONTAINER_OF(dwork, struct has_client, notify_work);
	int err;

	if (IS_ENABLED(CONFIG_BT_HAS_FEATURES_NOTIFIABLE) &&
	    atomic_test_and_clear_bit(client->context->flags, FLAG_FEATURES_CHANGED) &&
	    bt_gatt_is_subscribed(client->conn, hearing_aid_features_attr, BT_GATT_CCC_NOTIFY)) {
		err = bt_gatt_notify(client->conn, hearing_aid_features_attr, &has.features,
				     sizeof(has.features));
		if (err == -ENOMEM) {
			atomic_set_bit(client->context->flags, FLAG_FEATURES_CHANGED);
			notify_work_reschedule(client, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));
		} else if (err < 0) {
			LOG_ERR("Notify features err %d", err);
		}
	}

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
	if (atomic_test_and_clear_bit(client->context->flags, FLAG_PENDING_READ_PRESET_RESPONSE)) {
		err = read_preset_response(client);
		if (err == -ENOMEM) {
			atomic_set_bit(client->context->flags, FLAG_PENDING_READ_PRESET_RESPONSE);
			notify_work_reschedule(client, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));
		} else if (err < 0) {
			LOG_ERR("Notify read preset response err %d", err);
		}
	} else if (atomic_test_and_clear_bit(client->context->flags, FLAG_NOTIFY_PRESET_LIST)) {
		err = preset_list_changed(client);
		if (err == -ENOMEM) {
			atomic_set_bit(client->context->flags, FLAG_NOTIFY_PRESET_LIST);
			notify_work_reschedule(client, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));
		} else if (err < 0) {
			LOG_ERR("Notify preset list changed err %d", err);
		}
	} else if (atomic_test_and_clear_bit(client->context->flags,
					     FLAG_NOTIFY_PRESET_LIST_GENERIC_UPDATE_TAIL)) {
		err = preset_list_changed_generic_update_tail(client);
		if (err == -ENOMEM) {
			atomic_set_bit(client->context->flags,
				       FLAG_NOTIFY_PRESET_LIST_GENERIC_UPDATE_TAIL);
			notify_work_reschedule(client, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));
		} else if (err < 0) {
			LOG_ERR("Notify preset list changed generic update tail err %d", err);
		}
	} else if (atomic_test_and_clear_bit(client->context->flags,
					     FLAG_NOTIFY_PRESET_LIST_RECORD_DELETED_LAST)) {
		err = preset_list_changed_record_deleted_last(client);
		if (err == -ENOMEM) {
			atomic_set_bit(client->context->flags,
				       FLAG_NOTIFY_PRESET_LIST_RECORD_DELETED_LAST);
			notify_work_reschedule(client, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));
		} else if (err < 0) {
			LOG_ERR("Notify preset list changed recoed deleted last err %d", err);
		}
	}

#endif /* CONFIG_BT_HAS_PRESET_SUPPORT */

	if (IS_ENABLED(CONFIG_BT_HAS_PRESET_SUPPORT) &&
	    atomic_test_and_clear_bit(client->context->flags, FLAG_ACTIVE_INDEX_CHANGED) &&
	    bt_gatt_is_subscribed(client->conn, active_preset_index_attr, BT_GATT_CCC_NOTIFY)) {
		uint8_t active_index;

		active_index = bt_has_preset_active_get();

		err = bt_gatt_notify(client->conn, active_preset_index_attr,
				     &active_index, sizeof(active_index));
		if (err == -ENOMEM) {
			atomic_set_bit(client->context->flags, FLAG_ACTIVE_INDEX_CHANGED);
			notify_work_reschedule(client, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));
		} else if (err < 0) {
			LOG_ERR("Notify active index err %d", err);
		}
	}
}

static void notify(struct has_client *client, enum flag_internal flag)
{
	if (client != NULL) {
		atomic_set_bit(client->context->flags, flag);
		notify_work_reschedule(client, K_NO_WAIT);
		return;
	}

	/* Mark notification to be sent to all clients */
	for (size_t i = 0U; i < ARRAY_SIZE(contexts); i++) {
		atomic_set_bit(contexts[i].flags, flag);
	}

	for (size_t i = 0U; i < ARRAY_SIZE(has_client_list); i++) {
		client = &has_client_list[i];

		if (client->conn == NULL) {
			continue;
		}

		notify_work_reschedule(client, K_NO_WAIT);
	}
}

static void bond_deleted_cb(uint8_t id, const bt_addr_le_t *addr)
{
	struct client_context *context;

	context = context_find(addr);
	if (context != NULL) {
		context_free(context);
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_settings_delete("has", 0, addr);
	}
}

static struct bt_conn_auth_info_cb auth_info_cb = {
	.bond_deleted = bond_deleted_cb,
};
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT || CONFIG_BT_HAS_FEATURES_NOTIFIABLE */

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
static struct has_preset *active_preset;

/* HAS internal preset representation */
static struct has_preset {
	uint8_t index;
	enum bt_has_properties properties;
#if defined(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)
	char name[BT_HAS_PRESET_NAME_MAX + 1]; /* +1 byte for NULL-terminator */
#else
	const char *name;
#endif /* CONFIG_BT_HAS_PRESET_NAME_DYNAMIC */
	const struct bt_has_preset_ops *ops;
	sys_snode_t node;
} preset_pool[CONFIG_BT_HAS_PRESET_COUNT];

static sys_slist_t preset_list = SYS_SLIST_STATIC_INIT(&preset_list);
static sys_slist_t preset_free_list = SYS_SLIST_STATIC_INIT(&preset_free_list);

typedef uint8_t (*preset_func_t)(const struct has_preset *preset, void *user_data);

static void preset_foreach(uint8_t start_index, uint8_t end_index, preset_func_t func,
			   void *user_data)
{
	struct has_preset *preset, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&preset_list, preset, tmp, node) {
		if (preset->index < start_index) {
			continue;
		}

		if (preset->index > end_index) {
			return;
		}

		if (func(preset, user_data) == BT_HAS_PRESET_ITER_STOP) {
			return;
		}
	}
}

static uint8_t preset_found(const struct has_preset *preset, void *user_data)
{
	const struct has_preset **found = user_data;

	*found = preset;

	return BT_HAS_PRESET_ITER_STOP;
}

static void preset_insert(struct has_preset *preset)
{
	struct has_preset *tmp, *prev = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&preset_list, tmp, node) {
		if (tmp->index > preset->index) {
			if (prev) {
				sys_slist_insert(&preset_list, &prev->node, &preset->node);
			} else {
				sys_slist_prepend(&preset_list, &preset->node);
			}
			return;
		}

		prev = tmp;
	}

	sys_slist_append(&preset_list, &preset->node);
}

static struct has_preset *preset_alloc(uint8_t index, enum bt_has_properties properties,
				       const char *name, const struct bt_has_preset_ops *ops)
{
	struct has_preset *preset;
	sys_snode_t *node;

	node = sys_slist_get(&preset_free_list);
	if (node == NULL) {
		return NULL;
	}

	preset = CONTAINER_OF(node, struct has_preset, node);
	preset->index = index;
	preset->properties = properties;
#if defined(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)
	utf8_lcpy(preset->name, name, ARRAY_SIZE(preset->name));
#else
	preset->name = name;
#endif /* CONFIG_BT_HAS_PRESET_NAME_DYNAMIC */
	preset->ops = ops;

	preset_insert(preset);

	return preset;
}

static void preset_free(struct has_preset *preset)
{
	bool removed;

	removed = sys_slist_find_and_remove(&preset_list, &preset->node);
	if (removed) {
		sys_slist_append(&preset_free_list, &preset->node);
	}
}

static struct has_preset *preset_get_head(void)
{
	struct has_preset *next;

	return SYS_SLIST_PEEK_HEAD_CONTAINER(&preset_list, next, node);
}

static struct has_preset *preset_get_tail(void)
{
	struct has_preset *prev;

	return SYS_SLIST_PEEK_TAIL_CONTAINER(&preset_list, prev, node);
}

static struct has_preset *preset_get_prev(const struct has_preset *preset)
{
	struct has_preset *prev;

	SYS_SLIST_FOR_EACH_CONTAINER(&preset_list, prev, node) {
		if (SYS_SLIST_PEEK_NEXT_CONTAINER(prev, node) == preset) {
			return prev;
		}
	}

	prev = preset_get_tail();
	if (prev == preset) {
		return NULL;
	}

	return prev;
}

static struct has_preset *preset_lookup_index(uint8_t index)
{
	struct has_preset *preset;

	SYS_SLIST_FOR_EACH_CONTAINER(&preset_list, preset, node) {
		if (preset->index == index) {
			return preset;
		}
	}

	return NULL;
}

static struct has_preset *preset_get_next(struct has_preset *preset)
{
	struct has_preset *next;

	next = SYS_SLIST_PEEK_NEXT_CONTAINER(preset, node);
	if (next == NULL) {
		next = preset_get_head();
		if (next == preset) {
			return NULL;
		}
	}

	return next;
}

static uint8_t preset_get_prev_index(const struct has_preset *preset)
{
	const struct has_preset *prev;

	prev = preset_get_prev(preset);
	if (prev == NULL || prev->index >= preset->index) {
		return BT_HAS_PRESET_INDEX_NONE;
	}

	return prev->index;
}

static void control_point_ntf_complete(struct bt_conn *conn, void *user_data)
{
	struct has_client *client = client_find_by_conn(conn);

	LOG_DBG("conn %p", (void *)conn);

	/* Resubmit if needed */
	if (client != NULL && atomic_get(client->context->flags) != 0) {
		notify_work_reschedule(client, K_NO_WAIT);
	}
}

static void control_point_ind_complete(struct bt_conn *conn,
				       struct bt_gatt_indicate_params *params,
				       uint8_t err)
{
	if (err) {
		/* TODO: Handle error somehow */
		LOG_ERR("conn %p err 0x%02x", (void *)conn, err);
	}

	control_point_ntf_complete(conn, NULL);
}

static int control_point_send(struct has_client *client, struct net_buf_simple *buf)
{
	const uint16_t max_ntf_size = bt_audio_get_max_ntf_size(client->conn);

	if (max_ntf_size < buf->len) {
		LOG_WRN("Sending truncated control point PDU %u < %u", max_ntf_size, buf->len);
		buf->len = max_ntf_size;
	}

#if defined(CONFIG_BT_HAS_PRESET_CONTROL_POINT_NOTIFIABLE)
	if (bt_eatt_count(client->conn) > 0 &&
	    bt_gatt_is_subscribed(client->conn, preset_control_point_attr, BT_GATT_CCC_NOTIFY)) {
		memset(&client->params.ntf, 0, sizeof(client->params.ntf));
		client->params.ntf.attr = preset_control_point_attr;
		client->params.ntf.func = control_point_ntf_complete;
		client->params.ntf.data = buf->data;
		client->params.ntf.len = buf->len;

		return bt_gatt_notify_cb(client->conn, &client->params.ntf);
	}
#endif /* CONFIG_BT_HAS_PRESET_CONTROL_POINT_NOTIFIABLE */

	if (bt_gatt_is_subscribed(client->conn, preset_control_point_attr, BT_GATT_CCC_INDICATE)) {
		memset(&client->params.ind, 0, sizeof(client->params.ind));
		client->params.ind.attr = preset_control_point_attr;
		client->params.ind.func = control_point_ind_complete;
		client->params.ind.destroy = NULL;
		client->params.ind.data = buf->data;
		/* indications have same size as notifications */
		client->params.ind.len = buf->len;

		return bt_gatt_indicate(client->conn, &client->params.ind);
	}

	return -ECANCELED;
}

static int control_point_send_all(struct net_buf_simple *buf)
{
	int result = 0;

	for (size_t i = 0U; i < ARRAY_SIZE(contexts); i++) {
		struct client_context *context = &contexts[i];
		struct has_client *client = NULL;
		int err;

		for (size_t j = 0U; j < ARRAY_SIZE(has_client_list); j++) {
			if (has_client_list[j].context == context) {
				client = &has_client_list[j];
				break;
			}
		}

		if (client == NULL || client->conn == NULL) {
			/* Mark preset changed operation as pending */
			atomic_set_bit(context->flags, FLAG_NOTIFY_PRESET_LIST);
			continue;
		}

		if (!bt_gatt_is_subscribed(client->conn, preset_control_point_attr,
					   BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE)) {
			continue;
		}

		err = control_point_send(client, buf);
		if (err) {
			result = err;
			/* continue anyway */
		}
	}

	return result;
}

static int bt_has_cp_read_preset_rsp(struct has_client *client, const struct has_preset *preset,
				     bool is_last)
{
	struct bt_has_cp_hdr *hdr;
	struct bt_has_cp_read_preset_rsp *rsp;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*hdr) + sizeof(*rsp) + BT_HAS_PRESET_NAME_MAX);

	LOG_DBG("conn %p index 0x%02x prop 0x%02x %s is_last 0x%02x", (void *)client->conn,
		preset->index, preset->properties, preset->name, is_last);

	hdr = net_buf_simple_add(&buf, sizeof(*hdr));
	hdr->opcode = BT_HAS_OP_READ_PRESET_RSP;
	rsp = net_buf_simple_add(&buf, sizeof(*rsp));
	rsp->is_last = is_last ? 0x01 : 0x00;
	rsp->index = preset->index;
	rsp->properties = preset->properties;
	net_buf_simple_add_mem(&buf, preset->name, strlen(preset->name));

	return control_point_send(client, &buf);
}

static void preset_changed_prepare(struct net_buf_simple *buf, uint8_t change_id, uint8_t is_last)
{
	struct bt_has_cp_hdr *hdr;
	struct bt_has_cp_preset_changed *preset_changed;

	hdr = net_buf_simple_add(buf, sizeof(*hdr));
	hdr->opcode = BT_HAS_OP_PRESET_CHANGED;
	preset_changed = net_buf_simple_add(buf, sizeof(*preset_changed));
	preset_changed->change_id = change_id;
	preset_changed->is_last = is_last;
}

static int bt_has_cp_generic_update(struct has_client *client, uint8_t prev_index, uint8_t index,
				    uint8_t properties, const char *name, uint8_t is_last)
{
	struct bt_has_cp_generic_update *generic_update;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(struct bt_has_cp_hdr) +
			      sizeof(struct bt_has_cp_preset_changed) +
			      sizeof(struct bt_has_cp_generic_update) + BT_HAS_PRESET_NAME_MAX);

	LOG_DBG("client %p prev_index 0x%02x index 0x%02x prop 0x%02x %s is_last %d",
		client, prev_index, index, properties, name, is_last);

	preset_changed_prepare(&buf, BT_HAS_CHANGE_ID_GENERIC_UPDATE, is_last);

	generic_update = net_buf_simple_add(&buf, sizeof(*generic_update));
	generic_update->prev_index = prev_index;
	generic_update->index = index;
	generic_update->properties = properties;
	net_buf_simple_add_mem(&buf, name, strlen(name));

	if (client) {
		return control_point_send(client, &buf);
	} else {
		return control_point_send_all(&buf);
	}
}

#if defined(CONFIG_BT_SETTINGS)
struct client_context_store {
	/* Last notified preset index */
	uint8_t last_preset_index_known;
} __packed;

static int settings_set_cb(const char *name, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
	struct client_context_store store;
	struct client_context *context;
	bt_addr_le_t addr;
	ssize_t len;
	int err;

	if (!name) {
		LOG_ERR("Insufficient number of arguments");
		return -EINVAL;
	}

	err = bt_settings_decode_key(name, &addr);
	if (err) {
		LOG_ERR("Unable to decode address %s", name);
		return -EINVAL;
	}

	context = context_find(&addr);
	if (context == NULL) {
		/* Find and initialize a free entry */
		context = context_alloc(&addr);
		if (context == NULL) {
			LOG_ERR("Failed to allocate client_context for %s", bt_addr_le_str(&addr));
			return -ENOMEM;
		}
	}

	if (len_rd) {
		len = read_cb(cb_arg, &store, sizeof(store));
		if (len < 0) {
			LOG_ERR("Failed to decode value (err %zd)", len);
			return len;
		}

		context->last_preset_index_known = store.last_preset_index_known;
	} else {
		context->last_preset_index_known = 0x00;
	}

	/* Notify all the characteristics values after reboot */
	atomic_set(context->flags, BONDED_CLIENT_INIT_FLAGS);

	return 0;
}

static BT_SETTINGS_DEFINE(has, "has", settings_set_cb, NULL);

static void store_client_context(struct client_context *context)
{
	struct client_context_store store = {
		.last_preset_index_known = context->last_preset_index_known,
	};
	int err;

	LOG_DBG("%s last_preset_index_known 0x%02x",
		bt_addr_le_str(&context->addr), store.last_preset_index_known);

	err = bt_settings_store("has", 0, &context->addr, &store, sizeof(store));
	if (err != 0) {
		LOG_ERR("Failed to store err %d", err);
	}
}
#else
#define store_client_context(...)
#endif /* CONFIG_BT_SETTINGS */

static void update_last_preset_index_known(struct has_client *client, uint8_t index)
{
	if (client != NULL && client->context != NULL &&
	    client->context->last_preset_index_known != index) {
		client->context->last_preset_index_known = index;
		store_client_context(client->context);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(has_client_list); i++) {
		client = &has_client_list[i];

		/* For each connected client */
		if (client->conn != NULL && client->context != NULL &&
		    client->context->last_preset_index_known != index) {
			client->context->last_preset_index_known = index;
			store_client_context(client->context);
		}
	}
}

static int read_preset_response(struct has_client *client)
{
	const struct has_preset *preset = NULL;
	bool is_last = true;
	int err;

	__ASSERT_NO_MSG(client != NULL);

	preset_foreach(client->read_presets_req.start_index, BT_HAS_PRESET_INDEX_LAST,
		       preset_found, &preset);

	if (unlikely(preset == NULL)) {
		return bt_has_cp_read_preset_rsp(client, NULL, BT_HAS_IS_LAST);
	}

	if (client->read_presets_req.num_presets > 1) {
		const struct has_preset *next = NULL;

		preset_foreach(preset->index + 1, BT_HAS_PRESET_INDEX_LAST, preset_found, &next);

		is_last = next == NULL;
	}

	err = bt_has_cp_read_preset_rsp(client, preset, is_last);
	if (err != 0) {
		return err;
	}

	if (preset->index > client->context->last_preset_index_known) {
		update_last_preset_index_known(client, preset->index);
	}

	if (!is_last) {
		client->read_presets_req.start_index = preset->index + 1;
		client->read_presets_req.num_presets--;

		atomic_set_bit(client->context->flags, FLAG_PENDING_READ_PRESET_RESPONSE);
		notify_work_reschedule(client, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));
	}

	return 0;
}

static int bt_has_cp_preset_record_deleted(struct has_client *client, uint8_t index)
{
	NET_BUF_SIMPLE_DEFINE(buf, sizeof(struct bt_has_cp_hdr) +
			      sizeof(struct bt_has_cp_preset_changed) + sizeof(uint8_t));

	LOG_DBG("client %p index 0x%02x", client, index);

	preset_changed_prepare(&buf, BT_HAS_CHANGE_ID_PRESET_DELETED, BT_HAS_IS_LAST);
	net_buf_simple_add_u8(&buf, index);

	if (client != NULL) {
		return control_point_send(client, &buf);
	} else {
		return control_point_send_all(&buf);
	}
}

/* Generic Update the last (already deleted) preset */
static int preset_list_changed_generic_update_tail(struct has_client *client)
{
	const struct has_preset *prev;
	struct has_preset last = {
		/* The index value of the last preset the client knew about. */
		.index = client->context->last_preset_index_known,

		/* As the properties of deleted preset is not available anymore, we set this value
		 * to 0x00 meaning the preset is unavailable and non-writable which is actually true
		 */
		.properties = BT_HAS_PROP_NONE,

		/* As the name of deleted preset are not available anymore, we set this value
		 * to the value what is compliant with specification.
		 * As per HAS_v1.0 the Name is 1-40 octet value.
		 */
		.name = "N/A",
	};
	int err;

	prev = preset_get_tail();

	err = bt_has_cp_generic_update(client, prev ? prev->index : BT_HAS_PRESET_INDEX_NONE,
				       last.index, last.properties, last.name, false);
	if (err != 0) {
		return err;
	}

	return 0;
}

static int preset_list_changed_record_deleted_last(struct has_client *client)
{
	const struct has_preset *last;
	int err;

	err = bt_has_cp_preset_record_deleted(client, client->context->last_preset_index_known);
	if (err != 0) {
		return err;
	}

	last = preset_get_tail();

	update_last_preset_index_known(client, last ? last->index : BT_HAS_PRESET_INDEX_NONE);

	return 0;
}

static int preset_list_changed(struct has_client *client)
{
	const struct has_preset *preset = NULL;
	const struct has_preset *next = NULL;
	bool is_last = true;
	int err;

	if (sys_slist_is_empty(&preset_list)) {
		/* The preset list is empty. We need to indicate deletion of all presets */
		atomic_set_bit(client->context->flags,
			       FLAG_NOTIFY_PRESET_LIST_GENERIC_UPDATE_TAIL);
		atomic_set_bit(client->context->flags,
			       FLAG_NOTIFY_PRESET_LIST_RECORD_DELETED_LAST);
		notify_work_reschedule(client, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));

		return 0;
	}

	preset_foreach(client->preset_changed_index_next, BT_HAS_PRESET_INDEX_LAST,
		       preset_found, &preset);

	if (preset == NULL) {
		return 0;
	}

	preset_foreach(preset->index + 1, BT_HAS_PRESET_INDEX_LAST, preset_found, &next);

	/* It is last Preset Changed notification if there are no presets left to notify and the
	 * currently notified preset have the highest index known to the client.
	 */
	is_last = next == NULL && preset->index >= client->context->last_preset_index_known;

	err = bt_has_cp_generic_update(client, preset_get_prev_index(preset), preset->index,
				       preset->properties, preset->name, is_last);
	if (err != 0) {
		return err;
	}

	if (is_last) {
		client->preset_changed_index_next = 0;

		/* It's the last preset notified, so update the highest index known to the client */
		update_last_preset_index_known(client, preset->index);

		return 0;
	}

	if (next == NULL) {
		/* If we end up here, the last preset known to the client has been removed.
		 * As we do not hold the information about the deleted presets, we need to use
		 * Generic Update procedure to:
		 *   1. Notify the presets that have been removed in range
		 *      (PrevIndex = current_preset_last, Index=previous_preset_last)
		 *   2. Notify deletion of preset Index=previous_preset_last.
		 */
		atomic_set_bit(client->context->flags,
			       FLAG_NOTIFY_PRESET_LIST_GENERIC_UPDATE_TAIL);
		atomic_set_bit(client->context->flags,
			       FLAG_NOTIFY_PRESET_LIST_RECORD_DELETED_LAST);
	} else {
		client->preset_changed_index_next = preset->index + 1;

		atomic_set_bit(client->context->flags, FLAG_NOTIFY_PRESET_LIST);
	}

	notify_work_reschedule(client, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));

	return 0;
}

static uint8_t handle_read_preset_req(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_has_cp_read_presets_req *req;
	const struct has_preset *preset = NULL;
	struct has_client *client;

	if (buf->len < sizeof(*req)) {
		return BT_HAS_ERR_INVALID_PARAM_LEN;
	}

	/* As per HAS_d1.0r00 Client Characteristic Configuration Descriptor Improperly Configured
	 * shall be returned if client writes Read Presets Request but is not registered for
	 * indications.
	 */
	if (!bt_gatt_is_subscribed(conn, preset_control_point_attr, BT_GATT_CCC_INDICATE)) {
		return BT_ATT_ERR_CCC_IMPROPER_CONF;
	}

	client = client_find_by_conn(conn);
	if (client == NULL) {
		return BT_ATT_ERR_UNLIKELY;
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	LOG_DBG("start_index %d num_presets %d", req->start_index, req->num_presets);

	/* Abort if there is no preset in requested index range */
	preset_foreach(req->start_index, BT_HAS_PRESET_INDEX_LAST, preset_found, &preset);

	if (preset == NULL) {
		return BT_ATT_ERR_OUT_OF_RANGE;
	}

	/* Reject if already in progress */
	if (atomic_test_bit(client->context->flags, FLAG_PENDING_READ_PRESET_RESPONSE)) {
		return BT_HAS_ERR_OPERATION_NOT_POSSIBLE;
	}

	/* Store the request */
	client->read_presets_req.start_index = req->start_index;
	client->read_presets_req.num_presets = req->num_presets;

	notify(client, FLAG_PENDING_READ_PRESET_RESPONSE);

	return 0;
}

static int set_preset_name(uint8_t index, const char *name, size_t len)
{
	struct has_preset *preset = NULL;

	LOG_DBG("index %d name_len %zu", index, len);

	if (len < BT_HAS_PRESET_NAME_MIN || len > BT_HAS_PRESET_NAME_MAX) {
		return -EINVAL;
	}

	/* Abort if there is no preset in requested index range */
	preset_foreach(index, BT_HAS_PRESET_INDEX_LAST, preset_found, &preset);

	if (preset == NULL) {
		return -ENOENT;
	}

	if (!(preset->properties & BT_HAS_PROP_WRITABLE)) {
		return -EPERM;
	}

	IF_ENABLED(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC, (
		__ASSERT(len < ARRAY_SIZE(preset->name), "No space for name");

		(void)memcpy(preset->name, name, len);

		/* NULL-terminate string */
		preset->name[len] = '\0';

		/* Properly truncate a NULL-terminated UTF-8 string */
		utf8_trunc(preset->name);
	));

	if (preset->ops->name_changed) {
		preset->ops->name_changed(index, preset->name);
	}

	return bt_has_cp_generic_update(NULL, preset_get_prev_index(preset), preset->index,
					preset->properties, preset->name, BT_HAS_IS_LAST);
}

static uint8_t handle_write_preset_name(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_has_cp_write_preset_name *req;
	struct has_client *client;
	int err;

	if (buf->len < sizeof(*req)) {
		return BT_HAS_ERR_INVALID_PARAM_LEN;
	}

	/* As per HAS_v1.0 Client Characteristic Configuration Descriptor Improperly Configured
	 * shall be returned if client writes Write Preset Name opcode but is not registered for
	 * indications.
	 */
	if (!bt_gatt_is_subscribed(conn, preset_control_point_attr, BT_GATT_CCC_INDICATE)) {
		return BT_ATT_ERR_CCC_IMPROPER_CONF;
	}

	client = client_find_by_conn(conn);
	if (!client) {
		return BT_ATT_ERR_UNLIKELY;
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	err = set_preset_name(req->index, req->name, buf->len);
	if (err == -EINVAL) {
		return BT_HAS_ERR_INVALID_PARAM_LEN;
	} else if (err == -ENOENT) {
		return BT_ATT_ERR_OUT_OF_RANGE;
	} else if (err == -EPERM) {
		return BT_HAS_ERR_WRITE_NAME_NOT_ALLOWED;
	} else if (err) {
		return BT_ATT_ERR_UNLIKELY;
	}

	return BT_ATT_ERR_SUCCESS;
}

static void preset_set_active(struct has_preset *preset)
{
	if (active_preset != preset) {
		active_preset = preset;

		notify(NULL, FLAG_ACTIVE_INDEX_CHANGED);
	}
}

static uint8_t preset_select(struct has_preset *preset, bool sync)
{
	const int err = preset->ops->select(preset->index, sync);

	if (err == -EINPROGRESS) {
		/* User has to confirm once the requested preset becomes active by
		 * calling bt_has_preset_active_set.
		 */
		return 0;
	}

	if (err == -EBUSY) {
		return BT_HAS_ERR_OPERATION_NOT_POSSIBLE;
	}

	if (err) {
		return BT_ATT_ERR_UNLIKELY;
	}

	preset_set_active(preset);

	return 0;
}

static bool is_preset_available(const struct has_preset *preset)
{
	return (preset->properties & BT_HAS_PROP_AVAILABLE) != 0;
}

static uint8_t handle_set_active_preset(struct net_buf_simple *buf, bool sync)
{
	const struct bt_has_cp_set_active_preset *pdu;
	struct has_preset *preset;

	if (buf->len < sizeof(*pdu)) {
		return BT_HAS_ERR_INVALID_PARAM_LEN;
	}

	pdu = net_buf_simple_pull_mem(buf, sizeof(*pdu));

	preset = preset_lookup_index(pdu->index);
	if (preset == NULL) {
		return BT_ATT_ERR_OUT_OF_RANGE;
	}

	if (!is_preset_available(preset)) {
		return BT_HAS_ERR_OPERATION_NOT_POSSIBLE;
	}

	return preset_select(preset, sync);
}

static uint8_t handle_set_next_preset(bool sync)
{
	struct has_preset *next, *tmp;

	if (active_preset == NULL) {
		next = preset_get_head();
	} else {
		next = preset_get_next(active_preset);
	}

	tmp = next;
	do {
		if (next == NULL) {
			break;
		}

		if (is_preset_available(next)) {
			return preset_select(next, sync);
		}

		next = preset_get_next(next);
	} while (tmp != next);

	return BT_HAS_ERR_OPERATION_NOT_POSSIBLE;
}

static uint8_t handle_set_prev_preset(bool sync)
{
	struct has_preset *prev, *tmp;

	if (active_preset == NULL) {
		prev = preset_get_tail();
	} else {
		prev = preset_get_prev(active_preset);
	}

	tmp = prev;
	do {
		if (prev == NULL) {
			break;
		}

		if (is_preset_available(prev)) {
			return preset_select(prev, sync);
		}

		prev = preset_get_prev(prev);
	} while (tmp != prev);

	return BT_HAS_ERR_OPERATION_NOT_POSSIBLE;
}

static uint8_t handle_control_point_op(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_has_cp_hdr *hdr;

	hdr = net_buf_simple_pull_mem(buf, sizeof(*hdr));

	LOG_DBG("conn %p opcode %s (0x%02x)", (void *)conn, bt_has_op_str(hdr->opcode),
		hdr->opcode);

	switch (hdr->opcode) {
	case BT_HAS_OP_READ_PRESET_REQ:
		return handle_read_preset_req(conn, buf);
	case BT_HAS_OP_WRITE_PRESET_NAME:
		if (IS_ENABLED(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)) {
			return handle_write_preset_name(conn, buf);
		} else {
			return BT_HAS_ERR_WRITE_NAME_NOT_ALLOWED;
		}
		break;
	case BT_HAS_OP_SET_ACTIVE_PRESET:
		return handle_set_active_preset(buf, false);
	case BT_HAS_OP_SET_NEXT_PRESET:
		return handle_set_next_preset(false);
	case BT_HAS_OP_SET_PREV_PRESET:
		return handle_set_prev_preset(false);
	case BT_HAS_OP_SET_ACTIVE_PRESET_SYNC:
		if ((has.features & BT_HAS_FEAT_PRESET_SYNC_SUPP) != 0) {
			return handle_set_active_preset(buf, true);
		} else {
			return BT_HAS_ERR_PRESET_SYNC_NOT_SUPP;
		}
	case BT_HAS_OP_SET_NEXT_PRESET_SYNC:
		if ((has.features & BT_HAS_FEAT_PRESET_SYNC_SUPP) != 0) {
			return handle_set_next_preset(true);
		} else {
			return BT_HAS_ERR_PRESET_SYNC_NOT_SUPP;
		}
	case BT_HAS_OP_SET_PREV_PRESET_SYNC:
		if ((has.features & BT_HAS_FEAT_PRESET_SYNC_SUPP) != 0) {
			return handle_set_prev_preset(true);
		} else {
			return BT_HAS_ERR_PRESET_SYNC_NOT_SUPP;
		}
	};

	return BT_HAS_ERR_INVALID_OPCODE;
}

static ssize_t write_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *data, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct net_buf_simple buf;
	uint8_t err;

	LOG_DBG("conn %p attr %p data %p len %d offset %d flags 0x%02x", (void *)conn, attr, data,
		len, offset, flags);

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len == 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	err = handle_control_point_op(conn, &buf);
	if (err) {
		LOG_WRN("handle_control_point_op err 0x%02x", err);
		return BT_GATT_ERR(err);
	}

	return len;
}

int bt_has_preset_register(const struct bt_has_preset_register_param *param)
{
	struct has_preset *preset;
	size_t name_len;

	CHECKIF(param == NULL) {
		LOG_ERR("param is NULL");
		return -EINVAL;
	}

	CHECKIF(param->index == BT_HAS_PRESET_INDEX_NONE) {
		LOG_ERR("param->index is invalid");
		return -EINVAL;
	}

	CHECKIF(param->name == NULL) {
		LOG_ERR("param->name is NULL");
		return -EINVAL;
	}

	name_len = strlen(param->name);

	CHECKIF(name_len < BT_HAS_PRESET_NAME_MIN) {
		LOG_ERR("param->name is too short (%zu < %u)", name_len, BT_HAS_PRESET_NAME_MIN);
		return -EINVAL;
	}

	CHECKIF(name_len > BT_HAS_PRESET_NAME_MAX) {
		LOG_WRN("param->name is too long (%zu > %u)", name_len, BT_HAS_PRESET_NAME_MAX);
	}

	CHECKIF(param->ops == NULL) {
		LOG_ERR("param->ops is NULL");
		return -EINVAL;
	}

	CHECKIF(param->ops->select == NULL) {
		LOG_ERR("param->ops->select is NULL");
		return -EINVAL;
	}

	preset = preset_lookup_index(param->index);
	if (preset != NULL) {
		return -EALREADY;
	}

	CHECKIF(!IS_ENABLED(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC) &&
		(param->properties & BT_HAS_PROP_WRITABLE) > 0) {
		LOG_ERR("Writable presets are not supported");
		return -ENOTSUP;
	}

	preset = preset_alloc(param->index, param->properties, param->name, param->ops);
	if (preset == NULL) {
		return -ENOMEM;
	}

	if (preset == preset_get_tail()) {
		update_last_preset_index_known(NULL, preset->index);
	}

	return bt_has_cp_generic_update(NULL, preset_get_prev_index(preset), preset->index,
					preset->properties, preset->name, BT_HAS_IS_LAST);
}

int bt_has_preset_unregister(uint8_t index)
{
	struct has_preset *preset;
	int err;

	CHECKIF(index == BT_HAS_PRESET_INDEX_NONE) {
		LOG_ERR("index is invalid");
		return -EINVAL;
	}

	preset = preset_lookup_index(index);
	if (preset == NULL) {
		return -ENOENT;
	}

	if (preset == active_preset) {
		return -EADDRINUSE;
	}

	err = bt_has_cp_preset_record_deleted(NULL, preset->index);
	if (err != 0) {
		return err;
	}

	if (preset == preset_get_tail()) {
		update_last_preset_index_known(NULL, preset_get_prev_index(preset));
	}

	preset_free(preset);

	return 0;
}

static int set_preset_availability(uint8_t index, bool available)
{
	NET_BUF_SIMPLE_DEFINE(buf, sizeof(struct bt_has_cp_hdr) +
			      sizeof(struct bt_has_cp_preset_changed) + sizeof(uint8_t));
	struct has_preset *preset;
	uint8_t change_id;

	CHECKIF(index == BT_HAS_PRESET_INDEX_NONE) {
		LOG_ERR("index is invalid");
		return -EINVAL;
	}

	preset = preset_lookup_index(index);
	if (preset == NULL) {
		return -ENOENT;
	}

	if (is_preset_available(preset) == available) {
		/* availability not changed */
		return 0;
	}

	preset->properties ^= BT_HAS_PROP_AVAILABLE;

	if (is_preset_available(preset)) {
		change_id = BT_HAS_CHANGE_ID_PRESET_AVAILABLE;
	} else {
		change_id = BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE;
	}

	preset_changed_prepare(&buf, change_id, BT_HAS_IS_LAST);
	net_buf_simple_add_u8(&buf, preset->index);

	return control_point_send_all(&buf);
}

int bt_has_preset_available(uint8_t index)
{
	return set_preset_availability(index, true);
}

int bt_has_preset_unavailable(uint8_t index)
{
	return set_preset_availability(index, false);
}

struct bt_has_preset_foreach_data {
	bt_has_preset_func_t func;
	void *user_data;
};

static uint8_t bt_has_preset_foreach_func(const struct has_preset *preset, void *user_data)
{
	const struct bt_has_preset_foreach_data *data = user_data;

	return data->func(preset->index, preset->properties, preset->name, data->user_data);
}

void bt_has_preset_foreach(uint8_t index, bt_has_preset_func_t func, void *user_data)
{
	uint8_t start_index, end_index;
	struct bt_has_preset_foreach_data data = {
		.func = func,
		.user_data = user_data,
	};

	if (index == BT_HAS_PRESET_INDEX_NONE) {
		start_index = BT_HAS_PRESET_INDEX_FIRST;
		end_index = BT_HAS_PRESET_INDEX_LAST;
	} else {
		start_index = end_index = index;
	}

	preset_foreach(start_index, end_index, bt_has_preset_foreach_func, &data);
}

int bt_has_preset_active_set(uint8_t index)
{
	struct has_preset *preset;

	if (index == BT_HAS_PRESET_INDEX_NONE) {
		preset_set_active(NULL);
		return 0;
	}

	preset = preset_lookup_index(index);
	if (preset == NULL) {
		return -ENOENT;
	}

	if (!is_preset_available(preset)) {
		return -EINVAL;
	}

	preset_set_active(preset);

	return 0;
}

uint8_t bt_has_preset_active_get(void)
{
	if (active_preset == NULL) {
		return BT_HAS_PRESET_INDEX_NONE;
	}

	return active_preset->index;
}

int bt_has_preset_name_change(uint8_t index, const char *name)
{
	CHECKIF(name == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)) {
		return set_preset_name(index, name, strlen(name));
	} else {
		return -EOPNOTSUPP;
	}
}
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT */

static int has_features_register(const struct bt_has_features_param *features)
{
	/* Initialize the supported features characteristic value */
	has.features = features->type;

	if (IS_ENABLED(CONFIG_BT_HAS_PRESET_SUPPORT)) {
		has.features |= BT_HAS_FEAT_DYNAMIC_PRESETS;

		if (features->preset_sync_support) {
			if (features->type != BT_HAS_HEARING_AID_TYPE_BINAURAL) {
				LOG_DBG("Preset sync support only available "
					"for binaural hearing aid type");
				return -EINVAL;
			}

			has.features |= BT_HAS_FEAT_PRESET_SYNC_SUPP;
		}

		if (features->independent_presets) {
			if (features->type != BT_HAS_HEARING_AID_TYPE_BINAURAL) {
				LOG_DBG("Independent presets only available "
					"for binaural hearing aid type");
				return -EINVAL;
			}

			has.features |= BT_HAS_FEAT_INDEPENDENT_PRESETS;
		}
	}

	if (IS_ENABLED(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)) {
		has.features |= BT_HAS_FEAT_WRITABLE_PRESETS_SUPP;
	}

	return 0;
}

#if defined(CONFIG_BT_HAS_FEATURES_NOTIFIABLE)
int bt_has_features_set(const struct bt_has_features_param *features)
{
	int err;

	if (!has.registered) {
		return -ENOTSUP;
	}

	/* Check whether any features will change, otherwise we don't want to notify clients */
	if (FEATURE_DEVICE_TYPE_UNCHANGED(features->type) &&
	    FEATURE_SYNC_SUPPORT_UNCHANGED(features->preset_sync_support) &&
	    FEATURE_IND_PRESETS_UNCHANGED(features->independent_presets)) {
		return 0;
	}

	err = has_features_register(features);
	if (err != 0) {
		LOG_DBG("Failed to register features");
		return err;
	}

	notify(NULL, FLAG_FEATURES_CHANGED);

	return 0;
}
#endif /* CONFIG_BT_HAS_FEATURES_NOTIFIABLE */

int bt_has_register(const struct bt_has_features_param *features)
{
	int err;

	LOG_DBG("features %p", features);

	CHECKIF(!features) {
		LOG_DBG("NULL params pointer");
		return -EINVAL;
	}

	if (has.registered) {
		return -EALREADY;
	}

	err = has_features_register(features);
	if (err != 0) {
		LOG_DBG("HAS service failed to register features: %d", err);
		return err;
	}

	has_svc = (struct bt_gatt_service)BT_GATT_SERVICE(has_attrs);
	err = bt_gatt_service_register(&has_svc);
	if (err != 0) {
		LOG_DBG("HAS service register failed: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_HAS_FEATURES_NOTIFIABLE)) {
		hearing_aid_features_attr = bt_gatt_find_by_uuid(has_svc.attrs, has_svc.attr_count,
								 BT_UUID_HAS_HEARING_AID_FEATURES);
		__ASSERT_NO_MSG(hearing_aid_features_attr != NULL);
	}

	if (IS_ENABLED(CONFIG_BT_HAS_PRESET_SUPPORT)) {
		preset_control_point_attr = bt_gatt_find_by_uuid(has_svc.attrs, has_svc.attr_count,
								 BT_UUID_HAS_PRESET_CONTROL_POINT);
		__ASSERT_NO_MSG(preset_control_point_attr != NULL);

		active_preset_index_attr = bt_gatt_find_by_uuid(has_svc.attrs, has_svc.attr_count,
								BT_UUID_HAS_ACTIVE_PRESET_INDEX);
		__ASSERT_NO_MSG(active_preset_index_attr != NULL);
	}

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
	for (size_t i = 0; i < ARRAY_SIZE(preset_pool); i++) {
		struct has_preset *preset = &preset_pool[i];

		sys_slist_append(&preset_free_list, &preset->node);
	}
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT */

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT) || defined(CONFIG_BT_HAS_FEATURES_NOTIFIABLE)
	bt_conn_auth_info_cb_register(&auth_info_cb);
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT || CONFIG_BT_HAS_FEATURES_NOTIFIABLE */

	has.registered = true;

	return 0;
}
