/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>
#include <ztest.h>

LOG_MODULE_REGISTER(can_utilities, LOG_LEVEL_ERR);

/**
 * @addtogroup t_driver_can
 * @{
 * @defgroup t_can_utilities test_can_utilities
 * @}
 */

/**
 * @brief Test of @a can_copy_frame_to_zframe()
 */
static void test_can_frame_to_zcan_frame(void)
{
	struct can_frame frame = { 0 };
	struct zcan_frame expected = { 0 };
	struct zcan_frame msg;
	const uint8_t data[CAN_MAX_DLEN] = { 0x01, 0x02, 0x03, 0x04,
					  0x05, 0x06, 0x07, 0x08 };

	frame.can_id = BIT(31) | BIT(30) | 1234;
	frame.can_dlc = sizeof(data);
	memcpy(frame.data, data, sizeof(frame.data));

	expected.rtr = CAN_REMOTEREQUEST;
	expected.id_type = CAN_EXTENDED_IDENTIFIER;
	expected.id = 1234U;
	expected.dlc = sizeof(data);

	can_copy_frame_to_zframe(&frame, &msg);

	LOG_HEXDUMP_DBG((const uint8_t *)&frame, sizeof(frame), "frame");
	LOG_HEXDUMP_DBG((const uint8_t *)&msg, sizeof(msg), "msg");
	LOG_HEXDUMP_DBG((const uint8_t *)&expected, sizeof(expected), "expected");

	zassert_equal(msg.rtr, expected.rtr, "RTR bit not set");
	zassert_equal(msg.id_type, expected.id_type, "Id-type bit not set");
	zassert_equal(msg.id, expected.id, "CAN id invalid");
	zassert_equal(msg.dlc, expected.dlc, "Msg length invalid");
}

/**
 * @brief Test of @a can_copy_zframe_to_frame()
 */
static void test_zcan_frame_to_can_frame(void)
{
	struct can_frame frame = { 0 };
	struct can_frame expected = { 0 };
	struct zcan_frame msg = { 0 };
	const uint8_t data[CAN_MAX_DLEN] = { 0x01, 0x02, 0x03, 0x04,
					  0x05, 0x06, 0x07, 0x08 };

	expected.can_id = BIT(31) | BIT(30) | 1234;
	expected.can_dlc = sizeof(data);
	memcpy(expected.data, data, sizeof(expected.data));

	msg.rtr = CAN_REMOTEREQUEST;
	msg.id_type = CAN_EXTENDED_IDENTIFIER;
	msg.id = 1234U;
	msg.dlc = sizeof(data);
	memcpy(msg.data, data, sizeof(data));

	can_copy_zframe_to_frame(&msg, &frame);

	LOG_HEXDUMP_DBG((const uint8_t *)&frame, sizeof(frame), "frame");
	LOG_HEXDUMP_DBG((const uint8_t *)&msg, sizeof(msg), "msg");
	LOG_HEXDUMP_DBG((const uint8_t *)&expected, sizeof(expected), "expected");

	zassert_equal(frame.can_id, expected.can_id, "CAN ID not same");
	zassert_mem_equal(&frame.data, &expected.data, sizeof(frame.data),
			  "CAN data not same");
	zassert_equal(frame.can_dlc, expected.can_dlc,
		      "CAN msg length not same");
}

/**
 * @brief Test of @a can_copy_filter_to_zfilter()
 */
static void test_can_filter_to_zcan_filter(void)
{
	struct can_filter filter = { 0 };
	struct zcan_filter expected = { 0 };
	struct zcan_filter msg_filter = { 0 };

	filter.can_id = BIT(31) | BIT(30) | 1234;
	filter.can_mask = BIT(31) | BIT(30) | 1234;

	expected.rtr = CAN_REMOTEREQUEST;
	expected.id_type = CAN_EXTENDED_IDENTIFIER;
	expected.id = 1234U;
	expected.rtr_mask = 1U;
	expected.id_mask = 1234U;

	can_copy_filter_to_zfilter(&filter, &msg_filter);

	LOG_HEXDUMP_DBG((const uint8_t *)&msg_filter, sizeof(msg_filter),
			"msg_filter");
	LOG_HEXDUMP_DBG((const uint8_t *)&filter, sizeof(filter), "filter");
	LOG_HEXDUMP_DBG((const uint8_t *)&expected, sizeof(expected), "expected");

	zassert_equal(msg_filter.rtr, expected.rtr, "RTR bit not set");
	zassert_equal(msg_filter.id_type, expected.id_type,
		      "Id-type bit not set");
	zassert_equal(msg_filter.id, expected.id,
		      "CAN id invalid");
	zassert_equal(msg_filter.rtr_mask, expected.rtr_mask,
		      "RTR mask bit not set");
	zassert_equal(msg_filter.id_mask, expected.id_mask,
		      "id mask not set");
}

/**
 * @brief Test of @a can_copy_zfilter_to_filter()
 */
static void test_zcan_filter_to_can_filter(void)
{
	struct can_filter filter = { 0 };
	struct can_filter expected = { 0 };
	struct zcan_filter msg_filter = { 0 };

	expected.can_id = BIT(31) | BIT(30) | 1234;
	expected.can_mask = BIT(31) | BIT(30) | 1234;

	msg_filter.rtr = CAN_REMOTEREQUEST;
	msg_filter.id_type = CAN_EXTENDED_IDENTIFIER;
	msg_filter.id = 1234U;
	msg_filter.rtr_mask = 1U;
	msg_filter.id_mask = 1234U;

	can_copy_zfilter_to_filter(&msg_filter, &filter);

	LOG_HEXDUMP_DBG((const uint8_t *)&msg_filter, sizeof(msg_filter),
			"msg_filter");
	LOG_HEXDUMP_DBG((const uint8_t *)&filter, sizeof(filter), "filter");
	LOG_HEXDUMP_DBG((const uint8_t *)&expected, sizeof(expected), "expected");

	zassert_equal(filter.can_id, expected.can_id, "CAN ID not same");
	zassert_equal(filter.can_mask, expected.can_mask, "CAN mask not same");
}

/**
 * @brief Test of @a can_dlc_to_bytes()
 */
static void test_can_dlc_to_bytes(void)
{
	uint8_t dlc;

	/* CAN 2.0B/CAN-FD DLC, 0 to 8 data bytes */
	for (dlc = 0; dlc <= 8; dlc++) {
		zassert_equal(can_dlc_to_bytes(dlc), dlc, "wrong number of bytes for DLC %u", dlc);
	}

	/* CAN-FD DLC, 12 to 64 data bytes in steps */
	zassert_equal(can_dlc_to_bytes(9),  12, "wrong number of bytes for DLC 9");
	zassert_equal(can_dlc_to_bytes(10), 16, "wrong number of bytes for DLC 10");
	zassert_equal(can_dlc_to_bytes(11), 20, "wrong number of bytes for DLC 11");
	zassert_equal(can_dlc_to_bytes(12), 24, "wrong number of bytes for DLC 12");
	zassert_equal(can_dlc_to_bytes(13), 32, "wrong number of bytes for DLC 13");
	zassert_equal(can_dlc_to_bytes(14), 48, "wrong number of bytes for DLC 14");
	zassert_equal(can_dlc_to_bytes(15), 64, "wrong number of bytes for DLC 15");
}

/**
 * @brief Test of @a can_bytes_to_dlc()
 */
static void test_can_bytes_to_dlc(void)
{
	uint8_t bytes;

	/* CAN 2.0B DLC, 0 to 8 data bytes */
	for (bytes = 0; bytes <= 8; bytes++) {
		zassert_equal(can_bytes_to_dlc(bytes), bytes, "wrong DLC for %u byte(s)", bytes);
	}

	/* CAN-FD DLC, 12 to 64 data bytes in steps */
	zassert_equal(can_bytes_to_dlc(12), 9,  "wrong DLC for 12 bytes");
	zassert_equal(can_bytes_to_dlc(16), 10, "wrong DLC for 16 bytes");
	zassert_equal(can_bytes_to_dlc(20), 11, "wrong DLC for 20 bytes");
	zassert_equal(can_bytes_to_dlc(24), 12, "wrong DLC for 24 bytes");
	zassert_equal(can_bytes_to_dlc(32), 13, "wrong DLC for 32 bytes");
	zassert_equal(can_bytes_to_dlc(48), 14, "wrong DLC for 48 bytes");
	zassert_equal(can_bytes_to_dlc(64), 15, "wrong DLC for 64 bytes");
}

void test_main(void)
{
	ztest_test_suite(can_utilities_tests,
			 ztest_unit_test(test_can_frame_to_zcan_frame),
			 ztest_unit_test(test_zcan_frame_to_can_frame),
			 ztest_unit_test(test_can_filter_to_zcan_filter),
			 ztest_unit_test(test_zcan_filter_to_can_filter),
			 ztest_unit_test(test_can_dlc_to_bytes),
			 ztest_unit_test(test_can_bytes_to_dlc));
	ztest_run_test_suite(can_utilities_tests);
}
