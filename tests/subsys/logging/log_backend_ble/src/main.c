/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test, LOG_LEVEL_INF);

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define GENERIC_RP_SIZE 64U
#define LE_ACL_MAX_LEN  27U
#define LE_ACL_MAX_NUM  3U

/* The ATT PDUs that the peers send and receive */
#define ATT_OP_WRITE_REQ 0x12U
#define ATT_OP_WRITE_RSP 0x13U
#define ATT_OP_NOTIFY    0x1bU

/* Opcode, handle and value */
#define WRITE_REQ_LEN 5U

/* The characteristic that the backend notifies its output with */
#define LOG_TX_UUID                                                                                \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x6E400003, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E))

/* How long to wait for a PDU that the Host is to send. It sends it without
 * simulated time passing, so this only bounds a failed wait.
 */
#define TX_TIMEOUT K_MSEC(100)

/* A central that has a connection with the Host */
struct test_peer {
	uint16_t handle;
	bool connected;
	/* The data packets that the controller has not reported as completed */
	atomic_t unreported;
	/* The notifications of the log characteristic, and how many there were
	 * when the last line was logged
	 */
	atomic_t notified;
	atomic_val_t notified_before;
};

static struct test_peer peers[] = {
	{ .handle = 0x0001U },
	{ .handle = 0x0002U },
};

BUILD_ASSERT(ARRAY_SIZE(peers) <= CONFIG_BT_MAX_CONN);

static uint16_t value_handle;
static uint16_t ccc_handle;

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static K_SEM_DEFINE(write_rsp_sem, 0, 1);

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

static void generic_success(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *rp;

	rp = cmd_complete(evt, GENERIC_RP_SIZE, opcode);
	(void)memset(rp, 0, GENERIC_RP_SIZE);
	rp->status = BT_HCI_ERR_SUCCESS;
}

static void read_local_features(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE supported, BR/EDR not supported */
	rp->features[4] = BIT(5) | BIT(6);
}

static void read_supported_commands(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
}

static void read_bd_addr(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->bdaddr = (bt_addr_t){ .val = { 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6 } };
}

static void le_read_buffer_size(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->le_max_len = sys_cpu_to_le16(LE_ACL_MAX_LEN);
	rp->le_max_num = LE_ACL_MAX_NUM;
}

static void le_read_supp_states(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
}

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *hdr;
	uint16_t opcode;

	hdr = net_buf_pull_mem(cmd, sizeof(*hdr));
	opcode = sys_le16_to_cpu(hdr->opcode);

	switch (opcode) {
	case BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS:
		/* Has no response */
		return;
	case BT_HCI_OP_READ_LOCAL_FEATURES:
		read_local_features(&evt, opcode);
		break;
	case BT_HCI_OP_READ_SUPPORTED_COMMANDS:
		read_supported_commands(&evt, opcode);
		break;
	case BT_HCI_OP_READ_BD_ADDR:
		read_bd_addr(&evt, opcode);
		break;
	case BT_HCI_OP_LE_READ_BUFFER_SIZE:
		le_read_buffer_size(&evt, opcode);
		break;
	case BT_HCI_OP_LE_READ_SUPP_STATES:
		le_read_supp_states(&evt, opcode);
		break;
	default:
		generic_success(&evt, opcode);
		break;
	}

	bt_hci_recv(dev, evt);
}

static void completed_handler(struct k_work *work)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));

	ARG_UNUSED(work);

	ARRAY_FOR_EACH_PTR(peers, peer) {
		struct bt_hci_evt_num_completed_packets *ev;
		struct bt_hci_handle_count *hc;
		struct net_buf *evt;
		atomic_val_t count;

		count = atomic_clear(&peer->unreported);
		if (count == 0) {
			continue;
		}

		evt = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
		evt_create(evt, BT_HCI_EVT_NUM_COMPLETED_PACKETS, sizeof(*ev) + sizeof(*hc));
		ev = net_buf_add(evt, sizeof(*ev));
		ev->num_handles = 1U;
		hc = net_buf_add(evt, sizeof(*hc));
		hc->handle = sys_cpu_to_le16(peer->handle);
		hc->count = sys_cpu_to_le16((uint16_t)count);

		bt_hci_recv(dev, evt);
	}
}

static K_WORK_DEFINE(completed_work, completed_handler);

static struct test_peer *peer_get(uint16_t handle)
{
	ARRAY_FOR_EACH_PTR(peers, peer) {
		if (peer->handle == handle) {
			return peer;
		}
	}

	return NULL;
}

/* The controller reports each data packet as completed right away */
static void acl_handle(struct net_buf *acl)
{
	struct bt_hci_acl_hdr *hdr;
	struct test_peer *peer;
	uint16_t len;
	uint16_t cid;
	uint8_t op;

	hdr = net_buf_pull_mem(acl, sizeof(*hdr));
	peer = peer_get(bt_acl_handle(sys_le16_to_cpu(hdr->handle)));
	zassert_not_null(peer);
	zassert_true(peer->connected);

	(void)atomic_inc(&peer->unreported);
	zassert_true(k_work_submit(&completed_work) >= 0);

	/* Every PDU fits a single ACL packet */
	len = net_buf_pull_le16(acl);
	cid = net_buf_pull_le16(acl);
	zassert_equal(len, acl->len);

	if (cid != BT_L2CAP_CID_ATT) {
		return;
	}

	op = net_buf_pull_u8(acl);
	if (op == ATT_OP_WRITE_RSP) {
		k_sem_give(&write_rsp_sem);
	} else if (op == ATT_OP_NOTIFY) {
		zassert_equal(net_buf_pull_le16(acl), value_handle);
		(void)atomic_inc(&peer->notified);
	}
}

static int driver_open(const struct device *dev)
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
		acl_handle(buf);
	}

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	if (err == BT_HCI_ERR_SUCCESS) {
		k_sem_give(&connected_sem);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);

	k_sem_give(&disconnected_sem);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/* The peer connects to the advertising of the Host */
static void connect_peer(struct test_peer *peer)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	const bt_addr_le_t addr = {
		.type = BT_ADDR_LE_RANDOM,
		.a.val = { (uint8_t)peer->handle, 0x02U, 0x03U, 0x04U, 0x05U, 0xc6U },
	};
	struct bt_hci_evt_le_conn_complete *evt;
	struct bt_hci_evt_le_meta_event *meta;
	struct net_buf *buf;

	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0U, NULL, 0U));

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + sizeof(*evt));
	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_CONN_COMPLETE;
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(peer->handle);
	evt->role = BT_HCI_ROLE_PERIPHERAL;
	bt_addr_le_copy(&evt->peer_addr, &addr);
	evt->interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MIN);
	evt->supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U));
	peer->connected = true;
	bt_hci_recv(dev, buf);

	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connection");
}

/* The peer enables notifications of the log characteristic */
static void subscribe(struct test_peer *peer)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	net_buf_add_le16(buf, bt_acl_handle_pack(peer->handle, BT_ACL_START));
	net_buf_add_le16(buf, BT_L2CAP_HDR_SIZE + WRITE_REQ_LEN);
	net_buf_add_le16(buf, WRITE_REQ_LEN);
	net_buf_add_le16(buf, BT_L2CAP_CID_ATT);
	net_buf_add_u8(buf, ATT_OP_WRITE_REQ);
	net_buf_add_le16(buf, ccc_handle);
	net_buf_add_le16(buf, BT_GATT_CCC_NOTIFY);
	bt_hci_recv(dev, buf);

	zassert_ok(k_sem_take(&write_rsp_sem, TX_TIMEOUT), "No Write Response");
}

static void disconnect_peer(struct test_peer *peer)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	/* A controller reports no completed packets for a link that is gone */
	(void)atomic_clear(&peer->unreported);

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(peer->handle);
	evt->reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
	bt_hci_recv(dev, buf);

	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");
	peer->connected = false;
}

/* Let the logger output what it holds, and the Host send it, which takes no
 * simulated time
 */
static void flush(void)
{
	log_flush();
	(void)k_sleep(K_MSEC(1));
}

/* Log a line, after the output of what was logged before, ztest's printk()
 * included
 */
static void log_line(void)
{
	flush();

	ARRAY_FOR_EACH_PTR(peers, peer) {
		peer->notified_before = atomic_get(&peer->notified);
	}

	LOG_INF("line");
	flush();
}

/* Whether the peer got notifications since the last line was logged */
static bool got_line(const struct test_peer *peer)
{
	return atomic_get(&peer->notified) != peer->notified_before;
}

static void *setup(void)
{
	const struct bt_gatt_attr *attr;

	zassert_ok(bt_enable(NULL));

	/* The logger thread deactivates the backend when it starts */
	(void)k_sleep(K_MSEC(1));

	attr = bt_gatt_find_by_uuid(NULL, 0U, LOG_TX_UUID);
	zassert_not_null(attr);
	value_handle = bt_gatt_attr_get_handle(attr);

	attr = bt_gatt_find_by_uuid(attr, 0U, BT_UUID_GATT_CCC);
	zassert_not_null(attr);
	ccc_handle = bt_gatt_attr_get_handle(attr);

	return NULL;
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	ARRAY_FOR_EACH_PTR(peers, peer) {
		if (peer->connected) {
			disconnect_peer(peer);
		}
	}
}

ZTEST_SUITE(log_backend_ble, NULL, setup, NULL, after, NULL);

/* The output goes to each peer that has notifications enabled */
ZTEST(log_backend_ble, test_subscribed)
{
	connect_peer(&peers[0]);
	subscribe(&peers[0]);
	connect_peer(&peers[1]);

	log_line();
	zassert_true(got_line(&peers[0]));
	zassert_false(got_line(&peers[1]));

	subscribe(&peers[1]);

	log_line();
	zassert_true(got_line(&peers[0]));
	zassert_true(got_line(&peers[1]));
}

/* A disconnection leaves the output to the other subscribed peer */
ZTEST(log_backend_ble, test_disconnect)
{
	connect_peer(&peers[0]);
	subscribe(&peers[0]);
	connect_peer(&peers[1]);
	subscribe(&peers[1]);

	disconnect_peer(&peers[0]);

	log_line();
	zassert_true(got_line(&peers[1]));
}

/* A line that the logger took for the backend while the last subscribed
 * peer was still connected finds no peer once it is gone
 */
ZTEST(log_backend_ble, test_disconnect_last)
{
	const struct log_backend *backend = log_backend_get_by_name("log_backend_ble");

	zassert_not_null(backend);

	connect_peer(&peers[0]);
	subscribe(&peers[0]);
	disconnect_peer(&peers[0]);
	zassert_false(log_backend_is_active(backend));

	/* As the backend was when the logger took the line */
	log_backend_activate(backend, NULL);
	log_line();
	log_backend_deactivate(backend);

	zassert_false(got_line(&peers[0]));
}
