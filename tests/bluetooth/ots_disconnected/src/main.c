/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/services/ots.h>
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
#include "services/ots/ots_internal.h"
#include "services/ots/ots_obj_manager_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define GENERIC_RP_SIZE 64U
#define LE_ACL_MAX_LEN  64U
#define FIRST_HANDLE    0x0001U

/* Headers of an ACL packet from the controller, after its H:4 packet type */
#define ACL_HDR_OFFSET    1U
#define L2CAP_HDR_OFFSET  (ACL_HDR_OFFSET + sizeof(struct bt_hci_acl_hdr))
#define L2CAP_DATA_OFFSET (L2CAP_HDR_OFFSET + sizeof(struct bt_l2cap_hdr))

#define OTS_PSM    0x0025U
#define PEER_CID   0x0040U
#define PEER_IDENT 0x80U
/* The smallest MTU of an LE credit-based channel. Each SDU that the Host sends is one K-frame */
#define PEER_MTU   23U
#define PEER_MPS   (BT_L2CAP_SDU_HDR_SIZE + PEER_MTU)

/* How long to wait for what the Host is to do. It takes no simulated time, so this only bounds a
 * failed wait.
 */
#define TX_TIMEOUT K_MSEC(100)

/* The objects that each case starts with, and the one that a client creates */
#define OBJ_1       BT_OTS_OBJ_ID_MIN
#define OBJ_2       (BT_OTS_OBJ_ID_MIN + 1U)
#define OBJ_CREATED (BT_OTS_OBJ_ID_MIN + 2U)
#define OBJ_TYPE    0x2B01U

/* The peer reads an object in two SDUs, and writes in two SDUs after its end */
#define OBJ_SIZE      (2U * PEER_MTU)
#define WRITE_SDU_LEN 16U
#define WRITE_SIZE    (2U * WRITE_SDU_LEN)
BUILD_ASSERT(WRITE_SIZE <= OBJ_SIZE);

/* The connection of a client. The Host indicates to every client subscribed to a control point. */
struct peer {
	struct bt_conn *conn;
	bool oacp_ind;
	bool olcp_ind;
};

enum {
	A,
	B,
};

static struct peer peers[CONFIG_BT_MAX_CONN];

BUILD_ASSERT(ARRAY_SIZE(peers) == 2U);

static struct bt_ots *ots;

/* The PDUs that the Host sent and the test has not taken yet. The Host gives no order between
 * connections.
 */
NET_BUF_POOL_FIXED_DEFINE(sent_pool, 6, LE_ACL_MAX_LEN, 0, NULL);
static struct k_fifo sent[ARRAY_SIZE(peers)];

/* The packets of each connection not yet reported as completed */
static atomic_t unreported[ARRAY_SIZE(peers)];

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static K_SEM_DEFINE(read_done_sem, 0, 1);
static K_SEM_DEFINE(write_done_sem, 0, 1);

static uint8_t obj_data[2U * OBJ_SIZE];
static uint8_t written[WRITE_SIZE];
static uint32_t write_offset;
static size_t written_len;
static size_t write_count;
static size_t read_count;
static size_t deleted_count;
static uint8_t deleted_conn_index;

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
	rp->le_max_num = CONFIG_BT_BUF_ACL_TX_COUNT;
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
	ARG_UNUSED(work);

	ARRAY_FOR_EACH(unreported, i) {
		struct bt_hci_evt_num_completed_packets *ev;
		struct bt_hci_handle_count *hc;
		atomic_val_t count;
		struct net_buf *evt;

		count = atomic_clear(&unreported[i]);
		if (count == 0) {
			continue;
		}

		evt = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
		evt_create(evt, BT_HCI_EVT_NUM_COMPLETED_PACKETS, sizeof(*ev) + sizeof(*hc));
		ev = net_buf_add(evt, sizeof(*ev));
		ev->num_handles = 1U;
		hc = net_buf_add(evt, sizeof(*hc));
		hc->handle = sys_cpu_to_le16(FIRST_HANDLE + i);
		hc->count = sys_cpu_to_le16((uint16_t)count);

		bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), evt);
	}
}

static K_WORK_DEFINE(completed_work, completed_handler);

/* The Host returns credits as it consumes what the peer sends. No case looks at them. */
static bool is_credits(const struct net_buf *acl)
{
	const struct bt_l2cap_hdr *hdr = (const void *)acl->data;
	const struct bt_l2cap_sig_hdr *sig = (const void *)&hdr[1];

	return (sys_le16_to_cpu(hdr->cid) == BT_L2CAP_CID_LE_SIG) &&
	       (sig->code == BT_L2CAP_LE_CREDITS);
}

static void acl_handle(struct net_buf *acl)
{
	struct bt_hci_acl_hdr *hdr;
	uint16_t handle;

	/* Every PDU fits a single ACL packet */
	hdr = net_buf_pull_mem(acl, sizeof(*hdr));
	handle = bt_acl_handle(sys_le16_to_cpu(hdr->handle));

	if (!is_credits(acl)) {
		struct net_buf *pdu;

		pdu = net_buf_alloc(&sent_pool, K_NO_WAIT);
		zassert_not_null(pdu);
		(void)net_buf_add_mem(pdu, acl->data, acl->len);
		k_fifo_put(&sent[handle - FIRST_HANDLE], pdu);
	}

	(void)atomic_inc(&unreported[handle - FIRST_HANDLE]);
	zassert_true(k_work_submit(&completed_work) >= 0);
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_close(const struct device *dev)
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

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	zassert_equal(err, 0U);

	peers[conn->handle - FIRST_HANDLE].conn = bt_conn_ref(conn);
	k_sem_give(&connected_sem);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);

	k_sem_give(&disconnected_sem);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

/* A client connects to the advertising Host */
static void connect_peer(uint8_t i)
{
	struct bt_hci_evt_le_conn_complete *cc;
	struct bt_hci_evt_le_meta_event *mev;
	struct net_buf *buf;

	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0, NULL, 0));

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*mev) + sizeof(*cc));
	mev = net_buf_add(buf, sizeof(*mev));
	mev->subevent = BT_HCI_EVT_LE_CONN_COMPLETE;
	cc = net_buf_add(buf, sizeof(*cc));
	(void)memset(cc, 0, sizeof(*cc));
	cc->status = BT_HCI_ERR_SUCCESS;
	cc->handle = sys_cpu_to_le16(FIRST_HANDLE + i);
	cc->role = BT_HCI_ROLE_PERIPHERAL;
	cc->peer_addr = (bt_addr_le_t){
		.type = BT_ADDR_LE_PUBLIC,
		.a = { .val = { 0x01U + i, 0x02, 0x03, 0x04, 0x05, 0x06 } },
	};
	cc->interval = sys_cpu_to_le16(24U);
	cc->supv_timeout = sys_cpu_to_le16(400U);

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)));

	zassert_ok(bt_le_adv_stop());
}

/* Wait for the Host to report the end of the connection, after which only the test holds it: the
 * service released its references too.
 */
static void peer_gone(uint8_t i)
{
	struct k_work_sync sync;
	struct net_buf *buf;

	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)));

	/* The test's callback may run before the one of the service */
	(void)k_work_flush_delayable(&peers[i].conn->deferred_work, &sync);

	zassert_equal(atomic_get(&peers[i].conn->ref), 1);
	bt_conn_drop(&peers[i].conn);
	peers[i].oacp_ind = false;
	peers[i].olcp_ind = false;

	buf = k_fifo_get(&sent[i], K_NO_WAIT);
	while (buf != NULL) {
		net_buf_unref(buf);
		buf = k_fifo_get(&sent[i], K_NO_WAIT);
	}
}

/* The peer disconnects, and the Host has reported it to the application on return */
static void disconnect_peer(uint8_t i)
{
	struct bt_hci_evt_disconn_complete *dc;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*dc));
	dc = net_buf_add(buf, sizeof(*dc));
	dc->status = BT_HCI_ERR_SUCCESS;
	dc->handle = sys_cpu_to_le16(FIRST_HANDLE + i);
	dc->reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;

	/* A controller reports no completed packets on a handle after its disconnection */
	(void)atomic_clear(&unreported[i]);
	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);

	peer_gone(i);
}

/* The next PDU that the Host sent on the connection, which must be on the channel, with its header
 * pulled
 */
static struct net_buf *tx(uint8_t i, uint16_t cid)
{
	struct bt_l2cap_hdr *hdr;
	struct net_buf *buf;

	buf = k_fifo_get(&sent[i], TX_TIMEOUT);
	zassert_not_null(buf, "Nothing sent");

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	zassert_equal(sys_le16_to_cpu(hdr->cid), cid, "cid 0x%04x", sys_le16_to_cpu(hdr->cid));

	return buf;
}

/* The next PDU must be a signalling command with this code. Returns it with its header pulled. */
static struct net_buf *tx_sig(uint8_t i, uint8_t code, uint8_t *ident)
{
	struct bt_l2cap_sig_hdr *sig;
	struct net_buf *buf;

	buf = tx(i, BT_L2CAP_CID_LE_SIG);

	sig = net_buf_pull_mem(buf, sizeof(*sig));
	zassert_equal(sig->code, code, "code 0x%02x", sig->code);
	*ident = sig->ident;

	return buf;
}

/* The next PDU must be a K-frame with a whole SDU on the channel */
static void tx_sdu(uint8_t i, const uint8_t *data)
{
	struct net_buf *buf;

	buf = tx(i, PEER_CID);
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

/* The Host handles what it gets in order, when the test next waits */
static void recv_pdu(uint8_t i, struct net_buf *buf, uint16_t cid)
{
	struct bt_hci_acl_hdr *acl = (void *)&buf->data[ACL_HDR_OFFSET];
	struct bt_l2cap_hdr *hdr = (void *)&buf->data[L2CAP_HDR_OFFSET];

	hdr->len = sys_cpu_to_le16(buf->len - L2CAP_DATA_OFFSET);
	hdr->cid = sys_cpu_to_le16(cid);
	acl->handle = sys_cpu_to_le16(bt_acl_handle_pack(FIRST_HANDLE + i, BT_ACL_START));
	acl->len = sys_cpu_to_le16(buf->len - L2CAP_HDR_OFFSET);

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
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

static void recv_sig(uint8_t i, struct net_buf *buf)
{
	struct bt_l2cap_sig_hdr *sig = (void *)&buf->data[L2CAP_DATA_OFFSET];

	sig->len = sys_cpu_to_le16(buf->len - L2CAP_DATA_OFFSET - sizeof(*sig));
	recv_pdu(i, buf, BT_L2CAP_CID_LE_SIG);
}

/* The peer connects the Object Transfer Channel of the OTS instance */
static void connect_chan(uint8_t i, uint16_t credits)
{
	struct bt_l2cap_le_conn_rsp *rsp;
	struct net_buf *buf;
	uint8_t ident;

	buf = sig_cmd(BT_L2CAP_LE_CONN_REQ, PEER_IDENT);
	net_buf_add_le16(buf, OTS_PSM);
	net_buf_add_le16(buf, PEER_CID);
	net_buf_add_le16(buf, PEER_MTU);
	net_buf_add_le16(buf, PEER_MPS);
	net_buf_add_le16(buf, credits);
	recv_sig(i, buf);

	buf = tx_sig(i, BT_L2CAP_LE_CONN_RSP, &ident);
	zassert_equal(ident, PEER_IDENT);

	rsp = net_buf_pull_mem(buf, sizeof(*rsp));
	zassert_equal(sys_le16_to_cpu(rsp->result), BT_L2CAP_LE_SUCCESS);
	zassert_equal(sys_le16_to_cpu(rsp->dcid), ots->l2cap.ot_chan.rx.cid);

	net_buf_unref(buf);
}

static void recv_credits(uint8_t i, uint16_t credits)
{
	struct net_buf *buf;

	buf = sig_cmd(BT_L2CAP_LE_CREDITS, PEER_IDENT + 1U);
	net_buf_add_le16(buf, PEER_CID);
	net_buf_add_le16(buf, credits);
	recv_sig(i, buf);
}

static struct net_buf *att_req(uint8_t op, uint16_t handle, const void *val, size_t len)
{
	struct net_buf *buf;

	buf = rx_pdu();
	(void)net_buf_add_u8(buf, op);
	net_buf_add_le16(buf, handle);
	(void)net_buf_add_mem(buf, val, len);

	return buf;
}

/* The next PDU must be an ATT PDU with this opcode. The case looks no further into it. */
static void tx_att_op(uint8_t i, uint8_t op)
{
	struct net_buf *buf;

	buf = tx(i, BT_L2CAP_CID_ATT);
	zassert_equal(net_buf_pull_u8(buf), op);
	net_buf_unref(buf);
}

static void att_write(uint8_t i, uint16_t handle, const void *val, size_t len)
{
	recv_pdu(i, att_req(BT_ATT_OP_WRITE_REQ, handle, val, len), BT_L2CAP_CID_ATT);
	tx_att_op(i, BT_ATT_OP_WRITE_RSP);
}

static void recv_confirm(uint8_t i)
{
	struct net_buf *buf;

	buf = rx_pdu();
	(void)net_buf_add_u8(buf, BT_ATT_OP_CONFIRM);
	recv_pdu(i, buf, BT_L2CAP_CID_ATT);
}

static const struct bt_gatt_attr *attr_get(const struct bt_uuid *uuid)
{
	const struct bt_gatt_service *service = ots->service;

	for (size_t i = 0U; i < service->attr_count; i++) {
		if (bt_uuid_cmp(service->attrs[i].uuid, uuid) == 0) {
			return &service->attrs[i];
		}
	}

	zassert_unreachable("No such attribute");

	return NULL;
}

/* The CCC descriptor follows the value of each control point */
static void subscribe(uint8_t i, const struct bt_uuid *cp)
{
	const uint16_t value = sys_cpu_to_le16(BT_GATT_CCC_INDICATE);

	att_write(i, bt_gatt_attr_get_handle(attr_get(cp) + 1), &value, sizeof(value));

	if (bt_uuid_cmp(cp, BT_UUID_OTS_ACTION_CP) == 0) {
		peers[i].oacp_ind = true;
	} else {
		peers[i].olcp_ind = true;
	}
}

/* The next PDU must indicate the response to the procedure, and the peer confirms it */
static void tx_cp_rsp(uint8_t i, const struct bt_uuid *cp, uint8_t rsp_op, uint8_t req_op,
		      uint8_t status)
{
	struct net_buf *buf;

	buf = tx(i, BT_L2CAP_CID_ATT);
	zassert_equal(net_buf_pull_u8(buf), BT_ATT_OP_INDICATE);
	zassert_equal(net_buf_pull_le16(buf), bt_gatt_attr_get_handle(attr_get(cp)));
	zassert_equal(net_buf_pull_u8(buf), rsp_op);
	zassert_equal(net_buf_pull_u8(buf), req_op);
	zassert_equal(net_buf_pull_u8(buf), status, "status 0x%02x", status);
	net_buf_unref(buf);

	recv_confirm(i);
}

static void oacp_proc_rsp(uint8_t i, const uint8_t *req, size_t len, uint8_t status)
{
	att_write(i, bt_gatt_attr_get_handle(attr_get(BT_UUID_OTS_ACTION_CP)), req, len);

	ARRAY_FOR_EACH(peers, j) {
		if (peers[j].oacp_ind) {
			tx_cp_rsp(j, BT_UUID_OTS_ACTION_CP, BT_GATT_OTS_OACP_PROC_RESP, req[0],
				  status);
		}
	}
}

static void oacp_proc(uint8_t i, const uint8_t *req, size_t len)
{
	oacp_proc_rsp(i, req, len, BT_GATT_OTS_OACP_RES_SUCCESS);
}

/* The client selects the object */
static void select_obj(uint8_t i, uint64_t id)
{
	NET_BUF_SIMPLE_DEFINE(req, sizeof(uint8_t) + BT_GATT_OTS_OLCP_GOTO_PARAMS_SIZE);

	(void)net_buf_simple_add_u8(&req, BT_GATT_OTS_OLCP_PROC_GOTO);
	net_buf_simple_add_le48(&req, id);

	att_write(i, bt_gatt_attr_get_handle(attr_get(BT_UUID_OTS_LIST_CP)), req.data, req.len);

	ARRAY_FOR_EACH(peers, j) {
		if (peers[j].olcp_ind) {
			tx_cp_rsp(j, BT_UUID_OTS_LIST_CP, BT_GATT_OTS_OLCP_PROC_RESP,
				  BT_GATT_OTS_OLCP_PROC_GOTO, BT_GATT_OTS_OLCP_RES_SUCCESS);
		}
	}
}

static void read_req_init(struct net_buf_simple *req)
{
	(void)net_buf_simple_add_u8(req, BT_GATT_OTS_OACP_PROC_READ);
	net_buf_simple_add_le32(req, 0U);
	net_buf_simple_add_le32(req, OBJ_SIZE);
}

/* The client starts reading the current object */
static void start_read(uint8_t i)
{
	NET_BUF_SIMPLE_DEFINE(req, sizeof(uint8_t) + BT_GATT_OTS_OACP_READ_PARAMS_SIZE);

	read_req_init(&req);
	oacp_proc(i, req.data, req.len);
}

/* The same, up to the Host getting the request: what follows is up to the case */
static void send_read_req(uint8_t i)
{
	const uint16_t handle = bt_gatt_attr_get_handle(attr_get(BT_UUID_OTS_ACTION_CP));

	NET_BUF_SIMPLE_DEFINE(req, sizeof(uint8_t) + BT_GATT_OTS_OACP_READ_PARAMS_SIZE);

	read_req_init(&req);
	recv_pdu(i, att_req(BT_ATT_OP_WRITE_REQ, handle, req.data, req.len), BT_L2CAP_CID_ATT);
}

/* The client starts writing to the current object, at its end */
static void start_write(uint8_t i, uint32_t offset)
{
	NET_BUF_SIMPLE_DEFINE(req, sizeof(uint8_t) + BT_GATT_OTS_OACP_WRITE_PARAMS_SIZE);

	write_offset = offset;

	(void)net_buf_simple_add_u8(&req, BT_GATT_OTS_OACP_PROC_WRITE);
	net_buf_simple_add_le32(&req, offset);
	net_buf_simple_add_le32(&req, WRITE_SIZE);
	(void)net_buf_simple_add_u8(&req, 0U);

	oacp_proc(i, req.data, req.len);
}

static void recv_write_sdu(uint8_t i, size_t offset)
{
	struct net_buf *buf;

	buf = rx_pdu();
	net_buf_add_le16(buf, WRITE_SDU_LEN);
	(void)net_buf_add_mem(buf, &obj_data[offset], WRITE_SDU_LEN);

	recv_pdu(i, buf, ots->l2cap.ot_chan.rx.cid);
}

/* The client creates an object, which becomes its current object */
static void create_obj(uint8_t i)
{
	NET_BUF_SIMPLE_DEFINE(req, sizeof(uint8_t) + BT_GATT_OTS_OACP_CREATE_GENERIC_PARAMS_SIZE +
					   sizeof(uint16_t));

	(void)net_buf_simple_add_u8(&req, BT_GATT_OTS_OACP_PROC_CREATE);
	net_buf_simple_add_le32(&req, OBJ_SIZE);
	net_buf_simple_add_le16(&req, OBJ_TYPE);

	oacp_proc(i, req.data, req.len);
}

/* Reading the ID of the current object gives this response, without its opcode */
static struct net_buf *read_id(uint8_t i, uint8_t rsp_op)
{
	const uint16_t handle = bt_gatt_attr_get_handle(attr_get(BT_UUID_OTS_ID));
	struct net_buf *buf;

	recv_pdu(i, att_req(BT_ATT_OP_READ_REQ, handle, NULL, 0U), BT_L2CAP_CID_ATT);

	buf = tx(i, BT_L2CAP_CID_ATT);
	zassert_equal(net_buf_pull_u8(buf), rsp_op);

	return buf;
}

static struct bt_gatt_ots_object *cur_obj_of(uint8_t i)
{
	return ots->cur_obj[bt_conn_index(peers[i].conn)];
}

static bool obj_exists(uint64_t id)
{
	struct bt_gatt_ots_object *obj;

	return bt_gatt_ots_obj_manager_obj_get(ots->obj_manager, id, &obj) == 0;
}

static void cur_obj_is(uint8_t i, uint64_t id)
{
	struct net_buf *buf;

	buf = read_id(i, BT_ATT_OP_READ_RSP);
	zassert_equal(buf->len, BT_OTS_OBJ_ID_SIZE);
	zassert_equal(sys_get_le48(buf->data), id);
	net_buf_unref(buf);
}

static void cur_obj_is_none(uint8_t i)
{
	struct bt_att_error_rsp *rsp;
	struct net_buf *buf;

	buf = read_id(i, BT_ATT_OP_ERROR_RSP);
	rsp = net_buf_pull_mem(buf, sizeof(*rsp));
	zassert_equal(rsp->error, BT_GATT_OTS_OBJECT_NOT_SELECTED);
	net_buf_unref(buf);
}

/* A client reads an object. It has given credits for one SDU, so the second waits for a credit. */
static void read_first_sdu(uint8_t i)
{
	subscribe(i, BT_UUID_OTS_ACTION_CP);
	subscribe(i, BT_UUID_OTS_LIST_CP);
	select_obj(i, OBJ_1);
	connect_chan(i, 1U);
	start_read(i);
	tx_sdu(i, obj_data);
}

/* Both clients have the same object selected, and the first asks to read it. The Host has sent
 * the response to both, and neither has confirmed it yet.
 */
static void read_req_seen_by_both(void)
{
	connect_peer(B);
	subscribe(B, BT_UUID_OTS_ACTION_CP);
	subscribe(B, BT_UUID_OTS_LIST_CP);
	select_obj(B, OBJ_1);

	subscribe(A, BT_UUID_OTS_ACTION_CP);
	subscribe(A, BT_UUID_OTS_LIST_CP);
	select_obj(A, OBJ_1);
	connect_chan(A, 1U);

	send_read_req(A);
	tx_att_op(A, BT_ATT_OP_WRITE_RSP);
	tx_att_op(A, BT_ATT_OP_INDICATE);
	tx_att_op(B, BT_ATT_OP_INDICATE);
}

static void read_last_sdu(uint8_t i)
{
	recv_credits(i, 1U);
	tx_sdu(i, &obj_data[PEER_MTU]);
	zassert_ok(k_sem_take(&read_done_sem, TX_TIMEOUT));
}

static int obj_created(struct bt_ots *inst, struct bt_conn *conn, uint64_t id,
		       const struct bt_ots_obj_add_param *param,
		       struct bt_ots_obj_created_desc *desc)
{
	static char names[CONFIG_BT_OTS_MAX_OBJ_CNT][CONFIG_BT_OTS_OBJ_MAX_NAME_LEN + 1];
	char *name = names[id - BT_OTS_OBJ_ID_MIN];

	ARG_UNUSED(inst);

	/* A client names the objects it creates. It writes to the others after their end. */
	if (conn == NULL) {
		name[0] = (char)('a' + (id - BT_OTS_OBJ_ID_MIN));
		name[1] = '\0';
		desc->size.cur = param->size;
	} else {
		name[0] = '\0';
		desc->size.cur = 0U;
	}

	desc->name = name;
	desc->size.alloc = 2U * param->size;
	BT_OTS_OBJ_SET_PROP_DELETE(desc->props);
	BT_OTS_OBJ_SET_PROP_READ(desc->props);
	BT_OTS_OBJ_SET_PROP_WRITE(desc->props);

	return 0;
}

static int obj_deleted(struct bt_ots *inst, struct bt_conn *conn, uint64_t id)
{
	ARG_UNUSED(inst);
	ARG_UNUSED(id);

	deleted_count++;

	/* The application's own deletes have no connection */
	if (conn != NULL) {
		deleted_conn_index = bt_conn_index(conn);
	}

	return 0;
}

static ssize_t obj_read(struct bt_ots *inst, struct bt_conn *conn, uint64_t id, void **data,
			size_t len, off_t offset)
{
	ARG_UNUSED(inst);
	ARG_UNUSED(conn);
	ARG_UNUSED(id);

	if (data == NULL) {
		k_sem_give(&read_done_sem);

		return 0;
	}

	read_count++;
	*data = &obj_data[offset];

	return (ssize_t)len;
}

static ssize_t obj_write(struct bt_ots *inst, struct bt_conn *conn, uint64_t id, const void *data,
			 size_t len, off_t offset, size_t rem)
{
	ARG_UNUSED(inst);
	ARG_UNUSED(conn);
	ARG_UNUSED(id);

	zassert_equal(offset, write_offset + written_len);
	zassert_true(written_len + len <= sizeof(written));

	(void)memcpy(&written[written_len], data, len);
	written_len += len;
	write_count++;

	if (rem == 0U) {
		k_sem_give(&write_done_sem);
	}

	return (ssize_t)len;
}

static void *setup(void)
{
	static struct bt_ots_cb cb = {
		.obj_created = obj_created,
		.obj_deleted = obj_deleted,
		.obj_read = obj_read,
		.obj_write = obj_write,
	};
	struct bt_ots_init_param param = {
		.cb = &cb,
	};

	ARRAY_FOR_EACH(obj_data, i) {
		obj_data[i] = (uint8_t)i;
	}

	ARRAY_FOR_EACH_PTR(sent, fifo) {
		k_fifo_init(fifo);
	}

	BT_OTS_OACP_SET_FEAT_CREATE(param.features.oacp);
	BT_OTS_OACP_SET_FEAT_DELETE(param.features.oacp);
	BT_OTS_OACP_SET_FEAT_READ(param.features.oacp);
	BT_OTS_OACP_SET_FEAT_WRITE(param.features.oacp);
	BT_OTS_OLCP_SET_FEAT_GO_TO(param.features.olcp);

	zassert_ok(bt_enable(NULL));

	ots = bt_ots_free_instance_get();
	zassert_not_null(ots);
	zassert_ok(bt_ots_init(ots, &param));

	return NULL;
}

static void before(void *fixture)
{
	const struct bt_ots_obj_add_param param = {
		.size = OBJ_SIZE,
		.type = { .uuid_16 = BT_UUID_INIT_16(OBJ_TYPE) },
	};

	ARG_UNUSED(fixture);

	zassert_equal(bt_ots_obj_add(ots, &param), OBJ_1);
	zassert_equal(bt_ots_obj_add(ots, &param), OBJ_2);

	connect_peer(A);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	ARRAY_FOR_EACH(peers, i) {
		if (peers[i].conn != NULL) {
			disconnect_peer(i);
		}
	}

	/* No object may stay locked, and the delete fails for an ID without one */
	for (uint64_t id = OBJ_1; id < OBJ_1 + CONFIG_BT_OTS_MAX_OBJ_CNT; id++) {
		int err = bt_ots_obj_delete(ots, id);

		zassert_true(err == 0 || err == -EINVAL || err == -ENOENT, "err %d", err);
	}

	written_len = 0U;
	write_count = 0U;
	read_count = 0U;
	deleted_count = 0U;
	deleted_conn_index = 0U;
	k_sem_reset(&read_done_sem);
	k_sem_reset(&write_done_sem);

	/* As before the first transfer */
	ots->l2cap.closed = NULL;
}

ZTEST_SUITE(ots_disconnected, NULL, setup, before, after, NULL);

/* A connection that never used the service goes down during a read */
static ZTEST(ots_disconnected, test_other_down_during_read)
{
	read_first_sdu(A);

	connect_peer(B);
	disconnect_peer(B);

	read_last_sdu(A);
	cur_obj_is(A, OBJ_1);
}

/* A connection that never used the service goes down during a write */
static ZTEST(ots_disconnected, test_other_down_during_write)
{
	subscribe(A, BT_UUID_OTS_ACTION_CP);
	subscribe(A, BT_UUID_OTS_LIST_CP);
	select_obj(A, OBJ_1);
	connect_chan(A, 1U);
	start_write(A, OBJ_SIZE);
	recv_write_sdu(A, 0U);

	connect_peer(B);
	disconnect_peer(B);

	recv_write_sdu(A, WRITE_SDU_LEN);
	zassert_ok(k_sem_take(&write_done_sem, TX_TIMEOUT));
	zassert_mem_equal(written, obj_data, WRITE_SIZE);
	cur_obj_is(A, OBJ_1);
}

/* The confirmation of the other client, which comes first, does not start the read */
static ZTEST(ots_disconnected, test_other_confirms_read)
{
	read_req_seen_by_both();

	recv_confirm(B);
	cur_obj_is(B, OBJ_1);
	zassert_equal(read_count, 0U);

	recv_confirm(A);
	tx_sdu(A, obj_data);
	read_last_sdu(A);
}

/* The other client goes down without confirming, which does not end the read */
static ZTEST(ots_disconnected, test_other_down_unconfirmed_read)
{
	read_req_seen_by_both();

	disconnect_peer(B);
	zassert_equal(read_count, 0U);

	recv_confirm(A);
	tx_sdu(A, obj_data);
	read_last_sdu(A);
}

/* Each client has its own selection (OTS 1.0 section 1.12.1) */
static ZTEST(ots_disconnected, test_selection_per_client)
{
	connect_peer(B);
	cur_obj_is_none(A);
	cur_obj_is_none(B);

	subscribe(A, BT_UUID_OTS_LIST_CP);
	select_obj(A, OBJ_1);
	cur_obj_is(A, OBJ_1);
	cur_obj_is_none(B);

	subscribe(B, BT_UUID_OTS_LIST_CP);
	select_obj(B, OBJ_2);
	cur_obj_is(A, OBJ_1);
	cur_obj_is(B, OBJ_2);
}

/* A client keeps its selection when another connection goes down */
static ZTEST(ots_disconnected, test_other_down_selected)
{
	subscribe(A, BT_UUID_OTS_LIST_CP);
	select_obj(A, OBJ_2);

	connect_peer(B);
	disconnect_peer(B);

	cur_obj_is(A, OBJ_2);
}

/* A client can still name the object it created when another connection goes down */
static ZTEST(ots_disconnected, test_other_down_created)
{
	subscribe(A, BT_UUID_OTS_ACTION_CP);
	create_obj(A);

	connect_peer(B);
	disconnect_peer(B);

	zassert_equal(deleted_count, 0U);
	zassert_true(obj_exists(OBJ_CREATED));
	cur_obj_is(A, OBJ_CREATED);
	att_write(A, bt_gatt_attr_get_handle(attr_get(BT_UUID_OTS_NAME)), "x", 1U);
}

/* A client goes down during its own read, which leaves the object free for another client */
static ZTEST(ots_disconnected, test_own_down_during_read)
{
	read_first_sdu(A);

	connect_peer(B);
	disconnect_peer(A);

	cur_obj_is_none(B);
	read_first_sdu(B);
	read_last_sdu(B);
}

/* A selection ends with the connection of the client that made it, and only that one */
static ZTEST(ots_disconnected, test_own_down_selected)
{
	subscribe(A, BT_UUID_OTS_LIST_CP);
	select_obj(A, OBJ_1);

	connect_peer(B);
	subscribe(B, BT_UUID_OTS_LIST_CP);
	select_obj(B, OBJ_2);

	disconnect_peer(A);
	cur_obj_is(B, OBJ_2);

	connect_peer(A);
	cur_obj_is_none(A);
}

/* An object not named by the end of the connection that created it is deleted */
static ZTEST(ots_disconnected, test_own_down_created)
{
	const uint8_t index = bt_conn_index(peers[A].conn);

	subscribe(A, BT_UUID_OTS_ACTION_CP);
	create_obj(A);

	connect_peer(B);
	disconnect_peer(A);

	zassert_equal(deleted_count, 1U);
	zassert_equal(deleted_conn_index, index);
	zassert_false(obj_exists(OBJ_CREATED));
	cur_obj_is_none(B);
}

/* A client deletes the object it created, which ends the reference to its connection */
static ZTEST(ots_disconnected, test_own_delete_created)
{
	const uint8_t req = BT_GATT_OTS_OACP_PROC_DELETE;

	subscribe(A, BT_UUID_OTS_ACTION_CP);
	create_obj(A);
	oacp_proc(A, &req, sizeof(req));
	zassert_equal(deleted_count, 1U);
	cur_obj_is_none(A);

	disconnect_peer(A);
}

/* bt_disable() ends the connections too, with the service's references to them */
static ZTEST(ots_disconnected, test_own_down_on_disable)
{
	subscribe(A, BT_UUID_OTS_ACTION_CP);
	create_obj(A);

	zassert_ok(bt_disable());
	peer_gone(A);

	zassert_equal(deleted_count, 1U);
	zassert_false(obj_exists(OBJ_CREATED));
	zassert_ok(bt_enable(NULL));
}

/* An object named by then stays. A later connection did not create it, even on the same object. */
static ZTEST(ots_disconnected, test_own_down_created_named)
{
	const uint16_t name = bt_gatt_attr_get_handle(attr_get(BT_UUID_OTS_NAME));
	const uint8_t index = bt_conn_index(peers[A].conn);

	subscribe(A, BT_UUID_OTS_ACTION_CP);
	create_obj(A);
	att_write(A, name, "x", 1U);

	disconnect_peer(A);

	zassert_equal(deleted_count, 0U);

	connect_peer(A);
	zassert_equal(bt_conn_index(peers[A].conn), index);
	cur_obj_is_none(A);
	subscribe(A, BT_UUID_OTS_LIST_CP);
	select_obj(A, OBJ_CREATED);
	att_write(A, name, NULL, 0U);

	disconnect_peer(A);

	zassert_equal(deleted_count, 0U);
}

/* An object goes away with the connection that created it, with another client's selection */
static ZTEST(ots_disconnected, test_creator_down_selected_by_other)
{
	subscribe(A, BT_UUID_OTS_ACTION_CP);
	create_obj(A);

	connect_peer(B);
	subscribe(B, BT_UUID_OTS_LIST_CP);
	select_obj(B, OBJ_CREATED);

	disconnect_peer(A);

	zassert_equal(deleted_count, 1U);
	zassert_false(obj_exists(OBJ_CREATED));
	cur_obj_is_none(B);
}

/* An object that another client is writing to stays when its creator's connection goes down, and
 * goes with the connection of that client if it still has no name then
 */
static ZTEST(ots_disconnected, test_creator_down_during_write_by_other)
{
	subscribe(A, BT_UUID_OTS_ACTION_CP);
	create_obj(A);

	connect_peer(B);
	subscribe(B, BT_UUID_OTS_ACTION_CP);
	subscribe(B, BT_UUID_OTS_LIST_CP);
	select_obj(B, OBJ_CREATED);
	connect_chan(B, 1U);
	start_write(B, 0U);
	recv_write_sdu(B, 0U);

	disconnect_peer(A);

	zassert_equal(deleted_count, 0U);
	recv_write_sdu(B, WRITE_SDU_LEN);
	zassert_ok(k_sem_take(&write_done_sem, TX_TIMEOUT));
	cur_obj_is(B, OBJ_CREATED);

	disconnect_peer(B);

	zassert_equal(deleted_count, 1U);
	zassert_false(obj_exists(OBJ_CREATED));
}

/* The next PDU must disconnect the channel */
static void tx_chan_disconnect(uint8_t i)
{
	struct bt_l2cap_disconn_req *disc;
	struct net_buf *buf;
	uint8_t ident;

	buf = tx_sig(i, BT_L2CAP_DISCONN_REQ, &ident);
	disc = net_buf_pull_mem(buf, sizeof(*disc));
	zassert_equal(sys_le16_to_cpu(disc->dcid), PEER_CID);
	net_buf_unref(buf);
}

/* A client writes to an object */
static void write_obj(uint8_t i, uint64_t id)
{
	subscribe(i, BT_UUID_OTS_ACTION_CP);
	subscribe(i, BT_UUID_OTS_LIST_CP);
	select_obj(i, id);
	connect_chan(i, 1U);
	start_write(i, OBJ_SIZE);
	recv_write_sdu(i, 0U);
	recv_write_sdu(i, WRITE_SDU_LEN);
	zassert_ok(k_sem_take(&write_done_sem, TX_TIMEOUT));
}

/* Object data after a write has completed is refused, and closes the channel */
static ZTEST(ots_disconnected, test_write_data_after_write)
{
	size_t count;

	write_obj(A, OBJ_1);
	count = write_count;

	recv_write_sdu(A, 0U);
	tx_chan_disconnect(A);
	zassert_equal(write_count, count);
}

/* The channel closes with no current object, after which another client uses it: object data is
 * refused for that client as well
 */
static ZTEST(ots_disconnected, test_write_data_on_reused_channel)
{
	const uint8_t req = BT_GATT_OTS_OACP_PROC_DELETE;
	size_t count;

	write_obj(A, OBJ_1);
	count = write_count;
	oacp_proc(A, &req, sizeof(req));
	disconnect_peer(A);

	connect_peer(B);
	subscribe(B, BT_UUID_OTS_LIST_CP);
	select_obj(B, OBJ_2);
	connect_chan(B, 1U);

	recv_write_sdu(B, 0U);
	tx_chan_disconnect(B);
	zassert_equal(write_count, count);
}

/* A client selects another object during the read of a second client, and goes down */
static ZTEST(ots_disconnected, test_other_selects_during_read)
{
	read_first_sdu(A);

	connect_peer(B);
	subscribe(B, BT_UUID_OTS_LIST_CP);
	select_obj(B, OBJ_2);
	cur_obj_is(A, OBJ_1);
	disconnect_peer(B);

	read_last_sdu(A);
	cur_obj_is(A, OBJ_1);
}

/* A client selects the object that a second client is reading, and goes down */
static ZTEST(ots_disconnected, test_same_selection_down_during_read)
{
	read_first_sdu(A);

	connect_peer(B);
	subscribe(B, BT_UUID_OTS_LIST_CP);
	select_obj(B, OBJ_1);
	disconnect_peer(B);

	read_last_sdu(A);
	cur_obj_is(A, OBJ_1);
}

/* A client cannot delete the object that a second client is reading, and the response does not
 * start the read again when the second client confirms it
 */
static ZTEST(ots_disconnected, test_other_delete_during_read)
{
	const uint8_t req = BT_GATT_OTS_OACP_PROC_DELETE;

	read_first_sdu(A);

	connect_peer(B);
	subscribe(B, BT_UUID_OTS_ACTION_CP);
	subscribe(B, BT_UUID_OTS_LIST_CP);
	select_obj(B, OBJ_1);
	oacp_proc_rsp(B, &req, sizeof(req), BT_GATT_OTS_OACP_RES_OBJ_LOCKED);
	zassert_equal(deleted_count, 0U);

	/* Reading it also waits for the Host to handle the confirmation */
	cur_obj_is(A, OBJ_1);
	zassert_equal(cur_obj_of(A)->state.type, BT_GATT_OTS_OBJECT_READ_OP_STATE);
	read_last_sdu(A);
	zassert_equal(cur_obj_of(A)->state.type, BT_GATT_OTS_OBJECT_IDLE_STATE);
}

/* A client selects another object during its own read and deletes it, which leaves the read without
 * a current object: the Host closes the channel after the SDU that it has sent. The selection took
 * the read off its object, which stays locked.
 */
static ZTEST(ots_disconnected, test_own_delete_during_read)
{
	const uint8_t req = BT_GATT_OTS_OACP_PROC_DELETE;
	struct bt_gatt_ots_object *obj;

	read_first_sdu(A);
	obj = cur_obj_of(A);

	select_obj(A, OBJ_2);
	oacp_proc(A, &req, sizeof(req));

	recv_credits(A, 1U);
	tx_sdu(A, &obj_data[PEER_MTU]);
	tx_chan_disconnect(A);

	obj->state.type = BT_GATT_OTS_OBJECT_IDLE_STATE;
}

static K_SEM_DEFINE(workq_held, 0, 1);
static K_SEM_DEFINE(workq_release, 0, 1);

static void workq_hold_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_give(&workq_held);
	zassert_ok(k_sem_take(&workq_release, K_SECONDS(1)));
}

/* The connection goes down while the system workqueue is busy, so before the Host indicates the
 * response to a read. Without its own workqueue for TX notifications, the Host waits for the system
 * workqueue before it handles ACL data, which would hold back the request as well.
 */
static ZTEST(ots_disconnected, test_own_down_before_read_indicated)
{
	static K_WORK_DEFINE(hold, workq_hold_handler);
	struct k_work_sync sync;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_CONN_TX_NOTIFY_WQ);

	subscribe(A, BT_UUID_OTS_ACTION_CP);
	subscribe(A, BT_UUID_OTS_LIST_CP);
	select_obj(A, OBJ_1);
	connect_chan(A, 1U);

	zassert_true(k_work_submit(&hold) >= 0);
	zassert_ok(k_sem_take(&workq_held, K_SECONDS(1)));

	send_read_req(A);
	tx_att_op(A, BT_ATT_OP_WRITE_RSP);
	zassert_equal(cur_obj_of(A)->state.type, BT_GATT_OTS_OBJECT_READ_OP_STATE);

	disconnect_peer(A);
	k_sem_give(&workq_release);
	(void)k_work_flush(&ots->oacp_ind.work, &sync);

	/* The object is not left locked */
	connect_peer(A);
	read_first_sdu(A);
	read_last_sdu(A);
}

/* The connection goes down before the client confirms the response to a read */
static ZTEST(ots_disconnected, test_own_down_before_read_confirmed)
{
	subscribe(A, BT_UUID_OTS_ACTION_CP);
	subscribe(A, BT_UUID_OTS_LIST_CP);
	select_obj(A, OBJ_1);
	connect_chan(A, 1U);

	send_read_req(A);
	tx_att_op(A, BT_ATT_OP_WRITE_RSP);
	tx_att_op(A, BT_ATT_OP_INDICATE);
	zassert_equal(cur_obj_of(A)->state.type, BT_GATT_OTS_OBJECT_READ_OP_STATE);

	disconnect_peer(A);

	/* The read did not start, and the object is not left locked */
	zassert_equal(read_count, 0U);
	connect_peer(A);
	read_first_sdu(A);
	read_last_sdu(A);
}
