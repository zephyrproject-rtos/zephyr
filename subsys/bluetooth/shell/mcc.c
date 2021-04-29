/** @file
 *  @brief Media Control Client shell implementation
 *
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <bluetooth/mcc.h>
#include <shell/shell.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "bt.h"

#include "../host/audio/otc.h"
#include "../host/audio/media_proxy_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MCS)
#define LOG_MODULE_NAME bt_mcc_shell
#include "common/log.h"

#ifdef __cplusplus
extern "C" {
#endif

static struct bt_mcc_cb_t cb;

#ifdef CONFIG_BT_MCC_OTS
struct object_ids_t {
	uint64_t icon_obj_id;
	uint64_t track_segments_obj_id;
	uint64_t current_track_obj_id;
	uint64_t next_track_obj_id;
	uint64_t current_group_obj_id;
	uint64_t parent_group_obj_id;
	uint64_t search_results_obj_id;
};
static struct object_ids_t obj_ids;
#endif /* CONFIG_BT_MCC_OTS */


static void mcc_init_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell, "MCC init failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "MCC init complete");
}

static void mcc_discover_mcs_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell, "Discovery failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Discovery complete");
}

static void mcc_player_name_read_cb(struct bt_conn *conn, int err, char *name)
{
	if (err) {
		shell_error(ctx_shell, "Player Name read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Player name: %s", name);
}

#ifdef CONFIG_BT_MCC_OTS
static void mcc_icon_obj_id_read_cb(struct bt_conn *conn, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Icon Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Icon object ID: %s", str);

	obj_ids.icon_obj_id = id;
}
#endif /* CONFIG_BT_MCC_OTS */

static void mcc_icon_uri_read_cb(struct bt_conn *conn, int err, char *uri)
{
	if (err) {
		shell_error(ctx_shell, "Icon URI read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Icon URI: 0x%s", uri);
}

static void mcc_track_title_read_cb(struct bt_conn *conn, int err, char *title)
{
	if (err) {
		shell_error(ctx_shell, "Track title read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track title: %s", title);
}

static void mcc_track_changed_ntf_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell, "Icon URI read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track changed");
}

static void mcc_track_dur_read_cb(struct bt_conn *conn, int err, int32_t dur)
{
	if (err) {
		shell_error(ctx_shell, "Track duration read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track duration: %d", dur);
}

static void mcc_track_position_read_cb(struct bt_conn *conn, int err, int32_t pos)
{
	if (err) {
		shell_error(ctx_shell, "Track position read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track Position: %d", pos);
}

static void mcc_track_position_set_cb(struct bt_conn *conn, int err, int32_t pos)
{
	if (err) {
		shell_error(ctx_shell, "Track Position set failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track Position: %d", pos);
}

static void mcc_playback_speed_read_cb(struct bt_conn *conn, int err,
				       int8_t speed)
{
	if (err) {
		shell_error(ctx_shell, "Playback speed read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playback speed: %d", speed);
}

static void mcc_playback_speed_set_cb(struct bt_conn *conn, int err, int8_t speed)
{
	if (err) {
		shell_error(ctx_shell, "Playback speed set failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playback speed: %d", speed);
}

static void mcc_seeking_speed_read_cb(struct bt_conn *conn, int err,
				      int8_t speed)
{
	if (err) {
		shell_error(ctx_shell, "Seeking speed read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Seeking speed: %d", speed);
}


#ifdef CONFIG_BT_MCC_OTS
static void mcc_segments_obj_id_read_cb(struct bt_conn *conn, int err,
					uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell,
			    "Track Segments Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Track Segments Object ID: %s", str);

	obj_ids.track_segments_obj_id = id;
}


static void mcc_current_track_obj_id_read_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Current Track Object ID read failed (%d)",
			    err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Current Track Object ID: %s", str);

	obj_ids.current_track_obj_id = id;
}


static void mcc_next_track_obj_id_read_cb(struct bt_conn *conn, int err,
					  uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Next Track Object ID read failed (%d)",
			    err);
		return;
	}

	if (id == MPL_NO_TRACK_ID) {
		shell_print(ctx_shell, "Next Track Object ID is empty");
	} else {
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		shell_print(ctx_shell, "Next Track Object ID: %s", str);
	}

	obj_ids.next_track_obj_id = id;
}


static void mcc_current_group_obj_id_read_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell,
			    "Current Group Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Current Group Object ID: %s", str);

	obj_ids.current_group_obj_id = id;
}

static void mcc_parent_group_obj_id_read_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell,
			    "Parent Group Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Parent Group Object ID: %s", str);

	obj_ids.parent_group_obj_id = id;
}
#endif /* CONFIG_BT_MCC_OTS */


static void mcc_playing_order_read_cb(struct bt_conn *conn, int err, uint8_t order)
{
	if (err) {
		shell_error(ctx_shell, "Playing order read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playing order: %d", order);
}

static void mcc_playing_order_set_cb(struct bt_conn *conn, int err, uint8_t order)
{
	if (err) {
		shell_error(ctx_shell, "Playing order set failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playing order: %d", order);
}

static void mcc_playing_orders_supported_read_cb(struct bt_conn *conn, int err,
						 uint16_t orders)
{
	if (err) {
		shell_error(ctx_shell,
			    "Playing orders supported read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playing orders supported: %d", orders);
}

static void mcc_media_state_read_cb(struct bt_conn *conn, int err, uint8_t state)
{
	if (err) {
		shell_error(ctx_shell, "Media State read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Media State: %d", state);
}

static void mcc_cp_set_cb(struct bt_conn *conn, int err, struct mpl_op_t op)
{
	if (err) {
		shell_error(ctx_shell,
			    "Control Point set failed (%d) - operation: %d, param: %d",
			    err, op.opcode, op.param);
		return;
	}

	shell_print(ctx_shell, "Operation: %d, param: %d", op.opcode, op.param);
}

static void mcc_cp_ntf_cb(struct bt_conn *conn, int err,
			  struct mpl_op_ntf_t ntf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Control Point notification error (%d) - operation: %d, result: %d",
			    err, ntf.requested_opcode, ntf.result_code);
		return;
	}

	shell_print(ctx_shell, "Operation: %d, result: %d",
		    ntf.requested_opcode, ntf.result_code);
}

static void mcc_opcodes_supported_read_cb(struct bt_conn *conn, int err,
					    uint32_t opcodes)
{
	if (err) {
		shell_error(ctx_shell, "Opcodes supported read failed (%d)",
			    err);
		return;
	}

	shell_print(ctx_shell, "Opcodes supported: %d", opcodes);
}

#ifdef CONFIG_BT_MCC_OTS
static void mcc_scp_set_cb(struct bt_conn *conn, int err,
			   struct mpl_search_t search)
{
	if (err) {
		shell_error(ctx_shell,
			    "Search Control Point set failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Search Control Point set");
}

static void mcc_scp_ntf_cb(struct bt_conn *conn, int err, uint8_t result_code)
{
	if (err) {
		shell_error(ctx_shell,
			    "Search Control Point notification error (%d), result code: %d",
			    err, result_code);
		return;
	}

	shell_print(ctx_shell, "Search control point notification result code: %d",
		    result_code);
}

static void mcc_search_results_obj_id_read_cb(struct bt_conn *conn, int err,
					      uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell,
			    "Search Results Object ID read failed (%d)", err);
		return;
	}

	if (id == 0) {
		shell_print(ctx_shell, "Search Results Object ID: 0x000000000000");
	} else {
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		shell_print(ctx_shell, "Search Results Object ID: %s", str);
	}

	obj_ids.search_results_obj_id = id;
}
#endif /* CONFIG_BT_MCC_OTS */

static void mcc_content_control_id_read_cb(struct bt_conn *conn, int err, uint8_t ccid)
{
	if (err) {
		shell_error(ctx_shell, "Content Control ID read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Content Control ID: %d", ccid);
}

#ifdef CONFIG_BT_MCC_OTS
/**** Callback functions for the included Object Transfer service *************/
static void mcc_otc_obj_selected_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell,
			    "Error in selecting object (err %d)", err);
		return;
	}

	shell_print(ctx_shell, "Selecting object succeeded");
}

static void mcc_otc_obj_metadata_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell,
			    "Error in reading object metadata (err %d)", err);
		return;
	}

	shell_print(ctx_shell, "Reading object metadata succeeded\n");
}

static void mcc_icon_object_read_cb(struct bt_conn *conn, int err,
				    struct net_buf_simple *buf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Icon Object read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Icon content (%d octets)", buf->len);
	shell_hexdump(ctx_shell, buf->data, buf->len);
}

/* TODO: May want to use a parsed type, instead of the raw buf, here */
static void mcc_track_segments_object_read_cb(struct bt_conn *conn, int err,
					      struct net_buf_simple *buf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Track Segments Object read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track Segments content (%d octets)", buf->len);
	shell_hexdump(ctx_shell, buf->data, buf->len);
}

static void mcc_otc_read_current_track_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Current Track Object read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Current Track content (%d octets)", buf->len);
	shell_hexdump(ctx_shell, buf->data, buf->len);
}

static void mcc_otc_read_next_track_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Next Track Object read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Next Track content (%d octets)", buf->len);
	shell_hexdump(ctx_shell, buf->data, buf->len);
}

static void mcc_otc_read_current_group_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Current Group Object read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Current Group content (%d octets)", buf->len);
	shell_hexdump(ctx_shell, buf->data, buf->len);
}

static void mcc_otc_read_parent_group_object_cb(struct bt_conn *conn, int err,
						struct net_buf_simple *buf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Parent Group Object read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Parent Group content (%d octets)", buf->len);
	shell_hexdump(ctx_shell, buf->data, buf->len);
}

#endif /* CONFIG_BT_MCC_OTS */


int cmd_mcc_init(const struct shell *shell, size_t argc, char **argv)
{
	int result;

	if (!ctx_shell) {
		ctx_shell = shell;
	}

	/* Set up the callbacks */
	cb.init              = &mcc_init_cb;
	cb.discover_mcs      = &mcc_discover_mcs_cb;
	cb.player_name_read  = &mcc_player_name_read_cb;
#ifdef CONFIG_BT_MCC_OTS
	cb.icon_obj_id_read  = &mcc_icon_obj_id_read_cb;
#endif /* CONFIG_BT_MCC_OTS */
	cb.icon_uri_read     = &mcc_icon_uri_read_cb;
	cb.track_changed_ntf = &mcc_track_changed_ntf_cb;
	cb.track_title_read  = &mcc_track_title_read_cb;
	cb.track_dur_read    = &mcc_track_dur_read_cb;
	cb.track_position_read = &mcc_track_position_read_cb;
	cb.track_position_set  = &mcc_track_position_set_cb;
	cb.playback_speed_read = &mcc_playback_speed_read_cb;
	cb.playback_speed_set  = &mcc_playback_speed_set_cb;
	cb.seeking_speed_read  = &mcc_seeking_speed_read_cb;
#ifdef CONFIG_BT_MCC_OTS
	cb.segments_obj_id_read      = &mcc_segments_obj_id_read_cb;
	cb.current_track_obj_id_read = &mcc_current_track_obj_id_read_cb;
	cb.next_track_obj_id_read    = &mcc_next_track_obj_id_read_cb;
	cb.current_group_obj_id_read = &mcc_current_group_obj_id_read_cb;
	cb.parent_group_obj_id_read  = &mcc_parent_group_obj_id_read_cb;
#endif /* CONFIG_BT_MCC_OTS */
	cb.playing_order_read	     = &mcc_playing_order_read_cb;
	cb.playing_order_set         = &mcc_playing_order_set_cb;
	cb.playing_orders_supported_read = &mcc_playing_orders_supported_read_cb;
	cb.media_state_read = &mcc_media_state_read_cb;
	cb.cp_set           = &mcc_cp_set_cb;
	cb.cp_ntf           = &mcc_cp_ntf_cb;
	cb.opcodes_supported_read = &mcc_opcodes_supported_read_cb;
#ifdef CONFIG_BT_MCC_OTS
	cb.scp_set            = &mcc_scp_set_cb;
	cb.scp_ntf            = &mcc_scp_ntf_cb;
	cb.search_results_obj_id_read = &mcc_search_results_obj_id_read_cb;
#endif /* CONFIG_BT_MCC_OTS */
	cb.content_control_id_read = &mcc_content_control_id_read_cb;
#ifdef CONFIG_BT_MCC_OTS
	cb.otc_obj_selected = &mcc_otc_obj_selected_cb;
	cb.otc_obj_metadata = &mcc_otc_obj_metadata_cb;
	cb.otc_icon_object  = &mcc_icon_object_read_cb;
	cb.otc_track_segments_object = &mcc_track_segments_object_read_cb;
	cb.otc_current_track_object  = &mcc_otc_read_current_track_object_cb;
	cb.otc_next_track_object     = &mcc_otc_read_next_track_object_cb;
	cb.otc_current_group_object  = &mcc_otc_read_current_group_object_cb;
	cb.otc_parent_group_object   = &mcc_otc_read_parent_group_object_cb;
#endif /* CONFIG_BT_MCC_OTS */

	/* Initialize the module */
	result = bt_mcc_init(default_conn, &cb);

	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_discover_mcs(const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int subscribe = 1;

	if (argc > 1) {
		subscribe = strtol(argv[1], NULL, 0);

		if (subscribe < 0 || subscribe > 1) {
			shell_error(shell, "Invalid parameter");
			return -ENOEXEC;
		}
	}

	result = bt_mcc_discover_mcs(default_conn, (bool)subscribe);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_read_player_name(const struct shell *shell, size_t argc,
			     char *argv[])
{
	int result;

	result = bt_mcc_read_player_name(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

#ifdef CONFIG_BT_MCC_OTS
int cmd_mcc_read_icon_obj_id(const struct shell *shell, size_t argc,
			     char *argv[])
{
	int result;

	result = bt_mcc_read_icon_obj_id(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}
#endif /* CONFIG_BT_MCC_OTS */

int cmd_mcc_read_icon_uri(const struct shell *shell, size_t argc, char *argv[])
{
	int result;

	result = bt_mcc_read_icon_uri(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_read_track_title(const struct shell *shell, size_t argc,
			     char *argv[])
{
	int result;

	result = bt_mcc_read_track_title(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_read_track_duration(const struct shell *shell, size_t argc,
				char *argv[])
{
	int result;

	result = bt_mcc_read_track_dur(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_read_track_position(const struct shell *shell, size_t argc,
				char *argv[])
{
	int result;

	result = bt_mcc_read_track_position(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_set_track_position(const struct shell *shell, size_t argc,
			       char *argv[])
{
	int result;
	int32_t pos = strtol(argv[1], NULL, 0);

	/* Todo: Check input "pos" for validity, or for errors in conversion? */

	result = bt_mcc_set_track_position(default_conn, pos);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}


int cmd_mcc_read_playback_speed(const struct shell *shell, size_t argc,
				char *argv[])
{
	int result;

	result = bt_mcc_read_playback_speed(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}


int cmd_mcc_set_playback_speed(const struct shell *shell, size_t argc,
			       char *argv[])
{
	int result;
	int8_t speed = strtol(argv[1], NULL, 0);

	result = bt_mcc_set_playback_speed(default_conn, speed);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_read_seeking_speed(const struct shell *shell, size_t argc,
			       char *argv[])
{
	int result;

	result = bt_mcc_read_seeking_speed(default_conn);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}


#ifdef CONFIG_BT_MCC_OTS
int cmd_mcc_read_track_segments_obj_id(const struct shell *shell,
				       size_t argc, char *argv[])
{
	int result;

	result = bt_mcc_read_segments_obj_id(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}


int cmd_mcc_read_current_track_obj_id(const struct shell *shell, size_t argc,
				      char *argv[])
{
	int result;

	result = bt_mcc_read_current_track_obj_id(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_read_next_track_obj_id(const struct shell *shell, size_t argc,
				   char *argv[])
{
	int result;

	result = bt_mcc_read_next_track_obj_id(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_read_current_group_obj_id(const struct shell *shell, size_t argc,
				      char *argv[])
{
	int result;

	result = bt_mcc_read_current_group_obj_id(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_read_parent_group_obj_id(const struct shell *shell, size_t argc,
				     char *argv[])
{
	int result;

	result = bt_mcc_read_parent_group_obj_id(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}
#endif /* CONFIG_BT_MCC_OTS */

int cmd_mcc_read_playing_order(const struct shell *shell, size_t argc,
			       char *argv[])
{
	int result;

	result = bt_mcc_read_playing_order(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_set_playing_order(const struct shell *shell, size_t argc,
			      char *argv[])
{
	int result;
	uint8_t order = strtol(argv[1], NULL, 0);

	result = bt_mcc_set_playing_order(default_conn, order);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_read_playing_orders_supported(const struct shell *shell, size_t argc,
					  char *argv[])
{
	int result;

	result = bt_mcc_read_playing_orders_supported(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_read_media_state(const struct shell *shell, size_t argc,
			     char *argv[])
{
	int result;

	result = bt_mcc_read_media_state(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_set_cp(const struct shell *shell, size_t argc, char *argv[])
{
	int result;
	struct mpl_op_t op;


	if (argc > 1) {
		op.opcode = strtol(argv[1], NULL, 0);
	} else {
		shell_error(shell, "Invalid parameter");
		return -ENOEXEC;
	}

	if (argc > 2) {
		op.use_param = true;
		op.param = strtol(argv[2], NULL, 0);
	} else {
		op.use_param = false;
		op.param = 0;
	}

	result = bt_mcc_set_cp(default_conn, op);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_read_opcodes_supported(const struct shell *shell, size_t argc,
				   char *argv[])
{
	int result;

	result = bt_mcc_read_opcodes_supported(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

#ifdef CONFIG_BT_MCC_OTS
int cmd_mcc_set_scp_raw(const struct shell *shell, size_t argc, char *argv[])
{
	int result;
	struct mpl_search_t search;

	search.len = strlen(argv[1]);
	memcpy(search.search, argv[1], search.len);
	BT_DBG("Search string: %s", log_strdup(argv[1]));

	result = bt_mcc_set_scp(default_conn, search);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_set_scp_ioptest(const struct shell *shell, size_t argc,
			    char *argv[])
{
	/* Implementation follows Media control service testspec 0.9.0r13 */
	/* Testcase MCS/SR/SCP/BV-01-C [Search Control Point], rounds 1 - 9 */

	int result;
	uint8_t testround;
	struct mpl_sci_t sci_1 = {0};
	struct mpl_sci_t sci_2 = {0};
	struct mpl_search_t search;


	testround = strtol(argv[1], NULL, 0);

	switch (testround) {
	case 1:
	case 8:
	case 9:
		/* 1, 8 and 9 have the same first SCI */
		sci_1.type = BT_MCS_SEARCH_TYPE_TRACK_NAME;
		strcpy(sci_1.param, "TSPX_Track_Name");
		break;
	case 2:
		sci_1.type = BT_MCS_SEARCH_TYPE_ARTIST_NAME;
		strcpy(sci_1.param, "TSPX_Artist_Name");
		break;
	case 3:
		sci_1.type = BT_MCS_SEARCH_TYPE_ALBUM_NAME;
		strcpy(sci_1.param, "TSPX_Album_Name");
		break;
	case 4:
		sci_1.type = BT_MCS_SEARCH_TYPE_GROUP_NAME;
		strcpy(sci_1.param, "TSPX_Group_Name");
		break;
	case 5:
		sci_1.type = BT_MCS_SEARCH_TYPE_EARLIEST_YEAR;
		strcpy(sci_1.param, "TSPX_Earliest_Year");
		break;
	case 6:
		sci_1.type = BT_MCS_SEARCH_TYPE_LATEST_YEAR;
		strcpy(sci_1.param, "TSPX_Latest_Year");
		break;
	case 7:
		sci_1.type = BT_MCS_SEARCH_TYPE_GENRE;
		strcpy(sci_1.param, "TSPX_Genre");
		break;
	default:
		shell_error(shell, "Invalid parameter");
		return -ENOEXEC;
	}


	switch (testround) {
	case 8:
		sci_2.type = BT_MCS_SEARCH_TYPE_ONLY_TRACKS;
		break;
	case 9:
		sci_2.type = BT_MCS_SEARCH_TYPE_ONLY_GROUPS;
		break;
	}

	/* Length is length of type, plus length of param w/o termination */
	sci_1.len = sizeof(sci_1.type) + strlen(sci_1.param);

	search.len = 0;
	memcpy(&search.search[search.len], &sci_1.len, sizeof(sci_1.len));
	search.len += sizeof(sci_1.len);

	memcpy(&search.search[search.len], &sci_1.type, sizeof(sci_1.type));
	search.len += sizeof(sci_1.type);

	memcpy(&search.search[search.len], &sci_1.param, strlen(sci_1.param));
	search.len += strlen(sci_1.param);

	if (testround == 8 || testround == 9) {

		sci_2.len = sizeof(sci_2.type); /* The type only, no param */

		memcpy(&search.search[search.len], &sci_2.len,
		       sizeof(sci_2.len));
		search.len += sizeof(sci_2.len);

		memcpy(&search.search[search.len], &sci_2.type,
		       sizeof(sci_2.type));
		search.len += sizeof(sci_2.type);
	}

	shell_print(shell, "Search string: ");
	shell_hexdump(shell, (uint8_t *)&search.search, search.len);

	result = bt_mcc_set_scp(default_conn, search);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

#if defined(CONFIG_BT_DEBUG_MCC) && defined(CONFIG_BT_TESTING)
int cmd_mcc_test_set_scp_iop_invalid_type(const struct shell *shell,
					  size_t argc, char *argv[])
{
	int result;
	struct mpl_search_t search;

	search.search[0] = 2;
	search.search[1] = (char)14; /* Invalid type value */
	search.search[2] = 't';  /* Anything */
	search.len = 3;

	shell_print(shell, "Search string: ");
	shell_hexdump(shell, (uint8_t *)&search.search, search.len);

	result = bt_mcc_set_scp(default_conn, search);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

int cmd_mcc_test_set_scp_invalid_sci_len(const struct shell *shell,
					 size_t argc, char *argv[])
{
	/* Reproduce a search that caused hard fault when sent from peer */
	/* in IOP testing */

	int result;
	struct mpl_search_t search;

	char offending_search[9] = {6, 1, 't', 'r', 'a', 'c', 'k', 0, 1 };

	search.len = 9;
	memcpy(&search.search, offending_search, search.len);

	shell_print(shell, "Search string: ");
	shell_hexdump(shell, (uint8_t *)&search.search, search.len);

	result = bt_mcc_set_scp(default_conn, search);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}
#endif /* CONFIG_BT_DEBUG_MCC && CONFIG_BT_TESTING */

int cmd_mcc_read_search_results_obj_id(const struct shell *shell, size_t argc,
				       char *argv[])
{
	int result;

	result = bt_mcc_read_search_results_obj_id(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}
#endif /* CONFIG_BT_MCC_OTS */

int cmd_mcc_read_content_control_id(const struct shell *shell, size_t argc,
				     char *argv[])
{
	int result;

	result = bt_mcc_read_content_control_id(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}


#ifdef CONFIG_BT_MCC_OTS
int cmd_mcc_otc_read_features(const struct shell *shell, size_t argc,
			      char *argv[])
{
	int result;

	result = bt_otc_read_feature(default_conn, bt_mcc_otc_inst());
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_read(const struct shell *shell, size_t argc, char *argv[])
{
	int result;

	result = bt_otc_read(default_conn, bt_mcc_otc_inst());
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_read_metadata(const struct shell *shell, size_t argc,
			      char *argv[])
{
	int result;

	result = bt_otc_obj_metadata_read(default_conn, bt_mcc_otc_inst(),
					  BT_OTC_METADATA_REQ_ALL);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_select(const struct shell *shell, size_t argc, char *argv[])
{
	int result;
	uint64_t id;

	if (argc > 1) {
		id = strtol(argv[1], NULL, 0);
	} else {
		shell_error(shell, "Invalid parameter, requires the Object ID");
		return -ENOEXEC;
	}

	result = bt_otc_select_id(default_conn, bt_mcc_otc_inst(), id);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_select_first(const struct shell *shell, size_t argc,
			     char *argv[])
{
	int result;

	result = bt_otc_select_first(default_conn, bt_mcc_otc_inst());
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_select_last(const struct shell *shell, size_t argc,
			    char *argv[])
{
	int result;

	result = bt_otc_select_last(default_conn, bt_mcc_otc_inst());
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_select_next(const struct shell *shell, size_t argc,
			    char *argv[])
{
	int result;

	result = bt_otc_select_next(default_conn, bt_mcc_otc_inst());
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_select_prev(const struct shell *shell, size_t argc,
			    char *argv[])
{
	int result;

	result = bt_otc_select_prev(default_conn, bt_mcc_otc_inst());
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_read_icon_object(const struct shell *shell, size_t argc,
				 char *argv[])
{
	/* Assumes the Icon Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_icon_object(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_read_track_segments_object(const struct shell *shell,
					   size_t argc, char *argv[])
{
	/* Assumes the Segment Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_track_segments_object(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_read_current_track_object(const struct shell *shell,
					  size_t argc, char *argv[])
{
	/* Assumes the Curent Track Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_current_track_object(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_read_next_track_object(const struct shell *shell,
					  size_t argc, char *argv[])
{
	/* Assumes the Next Track Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_next_track_object(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_read_current_group_object(const struct shell *shell,
					  size_t argc, char *argv[])
{
	/* Assumes the Current Group Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_current_group_object(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_otc_read_parent_group_object(const struct shell *shell,
					 size_t argc, char *argv[])
{
	/* Assumes the Parent Group Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_parent_group_object(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_ots_select_first(const struct shell *shell, size_t argc,
			     char *argv[])
{
	int result;

	result = bt_otc_select_first(default_conn, 0);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_ots_select_last(const struct shell *shell, size_t argc,
			    char *argv[])
{
	int result;

	result = bt_otc_select_last(default_conn, 0);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_ots_select_next(const struct shell *shell, size_t argc,
			    char *argv[])
{
	int result;

	result = bt_otc_select_next(default_conn, 0);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

int cmd_mcc_ots_select_prev(const struct shell *shell, size_t argc,
			    char *argv[])
{
	int result;

	result = bt_otc_select_prev(default_conn, 0);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}
#endif /* CONFIG_BT_MCC_OTS */


static int cmd_mcc(const struct shell *shell, size_t argc, char **argv)
{
	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mcc_cmds,
	SHELL_CMD_ARG(init, NULL, "Initialize client",
		      cmd_mcc_init, 1, 0),
	SHELL_CMD_ARG(discover_mcs, NULL,
		      "Discover Media Control Service [subscribe]",
		      cmd_mcc_discover_mcs, 1, 1),
	SHELL_CMD_ARG(read_player_name, NULL, "Read Media Player Name",
		      cmd_mcc_read_player_name, 1, 0),
#ifdef CONFIG_BT_MCC_OTS
	SHELL_CMD_ARG(read_icon_obj_id, NULL, "Read Icon Object ID",
		      cmd_mcc_read_icon_obj_id, 1, 0),
#endif /* CONFIG_BT_MCC_OTS */
	SHELL_CMD_ARG(read_icon_uri, NULL, "Read Icon URI",
		      cmd_mcc_read_icon_uri, 1, 0),
	SHELL_CMD_ARG(read_track_title, NULL, "Read Track Title",
		      cmd_mcc_read_track_title, 1, 0),
	SHELL_CMD_ARG(read_track_duration, NULL, "Read Track Duration",
		      cmd_mcc_read_track_duration, 1, 0),
	SHELL_CMD_ARG(read_track_position, NULL, "Read Track Position",
		      cmd_mcc_read_track_position, 1, 0),
	SHELL_CMD_ARG(set_track_position, NULL, "Set Track position <position>",
		      cmd_mcc_set_track_position, 2, 0),
	SHELL_CMD_ARG(read_playback_speed, NULL, "Read Playback Speed",
		      cmd_mcc_read_playback_speed, 1, 0),
	SHELL_CMD_ARG(set_playback_speed, NULL, "Set Playback Speed <speed>",
		      cmd_mcc_set_playback_speed, 2, 0),
       SHELL_CMD_ARG(read_seeking_speed, NULL, "Read Seeking Speed",
		      cmd_mcc_read_seeking_speed, 1, 0),
#ifdef CONFIG_BT_MCC_OTS
	SHELL_CMD_ARG(read_track_segments_obj_id, NULL,
		      "Read Track Segments Object ID",
		      cmd_mcc_read_track_segments_obj_id, 1, 0),
	SHELL_CMD_ARG(read_current_track_obj_id, NULL,
		      "Read Current Track Object ID",
		      cmd_mcc_read_current_track_obj_id, 1, 0),
	SHELL_CMD_ARG(read_next_track_obj_id, NULL,
		      "Read Next Track Object ID",
		      cmd_mcc_read_next_track_obj_id, 1, 0),
	SHELL_CMD_ARG(read_current_group_obj_id, NULL,
		      "Read Current Group Object ID",
		      cmd_mcc_read_current_group_obj_id, 1, 0),
	SHELL_CMD_ARG(read_parent_group_obj_id, NULL,
		      "Read Parent Group Object ID",
		      cmd_mcc_read_parent_group_obj_id, 1, 0),
#endif /* CONFIG_BT_MCC_OTS */
	SHELL_CMD_ARG(read_playing_order, NULL, "Read Playing Order",
		      cmd_mcc_read_playing_order, 1, 0),
	SHELL_CMD_ARG(set_playing_order, NULL, "Set Playing Order <order>",
		      cmd_mcc_set_playing_order, 2, 0),
       SHELL_CMD_ARG(read_playing_orders_supported, NULL,
		     "Read Playing Orders Supported",
		      cmd_mcc_read_playing_orders_supported, 1, 0),
	SHELL_CMD_ARG(read_media_state, NULL, "Read Media State",
		      cmd_mcc_read_media_state, 1, 0),
	SHELL_CMD_ARG(set_cp, NULL, "Set opcode/operation <opcode> [argument]",
		      cmd_mcc_set_cp, 2, 1),
	SHELL_CMD_ARG(read_opcodes_supported, NULL, "Read Opcodes Supported",
		      cmd_mcc_read_opcodes_supported, 1, 0),
#ifdef CONFIG_BT_MCC_OTS
	SHELL_CMD_ARG(set_scp_raw, NULL, "Set search <search control item sequence>",
		      cmd_mcc_set_scp_raw, 2, 0),
	SHELL_CMD_ARG(set_scp_ioptest, NULL,
		      "Set search - IOP test round as input <round number>",
		      cmd_mcc_set_scp_ioptest, 2, 0),
#if defined(CONFIG_BT_DEBUG_MCC) && defined(CONFIG_BT_TESTING)
	SHELL_CMD_ARG(test_set_scp_iop_invalid_type, NULL,
		      "Set search - IOP test, invalid type value (test)",
		      cmd_mcc_test_set_scp_iop_invalid_type, 1, 0),
	SHELL_CMD_ARG(test_set_scp_invalid_sci_len, NULL,
		      "Set search - invalid sci length (test)",
		      cmd_mcc_test_set_scp_invalid_sci_len, 1, 0),
#endif /* CONFIG_BT_DEBUG_MCC && CONFIG_BT_TESTING */
	SHELL_CMD_ARG(read_search_results_obj_id, NULL,
		      "Read Search Results Object ID",
		      cmd_mcc_read_search_results_obj_id, 1, 0),
#endif /* CONFIG_BT_MCC_OTS */
	SHELL_CMD_ARG(read_content_control_id, NULL, "Read Content Control ID",
		      cmd_mcc_read_content_control_id, 1, 0),
#ifdef CONFIG_BT_MCC_OTS
	SHELL_CMD_ARG(ots_read_features, NULL, "Read OTC Features",
		      cmd_mcc_otc_read_features, 1, 0),
	SHELL_CMD_ARG(ots_oacp_read, NULL, "Read current object",
		      cmd_mcc_otc_read, 1, 0),
	SHELL_CMD_ARG(ots_read_metadata, NULL, "Read current object's metadata",
		      cmd_mcc_otc_read_metadata, 1, 0),
	SHELL_CMD_ARG(ots_select, NULL, "Select an object by its ID <ID>",
		      cmd_mcc_otc_select, 2, 0),
	SHELL_CMD_ARG(ots_read_icon_object, NULL, "Read Icon Object",
		      cmd_mcc_otc_read_icon_object, 1, 0),
	SHELL_CMD_ARG(ots_read_track_segments_object, NULL,
		      "Read Track Segments Object",
		      cmd_mcc_otc_read_track_segments_object, 1, 0),
	SHELL_CMD_ARG(ots_read_current_track_object, NULL,
		      "Read Current Track Object",
		      cmd_mcc_otc_read_current_track_object, 1, 0),
	SHELL_CMD_ARG(ots_read_next_track_object, NULL,
		      "Read Next Track Object",
		      cmd_mcc_otc_read_next_track_object, 1, 0),
	SHELL_CMD_ARG(ots_read_current_group_object, NULL,
		      "Read Current Group Object",
		      cmd_mcc_otc_read_current_group_object, 1, 0),
	SHELL_CMD_ARG(ots_read_parent_group_object, NULL,
		      "Read Parent Group Object",
		      cmd_mcc_otc_read_parent_group_object, 1, 0),
	SHELL_CMD_ARG(ots_select_first, NULL, "Select first object",
		      cmd_mcc_otc_select_first, 1, 0),
	SHELL_CMD_ARG(ots_select_last, NULL, "Select last object",
		      cmd_mcc_otc_select_last, 1, 0),
	SHELL_CMD_ARG(ots_select_next, NULL, "Select next object",
		      cmd_mcc_otc_select_next, 1, 0),
	SHELL_CMD_ARG(ots_select_previous, NULL, "Select previous object",
		      cmd_mcc_otc_select_prev, 1, 0),
#endif /* CONFIG_BT_MCC_OTS */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(mcc, &mcc_cmds, "MCC commands",
		       cmd_mcc, 1, 1);

#ifdef __cplusplus
}
#endif
