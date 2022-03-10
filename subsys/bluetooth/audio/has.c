/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <device.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <net/buf.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/check.h>
#include <sys/math_extras.h>
#include <sys/util.h>

#include <bluetooth/audio/has.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#include "has_internal.h"
#include "../host/conn_internal.h"
#include "../host/hci_core.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HAS)
#define LOG_MODULE_NAME bt_has
#include "common/log.h"

#define BT_HAS_MAX_CLIENT MIN(CONFIG_BT_MAX_CONN, CONFIG_BT_MAX_PAIRED)
#define BT_HAS_CP_WORK_TIMEOUT K_MSEC(10)
#define ATT_HANDLE_VALUE_IND_HDR_SIZE 3
#define BT_HAS_CP_TX_MTU (ATT_HANDLE_VALUE_IND_HDR_SIZE + 1 /* CP Opcode */ + \
                          2 /* Preset Changed */ + 3 /* Generic Update */ + \
                          BT_HAS_PRESET_NAME_MAX)
#define BT_HAS_TX_BUF_SIZE (BT_HAS_CP_TX_MTU - ATT_HANDLE_VALUE_IND_HDR_SIZE)

enum {
	CLIENT_FLAG_SEC_ENABLED,        /* if encrypted with sufficient security level */
	CLIENT_FLAG_ATT_MTU_VALID,      /* if ATT MTU meet HAS specification requirements */
	CLIENT_FLAG_CP_IND_ENABLED,     /* if Control Point indications are enabled */
	CLIENT_FLAG_CP_NTF_ENABLED,     /* if Control Point notifications are enabled */
	CLIENT_FLAG_CP_BUSY,            /* if Control Point operation is in progress */

	/* Total number of flags - must be at the end */
	CLIENT_NUM_FLAGS,
};

#if defined(CONFIG_BT_HAS)
static struct has_preset {
	uint8_t index;
	uint8_t properties;
#if defined(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)
	char name[BT_HAS_PRESET_NAME_MAX];
#else
	const char *name;
#endif /* CONFIG_BT_HAS_PRESET_NAME_DYNAMIC */
	bool hidden;
} has_preset_list[CONFIG_BT_HAS_PRESET_CNT];

/* Separate Preset Control Point configuration for each bonded device */
static struct has_client {
	/** Connection object */
	struct bt_conn *conn;
	/** Pending preset changed operation per each preset */
	struct {
		/* Preset changed pending if flag set */
		ATOMIC_DEFINE(pending, CONFIG_BT_HAS_PRESET_CNT);
		/* ChangeId to be sent */
		uint8_t change_id[CONFIG_BT_HAS_PRESET_CNT];
	} preset_changed;
	/** Read preset operation data */
	struct {
		/** Preset pending to be sent */
		struct has_preset *preset;
		/** Number of remaining presets to send */
		uint8_t num_presets;
	} read_preset_rsp;
	union {
		/** Control Point Indicate parameters */
		struct bt_gatt_indicate_params ind;
		/** Control Point Notify parameters */
		struct bt_gatt_notify_params ntf;
	} params;
	/* Control Point Tx work */
	struct k_work_delayable control_point_work;
	/* Control Point Tx sync state */
	struct k_work_sync control_point_sync;
	/* Internal flags */
	ATOMIC_DEFINE(flags, CLIENT_NUM_FLAGS);
} has_client_list[BT_HAS_MAX_CLIENT];

static struct bt_has has_local;
static struct bt_has_ops *preset_ops;
static uint8_t last_preset_index;
static struct k_work active_preset_work;
NET_BUF_SIMPLE_DEFINE_STATIC(control_point_buf, BT_HAS_TX_BUF_SIZE);

static ssize_t control_point_rx(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *data, uint16_t len, uint16_t offset, uint8_t flags);
static void process_control_point_work(struct k_work *work);

static struct has_client *client_get(struct bt_conn *conn)
{
	struct has_client *client = NULL;

	BT_DBG("conn %p", conn);

	for (size_t i = 0; i < ARRAY_SIZE(has_client_list); i++) {
		if (conn == has_client_list[i].conn) {
			return &has_client_list[i];
		}

		/* first free slot */
		if (!client && !has_client_list[i].conn) {
			client = &has_client_list[i];
		}
	}

	BT_ASSERT_MSG(client, "failed to alloc client %p", conn);

	client->conn = bt_conn_ref(conn);
	k_work_init_delayable(&client->control_point_work, process_control_point_work);

	return client;
}

static void client_free(struct has_client *client)
{
	BT_DBG("%p", client);

	/* Cancel ongoing work. Since the client can be re-used after this
	 * we need to sync to make sure that the kernel does not have it
	 * in its queue anymore.
	 */
	k_work_cancel_delayable_sync(&client->control_point_work, &client->control_point_sync);

	bt_conn_unref(client->conn);

	(void)memset(client, 0, sizeof(*client));
}

static struct has_client *client_lookup_conn(const struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(has_client_list); i++) {
		if (conn == has_client_list[i].conn) {
			return &has_client_list[i];
		}
	}

	return NULL;
}

static ssize_t read_features(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p offset %d", conn, attr, offset);

	if (offset > sizeof(has_local.features)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &has_local.features,
				 sizeof(has_local.features));
}

static ssize_t read_active_preset_index(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					void *buf, uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p offset %d", conn, attr, offset);

	if (offset > sizeof(has_local.active_index)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &has_local.active_index,
				 sizeof(has_local.active_index));
}

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

static ssize_t cp_ccc_cfg_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				uint16_t value)
{
	struct has_client *client;

	BT_DBG("conn %p, value 0x%04x", conn, value);

	client = client_get(conn);

	if (value | BT_GATT_CCC_INDICATE) {
		atomic_set_bit(client->flags, CLIENT_FLAG_CP_IND_ENABLED);
	} else {
		atomic_clear_bit(client->flags, CLIENT_FLAG_CP_IND_ENABLED);
	}

	if (value | BT_GATT_CCC_NOTIFY) {
		atomic_set_bit(client->flags, CLIENT_FLAG_CP_NTF_ENABLED);
	} else {
		atomic_clear_bit(client->flags, CLIENT_FLAG_CP_NTF_ENABLED);
	}

	/* Cancel work once client unsubscribed */
	if (!atomic_test_bit(client->flags, CLIENT_FLAG_CP_NTF_ENABLED) &&
	    !atomic_test_bit(client->flags, CLIENT_FLAG_CP_IND_ENABLED)) {
		k_work_cancel_delayable_sync(&client->control_point_work, &client->control_point_sync);
		atomic_clear_bit(client->flags, CLIENT_FLAG_CP_BUSY);
	}

	return sizeof(value);
}

/* Preset Control Point CCC */
static struct _bt_gatt_ccc cp_ccc_cfg =
	BT_GATT_CCC_INITIALIZER(ccc_cfg_changed, cp_ccc_cfg_write, NULL);

/* Hearing Access Service GATT Attributes */
BT_GATT_SERVICE_DEFINE(
	has_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_HAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_HEARING_AID_FEATURES, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_ENCRYPT, read_features, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_PRESET_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_WRITE_ENCRYPT, NULL, control_point_rx, NULL),
	BT_GATT_CCC_MANAGED(&cp_ccc_cfg, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_ACTIVE_PRESET_INDEX,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ_ENCRYPT,
			       read_active_preset_index, NULL, NULL),
	BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT));

static int preset_changed_popcount(const struct has_client *client)
{
#if CONFIG_BT_HAS_PRESET_CNT > 32
	int count = 0;

	for (size_t i = 0; i < ARRAY_SIZE(client->preset_changed.pending); i++) {
		count += popcount(atomic_get(&client->preset_changed.pending[i]));
	}

	return count;
#else
	return popcount(atomic_get(client->preset_changed.pending));
#endif
}

static bool is_preset_changed_pending(const struct has_client *client)
{
	return preset_changed_popcount(client) > 0;
}

typedef bool (*preset_func_t)(struct has_preset *preset, void *user_data);

static void preset_foreach(uint8_t start_index, uint8_t end_index, uint8_t num_matches,
			   preset_func_t func, void *user_data)
{
	if (!num_matches) {
		num_matches = UINT8_MAX;
	}

	if (start_index <= last_preset_index) {
		for (size_t i = 0; i < ARRAY_SIZE(has_preset_list); i++) {
			struct has_preset *preset = &has_preset_list[i];

			/* Skip ahead if index is not within requested range */
			if (preset->index < start_index) {
				continue;
			}

			if (preset->index > end_index) {
				return;
			}

			if (func && !func(preset, user_data)) {
				continue;
			}

			if (--num_matches > 0) {
				continue;
			}

			return;
		}
	}
}

static uint8_t preset_list_index(const struct has_preset *preset)
{
	const ptrdiff_t index = preset - has_preset_list;

	__ASSERT(index >= 0 && index < ARRAY_SIZE(has_preset_list), "Invalid preset pointer");

	return (uint8_t)index;
}

static struct has_preset *preset_get(uint8_t index)
{
	for (size_t i = 0; i < ARRAY_SIZE(has_preset_list); i++) {
		struct has_preset *preset = &has_preset_list[i];

		if (preset->index == index) {
			return preset;
		}
	}

	return NULL;
}

static void active_preset_work_process(struct k_work *work)
{
	bt_gatt_notify_uuid(NULL, BT_UUID_HAS_ACTIVE_PRESET_INDEX, has_svc.attrs,
			    &has_local.active_index, sizeof(has_local.active_index));
}

static bool is_read_preset_rsp_pending(const struct has_client *client)
{
	return client->read_preset_rsp.preset != NULL &&
	       client->read_preset_rsp.num_presets > 0;
}

static void control_point_work_submit(struct has_client *client, k_timeout_t delay)
{
	if (!atomic_test_and_set_bit(client->flags, CLIENT_FLAG_CP_BUSY)) {
		k_work_reschedule(&client->control_point_work, delay);
	}
}

static void control_point_send_done(struct bt_conn *conn, void *user_data)
{
	struct has_client *client = client_get(conn);

	BT_DBG("client %p\n", client);

	atomic_clear_bit(client->flags, CLIENT_FLAG_CP_BUSY);

	/* Resubmit if needed */
	if (is_preset_changed_pending(client) || is_read_preset_rsp_pending(client)) {
		control_point_work_submit(client, K_NO_WAIT);
	}
}

static void control_point_ind_done(struct bt_conn *conn, struct bt_gatt_indicate_params *params,
				   uint8_t err)
{
	if (err) {
		BT_ERR("conn %p err 0x%02x\n", conn, err);
	}

	control_point_send_done(conn, NULL);
}

static int control_point_send(struct has_client *client, struct net_buf_simple *buf)
{
	/* TODO: Decide whether to notify or indicate */
	if (atomic_test_bit(client->flags, CLIENT_FLAG_CP_NTF_ENABLED)) {
		client->params.ntf.attr = &has_svc.attrs[4];
		client->params.ntf.func = control_point_send_done;
		client->params.ntf.data = buf->data;
		client->params.ntf.len = buf->len;

		return bt_gatt_notify_cb(client->conn, &client->params.ntf);
	}

	if (atomic_test_bit(client->flags, CLIENT_FLAG_CP_IND_ENABLED)) {
		client->params.ind.attr = &has_svc.attrs[4];
		client->params.ind.func = control_point_ind_done;
		client->params.ind.destroy = NULL;
		client->params.ind.data = buf->data;
		client->params.ind.len = buf->len;

		return bt_gatt_indicate(client->conn, &client->params.ind);
	}

	return -ECANCELED;
}

/** Returns the first found preset index pending preset changed indication  */
static struct has_preset *get_first_preset_changed(struct has_client *client, uint8_t *change_id)
{
	int i = 0;

#if CONFIG_BT_HAS_PRESET_CNT > 32
	for (size_t j = 0; i < ARRAY_SIZE(client->preset_changed.pending); j++) {
		if (popcount(atomic_get(&client->preset_changed.pending[j])) > 0) {
			i = u32_count_trailing_zeros(
				atomic_get(&client->preset_changed.pending[j])) + j * 32;
			break;
		}
	}
#else
	i = u32_count_trailing_zeros(atomic_get(client->preset_changed.pending));
#endif
	__ASSERT(i >= 0 && i < ARRAY_SIZE(has_preset_list), "Invalid client pointer");

	*change_id = client->preset_changed.change_id[i];

	return &has_preset_list[i];
}

static int bt_has_cp_read_preset_rsp(struct has_client *client, struct has_preset *preset,
				     bool is_last)
{
	struct bt_has_cp_hdr *hdr;
	struct bt_has_cp_read_preset_rsp *rsp;
	const size_t slen = strlen(preset->name);

	BT_DBG("client %p preset %p is_last %d", client, preset, is_last);

	net_buf_simple_reset(&control_point_buf);

	hdr = net_buf_simple_add(&control_point_buf, sizeof(*hdr));
	hdr->op = BT_HAS_OP_READ_PRESET_RSP;
	rsp = net_buf_simple_add(&control_point_buf, sizeof(*rsp));
	rsp->is_last = is_last ? 0x01 : 0x00;
	rsp->index = preset->index;
	rsp->properties = preset->properties;
	net_buf_simple_add_mem(&control_point_buf, preset->name, slen);

	return control_point_send(client, &control_point_buf);
}

static bool find_available(struct has_preset *preset, void *user_data)
{
	struct has_preset **found = user_data;

	if ((preset->properties & BT_HAS_PROP_AVAILABLE) && !preset->hidden) {
		*found = preset;
		return true;
	}

	return false;
}

static int bt_has_cp_preset_changed(struct has_client *client, struct has_preset *preset,
				    uint8_t change_id, bool is_last)
{
	struct bt_has_cp_hdr *hdr;
	struct bt_has_cp_preset_changed *pc;

	BT_DBG("client %p preset %p is_last %d", client, preset, is_last);

	net_buf_simple_reset(&control_point_buf);

	hdr = net_buf_simple_add(&control_point_buf, sizeof(*hdr));
	hdr->op = BT_HAS_OP_PRESET_CHANGED;
	pc = net_buf_simple_add(&control_point_buf, sizeof(*pc));
	pc->change_id = change_id;
	pc->is_last = is_last ? 0x01 : 0x00;

	switch (pc->change_id) {
	case BT_HAS_CHANGE_ID_GENERIC_UPDATE: {
		struct bt_has_cp_generic_update *gu;
		struct has_preset *prev = NULL;

		/* Lookup previous preset */
		preset_foreach(1, preset->index - 1, 0, find_available, &prev);

		gu = net_buf_simple_add(&control_point_buf, sizeof(*gu));
		gu->prev_index = prev ? prev->index : BT_HAS_PRESET_INDEX_NONE;
		gu->index = preset->index;
		gu->properties = preset->properties;
		net_buf_simple_add_mem(&control_point_buf, preset->name, strlen(preset->name));
	} break;
	case BT_HAS_CHANGE_ID_PRESET_DELETED:
	case BT_HAS_CHANGE_ID_PRESET_AVAILABLE:
	case BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE:
		net_buf_simple_add_u8(&control_point_buf, preset->index);
		break;
	default:
		return -EINVAL;
	}

	return control_point_send(client, &control_point_buf);
}

static void preset_changed_set(struct has_client *client, uint8_t index, uint8_t change_id)
{
	client->preset_changed.change_id[index] = change_id;
	atomic_set_bit(client->preset_changed.pending, index);
}

static void preset_changed_clear(struct has_client *client, uint8_t index)
{
	atomic_clear_bit(client->preset_changed.pending, index);
}

static void preset_changed_clear_all(struct has_client *client)
{
	atomic_clear(client->preset_changed.pending);
}

static bool find_visible(struct has_preset *preset, void *user_data)
{
	struct has_preset **found = user_data;

	if (!preset->hidden) {
		*found = preset;
		return true;
	}

	return false;
}

static void process_control_point_work(struct k_work *work)
{
	struct has_client *client = CONTAINER_OF(work, struct has_client, control_point_work);
	struct has_preset *preset, *next;
	int err;

	if (!client->conn || client->conn->state != BT_CONN_CONNECTED) {
		err = -ENOTCONN;
	} else if (is_read_preset_rsp_pending(client)) {
		preset = client->read_preset_rsp.preset;
		next = NULL;

		if (client->read_preset_rsp.num_presets > 1) {
			preset_foreach(preset->index + 1, last_preset_index, 1, find_visible, &next);
		}

		err = bt_has_cp_read_preset_rsp(client, preset, !next);
		if (!err) {
			client->read_preset_rsp.preset = next;
			client->read_preset_rsp.num_presets--;
		}
	} else if (is_preset_changed_pending(client)) {
		bool is_last = preset_changed_popcount(client) == 1;
		uint8_t change_id;

		preset = get_first_preset_changed(client, &change_id);

		err = bt_has_cp_preset_changed(client, preset, change_id, is_last);
	} else {
		err = -ENODATA;
	}

	if (err) {
		atomic_clear_bit(client->flags, CLIENT_FLAG_CP_BUSY);
	} else {
		preset_changed_clear(client, preset_list_index(preset));
	}
}

static int handle_read_preset_req(struct has_client *client, struct net_buf_simple *buf)
{
	const struct bt_has_cp_read_preset_req *req;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_HAS_ERR_INVALID_PARAM_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("start_index %d num_presets %d", req->start_index, req->num_presets);

	/* Reject if already in progress */
	if (is_read_preset_rsp_pending(client)) {
		return BT_GATT_ERR(BT_HAS_ERR_OPERATION_NOT_POSSIBLE);
	}

	client->read_preset_rsp.preset = NULL;
	if (req->num_presets > 0) {
		preset_foreach(req->start_index, last_preset_index, 1, find_visible,
			       &client->read_preset_rsp.preset);
	}

	if (!client->read_preset_rsp.preset) {
		return BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE);
	}

	client->read_preset_rsp.num_presets = req->num_presets;

	control_point_work_submit(client, BT_HAS_CP_WORK_TIMEOUT);

	return 0;
}

static void preset_changed(struct has_preset *preset, uint8_t change_id)
{
	const uint8_t list_index = preset_list_index(preset);

	BT_DBG("preset %p %s", preset, bt_has_change_id_str(change_id));

	/* update the pending changeId flags */
	for (size_t i = 0; i < ARRAY_SIZE(has_client_list); i++) {
		struct has_client *client = &has_client_list[i];

		if (!atomic_test_bit(client->flags, CLIENT_FLAG_CP_IND_ENABLED) &&
		    !atomic_test_bit(client->flags, CLIENT_FLAG_CP_NTF_ENABLED)) {
			continue;
		}

		if (!client->conn || client->conn->state != BT_CONN_CONNECTED) {
			continue;
		}

		if (atomic_test_bit(client->preset_changed.pending, list_index)) {
			uint8_t change_id_pending = client->preset_changed.change_id[list_index];

			switch (change_id) {
			case BT_HAS_CHANGE_ID_GENERIC_UPDATE:
				if (change_id_pending == BT_HAS_CHANGE_ID_PRESET_DELETED) {
					/* skip visibility toggle */
					preset_changed_clear(client, list_index);
				} else {
					preset_changed_set(client, list_index, change_id);
				}

				break;
			case BT_HAS_CHANGE_ID_PRESET_DELETED:
				if (change_id_pending == BT_HAS_CHANGE_ID_GENERIC_UPDATE) {
					/* skip visibility toggle */
					preset_changed_clear(client, list_index);
				} else {
					preset_changed_set(client, list_index, change_id);
				}

				break;
			case BT_HAS_CHANGE_ID_PRESET_AVAILABLE:
				if (change_id_pending == BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE) {
					/* skip visibility toggle */
					preset_changed_clear(client, list_index);
				}

				break;
			case BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE:
				if (change_id_pending == BT_HAS_CHANGE_ID_PRESET_AVAILABLE) {
					/* skip visibility toggle */
					preset_changed_clear(client, list_index);
				}

				break;
			}
		} else {
			preset_changed_set(client, list_index, change_id);
		}

		if (is_preset_changed_pending(client)) {
			control_point_work_submit(client, BT_HAS_CP_WORK_TIMEOUT);
		} else if (!is_preset_changed_pending(client) ||
			   !is_read_preset_rsp_pending(client)) {
			k_work_cancel_delayable_sync(&client->control_point_work, &client->control_point_sync);
			atomic_clear_bit(client->flags, CLIENT_FLAG_CP_BUSY);
		}
	}
}

static int preset_name_set(struct has_preset *preset, const char *name, uint8_t len)
{
#if defined(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)
	utf8_lcpy(preset->name, name, ARRAY_SIZE(preset->name));
	preset->name[len] = '\0';

	/* Do not send preset changed notification if the preset is hidden */
	if (!preset->hidden) {
		preset_changed(preset, BT_HAS_CHANGE_ID_GENERIC_UPDATE);
	}

	if (preset_ops->name_changed) {
		preset_ops->name_changed(&has_local, preset->index, preset->name);
	}

	return 0;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_HAS_PRESET_NAME_DYNAMIC */
}

#if defined(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)
static int handle_write_preset_name(struct net_buf_simple *buf)
{
	const struct bt_has_cp_write_preset_name_req *req;
	struct has_preset *preset;

	if (buf->len < (sizeof(*req) + BT_HAS_PRESET_NAME_MIN) ||
	    buf->len > (sizeof(*req) + BT_HAS_PRESET_NAME_MAX)) {
		return BT_GATT_ERR(BT_HAS_ERR_INVALID_PARAM_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	preset = preset_get(req->index);
	if (!preset) {
		return BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE);
	}

	if (!(preset->properties & BT_HAS_PROP_WRITABLE)) {
		return BT_GATT_ERR(BT_HAS_ERR_WRITE_NAME_NOT_ALLOWED);
	}

	preset_name_set(preset, req->name, buf->len);

	return 0;
}
#endif /* CONFIG_BT_HAS_PRESET_NAME_DYNAMIC */

static int call_ops_set_active_preset(uint8_t index, bool sync)
{
	int err;

	if (preset_ops == NULL) {
		return BT_GATT_ERR(BT_HAS_ERR_OPERATION_NOT_POSSIBLE);
	}

	err = preset_ops->active_set(&has_local, index, sync);

	if (err == -EBUSY) {
		return BT_GATT_ERR(BT_HAS_ERR_OPERATION_NOT_POSSIBLE);
	}

	if (err == 0) {
		bt_has_preset_active_set(&has_local, index);
	}

	return 0;
}

static int handle_set_active_preset(struct net_buf_simple *buf, bool sync)
{
	const struct bt_has_cp_set_active_preset_req *req;
	const struct has_preset *preset;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_HAS_ERR_INVALID_PARAM_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	preset = preset_get(req->index);
	if (!preset) {
		return BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE);
	}

	if (!(preset->properties & BT_HAS_PROP_AVAILABLE)) {
		return BT_GATT_ERR(BT_HAS_ERR_OPERATION_NOT_POSSIBLE);
	}

	return call_ops_set_active_preset(preset->index, sync);
}

static int handle_set_next_preset(struct net_buf_simple *buf, bool sync)
{
	const struct has_preset *preset = NULL;

	preset_foreach(has_local.active_index + 1, last_preset_index, 1, find_available, &preset);
	if (preset) {
		return call_ops_set_active_preset(preset->index, sync);
	}

	preset_foreach(1, has_local.active_index - 1, 1, find_available, &preset);
	if (preset) {
		return call_ops_set_active_preset(preset->index, sync);
	}

	return BT_GATT_ERR(BT_HAS_ERR_OPERATION_NOT_POSSIBLE);
}

static int handle_set_prev_preset(struct net_buf_simple *buf, bool sync)
{
	const struct has_preset *preset = NULL;

	preset_foreach(1, has_local.active_index - 1, 0, find_available, &preset);
	if (preset) {
		return call_ops_set_active_preset(preset->index, sync);
	}

	preset_foreach(has_local.active_index + 1, last_preset_index, 0, find_available, &preset);
	if (preset) {
		return call_ops_set_active_preset(preset->index, sync);
	}

	return BT_GATT_ERR(BT_HAS_ERR_OPERATION_NOT_POSSIBLE);
}

static ssize_t control_point_rx(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *data, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct has_client *client;
	const struct bt_has_cp_hdr *hdr;
	struct net_buf_simple buf;
	ssize_t ret;

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len < sizeof(*hdr)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	client = client_get(conn);

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	hdr = net_buf_simple_pull_mem(&buf, sizeof(*hdr));

	BT_DBG("conn %p op %s (0x%02x)", conn, bt_has_op_str(hdr->op), hdr->op);

	switch (hdr->op) {
	case BT_HAS_OP_READ_PRESET_REQ:
		if (!atomic_test_bit(client->flags, CLIENT_FLAG_CP_IND_ENABLED)) {
			return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
		}

		/* Fail if ATT MTU is potentially insufficient to complete the operation */
		if (!atomic_test_bit(client->flags, CLIENT_FLAG_ATT_MTU_VALID)) {
			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		ret = handle_read_preset_req(client, &buf);
		break;
#if defined(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)
	case BT_HAS_OP_WRITE_PRESET_NAME:
		if (!atomic_test_bit(client->flags, CLIENT_FLAG_CP_IND_ENABLED)) {
			return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
		}

		/* Fail if ATT MTU is potentially insufficient to complete the operation */
		if (!atomic_test_bit(client->flags, CLIENT_FLAG_ATT_MTU_VALID)) {
			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		ret = handle_write_preset_name(&buf);
		break;
#endif /* CONFIG_BT_HAS_PRESET_NAME_DYNAMIC */
	case BT_HAS_OP_SET_ACTIVE_PRESET:
		if (!atomic_test_bit(client->flags, CLIENT_FLAG_CP_IND_ENABLED)) {
			return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
		}

		ret = handle_set_active_preset(&buf, false);
		break;
	case BT_HAS_OP_SET_NEXT_PRESET:
		ret = handle_set_next_preset(&buf, false);
		break;
	case BT_HAS_OP_SET_PREV_PRESET:
		ret = handle_set_prev_preset(&buf, false);
		break;
#if defined(CONFIG_BT_HAS_PRESET_SYNC_SUPPORT)
	case BT_HAS_OP_SET_ACTIVE_PRESET_SYNC:
		if (!atomic_test_bit(client->flags, CLIENT_FLAG_CP_IND_ENABLED)) {
			return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
		}

		ret = handle_set_active_preset(&buf, true);
		break;
	case BT_HAS_OP_SET_NEXT_PRESET_SYNC:
		ret = handle_set_next_preset(&buf, true);
		break;
	case BT_HAS_OP_SET_PREV_PRESET_SYNC:
		ret = handle_set_prev_preset(&buf, true);
		break;
#else
	case BT_HAS_OP_SET_ACTIVE_PRESET_SYNC:
	case BT_HAS_OP_SET_NEXT_PRESET_SYNC:
	case BT_HAS_OP_SET_PREV_PRESET_SYNC:
		ret = BT_GATT_ERR(BT_HAS_ERR_PRESET_SYNC_NOT_SUPP);
		break;
#endif /* CONFIG_BT_HAS_PRESET_SYNC_SUPPORT */
	default:
		ret = BT_GATT_ERR(BT_HAS_ERR_INVALID_OP);
	}

	return (ret < 0) ? ret : len;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct has_client *client;

	BT_DBG("conn %p err %d", conn, err);

	if (err != 0 || !bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
		return;
	}

	client = client_get(conn);
	if (unlikely(!client)) {
		BT_ERR("Failed to allocate client");
		return;
	}

	/* Mark all non-hidden presets to be sent via Preset Changed for bonded device.
	 *
	 * XXX: At this point stored GATT CCC configurations are not loaded yet,
	 *	thus we postpone bt_gatt_is_subscribed check on Control Point CCC to be
	 *	done in security_changed().
	 */
	for (size_t index = 0; index < ARRAY_SIZE(has_preset_list); index++) {
		if (has_preset_list[index].hidden) {
			continue;
		}

		preset_changed_set(client, index, BT_HAS_CHANGE_ID_GENERIC_UPDATE);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct has_client *client;

	BT_DBG("conn %p reason %d", conn, reason);

	client = client_lookup_conn(conn);
	if (client) {
		client_free(client);
	}
}

static void send_preset_changed_pending(struct has_client *client)
{
	if (bt_gatt_is_subscribed(client->conn, &has_svc.attrs[4], BT_GATT_CCC_INDICATE)) {
		atomic_set_bit(client->flags, CLIENT_FLAG_CP_IND_ENABLED);
	}

	if (bt_gatt_is_subscribed(client->conn, &has_svc.attrs[4], BT_GATT_CCC_NOTIFY)) {
		atomic_set_bit(client->flags, CLIENT_FLAG_CP_NTF_ENABLED);
	}

	/* If peer is not subscribed for Control Point PDUs, unmark the pending
	 * Preset Changed PDUs marked to be sent in connected().
	 */
	if (!atomic_test_bit(client->flags, CLIENT_FLAG_CP_IND_ENABLED) &&
	    !atomic_test_bit(client->flags, CLIENT_FLAG_CP_NTF_ENABLED)) {
		preset_changed_clear_all(client);
	} else {
		control_point_work_submit(client, BT_HAS_CP_WORK_TIMEOUT);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	struct has_client *client;

	BT_DBG("conn %p level %d err %d", conn, level, err);

	if (err != BT_SECURITY_ERR_SUCCESS) {
		return;
	}

	client = client_get(conn);
	if (unlikely(!client)) {
		BT_ERR("Failed to allocate client");
		return;
	}

	if (!atomic_test_bit(client->flags, CLIENT_FLAG_ATT_MTU_VALID) &&
	    bt_gatt_get_mtu(client->conn) >= BT_HAS_ATT_MTU_MIN) {
		atomic_set_bit(client->flags, CLIENT_FLAG_ATT_MTU_VALID);
	}

	if (level < BT_SECURITY_L4) {
		return;
	}

	if (atomic_test_and_set_bit(client->flags, CLIENT_FLAG_SEC_ENABLED)) {
		return;
	}

	/* Send pending Preset Changed if any */
	if (is_preset_changed_pending(client) &&
	    atomic_test_bit(client->flags, CLIENT_FLAG_ATT_MTU_VALID)) {
		send_preset_changed_pending(client);
	}
}

BT_CONN_CB_DEFINE(has_conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	struct has_client *client;

	BT_DBG("conn %p tx %u rx %u", conn, tx, rx);

	client = client_lookup_conn(conn);
	if (!client) {
		return;
	}

	/* HearingAidProfile_validation_r01
	 *
	 * If the HARC supports the Read All Presets procedure or the Read Preset by Index
	 * procedure, then the HARC shall support an ATT_MTU value no less than 49.
	 */
	if (tx < BT_HAS_ATT_MTU_MIN) {
		return;
	}

	if (atomic_test_and_set_bit(client->flags, CLIENT_FLAG_ATT_MTU_VALID)) {
		return;
	}

	/* Send pending Preset Changed if any */
	if (is_preset_changed_pending(client) &&
	    atomic_test_bit(client->flags, CLIENT_FLAG_SEC_ENABLED)) {
		send_preset_changed_pending(client);
	}
}

static struct bt_gatt_cb gatt_cb = {
	.att_mtu_updated = att_mtu_updated,
};

static int has_init(const struct device *dev)
{
	uint8_t type;

	ARG_UNUSED(dev);

	if (IS_ENABLED(CONFIG_BT_HAS_HEARING_AID_MONAURAL)) {
		type = BT_HAS_HEARING_AID_TYPE_MONAURAL;
	} else if (IS_ENABLED(CONFIG_BT_HAS_HEARING_AID_BANDED)) {
		type = BT_HAS_HEARING_AID_TYPE_BANDED;
	} else {
		type = BT_HAS_HEARING_AID_TYPE_BINAURAL;
	}

	/* Initialize the supported features characteristic value */
	has_local.features = BT_HAS_FEAT_HEARING_AID_TYPE_MASK & type;
	if (IS_ENABLED(CONFIG_BT_HAS_PRESET_SYNC_SUPPORT)) {
		has_local.features |= BT_HAS_FEAT_BIT_PRESET_SYNC;
	}
	if (!IS_ENABLED(CONFIG_BT_HAS_IDENTICAL_PRESET_RECORDS)) {
		has_local.features |= BT_HAS_FEAT_BIT_INDEPENDENT_PRESETS;
	}

	bt_gatt_cb_register(&gatt_cb);

	k_work_init(&active_preset_work, active_preset_work_process);

	return 0;
}

SYS_INIT(has_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#define BT_HAS_IS_LOCAL(_has) ((_has) == &has_local)
#else
#define BT_HAS_IS_LOCAL(_has) false
#endif /* CONFIG_BT_HAS */

#if defined(CONFIG_BT_HAS)
int bt_has_register(const struct bt_has_register_param *param, struct bt_has **out)
{
	bool writable_presets_support = false;

	*out = &has_local;

	CHECKIF(param == NULL || param->ops == NULL || param->ops->active_set == NULL) {
		return -EINVAL;
	}

	/* Check if already registered */
	if (preset_ops) {
		return -EALREADY;
	}

	for (size_t i = 0; i < ARRAY_SIZE(has_preset_list); i++) {
		const struct bt_has_preset_register_param *preset_param = NULL;
		struct has_preset *preset = &has_preset_list[i];

		/* Sort the presets in order of increasing index */
		for (size_t j = 0; j < ARRAY_SIZE(param->preset_param); j++) {
			const struct bt_has_preset_register_param *tmp = &param->preset_param[j];

			if ((!preset_param || tmp->index < preset_param->index) &&
			    tmp->index > last_preset_index) {
				preset_param = tmp;
			}
		}

		if (!preset_param) {
			break;
		}

		preset->index = preset_param->index;
		preset->properties = preset_param->properties;
#if defined(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)
		utf8_lcpy(preset->name, preset_param->name, ARRAY_SIZE(preset->name));
		preset->name[strlen(preset_param->name)] = '\0';
#else
		preset->name = preset_param->name;
#endif /* CONFIG_BT_HAS_PRESET_NAME_DYNAMIC */

		if (IS_ENABLED(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)) {
			/* If the server exposes at least one preset record with the Writable flag
			 * set, then the server shall set the Writable Presets Support flag.
			 */
			writable_presets_support |= preset_param->properties & BT_HAS_PROP_WRITABLE;
		}

		last_preset_index = preset->index;
	}

	if (writable_presets_support) {
		has_local.features |= BT_HAS_FEAT_BIT_WRITABLE_PRESETS;
	}

	preset_ops = param->ops;

	return 0;
}

int bt_has_preset_active_clear(struct bt_has *has)
{
	return bt_has_preset_active_set(has, BT_HAS_PRESET_INDEX_NONE);
}

int bt_has_preset_visibility_set(struct bt_has *has, uint8_t index, bool visible)
{
	struct has_preset *preset;

	CHECKIF(has == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_HAS_CLIENT) && !BT_HAS_IS_LOCAL(has)) {
		return -EOPNOTSUPP;
	}

	preset = preset_get(index);
	if (!preset) {
		return -ENOENT;
	}

	if (preset->hidden == visible) {
		preset->hidden = !visible;
		preset_changed(preset, visible ? BT_HAS_CHANGE_ID_GENERIC_UPDATE :
							BT_HAS_CHANGE_ID_PRESET_DELETED);
	}

	return 0;
}

int bt_has_preset_availability_set(struct bt_has *has, uint8_t index, bool available)
{
	struct has_preset *preset;

	CHECKIF(has == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_HAS_CLIENT) && !BT_HAS_IS_LOCAL(has)) {
		return -EOPNOTSUPP;
	}

	preset = preset_get(index);
	if (!preset) {
		return -ENOENT;
	}

	if (((preset->properties & BT_HAS_PROP_AVAILABLE) > 0) != available) {
		preset->properties ^= BT_HAS_PROP_AVAILABLE;

		/* Do not send preset changed notification if the preset is hidden */
		if (!preset->hidden) {
			preset_changed(preset,
					available ? BT_HAS_CHANGE_ID_PRESET_AVAILABLE :
							BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE);
		}
	}

	return 0;
}
#endif /* CONFIG_BT_HAS */

#if defined(CONFIG_BT_HAS_CLIENT)
int bt_has_preset_active_get(struct bt_has *has)
{
	CHECKIF(has == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_HAS) && BT_HAS_IS_LOCAL(has)) {
		return -EOPNOTSUPP;
	}

	return bt_has_client_preset_active_get(has);
}

int bt_has_preset_active_set_next(struct bt_has *has)
{
	CHECKIF(has == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_HAS) && BT_HAS_IS_LOCAL(has)) {
		return -EOPNOTSUPP;
	}

	return bt_has_client_preset_active_set_next(has);
}

int bt_has_preset_active_set_prev(struct bt_has *has)
{
	CHECKIF(has == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_HAS) && BT_HAS_IS_LOCAL(has)) {
		return -EOPNOTSUPP;
	}

	return bt_has_client_preset_active_set_prev(has);
}

int bt_has_preset_read(struct bt_has *has, uint8_t index)
{
	CHECKIF(has == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_HAS) && BT_HAS_IS_LOCAL(has)) {
		return -EOPNOTSUPP;
	}

	return bt_has_client_preset_read(has, index);
}

int bt_has_preset_read_multiple(struct bt_has *has, uint8_t start_index, uint8_t count)
{
	CHECKIF(has == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_HAS) && BT_HAS_IS_LOCAL(has)) {
		return -EOPNOTSUPP;
	}
	
	return bt_has_client_preset_read_multiple(has, start_index, count);
}
#endif /* CONFIG_BT_HAS_CLIENT */

int bt_has_preset_active_set(struct bt_has *has, uint8_t index)
{
	CHECKIF(has == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_HAS) && BT_HAS_IS_LOCAL(has)) {
		if (index == has_local.active_index) {
			/* no change */
			return 0;
		}

		if (index != BT_HAS_PRESET_INDEX_NONE) {
			struct has_preset *preset = preset_get(index);

			if (!preset) {
				return -ENOENT;
			}
		}

		has_local.active_index = index;

		k_work_submit(&active_preset_work);

		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_HAS_CLIENT)) {
		return bt_has_client_preset_active_set(has, index);
	}

	return -EOPNOTSUPP;
}

int bt_has_preset_name_set(struct bt_has *has, uint8_t index, const char *name)
{
	CHECKIF(has == NULL || name == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_HAS) && BT_HAS_IS_LOCAL(has)) {
		struct has_preset *preset = preset_get(index);

		if (!preset) {
			return -ENOENT;
		}

		return preset_name_set(preset, name, strlen(name));
	}

	if (IS_ENABLED(CONFIG_BT_HAS_CLIENT)) {
		return bt_has_client_preset_name_set(has, index, name);
	}

	return -EOPNOTSUPP;
}
