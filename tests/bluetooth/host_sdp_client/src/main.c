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
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/uuid.h>
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

#include "host/conn_internal.h"
#include "host/hci_core.h"
#include "host/classic/l2cap_br_internal.h"
#include "host/classic/sdp_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of initialization commands that are
 * irrelevant to this test
 */
#define GENERIC_RP_SIZE 64U

#define LE_ACL_MAX_LEN 27U
#define LE_ACL_MAX_NUM 3U
#define BR_ACL_MAX_LEN 255U
#define BR_ACL_MAX_NUM 8U

#define TEST_CONN_HANDLE 0x0001U
/* The channel of the peer */
#define PEER_CID         0x0040U
/* The identifier of the requests of the peer, which the Host only echoes */
#define PEER_IDENT       0x80U

#define TEST_RECORD_HANDLE 0x00010003U

/* Enough to tell a connection that is retried from one that is not. The peer
 * does not answer further attempts, so that the test gets to see the result
 * instead of spinning with the Host at no simulated time.
 */
#define MAX_CONN_REQS 3

/* What the peer does with a request for a channel */
enum conn_mode {
	CONN_ACCEPT,
	CONN_REFUSE,
	CONN_IGNORE,
};

/* What the peer does with the next SDP request */
enum sdp_mode {
	SDP_RSP_NOTHING,
	SDP_RSP_EMPTY_ATTR_LIST,
	SDP_RSP_ATTR_LIST,
	SDP_RSP_SEARCH_NO_RECORDS,
	SDP_RSP_SEARCH_ONE_RECORD,
	SDP_DISCONNECT_CHAN,
};

static enum conn_mode conn_mode;
static enum sdp_mode sdp_mode;
/* The connection requests the peer got, and the channel the last one is for */
static atomic_t conn_reqs;
static uint16_t host_cid;
/* The disconnection requests the peer got */
static atomic_t disconn_reqs;
/* The SDP requests the peer got */
static atomic_t sdp_reqs;

static atomic_t link_up;
static atomic_t acl_pending;

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
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->acl_max_len = sys_cpu_to_le16(BR_ACL_MAX_LEN);
	rp->sco_max_len = 0U;
	rp->acl_max_num = sys_cpu_to_le16(BR_ACL_MAX_NUM);
	rp->sco_max_num = 0U;
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

/* A data packet from the peer with an L2CAP PDU */
static void peer_pdu_send(uint16_t cid, const void *hdr, uint16_t hdr_len, const void *data,
			  uint16_t data_len)
{
	struct bt_hci_acl_hdr *acl;
	struct bt_l2cap_hdr *l2cap;
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	acl = net_buf_add(buf, sizeof(*acl));
	acl->handle = sys_cpu_to_le16(bt_acl_handle_pack(TEST_CONN_HANDLE, BT_ACL_START));
	acl->len = sys_cpu_to_le16(sizeof(*l2cap) + hdr_len + data_len);
	l2cap = net_buf_add(buf, sizeof(*l2cap));
	l2cap->len = sys_cpu_to_le16(hdr_len + data_len);
	l2cap->cid = sys_cpu_to_le16(cid);
	net_buf_add_mem(buf, hdr, hdr_len);
	net_buf_add_mem(buf, data, data_len);

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
}

static void sig_send(uint8_t code, uint8_t ident, const void *data, uint16_t len)
{
	const struct bt_l2cap_sig_hdr hdr = {
		.code = code,
		.ident = ident,
		.len = sys_cpu_to_le16(len),
	};

	peer_pdu_send(BT_L2CAP_CID_BR_SIG, &hdr, sizeof(hdr), data, len);
}

static void conn_rsp_send(uint8_t ident, uint16_t result)
{
	const struct bt_l2cap_conn_rsp rsp = {
		.dcid = sys_cpu_to_le16((result == BT_L2CAP_BR_SUCCESS) ? PEER_CID : 0U),
		.scid = sys_cpu_to_le16(host_cid),
		.result = sys_cpu_to_le16(result),
	};

	sig_send(BT_L2CAP_CONN_RSP, ident, &rsp, sizeof(rsp));
}

static void conf_req_send(void)
{
	const struct bt_l2cap_conf_req req = {
		.dcid = sys_cpu_to_le16(host_cid),
	};

	sig_send(BT_L2CAP_CONF_REQ, PEER_IDENT, &req, sizeof(req));
}

static void conf_rsp_send(uint8_t ident)
{
	const struct bt_l2cap_conf_rsp rsp = {
		.scid = sys_cpu_to_le16(host_cid),
		.result = sys_cpu_to_le16(BT_L2CAP_CONF_SUCCESS),
	};

	sig_send(BT_L2CAP_CONF_RSP, ident, &rsp, sizeof(rsp));
}

static void info_rsp_send(uint8_t ident, uint16_t type)
{
	const struct bt_l2cap_info_rsp rsp = {
		.type = sys_cpu_to_le16(type),
		.result = sys_cpu_to_le16(BT_L2CAP_INFO_NOTSUPP),
	};

	sig_send(BT_L2CAP_INFO_RSP, ident, &rsp, sizeof(rsp));
}

static void disconn_rsp_send(uint8_t ident)
{
	const struct bt_l2cap_disconn_rsp rsp = {
		.dcid = sys_cpu_to_le16(PEER_CID),
		.scid = sys_cpu_to_le16(host_cid),
	};

	sig_send(BT_L2CAP_DISCONN_RSP, ident, &rsp, sizeof(rsp));
}

/* The peer ends the channel */
static void disconn_req_send(void)
{
	const struct bt_l2cap_disconn_req req = {
		.dcid = sys_cpu_to_le16(host_cid),
		.scid = sys_cpu_to_le16(PEER_CID),
	};

	sig_send(BT_L2CAP_DISCONN_REQ, PEER_IDENT, &req, sizeof(req));
}

static void sig_handle(struct net_buf *buf)
{
	struct bt_l2cap_sig_hdr *hdr;
	uint8_t ident;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	ident = hdr->ident;

	switch (hdr->code) {
	case BT_L2CAP_INFO_REQ:
		info_rsp_send(ident, net_buf_pull_le16(buf));
		break;
	case BT_L2CAP_CONN_REQ:
		/* The PSM, then the channel of the Host */
		(void)net_buf_pull_le16(buf);
		host_cid = net_buf_pull_le16(buf);

		if (atomic_inc(&conn_reqs) >= MAX_CONN_REQS) {
			break;
		}

		switch (conn_mode) {
		case CONN_ACCEPT:
			conn_rsp_send(ident, BT_L2CAP_BR_SUCCESS);
			conf_req_send();
			break;
		case CONN_REFUSE:
			conn_rsp_send(ident, BT_L2CAP_BR_ERR_NO_RESOURCES);
			break;
		case CONN_IGNORE:
			break;
		}
		break;
	case BT_L2CAP_CONF_REQ:
		conf_rsp_send(ident);
		break;
	case BT_L2CAP_DISCONN_REQ:
		(void)atomic_inc(&disconn_reqs);
		disconn_rsp_send(ident);
		break;
	default:
		break;
	}
}

static void sdp_rsp_send(uint8_t op_code, uint16_t tid, const uint8_t *params, uint16_t len)
{
	const struct bt_sdp_hdr hdr = {
		.op_code = op_code,
		.tid = sys_cpu_to_be16(tid),
		.param_len = sys_cpu_to_be16(len),
	};

	peer_pdu_send(host_cid, &hdr, sizeof(hdr), params, len);
}

static void sdp_handle(struct net_buf *buf)
{
	/* The attribute list length, an empty list and no continuation state */
	static const uint8_t empty_attr_list[] = { 0x00, 0x02, BT_SDP_SEQ8, 0x00, 0x00 };
	/* As above, with attribute 0x0000 as an 8-bit unsigned integer */
	static const uint8_t attr_list[] = { 0x00, 0x07, BT_SDP_SEQ8, 0x05,
					     BT_SDP_UINT16, 0x00, 0x00, BT_SDP_UINT8, 0x01,
					     0x00 };
	/* Total and current record counts and no continuation state */
	static const uint8_t no_records[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
	/* As above, with one record handle */
	static const uint8_t one_record[] = { 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03,
					      0x00 };
	struct bt_sdp_hdr *hdr;
	uint16_t tid;

	(void)atomic_inc(&sdp_reqs);

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	tid = sys_be16_to_cpu(hdr->tid);

	switch (sdp_mode) {
	case SDP_RSP_NOTHING:
		break;
	case SDP_RSP_EMPTY_ATTR_LIST:
		sdp_rsp_send(BT_SDP_SVC_ATTR_RSP, tid, empty_attr_list, sizeof(empty_attr_list));
		break;
	case SDP_RSP_ATTR_LIST:
		sdp_rsp_send(BT_SDP_SVC_ATTR_RSP, tid, attr_list, sizeof(attr_list));
		break;
	case SDP_RSP_SEARCH_NO_RECORDS:
		sdp_rsp_send(BT_SDP_SVC_SEARCH_RSP, tid, no_records, sizeof(no_records));
		break;
	case SDP_RSP_SEARCH_ONE_RECORD:
		sdp_rsp_send(BT_SDP_SVC_SEARCH_RSP, tid, one_record, sizeof(one_record));
		break;
	case SDP_DISCONNECT_CHAN:
		disconn_req_send();
		break;
	}
}

static void acl_handle(struct net_buf *buf)
{
	uint16_t cid;

	(void)net_buf_pull_mem(buf, sizeof(struct bt_hci_acl_hdr));
	(void)net_buf_pull_le16(buf);
	cid = net_buf_pull_le16(buf);

	if (cid == BT_L2CAP_CID_BR_SIG) {
		sig_handle(buf);
	} else if (cid == PEER_CID) {
		sdp_handle(buf);
	}
}

static void completed_handler(struct k_work *work)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
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
	bt_hci_recv(dev, buf);
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

	if (type == BT_HCI_H4_CMD) {
		cmd_handle(dev, buf);
	} else if (type == BT_HCI_H4_ACL) {
		if (atomic_get(&link_up) != 0) {
			acl_handle(buf);
			(void)atomic_inc(&acl_pending);
			(void)k_work_submit(&completed_work);
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
	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
}

/* The peer connects to the Host */
static void link_create(void)
{
	static const bt_addr_t peer = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };
	struct bt_hci_evt_conn_request req;
	struct bt_hci_evt_conn_complete cc;

	(void)memset(&req, 0, sizeof(req));
	bt_addr_copy(&req.bdaddr, &peer);
	req.link_type = BT_HCI_ACL;
	evt_send(BT_HCI_EVT_CONN_REQUEST, &req, sizeof(req));

	/* The Host handled the request and sent its Accept Connection Request */
	k_sleep(K_MSEC(10));

	(void)memset(&cc, 0, sizeof(cc));
	cc.status = BT_HCI_ERR_SUCCESS;
	cc.handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	bt_addr_copy(&cc.bdaddr, &peer);
	cc.link_type = BT_HCI_ACL;

	(void)atomic_set(&link_up, 1);
	evt_send(BT_HCI_EVT_CONN_COMPLETE, &cc, sizeof(cc));

	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connection");
}

/* The link goes down, and all that held on to the connection let it go */
static void link_destroy(void)
{
	const struct bt_hci_evt_disconn_complete evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.handle = sys_cpu_to_le16(TEST_CONN_HANDLE),
		.reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN,
	};

	(void)atomic_set(&link_up, 0);
	evt_send(BT_HCI_EVT_DISCONN_COMPLETE, &evt, sizeof(evt));

	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");

	bt_conn_unref(test_conn);
	test_conn = NULL;

	zassert_ok(k_sem_take(&recycled_sem, K_SECONDS(1)), "Connection not released");
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

/* No buffer of any pool is in use. Buffers are freed on other threads too,
 * which are given the time to do so.
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

NET_BUF_POOL_FIXED_DEFINE(result_pool, 2, 128, 0, NULL);

/* A request and what the Host told its callback */
struct test_req {
	struct bt_sdp_discover_params params;
	size_t calls;
	size_t results;
	/* Requested from the callback */
	struct test_req *next;
	int next_err;
};

static struct test_req reqs[2];

static uint8_t discover_func(struct bt_conn *conn, struct bt_sdp_client_result *result,
			     const struct bt_sdp_discover_params *params)
{
	struct test_req *req = CONTAINER_OF(params, struct test_req, params);

	ARG_UNUSED(conn);

	req->calls++;

	if (result->resp_buf != NULL) {
		req->results++;
	}

	if (req->next != NULL) {
		req->next_err = bt_sdp_discover(conn, &req->next->params);
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

static void by_uuid_params_set(struct test_req *req, uint8_t type)
{
	static const struct bt_uuid_16 uuid = BT_UUID_INIT_16(BT_SDP_SDP_SERVER_SVCLASS);

	req->params.uuid = &uuid.uuid;
	req->params.func = discover_func;
	req->params.pool = &result_pool;
	req->params.type = type;
}

static void by_handle_params_set(struct test_req *req)
{
	req->params.handle = TEST_RECORD_HANDLE;
	req->params.func = discover_func;
	req->params.pool = &result_pool;
	req->params.type = BT_SDP_DISCOVER_SERVICE_ATTR;
}

static void by_uuid_discover(struct test_req *req, uint8_t type)
{
	by_uuid_params_set(req, type);

	zassert_ok(bt_sdp_discover(test_conn, &req->params));
}

static void by_handle_discover(struct test_req *req)
{
	by_handle_params_set(req);

	zassert_ok(bt_sdp_discover(test_conn, &req->params));
}

/* What the Host does in reaction to the peer runs at no simulated time */
static void host_settle(void)
{
	k_sleep(K_MSEC(100));
}

static void req_expect(const struct test_req *req, size_t results)
{
	zassert_equal(req->calls, 1U, "%zu callback calls", req->calls);
	zassert_equal(req->results, results, "%zu results", req->results);
}

static void *setup(void)
{
	zassert_ok(bt_enable(NULL));

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)memset(reqs, 0, sizeof(reqs));
	conn_mode = CONN_ACCEPT;
	sdp_mode = SDP_RSP_NOTHING;
	(void)atomic_set(&conn_reqs, 0);
	(void)atomic_set(&disconn_reqs, 0);
	(void)atomic_set(&sdp_reqs, 0);

	link_create();
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	link_destroy();

	/* No callback came a second time, late */
	ARRAY_FOR_EACH_PTR(reqs, req) {
		zassert_true(req->calls <= 1U, "%zu callback calls", req->calls);
	}

	pools_full_expect();
}

ZTEST_SUITE(host_sdp_client, NULL, setup, before, after, NULL);

/* The record of a discovery by record handle has no attribute in the range */
ZTEST(host_sdp_client, test_by_handle_empty_attr_list)
{
	sdp_mode = SDP_RSP_EMPTY_ATTR_LIST;
	by_handle_discover(&reqs[0]);
	host_settle();

	zassert_equal(atomic_get(&sdp_reqs), 1);
	req_expect(&reqs[0], 0U);
	/* The channel is done with */
	zassert_equal(atomic_get(&disconn_reqs), 1);
}

ZTEST(host_sdp_client, test_by_handle_attr_list)
{
	sdp_mode = SDP_RSP_ATTR_LIST;
	by_handle_discover(&reqs[0]);
	host_settle();

	req_expect(&reqs[0], 1U);
}

/* The server answers a discovery by record handle with a Service Search Response */
ZTEST(host_sdp_client, test_by_handle_other_rsp)
{
	sdp_mode = SDP_RSP_SEARCH_NO_RECORDS;
	by_handle_discover(&reqs[0]);
	host_settle();

	zassert_equal(atomic_get(&sdp_reqs), 1);
	zassert_equal(reqs[0].calls, 1U, "%zu callback calls", reqs[0].calls);

	sdp_mode = SDP_RSP_SEARCH_ONE_RECORD;
	by_handle_discover(&reqs[1]);
	host_settle();

	zassert_equal(atomic_get(&sdp_reqs), 2);
	zassert_equal(reqs[1].calls, 1U, "%zu callback calls", reqs[1].calls);
}

ZTEST(host_sdp_client, test_by_uuid_no_records)
{
	sdp_mode = SDP_RSP_SEARCH_NO_RECORDS;
	by_uuid_discover(&reqs[0], BT_SDP_DISCOVER_SERVICE_SEARCH);
	host_settle();

	zassert_equal(atomic_get(&sdp_reqs), 1);
	req_expect(&reqs[0], 0U);
}

/* Each failed connection ends one request */
ZTEST(host_sdp_client, test_conn_refused)
{
	conn_mode = CONN_REFUSE;

	k_sched_lock();
	by_uuid_discover(&reqs[0], BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR);
	by_handle_discover(&reqs[1]);
	k_sched_unlock();
	host_settle();

	zassert_equal(atomic_get(&conn_reqs), 2, "%ld connection attempts", atomic_get(&conn_reqs));
	req_expect(&reqs[0], 0U);
	req_expect(&reqs[1], 0U);
}

/* A request from the callback gets its own attempt */
ZTEST(host_sdp_client, test_conn_refused_request_from_callback)
{
	conn_mode = CONN_REFUSE;
	by_uuid_params_set(&reqs[1], BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR);
	reqs[0].next = &reqs[1];
	by_uuid_discover(&reqs[0], BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR);
	host_settle();

	zassert_ok(reqs[0].next_err);
	zassert_equal(atomic_get(&conn_reqs), 2, "%ld connection attempts", atomic_get(&conn_reqs));
	req_expect(&reqs[0], 0U);
	req_expect(&reqs[1], 0U);
}

/* A refused connection is not held against the next request */
ZTEST(host_sdp_client, test_conn_refused_then_accepted)
{
	conn_mode = CONN_REFUSE;
	by_uuid_discover(&reqs[0], BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR);
	host_settle();

	req_expect(&reqs[0], 0U);

	conn_mode = CONN_ACCEPT;
	sdp_mode = SDP_RSP_ATTR_LIST;
	by_handle_discover(&reqs[1]);
	host_settle();

	zassert_equal(atomic_get(&conn_reqs), 2, "%ld connection attempts", atomic_get(&conn_reqs));
	req_expect(&reqs[1], 1U);
}

/* A peer that does not answer has the same end as one that refuses */
ZTEST(host_sdp_client, test_conn_timeout)
{
	conn_mode = CONN_IGNORE;
	by_uuid_discover(&reqs[0], BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR);

	k_sleep(K_SECONDS(61));

	zassert_equal(atomic_get(&conn_reqs), 1, "%ld connection attempts", atomic_get(&conn_reqs));
	req_expect(&reqs[0], 0U);
}

/* The link going down while connecting ends the requests once */
ZTEST(host_sdp_client, test_link_lost_while_connecting)
{
	conn_mode = CONN_IGNORE;

	k_sched_lock();
	by_uuid_discover(&reqs[0], BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR);
	by_handle_discover(&reqs[1]);
	k_sched_unlock();
	host_settle();

	zassert_equal(atomic_get(&conn_reqs), 1, "%ld connection attempts", atomic_get(&conn_reqs));

	link_destroy();

	req_expect(&reqs[0], 0U);
	req_expect(&reqs[1], 0U);

	/* For after() */
	link_create();
}

/* A channel ended after it was connected: the request in flight is over, and the
 * next request gets a new channel.
 */
ZTEST(host_sdp_client, test_chan_ended_with_pending_request)
{
	sdp_mode = SDP_DISCONNECT_CHAN;

	k_sched_lock();
	by_handle_discover(&reqs[0]);
	by_handle_discover(&reqs[1]);
	k_sched_unlock();
	host_settle();

	req_expect(&reqs[0], 0U);
	/* The peer ends the new channel too, and the requests run out */
	req_expect(&reqs[1], 0U);
	zassert_equal(atomic_get(&conn_reqs), 2, "%ld connection attempts", atomic_get(&conn_reqs));
}
