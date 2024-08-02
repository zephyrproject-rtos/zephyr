/* btp_mcp.c - Bluetooth MCP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/mcc.h>
#include <zephyr/bluetooth/audio/mcs.h>
#include <../../subsys/bluetooth/audio/mpl_internal.h>
#include <../../subsys/bluetooth/audio/mcc_internal.h>
#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/bluetooth/audio/media_proxy.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "bap_endpoint.h"
#include "btp/btp.h"
#include "../../../../include/zephyr/bluetooth/audio/media_proxy.h"

#define LOG_MODULE_NAME bttester_mcp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

static struct media_player *mcs_media_player;
static uint64_t current_track_obj_id;
static uint64_t next_track_obj_id;
static uint8_t media_player_state;
static uint64_t current_id;
static uint64_t parent_id;
struct service_handles {
	struct {
		uint16_t player_name;
		uint16_t icon_obj_id;
		uint16_t icon_url;
		uint16_t track_changed;
		uint16_t track_title;
		uint16_t track_duration;
		uint16_t track_position;
		uint16_t playback_speed;
		uint16_t seeking_speed;
		uint16_t segments_obj_id;
		uint16_t current_track_obj_id;
		uint16_t next_track_obj_id;
		uint16_t current_group_obj_id;
		uint16_t parent_group_obj_id;
		uint16_t playing_order;
		uint16_t playing_orders_supported;
		uint16_t media_state;
		uint16_t cp;
		uint16_t opcodes_supported;
		uint16_t search_results_obj_id;
		uint16_t scp;
		uint16_t content_control_id;
	} gmcs_handles;

	struct {
		uint16_t feature;
		uint16_t obj_name;
		uint16_t obj_type;
		uint16_t obj_size;
		uint16_t obj_properties;
		uint16_t obj_created;
		uint16_t obj_modified;
		uint16_t obj_id;
		uint16_t oacp;
		uint16_t olcp;
	} ots_handles;
};

struct service_handles svc_chrc_handles;

#define SEARCH_LEN_MAX 64

static struct net_buf_simple *rx_ev_buf = NET_BUF_SIMPLE(SEARCH_LEN_MAX +
							 sizeof(struct btp_mcp_search_cp_ev));

/* Media Control Profile */
static void btp_send_mcp_found_ev(struct bt_conn *conn, uint8_t status,
				  const struct service_handles svc_chrc_handles)
{
	struct btp_mcp_discovered_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.gmcs_handles.player_name = sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.player_name);
	ev.gmcs_handles.icon_obj_id = sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.icon_obj_id);
	ev.gmcs_handles.icon_url = sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.icon_url);
	ev.gmcs_handles.track_changed =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.track_changed);
	ev.gmcs_handles.track_title = sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.track_title);
	ev.gmcs_handles.track_duration =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.track_duration);
	ev.gmcs_handles.track_position =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.track_position);
	ev.gmcs_handles.playback_speed =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.playback_speed);
	ev.gmcs_handles.seeking_speed =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.seeking_speed);
	ev.gmcs_handles.segments_obj_id =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.segments_obj_id);
	ev.gmcs_handles.current_track_obj_id =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.current_track_obj_id);
	ev.gmcs_handles.next_track_obj_id =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.next_track_obj_id);
	ev.gmcs_handles.current_group_obj_id =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.current_group_obj_id);
	ev.gmcs_handles.parent_group_obj_id =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.parent_group_obj_id);
	ev.gmcs_handles.playing_order =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.playing_order);
	ev.gmcs_handles.playing_orders_supported =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.playing_orders_supported);
	ev.gmcs_handles.media_state = sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.media_state);
	ev.gmcs_handles.cp = sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.cp);
	ev.gmcs_handles.opcodes_supported =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.opcodes_supported);
	ev.gmcs_handles.search_results_obj_id =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.search_results_obj_id);
	ev.gmcs_handles.scp = sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.scp);
	ev.gmcs_handles.content_control_id =
		sys_cpu_to_le16(svc_chrc_handles.gmcs_handles.content_control_id);
	ev.ots_handles.feature = sys_cpu_to_le16(svc_chrc_handles.ots_handles.feature);
	ev.ots_handles.obj_name = sys_cpu_to_le16(svc_chrc_handles.ots_handles.obj_name);
	ev.ots_handles.obj_type = sys_cpu_to_le16(svc_chrc_handles.ots_handles.obj_type);
	ev.ots_handles.obj_size = sys_cpu_to_le16(svc_chrc_handles.ots_handles.obj_size);
	ev.ots_handles.obj_properties =
		sys_cpu_to_le16(svc_chrc_handles.ots_handles.obj_properties);
	ev.ots_handles.obj_created = sys_cpu_to_le16(svc_chrc_handles.ots_handles.obj_created);
	ev.ots_handles.obj_modified = sys_cpu_to_le16(svc_chrc_handles.ots_handles.obj_modified);
	ev.ots_handles.obj_id = sys_cpu_to_le16(svc_chrc_handles.ots_handles.obj_id);
	ev.ots_handles.oacp = sys_cpu_to_le16(svc_chrc_handles.ots_handles.oacp);
	ev.ots_handles.olcp = sys_cpu_to_le16(svc_chrc_handles.ots_handles.olcp);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_DISCOVERED_EV, &ev, sizeof(ev));
}

static void btp_send_mcp_track_duration_ev(struct bt_conn *conn, uint8_t status, int32_t dur)
{
	struct btp_mcp_track_duration_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.dur = sys_cpu_to_le32(dur);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_TRACK_DURATION_EV, &ev, sizeof(ev));
}

static void btp_send_mcp_track_position_ev(struct bt_conn *conn, uint8_t status, int32_t pos)
{
	struct btp_mcp_track_position_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.pos = sys_cpu_to_le32(pos);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_TRACK_POSITION_EV, &ev, sizeof(ev));
}

static void btp_send_mcp_playback_speed_ev(struct bt_conn *conn, uint8_t status, int8_t speed)
{
	struct btp_mcp_playback_speed_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.speed = speed;

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_PLAYBACK_SPEED_EV, &ev, sizeof(ev));
}

static void btp_send_mcp_seeking_speed_ev(struct bt_conn *conn, uint8_t status, int8_t speed)
{
	struct btp_mcp_seeking_speed_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.speed = speed;

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_SEEKING_SPEED_EV, &ev, sizeof(ev));
}

static void btp_send_mcp_icon_obj_id_ev(struct bt_conn *conn, uint8_t status, uint64_t id)
{
	struct btp_mcp_icon_obj_id_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	sys_put_le48(id, ev.id);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_ICON_OBJ_ID_EV, &ev, sizeof(ev));
}

static void btp_send_mcp_next_track_obj_id_ev(struct bt_conn *conn, uint8_t status,
					      uint64_t id)
{
	struct btp_mcp_next_track_obj_id_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	sys_put_le48(id, ev.id);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_NEXT_TRACK_OBJ_ID_EV, &ev, sizeof(ev));
}

static void btp_send_parent_group_obj_id_ev(struct bt_conn *conn, uint8_t status, uint64_t id)
{
	struct btp_mcp_parent_group_obj_id_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	sys_put_le48(id, ev.id);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_PARENT_GROUP_OBJ_ID_EV, &ev, sizeof(ev));
}

static void btp_send_current_group_obj_id_ev(struct bt_conn *conn, uint8_t status, uint64_t id)
{
	struct btp_mcp_current_group_obj_id_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	sys_put_le48(id, ev.id);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_CURRENT_GROUP_OBJ_ID_EV, &ev, sizeof(ev));
}

static void btp_send_mcp_playing_order_ev(struct bt_conn *conn, uint8_t status, uint8_t order)
{
	struct btp_mcp_playing_order_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.order = order;

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_PLAYING_ORDER_EV, &ev, sizeof(ev));
}

static void btp_send_mcp_playing_orders_supported_ev(struct bt_conn *conn, uint8_t status,
						     uint16_t orders)
{
	struct btp_mcp_playing_orders_supported_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.orders = sys_cpu_to_le16(orders);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_PLAYING_ORDERS_SUPPORTED_EV, &ev, sizeof(ev));
}

static void btp_send_mcp_media_state_ev(struct bt_conn *conn, uint8_t status, uint8_t state)
{
	struct btp_mcp_media_state_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.state = state;

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_MEDIA_STATE_EV, &ev, sizeof(ev));
}

static void btp_send_mcp_opcodes_supported_ev(struct bt_conn *conn, uint8_t status,
					      uint32_t opcodes)
{
	struct btp_mcp_opcodes_supported_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.opcodes = sys_cpu_to_le32(opcodes);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_OPCODES_SUPPORTED_EV, &ev, sizeof(ev));
}

static void btp_send_mcp_content_control_id_ev(struct bt_conn *conn, uint8_t status,
					       uint8_t ccid)
{
	struct btp_mcp_content_control_id_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.ccid = ccid;

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_CONTENT_CONTROL_ID_EV, &ev, sizeof(ev));
}

static void btp_send_segments_obj_id_ev(struct bt_conn *conn, uint8_t status, uint64_t id)
{
	struct btp_mcp_segments_obj_id_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	sys_put_le48(id, ev.id);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_SEGMENTS_OBJ_ID_EV, &ev, sizeof(ev));
}

static void btp_send_current_track_obj_id_ev(struct bt_conn *conn, uint8_t status, uint64_t id)
{
	struct btp_mcp_current_track_obj_id_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	sys_put_le48(id, ev.id);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_CURRENT_TRACK_OBJ_ID_EV, &ev, sizeof(ev));
}

static void btp_send_media_cp_ev(struct bt_conn *conn, uint8_t status,
				 const struct mpl_cmd *cmd)
{
	struct btp_mcp_media_cp_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.opcode = cmd->opcode;
	ev.use_param = cmd->use_param;
	ev.param = sys_cpu_to_le32(cmd->param);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_MEDIA_CP_EV, &ev, sizeof(ev));
}

static void btp_send_search_cp_ev(struct bt_conn *conn, uint8_t status,
				  const struct mpl_search *search)
{
	struct btp_mcp_search_cp_ev *ev;
	uint8_t param[SEARCH_LEN_MAX];

	net_buf_simple_init(rx_ev_buf, 0);

	ev = net_buf_simple_add(rx_ev_buf, sizeof(*ev));

	bt_addr_le_copy(&ev->address, bt_conn_get_dst(conn));

	ev->status = status;
	ev->param_len = (uint8_t)search->search[0];

	if (ev->param_len > (SEARCH_LEN_MAX - sizeof(ev->param_len))) {
		return;
	}

	ev->search_type = search->search[1];
	strcpy(param, &search->search[2]);
	net_buf_simple_add_mem(rx_ev_buf, param, ev->param_len);

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_SEARCH_CP_EV, ev, sizeof(*ev) + ev->param_len);
}

static void btp_send_command_notifications_ev(struct bt_conn *conn, uint8_t status,
					      const struct mpl_cmd_ntf *ntf)
{
	struct btp_mcp_cmd_ntf_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.requested_opcode = ntf->requested_opcode;
	ev.result_code = ntf->result_code;

	tester_event(BTP_SERVICE_ID_MCP, BTP_MCP_NTF_EV, &ev, sizeof(ev));
}

static void btp_send_search_notifications_ev(struct bt_conn *conn, uint8_t status,
					     uint8_t result_code)
{
	struct btp_scp_cmd_ntf_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.result_code = result_code;

	tester_event(BTP_SERVICE_ID_MCP, BTP_SCP_NTF_EV, &ev, sizeof(ev));
}

static void mcc_discover_cb(struct bt_conn *conn, int err)
{
	struct mcs_instance_t *mcc_inst;

	if (err) {
		LOG_DBG("Discovery failed (%d)", err);
	}

	mcc_inst = lookup_inst_by_conn(conn);

	svc_chrc_handles.gmcs_handles.player_name = mcc_inst->player_name_handle;
	svc_chrc_handles.gmcs_handles.icon_obj_id = mcc_inst->icon_obj_id_handle;
	svc_chrc_handles.gmcs_handles.icon_url = mcc_inst->icon_url_handle;
	svc_chrc_handles.gmcs_handles.track_changed = mcc_inst->track_changed_handle;
	svc_chrc_handles.gmcs_handles.track_title = mcc_inst->track_title_handle;
	svc_chrc_handles.gmcs_handles.track_duration = mcc_inst->track_duration_handle;
	svc_chrc_handles.gmcs_handles.track_position = mcc_inst->track_position_handle;
	svc_chrc_handles.gmcs_handles.playback_speed = mcc_inst->playback_speed_handle;
	svc_chrc_handles.gmcs_handles.seeking_speed = mcc_inst->seeking_speed_handle;
	svc_chrc_handles.gmcs_handles.segments_obj_id = mcc_inst->segments_obj_id_handle;
	svc_chrc_handles.gmcs_handles.current_track_obj_id = mcc_inst->current_track_obj_id_handle;
	svc_chrc_handles.gmcs_handles.next_track_obj_id = mcc_inst->next_track_obj_id_handle;
	svc_chrc_handles.gmcs_handles.current_group_obj_id = mcc_inst->current_group_obj_id_handle;
	svc_chrc_handles.gmcs_handles.parent_group_obj_id = mcc_inst->parent_group_obj_id_handle;
	svc_chrc_handles.gmcs_handles.playing_order = mcc_inst->playing_order_handle;
	svc_chrc_handles.gmcs_handles.playing_orders_supported =
		mcc_inst->playing_orders_supported_handle;
	svc_chrc_handles.gmcs_handles.media_state = mcc_inst->media_state_handle;
	svc_chrc_handles.gmcs_handles.cp = mcc_inst->cp_handle;
	svc_chrc_handles.gmcs_handles.opcodes_supported = mcc_inst->opcodes_supported_handle;
	svc_chrc_handles.gmcs_handles.search_results_obj_id =
		mcc_inst->search_results_obj_id_handle;
	svc_chrc_handles.gmcs_handles.scp = mcc_inst->scp_handle;
	svc_chrc_handles.gmcs_handles.content_control_id = mcc_inst->content_control_id_handle;
	svc_chrc_handles.ots_handles.feature = mcc_inst->otc.feature_handle;
	svc_chrc_handles.ots_handles.obj_name = mcc_inst->otc.obj_name_handle;
	svc_chrc_handles.ots_handles.obj_type = mcc_inst->otc.obj_type_handle;
	svc_chrc_handles.ots_handles.obj_size = mcc_inst->otc.obj_size_handle;
	svc_chrc_handles.ots_handles.obj_id = mcc_inst->otc.obj_id_handle;
	svc_chrc_handles.ots_handles.obj_properties = mcc_inst->otc.obj_properties_handle;
	svc_chrc_handles.ots_handles.obj_created = mcc_inst->otc.obj_created_handle;
	svc_chrc_handles.ots_handles.obj_modified = mcc_inst->otc.obj_modified_handle;
	svc_chrc_handles.ots_handles.oacp = mcc_inst->otc.oacp_handle;
	svc_chrc_handles.ots_handles.olcp = mcc_inst->otc.olcp_handle;

	btp_send_mcp_found_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, svc_chrc_handles);
}

static void mcc_read_track_duration_cb(struct bt_conn *conn, int err, int32_t dur)
{
	LOG_DBG("MCC Read track duration cb (%d)", err);

	btp_send_mcp_track_duration_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, dur);
}

static void mcc_read_track_position_cb(struct bt_conn *conn, int err, int32_t pos)
{
	LOG_DBG("MCC Read track position cb (%d)", err);

	btp_send_mcp_track_position_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, pos);
}

static void mcc_set_track_position_cb(struct bt_conn *conn, int err, int32_t pos)
{
	LOG_DBG("MCC Set track position cb (%d)", err);

	btp_send_mcp_track_position_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, pos);
}

static void mcc_read_playback_speed_cb(struct bt_conn *conn, int err, int8_t speed)
{
	LOG_DBG("MCC read playback speed cb (%d)", err);

	btp_send_mcp_playback_speed_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, speed);
}

static void mcc_set_playback_speed_cb(struct bt_conn *conn, int err, int8_t speed)
{
	LOG_DBG("MCC set playback speed cb (%d)", err);

	btp_send_mcp_playback_speed_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, speed);
}

static void mcc_read_seeking_speed_cb(struct bt_conn *conn, int err, int8_t speed)
{
	LOG_DBG("MCC read seeking speed cb (%d)", err);

	btp_send_mcp_seeking_speed_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, speed);
}

static void mcc_read_icon_obj_id_cb(struct bt_conn *conn, int err, uint64_t id)
{
	LOG_DBG("MCC read Icon Object ID cb (%d)", err);

	btp_send_mcp_icon_obj_id_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, id);
}

static void mcc_read_next_track_obj_id_cb(struct bt_conn *conn, int err, uint64_t id)
{
	LOG_DBG("MCC read next track obj ID cb (%d)", err);

	btp_send_mcp_next_track_obj_id_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, id);
}

static void mcc_set_next_track_obj_id_cb(struct bt_conn *conn, int err, uint64_t id)
{
	LOG_DBG("MCC set next track obj ID cb (%d)", err);

	btp_send_mcp_next_track_obj_id_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, id);
}

static void mcc_read_parent_group_obj_id_cb(struct bt_conn *conn, int err, uint64_t id)
{
	LOG_DBG("MCC read parent group obj ID cb (%d)", err);

	btp_send_parent_group_obj_id_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, id);
}

static void mcc_read_current_group_obj_id_cb(struct bt_conn *conn, int err, uint64_t id)
{
	LOG_DBG("MCC read current group obj ID cb (%d)", err);

	btp_send_current_group_obj_id_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, id);
}

static void mcc_set_current_group_obj_id_cb(struct bt_conn *conn, int err, uint64_t id)
{
	LOG_DBG("MCC read current group obj ID cb (%d)", err);

	btp_send_current_group_obj_id_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, id);
}

static void mcc_read_playing_order_cb(struct bt_conn *conn, int err, uint8_t order)
{
	LOG_DBG("MCC read playing order cb (%d)", err);

	btp_send_mcp_playing_order_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, order);
}

static void mcc_set_playing_order_cb(struct bt_conn *conn, int err, uint8_t order)
{
	LOG_DBG("MCC set playing order cb (%d)", err);

	btp_send_mcp_playing_order_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, order);
}

static void mcc_read_playing_orders_supported_cb(struct bt_conn *conn, int err, uint16_t orders)
{
	LOG_DBG("MCC set playing order cb (%d)", err);

	btp_send_mcp_playing_orders_supported_ev(conn,
						 err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS,
						 orders);
}

static void mcc_media_state_read_cb(struct bt_conn *conn, int err, uint8_t state)
{
	LOG_DBG("MCC media state read cb (%d)", err);

	btp_send_mcp_media_state_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, state);
}

static void mcc_opcodes_supported_cb(struct bt_conn *conn, int err, uint32_t opcodes)
{
	LOG_DBG("MCC opcodes supported cb (%d)", err);

	btp_send_mcp_opcodes_supported_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS,
					  opcodes);
}

static void mcc_content_control_id_cb(struct bt_conn *conn, int err, uint8_t ccid)
{
	LOG_DBG("MCC Content control ID cb (%d)", err);

	btp_send_mcp_content_control_id_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS,
					   ccid);
}

static void mcc_segments_object_id_cb(struct bt_conn *conn, int err, uint64_t id)
{
	LOG_DBG("MCC Segments Object ID cb (%d)", err);

	btp_send_segments_obj_id_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, id);
}

static void mcc_current_track_obj_id_read_cb(struct bt_conn *conn, int err, uint64_t id)
{
	LOG_DBG("MCC Segments Object ID read cb (%d)", err);

	btp_send_current_track_obj_id_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, id);
}

static void mcc_current_track_obj_id_set_cb(struct bt_conn *conn, int err, uint64_t id)
{
	LOG_DBG("MCC Segments Object ID set cb (%d)", err);

	btp_send_current_track_obj_id_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, id);
}

static void mcc_send_cmd_cb(struct bt_conn *conn, int err, const struct mpl_cmd *cmd)
{
	LOG_DBG("MCC Send Command cb (%d)", err);

	btp_send_media_cp_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, cmd);
}

static void mcc_send_search_cb(struct bt_conn *conn, int err, const struct mpl_search *search)
{
	LOG_DBG("MCC Send Search cb (%d)", err);

	btp_send_search_cp_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, search);
}

static void mcc_cmd_ntf_cb(struct bt_conn *conn, int err, const struct mpl_cmd_ntf *ntf)
{
	LOG_DBG("MCC Media Control Point Command Notify cb (%d)", err);

	btp_send_command_notifications_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, ntf);
}

static void mcc_search_ntf_cb(struct bt_conn *conn, int err, uint8_t result_code)
{
	LOG_DBG("MCC Search Control Point Notify cb (%d)", err);

	btp_send_search_notifications_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS,
					 result_code);
}

static struct bt_mcc_cb mcp_cb = {
	.discover_mcs = mcc_discover_cb,
	.read_track_duration = mcc_read_track_duration_cb,
	.read_track_position = mcc_read_track_position_cb,
	.set_track_position = mcc_set_track_position_cb,
	.read_playback_speed = mcc_read_playback_speed_cb,
	.set_playback_speed = mcc_set_playback_speed_cb,
	.read_seeking_speed = mcc_read_seeking_speed_cb,
	.read_playing_order = mcc_read_playing_order_cb,
	.set_playing_order = mcc_set_playing_order_cb,
	.read_playing_orders_supported = mcc_read_playing_orders_supported_cb,
	.read_media_state = mcc_media_state_read_cb,
	.read_opcodes_supported = mcc_opcodes_supported_cb,
	.read_content_control_id = mcc_content_control_id_cb,
	.send_cmd = mcc_send_cmd_cb,
	.cmd_ntf = mcc_cmd_ntf_cb,
#ifdef CONFIG_BT_OTS_CLIENT
	.read_icon_obj_id = mcc_read_icon_obj_id_cb,
	.read_next_track_obj_id = mcc_read_next_track_obj_id_cb,
	.set_next_track_obj_id = mcc_set_next_track_obj_id_cb,
	.read_parent_group_obj_id = mcc_read_parent_group_obj_id_cb,
	.read_current_group_obj_id = mcc_read_current_group_obj_id_cb,
	.set_current_group_obj_id = mcc_set_current_group_obj_id_cb,
	.read_segments_obj_id = mcc_segments_object_id_cb,
	.read_current_track_obj_id = mcc_current_track_obj_id_read_cb,
	.set_current_track_obj_id = mcc_current_track_obj_id_set_cb,
	.send_search = mcc_send_search_cb,
	.search_ntf = mcc_search_ntf_cb,
#endif /* CONFIG_BT_OTS_CLIENT */
};

static uint8_t mcp_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	struct btp_mcp_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_MCP_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_MCP_DISCOVER);
	tester_set_bit(rp->data, BTP_MCP_TRACK_DURATION_READ);
	tester_set_bit(rp->data, BTP_MCP_TRACK_POSITION_READ);
	tester_set_bit(rp->data, BTP_MCP_TRACK_POSITION_SET);
	tester_set_bit(rp->data, BTP_MCP_PLAYBACK_SPEED_READ);
	tester_set_bit(rp->data, BTP_MCP_PLAYBACK_SPEED_SET);

	/* octet 1 */
	tester_set_bit(rp->data, BTP_MCP_SEEKING_SPEED_READ);
	tester_set_bit(rp->data, BTP_MCP_ICON_OBJ_ID_READ);
	tester_set_bit(rp->data, BTP_MCP_NEXT_TRACK_OBJ_ID_READ);
	tester_set_bit(rp->data, BTP_MCP_NEXT_TRACK_OBJ_ID_SET);
	tester_set_bit(rp->data, BTP_MCP_PARENT_GROUP_OBJ_ID_READ);
	tester_set_bit(rp->data, BTP_MCP_CURRENT_GROUP_OBJ_ID_READ);
	tester_set_bit(rp->data, BTP_MCP_CURRENT_GROUP_OBJ_ID_SET);

	/* octet 2 */
	tester_set_bit(rp->data, BTP_MCP_PLAYING_ORDER_READ);
	tester_set_bit(rp->data, BTP_MCP_PLAYING_ORDER_SET);
	tester_set_bit(rp->data, BTP_MCP_PLAYING_ORDERS_SUPPORTED_READ);
	tester_set_bit(rp->data, BTP_MCP_MEDIA_STATE_READ);
	tester_set_bit(rp->data, BTP_MCP_OPCODES_SUPPORTED_READ);
	tester_set_bit(rp->data, BTP_MCP_CONTENT_CONTROL_ID_READ);
	tester_set_bit(rp->data, BTP_MCP_SEGMENTS_OBJ_ID_READ);

	/* octet 3 */
	tester_set_bit(rp->data, BTP_MCP_CURRENT_TRACK_OBJ_ID_READ);
	tester_set_bit(rp->data, BTP_MCP_CURRENT_TRACK_OBJ_ID_SET);
	tester_set_bit(rp->data, BTP_MCP_CMD_SEND);
	tester_set_bit(rp->data, BTP_MCP_CMD_SEARCH);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_mcp_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_discover_mcs(conn, true);
	if (err) {
		LOG_DBG("Discovery failed: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_track_duration_read(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_mcp_track_duration_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Read track duration");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_track_duration(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_track_position_read(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_mcp_track_position_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Read track position");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_track_position(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_track_position_set(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_mcp_track_position_set_cmd *cp = cmd;
	uint32_t pos = sys_le32_to_cpu(cp->pos);
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Set track position");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_set_track_position(conn, pos);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_playback_speed_read(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_mcp_playback_speed_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Read playback speed");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_playback_speed(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_playback_speed_set(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_mcp_playback_speed_set *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Set playback speed");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_set_playback_speed(conn, cp->speed);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_seeking_speed_read(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_mcp_seeking_speed_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Read seeking speed");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_seeking_speed(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_read_icon_obj_id(const void *cmd, uint16_t cmd_len, void *rsp,
				    uint16_t *rsp_len)
{
	const struct btp_mcp_icon_obj_id_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Read Icon Object ID");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_icon_obj_id(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_read_next_track_obj_id(const void *cmd, uint16_t cmd_len, void *rsp,
					  uint16_t *rsp_len)
{
	const struct btp_mcp_next_track_obj_id_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Read Next Track Object ID");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_next_track_obj_id(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_set_next_track_obj_id(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_mcp_set_next_track_obj_id_cmd *cp = cmd;
	uint64_t id = sys_get_le48(cp->id);
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Set Next Track Object ID");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_set_next_track_obj_id(conn, id);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_parent_group_obj_id_read(const void *cmd, uint16_t cmd_len, void *rsp,
					    uint16_t *rsp_len)
{
	const struct btp_mcp_parent_group_obj_id_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Read Parent Group Object ID");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_parent_group_obj_id(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_current_group_obj_id_read(const void *cmd, uint16_t cmd_len, void *rsp,
					     uint16_t *rsp_len)
{
	const struct btp_mcp_current_group_obj_id_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Read Current Group Object ID");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_current_group_obj_id(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_set_current_group_obj_id(const void *cmd, uint16_t cmd_len, void *rsp,
					    uint16_t *rsp_len)
{
	const struct btp_mcp_current_group_obj_id_set_cmd *cp = cmd;
	uint64_t id = sys_get_le48(cp->id);
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Set Next Track Object ID");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_set_current_group_obj_id(conn, id);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_playing_order_read(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_mcp_playing_order_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Read Playing Order");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_playing_order(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_playing_order_set(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_mcp_playing_order_set_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Set Playing Order");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_set_playing_order(conn, cp->order);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_playing_orders_supported_read(const void *cmd, uint16_t cmd_len, void *rsp,
						 uint16_t *rsp_len)
{
	const struct btp_mcp_playing_orders_supported_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Playing orders supported read");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_playing_orders_supported(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_media_state_read(const void *cmd, uint16_t cmd_len, void *rsp,
				    uint16_t *rsp_len)
{
	const struct btp_mcp_media_state_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Media State read");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_media_state(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_opcodes_supported_read(const void *cmd, uint16_t cmd_len, void *rsp,
					  uint16_t *rsp_len)
{
	const struct btp_mcp_opcodes_supported_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Supported opcodes read");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_opcodes_supported(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_content_control_id_read(const void *cmd, uint16_t cmd_len, void *rsp,
					   uint16_t *rsp_len)
{
	const struct btp_mcp_content_control_id_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Content Control ID read");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_content_control_id(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_segments_obj_id_read(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_mcp_segments_obj_id_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Track Segments Object ID read");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_segments_obj_id(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_current_track_obj_id_read(const void *cmd, uint16_t cmd_len, void *rsp,
					     uint16_t *rsp_len)
{
	const struct btp_mcp_current_track_obj_id_read_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Current Track Object ID read");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_read_current_track_obj_id(conn);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_current_track_obj_id_set(const void *cmd, uint16_t cmd_len, void *rsp,
					    uint16_t *rsp_len)
{
	const struct btp_mcp_current_track_obj_id_set_cmd *cp = cmd;
	uint64_t id = sys_get_le48(cp->id);
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Set Current Track Object ID");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_mcc_set_current_track_obj_id(conn, id);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_cmd_send(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_mcp_send_cmd *cp = cmd;
	struct mpl_cmd mcp_cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Send Command");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	mcp_cmd.opcode = cp->opcode;
	mcp_cmd.use_param = cp->use_param;
	mcp_cmd.param = sys_le32_to_cpu(cp->param);

	err = bt_mcc_send_cmd(conn, &mcp_cmd);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcp_cmd_search(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_mcp_search_cmd *cp = cmd;
	struct mpl_search search_items;
	struct mpl_sci scp_cmd;
	struct bt_conn *conn;
	int err;

	LOG_DBG("MCC Send Search Control Point Command");

	if (cmd_len < sizeof(*cp) || cmd_len != sizeof(*cp) + cp->param_len) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	search_items.len = 0;
	scp_cmd.type = cp->type;

	if (scp_cmd.type == BT_MCS_SEARCH_TYPE_ONLY_TRACKS ||
	    scp_cmd.type == BT_MCS_SEARCH_TYPE_ONLY_GROUPS) {
		scp_cmd.len = sizeof(scp_cmd.type);

		if (ARRAY_SIZE(search_items.search) < (sizeof(scp_cmd.len) +
						       sizeof(scp_cmd.type))) {
			return BTP_STATUS_FAILED;
		}

		memcpy(&search_items.search[search_items.len], &scp_cmd.len, sizeof(scp_cmd.len));
		search_items.len += sizeof(scp_cmd.len);

		memcpy(&search_items.search[search_items.len], &scp_cmd.type,
		       sizeof(scp_cmd.type));
		search_items.len += sizeof(scp_cmd.type);
	} else {
		if (cp->param_len >= (SEARCH_LEN_MAX - 1)) {
			return BTP_STATUS_FAILED;
		}

		strcpy(scp_cmd.param, cp->param);
		scp_cmd.len = sizeof(scp_cmd.type) + strlen(scp_cmd.param);

		if (ARRAY_SIZE(search_items.search) < (sizeof(scp_cmd.len) + sizeof(scp_cmd.len) +
						       strlen(scp_cmd.param))) {
			return BTP_STATUS_FAILED;
		}

		memcpy(&search_items.search[search_items.len], &scp_cmd.len, sizeof(scp_cmd.len));
		search_items.len += sizeof(scp_cmd.len);

		memcpy(&search_items.search[search_items.len], &scp_cmd.type,
		       sizeof(scp_cmd.type));
		search_items.len += sizeof(scp_cmd.type);

		strcpy(&search_items.search[search_items.len], scp_cmd.param);
		search_items.len += strlen(scp_cmd.param);
		search_items.search[search_items.len] = '\0';
	}

	err = bt_mcc_send_search(conn, &search_items);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler mcp_handlers[] = {
	{
		.opcode = BTP_MCP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = mcp_supported_commands,
	},
	{
		.opcode = BTP_MCP_DISCOVER,
		.expect_len = sizeof(struct btp_mcp_discover_cmd),
		.func = mcp_discover,
	},
	{
		.opcode = BTP_MCP_TRACK_DURATION_READ,
		.expect_len = sizeof(struct btp_mcp_track_duration_cmd),
		.func = mcp_track_duration_read,
	},
	{
		.opcode = BTP_MCP_TRACK_POSITION_READ,
		.expect_len = sizeof(struct btp_mcp_track_position_read_cmd),
		.func = mcp_track_position_read,
	},
	{
		.opcode = BTP_MCP_TRACK_POSITION_SET,
		.expect_len = sizeof(struct btp_mcp_track_position_set_cmd),
		.func = mcp_track_position_set,
	},
	{
		.opcode = BTP_MCP_PLAYBACK_SPEED_READ,
		.expect_len = sizeof(struct btp_mcp_playback_speed_read_cmd),
		.func = mcp_playback_speed_read,
	},
	{
		.opcode = BTP_MCP_PLAYBACK_SPEED_SET,
		.expect_len = sizeof(struct btp_mcp_playback_speed_set),
		.func = mcp_playback_speed_set,
	},
	{
		.opcode = BTP_MCP_SEEKING_SPEED_READ,
		.expect_len = sizeof(struct btp_mcp_seeking_speed_read_cmd),
		.func = mcp_seeking_speed_read,
	},
	{
		.opcode = BTP_MCP_ICON_OBJ_ID_READ,
		.expect_len = sizeof(struct btp_mcp_icon_obj_id_read_cmd),
		.func = mcp_read_icon_obj_id,
	},
	{
		.opcode = BTP_MCP_NEXT_TRACK_OBJ_ID_READ,
		.expect_len = sizeof(struct btp_mcp_next_track_obj_id_cmd),
		.func = mcp_read_next_track_obj_id,
	},
	{
		.opcode = BTP_MCP_NEXT_TRACK_OBJ_ID_SET,
		.expect_len = sizeof(struct btp_mcp_set_next_track_obj_id_cmd),
		.func = mcp_set_next_track_obj_id,
	},
	{
		.opcode = BTP_MCP_PARENT_GROUP_OBJ_ID_READ,
		.expect_len = sizeof(struct btp_mcp_parent_group_obj_id_read_cmd),
		.func = mcp_parent_group_obj_id_read,
	},
	{
		.opcode = BTP_MCP_CURRENT_GROUP_OBJ_ID_READ,
		.expect_len = sizeof(struct btp_mcp_current_group_obj_id_read_cmd),
		.func = mcp_current_group_obj_id_read,
	},
	{
		.opcode = BTP_MCP_CURRENT_GROUP_OBJ_ID_SET,
		.expect_len = sizeof(struct btp_mcp_current_group_obj_id_set_cmd),
		.func = mcp_set_current_group_obj_id,
	},
	{
		.opcode = BTP_MCP_PLAYING_ORDER_READ,
		.expect_len = sizeof(struct btp_mcp_playing_order_read_cmd),
		.func = mcp_playing_order_read,
	},
	{
		.opcode = BTP_MCP_PLAYING_ORDER_SET,
		.expect_len = sizeof(struct btp_mcp_playing_order_set_cmd),
		.func = mcp_playing_order_set,
	},
	{
		.opcode = BTP_MCP_PLAYING_ORDERS_SUPPORTED_READ,
		.expect_len = sizeof(struct btp_mcp_playing_orders_supported_read_cmd),
		.func = mcp_playing_orders_supported_read,
	},
	{
		.opcode = BTP_MCP_MEDIA_STATE_READ,
		.expect_len = sizeof(struct btp_mcp_media_state_read_cmd),
		.func = mcp_media_state_read,
	},
	{
		.opcode = BTP_MCP_OPCODES_SUPPORTED_READ,
		.expect_len = sizeof(struct btp_mcp_opcodes_supported_read_cmd),
		.func = mcp_opcodes_supported_read,
	},
	{
		.opcode = BTP_MCP_CONTENT_CONTROL_ID_READ,
		.expect_len = sizeof(struct btp_mcp_content_control_id_read_cmd),
		.func = mcp_content_control_id_read,
	},
	{
		.opcode = BTP_MCP_SEGMENTS_OBJ_ID_READ,
		.expect_len = sizeof(struct btp_mcp_segments_obj_id_read_cmd),
		.func = mcp_segments_obj_id_read,
	},
	{
		.opcode = BTP_MCP_CURRENT_TRACK_OBJ_ID_READ,
		.expect_len = sizeof(struct btp_mcp_current_track_obj_id_read_cmd),
		.func = mcp_current_track_obj_id_read,
	},
	{
		.opcode = BTP_MCP_CURRENT_TRACK_OBJ_ID_SET,
		.expect_len = sizeof(struct btp_mcp_current_track_obj_id_set_cmd),
		.func = mcp_current_track_obj_id_set,
	},
	{
		.opcode = BTP_MCP_CMD_SEND,
		.expect_len = sizeof(struct btp_mcp_send_cmd),
		.func = mcp_cmd_send,
	},
	{
		.opcode = BTP_MCP_CMD_SEARCH,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mcp_cmd_search,
	},
};

uint8_t tester_init_mcp(void)
{
	int err;

	err = bt_mcc_init(&mcp_cb);
	if (err) {
		LOG_DBG("Failed to initialize Media Control Client: %d", err);
		return BTP_STATUS_FAILED;
	}

	tester_register_command_handlers(BTP_SERVICE_ID_MCP, mcp_handlers,
					 ARRAY_SIZE(mcp_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_mcp(void)
{
	return BTP_STATUS_SUCCESS;
}

/* Media Control Service */
static uint8_t mcs_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	struct btp_mcs_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_MCS_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_MCS_CMD_SEND);
	tester_set_bit(rp->data, BTP_MCS_CURRENT_TRACK_OBJ_ID_GET);
	tester_set_bit(rp->data, BTP_MCS_NEXT_TRACK_OBJ_ID_GET);
	tester_set_bit(rp->data, BTP_MCS_INACTIVE_STATE_SET);
	tester_set_bit(rp->data, BTP_MCS_PARENT_GROUP_SET);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcs_cmd_send(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_mcs_send_cmd *cp = cmd;
	struct mpl_cmd mcp_cmd;
	int err;

	LOG_DBG("MCS Send Command");

	mcp_cmd.opcode = cp->opcode;
	mcp_cmd.use_param = cp->use_param;
	mcp_cmd.param = (cp->use_param != 0) ? sys_le32_to_cpu(cp->param) : 0;

	err = media_proxy_ctrl_send_command(mcs_media_player, &mcp_cmd);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcs_next_track_obj_id_get(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	struct btp_mcs_next_track_obj_id_rp *rp = rsp;
	int err;

	LOG_DBG("MCS Read Next Track Obj Id");

	err = media_proxy_ctrl_get_next_track_id(mcs_media_player);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	sys_put_le48(next_track_obj_id, rp->id);

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcs_current_track_obj_id_get(const void *cmd, uint16_t cmd_len, void *rsp,
					    uint16_t *rsp_len)
{
	struct btp_mcs_current_track_obj_id_rp *rp = rsp;
	int err;

	LOG_DBG("MCS Read Current Track Obj Id");

	err = media_proxy_ctrl_get_current_track_id(mcs_media_player);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	sys_put_le48(current_track_obj_id, rp->id);

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcs_parent_group_set(const void *cmd, uint16_t cmd_len, void *rsp,
				    uint16_t *rsp_len)
{
	int err;

	LOG_DBG("MCS Set Current Group to be it's own parent");

	err = media_proxy_ctrl_get_current_group_id(mcs_media_player);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	/* Setting current group to be it's own parent */
	mpl_test_unset_parent_group();

	err = media_proxy_ctrl_get_parent_group_id(mcs_media_player);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	if (current_id != parent_id) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mcs_inactive_state_set(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	struct btp_mcs_state_set_rp *rp = rsp;

	LOG_DBG("MCS Set Media Player to inactive state");

	mpl_test_media_state_set(MEDIA_PROXY_STATE_INACTIVE);

	rp->state = media_player_state;

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static void mcs_player_instance_cb(struct media_player *plr, int err)
{
	mcs_media_player = plr;

	LOG_DBG("Media PLayer Instance cb");
}

static void mcs_command_send_cb(struct media_player *player, int err, const struct mpl_cmd *cmd)
{
	LOG_DBG("Media PLayer Send Command cb");
}

static void mcs_current_track_obj_id_cb(struct media_player *player, int err, uint64_t id)
{
	LOG_DBG("Media Player Current Track Object Id cb");

	current_track_obj_id = id;
}

static void mcs_next_track_obj_id_cb(struct media_player *player, int err, uint64_t id)
{
	LOG_DBG("Media PLayer Next Track Object ID cb");

	next_track_obj_id = id;
}

static void mcs_media_state_cb(struct media_player *player, int err, uint8_t state)
{
	LOG_DBG("Media Player State cb");

	media_player_state = state;
}

static void mcs_current_group_id_cb(struct media_player *player, int err, uint64_t id)
{
	LOG_DBG("Media Player Current Group ID cb");

	current_id = id;
}

static void mcs_parent_group_id_cb(struct media_player *player, int err, uint64_t id)
{
	LOG_DBG("Media Player Parent Group ID cb");

	parent_id = id;
}

static struct media_proxy_ctrl_cbs mcs_cbs = {
	.local_player_instance = mcs_player_instance_cb,
	.command_send = mcs_command_send_cb,
	.current_track_id_recv = mcs_current_track_obj_id_cb,
	.next_track_id_recv = mcs_next_track_obj_id_cb,
	.media_state_recv = mcs_media_state_cb,
	.current_group_id_recv = mcs_current_group_id_cb,
	.parent_group_id_recv = mcs_parent_group_id_cb,
};

static const struct btp_handler mcs_handlers[] = {
	{
		.opcode = BTP_MCS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = mcs_supported_commands,
	},
	{
		.opcode = BTP_MCS_CMD_SEND,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mcs_cmd_send,
	},
	{
		.opcode = BTP_MCS_CURRENT_TRACK_OBJ_ID_GET,
		.expect_len = 0,
		.func = mcs_current_track_obj_id_get,
	},
	{
		.opcode = BTP_MCS_NEXT_TRACK_OBJ_ID_GET,
		.expect_len = 0,
		.func = mcs_next_track_obj_id_get,
	},
	{
		.opcode = BTP_MCS_INACTIVE_STATE_SET,
		.expect_len = 0,
		.func = mcs_inactive_state_set,
	},
	{
		.opcode = BTP_MCS_PARENT_GROUP_SET,
		.expect_len = 0,
		.func = mcs_parent_group_set,
	},
};

uint8_t tester_init_mcs(void)
{
	int err;

	err = media_proxy_pl_init();
	if (err) {
		LOG_DBG("Failed to initialize Media Player: %d", err);
		return BTP_STATUS_FAILED;
	}

	err = media_proxy_ctrl_register(&mcs_cbs);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	tester_register_command_handlers(BTP_SERVICE_ID_GMCS, mcs_handlers,
					 ARRAY_SIZE(mcs_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_mcs(void)
{
	return BTP_STATUS_SUCCESS;
}
