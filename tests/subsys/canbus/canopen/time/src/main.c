/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/canbus/canopen.h>
#include <zephyr/drivers/can.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "self_reception.h"

#define TEST_NODE_ID 0x31U
#define TEST_TIME_ID 0x100U
#define TEST_TIMEOUT K_MSEC(100)

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

CAN_MSGQ_DEFINE(time_msgq, 4);
K_SEM_DEFINE(time_sem, 0, 1);

static uint32_t time_ms;
static uint16_t time_days;

static void test_time_cb(struct canopen_node *node, uint32_t ms, uint16_t days, void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(user_data);

	time_ms = ms;
	time_days = days;
	k_sem_give(&time_sem);
}

static const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.time_cb = test_time_cb,
	.node_id = TEST_NODE_ID,
};

static struct canopen_node test_node;

static uint32_t time_cob_id(void)
{
	uint32_t value = 0U;

	zassert_ok(canopen_od_read(&test_node, 0x1012, 0U, &value, sizeof(value)));

	return value;
}

static int time_cob_id_set(uint32_t value)
{
	return canopen_od_write(&test_node, 0x1012, 0U, &value, sizeof(value));
}

static void send_time(uint32_t raw_ms, uint16_t days, uint8_t dlc)
{
	struct can_frame frame = {
		.id = TEST_TIME_ID,
		.dlc = dlc,
	};

	sys_put_le32(raw_ms, frame.data);
	sys_put_le16(days, &frame.data[4]);
	zassert_ok(can_send(can_dev, &frame, K_FOREVER, NULL, NULL));
}

static void *time_setup(void)
{
	zassert_true(device_is_ready(can_dev));
	zassert_ok(can_set_mode(can_dev, CAN_MODE_LOOPBACK));
	if (!canopen_test_self_reception(can_dev)) {
		ztest_test_skip();
		return NULL;
	}
	zassert_ok(canopen_node_init(&test_node, &test_cfg));
	zassert_ok(canopen_node_start(&test_node));

	return NULL;
}

static void time_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_RESET_COMMUNICATION));
	k_sem_reset(&time_sem);
	k_msgq_purge(&time_msgq);
}

ZTEST(canopen_time, test_object)
{
	zassert_equal(time_cob_id(), 0x80000100U);
	zassert_equal(time_cob_id_set(0x80000101U), -EINVAL);
	zassert_equal(time_cob_id_set(0xA0000100U), -EINVAL);
	zassert_ok(time_cob_id_set(0x00000100U));
	zassert_equal(time_cob_id_set(0x00000123U), -EINVAL);
	zassert_equal(time_cob_id_set(0x00000701U), -EINVAL);
	zassert_ok(time_cob_id_set(0x000001A0U));
	zassert_ok(time_cob_id_set(0x400001A0U));
	zassert_equal(time_cob_id_set(0x40000100U), -EINVAL);
	zassert_equal(time_cob_id(), 0x400001A0U);

	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_RESET_COMMUNICATION));
	zassert_equal(time_cob_id(), 0x80000100U);
}

ZTEST(canopen_time, test_consumer)
{
	send_time(0xF0000000U | 45296789U, 15279U, 6U);
	zassert_ok(k_sem_take(&time_sem, TEST_TIMEOUT));
	zassert_equal(time_ms, 45296789U);
	zassert_equal(time_days, 15279U);

	send_time(1U, 2U, 5U);
	zassert_equal(k_sem_take(&time_sem, TEST_TIMEOUT), -EAGAIN);

	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_STOP));
	send_time(1U, 2U, 6U);
	zassert_equal(k_sem_take(&time_sem, TEST_TIMEOUT), -EAGAIN);

	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_START));
	zassert_ok(time_cob_id_set(0x00000100U));
	send_time(1U, 2U, 6U);
	zassert_equal(k_sem_take(&time_sem, TEST_TIMEOUT), -EAGAIN);
}

ZTEST(canopen_time, test_producer)
{
	const struct can_filter filter = {
		.id = TEST_TIME_ID,
		.mask = CAN_STD_ID_MASK,
	};
	struct can_frame frame;
	int filter_id;

	zassert_equal(canopen_time_send(&test_node, 1U, 2U), -EACCES);
	zassert_ok(time_cob_id_set(0x00000100U));
	zassert_ok(time_cob_id_set(0x40000100U));
	zassert_equal(canopen_time_send(&test_node, 0x10000000U, 0U), -EINVAL);

	filter_id = can_add_rx_filter_msgq(can_dev, &time_msgq, &filter);
	zassert_true(filter_id >= 0);

	zassert_ok(canopen_time_send(&test_node, 45296789U, 15279U));
	zassert_ok(k_msgq_get(&time_msgq, &frame, TEST_TIMEOUT));
	zassert_equal(frame.dlc, 6U);
	zassert_equal(sys_get_le32(frame.data), 45296789U);
	zassert_equal(sys_get_le16(&frame.data[4]), 15279U);
	zassert_equal(k_sem_take(&time_sem, K_NO_WAIT), -EBUSY);

	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_STOP));
	zassert_equal(canopen_time_send(&test_node, 1U, 2U), -EPERM);
	zassert_equal(k_msgq_get(&time_msgq, &frame, TEST_TIMEOUT), -EAGAIN);

	can_remove_rx_filter(can_dev, filter_id);
}

ZTEST_SUITE(canopen_time, NULL, time_setup, time_before, NULL, NULL);
