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
 * @defgroup t_can_canfd test_can_canfd
 * @}
 */


/**
 * @brief Test timeouts.
 */
#define TEST_SEND_TIMEOUT    K_MSEC(100)
#define TEST_RECEIVE_TIMEOUT K_MSEC(100)

/**
 * @brief Standard (11-bit) CAN IDs used for testing.
 */
#define TEST_CAN_STD_ID_1      0x555
#define TEST_CAN_STD_ID_2      0x556

/**
 * @brief Global variables.
 */
const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
struct k_sem rx_callback_sem;
struct k_sem tx_callback_sem;

CAN_MSGQ_DEFINE(can_msgq, 5);

/**
 * @brief Standard (11-bit) CAN ID frame 1.
 */
const struct zcan_frame test_std_frame_1 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.id      = TEST_CAN_STD_ID_1,
	.dlc     = 8,
	.data    = { 1, 2, 3, 4, 5, 6, 7, 8 }
};

/**
 * @brief Standard (11-bit) CAN ID frame 2.
 */
const struct zcan_frame test_std_frame_2 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.id      = TEST_CAN_STD_ID_2,
	.dlc     = 8,
	.data    = { 1, 2, 3, 4, 5, 6, 7, 8 }
};

/**
 * @brief Standard (11-bit) CAN ID frame 1 with CAN-FD payload.
 */
const struct zcan_frame test_std_frame_fd_1 = {
	.id  = TEST_CAN_STD_ID_1,
	.fd      = 1,
	.rtr     = CAN_DATAFRAME,
	.id_type = CAN_STANDARD_IDENTIFIER,
	.dlc     = 0xf,
	.brs     = 1,
	.data    = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
		    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
		    46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
		    61, 62, 63, 64 }
};

/**
 * @brief Standard (11-bit) CAN ID frame 1 with CAN-FD payload.
 */
const struct zcan_frame test_std_frame_fd_2 = {
	.id  = TEST_CAN_STD_ID_2,
	.fd      = 1,
	.rtr     = CAN_DATAFRAME,
	.id_type = CAN_STANDARD_IDENTIFIER,
	.dlc     = 0xf,
	.brs     = 1,
	.data    = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
		    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
		    46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
		    61, 62, 63, 64 }
};

/**
 * @brief Standard (11-bit) CAN ID filter 1.
 */
const struct zcan_filter test_std_filter_1 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_STD_ID_1,
	.rtr_mask = 1,
	.id_mask = CAN_STD_ID_MASK
};

/**
 * @brief Standard (11-bit) CAN ID filter 2.
 */
const struct zcan_filter test_std_filter_2 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_STD_ID_2,
	.rtr_mask = 1,
	.id_mask = CAN_STD_ID_MASK
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
	zassert_equal(frame1->fd, frame2->fd, "FD bit does not match");
	zassert_equal(frame1->rtr, frame2->rtr, "RTR bit does not match");
	zassert_equal(frame1->id, frame2->id, "ID does not match");
	zassert_equal(frame1->dlc, frame2->dlc, "DLC does not match");
	zassert_mem_equal(frame1->data, frame2->data, frame1->dlc, "Received data differ");
}

static void tx_std_callback_1(const struct device *dev, int error, void *user_data)
{
	const struct zcan_frame *frame = user_data;

	k_sem_give(&tx_callback_sem);

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_STD_ID_1, "ID does not match");
}

static void tx_std_callback_2(const struct device *dev, int error, void *user_data)
{
	const struct zcan_frame *frame = user_data;

	k_sem_give(&tx_callback_sem);

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_STD_ID_2, "ID does not match");
}

static void rx_std_callback_1(const struct device *dev, struct zcan_frame *frame, void *user_data)
{
	struct zcan_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_frame_1);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_std_filter_1, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

static void rx_std_callback_2(const struct device *dev, struct zcan_frame *frame, void *user_data)
{
	struct zcan_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_frame_2);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_std_filter_2, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

static void rx_std_callback_fd_1(const struct device *dev, struct zcan_frame *frame,
				 void *user_data)
{
	struct zcan_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_frame_fd_1);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_std_filter_1, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

static void rx_std_callback_fd_2(const struct device *dev, struct zcan_frame *frame,
				 void *user_data)
{
	struct zcan_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_frame_fd_2);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_std_filter_2, "filter does not match");

	k_sem_give(&rx_callback_sem);
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
 * @brief Send a CAN test frame with asserts.
 *
 * This function will block until the frame is queued or a test timeout
 * occurs.
 *
 * @param dev      Pointer to the device structure for the driver instance.
 * @param frame    Pointer to the CAN frame to send.
 * @param callback Transmit callback function.
 */
static void send_test_frame_nowait(const struct device *dev, const struct zcan_frame *frame,
				   can_tx_callback_t callback)
{
	int err;

	err = can_send(dev, frame, TEST_SEND_TIMEOUT, callback, (void *)frame);
	zassert_not_equal(err, -EBUSY, "arbitration lost in loopback mode");
	zassert_equal(err, 0, "failed to send frame (err %d)", err);
}

/**
 * @brief Add a CAN message queue with asserts.
 *
 * @param dev    Pointer to the device structure for the driver instance.
 * @param filter CAN filter for the CAN message queue.
 *
 * @return CAN filter ID.
 */
static inline int add_rx_msgq(const struct device *dev, const struct zcan_filter *filter)
{
	int filter_id;

	filter_id = can_add_rx_filter_msgq(dev, &can_msgq, filter);
	zassert_not_equal(filter_id, -ENOSPC, "no filters available");
	zassert_true(filter_id >= 0, "negative filter number");

	return filter_id;
}

/**
 * @brief Add a CAN filter with asserts.
 *
 * @param dev      Pointer to the device structure for the driver instance.
 * @param filter   CAN filter.
 * @param callback Receive callback function.
 *
 * @return CAN filter ID.
 */
static inline int add_rx_filter(const struct device *dev,
				const struct zcan_filter *filter,
				can_rx_callback_t callback)
{
	int filter_id;

	k_sem_reset(&rx_callback_sem);

	filter_id = can_add_rx_filter(dev, callback, (void *)filter, filter);
	zassert_not_equal(filter_id, -ENOSPC, "no filters available");
	zassert_true(filter_id >= 0, "negative filter number");

	return filter_id;
}

/**
 * @brief Perform a send/receive test with a set of CAN ID filters and CAN frames.
 *
 * @param filter1 CAN filter 1
 * @param filter2 CAN filter 2
 * @param frame1  CAN frame 1
 * @param frame2  CAN frame 2
 */
static void send_receive(const struct zcan_filter *filter1,
			 const struct zcan_filter *filter2,
			 const struct zcan_frame *frame1,
			 const struct zcan_frame *frame2)
{
	struct zcan_frame frame_buffer;
	int filter_id_1;
	int filter_id_2;
	int err;

	filter_id_1 = add_rx_msgq(can_dev, filter1);
	send_test_frame(can_dev, frame1);

	err = k_msgq_get(&can_msgq, &frame_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");

	assert_frame_equal(&frame_buffer, frame1);
	can_remove_rx_filter(can_dev, filter_id_1);

	k_sem_reset(&tx_callback_sem);

	if (frame1->fd) {
		filter_id_1 = add_rx_filter(can_dev, filter1, rx_std_callback_fd_1);
	} else {
		filter_id_1 = add_rx_filter(can_dev, filter1, rx_std_callback_1);
	}

	if (frame2->fd) {
		filter_id_2 = add_rx_filter(can_dev, filter2, rx_std_callback_fd_2);
	} else {
		filter_id_2 = add_rx_filter(can_dev, filter2, rx_std_callback_2);
	}

	send_test_frame_nowait(can_dev, frame1, tx_std_callback_1);
	send_test_frame_nowait(can_dev, frame2, tx_std_callback_2);

	err = k_sem_take(&rx_callback_sem, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");

	err = k_sem_take(&rx_callback_sem, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");

	err = k_sem_take(&tx_callback_sem, TEST_SEND_TIMEOUT);
	zassert_equal(err, 0, "missing TX callback");

	err = k_sem_take(&tx_callback_sem, TEST_SEND_TIMEOUT);
	zassert_equal(err, 0, "missing TX callback");

	can_remove_rx_filter(can_dev, filter_id_1);
	can_remove_rx_filter(can_dev, filter_id_2);
}

/**
 * @brief Test configuring the CAN controller for loopback mode.
 *
 * This test case must be run before sending/receiving test cases as it allows
 * these test cases to send/receive their own frames.
 */
static void test_set_loopback(void)
{
	int err;

	err = can_set_mode(can_dev, CAN_MODE_LOOPBACK | CAN_MODE_FD);
	zassert_equal(err, 0, "failed to set loopback-mode (err %d)", err);
}

/**
 * @brief Test send/receive with standard (11-bit) CAN IDs and classic CAN frames.
 */
void test_send_receive_classic(void)
{
	send_receive(&test_std_filter_1, &test_std_filter_2,
		     &test_std_frame_1, &test_std_frame_2);
}

/**
 * @brief Test send/receive with standard (11-bit) CAN IDs and CAN-FD frames.
 */
void test_send_receive_fd(void)
{
	send_receive(&test_std_filter_1, &test_std_filter_2,
		     &test_std_frame_fd_1, &test_std_frame_fd_2);
}

/**
 * @brief Test send/receive with (11-bit) CAN IDs, mixed classic and CAN-FD frames.
 */
void test_send_receive_mixed(void)
{
	send_receive(&test_std_filter_1, &test_std_filter_2,
		     &test_std_frame_fd_1, &test_std_frame_2);
}

void test_main(void)
{
	k_sem_init(&rx_callback_sem, 0, 2);
	k_sem_init(&tx_callback_sem, 0, 2);

	zassert_true(device_is_ready(can_dev), "CAN device not ready");

	ztest_test_suite(canfd_driver,
			 ztest_unit_test(test_set_loopback),
			 ztest_unit_test(test_send_receive_classic),
			 ztest_unit_test(test_send_receive_fd),
			 ztest_unit_test(test_send_receive_mixed));
	ztest_run_test_suite(canfd_driver);
}
