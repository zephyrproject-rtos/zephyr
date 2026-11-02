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
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/att_internal.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of initialization commands irrelevant to this test */
#define GENERIC_RP_SIZE 64U

#define LE_ACL_MAX_LEN 27U
#define LE_ACL_MAX_NUM 3U

#define TEST_CONN_HANDLE 0x0001U

/* The characteristics of the peer that the tests subscribe to */
#define VALUE_HANDLE_A 0x0010U
#define CCC_HANDLE_A   0x0011U
#define VALUE_HANDLE_B 0x0020U
#define CCC_HANDLE_B   0x0021U

static const bt_addr_le_t peer_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a.val = { 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0xc6U },
};

/* What the peer does with a Write Request. It answers as soon as the driver has the request, so
 * before a preemptible thread that sent it runs again.
 */
enum peer_mode {
	PEER_HOLD,
	PEER_ACCEPT,
	PEER_REFUSE,
};

static atomic_t peer_mode;

/* The values in the Write Requests that the controller got, and the data packets it did not expect
 */
K_MSGQ_DEFINE_STATIC(write_req_msgq, sizeof(uint16_t), 16, sizeof(uint16_t));
static atomic_t acl_unexpected;

/* Whether the link is up, and the data packets that the controller holds on it until it reports
 * them as completed
 */
static atomic_t link_up;
static atomic_t acl_pending;

static void hci_recv(struct net_buf **buf)
{
	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), net_buf_take(buf));
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
	rp->le_max_num = LE_ACL_MAX_NUM;
}

static void le_read_supp_states(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
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

	hci_recv(&evt);
}

/* A data packet from the peer with an ATT PDU of len bytes, which the caller adds */
static struct net_buf *att_pdu_create(uint16_t len)
{
	struct bt_hci_acl_hdr *acl;
	struct bt_l2cap_hdr *hdr;
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	acl = net_buf_add(buf, sizeof(*acl));
	acl->handle = sys_cpu_to_le16(bt_acl_handle_pack(TEST_CONN_HANDLE, BT_ACL_START));
	acl->len = sys_cpu_to_le16(sizeof(*hdr) + len);
	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->len = sys_cpu_to_le16(len);
	hdr->cid = sys_cpu_to_le16(BT_L2CAP_CID_ATT);

	return buf;
}

static void write_rsp_recv(void)
{
	struct net_buf *buf;

	buf = att_pdu_create(sizeof(struct bt_att_hdr));
	net_buf_add_u8(buf, BT_ATT_OP_WRITE_RSP);

	hci_recv(&buf);
}

static void error_rsp_recv(uint16_t handle, uint8_t err)
{
	struct bt_att_error_rsp *rsp;
	struct net_buf *buf;

	buf = att_pdu_create(sizeof(struct bt_att_hdr) + sizeof(*rsp));
	net_buf_add_u8(buf, BT_ATT_OP_ERROR_RSP);
	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->request = BT_ATT_OP_WRITE_REQ;
	rsp->handle = sys_cpu_to_le16(handle);
	rsp->error = err;

	hci_recv(&buf);
}

static void notification_recv(uint16_t handle)
{
	struct bt_att_notify *nfy;
	struct net_buf *buf;

	buf = att_pdu_create(sizeof(struct bt_att_hdr) + sizeof(*nfy) + sizeof(uint8_t));
	net_buf_add_u8(buf, BT_ATT_OP_NOTIFY);
	nfy = net_buf_add(buf, sizeof(*nfy));
	nfy->handle = sys_cpu_to_le16(handle);
	net_buf_add_u8(buf, 0x5aU);

	hci_recv(&buf);
}

static void write_req_handle(struct net_buf *buf)
{
	struct bt_att_write_req *req;
	uint16_t value;

	if (buf->len != sizeof(struct bt_att_hdr) + sizeof(*req) + sizeof(value)) {
		(void)atomic_inc(&acl_unexpected);
		return;
	}

	net_buf_pull(buf, sizeof(struct bt_att_hdr));
	req = net_buf_pull_mem(buf, sizeof(*req));
	value = net_buf_pull_le16(buf);
	if (k_msgq_put(&write_req_msgq, &value, K_NO_WAIT) != 0) {
		(void)atomic_inc(&acl_unexpected);
	}

	if (atomic_get(&peer_mode) == PEER_ACCEPT) {
		write_rsp_recv();
	} else if (atomic_get(&peer_mode) == PEER_REFUSE) {
		error_rsp_recv(sys_le16_to_cpu(req->handle), BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}
}

static void acl_handle(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *hdr;
	uint16_t len;
	uint16_t cid;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));

	if (bt_acl_handle(sys_le16_to_cpu(hdr->handle)) != TEST_CONN_HANDLE ||
	    buf->len < BT_L2CAP_HDR_SIZE) {
		(void)atomic_inc(&acl_unexpected);
		return;
	}

	len = net_buf_pull_le16(buf);
	cid = net_buf_pull_le16(buf);

	if (len != buf->len || buf->len == 0U || cid != BT_L2CAP_CID_ATT ||
	    buf->data[0] != BT_ATT_OP_WRITE_REQ) {
		(void)atomic_inc(&acl_unexpected);
		return;
	}

	write_req_handle(buf);
}

static void completed_handler(struct k_work *work)
{
	struct bt_hci_evt_num_completed_packets *evt;
	struct bt_hci_handle_count *hc;
	struct net_buf *buf;
	atomic_val_t count;

	ARG_UNUSED(work);

	count = atomic_set(&acl_pending, 0);
	if (count == 0 || atomic_get(&link_up) == 0) {
		return;
	}

	buf = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_NUM_COMPLETED_PACKETS, sizeof(*evt) + sizeof(*hc));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->num_handles = 1U;
	hc = net_buf_add(buf, sizeof(*hc));
	hc->handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	hc->count = sys_cpu_to_le16((uint16_t)count);
	hci_recv(&buf);
}

static K_WORK_DEFINE(completed_work, completed_handler);

static int driver_open(const struct device *dev)
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

		if (atomic_get(&link_up) != 0) {
			(void)atomic_inc(&acl_pending);
			(void)k_work_submit(&completed_work);
		}
	} else {
		(void)atomic_inc(&acl_unexpected);
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

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static K_SEM_DEFINE(recycled_sem, 0, 1);
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

static void recycled(void)
{
	k_sem_give(&recycled_sem);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled,
};

static void evt_send(uint8_t evt, const void *params, uint8_t len)
{
	struct net_buf *buf;

	buf = bt_buf_get_evt(evt, false, K_FOREVER);
	evt_create(buf, evt, len);
	net_buf_add_mem(buf, params, len);
	hci_recv(&buf);
}

static void connect_as_peripheral(void)
{
	struct {
		struct bt_hci_evt_le_meta_event meta;
		struct bt_hci_evt_le_conn_complete cc;
	} __packed evt;

	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");

	(void)memset(&evt, 0, sizeof(evt));
	evt.meta.subevent = BT_HCI_EVT_LE_CONN_COMPLETE;
	evt.cc.status = BT_HCI_ERR_SUCCESS;
	evt.cc.handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	evt.cc.role = BT_HCI_ROLE_PERIPHERAL;
	bt_addr_le_copy(&evt.cc.peer_addr, &peer_addr);
	evt.cc.interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MIN);
	evt.cc.supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U));

	(void)atomic_set(&link_up, 1);
	evt_send(BT_HCI_EVT_LE_META_EVENT, &evt, sizeof(evt));

	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connection");
}

/* The remote ends the connection, which is then gone and released */
static void disconnect(void)
{
	const struct bt_hci_evt_disconn_complete evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.handle = sys_cpu_to_le16(TEST_CONN_HANDLE),
		.reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN,
	};

	(void)atomic_set(&link_up, 0);
	evt_send(BT_HCI_EVT_DISCONN_COMPLETE, &evt, sizeof(evt));

	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "The connection did not go");
	bt_conn_drop(&test_conn);
	zassert_ok(k_sem_take(&recycled_sem, K_SECONDS(1)), "The connection was not released");
}

static bool pools_full(void)
{
	STRUCT_SECTION_FOREACH(net_buf_pool, pool) {
		if (atomic_get(&pool->avail_count) != pool->buf_count) {
			return false;
		}
	}

	return true;
}

/* No buffer of any pool is in use. Buffers are freed on other threads too, which are given the time
 * to do so.
 */
static void pools_full_expect(void)
{
	for (unsigned int i = 0U; i < 100U && !pools_full(); i++) {
		k_sleep(K_MSEC(10));
	}

	STRUCT_SECTION_FOREACH(net_buf_pool, pool) {
		zassert_equal(atomic_get(&pool->avail_count), pool->buf_count,
			      "%ld of the %u buffers of %s are free",
			      atomic_get(&pool->avail_count), pool->buf_count, pool->name);
	}
}

/* A subscription, and what its callbacks saw */
struct test_sub {
	struct bt_gatt_subscribe_params params;
	atomic_t subscribe_count;
	atomic_t notify_count;
	atomic_t unsubscribed_count;
	uint8_t subscribe_err;
	/* The NULL callback subscribes again, as the params are free for reuse */
	bool resubscribe;
	int resubscribe_err;
};

static uint8_t sub_notify(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			  const void *data, uint16_t length)
{
	struct test_sub *sub = CONTAINER_OF(params, struct test_sub, params);

	ARG_UNUSED(length);

	if (data != NULL) {
		(void)atomic_inc(&sub->notify_count);
		return BT_GATT_ITER_CONTINUE;
	}

	(void)atomic_inc(&sub->unsubscribed_count);

	if (sub->resubscribe) {
		sub->resubscribe = false;
		params->value = BT_GATT_CCC_NOTIFY;
		sub->resubscribe_err = bt_gatt_subscribe(conn, params);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void sub_subscribe(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_subscribe_params *params)
{
	struct test_sub *sub = CONTAINER_OF(params, struct test_sub, params);

	ARG_UNUSED(conn);

	sub->subscribe_err = err;
	(void)atomic_inc(&sub->subscribe_count);
}

static void sub_init(struct test_sub *sub, uint16_t value_handle, uint16_t ccc_handle)
{
	(void)memset(sub, 0, sizeof(*sub));
	sub->params.notify = sub_notify;
	sub->params.subscribe = sub_subscribe;
	sub->params.value_handle = value_handle;
	sub->params.ccc_handle = ccc_handle;
	sub->params.value = BT_GATT_CCC_NOTIFY;
}

/* The controller got a Write Request with this value */
static void write_req_expect(uint16_t value)
{
	uint16_t got;

	zassert_ok(k_msgq_get(&write_req_msgq, &got, K_SECONDS(1)),
		   "The controller got no request");
	zassert_equal(got, value, "The peer was asked to write 0x%04x", got);
}

/* A subscription that the peer has accepted */
static void sub_establish(struct test_sub *sub, uint16_t value_handle, uint16_t ccc_handle)
{
	sub_init(sub, value_handle, ccc_handle);

	zassert_ok(bt_gatt_subscribe(test_conn, &sub->params), "The subscription failed");
	write_req_expect(BT_GATT_CCC_NOTIFY);
	write_rsp_recv();

	zassert_equal(atomic_get(&sub->subscribe_count), 1, "The subscription did not complete");
	zassert_equal(sub->subscribe_err, BT_ATT_ERR_SUCCESS, "The subscription got 0x%02x",
		      sub->subscribe_err);
	(void)atomic_clear(&sub->subscribe_count);
}

/* The application got the params back once, with this error */
static void sub_handed_back_expect(const struct test_sub *sub, uint8_t err)
{
	zassert_equal(atomic_get(&sub->unsubscribed_count), 1, "%ld NULL callbacks",
		      atomic_get(&sub->unsubscribed_count));
	zassert_equal(atomic_get(&sub->subscribe_count), 1, "%ld subscribe callbacks",
		      atomic_get(&sub->subscribe_count));
	zassert_equal(sub->subscribe_err, err, "The subscribe callback got 0x%02x",
		      sub->subscribe_err);
}

static void *setup(void)
{
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	pools_full_expect();
	connect_as_peripheral();
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	atomic_val_t unexpected;

	if (test_conn != NULL) {
		disconnect();
	}

	unexpected = atomic_clear(&acl_unexpected);
	(void)atomic_set(&peer_mode, PEER_HOLD);
	k_msgq_purge(&write_req_msgq);

	zassert_equal(unexpected, 0, "The controller got %ld unexpected packets", unexpected);
	pools_full_expect();
}

ZTEST_SUITE(host_gatt_sub, NULL, setup, before, after, NULL);

/* The peer accepts the unsubscription */
static ZTEST(host_gatt_sub, test_unsubscribe)
{
	static struct test_sub sub;

	sub_establish(&sub, VALUE_HANDLE_A, CCC_HANDLE_A);

	zassert_ok(bt_gatt_unsubscribe(test_conn, &sub.params), "The unsubscription failed");
	write_req_expect(0x0000U);
	zassert_equal(atomic_get(&sub.unsubscribed_count), 0, "The params came back too early");

	write_rsp_recv();
	sub_handed_back_expect(&sub, BT_ATT_ERR_SUCCESS);
}

/* Another subscription to the value remains, so the peer is not asked */
static ZTEST(host_gatt_sub, test_unsubscribe_value_still_subscribed)
{
	static struct test_sub sub_1;
	static struct test_sub sub_2;

	sub_establish(&sub_1, VALUE_HANDLE_A, CCC_HANDLE_A);

	sub_init(&sub_2, VALUE_HANDLE_A, CCC_HANDLE_A);
	zassert_ok(bt_gatt_subscribe(test_conn, &sub_2.params), "The subscription failed");

	zassert_ok(bt_gatt_unsubscribe(test_conn, &sub_2.params), "The unsubscription failed");
	zassert_equal(atomic_get(&sub_2.unsubscribed_count), 1, "%ld NULL callbacks",
		      atomic_get(&sub_2.unsubscribed_count));
	zassert_equal(k_msgq_num_used_get(&write_req_msgq), 0U, "The peer was asked");

	notification_recv(VALUE_HANDLE_A);
	zassert_equal(atomic_get(&sub_1.notify_count), 1, "The other subscription stopped");
	zassert_equal(atomic_get(&sub_2.notify_count), 0, "The removed one was called");
}

/* The peer answers before the thread that unsubscribed runs again. The params are free for reuse
 * when the NULL callback runs.
 */
static ZTEST(host_gatt_sub, test_unsubscribe_reuse_in_callback)
{
	static struct test_sub sub;

	sub_establish(&sub, VALUE_HANDLE_A, CCC_HANDLE_A);

	sub.resubscribe = true;
	(void)atomic_set(&peer_mode, PEER_ACCEPT);

	zassert_ok(bt_gatt_unsubscribe(test_conn, &sub.params), "The unsubscription failed");
	write_req_expect(0x0000U);

	zassert_equal(atomic_get(&sub.unsubscribed_count), 1, "%ld NULL callbacks",
		      atomic_get(&sub.unsubscribed_count));
	zassert_ok(sub.resubscribe_err, "Reusing the params returned %d", sub.resubscribe_err);
	write_req_expect(BT_GATT_CCC_NOTIFY);

	notification_recv(VALUE_HANDLE_A);
	zassert_equal(atomic_get(&sub.notify_count), 1, "The new subscription is not there");
}

/* The peer refuses the subscription */
static ZTEST(host_gatt_sub, test_subscribe_refused)
{
	static struct test_sub sub;

	sub_init(&sub, VALUE_HANDLE_A, CCC_HANDLE_A);
	zassert_ok(bt_gatt_subscribe(test_conn, &sub.params), "The subscription failed");
	write_req_expect(BT_GATT_CCC_NOTIFY);

	error_rsp_recv(CCC_HANDLE_A, BT_ATT_ERR_WRITE_NOT_PERMITTED);
	sub_handed_back_expect(&sub, BT_ATT_ERR_WRITE_NOT_PERMITTED);

	sub_establish(&sub, VALUE_HANDLE_A, CCC_HANDLE_A);
}

/* The peer refuses the subscription before the thread that subscribed runs again. The application
 * gets the params back, and they are not left linked.
 */
static ZTEST(host_gatt_sub, test_subscribe_refused_before_return)
{
	static struct test_sub sub;

	sub_init(&sub, VALUE_HANDLE_A, CCC_HANDLE_A);
	(void)atomic_set(&peer_mode, PEER_REFUSE);

	zassert_ok(bt_gatt_subscribe(test_conn, &sub.params), "The subscription failed");
	write_req_expect(BT_GATT_CCC_NOTIFY);
	sub_handed_back_expect(&sub, BT_ATT_ERR_WRITE_NOT_PERMITTED);

	notification_recv(VALUE_HANDLE_A);
	zassert_equal(atomic_get(&sub.notify_count), 0, "The refused subscription is linked");

	(void)atomic_set(&peer_mode, PEER_HOLD);
	sub_establish(&sub, VALUE_HANDLE_A, CCC_HANDLE_A);
}

/* The peer is unpaired with the request of a subscription outstanding. The application gets the
 * params back once, and the failure of the request does not hand them back again.
 */
static ZTEST(host_gatt_sub, test_unpair_during_subscribe)
{
	static struct test_sub sub;

	sub_init(&sub, VALUE_HANDLE_A, CCC_HANDLE_A);
	zassert_ok(bt_gatt_subscribe(test_conn, &sub.params), "The subscription failed");
	write_req_expect(BT_GATT_CCC_NOTIFY);

	zassert_ok(bt_unpair(BT_ID_DEFAULT, &peer_addr), "Unpairing failed");

	write_rsp_recv();
	disconnect();

	zassert_equal(atomic_get(&sub.unsubscribed_count), 1, "%ld NULL callbacks",
		      atomic_get(&sub.unsubscribed_count));
}
