/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/hci_lockstep.h>
#include <zephyr/bluetooth/hci_pkt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/ztest.h>

#define TEST_OPCODE 0xfc01
#define TEST_OTHER_OPCODE 0xfc02

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
	memcpy(xport->sent, pkt, len);
	xport->sent_len = len;

	if (xport->send_err != 0) {
		return xport->send_err;
	}

	if (xport->rsp != NULL && xport->rsp_in_send) {
		feed_canned_rsp(xport);
	}

	return 0;
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
	memset(xport, 0, sizeof(*xport));
	xport->dev.name = "fake_hci";
	xport->dev.data = xport;
	k_work_init_delayable(&xport->rsp_work, rsp_work_handler);
	bt_hci_lockstep_init(&xport->ls, &xport->dev, fake_send);
	/* Keep the timeout test short */
	xport->ls.timeout = K_MSEC(100);
}

static void assert_unchanged(const struct net_buf_simple *buf, const uint8_t *data, size_t len)
{
	zassert_equal(buf->len, len, "len %u != %zu", buf->len, len);
	zassert_equal(buf->data, data, "data pointer moved");
}

ZTEST(bt_hci_pkt, test_cmd_define_and_push)
{
	const uint8_t expected[] = { BT_HCI_H4_CMD, 0x01, 0xfc, 0x02, 0xaa, 0xbb };

	BT_HCI_PKT_CMD_DEFINE(cmd, 2);

	zassert_equal(cmd.len, 0);
	zassert_equal(net_buf_simple_headroom(&cmd), BT_HCI_PKT_CMD_HDR_SIZE);
	zassert_equal(net_buf_simple_tailroom(&cmd), 2);

	net_buf_simple_add_u8(&cmd, 0xaa);
	net_buf_simple_add_u8(&cmd, 0xbb);

	zassert_equal(bt_hci_pkt_push_cmd_hdr(&cmd, TEST_OPCODE), 0);
	zassert_equal(cmd.len, sizeof(expected));
	zassert_mem_equal(cmd.data, expected, sizeof(expected));
}

ZTEST(bt_hci_pkt, test_cmd_push_without_params)
{
	const uint8_t expected[] = { BT_HCI_H4_CMD, 0x03, 0x0c, 0x00 };

	BT_HCI_PKT_CMD_DEFINE(cmd, 0);

	zassert_equal(bt_hci_pkt_push_cmd_hdr(&cmd, BT_HCI_OP_RESET), 0);
	zassert_equal(cmd.len, sizeof(expected));
	zassert_mem_equal(cmd.data, expected, sizeof(expected));
}

ZTEST(bt_hci_pkt, test_cmd_define_static)
{
	BT_HCI_PKT_CMD_DEFINE_STATIC(cmd, 1);

	bt_hci_pkt_reset_cmd(&cmd);
	net_buf_simple_add_u8(&cmd, 0x11);
	zassert_equal(bt_hci_pkt_push_cmd_hdr(&cmd, TEST_OPCODE), 0);
	zassert_equal(cmd.len, BT_HCI_PKT_CMD_SIZE(1));
}

ZTEST(bt_hci_pkt, test_cmd_push_insufficient_headroom)
{
	NET_BUF_SIMPLE_DEFINE(buf, 8);

	net_buf_simple_add_u8(&buf, 0xaa);

	zassert_equal(bt_hci_pkt_push_cmd_hdr(&buf, TEST_OPCODE), -EINVAL);
	zassert_equal(buf.len, 1);

	net_buf_simple_reset(&buf);
	net_buf_simple_reserve(&buf, BT_HCI_PKT_CMD_HDR_SIZE - 1);
	zassert_equal(bt_hci_pkt_push_cmd_hdr(&buf, TEST_OPCODE), -EINVAL);
}

ZTEST(bt_hci_pkt, test_cmd_push_too_many_params)
{
	BT_HCI_PKT_CMD_DEFINE(cmd, UINT8_MAX + 1);

	(void)net_buf_simple_add(&cmd, UINT8_MAX + 1);
	zassert_equal(bt_hci_pkt_push_cmd_hdr(&cmd, TEST_OPCODE), -EMSGSIZE);
	zassert_equal(cmd.len, UINT8_MAX + 1);

	/* Exactly the maximum is fine */
	bt_hci_pkt_reset_cmd(&cmd);
	(void)net_buf_simple_add(&cmd, UINT8_MAX);
	zassert_equal(bt_hci_pkt_push_cmd_hdr(&cmd, TEST_OPCODE), 0);
	zassert_equal(cmd.len, BT_HCI_PKT_CMD_SIZE(UINT8_MAX));
	zassert_equal(cmd.data[3], UINT8_MAX);
}

ZTEST(bt_hci_pkt, test_cmd_reset_and_reuse)
{
	uint8_t storage[BT_HCI_PKT_CMD_SIZE(1)];
	struct net_buf_simple own;

	BT_HCI_PKT_CMD_DEFINE(cmd, 4);

	net_buf_simple_add_le32(&cmd, 0x12345678);
	zassert_equal(bt_hci_pkt_push_cmd_hdr(&cmd, TEST_OPCODE), 0);

	bt_hci_pkt_reset_cmd(&cmd);
	zassert_equal(cmd.len, 0);
	zassert_equal(net_buf_simple_headroom(&cmd), BT_HCI_PKT_CMD_HDR_SIZE);

	net_buf_simple_add_u8(&cmd, 0x01);
	zassert_equal(bt_hci_pkt_push_cmd_hdr(&cmd, TEST_OTHER_OPCODE), 0);
	zassert_equal(cmd.len, BT_HCI_PKT_CMD_SIZE(1));
	zassert_equal(cmd.data[1], 0x02);
	zassert_equal(cmd.data[2], 0xfc);

	/* A buffer over caller-provided storage */
	net_buf_simple_init_with_data(&own, storage, sizeof(storage));
	bt_hci_pkt_reset_cmd(&own);
	zassert_equal(own.len, 0);
	net_buf_simple_add_u8(&own, 0x55);
	zassert_equal(bt_hci_pkt_push_cmd_hdr(&own, TEST_OPCODE), 0);
	zassert_equal(own.len, sizeof(storage));
	zassert_equal(own.data, storage);
}

ZTEST(bt_hci_pkt, test_pull_cmd_complete)
{
	/* ncmd 1, opcode HCI_Reset, status 0, two more return parameter bytes */
	uint8_t data[] = { 0x01, 0x03, 0x0c, 0x00, 0x11, 0x22 };
	struct net_buf_simple buf;
	struct bt_hci_pkt_cmd_rsp rsp;

	net_buf_simple_init_with_data(&buf, data, sizeof(data));

	zassert_equal(bt_hci_pkt_pull_cmd_complete(&buf, &rsp), 0);
	zassert_equal(rsp.opcode, BT_HCI_OP_RESET);
	zassert_equal(rsp.ncmd, 1);
	zassert_equal(rsp.status, BT_HCI_ERR_SUCCESS);
	/* Positioned at the return parameters, status included */
	zassert_equal(buf.len, 3);
	zassert_equal(buf.data, &data[3]);
	zassert_equal(buf.data[0], 0x00);
	zassert_equal(rsp.rp, &data[3]);
	zassert_equal(rsp.rp_len, 3);
}

ZTEST(bt_hci_pkt, test_pull_cmd_complete_error_status)
{
	uint8_t data[] = { 0x01, 0x01, 0xfc, BT_HCI_ERR_UNKNOWN_CMD };
	struct net_buf_simple buf;
	struct bt_hci_pkt_cmd_rsp rsp;

	net_buf_simple_init_with_data(&buf, data, sizeof(data));

	zassert_equal(bt_hci_pkt_pull_cmd_complete(&buf, &rsp), 0);
	zassert_equal(rsp.opcode, TEST_OPCODE);
	zassert_equal(rsp.status, BT_HCI_ERR_UNKNOWN_CMD);
	zassert_equal(buf.len, 1);
}

ZTEST(bt_hci_pkt, test_pull_cmd_complete_nop)
{
	/* Num_HCI_Command_Packets update only: no return parameters at all */
	uint8_t data[] = { 0x01, 0x00, 0x00 };
	struct net_buf_simple buf;
	struct bt_hci_pkt_cmd_rsp rsp;

	net_buf_simple_init_with_data(&buf, data, sizeof(data));

	zassert_equal(bt_hci_pkt_pull_cmd_complete(&buf, &rsp), 0);
	zassert_equal(rsp.opcode, BT_OP_NOP);
	zassert_equal(rsp.ncmd, 1);
	zassert_equal(rsp.status, BT_HCI_ERR_SUCCESS);
	zassert_equal(buf.len, 0);
}

ZTEST(bt_hci_pkt, test_pull_cmd_complete_malformed)
{
	uint8_t too_short[] = { 0x01, 0x03 };
	uint8_t no_status[] = { 0x01, 0x03, 0x0c };
	struct net_buf_simple buf;
	struct bt_hci_pkt_cmd_rsp rsp;

	net_buf_simple_init_with_data(&buf, too_short, sizeof(too_short));
	zassert_equal(bt_hci_pkt_pull_cmd_complete(&buf, &rsp), -EINVAL);
	assert_unchanged(&buf, too_short, sizeof(too_short));

	net_buf_simple_init_with_data(&buf, no_status, sizeof(no_status));
	zassert_equal(bt_hci_pkt_pull_cmd_complete(&buf, &rsp), -EINVAL);
	assert_unchanged(&buf, no_status, sizeof(no_status));
}

ZTEST(bt_hci_pkt, test_pull_cmd_status)
{
	/* status 0, ncmd 1, opcode LE_Create_Connection */
	uint8_t data[] = { 0x00, 0x01, 0x0d, 0x20 };
	struct net_buf_simple buf;
	struct bt_hci_pkt_cmd_rsp rsp;

	net_buf_simple_init_with_data(&buf, data, sizeof(data));

	zassert_equal(bt_hci_pkt_pull_cmd_status(&buf, &rsp), 0);
	zassert_equal(rsp.opcode, BT_HCI_OP_LE_CREATE_CONN);
	zassert_equal(rsp.ncmd, 1);
	zassert_equal(rsp.status, BT_HCI_ERR_SUCCESS);
	zassert_equal(buf.len, 0);
	zassert_equal(rsp.rp_len, 0);

	data[0] = BT_HCI_ERR_CMD_DISALLOWED;
	net_buf_simple_init_with_data(&buf, data, sizeof(data));
	zassert_equal(bt_hci_pkt_pull_cmd_status(&buf, &rsp), 0);
	zassert_equal(rsp.status, BT_HCI_ERR_CMD_DISALLOWED);
}

ZTEST(bt_hci_pkt, test_pull_cmd_status_malformed)
{
	uint8_t too_short[] = { 0x00, 0x01, 0x0d };
	struct net_buf_simple buf;
	struct bt_hci_pkt_cmd_rsp rsp;

	net_buf_simple_init_with_data(&buf, too_short, sizeof(too_short));
	zassert_equal(bt_hci_pkt_pull_cmd_status(&buf, &rsp), -EINVAL);
	assert_unchanged(&buf, too_short, sizeof(too_short));
}

ZTEST(bt_hci_pkt, test_parse_cmd_rsp_cmd_complete)
{
	static const uint8_t pkt[] = { BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x05,
				       0x01, 0x01, 0xfc, 0x00, 0x42 };
	struct bt_hci_pkt_cmd_rsp rsp;

	zassert_equal(bt_hci_pkt_parse_cmd_rsp(pkt, sizeof(pkt), &rsp), 0);
	zassert_equal(rsp.opcode, TEST_OPCODE);
	zassert_equal(rsp.ncmd, 1);
	zassert_equal(rsp.status, BT_HCI_ERR_SUCCESS);
	/* The return parameters, status included, within the packet */
	zassert_equal(rsp.rp, &pkt[6]);
	zassert_equal(rsp.rp_len, 2);
}

ZTEST(bt_hci_pkt, test_parse_cmd_rsp_cmd_status)
{
	static const uint8_t pkt[] = { BT_HCI_H4_EVT, BT_HCI_EVT_CMD_STATUS, 0x04,
				       0x00, 0x01, 0x01, 0xfc };
	struct bt_hci_pkt_cmd_rsp rsp;

	zassert_equal(bt_hci_pkt_parse_cmd_rsp(pkt, sizeof(pkt), &rsp), 0);
	zassert_equal(rsp.opcode, TEST_OPCODE);
	zassert_equal(rsp.ncmd, 1);
	zassert_equal(rsp.status, BT_HCI_ERR_SUCCESS);
	zassert_equal(rsp.rp_len, 0);
}

ZTEST(bt_hci_pkt, test_parse_cmd_rsp_trailing_bytes_ignored)
{
	static const uint8_t pkt[] = { BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x04,
				       0x01, 0x01, 0xfc, 0x00, 0xff };
	struct bt_hci_pkt_cmd_rsp rsp;

	zassert_equal(bt_hci_pkt_parse_cmd_rsp(pkt, sizeof(pkt), &rsp), 0);
	zassert_equal(rsp.opcode, TEST_OPCODE);
	/* Only the status: the byte beyond the event length is not a return parameter */
	zassert_equal(rsp.rp, &pkt[6]);
	zassert_equal(rsp.rp_len, 1);
}

ZTEST(bt_hci_pkt, test_parse_cmd_rsp_other_packets)
{
	static const uint8_t le_meta[] = { BT_HCI_H4_EVT, BT_HCI_EVT_LE_META_EVENT, 0x01, 0x00 };
	static const uint8_t acl[] = { BT_HCI_H4_ACL, 0x00, 0x00, 0x01, 0x00, 0xaa };
	static const uint8_t empty[1];
	struct bt_hci_pkt_cmd_rsp rsp;

	zassert_equal(bt_hci_pkt_parse_cmd_rsp(le_meta, sizeof(le_meta), &rsp), -ENOMSG);
	zassert_equal(bt_hci_pkt_parse_cmd_rsp(acl, sizeof(acl), &rsp), -ENOMSG);
	zassert_equal(bt_hci_pkt_parse_cmd_rsp(empty, 0, &rsp), -ENOMSG);
}

ZTEST(bt_hci_pkt, test_parse_cmd_rsp_malformed)
{
	static const uint8_t short_hdr[] = { BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE };
	static const uint8_t truncated[] = { BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x10, 0x01 };
	static const uint8_t short_cc[] = { BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x02,
					    0x01, 0x03 };
	struct bt_hci_pkt_cmd_rsp rsp;

	zassert_equal(bt_hci_pkt_parse_cmd_rsp(short_hdr, sizeof(short_hdr), &rsp), -EINVAL);
	zassert_equal(bt_hci_pkt_parse_cmd_rsp(truncated, sizeof(truncated), &rsp), -EINVAL);
	zassert_equal(bt_hci_pkt_parse_cmd_rsp(short_cc, sizeof(short_cc), &rsp), -EINVAL);
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

ZTEST(bt_hci_lockstep, test_cmd_complete_from_send)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cc_ok_rsp;
	xport.rsp_len = sizeof(cc_ok_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, 0);

	zassert_equal(xport.ls.status, BT_HCI_ERR_SUCCESS);
	zassert_equal(rsp.len, 3);
	zassert_equal(rsp.data[0], 0x00);
	zassert_equal(rsp.data[1], 0xaa);
	zassert_equal(rsp.data[2], 0xbb);
}

ZTEST(bt_hci_lockstep, test_cmd_complete_async)
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

ZTEST(bt_hci_lockstep, test_cmd_complete_error_status)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cc_err_rsp;
	xport.rsp_len = sizeof(cc_err_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, -EIO);

	zassert_equal(xport.ls.status, BT_HCI_ERR_UNSPECIFIED);
	zassert_equal(rsp.len, 1);
	zassert_equal(rsp.data[0], BT_HCI_ERR_UNSPECIFIED);
}

ZTEST(bt_hci_lockstep, test_cmd_status)
{
	struct fake_transport xport;

	BT_HCI_PKT_CMD_DEFINE(cmd, 1);
	NET_BUF_SIMPLE_DEFINE(rsp, 8);

	fake_transport_init(&xport);
	xport.rsp = cs_ok_rsp;
	xport.rsp_len = sizeof(cs_ok_rsp);
	xport.rsp_in_send = true;

	send_test_cmd(&xport, &cmd, &rsp, 0);

	zassert_equal(xport.ls.status, BT_HCI_ERR_SUCCESS);
	zassert_equal(rsp.len, 0);
}

ZTEST(bt_hci_lockstep, test_no_rsp_buffer)
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

ZTEST(bt_hci_lockstep, test_rsp_truncated_to_buffer)
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

ZTEST(bt_hci_lockstep, test_timeout)
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

ZTEST(bt_hci_lockstep, test_send_error)
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

ZTEST(bt_hci_lockstep, test_feed_idle)
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

ZTEST(bt_hci_lockstep, test_other_opcode_not_consumed)
{
	check_not_consumed_while_waiting(cc_other_rsp, sizeof(cc_other_rsp));
}

ZTEST(bt_hci_lockstep, test_unrelated_event_not_consumed)
{
	static const uint8_t le_meta[] = { BT_HCI_H4_EVT, BT_HCI_EVT_LE_META_EVENT, 0x01, 0x00 };

	check_not_consumed_while_waiting(le_meta, sizeof(le_meta));
}

ZTEST(bt_hci_lockstep, test_nop_cmd_complete_not_consumed)
{
	static const uint8_t nop_cc[] = { BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x03,
					  0x01, 0x00, 0x00 };

	check_not_consumed_while_waiting(nop_cc, sizeof(nop_cc));
}

ZTEST(bt_hci_lockstep, test_malformed_event_not_consumed)
{
	static const uint8_t short_cc[] = { BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x02,
					    0x01, 0x01 };

	check_not_consumed_while_waiting(short_cc, sizeof(short_cc));
}

ZTEST(bt_hci_lockstep, test_cmd_complete_from_isr)
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

ZTEST(bt_hci_lockstep, test_cmd_status_error)
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

	zassert_equal(xport.ls.status, BT_HCI_ERR_CMD_DISALLOWED);
	zassert_equal(rsp.len, 0);
}

ZTEST(bt_hci_lockstep, test_too_many_params)
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

ZTEST(bt_hci_lockstep, test_cmd_without_params)
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

ZTEST(bt_hci_lockstep, test_reuse)
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

	/* The response buffer is emptied for the new command */
	send_test_cmd(&xport, &cmd, &rsp, 0);
	zassert_equal(rsp.len, 0);
}

ZTEST_SUITE(bt_hci_pkt, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(bt_hci_lockstep, NULL, NULL, NULL, NULL, NULL);
