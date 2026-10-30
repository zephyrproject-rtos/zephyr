/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

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
#include "host/l2cap_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define GENERIC_RP_SIZE 64U
#define LE_ACL_MAX_LEN  27U
#define HANDLE          0x0001U

/* Headers of an ACL packet from the controller, after its H:4 packet type */
#define ACL_HDR_OFFSET   1U
#define L2CAP_HDR_OFFSET (ACL_HDR_OFFSET + sizeof(struct bt_hci_acl_hdr))
#define SIG_HDR_OFFSET   (L2CAP_HDR_OFFSET + sizeof(struct bt_l2cap_hdr))

#define DATA        "data"
#define PSM         0x0080U
#define UNKNOWN_PSM 0x0081U
#define PEER_CID    0x0040U
#define PEER_IDENT  0x80U
#define LE_MIN_MTU  23U

/* How long to wait for a PDU that the Host is to send. It sends it without
 * simulated time passing, so this only bounds a failed wait.
 */
#define TX_TIMEOUT K_MSEC(100)

/* One SDU is sent at a time */
NET_BUF_POOL_FIXED_DEFINE(sdu_pool, 1, BT_L2CAP_SDU_CHAN_SEND_RESERVE + sizeof(DATA),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
/* The PDUs that the Host sent and the test has not taken yet: at most a signalling
 * response and the SDU, which are all that it has to send at once
 */
NET_BUF_POOL_FIXED_DEFINE(sent_pool, 2, LE_ACL_MAX_LEN, 0, NULL);

static struct bt_conn *conn;
static struct bt_l2cap_le_chan chan;

/* The L2CAP PDUs that the Host sent */
static K_FIFO_DEFINE(sent);

/* The controller has one ACL buffer, which it reports free again unless the
 * test holds it, as for a peer that does not acknowledge at the link layer.
 */
static atomic_t unreported;
static bool hold;

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
	rp->le_max_num = 1U;
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

	bt_hci_recv(dev, evt);
}

static K_WORK_DEFINE(completed_work, completed_handler);

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

/* The next L2CAP PDU that the Host sent, with its header pulled */
static struct net_buf *tx(uint16_t cid)
{
	struct bt_l2cap_hdr *hdr;
	struct net_buf *buf;

	buf = k_fifo_get(&sent, TX_TIMEOUT);
	zassert_not_null(buf, "Nothing sent");

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	zassert_equal(sys_le16_to_cpu(hdr->cid), cid, "cid 0x%04x", sys_le16_to_cpu(hdr->cid));

	return buf;
}

/* The next PDU must be a signalling command with the given code. Returns its
 * identifier.
 */
static uint8_t tx_sig(uint8_t code)
{
	struct bt_l2cap_sig_hdr *sig;
	struct net_buf *buf;
	uint8_t ident;

	buf = tx(BT_L2CAP_CID_LE_SIG);

	sig = net_buf_pull_mem(buf, sizeof(*sig));
	zassert_equal(sig->code, code, "code 0x%02x", sig->code);
	ident = sig->ident;

	net_buf_unref(buf);

	return ident;
}

static struct net_buf *sig_cmd(uint8_t code, uint8_t ident)
{
	struct bt_l2cap_sig_hdr *sig;
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	(void)net_buf_add(buf, sizeof(struct bt_hci_acl_hdr) + sizeof(struct bt_l2cap_hdr));

	sig = net_buf_add(buf, sizeof(*sig));
	sig->code = code;
	sig->ident = ident;

	return buf;
}

/* Deliver the PDU, whose payload starts at SIG_HDR_OFFSET, and let the Host
 * handle it, which takes no simulated time
 */
static void recv_pdu(struct net_buf *buf, uint16_t cid)
{
	struct bt_hci_acl_hdr *acl = (void *)&buf->data[ACL_HDR_OFFSET];
	struct bt_l2cap_hdr *hdr = (void *)&buf->data[L2CAP_HDR_OFFSET];

	hdr->len = sys_cpu_to_le16(buf->len - SIG_HDR_OFFSET);
	hdr->cid = sys_cpu_to_le16(cid);
	acl->handle = sys_cpu_to_le16(bt_acl_handle_pack(HANDLE, BT_ACL_START));
	acl->len = sys_cpu_to_le16(buf->len - L2CAP_HDR_OFFSET);

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
	(void)k_sleep(K_MSEC(1));
}

static void recv_sig(struct net_buf *buf)
{
	struct bt_l2cap_sig_hdr *sig = (void *)&buf->data[SIG_HDR_OFFSET];

	sig->len = sys_cpu_to_le16(buf->len - SIG_HDR_OFFSET - sizeof(*sig));
	recv_pdu(buf, BT_L2CAP_CID_LE_SIG);
}

/* A connection request for a PSM without a server, which the Host refuses */
static void recv_unknown_conn_req(uint8_t ident)
{
	struct net_buf *buf;

	buf = sig_cmd(BT_L2CAP_LE_CONN_REQ, ident);
	net_buf_add_le16(buf, UNKNOWN_PSM);
	net_buf_add_le16(buf, PEER_CID + 1U);
	net_buf_add_le16(buf, LE_MIN_MTU);
	net_buf_add_le16(buf, LE_MIN_MTU);
	net_buf_add_le16(buf, 1U);
	recv_sig(buf);
}

static int chan_recv(struct bt_l2cap_chan *ch, struct net_buf *buf)
{
	return 0;
}

static const struct bt_l2cap_chan_ops chan_ops = {
	.recv = chan_recv,
};

/* Connect a credit-based channel, to which the peer gives no credits */
static void chan_connect(struct bt_l2cap_le_chan *ch, const struct bt_l2cap_chan_ops *ops,
			 uint16_t peer_cid)
{
	struct net_buf *buf;
	uint8_t ident;

	ch->chan.ops = ops;
	ch->rx.mtu = LE_MIN_MTU;
	zassert_ok(bt_l2cap_chan_connect(conn, &ch->chan, PSM));

	ident = tx_sig(BT_L2CAP_LE_CONN_REQ);
	buf = sig_cmd(BT_L2CAP_LE_CONN_RSP, ident);
	net_buf_add_le16(buf, peer_cid);
	net_buf_add_le16(buf, LE_MIN_MTU);
	net_buf_add_le16(buf, LE_MIN_MTU);
	net_buf_add_le16(buf, 0U);
	net_buf_add_le16(buf, BT_L2CAP_LE_SUCCESS);
	recv_sig(buf);

	zassert_equal(ch->state, BT_L2CAP_CONNECTED);
	zassert_equal(atomic_get(&ch->tx.credits), 0);
}

static void *setup(void)
{
	static const bt_addr_le_t addr = {
		.type = BT_ADDR_LE_PUBLIC,
		.a = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } },
	};

	zassert_ok(bt_enable(NULL));

	/* As on Connection Complete */
	conn = bt_conn_add_le(BT_ID_DEFAULT, &addr);
	zassert_not_null(conn);
	conn->handle = HANDLE;
	bt_conn_set_state(conn, BT_CONN_CONNECTED);
	bt_l2cap_connected(conn);

	chan_connect(&chan, &chan_ops, PEER_CID);

	return NULL;
}

ZTEST_SUITE(host_l2cap_le_tx, NULL, setup, NULL, NULL, NULL);

/* A channel without credits does not hold up another channel of the connection */
static ZTEST(host_l2cap_le_tx, test_no_credits)
{
	struct net_buf *buf;

	/* A response takes the controller's buffer until release() */
	hold = true;
	recv_unknown_conn_req(PEER_IDENT);
	zassert_equal(tx_sig(BT_L2CAP_LE_CONN_RSP), PEER_IDENT);

	/* Queued meanwhile: an SDU on the channel without credits, and behind it
	 * a response on the signalling channel
	 */
	buf = net_buf_alloc(&sdu_pool, K_NO_WAIT);
	zassert_not_null(buf);
	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
	(void)net_buf_add_mem(buf, DATA, strlen(DATA));
	zassert_ok(bt_l2cap_chan_send(&chan.chan, buf));

	recv_unknown_conn_req(PEER_IDENT + 1U);

	release();

	zassert_equal(tx_sig(BT_L2CAP_LE_CONN_RSP), PEER_IDENT + 1U);

	/* The SDU follows its credit */
	buf = sig_cmd(BT_L2CAP_LE_CREDITS, PEER_IDENT + 2U);
	net_buf_add_le16(buf, PEER_CID);
	net_buf_add_le16(buf, 1U);
	recv_sig(buf);

	/* A single K-frame: the SDU length, then the SDU */
	buf = tx(PEER_CID);
	zassert_equal(net_buf_pull_le16(buf), strlen(DATA));
	zassert_equal(buf->len, strlen(DATA));
	zassert_mem_equal(buf->data, DATA, strlen(DATA));
	net_buf_unref(buf);
}

/* Given by the status callback once the channel is out of credits */
static K_SEM_DEFINE(paused_sem, 0, 1);
/* What the status callback saw: the credits left and the command's result */
static atomic_val_t paused_credits;
static int paused_err;

/* Out of credits, wait for the Controller, as bt_conn_disconnect() does */
static void chan_status(struct bt_l2cap_chan *ch, atomic_t *status)
{
	if (atomic_test_bit(status, BT_L2CAP_STATUS_OUT)) {
		return;
	}

	paused_credits = atomic_get(&BT_L2CAP_LE_CHAN(ch)->tx.credits);
	paused_err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, NULL);
	k_sem_give(&paused_sem);
}

/* The status callback of a channel that runs out of credits may wait for the Controller */
static ZTEST(host_l2cap_le_tx, test_status_sync)
{
	static const struct bt_l2cap_chan_ops ops = {
		.recv = chan_recv,
		.status = chan_status,
	};
	static struct bt_l2cap_le_chan status_chan;
	/* Not a CID of the other test */
	const uint16_t peer_cid = PEER_CID + 2U;
	struct net_buf *buf;
	uint8_t ident;

	status_chan.chan.ops = &ops;
	status_chan.rx.mtu = LE_MIN_MTU;
	zassert_ok(bt_l2cap_chan_connect(conn, &status_chan.chan, PSM));

	/* Credit for one K-frame */
	ident = tx_sig(BT_L2CAP_LE_CONN_REQ);
	buf = sig_cmd(BT_L2CAP_LE_CONN_RSP, ident);
	net_buf_add_le16(buf, peer_cid);
	net_buf_add_le16(buf, LE_MIN_MTU);
	net_buf_add_le16(buf, LE_MIN_MTU);
	net_buf_add_le16(buf, 1U);
	net_buf_add_le16(buf, BT_L2CAP_LE_SUCCESS);
	recv_sig(buf);
	zassert_equal(status_chan.state, BT_L2CAP_CONNECTED);

	buf = net_buf_alloc(&sdu_pool, K_NO_WAIT);
	zassert_not_null(buf);
	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
	(void)net_buf_add_mem(buf, DATA, strlen(DATA));
	zassert_ok(bt_l2cap_chan_send(&status_chan.chan, buf));

	zassert_ok(k_sem_take(&paused_sem, TX_TIMEOUT), "The status callback did not return");
	zassert_equal(paused_credits, 0);
	zassert_ok(paused_err);

	buf = tx(peer_cid);
	zassert_equal(net_buf_pull_le16(buf), strlen(DATA));
	zassert_mem_equal(buf->data, DATA, strlen(DATA));
	net_buf_unref(buf);
}

/* Nothing else may follow what a test expected the Host to send */
static void tx_none(void)
{
	zassert_is_null(k_fifo_get(&sent, TX_TIMEOUT), "Unexpected PDU");
}

/* The next PDU must be a Disconnection Request, which the peer accepts. Returns
 * the destination CID of the request.
 */
static uint16_t disconn_accept(void)
{
	struct bt_l2cap_disconn_req *req;
	struct bt_l2cap_sig_hdr *sig;
	uint16_t dcid, scid;
	struct net_buf *buf;
	uint8_t ident;

	buf = tx(BT_L2CAP_CID_LE_SIG);
	sig = net_buf_pull_mem(buf, sizeof(*sig));
	zassert_equal(sig->code, BT_L2CAP_DISCONN_REQ, "code 0x%02x", sig->code);
	ident = sig->ident;
	req = net_buf_pull_mem(buf, sizeof(*req));
	dcid = sys_le16_to_cpu(req->dcid);
	scid = sys_le16_to_cpu(req->scid);
	net_buf_unref(buf);

	buf = sig_cmd(BT_L2CAP_DISCONN_RSP, ident);
	net_buf_add_le16(buf, dcid);
	net_buf_add_le16(buf, scid);
	recv_sig(buf);

	return dcid;
}

/* A channel is disconnected while the Disconnection Request of another one waits
 * for the controller
 */
static ZTEST(host_l2cap_le_tx, test_disconnect_two)
{
	static struct bt_l2cap_le_chan chans[2];
	/* Not CIDs of the other tests */
	const uint16_t peer_cid = PEER_CID + 3U;

	ARRAY_FOR_EACH(chans, i) {
		chan_connect(&chans[i], &chan_ops, peer_cid + i);
	}

	/* A response takes the controller's buffer until release() */
	hold = true;
	recv_unknown_conn_req(PEER_IDENT + 3U);
	zassert_equal(tx_sig(BT_L2CAP_LE_CONN_RSP), PEER_IDENT + 3U);

	ARRAY_FOR_EACH(chans, i) {
		zassert_ok(bt_l2cap_chan_disconnect(&chans[i].chan));
	}

	release();

	ARRAY_FOR_EACH(chans, i) {
		zassert_equal(disconn_accept(), peer_cid + i);
		zassert_equal(chans[i].state, BT_L2CAP_DISCONNECTED);
	}

	tx_none();
}

/* A channel that waits for a response to a request of its own keeps the request
 * and its timer when its Disconnection Request finds no buffer
 */
static ZTEST(host_l2cap_le_tx, test_disconnect_reconf_pending)
{
	static struct bt_l2cap_le_chan chans[2];
	struct bt_l2cap_chan *reconf[] = { &chans[1].chan, NULL };
	/* Not CIDs of the other tests */
	const uint16_t peer_cid = PEER_CID + 7U;
	struct net_buf *buf;
	uint8_t ident;

	ARRAY_FOR_EACH(chans, i) {
		chan_connect(&chans[i], &chan_ops, peer_cid + i);
	}

	zassert_ok(bt_l2cap_ecred_chan_reconfigure(reconf, LE_MIN_MTU));
	ident = tx_sig(BT_L2CAP_ECRED_RECONF_REQ);

	/* The Disconnection Request of the first channel waits for the controller */
	hold = true;
	recv_unknown_conn_req(PEER_IDENT + 5U);
	zassert_equal(tx_sig(BT_L2CAP_LE_CONN_RSP), PEER_IDENT + 5U);
	zassert_ok(bt_l2cap_chan_disconnect(&chans[0].chan));

	zassert_equal(bt_l2cap_chan_disconnect(&chans[1].chan), -ENOMEM);
	zassert_equal(chans[1].state, BT_L2CAP_CONNECTED);
	zassert_true(k_work_delayable_is_pending(&chans[1].rtx_work));

	release();
	zassert_equal(disconn_accept(), peer_cid);

	buf = sig_cmd(BT_L2CAP_ECRED_RECONF_RSP, ident);
	net_buf_add_le16(buf, BT_L2CAP_RECONF_SUCCESS);
	recv_sig(buf);

	zassert_ok(bt_l2cap_chan_disconnect(&chans[1].chan));
	zassert_equal(disconn_accept(), peer_cid + 1U);
	zassert_equal(chans[1].state, BT_L2CAP_DISCONNECTED);

	tx_none();
}

/* The pool that the Host allocates its signalling PDUs from, while a test holds all of it */
static struct net_buf *host_pdus[CONFIG_BT_L2CAP_TX_BUF_COUNT];

static void host_pdus_take(void)
{
	ARRAY_FOR_EACH(host_pdus, i) {
		host_pdus[i] = bt_conn_create_pdu_timeout(NULL, 0, K_NO_WAIT);
		zassert_not_null(host_pdus[i]);
	}
}

static void host_pdus_give(void)
{
	ARRAY_FOR_EACH(host_pdus, i) {
		net_buf_unref(host_pdus[i]);
		host_pdus[i] = NULL;
	}
}

/* The SDU that test_credits_no_buf receives */
NET_BUF_POOL_FIXED_DEFINE(rx_sdu_pool, 1, LE_MIN_MTU, sizeof(uint16_t), NULL);

static struct net_buf *chan_alloc_buf(struct bt_l2cap_chan *ch)
{
	return net_buf_alloc(&rx_sdu_pool, K_NO_WAIT);
}

/* What the disconnection by chan_shutdown_disconnect() returned */
static int shutdown_disconnect_err;

/* As an application may react to the shutdown */
static void chan_shutdown_disconnect(struct bt_l2cap_chan *ch, atomic_t *status)
{
	if (atomic_test_bit(status, BT_L2CAP_STATUS_SHUTDOWN)) {
		shutdown_disconnect_err = bt_l2cap_chan_disconnect(ch);
	}
}

/* A channel that cannot give the peer credits for the rest of an SDU is
 * disconnected, once also when its status callback disconnects it as well
 */
static ZTEST(host_l2cap_le_tx, test_credits_no_buf)
{
	static const struct bt_l2cap_chan_ops ops = {
		.alloc_buf = chan_alloc_buf,
		.recv = chan_recv,
		.status = chan_shutdown_disconnect,
	};
	static struct bt_l2cap_le_chan sdu_chan;
	/* Not a CID of the other tests */
	const uint16_t peer_cid = PEER_CID + 5U;
	struct net_buf *buf;

	chan_connect(&sdu_chan, &ops, peer_cid);

	/* No buffer is left for an LE Flow Control Credit packet */
	host_pdus_take();

	/* The first K-frame of an SDU that needs more */
	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	(void)net_buf_add(buf, sizeof(struct bt_hci_acl_hdr) + sizeof(struct bt_l2cap_hdr));
	net_buf_add_le16(buf, LE_MIN_MTU);
	(void)net_buf_add_mem(buf, DATA, strlen(DATA));
	recv_pdu(buf, sdu_chan.rx.cid);

	/* Until the Host gives up waiting for a buffer for the credits */
	(void)k_sleep(BT_L2CAP_RTX_TIMEOUT);

	/* The request sent first still holds the disconnect buffer */
	zassert_equal(shutdown_disconnect_err, -ENOMEM);
	zassert_equal(disconn_accept(), peer_cid);
	zassert_equal(sdu_chan.state, BT_L2CAP_DISCONNECTED);

	host_pdus_give();
	tx_none();
}

static K_SEM_DEFINE(released_sem, 0, 1);

static void chan_disconnected(struct bt_l2cap_chan *ch)
{
	k_sem_give(&released_sem);
}

/* A channel whose peer requests the disconnection is released also when the
 * Host has no buffer for the response
 */
static ZTEST(host_l2cap_le_tx, test_disconn_rsp_no_buf)
{
	static const struct bt_l2cap_chan_ops ops = {
		.recv = chan_recv,
		.disconnected = chan_disconnected,
	};
	static struct bt_l2cap_le_chan peer_chan;
	/* Not a CID of the other tests */
	const uint16_t peer_cid = PEER_CID + 6U;
	struct net_buf *buf;

	chan_connect(&peer_chan, &ops, peer_cid);

	host_pdus_take();

	buf = sig_cmd(BT_L2CAP_DISCONN_REQ, PEER_IDENT + 4U);
	net_buf_add_le16(buf, peer_chan.rx.cid);
	net_buf_add_le16(buf, peer_cid);
	recv_sig(buf);

	/* Until the Host gives up waiting for a buffer for the response */
	(void)k_sleep(BT_L2CAP_RTX_TIMEOUT);
	zassert_ok(k_sem_take(&released_sem, K_NO_WAIT), "The channel was not released");
	zassert_equal(peer_chan.state, BT_L2CAP_DISCONNECTED);

	host_pdus_give();
	tx_none();
}
