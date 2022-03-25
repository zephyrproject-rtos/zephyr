/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/can.h>
#include <ztest.h>

/**
 * @addtogroup t_can_driver
 * @{
 * @defgroup t_can_api test_can_api
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
#define TEST_CAN_STD_ID_1      0x555
#define TEST_CAN_STD_ID_2      0x556
#define TEST_CAN_STD_MASK_ID_1 0x55A
#define TEST_CAN_STD_MASK_ID_2 0x56A
#define TEST_CAN_STD_MASK      0x7F0
#define TEST_CAN_SOME_STD_ID   0x123

/**
 * @brief Extended (29-bit) CAN IDs and masks used for testing.
 */
#define TEST_CAN_EXT_ID_1      0x15555555
#define TEST_CAN_EXT_ID_2      0x15555556
#define TEST_CAN_EXT_MASK_ID_1 0x1555555A
#define TEST_CAN_EXT_MASK_ID_2 0x1555556A
#define TEST_CAN_EXT_MASK      0x1FFFFFF0

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
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

/**
 * @brief Standard (11-bit) CAN ID frame 2.
 */
const struct zcan_frame test_std_frame_2 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.id      = TEST_CAN_STD_ID_2,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

/**
 * @brief Extended (29-bit) CAN ID frame 1.
 */
const struct zcan_frame test_ext_frame_1 = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.id      = TEST_CAN_EXT_ID_1,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

/**
 * @brief Extended (29-bit) CAN ID frame 1.
 */
const struct zcan_frame test_ext_frame_2 = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.id      = TEST_CAN_EXT_ID_2,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

/**
 * @brief Standard (11-bit) CAN ID filter 1. This filter matches
 * ``test_std_frame_1``.
 */
const struct zcan_filter test_std_filter_1 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_STD_ID_1,
	.rtr_mask = 1,
	.id_mask = CAN_STD_ID_MASK
};

/**
 * @brief Standard (11-bit) CAN ID filter 2. This filter matches
 * ``test_std_frame_2``.
 */
const struct zcan_filter test_std_filter_2 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_STD_ID_2,
	.rtr_mask = 1,
	.id_mask = CAN_STD_ID_MASK
};

/**
 * @brief Standard (11-bit) CAN ID masked filter 1. This filter matches
 * ``test_std_frame_1``.
 */
const struct zcan_filter test_std_masked_filter_1 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_STD_MASK_ID_1,
	.rtr_mask = 1,
	.id_mask = TEST_CAN_STD_MASK
};

/**
 * @brief Standard (11-bit) CAN ID masked filter 2. This filter matches
 * ``test_std_frame_2``.
 */
const struct zcan_filter test_std_masked_filter_2 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_STD_MASK_ID_2,
	.rtr_mask = 1,
	.id_mask = TEST_CAN_STD_MASK
};

/**
 * @brief Extended (29-bit) CAN ID filter 1. This filter matches
 * ``test_ext_frame_1``.
 */
const struct zcan_filter test_ext_filter_1 = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_EXT_ID_1,
	.rtr_mask = 1,
	.id_mask = CAN_EXT_ID_MASK
};

/**
 * @brief Extended (29-bit) CAN ID filter 2. This filter matches
 * ``test_ext_frame_2``.
 */
const struct zcan_filter test_ext_filter_2 = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_EXT_ID_2,
	.rtr_mask = 1,
	.id_mask = CAN_EXT_ID_MASK
};

/**
 * @brief Extended (29-bit) CAN ID masked filter 1. This filter matches
 * ``test_ext_frame_1``.
 */
const struct zcan_filter test_ext_masked_filter_1 = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_EXT_MASK_ID_1,
	.rtr_mask = 1,
	.id_mask = TEST_CAN_EXT_MASK
};

/**
 * @brief Extended (29-bit) CAN ID masked filter 2. This filter matches
 * ``test_ext_frame_2``.
 */
const struct zcan_filter test_ext_masked_filter_2 = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_EXT_ID_1,
	.rtr_mask = 1,
	.id_mask = TEST_CAN_EXT_MASK
};

/**
 * @brief Standard (11-bit) CAN ID filter. This filter matches
 * ``TEST_CAN_SOME_STD_ID``.
 */
const struct zcan_filter test_std_some_filter = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_SOME_STD_ID,
	.rtr_mask = 1,
	.id_mask = CAN_STD_ID_MASK
};

/**
 * @brief Assert that two CAN frames are equal given a CAN ID mask.
 *
 * @param frame1  First CAN frame.
 * @param frame2  Second CAN frame.
 * @param id_mask CAN ID mask.
 */
static inline void assert_frame_equal(const struct zcan_frame *frame1,
				      const struct zcan_frame *frame2,
				      uint32_t id_mask)
{
	zassert_equal(frame1->id_type, frame2->id_type, "ID type does not match");
	zassert_equal(frame1->rtr, frame2->rtr, "RTR bit does not match");
	zassert_equal(frame1->id | id_mask, frame2->id | id_mask, "ID does not match");
	zassert_equal(frame1->dlc, frame2->dlc, "DLC does not match");
	zassert_mem_equal(frame1->data, frame2->data, frame1->dlc, "Received data differ");
}

/**
 * @brief Standard (11-bit) CAN ID transmit callback 1.
 *
 * See @a can_tx_callback_t() for argument description.
 */
static void tx_std_callback_1(const struct device *dev, int error, void *user_data)
{
	const struct zcan_frame *frame = user_data;

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
	const struct zcan_frame *frame = user_data;

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
	const struct zcan_frame *frame = user_data;

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
	const struct zcan_frame *frame = user_data;

	k_sem_give(&tx_callback_sem);

	zassert_equal(dev, can_dev, "CAN device does not match");
	zassert_equal(frame->id, TEST_CAN_EXT_ID_2, "ID does not match");
}

/**
 * @brief Standard (11-bit) CAN ID receive callback 1.
 *
 * See @a can_rx_callback_t() for argument description.
 */
static void rx_std_callback_1(const struct device *dev, struct zcan_frame *frame,
			      void *user_data)
{
	struct zcan_filter *filter = user_data;

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
static void rx_std_callback_2(const struct device *dev, struct zcan_frame *frame,
			      void *user_data)
{
	struct zcan_filter *filter = user_data;

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
static void rx_std_mask_callback_1(const struct device *dev, struct zcan_frame *frame,
				   void *user_data)
{
	struct zcan_filter *filter = user_data;

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
static void rx_std_mask_callback_2(const struct device *dev, struct zcan_frame *frame,
				   void *user_data)
{
	struct zcan_filter *filter = user_data;

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
static void rx_ext_callback_1(const struct device *dev, struct zcan_frame *frame,
			      void *user_data)
{
	struct zcan_filter *filter = user_data;

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
static void rx_ext_callback_2(const struct device *dev, struct zcan_frame *frame,
			      void *user_data)
{
	struct zcan_filter *filter = user_data;

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
static void rx_ext_mask_callback_1(const struct device *dev, struct zcan_frame *frame,
				   void *user_data)
{
	struct zcan_filter *filter = user_data;

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
static void rx_ext_mask_callback_2(const struct device *dev, struct zcan_frame *frame,
				   void *user_data)
{
	struct zcan_filter *filter = user_data;

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
	uint32_t mask = 0U;
	int filter_id_1;
	int filter_id_2;
	int err;

	filter_id_1 = add_rx_msgq(can_dev, filter1);
	send_test_frame(can_dev, frame1);

	err = k_msgq_get(&can_msgq, &frame_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(err, 0, "receive timeout");

	if (filter1->id_type == CAN_STANDARD_IDENTIFIER) {
		if (filter1->id_mask != CAN_STD_ID_MASK) {
			mask = 0x0F;
		}
	} else {
		if (filter1->id_mask != CAN_EXT_ID_MASK) {
			mask = 0x0F;
		}
	}

	assert_frame_equal(&frame_buffer, frame1, mask);
	can_remove_rx_filter(can_dev, filter_id_1);

	k_sem_reset(&tx_callback_sem);

	if (frame1->id_type == CAN_STANDARD_IDENTIFIER) {
		if (filter1->id_mask == CAN_STD_ID_MASK) {
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
	} else {
		if (filter1->id_mask == CAN_EXT_ID_MASK) {
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
 * @brief Test configuring the CAN controller for loopback mode.
 *
 * This must be the first test case as it allows the other test cases to
 * send/receive their own frames.
 */
static void test_set_loopback(void)
{
	int err;

	err = can_set_mode(can_dev, CAN_LOOPBACK_MODE);
	zassert_equal(err, 0, "failed to set loopback-mode (err %d)", err);
}

/**
 * @brief Test sending a message with no filters installed.
 *
 * This basic test work since the CAN controller is in loopback mode and
 * therefore ACKs its own frame.
 */
static void test_send_and_forget(void)
{
	send_test_frame(can_dev, &test_std_frame_1);
}

/**
 * @brief Test adding basic filters.
 *
 * Test each filter type but only one filter at a time.
 */
static void test_add_filter(void)
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
 * @brief Test that no message is received when nothing was sent.
 */
static void test_receive_timeout(void)
{
	struct zcan_frame frame;
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
static void test_send_callback(void)
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
void test_send_receive_std_id(void)
{
	send_receive(&test_std_filter_1, &test_std_filter_2,
		     &test_std_frame_1, &test_std_frame_2);
}

/**
 * @brief Test send/receive with extended (29-bit) CAN IDs.
 */
void test_send_receive_ext_id(void)
{
	send_receive(&test_ext_filter_1, &test_ext_filter_2,
		     &test_ext_frame_1, &test_ext_frame_2);
}

/**
 * @brief Test send/receive with standard (11-bit) masked CAN IDs.
 */
void test_send_receive_std_id_masked(void)
{
	send_receive(&test_std_masked_filter_1, &test_std_masked_filter_2,
		     &test_std_frame_1, &test_std_frame_2);
}

/**
 * @brief Test send/receive with extended (29-bit) masked CAN IDs.
 */
void test_send_receive_ext_id_masked(void)
{
	send_receive(&test_ext_masked_filter_1, &test_ext_masked_filter_2,
		     &test_ext_frame_1, &test_ext_frame_2);
}

/**
 * @brief Test send/receive with messages buffered in a CAN message queue.
 */
void test_send_receive_msgq(void)
{
	struct k_msgq_attrs attrs;
	struct zcan_frame frame;
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
	}

	for (i = 0; i < nframes; i++) {
		send_test_frame(can_dev, &test_std_frame_1);
	}

	for (i = 0; i < nframes; i++) {
		err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
		zassert_equal(err, 0, "receive timeout");
	}

	can_remove_rx_filter(can_dev, filter_id);
}

/**
 * @brief Test that non-matching CAN frames do not pass a filter.
 */
static void test_send_receive_wrong_id(void)
{
	struct zcan_frame frame_buffer;
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
static void test_send_invalid_dlc(void)
{
	struct zcan_frame frame;
	int err;

	frame.dlc = CAN_MAX_DLC + 1;

	err = can_send(can_dev, &frame, TEST_SEND_TIMEOUT, tx_std_callback_1, NULL);
	zassert_equal(err, -EINVAL, "sent a frame with an invalid DLC");
}

void test_main(void)
{
	k_sem_init(&rx_callback_sem, 0, 2);
	k_sem_init(&tx_callback_sem, 0, 2);

	zassert_true(device_is_ready(can_dev), "CAN device not ready");

	ztest_test_suite(can_api_tests,
			 ztest_unit_test(test_set_loopback),
			 ztest_unit_test(test_send_and_forget),
			 ztest_unit_test(test_add_filter),
			 ztest_unit_test(test_receive_timeout),
			 ztest_unit_test(test_send_callback),
			 ztest_unit_test(test_send_receive_std_id),
			 ztest_unit_test(test_send_receive_ext_id),
			 ztest_unit_test(test_send_receive_std_id_masked),
			 ztest_unit_test(test_send_receive_ext_id_masked),
			 ztest_unit_test(test_send_receive_msgq),
			 ztest_unit_test(test_send_invalid_dlc),
			 ztest_unit_test(test_send_receive_wrong_id));
	ztest_run_test_suite(can_api_tests);
}
