/** @file
 *  @brief Bluetooth Media Control Client/Protocol implementation
 */

/*
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/mcc.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/mcs.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

#include "../services/ots/ots_client_internal.h"
#include "common/bt_str.h"
#include "mcc_internal.h"
#include "mcs_internal.h"

/* TODO: Temporarily copied here from media_proxy_internal.h - clean up */
/* Debug output of 48 bit Object ID value */
/* (Zephyr does not yet support debug output of more than 32 bit values.) */
/* Takes a text and a 64-bit integer as input */
#define LOG_DBG_OBJ_ID(text, id64) \
	do { \
		if (IS_ENABLED(CONFIG_BT_MCS_LOG_LEVEL)) { \
			char t[BT_OTS_OBJ_ID_STR_LEN]; \
			(void)bt_ots_obj_id_to_str(id64, t, sizeof(t)); \
			LOG_DBG(text "0x%s", t); \
		} \
	} while (0)

LOG_MODULE_REGISTER(bt_mcc, CONFIG_BT_MCC_LOG_LEVEL);

static struct mcs_instance_t mcs_instance;
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

struct mcs_instance_t *lookup_inst_by_conn(struct bt_conn *conn)
{
	if (conn == NULL) {
		return NULL;
	}

	/* TODO: Expand when supporting more instances */
	return &mcs_instance;
}

static void mcc_player_name_cb(struct bt_conn *conn, uint8_t err, const void *data, uint16_t length)
{
	int cb_err = err;
	char name[CONFIG_BT_MCC_MEDIA_PLAYER_NAME_MAX];

	LOG_DBG("err: 0x%02x, length: %d, data: %p", err, length, data);

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!data) {
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		LOG_HEXDUMP_DBG(data, length, "Player name read");

		if (length >= sizeof(name)) {
			length = sizeof(name) - 1;
		}

		(void)memcpy(&name, data, length);
		name[length] = '\0';
		LOG_DBG("Player name: %s", name);
	}

	if (mcc_cb && mcc_cb->read_player_name) {
		mcc_cb->read_player_name(conn, cb_err, name);
	}
}

static uint8_t mcc_read_player_name_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_player_name_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}


#ifdef CONFIG_BT_MCC_OTS
static uint8_t mcc_read_icon_obj_id_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	mcs_inst->busy = false;
	LOG_DBG("err: 0x%02x, length: %d, data: %p", err, length, data);
	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if ((!pid) || (length != BT_OTS_OBJ_ID_SIZE)) {
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		LOG_HEXDUMP_DBG(pid, length, "Icon Object ID");
		id = sys_get_le48(pid);
		LOG_DBG_OBJ_ID("Icon Object ID: ", id);

		if (!BT_MCS_VALID_OBJ_ID(id)) {
			cb_err = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	if (mcc_cb && mcc_cb->read_icon_obj_id) {
		mcc_cb->read_icon_obj_id(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL)
static uint8_t mcc_read_icon_url_cb(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params,
				    const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);
	int cb_err = err;
	char url[CONFIG_BT_MCC_ICON_URL_MAX];

	mcs_inst->busy = false;
	LOG_DBG("err: 0x%02x, length: %d, data: %p", err, length, data);
	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!data) {
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (length >= sizeof(url)) {
		cb_err = BT_ATT_ERR_INSUFFICIENT_RESOURCES;
	} else {
		LOG_HEXDUMP_DBG(data, length, "Icon URL");
		(void)memcpy(&url, data, length);
		url[length] = '\0';
		LOG_DBG("Icon URL: %s", url);
	}

	if (mcc_cb && mcc_cb->read_icon_url) {
		mcc_cb->read_icon_url(conn, cb_err, url);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL) */

#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE)
static void mcc_track_title_cb(struct bt_conn *conn, uint8_t err, const void *data, uint16_t length)
{
	int cb_err = err;
	char title[CONFIG_BT_MCC_TRACK_TITLE_MAX];

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!data) {
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		LOG_HEXDUMP_DBG(data, length, "Track title");
		if (length >= sizeof(title)) {
			/* If the description is too long; clip it. */
			length = sizeof(title) - 1;
		}
		(void)memcpy(&title, data, length);
		title[length] = '\0';
		LOG_DBG("Track title: %s", title);
	}

	if (mcc_cb && mcc_cb->read_track_title) {
		mcc_cb->read_track_title(conn, cb_err, title);
	}
}

static uint8_t mcc_read_track_title_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_track_title_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE)*/

#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
static void mcc_track_duration_cb(struct bt_conn *conn, uint8_t err, const void *data,
				  uint16_t length)
{
	int cb_err = err;
	int32_t dur = 0;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(dur))) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		dur = sys_get_le32((uint8_t *)data);
		LOG_DBG("Track duration: %d", dur);
		LOG_HEXDUMP_DBG(data, sizeof(int32_t), "Track duration");
	}

	if (mcc_cb && mcc_cb->read_track_duration) {
		mcc_cb->read_track_duration(conn, cb_err, dur);
	}
}

static uint8_t mcc_read_track_duration_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_track_duration_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */

#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
static void mcc_track_position_cb(struct bt_conn *conn, uint8_t err, const void *data,
				  uint16_t length)
{
	int cb_err = err;
	int32_t pos = 0;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(pos))) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else  {
		pos = sys_get_le32((uint8_t *)data);
		LOG_DBG("Track position: %d", pos);
		LOG_HEXDUMP_DBG(data, sizeof(pos), "Track position");
	}

	if (mcc_cb && mcc_cb->read_track_position) {
		mcc_cb->read_track_position(conn, cb_err, pos);
	}
}

static uint8_t mcc_read_track_position_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_track_position_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */

#if defined(CONFIG_BT_MCC_SET_TRACK_POSITION)
static void mcs_write_track_position_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_write_params *params)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, write_params);
	int cb_err = err;
	int32_t pos = 0;

	mcs_inst->busy = false;
	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != sizeof(pos)) {
		LOG_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		pos = sys_get_le32((uint8_t *)params->data);
		LOG_DBG("Track position: %d", pos);
		LOG_HEXDUMP_DBG(params->data, sizeof(pos), "Track position in callback");
	}

	if (mcc_cb && mcc_cb->set_track_position) {
		mcc_cb->set_track_position(conn, cb_err, pos);
	}
}
#endif /* defined(CONFIG_BT_MCC_SET_TRACK_POSITION) */

#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
static void mcc_playback_speed_cb(struct bt_conn *conn, uint8_t err, const void *data,
				  uint16_t length)
{
	int cb_err = err;
	int8_t speed = 0;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(speed))) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		speed = *(int8_t *)data;
		LOG_DBG("Playback speed: %d", speed);
		LOG_HEXDUMP_DBG(data, sizeof(int8_t), "Playback speed");
	}

	if (mcc_cb && mcc_cb->read_playback_speed) {
		mcc_cb->read_playback_speed(conn, cb_err, speed);
	}
}

static uint8_t mcc_read_playback_speed_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_playback_speed_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) */

#if defined(CONFIG_BT_MCC_SET_PLAYBACK_SPEED)
static void mcs_write_playback_speed_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_write_params *params)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, write_params);
	int cb_err = err;
	int8_t speed = 0;

	mcs_inst->busy = false;
	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != sizeof(speed)) {
		LOG_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		speed = *(int8_t *)params->data;
		LOG_DBG("Playback_speed: %d", speed);
	}

	if (mcc_cb && mcc_cb->set_playback_speed) {
		mcc_cb->set_playback_speed(conn, cb_err, speed);
	}
}
#endif /* defined (CONFIG_BT_MCC_SET_PLAYBACK_SPEED) */

#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
static void mcc_seeking_speed_cb(struct bt_conn *conn, uint8_t err, const void *data,
				 uint16_t length)
{
	int cb_err = err;
	int8_t speed = 0;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(speed))) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		speed = *(int8_t *)data;
		LOG_DBG("Seeking speed: %d", speed);
		LOG_HEXDUMP_DBG(data, sizeof(int8_t), "Seeking speed");
	}

	if (mcc_cb && mcc_cb->read_seeking_speed) {
		mcc_cb->read_seeking_speed(conn, cb_err, speed);
	}
}

static uint8_t mcc_read_seeking_speed_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_read_params *params,
					 const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_seeking_speed_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */

#ifdef CONFIG_BT_MCC_OTS
static uint8_t mcc_read_segments_obj_id_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_read_params *params,
					   const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	mcs_inst->busy = false;
	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if ((!pid) || (length != BT_OTS_OBJ_ID_SIZE)) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		LOG_HEXDUMP_DBG(pid, length, "Segments Object ID");
		id = sys_get_le48(pid);
		LOG_DBG_OBJ_ID("Segments Object ID: ", id);

		if (!BT_MCS_VALID_OBJ_ID(id)) {
			cb_err = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	if (mcc_cb && mcc_cb->read_segments_obj_id) {
		mcc_cb->read_segments_obj_id(conn, cb_err, id);
	}

	return BT_GATT_ITER_STOP;
}

static void mcc_current_track_obj_id_cb(struct bt_conn *conn, uint8_t err, const void *data,
					uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if ((!pid) || (length != BT_OTS_OBJ_ID_SIZE)) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		LOG_HEXDUMP_DBG(pid, length, "Current Track Object ID");
		id = sys_get_le48(pid);
		LOG_DBG_OBJ_ID("Current Track Object ID: ", id);

		if (!BT_MCS_VALID_OBJ_ID(id)) {
			cb_err = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	if (mcc_cb && mcc_cb->read_current_track_obj_id) {
		mcc_cb->read_current_track_obj_id(conn, cb_err, id);
	}
}

static uint8_t mcc_read_current_track_obj_id_cb(struct bt_conn *conn, uint8_t err,
						struct bt_gatt_read_params *params,
						const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_current_track_obj_id_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}

static void mcs_write_current_track_obj_id_cb(struct bt_conn *conn, uint8_t err,
					      struct bt_gatt_write_params *params)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, write_params);
	int cb_err = err;
	uint64_t obj_id = 0;

	mcs_inst->busy = false;
	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != BT_OTS_OBJ_ID_SIZE) {
		LOG_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		obj_id = sys_get_le48((const uint8_t *)params->data);
		LOG_DBG_OBJ_ID("Object ID: ", obj_id);

		if (!BT_MCS_VALID_OBJ_ID(obj_id)) {
			cb_err = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	if (mcc_cb && mcc_cb->set_current_track_obj_id) {
		mcc_cb->set_current_track_obj_id(conn, cb_err, obj_id);
	}
}

static void mcc_next_track_obj_id_cb(struct bt_conn *conn, uint8_t err, const void *data,
				     uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (length == 0) {
		LOG_DBG("Characteristic is empty");
	} else if (!pid || (length != BT_OTS_OBJ_ID_SIZE)) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		LOG_HEXDUMP_DBG(pid, length, "Next Track Object ID");
		id = sys_get_le48(pid);
		LOG_DBG_OBJ_ID("Next Track Object ID: ", id);

		if (!BT_MCS_VALID_OBJ_ID(id)) {
			cb_err = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	if (mcc_cb && mcc_cb->read_next_track_obj_id) {
		mcc_cb->read_next_track_obj_id(conn, cb_err, id);
	}
}

static uint8_t mcc_read_next_track_obj_id_cb(struct bt_conn *conn, uint8_t err,
					     struct bt_gatt_read_params *params,
					     const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_next_track_obj_id_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}

static void mcs_write_next_track_obj_id_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_write_params *params)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, write_params);
	int cb_err = err;
	uint64_t obj_id = 0;

	mcs_inst->busy = false;
	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != BT_OTS_OBJ_ID_SIZE) {
		LOG_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		obj_id = sys_get_le48((const uint8_t *)params->data);
		LOG_DBG_OBJ_ID("Object ID: ", obj_id);

		if (!BT_MCS_VALID_OBJ_ID(obj_id)) {
			cb_err = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	if (mcc_cb && mcc_cb->set_next_track_obj_id) {
		mcc_cb->set_next_track_obj_id(conn, cb_err, obj_id);
	}
}

static void mcc_parent_group_obj_id_cb(struct bt_conn *conn, uint8_t err, const void *data,
				       uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!pid || (length != BT_OTS_OBJ_ID_SIZE)) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		LOG_HEXDUMP_DBG(pid, length, "Parent Group Object ID");
		id = sys_get_le48(pid);
		LOG_DBG_OBJ_ID("Parent Group Object ID: ", id);

		if (!BT_MCS_VALID_OBJ_ID(id)) {
			cb_err = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	if (mcc_cb && mcc_cb->read_parent_group_obj_id) {
		mcc_cb->read_parent_group_obj_id(conn, cb_err, id);
	}
}

static uint8_t mcc_read_parent_group_obj_id_cb(struct bt_conn *conn, uint8_t err,
					       struct bt_gatt_read_params *params,
					       const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_parent_group_obj_id_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}

static void mcc_current_group_obj_id_cb(struct bt_conn *conn, uint8_t err, const void *data,
					uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!pid || (length != BT_OTS_OBJ_ID_SIZE)) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		LOG_HEXDUMP_DBG(pid, length, "Current Group Object ID");
		id = sys_get_le48(pid);
		LOG_DBG_OBJ_ID("Current Group Object ID: ", id);

		if (!BT_MCS_VALID_OBJ_ID(id)) {
			cb_err = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	if (mcc_cb && mcc_cb->read_current_group_obj_id) {
		mcc_cb->read_current_group_obj_id(conn, cb_err, id);
	}
}

static uint8_t mcc_read_current_group_obj_id_cb(struct bt_conn *conn, uint8_t err,
						struct bt_gatt_read_params *params,
						const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_current_group_obj_id_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}

static void mcs_write_current_group_obj_id_cb(struct bt_conn *conn, uint8_t err,
					      struct bt_gatt_write_params *params)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, write_params);
	int cb_err = err;
	uint64_t obj_id = 0;

	mcs_inst->busy = false;
	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != BT_OTS_OBJ_ID_SIZE) {
		LOG_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		obj_id = sys_get_le48((const uint8_t *)params->data);
		LOG_DBG_OBJ_ID("Object ID: ", obj_id);

		if (!BT_MCS_VALID_OBJ_ID(obj_id)) {
			cb_err = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	if (mcc_cb && mcc_cb->set_current_group_obj_id) {
		mcc_cb->set_current_group_obj_id(conn, cb_err, obj_id);
	}
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
static void mcc_playing_order_cb(struct bt_conn *conn, uint8_t err, const void *data,
				 uint16_t length)
{
	int cb_err = err;
	uint8_t order = 0;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(order))) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		order = *(uint8_t *)data;
		LOG_DBG("Playing order: %d", order);
		LOG_HEXDUMP_DBG(data, sizeof(order), "Playing order");
	}

	if (mcc_cb && mcc_cb->read_playing_order) {
		mcc_cb->read_playing_order(conn, cb_err, order);
	}
}

static uint8_t mcc_read_playing_order_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_read_params *params,
					 const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_playing_order_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_SET_PLAYING_ORDER)
static void mcs_write_playing_order_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_write_params *params)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, write_params);
	int cb_err = err;
	uint8_t order = 0;

	mcs_inst->busy = false;
	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!params->data || params->length != sizeof(order)) {
		LOG_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		order = *(uint8_t *)params->data;
		LOG_DBG("Playing order: %d", *(uint8_t *)params->data);
	}

	if (mcc_cb && mcc_cb->set_playing_order) {
		mcc_cb->set_playing_order(conn, cb_err, order);
	}
}
#endif /* defined(CONFIG_BT_MCC_SET_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED)
static uint8_t mcc_read_playing_orders_supported_cb(struct bt_conn *conn, uint8_t err,
						    struct bt_gatt_read_params *params,
						    const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);
	int cb_err = err;
	uint16_t orders = 0;

	mcs_inst->busy = false;
	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!data || length != sizeof(orders)) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		orders = sys_get_le16((uint8_t *)data);
		LOG_DBG("Playing orders_supported: %d", orders);
		LOG_HEXDUMP_DBG(data, sizeof(orders), "Playing orders supported");
	}

	if (mcc_cb && mcc_cb->read_playing_orders_supported) {
		mcc_cb->read_playing_orders_supported(conn, cb_err, orders);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED) */

#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
static void mcc_media_state_cb(struct bt_conn *conn, uint8_t err, const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t state = 0;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!data || length != sizeof(state)) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		state = *(uint8_t *)data;
		LOG_DBG("Media state: %d", state);
		LOG_HEXDUMP_DBG(data, sizeof(uint8_t), "Media state");
	}

	if (mcc_cb && mcc_cb->read_media_state) {
		mcc_cb->read_media_state(conn, cb_err, state);
	}
}

static uint8_t mcc_read_media_state_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_media_state_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */

#if defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT)
static void mcs_write_cp_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_write_params *params)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, write_params);
	int cb_err = err;
	struct mpl_cmd cmd = {0};

	mcs_inst->busy = false;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!params->data ||
		   (params->length != sizeof(cmd.opcode) &&
		    params->length != sizeof(cmd.opcode) + sizeof(cmd.param))) {
		/* Above: No data pointer, or length not equal to any of the */
		/* two possible values */
		LOG_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		(void)memcpy(&cmd.opcode, params->data, sizeof(cmd.opcode));
		if (params->length == sizeof(cmd.opcode) + sizeof(cmd.param)) {
			(void)memcpy(&cmd.param,
			       (char *)(params->data) + sizeof(cmd.opcode),
			       sizeof(cmd.param));
			cmd.use_param = true;
			LOG_DBG("Command in callback: %d, param: %d", cmd.opcode, cmd.param);
		}
	}

	if (mcc_cb && mcc_cb->send_cmd) {
		mcc_cb->send_cmd(conn, cb_err, &cmd);
	}
}
#endif /* defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT) */

#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
static void mcc_opcodes_supported_cb(struct bt_conn *conn, uint8_t err, const void *data,
				     uint16_t length)
{
	int cb_err = err;
	int32_t operations = 0;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(operations))) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	} else {
		operations = sys_get_le32((uint8_t *)data);
		LOG_DBG("Opcodes supported: %d", operations);
		LOG_HEXDUMP_DBG(data, sizeof(operations), "Opcodes_supported");
	}

	if (mcc_cb && mcc_cb->read_opcodes_supported) {
		mcc_cb->read_opcodes_supported(conn, cb_err, operations);
	}
}

static uint8_t mcc_read_opcodes_supported_cb(struct bt_conn *conn, uint8_t err,
					     struct bt_gatt_read_params *params,
					     const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_opcodes_supported_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */

#ifdef CONFIG_BT_MCC_OTS
static void mcs_write_scp_cb(struct bt_conn *conn, uint8_t err,
			     struct bt_gatt_write_params *params)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, write_params);
	int cb_err = err;
	struct mpl_search search = {0};

	mcs_inst->busy = false;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (!params->data ||
		   (params->length > SEARCH_LEN_MAX)) {
		LOG_DBG("length: %d, data: %p", params->length, params->data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		search.len = params->length;
		(void)memcpy(search.search, params->data, params->length);
		LOG_DBG("Length of returned value in callback: %d", search.len);
	}

	if (mcc_cb && mcc_cb->send_search) {
		mcc_cb->send_search(conn, cb_err, &search);
	}
}

static void mcc_search_results_obj_id_cb(struct bt_conn *conn, uint8_t err,
					 const void *data, uint16_t length)
{
	int cb_err = err;
	uint8_t *pid = (uint8_t *)data;
	uint64_t id = 0;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if (length == 0) {
		/* OK - this characteristic may be zero length */
		/* cb_err and id already have correct values */
		LOG_DBG("Zero-length Search Results Object ID");
	} else if (!pid || (length != BT_OTS_OBJ_ID_SIZE)) {
		LOG_DBG("length: %d, pid: %p", length, pid);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else {
		id = sys_get_le48(pid);
		LOG_DBG_OBJ_ID("Search Results Object ID: ", id);

		if (!BT_MCS_VALID_OBJ_ID(id)) {
			cb_err = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	if (mcc_cb && mcc_cb->read_search_results_obj_id) {
		mcc_cb->read_search_results_obj_id(conn, cb_err, id);
	}
}

static uint8_t mcc_read_search_results_obj_id_cb(struct bt_conn *conn, uint8_t err,
						 struct bt_gatt_read_params *params,
						 const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);

	mcs_inst->busy = false;
	mcc_search_results_obj_id_cb(conn, err, data, length);

	return BT_GATT_ITER_STOP;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID)
static uint8_t mcc_read_content_control_id_cb(struct bt_conn *conn, uint8_t err,
					      struct bt_gatt_read_params *params,
					      const void *data, uint16_t length)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params, struct mcs_instance_t, read_params);
	int cb_err = err;
	uint8_t ccid = 0;

	mcs_inst->busy = false;

	if (err) {
		LOG_DBG("err: 0x%02x", err);
	} else if ((!data) || (length != sizeof(ccid))) {
		LOG_DBG("length: %d, data: %p", length, data);
		cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	} else {
		ccid = *(uint8_t *)data;
		LOG_DBG("Content control ID: %d", ccid);
	}

	if (mcc_cb && mcc_cb->read_content_control_id) {
		mcc_cb->read_content_control_id(conn, cb_err, ccid);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID) */

static uint8_t mcs_notify_handler(struct bt_conn *conn,
				  struct bt_gatt_subscribe_params *params,
				  const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct mcs_instance_t *mcs_inst;

	if (data == NULL) {
		LOG_DBG("[UNSUBSCRIBED] 0x%04X", params->value_handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_CONTINUE;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not find MCS instance from conn %p", (void *)conn);

		return BT_GATT_ITER_CONTINUE;
	}

	LOG_DBG("Notification, handle: %d", handle);

	if (handle == mcs_inst->player_name_handle) {
		LOG_DBG("Player Name notification");
		mcc_player_name_cb(conn, 0, data, length);

	} else if (handle == mcs_inst->track_changed_handle) {
		/* The Track Changed characteristic can only be */
		/* notified, so that is handled directly here */
		int cb_err = 0;

		LOG_DBG("Track Changed notification");
		LOG_DBG("data: %p, length: %u", data, length);

		if (length != 0) {
			LOG_DBG("Non-zero length: %u", length);
			cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		if (mcc_cb && mcc_cb->track_changed_ntf) {
			mcc_cb->track_changed_ntf(conn, cb_err);
		}

#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE_ENABLE_SUBSCRIPTION)
	} else if (handle == mcs_inst->track_title_handle) {
		LOG_DBG("Track Title notification");
		mcc_track_title_cb(conn, 0, data, length);
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE_ENABLE_SUBSCRIPTION) */

#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
	} else if (handle == mcs_inst->track_duration_handle) {
		LOG_DBG("Track Duration notification");
		mcc_track_duration_cb(conn, 0, data, length);
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */

#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
	} else if (handle == mcs_inst->track_position_handle) {
		LOG_DBG("Track Position notification");
		mcc_track_position_cb(conn, 0, data, length);
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */

#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
	} else if (handle == mcs_inst->playback_speed_handle) {
		LOG_DBG("Playback Speed notification");
		mcc_playback_speed_cb(conn, 0, data, length);
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) */

#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
	} else if (handle == mcs_inst->seeking_speed_handle) {
		LOG_DBG("Seeking Speed notification");
		mcc_seeking_speed_cb(conn, 0, data, length);
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */

#ifdef CONFIG_BT_MCC_OTS
	} else if (handle == mcs_inst->current_track_obj_id_handle) {
		LOG_DBG("Current Track notification");
		mcc_current_track_obj_id_cb(conn, 0, data, length);

	} else if (handle == mcs_inst->next_track_obj_id_handle) {
		LOG_DBG("Next Track notification");
		mcc_next_track_obj_id_cb(conn, 0, data, length);

	} else if (handle == mcs_inst->parent_group_obj_id_handle) {
		LOG_DBG("Parent Group notification");
		mcc_parent_group_obj_id_cb(conn, 0, data, length);

	} else if (handle == mcs_inst->current_group_obj_id_handle) {
		LOG_DBG("Current Group notification");
		mcc_current_group_obj_id_cb(conn, 0, data, length);
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
	} else if (handle == mcs_inst->playing_order_handle) {
		LOG_DBG("Playing Order notification");
		mcc_playing_order_cb(conn, 0, data, length);
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
	} else if (handle == mcs_inst->media_state_handle) {
		LOG_DBG("Media State notification");
		mcc_media_state_cb(conn, 0, data, length);
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */

	} else if (handle == mcs_inst->cp_handle) {
		/* The control point is a special case - only */
		/* writable and notifiable.  Handle directly here. */
		struct mpl_cmd_ntf ntf = {0};
		int cb_err = 0;

		LOG_DBG("Control Point notification");
		if (length == sizeof(ntf.requested_opcode) + sizeof(ntf.result_code)) {
			ntf.requested_opcode = *(uint8_t *)data;
			ntf.result_code = *((uint8_t *)data +
						sizeof(ntf.requested_opcode));
			LOG_DBG("Command: %d, result: %d",
				ntf.requested_opcode, ntf.result_code);
		} else {
			LOG_DBG("length: %d, data: %p", length, data);
			cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		if (mcc_cb && mcc_cb->cmd_ntf) {
			mcc_cb->cmd_ntf(conn, cb_err, &ntf);
		}

#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
	} else if (handle == mcs_inst->opcodes_supported_handle) {
		LOG_DBG("Opcodes Supported notification");
		mcc_opcodes_supported_cb(conn, 0, data, length);
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */

#ifdef CONFIG_BT_MCC_OTS
	} else if (handle == mcs_inst->scp_handle) {
		/* The search control point is a special case - only */
		/* writable and notifiable.  Handle directly here. */
		int cb_err = 0;
		uint8_t result_code = 0;

		LOG_DBG("Search Control Point notification");
		if (length == sizeof(result_code)) {
			result_code = *(uint8_t *)data;
			LOG_DBG("Result: %d", result_code);
		} else {
			LOG_DBG("length: %d, data: %p", length, data);
			cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		if (mcc_cb && mcc_cb->search_ntf) {
			mcc_cb->search_ntf(conn, cb_err, result_code);
		}

	} else if (handle == mcs_inst->search_results_obj_id_handle) {
		LOG_DBG("Search Results notification");
		mcc_search_results_obj_id_cb(conn, 0, data, length);
#endif /* CONFIG_BT_MCC_OTS */
	} else {
		LOG_DBG("Unknown handle: %d (0x%04X)", handle, handle);
	}

	return BT_GATT_ITER_CONTINUE;
}

static int reset_mcs_inst(struct mcs_instance_t *mcs_inst)
{
	if (mcs_inst->conn != NULL) {
		struct bt_conn *conn = mcs_inst->conn;
		struct bt_conn_info info;
		int err;

		err = bt_conn_get_info(conn, &info);
		if (err != 0) {
			return err;
		}

		if (info.state == BT_CONN_STATE_CONNECTED) {
			/* It's okay if these fail with -EINVAL as that means that they are
			 * not currently subscribed
			 */
			err = bt_gatt_unsubscribe(conn, &mcs_inst->player_name_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to name: %d", err);

				return err;
			}

			err = bt_gatt_unsubscribe(conn, &mcs_inst->track_changed_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to track change: %d", err);

				return err;
			}

#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE_ENABLE_SUBSCRIPTION)
			err = bt_gatt_unsubscribe(conn, &mcs_inst->track_title_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to track title: %d", err);

				return err;
			}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE_ENABLE_SUBSCRIPTION) */

#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
			err = bt_gatt_unsubscribe(conn, &mcs_inst->track_duration_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to track duration: %d", err);

				return err;
			}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */

#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
			err = bt_gatt_unsubscribe(conn, &mcs_inst->track_position_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to track position: %d", err);

				return err;
			}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */

#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
			err = bt_gatt_unsubscribe(conn, &mcs_inst->playback_speed_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to playback speed: %d", err);

				return err;
			}
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) */

#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
			err = bt_gatt_unsubscribe(conn, &mcs_inst->seeking_speed_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to seeking speed: %d", err);

				return err;
			}
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */

#ifdef CONFIG_BT_MCC_OTS
			err = bt_gatt_unsubscribe(conn, &mcs_inst->current_track_obj_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to current track object: %d", err);

				return err;
			}

			err = bt_gatt_unsubscribe(conn, &mcs_inst->next_track_obj_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to next track object: %d", err);

				return err;
			}

			err = bt_gatt_unsubscribe(conn, &mcs_inst->parent_group_obj_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to parent group object: %d", err);

				return err;
			}

			err = bt_gatt_unsubscribe(conn, &mcs_inst->current_group_obj_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to current group object: %d", err);

				return err;
			}

#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
			err = bt_gatt_unsubscribe(conn, &mcs_inst->playing_order_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to playing order: %d", err);

				return err;
			}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
			err = bt_gatt_unsubscribe(conn, &mcs_inst->media_state_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to media state: %d", err);

				return err;
			}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */

			err = bt_gatt_unsubscribe(conn, &mcs_inst->cp_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to control point: %d", err);

				return err;
			}

#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
			err = bt_gatt_unsubscribe(conn, &mcs_inst->opcodes_supported_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to supported opcodes: %d", err);

				return err;
			}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */

#ifdef CONFIG_BT_MCC_OTS
			err = bt_gatt_unsubscribe(conn, &mcs_inst->scp_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to search control point: %d", err);

				return err;
			}

			err = bt_gatt_unsubscribe(conn, &mcs_inst->search_results_obj_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to search results: %d", err);

				return err;
			}

			err = bt_gatt_unsubscribe(conn, &mcs_inst->otc.oacp_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to oacp: %d", err);

				return err;
			}

			err = bt_gatt_unsubscribe(conn, &mcs_inst->otc.olcp_sub_params);
			if (err != 0 && err != -EINVAL) {
				LOG_DBG("Failed to unsubscribe to olcp: %d", err);

				return err;
			}
#endif /* CONFIG_BT_MCC_OTS */
		}

		bt_conn_unref(conn);
		mcs_inst->conn = NULL;
	}

	(void)memset(mcs_inst, 0, offsetof(struct mcs_instance_t, busy));
#ifdef CONFIG_BT_MCC_OTS
	/* Reset OTC instance as well if supported */
	(void)memset(&mcs_inst->otc, 0, offsetof(struct bt_ots_client, oacp_sub_params));
#endif /* CONFIG_BT_MCC_OTS */

	return 0;
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	struct mcs_instance_t *mcs_inst;

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst != NULL) {
		(void)reset_mcs_inst(mcs_inst);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected_cb,
};

/* Called when discovery is completed - successfully or with error */
static void discovery_complete(struct bt_conn *conn, int err)
{
	struct mcs_instance_t *mcs_inst;

	LOG_DBG("Discovery completed, err: %d", err);

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst != NULL) {
		mcs_inst->busy = false;
		if (err != 0) {
			(void)reset_mcs_inst(mcs_inst);
		}
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
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params,
						       struct mcs_instance_t,
						       discover_params);
	int err = 0;
	struct bt_gatt_chrc *chrc;
	struct bt_gatt_subscribe_params *sub_params = NULL;

	if (attr) {
		/* Found an attribute */
		LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

		if (params->type != BT_GATT_DISCOVER_CHARACTERISTIC) {
			/* But it was not a characteristic - continue search */
			return BT_GATT_ITER_CONTINUE;
		}

		/* We have found an attribute, and it is a characteristic */
		/* Find out which attribute, and subscribe if we should */
		chrc = (struct bt_gatt_chrc *)attr->user_data;
		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_FEATURE)) {
			LOG_DBG("OTS Features");
			mcs_inst->otc.feature_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_NAME)) {
			LOG_DBG("Object Name");
			mcs_inst->otc.obj_name_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_TYPE)) {
			LOG_DBG("Object Type");
			mcs_inst->otc.obj_type_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_SIZE)) {
			LOG_DBG("Object Size");
			mcs_inst->otc.obj_size_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_ID)) {
			LOG_DBG("Object ID");
			mcs_inst->otc.obj_id_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_PROPERTIES)) {
			LOG_DBG("Object properties %d", chrc->value_handle);
			mcs_inst->otc.obj_properties_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_ACTION_CP)) {
			LOG_DBG("Object Action Control Point");
			mcs_inst->otc.oacp_handle = chrc->value_handle;
			sub_params = &mcs_inst->otc.oacp_sub_params;
			sub_params->disc_params = &mcs_inst->otc.oacp_sub_disc_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_OTS_LIST_CP)) {
			LOG_DBG("Object List Control Point");
			mcs_inst->otc.olcp_handle = chrc->value_handle;
			sub_params = &mcs_inst->otc.olcp_sub_params;
			sub_params->disc_params = &mcs_inst->otc.olcp_sub_disc_params;
		}

		if (sub_params) {
			/* With ccc_handle == 0 it will use auto discovery */
			sub_params->ccc_handle = 0;
			sub_params->end_handle = mcs_inst->otc.end_handle;
			sub_params->value = BT_GATT_CCC_INDICATE;
			sub_params->value_handle = chrc->value_handle;
			sub_params->notify = bt_ots_client_indicate_handler;
			atomic_set_bit(sub_params->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

			err = bt_gatt_subscribe(conn, sub_params);
			if (err != 0) {
				LOG_DBG("Failed to subscribe (err %d)", err);
				discovery_complete(conn, err);

				return BT_GATT_ITER_STOP;
			}
		}

		return BT_GATT_ITER_CONTINUE;
	}

	/* No more attributes found */
	mcs_inst->otc.cb = &otc_cb;
	bt_ots_client_register(&mcs_inst->otc);

	LOG_DBG("Setup complete for included OTS");
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
		struct mcs_instance_t *mcs_inst = CONTAINER_OF(params,
							       struct mcs_instance_t,
							       discover_params);

		LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

		__ASSERT(params->type == BT_GATT_DISCOVER_INCLUDE,
			 "Wrong type");

		/* We have found an included service */
		include = (struct bt_gatt_include *)attr->user_data;
		LOG_DBG("Include UUID %s", bt_uuid_str(include->uuid));

		if (bt_uuid_cmp(include->uuid, BT_UUID_OTS)) {
			/* But it is not OTS - continue search */
			LOG_WRN("Included service is not OTS");
			return BT_GATT_ITER_CONTINUE;
		}

		/* We have the included OTS service (MCS includes only one) */
		LOG_DBG("Discover include complete for GMCS: OTS");
		mcs_inst->otc.start_handle = include->start_handle;
		mcs_inst->otc.end_handle = include->end_handle;
		(void)memset(params, 0, sizeof(*params));

		/* Discover characteristics of the included OTS */
		mcs_inst->discover_params.start_handle = mcs_inst->otc.start_handle;
		mcs_inst->discover_params.end_handle = mcs_inst->otc.end_handle;
		mcs_inst->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		mcs_inst->discover_params.func = discover_otc_char_func;

		LOG_DBG("Start discovery of OTS characteristics");
		err = bt_gatt_discover(conn, &mcs_inst->discover_params);
		if (err) {
			LOG_DBG("Discovery of OTS chars. failed");
			discovery_complete(conn, err);
		}
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("No included OTS found");
	/* This is OK, the server may not support OTS. But in that case,
	 *  discovery stops here.
	 */
	discovery_complete(conn, err);
	return BT_GATT_ITER_STOP;
}

/* Start discovery of included services */
static void discover_included(struct mcs_instance_t *mcs_inst, struct bt_conn *conn)
{
	int err;

	memset(&mcs_inst->discover_params, 0, sizeof(mcs_inst->discover_params));

	mcs_inst->discover_params.start_handle = mcs_inst->start_handle;
	mcs_inst->discover_params.end_handle = mcs_inst->end_handle;
	mcs_inst->discover_params.type = BT_GATT_DISCOVER_INCLUDE;
	mcs_inst->discover_params.func = discover_include_func;

	LOG_DBG("Start discovery of included services");
	err = bt_gatt_discover(conn, &mcs_inst->discover_params);
	if (err) {
		LOG_DBG("Discovery of included service failed: %d", err);
		discovery_complete(conn, err);
	}
}
#endif /* CONFIG_BT_MCC_OTS */

static bool subscribe_next_mcs_char(struct mcs_instance_t *mcs_inst,
				    struct bt_conn *conn);

/* This function will subscribe to GMCS CCCDs.
 * After this, the function will start discovery of included services.
 */
static void subscribe_mcs_char_func(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_subscribe_params *params)
{
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params->disc_params,
						       struct mcs_instance_t,
						       discover_params);
	bool subscription_done;

	if (err) {
		LOG_DBG("Subscription callback error: %u", err);
		params->subscribe = NULL;
		discovery_complete(conn, err);
		return;
	}

	LOG_DBG("Subscribed: value handle: %d, ccc handle: %d",
	       params->value_handle, params->ccc_handle);

	if (params->value_handle == 0) {
		/* Unsubscribing, ignore */
		return;
	}

	/* Subscribe to next characteristic */
	subscription_done = subscribe_next_mcs_char(mcs_inst, conn);

	if (subscription_done) {
		params->subscribe = NULL;
#ifdef CONFIG_BT_MCC_OTS
		/* Start discovery of included services to find OTS */
		discover_included(mcs_inst, conn);
#else
		/* If OTS is not configured, discovery ends here */
		discovery_complete(conn, 0);
#endif /* CONFIG_BT_MCC_OTS */
	}
}

/* Subscribe to a characteristic - helper function */
static int do_subscribe(struct mcs_instance_t *mcs_inst, struct bt_conn *conn,
			uint16_t handle,
			struct bt_gatt_subscribe_params *sub_params)
{
	/* With ccc_handle == 0 it will use auto discovery */
	sub_params->ccc_handle = 0;
	sub_params->end_handle = mcs_inst->end_handle;
	sub_params->value_handle = handle;
	sub_params->notify = mcs_notify_handler;
	sub_params->subscribe = subscribe_mcs_char_func;
	/* disc_params pointer is also used as subscription flag */
	sub_params->disc_params = &mcs_inst->discover_params;
	atomic_set_bit(sub_params->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	LOG_DBG("Subscring to handle %d", handle);
	return bt_gatt_subscribe(conn, sub_params);
}

/* Subscribe to the next GMCS CCCD.
 * @return true if there are no more characteristics to subscribe to
 */
static bool subscribe_next_mcs_char(struct mcs_instance_t *mcs_inst,
				    struct bt_conn *conn)
{
	struct bt_gatt_subscribe_params *sub_params = NULL;
	uint16_t handle;

	/* The characteristics may be in any order on the server, and
	 * not all of them may exist => need to check all.
	 * For each of the subscribable characteristics
	 * - check if we have a handle for it
	 * - check sub_params.disc_params pointer to see if we have
	 *   already subscribed to it (set in do_subscribe() ).
	 */

	if (mcs_inst->player_name_handle &&
	    mcs_inst->player_name_sub_params.value &&
	    mcs_inst->player_name_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->player_name_sub_params;
		handle = mcs_inst->player_name_handle;
	} else if (mcs_inst->track_changed_handle &&
		   mcs_inst->track_changed_sub_params.value &&
		   mcs_inst->track_changed_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->track_changed_sub_params;
		handle = mcs_inst->track_changed_handle;
#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE_ENABLE_SUBSCRIPTION)
	} else if (mcs_inst->track_title_handle &&
		   mcs_inst->track_title_sub_params.value &&
		   mcs_inst->track_title_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->track_title_sub_params;
		handle = mcs_inst->track_title_handle;
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE_ENABLE_SUBSCRIPTION) */
#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
	} else if (mcs_inst->track_duration_handle &&
		   mcs_inst->track_duration_sub_params.value &&
		   mcs_inst->track_duration_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->track_duration_sub_params;
		handle = mcs_inst->track_duration_handle;
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */
#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
	} else if (mcs_inst->track_position_handle &&
		   mcs_inst->track_position_sub_params.value &&
		   mcs_inst->track_position_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->track_position_sub_params;
		handle = mcs_inst->track_position_handle;
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */
#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
	} else if (mcs_inst->playback_speed_handle &&
		   mcs_inst->playback_speed_sub_params.value &&
		   mcs_inst->playback_speed_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->playback_speed_sub_params;
		handle = mcs_inst->playback_speed_handle;
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) */
#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
	} else if (mcs_inst->seeking_speed_handle &&
		   mcs_inst->seeking_speed_sub_params.value &&
		   mcs_inst->seeking_speed_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->seeking_speed_sub_params;
		handle = mcs_inst->seeking_speed_handle;
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */
#ifdef CONFIG_BT_MCC_OTS
	} else if (mcs_inst->current_track_obj_id_handle &&
		   mcs_inst->current_track_obj_sub_params.value &&
		   mcs_inst->current_track_obj_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->current_track_obj_sub_params;
		handle = mcs_inst->current_track_obj_id_handle;
	} else if (mcs_inst->next_track_obj_id_handle &&
		   mcs_inst->next_track_obj_sub_params.value &&
		   mcs_inst->next_track_obj_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->next_track_obj_sub_params;
		handle = mcs_inst->next_track_obj_id_handle;
	} else if (mcs_inst->parent_group_obj_id_handle &&
		   mcs_inst->parent_group_obj_sub_params.value &&
		   mcs_inst->parent_group_obj_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->parent_group_obj_sub_params;
		handle = mcs_inst->parent_group_obj_id_handle;
	} else if (mcs_inst->current_group_obj_id_handle &&
		   mcs_inst->current_group_obj_sub_params.value &&
		   mcs_inst->current_group_obj_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->current_group_obj_sub_params;
		handle = mcs_inst->current_group_obj_id_handle;
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
	} else if (mcs_inst->playing_order_handle &&
		   mcs_inst->playing_order_sub_params.value &&
		   mcs_inst->playing_order_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->playing_order_sub_params;
		handle = mcs_inst->playing_order_handle;
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */
#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
	} else if (mcs_inst->media_state_handle &&
		  mcs_inst->media_state_sub_params.value &&
		  mcs_inst->media_state_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->media_state_sub_params;
		handle = mcs_inst->media_state_handle;
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */
	} else if (mcs_inst->cp_handle &&
		   mcs_inst->cp_sub_params.value &&
		   mcs_inst->cp_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->cp_sub_params;
		handle = mcs_inst->cp_handle;
#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
	} else if (mcs_inst->opcodes_supported_handle &&
		   mcs_inst->opcodes_supported_sub_params.value &&
		   mcs_inst->opcodes_supported_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->opcodes_supported_sub_params;
		handle = mcs_inst->opcodes_supported_handle;
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */
#ifdef CONFIG_BT_MCC_OTS
	} else if (mcs_inst->scp_handle &&
		   mcs_inst->scp_sub_params.value &&
		   mcs_inst->scp_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->scp_sub_params;
		handle = mcs_inst->scp_handle;
	} else if (mcs_inst->search_results_obj_id_handle &&
		   mcs_inst->search_results_obj_sub_params.value &&
		   mcs_inst->search_results_obj_sub_params.disc_params == NULL) {
		sub_params = &mcs_inst->search_results_obj_sub_params;
		handle = mcs_inst->search_results_obj_id_handle;
#endif /* CONFIG_BT_MCC_OTS */
	}

	if (sub_params != NULL) {
		const int err = do_subscribe(mcs_inst, conn, handle,
					     sub_params);

		if (err) {
			LOG_DBG("Could not subscribe: %d", err);
			discovery_complete(conn, err);
		}

		return false;
	}

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
	struct mcs_instance_t *mcs_inst = CONTAINER_OF(params,
						       struct mcs_instance_t,
						       discover_params);
	struct bt_gatt_chrc *chrc;
	bool subscription_done = true;

	if (attr) {
		/* Found an attribute */
		LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

		if (params->type != BT_GATT_DISCOVER_CHARACTERISTIC) {
			/* But it was not a characteristic - continue search */
			return BT_GATT_ITER_CONTINUE;
		}

		/* We have found an attribute, and it is a characteristic */
		/* Find out which attribute, and subscribe if we should */
		chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PLAYER_NAME)) {
			LOG_DBG("Player name, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->player_name_handle = chrc->value_handle;
			/* Use discovery params pointer as subscription flag */
			mcs_inst->player_name_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->player_name_sub_params.value = BT_GATT_CCC_NOTIFY;
			}
#ifdef CONFIG_BT_MCC_OTS
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_ICON_OBJ_ID)) {
			LOG_DBG("Icon Object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->icon_obj_id_handle = chrc->value_handle;
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL)
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_ICON_URL)) {
			LOG_DBG("Icon URL, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->icon_url_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL) */
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_CHANGED)) {
			LOG_DBG("Track Changed, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->track_changed_handle = chrc->value_handle;
			mcs_inst->track_changed_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->track_changed_sub_params.value = BT_GATT_CCC_NOTIFY;
			}
#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE)
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_TITLE)) {
			LOG_DBG("Track Title, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->track_title_handle = chrc->value_handle;
#if defined(BT_MCC_READ_TRACK_TITLE_ENABLE_SUBSCRIPTION)
			mcs_inst->track_title_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->track_title_sub_params.value = BT_GATT_CCC_NOTIFY;
			}
#endif /* defined(BT_MCC_READ_TRACK_TITLE_ENABLE_SUBSCRIPTION) */
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE) */
#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_DURATION)) {
			LOG_DBG("Track Duration, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->track_duration_handle = chrc->value_handle;
			mcs_inst->track_duration_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->track_duration_sub_params.value = BT_GATT_CCC_NOTIFY;
			}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */
#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION) || defined(CONFIG_BT_MCC_SET_TRACK_POSITION)
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_POSITION)) {
			LOG_DBG("Track Position, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->track_position_handle = chrc->value_handle;
#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
			mcs_inst->track_position_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->track_position_sub_params.value = BT_GATT_CCC_NOTIFY;
			}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) || defined(CONFIG_BT_MCC_SET_TRACK_POSITION) */
#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED) || defined(CONFIG_BT_MCC_SET_PLAYBACK_SPEED)
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PLAYBACK_SPEED)) {
			LOG_DBG("Playback Speed, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->playback_speed_handle = chrc->value_handle;
#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
			mcs_inst->playback_speed_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->playback_speed_sub_params.value = BT_GATT_CCC_NOTIFY;
			}
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) */
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) ||
	* defined (CONFIG_BT_MCC_SET_PLAYBACK_SPEED)
	*/
#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_SEEKING_SPEED)) {
			LOG_DBG("Seeking Speed, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->seeking_speed_handle = chrc->value_handle;
			mcs_inst->seeking_speed_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->seeking_speed_sub_params.value = BT_GATT_CCC_NOTIFY;
			}
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */
#ifdef CONFIG_BT_MCC_OTS
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_TRACK_SEGMENTS_OBJ_ID)) {
			LOG_DBG("Track Segments Object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->segments_obj_id_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_CURRENT_TRACK_OBJ_ID)) {
			LOG_DBG("Current Track Object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->current_track_obj_id_handle = chrc->value_handle;
			mcs_inst->current_track_obj_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->current_track_obj_sub_params.value =
					BT_GATT_CCC_NOTIFY;
			}
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_NEXT_TRACK_OBJ_ID)) {
			LOG_DBG("Next Track Object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->next_track_obj_id_handle = chrc->value_handle;
			mcs_inst->next_track_obj_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->next_track_obj_sub_params.value = BT_GATT_CCC_NOTIFY;
			}
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PARENT_GROUP_OBJ_ID)) {
			LOG_DBG("Parent Group Object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->parent_group_obj_id_handle = chrc->value_handle;
			mcs_inst->parent_group_obj_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->parent_group_obj_sub_params.value =
					BT_GATT_CCC_NOTIFY;
			}
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_CURRENT_GROUP_OBJ_ID)) {
			LOG_DBG("Group Object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->current_group_obj_id_handle = chrc->value_handle;
			mcs_inst->current_group_obj_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->current_group_obj_sub_params.value =
					BT_GATT_CCC_NOTIFY;
			}
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) || defined(CONFIG_BT_MCC_SET_PLAYING_ORDER)
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PLAYING_ORDER)) {
			LOG_DBG("Playing Order, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->playing_order_handle = chrc->value_handle;
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
			mcs_inst->playing_order_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->playing_order_sub_params.value = BT_GATT_CCC_NOTIFY;
			}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) || defined(CONFIG_BT_MCC_SET_PLAYING_ORDER) */
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED)
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_PLAYING_ORDERS)) {
			LOG_DBG("Playing Orders supported, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->playing_orders_supported_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED) */
#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_MEDIA_STATE)) {
			LOG_DBG("Media State, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->media_state_handle = chrc->value_handle;
			mcs_inst->media_state_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->media_state_sub_params.value = BT_GATT_CCC_NOTIFY;
			}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_MEDIA_CONTROL_POINT)) {
			LOG_DBG("Media Control Point, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->cp_handle = chrc->value_handle;
			mcs_inst->cp_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->cp_sub_params.value = BT_GATT_CCC_NOTIFY;
			}
#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_MEDIA_CONTROL_OPCODES)) {
			LOG_DBG("Media control opcodes supported, UUID: %s",
			       bt_uuid_str(chrc->uuid));
			mcs_inst->opcodes_supported_handle = chrc->value_handle;
			mcs_inst->opcodes_supported_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->opcodes_supported_sub_params.value =
					BT_GATT_CCC_NOTIFY;
			}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */
#ifdef CONFIG_BT_MCC_OTS
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_SEARCH_CONTROL_POINT)) {
			LOG_DBG("Search control point, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->scp_handle = chrc->value_handle;
			mcs_inst->scp_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->scp_sub_params.value = BT_GATT_CCC_NOTIFY;
			}
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID)) {
			LOG_DBG("Search Results object, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->search_results_obj_id_handle = chrc->value_handle;
			mcs_inst->search_results_obj_sub_params.disc_params = NULL;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				mcs_inst->search_results_obj_sub_params.value =
					BT_GATT_CCC_NOTIFY;
			}
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID)
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_CCID)) {
			LOG_DBG("Content Control ID, UUID: %s", bt_uuid_str(chrc->uuid));
			mcs_inst->content_control_id_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID) */
		}


		/* Continue to search for more attributes */
		return BT_GATT_ITER_CONTINUE;
	}

	/* No more attributes found */
	LOG_DBG("GMCS characteristics found");
	(void)memset(params, 0, sizeof(*params));

	/* Either subscribe to characteristics, or continue to discovery of
	 *included services.
	 * Subscription is done after discovery, not in parallel with it,
	 * to avoid queuing many ATT requests that requires buffers.
	 */
	if (subscribe_all) {
		subscription_done = subscribe_next_mcs_char(mcs_inst, conn);
	}

	if (subscription_done) {
		/* Not subscribing, or there was nothing to subscribe to */
#ifdef CONFIG_BT_MCC_OTS
		/* Start discovery of included services to find OTS */
		discover_included(mcs_inst, conn);
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
		struct mcs_instance_t *mcs_inst = CONTAINER_OF(params,
							       struct mcs_instance_t,
							       discover_params);
		int err;
		/* Found an attribute */
		LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

		if (params->type != BT_GATT_DISCOVER_PRIMARY) {
			/* But it was not a primary service - continue search */
			LOG_WRN("Unexpected parameters");
			return BT_GATT_ITER_CONTINUE;
		}

		/* We have found an attribute, and it is a primary service */
		/* (Must be GMCS, since that is the one we searched for.) */
		LOG_DBG("Primary discovery complete");
		LOG_DBG("UUID: %s", bt_uuid_str(attr->uuid));
		prim_service = (struct bt_gatt_service_val *)attr->user_data;
		LOG_DBG("UUID: %s", bt_uuid_str(prim_service->uuid));

		mcs_inst->start_handle = attr->handle + 1;
		mcs_inst->end_handle = prim_service->end_handle;

		/* Start discovery of characteristics */
		mcs_inst->discover_params.uuid = NULL;
		mcs_inst->discover_params.start_handle = mcs_inst->start_handle;
		mcs_inst->discover_params.end_handle = mcs_inst->end_handle;
		mcs_inst->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		mcs_inst->discover_params.func = discover_mcs_char_func;

		LOG_DBG("Start discovery of GMCS characteristics");
		err = bt_gatt_discover(conn, &mcs_inst->discover_params);
		if (err) {
			LOG_DBG("Discovery failed: %d", err);
			discovery_complete(conn, err);
		}
		return BT_GATT_ITER_STOP;
	}

	/* No attribute of the searched for type found */
	LOG_DBG("Could not find an GMCS instance on the server");

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

	LOG_DBG("Object selected callback: %p", otc_cb.obj_selected);
	LOG_DBG("Object content callback: %p", otc_cb.obj_data_read);
	LOG_DBG("Object metadata callback: %p", otc_cb.obj_metadata_read);
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
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(!conn) {
		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		/* TODO: Need to find existing or new MCS instance here */
		return -EINVAL;
	}

	if (mcs_inst->busy) {
		return -EBUSY;
	}

	subscribe_all = subscribe;
	err = reset_mcs_inst(mcs_inst);
	if (err != 0) {
		LOG_DBG("Failed to reset MCS instance %p: %d", mcs_inst, err);

		return err;
	}
	(void)memcpy(&uuid, BT_UUID_GMCS, sizeof(uuid));

	mcs_inst->discover_params.func = discover_primary_func;
	mcs_inst->discover_params.uuid = &uuid.uuid;
	mcs_inst->discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	mcs_inst->discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	mcs_inst->discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	LOG_DBG("start discovery of GMCS primary service");
	err = bt_gatt_discover(conn, &mcs_inst->discover_params);
	if (err != 0) {
		return err;
	}

	mcs_inst->conn = bt_conn_ref(conn);
	mcs_inst->busy = true;

	return 0;
}

int bt_mcc_read_player_name(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->player_name_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_player_name_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->player_name_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}


#ifdef CONFIG_BT_MCC_OTS
int bt_mcc_read_icon_obj_id(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->icon_obj_id_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_icon_obj_id_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->icon_obj_id_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL)
int bt_mcc_read_icon_url(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->icon_url_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_icon_url_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->icon_url_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL) */

#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE)
int bt_mcc_read_track_title(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->track_title_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_track_title_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->track_title_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE) */

#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
int bt_mcc_read_track_duration(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->track_duration_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_track_duration_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->track_duration_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */

#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
int bt_mcc_read_track_position(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->track_position_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_track_position_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->track_position_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */

#if defined(CONFIG_BT_MCC_SET_TRACK_POSITION)
int bt_mcc_set_track_position(struct bt_conn *conn, int32_t pos)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->track_position_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	(void)memcpy(mcs_inst->write_buf, &pos, sizeof(pos));

	mcs_inst->write_params.offset = 0;
	mcs_inst->write_params.data = mcs_inst->write_buf;
	mcs_inst->write_params.length = sizeof(pos);
	mcs_inst->write_params.handle = mcs_inst->track_position_handle;
	mcs_inst->write_params.func = mcs_write_track_position_cb;

	LOG_HEXDUMP_DBG(mcs_inst->write_params.data, sizeof(pos), "Track position sent");

	err = bt_gatt_write(conn, &mcs_inst->write_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined(CONFIG_BT_MCC_SET_TRACK_POSITION) */

#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
int bt_mcc_read_playback_speed(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->playback_speed_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_playback_speed_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->playback_speed_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) */

#if defined(CONFIG_BT_MCC_SET_PLAYBACK_SPEED)
int bt_mcc_set_playback_speed(struct bt_conn *conn, int8_t speed)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->playback_speed_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	(void)memcpy(mcs_inst->write_buf, &speed, sizeof(speed));

	mcs_inst->write_params.offset = 0;
	mcs_inst->write_params.data = mcs_inst->write_buf;
	mcs_inst->write_params.length = sizeof(speed);
	mcs_inst->write_params.handle = mcs_inst->playback_speed_handle;
	mcs_inst->write_params.func = mcs_write_playback_speed_cb;

	LOG_HEXDUMP_DBG(mcs_inst->write_params.data, sizeof(speed), "Playback speed");

	err = bt_gatt_write(conn, &mcs_inst->write_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined (CONFIG_BT_MCC_SET_PLAYBACK_SPEED) */

#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
int bt_mcc_read_seeking_speed(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->seeking_speed_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_seeking_speed_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->seeking_speed_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */

#ifdef CONFIG_BT_MCC_OTS
int bt_mcc_read_segments_obj_id(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->segments_obj_id_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_segments_obj_id_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->segments_obj_id_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_current_track_obj_id(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->current_track_obj_id_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_current_track_obj_id_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->current_track_obj_id_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_set_current_track_obj_id(struct bt_conn *conn, uint64_t obj_id)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	CHECKIF(!BT_MCS_VALID_OBJ_ID(obj_id)) {
		LOG_DBG("Object ID 0x%016llx invalid", obj_id);
		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->current_track_obj_id_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	sys_put_le48(obj_id, mcs_inst->write_buf);
	mcs_inst->write_params.offset = 0;
	mcs_inst->write_params.data = mcs_inst->write_buf;
	mcs_inst->write_params.length = BT_OTS_OBJ_ID_SIZE;
	mcs_inst->write_params.handle = mcs_inst->current_track_obj_id_handle;
	mcs_inst->write_params.func = mcs_write_current_track_obj_id_cb;

	LOG_HEXDUMP_DBG(mcs_inst->write_params.data, BT_OTS_OBJ_ID_SIZE, "Object Id");

	err = bt_gatt_write(conn, &mcs_inst->write_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_next_track_obj_id(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->next_track_obj_id_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_next_track_obj_id_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->next_track_obj_id_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_set_next_track_obj_id(struct bt_conn *conn, uint64_t obj_id)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	CHECKIF(!BT_MCS_VALID_OBJ_ID(obj_id)) {
		LOG_DBG("Object ID 0x%016llx invalid", obj_id);
		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->next_track_obj_id_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	sys_put_le48(obj_id, mcs_inst->write_buf);
	mcs_inst->write_params.offset = 0;
	mcs_inst->write_params.data = mcs_inst->write_buf;
	mcs_inst->write_params.length = BT_OTS_OBJ_ID_SIZE;
	mcs_inst->write_params.handle = mcs_inst->next_track_obj_id_handle;
	mcs_inst->write_params.func = mcs_write_next_track_obj_id_cb;

	LOG_HEXDUMP_DBG(mcs_inst->write_params.data, BT_OTS_OBJ_ID_SIZE, "Object Id");

	err = bt_gatt_write(conn, &mcs_inst->write_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_parent_group_obj_id(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->parent_group_obj_id_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_parent_group_obj_id_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->parent_group_obj_id_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_current_group_obj_id(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->current_group_obj_id_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_current_group_obj_id_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->current_group_obj_id_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_set_current_group_obj_id(struct bt_conn *conn, uint64_t obj_id)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	CHECKIF(!BT_MCS_VALID_OBJ_ID(obj_id)) {
		LOG_DBG("Object ID 0x%016llx invalid", obj_id);
		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->current_group_obj_id_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	sys_put_le48(obj_id, mcs_inst->write_buf);
	mcs_inst->write_params.offset = 0;
	mcs_inst->write_params.data = mcs_inst->write_buf;
	mcs_inst->write_params.length = BT_OTS_OBJ_ID_SIZE;
	mcs_inst->write_params.handle = mcs_inst->current_group_obj_id_handle;
	mcs_inst->write_params.func = mcs_write_current_group_obj_id_cb;

	LOG_HEXDUMP_DBG(mcs_inst->write_params.data, BT_OTS_OBJ_ID_SIZE, "Object Id");

	err = bt_gatt_write(conn, &mcs_inst->write_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
int bt_mcc_read_playing_order(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->playing_order_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_playing_order_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->playing_order_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_SET_PLAYING_ORDER)
int bt_mcc_set_playing_order(struct bt_conn *conn, uint8_t order)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->playing_order_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	CHECKIF(!IN_RANGE(order,
			  BT_MCS_PLAYING_ORDER_SINGLE_ONCE,
			  BT_MCS_PLAYING_ORDER_SHUFFLE_REPEAT)) {
		LOG_DBG("Invalid playing order 0x%02X", order);

		return -EINVAL;
	}

	(void)memcpy(mcs_inst->write_buf, &order, sizeof(order));

	mcs_inst->write_params.offset = 0;
	mcs_inst->write_params.data = mcs_inst->write_buf;
	mcs_inst->write_params.length = sizeof(order);
	mcs_inst->write_params.handle = mcs_inst->playing_order_handle;
	mcs_inst->write_params.func = mcs_write_playing_order_cb;

	LOG_HEXDUMP_DBG(mcs_inst->write_params.data, sizeof(order), "Playing order");

	err = bt_gatt_write(conn, &mcs_inst->write_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined(CONFIG_BT_MCC_SET_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED)
int bt_mcc_read_playing_orders_supported(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->playing_orders_supported_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_playing_orders_supported_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->playing_orders_supported_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED) */

#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
int bt_mcc_read_media_state(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->media_state_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_media_state_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->media_state_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */

#if defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT)
int bt_mcc_send_cmd(struct bt_conn *conn, const struct mpl_cmd *cmd)
{
	struct mcs_instance_t *mcs_inst;
	size_t length;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	CHECKIF(cmd == NULL) {
		LOG_DBG("cmd is NULL");

		return -EINVAL;
	}

	CHECKIF(!BT_MCS_VALID_OP(cmd->opcode)) {
		LOG_DBG("Opcode 0x%02X is invalid", cmd->opcode);

		return -EINVAL;
	}

	length = sizeof(cmd->opcode);
	(void)memcpy(mcs_inst->write_buf, &cmd->opcode, length);
	if (cmd->use_param) {
		length += sizeof(cmd->param);
		(void)memcpy(&mcs_inst->write_buf[sizeof(cmd->opcode)], &cmd->param,
		       sizeof(cmd->param));
	}

	mcs_inst->write_params.offset = 0;
	mcs_inst->write_params.data = mcs_inst->write_buf;
	mcs_inst->write_params.length = length;
	mcs_inst->write_params.handle = mcs_inst->cp_handle;
	mcs_inst->write_params.func = mcs_write_cp_cb;

	LOG_HEXDUMP_DBG(mcs_inst->write_params.data, sizeof(*cmd), "Command sent");

	err = bt_gatt_write(conn, &mcs_inst->write_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT) */

#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
int bt_mcc_read_opcodes_supported(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->opcodes_supported_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_opcodes_supported_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->opcodes_supported_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */

#ifdef CONFIG_BT_MCC_OTS
int bt_mcc_send_search(struct bt_conn *conn, const struct mpl_search *search)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->scp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	CHECKIF(search == NULL) {
		LOG_DBG("search is NULL");

		return -EINVAL;
	}

	CHECKIF(!IN_RANGE(search->len, SEARCH_LEN_MIN, SEARCH_LEN_MAX)) {
		LOG_DBG("Invalid search->len: %u", search->len);

		return -EINVAL;
	}

	(void)memcpy(mcs_inst->write_buf, &search->search, search->len);

	mcs_inst->write_params.offset = 0;
	mcs_inst->write_params.data = mcs_inst->write_buf;
	mcs_inst->write_params.length = search->len;
	mcs_inst->write_params.handle = mcs_inst->scp_handle;
	mcs_inst->write_params.func = mcs_write_scp_cb;

	LOG_HEXDUMP_DBG(mcs_inst->write_params.data, search->len, "Search sent");

	err = bt_gatt_write(conn, &mcs_inst->write_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}

int bt_mcc_read_search_results_obj_id(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->search_results_obj_id_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_search_results_obj_id_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->search_results_obj_id_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID)
int bt_mcc_read_content_control_id(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	} else if (mcs_inst->content_control_id_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	mcs_inst->read_params.func = mcc_read_content_control_id_cb;
	mcs_inst->read_params.handle_count = 1;
	mcs_inst->read_params.single.handle = mcs_inst->content_control_id_handle;
	mcs_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mcs_inst->read_params);
	if (!err) {
		mcs_inst->busy = true;
	}
	return err;
}
#endif /* defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID) */

#ifdef CONFIG_BT_MCC_OTS

void on_obj_selected(struct bt_ots_client *otc_inst,
		     struct bt_conn *conn, int result)
{
	LOG_DBG("Current object selected");
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

	LOG_DBG("Received Media Player Icon content, %i bytes at offset %i",
		len, offset);

	LOG_HEXDUMP_DBG(data_p, len, "Icon content");

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		LOG_WRN("Can not fit whole object");
		cb_err = -EMSGSIZE;
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		LOG_DBG("Icon object received");

		if (mcc_cb && mcc_cb->otc_icon_object) {
			mcc_cb->otc_icon_object(conn, cb_err, &otc_obj_buf);
		}
		/* Reset buf in case the same object is read again without */
		/* calling select in between */
		net_buf_simple_reset(&otc_obj_buf);
	}

	return BT_OTS_CONTINUE;
}

#if CONFIG_BT_MCC_LOG_LEVEL_DBG
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
			LOG_WRN("Segment too long");
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
#endif /* CONFIG_BT_MCC_LOG_LEVEL_DBG */

int on_track_segments_content(struct bt_ots_client *otc_inst,
			      struct bt_conn *conn, uint32_t offset,
			      uint32_t len, uint8_t *data_p, bool is_complete)
{
	int cb_err = 0;

	LOG_DBG("Received Track Segments content, %i bytes at offset %i",
		len, offset);

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		LOG_WRN("Can not fit whole object");
		cb_err = -EMSGSIZE;
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		LOG_DBG("Track segment object received");

#if CONFIG_BT_MCC_LOG_LEVEL_DBG
		struct track_segs_t track_segments;

		track_segments.cnt = 0;
		decode_track_segments(&otc_obj_buf, &track_segments);
		for (int i = 0; i < track_segments.cnt; i++) {
			LOG_DBG("Track segment %i:", i);
			LOG_DBG("\t-Name\t:%s",
			       track_segments.segs[i].name);
			LOG_DBG("\t-Position\t:%d", track_segments.segs[i].pos);
		}
#endif /* CONFIG_BT_MCC_LOG_LEVEL_DBG */

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

	LOG_DBG("Received Current Track content, %i bytes at offset %i",
	       len, offset);

	LOG_HEXDUMP_DBG(data_p, len, "Track content");

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		LOG_WRN("Can not fit whole object");
		cb_err = -EMSGSIZE;
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		LOG_DBG("Current Track Object received");

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

	LOG_DBG("Received Next Track content, %i bytes at offset %i",
	       len, offset);

	LOG_HEXDUMP_DBG(data_p, len, "Track content");

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		LOG_WRN("Can not fit whole object");
		cb_err = -EMSGSIZE;
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		LOG_DBG("Next Track Object received");

		if (mcc_cb && mcc_cb->otc_next_track_object) {
			mcc_cb->otc_next_track_object(conn, cb_err, &otc_obj_buf);
		}

		net_buf_simple_reset(&otc_obj_buf);
	}

	return BT_OTS_CONTINUE;
}


#if CONFIG_BT_MCC_LOG_LEVEL_DBG
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
#endif /* CONFIG_BT_MCC_LOG_LEVEL_DBG */

int on_parent_group_content(struct bt_ots_client *otc_inst,
			    struct bt_conn *conn, uint32_t offset,
			    uint32_t len, uint8_t *data_p, bool is_complete)
{
	int cb_err = 0;

	LOG_DBG("Received Parent Group content, %i bytes at offset %i",
		len, offset);

	LOG_HEXDUMP_DBG(data_p, len, "Group content");

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		LOG_WRN("Can not fit whole object");
		cb_err = -EMSGSIZE;
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		LOG_DBG("Parent Group object received");

#if CONFIG_BT_MCC_LOG_LEVEL_DBG
		struct id_list_t group = {0};

		decode_group(&otc_obj_buf, &group);
		for (int i = 0; i < group.cnt; i++) {
			char t[BT_OTS_OBJ_ID_STR_LEN];

			(void)bt_ots_obj_id_to_str(group.ids[i].id, t,
						   BT_OTS_OBJ_ID_STR_LEN);
			LOG_DBG("Object type: %d, object  ID: %s",
			       group.ids[i].type, t);
		}
#endif /* CONFIG_BT_MCC_LOG_LEVEL_DBG */

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

	LOG_DBG("Received Current Group content, %i bytes at offset %i",
		len, offset);

	LOG_HEXDUMP_DBG(data_p, len, "Group content");

	if (len > net_buf_simple_tailroom(&otc_obj_buf)) {
		LOG_WRN("Can not fit whole object");
		cb_err = -EMSGSIZE;
	}

	net_buf_simple_add_mem(&otc_obj_buf, data_p,
			       MIN(net_buf_simple_tailroom(&otc_obj_buf), len));

	if (is_complete) {
		LOG_DBG("Current Group object received");

#if CONFIG_BT_MCC_LOG_LEVEL_DBG
		struct id_list_t group = {0};

		decode_group(&otc_obj_buf, &group);
		for (int i = 0; i < group.cnt; i++) {
			char t[BT_OTS_OBJ_ID_STR_LEN];

			(void)bt_ots_obj_id_to_str(group.ids[i].id, t,
						   BT_OTS_OBJ_ID_STR_LEN);
			LOG_DBG("Object type: %d, object  ID: %s",
			       group.ids[i].type, t);
		}
#endif /* CONFIG_BT_MCC_LOG_LEVEL_DBG */

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
	LOG_INF("Object's meta data:");
	LOG_INF("\tCurrent size\t:%u", otc_inst->cur_object.size.cur);

	if (otc_inst->cur_object.size.cur > otc_obj_buf.size) {
		LOG_DBG("Object larger than allocated buffer");
	}

	bt_ots_metadata_display(&otc_inst->cur_object, 1);

	if (mcc_cb && mcc_cb->otc_obj_metadata) {
		mcc_cb->otc_obj_metadata(conn, err);
	}
}

int bt_mcc_otc_read_object_metadata(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	}

	err = bt_ots_client_read_object_metadata(&mcs_inst->otc, conn,
						 BT_OTS_METADATA_REQ_ALL);
	if (err) {
		LOG_DBG("Error reading the object: %d", err);
	}

	return err;
}


int bt_mcc_otc_read_icon_object(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	}

	mcs_inst->otc.cb->obj_data_read = on_icon_content;

	err = bt_ots_client_read_object_data(&mcs_inst->otc, conn);
	if (err) {
		LOG_DBG("Error reading the object: %d", err);
	}

	return err;
}

int bt_mcc_otc_read_track_segments_object(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	}

	/* TODO: Assumes object is already selected */
	mcs_inst->otc.cb->obj_data_read = on_track_segments_content;

	err = bt_ots_client_read_object_data(&mcs_inst->otc, conn);
	if (err) {
		LOG_DBG("Error reading the object: %d", err);
	}

	return err;
}

int bt_mcc_otc_read_current_track_object(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	}

	/* TODO: Assumes object is already selected */
	mcs_inst->otc.cb->obj_data_read = on_current_track_content;

	err = bt_ots_client_read_object_data(&mcs_inst->otc, conn);
	if (err) {
		LOG_DBG("Error reading the object: %d", err);
	}

	return err;
}

int bt_mcc_otc_read_next_track_object(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	}

	/* TODO: Assumes object is already selected */
	mcs_inst->otc.cb->obj_data_read = on_next_track_content;

	err = bt_ots_client_read_object_data(&mcs_inst->otc, conn);
	if (err) {
		LOG_DBG("Error reading the object: %d", err);
	}

	return err;
}

int bt_mcc_otc_read_parent_group_object(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	}

	/* TODO: Assumes object is already selected */

	/* Reuse callback for current group */
	mcs_inst->otc.cb->obj_data_read = on_parent_group_content;

	err = bt_ots_client_read_object_data(&mcs_inst->otc, conn);
	if (err) {
		LOG_DBG("Error reading the object: %d", err);
	}

	return err;
}

int bt_mcc_otc_read_current_group_object(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		LOG_DBG("Could not lookup mcs_inst from conn %p", (void *)conn);

		return -EINVAL;
	} else if (mcs_inst->busy) {

		LOG_DBG("mcs_inst busy");
		return -EBUSY;
	}

	/* TODO: Assumes object is already selected */
	mcs_inst->otc.cb->obj_data_read = on_current_group_content;

	err = bt_ots_client_read_object_data(&mcs_inst->otc, conn);
	if (err) {
		LOG_DBG("Error reading the object: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_MCC_SHELL)
struct bt_ots_client *bt_mcc_otc_inst(struct bt_conn *conn)
{
	struct mcs_instance_t *mcs_inst;

	mcs_inst = lookup_inst_by_conn(conn);
	if (mcs_inst == NULL) {
		return NULL;
	}

	return &mcs_inst->otc;
}
#endif /* defined(CONFIG_BT_MCC_SHELL) */

#endif /* CONFIG_BT_MCC_OTS */
