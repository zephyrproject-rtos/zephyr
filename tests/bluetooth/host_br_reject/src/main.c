/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* Requests of the peer that the Host turns down on a BR/EDR connection, and the
 * commands that a test HCI driver gets from the Host for them.
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/classic/sco_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of the commands without a handler,
 * which are zeroed.
 */
#define GENERIC_RP_SIZE 64U

#define CONN_HANDLE 0x0001U
#define ACL_MAX_LEN 255U
#define ACL_PKTS    4U

/* More than any test waits for */
#define SENT_CMDS_MAX 8U

/* The Host answers each event right away, so a test that waits this long for a
 * command fails rather than waits for one that does not come.
 */
#define HOST_TIMEOUT K_SECONDS(1)

/* The lowest reserved values of the HCI_IO_Capability_Response event */
#define IO_CAPA_RESERVED (BT_IO_NO_INPUT_OUTPUT + 1U)
#define AUTH_RESERVED    (BT_HCI_GENERAL_BONDING_MITM + 1U)

/* What the tests look at in a command that the Host sent */
struct sent_cmd {
	uint16_t opcode;
	/* The Reason parameter, of the commands that have one */
	uint8_t reason;
	/* The status that the controller answered with */
	uint8_t status;
};

struct br_reject_fixture {
	/* The ACL connection of the test */
	struct bt_conn *conn;
	/* The SCO server that the test registered */
	struct bt_sco_server *sco_server;
};

struct cmd_handler {
	uint16_t opcode;
	/* Answers the command, and fills in what the tests look at */
	void (*handler)(const uint8_t *params, struct sent_cmd *sent);
};

static const bt_addr_t peer = {
	.val = { 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U },
};

/* The commands that the Host sent since the end of bt_enable(), in order */
static struct k_msgq sent_cmds;
static atomic_t is_recording;
static atomic_t lost_cmd_cnt;

/* The controller has sent an HCI_IO_Capability_Request event, and the Host has
 * not answered it.
 */
static atomic_t is_io_capa_req_open;

static K_SEM_DEFINE(pairing_failed_sem, 0, 1);
static enum bt_security_err pairing_failed_reason;

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

static void cmd_complete(uint16_t opcode, const void *rp, uint8_t rp_len)
{
	struct bt_hci_evt_cmd_complete *cc;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + rp_len);
	cc = net_buf_add(buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);
	(void)net_buf_add_mem(buf, rp, rp_len);
	evt_send(buf);
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

/* For the commands whose return parameters are Status and BD_ADDR */
static void cmd_complete_addr(uint16_t opcode, uint8_t status, const bt_addr_t *addr)
{
	uint8_t rp[sizeof(status) + sizeof(*addr)];

	rp[0] = status;
	(void)memcpy(&rp[1], addr, sizeof(*addr));
	cmd_complete(opcode, rp, sizeof(rp));
}

static void ssp_complete(uint8_t status)
{
	struct bt_hci_evt_ssp_complete *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_SSP_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = status;
	bt_addr_copy(&evt->bdaddr, &peer);
	evt_send(buf);
}

static void generic_success(const uint8_t *params, struct sent_cmd *sent)
{
	/* Status success, and zeros for anything else */
	static const uint8_t rp[GENERIC_RP_SIZE];

	ARG_UNUSED(params);

	cmd_complete(sent->opcode, rp, sizeof(rp));
}

static void read_local_features(const uint8_t *params, struct sent_cmd *sent)
{
	struct bt_hci_rp_read_local_features rp = { 0 };

	ARG_UNUSED(params);

	rp.status = BT_HCI_ERR_SUCCESS;
	/* LE supported, and BR/EDR as the bit for BR/EDR Not Supported is clear */
	rp.features[4] = BIT(6);
	cmd_complete(sent->opcode, &rp, sizeof(rp));
}

static void read_supported_commands(const uint8_t *params, struct sent_cmd *sent)
{
	struct bt_hci_rp_read_supported_commands rp;

	ARG_UNUSED(params);

	rp.status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp.commands, 0xFF, sizeof(rp.commands));
	cmd_complete(sent->opcode, &rp, sizeof(rp));
}

static void read_bd_addr(const uint8_t *params, struct sent_cmd *sent)
{
	struct bt_hci_rp_read_bd_addr rp;

	ARG_UNUSED(params);

	rp.status = BT_HCI_ERR_SUCCESS;
	(void)memset(&rp.bdaddr, 0x11, sizeof(rp.bdaddr));
	cmd_complete(sent->opcode, &rp, sizeof(rp));
}

static void read_buffer_size(const uint8_t *params, struct sent_cmd *sent)
{
	struct bt_hci_rp_read_buffer_size rp = { 0 };

	ARG_UNUSED(params);

	rp.status = BT_HCI_ERR_SUCCESS;
	rp.acl_max_len = sys_cpu_to_le16(ACL_MAX_LEN);
	rp.acl_max_num = sys_cpu_to_le16(ACL_PKTS);
	cmd_complete(sent->opcode, &rp, sizeof(rp));
}

/* The commands that the controller answers with a Command Status event, and
 * for which the test sends the events that follow if it needs them.
 */
static void status_success(const uint8_t *params, struct sent_cmd *sent)
{
	ARG_UNUSED(params);

	cmd_status(sent->opcode, BT_HCI_ERR_SUCCESS);
}

static void reject_sync_conn(const uint8_t *params, struct sent_cmd *sent)
{
	const struct bt_hci_cp_reject_sync_conn_req *cp = (const void *)params;

	sent->reason = cp->reason;
	cmd_status(sent->opcode, BT_HCI_ERR_SUCCESS);
}

/* The controller takes one answer to each HCI_IO_Capability_Request event that
 * it sent, and refuses any other (Core Vol 4, Part E, 7.1.29 and 7.1.36).
 */
static uint8_t io_capa_answer_status(void)
{
	if (atomic_cas(&is_io_capa_req_open, 1, 0)) {
		return BT_HCI_ERR_SUCCESS;
	}

	return BT_HCI_ERR_CMD_DISALLOWED;
}

static void io_capa_reply(const uint8_t *params, struct sent_cmd *sent)
{
	const struct bt_hci_cp_io_capability_reply *cp = (const void *)params;

	sent->status = io_capa_answer_status();
	cmd_complete_addr(sent->opcode, sent->status, &cp->bdaddr);
}

static void io_capa_neg_reply(const uint8_t *params, struct sent_cmd *sent)
{
	const struct bt_hci_cp_io_capability_neg_reply *cp = (const void *)params;

	sent->reason = cp->reason;
	sent->status = io_capa_answer_status();
	cmd_complete_addr(sent->opcode, sent->status, &cp->bdaddr);

	if (sent->status == BT_HCI_ERR_SUCCESS) {
		ssp_complete(cp->reason);
	}
}

static void disconnect(const uint8_t *params, struct sent_cmd *sent)
{
	const struct bt_hci_cp_disconnect *cp = (const void *)params;
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	sent->reason = cp->reason;
	cmd_status(sent->opcode, BT_HCI_ERR_SUCCESS);

	buf = evt_create(BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = cp->handle;
	evt->reason = BT_HCI_ERR_LOCALHOST_TERM_CONN;
	evt_send(buf);
}

static void cmd_handle(struct net_buf *buf)
{
	static const struct cmd_handler cmds[] = {
		{ BT_HCI_OP_READ_LOCAL_FEATURES, read_local_features },
		{ BT_HCI_OP_READ_SUPPORTED_COMMANDS, read_supported_commands },
		{ BT_HCI_OP_READ_BD_ADDR, read_bd_addr },
		{ BT_HCI_OP_READ_BUFFER_SIZE, read_buffer_size },
		{ BT_HCI_OP_CONNECT, status_success },
		{ BT_HCI_OP_READ_REMOTE_FEATURES, status_success },
		{ BT_HCI_OP_REJECT_SYNC_CONN_REQ, reject_sync_conn },
		{ BT_HCI_OP_AUTH_REQUESTED, status_success },
		{ BT_HCI_OP_IO_CAPABILITY_REPLY, io_capa_reply },
		{ BT_HCI_OP_IO_CAPABILITY_NEG_REPLY, io_capa_neg_reply },
		{ BT_HCI_OP_DISCONNECT, disconnect },
	};
	void (*handler)(const uint8_t *params, struct sent_cmd *sent) = generic_success;
	const struct bt_hci_cmd_hdr *hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	struct sent_cmd sent = { 0 };

	sent.opcode = sys_le16_to_cpu(hdr->opcode);

	ARRAY_FOR_EACH_PTR(cmds, cmd) {
		if (cmd->opcode == sent.opcode) {
			handler = cmd->handler;
			break;
		}
	}

	handler(buf->data, &sent);

	if (atomic_get(&is_recording) == 0) {
		return;
	}

	if (k_msgq_put(&sent_cmds, &sent, K_NO_WAIT) != 0) {
		(void)atomic_inc(&lost_cmd_cnt);
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
	ARG_UNUSED(dev);

	/* ACL data is taken, but its completion is never reported */
	if (net_buf_pull_u8(buf) == BT_HCI_H4_CMD) {
		cmd_handle(buf);
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

static void conn_request(uint8_t link_type)
{
	struct bt_hci_evt_conn_request *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_CONN_REQUEST, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->link_type = link_type;
	evt_send(buf);
}

static void conn_complete(void)
{
	struct bt_hci_evt_conn_complete *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_CONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->link_type = BT_HCI_ACL;
	evt_send(buf);
}

static void io_capa_resp(uint8_t capability, uint8_t auth)
{
	struct bt_hci_evt_io_capa_resp *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_IO_CAPA_RESP, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->capability = capability;
	evt->oob_data = 0U;
	evt->authentication = auth;
	evt_send(buf);
}

static void io_capa_req(void)
{
	struct bt_hci_evt_io_capa_req *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_IO_CAPA_REQ, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	bt_addr_copy(&evt->bdaddr, &peer);
	(void)atomic_set(&is_io_capa_req_open, 1);
	evt_send(buf);
}

static void user_confirm_req(void)
{
	struct bt_hci_evt_user_confirm_req *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_USER_CONFIRM_REQ, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->passkey = 0U;
	evt_send(buf);
}

/* The next command that the Host sent, which has to be opcode */
static void expect_cmd(uint16_t opcode, struct sent_cmd *sent)
{
	zassert_ok(k_msgq_get(&sent_cmds, sent, HOST_TIMEOUT), "No command 0x%04x", opcode);
	zassert_equal(atomic_get(&lost_cmd_cnt), 0, "Commands lost");
	zassert_equal(sent->opcode, opcode, "Command 0x%04x instead of 0x%04x", sent->opcode,
		      opcode);
}

/* An ACL connection with the peer, not encrypted */
static void connect(struct br_reject_fixture *fixture)
{
	struct sent_cmd sent;

	fixture->conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(fixture->conn, "Creating the connection failed");
	expect_cmd(BT_HCI_OP_CONNECT, &sent);

	conn_complete();
	/* Sent once the connection is up */
	expect_cmd(BT_HCI_OP_READ_REMOTE_FEATURES, &sent);
}

/* The local Host starts a pairing, and answers the controller's request for its
 * IO capabilities.
 */
static void pairing_start(struct br_reject_fixture *fixture)
{
	struct sent_cmd sent;

	zassert_ok(bt_conn_set_security(fixture->conn, BT_SECURITY_L2),
		   "Starting the pairing failed");
	expect_cmd(BT_HCI_OP_AUTH_REQUESTED, &sent);

	io_capa_req();
	expect_cmd(BT_HCI_OP_IO_CAPABILITY_REPLY, &sent);
	zassert_equal(sent.status, BT_HCI_ERR_SUCCESS, "Reply refused with 0x%02x", sent.status);
}

static void sco_server_register(struct br_reject_fixture *fixture, struct bt_sco_server *server)
{
	zassert_ok(bt_sco_server_register(server), "Registering the SCO server failed");
	fixture->sco_server = server;
}

static int sco_accept_refused(const struct bt_sco_accept_info *info, struct bt_sco_chan **chan)
{
	ARGS_UNUSED(info, chan);

	return -ENOMEM;
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	ARG_UNUSED(conn);

	pairing_failed_reason = reason;
	k_sem_give(&pairing_failed_sem);
}

static void *setup(void)
{
	static struct bt_conn_auth_info_cb auth_info_cb = {
		.pairing_failed = pairing_failed,
	};
	static struct sent_cmd sent_cmds_buf[SENT_CMDS_MAX];
	static struct br_reject_fixture fixture;

	k_msgq_init(&sent_cmds, (char *)sent_cmds_buf, sizeof(sent_cmds_buf[0]),
		    ARRAY_SIZE(sent_cmds_buf));
	zassert_ok(bt_conn_auth_info_cb_register(&auth_info_cb),
		   "Registering the pairing callbacks failed");

	return &fixture;
}

static void before(void *f)
{
	ARG_UNUSED(f);

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	(void)atomic_set(&is_recording, 1);
}

static void after(void *f)
{
	struct br_reject_fixture *fixture = f;

	if (fixture->sco_server != NULL) {
		zassert_ok(bt_sco_server_unregister(fixture->sco_server),
			   "Unregistering the SCO server failed");
		fixture->sco_server = NULL;
	}

	bt_conn_drop(&fixture->conn);

	/* Also what a failed test left */
	(void)atomic_clear(&is_recording);
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	k_msgq_purge(&sent_cmds);
	(void)atomic_clear(&lost_cmd_cnt);
	(void)atomic_clear(&is_io_capa_req_open);
	k_sem_reset(&pairing_failed_sem);
	pairing_failed_reason = BT_SECURITY_ERR_SUCCESS;
}

ZTEST_SUITE(br_reject, NULL, setup, before, after, NULL);

/* A request on a link below the level that the SCO server requires gets the
 * reject command for synchronous links, with the reason of that check.
 */
static void sync_req_below_server_level(struct br_reject_fixture *fixture, uint8_t link_type)
{
	static struct bt_sco_server server = {
		.sec_level = BT_SECURITY_L2,
		.accept = sco_accept_refused,
	};
	struct sent_cmd sent;

	connect(fixture);
	sco_server_register(fixture, &server);

	conn_request(link_type);
	expect_cmd(BT_HCI_OP_REJECT_SYNC_CONN_REQ, &sent);
	zassert_equal(sent.reason, BT_HCI_ERR_INSUFFICIENT_SECURITY, "Rejected with 0x%02x",
		      sent.reason);
}

static ZTEST_F(br_reject, test_esco_req_below_server_level)
{
	sync_req_below_server_level(fixture, BT_HCI_ESCO);
}

static ZTEST_F(br_reject, test_sco_req_below_server_level)
{
	sync_req_below_server_level(fixture, BT_HCI_SCO);
}

/* Without a SCO server */
static ZTEST_F(br_reject, test_esco_req_without_server)
{
	struct sent_cmd sent;

	connect(fixture);

	conn_request(BT_HCI_ESCO);
	expect_cmd(BT_HCI_OP_REJECT_SYNC_CONN_REQ, &sent);
}

/* The SCO server does not accept the request */
static ZTEST_F(br_reject, test_esco_req_refused_by_server)
{
	static struct bt_sco_server server = {
		.sec_level = BT_SECURITY_L1,
		.accept = sco_accept_refused,
	};
	struct sent_cmd sent;

	connect(fixture);
	sco_server_register(fixture, &server);

	conn_request(BT_HCI_ESCO);
	expect_cmd(BT_HCI_OP_REJECT_SYNC_CONN_REQ, &sent);
}

/* A responder gets the IO capabilities of the peer before the request for its
 * own (Core Vol 2, Part F, 4.2.7), and answers that request with the negative
 * reply when they are reserved values.
 */
static void io_capa_reserved_responder(struct br_reject_fixture *fixture, uint8_t capability,
				       uint8_t auth)
{
	struct sent_cmd sent;

	connect(fixture);

	io_capa_resp(capability, auth);
	io_capa_req();
	expect_cmd(BT_HCI_OP_IO_CAPABILITY_NEG_REPLY, &sent);
	zassert_equal(sent.status, BT_HCI_ERR_SUCCESS, "Negative reply refused with 0x%02x",
		      sent.status);
	zassert_equal(sent.reason, BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL, "Rejected with 0x%02x",
		      sent.reason);

	/* No reply to the request, and the failed pairing ends the link */
	expect_cmd(BT_HCI_OP_DISCONNECT, &sent);
	zassert_ok(k_sem_take(&pairing_failed_sem, HOST_TIMEOUT), "No pairing failure reported");
}

static ZTEST_F(br_reject, test_io_capa_reserved_responder)
{
	io_capa_reserved_responder(fixture, IO_CAPA_RESERVED, BT_HCI_GENERAL_BONDING);
}

static ZTEST_F(br_reject, test_auth_reserved_responder)
{
	io_capa_reserved_responder(fixture, BT_IO_DISPLAY_YESNO, AUTH_RESERVED);
}

/* An initiator has answered the request when it gets the IO capabilities of the
 * peer (Core Vol 4, Part E, 7.1.29), and ends the pairing with the link.
 */
static ZTEST_F(br_reject, test_io_capa_reserved_initiator)
{
	struct sent_cmd sent;

	connect(fixture);
	pairing_start(fixture);

	io_capa_resp(IO_CAPA_RESERVED, BT_HCI_GENERAL_BONDING);
	expect_cmd(BT_HCI_OP_DISCONNECT, &sent);
	zassert_equal(sent.reason, BT_HCI_ERR_AUTH_FAIL, "Disconnected with 0x%02x", sent.reason);
	zassert_ok(k_sem_take(&pairing_failed_sem, HOST_TIMEOUT), "No pairing failure reported");
	zassert_equal(pairing_failed_reason, BT_SECURITY_ERR_AUTH_FAIL, "Pairing failed with %d",
		      pairing_failed_reason);
}

/* IO capabilities that the Host knows: a responder answers the request */
static ZTEST_F(br_reject, test_io_capa_valid_responder)
{
	struct sent_cmd sent;

	connect(fixture);

	io_capa_resp(BT_IO_DISPLAY_YESNO, BT_HCI_GENERAL_BONDING);
	io_capa_req();
	expect_cmd(BT_HCI_OP_IO_CAPABILITY_REPLY, &sent);
	zassert_equal(sent.status, BT_HCI_ERR_SUCCESS, "Reply refused with 0x%02x", sent.status);
}

/* And an initiator goes on with the pairing */
static ZTEST_F(br_reject, test_io_capa_valid_initiator)
{
	struct sent_cmd sent;

	connect(fixture);
	pairing_start(fixture);

	io_capa_resp(BT_IO_DISPLAY_YESNO, BT_HCI_GENERAL_BONDING);
	user_confirm_req();
	/* Just Works, which an initiator confirms without the application */
	expect_cmd(BT_HCI_OP_USER_CONFIRM_REPLY, &sent);
}
