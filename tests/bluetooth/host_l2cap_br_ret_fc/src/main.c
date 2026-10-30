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
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/classic/l2cap_br_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define GENERIC_RP_SIZE 64U
#define ACL_MAX_LEN     310U
#define LE_ACL_MAX_LEN  27U
#define LE_ACL_MAX_NUM  3U
#define HANDLE          0x0001U
#define OTHER_HANDLE    0x0002U

/* Headers of an ACL packet from the controller, after its H:4 packet type */
#define ACL_HDR_OFFSET   1U
#define L2CAP_HDR_OFFSET (ACL_HDR_OFFSET + sizeof(struct bt_hci_acl_hdr))
#define SIG_HDR_OFFSET   (L2CAP_HDR_OFFSET + sizeof(struct bt_l2cap_hdr))

#define DATA         "data"
#define SDP_PSM      0x0001U
#define PEER_CID     0x0040U
#define PEER_IDENT   0x80U
#define PEER_MPS     48U
#define MIN_MTU      48U
#define FEAT_ENH_RET BIT(3)
/* x^16 + x^15 + x^2 + 1, bit-reversed */
#define FCS_POLY     0xa001U

/* Shorter than the retransmission timeout, so no poll is sent meanwhile */
#define TX_TIMEOUT K_MSEC(100)

/* An SDU per case and two for the window case, which a case that fails keeps */
NET_BUF_POOL_FIXED_DEFINE(sdu_pool, 6, BT_L2CAP_CHAN_SEND_RESERVE + sizeof(DATA),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
/* The PDUs that the Host sent and the test has not taken yet: at most a response per
 * signalling buffer, the Disconnection Request and the I-frame that a wrong Host sends
 * again
 */
NET_BUF_POOL_FIXED_DEFINE(sent_pool, CONFIG_BT_L2CAP_SIG_TX_BUF_COUNT + 2, ACL_MAX_LEN, 0, NULL);

static struct bt_conn *conn;

/* The L2CAP PDUs that the Host sent */
static K_FIFO_DEFINE(sent);

/* The controller has one ACL buffer, which it reports free again unless the
 * test holds it, as for a peer that does not acknowledge at the baseband.
 * Counted per connection handle.
 */
static atomic_t unreported[2];
static bool hold;

/* The buffers of the Host that the driver keeps, as one sending them later does */
static K_FIFO_DEFINE(kept);
static bool keep;

static K_SEM_DEFINE(echo_sem, 0, 1);

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
	atomic_val_t count[ARRAY_SIZE(unreported)];
	struct bt_hci_evt_num_completed_packets *ev;
	struct bt_hci_handle_count *hc;
	uint8_t num_handles = 0U;
	struct net_buf *evt;

	ARG_UNUSED(work);

	ARRAY_FOR_EACH(unreported, i) {
		count[i] = atomic_clear(&unreported[i]);
		if (count[i] != 0) {
			num_handles++;
		}
	}

	if (num_handles == 0U) {
		return;
	}

	evt = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
	evt_create(evt, BT_HCI_EVT_NUM_COMPLETED_PACKETS,
		   sizeof(*ev) + (num_handles * sizeof(*hc)));
	ev = net_buf_add(evt, sizeof(*ev));
	ev->num_handles = num_handles;

	ARRAY_FOR_EACH(unreported, i) {
		if (count[i] != 0) {
			hc = net_buf_add(evt, sizeof(*hc));
			hc->handle = sys_cpu_to_le16(HANDLE + i);
			hc->count = sys_cpu_to_le16((uint16_t)count[i]);
		}
	}

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
	uint16_t handle;

	/* Every PDU fits a single ACL packet */
	hdr = net_buf_pull_mem(acl, sizeof(*hdr));
	handle = bt_acl_handle(sys_le16_to_cpu(hdr->handle));
	zassert_true((handle == HANDLE) || (handle == OTHER_HANDLE), "handle %u", handle);

	pdu = net_buf_alloc(&sent_pool, K_NO_WAIT);
	zassert_not_null(pdu);
	(void)net_buf_add_mem(pdu, acl->data, acl->len);
	k_fifo_put(&sent, pdu);

	(void)atomic_inc(&unreported[handle - HANDLE]);
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

		if (keep) {
			k_fifo_put(&kept, buf);
			return 0;
		}
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

static void unkeep(void)
{
	struct net_buf *buf;

	keep = false;

	while ((buf = k_fifo_get(&kept, K_NO_WAIT)) != NULL) {
		net_buf_unref(buf);
	}
}

/* The next L2CAP PDU that the Host sent */
static struct net_buf *tx(void)
{
	struct net_buf *buf;

	buf = k_fifo_get(&sent, TX_TIMEOUT);
	zassert_not_null(buf, "Nothing sent");

	return buf;
}

/* The next PDU must be a signalling command with the given code. Copies the
 * start of its data and returns its identifier.
 */
static uint8_t tx_sig(uint8_t code, void *data, size_t len)
{
	struct bt_l2cap_sig_hdr *sig;
	struct bt_l2cap_hdr *hdr;
	struct net_buf *buf;
	uint8_t ident;

	buf = tx();

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	zassert_equal(sys_le16_to_cpu(hdr->cid), BT_L2CAP_CID_BR_SIG, "cid 0x%04x",
		      sys_le16_to_cpu(hdr->cid));

	sig = net_buf_pull_mem(buf, sizeof(*sig));
	zassert_equal(sig->code, code, "code 0x%02x", sig->code);
	ident = sig->ident;

	if (len > 0U) {
		zassert_true(buf->len >= len);
		(void)memcpy(data, buf->data, len);
	}

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
static void recv_frame(struct net_buf *buf, uint16_t handle)
{
	struct bt_hci_acl_hdr *acl = (void *)&buf->data[ACL_HDR_OFFSET];

	acl->handle = sys_cpu_to_le16(bt_acl_handle_pack(handle, BT_ACL_START));
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

static void recv_sig_on(struct net_buf *buf, uint16_t handle)
{
	struct bt_l2cap_sig_hdr *sig = (void *)&buf->data[SIG_HDR_OFFSET];
	struct bt_l2cap_hdr *hdr = (void *)&buf->data[L2CAP_HDR_OFFSET];

	sig->len = sys_cpu_to_le16(buf->len - SIG_HDR_OFFSET - sizeof(*sig));
	hdr->len = sys_cpu_to_le16(buf->len - SIG_HDR_OFFSET);
	hdr->cid = sys_cpu_to_le16(BT_L2CAP_CID_BR_SIG);

	recv_frame(buf, handle);
}

static void recv_sig(struct net_buf *buf)
{
	recv_sig_on(buf, HANDLE);
}

static void recv_s_frame(struct bt_l2cap_br_chan *chan, uint8_t s, uint8_t req_seq)
{
	struct net_buf *buf;
	uint16_t fcs;

	buf = rx_frame();
	net_buf_add_le16(buf, BT_L2CAP_ENH_CONTROL_SIZE + BT_L2CAP_FCS_SIZE);
	net_buf_add_le16(buf, chan->rx.cid);
	net_buf_add_le16(buf, BT_L2CAP_S_FRAME_ENH_CONTROL_SET(s, 0U, 0U, req_seq));

	fcs = crc16_reflect(FCS_POLY, 0U, &buf->data[L2CAP_HDR_OFFSET],
			    buf->len - L2CAP_HDR_OFFSET);
	net_buf_add_le16(buf, fcs);

	recv_frame(buf, HANDLE);
}

/* The responses to Information Requests on a link take every signalling buffer */
static void take_sig_bufs(uint16_t handle)
{
	struct net_buf *buf;

	for (uint8_t i = 0U; i < CONFIG_BT_L2CAP_SIG_TX_BUF_COUNT; i++) {
		buf = sig_cmd(BT_L2CAP_INFO_REQ, PEER_IDENT);
		net_buf_add_le16(buf, BT_L2CAP_INFO_FEAT_MASK);
		recv_sig_on(buf, handle);
	}
}

static void expect_info_rsps(void)
{
	for (uint8_t i = 0U; i < CONFIG_BT_L2CAP_SIG_TX_BUF_COUNT; i++) {
		zassert_equal(tx_sig(BT_L2CAP_INFO_RSP, NULL, 0U), PEER_IDENT);
	}
}

static int chan_recv(struct bt_l2cap_chan *ch, struct net_buf *buf)
{
	return 0;
}

static const struct bt_l2cap_chan_ops chan_ops = {
	.recv = chan_recv,
};

/* Connect a channel in Enhanced Retransmission Mode */
static void chan_connect(struct bt_l2cap_br_chan *chan, uint16_t peer_cid, uint8_t window,
			 uint8_t max_transmit)
{
	struct net_buf *buf;
	uint8_t ident;

	chan->chan.ops = &chan_ops;
	chan->rx.mtu = MIN_MTU;
	chan->rx.mode = BT_L2CAP_BR_LINK_MODE_ERET;
	chan->rx.fcs = BT_L2CAP_BR_FCS_16BIT;
	chan->rx.max_transmit = 1U;
	zassert_ok(bt_l2cap_chan_connect(conn, &chan->chan, SDP_PSM));

	ident = tx_sig(BT_L2CAP_CONN_REQ, NULL, 0U);
	buf = sig_cmd(BT_L2CAP_CONN_RSP, ident);
	net_buf_add_le16(buf, peer_cid);
	net_buf_add_le16(buf, chan->rx.cid);
	net_buf_add_le16(buf, BT_L2CAP_BR_SUCCESS);
	net_buf_add_le16(buf, 0U);
	recv_sig(buf);

	ident = tx_sig(BT_L2CAP_CONF_REQ, NULL, 0U);

	buf = sig_cmd(BT_L2CAP_CONF_REQ, PEER_IDENT);
	net_buf_add_le16(buf, chan->rx.cid);
	net_buf_add_le16(buf, 0U);
	net_buf_add_u8(buf, BT_L2CAP_CONF_OPT_RET_FC);
	net_buf_add_u8(buf, sizeof(struct bt_l2cap_conf_opt_ret_fc));
	net_buf_add_u8(buf, BT_L2CAP_BR_LINK_MODE_ERET);
	/* TxWindow size, MaxTransmit, timeouts left to the receiver, MPS */
	net_buf_add_u8(buf, window);
	net_buf_add_u8(buf, max_transmit);
	net_buf_add_le16(buf, 0U);
	net_buf_add_le16(buf, 0U);
	net_buf_add_le16(buf, PEER_MPS);
	recv_sig(buf);

	zassert_equal(tx_sig(BT_L2CAP_CONF_RSP, NULL, 0U), PEER_IDENT);

	buf = sig_cmd(BT_L2CAP_CONF_RSP, ident);
	net_buf_add_le16(buf, chan->rx.cid);
	net_buf_add_le16(buf, 0U);
	net_buf_add_le16(buf, BT_L2CAP_CONF_SUCCESS);
	recv_sig(buf);

	zassert_equal(chan->state, BT_L2CAP_CONNECTED);
	zassert_equal(chan->tx.mode, BT_L2CAP_BR_LINK_MODE_ERET);
	zassert_equal(chan->tx.max_transmit, max_transmit);
	zassert_equal(chan->tx.max_window, window);
	zassert_equal(chan->rx.fcs, BT_L2CAP_BR_FCS_16BIT);
}

static void send_sdu(struct bt_l2cap_br_chan *chan)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&sdu_pool, K_NO_WAIT);
	zassert_not_null(buf);
	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
	(void)net_buf_add_mem(buf, DATA, strlen(DATA));

	zassert_ok(bt_l2cap_chan_send(&chan->chan, buf));
}

/* Send an I-frame, which the peer asks for again: a second transmission would
 * exceed a MaxTransmit of 1. The controller reports the I-frame complete only on
 * release(), so that the Host decides that the channel is lost after what the
 * test sets up meanwhile.
 */
static void lose_chan(struct bt_l2cap_br_chan *chan, uint16_t peer_cid)
{
	struct bt_l2cap_hdr *hdr;
	struct net_buf *buf;

	hold = true;
	send_sdu(chan);

	buf = tx();
	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	zassert_equal(sys_le16_to_cpu(hdr->cid), peer_cid);
	net_buf_unref(buf);

	recv_s_frame(chan, BT_L2CAP_CONTROL_S_SREJ, 0U);
}

static void expect_disconn_req(struct bt_l2cap_br_chan *chan, uint16_t peer_cid)
{
	struct bt_l2cap_disconn_req req;

	zassert_equal(tx_sig(BT_L2CAP_DISCONN_REQ, &req, sizeof(req)), chan->ident);
	zassert_equal(sys_le16_to_cpu(req.dcid), peer_cid);
	zassert_equal(sys_le16_to_cpu(req.scid), chan->rx.cid);
	zassert_equal(chan->state, BT_L2CAP_DISCONNECTING);
}

/* The peer agrees, which removes the channel */
static void recv_disconn_rsp(struct bt_l2cap_br_chan *chan, uint16_t peer_cid)
{
	struct net_buf *buf;

	buf = sig_cmd(BT_L2CAP_DISCONN_RSP, chan->ident);
	net_buf_add_le16(buf, peer_cid);
	net_buf_add_le16(buf, chan->rx.cid);
	recv_sig(buf);
}

static void echo_req(struct bt_conn *c, uint8_t identifier, struct net_buf *buf)
{
	k_sem_give(&echo_sem);
}

static struct bt_l2cap_br_echo_cb echo_cb = {
	.req = echo_req,
};

static void *setup(void)
{
	static const bt_addr_t addr = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };
	struct net_buf *buf;
	uint8_t ident;

	zassert_ok(bt_enable(NULL));
	zassert_ok(bt_l2cap_br_echo_cb_register(&echo_cb));

	/* As on Connection Complete */
	conn = bt_conn_add_br(&addr);
	zassert_not_null(conn);
	conn->handle = HANDLE;
	bt_conn_set_state(conn, BT_CONN_CONNECTED);
	bt_l2cap_br_connected(conn);

	ident = tx_sig(BT_L2CAP_INFO_REQ, NULL, 0U);
	buf = sig_cmd(BT_L2CAP_INFO_RSP, ident);
	net_buf_add_le16(buf, BT_L2CAP_INFO_FEAT_MASK);
	net_buf_add_le16(buf, BT_L2CAP_INFO_SUCCESS);
	net_buf_add_le32(buf, FEAT_ENH_RET);
	recv_sig(buf);

	return NULL;
}

ZTEST_SUITE(l2cap_br_ret_fc, NULL, setup, NULL, NULL, NULL);

/* The fallback of a signalling buffer allocation is empty */
static void take_acl_bufs(sys_slist_t *taken)
{
	struct net_buf *buf;

	sys_slist_init(taken);

	while ((buf = bt_conn_create_pdu_timeout(NULL, 0U, K_NO_WAIT)) != NULL) {
		net_buf_slist_put(taken, buf);
	}
}

static void give_acl_bufs(sys_slist_t *taken)
{
	struct net_buf *buf;

	while ((buf = net_buf_slist_get(taken)) != NULL) {
		net_buf_unref(buf);
	}
}

/* A channel lost to MaxTransmit is disconnected, although signalling responses
 * queued behind it hold every signalling buffer: the TX processor sends them first.
 */
static ZTEST(l2cap_br_ret_fc, test_max_transmit)
{
	static struct bt_l2cap_br_chan chan;

	chan_connect(&chan, PEER_CID, 1U, 1U);
	lose_chan(&chan, PEER_CID);
	take_sig_bufs(HANDLE);

	release();

	/* Not the I-frame again */
	expect_info_rsps();

	expect_disconn_req(&chan, PEER_CID);
	recv_disconn_rsp(&chan, PEER_CID);
}

/* The Disconnection Request of a lost channel waits for no buffer, neither on
 * the TX processor nor on the Bluetooth work queue: it follows once one is free.
 */
static ZTEST(l2cap_br_ret_fc, test_max_transmit_no_buf)
{
	static struct bt_l2cap_br_chan chan;
	struct net_buf *buf;
	sys_slist_t taken;

	chan_connect(&chan, PEER_CID + 1U, 1U, 1U);
	lose_chan(&chan, PEER_CID + 1U);
	take_sig_bufs(HANDLE);
	take_acl_bufs(&taken);

	/* The driver keeps the signalling buffers of the responses once sent */
	keep = true;
	release();

	expect_info_rsps();

	/* The Bluetooth work queue still runs */
	buf = sig_cmd(BT_L2CAP_ECHO_REQ, PEER_IDENT);
	recv_sig(buf);
	zassert_ok(k_sem_take(&echo_sem, TX_TIMEOUT));

	zassert_is_null(k_fifo_get(&sent, TX_TIMEOUT));
	zassert_equal(chan.state, BT_L2CAP_CONNECTED);

	unkeep();
	expect_disconn_req(&chan, PEER_CID + 1U);
	recv_disconn_rsp(&chan, PEER_CID + 1U);

	give_acl_bufs(&taken);
}

/* Nothing more is sent on a lost channel, although its window has room for a
 * pending SDU.
 */
static ZTEST(l2cap_br_ret_fc, test_max_transmit_stops_tx)
{
	static struct bt_l2cap_br_chan chan;

	chan_connect(&chan, PEER_CID + 2U, 2U, 1U);
	lose_chan(&chan, PEER_CID + 2U);
	send_sdu(&chan);
	take_sig_bufs(HANDLE);

	release();

	expect_info_rsps();
	expect_disconn_req(&chan, PEER_CID + 2U);
	zassert_is_null(k_fifo_get(&sent, TX_TIMEOUT));

	recv_disconn_rsp(&chan, PEER_CID + 2U);
}

/* The largest MaxTransmit allows that many transmissions, and no more */
static ZTEST(l2cap_br_ret_fc, test_max_transmit_255)
{
	static struct bt_l2cap_br_chan chan;
	struct bt_l2cap_hdr *hdr;
	struct net_buf *buf;

	chan_connect(&chan, PEER_CID + 3U, 1U, UINT8_MAX);
	send_sdu(&chan);

	for (uint16_t i = 0U; i < UINT8_MAX; i++) {
		buf = tx();
		hdr = net_buf_pull_mem(buf, sizeof(*hdr));
		zassert_equal(sys_le16_to_cpu(hdr->cid), PEER_CID + 3U, "transmission %u", i);
		net_buf_unref(buf);

		recv_s_frame(&chan, BT_L2CAP_CONTROL_S_SREJ, 0U);
	}

	expect_disconn_req(&chan, PEER_CID + 3U);
	recv_disconn_rsp(&chan, PEER_CID + 3U);
}

/* A lost channel does not hold up another link, whose queued signalling responses
 * hold every signalling buffer that the Disconnection Request needs.
 */
static ZTEST(l2cap_br_ret_fc, test_max_transmit_other_link)
{
	static const bt_addr_t addr = { .val = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 } };
	static struct bt_l2cap_br_chan chan;
	struct bt_conn *other;
	struct net_buf *buf;
	sys_slist_t taken;
	uint8_t ident;

	if (CONFIG_BT_MAX_CONN < 2) {
		ztest_test_skip();
	}

	other = bt_conn_add_br(&addr);
	zassert_not_null(other);
	other->handle = OTHER_HANDLE;
	bt_conn_set_state(other, BT_CONN_CONNECTED);
	bt_l2cap_br_connected(other);

	ident = tx_sig(BT_L2CAP_INFO_REQ, NULL, 0U);
	buf = sig_cmd(BT_L2CAP_INFO_RSP, ident);
	net_buf_add_le16(buf, BT_L2CAP_INFO_FEAT_MASK);
	net_buf_add_le16(buf, BT_L2CAP_INFO_SUCCESS);
	net_buf_add_le32(buf, 0U);
	recv_sig_on(buf, OTHER_HANDLE);

	chan_connect(&chan, PEER_CID + 4U, 1U, 1U);
	lose_chan(&chan, PEER_CID + 4U);
	take_sig_bufs(OTHER_HANDLE);
	take_acl_bufs(&taken);

	release();

	expect_info_rsps();
	expect_disconn_req(&chan, PEER_CID + 4U);
	recv_disconn_rsp(&chan, PEER_CID + 4U);

	give_acl_bufs(&taken);
}
