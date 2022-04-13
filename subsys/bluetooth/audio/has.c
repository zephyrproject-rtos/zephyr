/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr.h>

#include <device.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/capabilities.h>
#include <bluetooth/audio/has.h>
#include <sys/check.h>

#include "has_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HAS)
#define LOG_MODULE_NAME bt_has
#include "common/log.h"

/* The service allows operations with paired devices only.
 * For now, the context is kept for connected devices only, thus the number of contexts is
 * equal to maximum number of simultaneous connections to paired devices.
 */
#define BT_HAS_MAX_CONN MIN(CONFIG_BT_MAX_CONN, CONFIG_BT_MAX_PAIRED)

static struct bt_has has;

#if CONFIG_BT_HAS_PRESET_COUNT > 0
static ssize_t write_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *data, uint16_t len, uint16_t offset, uint8_t flags);

static ssize_t read_active_preset_index(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					void *buf, uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p offset %d", (void *)conn, attr, offset);

	if (offset > sizeof(has.active_index)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &has.active_index,
				 sizeof(has.active_index));
}
#endif /* CONFIG_BT_HAS_PRESET_COUNT > 0 */

static ssize_t read_features(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p offset %d", (void *)conn, attr, offset);

	if (offset > sizeof(has.features)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &has.features,
				 sizeof(has.features));
}

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

/* Hearing Access Service GATT Attributes */
BT_GATT_SERVICE_DEFINE(has_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_HEARING_AID_FEATURES, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_ENCRYPT, read_features, NULL, NULL),
#if CONFIG_BT_HAS_PRESET_COUNT > 0
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_PRESET_CONTROL_POINT,
#if defined(CONFIG_BT_EATT)
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_NOTIFY,
#else
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
#endif /* CONFIG_BT_EATT */
			       BT_GATT_PERM_WRITE_ENCRYPT, NULL, write_control_point, NULL),
	BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_ACTIVE_PRESET_INDEX,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ_ENCRYPT,
			       read_active_preset_index, NULL, NULL),
	BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#endif /* CONFIG_BT_HAS_PRESET_COUNT > 0 */
);

#if CONFIG_BT_HAS_PRESET_COUNT > 0
#define PRESET_CONTROL_POINT_ATTR &has_svc.attrs[4]

static struct has_client {
	struct bt_conn *conn;
	union {
		struct bt_gatt_indicate_params ind;
#if defined(CONFIG_BT_EATT)
		struct bt_gatt_notify_params ntf;
#endif /* CONFIG_BT_EATT */
	} params;

	struct bt_has_cp_read_presets_req read_presets_req;
	struct k_work control_point_work;
	struct k_work_sync control_point_work_sync;
} has_client_list[BT_HAS_MAX_CONN];

/* HAS internal preset representation */
static struct has_preset {
	uint8_t index;
	enum bt_has_properties properties;
#if defined(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)
	char name[BT_HAS_PRESET_NAME_MAX + 1]; /* +1 byte for NULL-terminator */
#else
	const char *name;
#endif /* CONFIG_BT_HAS_PRESET_NAME_DYNAMIC */
} has_preset_list[CONFIG_BT_HAS_PRESET_COUNT];

/* Number of registered presets */
static uint8_t has_preset_num;

static void process_control_point_work(struct k_work *work);

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

	k_work_init(&client->control_point_work, process_control_point_work);

	return client;
}

static bool read_presets_req_is_pending(struct has_client *client)
{
	return client->read_presets_req.num_presets > 0;
}

static void read_presets_req_free(struct has_client *client)
{
	client->read_presets_req.num_presets = 0;
}

static void client_free(struct has_client *client)
{
	(void)k_work_cancel(&client->control_point_work);

	read_presets_req_free(client);

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

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct has_client *client;

	BT_DBG("conn %p reason %d", (void *)conn, reason);

	client = client_get(conn);
	if (client) {
		client_free(client);
	}
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.disconnected = disconnected,
};

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
					  const char *name)
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

	BT_DBG("conn %p\n", (void *)conn);

	/* Resubmit if needed */
	if (client != NULL && read_presets_req_is_pending(client)) {
		k_work_submit(&client->control_point_work);
	}
}

static void control_point_ind_complete(struct bt_conn *conn,
				       struct bt_gatt_indicate_params *params,
				       uint8_t err)
{
	if (err) {
		/* TODO: Handle error somehow */
		BT_ERR("conn %p err 0x%02x\n", (void *)conn, err);
	}

	control_point_ntf_complete(conn, NULL);
}

static int control_point_send(struct has_client *client, struct net_buf_simple *buf)
{
#if defined(CONFIG_BT_EATT)
	if (bt_eatt_count(client->conn) > 0 &&
	    bt_gatt_is_subscribed(client->conn, PRESET_CONTROL_POINT_ATTR, BT_GATT_CCC_NOTIFY)) {
		client->params.ntf.attr = PRESET_CONTROL_POINT_ATTR;
		client->params.ntf.func = control_point_ntf_complete;
		client->params.ntf.data = buf->data;
		client->params.ntf.len = buf->len;

		return bt_gatt_notify_cb(client->conn, &client->params.ntf);
	}
#endif /* CONFIG_BT_EATT */

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

static int bt_has_cp_read_preset_rsp(struct has_client *client, const struct has_preset *preset,
				     bool is_last)
{
	struct bt_has_cp_hdr *hdr;
	struct bt_has_cp_read_preset_rsp *rsp;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*hdr) + sizeof(*rsp) + BT_HAS_PRESET_NAME_MAX);

	BT_DBG("conn %p preset %p is_last 0x%02x", (void *)client->conn, preset, is_last);

	hdr = net_buf_simple_add(&buf, sizeof(*hdr));
	hdr->opcode = BT_HAS_OP_READ_PRESET_RSP;
	rsp = net_buf_simple_add(&buf, sizeof(*rsp));
	rsp->is_last = is_last ? 0x01 : 0x00;
	rsp->index = preset->index;
	rsp->properties = preset->properties;
	net_buf_simple_add_mem(&buf, preset->name, strlen(preset->name));

	return control_point_send(client, &buf);
}

static void process_control_point_work(struct k_work *work)
{
	struct has_client *client = CONTAINER_OF(work, struct has_client, control_point_work);
	int err;

	if (!client->conn) {
		return;
	}

	if (read_presets_req_is_pending(client)) {
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
			BT_ERR("bt_has_cp_read_preset_rsp failed (err %d)", err);
		}

		if (err || is_last) {
			read_presets_req_free(client);
		} else {
			client->read_presets_req.start_index = preset->index + 1;
			client->read_presets_req.num_presets--;
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

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("start_index %d num_presets %d", req->start_index, req->num_presets);

	/* Abort if there is no preset in requested index range */
	preset_foreach(req->start_index, BT_HAS_PRESET_INDEX_LAST, preset_found, &preset);

	if (preset == NULL) {
		return BT_ATT_ERR_OUT_OF_RANGE;
	}

	client = client_get_or_new(conn);

	/* Reject if already in progress */
	if (read_presets_req_is_pending(client)) {
		return BT_HAS_ERR_OPERATION_NOT_POSSIBLE;
	}

	/* Store the request */
	client->read_presets_req.start_index = req->start_index;
	client->read_presets_req.num_presets = req->num_presets;

	k_work_submit(&client->control_point_work);

	return 0;
}

static uint8_t handle_control_point_op(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_has_cp_hdr *hdr;

	hdr = net_buf_simple_pull_mem(buf, sizeof(*hdr));

	BT_DBG("conn %p opcode %s (0x%02x)", (void *)conn, bt_has_op_str(hdr->opcode),
	       hdr->opcode);

	switch (hdr->opcode) {
	case BT_HAS_OP_READ_PRESET_REQ:
		return handle_read_preset_req(conn, buf);
	}

	return BT_HAS_ERR_INVALID_OPCODE;
}

static ssize_t write_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *data, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct net_buf_simple buf;
	uint8_t err;

	BT_DBG("conn %p attr %p data %p len %d offset %d flags 0x%02x", (void *)conn, attr, data,
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
		BT_WARN("err 0x%02x", err);
		return BT_GATT_ERR(err);
	}

	return len;
}

int bt_has_preset_register(const struct bt_has_preset_register_param *param)
{
	struct has_preset *preset = NULL;
	size_t name_len;

	CHECKIF(param == NULL) {
		BT_ERR("param is NULL");
		return -EINVAL;
	}

	CHECKIF(param->index == BT_HAS_PRESET_INDEX_NONE) {
		BT_ERR("param->index is invalid");
		return -EINVAL;
	}

	CHECKIF(param->name == NULL) {
		BT_ERR("param->name is NULL");
		return -EINVAL;
	}

	name_len = strlen(param->name);

	CHECKIF(name_len < BT_HAS_PRESET_NAME_MIN) {
		BT_ERR("param->name is too short (%zu < %u)", name_len, BT_HAS_PRESET_NAME_MIN);
		return -EINVAL;
	}

	CHECKIF(name_len > BT_HAS_PRESET_NAME_MAX) {
		BT_WARN("param->name is too long (%zu > %u)", name_len, BT_HAS_PRESET_NAME_MAX);
	}

	preset_foreach(param->index, param->index, preset_found, &preset);
	if (preset != NULL) {
		return -EALREADY;
	}

	preset = preset_alloc(param->index, param->properties, param->name);
	if (preset == NULL) {
		return -ENOMEM;
	}

	return 0;
}

int bt_has_preset_unregister(uint8_t index)
{
	struct has_preset *preset = NULL;

	CHECKIF(index == BT_HAS_PRESET_INDEX_NONE) {
		BT_ERR("index is invalid");
		return -EINVAL;
	}

	preset_foreach(index, index, preset_found, &preset);
	if (preset == NULL) {
		return -ENOENT;
	}

	preset_free(preset);

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
#endif /* CONFIG_BT_HAS_PRESET_COUNT > 0 */

static int has_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Initialize the supported features characteristic value */
	has.features = CONFIG_BT_HAS_HEARING_AID_TYPE & BT_HAS_FEAT_HEARING_AID_TYPE_MASK;

	if (IS_ENABLED(CONFIG_BT_HAS_HEARING_AID_BINAURAL)) {
		if (IS_ENABLED(CONFIG_BT_HAS_PRESET_SYNC_SUPPORT)) {
			has.features |= BT_HAS_FEAT_PRESET_SYNC_SUPP;
		}

		if (!IS_ENABLED(CONFIG_BT_HAS_IDENTICAL_PRESET_RECORDS)) {
			has.features |= BT_HAS_FEAT_INDEPENDENT_PRESETS;
		}
	}

	if (IS_ENABLED(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)) {
		has.features |= BT_HAS_FEAT_WRITABLE_PRESETS_SUPP;
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SNK_LOC)) {
		if (IS_ENABLED(CONFIG_BT_HAS_HEARING_AID_BANDED)) {
			/* HAP_d1.0r00; 3.7 BAP Unicast Server role requirements
			 * A Banded Hearing Aid in the HA role shall set the
			 * Front Left and the Front Right bits to a value of 0b1
			 * in the Sink Audio Locations characteristic value.
			 */
			bt_audio_capability_set_location(BT_AUDIO_SINK,
							 (BT_AUDIO_LOCATION_FRONT_LEFT |
								BT_AUDIO_LOCATION_FRONT_RIGHT));
		} else if (IS_ENABLED(CONFIG_BT_HAS_HEARING_AID_LEFT)) {
			bt_audio_capability_set_location(BT_AUDIO_SINK,
							 BT_AUDIO_LOCATION_FRONT_LEFT);
		} else {
			bt_audio_capability_set_location(BT_AUDIO_SINK,
							 BT_AUDIO_LOCATION_FRONT_RIGHT);
		}
	}

	return 0;
}

SYS_INIT(has_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
