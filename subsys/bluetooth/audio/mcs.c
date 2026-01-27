/** @file
 *  @brief Bluetooth Media Control Service
 */

/*
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/mcs.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>

#include "audio_internal.h"
#include "common/bt_str.h"
#include "media_proxy_internal.h"
#include "mcs_internal.h"

LOG_MODULE_REGISTER(bt_mcs, CONFIG_BT_MCS_LOG_LEVEL);

/* The value 1000 was arbitrarily chosen. No action should take longer than this, and using a
 * non-K_FOREVER value may help catch any programming issues as locking the mutexes may trigger
 * assertions
 */
#define MUTEX_TIMEOUT K_MSEC(1000U)

static int notify(struct bt_conn *conn, const struct bt_uuid *uuid, const void *data, uint16_t len);

static struct bt_mcs_cb *mcs_cbs;

struct mcs_flags {
	bool player_name_changed: 1;
	bool player_name_dirty: 1;
	bool icon_url_dirty: 1;
	bool track_changed_changed: 1;
	bool track_title_changed: 1;
	bool track_title_dirty: 1;
	bool track_duration_changed: 1;
	bool track_position_changed: 1;
	bool playback_speed_changed: 1;
	bool seeking_speed_changed: 1;
	bool playing_order_changed: 1;
	bool media_state_changed: 1;
	bool media_control_opcodes_changed: 1;
	bool media_control_point_busy: 1;
	bool media_control_point_result: 1;
#if defined(CONFIG_BT_OTS)
	bool current_track_obj_id_changed: 1;
	bool next_track_obj_id_changed: 1;
	bool parent_group_obj_id_changed: 1;
	bool current_group_obj_id_changed: 1;
	bool search_results_obj_id_changed: 1;
	bool search_control_point_busy: 1;
	bool search_control_point_result: 1;
#endif /* CONFIG_BT_OTS */
};

static struct mcs_inst {
	struct k_mutex mutex;

	/* Client states. Access and modification of these shall be guarded by the mutex */
	struct client_state {
		struct mcs_flags flags;
		struct mpl_cmd_ntf cmd_ntf;
#if defined(CONFIG_BT_OTS)
		uint8_t search_control_point_result;
#endif /* CONFIG_BT_OTS */
	} clients[CONFIG_BT_MAX_CONN];

	struct k_work_delayable notify_work;
} mcs_inst;

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	__maybe_unused int err;

	err = k_mutex_lock(&mcs_inst.mutex, MUTEX_TIMEOUT);
	__ASSERT(err == 0, "Failed to lock mutex: %d", err);

	/* Clear data on disconnect */
	memset(&mcs_inst.clients[bt_conn_index(conn)], 0, sizeof(struct client_state));

	err = k_mutex_unlock(&mcs_inst.mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

/* Functions for reading and writing attributes, and for keeping track
 * of attribute configuration changes.
 * Functions for notifications are placed after the service definition.
 */
static ssize_t read_player_name(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_player_name == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	const char *name = mcs_cbs->get_player_name();

	LOG_DBG("Player name read: %s (offset %u)", name, offset);

	/* Conn is NULL if doing a local read */
	if (conn != NULL) {
		struct mcs_flags *flags;
		__maybe_unused int err;

		err = k_mutex_lock(&mcs_inst.mutex, MUTEX_TIMEOUT);
		__ASSERT(err == 0, "Failed to lock mutex: %d", err);

		flags = &mcs_inst.clients[bt_conn_index(conn)].flags;

		if (flags->player_name_dirty && offset != 0U) {
			err = k_mutex_unlock(&mcs_inst.mutex);
			__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

			return BT_GATT_ERR(BT_MCS_ERR_LONG_VAL_CHANGED);
		}

		flags->player_name_dirty = false;

		err = k_mutex_unlock(&mcs_inst.mutex);
		__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, name, strlen(name));
}

static void player_name_cfg_changed(const struct bt_gatt_attr *attr,
				    uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

#ifdef CONFIG_BT_OTS
static ssize_t read_icon_id(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_icon_id == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint64_t icon_id = mcs_cbs->get_icon_id();
	uint8_t icon_id_le[BT_OTS_OBJ_ID_SIZE];

	sys_put_le48(icon_id, icon_id_le);

	LOG_DBG_OBJ_ID("Icon object read: ", icon_id);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, icon_id_le,
				 sizeof(icon_id_le));
}
#endif /* CONFIG_BT_OTS */

static ssize_t read_icon_url(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_icon_url == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	const char *url = mcs_cbs->get_icon_url();

	LOG_DBG("Icon URL read, offset: %d, len:%d, URL: %s", offset, len, url);

	/* Conn is NULL if doing a local read */
	if (conn != NULL) {
		struct mcs_flags *flags;
		__maybe_unused int err;

		err = k_mutex_lock(&mcs_inst.mutex, MUTEX_TIMEOUT);
		__ASSERT(err == 0, "Failed to lock mutex: %d", err);

		flags = &mcs_inst.clients[bt_conn_index(conn)].flags;

		if (flags->icon_url_dirty && offset != 0U) {
			err = k_mutex_unlock(&mcs_inst.mutex);
			__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

			return BT_GATT_ERR(BT_MCS_ERR_LONG_VAL_CHANGED);
		}

		flags->icon_url_dirty = false;

		err = k_mutex_unlock(&mcs_inst.mutex);
		__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, url,
				 strlen(url));
}

static void track_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_track_title(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_track_title == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	const char *title = mcs_cbs->get_track_title();

	LOG_DBG("Track title read, offset: %d, len:%d, title: %s", offset, len, title);

	/* Conn is NULL if doing a local read */
	if (conn != NULL) {
		struct mcs_flags *flags;
		__maybe_unused int err;

		err = k_mutex_lock(&mcs_inst.mutex, MUTEX_TIMEOUT);
		__ASSERT(err == 0, "Failed to lock mutex: %d", err);

		flags = &mcs_inst.clients[bt_conn_index(conn)].flags;

		if (flags->track_title_dirty && offset != 0U) {
			err = k_mutex_unlock(&mcs_inst.mutex);
			__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

			return BT_GATT_ERR(BT_MCS_ERR_LONG_VAL_CHANGED);
		}

		flags->track_title_dirty = false;

		err = k_mutex_unlock(&mcs_inst.mutex);
		__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, title,
				 strlen(title));
}

static void track_title_cfg_changed(const struct bt_gatt_attr *attr,
				    uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_track_duration(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_track_duration == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	int32_t duration = mcs_cbs->get_track_duration();
	int32_t duration_le = sys_cpu_to_le32(duration);

	LOG_DBG("Track duration read: %d (0x%08x)", duration, duration);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &duration_le, sizeof(duration_le));
}

static void track_duration_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_track_position(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_track_position == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	int32_t position = mcs_cbs->get_track_position();
	int32_t position_le = sys_cpu_to_le32(position);

	LOG_DBG("Track position read: %d (0x%08x)", position, position);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &position_le,
				 sizeof(position_le));
}

static ssize_t write_track_position(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    const void *buf, uint16_t len,
				    uint16_t offset, uint8_t flags)
{
	if (mcs_cbs == NULL || mcs_cbs->get_track_position == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	int32_t position;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(position)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	position = sys_get_le32((uint8_t *)buf);

	mcs_cbs->set_track_position(position);

	LOG_DBG("Track position write: %d", position);

	return len;
}

static void track_position_cfg_changed(const struct bt_gatt_attr *attr,
				       uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_playback_speed(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_playback_speed == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	int8_t speed = mcs_cbs->get_playback_speed();

	LOG_DBG("Playback speed read: %d", speed);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &speed, sizeof(speed));
}

static ssize_t write_playback_speed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	if (mcs_cbs == NULL || mcs_cbs->set_playback_speed == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	int8_t speed;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (len != sizeof(speed)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&speed, buf, len);

	mcs_cbs->set_playback_speed(speed);

	LOG_DBG("Playback speed write: %d", speed);

	return len;
}

static void playback_speed_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_seeking_speed(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_seeking_speed == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	int8_t speed = mcs_cbs->get_seeking_speed();

	LOG_DBG("Seeking speed read: %d", speed);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &speed,
				 sizeof(speed));
}

static void seeking_speed_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

#ifdef CONFIG_BT_OTS
static ssize_t read_track_segments_id(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_track_segments_id == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint64_t track_segments_id = mcs_cbs->get_track_segments_id();
	uint8_t track_segments_id_le[BT_OTS_OBJ_ID_SIZE];

	sys_put_le48(track_segments_id, track_segments_id_le);

	LOG_DBG_OBJ_ID("Track segments ID read: ", track_segments_id);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 track_segments_id_le, sizeof(track_segments_id_le));
}

static ssize_t read_current_track_id(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr, void *buf,
				     uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_current_track_id == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint64_t track_id = mcs_cbs->get_current_track_id();
	uint8_t track_id_le[BT_OTS_OBJ_ID_SIZE];

	sys_put_le48(track_id, track_id_le);

	LOG_DBG_OBJ_ID("Current track ID read: ", track_id);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, track_id_le,
				 sizeof(track_id_le));
}

static ssize_t write_current_track_id(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      const void *buf, uint16_t len, uint16_t offset,
				      uint8_t flags)
{
	if (mcs_cbs == NULL || mcs_cbs->set_current_track_id == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint64_t id;

	if (offset != 0) {
		LOG_DBG("Invalid offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != BT_OTS_OBJ_ID_SIZE) {
		LOG_DBG("Invalid length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	id = sys_get_le48((uint8_t *)buf);

	if (IS_ENABLED(CONFIG_BT_MCS_LOG_LEVEL_DBG)) {
		char str[BT_OTS_OBJ_ID_STR_LEN];
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		LOG_DBG("Current track write: offset: %d, len: %d, track ID: %s", offset, len, str);
	}

	mcs_cbs->set_current_track_id(id);

	return BT_OTS_OBJ_ID_SIZE;
}

static void current_track_id_cfg_changed(const struct bt_gatt_attr *attr,
					 uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_next_track_id(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_next_track_id == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint64_t track_id = mcs_cbs->get_next_track_id();
	uint8_t track_id_le[BT_OTS_OBJ_ID_SIZE];

	sys_put_le48(track_id, track_id_le);

	if (track_id == MPL_NO_TRACK_ID) {
		LOG_DBG("Next track read, but it is empty");
		/* "If the media player has no next track, the length of the */
		/* characteristic shall be zero." */
		return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
	}

	LOG_DBG_OBJ_ID("Next track read: ", track_id);
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 track_id_le, sizeof(track_id_le));
}

static ssize_t write_next_track_id(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset,
				   uint8_t flags)
{
	if (mcs_cbs == NULL || mcs_cbs->set_next_track_id == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint64_t id;

	if (offset != 0) {
		LOG_DBG("Invalid offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != BT_OTS_OBJ_ID_SIZE) {
		LOG_DBG("Invalid length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	id = sys_get_le48((uint8_t *)buf);

	if (IS_ENABLED(CONFIG_BT_MCS_LOG_LEVEL_DBG)) {
		char str[BT_OTS_OBJ_ID_STR_LEN];
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		LOG_DBG("Next  track write: offset: %d, len: %d, track ID: %s", offset, len, str);
	}

	mcs_cbs->set_next_track_id(id);

	return BT_OTS_OBJ_ID_SIZE;
}

static void next_track_id_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_parent_group_id(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr, void *buf,
				    uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_parent_group_id == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint64_t group_id = mcs_cbs->get_parent_group_id();
	uint8_t group_id_le[BT_OTS_OBJ_ID_SIZE];

	sys_put_le48(group_id, group_id_le);

	LOG_DBG_OBJ_ID("Parent group read: ", group_id);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, group_id_le,
				 sizeof(group_id_le));
}

static void parent_group_id_cfg_changed(const struct bt_gatt_attr *attr,
					uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_current_group_id(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr, void *buf,
				     uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_current_group_id == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint64_t group_id = mcs_cbs->get_current_group_id();
	uint8_t group_id_le[BT_OTS_OBJ_ID_SIZE];

	sys_put_le48(group_id, group_id_le);

	LOG_DBG_OBJ_ID("Current group read: ", group_id);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, group_id_le,
				 sizeof(group_id_le));
}

static ssize_t write_current_group_id(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      const void *buf, uint16_t len, uint16_t offset,
				      uint8_t flags)
{
	if (mcs_cbs == NULL || mcs_cbs->set_current_group_id == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint64_t id;

	if (offset != 0) {
		LOG_DBG("Invalid offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != BT_OTS_OBJ_ID_SIZE) {
		LOG_DBG("Invalid length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	id = sys_get_le48((uint8_t *)buf);

	if (IS_ENABLED(CONFIG_BT_MCS_LOG_LEVEL_DBG)) {
		char str[BT_OTS_OBJ_ID_STR_LEN];
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		LOG_DBG("Current group ID write: offset: %d, len: %d, track ID: %s", offset, len,
			str);
	}

	mcs_cbs->set_current_group_id(id);

	return BT_OTS_OBJ_ID_SIZE;
}

static void current_group_id_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}
#endif /* CONFIG_BT_OTS */

static ssize_t read_playing_order(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_playing_order == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint8_t order = mcs_cbs->get_playing_order();

	LOG_DBG("Playing order read: %d (0x%02x)", order, order);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &order, sizeof(order));
}

static ssize_t write_playing_order(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	if (mcs_cbs == NULL || mcs_cbs->set_playing_order == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	LOG_DBG("Playing order write");

	int8_t order;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (len != sizeof(order)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&order, buf, len);

	mcs_cbs->set_playing_order(order);

	LOG_DBG("Playing order write: %d", order);

	return len;
}

static void playing_order_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_playing_orders_supported(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					     void *buf, uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_playing_orders_supported == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint16_t orders = mcs_cbs->get_playing_orders_supported();
	uint16_t orders_le = sys_cpu_to_le16(orders);

	LOG_DBG("Playing orders read: %d (0x%04x)", orders, orders);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &orders_le, sizeof(orders_le));
}

static ssize_t read_media_state(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_media_state == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint8_t state = mcs_cbs->get_media_state();

	LOG_DBG("Media state read: %d", state);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &state,
				 sizeof(state));
}

static void media_state_cfg_changed(const struct bt_gatt_attr *attr,
				    uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t write_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset,
				   uint8_t write_flags)
{
	if (mcs_cbs == NULL || mcs_cbs->send_command == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	struct mpl_cmd command;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(command.opcode) &&
	    len != sizeof(command.opcode) + sizeof(command.param)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&command.opcode, buf, sizeof(command.opcode));
	LOG_DBG("Opcode: %d", command.opcode);
	command.use_param = false;

	if (conn != NULL) {
		struct client_state *client;
		struct mcs_flags *flags;
		__maybe_unused int err;

		err = k_mutex_lock(&mcs_inst.mutex, MUTEX_TIMEOUT);
		__ASSERT(err == 0, "Failed to lock mutex: %d", err);

		client = &mcs_inst.clients[bt_conn_index(conn)];
		flags = &client->flags;

		/* If we are already handling a control point operation, we cannot handle it.
		 * Similarly we do not send a notification, as we do not want to risk sending the
		 * notification for this 2nd request before we've sent the notification for the 1st
		 * request to avoid confusing the client. Unfortunately there is no flow control
		 * defined for control point operations.
		 */
		if (flags->media_control_point_busy || flags->media_control_point_result) {
			const bool media_control_point_busy = flags->media_control_point_busy;
			const bool media_control_point_result = flags->media_control_point_result;

			err = k_mutex_unlock(&mcs_inst.mutex);
			__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

			LOG_DBG("Rejecting CP write as one is already pending (%d %d)",
				media_control_point_busy, media_control_point_result);

			return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
		}

		if (!BT_MCS_VALID_OP(command.opcode)) {
			/* MCS does not specify what to return in case of an error - Only what to
			 * notify
			 */

			client->cmd_ntf.requested_opcode = command.opcode;
			client->cmd_ntf.result_code = BT_MCS_OPC_NTF_NOT_SUPPORTED;

			flags->media_control_point_result = true;

			err = k_mutex_unlock(&mcs_inst.mutex);
			__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
			LOG_DBG("Opcode 0x%02X is invalid", command.opcode);

			err = k_work_schedule(&mcs_inst.notify_work, K_NO_WAIT);
			__ASSERT(err >= 0, "Failed to schedule work: %d", err);

			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		flags->media_control_point_busy = true;

		err = k_mutex_unlock(&mcs_inst.mutex);
		__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
	} else {
		if (!BT_MCS_VALID_OP(command.opcode)) {
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	if (len == sizeof(command.opcode) + sizeof(command.param)) {
		command.param = sys_get_le32((char *)buf + sizeof(command.opcode));
		command.use_param = true;
		LOG_DBG("Parameter: %d", command.param);
	}

	mcs_cbs->send_command(&command);

	return len;
}

static void control_point_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_opcodes_supported(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_commands_supported == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint32_t opcodes = mcs_cbs->get_commands_supported();
	uint32_t opcodes_le = sys_cpu_to_le32(opcodes);

	LOG_DBG("Opcodes_supported read: %d (0x%08x)", opcodes, opcodes);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &opcodes_le, sizeof(opcodes_le));
}

static void opcodes_supported_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

#ifdef CONFIG_BT_OTS
static ssize_t write_search_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					  const void *buf, uint16_t len, uint16_t offset,
					  uint8_t write_flags)
{
	if (mcs_cbs == NULL || mcs_cbs->send_search == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	struct mpl_search search = {0};

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len > SEARCH_LEN_MAX || len < SEARCH_LEN_MIN) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (conn != NULL) {
		struct mcs_flags *flags;
		__maybe_unused int err;

		err = k_mutex_lock(&mcs_inst.mutex, MUTEX_TIMEOUT);
		__ASSERT(err == 0, "Failed to lock mutex: %d", err);

		flags = &mcs_inst.clients[bt_conn_index(conn)].flags;

		/* If we are already handling a search control point operation, we cannot handle it.
		 * Similarly we do not send a notification, as we do not want to risk sending the
		 * notification for this 2nd request before we've sent the notification for the 1st
		 * request to avoid confusing the client. Unfortunately there is no flow control
		 * defined for control point operations.
		 */
		if (flags->search_control_point_busy || flags->search_control_point_result) {
			const bool search_control_point_busy = flags->search_control_point_busy;
			const bool search_control_point_result = flags->search_control_point_result;

			err = k_mutex_unlock(&mcs_inst.mutex);
			__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

			LOG_DBG("Rejecting CP write as one is already pending (%d %d)",
				search_control_point_busy, search_control_point_result);

			return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
		}

		flags->search_control_point_busy = true;

		err = k_mutex_unlock(&mcs_inst.mutex);
		__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
	}

	memcpy(&search.search, (char *)buf, len);
	search.len = len;
	LOG_DBG("Search length: %d", len);
	LOG_HEXDUMP_DBG(&search.search, search.len, "Search content");

	mcs_cbs->send_search(&search);

	return len;
}

static void search_control_point_cfg_changed(const struct bt_gatt_attr *attr,
					     uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_search_results_id(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_search_results_id == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint64_t search_id = mcs_cbs->get_search_results_id();

	LOG_DBG_OBJ_ID("Search results id read: ", search_id);

	/* TODO: The permanent solution here should be that the call to */
	/* mpl should fill the UUID in a pointed-to value, and return a */
	/* length or an error code, to indicate whether this ID has a */
	/* value now.  This should be done for all functions of this kind. */
	/* For now, fix the issue here - send zero-length data if the */
	/* ID is zero. */
	/* *Spec requirement - IDs may not be valid, in which case the */
	/* characteristic shall be zero length. */

	if (search_id == 0) {
		return bt_gatt_attr_read(conn, attr, buf, len, offset,
					 NULL, 0);
	} else {
		uint8_t search_id_le[BT_OTS_OBJ_ID_SIZE];

		sys_put_le48(search_id, search_id_le);

		return bt_gatt_attr_read(conn, attr, buf, len, offset,
					 &search_id_le, sizeof(search_id_le));
	}
}

static void search_results_id_cfg_changed(const struct bt_gatt_attr *attr,
					  uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}
#endif /* CONFIG_BT_OTS */

static ssize_t read_content_ctrl_id(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr, void *buf,
				    uint16_t len, uint16_t offset)
{
	if (mcs_cbs == NULL || mcs_cbs->get_content_ctrl_id == NULL) {
		LOG_DBG("Callback not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	uint8_t id = mcs_cbs->get_content_ctrl_id();

	LOG_DBG("Content control ID read: %d", id);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &id,
				 sizeof(id));
}

/* Defines for OTS-dependent characteristics - empty if no OTS */
#ifdef CONFIG_BT_OTS
#define ICON_OBJ_ID_CHARACTERISTIC_IF_OTS  \
	BT_AUDIO_CHRC(BT_UUID_MCS_ICON_OBJ_ID, \
		      BT_GATT_CHRC_READ, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_icon_id, NULL, NULL),
#define SEGMENTS_TRACK_GROUP_ID_CHARACTERISTICS_IF_OTS  \
	BT_AUDIO_CHRC(BT_UUID_MCS_TRACK_SEGMENTS_OBJ_ID,	\
		      BT_GATT_CHRC_READ, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_track_segments_id, NULL, NULL), \
	BT_AUDIO_CHRC(BT_UUID_MCS_CURRENT_TRACK_OBJ_ID, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
		      BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, \
		      read_current_track_id, write_current_track_id, NULL), \
	BT_AUDIO_CCC(current_track_id_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_MCS_NEXT_TRACK_OBJ_ID, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
		      BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, \
		      read_next_track_id, write_next_track_id, NULL), \
	BT_AUDIO_CCC(next_track_id_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_MCS_PARENT_GROUP_OBJ_ID, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_parent_group_id, NULL, NULL), \
	BT_AUDIO_CCC(parent_group_id_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_MCS_CURRENT_GROUP_OBJ_ID, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
		      BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, \
		      read_current_group_id, write_current_group_id, NULL), \
	BT_AUDIO_CCC(current_group_id_cfg_changed),
#define	SEARCH_CHARACTERISTICS_IF_OTS \
	BT_AUDIO_CHRC(BT_UUID_MCS_SEARCH_CONTROL_POINT, \
		      BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_WRITE_ENCRYPT, \
		      NULL, write_search_control_point, NULL), \
	BT_AUDIO_CCC(search_control_point_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_search_results_id, NULL, NULL), \
	BT_AUDIO_CCC(search_results_id_cfg_changed),

#else
#define ICON_OBJ_ID_CHARACTERISTIC_IF_OTS
#define SEGMENTS_TRACK_GROUP_ID_CHARACTERISTICS_IF_OTS
#define SEARCH_CHARACTERISTICS_IF_OTS
#endif /* CONFIG_BT_OTS */

/* Media control service attributes */
#define BT_MCS_SERVICE_DEFINITION \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GMCS), \
	BT_GATT_INCLUDE_SERVICE(NULL), /* To be overwritten */ \
	BT_AUDIO_CHRC(BT_UUID_MCS_PLAYER_NAME, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_player_name, NULL, NULL), \
	BT_AUDIO_CCC(player_name_cfg_changed), \
	ICON_OBJ_ID_CHARACTERISTIC_IF_OTS \
	BT_AUDIO_CHRC(BT_UUID_MCS_ICON_URL, \
		      BT_GATT_CHRC_READ, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_icon_url, NULL, NULL), \
	BT_AUDIO_CHRC(BT_UUID_MCS_TRACK_CHANGED, \
		      BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_NONE, \
		      NULL, NULL, NULL), \
	BT_AUDIO_CCC(track_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_MCS_TRACK_TITLE, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_track_title, NULL, NULL), \
	BT_AUDIO_CCC(track_title_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_MCS_TRACK_DURATION, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_track_duration, NULL, NULL), \
	BT_AUDIO_CCC(track_duration_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_MCS_TRACK_POSITION, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
		      BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, \
		      read_track_position, write_track_position, NULL), \
	BT_AUDIO_CCC(track_position_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_MCS_PLAYBACK_SPEED, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
		      BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, \
		      read_playback_speed, write_playback_speed, NULL), \
	BT_AUDIO_CCC(playback_speed_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_MCS_SEEKING_SPEED, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_seeking_speed, NULL, NULL), \
	BT_AUDIO_CCC(seeking_speed_cfg_changed), \
	SEGMENTS_TRACK_GROUP_ID_CHARACTERISTICS_IF_OTS \
	BT_AUDIO_CHRC(BT_UUID_MCS_PLAYING_ORDER, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
		      BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, \
		      read_playing_order, write_playing_order, NULL), \
	BT_AUDIO_CCC(playing_order_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_MCS_PLAYING_ORDERS, \
		      BT_GATT_CHRC_READ, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_playing_orders_supported, NULL, NULL), \
	BT_AUDIO_CHRC(BT_UUID_MCS_MEDIA_STATE, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_media_state, NULL, NULL), \
	BT_AUDIO_CCC(media_state_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_MCS_MEDIA_CONTROL_POINT, \
		      BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_WRITE_ENCRYPT, \
		      NULL, write_control_point, NULL), \
	BT_AUDIO_CCC(control_point_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_MCS_MEDIA_CONTROL_OPCODES, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_opcodes_supported, NULL, NULL), \
	BT_AUDIO_CCC(opcodes_supported_cfg_changed), \
	SEARCH_CHARACTERISTICS_IF_OTS \
	BT_AUDIO_CHRC(BT_UUID_CCID, \
		      BT_GATT_CHRC_READ, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_content_ctrl_id, NULL, NULL)

static struct bt_gatt_attr svc_attrs[] = { BT_MCS_SERVICE_DEFINITION };
static struct bt_gatt_service mcs;
#ifdef CONFIG_BT_OTS
static struct bt_ots *ots;
#endif /* CONFIG_BT_OTS */

#ifdef CONFIG_BT_OTS
struct bt_ots *bt_mcs_get_ots(void)
{
	return ots;
}
#endif /* CONFIG_BT_OTS */

/* Callback functions from the media player, notifying attributes */
/* Placed here, after the service definition, because they reference it. */
static int notify(struct bt_conn *conn, const struct bt_uuid *uuid, const void *data, uint16_t len)
{
	const uint16_t max_ntf_size = bt_audio_get_max_ntf_size(conn);

	if (max_ntf_size < len) {
		LOG_DBG("Truncating notification to %u (was %u) for %p", max_ntf_size, len, conn);
		len = max_ntf_size;
	}

	return bt_gatt_notify_uuid(conn, uuid, mcs.attrs, data, len);
}

struct mcs_notify_cb_info {
	const struct bt_gatt_attr *attr;
	void (*value_cb)(struct mcs_flags *flags);
};

static void set_value_changed_cb(struct bt_conn *conn, void *data)
{
	struct mcs_notify_cb_info *cb_info = data;
	const struct bt_gatt_attr *attr = cb_info->attr;
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	__ASSERT(err == 0, "Failed to get conn info: %d", err);

	if (info.state != BT_CONN_STATE_CONNECTED) {
		/* Not connected */
		return;
	}

	if (!bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		/* Not subscribed */
		return;
	}

	/* Set the specific flag based on the provided callback */
	cb_info->value_cb(&mcs_inst.clients[bt_conn_index(conn)].flags);

	/* We may schedule the same work multiple times, but that is OK as scheduling the same work
	 * multiple times is a no-op
	 */
	err = k_work_schedule(&mcs_inst.notify_work, K_NO_WAIT);
	__ASSERT(err >= 0, "Failed to schedule work: %d", err);
}

static void set_value_changed(void (*value_cb)(struct mcs_flags *flags), const struct bt_uuid *uuid)
{
	struct mcs_notify_cb_info cb_info = {
		.value_cb = value_cb,
		.attr = bt_gatt_find_by_uuid(mcs.attrs, 0, uuid),
	};
	__maybe_unused int err;

	err = k_mutex_lock(&mcs_inst.mutex, MUTEX_TIMEOUT);
	__ASSERT(err == 0, "Failed to lock mutex: %d", err);

	__ASSERT(cb_info.attr != NULL, "Failed to find attribute for %s", bt_uuid_str(uuid));
	bt_conn_foreach(BT_CONN_TYPE_LE, set_value_changed_cb, &cb_info);

	err = k_mutex_unlock(&mcs_inst.mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
}

static void notify_cb(struct bt_conn *conn, void *data)
{

	struct client_state *client;
	struct mcs_flags *flags;
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		LOG_ERR("Failed to get conn info: %d", err);
		return;
	}

	if (info.state != BT_CONN_STATE_CONNECTED) {
		/* Not connected */
		return;
	}

	err = k_mutex_lock(&mcs_inst.mutex, K_NO_WAIT);
	if (err != 0) {
		LOG_DBG("Failed to take mutex: %d", err);
		err = k_work_schedule(&mcs_inst.notify_work, K_USEC(info.le.interval_us));
		__ASSERT(err >= 0, "Failed to reschedule work: %d", err);
		return;
	}

	client = &mcs_inst.clients[bt_conn_index(conn)];
	flags = &client->flags;

	if (flags->player_name_changed) {
		const char *name = mcs_cbs->get_player_name();

		LOG_DBG("Notifying player name: %s", name);
		err = notify(conn, BT_UUID_MCS_PLAYER_NAME, name, strlen(name));
		if (err == 0) {
			flags->player_name_changed = false;
		} else {
			goto fail;
		}
	}

	if (flags->track_title_changed) {
		const char *title = mcs_cbs->get_track_title();

		LOG_DBG("Notifying track title: %s", title);
		err = notify(conn, BT_UUID_MCS_TRACK_TITLE, title, strlen(title));
		if (err == 0) {
			flags->track_title_changed = false;
		} else {
			goto fail;
		}
	}

	if (flags->track_duration_changed) {
		int32_t duration = mcs_cbs->get_track_duration();
		int32_t duration_le = sys_cpu_to_le32(duration);

		LOG_DBG("Notifying track duration: %d", duration);
		err = notify(conn, BT_UUID_MCS_TRACK_DURATION, &duration_le, sizeof(duration_le));
		if (err == 0) {
			flags->track_duration_changed = false;
		} else {
			goto fail;
		}
	}

	if (flags->track_position_changed) {
		int32_t position = mcs_cbs->get_track_position();
		int32_t position_le = sys_cpu_to_le32(position);

		LOG_DBG("Notifying track position: %d", position);
		err = notify(conn, BT_UUID_MCS_TRACK_POSITION, &position_le, sizeof(position_le));
		if (err == 0) {
			flags->track_position_changed = false;
		} else {
			goto fail;
		}
	}

	if (flags->playback_speed_changed) {
		int8_t speed = mcs_cbs->get_playback_speed();

		LOG_DBG("Notifying playback speed: %d", speed);
		err = notify(conn, BT_UUID_MCS_PLAYBACK_SPEED, &speed, sizeof(speed));
		if (err == 0) {
			flags->playback_speed_changed = false;
		} else {
			goto fail;
		}
	}

	if (flags->seeking_speed_changed) {
		int8_t speed = mcs_cbs->get_seeking_speed();

		LOG_DBG("Notifying seeking speed: %d", speed);
		err = notify(conn, BT_UUID_MCS_SEEKING_SPEED, &speed, sizeof(speed));
		if (err == 0) {
			flags->seeking_speed_changed = false;
		} else {
			goto fail;
		}
	}

#if defined(CONFIG_BT_OTS)
	if (flags->current_track_obj_id_changed) {
		uint64_t track_id = mcs_cbs->get_current_track_id();
		uint8_t track_id_le[BT_OTS_OBJ_ID_SIZE];

		sys_put_le48(track_id, track_id_le);

		LOG_DBG_OBJ_ID("Notifying current track ID: ", track_id);
		err = notify(conn, BT_UUID_MCS_CURRENT_TRACK_OBJ_ID, track_id_le,
			     sizeof(track_id_le));
		if (err == 0) {
			flags->current_track_obj_id_changed = false;
		} else {
			goto fail;
		}
	}

	if (flags->next_track_obj_id_changed) {
		uint64_t track_id = mcs_cbs->get_next_track_id();

		if (track_id == MPL_NO_TRACK_ID) {
			/* "If the media player has no next track, the length of the
			 * characteristic shall be zero."
			 */
			LOG_DBG_OBJ_ID("Notifying EMPTY next track ID: ", track_id);
			err = notify(conn, BT_UUID_MCS_NEXT_TRACK_OBJ_ID, NULL, 0);
		} else {
			uint8_t track_id_le[BT_OTS_OBJ_ID_SIZE];

			sys_put_le48(track_id, track_id_le);

			LOG_DBG_OBJ_ID("Notifying next track ID: ", track_id);
			err = notify(conn, BT_UUID_MCS_NEXT_TRACK_OBJ_ID, track_id_le,
				     sizeof(track_id_le));
		}

		if (err == 0) {
			flags->next_track_obj_id_changed = false;
		} else {
			goto fail;
		}
	}

	if (flags->parent_group_obj_id_changed) {
		uint64_t group_id = mcs_cbs->get_parent_group_id();
		uint8_t group_id_le[BT_OTS_OBJ_ID_SIZE];

		sys_put_le48(group_id, group_id_le);

		LOG_DBG_OBJ_ID("Notifying parent group ID: ", group_id);
		err = notify(conn, BT_UUID_MCS_PARENT_GROUP_OBJ_ID, &group_id_le,
			     sizeof(group_id_le));
		if (err == 0) {
			flags->parent_group_obj_id_changed = false;
		} else {
			goto fail;
		}
	}

	if (flags->current_group_obj_id_changed) {
		uint64_t group_id = mcs_cbs->get_current_group_id();
		uint8_t group_id_le[BT_OTS_OBJ_ID_SIZE];

		sys_put_le48(group_id, group_id_le);

		LOG_DBG_OBJ_ID("Notifying current group ID: ", group_id);
		err = notify(conn, BT_UUID_MCS_CURRENT_GROUP_OBJ_ID, &group_id_le,
			     sizeof(group_id_le));
		if (err == 0) {
			flags->current_group_obj_id_changed = false;
		} else {
			goto fail;
		}
	}
#endif /* CONFIG_BT_OTS */

	if (flags->track_changed_changed) {
		LOG_DBG("Notifying track change");
		err = notify(conn, BT_UUID_MCS_TRACK_CHANGED, NULL, 0);
		if (err == 0) {
			flags->track_changed_changed = false;
		} else {
			goto fail;
		}
	}

	if (flags->playing_order_changed) {
		uint8_t order = mcs_cbs->get_playing_order();

		LOG_DBG("Notifying playing order: %d", order);
		err = notify(conn, BT_UUID_MCS_PLAYING_ORDER, &order, sizeof(order));
		if (err == 0) {
			flags->playing_order_changed = false;
		} else {
			goto fail;
		}
	}

	if (flags->media_state_changed) {
		uint8_t state = mcs_cbs->get_media_state();

		LOG_DBG("Notifying media state: %d", state);
		err = notify(conn, BT_UUID_MCS_MEDIA_STATE, &state, sizeof(state));
		if (err == 0) {
			flags->media_state_changed = false;
		} else {
			goto fail;
		}
	}

	if (flags->media_control_opcodes_changed) {
		uint32_t opcodes = mcs_cbs->get_commands_supported();
		uint32_t opcodes_le = sys_cpu_to_le32(opcodes);

		LOG_DBG("Notifying command opcodes supported: %d (0x%08x)", opcodes, opcodes);
		err = notify(conn, BT_UUID_MCS_MEDIA_CONTROL_OPCODES, &opcodes_le,
			     sizeof(opcodes_le));
		if (err == 0) {
			flags->media_control_opcodes_changed = false;
		} else {
			goto fail;
		}
	}

#if defined(CONFIG_BT_OTS)
	if (flags->search_results_obj_id_changed) {
		uint64_t search_id = mcs_cbs->get_search_results_id();
		uint8_t search_id_le[BT_OTS_OBJ_ID_SIZE];

		sys_put_le48(search_id, search_id_le);

		LOG_DBG_OBJ_ID("Notifying search results ID: ", search_id);
		err = notify(conn, BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID, &search_id_le,
			     sizeof(search_id_le));
		if (err == 0) {
			flags->search_results_obj_id_changed = false;
		} else {
			goto fail;
		}
	}

	if (flags->search_control_point_result) {
		uint8_t result_code = client->search_control_point_result;

		LOG_DBG("Notifying search control point - result: %d", result_code);
		err = notify(conn, BT_UUID_MCS_SEARCH_CONTROL_POINT, &result_code,
			     sizeof(result_code));
		if (err == 0) {
			flags->search_control_point_result = false;
		} else {
			goto fail;
		}
	}
#endif /* CONFIG_BT_OTS */

	if (flags->media_control_point_result) {
		LOG_DBG("Notifying control point command - opcode: %d, result: %d",
			client->cmd_ntf.requested_opcode, client->cmd_ntf.result_code);
		err = notify(conn, BT_UUID_MCS_MEDIA_CONTROL_POINT, &client->cmd_ntf,
			     sizeof(client->cmd_ntf));
		if (err == 0) {
			flags->media_control_point_result = false;
		} else {
			goto fail;
		}
	}

fail:
	if (err != 0) {
		LOG_DBG("Notify failed (%d), retrying next connection interval", err);
		err = k_work_schedule(&mcs_inst.notify_work, K_USEC(info.le.interval_us));
		__ASSERT(err >= 0, "Failed to reschedule work: %d", err);
	}

	err = k_mutex_unlock(&mcs_inst.mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
}

static void deferred_nfy_work_handler(struct k_work *work)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, notify_cb, NULL);
}

static void set_player_name_changed_cb(struct mcs_flags *flags)
{
	flags->player_name_changed = true;
	flags->player_name_dirty = true;
}

static void media_proxy_sctrl_player_name_cb(const char *name)
{
	set_value_changed(set_player_name_changed_cb, BT_UUID_MCS_PLAYER_NAME);
}

static void set_icon_url_changed_cb(struct mcs_flags *flags)
{
	flags->icon_url_dirty = true;
}

void media_proxy_sctrl_icon_url_cb(const char *name)
{
	set_value_changed(set_icon_url_changed_cb, BT_UUID_MCS_ICON_URL);
}

static void set_track_changed_changed_cb(struct mcs_flags *flags)
{
	flags->track_changed_changed = true;
}

void media_proxy_sctrl_track_changed_cb(void)
{
	set_value_changed(set_track_changed_changed_cb, BT_UUID_MCS_TRACK_CHANGED);
}

static void set_track_title_changed_cb(struct mcs_flags *flags)
{
	flags->track_title_changed = true;
	flags->track_title_dirty = true;
}

void media_proxy_sctrl_track_title_cb(const char *title)
{
	set_value_changed(set_track_title_changed_cb, BT_UUID_MCS_TRACK_TITLE);
}

static void set_track_position_changed_cb(struct mcs_flags *flags)
{
	flags->track_position_changed = true;
}

void media_proxy_sctrl_track_position_cb(int32_t position)
{
	set_value_changed(set_track_position_changed_cb, BT_UUID_MCS_TRACK_POSITION);
}

static void set_track_duration_changed_cb(struct mcs_flags *flags)
{
	flags->track_duration_changed = true;
}

void media_proxy_sctrl_track_duration_cb(int32_t duration)
{
	set_value_changed(set_track_duration_changed_cb, BT_UUID_MCS_TRACK_DURATION);
}

static void set_playback_speed_changed_cb(struct mcs_flags *flags)
{
	flags->playback_speed_changed = true;
}

void media_proxy_sctrl_playback_speed_cb(int8_t speed)
{
	set_value_changed(set_playback_speed_changed_cb, BT_UUID_MCS_PLAYBACK_SPEED);
}

static void set_seeking_speed_changed_cb(struct mcs_flags *flags)
{
	flags->seeking_speed_changed = true;
}

void media_proxy_sctrl_seeking_speed_cb(int8_t speed)
{
	set_value_changed(set_seeking_speed_changed_cb, BT_UUID_MCS_SEEKING_SPEED);
}

#if defined(CONFIG_BT_OTS)
static void set_current_track_obj_id_changed_cb(struct mcs_flags *flags)
{
	flags->current_track_obj_id_changed = true;
}

void media_proxy_sctrl_current_track_id_cb(uint64_t id)
{
	set_value_changed(set_current_track_obj_id_changed_cb, BT_UUID_MCS_CURRENT_TRACK_OBJ_ID);
}

static void set_next_track_obj_id_changed_cb(struct mcs_flags *flags)
{
	flags->next_track_obj_id_changed = true;
}

void media_proxy_sctrl_next_track_id_cb(uint64_t id)
{
	set_value_changed(set_next_track_obj_id_changed_cb, BT_UUID_MCS_NEXT_TRACK_OBJ_ID);
}

static void set_parent_group_obj_id_changed_cb(struct mcs_flags *flags)
{
	flags->parent_group_obj_id_changed = true;
}

void media_proxy_sctrl_parent_group_id_cb(uint64_t id)
{
	set_value_changed(set_parent_group_obj_id_changed_cb, BT_UUID_MCS_PARENT_GROUP_OBJ_ID);
}

static void set_current_group_obj_id_changed_cb(struct mcs_flags *flags)
{
	flags->current_group_obj_id_changed = true;
}

void media_proxy_sctrl_current_group_id_cb(uint64_t id)
{
	set_value_changed(set_current_group_obj_id_changed_cb, BT_UUID_MCS_CURRENT_GROUP_OBJ_ID);
}
#endif /* CONFIG_BT_OTS */

static void set_playing_order_changed_cb(struct mcs_flags *flags)
{
	flags->playing_order_changed = true;
}

void media_proxy_sctrl_playing_order_cb(uint8_t order)
{
	set_value_changed(set_playing_order_changed_cb, BT_UUID_MCS_PLAYING_ORDER);
}

static void set_media_state_changed_cb(struct mcs_flags *flags)
{
	flags->media_state_changed = true;
}

void media_proxy_sctrl_media_state_cb(uint8_t state)
{
	set_value_changed(set_media_state_changed_cb, BT_UUID_MCS_MEDIA_STATE);
}

static void defer_media_control_point_ntf(struct bt_conn *conn, void *data)
{
	const struct mpl_cmd_ntf *cmd_ntf = data;
	struct client_state *client;
	struct mcs_flags *flags;
	struct bt_conn_info info;
	bool submit_work = false;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		LOG_ERR("Failed to get conn info: %d", err);
		return;
	}

	if (info.state != BT_CONN_STATE_CONNECTED) {
		/* Not connected */
		return;
	}

	err = k_mutex_lock(&mcs_inst.mutex, MUTEX_TIMEOUT);
	__ASSERT(err == 0, "Failed to lock mutex: %d", err);

	client = &mcs_inst.clients[bt_conn_index(conn)];
	flags = &client->flags;

	if (flags->media_control_point_busy) {
		client->cmd_ntf = *cmd_ntf;
		flags->media_control_point_result = true;
		flags->media_control_point_busy = false;
		submit_work = true;
	}

	err = k_mutex_unlock(&mcs_inst.mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	if (submit_work) {
		k_work_schedule(&mcs_inst.notify_work, K_NO_WAIT);
	}
}

void media_proxy_sctrl_command_cb(const struct mpl_cmd_ntf *cmd_ntf)
{
	/* FIXME: Control Point notification shall be sent to operation initiator only */
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_media_control_point_ntf, (void *)cmd_ntf);
}

static void set_media_control_opcodes_changed_cb(struct mcs_flags *flags)
{
	flags->media_control_opcodes_changed = true;
}

void media_proxy_sctrl_commands_supported_cb(uint32_t opcodes)
{
	set_value_changed(set_media_control_opcodes_changed_cb, BT_UUID_MCS_MEDIA_CONTROL_OPCODES);
}

#if defined(CONFIG_BT_OTS)
static void defer_search_control_point_ntf(struct bt_conn *conn, void *data)
{
	struct client_state *client;
	struct bt_conn_info info;
	struct mcs_flags *flags;
	bool submit_work = false;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		LOG_ERR("Failed to get conn info: %d", err);
		return;
	}

	if (info.state != BT_CONN_STATE_CONNECTED) {
		/* Not connected */
		return;
	}

	err = k_mutex_lock(&mcs_inst.mutex, MUTEX_TIMEOUT);
	__ASSERT(err == 0, "Failed to lock mutex: %d", err);

	client = &mcs_inst.clients[bt_conn_index(conn)];
	flags = &client->flags;

	if (flags->search_control_point_busy) {
		client->search_control_point_result = POINTER_TO_UINT(data);
		flags->search_control_point_result = true;
		flags->search_control_point_busy = false;
		submit_work = true;
	}

	err = k_mutex_unlock(&mcs_inst.mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	if (submit_work) {
		k_work_schedule(&mcs_inst.notify_work, K_NO_WAIT);
	}
}

void media_proxy_sctrl_search_cb(uint8_t result_code)
{
	/* FIXME: Control Point notification shall be sent to operation initiator only */
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_search_control_point_ntf,
			UINT_TO_POINTER(result_code));
}

static void set_search_results_obj_id_changed_cb(struct mcs_flags *flags)
{
	flags->search_results_obj_id_changed = true;
}

void media_proxy_sctrl_search_results_id_cb(uint64_t id)
{
	set_value_changed(set_search_results_obj_id_changed_cb, BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID);
}
#endif /* CONFIG_BT_OTS */

/* Register the service */
int bt_mcs_init(struct bt_ots_cb *ots_cbs)
{
	static struct media_proxy_sctrl_cbs cbs;
	static bool initialized;
	int err;

	if (initialized) {
		LOG_DBG("Already initialized");
		return -EALREADY;
	}


	mcs = (struct bt_gatt_service)BT_GATT_SERVICE(svc_attrs);

#ifdef CONFIG_BT_OTS
	struct bt_ots_init_param ots_init;

	ots = bt_ots_free_instance_get();
	if (!ots) {
		LOG_ERR("Failed to retrieve OTS instance\n");
		return -ENOMEM;
	}

	/* Configure OTS initialization. */
	memset(&ots_init, 0, sizeof(ots_init));
	BT_OTS_OACP_SET_FEAT_READ(ots_init.features.oacp);
	BT_OTS_OLCP_SET_FEAT_GO_TO(ots_init.features.olcp);
	ots_init.cb = ots_cbs;

	/* Initialize OTS instance. */
	err = bt_ots_init(ots, &ots_init);
	if (err) {
		LOG_ERR("Failed to init OTS (err:%d)\n", err);
		return err;
	}

	/* TODO: Maybe the user_data pointer can be in a different way */
	for (int i = 0; i < mcs.attr_count; i++) {
		if (!bt_uuid_cmp(mcs.attrs[i].uuid, BT_UUID_GATT_INCLUDE)) {
			mcs.attrs[i].user_data = bt_ots_svc_decl_get(ots);
		}
	}
#endif /* CONFIG_BT_OTS */

	err = bt_gatt_service_register(&mcs);

	if (err) {
		LOG_ERR("Could not register the MCS service");
#ifdef CONFIG_BT_OTS
		/* TODO: How does one un-register the OTS? */
#endif /* CONFIG_BT_OTS */
		return -ENOEXEC;
	}

	/* Set up the callback structure */
	cbs.player_name          = media_proxy_sctrl_player_name_cb;
	cbs.icon_url             = media_proxy_sctrl_icon_url_cb;
	cbs.track_changed        = media_proxy_sctrl_track_changed_cb;
	cbs.track_title          = media_proxy_sctrl_track_title_cb;
	cbs.track_duration       = media_proxy_sctrl_track_duration_cb;
	cbs.track_position       = media_proxy_sctrl_track_position_cb;
	cbs.playback_speed       = media_proxy_sctrl_playback_speed_cb;
	cbs.seeking_speed        = media_proxy_sctrl_seeking_speed_cb;
#ifdef CONFIG_BT_OTS
	cbs.current_track_id     = media_proxy_sctrl_current_track_id_cb;
	cbs.next_track_id        = media_proxy_sctrl_next_track_id_cb;
	cbs.parent_group_id      = media_proxy_sctrl_parent_group_id_cb;
	cbs.current_group_id     = media_proxy_sctrl_current_group_id_cb;
#endif /* CONFIG_BT_OTS */
	cbs.playing_order        = media_proxy_sctrl_playing_order_cb;
	cbs.media_state          = media_proxy_sctrl_media_state_cb;
	cbs.command              = media_proxy_sctrl_command_cb;
	cbs.commands_supported   = media_proxy_sctrl_commands_supported_cb;
#ifdef CONFIG_BT_OTS
	cbs.search               = media_proxy_sctrl_search_cb;
	cbs.search_results_id    = media_proxy_sctrl_search_results_id_cb;
#endif /* CONFIG_BT_OTS */

	k_work_init_delayable(&mcs_inst.notify_work, deferred_nfy_work_handler);
	k_mutex_init(&mcs_inst.mutex);

	media_proxy_sctrl_register(&cbs);

	initialized = true;
	return 0;
}

static bool cbs_is_valid(struct bt_mcs_cb *cbs)
{
	if (cbs == NULL) {
		LOG_DBG("cbs is NULL");
		return false;
	}

	if (cbs->get_player_name == NULL) {
		LOG_DBG("get_player_name is NULL");
		return false;
	}

	if (cbs->get_track_title == NULL) {
		LOG_DBG("get_track_title is NULL");
		return false;
	}

	if (cbs->get_track_duration == NULL) {
		LOG_DBG("get_track_duration is NULL");
		return false;
	}

	if (cbs->get_track_position == NULL) {
		LOG_DBG("get_track_position is NULL");
		return false;
	}

	if (cbs->set_track_position == NULL) {
		LOG_DBG("set_track_position is NULL");
		return false;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	if (cbs->get_current_track_id != NULL) {
		if (cbs->get_track_segments_id == NULL) {
			LOG_DBG("get_track_segments_id is NULL");
			return false;
		}

		if (cbs->set_current_track_id == NULL) {
			LOG_DBG("set_current_track_id is NULL");
			return false;
		}

		if (cbs->get_next_track_id == NULL) {
			LOG_DBG("get_next_track_id is NULL");
			return false;
		}

		if (cbs->set_next_track_id == NULL) {
			LOG_DBG("set_next_track_id is NULL");
			return false;
		}

		if (cbs->get_parent_group_id == NULL) {
			LOG_DBG("get_parent_group_id is NULL");
			return false;
		}

		if (cbs->get_current_group_id == NULL) {
			LOG_DBG("get_current_group_id is NULL");
			return false;
		}

		if (cbs->set_current_group_id == NULL) {
			LOG_DBG("set_current_group_id is NULL");
			return false;
		}
	}
#endif /* CONFIG_BT_MPL_OBJECTS */
	if (cbs->get_media_state == NULL) {
		LOG_DBG("get_media_state is NULL");
		return false;
	}

	if (cbs->send_command != NULL && cbs->get_commands_supported == NULL) {
		LOG_DBG("get_commands_supported is NULL");
		return false;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	if (cbs->send_search == NULL) {
		LOG_DBG("send_search is NULL");
		return false;
	}

#endif /* CONFIG_BT_MPL_OBJECTS */
	if (cbs->get_content_ctrl_id == NULL) {
		LOG_DBG("get_content_ctrl_id is NULL");
		return false;
	}

	return true;
}

int bt_mcs_register_cb(struct bt_mcs_cb *cbs)
{
	if (!cbs_is_valid(cbs)) {
		return -EINVAL;
	}

	if (mcs_cbs != NULL) {
		LOG_DBG("Callbacks already registered");
		return -EALREADY;
	}

	mcs_cbs = cbs;

	return 0;
};
