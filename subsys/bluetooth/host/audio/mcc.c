/** @file
 *  @brief Bluetooth Media Control Client/Protocol implementation
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <sys/byteorder.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/mcc.h>

#include "mpl.h"
#include "mpl_internal.h"
#include "otc.h"
#include "otc_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MCC)
#define LOG_MODULE_NAME bt_mcc
#include "common/log.h"

#define FIRST_HANDLE			0x0001
#define LAST_HANDLE			0xFFFF

struct mcs_instance_t {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t player_name_handle;
#ifdef CONFIG_BT_OTC
	uint16_t icon_obj_id_handle;
#endif /* CONFIG_BT_OTC */
	uint16_t icon_uri_handle;
	uint16_t track_changed_handle;
	uint16_t track_title_handle;
	uint16_t track_dur_handle;
	uint16_t track_pos_handle;
	uint16_t playback_speed_handle;
	uint16_t seeking_speed_handle;
#ifdef CONFIG_BT_OTC
	uint16_t segments_obj_id_handle;
	uint16_t current_track_obj_id_handle;
	uint16_t next_track_obj_id_handle;
	uint16_t current_group_obj_id_handle;
	uint16_t parent_group_obj_id_handle;
#endif /* CONFIG_BT_OTC */
	uint16_t playing_order_handle;
	uint16_t playing_orders_supported_handle;
	uint16_t media_state_handle;
	uint16_t cp_handle;
	uint16_t opcodes_supported_handle;
#ifdef CONFIG_BT_OTC
	uint16_t scp_handle;
	uint16_t search_results_obj_id_handle;
#endif /* CONFIG_BT_OTC */
	uint16_t content_control_id_handle;

	struct bt_gatt_subscribe_params player_name_sub_params;
	struct bt_gatt_subscribe_params track_changed_sub_params;
	struct bt_gatt_subscribe_params track_title_sub_params;
	struct bt_gatt_subscribe_params track_dur_sub_params;
	struct bt_gatt_subscribe_params track_pos_sub_params;
	struct bt_gatt_subscribe_params playback_speed_sub_params;
	struct bt_gatt_subscribe_params seeking_speed_sub_params;
#ifdef CONFIG_BT_OTC
	struct bt_gatt_subscribe_params current_track_obj_sub_params;
	struct bt_gatt_subscribe_params next_track_obj_sub_params;
	struct bt_gatt_subscribe_params current_group_obj_sub_params;
	struct bt_gatt_subscribe_params parent_group_obj_sub_params;
#endif /* CONFIG_BT_OTC */
	struct bt_gatt_subscribe_params playing_order_sub_params;
	struct bt_gatt_subscribe_params media_state_sub_params;
	struct bt_gatt_subscribe_params cp_sub_params;
	struct bt_gatt_subscribe_params opcodes_supported_sub_params;
#ifdef CONFIG_BT_OTC
	struct bt_gatt_subscribe_params scp_sub_params;
	struct bt_gatt_subscribe_params search_results_obj_sub_params;
#endif /* CONFIG_BT_OTC */

	/* The write buffer is used for
	 * - track position    (4 octets)
	 * - playback speed    (1 octet)
	 * - playing order     (1 octet)
	 * - the control point (5 octets)
	 *                     (1 octet opcode + optionally 4 octet param)
	 *                     (mpl_op_t.opcode  + mpl_opt_t.param)
	 * If the object transfer client is included, it is also used for
	 * - object IDs (6 octets - BT_OTS_OBJ_ID_SIZE) and
	 * - the search control point (64 octets - SEARCH_LEN_MAX)
	 *
	 * If there is no OTC, the largest is control point
	 * If OTC is included, the largest is the search control point
	 */
#ifdef CONFIG_BT_OTC
	char write_buf[SEARCH_LEN_MAX];
#else
	/* Trick to be able to use sizeof on members of a struct type */
	/* TODO: Rewrite the mpl_op_t to have the "use_param" parameter */
	/* separately, and the opcode and param alone as a struct */
	char write_buf[sizeof(((struct mpl_op_t *)0)->opcode) +
		       sizeof(((struct mpl_op_t *)0)->param)];
#endif /* CONFIG_BT_OTC */

	struct bt_gatt_write_params     write_params;
	bool busy;

#ifdef CONFIG_BT_OTC
	struct bt_otc_instance_t otc[CONFIG_BT_MCC_MAX_OTS_INST];
	uint8_t otc_inst_cnt;
#endif /* CONFIG_BT_OTC */
};

static struct bt_gatt_discover_params  discover_params;
static struct bt_gatt_read_params      read_params;

static struct mcs_instance_t *cur_mcs_inst;

static struct mcs_instance_t mcs_inst;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

static struct bt_mcc_cb_t *mcc_cb;
static bool subscribe_all;

#ifdef CONFIG_BT_OTC
NET_BUF_SIMPLE_DEFINE_STATIC(otc_obj_buf, CONFIG_BT_MCC_OTC_OBJ_BUF_SIZE);
static struct bt_otc_instance_t *cur_otc_inst;
static struct bt_otc_cb otc_cb;
#endif /* CONFIG_BT_OTC */



#ifdef CONFIG_BT_OTC
void on_obj_selected(struct bt_conn *conn, int err,
		     struct bt_otc_instance_t *otc_inst);

void on_object_metadata(struct bt_conn *conn, int err,
			struct bt_otc_instance_t *otc_inst,
			uint8_t metadata_read);

int on_icon_content(struct bt_conn *conn, uint32_t offset, uint32_t len,
		    uint8_t *data_p,
		    bool is_complete, struct bt_otc_instance_t *otc_inst);
#endif /* CONFIG_BT_OTC */


static uint8_t mcc_read_player_name_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	int cb_err = err;
	char name[CONFIG_BT_MCS_MEDIA_PLAYER_NAME_MAX];

	mcs_inst.busy = false;
	BT_DBG("err: 0x%02x, length: %d, data: %p", err, length, data);

	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!data) {
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		BT_HEXDUMP_DBG(data, length, "Player name read");

		if (length >= sizeof(name)) {
			length = sizeof(name) - 1;
		}

		memcpy(&name, data, length);
		name[length] = '\0';
		BT_DBG("Player name: %s", log_strdup(name));
	}

	if (mcc_cb && mcc_cb->player_name_read) {
		mcc_cb->player_name_read(conn, cb_err, name);
	}

	return BT_GATT_ITER_STOP;
}


#ifdef CONFIG_BT_OTC
static uint8_t mcc_read_icon_obj_id_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	mcs_inst.busy = false;
	BT_DBG("err: 0x%02x, length: %d, data: %p", err, length, data);
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if ((!pid) || (length != BT_OTS_OBJ_ID_SIZE)) {
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		BT_HEXDUMP_DBG(pid, length, "Icon Object ID");
		id = sys_get_le48(pid);
		BT_DBG_OBJ_ID("Icon Object ID: ", id);
	}

	if (mcc_cb && mcc_cb->icon_obj_id_read) {
		mcc_cb->icon_obj_id_read(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* CONFIG_BT_OTC */

static uint8_t mcc_read_icon_uri_cb(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params,
				    const void *data, uint16_t length)
{
	int cb_err = err;
	char uri[CONFIG_BT_MCS_ICON_URI_MAX];

	mcs_inst.busy = false;
	BT_DBG("err: 0x%02x, length: %d, data: %p", err, length, data);
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!data) {
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (length >= sizeof(uri)) {
		cb_err = BT_ATT_ERR_INSUFFICIENT_RESOURCES;
	} else {
		BT_HEXDUMP_DBG(data, length, "Icon URI");
		memcpy(&uri, data, length);
		uri[length] = '\0';
		BT_DBG("Icon URI: %s", log_strdup(uri));
	}

	if (mcc_cb && mcc_cb->icon_uri_read) {
		mcc_cb->icon_uri_read(conn, cb_err, uri);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_track_title_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	int cb_err = err;
	char title[CONFIG_BT_MCS_TRACK_TITLE_MAX];

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!data) {
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		BT_HEXDUMP_DBG(data, length, "Track title");
		if (length >= sizeof(title)) {
			/* If the description is too long; clip it. */
			length = sizeof(title) - 1;
		}
		memcpy(&title, data, length);
		title[length] = '\0';
		BT_DBG("Track title: %s", log_strdup(title));
	}

	if (mcc_cb && mcc_cb->track_title_read) {
		mcc_cb->track_title_read(conn, cb_err, title);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_track_dur_cb(struct bt_conn *conn, uint8_t err,
				     struct bt_gatt_read_params *params,
				     const void *data, uint16_t length)
{
	int cb_err = err;
	int32_t dur = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(dur))) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		dur = sys_get_le32((uint8_t *)data);
		BT_DBG("Track duration: %d", dur);
		BT_HEXDUMP_DBG(data, sizeof(int32_t), "Track duration");
	}

	if (mcc_cb && mcc_cb->track_dur_read) {
		mcc_cb->track_dur_read(conn, cb_err, dur);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_track_position_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	int cb_err = err;
	int32_t pos = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(pos))) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else  {
		pos = sys_get_le32((uint8_t *)data);
		BT_DBG("Track position: %d", pos);
		BT_HEXDUMP_DBG(data, sizeof(pos), "Track position");
	}

	if (mcc_cb && mcc_cb->track_position_read) {
		mcc_cb->track_position_read(conn, cb_err, pos);
	}

	return BT_GATT_ITER_STOP;
}

static void mcs_write_track_position_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_write_params *params)
{
	int cb_err = err;
	int32_t pos = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != sizeof(pos)) {
		BT_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		pos = sys_get_le32((uint8_t *)params->data);
		BT_DBG("Track position: %d", pos);
		BT_HEXDUMP_DBG(params->data, sizeof(pos),
			       "Track position in callback");
	}

	if (mcc_cb && mcc_cb->track_position_set) {
		mcc_cb->track_position_set(conn, cb_err, pos);
	}
}

static uint8_t mcc_read_playback_speed_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	int cb_err = err;
	int8_t speed = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(speed))) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		speed = *(int8_t *)data;
		BT_DBG("Playback speed: %d", speed);
		BT_HEXDUMP_DBG(data, sizeof(int8_t), "Playback speed");
	}

	if (mcc_cb && mcc_cb->playback_speed_read) {
		mcc_cb->playback_speed_read(conn, cb_err, speed);
	}

	return BT_GATT_ITER_STOP;
}

static void mcs_write_playback_speed_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_write_params *params)
{
	int cb_err = err;
	int8_t speed = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != sizeof(speed)) {
		BT_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		speed = *(int8_t *)params->data;
		BT_DBG("Playback_speed: %d", speed);
	}

	if (mcc_cb && mcc_cb->playback_speed_set) {
		mcc_cb->playback_speed_set(conn, cb_err, speed);
	}
}

static uint8_t mcc_read_seeking_speed_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_read_params *params,
					 const void *data, uint16_t length)
{
	int cb_err = err;
	int8_t speed = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(speed))) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		speed = *(int8_t *)data;
		BT_DBG("Seeking speed: %d", speed);
		BT_HEXDUMP_DBG(data, sizeof(int8_t), "Seeking speed");
	}

	if (mcc_cb && mcc_cb->seeking_speed_read) {
		mcc_cb->seeking_speed_read(conn, cb_err, speed);
	}

	return BT_GATT_ITER_STOP;
}

#ifdef CONFIG_BT_OTC
static uint8_t mcc_read_segments_obj_id_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_read_params *params,
					   const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if ((!pid) || (length != BT_OTS_OBJ_ID_SIZE)) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		BT_HEXDUMP_DBG(pid, length, "Segments Object ID");
		id = sys_get_le48(pid);
		BT_DBG_OBJ_ID("Segments Object ID: ", id);
	}

	if (mcc_cb && mcc_cb->segments_obj_id_read) {
		mcc_cb->segments_obj_id_read(conn, err, id);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_current_track_obj_id_cb(struct bt_conn *conn, uint8_t err,
						struct bt_gatt_read_params *params,
						const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if ((!pid) || (length != BT_OTS_OBJ_ID_SIZE)) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		BT_HEXDUMP_DBG(pid, length, "Current Track Object ID");
		id = sys_get_le48(pid);
		BT_DBG_OBJ_ID("Current Track Object ID: ", id);
	}

	if (mcc_cb && mcc_cb->current_track_obj_id_read) {
		mcc_cb->current_track_obj_id_read(conn, err, id);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_next_track_obj_id_cb(struct bt_conn *conn, uint8_t err,
					     struct bt_gatt_read_params *params,
					     const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (length == 0) {
		BT_DBG("Characteristic is empty");
	} else if (!pid || (length != BT_OTS_OBJ_ID_SIZE)) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		BT_HEXDUMP_DBG(pid, length, "Next Track Object ID");
		id = sys_get_le48(pid);
		BT_DBG_OBJ_ID("Next Track Object ID: ", id);
	}

	if (mcc_cb && mcc_cb->next_track_obj_id_read) {
		mcc_cb->next_track_obj_id_read(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_current_group_obj_id_cb(struct bt_conn *conn, uint8_t err,
						struct bt_gatt_read_params *params,
						const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!pid || (length != BT_OTS_OBJ_ID_SIZE)) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		BT_HEXDUMP_DBG(pid, length, "Current Group Object ID");
		id = sys_get_le48(pid);
		BT_DBG_OBJ_ID("Current Group Object ID: ", id);
	}

	if (mcc_cb && mcc_cb->current_group_obj_id_read) {
		mcc_cb->current_group_obj_id_read(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_parent_group_obj_id_cb(struct bt_conn *conn, uint8_t err,
					       struct bt_gatt_read_params *params,
					       const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!pid || (length != BT_OTS_OBJ_ID_SIZE)) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		BT_HEXDUMP_DBG(pid, length, "Parent Group Object ID");
		id = sys_get_le48(pid);
		BT_DBG_OBJ_ID("Parent Group Object ID: ", id);
	}

	if (mcc_cb && mcc_cb->parent_group_obj_id_read) {
		mcc_cb->parent_group_obj_id_read(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* CONFIG_BT_OTC */

static uint8_t mcc_read_playing_order_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_read_params *params,
					 const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t order = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(order))) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		order = *(uint8_t *)data;
		BT_DBG("Playing order: %d", order);
		BT_HEXDUMP_DBG(data, sizeof(order), "Playing order");
	}

	if (mcc_cb && mcc_cb->playing_order_read) {
		mcc_cb->playing_order_read(conn, cb_err, order);
	}

	return BT_GATT_ITER_STOP;
}

static void mcs_write_playing_order_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_write_params *params)
{
	int cb_err = err;
	uint8_t order = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != sizeof(order)) {
		BT_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		order = *(uint8_t *)params->data;
		BT_DBG("Playing order: %d", *(uint8_t *)params->data);
	}

	if (mcc_cb && mcc_cb->playing_order_set) {
		mcc_cb->playing_order_set(conn, cb_err, order);
	}
}

static uint8_t mcc_read_playing_orders_supported_cb(struct bt_conn *conn, uint8_t err,
						    struct bt_gatt_read_params *params,
						    const void *data, uint16_t length)
{
	int cb_err = err;
	uint16_t orders = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!data || length != sizeof(orders)) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		orders = sys_get_le16((uint8_t *)data);
		BT_DBG("Playing orders_supported: %d", orders);
		BT_HEXDUMP_DBG(data, sizeof(orders), "Playing orders supported");
	}

	if (mcc_cb && mcc_cb->playing_orders_supported_read) {
		mcc_cb->playing_orders_supported_read(conn, cb_err, orders);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_media_state_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t state = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!data || length != sizeof(state)) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		state = *(uint8_t *)data;
		BT_DBG("Media state: %d", state);
		BT_HEXDUMP_DBG(data, sizeof(uint8_t), "Media state");
	}

	if (mcc_cb && mcc_cb->media_state_read) {
		mcc_cb->media_state_read(conn, cb_err, state);
	}

	return BT_GATT_ITER_STOP;
}

static void mcs_write_cp_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_write_params *params)
{
	int cb_err = err;
	struct mpl_op_t op = {0};

	mcs_inst.busy = false;

	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!params->data ||
		   (params->length != sizeof(op.opcode) &&
		    params->length != sizeof(op.opcode) + sizeof(op.param))) {
		/* Above: No data pointer, or length not equal to any of the */
		/* two possible values */
		BT_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		memcpy(&op.opcode, params->data, sizeof(op.opcode));
		if (params->length == sizeof(op.opcode) + sizeof(op.param)) {
			memcpy(&op.param,
			       (char *)(params->data) + sizeof(op.opcode),
			       sizeof(op.param));
			op.use_param = true;
			BT_DBG("Operation in callback: %d, param: %d", op.opcode, op.param);
		}
	}

	if (mcc_cb && mcc_cb->cp_set) {
		mcc_cb->cp_set(conn, cb_err, op);
	}
}

static uint8_t mcc_read_opcodes_supported_cb(struct bt_conn *conn, uint8_t err,
					     struct bt_gatt_read_params *params,
					     const void *data, uint16_t length)
{
	int cb_err = err;
	int32_t operations = 0;

	mcs_inst.busy = false;

	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(operations))) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	} else {
		operations = sys_get_le32((uint8_t *)data);
		BT_DBG("Opcodes supported: %d", operations);
		BT_HEXDUMP_DBG(data, sizeof(operations), "Opcodes_supported");
	}

	if (mcc_cb && mcc_cb->opcodes_supported_read) {
		mcc_cb->opcodes_supported_read(conn, cb_err, operations);
	}

	return BT_GATT_ITER_STOP;
}

#ifdef CONFIG_BT_OTC
static void mcs_write_scp_cb(struct bt_conn *conn, uint8_t err,
			     struct bt_gatt_write_params *params)
{
	int cb_err = err;
	struct mpl_search_t search = {0};

	mcs_inst.busy = false;

	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!params->data ||
		   (params->length > SEARCH_LEN_MAX)) {
		BT_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		search.len = params->length;
		memcpy(search.search, params->data, params->length);
		BT_DBG("Length of returned value in callback: %d", search.len);
	}

	if (mcc_cb && mcc_cb->scp_set) {
		mcc_cb->scp_set(conn, cb_err, search);
	}
}

static uint8_t mcc_read_search_results_obj_id_cb(struct bt_conn *conn, uint8_t err,
						 struct bt_gatt_read_params *params,
						 const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	mcs_inst.busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (length == 0) {
		/* OK - this characteristic may be zero length */
		/* cb_err and id already have correct values */
		BT_DBG("Zero-length Search Results Object ID");
	} else if (!pid || (length != BT_OTS_OBJ_ID_SIZE)) {
		BT_DBG("length: %d, pid: %p", length, pid);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		id = sys_get_le48(pid);
		BT_DBG_OBJ_ID("Search Results Object ID: ", id);
	}

	if (mcc_cb && mcc_cb->search_results_obj_id_read) {
		mcc_cb->search_results_obj_id_read(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* CONFIG_BT_OTC */

static uint8_t mcc_read_content_control_id_cb(struct bt_conn *conn, uint8_t err,
					      struct bt_gatt_read_params *params,
					      const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t ccid = 0;

	mcs_inst.busy = false;

	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(ccid))) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	} else {
		ccid = *(int8_t *)data;
		BT_DBG("Content control ID: %d", ccid);
	}

	if (mcc_cb && mcc_cb->content_control_id_read) {
		mcc_cb->content_control_id_read(conn, cb_err, ccid);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcs_notify_handler(struct bt_conn *conn,
				  struct bt_gatt_subscribe_params *params,
				  const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	BT_DBG("Notification, handle: %d", handle);

	if (data) {
		if (handle == mcs_inst.player_name_handle) {
			BT_DBG("Player Name notification");
			mcc_read_player_name_cb(conn, 0, NULL, data, length);

		} else if (handle == mcs_inst.track_changed_handle) {
			/* The Track Changed characteristic can only be */
			/* notified, so that is handled directly here */

			BT_DBG("Track Changed notification");
			if (mcc_cb && mcc_cb->track_changed_ntf) {
				mcc_cb->track_changed_ntf(conn, 0);
			}

		} else if (handle == mcs_inst.track_title_handle) {
			BT_DBG("Track Title notification");
			mcc_read_track_title_cb(conn, 0, NULL, data, length);

		} else if (handle == mcs_inst.track_dur_handle) {
			BT_DBG("Track Duration notification");
			mcc_read_track_dur_cb(conn, 0, NULL, data, length);

		} else if (handle == mcs_inst.track_pos_handle) {
			BT_DBG("Track Position notification");
			mcc_read_track_position_cb(conn, 0, NULL, data, length);

		} else if (handle == mcs_inst.playback_speed_handle) {
			BT_DBG("Playback Speed notification");
			mcc_read_playback_speed_cb(conn, 0, NULL, data, length);

		} else if (handle == mcs_inst.seeking_speed_handle) {
			BT_DBG("Seeking Speed notification");
			mcc_read_seeking_speed_cb(conn, 0, NULL, data, length);

#ifdef CONFIG_BT_OTC
		} else if (handle == mcs_inst.current_track_obj_id_handle) {
			BT_DBG("Current Track notification");
			mcc_read_current_track_obj_id_cb(conn, 0, NULL, data,
							 length);

		} else if (handle == mcs_inst.next_track_obj_id_handle) {
			BT_DBG("Next Track notification");
			mcc_read_next_track_obj_id_cb(conn, 0, NULL, data,
						      length);

		} else if (handle == mcs_inst.current_group_obj_id_handle) {
			BT_DBG("Current Group notification");
			mcc_read_current_group_obj_id_cb(conn, 0, NULL, data,
							 length);

		} else if (handle == mcs_inst.parent_group_obj_id_handle) {
			BT_DBG("Parent Group notification");
			mcc_read_parent_group_obj_id_cb(conn, 0, NULL, data,
							 length);
#endif /* CONFIG_BT_OTC */

		} else if (handle == mcs_inst.playing_order_handle) {
			BT_DBG("Playing Order notification");
			mcc_read_playing_order_cb(conn, 0, NULL, data, length);

		} else if (handle == mcs_inst.media_state_handle) {
			BT_DBG("Media State notification");
			mcc_read_media_state_cb(conn, 0, NULL, data, length);

		} else if (handle == mcs_inst.cp_handle) {
			/* The control point is is a special case - only */
			/* writable and notifiable.  Handle directly here. */

			int cb_err = 0;
			struct mpl_op_ntf_t ntf = {0};

			BT_DBG("Control Point notification");
			if (length == sizeof(ntf.requested_opcode) + sizeof(ntf.result_code)) {
				ntf.requested_opcode = *(uint8_t *)data;
				ntf.result_code = *((uint8_t *)data +
						    sizeof(ntf.requested_opcode));
				BT_DBG("Operation: %d, result: %d",
				       ntf.requested_opcode, ntf.result_code);
			} else {
				BT_DBG("length: %d, data: %p", length, data);
				cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
			}

			if (mcc_cb && mcc_cb->cp_ntf) {
				mcc_cb->cp_ntf(conn, cb_err, ntf);
			}

		} else if (handle == mcs_inst.opcodes_supported_handle) {
			BT_DBG("Opcodes Supported notification");
			mcc_read_opcodes_supported_cb(conn, 0, NULL, data,
						      length);

#ifdef CONFIG_BT_OTC
		} else if (handle == mcs_inst.scp_handle) {
			/* The search control point is a special case - only */
			/* writable and notifiable.  Handle directly here. */

			int cb_err = 0;
			uint8_t result_code = 0;

			BT_DBG("Search Control Point notification");
			if (length == sizeof(result_code)) {
				result_code = *(uint8_t *)data;
				BT_DBG("Result: %d", result_code);
			} else {
				BT_DBG("length: %d, data: %p", length, data);
				cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
			}

			if (mcc_cb && mcc_cb->scp_ntf) {
				mcc_cb->scp_ntf(conn, cb_err, result_code);
			}

		} else if (handle == mcs_inst.search_results_obj_id_handle) {
			BT_DBG("Search Results notification");
			mcc_read_search_results_obj_id_cb(conn, 0, NULL, data,
							  length);
#endif /* CONFIG_BT_OTC */

		} else {
			BT_DBG("Unknown handle: %d (0x%04X)", handle, handle);
		}
	}
	return BT_GATT_ITER_CONTINUE;
}

#ifdef CONFIG_BT_OTC
#if CONFIG_BT_MCC_MAX_OTS_INST > 0
static uint8_t discover_otc_char_func(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	int err = 0;
	struct bt_gatt_chrc *chrc;
	static uint8_t next_idx;
	struct bt_gatt_subscribe_params *sub_params = NULL;

	if (attr) {
		/* Found an attribute */
		BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

		if (params->type != BT_GATT_DISCOVER_CHARACTERISTIC) {
			/* But it was not a characteristic - continue search */
			return BT_GATT_ITER_CONTINUE;
		}

		/* We have found an attribute, and it is a characteristic */
		/* Find out which attribute, and subscribe if we should */
		chrc = (struct bt_gatt_chrc *)attr->user_data;
		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_FEATURE)) {
			BT_DBG("OTS Features");
			cur_otc_inst->feature_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_NAME)) {
			BT_DBG("Object Name");
			cur_otc_inst->obj_name_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_TYPE)) {
			BT_DBG("Object Type");
			cur_otc_inst->obj_type_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_SIZE)) {
			BT_DBG("Object Size");
			cur_otc_inst->obj_size_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_ID)) {
			BT_DBG("Object ID");
			cur_otc_inst->obj_id_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_PROPERTIES)) {
			BT_DBG("Object properties %d", chrc->value_handle);
			cur_otc_inst->obj_properties_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_ACTION_CP)) {
			BT_DBG("Object Action Control Point");
			cur_otc_inst->oacp_handle = chrc->value_handle;
			sub_params = &cur_otc_inst->oacp_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_LIST_CP)) {
			BT_DBG("Object List Control Point");
			cur_otc_inst->olcp_handle = chrc->value_handle;
			sub_params = &cur_otc_inst->olcp_sub_params;
		}

		if (sub_params) {
			sub_params->value = BT_GATT_CCC_INDICATE;
			sub_params->value_handle = chrc->value_handle;
			/*
			 * TODO: Don't assume that CCC is at handle + 2;
			 * do proper discovery;
			 */
			sub_params->ccc_handle = attr->handle + 2;
			sub_params->notify = bt_otc_indicate_handler;

			bt_gatt_subscribe(conn, sub_params);
		}

		return BT_GATT_ITER_CONTINUE;
	}

	/* No more attributes found */
	cur_otc_inst->cb = &otc_cb;
	bt_otc_register(cur_otc_inst);

	next_idx++;
	BT_DBG("Setup complete for OTS %u / %u", next_idx,
	       mcs_inst.otc_inst_cnt);
	(void)memset(params, 0, sizeof(*params));

	if (next_idx < mcs_inst.otc_inst_cnt) {
		/* Discover characteristics of next included OTS */
		cur_otc_inst = &mcs_inst.otc[next_idx];
		discover_params.start_handle = cur_otc_inst->start_handle;
		discover_params.end_handle = cur_otc_inst->end_handle;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		discover_params.func = discover_otc_char_func;

		BT_DBG("Start discovery of OTS characteristics");
		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			BT_DBG("Discovery of OTS chars. failed (err %d)",
				   err);
			cur_otc_inst = NULL;
			if (mcc_cb && mcc_cb->discover_mcs) {
				mcc_cb->discover_mcs(conn, err);
			}
		}
	} else {
		cur_otc_inst = NULL;
		if (mcc_cb && mcc_cb->discover_mcs) {
			mcc_cb->discover_mcs(conn, err);
		}
	}
	return BT_GATT_ITER_STOP;
}
#endif /* CONFIG_BT_MCC_MAX_OTS_INST > 0 */
#endif /* CONFIG_BT_OTC */


#ifdef CONFIG_BT_OTC
#if (CONFIG_BT_MCC_MAX_OTS_INST > 0)
/* This function is called when an included service is found.
 * The function will store the start and end handle for the service,
 * and continue the search for more instances of included services.
 * Lastly, it will start discovery of OTS characteristics.
 */

static uint8_t discover_include_func(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_include *include;
	uint8_t inst_idx;
	int err = 0;

	if (attr) {
		BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

		__ASSERT(params->type == BT_GATT_DISCOVER_INCLUDE,
			 "Wrong type");

		/* We have found an included service */
		include = (struct bt_gatt_include *)attr->user_data;
		BT_DBG("Include UUID %s", bt_uuid_str(include->uuid));

		if (bt_uuid_cmp(include->uuid, BT_UUID_OTS)) {
			/* But it is not OTS - continue search */
			BT_WARN("Included service is not OTS");
			return BT_GATT_ITER_CONTINUE;
		}

		/* We have an included OTS service */
		if (mcs_inst.otc_inst_cnt < CONFIG_BT_MCC_MAX_OTS_INST) {
			inst_idx = mcs_inst.otc_inst_cnt;
			mcs_inst.otc[inst_idx].start_handle =
				include->start_handle;
			mcs_inst.otc[inst_idx].end_handle =
				include->end_handle;
			mcs_inst.otc_inst_cnt++;
			return BT_GATT_ITER_CONTINUE;
		}

		BT_WARN("More OTS instances than expected - skipping");
	}

	/* No more included services found */
	BT_DBG("Discover include complete for GMCS: OTS");
	(void)memset(params, 0, sizeof(*params));
	if (mcs_inst.otc_inst_cnt) {

		/* Discover characteristics of any included OTS */
		cur_otc_inst = &mcs_inst.otc[0];
		discover_params.start_handle = cur_otc_inst->start_handle;
		discover_params.end_handle = cur_otc_inst->end_handle;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		discover_params.func = discover_otc_char_func;

		BT_DBG("Start discovery of OTS characteristics");
		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			BT_DBG("Discovery of OTS chars. failed (err %d)",
				   err);
			cur_otc_inst = NULL;
			if (mcc_cb && mcc_cb->discover_mcs) {
				mcc_cb->discover_mcs(conn, err);
			}
		}
	} else {
		cur_otc_inst = NULL;
		if (mcc_cb && mcc_cb->discover_mcs) {
			mcc_cb->discover_mcs(conn, err);
		}
	}
	return BT_GATT_ITER_STOP;
}
#endif /* (CONFIG_BT_MCC_MAX_OTS_INST > 0)*/
#endif /* CONFIG_BT_OTC */


/* This function is called when characteristics are found.
 * The function will store handles, and optionally subscribe to, GMCS
 * characteristics.
 * After this, the function will start discovery of included services.
 */
static uint8_t discover_mcs_char_func(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	struct bt_gatt_chrc *chrc;
	struct bt_gatt_subscribe_params *sub_params = NULL;

	if (attr) {
		/* Found an attribute */
		BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

		if (params->type != BT_GATT_DISCOVER_CHARACTERISTIC) {
			/* But it was not a characteristic - continue search */
			return BT_GATT_ITER_CONTINUE;
		}

		/* We have found an attribute, and it is a characteristic */
		/* Find out which attribute, and subscribe if we should */
		chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PLAYER_NAME)) {
			BT_DBG("Player name, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.player_name_handle = chrc->value_handle;
			sub_params = &mcs_inst.player_name_sub_params;
#ifdef CONFIG_BT_OTC
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_ICON_OBJ_ID)) {
			BT_DBG("Icon Object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.icon_obj_id_handle = chrc->value_handle;
#endif /* CONFIG_BT_OTC */
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_ICON_URI)) {
			BT_DBG("Icon URI, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.icon_uri_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_CHANGED)) {
			BT_DBG("Track Changed, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.track_changed_handle = chrc->value_handle;
			sub_params = &mcs_inst.track_changed_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_TITLE)) {
			BT_DBG("Track Title, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.track_title_handle = chrc->value_handle;
			sub_params = &mcs_inst.track_title_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_DURATION)) {
			BT_DBG("Track Duration, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.track_dur_handle = chrc->value_handle;
			sub_params = &mcs_inst.track_dur_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_POSITION)) {
			BT_DBG("Track Position, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.track_pos_handle = chrc->value_handle;
			sub_params = &mcs_inst.track_pos_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PLAYBACK_SPEED)) {
			BT_DBG("Playback Speed, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.playback_speed_handle = chrc->value_handle;
			sub_params = &mcs_inst.playback_speed_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_SEEKING_SPEED)) {
			BT_DBG("Seeking Speed, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.seeking_speed_handle = chrc->value_handle;
			sub_params = &mcs_inst.seeking_speed_sub_params;
#ifdef CONFIG_BT_OTC
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_SEGMENTS_OBJ_ID)) {
			BT_DBG("Track Segments Object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.segments_obj_id_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_CURRENT_TRACK_OBJ_ID)) {
			BT_DBG("Current Track Object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.current_track_obj_id_handle = chrc->value_handle;
			sub_params = &mcs_inst.current_track_obj_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_NEXT_TRACK_OBJ_ID)) {
			BT_DBG("Next Track Object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.next_track_obj_id_handle = chrc->value_handle;
			sub_params = &mcs_inst.next_track_obj_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_GROUP_OBJ_ID)) {
			BT_DBG("Group Object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.current_group_obj_id_handle = chrc->value_handle;
			sub_params = &mcs_inst.current_group_obj_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PARENT_GROUP_OBJ_ID)) {
			BT_DBG("Parent Group Object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.parent_group_obj_id_handle = chrc->value_handle;
			sub_params = &mcs_inst.parent_group_obj_sub_params;
#endif /* CONFIG_BT_OTC */
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PLAYING_ORDER)) {
			BT_DBG("Playing Order, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.playing_order_handle = chrc->value_handle;
			sub_params = &mcs_inst.playing_order_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PLAYING_ORDERS)) {
			BT_DBG("Playing Orders supported, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.playing_orders_supported_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_MEDIA_STATE)) {
			BT_DBG("Media State, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.media_state_handle = chrc->value_handle;
			sub_params = &mcs_inst.media_state_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_MEDIA_CONTROL_POINT)) {
			BT_DBG("Media Control Point, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.cp_handle = chrc->value_handle;
			sub_params = &mcs_inst.cp_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_MEDIA_CONTROL_OPCODES)) {
			BT_DBG("Media control opcodes supported, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.opcodes_supported_handle = chrc->value_handle;
			sub_params = &mcs_inst.opcodes_supported_sub_params;
#ifdef CONFIG_BT_OTC
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_SEARCH_CONTROL_POINT)) {
			BT_DBG("Search control point, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.scp_handle = chrc->value_handle;
			sub_params = &mcs_inst.scp_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID)) {
			BT_DBG("Search Results object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.search_results_obj_id_handle = chrc->value_handle;
			sub_params = &mcs_inst.search_results_obj_sub_params;
#endif /* CONFIG_BT_OTC */
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_CCID)) {
			BT_DBG("Content Control ID, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst.content_control_id_handle = chrc->value_handle;
		}


		if (subscribe_all && sub_params) {
			BT_DBG("Subscribing - handle: 0x%04x", attr->handle);
			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = chrc->value_handle;
			sub_params->ccc_handle = attr->handle + 2;
			sub_params->notify = mcs_notify_handler;
			bt_gatt_subscribe(conn, sub_params);
		}

		/* Continue to search for more attributes */
		return BT_GATT_ITER_CONTINUE;
	}

	/* No more attributes found */
	BT_DBG("Setup complete for GMCS");
	(void)memset(params, 0, sizeof(*params));

#ifdef CONFIG_BT_OTC
#if (CONFIG_BT_MCC_MAX_OTS_INST > 0)
	int err = 0;

	/* Discover included services */
	discover_params.start_handle = mcs_inst.start_handle;
	discover_params.end_handle = mcs_inst.end_handle;
	discover_params.type = BT_GATT_DISCOVER_INCLUDE;
	discover_params.func = discover_include_func;

	BT_DBG("Start discovery of included services");
	err = bt_gatt_discover(conn, &discover_params);
	if (err) {
		BT_DBG("Discover of included service failed (err %d)", err);
		if (mcc_cb && mcc_cb->discover_mcs) {
			mcc_cb->discover_mcs(conn, err);
		}
	}
#else
	if (mcc_cb && mcc_cb->discover_mcs) {
		mcc_cb->discover_mcs(conn, err);
	}
#endif /* (CONFIG_BT_MCC_MAX_OTS_INST > 0)*/
#endif /* CONFIG_BT_OTC */
	return BT_GATT_ITER_STOP;
}


/* This function is called when a (primary) GMCS service has been discovered.
 * The function will store the start and end handle for the service. It will
 * then start discovery of the characteristics of the GMCS service.
 */
static uint8_t discover_primary_func(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	int err;
	struct bt_gatt_service_val *prim_service;

	if (attr) {
		/* Found an attribute */
		BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

		if (params->type != BT_GATT_DISCOVER_PRIMARY) {
			/* But it was not a primary service - continue search */
			BT_WARN("Unexpected parameters");
			return BT_GATT_ITER_CONTINUE;
		}

		/* We have found an attribute, and it is a primary service */
		/* (Must be GMCS, since that is the one we searched for.) */
		BT_DBG("Primary discovery complete");
		BT_DBG("UUID: %s", bt_uuid_str(attr->uuid));
		prim_service = (struct bt_gatt_service_val *)attr->user_data;
		BT_DBG("UUID: %s", bt_uuid_str(prim_service->uuid));

		cur_mcs_inst = &mcs_inst;
		mcs_inst.start_handle = attr->handle + 1;
		mcs_inst.end_handle = prim_service->end_handle;

		/* Start discovery of characeristics */
		discover_params.uuid = NULL;
		discover_params.start_handle = mcs_inst.start_handle;
		discover_params.end_handle = mcs_inst.end_handle;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		discover_params.func = discover_mcs_char_func;

		BT_DBG("Start discovery of GMCS characteristics");
		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			BT_DBG("Discover failed (err %d)", err);
			cur_mcs_inst = NULL;
			if (mcc_cb && mcc_cb->discover_mcs) {
				mcc_cb->discover_mcs(conn, err);
			}
		}
		return BT_GATT_ITER_STOP;
	}

	/* No attribute of the searched for type found */
	BT_DBG("Could not find an GMCS instance on the server");
	cur_mcs_inst = NULL;
	if (mcc_cb && mcc_cb->discover_mcs) {
		mcc_cb->discover_mcs(conn, -ENODATA);
	}
	return BT_GATT_ITER_STOP;
}


int bt_mcc_init(struct bt_conn *conn, struct bt_mcc_cb_t *cb)
{
	mcc_cb = cb;

	if (mcc_cb && mcc_cb->init) {
		mcc_cb->init(conn, 0);
	}

#ifdef CONFIG_BT_OTC
	/* Set up the callbacks from OTC */
	/* TODO: Have one single content callback. */
	/* For now: Use the icon callback for content - it is the first, */
	/* and this will anyway be reset later. */
	otc_cb.content_cb  = on_icon_content;
	otc_cb.obj_selected = on_obj_selected;
	otc_cb.metadata_cb = on_object_metadata;

	BT_DBG("Current object selected callback: %p", otc_cb.obj_selected);
	BT_DBG("Content callback: %p", otc_cb.content_cb);
	BT_DBG("Metadata callback: %p", otc_cb.metadata_cb);
#endif /* CONFIG_BT_OTC */

	return 0;
}


/* Initiate discovery.
 * Discovery is handled by a chain of functions, where each function does its
 * part, and then initiates a further discovery, with a new callback function.
 *
 * The order of discovery is follows:
 * 1: Discover GMCS primary service (started here)
 * 2: Discover characteristics of GMCS
 * 3: Discover OTS service included in GMCS
 * 4: Discover characteristics of OTS
 */
int bt_mcc_discover_mcs(struct bt_conn *conn, bool subscribe)
{
	if (!conn) {
		return -ENOTCONN;
	} else if (cur_mcs_inst) {
		return -EBUSY;
	}

	subscribe_all = subscribe;
	memset(&discover_params, 0, sizeof(discover_params));
	memset(&mcs_inst, 0, sizeof(mcs_inst));
	memcpy(&uuid, BT_UUID_GMCS, sizeof(uuid));

	discover_params.func = discover_primary_func;
	discover_params.uuid = &uuid.uuid;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.start_handle = FIRST_HANDLE;
	discover_params.end_handle = LAST_HANDLE;

	BT_DBG("start discovery of GMCS primary service");
	return bt_gatt_discover(conn, &discover_params);
}


int bt_mcc_read_player_name(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.player_name_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_player_name_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.player_name_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}


#ifdef CONFIG_BT_OTC
int bt_mcc_read_icon_obj_id(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.icon_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_icon_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.icon_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}
#endif /* CONFIG_BT_OTC */

int bt_mcc_read_icon_uri(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.icon_uri_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_icon_uri_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.icon_uri_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_track_title(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.track_title_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_track_title_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.track_title_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_track_dur(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.track_dur_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_track_dur_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.track_dur_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_track_position(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.track_pos_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_track_position_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.track_pos_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_set_track_position(struct bt_conn *conn, int32_t pos)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.track_pos_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	memcpy(mcs_inst.write_buf, &pos, sizeof(pos));

	mcs_inst.write_params.offset = 0;
	mcs_inst.write_params.data = mcs_inst.write_buf;
	mcs_inst.write_params.length = sizeof(pos);
	mcs_inst.write_params.handle = mcs_inst.track_pos_handle;
	mcs_inst.write_params.func = mcs_write_track_position_cb;

	BT_HEXDUMP_DBG(mcs_inst.write_params.data, sizeof(pos),
		       "Track position sent");

	err = bt_gatt_write(conn, &mcs_inst.write_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_playback_speed(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.playback_speed_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_playback_speed_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.playback_speed_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_set_playback_speed(struct bt_conn *conn, int8_t speed)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.playback_speed_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	memcpy(mcs_inst.write_buf, &speed, sizeof(speed));

	mcs_inst.write_params.offset = 0;
	mcs_inst.write_params.data = mcs_inst.write_buf;
	mcs_inst.write_params.length = sizeof(speed);
	mcs_inst.write_params.handle = mcs_inst.playback_speed_handle;
	mcs_inst.write_params.func = mcs_write_playback_speed_cb;

	BT_HEXDUMP_DBG(mcs_inst.write_params.data, sizeof(speed),
		       "Playback speed");

	err = bt_gatt_write(conn, &mcs_inst.write_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_seeking_speed(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.seeking_speed_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_seeking_speed_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.seeking_speed_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

#ifdef CONFIG_BT_OTC
int bt_mcc_read_segments_obj_id(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.segments_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_segments_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.segments_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_current_track_obj_id(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.current_track_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_current_track_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.current_track_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_next_track_obj_id(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.next_track_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_next_track_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.next_track_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_current_group_obj_id(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.current_group_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_current_group_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.current_group_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_parent_group_obj_id(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.parent_group_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_parent_group_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.parent_group_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}
#endif /* CONFIG_BT_OTC */

int bt_mcc_read_playing_order(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.playing_order_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_playing_order_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.playing_order_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_set_playing_order(struct bt_conn *conn, uint8_t order)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.playing_order_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	memcpy(mcs_inst.write_buf, &order, sizeof(order));

	mcs_inst.write_params.offset = 0;
	mcs_inst.write_params.data = mcs_inst.write_buf;
	mcs_inst.write_params.length = sizeof(order);
	mcs_inst.write_params.handle = mcs_inst.playing_order_handle;
	mcs_inst.write_params.func = mcs_write_playing_order_cb;

	BT_HEXDUMP_DBG(mcs_inst.write_params.data, sizeof(order),
		       "Playing order");

	err = bt_gatt_write(conn, &mcs_inst.write_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_playing_orders_supported(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.playing_orders_supported_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_playing_orders_supported_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.playing_orders_supported_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_media_state(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.media_state_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_media_state_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.media_state_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_set_cp(struct bt_conn *conn, struct mpl_op_t op)
{
	int err;
	int length = sizeof(op.opcode);

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.cp_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	memcpy(mcs_inst.write_buf, &op.opcode, length);
	if (op.use_param) {
		length += sizeof(op.param);
		memcpy(&mcs_inst.write_buf[sizeof(op.opcode)], &op.param,
		       sizeof(op.param));
	}

	mcs_inst.write_params.offset = 0;
	mcs_inst.write_params.data = mcs_inst.write_buf;
	mcs_inst.write_params.length = length;
	mcs_inst.write_params.handle = mcs_inst.cp_handle;
	mcs_inst.write_params.func = mcs_write_cp_cb;

	BT_HEXDUMP_DBG(mcs_inst.write_params.data, sizeof(op),
		       "Operation sent");

	err = bt_gatt_write(conn, &mcs_inst.write_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_opcodes_supported(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.opcodes_supported_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_opcodes_supported_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.opcodes_supported_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

#ifdef CONFIG_BT_OTC
int bt_mcc_set_scp(struct bt_conn *conn, struct mpl_search_t search)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.scp_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	memcpy(mcs_inst.write_buf, &search.search, search.len);

	mcs_inst.write_params.offset = 0;
	mcs_inst.write_params.data = mcs_inst.write_buf;
	mcs_inst.write_params.length = search.len;
	mcs_inst.write_params.handle = mcs_inst.scp_handle;
	mcs_inst.write_params.func = mcs_write_scp_cb;

	BT_HEXDUMP_DBG(mcs_inst.write_params.data, search.len,
		       "Search sent");

	err = bt_gatt_write(conn, &mcs_inst.write_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

int bt_mcc_read_search_results_obj_id(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.search_results_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_search_results_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.search_results_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}
#endif /* CONFIG_BT_OTC */

int bt_mcc_read_content_control_id(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mcs_inst.content_control_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mcs_inst.busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_content_control_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = mcs_inst.content_control_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		mcs_inst.busy = true;
	}
	return err;
}

#ifdef CONFIG_BT_OTC
#if CONFIG_BT_MCC_MAX_OTS_INST > 0

void on_obj_selected(struct bt_conn *conn, int result,
		     struct bt_otc_instance_t *otc_inst)
{
	BT_DBG("Current object selected");
	/* TODO: Read metadata here? */
	/* For now: Left to the application */

	/* Only one object at a time is selected in OTS */
	/* When the selected callback comes, a new object is selected */
	/* Reset the object buffer */
	net_buf_simple_reset(&otc_obj_buf);

#if CONFIG_BT_DEBUG_MCC
	if (mcc_cb && mcc_cb->otc_obj_selected) {
		mcc_cb->otc_obj_selected(conn, OLCP_RESULT_TO_ERROR(result));
	}
#endif /* CONFIG_BT_DEBUG_MCC */
}


struct track_seg_t {
	uint8_t            name_len;
	char               name[CONFIG_BT_MCS_SEGMENT_NAME_MAX];
	int32_t            pos;
};


struct track_segs_t {
	uint16_t              cnt;
	struct track_seg_t    segs[CONFIG_BT_MCC_TRACK_SEGS_MAX_CNT];
};



static void decode_track_segments(struct net_buf_simple *buff,
				  struct track_segs_t *track_segs)
{
	uint16_t i;
	struct track_seg_t *seg;
	uint8_t *name;

	while (buff->len &&
	       track_segs->cnt < CONFIG_BT_MCC_TRACK_SEGS_MAX_CNT) {

		i = track_segs->cnt++;
		seg = &track_segs->segs[i];

		seg->name_len =  net_buf_simple_pull_u8(buff);
		if (seg->name_len + sizeof(int32_t) > buff->len) {
			BT_WARN("Segment too long");
			return;
		}

		if (seg->name_len) {

			name = net_buf_simple_pull_mem(buff, seg->name_len);

			if (seg->name_len >= CONFIG_BT_MCS_SEGMENT_NAME_MAX) {
				seg->name_len =
					CONFIG_BT_MCS_SEGMENT_NAME_MAX - 1;
			}
			memcpy(seg->name, name, seg->name_len);
		}
		seg->name[seg->name_len] = '\0';

		track_segs->segs[i].pos = (int32_t)net_buf_simple_pull_le32(buff);
	}
}


struct id_list_elem_t {
	uint8_t  type;
	uint64_t id;
};

struct id_list_t {
	struct id_list_elem_t ids[CONFIG_BT_MCC_GROUP_RECORDS_MAX];
	uint16_t cnt;
};

static void decode_current_group(struct net_buf_simple *buff,
				 struct id_list_t *ids)
{
	while ((buff->len) && (ids->cnt < CONFIG_BT_MCC_GROUP_RECORDS_MAX)) {
		ids->ids[ids->cnt].type = net_buf_simple_pull_u8(buff);
		ids->ids[ids->cnt++].id = net_buf_simple_pull_le48(buff);
	}
}

/* TODO: Merge the object callback functions into one */
/* Use a notion of the "active" object, as done in mpl.c, for tracking  */
int on_icon_content(struct bt_conn *conn, uint32_t offset, uint32_t len,
		    uint8_t *data_p, bool is_complete,
		    struct bt_otc_instance_t *otc_inst)
{
	BT_INFO("Received Media Player Icon content, %i bytes at offset %i",
		len, offset);

	BT_HEXDUMP_DBG(data_p, len, "Icon content");

	return BT_OTC_CONTINUE;
}

int on_track_segments_content(struct bt_conn *conn, uint32_t offset,
			      uint32_t len, uint8_t *data_p, bool is_complete,
			      struct bt_otc_instance_t *otc_inst)
{
	static struct track_segs_t track_segments;

	BT_INFO("Received Segments content, %i bytes at offset %i",
		len, offset);

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		BT_DBG("Can not fit whole object");
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		BT_INFO("Track segment object received");
		decode_track_segments(&otc_obj_buf, &track_segments);
		for (int i = 0; i < track_segments.cnt; i++) {
			BT_DBG("Track segment %i:", i);
			BT_DBG("\t-Name\t:%s",
			       log_strdup(track_segments.segs[i].name));
			BT_DBG("\t-Position\t:%d", track_segments.segs[i].pos);
		}
		/* Reset buf in case the same object is read again without */
		/* calling select in between */
		net_buf_simple_reset(&otc_obj_buf);
		track_segments.cnt = 0;
	}

	return BT_OTC_CONTINUE;
}

int on_track_content(struct bt_conn *conn, uint32_t offset, uint32_t len,
		     uint8_t *data_p, bool is_complete,
		     struct bt_otc_instance_t *otc_inst)
{
	BT_INFO("Received Current Track content, %i bytes at offset %i",
		len, offset);

	BT_HEXDUMP_DBG(data_p, len, "Track content");

	return BT_OTC_CONTINUE;
}

int on_next_track_content(struct bt_conn *conn, uint32_t offset, uint32_t len,
			  uint8_t *data_p, bool is_complete,
			  struct bt_otc_instance_t *otc_inst)
{
	BT_INFO("Received Next Track content, %i bytes at offset %i",
		len, offset);

	BT_HEXDUMP_DBG(data_p, len, "Track content");

	return BT_OTC_CONTINUE;
}

int on_group_content(struct bt_conn *conn, uint32_t offset, uint32_t len,
		     uint8_t *data_p, bool is_complete,
		     struct bt_otc_instance_t *otc_inst)
{
	struct id_list_t group = {0};

	BT_INFO("Received Current Group content, %i bytes at offset %i",
		len, offset);

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		BT_DBG("Can not fit whole object");
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		BT_INFO("Current Group object received");
		decode_current_group(&otc_obj_buf, &group);
		for (int i = 0; i < group.cnt; i++) {
			char t[BT_OTS_OBJ_ID_STR_LEN];

			(void)bt_ots_obj_id_to_str(group.ids[i].id, t,
						   BT_OTS_OBJ_ID_STR_LEN);
			BT_DBG("Object type: %d, object  ID: %s",
			       group.ids[i].type, log_strdup(t));
		}
		/* Reset buf in case the same object is read again without */
		/* calling select in between */
		net_buf_simple_reset(&otc_obj_buf);
	}

	return BT_OTC_CONTINUE;
}

void on_object_metadata(struct bt_conn *conn, int err,
			struct bt_otc_instance_t *otc_inst,
			uint8_t metadata_read)
{
	BT_INFO("Object's meta data:");
	BT_INFO("\tCurrent size\t:%u", otc_inst->cur_object.current_size);

	if (otc_inst->cur_object.current_size > otc_obj_buf.size) {
		BT_DBG("Object larger than allocated buffer");
	}

	bt_otc_metadata_display(&otc_inst->cur_object, 1);

#if CONFIG_BT_DEBUG_MCC
	if (mcc_cb && mcc_cb->otc_obj_metadata) {
		mcc_cb->otc_obj_metadata(conn, err);
	}
#endif /* CONFIG_BT_DEBUG_MCC */
}

int bt_mcc_otc_read_object_metadata(struct bt_conn *conn)
{
	int err;

	err = bt_otc_obj_metadata_read(conn, &mcs_inst.otc[0],
				       BT_OTC_METADATA_REQ_ALL);
	if (err) {
		BT_DBG("Error reading the object: %d", err);
	}

	return err;
}


int bt_mcc_otc_read_icon_object(struct bt_conn *conn)
{
	int err;
	/* TODO: Add handling for busy - either MCS or OTS */

	mcs_inst.otc[0].cb->content_cb = on_icon_content;

	err = bt_otc_read(conn, &mcs_inst.otc[0]);
	if (err) {
		BT_DBG("Error reading the object: %d", err);
	}

	return err;
}

int bt_mcc_otc_read_track_segments_object(struct bt_conn *conn)
{
	int err;

	/* TODO: Add handling for busy - either MCS or OTS */

	/* TODO: Assumes object is already selected */
	mcs_inst.otc[0].cb->content_cb = on_track_segments_content;

	err = bt_otc_read(conn, &mcs_inst.otc[0]);
	if (err) {
		BT_DBG("Error reading the object: %d", err);
	}

	return err;
}

int bt_mcc_otc_read_current_track_object(struct bt_conn *conn)
{
	int err;

	/* TODO: Add handling for busy - either MCS or OTS */

	/* TODO: Assumes object is already selected */
	mcs_inst.otc[0].cb->content_cb = on_track_content;

	err = bt_otc_read(conn, &mcs_inst.otc[0]);
	if (err) {
		BT_DBG("Error reading the object: %d", err);
	}

	return err;
}

int bt_mcc_otc_read_next_track_object(struct bt_conn *conn)
{
	int err;

	/* TODO: Add handling for busy - either MCS or OTS */

	/* TODO: Assumes object is already selected */
	mcs_inst.otc[0].cb->content_cb = on_next_track_content;

	err = bt_otc_read(conn, &mcs_inst.otc[0]);
	if (err) {
		BT_DBG("Error reading the object: %d", err);
	}

	return err;
}

int bt_mcc_otc_read_current_group_object(struct bt_conn *conn)
{
	int err;

	/* TODO: Add handling for busy - either MCS or OTS */

	/* TODO: Assumes object is already selected */
	mcs_inst.otc[0].cb->content_cb = on_group_content;

	err = bt_otc_read(conn, &mcs_inst.otc[0]);
	if (err) {
		BT_DBG("Error reading the object: %d", err);
	}

	return err;
}

int bt_mcc_otc_read_parent_group_object(struct bt_conn *conn)
{
	int err;

	/* TODO: Add handling for busy - either MCS or OTS */

	/* TODO: Assumes object is already selected */

	/* Reuse callback for current group */
	mcs_inst.otc[0].cb->content_cb = on_group_content;

	err = bt_otc_read(conn, &mcs_inst.otc[0]);
	if (err) {
		BT_DBG("Error reading the object: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_MCC_SHELL)
struct bt_otc_instance_t *bt_mcc_otc_inst(uint8_t index)
{
	return &mcs_inst.otc[index];
}
#endif /* defined(CONFIG_BT_MCC_SHELL) */

#endif /* CONFIG_BT_MCC_MAX_OTS_INST > 0 */
#endif /* CONFIG_BT_OTC */
