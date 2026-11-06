/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/canbus/canopen.h>
#include <zephyr/drivers/can.h>
#include <zephyr/ztest.h>

#define TEST_NODE_ID  0x32U
#define TEST_GUARD_ID (0x700U + TEST_NODE_ID)
#define TEST_HB_NODE  0x05U
#define TEST_TIMEOUT  K_MSEC(50)
#define ERR_REG_COMM  0x11U

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

CAN_MSGQ_DEFINE(guard_msgq, 8);

static const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.node_id = TEST_NODE_ID,
};

static struct canopen_node test_node;
static int guard_filter = -1;

static void od_write(uint16_t index, uint8_t sub, uint32_t value, size_t len)
{
	zassert_ok(canopen_od_write(&test_node, index, sub, &value, len));
}

static uint8_t error_register(void)
{
	uint8_t value = 0U;

	zassert_ok(canopen_od_read(&test_node, 0x1001, 0U, &value, sizeof(value)));

	return value;
}

static int guard_request(uint8_t *answer)
{
	const struct can_frame request = {
		.id = TEST_GUARD_ID,
		.dlc = 1U,
		.flags = CAN_FRAME_RTR,
	};
	struct can_frame frame;

	zassert_ok(can_send(can_dev, &request, K_FOREVER, NULL, NULL));

	while (k_msgq_get(&guard_msgq, &frame, TEST_TIMEOUT) == 0) {
		if ((frame.flags & CAN_FRAME_RTR) == 0U) {
			zassert_equal(frame.dlc, 1U);
			*answer = frame.data[0];
			return 0;
		}
	}

	return -EAGAIN;
}

static uint8_t guard_answer(void)
{
	uint8_t answer = 0U;

	zassert_ok(guard_request(&answer), "no answer to the guarding request");

	return answer;
}

static void *guard_setup(void)
{
	const struct can_filter filter = {
		.id = TEST_GUARD_ID,
		.mask = CAN_STD_ID_MASK,
	};

	zassert_true(device_is_ready(can_dev));
	zassert_ok(can_set_mode(can_dev, CAN_MODE_LOOPBACK));
	zassert_ok(canopen_node_init(&test_node, &test_cfg));
	zassert_ok(canopen_node_start(&test_node));
	guard_filter = can_add_rx_filter_msgq(can_dev, &guard_msgq, &filter);
	zassert_true(guard_filter >= 0);

	return NULL;
}

static void guard_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_RESET_COMMUNICATION));
	k_msleep(5);
	k_msgq_purge(&guard_msgq);
}

ZTEST(canopen_guard, test_objects)
{
	uint16_t guard_time = 1U;
	uint8_t factor = 1U;

	zassert_ok(canopen_od_read(&test_node, 0x100C, 0U, &guard_time, sizeof(guard_time)));
	zassert_ok(canopen_od_read(&test_node, 0x100D, 0U, &factor, sizeof(factor)));
	zassert_equal(guard_time, 0U);
	zassert_equal(factor, 0U);

	od_write(0x100C, 0U, 250U, 2U);
	od_write(0x100D, 0U, 3U, 1U);
	zassert_ok(canopen_od_read(&test_node, 0x100C, 0U, &guard_time, sizeof(guard_time)));
	zassert_ok(canopen_od_read(&test_node, 0x100D, 0U, &factor, sizeof(factor)));
	zassert_equal(guard_time, 250U);
	zassert_equal(factor, 3U);
}

ZTEST(canopen_guard, test_answer)
{
	zassert_equal(guard_answer(), 0x7FU);
	zassert_equal(guard_answer(), 0xFFU);

	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_START));
	zassert_equal(guard_answer(), 0x05U);
	zassert_equal(guard_answer(), 0x85U);

	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_STOP));
	zassert_equal(guard_answer(), 0x04U);

	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_ENTER_PRE_OPERATIONAL));
	zassert_equal(guard_answer(), 0xFFU);

	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_RESET_COMMUNICATION));
	k_msleep(5);
	k_msgq_purge(&guard_msgq);
	zassert_equal(guard_answer(), 0x7FU);
}

ZTEST(canopen_guard, test_no_answer_with_heartbeat)
{
	uint8_t answer;

	od_write(0x1017, 0U, 1000U, 2U);
	zassert_equal(guard_request(&answer), -EAGAIN);

	od_write(0x1017, 0U, 0U, 2U);
	zassert_equal(guard_answer(), 0x7FU);
}

ZTEST(canopen_guard, test_life_guarding)
{
	od_write(0x100C, 0U, 20U, 2U);
	od_write(0x100D, 0U, 3U, 1U);
	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_START));

	k_msleep(150);
	zassert_equal(error_register(), 0U, "life guarding before the first request");
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_OPERATIONAL);

	(void)guard_answer();
	k_msleep(40);
	zassert_equal(error_register(), 0U);
	(void)guard_answer();
	k_msleep(100);
	zassert_equal(error_register(), ERR_REG_COMM);
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_PRE_OPERATIONAL);

	(void)guard_answer();
	k_msleep(5);
	zassert_equal(error_register(), 0U);

	od_write(0x100D, 0U, 0U, 1U);
	(void)guard_answer();
	k_msleep(150);
	zassert_equal(error_register(), 0U, "life guarding with life time 0");
}

ZTEST(canopen_guard, test_shared_emcy)
{
	const struct can_frame heartbeat = {
		.id = 0x700U + TEST_HB_NODE,
		.dlc = 1U,
		.data = {0x05U},
	};

	od_write(0x100C, 0U, 20U, 2U);
	od_write(0x100D, 0U, 2U, 1U);
	od_write(0x1016, 1U, (TEST_HB_NODE << 16) | 30U, 4U);

	(void)guard_answer();
	zassert_ok(can_send(can_dev, &heartbeat, K_FOREVER, NULL, NULL));
	k_msleep(80);
	zassert_equal(error_register(), ERR_REG_COMM);

	zassert_ok(can_send(can_dev, &heartbeat, K_FOREVER, NULL, NULL));
	k_msleep(5);
	zassert_equal(error_register(), ERR_REG_COMM, "heartbeat hid the guarding timeout");

	od_write(0x100D, 0U, 0U, 1U);
	k_msleep(5);
	zassert_equal(error_register(), 0U);

	od_write(0x100D, 0U, 2U, 1U);
	(void)guard_answer();
	k_msleep(80);
	zassert_equal(error_register(), ERR_REG_COMM);
	(void)guard_answer();
	k_msleep(5);
	zassert_equal(error_register(), ERR_REG_COMM, "guarding hid the heartbeat timeout");

	od_write(0x1016, 1U, 0U, 4U);
	k_msleep(5);
	zassert_equal(error_register(), 0U);
}

ZTEST_SUITE(canopen_guard, NULL, guard_setup, guard_before, NULL, NULL);
