/** @file
 *  @brief Bluetooth Media Control Service - d09r01 IOP1 2019-08-22
 */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <stdbool.h>
#include <device.h>
#include <init.h>
#include <stdio.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/ots.h>

#include "media_proxy.h"
#include "media_proxy_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MCS)
#define LOG_MODULE_NAME bt_mcs
#include "common/log.h"


/* Functions for reading and writing attributes, and for keeping track
 * of attribute configuration changes.
 * Functions for notifications are placed after the service defition.
 */
static ssize_t player_name_read(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	char *name = media_proxy_pl_player_name_get();

	BT_DBG("Player name read: %s", log_strdup(name));

	return bt_gatt_attr_read(conn, attr, buf, len, offset, name,
				 strlen(name));
}

static void player_name_cfg_changed(const struct bt_gatt_attr *attr,
				    uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

#ifdef CONFIG_BT_OTS
static ssize_t icon_id_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	uint64_t icon_id = media_proxy_pl_icon_id_get();

	BT_DBG_OBJ_ID("Icon object read: ", icon_id);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &icon_id,
				 BT_OTS_OBJ_ID_SIZE);
}
#endif /* CONFIG_BT_OTS */

static ssize_t icon_url_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	char *url = media_proxy_pl_icon_url_get();

	BT_DBG("Icon URL read, offset: %d, len:%d, URL: %s", offset, len,
	       log_strdup(url));

	return bt_gatt_attr_read(conn, attr, buf, len, offset, url,
				 strlen(url));
}

static void track_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t track_title_read(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	char *title = media_proxy_pl_track_title_get();

	BT_DBG("Track title read, offset: %d, len:%d, title: %s", offset, len,
	       log_strdup(title));

	return bt_gatt_attr_read(conn, attr, buf, len, offset, title,
				 strlen(title));
}

static void track_title_cfg_changed(const struct bt_gatt_attr *attr,
				    uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t track_duration_read(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	int32_t duration = media_proxy_pl_track_duration_get();

	BT_DBG("Track duration read: %d (0x%08x)", duration, duration);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &duration,
				 sizeof(duration));
}

static void track_duration_cfg_changed(const struct bt_gatt_attr *attr,
				       uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t track_position_read(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	int32_t position = media_proxy_pl_track_position_get();

	BT_DBG("Track position read: %d (0x%08x)", position, position);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &position,
				 sizeof(position));
}

static ssize_t track_position_write(struct bt_conn *conn,
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

	memcpy(&position, buf, len);

	media_proxy_pl_track_position_set(position);

	BT_DBG("Track position write: %d", position);

	return len;
}

static void track_position_cfg_changed(const struct bt_gatt_attr *attr,
				       uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t playback_speed_read(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	int8_t speed = media_proxy_pl_playback_speed_get();

	BT_DBG("Playback speed read: %d", speed);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &speed,
				 sizeof(speed));
}

static ssize_t playback_speed_write(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    const void *buf, uint16_t len, uint16_t offset,
				    uint8_t flags)
{
	int8_t speed;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (len != sizeof(speed)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&speed, buf, len);

	media_proxy_pl_playback_speed_set(speed);

	BT_DBG("Playback speed write: %d", speed);

	return len;
}

static void playback_speed_cfg_changed(const struct bt_gatt_attr *attr,
				       uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t seeking_speed_read(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	int8_t speed = media_proxy_pl_seeking_speed_get();

	BT_DBG("Seeking speed read: %d", speed);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &speed,
				 sizeof(speed));
}

static void seeking_speed_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

#ifdef CONFIG_BT_OTS
static ssize_t track_segments_id_read(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	uint64_t track_segments_id = media_proxy_pl_track_segments_id_get();

	BT_DBG_OBJ_ID("Track segments ID read: ", track_segments_id);
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &track_segments_id, BT_OTS_OBJ_ID_SIZE);
}

static ssize_t current_track_id_read(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr, void *buf,
				     uint16_t len, uint16_t offset)
{
	uint64_t track_id = media_proxy_pl_current_track_id_get();

	BT_DBG_OBJ_ID("Current track ID read: ", track_id);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &track_id,
				 BT_OTS_OBJ_ID_SIZE);
}

static ssize_t current_track_id_write(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      const void *buf, uint16_t len, uint16_t offset,
				      uint8_t flags)
{
	uint64_t id;

	if (offset != 0) {
		BT_DBG("Invalid offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != BT_OTS_OBJ_ID_SIZE) {
		BT_DBG("Invalid length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	id = sys_get_le48((uint8_t *)buf);

	if (IS_ENABLED(CONFIG_BT_DEBUG_MCS)) {
		char str[BT_OTS_OBJ_ID_STR_LEN];
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		BT_DBG("Current track write: offset: %d, len: %d, track ID: %s",
		       offset, len, log_strdup(str));
	}

	media_proxy_pl_current_track_id_set(id);

	return BT_OTS_OBJ_ID_SIZE;
}

static void current_track_id_cfg_changed(const struct bt_gatt_attr *attr,
					 uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t next_track_id_read(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	uint64_t track_id = media_proxy_pl_next_track_id_get();

	if (track_id == MPL_NO_TRACK_ID) {
		BT_DBG("Next track read, but it is empty");
		/* "If the media player has no next track, the length of the */
		/* characteristic shall be zero." */
		return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
	}

	BT_DBG_OBJ_ID("Next track read: ", track_id);
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
					 &track_id, BT_OTS_OBJ_ID_SIZE);
}

static ssize_t next_track_id_write(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset,
				   uint8_t flags)
{
	uint64_t id;

	if (offset != 0) {
		BT_DBG("Invalid offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != BT_OTS_OBJ_ID_SIZE) {
		BT_DBG("Invalid length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	id = sys_get_le48((uint8_t *)buf);

	if (IS_ENABLED(CONFIG_BT_DEBUG_MCS)) {
		char str[BT_OTS_OBJ_ID_STR_LEN];
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		BT_DBG("Next  track write: offset: %d, len: %d, track ID: %s",
		       offset, len, log_strdup(str));
	}

	media_proxy_pl_next_track_id_set(id);

	return BT_OTS_OBJ_ID_SIZE;
}

static void next_track_id_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t current_group_id_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	uint64_t group_id = media_proxy_pl_current_group_id_get();

	BT_DBG_OBJ_ID("Current group read: ", group_id);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &group_id,
				 BT_OTS_OBJ_ID_SIZE);
}

static ssize_t current_group_id_write(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset,
			      uint8_t flags)
{
	uint64_t id;

	if (offset != 0) {
		BT_DBG("Invalid offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != BT_OTS_OBJ_ID_SIZE) {
		BT_DBG("Invalid length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	id = sys_get_le48((uint8_t *)buf);

	if (IS_ENABLED(CONFIG_BT_DEBUG_MCS)) {
		char str[BT_OTS_OBJ_ID_STR_LEN];
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		BT_DBG("Current group ID write: offset: %d, len: %d, track ID: %s",
		       offset, len, log_strdup(str));
	}

	media_proxy_pl_current_group_id_set(id);

	return BT_OTS_OBJ_ID_SIZE;
}

static void current_group_id_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t parent_group_id_read(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr, void *buf,
				    uint16_t len, uint16_t offset)
{
	uint64_t group_id = media_proxy_pl_parent_group_id_get();

	BT_DBG_OBJ_ID("Parent group read: ", group_id);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &group_id,
				 BT_OTS_OBJ_ID_SIZE);
}

static void parent_group_id_cfg_changed(const struct bt_gatt_attr *attr,
					uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}
#endif /* CONFIG_BT_OTS */

static ssize_t playing_order_read(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	uint8_t order = media_proxy_pl_playing_order_get();

	BT_DBG("Playing order read: %d (0x%02x)", order, order);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &order,
				 sizeof(order));
}

static ssize_t playing_order_write(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset,
				   uint8_t flags)
{
	BT_DBG("Playing order write");

	int8_t order;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (len != sizeof(order)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&order, buf, len);

	media_proxy_pl_playing_order_set(order);

	BT_DBG("Playing order write: %d", order);

	return len;
}

static void playing_order_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t playing_orders_supported_read(struct bt_conn *conn,
					     const struct bt_gatt_attr *attr,
					     void *buf, uint16_t len, uint16_t offset)
{
	uint16_t orders = media_proxy_pl_playing_orders_supported_get();

	BT_DBG("Playing orders read: %d (0x%04x)", orders, orders);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &orders,
				 sizeof(orders));
}

static ssize_t media_state_read(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	uint8_t state = media_proxy_pl_media_state_get();

	BT_DBG("Media state read: %d", state);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &state,
				 sizeof(state));
}

static void media_state_cfg_changed(const struct bt_gatt_attr *attr,
				    uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t control_point_write(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset,
				   uint8_t flags)
{
	struct mpl_op_t operation;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(operation.opcode) &&
	    len != sizeof(operation.opcode) + sizeof(operation.param)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&operation.opcode, buf, sizeof(operation.opcode));
	BT_DBG("Opcode: %d", operation.opcode);
	operation.use_param = false;

	if (len == sizeof(operation.opcode) + sizeof(operation.param)) {
		memcpy(&operation.param,
		       (char *)buf + sizeof(operation.opcode),
		       sizeof(operation.param));
		operation.use_param = true;
		BT_DBG("Parameter: %d", operation.param);
	}

	media_proxy_pl_operation_set(operation);

	return len;
}

static void control_point_cfg_changed(const struct bt_gatt_attr *attr,
					 uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t opcodes_supported_read(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	uint32_t opcodes = media_proxy_pl_operations_supported_get();

	BT_DBG("Opcodes_supported read: %d (0x%08x)", opcodes, opcodes);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &opcodes, BT_MCS_OPCODES_SUPPORTED_LEN);
}

static void opcodes_supported_cfg_changed(const struct bt_gatt_attr *attr,
					  uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

#ifdef CONFIG_BT_OTS
static ssize_t search_control_point_write(struct bt_conn *conn,
					  const struct bt_gatt_attr *attr,
					  const void *buf, uint16_t len,
					  uint16_t offset, uint8_t flags)
{
	struct mpl_search_t search = {0};

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len > SEARCH_LEN_MAX || len < SEARCH_LEN_MIN) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&search.search, (char *)buf, len);
	search.len = len;
	BT_DBG("Search length: %d", len);
	BT_HEXDUMP_DBG(&search.search, search.len, "Search content");

	media_proxy_pl_scp_set(search);

	return len;
}

static void search_control_point_cfg_changed(const struct bt_gatt_attr *attr,
					     uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t search_results_id_read(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	uint64_t search_id = media_proxy_pl_search_results_id_get();

	BT_DBG_OBJ_ID("Search results id read: ", search_id);

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
		return bt_gatt_attr_read(conn, attr, buf, len, offset,
					 &search_id, BT_OTS_OBJ_ID_SIZE);
	}
}

static void search_results_id_cfg_changed(const struct bt_gatt_attr *attr,
					  uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}
#endif /* CONFIG_BT_OTS */

static ssize_t content_ctrl_id_read(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr, void *buf,
				    uint16_t len, uint16_t offset)
{
	uint8_t id = media_proxy_pl_content_ctrl_id_get();

	BT_DBG("Content control ID read: %d", id);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &id,
				 sizeof(id));
}

/* Defines for OTS-dependent characteristics - empty if no OTS */
#ifdef CONFIG_BT_OTS
#define ICON_OBJ_ID_CHARACTERISTIC_IF_OTS  \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_ICON_OBJ_ID,	\
	BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT, \
	icon_id_read, NULL, NULL),
#define SEGMENTS_TRACK_GROUP_ID_CHARACTERISTICS_IF_OTS  \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_TRACK_SEGMENTS_OBJ_ID,	\
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT, \
			       track_segments_id_read, NULL, NULL), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_CURRENT_TRACK_OBJ_ID, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP | \
			       BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT | \
			       BT_GATT_PERM_WRITE_ENCRYPT, \
			       current_track_id_read, current_track_id_write, \
			       NULL), \
	BT_GATT_CCC(current_track_id_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_NEXT_TRACK_OBJ_ID, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP | \
			       BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT | \
			       BT_GATT_PERM_WRITE_ENCRYPT, \
			       next_track_id_read, next_track_id_write, NULL), \
	BT_GATT_CCC(next_track_id_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_CURRENT_GROUP_OBJ_ID, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP | \
			       BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT | \
			       BT_GATT_PERM_WRITE_ENCRYPT, \
			       current_group_id_read, current_group_id_write, NULL), \
	BT_GATT_CCC(current_group_id_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_PARENT_GROUP_OBJ_ID, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT, \
			       parent_group_id_read, NULL, NULL), \
	BT_GATT_CCC(parent_group_id_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#define	SEARCH_CHARACTERISTICS_IF_OTS \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_SEARCH_CONTROL_POINT, \
			       BT_GATT_CHRC_WRITE | \
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP | \
			       BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_WRITE_ENCRYPT, \
			       NULL, search_control_point_write, NULL), \
	BT_GATT_CCC(search_control_point_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT, \
			       search_results_id_read, NULL, NULL), \
	BT_GATT_CCC(search_results_id_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),

#else
#define ICON_OBJ_ID_CHARACTERISTIC_IF_OTS
#define SEGMENTS_TRACK_GROUP_ID_CHARACTERISTICS_IF_OTS
#define SEARCH_CHARACTERISTICS_IF_OTS
#endif /* CONFIG_BT_OTS */

/* Media control service attributes */
#define BT_MCS_SERVICE_DEFINITION \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GMCS), \
	BT_GATT_INCLUDE_SERVICE(NULL), /* To be overwritten */ \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_PLAYER_NAME, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT, \
			       player_name_read, NULL, NULL), \
	BT_GATT_CCC(player_name_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	ICON_OBJ_ID_CHARACTERISTIC_IF_OTS \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_ICON_URL, \
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT, \
			       icon_url_read, NULL, NULL), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_TRACK_CHANGED, \
			       BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, \
			       NULL, NULL, NULL), \
	BT_GATT_CCC(track_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_TRACK_TITLE, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT, \
			       track_title_read, NULL, NULL), \
	BT_GATT_CCC(track_title_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_TRACK_DURATION, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT, \
			       track_duration_read, NULL, NULL), \
	BT_GATT_CCC(track_duration_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_TRACK_POSITION, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP | \
			       BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT | \
			       BT_GATT_PERM_WRITE_ENCRYPT, \
			       track_position_read, \
			       track_position_write, NULL), \
	BT_GATT_CCC(track_position_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_PLAYBACK_SPEED, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP | \
			       BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT | \
			       BT_GATT_PERM_WRITE_ENCRYPT, \
			       playback_speed_read, playback_speed_write, \
			       NULL), \
	BT_GATT_CCC(playback_speed_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_SEEKING_SPEED, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT, \
			       seeking_speed_read, NULL, NULL), \
	BT_GATT_CCC(seeking_speed_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	SEGMENTS_TRACK_GROUP_ID_CHARACTERISTICS_IF_OTS \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_PLAYING_ORDER, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP | \
			       BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT | \
			       BT_GATT_PERM_WRITE_ENCRYPT, \
			       playing_order_read, playing_order_write, NULL), \
	BT_GATT_CCC(playing_order_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_PLAYING_ORDERS, \
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT, \
			       playing_orders_supported_read, NULL, NULL), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_MEDIA_STATE, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT, \
			       media_state_read, NULL, NULL), \
	BT_GATT_CCC(media_state_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_MEDIA_CONTROL_POINT, \
			       BT_GATT_CHRC_WRITE | \
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP | \
			       BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_WRITE_ENCRYPT, \
			       NULL, control_point_write, NULL), \
	BT_GATT_CCC(control_point_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_MCS_MEDIA_CONTROL_OPCODES, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT, \
			       opcodes_supported_read, NULL, NULL), \
	BT_GATT_CCC(opcodes_supported_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	SEARCH_CHARACTERISTICS_IF_OTS \
	BT_GATT_CHARACTERISTIC(BT_UUID_CCID, \
			       BT_GATT_CHRC_READ, \
			       BT_GATT_PERM_READ_ENCRYPT, \
			       content_ctrl_id_read, NULL, NULL)

static struct bt_gatt_attr svc_attrs[] = { BT_MCS_SERVICE_DEFINITION };
static struct bt_gatt_service mcs;
#ifdef CONFIG_BT_OTS
static struct bt_ots *ots;
#endif /* CONFIG_BT_OTS */

/* Register the service */
int bt_mcs_init(struct bt_ots_cb *ots_cbs)
{
	static bool initialized;

	if (initialized) {
		BT_DBG("Already initialized");
		return -EALREADY;
	}

	int err;
	mcs = (struct bt_gatt_service)BT_GATT_SERVICE(svc_attrs);

#ifdef CONFIG_BT_OTS
	struct bt_ots_init ots_init;

	ots = bt_ots_free_instance_get();
	if (!ots) {
		BT_ERR("Failed to retrieve OTS instance\n");
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
		BT_ERR("Failed to init OTS (err:%d)\n", err);
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
		BT_ERR("Could not register the MCS service");
#ifdef CONFIG_BT_OTS
		/* TODO: How does one un-register the OTS? */
#endif /* CONFIG_BT_OTS */
		return -ENOEXEC;
	}

	initialized = true;
	return 0;
}

#ifdef CONFIG_BT_OTS
struct bt_ots *bt_mcs_get_ots(void)
{
	return ots;
}
#endif /* CONFIG_BT_OTS */

/* Callback functions from the media player, notifying attributes */
/* Placed here, after the service definition, because they reference it. */

/* Helper function to shorten functions that notify */
static void notify(const struct bt_uuid *uuid, const void *data, uint16_t len)
{
	int err = bt_gatt_notify_uuid(NULL, uuid, mcs.attrs, data, len);

	if (err) {
		if (err == -ENOTCONN) {
			BT_DBG("Notification error: ENOTCONN (%d)", err);
		} else {
			BT_ERR("Notification error: %d", err);
		}
	}
}

void media_proxy_pl_track_changed_cb(void)
{
	BT_DBG("Notifying track change");
	notify(BT_UUID_MCS_TRACK_CHANGED, NULL, 0);
}

void media_proxy_pl_track_title_cb(char *title)
{
	BT_DBG("Notifying track title: %s", log_strdup(title));
	notify(BT_UUID_MCS_TRACK_TITLE, title, strlen(title));
}

void media_proxy_pl_track_position_cb(int32_t position)
{
	BT_DBG("Notifying track position: %d", position);
	notify(BT_UUID_MCS_TRACK_POSITION, &position, sizeof(position));
}

void media_proxy_pl_track_duration_cb(int32_t duration)
{
	BT_DBG("Notifying track duration: %d", duration);
	notify(BT_UUID_MCS_TRACK_DURATION, &duration, sizeof(duration));
}

void media_proxy_pl_playback_speed_cb(int8_t speed)
{
	BT_DBG("Notifying playback speed: %d", speed);
	notify(BT_UUID_MCS_PLAYBACK_SPEED, &speed, sizeof(speed));
}

void media_proxy_pl_seeking_speed_cb(int8_t speed)
{
	BT_DBG("Notifying seeking speed: %d", speed);
	notify(BT_UUID_MCS_SEEKING_SPEED, &speed, sizeof(speed));
}

void media_proxy_pl_current_track_id_cb(uint64_t id)
{
	BT_DBG_OBJ_ID("Notifying current track ID: ", id);
	notify(BT_UUID_MCS_CURRENT_TRACK_OBJ_ID, &id, BT_OTS_OBJ_ID_SIZE);
}

void media_proxy_pl_next_track_id_cb(uint64_t id)
{
	if (id == MPL_NO_TRACK_ID) {
		/* "If the media player has no next track, the length of the */
		/* characteristic shall be zero." */
		BT_DBG_OBJ_ID("Notifying EMPTY next track ID: ", id);
		notify(BT_UUID_MCS_NEXT_TRACK_OBJ_ID, NULL, 0);
	} else {
		BT_DBG_OBJ_ID("Notifying next track ID: ", id);
		notify(BT_UUID_MCS_NEXT_TRACK_OBJ_ID, &id, BT_OTS_OBJ_ID_SIZE);
	}
}

void media_proxy_pl_current_group_id_cb(uint64_t id)
{
	BT_DBG_OBJ_ID("Notifying current group ID: ", id);
	notify(BT_UUID_MCS_CURRENT_GROUP_OBJ_ID, &id, BT_OTS_OBJ_ID_SIZE);
}


void media_proxy_pl_parent_group_id_cb(uint64_t id)
{
	BT_DBG_OBJ_ID("Notifying parent group ID: ", id);
	notify(BT_UUID_MCS_PARENT_GROUP_OBJ_ID, &id, BT_OTS_OBJ_ID_SIZE);
}

void media_proxy_pl_playing_order_cb(uint8_t order)
{
	BT_DBG("Notifying playing order: %d", order);
	notify(BT_UUID_MCS_PLAYING_ORDER, &order, sizeof(order));
}

void media_proxy_pl_media_state_cb(uint8_t state)
{
	BT_DBG("Notifying media state: %d", state);
	notify(BT_UUID_MCS_MEDIA_STATE, &state, sizeof(state));
}

void media_proxy_pl_operation_cb(struct mpl_op_ntf_t op_ntf)
{
	BT_DBG("Notifying control point - opcode: %d, result: %d",
	       op_ntf.requested_opcode, op_ntf.result_code);
	notify(BT_UUID_MCS_MEDIA_CONTROL_POINT, &op_ntf, sizeof(op_ntf));
}

void media_proxy_pl_operations_supported_cb(uint32_t operations)
{
	BT_DBG("Notifying opcodes supported: %d (0x%08x)", operations,
	       operations);
	notify(BT_UUID_MCS_MEDIA_CONTROL_OPCODES, &operations,
	       BT_MCS_OPCODES_SUPPORTED_LEN);
}

void media_proxy_pl_search_cb(uint8_t result_code)
{
	BT_DBG("Notifying search control point - result: %d", result_code);
	notify(BT_UUID_MCS_SEARCH_CONTROL_POINT, &result_code,
	       sizeof(result_code));
}

void media_proxy_pl_search_results_id_cb(uint64_t id)
{
	BT_DBG_OBJ_ID("Notifying search results ID: ", id);
	notify(BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID, &id, BT_OTS_OBJ_ID_SIZE);
}
