/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/can.h>
#include <ztest.h>
#include <strings.h>

/*
 * @addtogroup t_can_driver
 * @{
 * @defgroup t_can_basic test_basic_can
 * @brief TestPurpose: verify can driver works
 * @details
 * - Test Steps
 *   -# Set driver to loopback mode
 *   -# Try to send a message.
 *   -# Attach a filter from every kind
 *   -# Test if message reception is really blocking
 *   -# Send and receive a message with standard id without masking
 *   -# Send and receive a message with extended id without masking
 *   -# Send and receive a message with standard id with masking
 *   -# Send and receive a message with extended id with masking
 *   -# Send and message with different id that should not be received.
 * - Expected Results
 *   -# All tests MUST pass
 * @}
 */

#define TEST_SEND_TIMEOUT    K_MSEC(100)
#define TEST_RECEIVE_TIMEOUT K_MSEC(100)

#define TEST_CAN_STD_ID_1      0x555
#define TEST_CAN_STD_ID_2      0x556
#define TEST_CAN_STD_MASK_ID_1 0x55A
#define TEST_CAN_STD_MASK_ID_2 0x56A
#define TEST_CAN_STD_MASK    0x7F0
#define TEST_CAN_SOME_STD_ID 0x123

#define TEST_CAN_EXT_ID_1      0x15555555
#define TEST_CAN_EXT_ID_2      0x15555556
#define TEST_CAN_EXT_MASK_ID_1 0x1555555A
#define TEST_CAN_EXT_MASK_ID_2 0x1555556A
#define TEST_CAN_EXT_MASK      0x1FFFFFF0


#if defined(CONFIG_CAN_LOOPBACK_DEV_NAME)
#define CAN_DEVICE_NAME CONFIG_CAN_LOOPBACK_DEV_NAME
#else
#define CAN_DEVICE_NAME DT_CHOSEN_ZEPHYR_CAN_PRIMARY_LABEL
#endif

CAN_DEFINE_MSGQ(can_msgq, 5);
struct k_sem rx_isr_sem;
struct k_sem rx_cb_sem;
struct k_sem tx_cb_sem;
const struct device *can_dev;

const struct zcan_frame test_std_msg_1 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.id      = TEST_CAN_STD_ID_1,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

const struct zcan_frame test_std_msg_2 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.id      = TEST_CAN_STD_ID_2,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

const struct zcan_frame test_ext_msg_1 = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.id      = TEST_CAN_EXT_ID_1,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

const struct zcan_frame test_ext_msg_2 = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.id      = TEST_CAN_EXT_ID_2,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

const struct zcan_filter test_std_filter_1 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_STD_ID_1,
	.rtr_mask = 1,
	.id_mask = CAN_STD_ID_MASK
};

const struct zcan_filter test_std_filter_2 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_STD_ID_2,
	.rtr_mask = 1,
	.id_mask = CAN_STD_ID_MASK
};

const struct zcan_filter test_std_masked_filter_1 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_STD_MASK_ID_1,
	.rtr_mask = 1,
	.id_mask = TEST_CAN_STD_MASK
};

const struct zcan_filter test_std_masked_filter_2 = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_STD_MASK_ID_2,
	.rtr_mask = 1,
	.id_mask = TEST_CAN_STD_MASK
};


const struct zcan_filter test_ext_filter_1 = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_EXT_ID_1,
	.rtr_mask = 1,
	.id_mask = CAN_EXT_ID_MASK
};

const struct zcan_filter test_ext_filter_2 = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_EXT_ID_2,
	.rtr_mask = 1,
	.id_mask = CAN_EXT_ID_MASK
};

const struct zcan_filter test_ext_masked_filter_1 = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_EXT_MASK_ID_1,
	.rtr_mask = 1,
	.id_mask = TEST_CAN_EXT_MASK
};

const struct zcan_filter test_ext_masked_filter_2 = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_EXT_ID_1,
	.rtr_mask = 1,
	.id_mask = TEST_CAN_EXT_MASK
};

const struct zcan_filter test_std_some_filter = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.id = TEST_CAN_SOME_STD_ID,
	.rtr_mask = 1,
	.id_mask = CAN_STD_ID_MASK
};

struct zcan_work can_work_1;
struct zcan_work can_work_2;

static inline void check_msg(const struct zcan_frame *msg1,
			     const struct zcan_frame *msg2,
			     uint32_t mask)
{
	int cmp_res;
	zassert_equal(msg1->id_type, msg2->id_type,
		      "ID type does not match");

	zassert_equal(msg1->rtr, msg2->rtr,
		      "RTR bit does not match");

	zassert_equal(msg1->id | mask, msg2->id | mask,
		      "ID does not match");

	zassert_equal(msg1->dlc, msg2->dlc,
		      "DLC does not match");
	cmp_res = memcmp(msg1->data, msg2->data, msg1->dlc);
	zassert_equal(cmp_res, 0, "Received data differ");
}

static void tx_std_isr_1(int error, void *arg)
{
	const struct zcan_frame *msg = (const struct zcan_frame *)arg;

	k_sem_give(&tx_cb_sem);

	zassert_equal(msg->id, TEST_CAN_STD_ID_1, "Arg does not match");
}

static void tx_std_isr_2(int error, void *arg)
{
	const struct zcan_frame *msg = (const struct zcan_frame *)arg;

	k_sem_give(&tx_cb_sem);

	zassert_equal(msg->id, TEST_CAN_STD_ID_2, "Arg does not match");
}

static void tx_ext_isr_1(int error, void *arg)
{
	const struct zcan_frame *msg = (const struct zcan_frame *)arg;

	k_sem_give(&tx_cb_sem);

	zassert_equal(msg->id, TEST_CAN_EXT_ID_1, "Arg does not match");
}

static void tx_ext_isr_2(int error, void *arg)
{
	const struct zcan_frame *msg = (const struct zcan_frame *)arg;

	k_sem_give(&tx_cb_sem);

	zassert_equal(msg->id, TEST_CAN_EXT_ID_2, "Arg does not match");
}

static void rx_std_isr_1(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_1, 0);
	zassert_equal_ptr(arg, &test_std_filter_1, "arg does not match");
	k_sem_give(&rx_isr_sem);
}

static void rx_std_isr_2(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_2, 0);
	zassert_equal_ptr(arg, &test_std_filter_2, "arg does not match");
	k_sem_give(&rx_isr_sem);
}

static void rx_std_mask_isr_1(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_1, 0x0F);
	zassert_equal_ptr(arg, &test_std_masked_filter_1, "arg does not match");
	k_sem_give(&rx_isr_sem);
}

static void rx_std_mask_isr_2(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_2, 0x0F);
	zassert_equal_ptr(arg, &test_std_masked_filter_2, "arg does not match");
	k_sem_give(&rx_isr_sem);
}

static void rx_ext_isr_1(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_ext_msg_1, 0);
	zassert_equal_ptr(arg, &test_ext_filter_1, "arg does not match");
	k_sem_give(&rx_isr_sem);
}

static void rx_ext_isr_2(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_ext_msg_2, 0);
	zassert_equal_ptr(arg, &test_ext_filter_2, "arg does not match");
	k_sem_give(&rx_isr_sem);
}

static void rx_ext_mask_isr_1(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_ext_msg_1, 0x0F);
	zassert_equal_ptr(arg, &test_ext_masked_filter_1, "arg does not match");
	k_sem_give(&rx_isr_sem);
}

static void rx_ext_mask_isr_2(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_ext_msg_2, 0x0F);
	zassert_equal_ptr(arg, &test_ext_masked_filter_2, "arg does not match");
	k_sem_give(&rx_isr_sem);
}

static void rx_std_cb_1(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_1, 0);
	zassert_equal_ptr(arg, &test_std_filter_1, "arg does not match");
	k_sem_give(&rx_cb_sem);
}

static void rx_std_cb_2(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_2, 0);
	zassert_equal_ptr(arg, &test_std_filter_2, "arg does not match");
	k_sem_give(&rx_cb_sem);
}

static void rx_std_mask_cb_1(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_1, 0x0F);
	zassert_equal_ptr(arg, &test_std_masked_filter_1, "arg does not match");
	k_sem_give(&rx_cb_sem);
}

static void rx_std_mask_cb_2(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_2, 0x0F);
	zassert_equal_ptr(arg, &test_std_masked_filter_2, "arg does not match");
	k_sem_give(&rx_cb_sem);
}

static void rx_ext_cb_1(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_ext_msg_1, 0);
	zassert_equal_ptr(arg, &test_ext_filter_1, "arg does not match");
	k_sem_give(&rx_cb_sem);
}

static void rx_ext_cb_2(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_ext_msg_2, 0);
	zassert_equal_ptr(arg, &test_ext_filter_2, "arg does not match");
	k_sem_give(&rx_cb_sem);
}

static void rx_ext_mask_cb_1(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_ext_msg_1, 0x0F);
	zassert_equal_ptr(arg, &test_ext_masked_filter_1, "arg does not match");
	k_sem_give(&rx_cb_sem);
}

static void rx_ext_mask_cb_2(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_ext_msg_2, 0x0F);
	zassert_equal_ptr(arg, &test_ext_masked_filter_2, "arg does not match");
	k_sem_give(&rx_cb_sem);
}

static void send_test_msg(const struct device *can_dev,
			  const struct zcan_frame *msg)
{
	int ret;

	ret = can_send(can_dev, msg, TEST_SEND_TIMEOUT, NULL, NULL);
	zassert_not_equal(ret, CAN_TX_ARB_LOST,
			  "Arbitration though in loopback mode");
	zassert_equal(ret, CAN_TX_OK, "Can't send a message. Err: %d", ret);
}

static void send_test_msg_nowait(const struct device *can_dev,
				 const struct zcan_frame *msg,
				 can_tx_callback_t cb)
{
	int ret;
	ret = can_send(can_dev, msg, TEST_SEND_TIMEOUT, cb,
			(struct zcan_frame *)msg);
	zassert_not_equal(ret, CAN_TX_ARB_LOST,
			  "Arbitration though in loopback mode");
	zassert_equal(ret, CAN_TX_OK, "Can't send a message. Err: %d", ret);
}

static inline int attach_msgq(const struct device *can_dev,
			      const struct zcan_filter *filter)
{
	int filter_id;

	filter_id = can_attach_msgq(can_dev, &can_msgq, filter);
	zassert_not_equal(filter_id, CAN_NO_FREE_FILTER,
			  "Filter full even for a single one");
	zassert_true((filter_id >= 0), "Negative filter number");

	return filter_id;
}

static inline int attach_workq(const struct device *can_dev,
			      const struct zcan_filter *filter,
			      struct zcan_work *work,
			      can_rx_callback_t cb)
{
	int filter_id;

	filter_id  = can_attach_workq(can_dev, &k_sys_work_q, work, cb,
				     (void *)filter, filter);

	zassert_not_equal(filter_id, CAN_NO_FREE_FILTER,
			  "Filter full even for a single one");
	zassert_true((filter_id >= 0), "Negative filter number");

	return filter_id;
}

static inline int attach_isr(const struct device *can_dev,
			     const struct zcan_filter *filter,
			     can_rx_callback_t isr)
{
	int filter_id;

	k_sem_reset(&rx_isr_sem);

	filter_id = can_attach_isr(can_dev, isr, (void *)filter, filter);
	zassert_not_equal(filter_id, CAN_NO_FREE_FILTER,
			  "Filter full even for a single one");
	zassert_true((filter_id >= 0), "Negative filter number");

	return filter_id;
}

static void send_receive(const struct zcan_filter *filter1,
			 const struct zcan_filter *filter2,
			 const struct zcan_frame *msg1,
			 const struct zcan_frame *msg2)
{
	int ret, filter_id_1, filter_id_2;
	struct zcan_frame msg_buffer;
	uint32_t mask = 0U;

	zassert_not_null(can_dev, "Device not not found");

	filter_id_1 = attach_msgq(can_dev, filter1);
	send_test_msg(can_dev, msg1);
	ret = k_msgq_get(&can_msgq, &msg_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, 0, "Receiving timeout");

	if (filter1->id_type == CAN_STANDARD_IDENTIFIER) {
		if (filter1->id_mask != CAN_STD_ID_MASK) {
			mask = 0x0F;
		}
	} else {
		if (filter1->id_mask != CAN_EXT_ID_MASK) {
			mask = 0x0F;
		}
	}

	check_msg(&msg_buffer, msg1, mask);
	can_detach(can_dev, filter_id_1);

	k_sem_reset(&tx_cb_sem);
	if (msg1->id_type == CAN_STANDARD_IDENTIFIER) {
		if (filter1->id_mask == CAN_STD_ID_MASK) {
			filter_id_1 = attach_isr(can_dev, filter1,
						 rx_std_isr_1);
			filter_id_2 = attach_isr(can_dev, filter2,
						 rx_std_isr_2);
			send_test_msg_nowait(can_dev, msg1, tx_std_isr_1);
			send_test_msg_nowait(can_dev, msg2, tx_std_isr_2);
		} else {
			filter_id_1 = attach_isr(can_dev, filter1,
						 rx_std_mask_isr_1);
			filter_id_2 = attach_isr(can_dev, filter2,
						 rx_std_mask_isr_2);
			send_test_msg_nowait(can_dev, msg1, tx_std_isr_1);
			send_test_msg_nowait(can_dev, msg2, tx_std_isr_2);
		}
	} else {
		if (filter1->id_mask == CAN_EXT_ID_MASK) {
			filter_id_1 = attach_isr(can_dev, filter1,
						 rx_ext_isr_1);
			filter_id_2 = attach_isr(can_dev, filter2,
						 rx_ext_isr_2);
			send_test_msg_nowait(can_dev, msg1, tx_ext_isr_1);
			send_test_msg_nowait(can_dev, msg2, tx_ext_isr_2);
		} else {
			filter_id_1 = attach_isr(can_dev, filter1,
						 rx_ext_mask_isr_1);
			filter_id_2 = attach_isr(can_dev, filter2,
						 rx_ext_mask_isr_2);
			send_test_msg_nowait(can_dev, msg1, tx_ext_isr_1);
			send_test_msg_nowait(can_dev, msg2, tx_ext_isr_2);
		}
	}

	ret = k_sem_take(&rx_isr_sem, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, 0, "Receiving timeout");
	ret = k_sem_take(&rx_isr_sem, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, 0, "Receiving timeout");
	ret = k_sem_take(&tx_cb_sem, TEST_SEND_TIMEOUT);
	zassert_equal(ret, 0, "Missing TX callback");
	ret = k_sem_take(&tx_cb_sem, TEST_SEND_TIMEOUT);
	zassert_equal(ret, 0, "Missing TX callback");
	can_detach(can_dev, filter_id_1);
	can_detach(can_dev, filter_id_2);

	if (msg1->id_type == CAN_STANDARD_IDENTIFIER) {
		if (filter1->id_mask == CAN_STD_ID_MASK) {
			filter_id_1 = attach_workq(can_dev, filter1,
						   &can_work_1, rx_std_cb_1);
			filter_id_2 = attach_workq(can_dev, filter2,
						   &can_work_2, rx_std_cb_2);
		} else {
			filter_id_1 = attach_workq(can_dev, filter1,
						   &can_work_1,
						   rx_std_mask_cb_1);
			filter_id_2 = attach_workq(can_dev, filter2,
						   &can_work_2,
						   rx_std_mask_cb_2);
		}
	} else {
		if (filter1->id_mask == CAN_EXT_ID_MASK) {
			filter_id_1 = attach_workq(can_dev, filter1,
						   &can_work_1, rx_ext_cb_1);
			filter_id_2 = attach_workq(can_dev, filter2,
						   &can_work_2, rx_ext_cb_2);
		} else {
			filter_id_1 = attach_workq(can_dev, filter1,
						    &can_work_1,
						    rx_ext_mask_cb_1);
			filter_id_2 = attach_workq(can_dev, filter2,
						   &can_work_2,
						   rx_ext_mask_cb_2);
		}
	}

	send_test_msg(can_dev, msg1);
	send_test_msg(can_dev, msg2);
	ret = k_sem_take(&rx_cb_sem, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, 0, "Receiving timeout");
	ret = k_sem_take(&rx_cb_sem, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, 0, "Receiving timeout");
	can_detach(can_dev, filter_id_2);
	can_detach(can_dev, filter_id_1);
}

/*
 * Set driver to loopback mode
 * The driver stays in loopback mode after that.
 * The controller can now be tested against itself
 */
static void test_set_loopback(void)
{
	int ret;

	ret = can_set_mode(can_dev, CAN_LOOPBACK_MODE);
	zassert_equal(ret, 0, "Can't set loopback-mode. Err: %d", ret);
}

/*
 * Sending a message to the wild should work because we are in loopback mode
 * and therfor ACK the frame ourselves
 */
static void test_send_and_forget(void)
{
	zassert_not_null(can_dev, "Device not not found");

	send_test_msg(can_dev, &test_std_msg_1);
}

/*
 * Test very basic filter attachment
 * Test each type but only one filter at a time
 */
static void test_filter_attach(void)
{
	int filter_id;

	filter_id = attach_isr(can_dev, &test_std_filter_1, rx_std_isr_1);
	can_detach(can_dev, filter_id);

	filter_id = attach_isr(can_dev, &test_ext_filter_1, rx_ext_isr_1);
	can_detach(can_dev, filter_id);

	filter_id = attach_msgq(can_dev, &test_std_filter_1);
	can_detach(can_dev, filter_id);

	filter_id = attach_msgq(can_dev, &test_ext_filter_1);
	can_detach(can_dev, filter_id);

	filter_id = attach_isr(can_dev, &test_std_masked_filter_1,
			       rx_std_mask_isr_1);
	can_detach(can_dev, filter_id);

	filter_id = attach_isr(can_dev, &test_ext_masked_filter_1,
			       rx_ext_mask_isr_1);
	can_detach(can_dev, filter_id);

	filter_id = attach_workq(can_dev, &test_std_filter_1, &can_work_1,
				 rx_std_cb_1);
	can_detach(can_dev, filter_id);

	filter_id = attach_workq(can_dev, &test_ext_filter_1, &can_work_2,
				 rx_ext_cb_1);
	can_detach(can_dev, filter_id);
}

/*
 * Test if a message is received wile nothing was sent.
 */
static void test_receive_timeout(void)
{
	int ret, filter_id;
	struct zcan_frame msg;

	filter_id = attach_msgq(can_dev, &test_std_filter_1);

	ret = k_msgq_get(&can_msgq, &msg, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, -EAGAIN, "Got a message without sending it");

	can_detach(can_dev, filter_id);
}

/*
 * Test if the callback function is called
 */
static void test_send_callback(void)
{
	int ret;

	k_sem_reset(&tx_cb_sem);

	send_test_msg_nowait(can_dev, &test_std_msg_1, tx_std_isr_1);

	ret = k_sem_take(&tx_cb_sem, TEST_SEND_TIMEOUT);
	zassert_equal(ret, 0, "Missing TX callback");
}

/*
 * Attach to a filter that should pass the message and send the message.
 * The massage should be received within a small timeout.
 * Standard identifier
 */
void test_send_receive_std(void)
{
	send_receive(&test_std_filter_1, &test_std_filter_2,
		     &test_std_msg_1, &test_std_msg_2);
}

/*
 * Attach to a filter that should pass the message and send the message.
 * The massage should be received within a small timeout
 * Extended identifier
 */
void test_send_receive_ext(void)
{
	send_receive(&test_ext_filter_1, &test_ext_filter_2,
		     &test_ext_msg_1, &test_ext_msg_2);
}

/*
 * Attach to a filter that should pass the message and send the message.
 * The massage should be received within a small timeout.
 * The message ID is slightly different to the filter but should still
 * because of the mask settind in the filter.
 * Standard identifier
 */
void test_send_receive_std_masked(void)
{
	send_receive(&test_std_masked_filter_1, &test_std_masked_filter_2,
		     &test_std_msg_1, &test_std_msg_2);
}

/*
 * Attach to a filter that should pass the message and send the message.
 * The massage should be received within a small timeout.
 * The message ID is slightly different to the filter but should still
 * because of the mask settind in the filter.
 * Extended identifier
 */
void test_send_receive_ext_masked(void)
{
	send_receive(&test_ext_masked_filter_1, &test_ext_masked_filter_2,
		     &test_ext_msg_1, &test_ext_msg_2);
}

/*
 * Attach to a filter that should pass the message and send multiple messages.
 * The massage should be received and buffered within a small timeout.
 * Extended identifier
 */
void test_send_receive_buffer(void)
{
	int filter_id, i, ret;

	filter_id = attach_workq(can_dev, &test_std_filter_1, &can_work_1,
				 rx_std_cb_1);
	k_sem_reset(&rx_cb_sem);

	for (i = 0; i < CONFIG_CAN_WORKQ_FRAMES_BUF_CNT; i++) {
		send_test_msg(can_dev, &test_std_msg_1);
	}

	for (i = 0; i < CONFIG_CAN_WORKQ_FRAMES_BUF_CNT; i++) {
		ret = k_sem_take(&rx_cb_sem, TEST_RECEIVE_TIMEOUT);
		zassert_equal(ret, 0, "Receiving timeout");
	}

	for (i = 0; i < CONFIG_CAN_WORKQ_FRAMES_BUF_CNT; i++) {
		send_test_msg(can_dev, &test_std_msg_1);
	}

	for (i = 0; i < CONFIG_CAN_WORKQ_FRAMES_BUF_CNT; i++) {
		ret = k_sem_take(&rx_cb_sem, TEST_RECEIVE_TIMEOUT);
		zassert_equal(ret, 0, "Receiving timeout");
	}

	can_detach(can_dev, filter_id);
}

/*
 * Attach to a filter that should not pass the message and send a message
 * with a different id.
 * The massage should not be received.
 */
static void test_send_receive_wrong_id(void)
{
	int ret, filter_id;
	struct zcan_frame msg_buffer;

	filter_id = attach_msgq(can_dev, &test_std_filter_1);

	send_test_msg(can_dev, &test_std_msg_2);

	ret = k_msgq_get(&can_msgq, &msg_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, -EAGAIN,
		      "Got a message that should not pass the filter");

	can_detach(can_dev, filter_id);
}

/*
 * Check if a call with dlc > CAN_MAX_DLC returns CAN_TX_EINVAL
 */
static void test_send_invalid_dlc(void)
{
	struct zcan_frame frame;
	int ret;

	frame.dlc = CAN_MAX_DLC + 1;

	ret = can_send(can_dev, &frame, TEST_SEND_TIMEOUT, tx_std_isr_1, NULL);
	zassert_equal(ret, CAN_TX_EINVAL,
		      "ret [%d] not equal to %d", ret, CAN_TX_EINVAL);
}

void test_main(void)
{
	k_sem_init(&rx_isr_sem, 0, 2);
	k_sem_init(&rx_cb_sem, 0, INT_MAX);
	k_sem_init(&tx_cb_sem, 0, 2);
	can_dev = device_get_binding(CAN_DEVICE_NAME);
	zassert_not_null(can_dev, "Device not found");

	ztest_test_suite(can_driver,
			 ztest_unit_test(test_set_loopback),
			 ztest_unit_test(test_send_and_forget),
			 ztest_unit_test(test_filter_attach),
			 ztest_unit_test(test_receive_timeout),
			 ztest_unit_test(test_send_callback),
			 ztest_unit_test(test_send_receive_std),
			 ztest_unit_test(test_send_invalid_dlc),
			 ztest_unit_test(test_send_receive_ext),
			 ztest_unit_test(test_send_receive_std_masked),
			 ztest_unit_test(test_send_receive_ext_masked),
			 ztest_unit_test(test_send_receive_buffer),
			 ztest_unit_test(test_send_receive_wrong_id));
	ztest_run_test_suite(can_driver);
}
