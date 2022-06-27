/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <ztest.h>

/**
 * @addtogroup t_can_driver
 * @{
 * @defgroup t_can_stm32 test_can_stm32
 * @}
 */

/**
 * @brief Test timeouts.
 */
#define TEST_SEND_TIMEOUT    K_MSEC(100)
#define TEST_RECEIVE_TIMEOUT K_MSEC(100)

/**
 * @brief Standard (11-bit) CAN IDs and masks used for testing.
 */
#define TEST_CAN_STD_ID      0x555
#define TEST_CAN_SOME_STD_ID 0x123

/**
 * @brief Extended (29-bit) CAN IDs and masks used for testing.
 */
#define TEST_CAN_EXT_ID      0x15555555
#define TEST_CAN_EXT_MASK    0x1FFFFFF0

/**
 * @brief Global variables.
 */
CAN_MSGQ_DEFINE(can_msgq, 5);

/**
 * @brief Standard (11-bit) CAN ID test frame.
 */
struct zcan_frame test_std_frame = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.id      = TEST_CAN_STD_ID,
	.dlc     = 8,
	.data    = { 1, 2, 3, 4, 5, 6, 7, 8 }
};

/**
 * @brief Standard (11-bit) CAN ID filter.
 */
const struct zcan_filter test_std_filter = {
	.id_type  = CAN_STANDARD_IDENTIFIER,
	.rtr      = CAN_DATAFRAME,
	.id       = TEST_CAN_STD_ID,
	.rtr_mask = 1,
	.id_mask  = CAN_STD_ID_MASK
};

/**
 * @brief Extended (29-bit) CAN ID filter.
 */
const struct zcan_filter test_ext_filter = {
	.id_type  = CAN_EXTENDED_IDENTIFIER,
	.rtr      = CAN_DATAFRAME,
	.id       = TEST_CAN_EXT_ID,
	.rtr_mask = 1,
	.id_mask  = CAN_EXT_ID_MASK
};

/**
 * @brief Extended (29-bit) CAN ID masked filter.
 */
const struct zcan_filter test_ext_masked_filter = {
	.id_type  = CAN_EXTENDED_IDENTIFIER,
	.rtr      = CAN_DATAFRAME,
	.id       = TEST_CAN_EXT_ID,
	.rtr_mask = 1,
	.id_mask  = TEST_CAN_EXT_MASK
};

/**
 * @brief Standard (11-bit) CAN ID filter. This filter matches
 * ``TEST_CAN_SOME_STD_ID``.
 */
const struct zcan_filter test_std_some_filter = {
	.id_type  = CAN_STANDARD_IDENTIFIER,
	.rtr      = CAN_DATAFRAME,
	.id       = TEST_CAN_SOME_STD_ID,
	.rtr_mask = 1,
	.id_mask  = CAN_STD_ID_MASK
};

/**
 * @brief Assert that two CAN frames are equal.
 *
 * @param frame1  First CAN frame.
 * @param frame2  Second CAN frame.
 */
static inline void assert_frame_equal(const struct zcan_frame *frame1,
				      const struct zcan_frame *frame2)
{
	zassert_equal(frame1->id_type, frame2->id_type, "ID type does not match");
	zassert_equal(frame1->rtr, frame2->rtr, "RTR bit does not match");
	zassert_equal(frame1->id, frame2->id, "ID does not match");
	zassert_equal(frame1->dlc, frame2->dlc, "DLC does not match");
	zassert_mem_equal(frame1->data, frame2->data, frame1->dlc, "Received data differ");
}

/**
 * @brief Send a CAN test frame with asserts.
 *
 * This function will block until the frame is transmitted or a test timeout
 * occurs.
 *
 * @param dev   Pointer to the device structure for the driver instance.
 * @param frame Pointer to the CAN frame to send.
 */
static void send_test_frame(const struct device *dev, const struct zcan_frame *frame)
{
	int err;

	err = can_send(dev, frame, TEST_SEND_TIMEOUT, NULL, NULL);
	zassert_not_equal(err, -EBUSY, "arbitration lost in loopback mode");
	zassert_equal(err, 0, "failed to send frame (err %d)", err);
}

/**
 * @brief Test a more advanced filter handling.
 *
 * Add more than one filter at the same time, remove and change the filters
 * before sending a frame. This tests the internal filter handling of the STM32
 * driver.
 */
static void test_filter_handling(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	struct zcan_frame frame_buffer;
	int filter_id_1;
	int filter_id_2;
	int err;

	zassert_true(device_is_ready(dev), "CAN device not ready");

	/* Set driver to loopback mode */
	err = can_set_mode(dev, CAN_MODE_LOOPBACK);
	zassert_equal(err, 0, "failed to set loopback mode");

	/* Add a extended and masked filter (1 bank/filter) */
	filter_id_1 = can_add_rx_filter_msgq(dev, &can_msgq, &test_ext_masked_filter);
	zassert_not_equal(filter_id_1, -ENOSPC, "no filters available");
	zassert_true(filter_id_1 >= 0, "negative filter number");

	/* Add a standard non-masked filter (1/4 bank/filter) */
	filter_id_2 = can_add_rx_filter_msgq(dev, &can_msgq, &test_std_filter);
	zassert_not_equal(filter_id_2, -ENOSPC, "no filters available");
	zassert_true(filter_id_2 >= 0, "negative filter number");

	/*
	 * Remove the first filter (first bank gets free) and add a different
	 * standard none masked filter (1/4 bank/filter).  Bank 0 is extended to
	 * 4 filters/bank which leads to a left shift of the first filter by 3
	 * and tests the corner case of the last filter is used.
	 */
	can_remove_rx_filter(dev, filter_id_1);
	filter_id_1 = can_add_rx_filter_msgq(dev, &can_msgq, &test_std_some_filter);
	zassert_not_equal(filter_id_1, -ENOSPC, "no filters available");
	zassert_true((filter_id_1 >= 0), "negative filter number");

	/* Test message sending and receiving */
	send_test_frame(dev, &test_std_frame);
	err = k_msgq_get(&can_msgq, &frame_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");
	assert_frame_equal(&test_std_frame, &frame_buffer);
	err = k_msgq_get(&can_msgq, &frame_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, -EAGAIN, "more than one frame in the queue");

	/*
	 * Remove first filter (gets free) and add an extended filter. This
	 * shrinks bank 0 to 2 filters/bank which leads to a right shift of the
	 * first buffer by two.
	 */
	can_remove_rx_filter(dev, filter_id_1);
	filter_id_1 = can_add_rx_filter_msgq(dev, &can_msgq, &test_ext_filter);
	zassert_not_equal(filter_id_1, -ENOSPC, "no filters available");
	zassert_true((filter_id_1 >= 0), "negative filter number");

	/* Test message sending and receiving */
	send_test_frame(dev, &test_std_frame);
	err = k_msgq_get(&can_msgq, &frame_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");
	assert_frame_equal(&test_std_frame, &frame_buffer);

	/* Remove both filters */
	can_remove_rx_filter(dev, filter_id_1);
	can_remove_rx_filter(dev, filter_id_2);
}

void test_main(void)
{
	ztest_test_suite(can_stm32_tests,
			 ztest_unit_test(test_filter_handling));
	ztest_run_test_suite(can_stm32_tests);
}
