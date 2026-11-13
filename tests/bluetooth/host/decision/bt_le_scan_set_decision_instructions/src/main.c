/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/decision.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>

#include "mocks/crypto.h"
#include "mocks/hci_core.h"
#include "mocks/kernel.h"
#include "mocks/net_buf.h"
#include "mocks/rpa.h"

DEFINE_FFF_GLOBALS;

#define TEST_LEN sizeof(struct bt_hci_le_decision_test)

static const uint8_t key[BT_LE_DECISION_KEY_SIZE] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
};

static uint8_t cmd_data[UINT8_MAX];
static struct net_buf cmd_buf;
static uint8_t sent[UINT8_MAX];
static size_t sent_len;
static int send_err;

static struct net_buf *cmd_alloc_fake(k_timeout_t timeout)
{
	ARG_UNUSED(timeout);

	net_buf_simple_init_with_data(&cmd_buf.b, cmd_data, sizeof(cmd_data));
	cmd_buf.len = 0U;

	return &cmd_buf;
}

static int cmd_send_sync_fake(uint16_t opcode, struct net_buf *buf, struct net_buf **rsp)
{
	ARG_UNUSED(rsp);

	zassert_equal(opcode, BT_HCI_OP_LE_SET_DECISION_INSTRUCTIONS);
	(void)memcpy(sent, buf->data, buf->len);
	sent_len = buf->len;

	return send_err;
}

static void set_decision_instructions_before(void *fixture)
{
	ARG_UNUSED(fixture);

	CRYPTO_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
	KERNEL_FFF_FAKES_LIST(RESET_FAKE);
	NET_BUF_FFF_FAKES_LIST(RESET_FAKE);
	RPA_FFF_FAKES_LIST(RESET_FAKE);

	bt_hci_cmd_alloc_fake.custom_fake = cmd_alloc_fake;
	bt_hci_cmd_send_sync_fake.custom_fake = cmd_send_sync_fake;

	(void)memset(&bt_dev, 0, sizeof(bt_dev));
	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	bt_dev.le.features[BT_LE_FEAT_BIT_DECISION_ADV_FILTER / 8U] |=
		BIT(BT_LE_FEAT_BIT_DECISION_ADV_FILTER % 8U);
	atomic_set_bit(bt_dev.supported_commands, BT_DEV_CMD_LE_SET_DECISION_INSTRUCTIONS);

	(void)memset(sent, 0, sizeof(sent));
	sent_len = 0U;
	send_err = 0;
}

ZTEST_SUITE(bt_le_scan_set_decision_instructions, NULL, NULL, set_decision_instructions_before,
	    NULL, NULL);

/*
 *  Test the command for Example 1 of the LE Set Decision Instructions command
 *  (Core Vol 4, Part E, 7.8.145)
 *
 *  Expected behaviour:
 *   - Num_Tests is followed by Test_Flags, Test_Field and Test_Parameters of each test
 */
ZTEST(bt_le_scan_set_decision_instructions, test_command)
{
	const struct bt_le_decision_test tests[] = {
		BT_LE_DECISION_TEST_TAG(key),
		BT_LE_DECISION_TEST_ADV_MODE(BT_LE_DECISION_ADV_MODE_NON_CONN_NON_SCAN |
					     BT_LE_DECISION_ADV_MODE_SCAN),
	};
	const struct bt_le_decision_group groups[] = {
		BT_LE_DECISION_GROUP(tests),
	};
	uint8_t expected[1U + 2U * TEST_LEN] = {0};

	expected[0] = 2U;
	expected[1] = 0b0011;
	expected[2] = 0U;
	(void)memcpy(&expected[3], key, sizeof(key));
	expected[1U + TEST_LEN] = 0b0010;
	expected[2U + TEST_LEN] = 6U;
	expected[3U + TEST_LEN] = 0x05;

	zassert_ok(bt_le_scan_set_decision_instructions(groups, ARRAY_SIZE(groups)));
	zassert_equal(sent_len, sizeof(expected), "Command length %zu", sent_len);
	zassert_mem_equal(sent, expected, sizeof(expected));
}

/*
 *  Test the largest number of tests
 *
 *  Expected behaviour:
 *   - BT_LE_DECISION_TESTS_MAX tests fit in the command
 */
ZTEST(bt_le_scan_set_decision_instructions, test_max_tests)
{
	struct bt_le_decision_test tests[BT_LE_DECISION_TESTS_MAX];
	const struct bt_le_decision_group groups[] = {
		BT_LE_DECISION_GROUP(tests),
	};

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		tests[i] = (struct bt_le_decision_test)BT_LE_DECISION_TEST_RSSI(-10, 10);
	}

	zassert_ok(bt_le_scan_set_decision_instructions(groups, ARRAY_SIZE(groups)));
	zassert_equal(sent_len, 1U + BT_LE_DECISION_TESTS_MAX * TEST_LEN);
	zassert_equal(sent[0], BT_LE_DECISION_TESTS_MAX);
}

/*
 *  Test instructions without groups
 *
 *  Expected behaviour:
 *   - One test that never passes is sent, so every decision PDU fails
 */
ZTEST(bt_le_scan_set_decision_instructions, test_no_groups)
{
	uint8_t expected[1U + TEST_LEN] = {1U, 0b0001, 6U};

	zassert_ok(bt_le_scan_set_decision_instructions(NULL, 0));
	zassert_equal(sent_len, sizeof(expected), "Command length %zu", sent_len);
	zassert_mem_equal(sent, expected, sizeof(expected));
}

/*
 *  Test invalid instructions
 *
 *  Expected behaviour:
 *   - -EINVAL is returned, the command buffer is released and no command is sent
 */
ZTEST(bt_le_scan_set_decision_instructions, test_invalid_instructions)
{
	const struct bt_le_decision_group empty = {.tests = NULL, .num_tests = 0};

	zassert_equal(bt_le_scan_set_decision_instructions(&empty, 1), -EINVAL);
	zassert_equal(net_buf_unref_fake.call_count, 1);
	zassert_equal(net_buf_unref_fake.arg0_val, &cmd_buf);
	zassert_equal(bt_hci_cmd_send_sync_fake.call_count, 0);
}

/*
 *  Test instructions that the Controller rejects
 *
 *  Expected behaviour:
 *   - The error from the command is returned
 */
ZTEST(bt_le_scan_set_decision_instructions, test_send_error)
{
	send_err = -ENOSPC;
	zassert_equal(bt_le_scan_set_decision_instructions(NULL, 0), -ENOSPC);
}

/*
 *  Test the state of Bluetooth and the Controller
 *
 *  Expected behaviour:
 *   - Before Bluetooth is enabled, -EAGAIN is returned
 *   - Without the feature or without the command, -ENOTSUP is returned
 */
ZTEST(bt_le_scan_set_decision_instructions, test_not_available)
{
	atomic_clear_bit(bt_dev.flags, BT_DEV_READY);
	zassert_equal(bt_le_scan_set_decision_instructions(NULL, 0), -EAGAIN);
	atomic_set_bit(bt_dev.flags, BT_DEV_READY);

	atomic_clear_bit(bt_dev.supported_commands, BT_DEV_CMD_LE_SET_DECISION_INSTRUCTIONS);
	zassert_equal(bt_le_scan_set_decision_instructions(NULL, 0), -ENOTSUP);
	atomic_set_bit(bt_dev.supported_commands, BT_DEV_CMD_LE_SET_DECISION_INSTRUCTIONS);

	(void)memset(bt_dev.le.features, 0, sizeof(bt_dev.le.features));
	zassert_equal(bt_le_scan_set_decision_instructions(NULL, 0), -ENOTSUP);

	zassert_equal(bt_hci_cmd_alloc_fake.call_count, 0);
}
