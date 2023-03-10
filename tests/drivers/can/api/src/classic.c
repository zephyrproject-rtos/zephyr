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
 * @defgroup t_can_classic test_can_classic
 * @}
 */

/**
 * @brief Standard (11-bit) CAN ID transmit callback 1.
 *
 * See @a can_tx_callback_t() for argument description.
 */
static void tx_std_callback_1(const struct device *dev, int error, void *user_data)
{
	const struct can_frame *frame = user_data;

	k_sem_give(&tx_callback_sem);

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_STD_ID_1, "ID does not match");
}

/**
 * @brief Standard (11-bit) CAN ID transmit callback 2.
 *
 * See @a can_tx_callback_t() for argument description.
 */
static void tx_std_callback_2(const struct device *dev, int error, void *user_data)
{
	const struct can_frame *frame = user_data;

	k_sem_give(&tx_callback_sem);

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_STD_ID_2, "ID does not match");
}

/**
 * @brief Extended (29-bit) CAN ID transmit callback 1.
 *
 * See @a can_tx_callback_t() for argument description.
 */
static void tx_ext_callback_1(const struct device *dev, int error, void *user_data)
{
	const struct can_frame *frame = user_data;

	k_sem_give(&tx_callback_sem);

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_EXT_ID_1, "ID does not match");
}

/**
 * @brief Extended (29-bit) CAN ID transmit callback 2.
 *
 * See @a can_tx_callback_t() for argument description.
 */
static void tx_ext_callback_2(const struct device *dev, int error, void *user_data)
{
	const struct can_frame *frame = user_data;

	k_sem_give(&tx_callback_sem);

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_EXT_ID_2, "ID does not match");
}

/**
 * @brief Standard (11-bit) CAN ID receive callback 1.
 *
 * See @a can_rx_callback_t() for argument description.
 */
static void rx_std_callback_1(const struct device *dev, struct can_frame *frame,
			      void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_frame_1, 0);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_std_filter_1, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

/**
 * @brief Standard (11-bit) CAN ID receive callback 2.
 *
 * See @a can_rx_callback_t() for argument description.
 */
static void rx_std_callback_2(const struct device *dev, struct can_frame *frame,
			      void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_frame_2, 0);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_std_filter_2, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

/**
 * @brief Standard (11-bit) masked CAN ID receive callback 1.
 *
 * See @a can_rx_callback_t() for argument description.
 */
static void rx_std_mask_callback_1(const struct device *dev, struct can_frame *frame,
				   void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_frame_1, 0x0F);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_std_masked_filter_1, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

/**
 * @brief Standard (11-bit) masked CAN ID receive callback 2.
 *
 * See @a can_rx_callback_t() for argument description.
 */
static void rx_std_mask_callback_2(const struct device *dev, struct can_frame *frame,
				   void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_std_frame_2, 0x0F);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_std_masked_filter_2, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

/**
 * @brief Extended (29-bit) CAN ID receive callback 1.
 *
 * See @a can_rx_callback_t() for argument description.
 */
static void rx_ext_callback_1(const struct device *dev, struct can_frame *frame,
			      void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_ext_frame_1, 0);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_ext_filter_1, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

/**
 * @brief Extended (29-bit) CAN ID receive callback 2.
 *
 * See @a can_rx_callback_t() for argument description.
 */
static void rx_ext_callback_2(const struct device *dev, struct can_frame *frame,
			      void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_ext_frame_2, 0);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_ext_filter_2, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

/**
 * @brief Extended (29-bit) masked CAN ID receive callback 1.
 *
 * See @a can_rx_callback_t() for argument description.
 */
static void rx_ext_mask_callback_1(const struct device *dev, struct can_frame *frame,
				   void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_ext_frame_1, 0x0F);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_ext_masked_filter_1, "filter does not match");

	k_sem_give(&rx_callback_sem);
}

/**
 * @brief Extended (29-bit) masked CAN ID receive callback 2.
 *
 * See @a can_rx_callback_t() for argument description.
 */
static void rx_ext_mask_callback_2(const struct device *dev, struct can_frame *frame,
				   void *user_data)
{
	struct can_filter *filter = user_data;

	assert_frame_equal(frame, &test_ext_frame_2, 0x0F);
	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal_ptr(filter, &test_ext_masked_filter_2, "filter does not match");

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
	uint32_t mask = 0U;
	int filter_id_1;
	int filter_id_2;
	int err;

	filter_id_1 = add_rx_msgq(can_dev, filter1);
	send_test_frame(can_dev, frame1);

	err = k_msgq_get(&can_msgq, &frame_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");

	if ((filter1->flags & CAN_FILTER_IDE) != 0) {
		if (filter1->mask != CAN_EXT_ID_MASK) {
			mask = 0x0F;
		}
	} else {
		if (filter1->mask != CAN_STD_ID_MASK) {
			mask = 0x0F;
		}
	}

	assert_frame_equal(&frame_buffer, frame1, mask);
	can_remove_rx_filter(can_dev, filter_id_1);

	k_sem_reset(&tx_callback_sem);

	if ((frame1->flags & CAN_FRAME_IDE) != 0) {
		if (filter1->mask == CAN_EXT_ID_MASK) {
			filter_id_1 = add_rx_filter(can_dev, filter1, rx_ext_callback_1);
			filter_id_2 = add_rx_filter(can_dev, filter2, rx_ext_callback_2);
			send_test_frame_nowait(can_dev, frame1, tx_ext_callback_1);
			send_test_frame_nowait(can_dev, frame2, tx_ext_callback_2);
		} else {
			filter_id_1 = add_rx_filter(can_dev, filter1, rx_ext_mask_callback_1);
			filter_id_2 = add_rx_filter(can_dev, filter2, rx_ext_mask_callback_2);
			send_test_frame_nowait(can_dev, frame1, tx_ext_callback_1);
			send_test_frame_nowait(can_dev, frame2, tx_ext_callback_2);
		}
	} else {
		if (filter1->mask == CAN_STD_ID_MASK) {
			filter_id_1 = add_rx_filter(can_dev, filter1, rx_std_callback_1);
			filter_id_2 = add_rx_filter(can_dev, filter2, rx_std_callback_2);
			send_test_frame_nowait(can_dev, frame1, tx_std_callback_1);
			send_test_frame_nowait(can_dev, frame2, tx_std_callback_2);
		} else {
			filter_id_1 = add_rx_filter(can_dev, filter1, rx_std_mask_callback_1);
			filter_id_2 = add_rx_filter(can_dev, filter2, rx_std_mask_callback_2);
			send_test_frame_nowait(can_dev, frame1, tx_std_callback_1);
			send_test_frame_nowait(can_dev, frame2, tx_std_callback_2);
		}
	}

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
 * @brief Perform a send/receive test with a set of CAN ID filters and CAN frames, RTR and data
 * frames.
 *
 * @param data_filter CAN data filter
 * @param rtr_filter  CAN RTR filter
 * @param data_frame  CAN data frame
 * @param rtr_frame   CAN RTR frame
 */
void send_receive_rtr(const struct can_filter *data_filter,
		      const struct can_filter *rtr_filter,
		      const struct can_frame *data_frame,
		      const struct can_frame *rtr_frame)
{
	struct can_frame frame;
	int filter_id;
	int err;

	filter_id = can_add_rx_filter_msgq(can_dev, &can_msgq, rtr_filter);
	if (filter_id == -ENOTSUP) {
		/* Not all CAN controller drivers support remote transmission requests */
		ztest_test_skip();
	}

	zassert_not_equal(filter_id, -ENOSPC, "no filters available");
	zassert_true(filter_id >= 0, "negative filter number");

	/* Verify that RTR filter does not match data frame */
	send_test_frame(can_dev, data_frame);
	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, -EAGAIN, "Data frame passed RTR filter");

	/* Verify that RTR filter matches RTR frame */
	send_test_frame(can_dev, rtr_frame);
	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");
	assert_frame_equal(&frame, rtr_frame, 0);

	can_remove_rx_filter(can_dev, filter_id);

	filter_id = add_rx_msgq(can_dev, data_filter);

	/* Verify that data filter does not match RTR frame */
	send_test_frame(can_dev, rtr_frame);
	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, -EAGAIN, "RTR frame passed data filter");

	/* Verify that data filter matches data frame */
	send_test_frame(can_dev, data_frame);
	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");
	assert_frame_equal(&frame, data_frame, 0);

	can_remove_rx_filter(can_dev, filter_id);
}

/**
 * @brief Test getting the CAN core clock rate.
 */
ZTEST_USER(can_classic, test_get_core_clock)
{
	uint32_t rate;
	int err;

	err = can_get_core_clock(can_dev, &rate);
	zassert_equal(err, 0, "failed to get CAN core clock rate (err %d)", err);
	zassert_not_equal(rate, 0, "CAN core clock rate is 0");
}

/**
 * @brief Test getting the CAN controller capabilities.
 */
ZTEST_USER(can_classic, test_get_capabilities)
{
	can_mode_t cap;
	int err;

	err = can_get_capabilities(can_dev, &cap);
	zassert_equal(err, 0, "failed to get CAN capabilities (err %d)", err);
	zassert_not_equal(cap & CAN_MODE_LOOPBACK, 0, "CAN loopback mode not supported");
}

/**
 * @brief CAN state change callback.
 */
static void state_change_callback(const struct device *dev, enum can_state state,
				  struct can_bus_err_cnt err_cnt, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(state);
	ARG_UNUSED(err_cnt);
	ARG_UNUSED(user_data);
}

/**
 * @brief Test setting the CAN state change callback.
 */
ZTEST(can_classic, test_set_state_change_callback)
{
	/* It is not possible to provoke a change of state, but test the API call */
	can_set_state_change_callback(can_dev, state_change_callback, NULL);
	can_set_state_change_callback(can_dev, NULL, NULL);
}

/**
 * @brief Test setting a too high bitrate.
 */
ZTEST_USER(can_classic, test_set_bitrate_too_high)
{
	uint32_t max;
	int err;

	err = can_get_max_bitrate(can_dev, &max);
	if (err == -ENOSYS) {
		ztest_test_skip();
	}

	zassert_equal(err, 0, "failed to get max bitrate (err %d)", err);
	zassert_not_equal(max, 0, "max bitrate is 0");

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_set_bitrate(can_dev, max + 1);
	zassert_equal(err, -ENOTSUP, "too high bitrate accepted");

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test setting bitrate.
 */
ZTEST_USER(can_classic, test_set_bitrate)
{
	int err;

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_set_bitrate(can_dev, TEST_BITRATE_1);
	zassert_equal(err, 0, "failed to set bitrate");

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test sending a message with no filters installed.
 *
 * This basic test work since the CAN controller is in loopback mode and
 * therefore ACKs its own frame.
 */
ZTEST_USER(can_classic, test_send_and_forget)
{
	send_test_frame(can_dev, &test_std_frame_1);
}

/**
 * @brief Test adding basic filters.
 *
 * Test each filter type but only one filter at a time.
 */
ZTEST(can_classic, test_add_filter)
{
	int filter_id;

	filter_id = add_rx_filter(can_dev, &test_std_filter_1, rx_std_callback_1);
	can_remove_rx_filter(can_dev, filter_id);

	filter_id = add_rx_filter(can_dev, &test_ext_filter_1, rx_ext_callback_1);
	can_remove_rx_filter(can_dev, filter_id);

	filter_id = add_rx_msgq(can_dev, &test_std_filter_1);
	can_remove_rx_filter(can_dev, filter_id);

	filter_id = add_rx_msgq(can_dev, &test_ext_filter_1);
	can_remove_rx_filter(can_dev, filter_id);

	filter_id = add_rx_filter(can_dev, &test_std_masked_filter_1, rx_std_mask_callback_1);
	can_remove_rx_filter(can_dev, filter_id);

	filter_id = add_rx_filter(can_dev, &test_ext_masked_filter_1, rx_ext_mask_callback_1);
	can_remove_rx_filter(can_dev, filter_id);
}

/**
 * @brief Test adding up to and above the maximum number of RX filters.
 *
 * @param ide standard (11-bit) CAN ID filters if false, or extended (29-bit) CAN ID filters if
 *            true.
 * @param id_mask filter
 */
static void add_remove_max_filters(bool ide)
{
	uint32_t id_mask = ide ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK;
	struct can_filter filter = {
		.flags = CAN_FILTER_DATA | (ide ? CAN_FILTER_IDE : 0),
		.id = 0,
		.mask = id_mask,
	};
	int filter_id;
	int max;
	int i;

	max = can_get_max_filters(can_dev, ide);
	if (max == -ENOSYS || max == 0) {
		/*
		 * Skip test if max is not known or no filters of the given type
		 * is supported.
		 */
		ztest_test_skip();
	}

	zassert_true(max > 0, "failed to get max filters (err %d)", max);

	int filter_ids[max];

	for (i = 0; i < max; i++) {
		filter.id++;
		filter_ids[i] = add_rx_msgq(can_dev, &filter);
	}

	filter.id++;
	filter_id = can_add_rx_filter_msgq(can_dev, &can_msgq, &filter);
	zassert_equal(filter_id, -ENOSPC, "added more than max filters");

	for (i = 0; i < max; i++) {
		can_remove_rx_filter(can_dev, filter_ids[i]);
	}
}

/**
 * @brief Test max standard (11-bit) CAN RX filters.
 */
ZTEST_USER(can_classic, test_max_std_filters)
{
	add_remove_max_filters(false);
}

/**
 * @brief Test max extended (29-bit) CAN RX filters.
 */
ZTEST_USER(can_classic, test_max_ext_filters)
{
	add_remove_max_filters(true);
}

/**
 * @brief Test that no message is received when nothing was sent.
 */
ZTEST_USER(can_classic, test_receive_timeout)
{
	struct can_frame frame;
	int filter_id;
	int err;

	filter_id = add_rx_msgq(can_dev, &test_std_filter_1);

	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, -EAGAIN, "received a frame without sending one");

	can_remove_rx_filter(can_dev, filter_id);
}

/**
 * @brief Test that transmit callback function is called.
 */
ZTEST(can_classic, test_send_callback)
{
	int err;

	k_sem_reset(&tx_callback_sem);

	send_test_frame_nowait(can_dev, &test_std_frame_1, tx_std_callback_1);

	err = k_sem_take(&tx_callback_sem, TEST_SEND_TIMEOUT);
	zassert_equal(err, 0, "missing TX callback");
}

/**
 * @brief Test send/receive with standard (11-bit) CAN IDs.
 */
ZTEST(can_classic, test_send_receive_std_id)
{
	send_receive(&test_std_filter_1, &test_std_filter_2,
		     &test_std_frame_1, &test_std_frame_2);
}

/**
 * @brief Test send/receive with extended (29-bit) CAN IDs.
 */
ZTEST(can_classic, test_send_receive_ext_id)
{
	send_receive(&test_ext_filter_1, &test_ext_filter_2,
		     &test_ext_frame_1, &test_ext_frame_2);
}

/**
 * @brief Test send/receive with standard (11-bit) masked CAN IDs.
 */
ZTEST(can_classic, test_send_receive_std_id_masked)
{
	send_receive(&test_std_masked_filter_1, &test_std_masked_filter_2,
		     &test_std_frame_1, &test_std_frame_2);
}

/**
 * @brief Test send/receive with extended (29-bit) masked CAN IDs.
 */
ZTEST(can_classic, test_send_receive_ext_id_masked)
{
	send_receive(&test_ext_masked_filter_1, &test_ext_masked_filter_2,
		     &test_ext_frame_1, &test_ext_frame_2);
}

/**
 * @brief Test send/receive with messages buffered in a CAN message queue.
 */
ZTEST_USER(can_classic, test_send_receive_msgq)
{
	struct k_msgq_attrs attrs;
	struct can_frame frame;
	int filter_id;
	int nframes;
	int err;
	int i;

	filter_id = add_rx_msgq(can_dev, &test_std_filter_1);

	k_msgq_get_attrs(&can_msgq, &attrs);
	nframes = attrs.max_msgs;

	for (i = 0; i < nframes; i++) {
		send_test_frame(can_dev, &test_std_frame_1);
	}

	for (i = 0; i < nframes; i++) {
		err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
		zassert_equal(err, 0, "receive timeout");
		assert_frame_equal(&frame, &test_std_frame_1, 0);
	}

	for (i = 0; i < nframes; i++) {
		send_test_frame(can_dev, &test_std_frame_1);
	}

	for (i = 0; i < nframes; i++) {
		err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
		zassert_equal(err, 0, "receive timeout");
		assert_frame_equal(&frame, &test_std_frame_1, 0);
	}

	can_remove_rx_filter(can_dev, filter_id);
}

/**
 * @brief Test send/receive with standard (11-bit) CAN IDs and remote transmission request (RTR).
 */
ZTEST_USER(can_classic, test_send_receive_std_id_rtr)
{
	send_receive_rtr(&test_std_filter_1, &test_std_rtr_filter_1,
			 &test_std_frame_1, &test_std_rtr_frame_1);
}

/**
 * @brief Test send/receive with extended (29-bit) CAN IDs and remote transmission request (RTR).
 */
ZTEST_USER(can_classic, test_send_receive_ext_id_rtr)
{
	send_receive_rtr(&test_ext_filter_1, &test_ext_rtr_filter_1,
			 &test_ext_frame_1, &test_ext_rtr_frame_1);
}

/**
 * @brief Test that non-matching CAN frames do not pass a filter.
 */
ZTEST(can_classic, test_send_receive_wrong_id)
{
	struct can_frame frame_buffer;
	int filter_id;
	int err;

	filter_id = add_rx_msgq(can_dev, &test_std_filter_1);

	send_test_frame(can_dev, &test_std_frame_2);

	err = k_msgq_get(&can_msgq, &frame_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, -EAGAIN, "recevied a frame that should not pass the filter");

	can_remove_rx_filter(can_dev, filter_id);
}

/**
 * @brief Test that frames with invalid Data Length Code (DLC) are rejected.
 */
ZTEST_USER(can_classic, test_send_invalid_dlc)
{
	struct can_frame frame = {0};
	int err;

	frame.id = TEST_CAN_STD_ID_1;
	frame.dlc = CAN_MAX_DLC + 1;

	err = can_send(can_dev, &frame, TEST_SEND_TIMEOUT, NULL, NULL);
	zassert_equal(err, -EINVAL, "sent a frame with an invalid DLC");
}

/**
 * @brief Test that CAN-FD format frames are rejected in non-FD mode.
 */
ZTEST_USER(can_classic, test_send_fd_format)
{
	struct can_frame frame = {0};
	int err;

	frame.id = TEST_CAN_STD_ID_1;
	frame.dlc = 0;
	frame.flags = CAN_FRAME_FDF;

	err = can_send(can_dev, &frame, TEST_SEND_TIMEOUT, NULL, NULL);
	zassert_equal(err, -ENOTSUP, "sent a CAN-FD format frame in non-FD mode");
}

/**
 * @brief Test CAN controller bus recovery.
 */
ZTEST_USER(can_classic, test_recover)
{
	int err;

	/* It is not possible to provoke a bus off state, but test the API call */
	err = can_recover(can_dev, TEST_RECOVER_TIMEOUT);
	if (err == -ENOTSUP) {
		ztest_test_skip();
	}

	zassert_equal(err, 0, "failed to recover (err %d)", err);
}

/**
 * @brief Test retrieving the state of the CAN controller.
 */
ZTEST_USER(can_classic, test_get_state)
{
	struct can_bus_err_cnt err_cnt;
	enum can_state state;
	int err;

	err = can_get_state(can_dev, NULL, NULL);
	zassert_equal(err, 0, "failed to get CAN state without destinations (err %d)", err);

	err = can_get_state(can_dev, &state, NULL);
	zassert_equal(err, 0, "failed to get CAN state (err %d)", err);

	err = can_get_state(can_dev, NULL, &err_cnt);
	zassert_equal(err, 0, "failed to get CAN error counters (err %d)", err);

	err = can_get_state(can_dev, &state, &err_cnt);
	zassert_equal(err, 0, "failed to get CAN state + error counters (err %d)", err);
}

/**
 * @brief Test that CAN RX filters are preserved through CAN controller mode changes.
 */
ZTEST_USER(can_classic, test_filters_preserved_through_mode_change)
{
	struct can_frame frame;
	enum can_state state;
	int filter_id;
	int err;

	filter_id = add_rx_msgq(can_dev, &test_std_filter_1);
	send_test_frame(can_dev, &test_std_frame_1);

	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");
	assert_frame_equal(&frame, &test_std_frame_1, 0);

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_get_state(can_dev, &state, NULL);
	zassert_equal(err, 0, "failed to get CAN state (err %d)", err);
	zassert_equal(state, CAN_STATE_STOPPED, "CAN controller not stopped");

	err = can_set_mode(can_dev, CAN_MODE_NORMAL);
	zassert_equal(err, 0, "failed to set normal mode (err %d)", err);

	err = can_set_mode(can_dev, CAN_MODE_LOOPBACK);
	zassert_equal(err, 0, "failed to set loopback-mode (err %d)", err);

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);

	send_test_frame(can_dev, &test_std_frame_1);

	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");
	assert_frame_equal(&frame, &test_std_frame_1, 0);

	can_remove_rx_filter(can_dev, filter_id);
}

/**
 * @brief Test that CAN RX filters are preserved through CAN controller bitrate changes.
 */
ZTEST_USER(can_classic, test_filters_preserved_through_bitrate_change)
{
	struct can_frame frame;
	enum can_state state;
	int filter_id;
	int err;

	filter_id = add_rx_msgq(can_dev, &test_std_filter_1);
	send_test_frame(can_dev, &test_std_frame_1);

	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");
	assert_frame_equal(&frame, &test_std_frame_1, 0);

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_get_state(can_dev, &state, NULL);
	zassert_equal(err, 0, "failed to get CAN state (err %d)", err);
	zassert_equal(state, CAN_STATE_STOPPED, "CAN controller not stopped");

	err = can_set_bitrate(can_dev, TEST_BITRATE_2);
	zassert_equal(err, 0, "failed to set bitrate");

	err = can_set_bitrate(can_dev, TEST_BITRATE_1);
	zassert_equal(err, 0, "failed to set bitrate");

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);

	send_test_frame(can_dev, &test_std_frame_1);

	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");
	assert_frame_equal(&frame, &test_std_frame_1, 0);

	can_remove_rx_filter(can_dev, filter_id);
}

/**
 * @brief Test that CAN RX filters can be added while CAN controller is stopped.
 */
ZTEST_USER(can_classic, test_filters_added_while_stopped)
{
	struct can_frame frame;
	int filter_id;
	int err;

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	filter_id = add_rx_msgq(can_dev, &test_std_filter_1);

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);

	send_test_frame(can_dev, &test_std_frame_1);

	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");
	assert_frame_equal(&frame, &test_std_frame_1, 0);

	can_remove_rx_filter(can_dev, filter_id);
}

/**
 * @brief Test stopping is not allowed while stopped.
 */
ZTEST_USER(can_classic, test_stop_while_stopped)
{
	int err;

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_stop(can_dev);
	zassert_not_equal(err, 0, "stopped CAN controller while stopped");
	zassert_equal(err, -EALREADY, "wrong error return code (err %d)", err);

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test starting is not allowed while started.
 */
ZTEST_USER(can_classic, test_start_while_started)
{
	int err;

	err = can_start(can_dev);
	zassert_not_equal(err, 0, "started CAN controller while started");
	zassert_equal(err, -EALREADY, "wrong error return code (err %d)", err);
}

/**
 * @brief Test recover is not allowed while started.
 */
ZTEST_USER(can_classic, test_recover_while_stopped)
{
	int err;

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_recover(can_dev, K_NO_WAIT);
	zassert_not_equal(err, 0, "recovered bus while stopped");
	zassert_equal(err, -ENETDOWN, "wrong error return code (err %d)", err);

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test sending is not allowed while stopped.
 */
ZTEST_USER(can_classic, test_send_while_stopped)
{
	int err;

	err = can_stop(can_dev);
	zassert_equal(err, 0, "failed to stop CAN controller (err %d)", err);

	err = can_send(can_dev, &test_std_frame_1, TEST_SEND_TIMEOUT, NULL, NULL);
	zassert_not_equal(err, 0, "sent a frame in stopped state");
	zassert_equal(err, -ENETDOWN, "wrong error return code (err %d)", err);

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}

/**
 * @brief Test setting bitrate is not allowed while started.
 */
ZTEST_USER(can_classic, test_set_bitrate_while_started)
{
	int err;

	err = can_set_bitrate(can_dev, TEST_BITRATE_2);
	zassert_not_equal(err, 0, "changed bitrate while started");
	zassert_equal(err, -EBUSY, "wrong error return code (err %d)", err);
}

/**
 * @brief Test setting timing is not allowed while started.
 */
ZTEST_USER(can_classic, test_set_timing_while_started)
{
	struct can_timing timing;
	int err;

	timing.sjw = CAN_SJW_NO_CHANGE;

	err = can_calc_timing(can_dev, &timing, TEST_BITRATE_1, TEST_SAMPLE_POINT);
	zassert_ok(err, "failed to calculate timing (err %d)", err);

	err = can_set_timing(can_dev, &timing);
	zassert_not_equal(err, 0, "changed timing while started");
	zassert_equal(err, -EBUSY, "wrong error return code (err %d)", err);
}

/**
 * @brief Test setting mode is not allowed while started.
 */
ZTEST_USER(can_classic, test_set_mode_while_started)
{
	int err;

	err = can_set_mode(can_dev, CAN_MODE_NORMAL);
	zassert_not_equal(err, 0, "changed mode while started");
	zassert_equal(err, -EBUSY, "wrong error return code (err %d)", err);
}

void *can_classic_setup(void)
{
	int err;

	k_sem_init(&rx_callback_sem, 0, 2);
	k_sem_init(&tx_callback_sem, 0, 2);

	k_object_access_grant(&can_msgq, k_current_get());
	k_object_access_grant(can_dev, k_current_get());

	zassert_true(device_is_ready(can_dev), "CAN device not ready");

	(void)can_stop(can_dev);

	err = can_set_mode(can_dev, CAN_MODE_LOOPBACK);
	zassert_equal(err, 0, "failed to set loopback mode (err %d)", err);

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);

	return NULL;
}

ZTEST_SUITE(can_classic, NULL, can_classic_setup, NULL, NULL, NULL);
