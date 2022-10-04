/** @file
 *  @brief Bluetooth Media Control Client/Protocol implementation
 */

/*
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/mcc.h>

#include <zephyr/bluetooth/services/ots.h>
#include "../services/ots/ots_client_internal.h"

/* TODO: Temporarily copied here from media_proxy_internal.h - clean up */
/* Debug output of 48 bit Object ID value */
/* (Zephyr does not yet support debug output of more than 32 bit values.) */
/* Takes a text and a 64-bit integer as input */
#define BT_DBG_OBJ_ID(text, id64) \
	do { \
		if (IS_ENABLED(CONFIG_BT_DEBUG_MCS)) { \
			char t[BT_OTS_OBJ_ID_STR_LEN]; \
			(void)bt_ots_obj_id_to_str(id64, t, sizeof(t)); \
			BT_DBG(text "0x%s", t); \
		} \
	} while (0)


#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MCC)
#define LOG_MODULE_NAME bt_mcc
#include "common/log.h"

struct mcs_instance_t {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t player_name_handle;
#ifdef CONFIG_BT_MCC_OTS
	uint16_t icon_obj_id_handle;
#endif /* CONFIG_BT_MCC_OTS */
	uint16_t icon_url_handle;
	uint16_t track_changed_handle;
	uint16_t track_title_handle;
	uint16_t track_duration_handle;
	uint16_t track_position_handle;
	uint16_t playback_speed_handle;
	uint16_t seeking_speed_handle;
#ifdef CONFIG_BT_MCC_OTS
	uint16_t segments_obj_id_handle;
	uint16_t current_track_obj_id_handle;
	uint16_t next_track_obj_id_handle;
	uint16_t current_group_obj_id_handle;
	uint16_t parent_group_obj_id_handle;
#endif /* CONFIG_BT_MCC_OTS */
	uint16_t playing_order_handle;
	uint16_t playing_orders_supported_handle;
	uint16_t media_state_handle;
	uint16_t cp_handle;
	uint16_t opcodes_supported_handle;
#ifdef CONFIG_BT_MCC_OTS
	uint16_t scp_handle;
	uint16_t search_results_obj_id_handle;
#endif /* CONFIG_BT_MCC_OTS */
	uint16_t content_control_id_handle;

	struct bt_gatt_subscribe_params player_name_sub_params;
	struct bt_gatt_subscribe_params track_changed_sub_params;
	struct bt_gatt_subscribe_params track_title_sub_params;
	struct bt_gatt_subscribe_params track_duration_sub_params;
	struct bt_gatt_subscribe_params track_position_sub_params;
	struct bt_gatt_subscribe_params playback_speed_sub_params;
	struct bt_gatt_subscribe_params seeking_speed_sub_params;
#ifdef CONFIG_BT_MCC_OTS
	struct bt_gatt_subscribe_params current_track_obj_sub_params;
	struct bt_gatt_subscribe_params next_track_obj_sub_params;
	struct bt_gatt_subscribe_params parent_group_obj_sub_params;
	struct bt_gatt_subscribe_params current_group_obj_sub_params;
#endif /* CONFIG_BT_MCC_OTS */
	struct bt_gatt_subscribe_params playing_order_sub_params;
	struct bt_gatt_subscribe_params media_state_sub_params;
	struct bt_gatt_subscribe_params cp_sub_params;
	struct bt_gatt_subscribe_params opcodes_supported_sub_params;
#ifdef CONFIG_BT_MCC_OTS
	struct bt_gatt_subscribe_params scp_sub_params;
	struct bt_gatt_subscribe_params search_results_obj_sub_params;
#endif /* CONFIG_BT_MCC_OTS */

	/* The write buffer is used for
	 * - track position    (4 octets)
	 * - playback speed    (1 octet)
	 * - playing order     (1 octet)
	 * - the control point (5 octets)
	 *                     (1 octet opcode + optionally 4 octet param)
	 *                     (mpl_cmd.opcode + mpl_cmd.param)
	 * If the object transfer client is included, it is also used for
	 * - object IDs (6 octets - BT_OTS_OBJ_ID_SIZE) and
	 * - the search control point (64 octets - SEARCH_LEN_MAX)
	 *
	 * If there is no OTC, the largest is control point
	 * If OTC is included, the largest is the search control point
	 */
#ifdef CONFIG_BT_MCC_OTS
	char write_buf[SEARCH_LEN_MAX];
#else
	/* Trick to be able to use sizeof on members of a struct type */
	/* TODO: Rewrite the mpl_cmd to have the "use_param" parameter */
	/* separately, and the opcode and param alone as a struct */
	char write_buf[sizeof(((struct mpl_cmd *)0)->opcode) +
		       sizeof(((struct mpl_cmd *)0)->param)];
#endif /* CONFIG_BT_MCC_OTS */

	struct bt_gatt_write_params     write_params;
	bool busy;

#ifdef CONFIG_BT_MCC_OTS
	struct bt_ots_client otc;
#endif /* CONFIG_BT_MCC_OTS */
};

static struct bt_gatt_discover_params  discover_params;
static struct bt_gatt_read_params      read_params;

static struct mcs_instance_t *cur_mcs_inst;

static struct mcs_instance_t mcs_inst;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

static struct bt_mcc_cb *mcc_cb;
static bool subscribe_all;

#ifdef CONFIG_BT_MCC_OTS
NET_BUF_SIMPLE_DEFINE_STATIC(otc_obj_buf, CONFIG_BT_MCC_OTC_OBJ_BUF_SIZE);
static struct bt_ots_client_cb otc_cb;
#endif /* CONFIG_BT_MCC_OTS */



#ifdef CONFIG_BT_MCC_OTS
void on_obj_selected(struct bt_ots_client *otc_inst,
		     struct bt_conn *conn, int err);

void on_object_metadata(struct bt_ots_client *otc_inst,
			struct bt_conn *conn, int err,
			uint8_t metadata_read);

int on_icon_content(struct bt_ots_client *otc_inst,
		    struct bt_conn *conn, uint32_t offset,
		    uint32_t len, uint8_t *data_p, bool is_complete);
#endif /* CONFIG_BT_MCC_OTS */


static uint8_t mcc_read_player_name_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	int cb_err = err;
	char name[CONFIG_BT_MCC_MEDIA_PLAYER_NAME_MAX];

	cur_mcs_inst->busy = false;
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

		(void)memcpy(&name, data, length);
		name[length] = '\0';
		BT_DBG("Player name: %s", name);
	}

	if (mcc_cb && mcc_cb->read_player_name) {
		mcc_cb->read_player_name(conn, cb_err, name);
	}

	return BT_GATT_ITER_STOP;
}


#ifdef CONFIG_BT_MCC_OTS
static uint8_t mcc_read_icon_obj_id_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->read_icon_obj_id) {
		mcc_cb->read_icon_obj_id(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* CONFIG_BT_MCC_OTS */

static uint8_t mcc_read_icon_url_cb(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params,
				    const void *data, uint16_t length)
{
	int cb_err = err;
	char url[CONFIG_BT_MCC_ICON_URL_MAX];

	cur_mcs_inst->busy = false;
	BT_DBG("err: 0x%02x, length: %d, data: %p", err, length, data);
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!data) {
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (length >= sizeof(url)) {
		cb_err = BT_ATT_ERR_INSUFFICIENT_RESOURCES;
	} else {
		BT_HEXDUMP_DBG(data, length, "Icon URL");
		(void)memcpy(&url, data, length);
		url[length] = '\0';
		BT_DBG("Icon URL: %s", url);
	}

	if (mcc_cb && mcc_cb->read_icon_url) {
		mcc_cb->read_icon_url(conn, cb_err, url);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_track_title_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	int cb_err = err;
	char title[CONFIG_BT_MCC_TRACK_TITLE_MAX];

	cur_mcs_inst->busy = false;
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
		(void)memcpy(&title, data, length);
		title[length] = '\0';
		BT_DBG("Track title: %s", title);
	}

	if (mcc_cb && mcc_cb->read_track_title) {
		mcc_cb->read_track_title(conn, cb_err, title);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_track_duration_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	int cb_err = err;
	int32_t dur = 0;

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->read_track_duration) {
		mcc_cb->read_track_duration(conn, cb_err, dur);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_track_position_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	int cb_err = err;
	int32_t pos = 0;

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->read_track_position) {
		mcc_cb->read_track_position(conn, cb_err, pos);
	}

	return BT_GATT_ITER_STOP;
}

static void mcs_write_track_position_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_write_params *params)
{
	int cb_err = err;
	int32_t pos = 0;

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->set_track_position) {
		mcc_cb->set_track_position(conn, cb_err, pos);
	}
}

static uint8_t mcc_read_playback_speed_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	int cb_err = err;
	int8_t speed = 0;

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->read_playback_speed) {
		mcc_cb->read_playback_speed(conn, cb_err, speed);
	}

	return BT_GATT_ITER_STOP;
}

static void mcs_write_playback_speed_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_write_params *params)
{
	int cb_err = err;
	int8_t speed = 0;

	cur_mcs_inst->busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != sizeof(speed)) {
		BT_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		speed = *(int8_t *)params->data;
		BT_DBG("Playback_speed: %d", speed);
	}

	if (mcc_cb && mcc_cb->set_playback_speed) {
		mcc_cb->set_playback_speed(conn, cb_err, speed);
	}
}

static uint8_t mcc_read_seeking_speed_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_read_params *params,
					 const void *data, uint16_t length)
{
	int cb_err = err;
	int8_t speed = 0;

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->read_seeking_speed) {
		mcc_cb->read_seeking_speed(conn, cb_err, speed);
	}

	return BT_GATT_ITER_STOP;
}

#ifdef CONFIG_BT_MCC_OTS
static uint8_t mcc_read_segments_obj_id_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_read_params *params,
					   const void *data, uint16_t length)
{
	int cb_err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	cur_mcs_inst->busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
		cb_err = err;
	} else if ((!pid) || (length != BT_OTS_OBJ_ID_SIZE)) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		BT_HEXDUMP_DBG(pid, length, "Segments Object ID");
		id = sys_get_le48(pid);
		BT_DBG_OBJ_ID("Segments Object ID: ", id);
		cb_err = 0;
	}

	if (mcc_cb && mcc_cb->read_segments_obj_id) {
		mcc_cb->read_segments_obj_id(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_current_track_obj_id_cb(struct bt_conn *conn, uint8_t err,
						struct bt_gatt_read_params *params,
						const void *data, uint16_t length)
{
	int cb_err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	cur_mcs_inst->busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
		cb_err = err;
	} else if ((!pid) || (length != BT_OTS_OBJ_ID_SIZE)) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		BT_HEXDUMP_DBG(pid, length, "Current Track Object ID");
		id = sys_get_le48(pid);
		BT_DBG_OBJ_ID("Current Track Object ID: ", id);
		cb_err = 0;
	}

	if (mcc_cb && mcc_cb->read_current_track_obj_id) {
		mcc_cb->read_current_track_obj_id(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}

static void mcs_write_current_track_obj_id_cb(struct bt_conn *conn, uint8_t err,
					      struct bt_gatt_write_params *params)
{
	int cb_err = err;
	uint64_t obj_id = 0;

	cur_mcs_inst->busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != BT_OTS_OBJ_ID_SIZE) {
		BT_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		obj_id = sys_get_le48((const uint8_t *)params->data);
		BT_DBG_OBJ_ID("Object ID: ", obj_id);
	}

	if (mcc_cb && mcc_cb->set_current_track_obj_id) {
		mcc_cb->set_current_track_obj_id(conn, cb_err, obj_id);
	}
}

static uint8_t mcc_read_next_track_obj_id_cb(struct bt_conn *conn, uint8_t err,
					     struct bt_gatt_read_params *params,
					     const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->read_next_track_obj_id) {
		mcc_cb->read_next_track_obj_id(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}

static void mcs_write_next_track_obj_id_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_write_params *params)
{
	int cb_err = err;
	uint64_t obj_id = 0;

	cur_mcs_inst->busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != BT_OTS_OBJ_ID_SIZE) {
		BT_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		obj_id = sys_get_le48((const uint8_t *)params->data);
		BT_DBG_OBJ_ID("Object ID: ", obj_id);
	}

	if (mcc_cb && mcc_cb->set_next_track_obj_id) {
		mcc_cb->set_next_track_obj_id(conn, cb_err, obj_id);
	}
}

static uint8_t mcc_read_parent_group_obj_id_cb(struct bt_conn *conn, uint8_t err,
					       struct bt_gatt_read_params *params,
					       const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->read_parent_group_obj_id) {
		mcc_cb->read_parent_group_obj_id(conn, cb_err, id);
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

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->read_current_group_obj_id) {
		mcc_cb->read_current_group_obj_id(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}

static void mcs_write_current_group_obj_id_cb(struct bt_conn *conn, uint8_t err,
					      struct bt_gatt_write_params *params)
{
	int cb_err = err;
	uint64_t obj_id = 0;

	cur_mcs_inst->busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != BT_OTS_OBJ_ID_SIZE) {
		BT_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		obj_id = sys_get_le48((const uint8_t *)params->data);
		BT_DBG_OBJ_ID("Object ID: ", obj_id);
	}

	if (mcc_cb && mcc_cb->set_current_group_obj_id) {
		mcc_cb->set_current_group_obj_id(conn, cb_err, obj_id);
	}
}
#endif /* CONFIG_BT_MCC_OTS */

static uint8_t mcc_read_playing_order_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_read_params *params,
					 const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t order = 0;

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->read_playing_order) {
		mcc_cb->read_playing_order(conn, cb_err, order);
	}

	return BT_GATT_ITER_STOP;
}

static void mcs_write_playing_order_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_write_params *params)
{
	int cb_err = err;
	uint8_t order = 0;

	cur_mcs_inst->busy = false;
	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != sizeof(order)) {
		BT_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		order = *(uint8_t *)params->data;
		BT_DBG("Playing order: %d", *(uint8_t *)params->data);
	}

	if (mcc_cb && mcc_cb->set_playing_order) {
		mcc_cb->set_playing_order(conn, cb_err, order);
	}
}

static uint8_t mcc_read_playing_orders_supported_cb(struct bt_conn *conn, uint8_t err,
						    struct bt_gatt_read_params *params,
						    const void *data, uint16_t length)
{
	int cb_err = err;
	uint16_t orders = 0;

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->read_playing_orders_supported) {
		mcc_cb->read_playing_orders_supported(conn, cb_err, orders);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcc_read_media_state_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t state = 0;

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->read_media_state) {
		mcc_cb->read_media_state(conn, cb_err, state);
	}

	return BT_GATT_ITER_STOP;
}

static void mcs_write_cp_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_write_params *params)
{
	int cb_err = err;
	struct mpl_cmd cmd = {0};

	cur_mcs_inst->busy = false;

	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!params->data ||
		   (params->length != sizeof(cmd.opcode) &&
		    params->length != sizeof(cmd.opcode) + sizeof(cmd.param))) {
		/* Above: No data pointer, or length not equal to any of the */
		/* two possible values */
		BT_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		(void)memcpy(&cmd.opcode, params->data, sizeof(cmd.opcode));
		if (params->length == sizeof(cmd.opcode) + sizeof(cmd.param)) {
			(void)memcpy(&cmd.param,
			       (char *)(params->data) + sizeof(cmd.opcode),
			       sizeof(cmd.param));
			cmd.use_param = true;
			BT_DBG("Command in callback: %d, param: %d", cmd.opcode, cmd.param);
		}
	}

	if (mcc_cb && mcc_cb->send_cmd) {
		mcc_cb->send_cmd(conn, cb_err, &cmd);
	}
}

static uint8_t mcc_read_opcodes_supported_cb(struct bt_conn *conn, uint8_t err,
					     struct bt_gatt_read_params *params,
					     const void *data, uint16_t length)
{
	int cb_err = err;
	int32_t operations = 0;

	cur_mcs_inst->busy = false;

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

	if (mcc_cb && mcc_cb->read_opcodes_supported) {
		mcc_cb->read_opcodes_supported(conn, cb_err, operations);
	}

	return BT_GATT_ITER_STOP;
}

#ifdef CONFIG_BT_MCC_OTS
static void mcs_write_scp_cb(struct bt_conn *conn, uint8_t err,
			     struct bt_gatt_write_params *params)
{
	int cb_err = err;
	struct mpl_search search = {0};

	cur_mcs_inst->busy = false;

	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if (!params->data ||
		   (params->length > SEARCH_LEN_MAX)) {
		BT_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		search.len = params->length;
		(void)memcpy(search.search, params->data, params->length);
		BT_DBG("Length of returned value in callback: %d", search.len);
	}

	if (mcc_cb && mcc_cb->send_search) {
		mcc_cb->send_search(conn, cb_err, &search);
	}
}

static uint8_t mcc_read_search_results_obj_id_cb(struct bt_conn *conn, uint8_t err,
						 struct bt_gatt_read_params *params,
						 const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	cur_mcs_inst->busy = false;
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

	if (mcc_cb && mcc_cb->read_search_results_obj_id) {
		mcc_cb->read_search_results_obj_id(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* CONFIG_BT_MCC_OTS */

static uint8_t mcc_read_content_control_id_cb(struct bt_conn *conn, uint8_t err,
					      struct bt_gatt_read_params *params,
					      const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t ccid = 0;

	cur_mcs_inst->busy = false;

	if (err) {
		BT_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(ccid))) {
		BT_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	} else {
		ccid = *(uint8_t *)data;
		BT_DBG("Content control ID: %d", ccid);
	}

	if (mcc_cb && mcc_cb->read_content_control_id) {
		mcc_cb->read_content_control_id(conn, cb_err, ccid);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t mcs_notify_handler(struct bt_conn *conn,
				  struct bt_gatt_subscribe_params *params,
				  const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;

	if (conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	BT_DBG("Notification, handle: %d", handle);

	if (data) {
		if (handle == cur_mcs_inst->player_name_handle) {
			BT_DBG("Player Name notification");
			mcc_read_player_name_cb(conn, 0, NULL, data, length);

		} else if (handle == cur_mcs_inst->track_changed_handle) {
			/* The Track Changed characteristic can only be */
			/* notified, so that is handled directly here */

			int cb_err = 0;

			BT_DBG("Track Changed notification");
			BT_DBG("data: %p, length: %u", data, length);

			if (length != 0) {
				BT_DBG("Non-zero length: %u", length);
				cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
			}

			if (mcc_cb && mcc_cb->track_changed_ntf) {
				mcc_cb->track_changed_ntf(conn, cb_err);
			}

		} else if (handle == cur_mcs_inst->track_title_handle) {
			BT_DBG("Track Title notification");
			mcc_read_track_title_cb(conn, 0, NULL, data, length);

		} else if (handle == cur_mcs_inst->track_duration_handle) {
			BT_DBG("Track Duration notification");
			mcc_read_track_duration_cb(conn, 0, NULL, data, length);

		} else if (handle == cur_mcs_inst->track_position_handle) {
			BT_DBG("Track Position notification");
			mcc_read_track_position_cb(conn, 0, NULL, data, length);

		} else if (handle == cur_mcs_inst->playback_speed_handle) {
			BT_DBG("Playback Speed notification");
			mcc_read_playback_speed_cb(conn, 0, NULL, data, length);

		} else if (handle == cur_mcs_inst->seeking_speed_handle) {
			BT_DBG("Seeking Speed notification");
			mcc_read_seeking_speed_cb(conn, 0, NULL, data, length);

#ifdef CONFIG_BT_MCC_OTS
		} else if (handle == cur_mcs_inst->current_track_obj_id_handle) {
			BT_DBG("Current Track notification");
			mcc_read_current_track_obj_id_cb(conn, 0, NULL, data,
							 length);

		} else if (handle == cur_mcs_inst->next_track_obj_id_handle) {
			BT_DBG("Next Track notification");
			mcc_read_next_track_obj_id_cb(conn, 0, NULL, data,
						      length);

		} else if (handle == cur_mcs_inst->parent_group_obj_id_handle) {
			BT_DBG("Parent Group notification");
			mcc_read_parent_group_obj_id_cb(conn, 0, NULL, data,
							 length);

		} else if (handle == cur_mcs_inst->current_group_obj_id_handle) {
			BT_DBG("Current Group notification");
			mcc_read_current_group_obj_id_cb(conn, 0, NULL, data,
							 length);
#endif /* CONFIG_BT_MCC_OTS */

		} else if (handle == cur_mcs_inst->playing_order_handle) {
			BT_DBG("Playing Order notification");
			mcc_read_playing_order_cb(conn, 0, NULL, data, length);

		} else if (handle == cur_mcs_inst->media_state_handle) {
			BT_DBG("Media State notification");
			mcc_read_media_state_cb(conn, 0, NULL, data, length);

		} else if (handle == cur_mcs_inst->cp_handle) {
			/* The control point is is a special case - only */
			/* writable and notifiable.  Handle directly here. */

			int cb_err = 0;
			struct mpl_cmd_ntf ntf = {0};

			BT_DBG("Control Point notification");
			if (length == sizeof(ntf.requested_opcode) + sizeof(ntf.result_code)) {
				ntf.requested_opcode = *(uint8_t *)data;
				ntf.result_code = *((uint8_t *)data +
						    sizeof(ntf.requested_opcode));
				BT_DBG("Command: %d, result: %d",
				       ntf.requested_opcode, ntf.result_code);
			} else {
				BT_DBG("length: %d, data: %p", length, data);
				cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
			}

			if (mcc_cb && mcc_cb->cmd_ntf) {
				mcc_cb->cmd_ntf(conn, cb_err, &ntf);
			}

		} else if (handle == cur_mcs_inst->opcodes_supported_handle) {
			BT_DBG("Opcodes Supported notification");
			mcc_read_opcodes_supported_cb(conn, 0, NULL, data,
						      length);

#ifdef CONFIG_BT_MCC_OTS
		} else if (handle == cur_mcs_inst->scp_handle) {
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

			if (mcc_cb && mcc_cb->search_ntf) {
				mcc_cb->search_ntf(conn, cb_err, result_code);
			}

		} else if (handle == cur_mcs_inst->search_results_obj_id_handle) {
			BT_DBG("Search Results notification");
			mcc_read_search_results_obj_id_cb(conn, 0, NULL, data,
							  length);
#endif /* CONFIG_BT_MCC_OTS */

		} else {
			BT_DBG("Unknown handle: %d (0x%04X)", handle, handle);
		}
	}
	return BT_GATT_ITER_CONTINUE;
}

/* Called when discovery is completed - successfully or with error */
static void discovery_complete(struct bt_conn *conn, int err)
{
	BT_DBG("Discovery completed, err: %d", err);

	/* TODO: Handle resets of instance, and re-discovery.
	 * For now, reset instance on error.
	 */
	if (err) {
		cur_mcs_inst = NULL;
	}

	if (mcc_cb && mcc_cb->discover_mcs) {
		mcc_cb->discover_mcs(conn, err);
	}
}

#ifdef CONFIG_BT_MCC_OTS
static uint8_t discover_otc_char_func(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	int err = 0;
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
		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_FEATURE)) {
			BT_DBG("OTS Features");
			cur_mcs_inst->otc.feature_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_NAME)) {
			BT_DBG("Object Name");
			cur_mcs_inst->otc.obj_name_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_TYPE)) {
			BT_DBG("Object Type");
			cur_mcs_inst->otc.obj_type_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_SIZE)) {
			BT_DBG("Object Size");
			cur_mcs_inst->otc.obj_size_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_ID)) {
			BT_DBG("Object ID");
			cur_mcs_inst->otc.obj_id_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_PROPERTIES)) {
			BT_DBG("Object properties %d", chrc->value_handle);
			cur_mcs_inst->otc.obj_properties_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_ACTION_CP)) {
			BT_DBG("Object Action Control Point");
			cur_mcs_inst->otc.oacp_handle = chrc->value_handle;
			sub_params = &cur_mcs_inst->otc.oacp_sub_params;
			sub_params->disc_params = &cur_mcs_inst->otc.oacp_sub_disc_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_LIST_CP)) {
			BT_DBG("Object List Control Point");
			cur_mcs_inst->otc.olcp_handle = chrc->value_handle;
			sub_params = &cur_mcs_inst->otc.olcp_sub_params;
			sub_params->disc_params = &cur_mcs_inst->otc.olcp_sub_disc_params;
		}

		if (sub_params) {
			/* With ccc_handle == 0 it will use auto discovery */
			sub_params->ccc_handle = 0;
			sub_params->end_handle = cur_mcs_inst->otc.end_handle;
			sub_params->value = BT_GATT_CCC_INDICATE;
			sub_params->value_handle = chrc->value_handle;
			sub_params->notify = bt_ots_client_indicate_handler;

			bt_gatt_subscribe(conn, sub_params);
		}

		return BT_GATT_ITER_CONTINUE;
	}

	/* No more attributes found */
	cur_mcs_inst->otc.cb = &otc_cb;
	bt_ots_client_register(&cur_mcs_inst->otc);

	BT_DBG("Setup complete for included OTS");
	(void)memset(params, 0, sizeof(*params));

	discovery_complete(conn, err);

	return BT_GATT_ITER_STOP;
}
#endif /* CONFIG_BT_MCC_OTS */

#ifdef CONFIG_BT_MCC_OTS
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

		/* We have the included OTS service (MCS includes only one) */
		BT_DBG("Discover include complete for GMCS: OTS");
		cur_mcs_inst->otc.start_handle = include->start_handle;
		cur_mcs_inst->otc.end_handle = include->end_handle;
		(void)memset(params, 0, sizeof(*params));

		/* Discover characteristics of the included OTS */
		discover_params.start_handle = cur_mcs_inst->otc.start_handle;
		discover_params.end_handle = cur_mcs_inst->otc.end_handle;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		discover_params.func = discover_otc_char_func;

		BT_DBG("Start discovery of OTS characteristics");
		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			BT_DBG("Discovery of OTS chars. failed");
			discovery_complete(conn, err);
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("No included OTS found");
	/* This is OK, the server may not support OTS. But in that case,
	 *  discovery stops here.
	 */
	discovery_complete(conn, err);
	return BT_GATT_ITER_STOP;
}

/* Start discovery of included services */
static void discover_included(struct bt_conn *conn)
{
	int err;

	discover_params.start_handle = cur_mcs_inst->start_handle;
	discover_params.end_handle = cur_mcs_inst->end_handle;
	discover_params.type = BT_GATT_DISCOVER_INCLUDE;
	discover_params.func = discover_include_func;

	BT_DBG("Start discovery of included services");
	err = bt_gatt_discover(conn, &discover_params);
	if (err) {
		BT_DBG("Discovery of included service failed: %d", err);
		discovery_complete(conn, err);
	}
}
#endif /* CONFIG_BT_MCC_OTS */

static bool subscribe_next_mcs_char(struct bt_conn *conn);

/* This function will subscribe to GMCS CCCDs.
 * After this, the function will start discovery of included services.
 */
static void subscribe_mcs_char_func(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_subscribe_params *params)
{
	bool subscription_done;

	if (err) {
		BT_DBG("Subscription callback error: %u", err);
		discovery_complete(conn, err);
		return;
	}

	BT_DBG("Subscribed: value handle: %d, ccc handle: %d",
	       params->value_handle, params->ccc_handle);

	/* Subscribe to next characteristic */
	subscription_done = subscribe_next_mcs_char(conn);

	if (subscription_done) {
#ifdef CONFIG_BT_MCC_OTS
		/* Start discovery of included services to find OTS */
		discover_included(conn);
#else
		/* If OTS is not configured, discovery ends here */
		discovery_complete(conn, 0);
#endif /* CONFIG_BT_MCC_OTS */
	}
}

/* Subscribe to a characteristic - helper function */
static int do_subscribe(struct bt_conn *conn, uint16_t handle,
			 struct bt_gatt_subscribe_params *sub_params)
{
	/* With ccc_handle == 0 it will use auto discovery */
	sub_params->ccc_handle = 0;
	sub_params->end_handle = cur_mcs_inst->end_handle;
	sub_params->value = BT_GATT_CCC_NOTIFY;
	sub_params->value_handle = handle;
	sub_params->notify = mcs_notify_handler;
	sub_params->subscribe = subscribe_mcs_char_func;
	/* disc_params pointer is also used as subscription flag */
	sub_params->disc_params = &discover_params;

	BT_DBG("Subscring to handle %d", handle);
	return bt_gatt_subscribe(conn, sub_params);
}

/* Subscribe to the next GMCS CCCD.
 * @return true if there are no more characteristics to subscribe to
 */
static bool subscribe_next_mcs_char(struct bt_conn *conn)
{
	struct bt_gatt_subscribe_params *sub_params = NULL;
	int err = 0;

	/* The characteristics may be in any order on the server, and
	 * not all of them may exist => need to check all.
	 * For each of the subscribable characteristics
	 * - check if we have a handle for it
	 * - check sub_params.disc_params pointer to see if we have
	 *   already subscribed to it (set in do_subscribe() ).
	 */

	if (cur_mcs_inst->player_name_handle &&
	    cur_mcs_inst->player_name_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->player_name_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->player_name_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

	if (cur_mcs_inst->track_changed_handle &&
	    cur_mcs_inst->track_changed_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->track_changed_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->track_changed_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}
	if (cur_mcs_inst->track_title_handle &&
	    cur_mcs_inst->track_title_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->track_title_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->track_title_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

	if (cur_mcs_inst->track_duration_handle &&
	    cur_mcs_inst->track_duration_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->track_duration_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->track_duration_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

	if (cur_mcs_inst->track_position_handle &&
	    cur_mcs_inst->track_position_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->track_position_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->track_position_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

	if (cur_mcs_inst->playback_speed_handle &&
	    cur_mcs_inst->playback_speed_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->playback_speed_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->playback_speed_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

	if (cur_mcs_inst->seeking_speed_handle &&
	    cur_mcs_inst->seeking_speed_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->seeking_speed_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->seeking_speed_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

#ifdef CONFIG_BT_MCC_OTS
	if (cur_mcs_inst->current_track_obj_id_handle &&
	    cur_mcs_inst->current_track_obj_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->current_track_obj_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->current_track_obj_id_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

	if (cur_mcs_inst->next_track_obj_id_handle &&
	    cur_mcs_inst->next_track_obj_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->next_track_obj_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->next_track_obj_id_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

	if (cur_mcs_inst->parent_group_obj_id_handle &&
	    cur_mcs_inst->parent_group_obj_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->parent_group_obj_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->parent_group_obj_id_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

	if (cur_mcs_inst->current_group_obj_id_handle &&
	    cur_mcs_inst->parent_group_obj_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->current_group_obj_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->current_group_obj_id_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

#endif /* CONFIG_BT_MCC_OTS */

	if (cur_mcs_inst->playing_order_handle &&
	    cur_mcs_inst->playing_order_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->playing_order_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->playing_order_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

	if (cur_mcs_inst->media_state_handle &&
	    cur_mcs_inst->media_state_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->media_state_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->media_state_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

	if (cur_mcs_inst->cp_handle &&
	    cur_mcs_inst->cp_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->cp_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->cp_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

	if (cur_mcs_inst->opcodes_supported_handle &&
	    cur_mcs_inst->opcodes_supported_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->opcodes_supported_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->opcodes_supported_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

#ifdef CONFIG_BT_MCC_OTS
	if (cur_mcs_inst->scp_handle &&
	    cur_mcs_inst->scp_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->scp_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->scp_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}

	if (cur_mcs_inst->search_results_obj_id_handle &&
	    cur_mcs_inst->search_results_obj_sub_params.disc_params == NULL) {
		sub_params = &cur_mcs_inst->search_results_obj_sub_params;
		err = do_subscribe(conn, cur_mcs_inst->search_results_obj_id_handle, sub_params);
		if (err) {
			BT_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}
		return false;
	}
#endif /* CONFIG_BT_MCC_OTS */

	/* If we have come here, there are no more characteristics to
	 * subscribe to, and we are done.
	 */
	return true;
}

/* This function is called when characteristics are found.
 * The function will store handles to GMCS characteristics.
 * After this, the function will start subscription to characteristics
 */
static uint8_t discover_mcs_char_func(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	struct bt_gatt_chrc *chrc;
	bool subscription_done = true;

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
			cur_mcs_inst->player_name_handle = chrc->value_handle;
			/* Use discovery params pointer as subscription flag */
			cur_mcs_inst->player_name_sub_params.disc_params = NULL;
#ifdef CONFIG_BT_MCC_OTS
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_ICON_OBJ_ID)) {
			BT_DBG("Icon Object, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->icon_obj_id_handle = chrc->value_handle;
#endif /* CONFIG_BT_MCC_OTS */
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_ICON_URL)) {
			BT_DBG("Icon URL, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->icon_url_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_CHANGED)) {
			BT_DBG("Track Changed, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->track_changed_handle = chrc->value_handle;
			cur_mcs_inst->track_changed_sub_params.disc_params = NULL;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_TITLE)) {
			BT_DBG("Track Title, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->track_title_handle = chrc->value_handle;
			cur_mcs_inst->track_title_sub_params.disc_params = NULL;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_DURATION)) {
			BT_DBG("Track Duration, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->track_duration_handle = chrc->value_handle;
			cur_mcs_inst->track_duration_sub_params.disc_params = NULL;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_POSITION)) {
			BT_DBG("Track Position, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->track_position_handle = chrc->value_handle;
			cur_mcs_inst->track_position_sub_params.disc_params = NULL;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PLAYBACK_SPEED)) {
			BT_DBG("Playback Speed, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->playback_speed_handle = chrc->value_handle;
			cur_mcs_inst->playback_speed_sub_params.disc_params = NULL;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_SEEKING_SPEED)) {
			BT_DBG("Seeking Speed, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->seeking_speed_handle = chrc->value_handle;
			cur_mcs_inst->seeking_speed_sub_params.disc_params = NULL;
#ifdef CONFIG_BT_MCC_OTS
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_SEGMENTS_OBJ_ID)) {
			BT_DBG("Track Segments Object, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->segments_obj_id_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_CURRENT_TRACK_OBJ_ID)) {
			BT_DBG("Current Track Object, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->current_track_obj_id_handle = chrc->value_handle;
			cur_mcs_inst->current_track_obj_sub_params.disc_params = NULL;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_NEXT_TRACK_OBJ_ID)) {
			BT_DBG("Next Track Object, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->next_track_obj_id_handle = chrc->value_handle;
			cur_mcs_inst->next_track_obj_sub_params.disc_params = NULL;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PARENT_GROUP_OBJ_ID)) {
			BT_DBG("Parent Group Object, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->parent_group_obj_id_handle = chrc->value_handle;
			cur_mcs_inst->parent_group_obj_sub_params.disc_params = NULL;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_CURRENT_GROUP_OBJ_ID)) {
			BT_DBG("Group Object, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->current_group_obj_id_handle = chrc->value_handle;
			cur_mcs_inst->current_group_obj_sub_params.disc_params = NULL;
#endif /* CONFIG_BT_MCC_OTS */
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PLAYING_ORDER)) {
			BT_DBG("Playing Order, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->playing_order_handle = chrc->value_handle;
			cur_mcs_inst->playing_order_sub_params.disc_params = NULL;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PLAYING_ORDERS)) {
			BT_DBG("Playing Orders supported, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->playing_orders_supported_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_MEDIA_STATE)) {
			BT_DBG("Media State, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->media_state_handle = chrc->value_handle;
			cur_mcs_inst->media_state_sub_params.disc_params = NULL;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_MEDIA_CONTROL_POINT)) {
			BT_DBG("Media Control Point, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->cp_handle = chrc->value_handle;
			cur_mcs_inst->cp_sub_params.disc_params = NULL;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_MEDIA_CONTROL_OPCODES)) {
			BT_DBG("Media control opcodes supported, UUID: %s",
			       bt_uuid_str(chrc->uuid));
			cur_mcs_inst->opcodes_supported_handle = chrc->value_handle;
			cur_mcs_inst->opcodes_supported_sub_params.disc_params = NULL;
#ifdef CONFIG_BT_MCC_OTS
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_SEARCH_CONTROL_POINT)) {
			BT_DBG("Search control point, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->scp_handle = chrc->value_handle;
			cur_mcs_inst->scp_sub_params.disc_params = NULL;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID)) {
			BT_DBG("Search Results object, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->search_results_obj_id_handle = chrc->value_handle;
			cur_mcs_inst->search_results_obj_sub_params.disc_params = NULL;
#endif /* CONFIG_BT_MCC_OTS */
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_CCID)) {
			BT_DBG("Content Control ID, UUID: %s", bt_uuid_str(chrc->uuid));
			cur_mcs_inst->content_control_id_handle = chrc->value_handle;
		}


		/* Continue to search for more attributes */
		return BT_GATT_ITER_CONTINUE;
	}

	/* No more attributes found */
	BT_DBG("GMCS characteristics found");
	(void)memset(params, 0, sizeof(*params));

	/* Either subscribe to characteristics, or continue to discovery of
	 *included services.
	 * Subscription is done after discovery, not in parallel with it,
	 * to avoid queuing many ATT requests that requires buffers.
	 */
	if (subscribe_all) {
		subscription_done = subscribe_next_mcs_char(conn);
	}

	if (subscription_done) {
		/* Not subscribing, or there was nothing to subscribe to */
#ifdef CONFIG_BT_MCC_OTS
		/* Start discovery of included services to find OTS */
		discover_included(conn);
#else
		/* If OTS is not configured, discovery ends here */
		discovery_complete(conn, 0);
#endif /* CONFIG_BT_MCC_OTS */
	}

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
	struct bt_gatt_service_val *prim_service;

	if (attr) {
		int err;
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
		cur_mcs_inst->start_handle = attr->handle + 1;
		cur_mcs_inst->end_handle = prim_service->end_handle;

		/* Start discovery of characteristics */
		discover_params.uuid = NULL;
		discover_params.start_handle = cur_mcs_inst->start_handle;
		discover_params.end_handle = cur_mcs_inst->end_handle;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		discover_params.func = discover_mcs_char_func;

		BT_DBG("Start discovery of GMCS characteristics");
		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			BT_DBG("Discovery failed: %d", err);
			discovery_complete(conn, err);
		}
		return BT_GATT_ITER_STOP;
	}

	/* No attribute of the searched for type found */
	BT_DBG("Could not find an GMCS instance on the server");
	cur_mcs_inst = NULL;
	discovery_complete(conn, -ENODATA);
	return BT_GATT_ITER_STOP;
}


int bt_mcc_init(struct bt_mcc_cb *cb)
{
	mcc_cb = cb;

#ifdef CONFIG_BT_MCC_OTS
	/* Set up the callbacks from OTC */
	/* TODO: Have one single content callback. */
	/* For now: Use the icon callback for content - it is the first, */
	/* and this will anyway be reset later. */
	otc_cb.obj_data_read     = on_icon_content;
	otc_cb.obj_selected      = on_obj_selected;
	otc_cb.obj_metadata_read = on_object_metadata;

	BT_DBG("Object selected callback: %p", otc_cb.obj_selected);
	BT_DBG("Object content callback: %p", otc_cb.obj_data_read);
	BT_DBG("Object metadata callback: %p", otc_cb.obj_metadata_read);
#endif /* CONFIG_BT_MCC_OTS */

	return 0;
}


/* Initiate discovery.
 * Discovery is handled by a chain of functions, where each function does its
 * part, and then initiates a further discovery, with a new callback function.
 *
 * The order of discovery is follows:
 * 1: Discover GMCS primary service (started here)
 * 2: Discover characteristics of GMCS
 * 3: Subscribe to characteristics of GMCS
 * 4: Discover OTS service included in GMCS
 * 5: Discover characteristics of OTS and subscribe to them
 */
int bt_mcc_discover_mcs(struct bt_conn *conn, bool subscribe)
{
	CHECKIF(!conn) {
		return -EINVAL;
	} else if (cur_mcs_inst) {
		return -EBUSY;
	}

	subscribe_all = subscribe;
	memset(&discover_params, 0, sizeof(discover_params));
	memset(&mcs_inst, 0, sizeof(mcs_inst));
	(void)memcpy(&uuid, BT_UUID_GMCS, sizeof(uuid));

	discover_params.func = discover_primary_func;
	discover_params.uuid = &uuid.uuid;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	BT_DBG("start discovery of GMCS primary service");
	return bt_gatt_discover(conn, &discover_params);
}


int bt_mcc_read_player_name(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->player_name_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_player_name_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->player_name_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}


#ifdef CONFIG_BT_MCC_OTS
int bt_mcc_read_icon_obj_id(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->icon_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_icon_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->icon_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}
#endif /* CONFIG_BT_MCC_OTS */

int bt_mcc_read_icon_url(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->icon_url_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_icon_url_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->icon_url_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_track_title(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->track_title_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_track_title_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->track_title_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_track_duration(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->track_duration_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_track_duration_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->track_duration_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_track_position(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->track_position_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_track_position_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->track_position_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_set_track_position(struct bt_conn *conn, int32_t pos)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->track_position_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	(void)memcpy(cur_mcs_inst->write_buf, &pos, sizeof(pos));

	cur_mcs_inst->write_params.offset = 0;
	cur_mcs_inst->write_params.data = cur_mcs_inst->write_buf;
	cur_mcs_inst->write_params.length = sizeof(pos);
	cur_mcs_inst->write_params.handle = cur_mcs_inst->track_position_handle;
	cur_mcs_inst->write_params.func = mcs_write_track_position_cb;

	BT_HEXDUMP_DBG(cur_mcs_inst->write_params.data, sizeof(pos),
		       "Track position sent");

	err = bt_gatt_write(conn, &cur_mcs_inst->write_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_playback_speed(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->playback_speed_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_playback_speed_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->playback_speed_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_set_playback_speed(struct bt_conn *conn, int8_t speed)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->playback_speed_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	(void)memcpy(cur_mcs_inst->write_buf, &speed, sizeof(speed));

	cur_mcs_inst->write_params.offset = 0;
	cur_mcs_inst->write_params.data = cur_mcs_inst->write_buf;
	cur_mcs_inst->write_params.length = sizeof(speed);
	cur_mcs_inst->write_params.handle = cur_mcs_inst->playback_speed_handle;
	cur_mcs_inst->write_params.func = mcs_write_playback_speed_cb;

	BT_HEXDUMP_DBG(cur_mcs_inst->write_params.data, sizeof(speed),
		       "Playback speed");

	err = bt_gatt_write(conn, &cur_mcs_inst->write_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_seeking_speed(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->seeking_speed_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_seeking_speed_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->seeking_speed_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

#ifdef CONFIG_BT_MCC_OTS
int bt_mcc_read_segments_obj_id(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->segments_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_segments_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->segments_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_current_track_obj_id(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->current_track_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_current_track_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->current_track_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_set_current_track_obj_id(struct bt_conn *conn, uint64_t obj_id)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	CHECKIF(obj_id < BT_OTS_OBJ_ID_MIN || obj_id > BT_OTS_OBJ_ID_MAX) {
		BT_DBG("Object ID invalid");
		return -EINVAL;
	}

	if (!cur_mcs_inst->current_track_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	sys_put_le48(obj_id, cur_mcs_inst->write_buf);
	cur_mcs_inst->write_params.offset = 0;
	cur_mcs_inst->write_params.data = cur_mcs_inst->write_buf;
	cur_mcs_inst->write_params.length = BT_OTS_OBJ_ID_SIZE;
	cur_mcs_inst->write_params.handle = cur_mcs_inst->current_track_obj_id_handle;
	cur_mcs_inst->write_params.func = mcs_write_current_track_obj_id_cb;

	BT_HEXDUMP_DBG(cur_mcs_inst->write_params.data, BT_OTS_OBJ_ID_SIZE, "Object Id");

	err = bt_gatt_write(conn, &cur_mcs_inst->write_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_next_track_obj_id(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->next_track_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_next_track_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->next_track_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_set_next_track_obj_id(struct bt_conn *conn, uint64_t obj_id)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	CHECKIF(obj_id < BT_OTS_OBJ_ID_MIN || obj_id > BT_OTS_OBJ_ID_MAX) {
		BT_DBG("Object ID invalid");
		return -EINVAL;
	}

	if (!cur_mcs_inst->next_track_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	sys_put_le48(obj_id, cur_mcs_inst->write_buf);
	cur_mcs_inst->write_params.offset = 0;
	cur_mcs_inst->write_params.data = cur_mcs_inst->write_buf;
	cur_mcs_inst->write_params.length = BT_OTS_OBJ_ID_SIZE;
	cur_mcs_inst->write_params.handle = cur_mcs_inst->next_track_obj_id_handle;
	cur_mcs_inst->write_params.func = mcs_write_next_track_obj_id_cb;

	BT_HEXDUMP_DBG(cur_mcs_inst->write_params.data, BT_OTS_OBJ_ID_SIZE, "Object Id");

	err = bt_gatt_write(conn, &cur_mcs_inst->write_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_parent_group_obj_id(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->parent_group_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_parent_group_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->parent_group_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_current_group_obj_id(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->current_group_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_current_group_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->current_group_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_set_current_group_obj_id(struct bt_conn *conn, uint64_t obj_id)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	CHECKIF(obj_id < BT_OTS_OBJ_ID_MIN || obj_id > BT_OTS_OBJ_ID_MAX) {
		BT_DBG("Object ID invalid");
		return -EINVAL;
	}

	if (!cur_mcs_inst->current_group_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	sys_put_le48(obj_id, cur_mcs_inst->write_buf);
	cur_mcs_inst->write_params.offset = 0;
	cur_mcs_inst->write_params.data = cur_mcs_inst->write_buf;
	cur_mcs_inst->write_params.length = BT_OTS_OBJ_ID_SIZE;
	cur_mcs_inst->write_params.handle = cur_mcs_inst->current_group_obj_id_handle;
	cur_mcs_inst->write_params.func = mcs_write_current_group_obj_id_cb;

	BT_HEXDUMP_DBG(cur_mcs_inst->write_params.data, BT_OTS_OBJ_ID_SIZE, "Object Id");

	err = bt_gatt_write(conn, &cur_mcs_inst->write_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}
#endif /* CONFIG_BT_MCC_OTS */

int bt_mcc_read_playing_order(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->playing_order_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_playing_order_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->playing_order_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_set_playing_order(struct bt_conn *conn, uint8_t order)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->playing_order_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	(void)memcpy(cur_mcs_inst->write_buf, &order, sizeof(order));

	cur_mcs_inst->write_params.offset = 0;
	cur_mcs_inst->write_params.data = cur_mcs_inst->write_buf;
	cur_mcs_inst->write_params.length = sizeof(order);
	cur_mcs_inst->write_params.handle = cur_mcs_inst->playing_order_handle;
	cur_mcs_inst->write_params.func = mcs_write_playing_order_cb;

	BT_HEXDUMP_DBG(cur_mcs_inst->write_params.data, sizeof(order),
		       "Playing order");

	err = bt_gatt_write(conn, &cur_mcs_inst->write_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_playing_orders_supported(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->playing_orders_supported_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_playing_orders_supported_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->playing_orders_supported_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_media_state(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->media_state_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_media_state_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->media_state_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_send_cmd(struct bt_conn *conn, const struct mpl_cmd *cmd)
{
	int err;
	int length = sizeof(cmd->opcode);

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->cp_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	(void)memcpy(cur_mcs_inst->write_buf, &cmd->opcode, length);
	if (cmd->use_param) {
		length += sizeof(cmd->param);
		(void)memcpy(&cur_mcs_inst->write_buf[sizeof(cmd->opcode)], &cmd->param,
		       sizeof(cmd->param));
	}

	cur_mcs_inst->write_params.offset = 0;
	cur_mcs_inst->write_params.data = cur_mcs_inst->write_buf;
	cur_mcs_inst->write_params.length = length;
	cur_mcs_inst->write_params.handle = cur_mcs_inst->cp_handle;
	cur_mcs_inst->write_params.func = mcs_write_cp_cb;

	BT_HEXDUMP_DBG(cur_mcs_inst->write_params.data, sizeof(*cmd),
		       "Command sent");

	err = bt_gatt_write(conn, &cur_mcs_inst->write_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_opcodes_supported(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->opcodes_supported_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_opcodes_supported_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->opcodes_supported_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

#ifdef CONFIG_BT_MCC_OTS
int bt_mcc_send_search(struct bt_conn *conn, const struct mpl_search *search)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->scp_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	(void)memcpy(cur_mcs_inst->write_buf, &search->search, search->len);

	cur_mcs_inst->write_params.offset = 0;
	cur_mcs_inst->write_params.data = cur_mcs_inst->write_buf;
	cur_mcs_inst->write_params.length = search->len;
	cur_mcs_inst->write_params.handle = cur_mcs_inst->scp_handle;
	cur_mcs_inst->write_params.func = mcs_write_scp_cb;

	BT_HEXDUMP_DBG(cur_mcs_inst->write_params.data, search->len,
		       "Search sent");

	err = bt_gatt_write(conn, &cur_mcs_inst->write_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_search_results_obj_id(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->search_results_obj_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_search_results_obj_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->search_results_obj_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}
#endif /* CONFIG_BT_MCC_OTS */

int bt_mcc_read_content_control_id(struct bt_conn *conn)
{
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	if (!cur_mcs_inst->content_control_id_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cur_mcs_inst->busy) {
		return -EBUSY;
	}

	read_params.func = mcc_read_content_control_id_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_mcs_inst->content_control_id_handle;
	read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params);
	if (!err) {
		cur_mcs_inst->busy = true;
	}
	return err;
}

#ifdef CONFIG_BT_MCC_OTS

void on_obj_selected(struct bt_ots_client *otc_inst,
		     struct bt_conn *conn, int result)
{
	BT_DBG("Current object selected");
	/* TODO: Read metadata here? */
	/* For now: Left to the application */

	/* Only one object at a time is selected in OTS */
	/* When the selected callback comes, a new object is selected */
	/* Reset the object buffer */
	net_buf_simple_reset(&otc_obj_buf);

	if (mcc_cb && mcc_cb->otc_obj_selected) {
		mcc_cb->otc_obj_selected(conn, OLCP_RESULT_TO_ERROR(result));
	}
}

/* TODO: Merge the object callback functions into one */
/* Use a notion of the "active" object, as done in mpl.c, for tracking  */
int on_icon_content(struct bt_ots_client *otc_inst, struct bt_conn *conn,
		    uint32_t offset, uint32_t len, uint8_t *data_p,
		    bool is_complete)
{
	int cb_err = 0;

	BT_DBG("Received Media Player Icon content, %i bytes at offset %i",
		len, offset);

	BT_HEXDUMP_DBG(data_p, len, "Icon content");

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		BT_WARN("Can not fit whole object");
		cb_err = -EMSGSIZE;
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		BT_DBG("Icon object received");

		if (mcc_cb && mcc_cb->otc_icon_object) {
			mcc_cb->otc_icon_object(conn, cb_err, &otc_obj_buf);
		}
		/* Reset buf in case the same object is read again without */
		/* calling select in between */
		net_buf_simple_reset(&otc_obj_buf);
	}

	return BT_OTS_CONTINUE;
}

#if CONFIG_BT_DEBUG_MCC
struct track_seg_t {
	uint8_t            name_len;
	char               name[CONFIG_BT_MCC_SEGMENT_NAME_MAX];
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
	struct net_buf_simple tmp_buf;

	/* Copy the buf, to not consume the original in this debug function */
	net_buf_simple_clone(buff, &tmp_buf);

	while (tmp_buf.len &&
	       track_segs->cnt < CONFIG_BT_MCC_TRACK_SEGS_MAX_CNT) {

		i = track_segs->cnt++;
		seg = &track_segs->segs[i];

		seg->name_len =  net_buf_simple_pull_u8(&tmp_buf);
		if (seg->name_len + sizeof(int32_t) > tmp_buf.len) {
			BT_WARN("Segment too long");
			return;
		}

		if (seg->name_len) {

			name = net_buf_simple_pull_mem(&tmp_buf, seg->name_len);

			if (seg->name_len >= CONFIG_BT_MCC_SEGMENT_NAME_MAX) {
				seg->name_len =
					CONFIG_BT_MCC_SEGMENT_NAME_MAX - 1;
			}
			(void)memcpy(seg->name, name, seg->name_len);
		}
		seg->name[seg->name_len] = '\0';

		track_segs->segs[i].pos = (int32_t)net_buf_simple_pull_le32(&tmp_buf);
	}
}
#endif /* CONFIG_BT_DEBUG_MCC */

int on_track_segments_content(struct bt_ots_client *otc_inst,
			      struct bt_conn *conn, uint32_t offset,
			      uint32_t len, uint8_t *data_p, bool is_complete)
{
	int cb_err = 0;

	BT_DBG("Received Track Segments content, %i bytes at offset %i",
		len, offset);

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		BT_WARN("Can not fit whole object");
		cb_err = -EMSGSIZE;
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		BT_DBG("Track segment object received");

#if CONFIG_BT_DEBUG_MCC
		struct track_segs_t track_segments;

		track_segments.cnt = 0;
		decode_track_segments(&otc_obj_buf, &track_segments);
		for (int i = 0; i < track_segments.cnt; i++) {
			BT_DBG("Track segment %i:", i);
			BT_DBG("\t-Name\t:%s",
			       track_segments.segs[i].name);
			BT_DBG("\t-Position\t:%d", track_segments.segs[i].pos);
		}
#endif /* CONFIG_BT_DEBUG_MCC */

		if (mcc_cb && mcc_cb->otc_track_segments_object) {
			mcc_cb->otc_track_segments_object(conn,
							   cb_err, &otc_obj_buf);
		}

		net_buf_simple_reset(&otc_obj_buf);
	}

	return BT_OTS_CONTINUE;
}

int on_current_track_content(struct bt_ots_client *otc_inst,
			     struct bt_conn *conn, uint32_t offset,
			     uint32_t len, uint8_t *data_p, bool is_complete)
{
	int cb_err = 0;

	BT_DBG("Received Current Track content, %i bytes at offset %i",
	       len, offset);

	BT_HEXDUMP_DBG(data_p, len, "Track content");

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		BT_WARN("Can not fit whole object");
		cb_err = -EMSGSIZE;
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		BT_DBG("Current Track Object received");

		if (mcc_cb && mcc_cb->otc_current_track_object) {
			mcc_cb->otc_current_track_object(conn, cb_err, &otc_obj_buf);
		}

		net_buf_simple_reset(&otc_obj_buf);
	}

	return BT_OTS_CONTINUE;
}

int on_next_track_content(struct bt_ots_client *otc_inst,
			  struct bt_conn *conn, uint32_t offset, uint32_t len,
			  uint8_t *data_p, bool is_complete)
{
	int cb_err = 0;

	BT_DBG("Received Next Track content, %i bytes at offset %i",
	       len, offset);

	BT_HEXDUMP_DBG(data_p, len, "Track content");

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		BT_WARN("Can not fit whole object");
		cb_err = -EMSGSIZE;
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		BT_DBG("Next Track Object received");

		if (mcc_cb && mcc_cb->otc_next_track_object) {
			mcc_cb->otc_next_track_object(conn, cb_err, &otc_obj_buf);
		}

		net_buf_simple_reset(&otc_obj_buf);
	}

	return BT_OTS_CONTINUE;
}


#if CONFIG_BT_DEBUG_MCC
struct id_list_elem_t {
	uint8_t  type;
	uint64_t id;
};

struct id_list_t {
	struct id_list_elem_t ids[CONFIG_BT_MCC_GROUP_RECORDS_MAX];
	uint16_t cnt;
};

static void decode_group(struct net_buf_simple *buff,
			 struct id_list_t *ids)
{
	struct net_buf_simple tmp_buf;

	/* Copy the buf, to not consume the original in this debug function */
	net_buf_simple_clone(buff, &tmp_buf);

	while ((tmp_buf.len) && (ids->cnt < CONFIG_BT_MCC_GROUP_RECORDS_MAX)) {
		ids->ids[ids->cnt].type = net_buf_simple_pull_u8(&tmp_buf);
		ids->ids[ids->cnt++].id = net_buf_simple_pull_le48(&tmp_buf);
	}
}
#endif /* CONFIG_BT_DEBUG_MCC */

int on_parent_group_content(struct bt_ots_client *otc_inst,
			    struct bt_conn *conn, uint32_t offset,
			    uint32_t len, uint8_t *data_p, bool is_complete)
{
	int cb_err = 0;

	BT_DBG("Received Parent Group content, %i bytes at offset %i",
		len, offset);

	BT_HEXDUMP_DBG(data_p, len, "Group content");

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		BT_WARN("Can not fit whole object");
		cb_err = -EMSGSIZE;
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		BT_DBG("Parent Group object received");

#if CONFIG_BT_DEBUG_MCC
		struct id_list_t group = {0};

		decode_group(&otc_obj_buf, &group);
		for (int i = 0; i < group.cnt; i++) {
			char t[BT_OTS_OBJ_ID_STR_LEN];

			(void)bt_ots_obj_id_to_str(group.ids[i].id, t,
						   BT_OTS_OBJ_ID_STR_LEN);
			BT_DBG("Object type: %d, object  ID: %s",
			       group.ids[i].type, t);
		}
#endif /* CONFIG_BT_DEBUG_MCC */

		if (mcc_cb && mcc_cb->otc_parent_group_object) {
			mcc_cb->otc_parent_group_object(conn, cb_err, &otc_obj_buf);
		}

		net_buf_simple_reset(&otc_obj_buf);
	}

	return BT_OTS_CONTINUE;
}

int on_current_group_content(struct bt_ots_client *otc_inst,
			     struct bt_conn *conn, uint32_t offset,
			     uint32_t len, uint8_t *data_p, bool is_complete)
{
	int cb_err = 0;

	BT_DBG("Received Current Group content, %i bytes at offset %i",
		len, offset);

	BT_HEXDUMP_DBG(data_p, len, "Group content");

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		BT_WARN("Can not fit whole object");
		cb_err = -EMSGSIZE;
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		BT_DBG("Current Group object received");

#if CONFIG_BT_DEBUG_MCC
		struct id_list_t group = {0};

		decode_group(&otc_obj_buf, &group);
		for (int i = 0; i < group.cnt; i++) {
			char t[BT_OTS_OBJ_ID_STR_LEN];

			(void)bt_ots_obj_id_to_str(group.ids[i].id, t,
						   BT_OTS_OBJ_ID_STR_LEN);
			BT_DBG("Object type: %d, object  ID: %s",
			       group.ids[i].type, t);
		}
#endif /* CONFIG_BT_DEBUG_MCC */

		if (mcc_cb && mcc_cb->otc_current_group_object) {
			mcc_cb->otc_current_group_object(conn, cb_err, &otc_obj_buf);
		}

		net_buf_simple_reset(&otc_obj_buf);
	}

	return BT_OTS_CONTINUE;
}

void on_object_metadata(struct bt_ots_client *otc_inst,
			struct bt_conn *conn, int err,
			uint8_t metadata_read)
{
	BT_INFO("Object's meta data:");
	BT_INFO("\tCurrent size\t:%u", otc_inst->cur_object.size.cur);

	if (otc_inst->cur_object.size.cur > otc_obj_buf.size) {
		BT_DBG("Object larger than allocated buffer");
	}

	bt_ots_metadata_display(&otc_inst->cur_object, 1);

	if (mcc_cb && mcc_cb->otc_obj_metadata) {
		mcc_cb->otc_obj_metadata(conn, err);
	}
}

int bt_mcc_otc_read_object_metadata(struct bt_conn *conn)
{
	int err;

	err = bt_ots_client_read_object_metadata(&cur_mcs_inst->otc, conn,
						 BT_OTS_METADATA_REQ_ALL);
	if (err) {
		BT_DBG("Error reading the object: %d", err);
	}

	return err;
}


int bt_mcc_otc_read_icon_object(struct bt_conn *conn)
{
	int err;
	/* TODO: Add handling for busy - either MCS or OTS */

	cur_mcs_inst->otc.cb->obj_data_read = on_icon_content;

	err = bt_ots_client_read_object_data(&cur_mcs_inst->otc, conn);
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
	cur_mcs_inst->otc.cb->obj_data_read = on_track_segments_content;

	err = bt_ots_client_read_object_data(&cur_mcs_inst->otc, conn);
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
	cur_mcs_inst->otc.cb->obj_data_read = on_current_track_content;

	err = bt_ots_client_read_object_data(&cur_mcs_inst->otc, conn);
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
	cur_mcs_inst->otc.cb->obj_data_read = on_next_track_content;

	err = bt_ots_client_read_object_data(&cur_mcs_inst->otc, conn);
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
	cur_mcs_inst->otc.cb->obj_data_read = on_parent_group_content;

	err = bt_ots_client_read_object_data(&cur_mcs_inst->otc, conn);
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
	cur_mcs_inst->otc.cb->obj_data_read = on_current_group_content;

	err = bt_ots_client_read_object_data(&cur_mcs_inst->otc, conn);
	if (err) {
		BT_DBG("Error reading the object: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_MCC_SHELL)
struct bt_ots_client *bt_mcc_otc_inst(void)
{
	return &cur_mcs_inst->otc;
}
#endif /* defined(CONFIG_BT_MCC_SHELL) */

#endif /* CONFIG_BT_MCC_OTS */
