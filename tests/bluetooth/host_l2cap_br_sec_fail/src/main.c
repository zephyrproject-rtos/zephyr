/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* BR/EDR channels waiting for a security procedure when it fails. released() overwrites the channel
 * as an application freeing it may, with all ones so that flags read as set and pointers fault:
 * any later read by the Host shows in these tests.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/classic/avrcp.h>
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
#define MAX_RSPS      8U

#define SERVER_PSM   0x1001U
#define HIGH_SEC_PSM 0x1003U
#define OUT_PSM      0x1005U
#define OUT_PSM_2    0x1007U
#define SDP_PSM      0x0001U
#define PEER_SCID    0x0040U
#define PEER_SCID_2  0x0041U
#define RX_MTU       48U

#define CONNECT_STACK_SIZE 2048U

/* A channel, and what its callbacks saw. released() leaves the counters in place. */
struct test_chan {
	struct bt_l2cap_br_chan br;
	uint32_t connected;
	uint32_t disconnected;
	uint32_t released;
	uint32_t encrypt_change;
	uint8_t encrypt_status;
};

struct conn_rsp {
	uint16_t scid;
	uint16_t result;
};

static const bt_addr_t peer = {
	.val = { 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U },
};

/* What the Controller answers */
static uint8_t auth_cmd_status;
static bool auth_cmd_hold;
static uint8_t enc_cmd_status;
static bool auth_at_once;
static uint8_t auth_at_once_status;
static bool disconnected_at_once;
static uint8_t enc_evt_status;
static bool hold_acl;

/* What the Host sent, also a buffer that the Controller is done with but keeps */
static uint32_t auth_count;
static uint32_t conn_req_count;
static uint32_t disconnect_count;
static struct net_buf *held_acl;
K_MSGQ_DEFINE(rsp_msgq, sizeof(struct conn_rsp), MAX_RSPS, 2);

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static K_SEM_DEFINE(features_sem, 0, 1);
static K_SEM_DEFINE(auth_sem, 0, 1);
static K_SEM_DEFINE(conn_req_sem, 0, NUM_CHANS);
static K_SEM_DEFINE(held_sem, 0, 1);
static K_SEM_DEFINE(security_changed_sem, 0, 1);
static K_SEM_DEFINE(released_sem, 0, NUM_CHANS);

static enum bt_security_err security_err;

static struct bt_conn *test_conn;
static struct test_chan chans[NUM_CHANS];
static size_t chans_used;
static uint8_t peer_ident;
static bool reconnect_on_release;

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

/* Without a Secure Simple Pairing bit in them */
static void remote_features(void)
{
	struct bt_hci_evt_remote_features *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_REMOTE_FEATURES, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
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

static void key_refresh_failed(void)
{
	struct bt_hci_evt_encrypt_key_refresh_complete *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_LMP_PDU_NOT_ALLOWED;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	evt_send(buf);
}

static void conn_complete(void)
{
	struct bt_hci_evt_conn_complete *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_CONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->link_type = BT_HCI_ACL;
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
		cmd_status(opcode, BT_HCI_ERR_SUCCESS);
		break;
	case BT_HCI_OP_READ_REMOTE_FEATURES:
		cmd_status(opcode, BT_HCI_ERR_SUCCESS);
		remote_features();
		k_sem_give(&features_sem);
		break;
	case BT_HCI_OP_DISCONNECT:
		disconnect_count++;
		cmd_status(opcode, BT_HCI_ERR_SUCCESS);
		break;
	case BT_HCI_OP_AUTH_REQUESTED:
		auth_count++;
		if (auth_cmd_hold) {
			k_sem_give(&auth_sem);
			break;
		}
		cmd_status(opcode, auth_cmd_status);
		if (auth_at_once) {
			auth_complete(auth_at_once_status);
		}
		if (disconnected_at_once) {
			disconn_complete();
		}
		k_sem_give(&auth_sem);
		break;
	case BT_HCI_OP_SET_CONN_ENCRYPT:
		cmd_status(opcode, enc_cmd_status);
		if (enc_cmd_status == BT_HCI_ERR_SUCCESS) {
			encrypt_change(enc_evt_status, (enc_evt_status == BT_HCI_ERR_SUCCESS)
							       ? BT_HCI_ENCRYPTION_ON_BR_E0
							       : BT_HCI_ENCRYPTION_OFF);
		}
		break;
	case BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS:
		break;
	default:
		generic_complete(opcode);
		break;
	}
}

/* Returns true for a Connection Response, the result of which then is in rsp */
static bool acl_handle(struct net_buf *buf, struct conn_rsp *rsp)
{
	const struct bt_l2cap_conn_rsp *l2cap_rsp;
	const struct bt_l2cap_sig_hdr *sig;
	const struct bt_l2cap_hdr *hdr;

	(void)net_buf_pull_mem(buf, sizeof(struct bt_hci_acl_hdr));
	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	if (sys_le16_to_cpu(hdr->cid) != BT_L2CAP_CID_BR_SIG) {
		return false;
	}

	sig = net_buf_pull_mem(buf, sizeof(*sig));
	if (sig->code == BT_L2CAP_CONN_REQ) {
		conn_req_count++;
		k_sem_give(&conn_req_sem);
		return false;
	}

	if (sig->code != BT_L2CAP_CONN_RSP) {
		return false;
	}

	l2cap_rsp = (const void *)buf->data;
	rsp->scid = sys_le16_to_cpu(l2cap_rsp->scid);
	rsp->result = sys_le16_to_cpu(l2cap_rsp->result);

	return true;
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
	struct conn_rsp rsp;
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

	/* Before telling the test, which counts on this buffer being free */
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

static struct test_chan *chan_alloc(const struct bt_l2cap_chan_ops *ops, bt_security_t sec_level)
{
	struct test_chan *tchan;

	zassert_true(chans_used < ARRAY_SIZE(chans), "Out of channels");

	tchan = &chans[chans_used++];
	tchan->br.chan.ops = ops;
	tchan->br.rx.mtu = RX_MTU;
	tchan->br.required_sec_level = sec_level;

	return tchan;
}

static void chan_connected(struct bt_l2cap_chan *chan)
{
	CONTAINER_OF(chan, struct test_chan, br.chan)->connected++;
}

static void chan_disconnected(struct bt_l2cap_chan *chan)
{
	CONTAINER_OF(chan, struct test_chan, br.chan)->disconnected++;
}

static void chan_released(struct bt_l2cap_chan *chan)
{
	struct test_chan *tchan = CONTAINER_OF(chan, struct test_chan, br.chan);
	const struct bt_l2cap_chan_ops *ops = chan->ops;

	tchan->released++;
	(void)memset(&tchan->br, 0xFF, sizeof(tchan->br));
	k_sem_give(&released_sem);

	if (reconnect_on_release) {
		tchan = chan_alloc(ops, BT_SECURITY_L2);
		zassert_ok(bt_l2cap_chan_connect(test_conn, &tchan->br.chan, OUT_PSM));
	}
}

static void chan_encrypt_change(struct bt_l2cap_chan *chan, uint8_t hci_status)
{
	struct test_chan *tchan = CONTAINER_OF(chan, struct test_chan, br.chan);

	tchan->encrypt_change++;
	tchan->encrypt_status = hci_status;
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
		.encrypt_change = chan_encrypt_change,
		.recv = chan_recv,
	};

	return chan_alloc(&ops, sec_level);
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
	ARG_UNUSED(conn);
	ARG_UNUSED(err);

	k_sem_give(&connected_sem);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(level);

	security_err = err;
	k_sem_give(&security_changed_sem);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);

	k_sem_give(&disconnected_sem);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void link_up(void)
{
	test_conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(test_conn, "Creating the connection failed");

	conn_complete();
	zassert_ok(k_sem_take(&connected_sem, EVENT_TIMEOUT), "No connected callback");
	zassert_ok(k_sem_take(&features_sem, EVENT_TIMEOUT), "No remote features read");
}

static void peer_sig_send(uint8_t code, const void *data, uint16_t len)
{
	struct bt_l2cap_sig_hdr sig = {
		.code = code,
		.ident = ++peer_ident,
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

static void expect_auth(void)
{
	zassert_ok(k_sem_take(&auth_sem, EVENT_TIMEOUT), "No authentication");
}

static void expect_rsp(uint16_t scid, uint16_t result)
{
	struct conn_rsp rsp;

	zassert_ok(k_msgq_get(&rsp_msgq, &rsp, EVENT_TIMEOUT), "No Connection Response");
	zassert_equal(rsp.scid, scid, "SCID 0x%04x", rsp.scid);
	zassert_equal(rsp.result, result, "Result 0x%04x", rsp.result);
}

static void peer_conn_req(uint16_t psm, uint16_t scid)
{
	struct bt_l2cap_conn_req req = {
		.psm = sys_cpu_to_le16(psm),
		.scid = sys_cpu_to_le16(scid),
	};

	peer_sig_send(BT_L2CAP_CONN_REQ, &req, sizeof(req));
}

static void expect_released(const struct test_chan *tchan)
{
	size_t idx = tchan - chans;

	zassert_equal(tchan->connected, 0U, "chan %zu connected", idx);
	zassert_equal(tchan->disconnected, 1U, "chan %zu: %u disconnected", idx,
		      tchan->disconnected);
	zassert_equal(tchan->released, 1U, "chan %zu: %u released", idx, tchan->released);
}

static void expect_kept(const struct test_chan *tchan)
{
	zassert_equal(tchan->disconnected, 0U, "Channel disconnected");
	zassert_equal(tchan->released, 0U, "Channel released");
}

/* A peer's request now waiting for the security procedure, with the first free channel */
static struct test_chan *incoming_pending(uint16_t scid)
{
	struct test_chan *tchan = &chans[chans_used];

	peer_conn_req(SERVER_PSM, scid);
	expect_rsp(scid, BT_L2CAP_BR_PENDING);

	return tchan;
}

/* A local request now waiting for the security procedure */
static struct test_chan *outgoing_pending(uint16_t psm)
{
	struct test_chan *tchan = chan_new(BT_SECURITY_L2);

	zassert_ok(bt_l2cap_chan_connect(test_conn, &tchan->br.chan, psm), "Connecting failed");
	expect_kept(tchan);

	return tchan;
}

static void expect_security_changed(enum bt_security_err err)
{
	zassert_ok(k_sem_take(&security_changed_sem, EVENT_TIMEOUT), "No security_changed");
	zassert_equal(security_err, err, "Error %d", security_err);
}

/* The Controller's result of the security procedure in flight. A failure of authentication or
 * encryption has one effect on the channels.
 */
static void procedure_fails(uint8_t auth_status, uint8_t enc_status)
{
	enc_evt_status = enc_status;
	auth_complete(auth_status);
	expect_security_changed((auth_status != BT_HCI_ERR_SUCCESS) ? BT_SECURITY_ERR_AUTH_FAIL
								    : BT_SECURITY_ERR_UNSPECIFIED);
}

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

static void expect_link_kept(void)
{
	/* TX completions, which is where a refusal can disconnect the link */
	workq_flush(k_work_submit);
	zassert_equal(disconnect_count, 0U, "The link was disconnected");
}

/* Every channel on the link waits for the one procedure */
static void outgoing_all_released(uint8_t auth_status, uint8_t enc_status)
{
	const struct test_chan *chan[3];

	link_up();

	chan[0] = outgoing_pending(OUT_PSM);
	expect_auth();
	chan[1] = outgoing_pending(OUT_PSM_2);
	chan[2] = outgoing_pending(OUT_PSM);
	zassert_equal(auth_count, 1U, "%u procedures", auth_count);

	procedure_fails(auth_status, enc_status);

	ARRAY_FOR_EACH(chan, i) {
		expect_released(chan[i]);
	}
	zassert_equal(conn_req_count, 0U, "Requests sent without security");
	expect_link_kept();
}

static ZTEST(l2cap_br_sec_fail, test_outgoing_all_released_auth_fails)
{
	outgoing_all_released(BT_HCI_ERR_AUTH_FAIL, BT_HCI_ERR_SUCCESS);
}

static ZTEST(l2cap_br_sec_fail, test_outgoing_all_released_encryption_fails)
{
	outgoing_all_released(BT_HCI_ERR_SUCCESS, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED);
}

/* A channel not waiting stays, and is told. Before it in the list goes one that was released. */
static ZTEST(l2cap_br_sec_fail, test_later_channel_told)
{
	const struct test_chan *waiting, *waiting_2;
	struct test_chan *other;

	link_up();

	waiting = outgoing_pending(OUT_PSM);
	expect_auth();

	other = chan_new(BT_SECURITY_L0);
	zassert_ok(bt_l2cap_chan_connect(test_conn, &other->br.chan, SDP_PSM), "Connecting failed");
	zassert_ok(k_sem_take(&conn_req_sem, EVENT_TIMEOUT), "No Connection Request");

	waiting_2 = outgoing_pending(OUT_PSM_2);

	procedure_fails(BT_HCI_ERR_AUTH_FAIL, BT_HCI_ERR_SUCCESS);

	expect_released(waiting);
	expect_released(waiting_2);
	zassert_equal(waiting->encrypt_change, 0U, "Released channel told");
	zassert_equal(waiting_2->encrypt_change, 0U, "Released channel told");

	expect_kept(other);
	zassert_equal(other->encrypt_change, 1U, "Told %u times", other->encrypt_change);
	zassert_equal(other->encrypt_status, BT_HCI_ERR_AUTH_FAIL, "Status 0x%02x",
		      other->encrypt_status);
}

/* Closed as refused: the peer gets its SCID back and a new request starts over */
static void incoming_refused(uint8_t auth_status, uint8_t enc_status)
{
	const struct test_chan *first, *second;

	link_up();

	first = incoming_pending(PEER_SCID);
	expect_auth();
	procedure_fails(auth_status, enc_status);
	expect_released(first);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK);

	second = incoming_pending(PEER_SCID);
	expect_auth();
	zassert_equal(auth_count, 2U, "%u procedures", auth_count);
	procedure_fails(auth_status, enc_status);
	expect_released(second);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK);

	zassert_equal(k_msgq_num_used_get(&rsp_msgq), 0U, "More responses");
	expect_link_kept();
}

static ZTEST(l2cap_br_sec_fail, test_incoming_refused_auth_fails)
{
	incoming_refused(BT_HCI_ERR_AUTH_FAIL, BT_HCI_ERR_SUCCESS);
}

static ZTEST(l2cap_br_sec_fail, test_incoming_refused_encryption_fails)
{
	incoming_refused(BT_HCI_ERR_SUCCESS, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED);
}

/* The link is encrypted already and has to go to a level above that */
static ZTEST(l2cap_br_sec_fail, test_incoming_refused_on_encrypted_link)
{
	const struct test_chan *tchan;

	link_up();

	encrypt_change(BT_HCI_ERR_SUCCESS, BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_security_changed(BT_SECURITY_ERR_SUCCESS);

	tchan = &chans[chans_used];
	peer_conn_req(HIGH_SEC_PSM, PEER_SCID);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_PENDING);
	expect_auth();

	procedure_fails(BT_HCI_ERR_AUTH_FAIL, BT_HCI_ERR_SUCCESS);

	expect_released(tchan);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK);
	expect_link_kept();
}

static ZTEST(l2cap_br_sec_fail, test_mixed_channels)
{
	const struct test_chan *in, *out, *in_2;

	link_up();

	in = incoming_pending(PEER_SCID);
	expect_auth();
	out = outgoing_pending(OUT_PSM);
	in_2 = incoming_pending(PEER_SCID_2);
	zassert_equal(auth_count, 1U, "%u procedures", auth_count);

	procedure_fails(BT_HCI_ERR_AUTH_FAIL, BT_HCI_ERR_SUCCESS);

	expect_released(in);
	expect_released(out);
	expect_released(in_2);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK);
	expect_rsp(PEER_SCID_2, BT_L2CAP_BR_ERR_SEC_BLOCK);
	zassert_equal(conn_req_count, 0U, "Requests sent without security");
}

/* The signaling buffer stays with the Controller, the default pool in the given queue. A response
 * then has to wait for one of them.
 */
static void tx_bufs_take(struct k_fifo *pool_bufs)
{
	struct bt_l2cap_info_req info_req = {
		.type = sys_cpu_to_le16(BT_L2CAP_INFO_FEAT_MASK),
	};
	struct net_buf *buf;

	hold_acl = true;
	peer_sig_send(BT_L2CAP_INFO_REQ, &info_req, sizeof(info_req));
	zassert_ok(k_sem_take(&held_sem, EVENT_TIMEOUT), "No Information Response");
	hold_acl = false;

	buf = bt_conn_create_pdu_timeout(NULL, 0, K_NO_WAIT);
	while (buf != NULL) {
		k_fifo_put(pool_bufs, buf);
		buf = bt_conn_create_pdu_timeout(NULL, 0, K_NO_WAIT);
	}
}

static void pool_bufs_give(struct k_fifo *pool_bufs)
{
	struct net_buf *buf;

	buf = k_fifo_get(pool_bufs, K_NO_WAIT);
	while (buf != NULL) {
		net_buf_unref(buf);
		buf = k_fifo_get(pool_bufs, K_NO_WAIT);
	}
}

/* What the Host sent on the signaling channel so far has reached the Controller */
static void acl_flush(void)
{
	struct bt_l2cap_info_req info_req = {
		.type = sys_cpu_to_le16(BT_L2CAP_INFO_FEAT_MASK),
	};

	hold_acl = true;
	peer_sig_send(BT_L2CAP_INFO_REQ, &info_req, sizeof(info_req));
	zassert_ok(k_sem_take(&held_sem, EVENT_TIMEOUT), "No Information Response");
	hold_acl = false;
	net_buf_drop(&held_acl);
}

/* The refusal waits for a buffer. The channel stays until it is sent. */
static ZTEST(l2cap_br_sec_fail, test_incoming_refused_after_buffer_wait)
{
	const struct test_chan *tchan;
	struct k_fifo pool_bufs;

	k_fifo_init(&pool_bufs);

	link_up();

	tchan = incoming_pending(PEER_SCID);
	expect_auth();

	tx_bufs_take(&pool_bufs);

	procedure_fails(BT_HCI_ERR_AUTH_FAIL, BT_HCI_ERR_SUCCESS);
	expect_kept(tchan);
	zassert_equal(k_msgq_num_used_get(&rsp_msgq), 0U, "Refused without a buffer");

	net_buf_drop(&held_acl);
	pool_bufs_give(&pool_bufs);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK);
	zassert_ok(k_sem_take(&released_sem, EVENT_TIMEOUT), "Channel not released");
	expect_released(tchan);
	expect_link_kept();
}

/* The procedure succeeded, then a later one failed, both while out of buffers: only the refusal
 * goes out. The default pool comes first, as the signaling buffer also wakes up the refusal.
 */
static ZTEST(l2cap_br_sec_fail, test_refusal_replaces_waiting_success)
{
	const struct test_chan *tchan;
	struct k_fifo pool_bufs;

	k_fifo_init(&pool_bufs);

	link_up();

	tchan = incoming_pending(PEER_SCID);
	expect_auth();

	tx_bufs_take(&pool_bufs);

	auth_complete(BT_HCI_ERR_SUCCESS);
	expect_security_changed(BT_SECURITY_ERR_SUCCESS);
	key_refresh_failed();
	expect_security_changed(BT_SECURITY_ERR_UNSPECIFIED);
	expect_kept(tchan);
	zassert_equal(k_msgq_num_used_get(&rsp_msgq), 0U, "Answered without a buffer");

	pool_bufs_give(&pool_bufs);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK);
	zassert_ok(k_sem_take(&released_sem, EVENT_TIMEOUT), "Channel not released");
	expect_released(tchan);

	net_buf_drop(&held_acl);
	workq_flush(bt_work_submit);
	zassert_equal(k_msgq_num_used_get(&rsp_msgq), 0U, "More responses");
}

/* Without an Encryption Change the Host learns of the failure from the Command Status */
static ZTEST(l2cap_br_sec_fail, test_encryption_cmd_refused)
{
	const struct test_chan *out, *in;

	link_up();

	out = outgoing_pending(OUT_PSM);
	expect_auth();
	in = incoming_pending(PEER_SCID);

	enc_cmd_status = BT_HCI_ERR_CMD_DISALLOWED;
	auth_complete(BT_HCI_ERR_SUCCESS);
	expect_security_changed(BT_SECURITY_ERR_UNSPECIFIED);

	expect_released(out);
	expect_released(in);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK);

	/* The procedure is over, so a new request starts another one */
	expect_kept(outgoing_pending(OUT_PSM_2));
	expect_auth();
	zassert_equal(auth_count, 2U, "%u procedures", auth_count);
}

/* The same for a link that is encrypted already and has to go to a level above that */
static ZTEST(l2cap_br_sec_fail, test_encryption_cmd_refused_encrypted_link)
{
	const struct test_chan *tchan;

	link_up();

	encrypt_change(BT_HCI_ERR_SUCCESS, BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_security_changed(BT_SECURITY_ERR_SUCCESS);

	tchan = &chans[chans_used];
	peer_conn_req(HIGH_SEC_PSM, PEER_SCID);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_PENDING);
	expect_auth();

	enc_cmd_status = BT_HCI_ERR_CMD_DISALLOWED;
	auth_complete(BT_HCI_ERR_SUCCESS);
	expect_security_changed(BT_SECURITY_ERR_UNSPECIFIED);

	expect_released(tchan);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK);
	zassert_equal(bt_conn_get_security(test_conn), BT_SECURITY_L2, "Level changed");
	expect_link_kept();
}

/* What released() starts waits for a procedure of its own, not for the one that just failed */
static ZTEST(l2cap_br_sec_fail, test_released_reconnects)
{
	const struct test_chan *first, *second;

	link_up();

	first = outgoing_pending(OUT_PSM);
	expect_auth();
	second = outgoing_pending(OUT_PSM_2);

	reconnect_on_release = true;
	procedure_fails(BT_HCI_ERR_AUTH_FAIL, BT_HCI_ERR_SUCCESS);
	reconnect_on_release = false;

	expect_released(first);
	expect_released(second);

	expect_auth();
	zassert_equal(auth_count, 2U, "%u procedures", auth_count);
	expect_kept(&chans[2]);
	expect_kept(&chans[3]);
}

/* The Host handles the events of auth_at_once and disconnected_at_once while this thread, which
 * waits for the command that started the procedure, gets to run only when all of that is done.
 */
static int connect_overtaken(struct test_chan *tchan)
{
	int prio = k_thread_priority_get(k_current_get());
	int err;

	k_thread_priority_set(k_current_get(), K_LOWEST_APPLICATION_THREAD_PRIO);
	err = bt_l2cap_chan_connect(test_conn, &tchan->br.chan, OUT_PSM);
	k_thread_priority_set(k_current_get(), prio);

	return err;
}

static ZTEST(l2cap_br_sec_fail, test_fails_during_connect)
{
	struct test_chan *tchan;

	link_up();

	auth_at_once = true;
	auth_at_once_status = BT_HCI_ERR_AUTH_FAIL;
	tchan = chan_new(BT_SECURITY_L2);
	zassert_equal(connect_overtaken(tchan), -EIO);
	expect_security_changed(BT_SECURITY_ERR_AUTH_FAIL);

	expect_released(tchan);
	zassert_equal(conn_req_count, 0U, "Request sent without security");
	expect_link_kept();
}

static ZTEST(l2cap_br_sec_fail, test_succeeds_during_connect)
{
	struct test_chan *tchan;

	link_up();

	auth_at_once = true;
	auth_at_once_status = BT_HCI_ERR_SUCCESS;
	tchan = chan_new(BT_SECURITY_L2);
	zassert_ok(connect_overtaken(tchan));
	expect_security_changed(BT_SECURITY_ERR_SUCCESS);

	zassert_ok(k_sem_take(&conn_req_sem, EVENT_TIMEOUT), "No Connection Request");
	expect_kept(tchan);

	acl_flush();
	zassert_equal(conn_req_count, 1U, "%u requests", conn_req_count);
}

static ZTEST(l2cap_br_sec_fail, test_disconnected_during_connect)
{
	struct test_chan *tchan;

	link_up();

	disconnected_at_once = true;
	tchan = chan_new(BT_SECURITY_L2);
	zassert_equal(connect_overtaken(tchan), -EIO);

	expect_released(tchan);
}

static ZTEST(l2cap_br_sec_fail, test_fails_and_disconnected_during_connect)
{
	struct test_chan *tchan;

	link_up();

	auth_at_once = true;
	auth_at_once_status = BT_HCI_ERR_AUTH_FAIL;
	disconnected_at_once = true;
	tchan = chan_new(BT_SECURITY_L2);
	zassert_equal(connect_overtaken(tchan), -EIO);

	expect_released(tchan);
}

static ZTEST(l2cap_br_sec_fail, test_auth_cmd_refused)
{
	struct test_chan *tchan;

	link_up();

	auth_cmd_status = BT_HCI_ERR_CMD_DISALLOWED;
	tchan = chan_new(BT_SECURITY_L2);
	zassert_equal(bt_l2cap_chan_connect(test_conn, &tchan->br.chan, OUT_PSM), -EIO);
	expect_auth();

	expect_released(tchan);
	expect_link_kept();

	/* Nothing is running, so a new request starts another procedure */
	auth_cmd_status = BT_HCI_ERR_SUCCESS;
	expect_kept(outgoing_pending(OUT_PSM_2));
	expect_auth();
	zassert_equal(auth_count, 2U, "%u procedures", auth_count);
}

static ZTEST(l2cap_br_sec_fail, test_auth_cmd_refused_and_disconnected)
{
	struct test_chan *tchan;

	link_up();

	auth_cmd_status = BT_HCI_ERR_CMD_DISALLOWED;
	disconnected_at_once = true;
	tchan = chan_new(BT_SECURITY_L2);
	zassert_equal(connect_overtaken(tchan), -EIO);

	expect_released(tchan);
}

/* A request for a higher level is refused: the running procedure goes on, and its channel waits */
static ZTEST(l2cap_br_sec_fail, test_auth_cmd_refused_with_procedure_running)
{
	const struct test_chan *waiting;
	struct test_chan *tchan;

	link_up();

	waiting = outgoing_pending(OUT_PSM);
	expect_auth();

	auth_cmd_status = BT_HCI_ERR_CMD_DISALLOWED;
	tchan = chan_new(BT_SECURITY_L3);
	zassert_equal(bt_l2cap_chan_connect(test_conn, &tchan->br.chan, OUT_PSM_2), -EIO);
	expect_auth();
	expect_released(tchan);

	/* Whatever the refused request made the signaling channel look at has been looked at */
	workq_flush(bt_work_submit);
	expect_kept(waiting);

	auth_cmd_status = BT_HCI_ERR_SUCCESS;
	auth_complete(BT_HCI_ERR_SUCCESS);
	expect_security_changed(BT_SECURITY_ERR_SUCCESS);
	zassert_ok(k_sem_take(&conn_req_sem, EVENT_TIMEOUT), "No Connection Request");
	expect_kept(waiting);
}

static void connect_entry(void *p1, void *p2, void *p3)
{
	int *err = p2;

	ARG_UNUSED(p3);

	*err = bt_l2cap_chan_connect(test_conn, p1, OUT_PSM);
}

static struct k_thread *connect_in_thread(struct test_chan *tchan, int *err)
{
	static K_THREAD_STACK_DEFINE(stack, CONNECT_STACK_SIZE);
	static struct k_thread thread;

	k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), connect_entry,
			&tchan->br.chan, err, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	return &thread;
}

/* The second channel relies on the procedure of the first, which never starts */
static ZTEST(l2cap_br_sec_fail, test_auth_cmd_refused_with_other_waiting)
{
	struct test_chan *first, *second;
	struct k_thread *thread;
	int err;

	link_up();

	auth_cmd_hold = true;
	first = chan_new(BT_SECURITY_L2);
	second = chan_new(BT_SECURITY_L2);
	thread = connect_in_thread(first, &err);
	expect_auth();

	zassert_ok(bt_l2cap_chan_connect(test_conn, &second->br.chan, OUT_PSM_2));
	expect_kept(second);
	zassert_equal(auth_count, 1U, "%u procedures", auth_count);

	cmd_status(BT_HCI_OP_AUTH_REQUESTED, BT_HCI_ERR_CMD_DISALLOWED);
	zassert_ok(k_thread_join(thread, EVENT_TIMEOUT));
	zassert_equal(err, -EIO);

	expect_released(first);
	zassert_ok(k_sem_take(&released_sem, EVENT_TIMEOUT));
	zassert_ok(k_sem_take(&released_sem, EVENT_TIMEOUT), "Second channel left waiting");
	expect_released(second);
	expect_link_kept();
}

/* The earlier procedure ends while a request is waiting for its command to be refused. What that
 * request saved of the level is out of date, and nothing is running afterwards.
 */
static ZTEST(l2cap_br_sec_fail, test_auth_cmd_refused_after_procedure_ended)
{
	const struct test_chan *waiting;
	struct test_chan *tchan;
	struct k_thread *thread;
	int err;

	link_up();

	waiting = outgoing_pending(OUT_PSM);
	expect_auth();

	auth_cmd_hold = true;
	tchan = chan_new(BT_SECURITY_L3);
	thread = connect_in_thread(tchan, &err);
	expect_auth();

	auth_complete(BT_HCI_ERR_AUTH_FAIL);
	expect_security_changed(BT_SECURITY_ERR_AUTH_FAIL);
	expect_released(waiting);

	cmd_status(BT_HCI_OP_AUTH_REQUESTED, BT_HCI_ERR_CMD_DISALLOWED);
	zassert_ok(k_thread_join(thread, EVENT_TIMEOUT));
	zassert_equal(err, -EIO);
	expect_released(tchan);

	auth_cmd_hold = false;
	expect_kept(outgoing_pending(OUT_PSM_2));
	expect_auth();
	zassert_equal(auth_count, 3U, "%u procedures", auth_count);
}

/* The refusal answers an earlier request than one that had to wait for a buffer too */
static ZTEST(l2cap_br_sec_fail, test_refusal_before_parked_request)
{
	const struct test_chan *tchan;
	struct k_fifo pool_bufs;

	k_fifo_init(&pool_bufs);

	link_up();

	tchan = incoming_pending(PEER_SCID);
	expect_auth();

	tx_bufs_take(&pool_bufs);

	procedure_fails(BT_HCI_ERR_AUTH_FAIL, BT_HCI_ERR_SUCCESS);
	peer_conn_req(SERVER_PSM, PEER_SCID);
	workq_flush(bt_work_submit);
	zassert_equal(k_msgq_num_used_get(&rsp_msgq), 0U, "Answered without a buffer");

	net_buf_drop(&held_acl);
	pool_bufs_give(&pool_bufs);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_PENDING);
	expect_released(tchan);
}

/* A request refused at one level stays refused when the link gets to a lower level afterwards */
static ZTEST(l2cap_br_sec_fail, test_refused_stays_refused)
{
	const struct test_chan *tchan;

	link_up();

	tchan = &chans[chans_used];
	peer_conn_req(HIGH_SEC_PSM, PEER_SCID);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_PENDING);
	expect_auth();

	procedure_fails(BT_HCI_ERR_AUTH_FAIL, BT_HCI_ERR_SUCCESS);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK);

	encrypt_change(BT_HCI_ERR_SUCCESS, BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_security_changed(BT_SECURITY_ERR_SUCCESS);
	zassert_equal(bt_conn_get_security(test_conn), BT_SECURITY_L2, "Level");

	acl_flush();
	zassert_equal(k_msgq_num_used_get(&rsp_msgq), 0U, "Accepted after the refusal");
	expect_released(tchan);
}

/* A refusal waiting for a buffer stands when the link then reaches the level of the channel */
static ZTEST(l2cap_br_sec_fail, test_parked_refusal_stays_at_level)
{
	const struct test_chan *tchan;
	struct k_fifo pool_bufs;

	k_fifo_init(&pool_bufs);

	link_up();

	tchan = incoming_pending(PEER_SCID);
	expect_auth();

	tx_bufs_take(&pool_bufs);

	procedure_fails(BT_HCI_ERR_AUTH_FAIL, BT_HCI_ERR_SUCCESS);
	expect_kept(tchan);

	/* A buffer is there for the success, before the retry of the refusal gets to run */
	pool_bufs_give(&pool_bufs);
	encrypt_change(BT_HCI_ERR_SUCCESS, BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_security_changed(BT_SECURITY_ERR_SUCCESS);
	zassert_equal(bt_conn_get_security(test_conn), BT_SECURITY_L2, "Level");

	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK);
	expect_released(tchan);
}

/* A failed channel with its refusal parked, then a procedure that succeeds below its level */
static ZTEST(l2cap_br_sec_fail, test_parked_refusal_stays_below_level)
{
	const struct test_chan *tchan;
	struct k_fifo pool_bufs;

	k_fifo_init(&pool_bufs);

	link_up();

	tchan = &chans[chans_used];
	peer_conn_req(HIGH_SEC_PSM, PEER_SCID);
	expect_rsp(PEER_SCID, BT_L2CAP_BR_PENDING);
	expect_auth();

	tx_bufs_take(&pool_bufs);

	procedure_fails(BT_HCI_ERR_AUTH_FAIL, BT_HCI_ERR_SUCCESS);
	expect_kept(tchan);

	/* A buffer is there for the success, before the retry of the refusal gets to run */
	pool_bufs_give(&pool_bufs);
	encrypt_change(BT_HCI_ERR_SUCCESS, BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_security_changed(BT_SECURITY_ERR_SUCCESS);
	zassert_equal(bt_conn_get_security(test_conn), BT_SECURITY_L2, "Level");

	expect_rsp(PEER_SCID, BT_L2CAP_BR_ERR_SEC_BLOCK);
	expect_released(tchan);
}

/* A procedure for another level ends in success: not for a channel that needs more */
static ZTEST(l2cap_br_sec_fail, test_success_below_channel_level)
{
	struct test_chan *tchan;

	link_up();

	tchan = chan_new(BT_SECURITY_L3);
	zassert_ok(bt_l2cap_chan_connect(test_conn, &tchan->br.chan, OUT_PSM));
	expect_auth();

	zassert_ok(bt_conn_set_security(test_conn, BT_SECURITY_L2 | BT_SECURITY_FORCE_PAIR));
	expect_auth();
	expect_kept(tchan);

	auth_complete(BT_HCI_ERR_SUCCESS);
	expect_security_changed(BT_SECURITY_ERR_SUCCESS);

	expect_released(tchan);
	acl_flush();
	zassert_equal(conn_req_count, 0U, "Request sent below the level");
}

/* AVRCP's disconnected callback relies on its connection, also on the way out of a connect call */
static ZTEST(l2cap_br_sec_fail, test_avrcp_connect_refused)
{
	atomic_val_t refs;

	link_up();

	refs = atomic_get(&test_conn->ref);
	auth_cmd_status = BT_HCI_ERR_CMD_DISALLOWED;
	zassert_equal(bt_avrcp_connect(test_conn), -EIO);
	expect_auth();

	zassert_equal(atomic_get(&test_conn->ref), refs, "Reference left behind");
}

/* The same when the channel never gets as far as being added, so no callback runs */
static ZTEST(l2cap_br_sec_fail, test_avrcp_connect_link_gone)
{
	atomic_val_t refs;

	link_up();

	disconn_complete();
	zassert_ok(k_sem_take(&disconnected_sem, EVENT_TIMEOUT), "No disconnected callback");

	refs = atomic_get(&test_conn->ref);
	zassert_equal(bt_avrcp_connect(test_conn), -ENOTCONN);
	zassert_equal(atomic_get(&test_conn->ref), refs, "Reference left behind");
}

/* The control: the same channels when the procedure succeeds */
static ZTEST(l2cap_br_sec_fail, test_procedure_succeeds)
{
	const struct test_chan *out, *in;

	link_up();

	out = outgoing_pending(OUT_PSM);
	expect_auth();
	in = incoming_pending(PEER_SCID);

	enc_evt_status = BT_HCI_ERR_SUCCESS;
	auth_complete(BT_HCI_ERR_SUCCESS);
	expect_security_changed(BT_SECURITY_ERR_SUCCESS);

	expect_rsp(PEER_SCID, BT_L2CAP_BR_SUCCESS);
	zassert_ok(k_sem_take(&conn_req_sem, EVENT_TIMEOUT), "No Connection Request");
	expect_kept(out);
	expect_kept(in);
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	ARG_UNUSED(conn);
}

static void *setup(void)
{
	static struct bt_l2cap_server server = {
		.psm = SERVER_PSM,
		.sec_level = BT_SECURITY_L2,
		.accept = server_accept,
	};
	static struct bt_l2cap_server high_sec_server = {
		.psm = HIGH_SEC_PSM,
		.sec_level = BT_SECURITY_L3,
		.accept = server_accept,
	};
	/* A display, without which the Host does not try for the higher level */
	static const struct bt_conn_auth_cb auth_cb = {
		.passkey_display = auth_passkey_display,
		.cancel = auth_cancel,
	};

	zassert_ok(bt_conn_auth_cb_register(&auth_cb));
	zassert_ok(bt_l2cap_br_server_register(&server));
	zassert_ok(bt_l2cap_br_server_register(&high_sec_server));

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(bt_enable(NULL));
}

static void after(void *fixture)
{
	struct conn_rsp rsp;

	ARG_UNUSED(fixture);

	hold_acl = false;
	reconnect_on_release = false;
	net_buf_drop(&held_acl);

	zassert_ok(bt_disable());

	/* The connection and the channels on it go on the queue of the Host */
	workq_flush(bt_work_submit);
	bt_conn_drop(&test_conn);

	enc_cmd_status = BT_HCI_ERR_SUCCESS;
	enc_evt_status = BT_HCI_ERR_SUCCESS;
	auth_cmd_status = BT_HCI_ERR_SUCCESS;
	auth_cmd_hold = false;
	auth_at_once = false;
	auth_at_once_status = BT_HCI_ERR_SUCCESS;
	disconnected_at_once = false;
	auth_count = 0U;
	conn_req_count = 0U;
	disconnect_count = 0U;
	chans_used = 0U;
	(void)memset(chans, 0, sizeof(chans));

	while (k_msgq_get(&rsp_msgq, &rsp, K_NO_WAIT) == 0) {
		/* Left by a test that failed */
	}

	k_sem_reset(&connected_sem);
	k_sem_reset(&disconnected_sem);
	k_sem_reset(&features_sem);
	k_sem_reset(&auth_sem);
	k_sem_reset(&conn_req_sem);
	k_sem_reset(&held_sem);
	k_sem_reset(&security_changed_sem);
	k_sem_reset(&released_sem);
}

ZTEST_SUITE(l2cap_br_sec_fail, NULL, setup, before, after, NULL);
