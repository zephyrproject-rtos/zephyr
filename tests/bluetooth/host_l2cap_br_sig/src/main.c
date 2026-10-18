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
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/classic/l2cap_br_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define SIG_CID      0x0001U
#define CONNLESS_CID 0x0002U
#define ECHO_REQ     0x08U
#define ECHO_RSP     0x09U
#define CONNLESS_PSM 0x1001U
#define MAX_CALLS    2
#define MAX_DATA     8

#define GENERIC_RP_SIZE 64U
#define ACL_MAX_LEN     310U
#define LE_ACL_MAX_LEN  27U
#define LE_ACL_MAX_NUM  3U
#define HANDLE          0x0001U

/* Headers of an ACL packet from the controller, after its H:4 packet type */
#define ACL_HDR_OFFSET   1U
#define L2CAP_HDR_OFFSET (ACL_HDR_OFFSET + sizeof(struct bt_hci_acl_hdr))
#define SIG_HDR_OFFSET   (L2CAP_HDR_OFFSET + sizeof(struct bt_l2cap_hdr))

#define SEC_PSM    0x1003U
#define PEER_CID   0x0040U
#define PEER_IDENT 0x80U

/* Long enough for the Host to act on what the test did, far shorter than any
 * signalling or HCI command timeout
 */
#define TIMEOUT K_MSEC(100)

/* What one callback invocation saw */
struct seen {
	uint8_t ident;
	uint16_t len;
	uint8_t data[MAX_DATA];
};

/* Calls seen by each of two registered callbacks */
struct cb_log {
	struct seen calls[MAX_CALLS];
	size_t count;
};

static struct cb_log echo_log[2];
static struct cb_log echo_rsp_log[2];
static struct cb_log connless_log[2];
static struct bt_conn *conn;

NET_BUF_POOL_FIXED_DEFINE(pdu_pool, 1, 32, 0, NULL);

/* The PDUs that the Host sent and the test has not taken yet: at most the
 * Echo Responses of a test
 */
NET_BUF_POOL_FIXED_DEFINE(sent_pool, 2, ACL_MAX_LEN, 0, NULL);

/* The L2CAP PDUs that the Host sent */
static K_FIFO_DEFINE(sent);

/* ACL packets that the controller has not reported completed yet */
static atomic_t unreported;

/* The controller holds back its report of completed packets until
 * report_completed()
 */
static bool hold_completed;

/* The driver fails to send the next ACL packet */
static bool fail;

/* Received from the peer as the driver takes the next ACL packet, before the
 * Host learns how sending it went
 */
static struct net_buf *peer_rx;

/* The controller rejects a disconnection with this reason */
static uint8_t rejected_reason;

/* The reason of the last disconnection that the controller accepted */
static uint8_t disconnect_reason;
static K_SEM_DEFINE(disconnect_sem, 0, 1);
static K_SEM_DEFINE(link_down_sem, 0, 1);

/* Counts the disconnected callbacks of the channels */
static K_SEM_DEFINE(chan_down_sem, 0, K_SEM_MAX_LIMIT);

/* Records the data and then consumes it, as a callback parsing it would */
static void record(struct cb_log *rec, uint8_t ident, struct net_buf *buf)
{
	struct seen *s;

	zassert_true(rec->count < ARRAY_SIZE(rec->calls), "Too many calls");
	zassert_true(buf->len <= sizeof(s->data));

	s = &rec->calls[rec->count++];
	s->ident = ident;
	s->len = buf->len;
	memcpy(s->data, buf->data, buf->len);

	(void)net_buf_pull(buf, buf->len);
}

static void check(const struct cb_log *rec, size_t call, uint8_t ident, const char *data)
{
	const struct seen *s = &rec->calls[call];

	zassert_equal(s->ident, ident, "call %zu: ident %u", call, s->ident);
	zassert_equal(s->len, strlen(data), "call %zu: len %u", call, s->len);
	zassert_mem_equal(s->data, data, strlen(data), "call %zu: data", call);
}

static void echo_req_0(struct bt_conn *acl, uint8_t ident, struct net_buf *buf)
{
	record(&echo_log[0], ident, buf);
}

static void echo_req_1(struct bt_conn *acl, uint8_t ident, struct net_buf *buf)
{
	record(&echo_log[1], ident, buf);
}

static void echo_rsp_0(struct bt_conn *acl, struct net_buf *buf)
{
	record(&echo_rsp_log[0], 0, buf);
}

static void echo_rsp_1(struct bt_conn *acl, struct net_buf *buf)
{
	record(&echo_rsp_log[1], 0, buf);
}

static struct bt_l2cap_br_echo_cb echo_cb[] = {
	{ .req = echo_req_0, .rsp = echo_rsp_0 },
	{ .req = echo_req_1, .rsp = echo_rsp_1 },
};

static void connless_recv_0(struct bt_conn *acl, uint16_t psm, struct net_buf *buf)
{
	zassert_equal(psm, CONNLESS_PSM);
	record(&connless_log[0], 0, buf);
}

static void connless_recv_1(struct bt_conn *acl, uint16_t psm, struct net_buf *buf)
{
	zassert_equal(psm, CONNLESS_PSM);
	record(&connless_log[1], 0, buf);
}

/* One callback for all PSMs and one for CONNLESS_PSM, so both get the frame */
static struct bt_l2cap_br_connless_cb connless_cb[] = {
	{ .psm = 0, .sec_level = BT_SECURITY_L0, .recv = connless_recv_0 },
	{ .psm = CONNLESS_PSM, .sec_level = BT_SECURITY_L0, .recv = connless_recv_1 },
};

static struct net_buf *l2cap_pdu(uint16_t cid)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&pdu_pool, K_NO_WAIT);
	zassert_not_null(buf);

	/* Length, filled in by send_pdu() */
	net_buf_add_le16(buf, 0U);
	net_buf_add_le16(buf, cid);

	return buf;
}

static void send_pdu(struct net_buf *buf)
{
	sys_put_le16(buf->len - 4U, buf->data);
	bt_l2cap_br_recv(conn, buf);
}

static void add_sig_cmd(struct net_buf *buf, uint8_t code, uint8_t ident, const char *data)
{
	net_buf_add_u8(buf, code);
	net_buf_add_u8(buf, ident);
	net_buf_add_le16(buf, strlen(data));
	net_buf_add_mem(buf, data, strlen(data));
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
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
}

static void cmd_status(struct net_buf **buf, uint8_t status, uint16_t opcode)
{
	struct bt_hci_evt_cmd_status *cs;

	*buf = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(*buf, sizeof(*cs));
	cs->status = status;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);
}

static void disconnect(struct net_buf *cmd, struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_cp_disconnect *cp = (void *)cmd->data;

	if (cp->reason == rejected_reason) {
		cmd_status(evt, BT_HCI_ERR_CMD_DISALLOWED, opcode);
		return;
	}

	disconnect_reason = cp->reason;
	k_sem_give(&disconnect_sem);
	cmd_status(evt, BT_HCI_ERR_SUCCESS, opcode);
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
	case BT_HCI_OP_DISCONNECT:
		disconnect(cmd, &evt, opcode);
		break;
	default:
		generic_success(&evt, opcode);
		break;
	}

	bt_hci_recv(dev, evt);
}

static void completed_handler(struct k_work *work)
{
	struct bt_hci_evt_num_completed_packets *ev;
	struct bt_hci_handle_count *hc;
	struct net_buf *evt;
	atomic_val_t count;

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

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), evt);
}

/* Reports completed packets from a thread of its own, as a driver does, rather
 * than from the system workqueue, on which the Host runs TX callbacks
 */
static K_THREAD_STACK_DEFINE(ctlr_stack, 1024);
static struct k_work_q ctlr_wq;
static K_WORK_DEFINE(completed_work, completed_handler);

/* The controller has one ACL buffer, which it reports free again */
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
	if (!hold_completed) {
		zassert_true(k_work_submit_to_queue(&ctlr_wq, &completed_work) >= 0);
	}
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type;

	if ((buf->data[0] == BT_HCI_H4_ACL) && (peer_rx != NULL)) {
		bt_hci_recv(dev, peer_rx);
		peer_rx = NULL;
	}

	/* The buffer stays with the Host on an error */
	if ((buf->data[0] == BT_HCI_H4_ACL) && fail) {
		fail = false;
		return -EIO;
	}

	type = net_buf_pull_u8(buf);
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

static K_FIFO_DEFINE(rx_fifo);
static K_SEM_DEFINE(rx_done, 0, 1);

/* Delivers events as the receive thread of a driver does, so that the test
 * sees when the Host blocks it
 */
static void rx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), k_fifo_get(&rx_fifo, K_FOREVER));
		k_sem_give(&rx_done);
	}
}

K_THREAD_DEFINE(rx_tid, 1024, rx_thread, NULL, NULL, NULL,
		K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO), 0, 0);

/* The controller reports the link down, and the Host cleans up after it */
static void link_down(uint8_t reason)
{
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	/* The controller drops what it had to send on the link */
	(void)atomic_clear(&unreported);

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, TIMEOUT);
	zassert_not_null(buf);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(HANDLE);
	evt->reason = reason;

	k_fifo_put(&rx_fifo, buf);
	zassert_ok(k_sem_take(&rx_done, TIMEOUT), "Receive thread blocked");
	zassert_ok(k_sem_take(&link_down_sem, TIMEOUT), "Link not cleaned up");
}

/* The next L2CAP PDU that the Host sent must be a signalling command with
 * the given code. Returns its identifier.
 */
static uint8_t tx_sig(uint8_t code)
{
	struct bt_l2cap_sig_hdr *sig;
	struct bt_l2cap_hdr *hdr;
	struct net_buf *buf;
	uint8_t ident;

	buf = k_fifo_get(&sent, TIMEOUT);
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

/* A signalling command from the peer, up to its data */
static struct net_buf *sig_cmd(uint8_t code, uint8_t ident)
{
	struct bt_l2cap_sig_hdr *sig;
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, TIMEOUT);
	zassert_not_null(buf);
	(void)net_buf_add(buf, sizeof(struct bt_hci_acl_hdr));
	(void)net_buf_add(buf, sizeof(struct bt_l2cap_hdr));

	sig = net_buf_add(buf, sizeof(*sig));
	sig->code = code;
	sig->ident = ident;

	return buf;
}

/* Fill in the headers of a command from sig_cmd(), once its data is added */
static void sig_pack(struct net_buf *buf)
{
	struct bt_l2cap_sig_hdr *sig = (void *)&buf->data[SIG_HDR_OFFSET];
	struct bt_l2cap_hdr *hdr = (void *)&buf->data[L2CAP_HDR_OFFSET];
	struct bt_hci_acl_hdr *acl = (void *)&buf->data[ACL_HDR_OFFSET];

	sig->len = sys_cpu_to_le16(buf->len - SIG_HDR_OFFSET - sizeof(*sig));
	hdr->len = sys_cpu_to_le16(buf->len - SIG_HDR_OFFSET);
	hdr->cid = sys_cpu_to_le16(BT_L2CAP_CID_BR_SIG);
	acl->handle = sys_cpu_to_le16(bt_acl_handle_pack(HANDLE, BT_ACL_START));
	acl->len = sys_cpu_to_le16(buf->len - L2CAP_HDR_OFFSET);
}

/* Deliver the command and let the Host handle it, which takes no simulated time */
static void recv_sig(struct net_buf *buf)
{
	sig_pack(buf);
	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
	(void)k_sleep(K_MSEC(1));
}

/* The peer asks for a channel that the Host refuses */
static void refused_conn_req(void)
{
	struct net_buf *buf;

	buf = sig_cmd(BT_L2CAP_CONN_REQ, PEER_IDENT);
	net_buf_add_le16(buf, SEC_PSM);
	net_buf_add_le16(buf, PEER_CID);
	recv_sig(buf);
}

/* As refused_conn_req(), with a Connection Response that the driver fails to send */
static void fail_conn_rsp(void)
{
	fail = true;
	refused_conn_req();
}

/* The peer disconnects the channel it asked for as the driver takes the
 * Connection Response
 */
static void peer_disconnects(void)
{
	struct net_buf *buf;

	buf = sig_cmd(BT_L2CAP_DISCONN_REQ, PEER_IDENT + 1U);
	/* The Host finds the channel by the CID of the peer */
	net_buf_add_le16(buf, 0x0000U);
	net_buf_add_le16(buf, PEER_CID);
	sig_pack(buf);
	peer_rx = buf;
}

/* The controller reports the packets that it held back completed */
static void report_completed(void)
{
	hold_completed = false;
	zassert_true(k_work_submit_to_queue(&ctlr_wq, &completed_work) >= 0);
}

static int chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	ARG_UNUSED(chan);
	ARG_UNUSED(buf);

	return 0;
}

static void chan_disconnected(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);

	k_sem_give(&chan_down_sem);
}

static int accept(struct bt_conn *c, struct bt_l2cap_server *server, struct bt_l2cap_chan **chan)
{
	static const struct bt_l2cap_chan_ops chan_ops = {
		.recv = chan_recv,
		.disconnected = chan_disconnected,
	};
	static struct bt_l2cap_br_chan br_chan;

	ARG_UNUSED(c);
	ARG_UNUSED(server);

	(void)memset(&br_chan, 0, sizeof(br_chan));
	br_chan.chan.ops = &chan_ops;
	*chan = &br_chan.chan;

	return 0;
}

static void link_disconnected(struct bt_conn *c, uint8_t reason)
{
	ARG_UNUSED(c);
	ARG_UNUSED(reason);

	k_sem_give(&link_down_sem);
}

static void *setup(void)
{
	static struct bt_l2cap_server server = {
		.psm = SEC_PSM,
		/* Out of reach without authentication callbacks */
		.sec_level = BT_SECURITY_L3,
		.accept = accept,
	};
	static struct bt_conn_cb conn_cb = {
		.disconnected = link_disconnected,
	};

	ARRAY_FOR_EACH_PTR(echo_cb, cb) {
		zassert_ok(bt_l2cap_br_echo_cb_register(cb));
	}

	ARRAY_FOR_EACH_PTR(connless_cb, cb) {
		zassert_ok(bt_l2cap_br_connless_register(cb));
	}

	k_work_queue_start(&ctlr_wq, ctlr_stack, K_THREAD_STACK_SIZEOF(ctlr_stack),
			   K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO), NULL);

	zassert_ok(bt_enable(NULL));
	zassert_ok(bt_conn_cb_register(&conn_cb));
	zassert_ok(bt_l2cap_br_server_register(&server));

	return NULL;
}

static void before(void *fixture)
{
	static const bt_addr_t addr = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };
	struct net_buf *buf;
	uint8_t ident;

	ARG_UNUSED(fixture);

	memset(echo_log, 0, sizeof(echo_log));
	memset(echo_rsp_log, 0, sizeof(echo_rsp_log));
	memset(connless_log, 0, sizeof(connless_log));

	hold_completed = false;
	fail = false;
	rejected_reason = 0U;
	k_sem_reset(&disconnect_sem);
	k_sem_reset(&chan_down_sem);

	/* As on Connection Complete */
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

static void after(void *fixture)
{
	struct net_buf *buf;

	ARG_UNUSED(fixture);

	/* Left over when the driver did not get to send */
	if (peer_rx != NULL) {
		net_buf_unref(peer_rx);
		peer_rx = NULL;
	}

	if (conn == NULL) {
		return;
	}

	if (conn->state == BT_CONN_CONNECTED) {
		link_down(BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	/* What the Host sent that the test did not take */
	buf = k_fifo_get(&sent, K_NO_WAIT);
	while (buf != NULL) {
		net_buf_unref(buf);
		buf = k_fifo_get(&sent, K_NO_WAIT);
	}

	bt_conn_unref(conn);
	conn = NULL;
}

ZTEST_SUITE(l2cap_br_sig, NULL, setup, before, after, NULL);

/* Each Echo Request callback sees exactly the data of that request, although
 * another request follows in the same signaling packet and the other callback
 * consumes the data.
 */
ZTEST(l2cap_br_sig, test_echo_req)
{
	struct net_buf *buf = l2cap_pdu(SIG_CID);

	add_sig_cmd(buf, ECHO_REQ, 1, "abc");
	add_sig_cmd(buf, ECHO_REQ, 2, "xy");
	send_pdu(buf);

	ARRAY_FOR_EACH_PTR(echo_log, rec) {
		zassert_equal(rec->count, 2);
		check(rec, 0, 1, "abc");
		check(rec, 1, 2, "xy");
	}
}

/* Each Echo Response callback sees exactly the data of the response */
ZTEST(l2cap_br_sig, test_echo_rsp)
{
	struct bt_l2cap_chan *sig = bt_l2cap_br_lookup_rx_cid(conn, SIG_CID);
	struct net_buf *buf;

	zassert_not_null(sig);

	/* As if an Echo Request with identifier 3 had been sent */
	CONTAINER_OF(sig, struct bt_l2cap_br_chan, chan)->ident = 3;

	buf = l2cap_pdu(SIG_CID);
	add_sig_cmd(buf, ECHO_RSP, 3, "pq");
	add_sig_cmd(buf, ECHO_REQ, 4, "z");
	send_pdu(buf);

	ARRAY_FOR_EACH_PTR(echo_rsp_log, rec) {
		zassert_equal(rec->count, 1);
		check(rec, 0, 0, "pq");
	}
}

/* Each connectionless callback sees the whole data, although the other one consumes it */
ZTEST(l2cap_br_sig, test_connless_recv)
{
	struct net_buf *buf = l2cap_pdu(CONNLESS_CID);

	net_buf_add_le16(buf, CONNLESS_PSM);
	net_buf_add_mem(buf, "hello", strlen("hello"));
	send_pdu(buf);

	ARRAY_FOR_EACH_PTR(connless_log, rec) {
		zassert_equal(rec->count, 1);
		check(rec, 0, 0, "hello");
	}
}

/* A Connection Response refusing a channel that the peer disconnects before
 * the Host learns that the response was sent: the refusal still takes the link
 * down.
 */
ZTEST(l2cap_br_sig, test_conn_rsp_peer_disconnects)
{
	hold_completed = true;
	peer_disconnects();
	refused_conn_req();

	zassert_equal(k_sem_count_get(&chan_down_sem), 1U, "Channel not disconnected by the peer");
	report_completed();

	zassert_ok(k_sem_take(&disconnect_sem, TIMEOUT), "Link not disconnected");
	zassert_equal(disconnect_reason, BT_HCI_ERR_AUTH_FAIL, "reason 0x%02x", disconnect_reason);
	link_down(BT_HCI_ERR_LOCALHOST_TERM_CONN);
	zassert_equal(k_sem_count_get(&chan_down_sem), 1U);
}

/* A Connection Response refusing a channel, which the driver fails to send:
 * the TX processor does not wait for the HCI command that disconnects the
 * link.
 */
ZTEST(l2cap_br_sig, test_conn_rsp_send_failure)
{
	fail_conn_rsp();

	zassert_ok(k_sem_take(&disconnect_sem, TIMEOUT), "Link not disconnected");
	link_down(BT_HCI_ERR_LOCALHOST_TERM_CONN);
	zassert_equal(k_sem_count_get(&chan_down_sem), 1U);
}

/* As above, but the controller rejects the disconnection for the failure, so
 * that the link would go on: the refusal still takes the link down.
 */
ZTEST(l2cap_br_sig, test_conn_rsp_send_failure_disconnect_rejected)
{
	rejected_reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
	fail_conn_rsp();

	zassert_ok(k_sem_take(&disconnect_sem, TIMEOUT), "Link not disconnected");
	zassert_equal(disconnect_reason, BT_HCI_ERR_AUTH_FAIL, "reason 0x%02x", disconnect_reason);
	link_down(BT_HCI_ERR_LOCALHOST_TERM_CONN);
	zassert_equal(k_sem_count_get(&chan_down_sem), 1U);
}

/* As above, and the peer disconnects the channel before the Host gets to act
 * on the failure: the refusal still takes the link down.
 */
ZTEST(l2cap_br_sig, test_conn_rsp_send_failure_peer_disconnects)
{
	peer_disconnects();
	rejected_reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
	fail_conn_rsp();

	zassert_ok(k_sem_take(&disconnect_sem, TIMEOUT), "Link not disconnected");
	zassert_equal(disconnect_reason, BT_HCI_ERR_AUTH_FAIL, "reason 0x%02x", disconnect_reason);
	zassert_equal(k_sem_count_get(&chan_down_sem), 1U, "Channel not disconnected by the peer");
	link_down(BT_HCI_ERR_LOCALHOST_TERM_CONN);
	zassert_equal(k_sem_count_get(&chan_down_sem), 1U);
}
