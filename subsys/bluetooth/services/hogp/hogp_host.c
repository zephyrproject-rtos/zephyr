/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hogp_host.c
 * @brief HID over GATT Host (HOGP Host / HIDS Client)
 *
 * GATT client state machine for discovering and interacting with
 * remote HID Service (UUID 0x1812).
 */

#include <string.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/services/hogp_host.h>
#include <zephyr/bluetooth/services/hogp_device.h>

LOG_MODULE_REGISTER(bt_hogp_host, CONFIG_BT_HOGP_HOST_LOG_LEVEL);

#define PNP_ID_VID_SRC_OFFSET  0
#define PNP_ID_VID_OFFSET      1
#define PNP_ID_PID_OFFSET      3
#define PNP_ID_VERSION_OFFSET  5

/* ---- Internal types (see hogp_internal.h) ---- */

#include "hogp_internal.h"

/* ---- Globals ---- */

static struct hogp_host_conn g_conns[CONFIG_BT_HOGP_HOST_MAX_CONNECTIONS];

static const char *state_name(enum hogp_host_state s)
{
	static const char *const names[] = {
		[HOGP_HOST_STATE_IDLE] = "IDLE",
		[HOGP_HOST_STATE_DISCOVER_SERVICE] = "DISCOVER_SERVICE",
		[HOGP_HOST_STATE_DISCOVER_CHARS] = "DISCOVER_CHARS",
		[HOGP_HOST_STATE_SET_PROTOCOL_MODE] = "SET_PROTOCOL_MODE",
		[HOGP_HOST_STATE_READ_HID_INFO] = "READ_HID_INFO",
		[HOGP_HOST_STATE_READ_REPORT_MAP] = "READ_REPORT_MAP",
		[HOGP_HOST_STATE_DISCOVER_REPORT_MAP_DESCS] = "DISCOVER_REPORT_MAP_DESCS",
		[HOGP_HOST_STATE_READ_EXT_REF_UUID] = "READ_EXT_REF_UUID",
		[HOGP_HOST_STATE_DISCOVER_EXT_CHARS] = "DISCOVER_EXT_CHARS",
		[HOGP_HOST_STATE_FIND_REPORT_DESCS] = "FIND_REPORT_DESCS",
		[HOGP_HOST_STATE_READ_REPORT_ID_TYPE] = "READ_REPORT_ID_TYPE",
		[HOGP_HOST_STATE_ENABLE_NOTIFICATIONS] = "ENABLE_NOTIFICATIONS",
		[HOGP_HOST_STATE_DISCOVER_DIS] = "DISCOVER_DIS",
		[HOGP_HOST_STATE_DISCOVER_DIS_CHARS] = "DISCOVER_DIS_CHARS",
		[HOGP_HOST_STATE_READ_PNP_ID] = "READ_PNP_ID",
		[HOGP_HOST_STATE_DISCOVER_BAS] = "DISCOVER_BAS",
		[HOGP_HOST_STATE_DISCOVER_BAS_CHARS] = "DISCOVER_BAS_CHARS",
		[HOGP_HOST_STATE_DISCOVER_BAS_DESCS] = "DISCOVER_BAS_DESCS",
		[HOGP_HOST_STATE_READ_BAT_LEVEL] = "READ_BAT_LEVEL",
		[HOGP_HOST_STATE_ENABLE_BAT_NOTIFY] = "ENABLE_BAT_NOTIFY",
		[HOGP_HOST_STATE_CONNECTED] = "CONNECTED",
		[HOGP_HOST_STATE_GET_REPORT] = "GET_REPORT",
		[HOGP_HOST_STATE_SET_REPORT] = "SET_REPORT",
	};

	if (s < ARRAY_SIZE(names) && names[s]) {
		return names[s];
	}

	return "UNKNOWN";
}

/* ---- Forward declarations ---- */

static void next_step(struct hogp_host_conn *c);
static void finish_connected(struct hogp_host_conn *c, int status);
static void advance_to_finish(struct hogp_host_conn *c);
static void advance_to_dis(struct hogp_host_conn *c);

/* ---- Connection lookup ---- */

static struct hogp_host_conn *find_conn(struct bt_conn *conn)
{
	for (int i = 0; i < CONFIG_BT_HOGP_HOST_MAX_CONNECTIONS; i++) {
		if (g_conns[i].conn == conn) {
			return &g_conns[i];
		}
	}

	return NULL;
}

static struct hogp_host_conn *alloc_conn(struct bt_conn *conn)
{
	for (int i = 0; i < CONFIG_BT_HOGP_HOST_MAX_CONNECTIONS; i++) {
		if (g_conns[i].conn == NULL) {
			memset(&g_conns[i], 0, sizeof(g_conns[i]));
			g_conns[i].conn = bt_conn_ref(conn);
			return &g_conns[i];
		}
	}

	return NULL;
}

static void free_conn(struct hogp_host_conn *c)
{
	if (c->conn) {
		/* Unsubscribe all notifications */
		for (uint8_t i = 0; i < c->num_reports; i++) {
			if (c->reports[i].sub_params.value_handle) {
				bt_gatt_unsubscribe(c->conn, &c->reports[i].sub_params);
			}
		}

		for (uint8_t i = 0; i < c->num_bats; i++) {
			if (c->bats[i].sub_params.value_handle) {
				bt_gatt_unsubscribe(c->conn, &c->bats[i].sub_params);
			}
		}

		bt_conn_unref(c->conn);
	}
	c->conn = NULL;
	c->state = HOGP_HOST_STATE_IDLE;
	c->desc_used = 0;
}

/* ---- Descriptor storage ---- */

static int desc_store_byte(struct hogp_host_conn *c, uint8_t byte)
{
	if (c->desc_used >= CONFIG_BT_HOGP_HOST_DESC_STORAGE_LEN) {
		return -ENOMEM;
	}

	c->desc_storage[c->desc_used++] = byte;
	c->services[c->svc_idx].desc_len++;
	return 0;
}

/* ---- Report helpers ---- */

static uint8_t add_report(struct hogp_host_conn *c, uint16_t value_handle,
			  uint16_t end_handle, uint8_t properties,
			  uint8_t report_id, uint8_t report_type, bool boot)
{
	uint8_t idx;
	struct hogp_host_report *r;

	if (c->num_reports >= CONFIG_BT_HOGP_HOST_MAX_REPORTS) {
		LOG_WRN("Too many reports, increase CONFIG_BT_HOGP_HOST_MAX_REPORTS");
		return UINT8_MAX;
	}

	idx = c->num_reports++;
	r = &c->reports[idx];

	r->value_handle = value_handle;
	r->end_handle = end_handle;
	r->properties = properties;
	r->service_index = c->svc_idx;
	r->report_id = report_id;
	r->report_type = report_type;
	r->boot_report = boot;
	r->ccc_handle = 0;
	return idx;
}

static struct hogp_host_report *find_report(struct hogp_host_conn *c,
					    uint8_t report_id,
					    uint8_t report_type)
{
	for (uint8_t i = 0; i < c->num_reports; i++) {
		struct hogp_host_report *r = &c->reports[i];
		uint8_t pm = c->services[r->service_index].protocol_mode;

		if (pm == BT_HID_PROTOCOL_BOOT && !r->boot_report) {
			continue;
		}

		if (pm == BT_HID_PROTOCOL_REPORT && r->boot_report) {
			continue;
		}

		if (r->report_id == report_id && r->report_type == report_type) {
			return r;
		}
	}

	return NULL;
}

static struct hogp_host_report *find_report_by_handle(struct hogp_host_conn *c,
						      uint16_t handle)
{
	for (uint8_t i = 0; i < c->num_reports; i++) {
		if (c->reports[i].value_handle == handle) {
			return &c->reports[i];
		}
	}

	return NULL;
}

/* ---- Notification handler ---- */

static uint8_t notify_cb(struct bt_conn *conn,
			 struct bt_gatt_subscribe_params *params,
			 const void *data, uint16_t length)
{
	struct hogp_host_conn *c;
	struct hogp_host_report *r;

	if (!data) {
		LOG_DBG("%s: unsubscribed handle=0x%04x", __func__, params->value_handle);
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	c = find_conn(conn);

	if (!c || !c->cb || !c->cb->input_report) {
		return BT_GATT_ITER_CONTINUE;
	}

	r = find_report_by_handle(c, params->value_handle);
	if (!r) {
		LOG_WRN("%s: no report for handle 0x%04x", __func__, params->value_handle);
		return BT_GATT_ITER_CONTINUE;
	}

	c->cb->input_report(conn, r->service_index, r->report_id, data, length);
	return BT_GATT_ITER_CONTINUE;
}

/* ---- Battery notification handler ---- */

static uint8_t bat_notify_cb(struct bt_conn *conn,
			     struct bt_gatt_subscribe_params *params,
			     const void *data, uint16_t length)
{
	struct hogp_host_conn *c;

	if (!data) {
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	c = find_conn(conn);

	if (!c || !c->cb || !c->cb->battery_level || length < 1) {
		return BT_GATT_ITER_CONTINUE;
	}

	for (uint8_t i = 0; i < c->num_bats; i++) {
		if (c->bats[i].sub_params.value_handle == params->value_handle) {
			c->cb->battery_level(conn, i, ((const uint8_t *)data)[0]);
			break;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

/* ---- State machine: iteration helpers ---- */

/* Find next Input Report for notification subscription */
static int next_input_report_idx(struct hogp_host_conn *c, uint8_t start)
{
	for (uint8_t i = start; i < c->num_reports; i++) {
		struct hogp_host_report *r = &c->reports[i];
		uint8_t pm = c->services[r->service_index].protocol_mode;

		if (pm == BT_HID_PROTOCOL_BOOT && !r->boot_report) {
			continue;
		}

		if (pm == BT_HID_PROTOCOL_REPORT && r->boot_report) {
			continue;
		}

		if (r->report_type == BT_HID_REPORT_TYPE_INPUT) {
			return i;
		}
	}

	return -1;
}

/* Find next non-boot Report char that needs Report Reference discovery */
static int next_report_char_idx(struct hogp_host_conn *c, uint8_t start)
{
	for (uint8_t i = start; i < c->num_reports; i++) {
		if (!c->reports[i].boot_report &&
		    c->reports[i].report_type == 0) {
			/* report_type==0 means not yet resolved */
			return i;
		}
	}

	return -1;
}

/* ---- State machine: GATT callbacks ---- */

/* Service discovery callback */
static uint8_t discover_service_cb(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   struct bt_gatt_discover_params *params)
{
	struct hogp_host_conn *c = find_conn(conn);

	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!attr) {
		/* Discovery complete */
		LOG_DBG("%s: complete, instances=%u", __func__, c->num_instances);
		if (c->num_instances == 0) {
			finish_connected(c, -ENOENT);
			return BT_GATT_ITER_STOP;
		}

		c->svc_idx = 0;
		c->state = HOGP_HOST_STATE_DISCOVER_CHARS;
		next_step(c);
		return BT_GATT_ITER_STOP;
	}

	if (c->num_instances < CONFIG_BT_HOGP_HOST_MAX_SERVICES) {
		struct bt_gatt_service_val *sv = attr->user_data;
		uint8_t idx = c->num_instances++;

		c->services[idx].start_handle = attr->handle;
		c->services[idx].end_handle = sv->end_handle;
		c->services[idx].desc_offset = c->desc_used;
		c->services[idx].desc_len = 0;
		LOG_DBG("HID Service[%u]: 0x%04x-0x%04x", idx,
			attr->handle, sv->end_handle);
	}

	return BT_GATT_ITER_CONTINUE;
}

/* Characteristic discovery callback */
static uint8_t discover_chars_cb(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 struct bt_gatt_discover_params *params)
{
	struct hogp_host_conn *c = find_conn(conn);
	struct bt_gatt_chrc *chrc;
	uint16_t uuid16;

	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!attr) {
		/* Done with this service's chars */
		if ((c->svc_idx + 1) < c->num_instances) {
			c->svc_idx++;
			c->state = HOGP_HOST_STATE_DISCOVER_CHARS;
			next_step(c);
		} else {
			/* All services' chars discovered */
			if (c->required_protocol_mode == BT_HID_PROTOCOL_BOOT) {
				c->svc_idx = 0;
				c->state = HOGP_HOST_STATE_SET_PROTOCOL_MODE;
				next_step(c);
			} else {
				/* Report mode: set protocol_mode field, go read HID Info */
				for (uint8_t i = 0; i < c->num_instances; i++) {
					c->services[i].protocol_mode =
						BT_HID_PROTOCOL_REPORT;
				}

				c->svc_idx = 0;
				c->state = HOGP_HOST_STATE_READ_HID_INFO;
				next_step(c);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;
	uuid16 = BT_UUID_16(chrc->uuid)->val;

	LOG_DBG("%s: svc[%u] uuid=0x%04x handle=0x%04x props=0x%02x", __func__,
		c->svc_idx, uuid16, chrc->value_handle, chrc->properties);

	switch (uuid16) {
	case BT_UUID_HIDS_PROTOCOL_MODE_VAL:
		c->services[c->svc_idx].protocol_mode_handle = chrc->value_handle;
		break;
	case BT_UUID_HIDS_BOOT_KB_IN_REPORT_VAL:
		add_report(c, chrc->value_handle, 0, chrc->properties,
			   1, BT_HID_REPORT_TYPE_INPUT, true);
		break;
	case BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT_VAL:
		add_report(c, chrc->value_handle, 0, chrc->properties,
			   2, BT_HID_REPORT_TYPE_INPUT, true);
		break;
	case BT_UUID_HIDS_BOOT_KB_OUT_REPORT_VAL:
		add_report(c, chrc->value_handle, 0, chrc->properties,
			   1, BT_HID_REPORT_TYPE_OUTPUT, true);
		break;
	case BT_UUID_HIDS_REPORT_VAL:
		/* report_type=0 means unresolved, will be read from Report Reference */
		add_report(c, chrc->value_handle, 0, chrc->properties,
			   0, 0, false);
		break;
	case BT_UUID_HIDS_REPORT_MAP_VAL:
		c->services[c->svc_idx].report_map_handle = chrc->value_handle;
		break;
	case BT_UUID_HIDS_INFO_VAL:
		c->services[c->svc_idx].hid_info_handle = chrc->value_handle;
		break;
	case BT_UUID_HIDS_CTRL_POINT_VAL:
		c->services[c->svc_idx].ctrl_point_handle = chrc->value_handle;
		break;
	default:
		break;
	}

	/* Track end_handle for the last added report in this service */
	if (c->num_reports > 0) {
		struct hogp_host_report *last = &c->reports[c->num_reports - 1];

		if (last->end_handle == 0 && last->service_index == c->svc_idx) {
			/* Will be updated when next char is found or service ends */
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

/* HID Information read callback */
static uint8_t read_hid_info_cb(struct bt_conn *conn, uint8_t err,
				struct bt_gatt_read_params *params,
				const void *data, uint16_t length)
{
	struct hogp_host_conn *c = find_conn(conn);

	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!err && data && length >= 4) {
		const uint8_t *v = data;
		struct hogp_host_hid_svc *svc = &c->services[c->svc_idx];

		svc->bcd_hid = v[0] | (v[1] << 8);
		svc->b_country_code = v[2];
		svc->hid_flags = v[3];
		LOG_DBG("HID Info[%u]: bcdHID=0x%04x country=%u flags=0x%02x",
			c->svc_idx, svc->bcd_hid, svc->b_country_code,
			svc->hid_flags);

		if (c->cb && c->cb->hid_info) {
			c->cb->hid_info(c->conn, c->svc_idx, svc->bcd_hid,
					svc->b_country_code, svc->hid_flags);
		}
	}

	/* Next service's HID Info */
	c->svc_idx++;
	while (c->svc_idx < c->num_instances) {
		if (c->services[c->svc_idx].hid_info_handle) {
			c->state = HOGP_HOST_STATE_READ_HID_INFO;
			next_step(c);
			return BT_GATT_ITER_STOP;
		}

		c->svc_idx++;
	}

	/* All HID Info read — go to Report Map */
	c->svc_idx = 0;
	c->state = HOGP_HOST_STATE_READ_REPORT_MAP;
	next_step(c);
	return BT_GATT_ITER_STOP;
}

/* Report Map read callback (supports long read) */
static uint8_t read_report_map_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params,
				  const void *data, uint16_t length)
{
	struct hogp_host_conn *c = find_conn(conn);
	struct hogp_host_hid_svc *svc;

	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (err) {
		LOG_ERR("Report map read error: %u", err);
		finish_connected(c, -EIO);
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("%s: err=%u data=%p len=%u svc_idx=%u", __func__,
		err, data, length, c->svc_idx);

	if (data && length > 0) {
		const uint8_t *bytes = data;

		for (uint16_t i = 0; i < length; i++) {
			if (desc_store_byte(c, bytes[i]) < 0) {
				LOG_WRN("Descriptor storage full");
				break;
			}
		}
		/* Continue long read */
		return BT_GATT_ITER_CONTINUE;
	}

	/* data==NULL: read complete for this service */
	svc = &c->services[c->svc_idx];

	LOG_DBG("Report map[%u]: %u bytes", c->svc_idx, svc->desc_len);

	/* Notify report map */
	if (c->cb && c->cb->report_map) {
		c->cb->report_map(conn, c->svc_idx,
				  &c->desc_storage[svc->desc_offset],
				  svc->desc_len);
	}

	/* Next service's report map */
	c->svc_idx++;
	while (c->svc_idx < c->num_instances) {
		if (c->services[c->svc_idx].report_map_handle) {
			c->state = HOGP_HOST_STATE_READ_REPORT_MAP;
			next_step(c);
			return BT_GATT_ITER_STOP;
		}

		c->svc_idx++;
	}

	/* All report maps read. Discover External Report Reference descriptors. */
	c->svc_idx = 0;
	c->state = HOGP_HOST_STATE_DISCOVER_REPORT_MAP_DESCS;
	LOG_DBG("%s: all maps read, -> DISCOVER_REPORT_MAP_DESCS", __func__);
	next_step(c);
	return BT_GATT_ITER_STOP;
}

/* External Report Reference descriptor discovery callback */
static uint8_t discover_ext_ref_cb(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   struct bt_gatt_discover_params *params)
{
	struct hogp_host_conn *c = find_conn(conn);
	uint16_t uuid16;

	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!attr) {
		/* Next service */
		c->svc_idx++;
		while (c->svc_idx < c->num_instances) {
			if (c->services[c->svc_idx].report_map_handle) {
				c->state = HOGP_HOST_STATE_DISCOVER_REPORT_MAP_DESCS;
				next_step(c);
				return BT_GATT_ITER_STOP;
			}

			c->svc_idx++;
		}
		/* All done, proceed to Report Reference discovery */
		int idx = next_report_char_idx(c, 0);

		if (idx >= 0) {
			c->rpt_idx = idx;
			c->state = HOGP_HOST_STATE_FIND_REPORT_DESCS;
			next_step(c);
		} else {
			idx = next_input_report_idx(c, 0);
			if (idx >= 0) {
				c->rpt_idx = idx;
				c->state = HOGP_HOST_STATE_ENABLE_NOTIFICATIONS;
				next_step(c);
			} else {
				advance_to_dis(c);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	uuid16 = BT_UUID_16(attr->uuid)->val;
	if (uuid16 == BT_UUID_HIDS_EXT_REPORT_VAL &&
	    c->num_ext_reports < ARRAY_SIZE(c->ext_reports)) {
		struct hogp_host_ext_report *er =
			&c->ext_reports[c->num_ext_reports++];
		er->desc_handle = attr->handle;
		er->service_index = c->svc_idx;
		LOG_DBG("Ext Report Ref desc at 0x%04x svc[%u]",
			attr->handle, c->svc_idx);
	}

	return BT_GATT_ITER_CONTINUE;
}

/* Descriptor discovery for Report Reference */
static uint8_t discover_report_desc_cb(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr,
				       struct bt_gatt_discover_params *params)
{
	struct hogp_host_conn *c = find_conn(conn);
	uint16_t uuid16;

	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!attr) {
		/* Done discovering descriptors for this report char */
		if (c->tmp_handle != 0) {
			/* Found Report Reference, read it */
			c->state = HOGP_HOST_STATE_READ_REPORT_ID_TYPE;
			next_step(c);
		} else {
			/* No Report Reference found, try next report */
			int idx = next_report_char_idx(c, c->rpt_idx + 1);

			if (idx >= 0) {
				c->rpt_idx = idx;
				c->state = HOGP_HOST_STATE_FIND_REPORT_DESCS;
				next_step(c);
			} else {
				/* All done, enable notifications */
				int nidx = next_input_report_idx(c, 0);

				if (nidx >= 0) {
					c->rpt_idx = nidx;
					c->state = HOGP_HOST_STATE_ENABLE_NOTIFICATIONS;
					next_step(c);
				} else {
					advance_to_dis(c);
				}
			}
		}

		return BT_GATT_ITER_STOP;
	}

	uuid16 = BT_UUID_16(attr->uuid)->val;

	if (uuid16 == BT_UUID_HIDS_REPORT_REF_VAL) {
		c->tmp_handle = attr->handle;
		LOG_DBG("Report Reference desc at 0x%04x for report[%u]",
			attr->handle, c->rpt_idx);
	} else if (uuid16 == BT_UUID_GATT_CCC_VAL) {
		LOG_DBG("CCC desc at 0x%04x for report[%u] (current=0x%04x)",
			attr->handle, c->rpt_idx, c->reports[c->rpt_idx].ccc_handle);
		/* Only use the first CCC found for this report;
		 * later CCC descriptors belong to other characteristics
		 * (e.g. SCI Mode) sharing the same HIDS service range.
		 */
		if (c->reports[c->rpt_idx].ccc_handle == 0) {
			c->reports[c->rpt_idx].ccc_handle = attr->handle;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

/* Read Report Reference descriptor result */
static uint8_t read_report_ref_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params,
				  const void *data, uint16_t length)
{
	struct hogp_host_conn *c = find_conn(conn);
	int idx;

	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!err && data && length >= 2) {
		const uint8_t *val = data;

		c->reports[c->rpt_idx].report_id = val[0];
		c->reports[c->rpt_idx].report_type = val[1];
		LOG_DBG("Report[%u]: id=%u type=%u", c->rpt_idx, val[0], val[1]);
	}

	/* Next report char */
	idx = next_report_char_idx(c, c->rpt_idx + 1);
	if (idx >= 0) {
		c->rpt_idx = idx;
		c->state = HOGP_HOST_STATE_FIND_REPORT_DESCS;
		next_step(c);
	} else {
		/* Enable notifications */
		int nidx = next_input_report_idx(c, 0);

		if (nidx >= 0) {
			c->rpt_idx = nidx;
			c->state = HOGP_HOST_STATE_ENABLE_NOTIFICATIONS;
			next_step(c);
		} else {
			advance_to_dis(c);
		}
	}

	return BT_GATT_ITER_STOP;
}

/* Subscribe callback */
static void subscribe_cb(struct bt_conn *conn, uint8_t err,
			 struct bt_gatt_subscribe_params *params)
{
	struct hogp_host_conn *c = find_conn(conn);
	int idx;

	if (!c) {
		return;
	}

	if (err) {
		LOG_WRN("Subscribe failed for handle 0x%04x: %u",
			params->value_handle, err);
	}

	/* Next input report */
	idx = next_input_report_idx(c, c->rpt_idx + 1);
	if (idx >= 0) {
		c->rpt_idx = idx;
		c->state = HOGP_HOST_STATE_ENABLE_NOTIFICATIONS;
		next_step(c);
	} else {
		advance_to_dis(c);
	}
}

/* ---- DIS discovery callback ---- */

static uint8_t discover_dis_cb(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       struct bt_gatt_discover_params *params)
{
	struct hogp_host_conn *c = find_conn(conn);
	struct bt_gatt_service_val *sv;

	LOG_DBG("%s: conn=%p attr=%p c=%p", __func__, conn, attr, c);
	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!attr) {
		if (c->dis_start_handle) {
			c->state = HOGP_HOST_STATE_DISCOVER_DIS_CHARS;
		} else {
			c->state = HOGP_HOST_STATE_DISCOVER_BAS;
		}

		next_step(c);
		return BT_GATT_ITER_STOP;
	}

	sv = attr->user_data;
	c->dis_start_handle = attr->handle;
	c->dis_end_handle = sv->end_handle;
	LOG_DBG("DIS: 0x%04x-0x%04x", attr->handle, sv->end_handle);

	/* Advance immediately — ITER_STOP does not trigger a NULL callback */
	c->state = HOGP_HOST_STATE_DISCOVER_DIS_CHARS;
	next_step(c);
	return BT_GATT_ITER_STOP; /* only one DIS instance */
}

/* DIS characteristic discovery callback */
static uint8_t discover_dis_chars_cb(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	struct hogp_host_conn *c = find_conn(conn);
	struct bt_gatt_chrc *chrc;

	LOG_DBG("%s: conn=%p attr=%p c=%p", __func__, conn, attr, c);
	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!attr) {
		if (c->pnp_id_handle) {
			c->state = HOGP_HOST_STATE_READ_PNP_ID;
		} else {
			c->state = HOGP_HOST_STATE_DISCOVER_BAS;
		}

		next_step(c);
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;
	if (BT_UUID_16(chrc->uuid)->val == BT_UUID_DIS_PNP_ID_VAL) {
		c->pnp_id_handle = chrc->value_handle;
	}

	return BT_GATT_ITER_CONTINUE;
}

/* PnP ID read callback */
static uint8_t read_pnp_id_cb(struct bt_conn *conn, uint8_t err,
			       struct bt_gatt_read_params *params,
			       const void *data, uint16_t length)
{
	struct hogp_host_conn *c = find_conn(conn);

	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!err && data && length >= 7 && c->cb && c->cb->pnp_id) {
		const uint8_t *v = data;
		struct bt_hogp_host_pnp_id id;

		id.vid_src = v[PNP_ID_VID_SRC_OFFSET];
		id.vid = v[PNP_ID_VID_OFFSET] | (v[PNP_ID_VID_OFFSET + 1] << 8);
		id.pid = v[PNP_ID_PID_OFFSET] | (v[PNP_ID_PID_OFFSET + 1] << 8);
		id.version = v[PNP_ID_VERSION_OFFSET] | (v[PNP_ID_VERSION_OFFSET + 1] << 8);
		c->cb->pnp_id(conn, &id);
	}

	c->state = HOGP_HOST_STATE_DISCOVER_BAS;
	next_step(c);
	return BT_GATT_ITER_STOP;
}

/* ---- BAS discovery callback ---- */

static uint8_t discover_bas_cb(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       struct bt_gatt_discover_params *params)
{
	struct hogp_host_conn *c = find_conn(conn);

	LOG_DBG("%s: conn=%p attr=%p c=%p", __func__, conn, attr, c);
	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!attr) {
		if (c->num_bats > 0) {
			c->bat_idx = 0;
			c->state = HOGP_HOST_STATE_DISCOVER_BAS_CHARS;
		} else {
			advance_to_finish(c);
			return BT_GATT_ITER_STOP;
		}

		next_step(c);
		return BT_GATT_ITER_STOP;
	}

	if (c->num_bats < BT_HOGP_HOST_MAX_BAT_INSTANCES) {
		struct bt_gatt_service_val *sv = attr->user_data;
		uint8_t idx = c->num_bats++;

		c->bats[idx].start_handle = attr->handle;
		c->bats[idx].end_handle = sv->end_handle;
		LOG_DBG("BAS[%u]: 0x%04x-0x%04x", idx,
			attr->handle, sv->end_handle);
	}

	return BT_GATT_ITER_CONTINUE;
}

/* BAS characteristic discovery callback */
static uint8_t discover_bas_chars_cb(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	struct hogp_host_conn *c = find_conn(conn);
	struct bt_gatt_chrc *chrc;

	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!attr) {
		if ((c->bat_idx + 1) < c->num_bats) {
			c->bat_idx++;
			c->state = HOGP_HOST_STATE_DISCOVER_BAS_CHARS;
			next_step(c);
		} else {
			/* Discover BAS descriptors to find CCC handles */
			c->bat_idx = 0;
			c->state = HOGP_HOST_STATE_DISCOVER_BAS_DESCS;
			next_step(c);
		}

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;
	if (BT_UUID_16(chrc->uuid)->val == BT_UUID_BAS_BATTERY_LEVEL_VAL) {
		c->bats[c->bat_idx].level_handle = chrc->value_handle;
	}

	return BT_GATT_ITER_CONTINUE;
}

/* BAS subscribe callback */
static void bat_subscribe_cb(struct bt_conn *conn, uint8_t err,
			     struct bt_gatt_subscribe_params *params)
{
	struct hogp_host_conn *c = find_conn(conn);

	if (!c) {
		return;
	}

	if ((c->bat_idx + 1) < c->num_bats) {
		c->bat_idx++;
		c->state = HOGP_HOST_STATE_ENABLE_BAT_NOTIFY;
		next_step(c);
	} else {
		advance_to_finish(c);
	}
}

/* BAS descriptor discovery callback - find CCC handle */
static uint8_t discover_bas_descs_cb(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	struct hogp_host_conn *c = find_conn(conn);

	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!attr) {
		if ((c->bat_idx + 1) < c->num_bats) {
			c->bat_idx++;
			c->state = HOGP_HOST_STATE_DISCOVER_BAS_DESCS;
		} else {
			c->bat_idx = 0;
			c->state = HOGP_HOST_STATE_READ_BAT_LEVEL;
		}
		next_step(c);
		return BT_GATT_ITER_STOP;
	}

	if (BT_UUID_16(params->uuid)->val == BT_UUID_GATT_CCC_VAL) {
		c->bats[c->bat_idx].ccc_handle = attr->handle;
		LOG_DBG("BAS[%u] CCC handle: 0x%04x", c->bat_idx, attr->handle);
	}

	return BT_GATT_ITER_CONTINUE;
}

/* BAS read battery level callback */
static uint8_t read_bat_level_cb(struct bt_conn *conn, uint8_t err,
				 struct bt_gatt_read_params *params,
				 const void *data, uint16_t length)
{
	struct hogp_host_conn *c = find_conn(conn);

	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	if (!err && data && length >= 1 && c->cb && c->cb->battery_level) {
		c->cb->battery_level(conn, c->bat_idx, ((const uint8_t *)data)[0]);
	}

	if ((c->bat_idx + 1) < c->num_bats) {
		c->bat_idx++;
		c->state = HOGP_HOST_STATE_READ_BAT_LEVEL;
	} else {
		c->bat_idx = 0;
		c->state = HOGP_HOST_STATE_ENABLE_BAT_NOTIFY;
	}
	next_step(c);

	return BT_GATT_ITER_STOP;
}

/* ---- advance_to_dis: transition from HID discovery to DIS/BAS ---- */

static void advance_to_finish(struct hogp_host_conn *c)
{
	finish_connected(c, 0);
}

static void advance_to_dis(struct hogp_host_conn *c)
{
	c->state = HOGP_HOST_STATE_DISCOVER_DIS;
	next_step(c);
}

/* ---- State machine driver ---- */

static void finish_connected(struct hogp_host_conn *c, int status)
{
	LOG_DBG("%s: status=%d, instances=%u, reports=%u, bats=%u", __func__,
		status, c->num_instances, c->num_reports, c->num_bats);
	if (status == 0) {
		c->state = HOGP_HOST_STATE_CONNECTED;
		LOG_DBG("HOGP Host connected: %u services, %u reports",
			c->num_instances, c->num_reports);
		for (uint8_t i = 0; i < c->num_reports; i++) {
			LOG_DBG("  report[%u]: handle=0x%04x id=%u type=%u boot=%d svc=%u",
				i, c->reports[i].value_handle, c->reports[i].report_id,
				c->reports[i].report_type, c->reports[i].boot_report,
				c->reports[i].service_index);
		}
	}

	if (c->cb && c->cb->connected) {
		c->cb->connected(c->conn, status, c->num_instances,
				 c->required_protocol_mode);
	}

	if (status != 0) {
		free_conn(c);
	}
}

static void next_step(struct hogp_host_conn *c)
{
	int err;

	static struct bt_uuid_16 hids_uuid = BT_UUID_INIT_16(BT_UUID_HIDS_VAL);

	LOG_DBG("%s: state=%s(%d) svc_idx=%u rpt_idx=%u", __func__,
		state_name(c->state), c->state, c->svc_idx, c->rpt_idx);

	switch (c->state) {
	case HOGP_HOST_STATE_DISCOVER_SERVICE:
		memset(&c->disc_params, 0, sizeof(c->disc_params));
		c->disc_params.uuid = &hids_uuid.uuid;
		c->disc_params.func = discover_service_cb;
		c->disc_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		c->disc_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		c->disc_params.type = BT_GATT_DISCOVER_PRIMARY;

		err = bt_gatt_discover(c->conn, &c->disc_params);
		LOG_DBG("bt_gatt_discover(PRIMARY, uuid=0x1812) ret=%d conn=%p", err, c->conn);
		if (err) {
			LOG_ERR("Service discovery failed: %d", err);
			finish_connected(c, err);
		}
		break;

	case HOGP_HOST_STATE_DISCOVER_CHARS:
		memset(&c->disc_params, 0, sizeof(c->disc_params));
		c->disc_params.uuid = NULL;
		c->disc_params.func = discover_chars_cb;
		c->disc_params.start_handle = c->services[c->svc_idx].start_handle;
		c->disc_params.end_handle = c->services[c->svc_idx].end_handle;
		c->disc_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(c->conn, &c->disc_params);
		if (err) {
			LOG_ERR("Char discovery failed: %d", err);
			finish_connected(c, err);
		}
		break;

	case HOGP_HOST_STATE_SET_PROTOCOL_MODE: {
		struct hogp_host_hid_svc *svc = &c->services[c->svc_idx];

		if (svc->protocol_mode_handle == 0) {
			if ((c->svc_idx + 1) < c->num_instances) {
				c->svc_idx++;
				next_step(c);
			} else {
				c->svc_idx = 0;
				c->state = HOGP_HOST_STATE_READ_HID_INFO;
				next_step(c);
			}
			break;
		}

		uint8_t mode = BT_HID_PROTOCOL_BOOT;

		err = bt_gatt_write_without_response(c->conn,
			svc->protocol_mode_handle, &mode, 1, false);
		if (err) {
			LOG_WRN("Set protocol mode failed: %d", err);
		}

		svc->protocol_mode = BT_HID_PROTOCOL_BOOT;

		if ((c->svc_idx + 1) < c->num_instances) {
			c->svc_idx++;
			next_step(c);
		} else {
			c->svc_idx = 0;
			c->state = HOGP_HOST_STATE_READ_HID_INFO;
			next_step(c);
		}
		break;
	}

	case HOGP_HOST_STATE_READ_HID_INFO: {
		/* Find next service with hid_info_handle */
		while (c->svc_idx < c->num_instances) {
			if (c->services[c->svc_idx].hid_info_handle) {
				break;
			}

			c->svc_idx++;
		}

		if (c->svc_idx >= c->num_instances) {
			/* No HID Info to read, go to Report Map */
			c->svc_idx = 0;
			c->state = HOGP_HOST_STATE_READ_REPORT_MAP;
			next_step(c);
			break;
		}

		memset(&c->read_params, 0, sizeof(c->read_params));
		c->read_params.func = read_hid_info_cb;
		c->read_params.handle_count = 1;
		c->read_params.single.handle =
			c->services[c->svc_idx].hid_info_handle;
		c->read_params.single.offset = 0;

		err = bt_gatt_read(c->conn, &c->read_params);
		if (err) {
			LOG_WRN("HID Info read failed: %d", err);
			c->svc_idx = 0;
			c->state = HOGP_HOST_STATE_READ_REPORT_MAP;
			next_step(c);
		}
		break;
	}

	case HOGP_HOST_STATE_READ_REPORT_MAP: {
		struct hogp_host_hid_svc *svc = &c->services[c->svc_idx];

		if (svc->report_map_handle == 0) {
			/* Skip services without report map */
			c->svc_idx++;
			while (c->svc_idx < c->num_instances) {
				if (c->services[c->svc_idx].report_map_handle) {
					break;
				}

				c->svc_idx++;
			}

			if (c->svc_idx < c->num_instances) {
				next_step(c);
			} else {
				/* No report maps, go to report ref discovery */
				int idx = next_report_char_idx(c, 0);

				if (idx >= 0) {
					c->rpt_idx = idx;
					c->state = HOGP_HOST_STATE_FIND_REPORT_DESCS;
					next_step(c);
				} else {
					int nidx = next_input_report_idx(c, 0);

					if (nidx >= 0) {
						c->rpt_idx = nidx;
						c->state = HOGP_HOST_STATE_ENABLE_NOTIFICATIONS;
						next_step(c);
					} else {
						advance_to_dis(c);
					}
				}
			}
			break;
		}

		svc->desc_offset = c->desc_used;
		svc->desc_len = 0;

		memset(&c->read_params, 0, sizeof(c->read_params));
		c->read_params.func = read_report_map_cb;
		c->read_params.handle_count = 1;
		c->read_params.single.handle = svc->report_map_handle;
		c->read_params.single.offset = 0;

		err = bt_gatt_read(c->conn, &c->read_params);
		if (err) {
			LOG_ERR("Report map read failed: %d", err);
			finish_connected(c, err);
		}
		break;
	}

	case HOGP_HOST_STATE_DISCOVER_REPORT_MAP_DESCS: {
		/* Find next service with report_map_handle */
		while (c->svc_idx < c->num_instances) {
			if (c->services[c->svc_idx].report_map_handle) {
				break;
			}

			c->svc_idx++;
		}

		if (c->svc_idx >= c->num_instances) {
			/* No more, go to Report Reference discovery */
			int idx = next_report_char_idx(c, 0);

			LOG_DBG("DISCOVER_REPORT_MAP_DESCS: no more svcs, "
				"next_report_char=%d num_reports=%u",
				idx, c->num_reports);
			if (idx >= 0) {
				c->rpt_idx = idx;
				c->state = HOGP_HOST_STATE_FIND_REPORT_DESCS;
				next_step(c);
			} else {
				idx = next_input_report_idx(c, 0);
				if (idx >= 0) {
					c->rpt_idx = idx;
					c->state = HOGP_HOST_STATE_ENABLE_NOTIFICATIONS;
					next_step(c);
				} else {
					advance_to_dis(c);
				}
			}
			break;
		}

		struct hogp_host_hid_svc *svc = &c->services[c->svc_idx];

		memset(&c->disc_params, 0, sizeof(c->disc_params));
		c->disc_params.uuid = NULL;
		c->disc_params.func = discover_ext_ref_cb;
		c->disc_params.start_handle = svc->report_map_handle;
		/* end_handle: find next char handle or service end */
		uint16_t end = svc->end_handle;

		for (uint8_t i = 0; i < c->num_reports; i++) {
			if (c->reports[i].service_index == c->svc_idx &&
			    c->reports[i].value_handle > svc->report_map_handle &&
			    c->reports[i].value_handle < end) {
				end = c->reports[i].value_handle - 1;
			}
		}

		c->disc_params.end_handle = end;
		c->disc_params.type = BT_GATT_DISCOVER_DESCRIPTOR;

		LOG_DBG("DISCOVER_REPORT_MAP_DESCS: svc[%u] discover 0x%04x-0x%04x",
			c->svc_idx, svc->report_map_handle, end);
		err = bt_gatt_discover(c->conn, &c->disc_params);
		if (err) {
			LOG_WRN("Ext ref desc discovery failed: %d", err);
			c->svc_idx++;
			c->state = HOGP_HOST_STATE_DISCOVER_REPORT_MAP_DESCS;
			next_step(c);
		}
		break;
	}

	case HOGP_HOST_STATE_FIND_REPORT_DESCS: {
		struct hogp_host_report *r = &c->reports[c->rpt_idx];

		c->tmp_handle = 0;
		memset(&c->disc_params, 0, sizeof(c->disc_params));
		c->disc_params.uuid = NULL;
		c->disc_params.func = discover_report_desc_cb;
		c->disc_params.start_handle = r->value_handle;
		/* end_handle: use next char's handle - 1, or service end */
		uint16_t end = c->services[r->service_index].end_handle;
		/* Find next report in same service with higher handle */
		for (uint8_t i = 0; i < c->num_reports; i++) {
			if (c->reports[i].service_index == r->service_index &&
			    c->reports[i].value_handle > r->value_handle &&
			    c->reports[i].value_handle < end) {
				end = c->reports[i].value_handle - 1;
			}
		}

		c->disc_params.end_handle = end;
		c->disc_params.type = BT_GATT_DISCOVER_DESCRIPTOR;

		err = bt_gatt_discover(c->conn, &c->disc_params);
		if (err) {
			LOG_ERR("Descriptor discovery failed: %d", err);
			finish_connected(c, err);
		}
		break;
	}

	case HOGP_HOST_STATE_READ_REPORT_ID_TYPE:
		memset(&c->read_params, 0, sizeof(c->read_params));
		c->read_params.func = read_report_ref_cb;
		c->read_params.handle_count = 1;
		c->read_params.single.handle = c->tmp_handle;
		c->read_params.single.offset = 0;

		err = bt_gatt_read(c->conn, &c->read_params);
		if (err) {
			LOG_ERR("Report ref read failed: %d", err);
			finish_connected(c, err);
		}
		break;

	case HOGP_HOST_STATE_ENABLE_NOTIFICATIONS: {
		struct hogp_host_report *r = &c->reports[c->rpt_idx];

		LOG_DBG("ENABLE_NOTIFICATIONS: rpt_idx=%d value_handle=0x%04x "
			"ccc_handle=0x%04x id=%d type=%d",
			c->rpt_idx, r->value_handle, r->ccc_handle,
			r->report_id, r->report_type);

		memset(&r->sub_params, 0, sizeof(r->sub_params));
		r->sub_params.notify = notify_cb;
		r->sub_params.subscribe = subscribe_cb;
		r->sub_params.value_handle = r->value_handle;
		r->sub_params.ccc_handle = r->ccc_handle;
		r->sub_params.value = BT_GATT_CCC_NOTIFY;
#if defined(CONFIG_BT_GATT_AUTO_DISCOVER_CCC)
		if (r->ccc_handle == 0) {
			r->sub_params.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
			r->sub_params.end_handle =
				c->services[r->service_index].end_handle;
			r->sub_params.disc_params = &c->disc_params;
		}
#endif

		atomic_set_bit(r->sub_params.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

		err = bt_gatt_subscribe(c->conn, &r->sub_params);
		if (err) {
			LOG_WRN("Subscribe failed handle 0x%04x: %d",
				r->value_handle, err);
			/* Continue to next */
			int idx = next_input_report_idx(c, c->rpt_idx + 1);

			if (idx >= 0) {
				c->rpt_idx = idx;
				next_step(c);
			} else {
				advance_to_dis(c);
			}
		}
		break;
	}

	case HOGP_HOST_STATE_DISCOVER_DIS: {
		static struct bt_uuid_16 dis_uuid = BT_UUID_INIT_16(BT_UUID_DIS_VAL);

		memset(&c->disc_params, 0, sizeof(c->disc_params));
		c->disc_params.uuid = &dis_uuid.uuid;
		c->disc_params.func = discover_dis_cb;
		c->disc_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		c->disc_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		c->disc_params.type = BT_GATT_DISCOVER_PRIMARY;

		err = bt_gatt_discover(c->conn, &c->disc_params);
		if (err) {
			LOG_WRN("DIS discovery failed: %d", err);
			c->state = HOGP_HOST_STATE_DISCOVER_BAS;
			next_step(c);
		}
		break;
	}

	case HOGP_HOST_STATE_DISCOVER_DIS_CHARS:
		memset(&c->disc_params, 0, sizeof(c->disc_params));
		c->disc_params.uuid = NULL;
		c->disc_params.func = discover_dis_chars_cb;
		c->disc_params.start_handle = c->dis_start_handle;
		c->disc_params.end_handle = c->dis_end_handle;
		c->disc_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(c->conn, &c->disc_params);
		if (err) {
			LOG_WRN("DIS char discovery failed: %d", err);
			c->state = HOGP_HOST_STATE_DISCOVER_BAS;
			next_step(c);
		}
		break;

	case HOGP_HOST_STATE_READ_PNP_ID:
		memset(&c->read_params, 0, sizeof(c->read_params));
		c->read_params.func = read_pnp_id_cb;
		c->read_params.handle_count = 1;
		c->read_params.single.handle = c->pnp_id_handle;
		c->read_params.single.offset = 0;

		err = bt_gatt_read(c->conn, &c->read_params);
		if (err) {
			LOG_WRN("PnP ID read failed: %d", err);
			c->state = HOGP_HOST_STATE_DISCOVER_BAS;
			next_step(c);
		}
		break;

	case HOGP_HOST_STATE_DISCOVER_BAS: {
		static struct bt_uuid_16 bas_uuid = BT_UUID_INIT_16(BT_UUID_BAS_VAL);

		memset(&c->disc_params, 0, sizeof(c->disc_params));
		c->disc_params.uuid = &bas_uuid.uuid;
		c->disc_params.func = discover_bas_cb;
		c->disc_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		c->disc_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		c->disc_params.type = BT_GATT_DISCOVER_PRIMARY;

		err = bt_gatt_discover(c->conn, &c->disc_params);
		if (err) {
			LOG_WRN("BAS discovery failed: %d", err);
			advance_to_finish(c);
		}
		break;
	}

	case HOGP_HOST_STATE_DISCOVER_BAS_CHARS:
		memset(&c->disc_params, 0, sizeof(c->disc_params));
		c->disc_params.uuid = NULL;
		c->disc_params.func = discover_bas_chars_cb;
		c->disc_params.start_handle = c->bats[c->bat_idx].start_handle;
		c->disc_params.end_handle = c->bats[c->bat_idx].end_handle;
		c->disc_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(c->conn, &c->disc_params);
		if (err) {
			LOG_WRN("BAS char discovery failed: %d", err);
			advance_to_finish(c);
		}
		break;

	case HOGP_HOST_STATE_DISCOVER_BAS_DESCS: {
		static struct bt_uuid_16 ccc_uuid = BT_UUID_INIT_16(BT_UUID_GATT_CCC_VAL);
		struct hogp_host_bat *b = &c->bats[c->bat_idx];

		if (b->level_handle == 0) {
			if ((c->bat_idx + 1) < c->num_bats) {
				c->bat_idx++;
				next_step(c);
			} else {
				c->bat_idx = 0;
				c->state = HOGP_HOST_STATE_READ_BAT_LEVEL;
				next_step(c);
			}
			break;
		}

		memset(&c->disc_params, 0, sizeof(c->disc_params));
		c->disc_params.uuid = &ccc_uuid.uuid;
		c->disc_params.func = discover_bas_descs_cb;
		c->disc_params.start_handle = b->level_handle + 1;
		c->disc_params.end_handle = b->end_handle;
		c->disc_params.type = BT_GATT_DISCOVER_DESCRIPTOR;

		err = bt_gatt_discover(c->conn, &c->disc_params);
		if (err) {
			LOG_WRN("BAS desc discovery failed: %d", err);
			if ((c->bat_idx + 1) < c->num_bats) {
				c->bat_idx++;
				next_step(c);
			} else {
				c->bat_idx = 0;
				c->state = HOGP_HOST_STATE_READ_BAT_LEVEL;
				next_step(c);
			}
		}
		break;
	}

	case HOGP_HOST_STATE_READ_BAT_LEVEL: {
		struct hogp_host_bat *b = &c->bats[c->bat_idx];

		if (b->level_handle == 0) {
			if ((c->bat_idx + 1) < c->num_bats) {
				c->bat_idx++;
				next_step(c);
			} else {
				c->bat_idx = 0;
				c->state = HOGP_HOST_STATE_ENABLE_BAT_NOTIFY;
				next_step(c);
			}
			break;
		}

		c->read_params.func = read_bat_level_cb;
		c->read_params.handle_count = 1;
		c->read_params.single.handle = b->level_handle;
		c->read_params.single.offset = 0;

		err = bt_gatt_read(c->conn, &c->read_params);
		if (err) {
			LOG_WRN("BAS read level failed: %d", err);
			if ((c->bat_idx + 1) < c->num_bats) {
				c->bat_idx++;
				next_step(c);
			} else {
				c->bat_idx = 0;
				c->state = HOGP_HOST_STATE_ENABLE_BAT_NOTIFY;
				next_step(c);
			}
		}
		break;
	}

	case HOGP_HOST_STATE_ENABLE_BAT_NOTIFY: {
		struct hogp_host_bat *b = &c->bats[c->bat_idx];

		if (b->level_handle == 0) {
			if ((c->bat_idx + 1) < c->num_bats) {
				c->bat_idx++;
				next_step(c);
			} else {
				advance_to_finish(c);
			}
			break;
		}

		memset(&b->sub_params, 0, sizeof(b->sub_params));
		b->sub_params.notify = bat_notify_cb;
		b->sub_params.subscribe = bat_subscribe_cb;
		b->sub_params.value_handle = b->level_handle;
		b->sub_params.ccc_handle = b->ccc_handle;
		b->sub_params.value = BT_GATT_CCC_NOTIFY;
#if defined(CONFIG_BT_GATT_AUTO_DISCOVER_CCC)
		if (b->ccc_handle == 0) {
			b->sub_params.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
			b->sub_params.end_handle = b->end_handle;
			b->sub_params.disc_params = &c->disc_params;
		}
#endif

		atomic_set_bit(b->sub_params.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

		err = bt_gatt_subscribe(c->conn, &b->sub_params);
		if (err) {
			LOG_WRN("BAS subscribe failed: %d", err);
			if ((c->bat_idx + 1) < c->num_bats) {
				c->bat_idx++;
				next_step(c);
			} else {
				advance_to_finish(c);
			}
		}
		break;
	}

	default:
		break;
	}
}

/* ---- Disconnect callback ---- */

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	struct hogp_host_conn *c = find_conn(conn);
	const struct bt_hogp_host_cb *cb;

	if (!c) {
		return;
	}

	LOG_DBG("%s: conn:%p reason=0x%02x state=%s", __func__,
		conn, reason, state_name(c->state));

	cb = c->cb;

	free_conn(c);
	if (cb && cb->disconnected) {
		cb->disconnected(conn, reason);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.disconnected = disconnected_cb,
};

static bool g_initialized;

/* ---- Get Report read callback ---- */

static uint8_t get_report_read_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params,
				  const void *data, uint16_t length)
{
	struct hogp_host_conn *c = find_conn(conn);

	LOG_DBG("%s: err=%u data=%p len=%u c=%p", __func__, err, data, length, c);
	if (!c) {
		return BT_GATT_ITER_STOP;
	}

	c->state = HOGP_HOST_STATE_CONNECTED;

	if (c->cb && c->cb->get_report_result) {
		struct hogp_host_report *r = find_report_by_handle(
			c, params->single.handle);

		if (r) {
			if (!err && data && length > 0) {
				c->cb->get_report_result(conn, r->report_id,
							 r->report_type,
							 data, length, 0);
			} else {
				c->cb->get_report_result(conn, r->report_id,
							 r->report_type,
							 NULL, 0,
							 err ? -EIO : -ENODATA);
			}
		}
	}

	return BT_GATT_ITER_STOP;
}

/* ---- Set Report write callback ---- */

static void set_report_write_cb(struct bt_conn *conn, uint8_t err,
				struct bt_gatt_write_params *params)
{
	struct hogp_host_conn *c = find_conn(conn);

	if (c) {
		c->state = HOGP_HOST_STATE_CONNECTED;
	}

	if (err) {
		LOG_WRN("Set report write err: %u", err);
	}
}

/* ---- Public API ---- */

void bt_hogp_host_init(void)
{
	if (g_initialized) {
		return;
	}

	memset(g_conns, 0, sizeof(g_conns));
	bt_conn_cb_register(&conn_callbacks);
	g_initialized = true;
}

int bt_hogp_host_connect(struct bt_conn *conn, uint8_t protocol_mode,
			 const struct bt_hogp_host_cb *cb)
{
	struct hogp_host_conn *c;

	LOG_DBG("%s: conn:%p protocol=%u", __func__, conn, protocol_mode);

	if (!conn || !cb) {
		return -EINVAL;
	}

	if (find_conn(conn)) {
		return -EALREADY;
	}

	c = alloc_conn(conn);

	if (!c) {
		return -ENOMEM;
	}

	c->cb = cb;
	c->required_protocol_mode = protocol_mode;
	c->state = HOGP_HOST_STATE_DISCOVER_SERVICE;
	next_step(c);
	return 0;
}

int bt_hogp_host_disconnect(struct bt_conn *conn)
{
	struct hogp_host_conn *c;

	LOG_DBG("%s: conn:%p", __func__, conn);
	c = find_conn(conn);

	if (!c) {
		return -ENOTCONN;
	}

	free_conn(c);
	return 0;
}

int bt_hogp_host_get_report(struct bt_conn *conn, uint8_t report_id,
			    uint8_t report_type)
{
	struct hogp_host_conn *c;
	struct hogp_host_report *r;
	int err;

	LOG_DBG("%s: id=%u type=%u", __func__, report_id, report_type);
	c = find_conn(conn);

	if (!c || c->state != HOGP_HOST_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	r = find_report(c, report_id, report_type);
	if (!r) {
		return -ENOENT;
	}

	c->state = HOGP_HOST_STATE_GET_REPORT;
	memset(&c->read_params, 0, sizeof(c->read_params));
	c->read_params.func = get_report_read_cb;
	c->read_params.handle_count = 1;
	c->read_params.single.handle = r->value_handle;
	c->read_params.single.offset = 0;

	err = bt_gatt_read(c->conn, &c->read_params);
	if (err) {
		c->state = HOGP_HOST_STATE_CONNECTED;
	}

	return err;
}

int bt_hogp_host_set_report(struct bt_conn *conn, uint8_t report_id,
			    uint8_t report_type, const uint8_t *data,
			    uint16_t len)
{
	struct hogp_host_conn *c;
	struct hogp_host_report *r;
	int err;

	LOG_DBG("%s: id=%u type=%u len=%u", __func__, report_id, report_type, len);
	c = find_conn(conn);

	if (!c || c->state != HOGP_HOST_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	r = find_report(c, report_id, report_type);
	if (!r) {
		return -ENOENT;
	}

	c->state = HOGP_HOST_STATE_SET_REPORT;
	memset(&c->write_params, 0, sizeof(c->write_params));
	c->write_params.func = set_report_write_cb;
	c->write_params.handle = r->value_handle;
	c->write_params.offset = 0;
	c->write_params.data = data;
	c->write_params.length = len;

	err = bt_gatt_write(c->conn, &c->write_params);
	if (err) {
		c->state = HOGP_HOST_STATE_CONNECTED;
	}

	return err;
}

int bt_hogp_host_set_protocol_mode(struct bt_conn *conn, uint8_t service_index,
				   uint8_t protocol_mode)
{
	struct hogp_host_conn *c;
	struct hogp_host_hid_svc *svc;
	int err;

	c = find_conn(conn);

	if (!c || c->state != HOGP_HOST_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (service_index >= c->num_instances) {
		return -EINVAL;
	}

	svc = &c->services[service_index];
	if (svc->protocol_mode_handle == 0) {
		return -ENOTSUP;
	}

	err = bt_gatt_write_without_response(c->conn,
		svc->protocol_mode_handle, &protocol_mode, 1, false);
	if (!err) {
		svc->protocol_mode = protocol_mode;
	}

	return err;
}

int bt_hogp_host_suspend(struct bt_conn *conn, uint8_t service_index)
{
	struct hogp_host_conn *c;
	uint8_t val;

	c = find_conn(conn);

	if (!c || c->state != HOGP_HOST_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (service_index >= c->num_instances ||
	    c->services[service_index].ctrl_point_handle == 0) {
		return -EINVAL;
	}

	val = 0; /* Suspend */
	return bt_gatt_write_without_response(c->conn,
		c->services[service_index].ctrl_point_handle, &val, 1, false);
}

int bt_hogp_host_exit_suspend(struct bt_conn *conn, uint8_t service_index)
{
	struct hogp_host_conn *c;
	uint8_t val;

	c = find_conn(conn);

	if (!c || c->state != HOGP_HOST_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (service_index >= c->num_instances ||
	    c->services[service_index].ctrl_point_handle == 0) {
		return -EINVAL;
	}

	val = 1; /* Exit Suspend */
	return bt_gatt_write_without_response(c->conn,
		c->services[service_index].ctrl_point_handle, &val, 1, false);
}

/* ---- Phase 2 stubs ---- */

int bt_hogp_host_set_mode(struct bt_conn *conn,
			  const struct bt_hogp_host_mode_param *param)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(param);

	return -ENOTSUP;
}

int bt_hogp_host_get_mode(struct bt_conn *conn,
			  struct bt_hogp_host_mode_param *param)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(param);

	return -ENOTSUP;
}


int bt_hogp_host_get_report_map(struct bt_conn *conn, uint8_t service_index,
				const uint8_t **data, uint16_t *len)
{
	struct hogp_host_conn *c;
	struct hogp_host_hid_svc *svc;

	c = find_conn(conn);

	if (!c) {
		return -ENOTCONN;
	}

	if (service_index >= c->num_instances) {
		return -EINVAL;
	}

	svc = &c->services[service_index];
	*data = &c->desc_storage[svc->desc_offset];
	*len = svc->desc_len;
	return 0;
}
