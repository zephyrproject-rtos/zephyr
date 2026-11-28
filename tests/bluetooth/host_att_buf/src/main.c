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
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/att_internal.h"
#include "host/conn_internal.h"
#include "host/hci_core.h"
#include "host/l2cap_internal.h"

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

/* Set Controller To Host Flow Control, Host Buffer Size and Host Number Of
 * Completed Packets in the supported commands (Core v6.3 Vol 4 Part E 6.27)
 */
#define HOST_FC_CMDS_OCTET 10U
#define HOST_FC_CMDS_MASK  (BIT(5) | BIT(6) | BIT(7))

/* The handles that the controller gives the connection of the tests, and two
 * others: one that goes and one that comes while a request is processed
 */
#define TEST_CONN_HANDLE  0x0001U
#define OTHER_CONN_HANDLE 0x0002U
#define NEW_CONN_HANDLE   0x0003U

/* Headers of an ACL packet to the Host, after its H:4 packet type */
#define ACL_HDR_OFFSET   1U
#define L2CAP_HDR_OFFSET (ACL_HDR_OFFSET + sizeof(struct bt_hci_acl_hdr))
#define PAYLOAD_OFFSET   (L2CAP_HDR_OFFSET + sizeof(struct bt_l2cap_hdr))

/* The bearers: the unenhanced one, then the enhanced ones that the peer opens */
#define UATT         0U
#define BEARER_COUNT (1U + CONFIG_BT_EATT_MAX)
#define EATT_MTU     BT_L2CAP_ECRED_MIN_MTU

/* The peer's CID for its end of the first enhanced bearer, the first of the LE
 * dynamic range (Core v6.3 Vol 3 Part A 2.1), and the identifier of its first
 * signaling request: values that the peer picks
 */
#define PEER_CID   0x0040U
#define PEER_IDENT 0x80U

/* Credits of the peer that let the Host send all that a test makes it send */
#define PEER_CREDITS 100U

/* The characteristics that the tests read and write, each with a value of its
 * own
 */
#define TEST_CHRC_COUNT 3U
#define TEST_VALUE_LEN  4U

/* A handle of the peer's database, which the peer indicates */
#define PEER_HANDLE 0x0003U

/* An opcode that ATT does not define, without the command flag */
#define UNKNOWN_REQ_OP 0x30U

/* The Host answers what it receives before the test goes on, as its threads
 * outrank the test's: the time is for a response that the test lets out
 * through a work item of the system workqueue.
 */
#define RSP_TIMEOUT K_SECONDS(1)

/* For bt_workq to get to an event that is queued behind what a test sent: it
 * does so at once unless it waits for a buffer, which took 30 s.
 */
#define HANDLED_TIMEOUT K_MSEC(100)

/* How long after a timeout of the Host a test looks at what the timeout did */
#define TIMEOUT_MARGIN_SEC 1

static const uint8_t initial_value[TEST_VALUE_LEN] = { 0x01U, 0x02U, 0x03U, 0x04U };
static const uint8_t new_value[TEST_VALUE_LEN] = { 0x05U, 0x06U, 0x07U, 0x08U };

struct test_chrc {
	uint8_t value[TEST_VALUE_LEN];
};

static struct test_chrc chrcs[TEST_CHRC_COUNT];

/* What the read callback fails with, if anything */
static uint8_t read_err;

/* How often the application's callbacks ran, a Prepare Write Request's
 * check counted apart
 */
static atomic_t read_count;
static atomic_t write_count;
static atomic_t prepare_count;

static ssize_t test_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	struct test_chrc *chrc = attr->user_data;

	(void)atomic_inc(&read_count);

	if (read_err != 0U) {
		return BT_GATT_ERR(read_err);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, chrc->value, sizeof(chrc->value));
}

static ssize_t test_write(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	struct test_chrc *chrc = attr->user_data;

	ARG_UNUSED(conn);

	if (offset != 0U || len != sizeof(chrc->value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* A Prepare Write Request only asks whether the write would be allowed */
	if ((flags & BT_GATT_WRITE_FLAG_PREPARE) != 0U) {
		(void)atomic_inc(&prepare_count);
		return 0;
	}

	(void)memcpy(chrc->value, buf, len);
	(void)atomic_inc(&write_count);

	return len;
}

#define TEST_CHRC(_uuid, _chrc)                                                                    \
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(_uuid),                                          \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |                            \
				       BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY |     \
				       BT_GATT_CHRC_INDICATE,                                      \
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |                            \
				       BT_GATT_PERM_PREPARE_WRITE,                                 \
			       test_read, test_write, _chrc)

BT_GATT_SERVICE_DEFINE(test_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0xfff0)),
	TEST_CHRC(0xfff1, &chrcs[0]),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	TEST_CHRC(0xfff2, &chrcs[1]),
	TEST_CHRC(0xfff3, &chrcs[2]),
);

/* The attribute of each characteristic's value */
static const struct bt_gatt_attr *const value_attrs[TEST_CHRC_COUNT] = {
	&test_svc.attrs[2],
	&test_svc.attrs[5],
	&test_svc.attrs[7],
};

static uint16_t value_handles[TEST_CHRC_COUNT];

/* An enhanced bearer, by the CIDs of its two ends, with the credits that the
 * Host has given the peer for it
 */
struct eatt_bearer {
	uint16_t peer_cid;
	uint16_t host_cid;
	atomic_t credits;
};

static struct eatt_bearer eatt[CONFIG_BT_EATT_MAX];
static uint16_t ecred_result;
static K_SEM_DEFINE(ecred_rsp_sem, 0, 1);
static K_SEM_DEFINE(disconn_rsp_sem, 0, 1);

/* An ATT PDU that the Host sent, other than a notification, numbered among
 * all the data packets that the controller got
 */
struct att_pdu {
	uint16_t handle;
	uint8_t bearer;
	uint16_t len;
	atomic_val_t seq;
	uint8_t data[BT_ATT_DEFAULT_LE_MTU];
};

/* More than any test lets the Host send before it takes them */
#define ATT_TX_MSGQ_LEN 8U

K_MSGQ_DEFINE(att_tx_msgq, sizeof(struct att_pdu), ATT_TX_MSGQ_LEN, __alignof__(struct att_pdu));

/* The data packets that the controller holds without having reported them as
 * completed, for the connection of the tests and for the others, and whether
 * it withholds those reports. The number of the last
 * notification among the data packets that it got. The packets from the peer
 * that the Host has reported as completed. And what the controller does not
 * expect: a packet that the tests do not handle, or a failure.
 */
static atomic_t acl_pending;
static atomic_t other_acl_pending;
static atomic_t credits_withheld;
static atomic_t notify_seq;
static atomic_t host_completed;
static atomic_t unexpected_count;

/* A controller without the commands of Controller to Host flow control, and
 * how often the Host has turned the flow control on
 */
static bool is_host_fc_missing;
static atomic_t host_fc_enable_count;

/* The Host asked the controller to disconnect */
static K_SEM_DEFINE(hci_disconnect_sem, 0, 1);

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

	if (is_host_fc_missing) {
		rp->commands[HOST_FC_CMDS_OCTET] &= (uint8_t)~HOST_FC_CMDS_MASK;
	}
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
	rp->le_max_num = LE_ACL_MAX_NUM;
}

static void le_read_supp_states(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
}

/* The packets from the peer that the Host reports as completed */
static void host_num_completed_packets(struct net_buf *cmd)
{
	struct bt_hci_cp_host_num_completed_packets *cp;

	cp = net_buf_pull_mem(cmd, sizeof(*cp));

	for (uint8_t i = 0U; i < cp->num_handles; i++) {
		struct bt_hci_handle_count *hc = net_buf_pull_mem(cmd, sizeof(*hc));

		(void)atomic_add(&host_completed, sys_le16_to_cpu(hc->count));
	}
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
		host_num_completed_packets(cmd);
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
	case BT_HCI_OP_SET_CTL_TO_HOST_FLOW:
		(void)atomic_inc(&host_fc_enable_count);
		generic_success(&evt, opcode);
		break;
	case BT_HCI_OP_DISCONNECT:
		k_sem_give(&hci_disconnect_sem);
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

static struct eatt_bearer *eatt_by_host_cid(uint16_t cid)
{
	ARRAY_FOR_EACH_PTR(eatt, bearer) {
		if (bearer->host_cid == cid) {
			return bearer;
		}
	}

	return NULL;
}

/* A signalling PDU from the Host, of those that the tests make it send */
static void sig_from_host(struct net_buf *buf)
{
	struct bt_l2cap_ecred_conn_rsp *rsp;
	struct bt_l2cap_le_credits *ev;
	struct bt_l2cap_sig_hdr *hdr;
	struct eatt_bearer *bearer;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));

	switch (hdr->code) {
	case BT_L2CAP_ECRED_CONN_RSP:
		rsp = net_buf_pull_mem(buf, sizeof(*rsp));
		ecred_result = sys_le16_to_cpu(rsp->result);
		ARRAY_FOR_EACH_PTR(eatt, b) {
			b->host_cid = net_buf_pull_le16(buf);
			(void)atomic_set(&b->credits, sys_le16_to_cpu(rsp->credits));
		}
		k_sem_give(&ecred_rsp_sem);
		break;
	case BT_L2CAP_LE_CREDITS:
		ev = net_buf_pull_mem(buf, sizeof(*ev));
		bearer = eatt_by_host_cid(sys_le16_to_cpu(ev->cid));
		if (bearer == NULL) {
			(void)atomic_inc(&unexpected_count);
			break;
		}
		(void)atomic_add(&bearer->credits, sys_le16_to_cpu(ev->credits));
		break;
	case BT_L2CAP_DISCONN_RSP:
		k_sem_give(&disconn_rsp_sem);
		break;
	default:
		(void)atomic_inc(&unexpected_count);
		break;
	}
}

/* The bearer that an L2CAP PDU from the Host is for, with the SDU length of a
 * K-frame pulled. Returns false for no bearer.
 */
static bool att_bearer_get(uint16_t cid, struct net_buf *buf, uint8_t *bearer)
{
	if (cid == BT_L2CAP_CID_ATT) {
		*bearer = UATT;
		return true;
	}

	ARRAY_FOR_EACH(eatt, i) {
		uint16_t sdu_len;

		if (eatt[i].peer_cid != cid) {
			continue;
		}

		if (buf->len < sizeof(sdu_len)) {
			return false;
		}

		sdu_len = net_buf_pull_le16(buf);
		if (sdu_len != buf->len) {
			return false;
		}

		*bearer = i + 1U;
		return true;
	}

	return false;
}

/* Returns the handle of the connection that the packet is for */
static uint16_t acl_from_host(struct net_buf *buf)
{
	/* The data packets that the controller got */
	static atomic_t acl_seq;
	struct bt_hci_acl_hdr *hdr;
	struct att_pdu pdu;
	uint16_t handle;
	uint16_t len;
	uint16_t cid;

	pdu.seq = atomic_inc(&acl_seq) + 1;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	handle = sys_le16_to_cpu(hdr->handle);
	pdu.handle = bt_acl_handle(handle);

	if (bt_acl_flags_pb(bt_acl_flags(handle)) != BT_ACL_START_NO_FLUSH ||
	    buf->len < BT_L2CAP_HDR_SIZE) {
		(void)atomic_inc(&unexpected_count);
		return pdu.handle;
	}

	len = net_buf_pull_le16(buf);
	cid = net_buf_pull_le16(buf);

	if (len != buf->len) {
		(void)atomic_inc(&unexpected_count);
		return pdu.handle;
	}

	if (cid == BT_L2CAP_CID_LE_SIG) {
		sig_from_host(buf);
		return pdu.handle;
	}

	if (!att_bearer_get(cid, buf, &pdu.bearer) || buf->len == 0U ||
	    buf->len > sizeof(pdu.data)) {
		(void)atomic_inc(&unexpected_count);
		return pdu.handle;
	}

	/* Notifications are only there to use buffers and controller credits */
	if (buf->data[0] == BT_ATT_OP_NOTIFY) {
		(void)atomic_set(&notify_seq, pdu.seq);
		return pdu.handle;
	}

	pdu.len = buf->len;
	(void)memcpy(pdu.data, buf->data, buf->len);

	if (k_msgq_put(&att_tx_msgq, &pdu, K_NO_WAIT) != 0) {
		(void)atomic_inc(&unexpected_count);
	}

	return pdu.handle;
}

static void num_completed_packets(uint16_t handle, uint16_t count)
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
	hc->handle = sys_cpu_to_le16(handle);
	hc->count = sys_cpu_to_le16(count);

	err = bt_hci_recv_err(dev, buf);
	if (err != 0) {
		(void)atomic_inc(&unexpected_count);
	}
}

/* The controller reports what it holds as completed, unless it withholds the
 * reports, which gives the Host no credits for more data packets
 */
static void completed_handler(struct k_work *work)
{
	atomic_val_t count;

	ARG_UNUSED(work);

	if (atomic_get(&credits_withheld) != 0) {
		return;
	}

	count = atomic_set(&acl_pending, 0);
	if (count > 0) {
		num_completed_packets(TEST_CONN_HANDLE, (uint16_t)count);
	}

	count = atomic_set(&other_acl_pending, 0);
	if (count > 0) {
		num_completed_packets(OTHER_CONN_HANDLE, (uint16_t)count);
	}
}

static void completed_submit(void)
{
	static K_WORK_DEFINE(completed_work, completed_handler);

	if (k_work_submit(&completed_work) < 0) {
		(void)atomic_inc(&unexpected_count);
	}
}

static void credits_withhold(void)
{
	(void)atomic_set(&credits_withheld, 1);
}

static void credits_release(void)
{
	(void)atomic_set(&credits_withheld, 0);
	completed_submit();
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
	(void)atomic_set(&other_acl_pending, 0);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	if (type == BT_HCI_H4_CMD) {
		cmd_from_host(dev, buf);
	} else if (type == BT_HCI_H4_ACL) {
		if (acl_from_host(buf) == TEST_CONN_HANDLE) {
			(void)atomic_inc(&acl_pending);
		} else {
			(void)atomic_inc(&other_acl_pending);
		}

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
static K_SEM_DEFINE(other_connected_sem, 0, 1);
static K_SEM_DEFINE(other_disconnected_sem, 0, 1);
static K_SEM_DEFINE(recycled_sem, 0, 1);
static K_SEM_DEFINE(param_updated_sem, 0, 1);
static struct bt_conn *test_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	uint16_t handle;

	if (err != 0U) {
		return;
	}

	if (bt_hci_get_conn_handle(conn, &handle) != 0) {
		(void)atomic_inc(&unexpected_count);
		return;
	}

	if (handle == TEST_CONN_HANDLE) {
		test_conn = bt_conn_ref(conn);
		k_sem_give(&connected_sem);
	} else {
		k_sem_give(&other_connected_sem);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	if (conn == test_conn) {
		k_sem_give(&disconnected_sem);
	} else {
		k_sem_give(&other_disconnected_sem);
	}
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

static void adv_start(void)
{
	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");
}

/* A peer connects to the advertiser, from an address of its own */
static void conn_complete_send(uint16_t handle)
{
	const bt_addr_le_t peer = {
		.type = BT_ADDR_LE_RANDOM,
		.a.val = { (uint8_t)handle, 0x02U, 0x03U, 0x04U, 0x05U, 0xc6U },
	};
	struct bt_hci_evt_le_conn_complete evt;

	(void)memset(&evt, 0, sizeof(evt));
	evt.status = BT_HCI_ERR_SUCCESS;
	evt.handle = sys_cpu_to_le16(handle);
	evt.role = BT_HCI_ROLE_PERIPHERAL;
	bt_addr_le_copy(&evt.peer_addr, &peer);
	evt.interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MIN);
	evt.supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U));
	le_meta_evt_send(BT_HCI_EVT_LE_CONN_COMPLETE, &evt, sizeof(evt));
}

static void connect_as_peripheral(void)
{
	adv_start();
	conn_complete_send(TEST_CONN_HANDLE);

	zassert_ok(k_sem_take(&connected_sem, RSP_TIMEOUT), "No connection");
}

/* A connection ends. The controller drops what it holds for it: the data
 * packets that it holds are all for the connection of the tests.
 */
static void disconn_complete_send(uint16_t handle)
{
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	if (handle == TEST_CONN_HANDLE) {
		(void)atomic_set(&acl_pending, 0);
	} else {
		(void)atomic_set(&other_acl_pending, 0);
	}

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(handle);
	evt->reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
	evt_recv(buf);
}

/* The connection is gone and released */
static int disconnected_wait(void)
{
	int err;

	err = k_sem_take(&disconnected_sem, RSP_TIMEOUT);
	if (err != 0) {
		return err;
	}

	bt_conn_drop(&test_conn);

	return k_sem_take(&recycled_sem, RSP_TIMEOUT);
}

static int disconnect(void)
{
	disconn_complete_send(TEST_CONN_HANDLE);

	return disconnected_wait();
}

/* Another connection ends, and is released */
static int other_disconnect(uint16_t handle)
{
	int err;

	disconn_complete_send(handle);

	err = k_sem_take(&other_disconnected_sem, RSP_TIMEOUT);
	if (err != 0) {
		return err;
	}

	return k_sem_take(&recycled_sem, RSP_TIMEOUT);
}

/* An event that the Host handles on bt_workq, and calls the application for */
static void conn_update_complete_send(void)
{
	struct bt_hci_evt_le_conn_update_complete evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.handle = sys_cpu_to_le16(TEST_CONN_HANDLE),
		.interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MAX),
		.latency = 0U,
		.supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U)),
	};

	le_meta_evt_send(BT_HCI_EVT_LE_CONN_UPDATE_COMPLETE, &evt, sizeof(evt));
}

/* bt_workq has handled what the peer sent before: it handles that before the
 * event that follows, for which it calls the application.
 */
static void handled_expect(void)
{
	conn_update_complete_send();

	zassert_ok(k_sem_take(&param_updated_sem, HANDLED_TIMEOUT), "bt_workq is blocked");
}

/* A data packet from the peer, with room for its headers */
static struct net_buf *pdu_alloc(void)
{
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	zassert_not_null(buf, "No buffer for a data packet from the peer");
	(void)net_buf_add(buf, sizeof(struct bt_hci_acl_hdr) + sizeof(struct bt_l2cap_hdr));

	return buf;
}

static void pdu_recv_on(struct net_buf *buf, uint16_t conn_handle, uint16_t cid)
{
	struct bt_hci_acl_hdr *acl = (void *)&buf->data[ACL_HDR_OFFSET];
	struct bt_l2cap_hdr *hdr = (void *)&buf->data[L2CAP_HDR_OFFSET];

	hdr->len = sys_cpu_to_le16(buf->len - PAYLOAD_OFFSET);
	hdr->cid = sys_cpu_to_le16(cid);
	acl->handle = sys_cpu_to_le16(bt_acl_handle_pack(conn_handle, BT_ACL_START));
	acl->len = sys_cpu_to_le16(buf->len - L2CAP_HDR_OFFSET);

	evt_recv(buf);
}

static void pdu_recv(struct net_buf *buf, uint16_t cid)
{
	pdu_recv_on(buf, TEST_CONN_HANDLE, cid);
}

/* The peer sends an ATT PDU on a bearer. On an enhanced one it takes a credit. */
static void att_send(uint8_t bearer, const uint8_t *pdu, uint16_t len)
{
	struct net_buf *buf = pdu_alloc();
	uint16_t cid = BT_L2CAP_CID_ATT;

	if (bearer != UATT) {
		zassert_true(atomic_dec(&eatt[bearer - 1U].credits) > 0,
			     "The peer has no credit for bearer %u", bearer);
		net_buf_add_le16(buf, len);
		cid = eatt[bearer - 1U].host_cid;
	}

	(void)net_buf_add_mem(buf, pdu, len);
	pdu_recv(buf, cid);
}

/* A Read Request on the unenhanced bearer of another connection */
static void read_req_send_on(uint16_t conn_handle, uint16_t handle)
{
	struct net_buf *buf = pdu_alloc();

	(void)net_buf_add_u8(buf, BT_ATT_OP_READ_REQ);
	net_buf_add_le16(buf, handle);
	pdu_recv_on(buf, conn_handle, BT_L2CAP_CID_ATT);
}

static void read_req_send(uint8_t bearer, uint16_t handle)
{
	uint8_t pdu[sizeof(struct bt_att_hdr) + sizeof(struct bt_att_read_req)];

	pdu[0] = BT_ATT_OP_READ_REQ;
	sys_put_le16(handle, &pdu[1]);
	att_send(bearer, pdu, sizeof(pdu));
}

/* A Write Request or a Write Command */
static void write_send(uint8_t bearer, uint8_t op, uint16_t handle, const uint8_t *value)
{
	uint8_t pdu[sizeof(struct bt_att_hdr) + sizeof(struct bt_att_write_req) + TEST_VALUE_LEN];

	pdu[0] = op;
	sys_put_le16(handle, &pdu[1]);
	(void)memcpy(&pdu[3], value, TEST_VALUE_LEN);
	att_send(bearer, pdu, sizeof(pdu));
}

/* The peer indicates a value of its own database */
static void indicate_send(uint8_t bearer)
{
	uint8_t pdu[sizeof(struct bt_att_hdr) + sizeof(struct bt_att_indicate) + TEST_VALUE_LEN];

	pdu[0] = BT_ATT_OP_INDICATE;
	sys_put_le16(PEER_HANDLE, &pdu[1]);
	(void)memcpy(&pdu[3], new_value, TEST_VALUE_LEN);
	att_send(bearer, pdu, sizeof(pdu));
}

static struct net_buf *sig_alloc(uint8_t code, uint8_t ident)
{
	struct net_buf *buf = pdu_alloc();
	struct bt_l2cap_sig_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = code;
	hdr->ident = ident;

	return buf;
}

static void sig_recv(struct net_buf *buf)
{
	struct bt_l2cap_sig_hdr *hdr = (void *)&buf->data[PAYLOAD_OFFSET];

	hdr->len = sys_cpu_to_le16(buf->len - PAYLOAD_OFFSET - sizeof(*hdr));
	pdu_recv(buf, BT_L2CAP_CID_LE_SIG);
}

/* The peer opens the enhanced bearers, with the credits that it gives the Host
 * for each
 */
static void eatt_connect(uint16_t credits)
{
	struct net_buf *buf;

	buf = sig_alloc(BT_L2CAP_ECRED_CONN_REQ, PEER_IDENT);
	net_buf_add_le16(buf, BT_EATT_PSM);
	net_buf_add_le16(buf, EATT_MTU);
	net_buf_add_le16(buf, EATT_MTU);
	net_buf_add_le16(buf, credits);
	ARRAY_FOR_EACH(eatt, i) {
		eatt[i].peer_cid = PEER_CID + i;
		net_buf_add_le16(buf, eatt[i].peer_cid);
	}
	sig_recv(buf);

	zassert_ok(k_sem_take(&ecred_rsp_sem, RSP_TIMEOUT), "No response to the connection");
	zassert_equal(ecred_result, BT_L2CAP_LE_SUCCESS, "Connection refused: 0x%04x",
		      ecred_result);
	zassert_equal(bt_eatt_count(test_conn), ARRAY_SIZE(eatt), "%zu enhanced bearers",
		      bt_eatt_count(test_conn));
}

/* The peer gives the Host credits for an enhanced bearer */
static void peer_credits_give(uint8_t bearer, uint16_t credits)
{
	struct net_buf *buf;

	buf = sig_alloc(BT_L2CAP_LE_CREDITS, PEER_IDENT + bearer);
	net_buf_add_le16(buf, eatt[bearer - 1U].peer_cid);
	net_buf_add_le16(buf, credits);
	sig_recv(buf);
}

/* The peer closes an enhanced bearer. The Host's response is sent once the
 * controller has room for it.
 */
static void eatt_disconnect_send(uint8_t bearer)
{
	struct net_buf *buf;

	buf = sig_alloc(BT_L2CAP_DISCONN_REQ, PEER_IDENT + bearer);
	net_buf_add_le16(buf, eatt[bearer - 1U].host_cid);
	net_buf_add_le16(buf, eatt[bearer - 1U].peer_cid);
	sig_recv(buf);
}

/* The next ATT PDU that the Host sent, other than a notification */
static void att_pdu_expect(struct att_pdu *pdu, uint8_t bearer, uint8_t op)
{
	zassert_ok(k_msgq_get(&att_tx_msgq, pdu, RSP_TIMEOUT), "The Host did not send 0x%02x", op);
	zassert_equal(pdu->data[0], op, "The Host sent 0x%02x instead of 0x%02x", pdu->data[0], op);
	zassert_equal(pdu->bearer, bearer, "0x%02x on bearer %u instead of %u", op, pdu->bearer,
		      bearer);
}

/* A Read Response on the unenhanced bearer of a connection */
static void read_rsp_expect_on(uint16_t conn_handle, const uint8_t *value)
{
	struct att_pdu pdu;

	att_pdu_expect(&pdu, UATT, BT_ATT_OP_READ_RSP);
	zassert_equal(pdu.handle, conn_handle, "Read Response on connection 0x%04x",
		      pdu.handle);
	zassert_mem_equal(&pdu.data[1], value, TEST_VALUE_LEN, "Read Response with another value");
}

static void read_rsp_expect(uint8_t bearer, const uint8_t *value)
{
	struct att_pdu pdu;

	att_pdu_expect(&pdu, bearer, BT_ATT_OP_READ_RSP);
	zassert_equal(pdu.len, sizeof(struct bt_att_hdr) + TEST_VALUE_LEN,
		      "Read Response of %u octets", pdu.len);
	zassert_mem_equal(&pdu.data[1], value, TEST_VALUE_LEN, "Read Response with another value");
}

static void err_rsp_expect(uint8_t bearer, uint8_t req, uint8_t err)
{
	struct bt_att_error_rsp *rsp;
	struct att_pdu pdu;

	att_pdu_expect(&pdu, bearer, BT_ATT_OP_ERROR_RSP);
	zassert_equal(pdu.len, sizeof(struct bt_att_hdr) + sizeof(*rsp),
		      "Error Response of %u octets", pdu.len);
	rsp = (void *)&pdu.data[1];
	zassert_equal(rsp->request, req, "Error Response for 0x%02x", rsp->request);
	zassert_equal(rsp->error, err, "Error 0x%02x", rsp->error);
}

/* Nothing was sent but notifications */
static void nothing_sent_expect(void)
{
	struct att_pdu pdu = { 0 };

	zassert_equal(k_msgq_get(&att_tx_msgq, &pdu, K_NO_WAIT), -ENOMSG,
		      "The Host sent 0x%02x on bearer %u", pdu.data[0], pdu.bearer);
}

/* The Host disconnects, and the controller reports the connection gone */
static void host_disconnect_expect(void)
{
	zassert_ok(k_sem_take(&hci_disconnect_sem, RSP_TIMEOUT), "The Host did not disconnect");
	zassert_ok(disconnect(), "The connection did not go");
}

static struct net_buf_pool *att_pool;
static struct net_buf_pool *acl_rx_pool;

static struct net_buf_pool *pool_get(const char *name)
{
	STRUCT_SECTION_FOREACH(net_buf_pool, pool) {
		if (strcmp(pool->name, name) == 0) {
			return pool;
		}
	}

	return NULL;
}

/* No buffer of any pool is in use */
static void pools_idle_expect(void)
{
	STRUCT_SECTION_FOREACH(net_buf_pool, pool) {
		zassert_equal(atomic_get(&pool->avail_count), pool->buf_count,
			      "%ld of the %u buffers of %s are free",
			      atomic_get(&pool->avail_count), pool->buf_count, pool->name);
	}
}

/* The buffers that hold what the peer sent, all but the given number are free */
static void acl_rx_held_expect(uint16_t count)
{
	zassert_equal(atomic_get(&acl_rx_pool->avail_count), acl_rx_pool->buf_count - count,
		      "%ld of the %u buffers of %s are free", atomic_get(&acl_rx_pool->avail_count),
		      acl_rx_pool->buf_count, acl_rx_pool->name);
}

/* An application notification */
static int notify(enum bt_att_chan_opt chan_opt)
{
	static const uint8_t value = 0x5aU;
	struct bt_gatt_notify_params params = {
		.attr = value_attrs[0],
		.data = &value,
		.len = sizeof(value),
		.chan_opt = chan_opt,
	};

	return bt_gatt_notify_cb(test_conn, &params);
}

/* The controller holds as many data packets as it takes and reports none as
 * completed, so the Host cannot send what it queues from here on
 */
static void controller_fill(void)
{
	credits_withhold();

	for (uint8_t i = 0U; i < LE_ACL_MAX_NUM; i++) {
		zassert_ok(notify(BT_ATT_CHAN_OPT_UNENHANCED_ONLY), "Notification %u failed", i);
	}

	zassert_equal(atomic_get(&acl_pending), LE_ACL_MAX_NUM,
		      "The controller holds %ld data packets", atomic_get(&acl_pending));
}

/* A bearer's response buffer is away: the response to a Read Request of the
 * first characteristic waits in the Host for the controller to take it
 */
static void rsp_buf_away(uint8_t bearer)
{
	controller_fill();

	read_req_send(bearer, value_handles[0]);
	handled_expect();
	zassert_equal(atomic_get(&read_count), 1, "The read callback ran %ld times",
		      atomic_get(&read_count));
	nothing_sent_expect();
}

/* The test holds the buffers of att_pool, as an application's threads do when
 * they queue more than the connection takes
 */
static struct net_buf *held[CONFIG_BT_ATT_TX_COUNT];

static void att_pool_hold(void)
{
	/* The buffers that the Host has in use stay with it */
	ARRAY_FOR_EACH(held, i) {
		held[i] = net_buf_alloc(att_pool, K_NO_WAIT);
	}

	zassert_equal(atomic_get(&att_pool->avail_count), 0, "att_pool has %ld free buffers",
		      atomic_get(&att_pool->avail_count));
}

static void att_pool_release(void)
{
	ARRAY_FOR_EACH(held, i) {
		net_buf_drop(&held[i]);
	}
}

/* bt_workq runs nothing between workq_block() and workq_unblock() */
static K_SEM_DEFINE(blocker_sem, 0, 1);
static K_SEM_DEFINE(blocker_running_sem, 0, 1);
static atomic_t is_workq_blocked;

static void blocker_handler(struct k_work *work)
{
	__maybe_unused int err;

	ARG_UNUSED(work);

	k_sem_give(&blocker_running_sem);
	err = k_sem_take(&blocker_sem, K_FOREVER);
	__ASSERT_NO_MSG(err == 0);
}

static K_WORK_DEFINE(blocker_work, blocker_handler);

/* For a test that has bt_workq busy, behind which the blocker queues */
static void workq_block_submit(void)
{
	int err;

	err = bt_work_submit(&blocker_work);
	zassert_true(err >= 0, "Blocker not submitted: %d", err);
}

static void workq_block_wait(void)
{
	zassert_ok(k_sem_take(&blocker_running_sem, RSP_TIMEOUT), "Blocker not running");
	(void)atomic_set(&is_workq_blocked, 1);
}

static void workq_block(void)
{
	workq_block_submit();
	workq_block_wait();
}

static void workq_unblock(void)
{
	if (atomic_cas(&is_workq_blocked, 1, 0)) {
		k_sem_give(&blocker_sem);
	}
}

/* An application thread that queues one notification while att_pool is empty */
#define NOTIFIER_STACK_SIZE 2048U

static struct k_thread notifier_thread;
static atomic_t is_notifier_started;
static atomic_t is_notifier_done;

static void notifier(void *p1, void *p2, void *p3)
{
	ARGS_UNUSED(p1, p2, p3);

	if (notify(BT_ATT_CHAN_OPT_UNENHANCED_ONLY) == 0) {
		(void)atomic_set(&is_notifier_done, 1);
	}
}

/* Above the test's thread, so that it blocks in the allocator before the test
 * goes on
 */
static void notifier_start(void)
{
	static K_THREAD_STACK_DEFINE(notifier_stack, NOTIFIER_STACK_SIZE);

	(void)atomic_set(&is_notifier_started, 1);
	(void)k_thread_create(&notifier_thread, notifier_stack,
			      K_THREAD_STACK_SIZEOF(notifier_stack), notifier, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
}

static int notifier_join(void)
{
	if (!atomic_cas(&is_notifier_started, 1, 0)) {
		return 0;
	}

	return k_thread_join(&notifier_thread, RSP_TIMEOUT);
}

/* No bearer still counts a response as being sent: on a new connection with
 * all the bearers that the Host can have, each answers a request at once
 */
static void rsp_bufs_expect(void)
{
	credits_release();
	connect_as_peripheral();
	eatt_connect(PEER_CREDITS);

	for (uint8_t bearer = UATT; bearer < BEARER_COUNT; bearer++) {
		read_req_send(bearer, value_handles[bearer]);
		read_rsp_expect(bearer, initial_value);
	}
}

/* The memory of the unenhanced bearer in the slab that it is allocated from,
 * which a test takes once the Host has freed the bearer
 */
static struct {
	struct k_mem_slab *slab;
	char *block;
	bool is_taken;
} uatt_block;

static void uatt_block_find(void)
{
	struct bt_l2cap_chan *chan = bt_l2cap_le_lookup_rx_cid(test_conn, BT_L2CAP_CID_ATT);
	char *addr = (char *)chan;

	zassert_not_null(chan, "No unenhanced bearer");

	STRUCT_SECTION_FOREACH(k_mem_slab, slab) {
		size_t size = slab->info.num_blocks * slab->info.block_size;

		if (addr >= slab->buffer && addr < slab->buffer + size) {
			size_t index = (size_t)(addr - slab->buffer) / slab->info.block_size;

			uatt_block.slab = slab;
			uatt_block.block = slab->buffer + index * slab->info.block_size;
			return;
		}
	}

	zassert_unreachable("No slab holds the unenhanced bearer");
}

static void uatt_block_release(void)
{
	if (uatt_block.is_taken) {
		uatt_block.is_taken = false;
		k_mem_slab_free(uatt_block.slab, uatt_block.block);
	}
}

static void *setup(void)
{
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	att_pool = pool_get("att_pool");
	zassert_not_null(att_pool, "No att_pool");

	acl_rx_pool = pool_get(IS_ENABLED(CONFIG_BT_HCI_ACL_FLOW_CONTROL) ? "acl_in_pool"
									  : "hci_rx_pool");
	zassert_not_null(acl_rx_pool, "No pool for received data packets");

	ARRAY_FOR_EACH(value_handles, i) {
		value_handles[i] = bt_gatt_attr_get_handle(value_attrs[i]);
		zassert_not_equal(value_handles[i], 0U, "No handle for value %zu", i);
	}

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	ARRAY_FOR_EACH_PTR(chrcs, chrc) {
		(void)memcpy(chrc->value, initial_value, sizeof(chrc->value));
	}

	read_err = 0U;
	(void)atomic_clear(&read_count);
	(void)atomic_clear(&write_count);
	(void)atomic_clear(&prepare_count);
	(void)memset(eatt, 0, sizeof(eatt));
	k_sem_reset(&param_updated_sem);
	k_sem_reset(&other_connected_sem);
	k_sem_reset(&other_disconnected_sem);
	k_sem_reset(&ecred_rsp_sem);
	k_sem_reset(&disconn_rsp_sem);
	k_sem_reset(&hci_disconnect_sem);
	k_msgq_purge(&att_tx_msgq);
	(void)atomic_clear(&credits_withheld);
	(void)atomic_clear(&unexpected_count);
	(void)atomic_clear(&is_notifier_done);
	(void)atomic_clear(&notify_seq);
	(void)atomic_clear(&host_completed);

	if (!bt_is_ready()) {
		zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	}

	/* Nothing is in use before the connection */
	pools_idle_expect();

	connect_as_peripheral();
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* What a failed test leaves behind */
	workq_unblock();
	uatt_block_release();
	att_pool_release();
	credits_release();

	if (test_conn != NULL) {
		zassert_ok(disconnect(), "The connection did not go");
	}

	for (uint16_t handle = OTHER_CONN_HANDLE; handle <= NEW_CONN_HANDLE; handle++) {
		struct bt_conn *conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_LE);

		if (conn != NULL) {
			bt_conn_unref(conn);
			zassert_ok(other_disconnect(handle), "Connection %u did not go", handle);
		}
	}

	/* An advertiser that a test started and no peer connected to */
	if (bt_is_ready()) {
		zassert_ok(bt_le_adv_stop(), "Advertising failed to stop");
	}

	zassert_ok(notifier_join(), "The notifying thread is still blocked");

	if (is_host_fc_missing) {
		is_host_fc_missing = false;
		zassert_ok(bt_disable(), "Bluetooth disable failed");
	}
}

/* Bluetooth is left disabled, as the suite found it */
static void teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	if (bt_is_ready()) {
		zassert_ok(bt_disable(), "Bluetooth disable failed");
	}
}

ZTEST_SUITE(att_buf, NULL, setup, before, after, teardown);

/* The test disconnects, and the Host has no buffer left in use */
static void disconnect_expect(void)
{
	zassert_ok(disconnect(), "The connection did not go");
	pools_idle_expect();
	zassert_equal(atomic_get(&unexpected_count), 0, "%ld unexpected packets or failures",
		      atomic_get(&unexpected_count));
}

/* Requests on all bearers are answered at once while the application's
 * threads hold every buffer of att_pool: the responses do not come from it
 */
static ZTEST(att_buf, test_read_att_pool_empty)
{
	eatt_connect(PEER_CREDITS);
	att_pool_hold();

	for (uint8_t bearer = UATT; bearer < BEARER_COUNT; bearer++) {
		read_req_send(bearer, value_handles[bearer]);
		read_rsp_expect(bearer, initial_value);
	}

	handled_expect();
	zassert_equal(atomic_get(&read_count), BEARER_COUNT, "The read callback ran %ld times",
		      atomic_get(&read_count));

	att_pool_release();
	disconnect_expect();
}

/* A request on one connection while the application's threads hold every
 * buffer of att_pool does not hold up the events of the others: another
 * connection is lost and a new one comes, and the application hears of both
 * at once.
 */
static ZTEST(att_buf, test_other_connections_while_att_pool_empty)
{
	adv_start();
	conn_complete_send(OTHER_CONN_HANDLE);
	zassert_ok(k_sem_take(&other_connected_sem, RSP_TIMEOUT), "No other connection");

	/* For the new connection, before bt_workq gets the request */
	adv_start();
	att_pool_hold();

	read_req_send(UATT, value_handles[0]);
	disconn_complete_send(OTHER_CONN_HANDLE);
	conn_complete_send(NEW_CONN_HANDLE);

	zassert_ok(k_sem_take(&other_disconnected_sem, HANDLED_TIMEOUT),
		   "The lost connection was not reported");
	zassert_ok(k_sem_take(&other_connected_sem, HANDLED_TIMEOUT),
		   "The new connection was not reported");
	read_rsp_expect(UATT, initial_value);
	zassert_ok(k_sem_take(&recycled_sem, RSP_TIMEOUT), "The lost connection was not released");

	att_pool_release();
	zassert_ok(other_disconnect(NEW_CONN_HANDLE), "The new connection did not go");
	disconnect_expect();
}

/* A Write Request is applied and gets its Write Response while att_pool is
 * empty
 */
static ZTEST(att_buf, test_write_req_att_pool_empty)
{
	struct att_pdu pdu;

	att_pool_hold();

	write_send(UATT, BT_ATT_OP_WRITE_REQ, value_handles[0], new_value);

	att_pdu_expect(&pdu, UATT, BT_ATT_OP_WRITE_RSP);
	zassert_equal(pdu.len, sizeof(struct bt_att_hdr), "Write Response of %u octets", pdu.len);
	zassert_equal(atomic_get(&write_count), 1, "The write callback ran %ld times",
		      atomic_get(&write_count));
	zassert_mem_equal(chrcs[0].value, new_value, TEST_VALUE_LEN, "The value was not written");

	att_pool_release();
	disconnect_expect();
}

/* A handler that drops its response for an Error Response sends it in the
 * same buffer: att_pool has none
 */
static ZTEST(att_buf, test_error_rsp_att_pool_empty)
{
	read_err = BT_ATT_ERR_UNLIKELY;
	att_pool_hold();

	read_req_send(UATT, value_handles[0]);

	err_rsp_expect(UATT, BT_ATT_OP_READ_REQ, BT_ATT_ERR_UNLIKELY);
	zassert_equal(atomic_get(&read_count), 1, "The read callback ran %ld times",
		      atomic_get(&read_count));

	att_pool_release();
	disconnect_expect();
}

/* What the server does not know and is no command is answered like a request */
static ZTEST(att_buf, test_unknown_request_att_pool_empty)
{
	const uint8_t pdu = UNKNOWN_REQ_OP;

	att_pool_hold();

	att_send(UATT, &pdu, sizeof(pdu));

	err_rsp_expect(UATT, UNKNOWN_REQ_OP, BT_ATT_ERR_NOT_SUPPORTED);

	att_pool_release();
	disconnect_expect();
}

/* A Prepare Write Request of a value of a characteristic */
static void prepare_send(uint8_t bearer, uint16_t handle, const uint8_t *value)
{
	uint8_t pdu[sizeof(struct bt_att_hdr) + sizeof(struct bt_att_prepare_write_req) +
		    TEST_VALUE_LEN];

	pdu[0] = BT_ATT_OP_PREPARE_WRITE_REQ;
	sys_put_le16(handle, &pdu[1]);
	sys_put_le16(0U, &pdu[3]);
	(void)memcpy(&pdu[5], value, TEST_VALUE_LEN);
	att_send(bearer, pdu, sizeof(pdu));
}

static void exec_send(uint8_t bearer)
{
	const uint8_t pdu[] = { BT_ATT_OP_EXEC_WRITE_REQ, BT_ATT_FLAG_EXEC };

	att_send(bearer, pdu, sizeof(pdu));
}

/* att_pool has all its buffers but the given number free */
static void att_pool_used_expect(uint16_t count)
{
	zassert_equal(atomic_get(&att_pool->avail_count), att_pool->buf_count - count,
		      "%ld of the %u buffers of att_pool are free",
		      atomic_get(&att_pool->avail_count), att_pool->buf_count);
}

/* A request that arrives before the response to the previous one has left
 * the Host is processed at once, its response from att_pool, and the two
 * responses go out in the order of the requests
 */
static ZTEST(att_buf, test_second_request_from_att_pool)
{
	(void)memcpy(chrcs[1].value, new_value, TEST_VALUE_LEN);

	rsp_buf_away(UATT);
	read_req_send(UATT, value_handles[1]);
	handled_expect();

	zassert_equal(atomic_get(&read_count), 2, "The read callback ran %ld times",
		      atomic_get(&read_count));
	att_pool_used_expect(1U);
	nothing_sent_expect();

	credits_release();
	read_rsp_expect(UATT, initial_value);
	read_rsp_expect(UATT, new_value);

	disconnect_expect();
}

/* The same on an enhanced bearer whose peer gives no credits for the first
 * response: the second waits on the bearer behind it
 */
static ZTEST(att_buf, test_second_request_from_att_pool_no_credits)
{
	(void)memcpy(chrcs[2].value, new_value, TEST_VALUE_LEN);

	/* The peer's one credit goes to a response */
	eatt_connect(1U);
	read_req_send(1U, value_handles[0]);
	read_rsp_expect(1U, initial_value);

	read_req_send(1U, value_handles[1]);
	read_req_send(1U, value_handles[2]);
	handled_expect();

	zassert_equal(atomic_get(&read_count), 3, "The read callback ran %ld times",
		      atomic_get(&read_count));
	att_pool_used_expect(1U);
	nothing_sent_expect();

	peer_credits_give(1U, PEER_CREDITS);
	read_rsp_expect(1U, initial_value);
	read_rsp_expect(1U, new_value);

	disconnect_expect();
}

/* Requests that a client sends at the same time on two enhanced bearers of
 * one connection: the connection's buffer answers the first, att_pool the
 * second
 */
static ZTEST(att_buf, test_requests_on_two_bearers)
{
	(void)memcpy(chrcs[2].value, new_value, TEST_VALUE_LEN);
	eatt_connect(PEER_CREDITS);
	controller_fill();

	read_req_send(1U, value_handles[1]);
	read_req_send(2U, value_handles[2]);
	handled_expect();

	zassert_equal(atomic_get(&read_count), 2, "The read callback ran %ld times",
		      atomic_get(&read_count));
	att_pool_used_expect(1U);
	nothing_sent_expect();

	credits_release();
	read_rsp_expect(1U, initial_value);
	read_rsp_expect(2U, new_value);

	disconnect_expect();
}

/* The same while att_pool is empty: the second request ends the connection
 * and is not processed
 */
static ZTEST(att_buf, test_requests_on_two_bearers_att_pool_empty)
{
	eatt_connect(PEER_CREDITS);
	controller_fill();

	read_req_send(1U, value_handles[1]);
	handled_expect();
	att_pool_hold();

	read_req_send(2U, value_handles[2]);

	host_disconnect_expect();
	zassert_equal(atomic_get(&read_count), 1, "The second request was processed");

	att_pool_release();
	pools_idle_expect();
	rsp_bufs_expect();
}

/* Two connections each have a buffer for their responses: while the first
 * connection's response waits and att_pool is empty, a request on the other
 * one is answered
 */
static ZTEST(att_buf, test_two_connections_own_buffers)
{
	adv_start();
	conn_complete_send(OTHER_CONN_HANDLE);
	zassert_ok(k_sem_take(&other_connected_sem, RSP_TIMEOUT), "No other connection");

	rsp_buf_away(UATT);
	att_pool_hold();

	read_req_send_on(OTHER_CONN_HANDLE, value_handles[1]);
	handled_expect();
	zassert_equal(atomic_get(&read_count), 2, "The read callback ran %ld times",
		      atomic_get(&read_count));
	zassert_equal(k_sem_count_get(&hci_disconnect_sem), 0U, "The Host disconnected");

	credits_release();
	read_rsp_expect_on(TEST_CONN_HANDLE, initial_value);
	read_rsp_expect_on(OTHER_CONN_HANDLE, initial_value);

	att_pool_release();
	zassert_ok(other_disconnect(OTHER_CONN_HANDLE), "The other connection did not go");
	disconnect_expect();
}

/* A request that finds neither its connection's buffer nor one of att_pool ends
 * the connection, and is not processed: a Write Request writes nothing
 */
static ZTEST(att_buf, test_write_req_without_buffer)
{
	rsp_buf_away(UATT);
	att_pool_hold();

	write_send(UATT, BT_ATT_OP_WRITE_REQ, value_handles[1], new_value);

	host_disconnect_expect();
	zassert_equal(atomic_get(&write_count), 0, "The write callback ran");
	zassert_mem_equal(chrcs[1].value, initial_value, TEST_VALUE_LEN, "The value was written");

	att_pool_release();
	pools_idle_expect();
	rsp_bufs_expect();
}

/* The same for a Prepare Write Request: nothing is prepared */
static ZTEST(att_buf, test_prepare_write_without_buffer)
{
	rsp_buf_away(UATT);
	att_pool_hold();

	prepare_send(UATT, value_handles[1], new_value);

	host_disconnect_expect();
	zassert_equal(atomic_get(&prepare_count), 0, "The value was prepared");

	att_pool_release();
	pools_idle_expect();
	rsp_bufs_expect();
}

/* The same for an Execute Write Request: the prepared value is not written */
static ZTEST(att_buf, test_exec_write_without_buffer)
{
	struct att_pdu pdu;

	prepare_send(UATT, value_handles[1], new_value);
	att_pdu_expect(&pdu, UATT, BT_ATT_OP_PREPARE_WRITE_RSP);

	rsp_buf_away(UATT);
	att_pool_hold();

	exec_send(UATT);

	host_disconnect_expect();
	zassert_equal(atomic_get(&write_count), 0, "The prepared value was written");
	zassert_mem_equal(chrcs[1].value, initial_value, TEST_VALUE_LEN, "The value was written");

	att_pool_release();
	pools_idle_expect();
	rsp_bufs_expect();
}

/* A handler that sends an Error Response in place of its response, for a
 * request answered from att_pool: the error goes out in the same buffer
 */
static ZTEST(att_buf, test_error_rsp_from_att_pool)
{
	rsp_buf_away(UATT);

	read_err = BT_ATT_ERR_UNLIKELY;
	read_req_send(UATT, value_handles[1]);
	handled_expect();
	att_pool_used_expect(1U);

	credits_release();
	read_rsp_expect(UATT, initial_value);
	err_rsp_expect(UATT, BT_ATT_OP_READ_REQ, BT_ATT_ERR_UNLIKELY);

	disconnect_expect();
}

/* A request that gets no response frees the buffer secured for it, the
 * connection's own and one of att_pool. The Host ignores opcode 0x00.
 */
static ZTEST(att_buf, test_request_without_response)
{
	const uint8_t pdu = 0x00U;

	att_send(UATT, &pdu, sizeof(pdu));
	handled_expect();
	nothing_sent_expect();
	pools_idle_expect();

	rsp_buf_away(UATT);
	att_send(UATT, &pdu, sizeof(pdu));
	handled_expect();
	att_pool_used_expect(0U);

	credits_release();
	read_rsp_expect(UATT, initial_value);
	nothing_sent_expect();

	disconnect_expect();
}

/* Requests that the peer sends without waiting are answered from att_pool
 * until it is empty. The next one ends the connection, and everything that
 * waits to be sent is freed.
 */
static ZTEST(att_buf, test_pipelined_requests)
{
	const uint16_t reads = att_pool->buf_count;

	rsp_buf_away(UATT);

	for (uint16_t i = 0U; i < reads; i++) {
		read_req_send(UATT, value_handles[1]);
	}

	handled_expect();
	zassert_equal(atomic_get(&read_count), 1 + reads, "The read callback ran %ld times",
		      atomic_get(&read_count));
	att_pool_used_expect(reads);

	read_req_send(UATT, value_handles[1]);

	host_disconnect_expect();
	zassert_equal(atomic_get(&read_count), 1 + reads, "The last request was processed");
	pools_idle_expect();
	rsp_bufs_expect();
}

/* Responses that wait on an enhanced bearer whose peer gives no credits: the
 * first in the connection's buffer, the second from att_pool
 */
static void eatt_rsps_wait(void)
{
	eatt_connect(1U);
	read_req_send(1U, value_handles[0]);
	read_rsp_expect(1U, initial_value);

	read_req_send(1U, value_handles[1]);
	read_req_send(1U, value_handles[2]);
	handled_expect();
	att_pool_used_expect(1U);
}

/* The connection goes while responses wait on a bearer: they are freed */
static ZTEST(att_buf, test_disconnect_with_rsps_waiting)
{
	eatt_rsps_wait();

	disconnect_expect();
	rsp_bufs_expect();
}

/* The peer closes the bearer on which responses wait: they are freed, and the
 * other bearers go on
 */
static ZTEST(att_buf, test_eatt_closed_with_rsps_waiting)
{
	eatt_rsps_wait();

	eatt_disconnect_send(1U);
	zassert_ok(k_sem_take(&disconn_rsp_sem, RSP_TIMEOUT), "No Disconnection Response");
	zassert_equal(bt_eatt_count(test_conn), ARRAY_SIZE(eatt) - 1U, "%zu enhanced bearers",
		      bt_eatt_count(test_conn));
	att_pool_used_expect(0U);
	nothing_sent_expect();

	read_req_send(2U, value_handles[2]);
	read_rsp_expect(2U, initial_value);
	read_req_send(UATT, value_handles[0]);
	read_rsp_expect(UATT, initial_value);

	disconnect_expect();
	rsp_bufs_expect();
}

/* bt_disable() while responses wait and a confirmation is owed: nothing is
 * left in use, and after bt_enable() the bearers have their buffers
 */
static ZTEST(att_buf, test_disable_with_rsps_waiting)
{
	eatt_rsps_wait();
	att_pool_hold();
	indicate_send(UATT);
	handled_expect();

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_ok(disconnected_wait(), "The connection did not go");

	att_pool_release();
	pools_idle_expect();

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	rsp_bufs_expect();
}

/* bt_disable() while bt_workq is blocked: the connection's teardown, which
 * frees the bearers, queues behind the blocker, and the blocker again behind
 * the teardown. What the test queues next runs after the blocker.
 */
static void disable_with_workq_blocked(void)
{
	workq_block();
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	workq_block_submit();
}

/* The teardown has freed the bearer: the test takes its memory and fills it,
 * so that the Host fails if the work behind the blocker uses the bearer
 */
static void uatt_block_take(void)
{
	void *mem;

	workq_block_wait();

	zassert_ok(k_mem_slab_alloc(uatt_block.slab, &mem, K_NO_WAIT), "The bearer was not freed");
	uatt_block.is_taken = true;
	zassert_equal_ptr(mem, uatt_block.block, "The slab gave another block than the bearer's");
	(void)memset(mem, 0xff, uatt_block.slab->info.block_size);
}

/* After the work behind the blocker has run */
static void uatt_block_give_back(void)
{
	workq_unblock();
	workq_block();
	workq_unblock();

	uatt_block_release();
}

/* bt_disable() while the response of the unenhanced bearer is held: the work
 * that the freed response submits runs after the bearer is freed, and does
 * not use it
 */
static ZTEST(att_buf, test_disable_rsp_work_after_bearer_freed)
{
	uatt_block_find();
	rsp_buf_away(UATT);

	disable_with_workq_blocked();
	workq_unblock();

	uatt_block_take();
	uatt_block_give_back();

	zassert_ok(disconnected_wait(), "The connection did not go");
	pools_idle_expect();

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	rsp_bufs_expect();
}

/* bt_disable() after a buffer was kept for an owed confirmation and before
 * bt_workq has sent it: the confirmation's work runs after the bearer is
 * freed, does not use it, and the buffer goes back to att_pool
 */
static ZTEST(att_buf, test_disable_confirm_work_after_bearer_freed)
{
	uatt_block_find();
	att_pool_hold();
	indicate_send(UATT);
	handled_expect();

	disable_with_workq_blocked();
	net_buf_drop(&held[0]);
	workq_unblock();

	uatt_block_take();
	uatt_block_give_back();

	zassert_ok(disconnected_wait(), "The connection did not go");
	att_pool_release();
	pools_idle_expect();
	nothing_sent_expect();

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	rsp_bufs_expect();
}

/* A controller without the commands of Controller to Host flow control,
 * although the Host is built with it: a request answered from att_pool
 * while Write Commands arrive, all in the order of arrival
 */
static ZTEST(att_buf, test_controller_without_host_flow_control)
{
	/* Every buffer that the Host has for data packets is used once */
	const uint16_t writes = acl_rx_pool->buf_count;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_HCI_ACL_FLOW_CONTROL);

	zassert_ok(disconnect(), "The connection did not go");
	zassert_ok(bt_disable(), "Bluetooth disable failed");

	is_host_fc_missing = true;
	(void)atomic_clear(&host_fc_enable_count);
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_equal(atomic_get(&host_fc_enable_count), 0, "The Host turned flow control on");

	connect_as_peripheral();
	(void)atomic_clear(&host_completed);

	rsp_buf_away(UATT);
	read_req_send(UATT, value_handles[2]);

	for (uint16_t i = 0U; i < writes; i++) {
		write_send(UATT, BT_ATT_OP_WRITE_CMD, value_handles[2], new_value);
	}

	handled_expect();
	zassert_equal(atomic_get(&write_count), writes, "%ld of %u writes",
		      atomic_get(&write_count), writes);
	acl_rx_held_expect(0U);
	zassert_equal(atomic_get(&host_completed), 0, "The Host reported completed packets");

	credits_release();

	/* The read came before the writes */
	read_rsp_expect(UATT, initial_value);
	read_rsp_expect(UATT, initial_value);

	disconnect_expect();
}

/* An indication that arrives while att_pool is empty is processed without
 * blocking bt_workq, and confirmed when a buffer is freed
 */
static ZTEST(att_buf, test_confirm_att_pool_empty)
{
	struct att_pdu pdu;

	att_pool_hold();

	indicate_send(UATT);

	handled_expect();
	nothing_sent_expect();

	net_buf_drop(&held[0]);

	att_pdu_expect(&pdu, UATT, BT_ATT_OP_CONFIRM);
	zassert_equal(pdu.len, sizeof(struct bt_att_hdr), "Confirmation of %u octets", pdu.len);

	att_pool_release();
	disconnect_expect();
}

/* A second indication while the confirmation of the first is owed is one that
 * the peer must not send: the Host disconnects
 */
static ZTEST(att_buf, test_second_indication_disconnects)
{
	att_pool_hold();

	indicate_send(UATT);
	handled_expect();

	indicate_send(UATT);

	host_disconnect_expect();

	att_pool_release();
	pools_idle_expect();
	nothing_sent_expect();
}

/* An indication on an enhanced bearer whose peer gives no credits: the
 * confirmation waits in the Host, and stays owed until it has left. A second
 * indication meanwhile ends the connection, so the bearer holds one buffer
 * of att_pool for confirmations.
 */
static ZTEST(att_buf, test_second_indication_before_confirm_sent)
{
	/* The peer's one credit for the bearer goes to a response */
	eatt_connect(1U);
	read_req_send(1U, value_handles[1]);
	read_rsp_expect(1U, initial_value);

	indicate_send(1U);
	handled_expect();
	nothing_sent_expect();
	att_pool_used_expect(1U);

	indicate_send(1U);

	host_disconnect_expect();
	pools_idle_expect();
}

/* Two bearers owe a confirmation and an application thread is blocked in the
 * allocator. The buffers that are freed go to the bearer that has waited
 * longest, then to the thread, then to the other bearer.
 */
static ZTEST(att_buf, test_confirms_and_blocked_allocator)
{
	struct att_pdu first;
	struct att_pdu second;

	eatt_connect(PEER_CREDITS);
	att_pool_hold();

	indicate_send(1U);
	indicate_send(UATT);
	handled_expect();

	notifier_start();
	zassert_equal(atomic_get(&is_notifier_done), 0, "The notification got a buffer");

	/* One buffer, which each that gets it frees again by sending */
	net_buf_drop(&held[0]);

	att_pdu_expect(&first, 1U, BT_ATT_OP_CONFIRM);
	att_pdu_expect(&second, UATT, BT_ATT_OP_CONFIRM);
	zassert_ok(notifier_join(), "The notifying thread is still blocked");
	zassert_equal(atomic_get(&is_notifier_done), 1, "The notification failed");
	zassert_true(first.seq < atomic_get(&notify_seq) && atomic_get(&notify_seq) < second.seq,
		     "Confirmations sent as %ld and %ld, the notification as %ld", first.seq,
		     second.seq, atomic_get(&notify_seq));

	att_pool_release();
	disconnect_expect();
}

/* Two bearers owe a confirmation and nothing else wants a buffer: the one
 * buffer that is freed serves both, the second bearer when the first has sent.
 */
static ZTEST(att_buf, test_confirms_share_one_buffer)
{
	struct att_pdu pdu;

	eatt_connect(PEER_CREDITS);
	att_pool_hold();

	indicate_send(UATT);
	indicate_send(1U);
	handled_expect();

	net_buf_drop(&held[0]);

	att_pdu_expect(&pdu, UATT, BT_ATT_OP_CONFIRM);
	att_pdu_expect(&pdu, 1U, BT_ATT_OP_CONFIRM);

	att_pool_release();
	disconnect_expect();
}

/* The connection goes while a confirmation is owed: the buffers that are
 * freed afterwards go back to att_pool
 */
static ZTEST(att_buf, test_disconnect_while_confirm_owed)
{
	att_pool_hold();

	indicate_send(UATT);
	handled_expect();

	zassert_ok(disconnect(), "The connection did not go");

	att_pool_release();
	pools_idle_expect();
	nothing_sent_expect();
}

/* The connection goes after a buffer was kept for an owed confirmation and
 * before bt_workq has sent it: the buffer goes back to att_pool.
 */
static ZTEST(att_buf, test_disconnect_after_confirm_buf_kept)
{
	att_pool_hold();

	indicate_send(UATT);
	handled_expect();

	/* bt_workq gets to the disconnection first, then to the confirmation */
	workq_block();
	disconn_complete_send(TEST_CONN_HANDLE);
	net_buf_drop(&held[0]);
	zassert_equal(atomic_get(&att_pool->avail_count), 0,
		      "The freed buffer was not kept for the confirmation");
	workq_unblock();

	zassert_ok(disconnected_wait(), "The connection did not go");

	att_pool_release();
	pools_idle_expect();
	nothing_sent_expect();
}

/* The Host ends the bearer for a second indication after a buffer was kept
 * for the confirmation of the first, and before bt_workq has sent it: the
 * buffer goes back to att_pool.
 */
static ZTEST(att_buf, test_second_indication_after_confirm_buf_kept)
{
	att_pool_hold();

	indicate_send(UATT);
	handled_expect();

	/* bt_workq gets to the second indication first, then to the confirmation */
	workq_block();
	indicate_send(UATT);
	net_buf_drop(&held[0]);
	zassert_equal(atomic_get(&att_pool->avail_count), 0,
		      "The freed buffer was not kept for the confirmation");
	workq_unblock();

	host_disconnect_expect();

	att_pool_release();
	pools_idle_expect();
	nothing_sent_expect();
}
