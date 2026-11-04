/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/hci_core.h"
#include "host/classic/conn_br_internal.h"
#include "host/classic/l2cap_br_interface.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of initialization commands that are
 * irrelevant to this test
 */
#define GENERIC_RP_SIZE 64U

/* Small, so that a PDU of PAYLOAD_LEN bytes goes out in several fragments */
#define ACL_MAX_LEN 20U
#define ACL_MAX_NUM 8U

#define TEST_CONN_HANDLE 0x0001U

/* Connectionless data channel */
#define CID_CONNLESS 0x0002U

#define PAYLOAD_LEN 40U
/* L2CAP header and payload, in fragments of up to ACL_MAX_LEN bytes */
#define PAYLOAD_FRAGS DIV_ROUND_UP(BT_L2CAP_HDR_SIZE + PAYLOAD_LEN, ACL_MAX_LEN)

#define MAX_PACKETS 16U

NET_BUF_POOL_FIXED_DEFINE(tx_pool, 2, BT_L2CAP_BUF_SIZE(PAYLOAD_LEN),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/* The Packet Boundary Flag and length of each data packet the controller got */
static uint8_t pkt_pb[MAX_PACKETS];
static uint16_t pkt_len[MAX_PACKETS];
static atomic_t pkt_count;
static atomic_t acl_pending;

/* The parameters of the last Write Automatic Flush Timeout command */
static uint16_t flush_handle;
static uint16_t flush_timeout;
static atomic_t flush_cmds;

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
	/* BR/EDR and LE supported */
	rp->features[4] = BIT(6);
	/* Non-flushable Packet Boundary Flag */
	rp->features[6] = BIT(6);
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

static void read_buffer_size(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_read_buffer_size *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	(void)memset(rp, 0, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->acl_max_len = sys_cpu_to_le16(ACL_MAX_LEN);
	rp->acl_max_num = sys_cpu_to_le16(ACL_MAX_NUM);
}

static void le_read_buffer_size(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	/* LE shares the BR/EDR buffers */
	rp->le_max_len = 0U;
	rp->le_max_num = 0U;
}

static void write_auto_flush_timeout(struct net_buf *cmd, struct net_buf **evt, uint16_t opcode)
{
	const struct bt_hci_cp_write_auto_flush_timeout *cp = (const void *)cmd->data;
	struct bt_hci_evt_cc_status *rp;

	flush_handle = sys_le16_to_cpu(cp->handle);
	flush_timeout = sys_le16_to_cpu(cp->timeout);
	(void)atomic_inc(&flush_cmds);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
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
	case BT_HCI_OP_READ_BUFFER_SIZE:
		read_buffer_size(&evt, opcode);
		break;
	case BT_HCI_OP_LE_READ_BUFFER_SIZE:
		le_read_buffer_size(&evt, opcode);
		break;
	case BT_HCI_OP_WRITE_AUTO_FLUSH_TIMEOUT:
		write_auto_flush_timeout(cmd, &evt, opcode);
		break;
	default:
		generic_success(&evt, opcode);
		break;
	}

	bt_hci_recv(dev, evt);
}

static void acl_handle(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *hdr;
	atomic_val_t n;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));

	n = atomic_inc(&pkt_count);
	if (n < MAX_PACKETS) {
		pkt_pb[n] = bt_acl_flags_pb(bt_acl_flags(sys_le16_to_cpu(hdr->handle)));
		pkt_len[n] = sys_le16_to_cpu(hdr->len);
	}
}

static void completed_handler(struct k_work *work)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_evt_num_completed_packets *evt;
	struct bt_hci_handle_count *hc;
	struct net_buf *buf;
	atomic_val_t count;

	ARG_UNUSED(work);

	count = atomic_set(&acl_pending, 0);
	if (count == 0) {
		return;
	}

	buf = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_NUM_COMPLETED_PACKETS, sizeof(*evt) + sizeof(*hc));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->num_handles = 1U;
	hc = net_buf_add(buf, sizeof(*hc));
	hc->handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	hc->count = sys_cpu_to_le16((uint16_t)count);
	bt_hci_recv(dev, buf);
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
		(void)atomic_inc(&acl_pending);
		(void)k_work_submit(&completed_work);
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

static struct bt_conn *test_conn;

static void evt_send(uint8_t evt, const void *params, uint8_t len)
{
	struct net_buf *buf;

	buf = bt_buf_get_evt(evt, false, K_FOREVER);
	evt_create(buf, evt, len);
	net_buf_add_mem(buf, params, len);
	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
}

static void connect(void)
{
	static const bt_addr_t peer = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };
	struct bt_hci_evt_conn_complete evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.handle = sys_cpu_to_le16(TEST_CONN_HANDLE),
		.link_type = BT_HCI_ACL,
	};

	test_conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(test_conn, "Connecting failed");

	bt_addr_copy(&evt.bdaddr, &peer);
	evt_send(BT_HCI_EVT_CONN_COMPLETE, &evt, sizeof(evt));

	for (unsigned int i = 0U; i < 100U && test_conn->state != BT_CONN_CONNECTED; i++) {
		k_sleep(K_MSEC(10));
	}
	zassert_equal(test_conn->state, BT_CONN_CONNECTED, "No connection");
}

/* Wait until the controller got count data packets, and no more */
static void packets_expect(atomic_val_t count)
{
	for (unsigned int i = 0U; i < 100U && atomic_get(&pkt_count) < count; i++) {
		k_sleep(K_MSEC(10));
	}
	k_sleep(K_MSEC(50));

	zassert_equal(atomic_get(&pkt_count), count, "%ld data packets, expected %ld",
		      atomic_get(&pkt_count), count);
}

static void pdu_send(uint16_t cid, size_t len)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
	zassert_not_null(buf);

	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
	(void)memset(net_buf_add(buf, len), 0x5a, len);

	zassert_ok(bt_l2cap_br_send_cb(test_conn, cid, buf, NULL, NULL), "Send failed");
}

static struct bt_l2cap_chan *connless_chan(void)
{
	struct bt_l2cap_chan *chan;

	chan = bt_l2cap_br_lookup_rx_cid(test_conn, CID_CONNLESS);
	zassert_not_null(chan);

	return chan;
}

static void *setup(void)
{
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	connect();

	return NULL;
}

static void before(void *f)
{
	ARG_UNUSED(f);

	/* Let what the connection sent on its own (e.g. an Information
	 * Request) go out first
	 */
	k_sleep(K_MSEC(100));

	bt_dev.features[0][6] |= BIT(6);
	zassert_ok(bt_l2cap_br_chan_set_flushable(connless_chan(), false));

	(void)atomic_set(&pkt_count, 0);
	(void)atomic_set(&flush_cmds, 0);
}

ZTEST_SUITE(l2cap_br_flush, NULL, setup, before, NULL, NULL);

/* The fragments of a PDU on a flushable channel are flushable, and the PDU
 * that follows on another channel is not
 */
ZTEST(l2cap_br_flush, test_flushable_pdu)
{
	zassert_ok(bt_l2cap_br_chan_set_flushable(connless_chan(), true));

	pdu_send(CID_CONNLESS, PAYLOAD_LEN);
	pdu_send(BT_L2CAP_CID_BR_SIG, 4U);
	packets_expect(PAYLOAD_FRAGS + 1U);

	zassert_equal(pkt_pb[0], BT_ACL_START, "PB 0x%02x", pkt_pb[0]);
	for (unsigned int i = 1U; i < PAYLOAD_FRAGS; i++) {
		zassert_equal(pkt_pb[i], BT_ACL_CONT, "PB 0x%02x of fragment %u", pkt_pb[i], i);
	}
	zassert_equal(pkt_pb[PAYLOAD_FRAGS], BT_ACL_START_NO_FLUSH, "PB 0x%02x",
		      pkt_pb[PAYLOAD_FRAGS]);
	zassert_equal(pkt_len[PAYLOAD_FRAGS], BT_L2CAP_HDR_SIZE + 4U);
}

/* Channels are not flushable unless set so */
ZTEST(l2cap_br_flush, test_non_flushable_by_default)
{
	pdu_send(CID_CONNLESS, PAYLOAD_LEN);
	packets_expect(PAYLOAD_FRAGS);

	zassert_equal(pkt_pb[0], BT_ACL_START_NO_FLUSH, "PB 0x%02x", pkt_pb[0]);
	for (unsigned int i = 1U; i < PAYLOAD_FRAGS; i++) {
		zassert_equal(pkt_pb[i], BT_ACL_CONT, "PB 0x%02x of fragment %u", pkt_pb[i], i);
	}
}

/* Without the Non-flushable Packet Boundary Flag, the controller would take
 * every packet as flushable, so no channel can be made flushable
 */
ZTEST(l2cap_br_flush, test_flushable_not_supported)
{
	bt_dev.features[0][6] &= ~BIT(6);

	zassert_equal(bt_l2cap_br_chan_set_flushable(connless_chan(), true), -ENOTSUP);
	zassert_ok(bt_l2cap_br_chan_set_flushable(connless_chan(), false));
}

ZTEST(l2cap_br_flush, test_auto_flush_timeout)
{
	zassert_ok(bt_conn_br_set_auto_flush_timeout(test_conn, 32U));
	zassert_equal(atomic_get(&flush_cmds), 1);
	zassert_equal(flush_handle, TEST_CONN_HANDLE, "handle 0x%04x", flush_handle);
	zassert_equal(flush_timeout, 32U, "timeout %u", flush_timeout);

	zassert_equal(bt_conn_br_set_auto_flush_timeout(test_conn,
							BT_HCI_AUTO_FLUSH_TIMEOUT_MAX + 1U),
		      -EINVAL);
	zassert_equal(bt_conn_br_set_auto_flush_timeout(NULL, 32U), -EINVAL);
	zassert_equal(atomic_get(&flush_cmds), 1);
}
