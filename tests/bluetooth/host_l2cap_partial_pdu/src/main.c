/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
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
#include "host/l2cap_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define GENERIC_RP_SIZE 64U
#define HANDLE          0x0001U

/* The controller takes a fragment of this size at a time, and one at most until it reports
 * it sent
 */
#define ACL_MAX_LEN 27U

/* Headers of an ACL packet from the controller, after its H:4 packet type */
#define ACL_HDR_OFFSET   1U
#define L2CAP_HDR_OFFSET (ACL_HDR_OFFSET + sizeof(struct bt_hci_acl_hdr))
#define SIG_HDR_OFFSET   (L2CAP_HDR_OFFSET + sizeof(struct bt_l2cap_hdr))

#define PEER_CID   0x0040U
#define PEER_IDENT 0x80U

/* l2cap_br_internal.h does not go along with l2cap_internal.h */
#define BR_CONN_REQ 0x02U
#define BR_CONN_RSP 0x03U
#define BR_CONF_REQ 0x04U
#define BR_CONF_RSP 0x05U
#define BR_INFO_REQ 0x0aU
#define BR_INFO_RSP 0x0bU

#define BR_INFO_FEAT_MASK     0x0002U
#define BR_FEAT_ENH_RET       BIT(3)
#define BR_OPT_RET_FC         0x04U
#define BR_OPT_RET_FC_LEN     9U
#define BR_TX_WINDOW          1U
#define BR_MAX_TRANSMIT       1U
#define BR_RET_TIMEOUT_MS     2000U
#define BR_MONITOR_TIMEOUT_MS 12000U

#define LE_PSM 0x0080U
#define BR_PSM 0x1001U

/* The size of a frame on the channels: more than a single fragment */
#define PEER_MPS     40U
#define PEER_MTU     100U
#define PEER_CREDITS 4U

/* Takes more than one frame of PEER_MPS */
#define SDU_LEN  60U
#define SDU_BYTE 0xa5U

/* How long to wait for a PDU that the Host is to send. It sends it without simulated time
 * passing, so this only bounds a failed wait.
 */
#define TX_TIMEOUT K_MSEC(100)

/* The Host waits for a buffer for up to the minimum RTX timeout */
#define BUF_WAIT K_SECONDS(3)

NET_BUF_POOL_FIXED_DEFINE(sdu_pool, 1, BT_L2CAP_SDU_CHAN_SEND_RESERVE + SDU_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
/* The PDUs that the Host sent and the test has not taken yet */
NET_BUF_POOL_FIXED_DEFINE(sent_pool, 3, 2U * PEER_MTU, 0, NULL);

/* The default pool of L2CAP PDUs, held by a test to have the Host run out of it */
static struct net_buf *tx_pool_held[CONFIG_BT_L2CAP_TX_BUF_COUNT];

static struct bt_conn *conn;
static uint16_t sig_cid;

/* What the Host sent, recombined the way a peer does it */
static K_FIFO_DEFINE(sent);
static struct net_buf *partial;
static const char *framing_error;

/* The fragments that the Host sent */
static atomic_t frags;

/* The controller reports the fragments sent unless the test holds them, as for a peer that
 * does not acknowledge at the link layer. A held fragment also stays with the driver.
 */
static atomic_t unreported;
static struct net_buf *held_frag;
static bool hold;

static K_SEM_DEFINE(disconnected_sem, 0, 1);

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
	/* LE and BR/EDR supported */
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
	rp->le_max_len = sys_cpu_to_le16(ACL_MAX_LEN);
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

/* The controller gets on with what it holds */
static void release(void)
{
	hold = false;
	net_buf_drop(&held_frag);
	zassert_true(k_work_submit(&completed_work) >= 0);
}

static void recombine(uint8_t pb, const struct net_buf *acl)
{
	size_t total;

	if (pb == BT_ACL_CONT) {
		if (partial == NULL) {
			framing_error = "Continuation without a start";
			return;
		}
	} else {
		if (pb != BT_ACL_START_NO_FLUSH) {
			framing_error = "Neither a continuation nor a non-flushable start";
		}

		if (partial != NULL) {
			framing_error = "Start in the middle of a PDU";
			net_buf_drop(&partial);
		}

		partial = net_buf_alloc(&sent_pool, K_NO_WAIT);
		zassert_not_null(partial);
	}

	if (acl->len > net_buf_tailroom(partial)) {
		framing_error = "More than the PDU in recombination";
		return;
	}

	(void)net_buf_add_mem(partial, acl->data, acl->len);

	total = sizeof(struct bt_l2cap_hdr) + sys_get_le16(partial->data);
	if (partial->len > total) {
		framing_error = "A PDU longer than its header says";
	} else if (partial->len == total) {
		k_fifo_put(&sent, net_buf_take(&partial));
	}
}

static void acl_handle(struct net_buf *acl)
{
	struct bt_hci_acl_hdr *hdr;
	uint16_t handle;

	hdr = net_buf_pull_mem(acl, sizeof(*hdr));
	handle = sys_le16_to_cpu(hdr->handle);
	zassert_equal(bt_acl_handle(handle), HANDLE);

	recombine(bt_acl_flags_pb(bt_acl_flags(handle)), acl);

	(void)atomic_inc(&frags);
	(void)atomic_inc(&unreported);

	if (hold) {
		held_frag = net_buf_ref(acl);
	} else {
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
	zassert_is_null(framing_error, "%s", framing_error);
	zassert_not_null(buf, "Nothing sent");

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	zassert_equal(sys_le16_to_cpu(hdr->cid), cid, "cid 0x%04x", sys_le16_to_cpu(hdr->cid));

	return buf;
}

/* The next PDU must be a signalling command with the given code. Returns the command with its
 * header pulled, and stores its identifier.
 */
static struct net_buf *tx_sig_cmd(uint8_t code, uint8_t *ident)
{
	struct bt_l2cap_sig_hdr *sig;
	struct net_buf *buf;

	buf = tx(sig_cid);

	sig = net_buf_pull_mem(buf, sizeof(*sig));
	zassert_equal(sig->code, code, "code 0x%02x", sig->code);
	*ident = sig->ident;

	return buf;
}

static uint8_t tx_sig(uint8_t code)
{
	uint8_t ident;

	net_buf_unref(tx_sig_cmd(code, &ident));

	return ident;
}

/* The Host has sent no more than the test took, in fragments that add up to whole PDUs */
static void expect_nothing_more(void)
{
	zassert_is_null(k_fifo_get(&sent, K_MSEC(1)), "Sent more");
	zassert_is_null(framing_error, "%s", framing_error);
	zassert_is_null(partial, "Only part of a PDU sent");
}

static void expect_sdu_buf_free(void)
{
	zassert_equal(atomic_get(&sdu_pool.avail_count), sdu_pool.buf_count, "SDU buffer not free");
}

/* The rest of a frame carries what the SDU has */
static void expect_sdu_bytes(const struct net_buf *buf)
{
	for (size_t i = 0U; i < buf->len; i++) {
		zassert_equal(buf->data[i], SDU_BYTE, "Byte %zu of %u is 0x%02x", i, buf->len,
			      buf->data[i]);
	}
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

/* Deliver the PDU and let the Host handle it, which takes no simulated time */
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
	recv_pdu(buf, sig_cid);
}

/* The peer takes the channel down */
static void recv_disconn_req(uint16_t host_cid)
{
	struct net_buf *buf;

	buf = sig_cmd(BT_L2CAP_DISCONN_REQ, PEER_IDENT);
	net_buf_add_le16(buf, host_cid);
	net_buf_add_le16(buf, PEER_CID);
	recv_sig(buf);
}

static uint32_t chan_disconnected_count;

static void chan_disconnected(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);

	chan_disconnected_count++;
}

static int chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	ARG_UNUSED(chan);
	ARG_UNUSED(buf);

	return 0;
}

static const struct bt_l2cap_chan_ops chan_ops = {
	.disconnected = chan_disconnected,
	.recv = chan_recv,
};

static struct net_buf *sdu_alloc(void)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&sdu_pool, K_NO_WAIT);
	zassert_not_null(buf);

	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
	(void)memset(net_buf_add(buf, SDU_LEN), SDU_BYTE, SDU_LEN);

	return buf;
}

/* From here on the controller holds on to what the Host sends */
static void hold_tx(void)
{
	(void)atomic_clear(&frags);
	hold = true;
}

/* A single fragment of what was sent has gone out */
static void expect_one_frag_out(void)
{
	(void)k_sleep(K_MSEC(1));
	zassert_equal(atomic_get(&frags), 1, "%ld fragments sent", (long)atomic_get(&frags));
}

/* The link of the new conn as on Connection Complete */
static void link_up(uint16_t sig)
{
	zassert_not_null(conn);
	conn->handle = HANDLE;
	bt_conn_set_state(conn, BT_CONN_CONNECTED);
	sig_cid = sig;
	bt_l2cap_connected(conn);
}

/* The controller reports the end of the link */
static void link_down(void)
{
	struct bt_hci_evt_disconn_complete *ev;
	struct net_buf *evt;

	evt = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(evt, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*ev));
	ev = net_buf_add(evt, sizeof(*ev));
	ev->status = BT_HCI_ERR_SUCCESS;
	ev->handle = sys_cpu_to_le16(HANDLE);
	ev->reason = BT_HCI_ERR_CONN_TIMEOUT;
	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), evt);

	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnected callback");

	/* What was not sent goes with the link */
	(void)atomic_clear(&unreported);
	net_buf_drop(&held_frag);
	net_buf_drop(&partial);
	hold = false;

	/* The Host lets go of its references after the callback, and of the one
	 * for the TX processor once that has a fragment again
	 */
	(void)k_sleep(K_MSEC(1));
	bt_conn_drop(&conn);
}

static void le_link_up(void)
{
	static const bt_addr_le_t addr = {
		.type = BT_ADDR_LE_PUBLIC,
		.a = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } },
	};

	conn = bt_conn_add_le(BT_ID_DEFAULT, &addr);
	link_up(BT_L2CAP_CID_LE_SIG);
}

/* Connect a credit-based channel with credits for all of an SDU */
static void le_chan_connect(struct bt_l2cap_le_chan *le_chan)
{
	struct net_buf *buf;
	uint8_t ident;

	(void)memset(le_chan, 0, sizeof(*le_chan));
	le_chan->chan.ops = &chan_ops;
	le_chan->rx.mtu = BT_L2CAP_SDU_RX_MTU;
	zassert_ok(bt_l2cap_chan_connect(conn, &le_chan->chan, LE_PSM));

	ident = tx_sig(BT_L2CAP_LE_CONN_REQ);
	buf = sig_cmd(BT_L2CAP_LE_CONN_RSP, ident);
	net_buf_add_le16(buf, PEER_CID);
	net_buf_add_le16(buf, PEER_MTU);
	net_buf_add_le16(buf, PEER_MPS);
	net_buf_add_le16(buf, PEER_CREDITS);
	net_buf_add_le16(buf, BT_L2CAP_LE_SUCCESS);
	recv_sig(buf);

	zassert_equal(le_chan->state, BT_L2CAP_CONNECTED);
}

/* A K-frame of an SDU goes out in part */
static void le_sdu_partly_sent(struct bt_l2cap_le_chan *le_chan)
{
	hold_tx();
	zassert_true(bt_l2cap_chan_send(&le_chan->chan, sdu_alloc()) >= 0);
	expect_one_frag_out();
}

/* The first K-frame of the SDU */
static void le_expect_first_k_frame(void)
{
	struct net_buf *buf;

	buf = tx(PEER_CID);
	zassert_equal(buf->len, PEER_MPS, "K-frame of %u", buf->len);
	zassert_equal(net_buf_pull_le16(buf), SDU_LEN);
	expect_sdu_bytes(buf);
	net_buf_unref(buf);
}

static uint8_t br_chan_mode;
static struct bt_l2cap_br_chan br_chan;

static int br_server_accept(struct bt_conn *accept_conn, struct bt_l2cap_server *server,
			    struct bt_l2cap_chan **chan)
{
	ARG_UNUSED(accept_conn);
	ARG_UNUSED(server);

	(void)memset(&br_chan, 0, sizeof(br_chan));
	br_chan.chan.ops = &chan_ops;
	br_chan.rx.mtu = PEER_MTU;
	br_chan.rx.mode = br_chan_mode;
	*chan = &br_chan.chan;

	return 0;
}

static void br_link_up(void)
{
	static const bt_addr_t addr = { .val = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 } };
	struct net_buf *buf;
	uint8_t ident;

	conn = bt_conn_add_br(&addr);
	link_up(BT_L2CAP_CID_BR_SIG);

	/* The Host asks on connection */
	ident = tx_sig(BR_INFO_REQ);
	buf = sig_cmd(BR_INFO_RSP, ident);
	net_buf_add_le16(buf, BR_INFO_FEAT_MASK);
	net_buf_add_le16(buf, 0U);
	net_buf_add_le32(buf, BR_FEAT_ENH_RET);
	recv_sig(buf);
}

/* The peer opens a channel in the mode given */
static void br_chan_connect(uint8_t mode)
{
	struct net_buf *buf;
	uint16_t host_cid;
	uint8_t ident;

	br_chan_mode = mode;

	buf = sig_cmd(BR_CONN_REQ, PEER_IDENT);
	net_buf_add_le16(buf, BR_PSM);
	net_buf_add_le16(buf, PEER_CID);
	recv_sig(buf);

	buf = tx_sig_cmd(BR_CONN_RSP, &ident);
	host_cid = net_buf_pull_le16(buf);
	net_buf_unref(buf);
	zassert_equal(br_chan.rx.cid, host_cid);

	ident = tx_sig(BR_CONF_REQ);
	buf = sig_cmd(BR_CONF_RSP, ident);
	net_buf_add_le16(buf, host_cid);
	net_buf_add_le16(buf, 0U);
	net_buf_add_le16(buf, 0U);
	recv_sig(buf);

	buf = sig_cmd(BR_CONF_REQ, PEER_IDENT + 1U);
	net_buf_add_le16(buf, host_cid);
	net_buf_add_le16(buf, 0U);
	if (mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		net_buf_add_u8(buf, BR_OPT_RET_FC);
		net_buf_add_u8(buf, BR_OPT_RET_FC_LEN);
		net_buf_add_u8(buf, mode);
		net_buf_add_u8(buf, BR_TX_WINDOW);
		net_buf_add_u8(buf, BR_MAX_TRANSMIT);
		net_buf_add_le16(buf, BR_RET_TIMEOUT_MS);
		net_buf_add_le16(buf, BR_MONITOR_TIMEOUT_MS);
		net_buf_add_le16(buf, PEER_MPS);
	}
	recv_sig(buf);

	zassert_equal(tx_sig(BR_CONF_RSP), PEER_IDENT + 1U);
	zassert_equal(br_chan.state, BT_L2CAP_CONNECTED);
	zassert_equal(br_chan.tx.mode, mode, "TX mode %u", br_chan.tx.mode);
}

static int br_tx_cb_err;

static void br_tx_cb(struct bt_conn *cb_conn, void *user_data, int err)
{
	ARG_UNUSED(cb_conn);
	ARG_UNUSED(user_data);

	br_tx_cb_err = err;
}

static void after(void *fixture)
{
	struct net_buf *buf;
	const char *error;

	ARG_UNUSED(fixture);

	ARRAY_FOR_EACH(tx_pool_held, i) {
		net_buf_drop(&tx_pool_held[i]);
	}

	if (conn != NULL) {
		link_down();
	}

	while ((buf = k_fifo_get(&sent, K_NO_WAIT)) != NULL) {
		net_buf_unref(buf);
	}

	net_buf_drop(&partial);
	error = framing_error;
	framing_error = NULL;
	chan_disconnected_count = 0U;
	br_tx_cb_err = -EINPROGRESS;

	zassert_is_null(error, "%s", error);
	zassert_equal(atomic_get(&sdu_pool.avail_count), sdu_pool.buf_count,
		      "SDU buffer left behind");
}

static void disconnected(struct bt_conn *disconnected_conn, uint8_t reason)
{
	ARG_UNUSED(disconnected_conn);
	ARG_UNUSED(reason);

	k_sem_give(&disconnected_sem);
}

static void *setup(void)
{
	static struct bt_conn_cb conn_cb = {
		.disconnected = disconnected,
	};
	static struct bt_l2cap_server br_server = {
		.psm = BR_PSM,
		.sec_level = BT_SECURITY_L1,
		.accept = br_server_accept,
	};

	br_tx_cb_err = -EINPROGRESS;

	zassert_ok(bt_enable(NULL));
	zassert_ok(bt_conn_cb_register(&conn_cb));
	zassert_ok(bt_l2cap_br_server_register(&br_server));

	return NULL;
}

ZTEST_SUITE(host_l2cap_partial_pdu, NULL, setup, NULL, after, NULL);

/* The peer takes a channel down in the middle of a K-frame: it gets the rest of that K-frame and
 * then the response, as a PDU of its own, and no later K-frame of the SDU.
 */
static ZTEST(host_l2cap_partial_pdu, test_le_peer_disconnects)
{
	static struct bt_l2cap_le_chan le_chan;

	le_link_up();
	le_chan_connect(&le_chan);
	le_sdu_partly_sent(&le_chan);

	recv_disconn_req(le_chan.rx.cid);
	zassert_equal(chan_disconnected_count, 1U);

	release();

	le_expect_first_k_frame();
	zassert_equal(tx_sig(BT_L2CAP_DISCONN_RSP), PEER_IDENT);
	expect_nothing_more();
	expect_sdu_buf_free();
}

/* Out of buffers to give credits, the Host shuts a channel down in the middle of a K-frame */
static ZTEST(host_l2cap_partial_pdu, test_le_shutdown)
{
	static struct bt_l2cap_le_chan le_chan;
	struct net_buf *buf;

	le_link_up();
	le_chan_connect(&le_chan);
	le_sdu_partly_sent(&le_chan);

	ARRAY_FOR_EACH(tx_pool_held, i) {
		tx_pool_held[i] = bt_l2cap_create_pdu_timeout(NULL, 0, K_NO_WAIT);
	}

	/* An SDU from the peer has the Host give credits */
	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	(void)net_buf_add(buf, sizeof(struct bt_hci_acl_hdr) + sizeof(struct bt_l2cap_hdr));
	net_buf_add_le16(buf, 1U);
	net_buf_add_u8(buf, SDU_BYTE);
	recv_pdu(buf, le_chan.rx.cid);

	(void)k_sleep(BUF_WAIT);
	zassert_true(atomic_test_bit(le_chan.chan.status, BT_L2CAP_STATUS_SHUTDOWN));

	ARRAY_FOR_EACH(tx_pool_held, i) {
		net_buf_drop(&tx_pool_held[i]);
	}

	release();

	le_expect_first_k_frame();

	zassert_ok(bt_l2cap_chan_disconnect(&le_chan.chan));
	(void)tx_sig(BT_L2CAP_DISCONN_REQ);
	expect_nothing_more();
	expect_sdu_buf_free();
}

/* The link goes down with the rest of a K-frame to send, which the next connection does not get */
static ZTEST(host_l2cap_partial_pdu, test_le_link_lost)
{
	static struct bt_l2cap_le_chan le_chan;

	le_link_up();
	le_chan_connect(&le_chan);
	le_sdu_partly_sent(&le_chan);

	recv_disconn_req(le_chan.rx.cid);
	link_down();

	expect_sdu_buf_free();

	le_link_up();
	le_chan_connect(&le_chan);
	expect_nothing_more();
}

/* Basic mode. The TX callback is not called, as what it reports to may be gone with the channel. */
static ZTEST(host_l2cap_partial_pdu, test_br_basic_peer_disconnects)
{
	struct net_buf *buf;

	br_link_up();
	br_chan_connect(BT_L2CAP_BR_LINK_MODE_BASIC);

	hold_tx();
	zassert_ok(bt_l2cap_br_chan_send_cb(&br_chan.chan, sdu_alloc(), br_tx_cb, NULL));
	expect_one_frag_out();

	recv_disconn_req(br_chan.rx.cid);
	zassert_equal(chan_disconnected_count, 1U);

	release();

	buf = tx(PEER_CID);
	zassert_equal(buf->len, SDU_LEN);
	expect_sdu_bytes(buf);
	net_buf_unref(buf);

	zassert_equal(tx_sig(BT_L2CAP_DISCONN_RSP), PEER_IDENT);
	expect_nothing_more();
	expect_sdu_buf_free();
	zassert_equal(br_tx_cb_err, -EINPROGRESS, "TX callback err %d", br_tx_cb_err);
}

/* Basic mode, with the link going down before the rest is sent */
static ZTEST(host_l2cap_partial_pdu, test_br_basic_link_lost)
{
	br_link_up();
	br_chan_connect(BT_L2CAP_BR_LINK_MODE_BASIC);

	hold_tx();
	zassert_ok(bt_l2cap_br_chan_send_cb(&br_chan.chan, sdu_alloc(), br_tx_cb, NULL));
	expect_one_frag_out();

	recv_disconn_req(br_chan.rx.cid);
	link_down();

	expect_sdu_buf_free();
	zassert_equal(br_tx_cb_err, -EINPROGRESS, "TX callback err %d", br_tx_cb_err);
}

/* Enhanced Retransmission mode, where the frame in transmission is apart from the SDU */
static ZTEST(host_l2cap_partial_pdu, test_br_eret_peer_disconnects)
{
	struct net_buf *buf;

	br_link_up();
	br_chan_connect(BT_L2CAP_BR_LINK_MODE_ERET);

	hold_tx();
	zassert_true(bt_l2cap_chan_send(&br_chan.chan, sdu_alloc()) >= 0);
	expect_one_frag_out();
	zassert_not_null(br_chan._pdu_buf);

	recv_disconn_req(br_chan.rx.cid);
	zassert_equal(chan_disconnected_count, 1U);

	release();

	buf = tx(PEER_CID);
	zassert_true(buf->len > ACL_MAX_LEN, "I-frame of %u", buf->len);
	net_buf_unref(buf);

	zassert_equal(tx_sig(BT_L2CAP_DISCONN_RSP), PEER_IDENT);
	expect_nothing_more();
	expect_sdu_buf_free();
	zassert_is_null(br_chan._pdu_buf);
	zassert_equal(br_chan._pdu_remaining, 0U);
}

/* Nor does the frame stay on the channel object when the link goes down */
static ZTEST(host_l2cap_partial_pdu, test_br_eret_link_lost)
{
	br_link_up();
	br_chan_connect(BT_L2CAP_BR_LINK_MODE_ERET);

	hold_tx();
	zassert_true(bt_l2cap_chan_send(&br_chan.chan, sdu_alloc()) >= 0);
	expect_one_frag_out();
	zassert_not_null(br_chan._pdu_buf);

	link_down();

	zassert_is_null(br_chan._pdu_buf);
	zassert_equal(br_chan._pdu_remaining, 0U);
}
