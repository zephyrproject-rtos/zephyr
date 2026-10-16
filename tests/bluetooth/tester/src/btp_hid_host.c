/* btp_hid_host.c - Bluetooth HID Host Profile Tester */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/hid.h>
#include <zephyr/bluetooth/classic/hid_host.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME btp_hid_host
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

NET_BUF_POOL_FIXED_DEFINE(hid_host_tx_pool, 1, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_hid_host *default_hid_host;
static bool hid_host_registered;
static bool hid_host_handlers_registered;
static bt_addr_t hid_host_peer_addr;

#define HID_HOST_REPORT_MAX_LEN                                                                    \
	MIN(CONFIG_BT_BUF_ACL_RX_SIZE,                                                             \
	    BTP_DATA_MAX_SIZE - sizeof(struct btp_hid_host_get_report_ev))

static uint8_t hid_host_ev_buf[sizeof(struct btp_hid_host_get_report_ev) +
			       HID_HOST_REPORT_MAX_LEN] __aligned(4);

BUILD_ASSERT(sizeof(struct btp_hid_host_input_report_ev) <=
	     sizeof(struct btp_hid_host_get_report_ev),
	     "hid_host_ev_buf is sized from the get_report header, which must be the largest");
BUILD_ASSERT(sizeof(hid_host_ev_buf) <= BTP_DATA_MAX_SIZE,
	     "a full hid_host_ev_buf must fit in the BTP data section");

static bool hid_report_type_valid(uint8_t type)
{
	return (type == BT_HID_REPORT_TYPE_INPUT) || (type == BT_HID_REPORT_TYPE_OUTPUT) ||
	       (type == BT_HID_REPORT_TYPE_FEATURE);
}

static void hid_host_send_conn_event(uint8_t opcode)
{
	struct btp_hid_host_connected_ev ev;

	memset(&ev, 0, sizeof(ev));

	bt_addr_copy(&ev.address, &hid_host_peer_addr);

	tester_event(BTP_SERVICE_ID_HID_HOST, opcode, &ev, sizeof(ev));
}

static void hid_host_connected_cb(struct bt_hid_host *hid)
{
	struct bt_conn *conn;
	const bt_addr_t *peer;

	LOG_DBG("HID host connected (%p)", hid);
	default_hid_host = hid;

	bt_addr_copy(&hid_host_peer_addr, BT_ADDR_ANY);

	conn = bt_hid_host_get_conn(hid);
	if (conn != NULL) {
		peer = bt_conn_get_dst_br(conn);
		if (peer != NULL) {
			bt_addr_copy(&hid_host_peer_addr, peer);
		}
		bt_conn_unref(conn);
	}

	hid_host_send_conn_event(BTP_HID_HOST_EV_CONNECTED);
}

static void hid_host_disconnected_cb(struct bt_hid_host *hid)
{
	LOG_DBG("HID host disconnected (%p)", hid);

	hid_host_send_conn_event(BTP_HID_HOST_EV_DISCONNECTED);

	bt_addr_copy(&hid_host_peer_addr, BT_ADDR_ANY);
	default_hid_host = NULL;
}

static void hid_host_input_report_cb(struct bt_hid_host *hid, struct net_buf *buf)
{
	struct btp_hid_host_input_report_ev *ev = (void *)hid_host_ev_buf;
	uint16_t data_len = buf->len;

	LOG_DBG("HID host input report len %u", data_len);

	if (data_len > (sizeof(hid_host_ev_buf) - sizeof(*ev))) {
		/* Truncate rather than drop, so the upper tester gets an event. */
		LOG_ERR("HID host input report too long (%u), truncating", data_len);
		data_len = sizeof(hid_host_ev_buf) - sizeof(*ev);
	}

	ev->data_len = sys_cpu_to_le16(data_len);
	memcpy(ev->data, buf->data, data_len);

	tester_event(BTP_SERVICE_ID_HID_HOST, BTP_HID_HOST_EV_INPUT_REPORT, ev,
		     sizeof(*ev) + data_len);
}

static void hid_host_get_report_cb(struct bt_hid_host *hid, uint8_t result_code, uint8_t type,
				   struct net_buf *buf)
{
	struct btp_hid_host_get_report_ev *ev = (void *)hid_host_ev_buf;
	uint16_t data_len = 0;

	if (result_code == BT_HID_HS_RSP_SUCCESS && buf != NULL) {
		data_len = buf->len;
	}

	LOG_DBG("HID host get report result %u type %u len %u", result_code, type, data_len);

	if (data_len > (sizeof(hid_host_ev_buf) - sizeof(*ev))) {
		/* Truncate rather than drop, so the upper tester gets an event. */
		LOG_ERR("HID host get report too long (%u), truncating", data_len);
		data_len = sizeof(hid_host_ev_buf) - sizeof(*ev);
	}

	ev->result_code = result_code;
	ev->report_type = type;
	ev->data_len = sys_cpu_to_le16(data_len);
	if ((buf != NULL) && (data_len != 0U)) {
		memcpy(ev->data, buf->data, data_len);
	}

	tester_event(BTP_SERVICE_ID_HID_HOST, BTP_HID_HOST_EV_GET_REPORT, ev,
		     sizeof(*ev) + data_len);
}

static void hid_host_set_report_cb(struct bt_hid_host *hid, uint8_t result_code)
{
	struct btp_hid_host_set_report_ev ev;

	LOG_DBG("HID host set report result %u", result_code);

	ev.result_code = result_code;
	tester_event(BTP_SERVICE_ID_HID_HOST, BTP_HID_HOST_EV_SET_REPORT, &ev, sizeof(ev));
}

static void hid_host_get_protocol_cb(struct bt_hid_host *hid, uint8_t result_code, uint8_t protocol)
{
	struct btp_hid_host_get_protocol_ev ev;

	LOG_DBG("HID host get protocol result %u protocol %u", result_code, protocol);

	ev.result_code = result_code;
	ev.protocol = protocol;
	tester_event(BTP_SERVICE_ID_HID_HOST, BTP_HID_HOST_EV_GET_PROTOCOL, &ev, sizeof(ev));
}

static void hid_host_set_protocol_cb(struct bt_hid_host *hid, uint8_t result_code)
{
	struct btp_hid_host_set_protocol_ev ev;

	LOG_DBG("HID host set protocol result %u", result_code);

	ev.result_code = result_code;
	tester_event(BTP_SERVICE_ID_HID_HOST, BTP_HID_HOST_EV_SET_PROTOCOL, &ev, sizeof(ev));
}

static void hid_host_vc_unplug_cb(struct bt_hid_host *hid)
{
	ARG_UNUSED(hid);

	/* The link is torn down next, so disconnected() reports the change. */
	LOG_DBG("HID host virtual cable unplug");
}

static const struct bt_hid_host_cb hid_host_cb = {
	.connected = hid_host_connected_cb,
	.disconnected = hid_host_disconnected_cb,
	.input_report = hid_host_input_report_cb,
	.get_report = hid_host_get_report_cb,
	.set_report = hid_host_set_report_cb,
	.get_protocol = hid_host_get_protocol_cb,
	.set_protocol = hid_host_set_protocol_cb,
	.vc_unplug = hid_host_vc_unplug_cb,
};

/* ---- BTP command handlers --------------------------------------------- */
static uint8_t hid_host_supported(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct btp_hid_host_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_HID_HOST, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static int hid_host_do_register(void)
{
	int err;

	if (hid_host_registered) {
		return 0;
	}

	err = bt_hid_host_register(&hid_host_cb);
	if (err != 0) {
		LOG_ERR("HID host register failed (%d)", err);
		return err;
	}

	hid_host_registered = true;
	LOG_DBG("HID host registered");
	return 0;
}

/* Idempotent: tester_init_hid_host() already registered the profile. */
static uint8_t hid_host_register(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	if (hid_host_do_register() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static int hid_host_do_unregister(void)
{
	int err;

	if (!hid_host_registered) {
		return 0;
	}

	err = bt_hid_host_unregister();
	if (err != 0) {
		LOG_ERR("HID host unregister failed (%d)", err);
		return err;
	}

	hid_host_registered = false;
	default_hid_host = NULL;
	LOG_DBG("HID host unregistered");
	return 0;
}

static uint8_t hid_host_unregister(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	if (hid_host_do_unregister() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_host_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_hid_host_connect_cmd *cp = cmd;
	struct bt_conn *conn;
	bt_addr_t addr;
	int err;

	if (!hid_host_registered) {
		LOG_ERR("HID host not registered");
		return BTP_STATUS_FAILED;
	}

	if (cp->address_type != BTP_BR_ADDRESS_TYPE) {
		LOG_ERR("Invalid address type: 0x%02x", cp->address_type);
		return BTP_STATUS_FAILED;
	}

	bt_addr_copy(&addr, &cp->address);

	conn = bt_conn_lookup_addr_br(&addr);
	if (conn == NULL) {
		LOG_ERR("BR/EDR connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hid_host_connect(conn, &default_hid_host);
	bt_conn_unref(conn);
	if (err != 0) {
		LOG_ERR("HID host connect failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID host connect initiated");
	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_host_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	if (!hid_host_registered) {
		LOG_ERR("HID host not registered");
		return BTP_STATUS_FAILED;
	}

	if (default_hid_host == NULL) {
		LOG_ERR("HID host not connected");
		return BTP_STATUS_FAILED;
	}

	err = bt_hid_host_disconnect(default_hid_host);
	if (err != 0) {
		LOG_ERR("HID host disconnect failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID host disconnect initiated");
	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_host_get_report(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_hid_host_get_report_cmd *cp = cmd;
	int err;

	if (!hid_host_registered || default_hid_host == NULL) {
		LOG_ERR("HID host not connected");
		return BTP_STATUS_FAILED;
	}

	if (!hid_report_type_valid(cp->report_type)) {
		LOG_ERR("Invalid report type: 0x%02x", cp->report_type);
		return BTP_STATUS_FAILED;
	}

	err = bt_hid_host_get_report(default_hid_host, cp->report_type, cp->report_id,
				     sys_le16_to_cpu(cp->buffer_size));
	if (err != 0) {
		LOG_ERR("HID host get report failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID host get report initiated");
	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_host_set_report(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_hid_host_set_report_cmd *cp = cmd;
	struct net_buf *buf;
	uint16_t data_len;
	int err;

	if (!hid_host_registered || default_hid_host == NULL) {
		LOG_ERR("HID host not connected");
		return BTP_STATUS_FAILED;
	}

	if (!hid_report_type_valid(cp->report_type)) {
		LOG_ERR("Invalid report type: 0x%02x", cp->report_type);
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	if (cmd_len < sizeof(*cp) + data_len) {
		LOG_ERR("Invalid command length");
		return BTP_STATUS_FAILED;
	}

	buf = bt_hid_host_create_pdu(&hid_host_tx_pool);
	if (buf == NULL) {
		LOG_ERR("HID host failed to create PDU");
		return BTP_STATUS_FAILED;
	}

	if (data_len > net_buf_tailroom(buf)) {
		LOG_ERR("HID host report too long (%u > %zu)", data_len, net_buf_tailroom(buf));
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	net_buf_add_mem(buf, cp->data, data_len);

	err = bt_hid_host_set_report(default_hid_host, cp->report_type, buf);
	if (err != 0) {
		net_buf_unref(buf);
		LOG_ERR("HID host set report failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID host set report sent (%u bytes)", data_len);
	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_host_get_protocol(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	int err;

	if (!hid_host_registered || default_hid_host == NULL) {
		LOG_ERR("HID host not connected");
		return BTP_STATUS_FAILED;
	}

	err = bt_hid_host_get_protocol(default_hid_host);
	if (err != 0) {
		LOG_ERR("HID host get protocol failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID host get protocol initiated");
	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_host_set_protocol(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_hid_host_set_protocol_cmd *cp = cmd;
	int err;

	if (!hid_host_registered || default_hid_host == NULL) {
		LOG_ERR("HID host not connected");
		return BTP_STATUS_FAILED;
	}

	if ((cp->protocol != BT_HID_PROTOCOL_BOOT_MODE) &&
	    (cp->protocol != BT_HID_PROTOCOL_REPORT_MODE)) {
		LOG_ERR("Invalid protocol mode: 0x%02x", cp->protocol);
		return BTP_STATUS_FAILED;
	}

	err = bt_hid_host_set_protocol(default_hid_host, cp->protocol);
	if (err != 0) {
		LOG_ERR("HID host set protocol failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID host set protocol initiated");
	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_host_send_output_report(const void *cmd, uint16_t cmd_len, void *rsp,
					   uint16_t *rsp_len)
{
	const struct btp_hid_host_send_output_report_cmd *cp = cmd;
	struct net_buf *buf;
	uint16_t data_len;
	int err;

	if (!hid_host_registered || default_hid_host == NULL) {
		LOG_ERR("HID host not connected");
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	if (cmd_len < sizeof(*cp) + data_len) {
		LOG_ERR("Invalid command length");
		return BTP_STATUS_FAILED;
	}

	buf = bt_hid_host_create_pdu(&hid_host_tx_pool);
	if (buf == NULL) {
		LOG_ERR("HID host failed to create PDU");
		return BTP_STATUS_FAILED;
	}

	if (data_len > net_buf_tailroom(buf)) {
		LOG_ERR("HID host report too long (%u > %zu)", data_len, net_buf_tailroom(buf));
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	net_buf_add_mem(buf, cp->data, data_len);

	err = bt_hid_host_output_report(default_hid_host, buf);
	if (err != 0) {
		net_buf_unref(buf);
		LOG_ERR("HID host output report failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID host output report sent (%u bytes)", data_len);
	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_host_suspend(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	if (!hid_host_registered || default_hid_host == NULL) {
		LOG_ERR("HID host not connected");
		return BTP_STATUS_FAILED;
	}

	err = bt_hid_host_suspend(default_hid_host);
	if (err != 0) {
		LOG_ERR("HID host suspend failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID host suspend sent");
	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_host_exit_suspend(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	int err;

	if (!hid_host_registered || default_hid_host == NULL) {
		LOG_ERR("HID host not connected");
		return BTP_STATUS_FAILED;
	}

	err = bt_hid_host_exit_suspend(default_hid_host);
	if (err != 0) {
		LOG_ERR("HID host exit suspend failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID host exit suspend sent");
	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_host_virtual_cable_unplug(const void *cmd, uint16_t cmd_len, void *rsp,
					     uint16_t *rsp_len)
{
	int err;

	if (!hid_host_registered || default_hid_host == NULL) {
		LOG_ERR("HID host not connected");
		return BTP_STATUS_FAILED;
	}

	err = bt_hid_host_virtual_cable_unplug(default_hid_host);
	if (err != 0) {
		LOG_ERR("HID host virtual cable unplug failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID host virtual cable unplug initiated");
	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_POWER_MODE_CONTROL)
static uint8_t hid_host_sniff_subrating(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_hid_host_sniff_subrating_cmd *cp = cmd;
	struct bt_conn *conn;
	bt_addr_t addr;
	int err;

	if (cp->address_type != BTP_BR_ADDRESS_TYPE) {
		LOG_ERR("Invalid address type: 0x%02x", cp->address_type);
		return BTP_STATUS_FAILED;
	}

	bt_addr_copy(&addr, &cp->address);

	conn = bt_conn_lookup_addr_br(&addr);
	if (conn == NULL) {
		LOG_ERR("BR/EDR connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_conn_br_set_sniff_subrating(conn, sys_le16_to_cpu(cp->max_latency),
					     sys_le16_to_cpu(cp->min_remote_timeout),
					     sys_le16_to_cpu(cp->min_local_timeout));
	bt_conn_unref(conn);
	if (err != 0) {
		LOG_ERR("Sniff subrating failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Sniff subrating requested");
	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_POWER_MODE_CONTROL */

/* ---- registration ------------------------------------------------------ */

static const struct btp_handler hid_host_handlers[] = {
	{
		.opcode     = BTP_HID_HOST_READ_SUPPORTED_COMMANDS,
		.index      = BTP_INDEX_NONE,
		.expect_len = 0,
		.func       = hid_host_supported,
	},
	{
		.opcode     = BTP_HID_HOST_CMD_REGISTER,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func       = hid_host_register,
	},
	{
		.opcode     = BTP_HID_HOST_CMD_UNREGISTER,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func       = hid_host_unregister,
	},
	{
		.opcode     = BTP_HID_HOST_CMD_CONNECT,
		.expect_len = sizeof(struct btp_hid_host_connect_cmd),
		.func       = hid_host_connect,
	},
	{
		.opcode     = BTP_HID_HOST_CMD_DISCONNECT,
		.expect_len = sizeof(struct btp_hid_host_disconnect_cmd),
		.func       = hid_host_disconnect,
	},
	{
		.opcode     = BTP_HID_HOST_CMD_GET_REPORT,
		.expect_len = sizeof(struct btp_hid_host_get_report_cmd),
		.func       = hid_host_get_report,
	},
	{
		.opcode     = BTP_HID_HOST_CMD_SET_REPORT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func       = hid_host_set_report,
	},
	{
		.opcode     = BTP_HID_HOST_CMD_GET_PROTOCOL,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func       = hid_host_get_protocol,
	},
	{
		.opcode     = BTP_HID_HOST_CMD_SET_PROTOCOL,
		.expect_len = sizeof(struct btp_hid_host_set_protocol_cmd),
		.func       = hid_host_set_protocol,
	},
	{
		.opcode     = BTP_HID_HOST_CMD_SEND_OUTPUT_REPORT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func       = hid_host_send_output_report,
	},
	{
		.opcode     = BTP_HID_HOST_CMD_SUSPEND,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func       = hid_host_suspend,
	},
	{
		.opcode     = BTP_HID_HOST_CMD_EXIT_SUSPEND,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func       = hid_host_exit_suspend,
	},
	{
		.opcode     = BTP_HID_HOST_CMD_VIRTUAL_CABLE_UNPLUG,
		.expect_len = sizeof(struct btp_hid_host_virtual_cable_unplug_cmd),
		.func       = hid_host_virtual_cable_unplug,
	},
#if defined(CONFIG_BT_POWER_MODE_CONTROL)
	{
		.opcode     = BTP_HID_HOST_CMD_SNIFF_SUBRATING,
		.expect_len = sizeof(struct btp_hid_host_sniff_subrating_cmd),
		.func       = hid_host_sniff_subrating,
	},
#endif /* CONFIG_BT_POWER_MODE_CONTROL */
};

uint8_t tester_init_hid_host(void)
{
	if (!hid_host_handlers_registered) {
		tester_register_command_handlers(BTP_SERVICE_ID_HID_HOST, hid_host_handlers,
						 ARRAY_SIZE(hid_host_handlers));
		hid_host_handlers_registered = true;
	}

	if (hid_host_do_register() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_hid_host(void)
{
	if (hid_host_do_unregister() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
