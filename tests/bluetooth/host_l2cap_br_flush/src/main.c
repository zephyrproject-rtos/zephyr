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
#include "host/classic/l2cap_br_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of initialization commands that are
 * irrelevant to this test
 */
#define GENERIC_RP_SIZE 64U

/* Small, so that a PDU of PAYLOAD_LEN bytes goes out in several fragments, but
 * large enough for each signaling packet of the host to fit in one
 */
#define ACL_MAX_LEN 32U
#define ACL_MAX_NUM 8U

#define TEST_CONN_HANDLE 0x0001U

/* Non-flushable Packet Boundary Flag in the LMP features */
#define FEAT_NON_FLUSH_PB_OCTET 6U
#define FEAT_NON_FLUSH_PB_BIT   BIT(6)

#define TEST_PSM   0x1001U
/* Channel ID of the channel on the peer */
#define PEER_CID   0x0040U
/* Identifier of the Configuration Request of the peer */
#define PEER_IDENT 0x80U

#define PAYLOAD_LEN   80U
/* L2CAP header and payload, in fragments of up to ACL_MAX_LEN bytes */
#define PAYLOAD_FRAGS DIV_ROUND_UP(BT_L2CAP_HDR_SIZE + PAYLOAD_LEN, ACL_MAX_LEN)

#define MAX_PACKETS   16U
#define MAX_CONF_REQS 4U

#define CONTROLLER_STACK_SIZE 1024U

/* How long to wait for the host to do something */
#define WAIT_US (USEC_PER_SEC)

/* Flush timeouts, in milliseconds and in 0.625 ms units. 1 ms, no
 * retransmissions, gets the smallest value all controllers support.
 */
#define TEST_FLUSH_TIMEOUT          ((uint16_t)CONFIG_BT_L2CAP_TX_FLUSH_TO)
#define TEST_FLUSH_TIMEOUT_HCI      MAX(TEST_FLUSH_TIMEOUT * 8U / 5U, 2U)
#define PEER_FLUSH_TIMEOUT          100U
#define PEER_FLUSH_TIMEOUT_HCI      160U
/* More than the Automatic Flush Timeout can hold */
#define PEER_FLUSH_TIMEOUT_UNUSABLE 2000U
#define FLUSH_TIMEOUT_INFINITE      0xffffU

/* Streaming mode in the extended feature mask of the peer */
#define PEER_FEAT_STREAM BIT(4)

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
/* The status the controller answers it with */
static uint8_t flush_status;

/* The peer answers the first Configuration Request with Unacceptable
 * Parameters and this flush timeout if it is not 0, and accepts the others
 */
static uint16_t peer_unaccept_timeout;
/* The flush timeout the Success responses of the peer carry if it is not 0 */
static uint16_t peer_success_timeout;
/* The Flush Timeout option of each Configuration Request, 0 if it had none */
static uint16_t conf_flush[MAX_CONF_REQS];
static atomic_t conf_reqs;
static atomic_t info_rsps;
/* Channel ID of the channel on the host */
static uint16_t host_cid;

/* Sends the packets of the peer from its own thread, as the RX thread of a
 * driver does. It has the lowest priority, so that, as with a real peer, the
 * host and the test are done with a request before the response arrives.
 */
static struct k_work_q ctlr_work_q;
static K_THREAD_STACK_DEFINE(ctlr_work_q_stack, CONTROLLER_STACK_SIZE);
static K_FIFO_DEFINE(peer_fifo);

static void peer_handler(struct k_work *work)
{
	struct net_buf *buf;

	ARG_UNUSED(work);

	buf = k_fifo_get(&peer_fifo, K_NO_WAIT);
	while (buf != NULL) {
		bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
		buf = k_fifo_get(&peer_fifo, K_NO_WAIT);
	}
}

static K_WORK_DEFINE(peer_work, peer_handler);

/* Delivers the events of the controller from the system work queue, which the
 * driver API allows. The host then runs the TX callbacks inline there, so
 * nothing they do may wait for the controller.
 */
static K_FIFO_DEFINE(evt_fifo);

static void evt_handler(struct k_work *work)
{
	struct net_buf *buf;

	ARG_UNUSED(work);

	buf = k_fifo_get(&evt_fifo, K_NO_WAIT);
	while (buf != NULL) {
		bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
		buf = k_fifo_get(&evt_fifo, K_NO_WAIT);
	}
}

static K_WORK_DEFINE(evt_work, evt_handler);

static void evt_deliver(struct net_buf *buf)
{
	k_fifo_put(&evt_fifo, buf);
	(void)k_work_submit(&evt_work);
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
	/* BR/EDR and LE supported */
	rp->features[4] = BIT(6);
	rp->features[FEAT_NON_FLUSH_PB_OCTET] = FEAT_NON_FLUSH_PB_BIT;
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
	rp->bdaddr = (bt_addr_t){.val = {0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6}};
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
	rp->status = flush_status;
}

static void cmd_handle(struct net_buf *cmd)
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

	evt_deliver(evt);
}

static const struct device *hci_dev(void)
{
	return DEVICE_DT_GET(DT_DRV_INST(0));
}

/* Send a signaling packet of the peer to the host */
static void sig_send(uint8_t code, uint8_t ident, const void *data, uint16_t len)
{
	struct bt_l2cap_sig_hdr *sig;
	struct bt_hci_acl_hdr *acl;
	struct bt_l2cap_hdr *l2;
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	acl = net_buf_add(buf, sizeof(*acl));
	acl->handle = sys_cpu_to_le16(bt_acl_handle_pack(TEST_CONN_HANDLE, BT_ACL_START));
	acl->len = sys_cpu_to_le16(sizeof(*l2) + sizeof(*sig) + len);
	l2 = net_buf_add(buf, sizeof(*l2));
	l2->len = sys_cpu_to_le16(sizeof(*sig) + len);
	l2->cid = sys_cpu_to_le16(BT_L2CAP_CID_BR_SIG);
	sig = net_buf_add(buf, sizeof(*sig));
	sig->code = code;
	sig->ident = ident;
	sig->len = sys_cpu_to_le16(len);
	(void)net_buf_add_mem(buf, data, len);

	k_fifo_put(&peer_fifo, buf);
	(void)k_work_submit_to_queue(&ctlr_work_q, &peer_work);
}

/* With CONFIG_BT_L2CAP_RET_FC, the peer supports Streaming mode */
static void info_req_handle(uint8_t ident, struct net_buf *buf)
{
	const struct bt_l2cap_info_req *req = (const void *)buf->data;
	struct {
		struct bt_l2cap_info_rsp rsp;
		uint32_t mask;
	} __packed rsp = {
		.rsp.type = req->type,
		.rsp.result = sys_cpu_to_le16(BT_L2CAP_INFO_NOTSUPP),
	};
	uint16_t len = sizeof(rsp.rsp);

	if (IS_ENABLED(CONFIG_BT_L2CAP_RET_FC) &&
	    sys_le16_to_cpu(req->type) == BT_L2CAP_INFO_FEAT_MASK) {
		rsp.rsp.result = sys_cpu_to_le16(BT_L2CAP_INFO_SUCCESS);
		rsp.mask = sys_cpu_to_le32(PEER_FEAT_STREAM);
		len = sizeof(rsp);
	}

	sig_send(BT_L2CAP_INFO_RSP, ident, &rsp, len);
	(void)atomic_inc(&info_rsps);
}

/* Accept the channel and configure it with the default parameters */
static void conn_req_handle(uint8_t ident, struct net_buf *buf)
{
	const struct bt_l2cap_conn_req *req = (const void *)buf->data;
	struct bt_l2cap_conn_rsp rsp = {
		.dcid = sys_cpu_to_le16(PEER_CID),
		.scid = req->scid,
		.result = sys_cpu_to_le16(BT_L2CAP_BR_SUCCESS),
		.status = sys_cpu_to_le16(BT_L2CAP_CS_NO_INFO),
	};
	struct bt_l2cap_conf_req conf = {
		.dcid = req->scid,
	};

	host_cid = sys_le16_to_cpu(req->scid);
	sig_send(BT_L2CAP_CONN_RSP, ident, &rsp, sizeof(rsp));
	sig_send(BT_L2CAP_CONF_REQ, PEER_IDENT, &conf, sizeof(conf));
}

static uint16_t conf_req_flush_timeout(struct net_buf *buf)
{
	while (buf->len >= sizeof(struct bt_l2cap_conf_opt)) {
		const struct bt_l2cap_conf_opt *opt = net_buf_pull_mem(buf, sizeof(*opt));

		if (opt->len > buf->len) {
			break;
		}

		if ((opt->type & BT_L2CAP_CONF_MASK) == BT_L2CAP_CONF_OPT_FLUSH_TIMEOUT &&
		    opt->len == sizeof(struct bt_l2cap_conf_opt_flush_timeout)) {
			return sys_get_le16(opt->data);
		}

		(void)net_buf_pull_mem(buf, opt->len);
	}

	return 0U;
}

static void conf_req_handle(uint8_t ident, struct net_buf *buf)
{
	struct {
		struct bt_l2cap_conf_rsp rsp;
		struct bt_l2cap_conf_opt opt;
		struct bt_l2cap_conf_opt_flush_timeout to;
	} __packed rsp = {
		.rsp.scid = sys_cpu_to_le16(host_cid),
		.rsp.result = sys_cpu_to_le16(BT_L2CAP_CONF_SUCCESS),
	};
	uint16_t len = sizeof(rsp.rsp);
	atomic_val_t n;

	(void)net_buf_pull_mem(buf, sizeof(struct bt_l2cap_conf_req));

	n = atomic_inc(&conf_reqs);
	if (n < MAX_CONF_REQS) {
		conf_flush[n] = conf_req_flush_timeout(buf);
	}

	if (n == 0 && peer_unaccept_timeout != 0U) {
		rsp.rsp.result = sys_cpu_to_le16(BT_L2CAP_CONF_UNACCEPT);
		rsp.opt.type = BT_L2CAP_CONF_OPT_FLUSH_TIMEOUT;
		rsp.opt.len = sizeof(rsp.to);
		rsp.to.timeout = sys_cpu_to_le16(peer_unaccept_timeout);
		len = sizeof(rsp);
	} else if (peer_success_timeout != 0U) {
		rsp.opt.type = BT_L2CAP_CONF_OPT_FLUSH_TIMEOUT;
		rsp.opt.len = sizeof(rsp.to);
		rsp.to.timeout = sys_cpu_to_le16(peer_success_timeout);
		len = sizeof(rsp);
	}

	sig_send(BT_L2CAP_CONF_RSP, ident, &rsp, len);
}

static void disconn_req_handle(uint8_t ident, struct net_buf *buf)
{
	const struct bt_l2cap_disconn_req *req = (const void *)buf->data;
	struct bt_l2cap_disconn_rsp rsp = {
		.dcid = req->dcid,
		.scid = req->scid,
	};

	sig_send(BT_L2CAP_DISCONN_RSP, ident, &rsp, sizeof(rsp));
}

/* Answer a signaling packet of the host as the peer would */
static void sig_handle(struct net_buf *buf)
{
	const struct bt_l2cap_sig_hdr *sig;

	if (buf->len < sizeof(*sig)) {
		return;
	}

	sig = net_buf_pull_mem(buf, sizeof(*sig));
	if (sys_le16_to_cpu(sig->len) != buf->len) {
		/* Not a command, e.g. data the test sends on the signaling channel */
		return;
	}

	switch (sig->code) {
	case BT_L2CAP_INFO_REQ:
		info_req_handle(sig->ident, buf);
		break;
	case BT_L2CAP_CONN_REQ:
		conn_req_handle(sig->ident, buf);
		break;
	case BT_L2CAP_CONF_REQ:
		conf_req_handle(sig->ident, buf);
		break;
	case BT_L2CAP_DISCONN_REQ:
		disconn_req_handle(sig->ident, buf);
		break;
	default:
		break;
	}
}

static void acl_handle(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *hdr;
	struct bt_l2cap_hdr *l2;
	atomic_val_t n;
	uint8_t pb;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	pb = bt_acl_flags_pb(bt_acl_flags(sys_le16_to_cpu(hdr->handle)));

	n = atomic_inc(&pkt_count);
	if (n < MAX_PACKETS) {
		pkt_pb[n] = pb;
		pkt_len[n] = sys_le16_to_cpu(hdr->len);
	}

	if (pb == BT_ACL_CONT || buf->len < sizeof(*l2)) {
		return;
	}

	l2 = net_buf_pull_mem(buf, sizeof(*l2));
	if (sys_le16_to_cpu(l2->cid) == BT_L2CAP_CID_BR_SIG &&
	    sys_le16_to_cpu(l2->len) == buf->len) {
		sig_handle(buf);
	}
}

static void completed_handler(struct k_work *work)
{
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
	evt_deliver(buf);
}

static K_WORK_DEFINE(completed_work, completed_handler);

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/* Needed for bt_disable() */
static int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	ARG_UNUSED(dev);

	if (type == BT_HCI_H4_CMD) {
		cmd_handle(buf);
	} else if (type == BT_HCI_H4_ACL) {
		acl_handle(buf);
		(void)atomic_inc(&acl_pending);
		(void)k_work_submit_to_queue(&ctlr_work_q, &completed_work);
	}

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

static struct bt_conn *test_conn;
static struct bt_l2cap_br_chan test_chan;
static atomic_t chan_connected;
static atomic_t chan_disconnected;

static void evt_send(uint8_t evt, const void *params, uint8_t len)
{
	struct net_buf *buf;

	buf = bt_buf_get_evt(evt, false, K_FOREVER);
	evt_create(buf, evt, len);
	(void)net_buf_add_mem(buf, params, len);
	bt_hci_recv(hci_dev(), buf);
}

static void chan_connected_cb(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);

	(void)atomic_inc(&chan_connected);
}

static void chan_disconnected_cb(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);

	(void)atomic_inc(&chan_disconnected);
}

static int chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	ARGS_UNUSED(chan, buf);

	return 0;
}

static const struct bt_l2cap_chan_ops chan_ops = {
	.connected = chan_connected_cb,
	.disconnected = chan_disconnected_cb,
	.recv = chan_recv,
};

/* Connect the test channel */
static void chan_connect(bool flushable)
{
	test_chan.flushable = flushable;

	zassert_ok(bt_l2cap_chan_connect(test_conn, &test_chan.chan, TEST_PSM));
	zassert_true(WAIT_FOR(atomic_get(&chan_connected) == 1, WAIT_US, (void)k_msleep(1)),
		     "Channel not connected");

	/* The signaling went out before the channel was connected, so only the
	 * packets the test sends are counted from here on
	 */
	(void)atomic_set(&pkt_count, 0);
}

/* Wait until the controller got count data packets, and no more */
static void packets_expect(atomic_val_t count)
{
	zassert_true(WAIT_FOR(atomic_get(&pkt_count) >= count, WAIT_US, (void)k_msleep(1)),
		     "%ld data packets, expected %ld", atomic_get(&pkt_count), count);

	/* The host sends all queued fragments as soon as the test controller
	 * completes the previous ones, which it does right away, so a packet
	 * that was not expected would have been sent within this time
	 */
	(void)k_msleep(50);

	zassert_equal(atomic_get(&pkt_count), count, "%ld data packets, expected %ld",
		      atomic_get(&pkt_count), count);
}

static void pdu_send(void)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
	zassert_not_null(buf);

	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
	(void)memset(net_buf_add(buf, PAYLOAD_LEN), 0x5a, PAYLOAD_LEN);

	zassert_ok(bt_l2cap_chan_send(&test_chan.chan, buf), "Send failed");
}

/* A PDU on the signaling channel, which is never flushable */
static void sig_pdu_send(void)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
	zassert_not_null(buf);

	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
	(void)memset(net_buf_add(buf, sizeof(uint32_t)), 0x5a, sizeof(uint32_t));

	zassert_ok(bt_l2cap_br_send_cb(test_conn, BT_L2CAP_CID_BR_SIG, buf, NULL, NULL),
		   "Send failed");
}

/* Check the fragments of a PDU on the test channel */
static void pdu_expect(unsigned int first, uint8_t pb)
{
	zassert_equal(pkt_pb[first], pb, "PB 0x%02x", pkt_pb[first]);
	for (unsigned int i = first + 1U; i < first + PAYLOAD_FRAGS; i++) {
		zassert_equal(pkt_pb[i], BT_ACL_CONT, "PB 0x%02x of fragment %u", pkt_pb[i], i);
	}
}

static void *setup(void)
{
	static const bt_addr_t peer = {.val = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
	struct bt_hci_evt_conn_complete evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.handle = sys_cpu_to_le16(TEST_CONN_HANDLE),
		.link_type = BT_HCI_ACL,
	};

	k_work_queue_start(&ctlr_work_q, ctlr_work_q_stack,
			   K_THREAD_STACK_SIZEOF(ctlr_work_q_stack),
			   K_LOWEST_APPLICATION_THREAD_PRIO, NULL);

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	test_conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(test_conn, "Connecting failed");

	bt_addr_copy(&evt.bdaddr, &peer);
	evt_send(BT_HCI_EVT_CONN_COMPLETE, &evt, sizeof(evt));

	zassert_true(WAIT_FOR(test_conn->state == BT_CONN_CONNECTED, WAIT_US, (void)k_msleep(1)),
		     "No connection");
	/* The host asks for the features of the peer when connected */
	zassert_true(WAIT_FOR(atomic_get(&info_rsps) > 0, WAIT_US, (void)k_msleep(1)),
		     "No Information Request");

	return NULL;
}

static void before(void *f)
{
	ARG_UNUSED(f);

	(void)memset(&test_chan, 0, sizeof(test_chan));
	test_chan.chan.ops = &chan_ops;

	peer_unaccept_timeout = 0U;
	peer_success_timeout = 0U;
	flush_status = BT_HCI_ERR_SUCCESS;
	(void)memset(conf_flush, 0, sizeof(conf_flush));
	(void)atomic_set(&conf_reqs, 0);
	(void)atomic_set(&chan_connected, 0);
	(void)atomic_set(&chan_disconnected, 0);
	(void)atomic_set(&pkt_count, 0);
	(void)atomic_set(&flush_cmds, 0);
}

static void after(void *f)
{
	ARG_UNUSED(f);

	bt_dev.features[0][FEAT_NON_FLUSH_PB_OCTET] |= FEAT_NON_FLUSH_PB_BIT;

	if (test_chan.state == BT_L2CAP_DISCONNECTED) {
		return;
	}

	zassert_ok(bt_l2cap_chan_disconnect(&test_chan.chan));
	zassert_true(WAIT_FOR(atomic_get(&chan_disconnected) == 1, WAIT_US, (void)k_msleep(1)),
		     "Channel not disconnected");
}

static void teardown(void *f)
{
	struct bt_hci_evt_disconn_complete evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.handle = sys_cpu_to_le16(TEST_CONN_HANDLE),
		.reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN,
	};

	ARG_UNUSED(f);

	evt_send(BT_HCI_EVT_DISCONN_COMPLETE, &evt, sizeof(evt));
	zassert_true(WAIT_FOR(test_conn->state == BT_CONN_DISCONNECTED, WAIT_US, (void)k_msleep(1)),
		     "Not disconnected");
	bt_conn_drop(&test_conn);

	zassert_ok(bt_disable(), "Bluetooth disable failed");
}

ZTEST_SUITE(l2cap_br_flush, NULL, setup, before, after, teardown);

/* Without a flush timeout, the configuration has none and the PDUs are not
 * flushable
 */
static ZTEST(l2cap_br_flush, test_non_flushable_by_default)
{
	chan_connect(false);

	zassert_equal(atomic_get(&conf_reqs), 1);
	zassert_equal(conf_flush[0], 0U, "Flush timeout %u", conf_flush[0]);
	zassert_equal(atomic_get(&flush_cmds), 0);

	pdu_send();
	packets_expect(PAYLOAD_FRAGS);
	pdu_expect(0U, BT_ACL_START_NO_FLUSH);
}

static ZTEST(l2cap_br_flush, test_auto_flush_timeout)
{
	zassert_ok(bt_conn_br_set_auto_flush_timeout(test_conn, TEST_FLUSH_TIMEOUT_HCI));
	zassert_equal(atomic_get(&flush_cmds), 1);
	zassert_equal(flush_handle, TEST_CONN_HANDLE, "handle 0x%04x", flush_handle);
	zassert_equal(flush_timeout, TEST_FLUSH_TIMEOUT_HCI, "timeout %u", flush_timeout);

	zassert_equal(
		bt_conn_br_set_auto_flush_timeout(test_conn, BT_HCI_AUTO_FLUSH_TIMEOUT_MAX + 1U),
		-EINVAL);
	zassert_equal(bt_conn_br_set_auto_flush_timeout(NULL, TEST_FLUSH_TIMEOUT_HCI), -EINVAL);
	zassert_equal(atomic_get(&flush_cmds), 1);
}

#if CONFIG_BT_L2CAP_TX_FLUSH_TO != 0
/* The configuration announces the flush timeout. Once the peer accepted it,
 * the link gets it as Automatic Flush Timeout, and the PDUs of the channel are
 * flushable while those of other channels are not.
 */
static ZTEST(l2cap_br_flush, test_flush_timeout_accepted)
{
	chan_connect(true);

	zassert_equal(atomic_get(&conf_reqs), 1);
	zassert_equal(conf_flush[0], TEST_FLUSH_TIMEOUT, "Flush timeout %u", conf_flush[0]);
	zassert_equal(atomic_get(&flush_cmds), 1);
	zassert_equal(flush_handle, TEST_CONN_HANDLE, "handle 0x%04x", flush_handle);
	zassert_equal(flush_timeout, TEST_FLUSH_TIMEOUT_HCI, "timeout %u", flush_timeout);

	pdu_send();
	sig_pdu_send();
	packets_expect(PAYLOAD_FRAGS + 1U);
	pdu_expect(0U, BT_ACL_START);
	zassert_equal(pkt_pb[PAYLOAD_FRAGS], BT_ACL_START_NO_FLUSH, "PB 0x%02x",
		      pkt_pb[PAYLOAD_FRAGS]);
	zassert_equal(pkt_len[PAYLOAD_FRAGS], BT_L2CAP_HDR_SIZE + sizeof(uint32_t));
}

/* A peer that only accepts 0xFFFF gets it in the next configuration request,
 * and the channel is not flushable
 */
static ZTEST(l2cap_br_flush, test_flush_timeout_unaccepted_infinite)
{
	peer_unaccept_timeout = FLUSH_TIMEOUT_INFINITE;

	chan_connect(true);

	zassert_equal(atomic_get(&conf_reqs), 2);
	zassert_equal(conf_flush[0], TEST_FLUSH_TIMEOUT, "Flush timeout %u", conf_flush[0]);
	zassert_equal(conf_flush[1], FLUSH_TIMEOUT_INFINITE, "Flush timeout %u", conf_flush[1]);
	zassert_equal(atomic_get(&flush_cmds), 0);

	pdu_send();
	packets_expect(PAYLOAD_FRAGS);
	pdu_expect(0U, BT_ACL_START_NO_FLUSH);
}

/* A Success response may echo the flush timeout */
static ZTEST(l2cap_br_flush, test_flush_timeout_accepted_echo)
{
	peer_success_timeout = TEST_FLUSH_TIMEOUT;

	chan_connect(true);

	zassert_equal(atomic_get(&flush_cmds), 1);
	zassert_equal(flush_timeout, TEST_FLUSH_TIMEOUT_HCI, "timeout %u", flush_timeout);

	pdu_send();
	packets_expect(PAYLOAD_FRAGS);
	pdu_expect(0U, BT_ACL_START);
}

/* A Success response with another flush timeout, which a positive response may
 * not adjust, ends the channel
 */
static ZTEST(l2cap_br_flush, test_flush_timeout_accepted_other)
{
	peer_success_timeout = PEER_FLUSH_TIMEOUT;

	test_chan.flushable = true;
	zassert_ok(bt_l2cap_chan_connect(test_conn, &test_chan.chan, TEST_PSM));
	zassert_true(WAIT_FOR(atomic_get(&chan_disconnected) == 1, WAIT_US, (void)k_msleep(1)),
		     "Channel not disconnected");

	zassert_equal(atomic_get(&chan_connected), 0);
	zassert_equal(atomic_get(&flush_cmds), 0);
}

/* If the link does not get the Automatic Flush Timeout, the channel is not
 * flushable
 */
static ZTEST(l2cap_br_flush, test_flush_timeout_write_failed)
{
	flush_status = BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;

	chan_connect(true);

	zassert_equal(atomic_get(&flush_cmds), 1);

	pdu_send();
	packets_expect(PAYLOAD_FRAGS);
	pdu_expect(0U, BT_ACL_START_NO_FLUSH);
}

/* The flush timeout the peer proposes is taken if the link can hold it */
static ZTEST(l2cap_br_flush, test_flush_timeout_unaccepted_value)
{
	peer_unaccept_timeout = PEER_FLUSH_TIMEOUT;

	chan_connect(true);

	zassert_equal(atomic_get(&conf_reqs), 2);
	zassert_equal(conf_flush[1], PEER_FLUSH_TIMEOUT, "Flush timeout %u", conf_flush[1]);
	zassert_equal(atomic_get(&flush_cmds), 1);
	zassert_equal(flush_timeout, PEER_FLUSH_TIMEOUT_HCI, "timeout %u", flush_timeout);

	pdu_send();
	packets_expect(PAYLOAD_FRAGS);
	pdu_expect(0U, BT_ACL_START);
}

/* A flush timeout the link cannot hold ends the channel */
static ZTEST(l2cap_br_flush, test_flush_timeout_unaccepted_unusable)
{
	peer_unaccept_timeout = PEER_FLUSH_TIMEOUT_UNUSABLE;

	test_chan.flushable = true;
	zassert_ok(bt_l2cap_chan_connect(test_conn, &test_chan.chan, TEST_PSM));
	zassert_true(WAIT_FOR(atomic_get(&chan_disconnected) == 1, WAIT_US, (void)k_msleep(1)),
		     "Channel not disconnected");

	zassert_equal(atomic_get(&chan_connected), 0);
	zassert_equal(atomic_get(&conf_reqs), 1);
	zassert_equal(atomic_get(&flush_cmds), 0);
}

/* Without the Non-flushable Packet Boundary Flag, the controller would take
 * every packet as flushable, so no channel is flushable
 */
static ZTEST(l2cap_br_flush, test_flushable_not_supported)
{
	bt_dev.features[0][FEAT_NON_FLUSH_PB_OCTET] &= ~FEAT_NON_FLUSH_PB_BIT;

	chan_connect(true);

	zassert_equal(conf_flush[0], 0U, "Flush timeout %u", conf_flush[0]);
	zassert_equal(atomic_get(&flush_cmds), 0);

	pdu_send();
	packets_expect(PAYLOAD_FRAGS);
	pdu_expect(0U, BT_ACL_START_NO_FLUSH);
}

#if defined(CONFIG_BT_L2CAP_RET_FC)
/* Only a channel in Basic mode is flushable */
static ZTEST(l2cap_br_flush, test_flushable_not_basic_mode)
{
	test_chan.flushable = true;
	test_chan.rx.mode = BT_L2CAP_BR_LINK_MODE_STREAM;

	/* Streaming mode needs the features of the peer, which the host may not
	 * have taken from the Information Response yet
	 */
	zassert_true(
		WAIT_FOR(bt_l2cap_chan_connect(test_conn, &test_chan.chan, TEST_PSM) != -ENOTSUP,
			 WAIT_US, (void)k_msleep(1)),
		"Streaming mode not supported");
	zassert_true(WAIT_FOR(atomic_get(&conf_reqs) > 0, WAIT_US, (void)k_msleep(1)),
		     "No Configuration Request");

	zassert_equal(conf_flush[0], 0U, "Flush timeout %u", conf_flush[0]);
	zassert_equal(atomic_get(&flush_cmds), 0);
}

/* A channel that falls back to Basic mode stays flushable */
static ZTEST(l2cap_br_flush, test_flushable_basic_mode_fallback)
{
	test_chan.rx.mode = BT_L2CAP_BR_LINK_MODE_ERET;
	test_chan.rx.optional = true;

	chan_connect(true);

	zassert_equal(test_chan.rx.mode, BT_L2CAP_BR_LINK_MODE_BASIC, "Mode %u", test_chan.rx.mode);
	zassert_equal(conf_flush[0], TEST_FLUSH_TIMEOUT, "Flush timeout %u", conf_flush[0]);
	zassert_equal(atomic_get(&flush_cmds), 1);
}
#endif /* CONFIG_BT_L2CAP_RET_FC */

#else  /* CONFIG_BT_L2CAP_TX_FLUSH_TO == 0 */

/* With CONFIG_BT_L2CAP_TX_FLUSH_TO 0, a flushable channel is not flushed */
static ZTEST(l2cap_br_flush, test_flushable_disabled)
{
	chan_connect(true);

	zassert_equal(conf_flush[0], 0U, "Flush timeout %u", conf_flush[0]);
	zassert_equal(atomic_get(&flush_cmds), 0);

	pdu_send();
	sig_pdu_send();
	packets_expect(PAYLOAD_FRAGS + 1U);
	pdu_expect(0U, BT_ACL_START_NO_FLUSH);
	zassert_equal(pkt_pb[PAYLOAD_FRAGS], BT_ACL_START_NO_FLUSH, "PB 0x%02x",
		      pkt_pb[PAYLOAD_FRAGS]);
}
#endif /* CONFIG_BT_L2CAP_TX_FLUSH_TO != 0 */
