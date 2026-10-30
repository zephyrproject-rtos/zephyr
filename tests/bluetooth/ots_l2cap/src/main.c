/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

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

#include <zephyr/bluetooth/services/ots.h>

#include "host/att_internal.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"
#include "services/ots/ots_client_internal.h"
#include "services/ots/ots_l2cap_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define GENERIC_RP_SIZE 64U
#define LE_ACL_MAX_LEN  64U
#define HANDLE          0x0001U

/* Headers of an ACL packet from the controller, after its H:4 packet type */
#define ACL_HDR_OFFSET    1U
#define L2CAP_HDR_OFFSET  (ACL_HDR_OFFSET + sizeof(struct bt_hci_acl_hdr))
#define L2CAP_DATA_OFFSET (L2CAP_HDR_OFFSET + sizeof(struct bt_l2cap_hdr))

#define OTS_PSM    0x0025U
#define PEER_CID   0x0040U
#define PEER_IDENT 0x80U
/* The smallest MTU of an LE credit-based channel. Each SDU that the Host sends
 * is a single K-frame.
 */
#define PEER_MTU   23U
#define PEER_MPS   (BT_L2CAP_SDU_HDR_SIZE + PEER_MTU)

/* The SDUs that the Host receives take two PDUs */
BUILD_ASSERT(CONFIG_BT_OTS_L2CAP_CHAN_RX_MTU > BT_L2CAP_SDU_RX_MTU);

/* How long to wait for a PDU that the Host is to send. It sends it without
 * simulated time passing, so this only bounds a failed wait.
 */
#define TX_TIMEOUT K_MSEC(100)

/* The PDUs that the Host sent and the test has not taken yet. At most three
 * wait at once: in the client read case, the ATT request and the responses to
 * the peer's disconnection and connection requests.
 */
NET_BUF_POOL_FIXED_DEFINE(sent_pool, 3, LE_ACL_MAX_LEN, 0, NULL);

/* A channel context, as an OTS instance or an OTS client instance registers it */
struct test_ctx {
	struct bt_gatt_ots_l2cap l2cap;
	/* The SDU data of its channel, both ways, which setup() fills */
	uint8_t data[MAX(CONFIG_BT_OTS_L2CAP_CHAN_RX_MTU, 2U * PEER_MTU)];
	/* The last SDU that it received */
	uint8_t rx[CONFIG_BT_OTS_L2CAP_CHAN_RX_MTU];
	size_t rx_len;
	size_t tx_done_count;
	size_t closed_count;
};

/* As many as the OTS and OTS client instances of the configuration */
static struct test_ctx ctxs[CONFIG_BT_OTS_MAX_INST_CNT + OTS_CLIENT_INST_COUNT];

/* The cases use one channel without credits and one with */
BUILD_ASSERT(ARRAY_SIZE(ctxs) == 2U);

/* The ACL connection the peer established */
static struct bt_conn *conn;

/* The PDUs that the Host sent */
static K_FIFO_DEFINE(sent);

/* The controller has one ACL buffer, which it reports free again unless the
 * test holds it, as for a peer that does not acknowledge at the link layer.
 */
static atomic_t unreported;
static bool hold;

/* Set to keep the next ACL packet after reporting it, as a driver that releases
 * a sent buffer only later. The packet holds the buffer of the SDU in it.
 */
static struct net_buf *kept;
static bool keep;

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

	if (keep) {
		keep = false;
		kept = net_buf_ref(acl);
	}

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

/* The next PDU must be a signalling command with the given code. Returns it with
 * its header pulled.
 */
static struct net_buf *tx_sig(uint8_t code, uint8_t *ident)
{
	struct bt_l2cap_sig_hdr *sig;
	struct net_buf *buf;

	buf = tx(BT_L2CAP_CID_LE_SIG);

	sig = net_buf_pull_mem(buf, sizeof(*sig));
	zassert_equal(sig->code, code, "code 0x%02x", sig->code);
	*ident = sig->ident;

	return buf;
}

/* The next PDU must give the peer credits for the channel */
static void tx_credits(const struct bt_l2cap_le_chan *ch, uint16_t credits)
{
	struct bt_l2cap_le_credits *ev;
	struct net_buf *buf;
	uint8_t ident;

	buf = tx_sig(BT_L2CAP_LE_CREDITS, &ident);
	ev = net_buf_pull_mem(buf, sizeof(*ev));
	zassert_equal(sys_le16_to_cpu(ev->cid), ch->rx.cid);
	zassert_equal(sys_le16_to_cpu(ev->credits), credits);

	net_buf_unref(buf);
}

/* The next PDU must be a K-frame with a whole SDU on the channel of ctxs[i] */
static void tx_sdu(uint8_t i, const uint8_t *data)
{
	struct net_buf *buf;

	buf = tx(PEER_CID + i);
	zassert_equal(net_buf_pull_le16(buf), PEER_MTU);
	zassert_equal(buf->len, PEER_MTU);
	zassert_mem_equal(buf->data, data, PEER_MTU);

	net_buf_unref(buf);
}

/* An ACL packet from the peer, for one L2CAP PDU */
static struct net_buf *rx_pdu(void)
{
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	(void)net_buf_add(buf, sizeof(struct bt_hci_acl_hdr) + sizeof(struct bt_l2cap_hdr));

	return buf;
}

/* Deliver the PDU and let the Host handle it, which takes no simulated time */
static void recv_pdu(struct net_buf *buf, uint16_t cid)
{
	struct bt_hci_acl_hdr *acl = (void *)&buf->data[ACL_HDR_OFFSET];
	struct bt_l2cap_hdr *hdr = (void *)&buf->data[L2CAP_HDR_OFFSET];

	hdr->len = sys_cpu_to_le16(buf->len - L2CAP_DATA_OFFSET);
	hdr->cid = sys_cpu_to_le16(cid);
	acl->handle = sys_cpu_to_le16(bt_acl_handle_pack(HANDLE, BT_ACL_START));
	acl->len = sys_cpu_to_le16(buf->len - L2CAP_HDR_OFFSET);

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
	(void)k_sleep(K_MSEC(1));
}

static struct net_buf *sig_cmd(uint8_t code, uint8_t ident)
{
	struct bt_l2cap_sig_hdr *sig;
	struct net_buf *buf;

	buf = rx_pdu();

	sig = net_buf_add(buf, sizeof(*sig));
	sig->code = code;
	sig->ident = ident;

	return buf;
}

static void recv_sig(struct net_buf *buf)
{
	struct bt_l2cap_sig_hdr *sig = (void *)&buf->data[L2CAP_DATA_OFFSET];

	sig->len = sys_cpu_to_le16(buf->len - L2CAP_DATA_OFFSET - sizeof(*sig));
	recv_pdu(buf, BT_L2CAP_CID_LE_SIG);
}

/* The peer gives credits for the channel of ctxs[i] */
static void recv_credits(uint8_t i, uint16_t credits)
{
	struct net_buf *buf;

	buf = sig_cmd(BT_L2CAP_LE_CREDITS, PEER_IDENT + ARRAY_SIZE(ctxs));
	net_buf_add_le16(buf, PEER_CID + i);
	net_buf_add_le16(buf, credits);
	recv_sig(buf);
}

/* The next PDU must disconnect the channel between the peer's dcid and the
 * Host's scid, which the peer confirms
 */
static void tx_disconn(uint16_t dcid, uint16_t scid)
{
	struct bt_l2cap_disconn_req *req;
	struct net_buf *buf;
	uint8_t ident;

	buf = tx_sig(BT_L2CAP_DISCONN_REQ, &ident);
	req = net_buf_pull_mem(buf, sizeof(*req));
	zassert_equal(sys_le16_to_cpu(req->dcid), dcid);
	zassert_equal(sys_le16_to_cpu(req->scid), scid);
	net_buf_unref(buf);

	buf = sig_cmd(BT_L2CAP_DISCONN_RSP, ident);
	net_buf_add_le16(buf, dcid);
	net_buf_add_le16(buf, scid);
	recv_sig(buf);
}

/* Both buffers taken: one by an SDU on the first channel, which has no credits,
 * and one by the SDU in the ACL packet that the driver keeps
 */
static void take_tx_buffers(size_t len1)
{
	zassert_ok(bt_gatt_ots_l2cap_send(&ctxs[0].l2cap, ctxs[0].data, PEER_MTU));

	recv_credits(1U, 1U);
	keep = true;
	zassert_ok(bt_gatt_ots_l2cap_send(&ctxs[1].l2cap, ctxs[1].data, len1));
	tx_sdu(1U, ctxs[1].data);

	/* Let the Host handle the completed packet */
	(void)k_sleep(K_MSEC(1));
}

/* The SDU waiting on the first channel goes out on a credit, freeing its buffer */
static void send_waiting_sdu(void)
{
	recv_credits(0U, 1U);
	tx_sdu(0U, ctxs[0].data);
	(void)k_sleep(K_MSEC(1));
}

/* The peer sends the first or the second PDU of an SDU of the channel's MTU */
static void recv_seg(uint8_t i, bool is_first)
{
	const struct bt_l2cap_le_chan *ch = &ctxs[i].l2cap.ot_chan;
	const size_t first_len = ch->rx.mps - BT_L2CAP_SDU_HDR_SIZE;
	const size_t end = is_first ? first_len : ch->rx.mtu;
	const size_t start = is_first ? 0U : first_len;
	struct net_buf *buf;

	buf = rx_pdu();

	if (is_first) {
		net_buf_add_le16(buf, ch->rx.mtu);
	}

	(void)net_buf_add_mem(buf, &ctxs[i].data[start], end - start);

	recv_pdu(buf, ch->rx.cid);
}

static ssize_t rx_done(struct bt_gatt_ots_l2cap *l2cap, struct bt_conn *link, struct net_buf *buf)
{
	struct test_ctx *ctx = CONTAINER_OF(l2cap, struct test_ctx, l2cap);

	ARG_UNUSED(link);

	ctx->rx_len = buf->len;
	(void)memcpy(ctx->rx, buf->data, MIN(buf->len, sizeof(ctx->rx)));

	return 0;
}

static void tx_done(struct bt_gatt_ots_l2cap *l2cap, struct bt_conn *link)
{
	struct test_ctx *ctx = CONTAINER_OF(l2cap, struct test_ctx, l2cap);

	ARG_UNUSED(link);

	ctx->tx_done_count++;
}

static void closed(struct bt_gatt_ots_l2cap *l2cap, struct bt_conn *link)
{
	struct test_ctx *ctx = CONTAINER_OF(l2cap, struct test_ctx, l2cap);

	ARG_UNUSED(link);

	ctx->closed_count++;
}

/* The peer connects a channel to the OTS PSM, which the Host puts on ctxs[i] */
static void connect_chan(uint8_t i, uint16_t credits)
{
	struct bt_l2cap_le_conn_rsp *rsp;
	struct net_buf *buf;
	uint8_t ident;

	buf = sig_cmd(BT_L2CAP_LE_CONN_REQ, PEER_IDENT + i);
	net_buf_add_le16(buf, OTS_PSM);
	net_buf_add_le16(buf, PEER_CID + i);
	net_buf_add_le16(buf, PEER_MTU);
	net_buf_add_le16(buf, PEER_MPS);
	net_buf_add_le16(buf, credits);
	recv_sig(buf);

	buf = tx_sig(BT_L2CAP_LE_CONN_RSP, &ident);
	zassert_equal(ident, PEER_IDENT + i);

	rsp = net_buf_pull_mem(buf, sizeof(*rsp));
	zassert_equal(sys_le16_to_cpu(rsp->result), BT_L2CAP_LE_SUCCESS);
	zassert_equal(sys_le16_to_cpu(rsp->dcid), ctxs[i].l2cap.ot_chan.rx.cid);

	net_buf_unref(buf);
}

#if defined(CONFIG_BT_OTS_CLIENT)
/* The next signalling PDU with the given code, draining any other PDUs the Host
 * sends meanwhile (the ATT write of the OACP command, an earlier response). The
 * signalling header is pulled; *ident is the identifier.
 */
static struct net_buf *wait_sig(uint8_t code, uint8_t *ident)
{
	while (true) {
		struct bt_l2cap_sig_hdr *sig;
		struct bt_l2cap_hdr *hdr;
		struct net_buf *buf;

		buf = k_fifo_get(&sent, TX_TIMEOUT);
		zassert_not_null(buf, "Expected signalling code 0x%02x", code);

		hdr = net_buf_pull_mem(buf, sizeof(*hdr));
		if (sys_le16_to_cpu(hdr->cid) == BT_L2CAP_CID_LE_SIG) {
			sig = net_buf_pull_mem(buf, sizeof(*sig));
			if (sig->code == code) {
				*ident = sig->ident;
				return buf;
			}
		}

		net_buf_unref(buf);
	}
}

/* Accept, as the peer, the channel the Host connects as the OTS client. The
 * response advertises the peer's own receive parameters, which the read does
 * not use.
 */
static void send_conn_rsp(uint8_t ident, uint16_t dcid)
{
	struct net_buf *buf;

	buf = sig_cmd(BT_L2CAP_LE_CONN_RSP, ident);
	net_buf_add_le16(buf, dcid);
	net_buf_add_le16(buf, PEER_MTU);
	net_buf_add_le16(buf, PEER_MPS);
	net_buf_add_le16(buf, 0U);
	net_buf_add_le16(buf, BT_L2CAP_LE_SUCCESS);
	recv_sig(buf);
}

/* The peer answers the ATT Write Request of the OACP command, which leaves the
 * OTS client free for its next procedure
 */
static void recv_att_write_rsp(void)
{
	struct net_buf *buf;

	buf = rx_pdu();
	net_buf_add_u8(buf, BT_ATT_OP_WRITE_RSP);
	recv_pdu(buf, BT_L2CAP_CID_ATT);
}
#endif /* CONFIG_BT_OTS_CLIENT */

/* Two channel contexts, each connected by the peer. The first channel gets no
 * credits from the peer.
 */
static void *setup(void)
{
	static const bt_addr_le_t addr = {
		.type = BT_ADDR_LE_PUBLIC,
		.a = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } },
	};

	/* Each byte holds the channel index in its top bit and its offset in the
	 * others, so a byte that reaches the wrong channel or offset shows
	 */
	ARRAY_FOR_EACH(ctxs, i) {
		ARRAY_FOR_EACH(ctxs[i].data, j) {
			ctxs[i].data[j] = (uint8_t)((i << 7) | (j & BIT_MASK(7)));
		}
	}

	zassert_ok(bt_enable(NULL));

	ARRAY_FOR_EACH_PTR(ctxs, ctx) {
		ctx->l2cap.rx_done = rx_done;
		ctx->l2cap.tx_done = tx_done;
		ctx->l2cap.closed = closed;
		zassert_ok(bt_gatt_ots_l2cap_register(&ctx->l2cap));
	}

	/* As on Connection Complete */
	conn = bt_conn_add_le(BT_ID_DEFAULT, &addr);
	zassert_not_null(conn);
	conn->handle = HANDLE;
	bt_conn_set_state(conn, BT_CONN_CONNECTED);
	bt_l2cap_connected(conn);

	connect_chan(0U, 0U);
	connect_chan(1U, 2U);

	return NULL;
}

/* Each case counts from zero, whichever cases ran before it */
static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	ARRAY_FOR_EACH_PTR(ctxs, ctx) {
		ctx->rx_len = 0U;
		ctx->tx_done_count = 0U;
		ctx->closed_count = 0U;
	}
}

ZTEST_SUITE(ots_l2cap, NULL, setup, before, NULL, NULL);

/* A channel that is reassembling an SDU does not hold up the SDU of another */
static ZTEST(ots_l2cap, test_rx_interleaved)
{
	/* The Host gives a credit for the second PDU of each SDU... */
	recv_seg(0U, true);
	tx_credits(&ctxs[0].l2cap.ot_chan, 1U);
	recv_seg(1U, true);
	tx_credits(&ctxs[1].l2cap.ot_chan, 1U);

	/* ...and one for the next SDU */
	recv_seg(0U, false);
	tx_credits(&ctxs[0].l2cap.ot_chan, 1U);
	recv_seg(1U, false);
	tx_credits(&ctxs[1].l2cap.ot_chan, 1U);

	ARRAY_FOR_EACH(ctxs, i) {
		zassert_equal(ctxs[i].rx_len, CONFIG_BT_OTS_L2CAP_CHAN_RX_MTU);
		zassert_mem_equal(ctxs[i].rx, ctxs[i].data, ctxs[i].rx_len, "SDU %zu", i);
	}
}

/* A channel without credits does not hold up the transfer on another channel */
static ZTEST(ots_l2cap, test_tx_concurrent)
{
	/* The first SDU of a transfer of two takes the controller's buffer until
	 * release()
	 */
	hold = true;
	zassert_ok(bt_gatt_ots_l2cap_send(&ctxs[1].l2cap, ctxs[1].data, 2U * PEER_MTU));
	tx_sdu(1U, &ctxs[1].data[0]);

	/* Queued meanwhile on the channel without credits */
	zassert_ok(bt_gatt_ots_l2cap_send(&ctxs[0].l2cap, ctxs[0].data, PEER_MTU));

	/* The second SDU of the transfer follows the first one */
	release();
	tx_sdu(1U, &ctxs[1].data[PEER_MTU]);

	/* The SDU on the other channel follows its credit */
	recv_credits(0U, 1U);
	tx_sdu(0U, ctxs[0].data);

	/* Let the Host handle the completed packets */
	(void)k_sleep(K_MSEC(1));

	zassert_equal(ctxs[0].tx_done_count, 1U);
	zassert_equal(ctxs[1].tx_done_count, 1U);
}

/* A transfer that cannot get a buffer for its first SDU leaves the context free
 * for the next transfer
 */
static ZTEST(ots_l2cap, test_tx_failed_first_sdu)
{
	take_tx_buffers(PEER_MTU);

	zassert_equal(bt_gatt_ots_l2cap_send(&ctxs[1].l2cap, ctxs[1].data, PEER_MTU), -ENOMEM);

	/* The driver releases the packet */
	net_buf_drop(&kept);

	recv_credits(1U, 1U);
	zassert_ok(bt_gatt_ots_l2cap_send(&ctxs[1].l2cap, ctxs[1].data, PEER_MTU));
	tx_sdu(1U, ctxs[1].data);

	send_waiting_sdu();
}

/* A transfer that cannot get a buffer for its next SDU ends with its channel
 * instead of stalling
 */
static ZTEST(ots_l2cap, test_tx_failed_next_sdu)
{
	take_tx_buffers(2U * PEER_MTU);
	zassert_equal(ctxs[1].l2cap.tx.len, 0U);

	/* The Host sends the disconnection once the driver releases the packet */
	net_buf_drop(&kept);
	tx_disconn(PEER_CID + 1U, ctxs[1].l2cap.ot_chan.rx.cid);
	zassert_equal(ctxs[1].closed_count, 1U);
	zassert_equal(ctxs[1].tx_done_count, 0U);

	/* Back to the state of setup() */
	connect_chan(1U, 2U);
	send_waiting_sdu();
}

#if defined(CONFIG_BT_OTS_CLIENT)
/* GATT handles a discovery of the peer's OTS would have filled in */
#define CLIENT_START_HANDLE 0x0010U
#define CLIENT_END_HANDLE   0x0020U
#define CLIENT_OACP_HANDLE  0x0015U
/* The peer's CIDs for the client channel and for the later incoming channel */
#define CLIENT_PEER_CID   0x0060U
#define INCOMING_PEER_CID 0x0070U

static int obj_data_read(struct bt_ots_client *inst, struct bt_conn *link, uint32_t offset,
			 uint32_t len, uint8_t *data, bool is_complete)
{
	ARGS_UNUSED(inst, link, offset, len, data, is_complete);

	return BT_OTS_CONTINUE;
}

/* The OTS client instance, which the first case that uses it registers */
static struct bt_ots_client *client(void)
{
	static struct bt_ots_client otc;
	static struct bt_ots_client_cb cb = { .obj_data_read = obj_data_read };

	if (otc.cb == NULL) {
		otc.start_handle = CLIENT_START_HANDLE;
		otc.end_handle = CLIENT_END_HANDLE;
		otc.oacp_handle = CLIENT_OACP_HANDLE;
		otc.cb = &cb;
		zassert_ok(bt_ots_client_register(&otc));
	}

	return &otc;
}

/* The OTS client connects a channel for a procedure. Returns the Host's CID. */
static uint16_t client_chan_connect(void)
{
	struct bt_l2cap_le_conn_req *req;
	struct net_buf *buf;
	uint16_t host_cid;
	uint8_t ident;

	buf = wait_sig(BT_L2CAP_LE_CONN_REQ, &ident);
	req = net_buf_pull_mem(buf, sizeof(*req));
	zassert_equal(sys_le16_to_cpu(req->psm), OTS_PSM);
	host_cid = sys_le16_to_cpu(req->scid);
	net_buf_unref(buf);
	send_conn_rsp(ident, CLIENT_PEER_CID);

	return host_cid;
}

/* After an OTS client transfer, the client's channel context is free but keeps
 * the client's receive callback. The acceptor must not hand that context to an
 * incoming channel, where a received SDU would run the client receive path with
 * no transfer in progress.
 */
static ZTEST(ots_l2cap, test_client_context_not_reused_on_accept)
{
	struct bt_ots_client *otc = client();
	struct bt_l2cap_le_conn_rsp *rsp;
	struct net_buf *buf;
	uint16_t host_cid;
	uint16_t result;
	uint8_t ident;

	otc->cur_object.size.cur = 1U;

	/* Start reading an object: the client connects a channel and marks a
	 * transfer in progress.
	 */
	(void)bt_ots_client_read_object_data(otc, conn);

	host_cid = client_chan_connect();
	recv_att_write_rsp();

	/* Close the channel before any object data: the client ends the transfer
	 * and frees its context, which keeps the client's receive callback.
	 */
	buf = sig_cmd(BT_L2CAP_DISCONN_REQ, PEER_IDENT);
	net_buf_add_le16(buf, host_cid);
	net_buf_add_le16(buf, CLIENT_PEER_CID);
	recv_sig(buf);

	/* A peer now opens the OTS PSM. The acceptor must not reuse the client's
	 * context; this image has no free server context, so it refuses.
	 */
	buf = sig_cmd(BT_L2CAP_LE_CONN_REQ, PEER_IDENT + 1U);
	net_buf_add_le16(buf, OTS_PSM);
	net_buf_add_le16(buf, INCOMING_PEER_CID);
	net_buf_add_le16(buf, PEER_MTU);
	net_buf_add_le16(buf, PEER_MPS);
	net_buf_add_le16(buf, 1U);
	recv_sig(buf);

	buf = wait_sig(BT_L2CAP_LE_CONN_RSP, &ident);
	zassert_equal(ident, PEER_IDENT + 1U);
	rsp = net_buf_pull_mem(buf, sizeof(*rsp));
	result = sys_le16_to_cpu(rsp->result);
	net_buf_unref(buf);

	zassert_equal(result, BT_L2CAP_LE_ERR_NO_RESOURCES, "accepted with result 0x%04x", result);
}

/* An OTS client object write that cannot get a buffer for its first SDU ends
 * with its channel, as the server waits for the object data
 */
static ZTEST(ots_l2cap, test_client_write_failed_first_sdu)
{
	struct bt_ots_client *otc = client();
	uint16_t host_cid;

	/* A write from the start of an empty object */
	otc->cur_object.size.cur = 0U;
	otc->cur_object.size.alloc = PEER_MTU;
	zassert_ok(bt_ots_client_write_object_data(otc, conn, ctxs[0].data, PEER_MTU, 0,
						   BT_OTS_OACP_WRITE_OP_MODE_NONE));

	host_cid = client_chan_connect();
	/* The ATT Write Request of the OACP command */
	net_buf_unref(tx(BT_L2CAP_CID_ATT));

	/* The data goes out once the server accepts the command, by when the
	 * other channels hold both buffers
	 */
	take_tx_buffers(PEER_MTU);
	recv_att_write_rsp();

	/* The Host sends the disconnection once the driver releases the packet */
	net_buf_drop(&kept);
	tx_disconn(CLIENT_PEER_CID, host_cid);

	send_waiting_sdu();
}
#endif /* CONFIG_BT_OTS_CLIENT */
