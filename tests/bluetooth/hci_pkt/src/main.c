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
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/ztest.h>

#define TEST_OPCODE 0xfc01U
#define TEST_OTHER_OPCODE 0xfc02U

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

ZTEST(bt_hci_pkt, test_pull_cmd_complete_no_status)
{
	/* ncmd 1, opcode HCI_Reset, no return parameters at all */
	uint8_t no_status[] = { 0x01, 0x03, 0x0c };
	struct net_buf_simple buf;
	struct bt_hci_pkt_cmd_rsp rsp;

	net_buf_simple_init_with_data(&buf, no_status, sizeof(no_status));

	zassert_equal(bt_hci_pkt_pull_cmd_complete(&buf, &rsp), -ENODATA);
	zassert_equal(rsp.opcode, BT_HCI_OP_RESET);
	zassert_equal(rsp.ncmd, 1);
	zassert_equal(rsp.status, BT_HCI_ERR_UNSPECIFIED);
	zassert_equal(rsp.rp_len, 0);
	/* Positioned as on success, at the (empty) return parameters */
	zassert_equal(buf.len, 0);
}

ZTEST(bt_hci_pkt, test_pull_cmd_complete_malformed)
{
	uint8_t too_short[] = { 0x01, 0x03 };
	struct net_buf_simple buf;
	struct bt_hci_pkt_cmd_rsp rsp;

	net_buf_simple_init_with_data(&buf, too_short, sizeof(too_short));
	zassert_equal(bt_hci_pkt_pull_cmd_complete(&buf, &rsp), -EINVAL);
	assert_unchanged(&buf, too_short, sizeof(too_short));
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

ZTEST(bt_hci_pkt, test_parse_cmd_rsp_no_status)
{
	static const uint8_t pkt[] = { BT_HCI_H4_EVT, BT_HCI_EVT_CMD_COMPLETE, 0x03,
				       0x01, 0x01, 0xfc };
	struct bt_hci_pkt_cmd_rsp rsp;

	zassert_equal(bt_hci_pkt_parse_cmd_rsp(pkt, sizeof(pkt), &rsp), -ENODATA);
	zassert_equal(rsp.opcode, TEST_OPCODE);
	zassert_equal(rsp.ncmd, 1);
	zassert_equal(rsp.status, BT_HCI_ERR_UNSPECIFIED);
	zassert_equal(rsp.rp_len, 0);
}

ZTEST_SUITE(bt_hci_pkt, NULL, NULL, NULL, NULL, NULL);
