/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* Disconnection Complete events for a handle that the Host has no connection
 * object for. The next connection with the handle stays up.
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of the commands without a handler,
 * which are zeroed.
 */
#define GENERIC_RP_SIZE 64U

#define CONN_HANDLE 0x0001U

struct cmd_handler {
	uint16_t opcode;
	uint8_t len;
	/* Sets evt to NULL when it sends its events itself */
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

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
	/* LE supported, and BR/EDR not supported */
	rp->features[4] = BIT(5) | BIT(6);
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

static void le_read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	/* Extended advertising makes the Host read the PHY of a new connection */
	sys_put_le64(BIT64(BT_LE_FEAT_BIT_EXT_ADV) | BIT64(BT_LE_FEAT_BIT_PHY_2M), rp->features);
}

static void le_read_buffer_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->le_max_len = sys_cpu_to_le16(27U);
	rp->le_max_num = 3U;
}

static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
}

static void le_read_max_adv_data_len(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				     uint16_t opcode)
{
	struct bt_hci_rp_le_read_max_adv_data_len *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->max_adv_data_len = sys_cpu_to_le16(BT_GAP_ADV_MAX_EXT_ADV_DATA_LEN);
}

static struct net_buf *read_phy_rsp(uint16_t handle, uint8_t status)
{
	struct bt_hci_rp_le_read_phy *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), BT_HCI_OP_LE_READ_PHY);
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
	rp->tx_phy = BT_HCI_LE_PHY_2M;
	rp->rx_phy = BT_HCI_LE_PHY_2M;

	return buf;
}

static void le_read_phy(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	const struct bt_hci_cp_le_read_phy *cp = (const void *)buf->data;

	ARG_UNUSED(len);
	ARG_UNUSED(opcode);

	*evt = read_phy_rsp(sys_le16_to_cpu(cp->handle), BT_HCI_ERR_SUCCESS);
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

/* The controller takes the connection down at once */
static void disconnect(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	const struct bt_hci_cp_disconnect *cp = (const void *)buf->data;
	struct bt_hci_evt_cmd_status *cs;
	struct net_buf *status;

	ARG_UNUSED(len);

	status = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	evt_create(status, BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(status, sizeof(*cs));
	cs->status = BT_HCI_ERR_SUCCESS;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);
	bt_hci_recv(DEVICE_DT_INST_GET(0), status);

	disconn_complete(sys_le16_to_cpu(cp->handle), BT_HCI_ERR_LOCALHOST_TERM_CONN);
	k_sem_give(&disconnect_sem);

	*evt = NULL;
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
		BT_HCI_OP_LE_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_le_read_local_features),
		le_read_local_features,
	},
	{
		BT_HCI_OP_LE_READ_BUFFER_SIZE,
		sizeof(struct bt_hci_rp_le_read_buffer_size),
		le_read_buffer_size,
	},
	{
		BT_HCI_OP_LE_READ_SUPP_STATES,
		sizeof(struct bt_hci_rp_le_read_supp_states),
		le_read_supp_states,
	},
	{
		BT_HCI_OP_LE_READ_MAX_ADV_DATA_LEN,
		sizeof(struct bt_hci_rp_le_read_max_adv_data_len),
		le_read_max_adv_data_len,
	},
	{
		BT_HCI_OP_LE_READ_PHY,
		sizeof(struct bt_hci_rp_le_read_phy),
		le_read_phy,
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

	zassert_equal(type, BT_HCI_H4_CMD, "Unexpected buffer type %u", type);

	cmd_handle(dev, buf);

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

static void le_meta_event(uint8_t subevent, const void *data, uint8_t len)
{
	struct bt_hci_evt_le_meta_event *meta;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + len);
	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = subevent;
	net_buf_add_mem(buf, data, len);
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static const bt_addr_le_t peer = {
	.type = BT_ADDR_LE_RANDOM,
	.a.val = { 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0xc6U },
};

static void conn_complete(uint16_t handle, uint8_t role)
{
	struct bt_hci_evt_le_enh_conn_complete evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.handle = sys_cpu_to_le16(handle),
		.role = role,
		.interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MIN),
		.supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U)),
	};

	bt_addr_le_copy(&evt.peer_addr, &peer);
	le_meta_event(BT_HCI_EVT_LE_ENH_CONN_COMPLETE, &evt, sizeof(evt));
}

/* What the connected callback was last called with */
static uint8_t connected_err;

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static K_SEM_DEFINE(recycled_sem, 0, 1);

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	connected_err = err;
	k_sem_give(&connected_sem);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);

	k_sem_give(&disconnected_sem);
}

static void recycled(void)
{
	k_sem_give(&recycled_sem);
}

static struct bt_conn *connect_central(uint16_t handle)
{
	struct bt_conn *conn = NULL;

	zassert_ok(
		bt_conn_le_create(&peer, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &conn),
		"Creating the connection failed");
	conn_complete(handle, BT_HCI_ROLE_CENTRAL);

	return conn;
}

/* The connection object is freed once the application has let go of it */
static void expect_recycled(struct bt_conn *conn)
{
	if (conn != NULL) {
		bt_conn_unref(conn);
	}

	zassert_ok(k_sem_take(&recycled_sem, K_SECONDS(1)), "The connection was not freed");
}

/* A connection that comes up with the handle afterwards is up */
static void expect_next_connection(uint16_t handle)
{
	struct bt_conn_info info;
	struct bt_conn *conn;

	conn = connect_central(handle);
	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connected callback");
	zassert_equal(connected_err, BT_HCI_ERR_SUCCESS, "Connected with 0x%02x", connected_err);
	zassert_ok(bt_conn_get_info(conn, &info), "Getting the connection info failed");
	zassert_equal(info.state, BT_CONN_STATE_CONNECTED, "Connection in state %u", info.state);

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "Disconnecting failed");
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnected callback");
	expect_recycled(conn);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&disconnect_sem);

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

	zassert_ok(bt_conn_cb_register(&conn_cb), "Registering the callbacks failed");

	return NULL;
}

ZTEST_SUITE(disconn_complete, NULL, setup, before, after, NULL);

/* The Host disconnects a connection that it did not expect. The next
 * connection with the handle is not taken for the one that went down.
 */
static ZTEST(disconn_complete, test_no_conn)
{
	conn_complete(CONN_HANDLE, BT_HCI_ROLE_CENTRAL);
	zassert_ok(k_sem_take(&disconnect_sem, K_SECONDS(1)), "Not disconnected");
	zassert_equal(k_sem_take(&connected_sem, K_MSEC(10)), -EAGAIN,
		      "Connected callback for an unexpected connection");

	expect_next_connection(CONN_HANDLE);
}
