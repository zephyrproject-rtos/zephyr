/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/hci_pkt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/drivers/bluetooth/hci_lockstep.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/ztest.h>

#define TEST_OPCODE 0xfc01U

/* Fake transport for the lockstep tests: records the sent packet and can
 * deliver a canned response, either synchronously from within send (as a
 * request/response transport does) or from a delayed work item.
 */
struct fake_transport {
	struct device dev;
	struct bt_hci_lockstep ls;
	uint8_t sent[BT_HCI_PKT_CMD_SIZE(UINT8_MAX)];
	size_t sent_len;
	int send_err;
	/* Optional packet delivered before the response, e.g. an unrelated
	 * or malformed event; its fate is recorded for the test to check.
	 */
	const uint8_t *pre;
	size_t pre_len;
	bool pre_consumed;
	const uint8_t *rsp;
	size_t rsp_len;
	bool rsp_in_send;
	bool rsp_from_isr;
	bool rsp_fed_in_isr;
	unsigned int sends;
	struct k_work_delayable rsp_work;
};

static void feed_canned_rsp(struct fake_transport *xport)
{
	if (xport->pre != NULL) {
		xport->pre_consumed = bt_hci_lockstep_feed(&xport->ls, xport->pre, xport->pre_len);
	}

	/* A driver feeds the packet it received, from its own buffer */
	(void)bt_hci_lockstep_feed(&xport->ls, xport->rsp, xport->rsp_len);
}

static void feed_canned_rsp_isr(const void *arg)
{
	struct fake_transport *xport = (struct fake_transport *)arg;

	xport->rsp_fed_in_isr = k_is_in_isr();
	feed_canned_rsp(xport);
}

static int fake_send(const struct device *dev, const uint8_t *pkt, size_t len)
{
	struct fake_transport *xport = dev->data;

	zassert_true(len <= sizeof(xport->sent));
	(void)memcpy(xport->sent, pkt, len);
	xport->sent_len = len;
	xport->sends++;

	/* A transport may deliver packets before it reports a failure */
	if (xport->rsp != NULL && xport->rsp_in_send) {
		feed_canned_rsp(xport);
	}

	return xport->send_err;
}

static void rsp_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct fake_transport *xport = CONTAINER_OF(dwork, struct fake_transport, rsp_work);

	if (xport->rsp_from_isr) {
		irq_offload(feed_canned_rsp_isr, xport);
	} else {
		feed_canned_rsp(xport);
	}
}

static void fake_transport_init(struct fake_transport *xport)
{
	(void)memset(xport, 0, sizeof(*xport));
	xport->dev.name = "fake_hci";
	xport->dev.data = xport;
	k_work_init_delayable(&xport->rsp_work, rsp_work_handler);
	bt_hci_lockstep_init(&xport->ls, &xport->dev, fake_send);
	/* Keep the timeout test short */
	xport->ls.timeout = K_MSEC(100);
}

static const uint8_t cc_ok_rsp[] = {
	BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x06, 0x01, 0x01, 0xfc, 0x00, 0xaa, 0xbb
};
static const uint8_t cc_err_rsp[] = {
	BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x04, 0x01, 0x01, 0xfc, BT_HCI_ERR_UNSPECIFIED
};
static const uint8_t cc_other_rsp[] = {
	BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x04, 0x01, 0x02, 0xfc, 0x00
};
static const uint8_t cs_ok_rsp[] = {
	BT_HCI_H4_EVT, BT_HCI_EVT_CMD_STATUS, 0x04, 0x00, 0x01, 0x01, 0xfc
};
/* Responses whose Num_HCI_Command_Packets allows no further command */
static const uint8_t cc_ok_no_credit_rsp[] = {
	BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x06, 0x00, 0x01, 0xfc, 0x00, 0xaa, 0xbb
};
static const uint8_t cs_ok_no_credit_rsp[] = {
	BT_HCI_H4_EVT, BT_HCI_EVT_CMD_STATUS, 0x04, 0x00, 0x00, 0x01, 0xfc
};
/* The NOP Command Complete a controller sends to allow one command, and
 * one allowing none.
 */
static const uint8_t nop_cc_grant[] = {
	BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x03, 0x01, 0x00, 0x00
};
static const uint8_t nop_cc_revoke[] = {
	BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x03, 0x00, 0x00, 0x00
};

static void send_test_cmd(struct fake_transport *xport, struct net_buf_simple *cmd,
			  struct net_buf_simple *rsp, int expected_err)
{
	const uint8_t expected_pkt[] = { BT_HCI_H4_CMD, 0x01, 0xfc, 0x01, 0x5a };
	int err;

	bt_hci_pkt_reset_cmd(cmd);
	net_buf_simple_add_u8(cmd, 0x5a);

	err = bt_hci_lockstep_cmd_send_sync(&xport->ls, TEST_OPCODE, cmd, rsp);
	zassert_equal(err, expected_err, "err %d != %d", err, expected_err);

	zassert_equal(xport->sent_len, sizeof(expected_pkt));
	zassert_mem_equal(xport->sent, expected_pkt, sizeof(expected_pkt));
}

static ZTEST(bt_hci_lockstep, test_cmd_complete_from_send)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, 0);

	zassert_equal(rsp.len, 3);
	zassert_equal(rsp.data[0], BT_HCI_ERR_SUCCESS);
	zassert_equal(rsp.data[1], 0xaa);
	zassert_equal(rsp.data[2], 0xbb);
}

static ZTEST(bt_hci_lockstep, test_cmd_complete_async)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	k_work_schedule(&xport.rsp_work, K_MSEC(20));

	send_test_cmd(&xport, &cmd, &rsp, 0);

	zassert_equal(rsp.len, 3);
	zassert_equal(rsp.data[1], 0xaa);
}

static ZTEST(bt_hci_lockstep, test_cmd_complete_error_status)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cc_err_rsp;
	xport.rsp_len = sizeof(cc_err_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, -EIO);

	zassert_equal(rsp.len, 1);
	zassert_equal(rsp.data[0], BT_HCI_ERR_UNSPECIFIED);
}

static ZTEST(bt_hci_lockstep, test_cmd_status)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cs_ok_rsp;
	xport.rsp_len = sizeof(cs_ok_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, 0);

	/* A Command Status has nothing but its status */
	zassert_equal(rsp.len, 1);
	zassert_equal(rsp.data[0], BT_HCI_ERR_SUCCESS);
}

static ZTEST(bt_hci_lockstep, test_no_rsp_buffer)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);

	fake_transport_init(&xport);
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, NULL, 0);

	/* A following command with a response buffer is unaffected */
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	send_test_cmd(&xport, &cmd, &rsp, 0);
	zassert_equal(rsp.len, 3);
}

static ZTEST(bt_hci_lockstep, test_rsp_truncated_to_buffer)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 2);

	fake_transport_init(&xport);
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, 0);

	zassert_equal(rsp.len, 2);
	zassert_equal(rsp.data[0], 0x00);
	zassert_equal(rsp.data[1], 0xaa);
}

static ZTEST(bt_hci_lockstep, test_timeout)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);

	send_test_cmd(&xport, &cmd, &rsp, -EAGAIN);

	/* A late response is not consumed */
	zassert_false(bt_hci_lockstep_feed(&xport.ls, cc_ok_rsp, sizeof(cc_ok_rsp)));
	zassert_equal(rsp.len, 0);
}

static ZTEST(bt_hci_lockstep, test_send_error)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.send_err = -EBUSY;

	send_test_cmd(&xport, &cmd, &rsp, -EBUSY);

	/* The helper is not armed */
	zassert_false(bt_hci_lockstep_feed(&xport.ls, cc_ok_rsp, sizeof(cc_ok_rsp)));
}

static ZTEST(bt_hci_lockstep, test_feed_idle)
{
	struct fake_transport xport;

	fake_transport_init(&xport);

	/* Nothing outstanding: nothing is consumed */
	zassert_false(bt_hci_lockstep_feed(&xport.ls, cc_ok_rsp, sizeof(cc_ok_rsp)));
}

/* While a command is outstanding, packets that are not its response are
 * left untouched for the driver: a response to another opcode, an unrelated
 * event, the NOP Command Complete and a malformed event.
 */
static void check_not_consumed_while_waiting(const uint8_t *pre, size_t pre_len)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.pre = pre;
	xport.pre_len = pre_len;
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, 0);

	zassert_false(xport.pre_consumed);
	zassert_equal(rsp.len, 3);
}

static ZTEST(bt_hci_lockstep, test_other_opcode_not_consumed)
{
	check_not_consumed_while_waiting(cc_other_rsp, sizeof(cc_other_rsp));
}

static ZTEST(bt_hci_lockstep, test_unrelated_event_not_consumed)
{
	static const uint8_t le_meta[] = { BT_HCI_H4_EVT, BT_HCI_EVT_LE_META_EVENT, 0x01, 0x00 };

	check_not_consumed_while_waiting(le_meta, sizeof(le_meta));
}

static ZTEST(bt_hci_lockstep, test_nop_cmd_complete_not_consumed)
{
	check_not_consumed_while_waiting(nop_cc_grant, sizeof(nop_cc_grant));
}

static ZTEST(bt_hci_lockstep, test_malformed_event_not_consumed)
{
	static const uint8_t short_cc[] = { BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x02,
					    0x01, 0x01 };

	check_not_consumed_while_waiting(short_cc, sizeof(short_cc));
}

static ZTEST(bt_hci_lockstep, test_cmd_complete_from_isr)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	xport.rsp_from_isr = true;
	k_work_schedule(&xport.rsp_work, K_MSEC(20));

	send_test_cmd(&xport, &cmd, &rsp, 0);

	zassert_true(xport.rsp_fed_in_isr);
	zassert_equal(rsp.len, 3);
	zassert_equal(rsp.data[1], 0xaa);
}

static ZTEST(bt_hci_lockstep, test_cmd_status_error)
{
	static const uint8_t cs_err_rsp[] = { BT_HCI_H4_EVT, BT_HCI_EVT_CMD_STATUS, 0x04,
					      BT_HCI_ERR_CMD_DISALLOWED, 0x01, 0x01, 0xfc };
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cs_err_rsp;
	xport.rsp_len = sizeof(cs_err_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, -EIO);

	zassert_equal(rsp.len, 1);
	zassert_equal(rsp.data[0], BT_HCI_ERR_CMD_DISALLOWED);
}

static ZTEST(bt_hci_lockstep, test_cmd_complete_without_status)
{
	/* Command Complete for the awaited opcode with no return parameters */
	static const uint8_t cc_no_status[] = { BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x03,
						0x01, 0x01, 0xfc };
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cc_no_status;
	xport.rsp_len = sizeof(cc_no_status);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, -EIO);

	/* No status byte arrived, so none is delivered */
	zassert_equal(rsp.len, 0);
}

static ZTEST(bt_hci_lockstep, test_too_many_params)
{
	struct fake_transport xport;
	int err;

	BT_HCI_PKT_CMD_DEFINE(cmd, UINT8_MAX + 1);

	fake_transport_init(&xport);
	(void)net_buf_simple_add(&cmd, UINT8_MAX + 1);

	err = bt_hci_lockstep_cmd_send_sync(&xport.ls, TEST_OPCODE, &cmd, NULL);
	zassert_equal(err, -EMSGSIZE);
	/* Nothing was sent and the helper is not armed */
	zassert_equal(xport.sent_len, 0);
	zassert_equal(cmd.len, UINT8_MAX + 1);
	zassert_false(bt_hci_lockstep_feed(&xport.ls, cc_ok_rsp, sizeof(cc_ok_rsp)));
}

static ZTEST(bt_hci_lockstep, test_cmd_without_params)
{
	const uint8_t expected_pkt[] = { BT_HCI_H4_CMD, 0x01, 0xfc, 0x00 };
	struct fake_transport xport;
	int err;

	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	xport.rsp_in_send = true;

	/* NULL stands for "no parameters": the helper frames the packet itself */
	err = bt_hci_lockstep_cmd_send_sync(&xport.ls, TEST_OPCODE, NULL, &rsp);
	zassert_equal(err, 0);
	zassert_equal(xport.sent_len, sizeof(expected_pkt));
	zassert_mem_equal(xport.sent, expected_pkt, sizeof(expected_pkt));
	zassert_equal(rsp.len, 3);
}

static ZTEST(bt_hci_lockstep, test_reuse)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, 0);
	zassert_equal(rsp.len, 3);

	xport.rsp = cs_ok_rsp;
	xport.rsp_len = sizeof(cs_ok_rsp);

	/* The response buffer is emptied for the new command, which gets a
	 * Command Status: its status alone.
	 */
	send_test_cmd(&xport, &cmd, &rsp, 0);
	zassert_equal(rsp.len, 1);
	zassert_equal(rsp.data[0], BT_HCI_ERR_SUCCESS);
}

/* Command flow control: a response that allows no further command holds the
 * next one until the controller allows it, here with the NOP Command Complete
 * it sends for that purpose, which is not consumed.
 */
static void check_no_credit_holds_next_send(const uint8_t *rsp_pkt, size_t rsp_len)
{
	struct fake_transport xport;
	int err;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = rsp_pkt;
	xport.rsp_len = rsp_len;
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, 0);
	zassert_equal(xport.sends, 1);

	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);

	bt_hci_pkt_reset_cmd(&cmd);
	net_buf_simple_add_u8(&cmd, 0x5a);
	err = bt_hci_lockstep_cmd_send_sync(&xport.ls, TEST_OPCODE, &cmd, &rsp);
	zassert_equal(err, -EAGAIN, "err %d", err);
	zassert_equal(xport.sends, 1);

	zassert_false(bt_hci_lockstep_feed(&xport.ls, nop_cc_grant, sizeof(nop_cc_grant)));

	send_test_cmd(&xport, &cmd, &rsp, 0);
	zassert_equal(xport.sends, 2);
	zassert_equal(rsp.len, 3);
}

static ZTEST(bt_hci_lockstep, test_cmd_complete_no_credit)
{
	check_no_credit_holds_next_send(cc_ok_no_credit_rsp, sizeof(cc_ok_no_credit_rsp));
}

static ZTEST(bt_hci_lockstep, test_cmd_status_no_credit)
{
	check_no_credit_holds_next_send(cs_ok_no_credit_rsp, sizeof(cs_ok_no_credit_rsp));
}

static ZTEST(bt_hci_lockstep, test_send_error_keeps_credit)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.send_err = -EBUSY;

	send_test_cmd(&xport, &cmd, &rsp, -EBUSY);

	/* Nothing reached the controller, so the next command is allowed */
	xport.send_err = 0;
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, 0);
	zassert_equal(rsp.len, 3);
}

static ZTEST(bt_hci_lockstep, test_timeout_uses_credit)
{
	struct fake_transport xport;
	int err;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);

	send_test_cmd(&xport, &cmd, &rsp, -EAGAIN);
	zassert_equal(xport.sends, 1);

	/* The unanswered command used up the allowance */
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	xport.rsp_in_send = true;

	bt_hci_pkt_reset_cmd(&cmd);
	net_buf_simple_add_u8(&cmd, 0x5a);
	err = bt_hci_lockstep_cmd_send_sync(&xport.ls, TEST_OPCODE, &cmd, &rsp);
	zassert_equal(err, -EAGAIN, "err %d", err);
	zassert_equal(xport.sends, 1);

	/* Until the controller allows a command again */
	zassert_false(bt_hci_lockstep_feed(&xport.ls, nop_cc_grant, sizeof(nop_cc_grant)));

	send_test_cmd(&xport, &cmd, &rsp, 0);
	zassert_equal(xport.sends, 2);
}

static ZTEST(bt_hci_lockstep, test_allowance_revoked_before_send)
{
	struct fake_transport xport;
	int err;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cc_ok_no_credit_rsp;
	xport.rsp_len = sizeof(cc_ok_no_credit_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, 0);
	zassert_equal(xport.sends, 1);

	/* While the next command waits for an allowance, the controller
	 * grants one and revokes it again before the sender gets to run (both
	 * fed from an ISR): the latest word counts, nothing is sent.
	 */
	xport.rsp_in_send = false;
	xport.rsp_from_isr = true;
	xport.pre = nop_cc_grant;
	xport.pre_len = sizeof(nop_cc_grant);
	xport.rsp = nop_cc_revoke;
	xport.rsp_len = sizeof(nop_cc_revoke);
	k_work_schedule(&xport.rsp_work, K_MSEC(20));

	bt_hci_pkt_reset_cmd(&cmd);
	net_buf_simple_add_u8(&cmd, 0x5a);
	err = bt_hci_lockstep_cmd_send_sync(&xport.ls, TEST_OPCODE, &cmd, &rsp);
	zassert_equal(err, -EAGAIN, "err %d", err);
	zassert_equal(xport.sends, 1);
	zassert_true(xport.rsp_fed_in_isr);
	zassert_false(xport.pre_consumed);

	/* A grant on its own lets the command through */
	xport.pre = NULL;
	xport.rsp_from_isr = false;
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	xport.rsp_in_send = true;
	zassert_false(bt_hci_lockstep_feed(&xport.ls, nop_cc_grant, sizeof(nop_cc_grant)));

	send_test_cmd(&xport, &cmd, &rsp, 0);
	zassert_equal(xport.sends, 2);
}

static ZTEST(bt_hci_lockstep, test_nop_while_waiting_is_no_response)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = nop_cc_grant;
	xport.rsp_len = sizeof(nop_cc_grant);
	xport.rsp_in_send = true;

	/* The NOP wakes the sender, which keeps waiting for its response */
	send_test_cmd(&xport, &cmd, &rsp, -EAGAIN);
	zassert_equal(rsp.len, 0);
}

/* A transport failure reported after the controller's allowance changed
 * leaves that allowance as the controller set it.
 */
static void check_send_error_keeps_allowance(const uint8_t *nop, size_t nop_len,
					     int second_err, unsigned int sends)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = nop;
	xport.rsp_len = nop_len;
	xport.rsp_in_send = true;
	xport.send_err = -EIO;

	send_test_cmd(&xport, &cmd, &rsp, -EIO);
	send_test_cmd(&xport, &cmd, &rsp, second_err);
	zassert_equal(xport.sends, sends);
}

static ZTEST(bt_hci_lockstep, test_send_error_after_revoke)
{
	check_send_error_keeps_allowance(nop_cc_revoke, sizeof(nop_cc_revoke), -EAGAIN, 1);
}

static ZTEST(bt_hci_lockstep, test_send_error_after_grant)
{
	check_send_error_keeps_allowance(nop_cc_grant, sizeof(nop_cc_grant), -EIO, 2);
}

static ZTEST(bt_hci_lockstep, test_response_before_send_error)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	xport.rsp_in_send = true;
	xport.send_err = -EIO;

	/* The response arrived, so the command reached the controller and
	 * the transport's failure does not count.
	 */
	send_test_cmd(&xport, &cmd, &rsp, 0);
	zassert_equal(rsp.len, 3);
	zassert_equal(rsp.data[0], BT_HCI_ERR_SUCCESS);

	/* And its allowance was replaced by the response's */
	xport.send_err = 0;
	send_test_cmd(&xport, &cmd, &rsp, 0);
	zassert_equal(xport.sends, 2);
}

ZTEST_SUITE(bt_hci_lockstep, NULL, NULL, NULL, NULL, NULL);
