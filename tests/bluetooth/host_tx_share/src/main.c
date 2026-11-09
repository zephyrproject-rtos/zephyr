/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Xiaomi Corporation
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
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/att_internal.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of initialization commands that are
 * irrelevant to this test
 */
#define GENERIC_RP_SIZE 64U

#define LE_ACL_MAX_LEN 27U
/* The controller buffers, shared by both connections */
#define LE_ACL_MAX_NUM COND_CODE_1(CONFIG_TEST_SIX_BUFFERS, (6U), (1U))

#define TEST_CONN_NUM 2U
#define TEST_NOTIFY_NUM 6U
/* Notifications the first connection queues in the test with six buffers */
#define TEST_BURST_NUM 8U

BT_GATT_SERVICE_DEFINE(test_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0xfff0)),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0xfff1), BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
);

/* The handles of the notifications in the order the controller got them */
static uint16_t sent[TEST_BURST_NUM + 1U];
static atomic_t sent_num;
static atomic_t acl_unexpected;

/* The packets the controller holds on each connection until it reports them as
 * completed
 */
static atomic_t acl_pending[TEST_CONN_NUM];

static uint16_t conn_handle(size_t i)
{
	return (uint16_t)(i + 1U);
}

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
	case BT_HCI_OP_READ_LOCAL_FEATURES: {
		struct bt_hci_rp_read_local_features *rp;

		rp = cmd_complete(&evt, sizeof(*rp), opcode);
		rp->status = BT_HCI_ERR_SUCCESS;
		(void)memset(rp->features, 0, sizeof(rp->features));
		/* LE supported, BR/EDR not supported */
		rp->features[4] = BIT(5) | BIT(6);
		break;
	}
	case BT_HCI_OP_READ_SUPPORTED_COMMANDS: {
		struct bt_hci_rp_read_supported_commands *rp;

		rp = cmd_complete(&evt, sizeof(*rp), opcode);
		rp->status = BT_HCI_ERR_SUCCESS;
		(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
		break;
	}
	case BT_HCI_OP_READ_BD_ADDR: {
		struct bt_hci_rp_read_bd_addr *rp;

		rp = cmd_complete(&evt, sizeof(*rp), opcode);
		rp->status = BT_HCI_ERR_SUCCESS;
		rp->bdaddr = (bt_addr_t){ .val = { 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6 } };
		break;
	}
	case BT_HCI_OP_LE_READ_BUFFER_SIZE: {
		struct bt_hci_rp_le_read_buffer_size *rp;

		rp = cmd_complete(&evt, sizeof(*rp), opcode);
		rp->status = BT_HCI_ERR_SUCCESS;
		rp->le_max_len = sys_cpu_to_le16(LE_ACL_MAX_LEN);
		rp->le_max_num = LE_ACL_MAX_NUM;
		break;
	}
	case BT_HCI_OP_LE_READ_SUPP_STATES: {
		struct bt_hci_rp_le_read_supp_states *rp;

		rp = cmd_complete(&evt, sizeof(*rp), opcode);
		rp->status = BT_HCI_ERR_SUCCESS;
		(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
		break;
	}
	default: {
		struct bt_hci_evt_cc_status *rp;

		rp = cmd_complete(&evt, GENERIC_RP_SIZE, opcode);
		(void)memset(rp, 0, GENERIC_RP_SIZE);
		rp->status = BT_HCI_ERR_SUCCESS;
		break;
	}
	}

	bt_hci_recv(dev, evt);
}

/* A data packet from the Host must carry a notification on one of the
 * connections
 */
static void acl_handle(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *hdr;
	uint16_t handle;
	uint16_t len;
	uint16_t cid;
	atomic_val_t n;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	handle = bt_acl_handle(sys_le16_to_cpu(hdr->handle));

	if (handle == 0U || handle > TEST_CONN_NUM || buf->len < BT_L2CAP_HDR_SIZE) {
		(void)atomic_inc(&acl_unexpected);
		return;
	}

	(void)atomic_inc(&acl_pending[handle - 1U]);

	len = net_buf_pull_le16(buf);
	cid = net_buf_pull_le16(buf);

	if (len != buf->len || cid != BT_L2CAP_CID_ATT || buf->data[0] != BT_ATT_OP_NOTIFY) {
		(void)atomic_inc(&acl_unexpected);
		return;
	}

	n = atomic_inc(&sent_num);
	if (n < ARRAY_SIZE(sent)) {
		sent[n] = handle;
	} else {
		(void)atomic_inc(&acl_unexpected);
	}
}

static void completed_send(size_t i, uint16_t count)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_evt_num_completed_packets *evt;
	struct bt_hci_handle_count *hc;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_NUM_COMPLETED_PACKETS, sizeof(*evt) + sizeof(*hc));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->num_handles = 1U;
	hc = net_buf_add(buf, sizeof(*hc));
	hc->handle = sys_cpu_to_le16(conn_handle(i));
	hc->count = sys_cpu_to_le16(count);
	bt_hci_recv(dev, buf);
}

/* The controller sends each packet at once and reports it as completed */
static void completed_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	for (size_t i = 0U; i < TEST_CONN_NUM; i++) {
		atomic_val_t count = atomic_set(&acl_pending[i], 0);

		if (count != 0) {
			completed_send(i, (uint16_t)count);
		}
	}
}

static K_WORK_DEFINE(completed_work, completed_handler);

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
		/* With six buffers, the test reports the packets as completed */
		if (!IS_ENABLED(CONFIG_TEST_SIX_BUFFERS)) {
			(void)k_work_submit(&completed_work);
		}
	} else {
		(void)atomic_inc(&acl_unexpected);
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

static K_SEM_DEFINE(connected_sem, 0, 1);
static struct bt_conn *test_conn[TEST_CONN_NUM];

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	if (err != 0U || bt_conn_get_info(conn, &info) != 0 || info.id != BT_ID_DEFAULT ||
	    bt_conn_index(conn) >= TEST_CONN_NUM) {
		return;
	}

	test_conn[bt_conn_index(conn)] = bt_conn_ref(conn);
	k_sem_give(&connected_sem);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
};

static void evt_send(uint8_t evt, const void *params, uint8_t len)
{
	struct net_buf *buf;

	buf = bt_buf_get_evt(evt, false, K_FOREVER);
	evt_create(buf, evt, len);
	net_buf_add_mem(buf, params, len);
	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
}

static void connect_as_peripheral(size_t i)
{
	const bt_addr_le_t peer = {
		.type = BT_ADDR_LE_RANDOM,
		.a.val = { 0x01U, 0x02U, 0x03U, 0x04U, (uint8_t)i, 0xc6U },
	};
	struct {
		struct bt_hci_evt_le_meta_event meta;
		struct bt_hci_evt_le_conn_complete cc;
	} __packed evt;

	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");

	(void)memset(&evt, 0, sizeof(evt));
	evt.meta.subevent = BT_HCI_EVT_LE_CONN_COMPLETE;
	evt.cc.status = BT_HCI_ERR_SUCCESS;
	evt.cc.handle = sys_cpu_to_le16(conn_handle(i));
	evt.cc.role = BT_HCI_ROLE_PERIPHERAL;
	bt_addr_le_copy(&evt.cc.peer_addr, &peer);
	evt.cc.interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MIN);
	evt.cc.supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U));

	evt_send(BT_HCI_EVT_LE_META_EVENT, &evt, sizeof(evt));

	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connection %zu", i);
}

static int notify(struct bt_conn *conn)
{
	static const uint8_t value = 0x5aU;

	return bt_gatt_notify(conn, &test_svc.attrs[2], &value, sizeof(value));
}

static void wait_sent(size_t num)
{
	for (unsigned int i = 0U; i < 100U && atomic_get(&sent_num) < num; i++) {
		k_sleep(K_MSEC(10));
	}
}

static void *setup(void)
{
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	for (size_t i = 0U; i < TEST_CONN_NUM; i++) {
		connect_as_peripheral(i);
	}

	return NULL;
}

ZTEST_SUITE(host_tx_share, NULL, setup, NULL, NULL, NULL);

/* With a single controller buffer, a connection whose packets complete before
 * the next is sent must still let another connection send. The first connection
 * queues a burst, then the second one a single notification: it goes out before
 * the burst is done.
 */
static ZTEST(host_tx_share, test_one_buffer_two_connections)
{
	const size_t total = TEST_NOTIFY_NUM + 1U;
	int pos = -1;

	Z_TEST_SKIP_IFDEF(CONFIG_TEST_SIX_BUFFERS);

	/* Queue everything before the TX processor looks at the list */
	k_sched_lock();
	for (size_t i = 0U; i < TEST_NOTIFY_NUM; i++) {
		zassert_ok(notify(test_conn[0]), "Notification %zu failed", i);
	}
	zassert_ok(notify(test_conn[1]), "Notification on the second connection failed");
	k_sched_unlock();

	wait_sent(total);

	zassert_equal(atomic_get(&sent_num), total, "The controller got %ld of %zu",
		      atomic_get(&sent_num), total);
	zassert_equal(atomic_get(&acl_unexpected), 0, "The controller got %ld unexpected packets",
		      atomic_get(&acl_unexpected));

	for (size_t i = 0U; i < total; i++) {
		if (sent[i] == conn_handle(1U)) {
			pos = (int)i;
			break;
		}
	}

	/* The first connection may send once before the second one is served */
	zassert_true(pos >= 0 && pos <= 1, "The second connection sent as packet %d of %zu",
		     pos, total);
}

/* With six controller buffers, a connection that holds all of them while the
 * other one is idle must leave the next freed buffer to the other one once it
 * has data. The first connection is over its share of 3, the second is not.
 */
static ZTEST(host_tx_share, test_freed_buffer_to_connection_below_share)
{
	const size_t total = TEST_BURST_NUM + 1U;

	Z_TEST_SKIP_IFNDEF(CONFIG_TEST_SIX_BUFFERS);

	k_sched_lock();
	for (size_t i = 0U; i < TEST_BURST_NUM; i++) {
		zassert_ok(notify(test_conn[0]), "Notification %zu failed", i);
	}
	k_sched_unlock();

	/* The first connection takes every buffer */
	wait_sent(LE_ACL_MAX_NUM);
	zassert_equal(atomic_get(&sent_num), LE_ACL_MAX_NUM, "The controller got %ld of %u",
		      atomic_get(&sent_num), LE_ACL_MAX_NUM);

	zassert_ok(notify(test_conn[1]), "Notification on the second connection failed");

	/* Nothing goes out until a buffer is freed */
	k_sleep(K_MSEC(50));
	zassert_equal(atomic_get(&sent_num), LE_ACL_MAX_NUM, "The controller got %ld of %u",
		      atomic_get(&sent_num), LE_ACL_MAX_NUM);

	/* Free one buffer of the first connection: it goes to the second one */
	(void)atomic_dec(&acl_pending[0]);
	completed_send(0U, 1U);

	wait_sent(LE_ACL_MAX_NUM + 1U);
	zassert_equal(atomic_get(&sent_num), LE_ACL_MAX_NUM + 1U, "The controller got %ld of %u",
		      atomic_get(&sent_num), LE_ACL_MAX_NUM + 1U);
	zassert_equal(sent[LE_ACL_MAX_NUM], conn_handle(1U),
		      "The freed buffer went to connection %u", sent[LE_ACL_MAX_NUM]);

	/* Let the rest of the burst go out */
	for (unsigned int i = 0U; i < 100U && atomic_get(&sent_num) < total; i++) {
		completed_handler(NULL);
		k_sleep(K_MSEC(10));
	}

	zassert_equal(atomic_get(&sent_num), total, "The controller got %ld of %zu",
		      atomic_get(&sent_num), total);
	zassert_equal(atomic_get(&acl_unexpected), 0, "The controller got %ld unexpected packets",
		      atomic_get(&acl_unexpected));
}
