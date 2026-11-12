/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
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
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/gap.h>
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
#include "host/smp.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of initialization commands that are
 * irrelevant to these tests
 */
#define GENERIC_RP_SIZE 64U

/* The data packets the controller takes: the smallest size an LE controller
 * may have, and a few of them
 */
#define LE_ACL_MAX_LEN 27U
#define LE_ACL_MAX_NUM 3U

/* The handle that the controller gives the connection, any that it may use */
#define TEST_CONN_HANDLE 0x0001U

/* Headers of an ACL packet to the Host, after its H:4 packet type */
#define ACL_HDR_OFFSET   1U
#define L2CAP_HDR_OFFSET (ACL_HDR_OFFSET + sizeof(struct bt_hci_acl_hdr))
#define PAYLOAD_OFFSET   (L2CAP_HDR_OFFSET + sizeof(struct bt_l2cap_hdr))

/* The largest SMP PDU, a Pairing Public Key */
#define SMP_PDU_MAX_LEN (sizeof(struct bt_smp_hdr) + sizeof(struct bt_smp_public_key))

/* A Pairing Request or Response with its code */
#define SMP_PAIRING_LEN (sizeof(struct bt_smp_hdr) + sizeof(struct bt_smp_pairing))

/* A confirm or random value */
#define SMP_VALUE_LEN sizeof(struct bt_smp_pairing_random)

/* The Host answers what it receives before the test goes on, as its threads
 * outrank the test's: the time is for a PDU that goes out once the
 * controller has reported the previous ones as completed, which a work item
 * of the system workqueue does.
 */
#define RSP_TIMEOUT K_SECONDS(1)

/* For bt_workq to get to an event that is queued behind what a test sent: it
 * does so at once unless it waits for a buffer.
 */
#define HANDLED_TIMEOUT K_MSEC(100)

/* The SMP timeout of Core v6.3 Vol 3 Part H 3.4, which a test holds the
 * Host's SMP timer to
 */
#define SMP_TIMEOUT_SEC 30U

/* How long after the SMP timeout a test looks at what the timeout did */
#define TIMEOUT_MARGIN_SEC 1U

/* A test keeps the pool empty for this long, which is less than the SMP
 * timeout
 */
#define POOL_EMPTY_SEC 20U

/* The random value of the peer in the legacy pairing */
static const uint8_t peer_rand[SMP_VALUE_LEN] = {
	0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U,
	0x18U, 0x19U, 0x1aU, 0x1bU, 0x1cU, 0x1dU, 0x1eU, 0x1fU,
};

/* The Pairing Request of the peer: no input or output, no OOB data, bonding,
 * the largest key, and only the Host's LTK to be distributed
 */
static const uint8_t peer_pairing_req[SMP_PAIRING_LEN] = {
	BT_SMP_CMD_PAIRING_REQ, BT_SMP_IO_NO_INPUT_OUTPUT, BT_SMP_OOB_NOT_PRESENT,
	BT_SMP_AUTH_BONDING,    BT_SMP_MAX_ENC_KEY_SIZE,   0x00U,
	BT_SMP_DIST_ENC_KEY,
};

/* An SMP PDU that the Host sent */
struct smp_pdu {
	uint16_t len;
	uint8_t data[SMP_PDU_MAX_LEN];
};

/* More than any test lets the Host send before it takes them */
#define SMP_TX_MSGQ_LEN 4U

K_MSGQ_DEFINE(smp_tx_msgq, sizeof(struct smp_pdu), SMP_TX_MSGQ_LEN, __alignof__(struct smp_pdu));

/* The data packets that the controller holds without having reported them as
 * completed, and what the controller does not expect: a packet that the
 * tests do not handle, or a failure
 */
static atomic_t acl_pending;
static atomic_t unexpected_count;

/* The Host replied to the LE Long Term Key Request */
static K_SEM_DEFINE(ltk_reply_sem, 0, 1);

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
	rp->bdaddr = (bt_addr_t){ .val = { 0xa1U, 0xa2U, 0xa3U, 0xa4U, 0xa5U, 0xa6U } };
}

static void le_read_local_features(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE Encryption */
	rp->features[0] = BIT(0);
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

static void cmd_from_host(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *hdr;
	uint16_t opcode;
	int err;

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
	case BT_HCI_OP_LE_READ_LOCAL_FEATURES:
		le_read_local_features(&evt, opcode);
		break;
	case BT_HCI_OP_LE_READ_BUFFER_SIZE:
		le_read_buffer_size(&evt, opcode);
		break;
	case BT_HCI_OP_LE_READ_SUPP_STATES:
		le_read_supp_states(&evt, opcode);
		break;
	case BT_HCI_OP_LE_LTK_REQ_REPLY:
		k_sem_give(&ltk_reply_sem);
		generic_success(&evt, opcode);
		break;
	default:
		generic_success(&evt, opcode);
		break;
	}

	err = bt_hci_recv_err(dev, evt);
	if (err != 0) {
		(void)atomic_inc(&unexpected_count);
	}
}

static void acl_from_host(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *hdr;
	struct smp_pdu pdu;
	uint16_t handle;
	uint16_t len;
	uint16_t cid;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	handle = sys_le16_to_cpu(hdr->handle);

	if (bt_acl_flags_pb(bt_acl_flags(handle)) != BT_ACL_START_NO_FLUSH ||
	    buf->len < BT_L2CAP_HDR_SIZE) {
		(void)atomic_inc(&unexpected_count);
		return;
	}

	len = net_buf_pull_le16(buf);
	cid = net_buf_pull_le16(buf);

	if (len != buf->len || cid != BT_L2CAP_CID_SMP || buf->len == 0U ||
	    buf->len > sizeof(pdu.data)) {
		(void)atomic_inc(&unexpected_count);
		return;
	}

	pdu.len = buf->len;
	(void)memcpy(pdu.data, buf->data, buf->len);

	if (k_msgq_put(&smp_tx_msgq, &pdu, K_NO_WAIT) != 0) {
		(void)atomic_inc(&unexpected_count);
	}
}

static void num_completed_packets(uint16_t count)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_evt_num_completed_packets *evt;
	struct bt_hci_handle_count *hc;
	struct net_buf *buf;
	int err;

	buf = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_NUM_COMPLETED_PACKETS, sizeof(*evt) + sizeof(*hc));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->num_handles = 1U;
	hc = net_buf_add(buf, sizeof(*hc));
	hc->handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	hc->count = sys_cpu_to_le16(count);

	err = bt_hci_recv_err(dev, buf);
	if (err != 0) {
		(void)atomic_inc(&unexpected_count);
	}
}

/* The controller reports what it holds as completed */
static void completed_handler(struct k_work *work)
{
	atomic_val_t count;

	ARG_UNUSED(work);

	count = atomic_set(&acl_pending, 0);
	if (count > 0) {
		num_completed_packets((uint16_t)count);
	}
}

static void completed_submit(void)
{
	static K_WORK_DEFINE(completed_work, completed_handler);

	if (k_work_submit(&completed_work) < 0) {
		(void)atomic_inc(&unexpected_count);
	}
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/* The controller drops what it holds when the transport is closed */
static int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	(void)atomic_set(&acl_pending, 0);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	if (type == BT_HCI_H4_CMD) {
		cmd_from_host(dev, buf);
	} else if (type == BT_HCI_H4_ACL) {
		acl_from_host(buf);
		(void)atomic_inc(&acl_pending);
		completed_submit();
	} else {
		(void)atomic_inc(&unexpected_count);
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
	static struct bt_hci_driver_data driver_data_##inst = { 0 };                               \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static K_SEM_DEFINE(recycled_sem, 0, 1);
static K_SEM_DEFINE(param_updated_sem, 0, 1);
static struct bt_conn *test_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0U) {
		return;
	}

	test_conn = bt_conn_ref(conn);
	k_sem_give(&connected_sem);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARGS_UNUSED(conn, reason);

	k_sem_give(&disconnected_sem);
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
			     uint16_t timeout)
{
	ARGS_UNUSED(conn, interval, latency, timeout);

	k_sem_give(&param_updated_sem);
}

static void recycled(void)
{
	k_sem_give(&recycled_sem);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_updated = le_param_updated,
	.recycled = recycled,
};

/* The outcome of the pairing */
static K_SEM_DEFINE(pairing_complete_sem, 0, 1);
static K_SEM_DEFINE(pairing_failed_sem, 0, 1);
static bool is_bonded;
static enum bt_security_err pairing_err;

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	ARG_UNUSED(conn);

	is_bonded = bonded;
	k_sem_give(&pairing_complete_sem);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	ARG_UNUSED(conn);

	pairing_err = reason;
	k_sem_give(&pairing_failed_sem);
}

static struct bt_conn_auth_info_cb auth_info_cb = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

static void evt_recv(struct net_buf *buf)
{
	zassert_ok(bt_hci_recv_err(DEVICE_DT_GET(DT_DRV_INST(0)), buf),
		   "The Host refused a packet");
}

static void le_meta_evt_send(uint8_t subevent, const void *params, uint8_t len)
{
	struct bt_hci_evt_le_meta_event *meta;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + len);
	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = subevent;
	(void)net_buf_add_mem(buf, params, len);
	evt_recv(buf);
}

static void connect_as_peripheral(void)
{
	const bt_addr_le_t peer = {
		.type = BT_ADDR_LE_RANDOM,
		.a.val = { 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0xc6U },
	};
	struct bt_hci_evt_le_conn_complete evt;

	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");

	(void)memset(&evt, 0, sizeof(evt));
	evt.status = BT_HCI_ERR_SUCCESS;
	evt.handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	evt.role = BT_HCI_ROLE_PERIPHERAL;
	bt_addr_le_copy(&evt.peer_addr, &peer);
	evt.interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MIN);
	evt.supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U));
	le_meta_evt_send(BT_HCI_EVT_LE_CONN_COMPLETE, &evt, sizeof(evt));

	zassert_ok(k_sem_take(&connected_sem, RSP_TIMEOUT), "No connection");
}

/* The connection ends. The controller drops what it holds for it. */
static int disconnect(void)
{
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;
	int err;

	(void)atomic_set(&acl_pending, 0);

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	evt->reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
	evt_recv(buf);

	err = k_sem_take(&disconnected_sem, RSP_TIMEOUT);
	if (err != 0) {
		return err;
	}

	bt_conn_drop(&test_conn);

	return k_sem_take(&recycled_sem, RSP_TIMEOUT);
}

/* bt_workq has handled what the peer sent before: it handles that before the
 * event that follows, for which it calls the application.
 */
static void handled_expect(void)
{
	struct bt_hci_evt_le_conn_update_complete evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.handle = sys_cpu_to_le16(TEST_CONN_HANDLE),
		.interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MAX),
		.latency = 0U,
		.supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U)),
	};

	le_meta_evt_send(BT_HCI_EVT_LE_CONN_UPDATE_COMPLETE, &evt, sizeof(evt));

	zassert_ok(k_sem_take(&param_updated_sem, HANDLED_TIMEOUT), "bt_workq is blocked");
}

/* The peer sends an SMP PDU */
static void smp_recv(const uint8_t *data, size_t len)
{
	struct net_buf *buf;
	struct bt_hci_acl_hdr *acl;
	struct bt_l2cap_hdr *hdr;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	zassert_not_null(buf, "No buffer for a data packet from the peer");
	(void)net_buf_add(buf, sizeof(*acl) + sizeof(*hdr));
	(void)net_buf_add_mem(buf, data, len);

	acl = (void *)&buf->data[ACL_HDR_OFFSET];
	hdr = (void *)&buf->data[L2CAP_HDR_OFFSET];
	hdr->len = sys_cpu_to_le16(buf->len - PAYLOAD_OFFSET);
	hdr->cid = sys_cpu_to_le16(BT_L2CAP_CID_SMP);
	acl->handle = sys_cpu_to_le16(bt_acl_handle_pack(TEST_CONN_HANDLE, BT_ACL_START));
	acl->len = sys_cpu_to_le16(buf->len - L2CAP_HDR_OFFSET);

	evt_recv(buf);
}

/* The peer sends a Pairing Confirm or a Pairing Random */
static void smp_value_recv(uint8_t code, const uint8_t value[SMP_VALUE_LEN])
{
	uint8_t pdu[sizeof(struct bt_smp_hdr) + SMP_VALUE_LEN];

	pdu[0] = code;
	(void)memcpy(&pdu[sizeof(struct bt_smp_hdr)], value, SMP_VALUE_LEN);
	smp_recv(pdu, sizeof(pdu));
}

/* The Host sends an SMP PDU */
static void smp_expect(uint8_t code, struct smp_pdu *pdu)
{
	zassert_ok(k_msgq_get(&smp_tx_msgq, pdu, RSP_TIMEOUT), "No SMP PDU 0x%02x", code);
	zassert_equal(pdu->data[0], code, "SMP PDU 0x%02x instead of 0x%02x", pdu->data[0], code);
}

static void nothing_sent_expect(void)
{
	zassert_equal(k_msgq_num_used_get(&smp_tx_msgq), 0U, "The Host sent an SMP PDU");
}

/* c1 of the legacy pairing (Core v6.3 Vol 3 Part H 2.2.3), in the byte order
 * of SMP
 */
static void c1(const uint8_t k[SMP_VALUE_LEN], const uint8_t r[SMP_VALUE_LEN],
	       const uint8_t preq[SMP_PAIRING_LEN], const uint8_t pres[SMP_PAIRING_LEN],
	       const bt_addr_le_t *ia, const bt_addr_le_t *ra, uint8_t out[SMP_VALUE_LEN])
{
	uint8_t p1[SMP_VALUE_LEN];
	uint8_t p2[SMP_VALUE_LEN];

	p1[0] = ia->type;
	p1[1] = ra->type;
	(void)memcpy(&p1[2], preq, SMP_PAIRING_LEN);
	(void)memcpy(&p1[2U + SMP_PAIRING_LEN], pres, SMP_PAIRING_LEN);

	(void)memcpy(p2, ra->a.val, sizeof(ra->a.val));
	(void)memcpy(&p2[sizeof(ra->a.val)], ia->a.val, sizeof(ia->a.val));
	(void)memset(&p2[2U * sizeof(ia->a.val)], 0, sizeof(p2) - (2U * sizeof(ia->a.val)));

	for (size_t i = 0U; i < sizeof(p1); i++) {
		out[i] = r[i] ^ p1[i];
	}

	zassert_ok(bt_encrypt_le(k, out, out), "Encryption failed");

	for (size_t i = 0U; i < sizeof(p2); i++) {
		out[i] ^= p2[i];
	}

	zassert_ok(bt_encrypt_le(k, out, out), "Encryption failed");
}

/* The peer, as central, pairs with legacy Just Works up to the Host's Pairing
 * Random, which leaves the Host waiting for encryption with the STK
 */
static void pairing_to_random(void)
{
	/* The TK of Just Works */
	static const uint8_t tk[SMP_VALUE_LEN];
	uint8_t confirm[SMP_VALUE_LEN];
	struct smp_pdu pdu;

	smp_recv(peer_pairing_req, sizeof(peer_pairing_req));
	smp_expect(BT_SMP_CMD_PAIRING_RSP, &pdu);
	zassert_equal(pdu.len, SMP_PAIRING_LEN, "Pairing Response of %u octets", pdu.len);

	c1(tk, peer_rand, peer_pairing_req, pdu.data, &test_conn->le.init_addr,
	   &test_conn->le.resp_addr, confirm);

	smp_value_recv(BT_SMP_CMD_PAIRING_CONFIRM, confirm);
	smp_expect(BT_SMP_CMD_PAIRING_CONFIRM, &pdu);

	smp_value_recv(BT_SMP_CMD_PAIRING_RANDOM, peer_rand);
	smp_expect(BT_SMP_CMD_PAIRING_RANDOM, &pdu);
}

/* The central starts encryption with the STK */
static void ltk_request_send(void)
{
	struct bt_hci_evt_le_ltk_request evt;

	(void)memset(&evt, 0, sizeof(evt));
	evt.handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	le_meta_evt_send(BT_HCI_EVT_LE_LTK_REQUEST, &evt, sizeof(evt));

	zassert_ok(k_sem_take(&ltk_reply_sem, RSP_TIMEOUT), "No reply to the LTK request");
}

static void encrypt_change_send(void)
{
	struct bt_hci_evt_encrypt_change *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_ENCRYPT_CHANGE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_ENCRYPT_CHANGE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	evt->encrypt = 0x01U;
	evt_recv(buf);
}

/* The buffers of the pool that SMP allocates from, while the test holds them */
static struct net_buf *held[CONFIG_BT_L2CAP_TX_BUF_COUNT];

static void pool_hold(void)
{
	ARRAY_FOR_EACH(held, i) {
		held[i] = bt_l2cap_create_pdu_timeout(NULL, 0U, K_NO_WAIT);
		zassert_not_null(held[i], "Buffer %zu of the pool is in use", i);
	}

	zassert_is_null(bt_l2cap_create_pdu_timeout(NULL, 0U, K_NO_WAIT), "The pool is not empty");
}

/* One buffer goes back to the pool */
static void pool_release_one(void)
{
	ARRAY_FOR_EACH(held, i) {
		if (held[i] != NULL) {
			net_buf_drop(&held[i]);
			return;
		}
	}

	ztest_test_fail();
}

static void pool_release(void)
{
	ARRAY_FOR_EACH(held, i) {
		net_buf_drop(&held[i]);
	}
}

static void *setup(void)
{
	zassert_ok(bt_conn_auth_info_cb_register(&auth_info_cb), "Callbacks not registered");
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&param_updated_sem);
	k_sem_reset(&ltk_reply_sem);
	k_sem_reset(&pairing_complete_sem);
	k_sem_reset(&pairing_failed_sem);
	k_msgq_purge(&smp_tx_msgq);
	(void)atomic_clear(&unexpected_count);
	is_bonded = false;
	pairing_err = BT_SECURITY_ERR_SUCCESS;

	connect_as_peripheral();
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* What a failed test leaves behind */
	pool_release();

	if (test_conn != NULL) {
		zassert_ok(disconnect(), "The connection did not go");
	}

	zassert_ok(bt_unpair(BT_ID_DEFAULT, NULL), "The bonds were not removed");
	zassert_equal(atomic_get(&unexpected_count), 0, "The controller got %ld unexpected packets",
		      atomic_get(&unexpected_count));
}

static void teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_ok(bt_conn_auth_info_cb_unregister(&auth_info_cb), "Callbacks not unregistered");
}

ZTEST_SUITE(smp_buf, NULL, setup, before, after, teardown);

/* The Host owes its keys while the pool is empty, for less than the SMP
 * timeout: bt_workq goes on, and the keys go out once a buffer is back
 */
static ZTEST(smp_buf, test_key_distribution_without_buffers)
{
	struct smp_pdu pdu;

	pairing_to_random();
	ltk_request_send();

	pool_hold();
	encrypt_change_send();
	handled_expect();

	/* Nothing ends the pairing while the pool is empty for less than the
	 * SMP timeout, which runs from the time the keys became owed
	 */
	zassert_equal(k_sem_take(&pairing_failed_sem, K_SECONDS(POOL_EMPTY_SEC)), -EAGAIN,
		      "The pairing failed");
	nothing_sent_expect();

	/* The buffer that went out with the first key comes back for the
	 * second one
	 */
	pool_release_one();
	smp_expect(BT_SMP_CMD_ENCRYPT_INFO, &pdu);
	smp_expect(BT_SMP_CMD_CENTRAL_IDENT, &pdu);

	zassert_ok(k_sem_take(&pairing_complete_sem, RSP_TIMEOUT), "The pairing did not complete");
	zassert_true(is_bonded, "No bond");
}

/* A Pairing Request while the pool is empty: the Host's response waits for a
 * buffer, bt_workq does not
 */
static ZTEST(smp_buf, test_request_without_buffers)
{
	struct smp_pdu pdu;

	pool_hold();
	smp_recv(peer_pairing_req, sizeof(peer_pairing_req));
	handled_expect();
	nothing_sent_expect();

	pool_release_one();
	smp_expect(BT_SMP_CMD_PAIRING_RSP, &pdu);
}

/* The SMP timer runs while a PDU is owed: it ends the pairing, and the PDU is
 * not sent afterwards
 */
static ZTEST(smp_buf, test_timeout_while_owed)
{
	struct smp_pdu pdu;
	int64_t start;

	pool_hold();
	start = k_uptime_get();
	smp_recv(peer_pairing_req, sizeof(peer_pairing_req));
	handled_expect();

	zassert_ok(k_sem_take(&pairing_failed_sem, K_SECONDS(SMP_TIMEOUT_SEC + TIMEOUT_MARGIN_SEC)),
		   "The pairing did not fail");
	zassert_true(k_uptime_get() - start >= (SMP_TIMEOUT_SEC * MSEC_PER_SEC),
		     "The pairing failed before the SMP timeout");
	zassert_equal(pairing_err, BT_SECURITY_ERR_UNSPECIFIED, "Pairing failed with %d",
		      pairing_err);

	pool_release();
	zassert_not_equal(k_msgq_get(&smp_tx_msgq, &pdu, RSP_TIMEOUT), 0,
			  "SMP PDU 0x%02x sent after the timeout", pdu.data[0]);
}
