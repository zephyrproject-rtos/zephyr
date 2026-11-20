/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* A connection complete event that reports an encrypted link makes the Host
 * check the key size with the controller. A link that falls short is disconnected.
 * The application gets connected() with the error and disconnected(), L2CAP takes
 * no part in it, and the connection object is freed. Until the outcome is known,
 * the application cannot use the connection.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/classic/l2cap_br.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/hci_core.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of the commands without a handler,
 * which are zeroed.
 */
#define GENERIC_RP_SIZE 64U

#define CONN_HANDLE 0x0001U
#define ACL_PKTS    4U

/* Below CONFIG_BT_BR_MIN_ENC_KEY_SIZE, and a size that a controller can report */
#define KEY_SIZE_SHORT 7U
#define KEY_SIZE_LONG  16U

#define L2CAP_PSM      0x1001U
#define L2CAP_SIG_CID  0x0001U
#define L2CAP_CONN_REQ 0x02U
#define L2CAP_SCID     0x0040U

struct cmd_handler {
	uint16_t opcode;
	uint8_t len;
	/* Sets evt to NULL when it sends its events itself, or none */
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

static const bt_addr_t peer = {
	.val = { 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U },
};

/* What the controller answers to HCI Read Encryption Key Size */
static uint8_t key_size;
static uint8_t key_size_status;

/* With key_size_hold, the controller keeps the response until the test sends it
 * with key_size_respond().
 */
static bool key_size_hold;
static K_SEM_DEFINE(key_size_sem, 0, 1);

/* With disconnect_hold, the same for HCI Disconnect with disconnect_respond() */
static bool disconnect_hold;
static K_SEM_DEFINE(disconnect_hold_sem, 0, 1);

/* What the Host sent */
static uint8_t disconnect_reason;
static uint32_t disconnect_count;
static uint32_t remote_features_count;
static uint32_t acl_count;

static K_SEM_DEFINE(accept_sem, 0, 1);
static K_SEM_DEFINE(disconnect_sem, 0, 1);

static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

static void *cmd_complete(struct net_buf **buf, uint8_t plen, uint16_t opcode)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);
	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return net_buf_add(*buf, plen);
}

static void cmd_status(uint16_t opcode, uint8_t status)
{
	struct bt_hci_evt_cmd_status *cs;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(buf, sizeof(*cs));
	cs->status = status;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static void generic_success(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *rp;

	ARG_UNUSED(buf);

	rp = cmd_complete(evt, len, opcode);
	(void)memset(rp, 0, len);
	rp->status = BT_HCI_ERR_SUCCESS;
}

static void read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE and BR/EDR supported */
	rp->features[4] = BIT(6);
}

static void read_supported_commands(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
}

static void read_bd_addr(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
}

static void read_buffer_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			     uint16_t opcode)
{
	struct bt_hci_rp_read_buffer_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	(void)memset(rp, 0, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->acl_max_len = sys_cpu_to_le16(255U);
	rp->acl_max_num = sys_cpu_to_le16(ACL_PKTS);
}

static void accept_conn_req(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(evt);
	ARG_UNUSED(len);

	cmd_status(opcode, BT_HCI_ERR_SUCCESS);
	k_sem_give(&accept_sem);
}

static void read_remote_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				 uint16_t opcode)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(evt);
	ARG_UNUSED(len);

	remote_features_count++;
	cmd_status(opcode, BT_HCI_ERR_SUCCESS);
}

static struct net_buf *key_size_rsp(void)
{
	struct bt_hci_rp_read_encryption_key_size *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE);
	(void)memset(rp, 0, sizeof(*rp));
	rp->status = key_size_status;
	rp->handle = sys_cpu_to_le16(CONN_HANDLE);
	rp->key_size = key_size;

	return buf;
}

static void read_key_size(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(opcode);

	if (key_size_hold) {
		*evt = NULL;
		k_sem_give(&key_size_sem);
		return;
	}

	*evt = key_size_rsp();
}

/* Send the response that the controller holds */
static void key_size_respond(void)
{
	bt_hci_recv(DEVICE_DT_INST_GET(0), key_size_rsp());
}

static void disconn_complete(uint16_t handle, uint8_t reason)
{
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(handle);
	evt->reason = reason;
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

/* The test reports the disconnection when it wants to */
static void disconnect(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	const struct bt_hci_cp_disconnect *cp = (const void *)buf->data;

	ARG_UNUSED(evt);
	ARG_UNUSED(len);

	disconnect_reason = cp->reason;
	disconnect_count++;
	k_sem_give(&disconnect_sem);

	if (disconnect_hold) {
		k_sem_give(&disconnect_hold_sem);
		return;
	}

	cmd_status(opcode, BT_HCI_ERR_SUCCESS);
}

static void disconnect_respond(void)
{
	cmd_status(BT_HCI_OP_DISCONNECT, BT_HCI_ERR_SUCCESS);
}

static void create_connection(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			      uint16_t opcode)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(evt);
	ARG_UNUSED(len);

	cmd_status(opcode, BT_HCI_ERR_SUCCESS);
}

static const struct cmd_handler cmds[] = {
	{
		BT_HCI_OP_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_read_local_features),
		read_local_features,
	},
	{
		BT_HCI_OP_READ_SUPPORTED_COMMANDS,
		sizeof(struct bt_hci_rp_read_supported_commands),
		read_supported_commands,
	},
	{
		BT_HCI_OP_READ_BD_ADDR,
		sizeof(struct bt_hci_rp_read_bd_addr),
		read_bd_addr,
	},
	{
		BT_HCI_OP_READ_BUFFER_SIZE,
		sizeof(struct bt_hci_rp_read_buffer_size),
		read_buffer_size,
	},
	{
		BT_HCI_OP_CONNECT,
		0U,
		create_connection,
	},
	{
		BT_HCI_OP_ACCEPT_CONN_REQ,
		0U,
		accept_conn_req,
	},
	{
		BT_HCI_OP_READ_REMOTE_FEATURES,
		0U,
		read_remote_features,
	},
	{
		BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE,
		0U,
		read_key_size,
	},
	{
		BT_HCI_OP_DISCONNECT,
		0U,
		disconnect,
	},
};

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	ARRAY_FOR_EACH_PTR(cmds, handler) {
		if (handler->opcode == opcode) {
			handler->handler(cmd, &evt, handler->len, opcode);
			if (evt != NULL) {
				bt_hci_recv(dev, evt);
			}

			return;
		}
	}

	generic_success(cmd, &evt, GENERIC_RP_SIZE, opcode);
	bt_hci_recv(dev, evt);
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	if (type == BT_HCI_H4_CMD) {
		cmd_handle(dev, buf);
	} else if (type == BT_HCI_H4_ACL) {
		/* Taken, but its completion is never reported */
		acl_count++;
	}

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.close = driver_close,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static void conn_request(void)
{
	struct bt_hci_evt_conn_request *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_CONN_REQUEST, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_CONN_REQUEST, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->link_type = BT_HCI_ACL;
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static void conn_complete(uint8_t encr_enabled)
{
	struct bt_hci_evt_conn_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_CONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_CONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->link_type = BT_HCI_ACL;
	evt->encr_enabled = encr_enabled;
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

/* An L2CAP Connection Request of the peer */
static void l2cap_conn_req(void)
{
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	net_buf_add_le16(buf, bt_acl_handle_pack(CONN_HANDLE, BT_ACL_START));
	net_buf_add_le16(buf, 12U);
	/* L2CAP header */
	net_buf_add_le16(buf, 8U);
	net_buf_add_le16(buf, L2CAP_SIG_CID);
	/* Signaling header */
	net_buf_add_u8(buf, L2CAP_CONN_REQ);
	net_buf_add_u8(buf, 1U);
	net_buf_add_le16(buf, 4U);
	net_buf_add_le16(buf, L2CAP_PSM);
	net_buf_add_le16(buf, L2CAP_SCID);
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static uint32_t l2cap_accept_count;

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			struct bt_l2cap_chan **chan)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(server);
	ARG_UNUSED(chan);

	l2cap_accept_count++;

	return -ENOMEM;
}

/* What the callbacks were last called with */
static bool connected_called;
static bool disconnected_before_connected;
static uint8_t connected_err;
static uint8_t disconnected_reason;

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static K_SEM_DEFINE(recycled_sem, 0, 1);

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	connected_called = true;
	connected_err = err;
	k_sem_give(&connected_sem);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);

	disconnected_before_connected = !connected_called;
	disconnected_reason = reason;
	k_sem_give(&disconnected_sem);
}

static void recycled(void)
{
	k_sem_give(&recycled_sem);
}

/* The application learned of the connection and its end, and the connection
 * object is freed once the application has let go of it. Nothing went out on the
 * link.
 */
static void expect_gone(struct bt_conn *conn, uint8_t reason)
{
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnected callback");
	zassert_false(disconnected_before_connected, "Disconnected callback came first");
	zassert_equal(disconnected_reason, reason, "Disconnected with 0x%02x", disconnected_reason);

	if (conn != NULL) {
		bt_conn_unref(conn);
	}

	zassert_ok(k_sem_take(&recycled_sem, K_SECONDS(1)), "The connection was not freed");
	zassert_equal(remote_features_count, 0U, "Remote features read");
	zassert_equal(acl_count, 0U, "Data sent");
	zassert_equal(k_sem_count_get(&bt_dev.br.pkts), ACL_PKTS, "%u ACL buffers",
		      k_sem_count_get(&bt_dev.br.pkts));
}

/* The Host refuses the link, and disconnects it */
static void expect_refused(struct bt_conn *conn)
{
	zassert_ok(k_sem_take(&disconnect_sem, K_SECONDS(1)), "Not disconnected");
	zassert_equal(disconnect_reason, BT_HCI_ERR_AUTH_FAIL, "Disconnected with 0x%02x",
		      disconnect_reason);
	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connected callback");
	zassert_equal(connected_err, BT_HCI_ERR_AUTH_FAIL, "Connected with 0x%02x", connected_err);

	disconn_complete(CONN_HANDLE, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	expect_gone(conn, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	zassert_equal(disconnect_count, 1U, "%u disconnections", disconnect_count);
}

static struct bt_conn *create_conn(void)
{
	struct bt_conn *conn;

	conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(conn, "Creating the connection failed");

	return conn;
}

/* A connection to the peer afterwards comes up */
static void expect_next_connection(void)
{
	struct bt_conn_info info;
	struct bt_conn *conn;

	key_size = KEY_SIZE_LONG;
	key_size_status = BT_HCI_ERR_SUCCESS;
	connected_called = false;
	disconnect_count = 0U;
	remote_features_count = 0U;

	conn = create_conn();
	conn_complete(1U);
	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connected callback");
	zassert_equal(connected_err, BT_HCI_ERR_SUCCESS, "Connected with 0x%02x", connected_err);
	zassert_ok(bt_conn_get_info(conn, &info), "Getting the connection info failed");
	zassert_equal(info.state, BT_CONN_STATE_CONNECTED, "Connection in state %u", info.state);
	zassert_equal(bt_conn_get_security(conn), BT_SECURITY_L2, "Security level %d",
		      bt_conn_get_security(conn));
	zassert_equal(disconnect_count, 0U, "%u disconnections", disconnect_count);

	bt_conn_unref(conn);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	key_size = KEY_SIZE_LONG;
	key_size_status = BT_HCI_ERR_SUCCESS;
	key_size_hold = false;
	k_sem_reset(&key_size_sem);
	disconnect_hold = false;
	k_sem_reset(&disconnect_hold_sem);
	k_sem_reset(&accept_sem);
	k_sem_reset(&disconnect_sem);
	disconnect_reason = 0U;
	disconnect_count = 0U;
	remote_features_count = 0U;
	acl_count = 0U;
	l2cap_accept_count = 0U;
	connected_called = false;
	disconnected_before_connected = false;

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	k_sem_reset(&connected_sem);
	k_sem_reset(&disconnected_sem);
	k_sem_reset(&recycled_sem);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Also what a failed test left */
	zassert_ok(bt_disable(), "Bluetooth disable failed");
}

static void *setup(void)
{
	static struct bt_conn_cb conn_cb = {
		.connected = connected,
		.disconnected = disconnected,
		.recycled = recycled,
	};
	static struct bt_l2cap_server server = {
		.psm = L2CAP_PSM,
		.sec_level = BT_SECURITY_L1,
		.accept = l2cap_accept,
	};

	zassert_ok(bt_conn_cb_register(&conn_cb), "Registering the callbacks failed");
	zassert_ok(bt_l2cap_br_server_register(&server), "Registering the L2CAP server failed");

	return NULL;
}

ZTEST_SUITE(br_conn_setup, NULL, setup, before, after, NULL);

/* A key that is long enough: the connection comes up, and L2CAP takes what the
 * peer sends.
 */
ZTEST(br_conn_setup, test_encrypted)
{
	expect_next_connection();
	zassert_equal(remote_features_count, 1U, "Remote features not read");

	l2cap_conn_req();
	k_sleep(K_MSEC(10));
	zassert_equal(l2cap_accept_count, 1U, "%u L2CAP accepts", l2cap_accept_count);
}

/* A central with a key that is too short */
ZTEST(br_conn_setup, test_short_key_central)
{
	struct bt_conn *conn;

	key_size = KEY_SIZE_SHORT;

	conn = create_conn();
	conn_complete(1U);

	expect_refused(conn);

	expect_next_connection();
}

/* A peripheral has its connection parameter update timer running */
ZTEST(br_conn_setup, test_short_key_peripheral)
{
	key_size = KEY_SIZE_SHORT;

	conn_request();
	zassert_ok(k_sem_take(&accept_sem, K_SECONDS(1)), "The connection was not accepted");
	conn_complete(1U);

	expect_refused(NULL);

	expect_next_connection();
}

/* The peer can send until the link is down, and L2CAP takes nothing of it */
ZTEST(br_conn_setup, test_peer_data_on_refused_link)
{
	struct bt_conn *conn;

	key_size = KEY_SIZE_SHORT;

	conn = create_conn();
	conn_complete(1U);
	zassert_ok(k_sem_take(&disconnect_sem, K_SECONDS(1)), "Not disconnected");
	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connected callback");

	l2cap_conn_req();
	k_sleep(K_MSEC(10));
	zassert_equal(l2cap_accept_count, 0U, "%u L2CAP accepts", l2cap_accept_count);

	disconn_complete(CONN_HANDLE, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	expect_gone(conn, BT_HCI_ERR_LOCALHOST_TERM_CONN);
}

/* The controller refuses the read for a link that it still has */
ZTEST(br_conn_setup, test_key_size_read_fails)
{
	struct bt_conn *conn;

	key_size_status = BT_HCI_ERR_UNSPECIFIED;

	conn = create_conn();
	conn_complete(1U);

	expect_refused(conn);

	expect_next_connection();
}

/* While the key size is being read, the application cannot use the connection */
ZTEST(br_conn_setup, test_no_use_during_key_size_read)
{
	struct bt_conn *conn;

	key_size_hold = true;

	conn = create_conn();
	conn_complete(1U);
	zassert_ok(k_sem_take(&key_size_sem, K_SECONDS(1)), "The key size was not read");

	zassert_equal(bt_conn_set_security(conn, BT_SECURITY_L3), -ENOTCONN,
		      "Security requested for a connection that is not up");

	key_size_respond();
	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connected callback");
	zassert_equal(connected_err, BT_HCI_ERR_SUCCESS, "Connected with 0x%02x", connected_err);
	zassert_equal(disconnect_count, 0U, "%u disconnections", disconnect_count);

	bt_conn_unref(conn);
}

/* Nor while the controller handles the disconnection of a refused link */
ZTEST(br_conn_setup, test_no_use_while_refusing)
{
	struct bt_conn *conn;

	key_size = KEY_SIZE_SHORT;
	disconnect_hold = true;

	conn = create_conn();
	conn_complete(1U);
	zassert_ok(k_sem_take(&disconnect_hold_sem, K_SECONDS(1)), "Not disconnected");

	zassert_equal(bt_conn_set_security(conn, BT_SECURITY_L3), -ENOTCONN,
		      "Security requested for a connection that is not up");

	disconnect_respond();
	expect_refused(conn);
}

/* The controller reports the end of the link while it handles the disconnection.
 * The application gets that reason.
 */
ZTEST(br_conn_setup, test_disconnection_while_refusing)
{
	struct bt_conn *conn;

	key_size = KEY_SIZE_SHORT;
	disconnect_hold = true;

	conn = create_conn();
	conn_complete(1U);
	zassert_ok(k_sem_take(&disconnect_hold_sem, K_SECONDS(1)), "Not disconnected");

	disconn_complete(CONN_HANDLE, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	disconnect_respond();

	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connected callback");
	zassert_equal(connected_err, BT_HCI_ERR_REMOTE_USER_TERM_CONN, "Connected with 0x%02x",
		      connected_err);
	expect_gone(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}
