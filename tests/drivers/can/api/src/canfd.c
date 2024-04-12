/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/ztest.h>

#include "common.h"

/**
 * @addtogroup t_can_driver
 * @{
 * @defgroup t_can_canfd test_can_canfd
 * @}
 */

static void tx_std_callback_1(const struct device *dev, int error, void *user_data)
{
	const struct can_frame *frame = user_data;

	k_sem_give(&tx_callback_sem);

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_STD_ID_1, "ID does not match");
}

static void tx_std_callback_2(const struct device *dev, int error, void *user_data)
{
	const struct can_frame *frame = user_data;

	k_sem_give(&tx_callback_sem);

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_STD_ID_2, "ID does not match");
}

static void rx_std_callback_1(const struct device *dev, struct can_frame *frame, void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_frame_1, 0);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_std_filter_1, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

static void rx_std_callback_2(const struct device *dev, struct can_frame *frame, void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_frame_2, 0);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_std_filter_2, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

static void rx_std_callback_fd_1(const struct device *dev, struct can_frame *frame,
				 void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_fdf_frame_1, 0);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_std_filter_1, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

static void rx_std_callback_fd_2(const struct device *dev, struct can_frame *frame,
				 void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_fdf_frame_2, 0);
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
static void send_test_frame(const struct device *dev, const struct can_frame *frame)
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
static void send_test_frame_nowait(const struct device *dev, const struct can_frame *frame,
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
static inline int add_rx_msgq(const struct device *dev, const struct can_filter *filter)
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
				const struct can_filter *filter,
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
static void send_receive(const struct can_filter *filter1,
			 const struct can_filter *filter2,
			 const struct can_frame *frame1,
			 const struct can_frame *frame2)
{
	struct can_frame frame_buffer;
	int filter_id_1;
	int filter_id_2;
	int err;

	filter_id_1 = add_rx_msgq(can_dev, filter1);
	send_test_frame(can_dev, frame1);

	err = k_msgq_get(&can_msgq, &frame_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");

	assert_frame_equal(&frame_buffer, frame1, 0);
	can_remove_rx_filter(can_dev, filter_id_1);

	k_sem_reset(&tx_callback_sem);

	if ((frame1->flags & CAN_FRAME_FDF) != 0) {
		filter_id_1 = add_rx_filter(can_dev, filter1, rx_std_callback_fd_1);
	} else {
		filter_id_1 = add_rx_filter(can_dev, filter1, rx_std_callback_1);
	}

	if ((frame2->flags & CAN_FRAME_FDF) != 0) {
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
 * @brief Test getting the CAN controller capabilities.
 */
ZTEST(canfd, test_canfd_get_capabilities)
{
	can_mode_t cap;
	int err;

	err = can_get_capabilities(can_dev, &cap);
	zassert_equal(err, 0, "failed to get CAN capabilities (err %d)", err);
	zassert_not_equal(cap & (CAN_MODE_LOOPBACK | CAN_MODE_FD), 0,
			  "CAN FD loopback mode not supported");
}

/**
 * @brief Test send/receive with standard (11-bit) CAN IDs and classic CAN frames.
 */
ZTEST(canfd, test_send_receive_classic)
{
	send_receive(&test_std_filter_1, &test_std_filter_2,
		     &test_std_frame_1, &test_std_frame_2);
}

/**
 * @brief Test send/receive with standard (11-bit) CAN IDs and CAN FD frames.
 */
ZTEST(canfd, test_send_receive_fd)
{
	send_receive(&test_std_filter_1, &test_std_filter_2,
		     &test_std_fdf_frame_1, &test_std_fdf_frame_2);
}

/**
 * @brief Test send/receive with (11-bit) CAN IDs, mixed classic and CAN FD frames.
 */
ZTEST(canfd, test_send_receive_mixed)
{
	send_receive(&test_std_filter_1, &test_std_filter_2,
		     &test_std_fdf_frame_1, &test_std_frame_2);
}

/**
 * @brief Test that CAN RX filters are preserved through CAN controller mode changes.
 */
static void check_filters_preserved_between_modes(can_mode_t first, can_mode_t second)
{
	struct can_frame frame;
	enum can_state state;
	int filter_id_1;
	int filter_id_2;
	int err;

	/* Stop controller and set first mode */
	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_get_state(can_dev, &state, NULL);
	zassert_equal(err, 0, "failed to get CAN state (err %d)", err);
	zassert_equal(state, CAN_STATE_STOPPED, "CAN controller not stopped");

	err = can_set_mode(can_dev, first | CAN_MODE_LOOPBACK);
	zassert_equal(err, 0, "failed to set first loopback mode (err %d)", err);
	zassert_equal(first | CAN_MODE_LOOPBACK, can_get_mode(can_dev));

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);

	/* Add classic CAN and CAN FD filter */
	filter_id_1 = add_rx_msgq(can_dev, &test_std_filter_1);
	filter_id_2 = add_rx_msgq(can_dev, &test_std_filter_2);

	/* Verify classic filter in first mode */
	send_test_frame(can_dev, &test_std_frame_1);
	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");
	assert_frame_equal(&frame, &test_std_frame_1, 0);

	if ((first & CAN_MODE_FD) != 0) {
		/* Verify CAN FD filter in first mode */
		send_test_frame(can_dev, &test_std_fdf_frame_2);
		err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
		zassert_equal(err, 0, "receive timeout");
		assert_frame_equal(&frame, &test_std_fdf_frame_2, 0);
	}

	/* Stop controller and set second mode */
	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_get_state(can_dev, &state, NULL);
	zassert_equal(err, 0, "failed to get CAN state (err %d)", err);
	zassert_equal(state, CAN_STATE_STOPPED, "CAN controller not stopped");

	err = can_set_mode(can_dev, second | CAN_MODE_LOOPBACK);
	zassert_equal(err, 0, "failed to set second loopback mode (err %d)", err);
	zassert_equal(second | CAN_MODE_LOOPBACK, can_get_mode(can_dev));

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);

	/* Verify classic filter in second mode */
	send_test_frame(can_dev, &test_std_frame_1);
	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");
	assert_frame_equal(&frame, &test_std_frame_1, 0);

	if ((second & CAN_MODE_FD) != 0) {
		/* Verify CAN FD filter in second mode */
		send_test_frame(can_dev, &test_std_fdf_frame_2);
		err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
		zassert_equal(err, 0, "receive timeout");
		assert_frame_equal(&frame, &test_std_fdf_frame_2, 0);
	}

	/* Stop controller and restore CAN FD loopback mode */
	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_get_state(can_dev, &state, NULL);
	zassert_equal(err, 0, "failed to get CAN state (err %d)", err);
	zassert_equal(state, CAN_STATE_STOPPED, "CAN controller not stopped");

	err = can_set_mode(can_dev, CAN_MODE_FD | CAN_MODE_LOOPBACK);
	zassert_equal(err, 0, "failed to set loopback-mode (err %d)", err);
	zassert_equal(CAN_MODE_FD | CAN_MODE_LOOPBACK, can_get_mode(can_dev));

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);

	can_remove_rx_filter(can_dev, filter_id_1);
	can_remove_rx_filter(can_dev, filter_id_2);
}

/**
 * @brief Test that CAN RX filters are preserved through CAN controller mode changes between classic
 * CAN and CAN FD.
 */
ZTEST_USER(canfd, test_filters_preserved_through_classic_to_fd_mode_change)
{
	check_filters_preserved_between_modes(CAN_MODE_NORMAL, CAN_MODE_FD);
}

/**
 * @brief Test that CAN RX filters are preserved through CAN controller mode changes between CAN FD
 * and classic CAN.
 */
ZTEST_USER(canfd, test_filters_preserved_through_fd_to_classic_mode_change)
{
	check_filters_preserved_between_modes(CAN_MODE_FD, CAN_MODE_NORMAL);
}

/**
 * @brief Test that the minimum timing values for the data phase can be set.
 */
ZTEST_USER(canfd, test_set_timing_data_min)
{
	int err;

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_set_timing_data(can_dev, can_get_timing_data_min(can_dev));
	zassert_equal(err, 0, "failed to set minimum timing data parameters (err %d)", err);

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test setting a too high data phase bitrate.
 */
ZTEST_USER(canfd, test_set_bitrate_too_high)
{
	uint32_t max = can_get_bitrate_max(can_dev);
	int err;

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_set_bitrate_data(can_dev, max + 1);
	zassert_equal(err, -ENOTSUP, "too high data phase bitrate accepted");

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test using an invalid sample point.
 */
ZTEST_USER(canfd, test_invalid_sample_point)
{
	struct can_timing timing;
	int err;

	err = can_calc_timing_data(can_dev, &timing, TEST_BITRATE_3, 1000);
	zassert_equal(err, -EINVAL, "invalid sample point of 100.0% accepted (err %d)", err);
}

/**
 * @brief Test that the maximum timing values for the data phase can be set.
 */
ZTEST_USER(canfd, test_set_timing_data_max)
{
	int err;

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_set_timing_data(can_dev, can_get_timing_data_max(can_dev));
	zassert_equal(err, 0, "failed to set maximum timing data parameters (err %d)", err);

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test setting data phase bitrate is not allowed while started.
 */
ZTEST_USER(canfd, test_set_bitrate_data_while_started)
{
	int err;

	err = can_set_bitrate_data(can_dev, TEST_BITRATE_3);
	zassert_not_equal(err, 0, "changed data bitrate while started");
	zassert_equal(err, -EBUSY, "wrong error return code (err %d)", err);
}

/**
 * @brief Test setting data phase timing is not allowed while started.
 */
ZTEST_USER(canfd, test_set_timing_data_while_started)
{
	struct can_timing timing = { 0 };
	int err;

	err = can_calc_timing_data(can_dev, &timing, TEST_BITRATE_3, TEST_SAMPLE_POINT);
	zassert_ok(err, "failed to calculate data timing (err %d)", err);

	err = can_set_timing_data(can_dev, &timing);
	zassert_not_equal(err, 0, "changed data timing while started");
	zassert_equal(err, -EBUSY, "wrong error return code (err %d)", err);
}

static bool canfd_predicate(const void *state)
{
	can_mode_t cap;
	int err;

	ARG_UNUSED(state);

	if (!device_is_ready(can_dev)) {
		TC_PRINT("CAN device not ready");
		return false;
	}

	err = can_get_capabilities(can_dev, &cap);
	zassert_equal(err, 0, "failed to get CAN controller capabilities (err %d)", err);

	if ((cap & CAN_MODE_FD) == 0) {
		return false;
	}

	return true;
}

void *canfd_setup(void)
{
	int err;

	k_sem_init(&rx_callback_sem, 0, 2);
	k_sem_init(&tx_callback_sem, 0, 2);

	(void)can_stop(can_dev);

	err = can_set_mode(can_dev, CAN_MODE_LOOPBACK | CAN_MODE_FD);
	zassert_equal(err, 0, "failed to set CAN FD loopback mode (err %d)", err);
	zassert_equal(CAN_MODE_LOOPBACK | CAN_MODE_FD, can_get_mode(can_dev));

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);

	return NULL;
}

ZTEST_SUITE(canfd, canfd_predicate, canfd_setup, NULL, NULL, NULL);
