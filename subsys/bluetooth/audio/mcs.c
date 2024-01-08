/** @file
 *  @brief Bluetooth Media Control Service
 */

/*
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/bluetooth/audio/media_proxy.h>

#include "audio_internal.h"
#include "media_proxy_internal.h"
#include "mcs_internal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_mcs, CONFIG_BT_MCS_LOG_LEVEL);

static void notify(const struct bt_uuid *uuid, const void *data, uint16_t len);

static struct media_proxy_sctrl_cbs cbs;

enum {
	FLAG_PLAYER_NAME_CHANGED,
	FLAG_ICON_URL_CHANGED,
	FLAG_TRACK_CHANGED,
	FLAG_TRACK_TITLE_CHANGED,
	FLAG_TRACK_DURATION_CHANGED,
	FLAG_TRACK_POSITION_CHANGED,
	FLAG_PLAYBACK_SPEED_CHANGED,
	FLAG_SEEKING_SPEED_CHANGED,
	FLAG_PLAYING_ORDER_CHANGED,
	FLAG_MEDIA_STATE_CHANGED,
	FLAG_MEDIA_CONTROL_OPCODES_CHANGED,
	FLAG_MEDIA_CONTROL_POINT_BUSY,
	FLAG_MEDIA_CONTROL_POINT_RESULT,
#if defined(CONFIG_BT_OTS)
	FLAG_CURRENT_TRACK_OBJ_ID_CHANGED,
	FLAG_NEXT_TRACK_OBJ_ID_CHANGED,
	FLAG_PARENT_GROUP_OBJ_ID_CHANGED,
	FLAG_CURRENT_GROUP_OBJ_ID_CHANGED,
	FLAG_SEARCH_RESULTS_OBJ_ID_CHANGED,
	FLAG_SEARCH_CONTROL_POINT_BUSY,
	FLAG_SEARCH_CONTROL_POINT_RESULT,
#endif /* CONFIG_BT_OTS */
	FLAG_NUM,
};

static struct client_state {
	ATOMIC_DEFINE(flags, FLAG_NUM);
	struct mpl_cmd_ntf cmd_ntf;
#if defined(CONFIG_BT_OTS)
	uint8_t search_control_point_result;
#endif /* CONFIG_BT_OTS */
} clients[CONFIG_BT_MAX_CONN];

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	/* Clear data on disconnect */
	memset(&clients[bt_conn_index(conn)], 0, sizeof(struct client_state));
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
	const char *name = media_proxy_sctrl_get_player_name();

	LOG_DBG("Player name read: %s (offset %u)", name, offset);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		if (offset == 0) {
			atomic_clear_bit(client->flags, FLAG_PLAYER_NAME_CHANGED);
		} else if (atomic_test_bit(client->flags, FLAG_PLAYER_NAME_CHANGED)) {
			return BT_GATT_ERR(BT_MCS_ERR_LONG_VAL_CHANGED);
		}
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, name,
				 strlen(name));
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
	uint64_t icon_id = media_proxy_sctrl_get_icon_id();
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
	const char *url = media_proxy_sctrl_get_icon_url();

	LOG_DBG("Icon URL read, offset: %d, len:%d, URL: %s", offset, len, url);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		if (offset == 0) {
			atomic_clear_bit(client->flags, FLAG_ICON_URL_CHANGED);
		} else if (atomic_test_bit(client->flags, FLAG_ICON_URL_CHANGED)) {
			return BT_GATT_ERR(BT_MCS_ERR_LONG_VAL_CHANGED);
		}
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
	const char *title = media_proxy_sctrl_get_track_title();

	LOG_DBG("Track title read, offset: %d, len:%d, title: %s", offset, len, title);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		if (offset == 0) {
			atomic_clear_bit(client->flags, FLAG_TRACK_TITLE_CHANGED);
		} else if (atomic_test_bit(client->flags, FLAG_TRACK_TITLE_CHANGED)) {
			return BT_GATT_ERR(BT_MCS_ERR_LONG_VAL_CHANGED);
		}
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
	int32_t duration = media_proxy_sctrl_get_track_duration();
	int32_t duration_le = sys_cpu_to_le32(duration);

	LOG_DBG("Track duration read: %d (0x%08x)", duration, duration);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		atomic_clear_bit(client->flags, FLAG_TRACK_DURATION_CHANGED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &duration_le, sizeof(duration_le));
}

static void track_duration_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_track_position(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	int32_t position = media_proxy_sctrl_get_track_position();
	int32_t position_le = sys_cpu_to_le32(position);

	LOG_DBG("Track position read: %d (0x%08x)", position, position);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		atomic_clear_bit(client->flags, FLAG_TRACK_POSITION_CHANGED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &position_le,
				 sizeof(position_le));
}

static ssize_t write_track_position(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    const void *buf, uint16_t len,
				    uint16_t offset, uint8_t flags)
{
	int32_t position;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(position)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	position = sys_get_le32((uint8_t *)buf);

	media_proxy_sctrl_set_track_position(position);

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
	int8_t speed = media_proxy_sctrl_get_playback_speed();

	LOG_DBG("Playback speed read: %d", speed);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		atomic_clear_bit(client->flags, FLAG_PLAYBACK_SPEED_CHANGED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &speed, sizeof(speed));
}

static ssize_t write_playback_speed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	int8_t speed;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (len != sizeof(speed)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&speed, buf, len);

	media_proxy_sctrl_set_playback_speed(speed);

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
	int8_t speed = media_proxy_sctrl_get_seeking_speed();

	LOG_DBG("Seeking speed read: %d", speed);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		atomic_clear_bit(client->flags, FLAG_SEEKING_SPEED_CHANGED);
	}

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
	uint64_t track_segments_id = media_proxy_sctrl_get_track_segments_id();
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
	uint64_t track_id = media_proxy_sctrl_get_current_track_id();
	uint8_t track_id_le[BT_OTS_OBJ_ID_SIZE];

	sys_put_le48(track_id, track_id_le);

	LOG_DBG_OBJ_ID("Current track ID read: ", track_id);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		atomic_clear_bit(client->flags, FLAG_CURRENT_TRACK_OBJ_ID_CHANGED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, track_id_le,
				 sizeof(track_id_le));
}

static ssize_t write_current_track_id(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      const void *buf, uint16_t len, uint16_t offset,
				      uint8_t flags)
{
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

	media_proxy_sctrl_set_current_track_id(id);

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
	uint64_t track_id = media_proxy_sctrl_get_next_track_id();
	uint8_t track_id_le[BT_OTS_OBJ_ID_SIZE];

	sys_put_le48(track_id, track_id_le);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		atomic_clear_bit(client->flags, FLAG_NEXT_TRACK_OBJ_ID_CHANGED);
	}

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

	media_proxy_sctrl_set_next_track_id(id);

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
	uint64_t group_id = media_proxy_sctrl_get_parent_group_id();
	uint8_t group_id_le[BT_OTS_OBJ_ID_SIZE];

	sys_put_le48(group_id, group_id_le);

	LOG_DBG_OBJ_ID("Parent group read: ", group_id);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		atomic_clear_bit(client->flags, FLAG_PARENT_GROUP_OBJ_ID_CHANGED);
	}

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
	uint64_t group_id = media_proxy_sctrl_get_current_group_id();
	uint8_t group_id_le[BT_OTS_OBJ_ID_SIZE];

	sys_put_le48(group_id, group_id_le);

	LOG_DBG_OBJ_ID("Current group read: ", group_id);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		atomic_clear_bit(client->flags, FLAG_CURRENT_GROUP_OBJ_ID_CHANGED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, group_id_le,
				 sizeof(group_id_le));
}

static ssize_t write_current_group_id(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      const void *buf, uint16_t len, uint16_t offset,
				      uint8_t flags)
{
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

	media_proxy_sctrl_set_current_group_id(id);

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
	uint8_t order = media_proxy_sctrl_get_playing_order();

	LOG_DBG("Playing order read: %d (0x%02x)", order, order);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		atomic_clear_bit(client->flags, FLAG_PLAYING_ORDER_CHANGED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &order, sizeof(order));
}

static ssize_t write_playing_order(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Playing order write");

	int8_t order;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (len != sizeof(order)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&order, buf, len);

	media_proxy_sctrl_set_playing_order(order);

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
	uint16_t orders = media_proxy_sctrl_get_playing_orders_supported();
	uint16_t orders_le = sys_cpu_to_le16(orders);

	LOG_DBG("Playing orders read: %d (0x%04x)", orders, orders);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &orders_le, sizeof(orders_le));
}

static ssize_t read_media_state(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	uint8_t state = media_proxy_sctrl_get_media_state();

	LOG_DBG("Media state read: %d", state);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		atomic_clear_bit(client->flags, FLAG_MEDIA_STATE_CHANGED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &state,
				 sizeof(state));
}

static void media_state_cfg_changed(const struct bt_gatt_attr *attr,
				    uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t write_control_point(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset,
				   uint8_t flags)
{
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

	if (!BT_MCS_VALID_OP(command.opcode)) {
		/* MCS does not specify what to return in case of an error - Only what to notify*/

		const struct mpl_cmd_ntf cmd_ntf = {
			.requested_opcode = command.opcode,
			.result_code = BT_MCS_OPC_NTF_NOT_SUPPORTED,
		};

		LOG_DBG("Opcode 0x%02X is invalid", command.opcode);

		notify(BT_UUID_MCS_MEDIA_CONTROL_POINT, &cmd_ntf, sizeof(cmd_ntf));

		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		if (atomic_test_and_set_bit(client->flags, FLAG_MEDIA_CONTROL_POINT_BUSY)) {
			const struct mpl_cmd_ntf cmd_ntf = {
				.requested_opcode = command.opcode,
				.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED,
			};

			LOG_DBG("Busy with other operation");

			notify(BT_UUID_MCS_MEDIA_CONTROL_POINT, &cmd_ntf, sizeof(cmd_ntf));

			return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
		}
	}

	if (len == sizeof(command.opcode) + sizeof(command.param)) {
		command.param = sys_get_le32((char *)buf + sizeof(command.opcode));
		command.use_param = true;
		LOG_DBG("Parameter: %d", command.param);
	}

	media_proxy_sctrl_send_command(&command);

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
	uint32_t opcodes = media_proxy_sctrl_get_commands_supported();
	uint32_t opcodes_le = sys_cpu_to_le32(opcodes);

	LOG_DBG("Opcodes_supported read: %d (0x%08x)", opcodes, opcodes);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		atomic_clear_bit(client->flags, FLAG_MEDIA_CONTROL_OPCODES_CHANGED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &opcodes_le, sizeof(opcodes_le));
}

static void opcodes_supported_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

#ifdef CONFIG_BT_OTS
static ssize_t write_search_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					  const void *buf, uint16_t len, uint16_t offset,
					  uint8_t flags)
{
	struct mpl_search search = {0};

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len > SEARCH_LEN_MAX || len < SEARCH_LEN_MIN) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		if (atomic_test_and_set_bit(client->flags, FLAG_SEARCH_CONTROL_POINT_BUSY)) {
			const uint8_t result_code = BT_MCS_SCP_NTF_FAILURE;

			LOG_DBG("Busy with other operation");

			notify(BT_UUID_MCS_SEARCH_CONTROL_POINT, &result_code, sizeof(result_code));

			return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
		}
	}

	memcpy(&search.search, (char *)buf, len);
	search.len = len;
	LOG_DBG("Search length: %d", len);
	LOG_HEXDUMP_DBG(&search.search, search.len, "Search content");

	media_proxy_sctrl_send_search(&search);

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
	uint64_t search_id = media_proxy_sctrl_get_search_results_id();

	LOG_DBG_OBJ_ID("Search results id read: ", search_id);

	if (conn != NULL) {
		struct client_state *client = &clients[bt_conn_index(conn)];

		atomic_clear_bit(client->flags, FLAG_SEARCH_RESULTS_OBJ_ID_CHANGED);
	}

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
	uint8_t id = media_proxy_sctrl_get_content_ctrl_id();

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

/* Helper function to notify non-string values */
static void notify(const struct bt_uuid *uuid, const void *data, uint16_t len)
{
	int err = bt_gatt_notify_uuid(NULL, uuid, mcs.attrs, data, len);

	if (err) {
		if (err == -ENOTCONN) {
			LOG_DBG("Notification error: ENOTCONN (%d)", err);
		} else {
			LOG_ERR("Notification error: %d", err);
		}
	}
}

static void notify_string(struct bt_conn *conn, const struct bt_uuid *uuid, const char *str)
{
	const uint8_t att_header_size = 3; /* opcode + handle */
	uint16_t att_mtu;
	uint16_t maxlen;
	int err;

	att_mtu = bt_gatt_get_mtu(conn);
	__ASSERT(att_mtu > att_header_size, "Could not get valid ATT MTU");
	maxlen = att_mtu - att_header_size; /* Subtract opcode and handle */

	/* Send notifcation potentially truncated to the MTU */
	err = bt_gatt_notify_uuid(conn, uuid, mcs.attrs, (void *)str,
				  MIN(strlen(str), maxlen));
	if (err != 0) {
		LOG_ERR("Notification error: %d", err);
	}
}

static void mark_icon_url_changed_cb(struct bt_conn *conn, void *data)
{
	struct client_state *client = &clients[bt_conn_index(conn)];
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

	atomic_set_bit(client->flags, FLAG_ICON_URL_CHANGED);
}

static void notify_cb(struct bt_conn *conn, void *data)
{
	struct client_state *client = &clients[bt_conn_index(conn)];
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

	if (atomic_test_and_clear_bit(client->flags, FLAG_PLAYER_NAME_CHANGED)) {
		const char *name = media_proxy_sctrl_get_player_name();

		LOG_DBG("Notifying player name: %s", name);
		notify_string(conn, BT_UUID_MCS_PLAYER_NAME, name);
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_TRACK_TITLE_CHANGED)) {
		const char *title = media_proxy_sctrl_get_track_title();

		LOG_DBG("Notifying track title: %s", title);
		notify_string(conn, BT_UUID_MCS_TRACK_TITLE, title);
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_TRACK_DURATION_CHANGED)) {
		int32_t duration = media_proxy_sctrl_get_track_duration();
		int32_t duration_le = sys_cpu_to_le32(duration);

		LOG_DBG("Notifying track duration: %d", duration);
		notify(BT_UUID_MCS_TRACK_DURATION, &duration_le, sizeof(duration_le));
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_TRACK_POSITION_CHANGED)) {
		int32_t position = media_proxy_sctrl_get_track_position();
		int32_t position_le = sys_cpu_to_le32(position);

		LOG_DBG("Notifying track position: %d", position);
		notify(BT_UUID_MCS_TRACK_POSITION, &position_le, sizeof(position_le));
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_PLAYBACK_SPEED_CHANGED)) {
		int8_t speed = media_proxy_sctrl_get_playback_speed();

		LOG_DBG("Notifying playback speed: %d", speed);
		notify(BT_UUID_MCS_PLAYBACK_SPEED, &speed, sizeof(speed));
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_SEEKING_SPEED_CHANGED)) {
		int8_t speed = media_proxy_sctrl_get_seeking_speed();

		LOG_DBG("Notifying seeking speed: %d", speed);
		notify(BT_UUID_MCS_SEEKING_SPEED, &speed, sizeof(speed));
	}

#if defined(CONFIG_BT_OTS)
	if (atomic_test_and_clear_bit(client->flags, FLAG_CURRENT_TRACK_OBJ_ID_CHANGED)) {
		uint64_t track_id = media_proxy_sctrl_get_current_track_id();
		uint8_t track_id_le[BT_OTS_OBJ_ID_SIZE];

		sys_put_le48(track_id, track_id_le);

		LOG_DBG_OBJ_ID("Notifying current track ID: ", track_id);
		notify(BT_UUID_MCS_CURRENT_TRACK_OBJ_ID, track_id_le, sizeof(track_id_le));
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_NEXT_TRACK_OBJ_ID_CHANGED)) {
		uint64_t track_id = media_proxy_sctrl_get_next_track_id();

		if (track_id == MPL_NO_TRACK_ID) {
			/* "If the media player has no next track, the length of the
			 * characteristic shall be zero."
			 */
			LOG_DBG_OBJ_ID("Notifying EMPTY next track ID: ", track_id);
			notify(BT_UUID_MCS_NEXT_TRACK_OBJ_ID, NULL, 0);
		} else {
			uint8_t track_id_le[BT_OTS_OBJ_ID_SIZE];

			sys_put_le48(track_id, track_id_le);

			LOG_DBG_OBJ_ID("Notifying next track ID: ", track_id);
			notify(BT_UUID_MCS_NEXT_TRACK_OBJ_ID, track_id_le, sizeof(track_id_le));
		}
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_PARENT_GROUP_OBJ_ID_CHANGED)) {
		uint64_t group_id = media_proxy_sctrl_get_parent_group_id();
		uint8_t group_id_le[BT_OTS_OBJ_ID_SIZE];

		sys_put_le48(group_id, group_id_le);

		LOG_DBG_OBJ_ID("Notifying parent group ID: ", group_id);
		notify(BT_UUID_MCS_PARENT_GROUP_OBJ_ID, &group_id_le, sizeof(group_id_le));
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_CURRENT_GROUP_OBJ_ID_CHANGED)) {
		uint64_t group_id = media_proxy_sctrl_get_current_group_id();
		uint8_t group_id_le[BT_OTS_OBJ_ID_SIZE];

		sys_put_le48(group_id, group_id_le);

		LOG_DBG_OBJ_ID("Notifying current group ID: ", group_id);
		notify(BT_UUID_MCS_CURRENT_GROUP_OBJ_ID, &group_id_le, sizeof(group_id_le));
	}
#endif /* CONFIG_BT_OTS */

	if (atomic_test_and_clear_bit(client->flags, FLAG_TRACK_CHANGED)) {
		LOG_DBG("Notifying track change");
		notify(BT_UUID_MCS_TRACK_CHANGED, NULL, 0);
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_PLAYING_ORDER_CHANGED)) {
		uint8_t order = media_proxy_sctrl_get_playing_order();

		LOG_DBG("Notifying playing order: %d", order);
		notify(BT_UUID_MCS_PLAYING_ORDER, &order, sizeof(order));
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_MEDIA_STATE_CHANGED)) {
		uint8_t state = media_proxy_sctrl_get_media_state();

		LOG_DBG("Notifying media state: %d", state);
		notify(BT_UUID_MCS_MEDIA_STATE, &state, sizeof(state));
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_MEDIA_CONTROL_OPCODES_CHANGED)) {
		uint32_t opcodes = media_proxy_sctrl_get_commands_supported();
		uint32_t opcodes_le = sys_cpu_to_le32(opcodes);

		LOG_DBG("Notifying command opcodes supported: %d (0x%08x)", opcodes, opcodes);
		notify(BT_UUID_MCS_MEDIA_CONTROL_OPCODES, &opcodes_le, sizeof(opcodes_le));
	}

#if defined(CONFIG_BT_OTS)
	if (atomic_test_and_clear_bit(client->flags, FLAG_SEARCH_RESULTS_OBJ_ID_CHANGED)) {
		uint64_t search_id = media_proxy_sctrl_get_search_results_id();
		uint8_t search_id_le[BT_OTS_OBJ_ID_SIZE];

		sys_put_le48(search_id, search_id_le);

		LOG_DBG_OBJ_ID("Notifying search results ID: ", search_id);
		notify(BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID, &search_id_le, sizeof(search_id_le));
	}

	if (atomic_test_and_clear_bit(client->flags, FLAG_SEARCH_CONTROL_POINT_RESULT)) {
		uint8_t result_code = client->search_control_point_result;

		LOG_DBG("Notifying search control point - result: %d", result_code);
		notify(BT_UUID_MCS_SEARCH_CONTROL_POINT, &result_code, sizeof(result_code));
	}
#endif /* CONFIG_BT_OTS */

	if (atomic_test_and_clear_bit(client->flags, FLAG_MEDIA_CONTROL_POINT_RESULT)) {
		LOG_DBG("Notifying control point command - opcode: %d, result: %d",
			client->cmd_ntf.requested_opcode, client->cmd_ntf.result_code);
		notify(BT_UUID_MCS_MEDIA_CONTROL_POINT, &client->cmd_ntf, sizeof(client->cmd_ntf));
	}
}

static void deferred_nfy_work_handler(struct k_work *work)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, notify_cb, NULL);
}

static K_WORK_DEFINE(deferred_nfy_work, deferred_nfy_work_handler);

static void defer_value_ntf(struct bt_conn *conn, void *data)
{
	struct client_state *client = &clients[bt_conn_index(conn)];
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

	atomic_set_bit(client->flags, POINTER_TO_UINT(data));
	k_work_submit(&deferred_nfy_work);
}

static void media_proxy_sctrl_player_name_cb(const char *name)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_PLAYER_NAME_CHANGED));
}

void media_proxy_sctrl_icon_url_cb(const char *name)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, mark_icon_url_changed_cb, NULL);
}

void media_proxy_sctrl_track_changed_cb(void)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_TRACK_CHANGED));
}

void media_proxy_sctrl_track_title_cb(const char *title)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_TRACK_TITLE_CHANGED));
}

void media_proxy_sctrl_track_position_cb(int32_t position)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_TRACK_POSITION_CHANGED));
}

void media_proxy_sctrl_track_duration_cb(int32_t duration)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_TRACK_DURATION_CHANGED));
}

void media_proxy_sctrl_playback_speed_cb(int8_t speed)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_PLAYBACK_SPEED_CHANGED));
}

void media_proxy_sctrl_seeking_speed_cb(int8_t speed)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_SEEKING_SPEED_CHANGED));
}

#if defined(CONFIG_BT_OTS)
void media_proxy_sctrl_current_track_id_cb(uint64_t id)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_CURRENT_TRACK_OBJ_ID_CHANGED));
}

void media_proxy_sctrl_next_track_id_cb(uint64_t id)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_NEXT_TRACK_OBJ_ID_CHANGED));
}

void media_proxy_sctrl_parent_group_id_cb(uint64_t id)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_PARENT_GROUP_OBJ_ID_CHANGED));
}

void media_proxy_sctrl_current_group_id_cb(uint64_t id)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_CURRENT_GROUP_OBJ_ID_CHANGED));
}
#endif /* CONFIG_BT_OTS */

void media_proxy_sctrl_playing_order_cb(uint8_t order)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_PLAYING_ORDER_CHANGED));
}

void media_proxy_sctrl_media_state_cb(uint8_t state)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_MEDIA_STATE_CHANGED));
}

static void defer_media_control_point_ntf(struct bt_conn *conn, void *data)
{
	struct client_state *client = &clients[bt_conn_index(conn)];
	const struct mpl_cmd_ntf *cmd_ntf = data;
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

	if (atomic_test_and_clear_bit(client->flags, FLAG_MEDIA_CONTROL_POINT_BUSY)) {
		client->cmd_ntf = *cmd_ntf;
		atomic_set_bit(client->flags, FLAG_MEDIA_CONTROL_POINT_RESULT);
		k_work_submit(&deferred_nfy_work);
	}
}

void media_proxy_sctrl_command_cb(const struct mpl_cmd_ntf *cmd_ntf)
{
	/* FIXME: Control Point notification shall be sent to operation initiator only */
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_media_control_point_ntf, (void *)cmd_ntf);
}

void media_proxy_sctrl_commands_supported_cb(uint32_t opcodes)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_MEDIA_CONTROL_OPCODES_CHANGED));
}

#if defined(CONFIG_BT_OTS)
static void defer_search_control_point_ntf(struct bt_conn *conn, void *data)
{
	struct client_state *client = &clients[bt_conn_index(conn)];
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

	if (atomic_test_and_clear_bit(client->flags, FLAG_SEARCH_CONTROL_POINT_BUSY)) {
		client->search_control_point_result = POINTER_TO_UINT(data);
		atomic_set_bit(client->flags, FLAG_SEARCH_CONTROL_POINT_RESULT);
		k_work_submit(&deferred_nfy_work);
	}
}

void media_proxy_sctrl_search_cb(uint8_t result_code)
{
	/* FIXME: Control Point notification shall be sent to operation initiator only */
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_search_control_point_ntf,
			UINT_TO_POINTER(result_code));
}

void media_proxy_sctrl_search_results_id_cb(uint64_t id)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, defer_value_ntf,
			UINT_TO_POINTER(FLAG_SEARCH_RESULTS_OBJ_ID_CHANGED));
}
#endif /* CONFIG_BT_OTS */

/* Register the service */
int bt_mcs_init(struct bt_ots_cb *ots_cbs)
{
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

	media_proxy_sctrl_register(&cbs);

	initialized = true;
	return 0;
}
