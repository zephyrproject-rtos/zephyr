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
#include "host/classic/l2cap_br_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define GENERIC_RP_SIZE 64U
#define ACL_MAX_LEN     310U
#define LE_ACL_MAX_LEN  27U
#define LE_ACL_MAX_NUM  3U
#define HANDLE          0x0001U

/* Headers of an ACL packet from the controller, after its H:4 packet type */
#define ACL_HDR_OFFSET   1U
#define L2CAP_HDR_OFFSET (ACL_HDR_OFFSET + sizeof(struct bt_hci_acl_hdr))
#define SIG_HDR_OFFSET   (L2CAP_HDR_OFFSET + sizeof(struct bt_l2cap_hdr))

#define DATA       "data"
#define SDP_PSM    0x0001U
#define PEER_CID   0x0040U
#define PEER_IDENT 0x80U
#define MIN_MTU    48U

#define TX_TIMEOUT K_MSEC(100)

/* The PDUs that the Host sent and the test has not taken yet */
NET_BUF_POOL_FIXED_DEFINE(sent_pool, 4, ACL_MAX_LEN, 0, NULL);

static struct bt_conn *conn;

/* The L2CAP PDUs that the Host sent */
static K_FIFO_DEFINE(sent);

/* The controller has one ACL buffer, which it reports free again unless the
 * test holds it, as for a Number Of Completed Packets event that comes late.
 */
static atomic_t unreported;
static bool hold;

/* What the channel callbacks saw */
static atomic_t connected_count;
static atomic_t connected_off_workq;
static atomic_t recv_count;
static atomic_t recv_before_connected;
static atomic_t released_count;
static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(released_sem, 0, 1);
static K_SEM_DEFINE(recycled_sem, 0, 1);
static K_SEM_DEFINE(workq_sem, 0, 1);

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
	/* LE supported, BR/EDR supported */
	rp->features[4] = BIT(6);
}

static void read_supported_commands(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xffU, sizeof(rp->commands));
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
	rp->acl_max_num = sys_cpu_to_le16(1U);
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
	(void)memset(rp->le_states, 0xffU, sizeof(rp->le_states));
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
	struct bt_hci_evt_num_completed_packets *ev;
	struct bt_hci_handle_count *hc;
	atomic_val_t count;
	struct net_buf *evt;

	ARG_UNUSED(work);

	count = atomic_clear(&unreported);
	if (count == 0) {
		return;
	}

	evt = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
	evt_create(evt, BT_HCI_EVT_NUM_COMPLETED_PACKETS, sizeof(*ev) + sizeof(*hc));
	ev = net_buf_add(evt, sizeof(*ev));
	ev->num_handles = 1U;
	hc = net_buf_add(evt, sizeof(*hc));
	hc->handle = sys_cpu_to_le16(HANDLE);
	hc->count = sys_cpu_to_le16((uint16_t)count);

	bt_hci_recv(dev, evt);
}

static K_WORK_DEFINE(completed_work, completed_handler);

/* Report the held ACL buffer as completed */
static void release(void)
{
	hold = false;
	zassert_true(k_work_submit(&completed_work) >= 0);
}

static void acl_handle(struct net_buf *acl)
{
	struct bt_hci_acl_hdr *hdr;
	struct net_buf *pdu;

	/* Every PDU fits a single ACL packet */
	hdr = net_buf_pull_mem(acl, sizeof(*hdr));
	zassert_equal(bt_acl_handle(sys_le16_to_cpu(hdr->handle)), HANDLE);

	pdu = net_buf_alloc(&sent_pool, K_NO_WAIT);
	zassert_not_null(pdu);
	(void)net_buf_add_mem(pdu, acl->data, acl->len);
	k_fifo_put(&sent, pdu);

	(void)atomic_inc(&unreported);
	if (!hold) {
		zassert_true(k_work_submit(&completed_work) >= 0);
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

/* The next PDU must be a signalling command with the given code. Returns its
 * identifier.
 */
static uint8_t tx_sig(uint8_t code)
{
	struct bt_l2cap_sig_hdr *sig;
	struct bt_l2cap_hdr *hdr;
	struct net_buf *buf;
	uint8_t ident;

	buf = k_fifo_get(&sent, TX_TIMEOUT);
	zassert_not_null(buf, "Nothing sent");

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	zassert_equal(sys_le16_to_cpu(hdr->cid), BT_L2CAP_CID_BR_SIG, "cid 0x%04x",
		      sys_le16_to_cpu(hdr->cid));

	sig = net_buf_pull_mem(buf, sizeof(*sig));
	zassert_equal(sig->code, code, "code 0x%02x", sig->code);
	ident = sig->ident;

	net_buf_unref(buf);

	return ident;
}

/* An ACL packet from the controller, up to its L2CAP header */
static struct net_buf *rx_frame(void)
{
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	(void)net_buf_add(buf, sizeof(struct bt_hci_acl_hdr));

	return buf;
}

/* Deliver the packet and let the Host handle it, which takes no simulated time */
static void recv_frame(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *acl = (void *)&buf->data[ACL_HDR_OFFSET];

	acl->handle = sys_cpu_to_le16(bt_acl_handle_pack(HANDLE, BT_ACL_START));
	acl->len = sys_cpu_to_le16(buf->len - L2CAP_HDR_OFFSET);

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
	(void)k_sleep(K_MSEC(1));
}

static struct net_buf *sig_cmd(uint8_t code, uint8_t ident)
{
	struct bt_l2cap_sig_hdr *sig;
	struct net_buf *buf;

	buf = rx_frame();
	(void)net_buf_add(buf, sizeof(struct bt_l2cap_hdr));

	sig = net_buf_add(buf, sizeof(*sig));
	sig->code = code;
	sig->ident = ident;

	return buf;
}

static void recv_sig(struct net_buf *buf)
{
	struct bt_l2cap_sig_hdr *sig = (void *)&buf->data[SIG_HDR_OFFSET];
	struct bt_l2cap_hdr *hdr = (void *)&buf->data[L2CAP_HDR_OFFSET];

	sig->len = sys_cpu_to_le16(buf->len - SIG_HDR_OFFSET - sizeof(*sig));
	hdr->len = sys_cpu_to_le16(buf->len - SIG_HDR_OFFSET);
	hdr->cid = sys_cpu_to_le16(BT_L2CAP_CID_BR_SIG);

	recv_frame(buf);
}

/* A Basic mode PDU from the peer on the channel */
static void recv_data(struct bt_l2cap_br_chan *chan)
{
	struct net_buf *buf;

	buf = rx_frame();
	net_buf_add_le16(buf, (uint16_t)strlen(DATA));
	net_buf_add_le16(buf, chan->rx.cid);
	(void)net_buf_add_mem(buf, DATA, strlen(DATA));

	recv_frame(buf);
}

static void chan_connected(struct bt_l2cap_chan *ch)
{
	ARG_UNUSED(ch);

	if (!bt_is_work_thread()) {
		(void)atomic_inc(&connected_off_workq);
	}

	(void)atomic_inc(&connected_count);
	k_sem_give(&connected_sem);
}

static int chan_recv(struct bt_l2cap_chan *ch, struct net_buf *buf)
{
	ARGS_UNUSED(ch, buf);

	if (atomic_get(&connected_count) == 0) {
		(void)atomic_inc(&recv_before_connected);
	}

	(void)atomic_inc(&recv_count);

	return 0;
}

static void chan_released(struct bt_l2cap_chan *ch)
{
	ARG_UNUSED(ch);

	(void)atomic_inc(&released_count);
	k_sem_give(&released_sem);
}

static const struct bt_l2cap_chan_ops chan_ops = {
	.connected = chan_connected,
	.recv = chan_recv,
	.released = chan_released,
};

static void conn_recycled(void)
{
	k_sem_give(&recycled_sem);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.recycled = conn_recycled,
};

static void workq_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_give(&workq_sem);
}

static K_WORK_DEFINE(workq_work, workq_handler);

/* Whether the Bluetooth workqueue still runs work items */
static bool workq_runs(void)
{
	k_sem_reset(&workq_sem);
	zassert_true(bt_work_submit(&workq_work) >= 0);

	return k_sem_take(&workq_sem, K_MSEC(100)) == 0;
}

/* A Disconnection Complete event for the ACL, as when the link is lost */
static void recv_disconn_complete(void)
{
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(HANDLE);
	evt->reason = BT_HCI_ERR_CONN_TIMEOUT;

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
}

/* Configure a channel in Basic mode up to the point where only the Number Of
 * Completed Packets event for the Configuration Response of the Host is
 * missing: the peer has accepted the configuration of the Host.
 */
static void chan_config_held(struct bt_l2cap_br_chan *chan, uint16_t peer_cid)
{
	struct net_buf *buf;
	uint8_t ident;

	chan->chan.ops = &chan_ops;
	chan->rx.mtu = MIN_MTU;
	zassert_ok(bt_l2cap_chan_connect(conn, &chan->chan, SDP_PSM));

	ident = tx_sig(BT_L2CAP_CONN_REQ);
	buf = sig_cmd(BT_L2CAP_CONN_RSP, ident);
	net_buf_add_le16(buf, peer_cid);
	net_buf_add_le16(buf, chan->rx.cid);
	net_buf_add_le16(buf, BT_L2CAP_BR_SUCCESS);
	net_buf_add_le16(buf, 0U);
	recv_sig(buf);

	ident = tx_sig(BT_L2CAP_CONF_REQ);

	/* The controller reports the Configuration Response complete only on release() */
	hold = true;

	buf = sig_cmd(BT_L2CAP_CONF_REQ, PEER_IDENT);
	net_buf_add_le16(buf, chan->rx.cid);
	net_buf_add_le16(buf, 0U);
	recv_sig(buf);

	zassert_equal(tx_sig(BT_L2CAP_CONF_RSP), PEER_IDENT);

	buf = sig_cmd(BT_L2CAP_CONF_RSP, ident);
	net_buf_add_le16(buf, chan->rx.cid);
	net_buf_add_le16(buf, 0U);
	net_buf_add_le16(buf, BT_L2CAP_CONF_SUCCESS);
	recv_sig(buf);

	zassert_equal(chan->state, BT_L2CAP_CONFIG);
	zassert_equal(atomic_get(&connected_count), 0);
}

/* As on Connection Complete, followed by the Information Request of the Host */
static void acl_connect(void)
{
	static const bt_addr_t addr = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };
	struct net_buf *buf;
	struct net_buf *old;
	uint8_t ident;

	/* What the Host sent on the previous ACL */
	old = k_fifo_get(&sent, K_NO_WAIT);
	while (old != NULL) {
		net_buf_unref(old);
		old = k_fifo_get(&sent, K_NO_WAIT);
	}

	conn = bt_conn_add_br(&addr);
	zassert_not_null(conn);
	conn->handle = HANDLE;
	bt_conn_set_state(conn, BT_CONN_CONNECTED);
	bt_l2cap_br_connected(conn);

	ident = tx_sig(BT_L2CAP_INFO_REQ);
	buf = sig_cmd(BT_L2CAP_INFO_RSP, ident);
	net_buf_add_le16(buf, BT_L2CAP_INFO_FEAT_MASK);
	net_buf_add_le16(buf, BT_L2CAP_INFO_SUCCESS);
	net_buf_add_le32(buf, 0U);
	recv_sig(buf);
}

static void *setup(void)
{
	zassert_ok(bt_enable(NULL));

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)atomic_clear(&connected_count);
	(void)atomic_clear(&connected_off_workq);
	(void)atomic_clear(&recv_count);
	(void)atomic_clear(&recv_before_connected);
	(void)atomic_clear(&released_count);
	k_sem_reset(&connected_sem);
	k_sem_reset(&released_sem);

	if (conn == NULL) {
		acl_connect();
	}
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (hold) {
		release();
	}

	/* Let a late completion run before the next case counts again */
	(void)k_sleep(K_MSEC(10));
}

ZTEST_SUITE(l2cap_br_conf, NULL, setup, before, after, NULL);

/* When the Configuration Response of the Host is the last step to complete,
 * the channel reports connected() on the Bluetooth workqueue, which also
 * handles the data and the disconnection of the channel, not on the context
 * that reports the response as sent.
 */
static ZTEST(l2cap_br_conf, test_connected_on_workq)
{
	static struct bt_l2cap_br_chan chan;

	chan_config_held(&chan, PEER_CID);

	release();

	zassert_ok(k_sem_take(&connected_sem, TX_TIMEOUT));
	zassert_equal(chan.state, BT_L2CAP_CONNECTED);
	zassert_equal(atomic_get(&connected_count), 1);
	zassert_equal(atomic_get(&connected_off_workq), 0, "connected() off the workqueue");
}

/* Data from the peer before the response is reported as sent completes the
 * configuration first, so the channel is connected before it gets the data,
 * and the late completion does not report connected() again.
 */
static ZTEST(l2cap_br_conf, test_data_before_sent)
{
	static struct bt_l2cap_br_chan chan;

	chan_config_held(&chan, PEER_CID + 1U);

	recv_data(&chan);

	zassert_equal(chan.state, BT_L2CAP_CONNECTED);
	zassert_equal(atomic_get(&recv_count), 1);
	zassert_equal(atomic_get(&recv_before_connected), 0);
	zassert_equal(atomic_get(&connected_count), 1);

	release();

	/* Nothing is to happen, so wait as long as the completion takes to arrive */
	(void)k_sleep(K_MSEC(10));

	zassert_equal(atomic_get(&connected_count), 1);
	zassert_equal(atomic_get(&connected_off_workq), 0, "connected() off the workqueue");
}

/* The ACL goes down while the Configuration Response of the Host is still
 * unacknowledged, so the response is reported with -ESHUTDOWN. The channel is
 * released with the ACL, connected() is not called, and the Bluetooth
 * workqueue keeps running.
 */
static void acl_lost_expect_released(void)
{
	/* The controller is not to report the held packet any more */
	hold = false;
	(void)atomic_clear(&unreported);

	recv_disconn_complete();

	zassert_ok(k_sem_take(&released_sem, K_MSEC(100)), "Channel not released");
	zassert_equal(atomic_get(&released_count), 1);
	zassert_equal(atomic_get(&connected_count), 0);
	zassert_true(workq_runs(), "Bluetooth workqueue stalled");

	bt_conn_drop(&conn);
	zassert_ok(k_sem_take(&recycled_sem, K_SECONDS(1)), "Connection not recycled");
}

/* The response fails while the channel is still being configured */
static ZTEST(l2cap_br_conf, test_acl_lost_in_config)
{
	static struct bt_l2cap_br_chan chan;

	chan_config_held(&chan, PEER_CID + 2U);

	acl_lost_expect_released();
}

/* The response fails after the application has started to disconnect the
 * channel
 */
static ZTEST(l2cap_br_conf, test_acl_lost_while_disconnecting)
{
	static struct bt_l2cap_br_chan chan;

	chan_config_held(&chan, PEER_CID + 3U);

	zassert_ok(bt_l2cap_chan_disconnect(&chan.chan));
	zassert_equal(chan.state, BT_L2CAP_DISCONNECTING);

	acl_lost_expect_released();
}
