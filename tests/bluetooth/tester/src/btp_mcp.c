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
#include <zephyr/bluetooth/audio/media_proxy.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "bap_endpoint.h"
#include "btp/btp.h"
#include "../../../../include/zephyr/bluetooth/audio/media_proxy.h"

#define LOG_MODULE_NAME bttester_mcp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#define SEARCH_LEN_MAX 64

static struct net_buf_simple *rx_ev_buf = NET_BUF_SIMPLE(SEARCH_LEN_MAX +
							 sizeof(struct btp_mcp_search_cp_ev));

/* Media Control Profile */
static void btp_send_mcp_found_ev(struct bt_conn *conn, uint8_t status)
{
	struct btp_mcp_discovered_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;

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
	if (err) {
		LOG_DBG("Discovery failed (%d)", err);
		return;
	}

	btp_send_mcp_found_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
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
