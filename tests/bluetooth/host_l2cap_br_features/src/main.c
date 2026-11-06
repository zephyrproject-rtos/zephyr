/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* BR/EDR channels requested before the peer's features are known. A channel at BT_SECURITY_L1 needs
 * encryption only with a peer that supports Secure Simple Pairing, so the outcome must not depend
 * on which comes first. The Controller here reports the features when a test says so.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/classic/l2cap_br.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/hci_core.h"
#include "host/classic/l2cap_br_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of the commands that get zeroes */
#define GENERIC_RP_SIZE 64U

#define CONN_HANDLE   0x0001U
#define ACL_PKTS      4U
#define ENC_KEY_SIZE  16U
#define EVENT_TIMEOUT K_SECONDS(1)
#define NUM_CHANS     4U
#define MAX_MSGS      8U

#define SDP_PSM     0x0001U
#define RFCOMM_PSM  0x0003U
#define L1_PSM      0x1001U
#define L2_PSM      0x1003U
#define OUT_PSM     0x1005U
#define PEER_SCID   0x0040U
#define PEER_SCID_2 0x0041U
#define RX_MTU      48U

/* Core 6.3 Vol 2 Part C 3.3: extended features in page 0, the Host's support of SSP in page 1 */
#define FEAT_PAGE0_EXT_BYTE 7U
#define FEAT_PAGE0_EXT_BIT  BIT(7)
#define FEAT_PAGE1_SSP_BYTE 0U
#define FEAT_PAGE1_SSP_BIT  BIT(0)

struct test_chan {
	struct bt_l2cap_br_chan br;
	uint32_t connected;
	uint32_t disconnected;
	uint32_t released;
};

struct peer_rsp {
	uint16_t dcid;
	uint16_t scid;
	uint16_t result;
	uint16_t status;
};

static const bt_addr_t peer = {
	.val = { 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U },
};

/* What the Controller answers to the commands that read the features */
static uint8_t features_cmd_status;
static uint8_t ext_features_cmd_status;

/* A buffer that the Controller is done with but keeps */
static bool hold_acl;
static struct net_buf *held_acl;
static K_SEM_DEFINE(held_sem, 0, 1);

/* What the Host sent */
static uint32_t auth_count;
static uint32_t conn_req_count;
static uint16_t conn_req_psm;
static uint8_t conf_req_ident;
K_MSGQ_DEFINE(rsp_msgq, sizeof(struct peer_rsp), MAX_MSGS, 2);

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(features_sem, 0, 1);
static K_SEM_DEFINE(ext_features_sem, 0, 1);
static K_SEM_DEFINE(auth_sem, 0, 1);
static K_SEM_DEFINE(conn_req_sem, 0, NUM_CHANS);
static K_SEM_DEFINE(conf_req_sem, 0, NUM_CHANS);
static K_SEM_DEFINE(security_changed_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static K_SEM_DEFINE(chan_connected_sem, 0, NUM_CHANS);

static struct bt_conn *test_conn;
static struct test_chan chans[NUM_CHANS];
static size_t chans_used;
static uint8_t peer_ident;
static struct test_chan *connect_in_connected;
/* As an application freeing the channel may: any later read by the Host shows */
static bool overwrite_on_release;
static struct test_chan *remove_on_disconnected;
static struct bt_rfcomm_dlc dlc;

static void evt_send(struct net_buf *buf)
{
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static struct net_buf *evt_create(uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;

	buf = bt_buf_get_evt(evt, false, K_FOREVER);
	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;

	return buf;
}

static void *cmd_complete(struct net_buf **buf, uint8_t plen, uint16_t opcode)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = evt_create(BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);
	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return net_buf_add(*buf, plen);
}

static void cmd_status(uint16_t opcode, uint8_t status)
{
	struct bt_hci_evt_cmd_status *cs;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(buf, sizeof(*cs));
	cs->status = status;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);
	evt_send(buf);
}

static void generic_complete(uint16_t opcode)
{
	struct net_buf *buf;
	uint8_t *rp;

	rp = cmd_complete(&buf, GENERIC_RP_SIZE, opcode);
	(void)memset(rp, 0, GENERIC_RP_SIZE);
	evt_send(buf);
}

static void read_local_features(uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), opcode);
	(void)memset(rp, 0, sizeof(*rp));
	/* LE and BR/EDR supported */
	rp->features[4] = BIT(6);
	evt_send(buf);
}

static void read_supported_commands(uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
	evt_send(buf);
}

static void read_bd_addr(uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
	evt_send(buf);
}

static void read_buffer_size(uint16_t opcode)
{
	struct bt_hci_rp_read_buffer_size *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), opcode);
	(void)memset(rp, 0, sizeof(*rp));
	rp->acl_max_len = sys_cpu_to_le16(BT_BUF_ACL_SIZE(CONFIG_BT_BUF_ACL_TX_SIZE));
	rp->acl_max_num = sys_cpu_to_le16(ACL_PKTS);
	evt_send(buf);
}

static void read_enc_key_size(uint16_t opcode)
{
	struct bt_hci_rp_read_encryption_key_size *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), opcode);
	(void)memset(rp, 0, sizeof(*rp));
	rp->key_size = ENC_KEY_SIZE;
	evt_send(buf);
}

static void encrypt_change(uint8_t status, uint8_t encrypt)
{
	struct bt_hci_evt_encrypt_change *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_ENCRYPT_CHANGE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = status;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	evt->encrypt = encrypt;
	evt_send(buf);
}

static void auth_complete(uint8_t status)
{
	struct bt_hci_evt_auth_complete *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_AUTH_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = status;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	evt_send(buf);
}

/* The peer has started pairing */
static void io_capa_resp(void)
{
	struct bt_hci_evt_io_capa_resp *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_IO_CAPA_RESP, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	bt_addr_copy(&evt->bdaddr, &peer);
	evt_send(buf);
}

static void conn_complete(uint8_t encr_enabled)
{
	struct bt_hci_evt_conn_complete *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_CONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->link_type = BT_HCI_ACL;
	evt->encr_enabled = encr_enabled;
	evt_send(buf);
}

static void disconn_complete(void)
{
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	evt->reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
	evt_send(buf);
}

static void num_completed_packet(void)
{
	struct bt_hci_evt_num_completed_packets *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_NUM_COMPLETED_PACKETS, sizeof(*evt) + sizeof(evt->h[0]));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->num_handles = 1U;
	net_buf_add_le16(buf, CONN_HANDLE);
	net_buf_add_le16(buf, 1U);
	evt_send(buf);
}

/* Read Remote Supported Features Complete */
static void features_page0(uint8_t status, bool ext_features)
{
	struct bt_hci_evt_remote_features *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_REMOTE_FEATURES, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->status = status;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	if (ext_features) {
		evt->features[FEAT_PAGE0_EXT_BYTE] = FEAT_PAGE0_EXT_BIT;
	}
	evt_send(buf);
}

/* Read Remote Extended Features Complete, for page 1 */
static void features_page1(uint8_t status, bool ssp)
{
	struct bt_hci_evt_remote_ext_features *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_REMOTE_EXT_FEATURES, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->status = status;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	evt->page = 0x01U;
	evt->max_page = 0x01U;
	if (ssp) {
		evt->features[FEAT_PAGE1_SSP_BYTE] = FEAT_PAGE1_SSP_BIT;
	}
	evt_send(buf);
}

static void cmd_handle(struct net_buf *buf)
{
	const struct bt_hci_cmd_hdr *hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	uint16_t opcode = sys_le16_to_cpu(hdr->opcode);

	switch (opcode) {
	case BT_HCI_OP_READ_LOCAL_FEATURES:
		read_local_features(opcode);
		break;
	case BT_HCI_OP_READ_SUPPORTED_COMMANDS:
		read_supported_commands(opcode);
		break;
	case BT_HCI_OP_READ_BD_ADDR:
		read_bd_addr(opcode);
		break;
	case BT_HCI_OP_READ_BUFFER_SIZE:
		read_buffer_size(opcode);
		break;
	case BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE:
		read_enc_key_size(opcode);
		break;
	case BT_HCI_OP_CONNECT:
	case BT_HCI_OP_DISCONNECT:
		cmd_status(opcode, BT_HCI_ERR_SUCCESS);
		break;
	case BT_HCI_OP_READ_REMOTE_FEATURES:
		cmd_status(opcode, features_cmd_status);
		k_sem_give(&features_sem);
		break;
	case BT_HCI_OP_READ_REMOTE_EXT_FEATURES:
		cmd_status(opcode, ext_features_cmd_status);
		k_sem_give(&ext_features_sem);
		break;
	case BT_HCI_OP_AUTH_REQUESTED:
		auth_count++;
		cmd_status(opcode, BT_HCI_ERR_SUCCESS);
		k_sem_give(&auth_sem);
		break;
	case BT_HCI_OP_SET_CONN_ENCRYPT:
		cmd_status(opcode, BT_HCI_ERR_SUCCESS);
		encrypt_change(BT_HCI_ERR_SUCCESS, BT_HCI_ENCRYPTION_ON_BR_E0);
		break;
	case BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS:
		break;
	default:
		generic_complete(opcode);
		break;
	}
}

/* Returns true for a Connection Response, which then is in rsp */
static bool acl_handle(struct net_buf *buf, struct peer_rsp *rsp)
{
	const struct bt_l2cap_conn_req *req;
	const struct bt_l2cap_conn_rsp *conn_rsp;
	const struct bt_l2cap_sig_hdr *sig;
	const struct bt_l2cap_hdr *hdr;

	(void)net_buf_pull_mem(buf, sizeof(struct bt_hci_acl_hdr));
	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	if (sys_le16_to_cpu(hdr->cid) != BT_L2CAP_CID_BR_SIG) {
		return false;
	}

	sig = net_buf_pull_mem(buf, sizeof(*sig));
	switch (sig->code) {
	case BT_L2CAP_CONN_REQ:
		req = (const void *)buf->data;
		conn_req_psm = sys_le16_to_cpu(req->psm);
		conn_req_count++;
		k_sem_give(&conn_req_sem);
		return false;
	case BT_L2CAP_CONF_REQ:
		conf_req_ident = sig->ident;
		k_sem_give(&conf_req_sem);
		return false;
	case BT_L2CAP_CONN_RSP:
		conn_rsp = (const void *)buf->data;
		rsp->dcid = sys_le16_to_cpu(conn_rsp->dcid);
		rsp->scid = sys_le16_to_cpu(conn_rsp->scid);
		rsp->result = sys_le16_to_cpu(conn_rsp->result);
		rsp->status = sys_le16_to_cpu(conn_rsp->status);
		return true;
	default:
		return false;
	}
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
	struct peer_rsp rsp;
	uint8_t type;
	bool is_rsp;

	ARG_UNUSED(dev);

	type = net_buf_pull_u8(buf);
	if (type == BT_HCI_H4_CMD) {
		cmd_handle(buf);
		net_buf_unref(buf);
		return 0;
	}

	if (hold_acl) {
		zassert_is_null(held_acl);
		held_acl = buf;
		num_completed_packet();
		k_sem_give(&held_sem);
		return 0;
	}

	is_rsp = acl_handle(buf, &rsp);

	net_buf_unref(buf);
	num_completed_packet();

	if (is_rsp) {
		zassert_ok(k_msgq_put(&rsp_msgq, &rsp, K_NO_WAIT));
	}

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

static void chan_connected(struct bt_l2cap_chan *chan)
{
	CONTAINER_OF(chan, struct test_chan, br.chan)->connected++;
	k_sem_give(&chan_connected_sem);
}

static void chan_disconnected(struct bt_l2cap_chan *chan)
{
	struct test_chan *other = remove_on_disconnected;

	CONTAINER_OF(chan, struct test_chan, br.chan)->disconnected++;

	if (other != NULL) {
		remove_on_disconnected = NULL;
		bt_l2cap_chan_remove(other->br.chan.conn, &other->br.chan);
		bt_l2cap_br_chan_del(&other->br.chan);
	}
}

static void chan_released(struct bt_l2cap_chan *chan)
{
	struct test_chan *tchan = CONTAINER_OF(chan, struct test_chan, br.chan);

	tchan->released++;

	if (overwrite_on_release) {
		(void)memset(&tchan->br, 0xFF, sizeof(tchan->br));
	}
}

static int chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	ARG_UNUSED(chan);
	ARG_UNUSED(buf);

	return 0;
}

static struct test_chan *chan_new(bt_security_t sec_level)
{
	static const struct bt_l2cap_chan_ops ops = {
		.connected = chan_connected,
		.disconnected = chan_disconnected,
		.released = chan_released,
		.recv = chan_recv,
	};
	struct test_chan *tchan;

	zassert_true(chans_used < ARRAY_SIZE(chans), "Out of channels");

	tchan = &chans[chans_used++];
	tchan->br.chan.ops = &ops;
	tchan->br.rx.mtu = RX_MTU;
	tchan->br.required_sec_level = sec_level;

	return tchan;
}

static int server_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **chan)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(server);

	*chan = &chan_new(BT_SECURITY_L0)->br.chan;

	return 0;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(err);

	if (connect_in_connected != NULL) {
		zassert_ok(bt_l2cap_chan_connect(conn, &connect_in_connected->br.chan, OUT_PSM));
	}

	k_sem_give(&connected_sem);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);

	k_sem_give(&disconnected_sem);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(level);
	ARG_UNUSED(err);

	k_sem_give(&security_changed_sem);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void noop_work(struct k_work *work)
{
	ARG_UNUSED(work);
}

/* What the queue was given so far is done */
static void workq_flush(int (*submit)(struct k_work *work))
{
	struct k_work_sync sync;
	struct k_work work;

	k_work_init(&work, noop_work);
	zassert_true(submit(&work) >= 0);
	(void)k_work_flush(&work, &sync);
}

/* The Host handled what it was given, and what it sent reached the Controller */
static void settle(void)
{
	workq_flush(bt_work_submit);
	workq_flush(k_work_submit);
}

static void link_up_encrypted(uint8_t encr_enabled)
{
	test_conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(test_conn, "Creating the connection failed");

	conn_complete(encr_enabled);
	zassert_ok(k_sem_take(&connected_sem, EVENT_TIMEOUT), "No connected callback");
	zassert_ok(k_sem_take(&features_sem, EVENT_TIMEOUT), "No read of the remote features");
	settle();
}

static void link_up(void)
{
	link_up_encrypted(BT_HCI_ENCRYPTION_OFF);
}

/* The Controller's reports for a peer with or without Secure Simple Pairing, one at a time */
static void features_page0_ext(void)
{
	features_page0(BT_HCI_ERR_SUCCESS, true);
	zassert_ok(k_sem_take(&ext_features_sem, EVENT_TIMEOUT), "No read of page 1");
	settle();
}

static void features_report(bool ssp)
{
	features_page0_ext();
	features_page1(BT_HCI_ERR_SUCCESS, ssp);
	settle();
}

static void peer_sig_send(uint8_t code, uint8_t ident, const void *data, uint16_t len)
{
	struct bt_l2cap_sig_hdr sig = {
		.code = code,
		.ident = ident,
		.len = sys_cpu_to_le16(len),
	};
	struct bt_l2cap_hdr l2cap = {
		.len = sys_cpu_to_le16(sizeof(sig) + len),
		.cid = sys_cpu_to_le16(BT_L2CAP_CID_BR_SIG),
	};
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	net_buf_add_le16(buf, bt_acl_handle_pack(CONN_HANDLE, BT_ACL_START));
	net_buf_add_le16(buf, sizeof(l2cap) + sizeof(sig) + len);
	net_buf_add_mem(buf, &l2cap, sizeof(l2cap));
	net_buf_add_mem(buf, &sig, sizeof(sig));
	net_buf_add_mem(buf, data, len);
	evt_send(buf);
}

static void peer_conn_req(uint16_t psm, uint16_t scid)
{
	struct bt_l2cap_conn_req req = {
		.psm = sys_cpu_to_le16(psm),
		.scid = sys_cpu_to_le16(scid),
	};

	peer_sig_send(BT_L2CAP_CONN_REQ, ++peer_ident, &req, sizeof(req));
}

static void expect_rsp(uint16_t scid, uint16_t result, uint16_t status, uint16_t *dcid)
{
	struct peer_rsp rsp;

	zassert_ok(k_msgq_get(&rsp_msgq, &rsp, EVENT_TIMEOUT), "No Connection Response");
	zassert_equal(rsp.scid, scid, "SCID 0x%04x", rsp.scid);
	zassert_equal(rsp.result, result, "Result 0x%04x", rsp.result);
	zassert_equal(rsp.status, status, "Status 0x%04x", rsp.status);

	if (dcid != NULL) {
		*dcid = rsp.dcid;
	}
}

static void expect_no_rsp(void)
{
	settle();
	zassert_equal(k_msgq_num_used_get(&rsp_msgq), 0U, "A Connection Response went out");
}

/* A peer's request now waiting for the features, on the first free channel */
static struct test_chan *incoming_waiting(uint16_t psm, uint16_t scid)
{
	struct test_chan *tchan = &chans[chans_used];

	peer_conn_req(psm, scid);
	expect_rsp(scid, BT_L2CAP_BR_PENDING, BT_L2CAP_CS_NO_INFO, NULL);
	expect_no_rsp();

	return tchan;
}

/* A local request now waiting for the features */
static struct test_chan *outgoing_waiting(void)
{
	struct test_chan *tchan = chan_new(BT_SECURITY_L1);

	zassert_ok(bt_l2cap_chan_connect(test_conn, &tchan->br.chan, OUT_PSM), "Connecting failed");
	settle();
	zassert_equal(conn_req_count, 0U, "Request sent before the features");
	zassert_equal(auth_count, 0U, "Security started before the features");

	return tchan;
}

static void expect_conn_req(uint16_t psm)
{
	zassert_ok(k_sem_take(&conn_req_sem, EVENT_TIMEOUT), "No Connection Request");
	zassert_equal(conn_req_psm, psm, "PSM 0x%04x", conn_req_psm);
	settle();
	zassert_equal(conn_req_count, 1U, "%u Connection Requests", conn_req_count);
}

/* The peer's Controller finishes the authentication the Host asked for, then encryption follows */
static void security_completes(void)
{
	zassert_ok(k_sem_take(&auth_sem, EVENT_TIMEOUT), "No authentication");
	auth_complete(BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&security_changed_sem, EVENT_TIMEOUT), "No security_changed");
}

/* A refused channel has not seen connected(), whatever happens to it next */
static void expect_refused(uint16_t scid, const struct test_chan *tchan)
{
	expect_rsp(scid, BT_L2CAP_BR_ERR_SEC_BLOCK, BT_L2CAP_CS_NO_INFO, NULL);
	zassert_equal(tchan->connected, 0U, "Refused channel connected");
	zassert_equal(auth_count, 0U, "Security started for a refused channel");
}

/* A peer with Secure Simple Pairing and no encryption is refused as if the features came first */
static ZTEST(l2cap_br_features, test_incoming_ssp_peer_refused)
{
	const struct test_chan *tchan;

	link_up();
	tchan = incoming_waiting(L1_PSM, PEER_SCID);

	/* Page 0 does not tell */
	features_page0_ext();
	expect_no_rsp();

	features_page1(BT_HCI_ERR_SUCCESS, true);
	expect_refused(PEER_SCID, tchan);
}

/* Every channel in the list is told, also when the one before it goes */
static ZTEST(l2cap_br_features, test_incoming_ssp_peer_all_refused)
{
	const struct test_chan *first, *second;

	link_up();
	first = incoming_waiting(L1_PSM, PEER_SCID);
	second = incoming_waiting(L1_PSM, PEER_SCID_2);

	features_report(true);

	expect_refused(PEER_SCID, first);
	expect_refused(PEER_SCID_2, second);
}

/* A peer without Secure Simple Pairing gets the channel all the way, once it is known */
static ZTEST(l2cap_br_features, test_incoming_no_ssp_admitted)
{
	struct bt_l2cap_conf_rsp conf_rsp = {
		.scid = sys_cpu_to_le16(PEER_SCID),
	};
	struct bt_l2cap_conf_req conf_req = { 0 };
	const struct test_chan *tchan;
	uint16_t dcid;

	link_up();
	tchan = incoming_waiting(L1_PSM, PEER_SCID);

	features_page0_ext();
	expect_no_rsp();

	features_page1(BT_HCI_ERR_SUCCESS, false);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, &dcid);
	zassert_equal(auth_count, 0U, "Security started");

	zassert_ok(k_sem_take(&conf_req_sem, EVENT_TIMEOUT), "No Configuration Request");
	peer_sig_send(BT_L2CAP_CONF_RSP, conf_req_ident, &conf_rsp, sizeof(conf_rsp));

	conf_req.dcid = sys_cpu_to_le16(dcid);
	peer_sig_send(BT_L2CAP_CONF_REQ, ++peer_ident, &conf_req, sizeof(conf_req));

	zassert_ok(k_sem_take(&chan_connected_sem, EVENT_TIMEOUT), "Channel not connected");
	zassert_equal(tchan->connected, 1U, "%u connected()", tchan->connected);
}

/* No extended features: there is no page 1 to wait for */
static ZTEST(l2cap_br_features, test_incoming_no_ext_features_admitted)
{
	link_up();
	(void)incoming_waiting(L1_PSM, PEER_SCID);

	features_page0(BT_HCI_ERR_SUCCESS, false);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);
}

/* A peer that never indicated Secure Simple Pairing is one without it */
static ZTEST(l2cap_br_features, test_incoming_page0_failed)
{
	link_up();
	(void)incoming_waiting(L1_PSM, PEER_SCID);

	features_page0(BT_HCI_ERR_LL_RESP_TIMEOUT, false);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);
}

static ZTEST(l2cap_br_features, test_incoming_page1_failed)
{
	link_up();
	(void)incoming_waiting(L1_PSM, PEER_SCID);

	features_page0_ext();
	features_page1(BT_HCI_ERR_LL_RESP_TIMEOUT, false);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);
}

/* No completion event follows a refused command, so this ends the wait too */
static ZTEST(l2cap_br_features, test_incoming_page1_cmd_refused)
{
	link_up();
	(void)incoming_waiting(L1_PSM, PEER_SCID);

	ext_features_cmd_status = BT_HCI_ERR_CMD_DISALLOWED;
	features_page0(BT_HCI_ERR_SUCCESS, true);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);
}

static ZTEST(l2cap_br_features, test_incoming_page0_cmd_refused)
{
	features_cmd_status = BT_HCI_ERR_CMD_DISALLOWED;
	link_up();

	peer_conn_req(L1_PSM, PEER_SCID);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);
}

/* Encryption settles what the features would have */
static ZTEST(l2cap_br_features, test_incoming_encrypted_while_waiting)
{
	link_up();
	(void)incoming_waiting(L1_PSM, PEER_SCID);

	encrypt_change(BT_HCI_ERR_SUCCESS, BT_HCI_ENCRYPTION_ON_BR_E0);
	zassert_ok(k_sem_take(&security_changed_sem, EVENT_TIMEOUT), "No security_changed");
	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);

	/* The channel is answered, once */
	features_report(true);
	expect_no_rsp();
}

/* The channel was not waiting for this procedure: the features still decide */
static ZTEST(l2cap_br_features, test_incoming_other_procedure_fails_while_waiting)
{
	link_up();
	(void)incoming_waiting(L1_PSM, PEER_SCID);

	zassert_ok(bt_conn_set_security(test_conn, BT_SECURITY_L2));
	zassert_ok(k_sem_take(&auth_sem, EVENT_TIMEOUT), "No authentication");
	auth_complete(BT_HCI_ERR_AUTH_FAIL);
	zassert_ok(k_sem_take(&security_changed_sem, EVENT_TIMEOUT), "No security_changed");
	expect_no_rsp();

	features_report(false);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);
}

static ZTEST(l2cap_br_features, test_incoming_link_down_while_waiting)
{
	const struct test_chan *tchan;

	link_up();
	tchan = incoming_waiting(L1_PSM, PEER_SCID);

	disconn_complete();
	zassert_ok(k_sem_take(&disconnected_sem, EVENT_TIMEOUT), "No disconnected callback");
	settle();

	zassert_equal(tchan->connected, 0U, "Channel connected");
	zassert_equal(tchan->disconnected, 1U, "No disconnected()");
	zassert_equal(tchan->released, 1U, "No released()");

	/* The handle is gone by now */
	features_page0(BT_HCI_ERR_SUCCESS, false);
	expect_no_rsp();
}

/* The link is going down: what the features would decide is left to that */
static ZTEST(l2cap_br_features, test_incoming_disconnecting_while_waiting)
{
	const struct test_chan *tchan;

	link_up();
	tchan = incoming_waiting(L1_PSM, PEER_SCID);

	zassert_ok(bt_conn_disconnect(test_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN));
	settle();

	features_page0(BT_HCI_ERR_SUCCESS, false);
	expect_no_rsp();
	zassert_equal(tchan->connected, 0U, "Channel connected");

	disconn_complete();
	zassert_ok(k_sem_take(&disconnected_sem, EVENT_TIMEOUT), "No disconnected callback");
	settle();
	zassert_equal(tchan->released, 1U, "No released()");
}

/* The signaling buffer stays with the Controller, the default pool in the given queue. An answer
 * then has to wait for one of them.
 */
static void tx_bufs_take(struct k_fifo *pool_bufs)
{
	struct bt_l2cap_info_req info_req = {
		.type = sys_cpu_to_le16(BT_L2CAP_INFO_FEAT_MASK),
	};
	struct net_buf *buf;

	hold_acl = true;
	peer_sig_send(BT_L2CAP_INFO_REQ, ++peer_ident, &info_req, sizeof(info_req));
	zassert_ok(k_sem_take(&held_sem, EVENT_TIMEOUT), "No Information Response");
	hold_acl = false;

	buf = bt_conn_create_pdu_timeout(NULL, 0, K_NO_WAIT);
	while (buf != NULL) {
		k_fifo_put(pool_bufs, buf);
		buf = bt_conn_create_pdu_timeout(NULL, 0, K_NO_WAIT);
	}
}

static void tx_bufs_give(struct k_fifo *pool_bufs)
{
	struct net_buf *buf;

	net_buf_drop(&held_acl);

	buf = k_fifo_get(pool_bufs, K_NO_WAIT);
	while (buf != NULL) {
		net_buf_unref(buf);
		buf = k_fifo_get(pool_bufs, K_NO_WAIT);
	}
}

/* Out of buffers when the features arrive: the answer is late, not lost */
static void answer_waits_for_buffer(bool ssp, uint16_t result)
{
	struct k_fifo pool_bufs;

	k_fifo_init(&pool_bufs);

	link_up();
	(void)incoming_waiting(L1_PSM, PEER_SCID);

	/* Page 1 is read before the buffers are gone */
	features_page0_ext();
	tx_bufs_take(&pool_bufs);

	features_page1(BT_HCI_ERR_SUCCESS, ssp);
	expect_no_rsp();

	tx_bufs_give(&pool_bufs);

	expect_rsp(PEER_SCID, result, BT_L2CAP_CS_NO_INFO, NULL);
	expect_no_rsp();
}

static ZTEST(l2cap_br_features, test_incoming_admission_waits_for_buffer)
{
	answer_waits_for_buffer(false, BT_L2CAP_BR_SUCCESS);
}

static ZTEST(l2cap_br_features, test_incoming_refusal_waits_for_buffer)
{
	answer_waits_for_buffer(true, BT_L2CAP_BR_ERR_SEC_BLOCK);
}

/* Encryption admitted the channel, its answer waits for a buffer, then the features arrive */
static ZTEST(l2cap_br_features, test_incoming_encrypted_answer_waits_for_buffer)
{
	struct k_fifo pool_bufs;

	k_fifo_init(&pool_bufs);

	link_up();
	(void)incoming_waiting(L1_PSM, PEER_SCID);

	features_page0_ext();
	tx_bufs_take(&pool_bufs);

	encrypt_change(BT_HCI_ERR_SUCCESS, BT_HCI_ENCRYPTION_ON_BR_E0);
	zassert_ok(k_sem_take(&security_changed_sem, EVENT_TIMEOUT), "No security_changed");
	features_page1(BT_HCI_ERR_SUCCESS, true);
	expect_no_rsp();

	tx_bufs_give(&pool_bufs);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);
	expect_no_rsp();
}

/* The wait is over and the answer waits for a buffer: a procedure failing now refuses the channel
 * as any other waiting for an answer. Only the first response counts here.
 */
static void waiting_over_then_procedure_fails(void (*wait_ends)(void))
{
	struct k_fifo pool_bufs;

	k_fifo_init(&pool_bufs);

	link_up();
	(void)incoming_waiting(L1_PSM, PEER_SCID);

	features_page0_ext();
	zassert_ok(bt_conn_set_security(test_conn, BT_SECURITY_L2));
	zassert_ok(k_sem_take(&auth_sem, EVENT_TIMEOUT), "No authentication");
	tx_bufs_take(&pool_bufs);

	wait_ends();
	auth_complete(BT_HCI_ERR_AUTH_FAIL);
	zassert_ok(k_sem_take(&security_changed_sem, EVENT_TIMEOUT), "No security_changed");
	expect_no_rsp();

	tx_bufs_give(&pool_bufs);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK, BT_L2CAP_CS_NO_INFO, NULL);
}

static void wait_ends_with_encryption(void)
{
	encrypt_change(BT_HCI_ERR_SUCCESS, BT_HCI_ENCRYPTION_ON_BR_E0);
	zassert_ok(k_sem_take(&security_changed_sem, EVENT_TIMEOUT), "No security_changed");
}

static void wait_ends_with_features(void)
{
	features_page1(BT_HCI_ERR_SUCCESS, false);
	settle();
}

static ZTEST(l2cap_br_features, test_incoming_procedure_fails_after_encryption)
{
	waiting_over_then_procedure_fails(wait_ends_with_encryption);
}

static ZTEST(l2cap_br_features, test_incoming_procedure_fails_after_features)
{
	waiting_over_then_procedure_fails(wait_ends_with_features);
}

/* The cases that wait for nothing */
static ZTEST(l2cap_br_features, test_incoming_features_known)
{
	link_up();
	features_report(false);

	peer_conn_req(L1_PSM, PEER_SCID);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);
}

static ZTEST(l2cap_br_features, test_incoming_features_known_ssp_refused)
{
	link_up();
	features_report(true);

	peer_conn_req(L1_PSM, PEER_SCID);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK, BT_L2CAP_CS_NO_INFO, NULL);
}

static ZTEST(l2cap_br_features, test_incoming_encrypted_link)
{
	link_up_encrypted(BT_HCI_ENCRYPTION_ON_BR_E0);

	peer_conn_req(L1_PSM, PEER_SCID);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);
}

static ZTEST(l2cap_br_features, test_incoming_sdp)
{
	link_up();

	peer_conn_req(SDP_PSM, PEER_SCID);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);
}

static ZTEST(l2cap_br_features, test_incoming_l2_starts_security)
{
	link_up();

	peer_conn_req(L2_PSM, PEER_SCID);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_PENDING, BT_L2CAP_CS_AUTHEN_PEND, NULL);
	security_completes();
	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);
}

/* The same for RFCOMM, whatever level the DLC asks for: the session goes over L2CAP */
static ZTEST(l2cap_br_features, test_incoming_rfcomm_ssp_peer_refused)
{
	link_up();

	peer_conn_req(RFCOMM_PSM, PEER_SCID);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_PENDING, BT_L2CAP_CS_NO_INFO, NULL);

	features_report(true);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK, BT_L2CAP_CS_NO_INFO, NULL);
}

static ZTEST(l2cap_br_features, test_incoming_rfcomm_no_ssp_admitted)
{
	link_up();

	peer_conn_req(RFCOMM_PSM, PEER_SCID);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_PENDING, BT_L2CAP_CS_NO_INFO, NULL);

	features_report(false);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS, BT_L2CAP_CS_NO_INFO, NULL);
}

/* An SSP peer: security first, as when the features are known before the channel is connected */
static ZTEST(l2cap_br_features, test_outgoing_ssp_peer_security_first)
{
	const struct test_chan *tchan;

	link_up();
	tchan = outgoing_waiting();

	/* Page 0 does not tell */
	features_page0_ext();
	zassert_equal(auth_count, 0U, "Security started");
	zassert_equal(conn_req_count, 0U, "Request sent");

	features_page1(BT_HCI_ERR_SUCCESS, true);
	settle();
	zassert_equal(conn_req_count, 0U, "Request sent before security");

	security_completes();
	expect_conn_req(OUT_PSM);

	zassert_equal(tchan->disconnected, 0U, "Channel disconnected");
}

/* A connected() callback runs before the features are even asked for */
static ZTEST(l2cap_br_features, test_outgoing_from_connected)
{
	connect_in_connected = chan_new(BT_SECURITY_L1);
	link_up();

	zassert_equal(conn_req_count, 0U, "Request sent before the features");
	zassert_equal(auth_count, 0U, "Security started before the features");

	features_report(true);
	security_completes();
	expect_conn_req(OUT_PSM);
}

static ZTEST(l2cap_br_features, test_outgoing_no_ssp)
{
	link_up();
	(void)outgoing_waiting();

	features_page0_ext();
	zassert_equal(conn_req_count, 0U, "Request sent");

	features_page1(BT_HCI_ERR_SUCCESS, false);

	expect_conn_req(OUT_PSM);
	zassert_equal(auth_count, 0U, "Security started");
}

static ZTEST(l2cap_br_features, test_outgoing_page0_failed)
{
	link_up();
	(void)outgoing_waiting();

	features_page0(BT_HCI_ERR_LL_RESP_TIMEOUT, false);

	expect_conn_req(OUT_PSM);
}

/* Encryption settles what the features would have, and they do not send a second request */
static ZTEST(l2cap_br_features, test_outgoing_encrypted_while_waiting)
{
	link_up();
	(void)outgoing_waiting();

	encrypt_change(BT_HCI_ERR_SUCCESS, BT_HCI_ENCRYPTION_ON_BR_E0);
	zassert_ok(k_sem_take(&security_changed_sem, EVENT_TIMEOUT), "No security_changed");
	expect_conn_req(OUT_PSM);

	features_report(true);
	zassert_equal(conn_req_count, 1U, "%u Connection Requests", conn_req_count);
	zassert_equal(auth_count, 0U, "Security started");
}

/* The channel was not waiting for this procedure: the features still decide */
static ZTEST(l2cap_br_features, test_outgoing_other_procedure_fails_while_waiting)
{
	const struct test_chan *tchan;

	link_up();
	tchan = outgoing_waiting();

	zassert_ok(bt_conn_set_security(test_conn, BT_SECURITY_L2));
	zassert_ok(k_sem_take(&auth_sem, EVENT_TIMEOUT), "No authentication");
	auth_complete(BT_HCI_ERR_AUTH_FAIL);
	zassert_ok(k_sem_take(&security_changed_sem, EVENT_TIMEOUT), "No security_changed");
	settle();
	zassert_equal(tchan->released, 0U, "Channel released");
	zassert_equal(conn_req_count, 0U, "Request sent");

	features_report(false);
	expect_conn_req(OUT_PSM);
}

/* Started for the channel after the features, this procedure is the channel's own */
static ZTEST(l2cap_br_features, test_outgoing_security_fails)
{
	const struct test_chan *tchan;

	link_up();
	tchan = outgoing_waiting();

	features_report(true);
	zassert_ok(k_sem_take(&auth_sem, EVENT_TIMEOUT), "No authentication");
	auth_complete(BT_HCI_ERR_AUTH_FAIL);
	zassert_ok(k_sem_take(&security_changed_sem, EVENT_TIMEOUT), "No security_changed");
	settle();

	zassert_equal(tchan->disconnected, 1U, "No disconnected()");
	zassert_equal(tchan->released, 1U, "No released()");
	zassert_equal(conn_req_count, 0U, "Request sent");
}

/* No security can start while the peer is pairing. Every channel is told, one after the other
 * released.
 */
static ZTEST(l2cap_br_features, test_outgoing_security_cannot_start)
{
	const struct test_chan *first, *second;

	overwrite_on_release = true;

	link_up();
	first = outgoing_waiting();
	second = outgoing_waiting();

	io_capa_resp();
	features_report(true);

	zassert_equal(first->released, 1U, "%u released()", first->released);
	zassert_equal(second->released, 1U, "%u released()", second->released);
	zassert_equal(first->disconnected, 1U, "%u disconnected()", first->disconnected);
	zassert_equal(second->disconnected, 1U, "%u disconnected()", second->disconnected);
	zassert_equal(auth_count, 0U, "Security started");
	zassert_equal(conn_req_count, 0U, "Request sent");
}

/* Starting a channel can block, and another thread can take the next channel off the list then */
static ZTEST(l2cap_br_features, test_outgoing_next_channel_gone_during_start)
{
	const struct test_chan *first;
	struct test_chan *second;

	overwrite_on_release = true;

	link_up();
	first = outgoing_waiting();
	second = outgoing_waiting();
	remove_on_disconnected = second;

	io_capa_resp();
	features_report(true);

	zassert_equal(first->released, 1U, "%u released()", first->released);
	zassert_equal(second->released, 1U, "%u released()", second->released);
	zassert_equal(second->disconnected, 1U, "%u disconnected()", second->disconnected);
}

/* The application gave up on the channel in the meantime */
static ZTEST(l2cap_br_features, test_outgoing_disconnected_while_waiting)
{
	struct test_chan *tchan;

	link_up();
	tchan = outgoing_waiting();

	zassert_ok(bt_l2cap_chan_disconnect(&tchan->br.chan));
	settle();

	features_report(true);

	zassert_equal(auth_count, 0U, "Security started");
	zassert_equal(conn_req_count, 0U, "Request sent");
}

static ZTEST(l2cap_br_features, test_outgoing_link_down_while_waiting)
{
	const struct test_chan *tchan;

	link_up();
	tchan = outgoing_waiting();

	disconn_complete();
	zassert_ok(k_sem_take(&disconnected_sem, EVENT_TIMEOUT), "No disconnected callback");
	settle();

	zassert_equal(tchan->disconnected, 1U, "No disconnected()");
	zassert_equal(tchan->released, 1U, "No released()");
	zassert_equal(conn_req_count, 0U, "Request sent");
}

/* The cases that wait for nothing */
static ZTEST(l2cap_br_features, test_outgoing_features_known)
{
	struct test_chan *tchan = chan_new(BT_SECURITY_L1);

	link_up();
	features_report(false);

	zassert_ok(bt_l2cap_chan_connect(test_conn, &tchan->br.chan, OUT_PSM));

	expect_conn_req(OUT_PSM);
}

static ZTEST(l2cap_br_features, test_outgoing_features_known_ssp)
{
	struct test_chan *tchan = chan_new(BT_SECURITY_L1);

	link_up();
	features_report(true);

	zassert_ok(bt_l2cap_chan_connect(test_conn, &tchan->br.chan, OUT_PSM));
	settle();
	zassert_equal(conn_req_count, 0U, "Request sent before security");

	security_completes();
	expect_conn_req(OUT_PSM);
}

static ZTEST(l2cap_br_features, test_outgoing_sdp)
{
	struct test_chan *tchan = chan_new(BT_SECURITY_L0);

	link_up();

	zassert_ok(bt_l2cap_chan_connect(test_conn, &tchan->br.chan, SDP_PSM));

	expect_conn_req(SDP_PSM);
}

static ZTEST(l2cap_br_features, test_outgoing_l2_starts_security)
{
	struct test_chan *tchan = chan_new(BT_SECURITY_L2);

	link_up();

	zassert_ok(bt_l2cap_chan_connect(test_conn, &tchan->br.chan, OUT_PSM));
	settle();
	zassert_equal(conn_req_count, 0U, "Request sent before security");

	security_completes();
	expect_conn_req(OUT_PSM);
}

static void rfcomm_connect_l1(void)
{
	static struct bt_rfcomm_dlc_ops ops;

	dlc.ops = &ops;
	dlc.mtu = RX_MTU;
	dlc.required_sec_level = BT_SECURITY_L1;

	zassert_ok(bt_rfcomm_dlc_connect(test_conn, &dlc, 1U));
	settle();
	zassert_equal(conn_req_count, 0U, "Request sent before the features");
	zassert_equal(auth_count, 0U, "Security started before the features");
}

/* The L2CAP channel of the session carries the level of the DLC */
static ZTEST(l2cap_br_features, test_outgoing_rfcomm_ssp_peer_security_first)
{
	link_up();
	rfcomm_connect_l1();

	features_report(true);
	settle();
	zassert_equal(conn_req_count, 0U, "Request sent before security");

	security_completes();
	expect_conn_req(RFCOMM_PSM);
}

static ZTEST(l2cap_br_features, test_outgoing_rfcomm_no_ssp)
{
	link_up();
	rfcomm_connect_l1();

	features_report(false);

	expect_conn_req(RFCOMM_PSM);
	zassert_equal(auth_count, 0U, "Security started");
}

static void *setup(void)
{
	static struct bt_l2cap_server l1_server = {
		.psm = L1_PSM,
		.sec_level = BT_SECURITY_L1,
		.accept = server_accept,
	};
	static struct bt_l2cap_server l2_server = {
		.psm = L2_PSM,
		.sec_level = BT_SECURITY_L2,
		.accept = server_accept,
	};

	zassert_ok(bt_l2cap_br_server_register(&l1_server));
	zassert_ok(bt_l2cap_br_server_register(&l2_server));

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(bt_enable(NULL));
}

static void after(void *fixture)
{
	struct peer_rsp rsp;

	ARG_UNUSED(fixture);

	hold_acl = false;
	net_buf_drop(&held_acl);

	zassert_ok(bt_disable());

	/* The connection and the channels on it go on the queue of the Host */
	workq_flush(bt_work_submit);
	bt_conn_drop(&test_conn);

	features_cmd_status = BT_HCI_ERR_SUCCESS;
	ext_features_cmd_status = BT_HCI_ERR_SUCCESS;
	connect_in_connected = NULL;
	overwrite_on_release = false;
	remove_on_disconnected = NULL;
	auth_count = 0U;
	conn_req_count = 0U;
	conn_req_psm = 0U;
	conf_req_ident = 0U;
	chans_used = 0U;
	(void)memset(chans, 0, sizeof(chans));
	(void)memset(&dlc, 0, sizeof(dlc));

	while (k_msgq_get(&rsp_msgq, &rsp, K_NO_WAIT) == 0) {
		/* Left by a test that failed */
	}

	k_sem_reset(&connected_sem);
	k_sem_reset(&features_sem);
	k_sem_reset(&ext_features_sem);
	k_sem_reset(&auth_sem);
	k_sem_reset(&conn_req_sem);
	k_sem_reset(&conf_req_sem);
	k_sem_reset(&security_changed_sem);
	k_sem_reset(&disconnected_sem);
	k_sem_reset(&chan_connected_sem);
	k_sem_reset(&held_sem);
}

ZTEST_SUITE(l2cap_br_features, NULL, setup, before, after, NULL);
