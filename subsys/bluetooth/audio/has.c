/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/check.h>

#include <zephyr/device.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/sys/check.h>

#include "../bluetooth/host/conn_internal.h"
#include "../bluetooth/host/hci_core.h"
#include "audio_internal.h"
#include "has_internal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_has, CONFIG_BT_HAS_LOG_LEVEL);

/* The service allows operations with paired devices only.
 * For now, the context is kept for connected devices only, thus the number of contexts is
 * equal to maximum number of simultaneous connections to paired devices.
 */
#define BT_HAS_MAX_CONN MIN(CONFIG_BT_MAX_CONN, CONFIG_BT_MAX_PAIRED)

#define BITS_CHANGED(_new_value, _old_value) ((_new_value) ^ (_old_value))
#define FEATURE_DEVICE_TYPE_UNCHANGED(_new_value) \
	!BITS_CHANGED(_new_value, (has.features & BT_HAS_FEAT_HEARING_AID_TYPE_MASK))
#define FEATURE_SYNC_SUPPORT_UNCHANGED(_new_value) \
	!BITS_CHANGED(_new_value, ((has.features & BT_HAS_FEAT_PRESET_SYNC_SUPP) != 0 ? 1 : 0))
#define FEATURE_IND_PRESETS_UNCHANGED(_new_value) \
	!BITS_CHANGED(_new_value, ((has.features & BT_HAS_FEAT_INDEPENDENT_PRESETS) != 0 ? 1 : 0))

static struct bt_has has;

#if defined(CONFIG_BT_HAS_ACTIVE_PRESET_INDEX)
static void active_preset_index_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_HAS_ACTIVE_PRESET_INDEX */

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
struct has_client;

/* Active preset notification work */
static void active_preset_work_process(struct k_work *work);
static K_WORK_DEFINE(active_preset_work, active_preset_work_process);

static void process_control_point_work(struct k_work *work);
static void read_presets_req_free(struct has_client *client);
static ssize_t write_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *data, uint16_t len, uint16_t offset, uint8_t flags);

static void preset_cp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

static ssize_t read_active_preset_index(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					void *buf, uint16_t len, uint16_t offset)
{
	LOG_DBG("conn %p attr %p offset %d", (void *)conn, attr, offset);

	if (offset > sizeof(has.active_index)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &has.active_index,
				 sizeof(has.active_index));
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

#if defined(CONFIG_BT_HAS_FEATURES_NOTIFIABLE)
/* Features notification work */
static void features_work_process(struct k_work *work);
static K_WORK_DEFINE(features_work, features_work_process);

#define FEATURES_ATTR &has_attrs[2]
#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
#define PRESET_CONTROL_POINT_ATTR &has_attrs[5]
#define ACTIVE_PRESET_INDEX_ATTR &has_attrs[8]
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT */
#else
#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
#define PRESET_CONTROL_POINT_ATTR &has_attrs[4]
#define ACTIVE_PRESET_INDEX_ATTR &has_attrs[7]
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT */
#endif /* CONFIG_BT_HAS_FEATURES_NOTIFIABLE */

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT) || defined(CONFIG_BT_HAS_FEATURES_NOTIFIABLE)
enum {
	FLAG_ACTIVE_INDEX_CHANGED,
	FLAG_CONTROL_POINT_NOTIFY,
	FLAG_FEATURES_CHANGED,
	FLAG_NUM,
};

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
	struct k_work control_point_work;
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT */
	ATOMIC_DEFINE(flags, FLAG_NUM);
} has_client_list[BT_HAS_MAX_CONN];

static struct has_client *client_get_or_new(struct bt_conn *conn)
{
	struct has_client *client = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(has_client_list); i++) {
		if (conn == has_client_list[i].conn) {
			return &has_client_list[i];
		}

		/* first free slot */
		if (!client && !has_client_list[i].conn) {
			client = &has_client_list[i];
		}
	}

	__ASSERT(client, "failed to get client for conn %p", (void *)conn);

	client->conn = bt_conn_ref(conn);

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
	k_work_init(&client->control_point_work, process_control_point_work);
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT */

	return client;
}

static void client_free(struct has_client *client)
{
#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
	(void)k_work_cancel(&client->control_point_work);
	read_presets_req_free(client);
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT */

	atomic_clear_bit(client->flags, FLAG_CONTROL_POINT_NOTIFY);
	atomic_clear_bit(client->flags, FLAG_ACTIVE_INDEX_CHANGED);
	atomic_clear_bit(client->flags, FLAG_FEATURES_CHANGED);

	bt_conn_unref(client->conn);

	client->conn = NULL;
}

static struct has_client *client_get(struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(has_client_list); i++) {
		if (conn == has_client_list[i].conn) {
			return &has_client_list[i];
		}
	}

	return NULL;
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	struct has_client *client;

	LOG_DBG("conn %p level %d err %d", (void *)conn, level, err);

	if (err != BT_SECURITY_ERR_SUCCESS) {
		return;
	}

	client = client_get_or_new(conn);
	if (unlikely(!client)) {
		LOG_ERR("Failed to allocate client");
		return;
	}

	if (!bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
		return;
	}

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
	/* Notify after reconnection */
	if (atomic_test_and_clear_bit(client->flags, FLAG_ACTIVE_INDEX_CHANGED)) {
		/* Emit active preset notification */
		k_work_submit(&active_preset_work);
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_CONTROL_POINT_NOTIFY)) {
		/* Emit preset changed notifications */
		k_work_submit(&client->control_point_work);
	}
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT */

#if defined(CONFIG_BT_HAS_FEATURES_NOTIFIABLE)
	if (atomic_test_and_clear_bit(client->flags, FLAG_FEATURES_CHANGED)) {
		/* Emit preset changed notifications */
		k_work_submit(&features_work);
	}
#endif /* CONFIG_BT_HAS_FEATURES_NOTIFIABLE */
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct has_client *client;

	LOG_DBG("conn %p err %d", conn, err);

	if (err != 0 || !bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
		return;
	}

	client = client_get_or_new(conn);
	if (unlikely(!client)) {
		LOG_ERR("Failed to allocate client");
		return;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct has_client *client;

	LOG_DBG("conn %p reason %d", (void *)conn, reason);

	client = client_get(conn);
	if (client) {
		client_free(client);
	}
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};
#endif /* CONFIG_BT_HAS_PRESET_SUPPORT || CONFIG_BT_HAS_FEATURES_NOTIFIABLE */

#if defined(CONFIG_BT_HAS_PRESET_SUPPORT)
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
} has_preset_list[CONFIG_BT_HAS_PRESET_COUNT];

/* Number of registered presets */
static uint8_t has_preset_num;

static bool read_presets_req_pending_cp(const struct has_client *client)
{
	return client->read_presets_req.num_presets > 0;
}

static void read_presets_req_free(struct has_client *client)
{
	client->read_presets_req.num_presets = 0;
}

typedef uint8_t (*preset_func_t)(const struct has_preset *preset, void *user_data);

static void preset_foreach(uint8_t start_index, uint8_t end_index, preset_func_t func,
			   void *user_data)
{
	for (size_t i = 0; i < ARRAY_SIZE(has_preset_list); i++) {
		const struct has_preset *preset = &has_preset_list[i];

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

static int preset_index_compare(const void *p1, const void *p2)
{
	const struct has_preset *preset_1 = p1;
	const struct has_preset *preset_2 = p2;

	if (preset_1->index == BT_HAS_PRESET_INDEX_NONE) {
		return 1;
	}

	if (preset_2->index == BT_HAS_PRESET_INDEX_NONE) {
		return -1;
	}

	return preset_1->index - preset_2->index;
}

static struct has_preset *preset_alloc(uint8_t index, enum bt_has_properties properties,
				       const char *name, const struct bt_has_preset_ops *ops)
{
	struct has_preset *preset = NULL;

	if (has_preset_num < ARRAY_SIZE(has_preset_list)) {
		preset = &has_preset_list[has_preset_num];
		preset->index = index;
		preset->properties = properties;
#if defined(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)
		utf8_lcpy(preset->name, name, ARRAY_SIZE(preset->name));
#else
		preset->name = name;
#endif /* CONFIG_BT_HAS_PRESET_NAME_DYNAMIC */
		preset->ops = ops;

		has_preset_num++;

		/* sort the presets in index ascending order */
		qsort(has_preset_list, has_preset_num, sizeof(*preset), preset_index_compare);
	}

	return preset;
}

static void preset_free(struct has_preset *preset)
{
	preset->index = BT_HAS_PRESET_INDEX_NONE;

	/* sort the presets in index ascending order */
	if (has_preset_num > 1) {
		qsort(has_preset_list, has_preset_num, sizeof(*preset), preset_index_compare);
	}

	has_preset_num--;
}

static void control_point_ntf_complete(struct bt_conn *conn, void *user_data)
{
	struct has_client *client = client_get(conn);

	LOG_DBG("conn %p", (void *)conn);

	/* Resubmit if needed */
	if (client != NULL &&
	    (read_presets_req_pending_cp(client) ||
	     atomic_test_and_clear_bit(client->flags, FLAG_CONTROL_POINT_NOTIFY))) {
		k_work_submit(&client->control_point_work);
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
#if defined(CONFIG_BT_HAS_PRESET_CONTROL_POINT_NOTIFIABLE)
	if (bt_eatt_count(client->conn) > 0 &&
	    bt_gatt_is_subscribed(client->conn, PRESET_CONTROL_POINT_ATTR, BT_GATT_CCC_NOTIFY)) {
		client->params.ntf.attr = PRESET_CONTROL_POINT_ATTR;
		client->params.ntf.func = control_point_ntf_complete;
		client->params.ntf.data = buf->data;
		client->params.ntf.len = buf->len;

		return bt_gatt_notify_cb(client->conn, &client->params.ntf);
	}
#endif /* CONFIG_BT_HAS_PRESET_CONTROL_POINT_NOTIFIABLE */

	if (bt_gatt_is_subscribed(client->conn, PRESET_CONTROL_POINT_ATTR, BT_GATT_CCC_INDICATE)) {
		client->params.ind.attr = PRESET_CONTROL_POINT_ATTR;
		client->params.ind.func = control_point_ind_complete;
		client->params.ind.destroy = NULL;
		client->params.ind.data = buf->data;
		client->params.ind.len = buf->len;

		return bt_gatt_indicate(client->conn, &client->params.ind);
	}

	return -ECANCELED;
}

static int control_point_send_all(struct net_buf_simple *buf)
{
	int result = 0;

	for (size_t i = 0; i < ARRAY_SIZE(has_client_list); i++) {
		struct has_client *client = &has_client_list[i];
		int err;

		if (!client->conn) {
			/* Mark preset changed operation as pending */
			atomic_set_bit(client->flags, FLAG_CONTROL_POINT_NOTIFY);
			/* For simplicity we simply start with the first index,
			 * rather than keeping detailed logs of which clients
			 * have knowledge of which presets
			 */
			client->preset_changed_index_next = BT_HAS_PRESET_INDEX_FIRST;
			continue;
		}

		if (!bt_gatt_is_subscribed(client->conn, PRESET_CONTROL_POINT_ATTR,
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

	LOG_DBG("conn %p preset %p is_last 0x%02x", (void *)client->conn, preset, is_last);

	hdr = net_buf_simple_add(&buf, sizeof(*hdr));
	hdr->opcode = BT_HAS_OP_READ_PRESET_RSP;
	rsp = net_buf_simple_add(&buf, sizeof(*rsp));
	rsp->is_last = is_last ? 0x01 : 0x00;
	rsp->index = preset->index;
	rsp->properties = preset->properties;
	net_buf_simple_add_mem(&buf, preset->name, strlen(preset->name));

	return control_point_send(client, &buf);
}

static uint8_t get_prev_preset_index(const struct has_preset *preset)
{
	const struct has_preset *prev = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(has_preset_list); i++) {
		const struct has_preset *tmp = &has_preset_list[i];

		if (tmp->index == BT_HAS_PRESET_INDEX_NONE || tmp == preset) {
			break;
		}

		prev = tmp;
	}

	return prev ? prev->index : BT_HAS_PRESET_INDEX_NONE;
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

static int bt_has_cp_generic_update(struct has_client *client, const struct has_preset *preset,
				    uint8_t is_last)
{
	struct bt_has_cp_generic_update *generic_update;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(struct bt_has_cp_hdr) +
			      sizeof(struct bt_has_cp_preset_changed) +
			      sizeof(struct bt_has_cp_generic_update) + BT_HAS_PRESET_NAME_MAX);

	preset_changed_prepare(&buf, BT_HAS_CHANGE_ID_GENERIC_UPDATE, is_last);

	generic_update = net_buf_simple_add(&buf, sizeof(*generic_update));
	generic_update->prev_index = get_prev_preset_index(preset);
	generic_update->index = preset->index;
	generic_update->properties = preset->properties;
	net_buf_simple_add_mem(&buf, preset->name, strlen(preset->name));

	if (client) {
		return control_point_send(client, &buf);
	} else {
		return control_point_send_all(&buf);
	}
}

static void process_control_point_work(struct k_work *work)
{
	struct has_client *client = CONTAINER_OF(work, struct has_client, control_point_work);
	int err;

	if (!client->conn) {
		return;
	}

	if (read_presets_req_pending_cp(client)) {
		const struct has_preset *preset = NULL;
		bool is_last = true;

		preset_foreach(client->read_presets_req.start_index, BT_HAS_PRESET_INDEX_LAST,
			       preset_found, &preset);

		if (unlikely(preset == NULL)) {
			(void)bt_has_cp_read_preset_rsp(client, NULL, 0x01);

			return;
		}

		if (client->read_presets_req.num_presets > 1) {
			const struct has_preset *next = NULL;

			preset_foreach(preset->index + 1, BT_HAS_PRESET_INDEX_LAST,
				       preset_found, &next);

			is_last = next == NULL;

		}

		err = bt_has_cp_read_preset_rsp(client, preset, is_last);
		if (err) {
			LOG_ERR("bt_has_cp_read_preset_rsp failed (err %d)", err);
		}

		if (err || is_last) {
			read_presets_req_free(client);
		} else {
			client->read_presets_req.start_index = preset->index + 1;
			client->read_presets_req.num_presets--;
		}
	} else if (atomic_test_and_clear_bit(client->flags, FLAG_CONTROL_POINT_NOTIFY)) {
		const struct has_preset *preset = NULL;
		const struct has_preset *next = NULL;
		bool is_last = true;

		preset_foreach(client->preset_changed_index_next,
			       BT_HAS_PRESET_INDEX_LAST, preset_found, &preset);

		if (preset == NULL) {
			return;
		}

		preset_foreach(preset->index + 1, BT_HAS_PRESET_INDEX_LAST,
			       preset_found, &next);

		is_last = next == NULL;

		err = bt_has_cp_generic_update(client, preset, is_last);
		if (err) {
			LOG_ERR("bt_has_cp_read_preset_rsp failed (err %d)", err);
		}

		if (err || is_last) {
			atomic_clear_bit(client->flags, FLAG_CONTROL_POINT_NOTIFY);
		} else {
			client->preset_changed_index_next = preset->index + 1;
		}
	}
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
	if (!bt_gatt_is_subscribed(conn, PRESET_CONTROL_POINT_ATTR, BT_GATT_CCC_INDICATE)) {
		return BT_ATT_ERR_CCC_IMPROPER_CONF;
	}

	client = client_get(conn);
	if (!client) {
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
	if (read_presets_req_pending_cp(client)) {
		return BT_HAS_ERR_OPERATION_NOT_POSSIBLE;
	}

	/* Store the request */
	client->read_presets_req.start_index = req->start_index;
	client->read_presets_req.num_presets = req->num_presets;

	k_work_submit(&client->control_point_work);

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

	return bt_has_cp_generic_update(NULL, preset, BT_HAS_IS_LAST);
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
	if (!bt_gatt_is_subscribed(conn, PRESET_CONTROL_POINT_ATTR, BT_GATT_CCC_INDICATE)) {
		return BT_ATT_ERR_CCC_IMPROPER_CONF;
	}

	client = client_get(conn);
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

static void active_preset_work_process(struct k_work *work)
{
	const uint8_t active_index = bt_has_preset_active_get();

	for (size_t i = 0U; i < ARRAY_SIZE(has_client_list); i++) {
		struct has_client *client = &has_client_list[i];
		int err;

		if (client->conn == NULL) {
			/* mark to notify on reconnect */
			atomic_set_bit(client->flags, FLAG_ACTIVE_INDEX_CHANGED);
			continue;
		} else if (atomic_test_and_clear_bit(client->flags, FLAG_ACTIVE_INDEX_CHANGED)) {
			err = bt_gatt_notify(client->conn, ACTIVE_PRESET_INDEX_ATTR, &active_index,
					     sizeof(active_index));
			if (err != 0) {
				LOG_DBG("failed to notify active_index for %p: %d", client->conn,
				     err);
			}
		}
	}
}

static void preset_active_set(uint8_t index)
{
	if (index != has.active_index) {
		has.active_index = index;

	for (size_t i = 0U; i < ARRAY_SIZE(has_client_list); i++) {
		struct has_client *client = &has_client_list[i];
			/* mark to notify */
		atomic_set_bit(client->flags, FLAG_ACTIVE_INDEX_CHANGED);
	}

		/* Emit active preset notification */
		k_work_submit(&active_preset_work);
	}
}

static uint8_t preset_select(const struct has_preset *preset, bool sync)
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

	preset_active_set(preset->index);

	return 0;
}

static uint8_t handle_set_active_preset(struct net_buf_simple *buf, bool sync)
{
	const struct bt_has_cp_set_active_preset *pdu;
	const struct has_preset *preset = NULL;

	if (buf->len < sizeof(*pdu)) {
		return BT_HAS_ERR_INVALID_PARAM_LEN;
	}

	pdu = net_buf_simple_pull_mem(buf, sizeof(*pdu));

	preset_foreach(pdu->index, pdu->index, preset_found, &preset);
	if (preset == NULL) {
		return BT_ATT_ERR_OUT_OF_RANGE;
	}

	if (!(preset->properties & BT_HAS_PROP_AVAILABLE)) {
		return BT_HAS_ERR_OPERATION_NOT_POSSIBLE;
	}

	return preset_select(preset, sync);
}

static uint8_t handle_set_next_preset(bool sync)
{
	const struct has_preset *next_avail = NULL;
	const struct has_preset *first_avail = NULL;

	for (size_t i = 0; i < has_preset_num; i++) {
		const struct has_preset *tmp = &has_preset_list[i];

		if (tmp->index == BT_HAS_PRESET_INDEX_NONE) {
			break;
		}

		if (!(tmp->properties & BT_HAS_PROP_AVAILABLE)) {
			continue;
		}

		if (tmp->index < has.active_index && !first_avail) {
			first_avail = tmp;
			continue;
		}

		if (tmp->index > has.active_index) {
			next_avail = tmp;
			break;
		}
	}

	if (next_avail) {
		return preset_select(next_avail, sync);
	}

	if (first_avail) {
		return preset_select(first_avail, sync);
	}

	return BT_HAS_ERR_OPERATION_NOT_POSSIBLE;
}

static uint8_t handle_set_prev_preset(bool sync)
{
	const struct has_preset *prev_available = NULL;
	const struct has_preset *last_available = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(has_preset_list); i++) {
		const struct has_preset *tmp = &has_preset_list[i];

		if (tmp->index == BT_HAS_PRESET_INDEX_NONE) {
			break;
		}

		if (!(tmp->properties & BT_HAS_PROP_AVAILABLE)) {
			continue;
		}

		if (tmp->index < has.active_index) {
			prev_available = tmp;
			continue;
		}

		if (prev_available) {
			break;
		}

		if (tmp->index > has.active_index) {
			last_available = tmp;
			continue;
		}
	}

	if (prev_available) {
		return preset_select(prev_available, sync);
	}

	if (last_available) {
		return preset_select(last_available, sync);
	}

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
		LOG_WRN("err 0x%02x", err);
		return BT_GATT_ERR(err);
	}

	return len;
}

int bt_has_preset_register(const struct bt_has_preset_register_param *param)
{
	struct has_preset *preset = NULL;
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

	preset_foreach(param->index, param->index, preset_found, &preset);
	if (preset != NULL) {
		return -EALREADY;
	}

	preset = preset_alloc(param->index, param->properties, param->name, param->ops);
	if (preset == NULL) {
		return -ENOMEM;
	}

	return bt_has_cp_generic_update(NULL, preset, BT_HAS_IS_LAST);
}

int bt_has_preset_unregister(uint8_t index)
{
	struct has_preset *preset = NULL;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(struct bt_has_cp_hdr) +
			      sizeof(struct bt_has_cp_preset_changed) + sizeof(uint8_t));

	CHECKIF(index == BT_HAS_PRESET_INDEX_NONE) {
		LOG_ERR("index is invalid");
		return -EINVAL;
	}

	preset_foreach(index, index, preset_found, &preset);
	if (preset == NULL) {
		return -ENOENT;
	}

	preset_changed_prepare(&buf, BT_HAS_CHANGE_ID_PRESET_DELETED, BT_HAS_IS_LAST);
	net_buf_simple_add_u8(&buf, preset->index);

	preset_free(preset);

	return control_point_send_all(&buf);
}

int bt_has_preset_available(uint8_t index)
{
	struct has_preset *preset = NULL;

	CHECKIF(index == BT_HAS_PRESET_INDEX_NONE) {
		LOG_ERR("index is invalid");
		return -EINVAL;
	}

	preset_foreach(index, index, preset_found, &preset);
	if (preset == NULL) {
		return -ENOENT;
	}

	/* toggle property bit if needed */
	if (!(preset->properties & BT_HAS_PROP_AVAILABLE)) {
		NET_BUF_SIMPLE_DEFINE(buf, sizeof(struct bt_has_cp_hdr) +
				      sizeof(struct bt_has_cp_preset_changed) + sizeof(uint8_t));

		preset->properties ^= BT_HAS_PROP_AVAILABLE;

		preset_changed_prepare(&buf, BT_HAS_CHANGE_ID_PRESET_AVAILABLE, BT_HAS_IS_LAST);
		net_buf_simple_add_u8(&buf, preset->index);

		return control_point_send_all(&buf);
	}

	return 0;
}

int bt_has_preset_unavailable(uint8_t index)
{
	struct has_preset *preset = NULL;

	CHECKIF(index == BT_HAS_PRESET_INDEX_NONE) {
		LOG_ERR("index is invalid");
		return -EINVAL;
	}

	preset_foreach(index, index, preset_found, &preset);
	if (preset == NULL) {
		return -ENOENT;
	}

	/* toggle property bit if needed */
	if (preset->properties & BT_HAS_PROP_AVAILABLE) {
		NET_BUF_SIMPLE_DEFINE(buf, sizeof(struct bt_has_cp_hdr) +
				      sizeof(struct bt_has_cp_preset_changed) + sizeof(uint8_t));

		preset->properties ^= BT_HAS_PROP_AVAILABLE;

		preset_changed_prepare(&buf, BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE, BT_HAS_IS_LAST);
		net_buf_simple_add_u8(&buf, preset->index);

		return control_point_send_all(&buf);
	}

	return 0;
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
	if (index != BT_HAS_PRESET_INDEX_NONE) {
		struct has_preset *preset = NULL;

		preset_foreach(index, index, preset_found, &preset);
		if (preset == NULL) {
			return -ENOENT;
		}

		if (!(preset->properties & BT_HAS_PROP_AVAILABLE)) {
			return -EINVAL;
		}
	}

	preset_active_set(index);

	return 0;
}

uint8_t bt_has_preset_active_get(void)
{
	return has.active_index;
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
static void features_work_process(struct k_work *work)
{
	for (size_t i = 0U; i < ARRAY_SIZE(has_client_list); i++) {
		struct has_client *client = &has_client_list[i];
		int err;

		if (client->conn == NULL) {
			/* mark to notify on reconnect */
			atomic_set_bit(client->flags, FLAG_FEATURES_CHANGED);
			continue;
		} else if (atomic_test_and_clear_bit(client->flags, FLAG_CONTROL_POINT_NOTIFY)) {
			err = bt_gatt_notify(client->conn, FEATURES_ATTR, &has.features,
					     sizeof(has.features));
			if (err != 0) {
				LOG_DBG("failed to notify features for %p: %d",	client->conn, err);
			}
		}
	}
}

int bt_has_features_set(const struct bt_has_features_param *features)
{
	uint8_t tmp_features;
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

	tmp_features = has.features;

	err = has_features_register(features);
	if (err != 0) {
		LOG_DBG("Failed to register features");
		return err;
	}

	bool tmp_pending_ntf_features[ARRAY_SIZE(has_client_list)];

	for (size_t i = 0U; i < ARRAY_SIZE(has_client_list); i++) {
		struct has_client *client = &has_client_list[i];
		/* save old state */
		tmp_pending_ntf_features[i] = atomic_test_bit(client->flags, FLAG_FEATURES_CHANGED);
		/* mark to notify */
		atomic_set_bit(client->flags, FLAG_FEATURES_CHANGED);
	}

	err = k_work_submit(&features_work);
	if (err < 0) {
		/* restore old values */
		for (size_t i = 0U; i < ARRAY_SIZE(has_client_list); i++) {
			struct has_client *client = &has_client_list[i];

			atomic_set_bit_to(client->flags, FLAG_FEATURES_CHANGED,
					  tmp_pending_ntf_features[i]);
		}
		has.features = tmp_features;

		return err;
	}

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

	has.registered = true;

	return 0;
}
