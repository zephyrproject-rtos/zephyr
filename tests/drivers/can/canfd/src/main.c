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
 * @defgroup t_canfd test_canfd
 * @brief TestPurpose: Test CAN-FD
 * @details
 * - Test Steps
 *   -# Set driver to loopback mode
 *   -# Send and receive a message
 *   -# Send and receive a message with fd
 * - Expected Results
 *   -# All tests MUST pass
 * @}
 */

#define TEST_SEND_TIMEOUT    K_MSEC(100)
#define TEST_RECEIVE_TIMEOUT K_MSEC(100)

#define TEST_CAN_STD_ID_1      0x555
#define TEST_CAN_STD_ID_2      0x556


#if defined(CONFIG_CAN_LOOPBACK_DEV_NAME)
#define CAN_DEVICE_NAME CONFIG_CAN_LOOPBACK_DEV_NAME
#else
#define CAN_DEVICE_NAME DT_LABEL(DT_CHOSEN(zephyr_canbus))
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

const struct zcan_frame test_std_msg_fd_1 = {
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
		    61, 62, 63, 64}
};

const struct zcan_frame test_std_msg_fd_2 = {
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
		    61, 62, 63, 64}
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


struct zcan_work can_work_1;
struct zcan_work can_work_2;

static inline void check_msg(const struct zcan_frame *msg1,
			     const struct zcan_frame *msg2)
{
	int cmp_res;

	zassert_equal(msg1->id_type, msg2->id_type, "ID type does not match");

	zassert_equal(msg1->rtr, msg2->rtr, "RTR bit does not match");

	zassert_equal(msg1->id, msg2->id, "ID does not match");

	zassert_equal(msg1->dlc, msg2->dlc, "DLC does not match");

	cmp_res = memcmp(msg1->data, msg2->data, can_dlc_to_bytes(msg1->dlc));

	zassert_equal(cmp_res, 0, "Received data differ");
}

static void tx_std_isr_1(uint32_t error_flags, void *arg)
{
	const struct zcan_frame *msg = (const struct zcan_frame *)arg;

	k_sem_give(&tx_cb_sem);

	zassert_equal(msg->id, TEST_CAN_STD_ID_1, "Arg does not match");
}

static void tx_std_isr_2(uint32_t error_flags, void *arg)
{
	const struct zcan_frame *msg = (const struct zcan_frame *)arg;

	k_sem_give(&tx_cb_sem);

	zassert_equal(msg->id, TEST_CAN_STD_ID_2, "Arg does not match");
}

static void rx_std_isr_1(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_1);
	zassert_equal_ptr(arg, &test_std_filter_1, "arg does not match");
	k_sem_give(&rx_isr_sem);
}

static void rx_std_isr_2(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_2);
	zassert_equal_ptr(arg, &test_std_filter_2, "arg does not match");
	k_sem_give(&rx_isr_sem);
}

static void rx_std_isr_fd_1(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_fd_1);
	zassert_equal_ptr(arg, &test_std_filter_1, "arg does not match");
	k_sem_give(&rx_isr_sem);
}

static void rx_std_isr_fd_2(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_fd_2);
	zassert_equal_ptr(arg, &test_std_filter_2, "arg does not match");
	k_sem_give(&rx_isr_sem);
}

static void rx_std_cb_1(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_1);
	zassert_equal_ptr(arg, &test_std_filter_1, "arg does not match");
	k_sem_give(&rx_cb_sem);
}

static void rx_std_cb_2(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_2);
	zassert_equal_ptr(arg, &test_std_filter_2, "arg does not match");
	k_sem_give(&rx_cb_sem);
}

static void rx_std_fd_cb_1(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_fd_1);
	zassert_equal_ptr(arg, &test_std_filter_1, "arg does not match");
	k_sem_give(&rx_cb_sem);
}

static void rx_std_fd_cb_2(struct zcan_frame *msg, void *arg)
{
	check_msg(msg, &test_std_msg_fd_2);
	zassert_equal_ptr(arg, &test_std_filter_2, "arg does not match");
	k_sem_give(&rx_cb_sem);
}

static void send_test_msg(const struct device *can_dev,
			  const struct zcan_frame *msg)
{
	int ret;

	ret = can_send(can_dev, msg, TEST_SEND_TIMEOUT, NULL, NULL);
	zassert_not_equal(ret, -EBUSY,
			  "Arbitration though in loopback mode");
	zassert_equal(ret, 0, "Can't send a message. Err: %d", ret);
}

static void send_test_msg_nowait(const struct device *can_dev,
				 const struct zcan_frame *msg,
				 can_tx_callback_t cb)
{
	int ret;

	ret = can_send(can_dev, msg, TEST_SEND_TIMEOUT, cb,
			(struct zcan_frame *)msg);
	zassert_not_equal(ret, -EBUSY,
			  "Arbitration though in loopback mode");
	zassert_equal(ret, 0, "Can't send a message. Err: %d", ret);
}

static inline int attach_msgq(const struct device *can_dev,
			      const struct zcan_filter *filter)
{
	int filter_id;

	filter_id = can_attach_msgq(can_dev, &can_msgq, filter);
	zassert_not_equal(filter_id, -ENOSPC,
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

	zassert_not_equal(filter_id, -ENOSPC,
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
	zassert_not_equal(filter_id, -ENOSPC,
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

	zassert_not_null(can_dev, "Device not not found");

	filter_id_1 = attach_msgq(can_dev, filter1);
	send_test_msg(can_dev, msg1);
	ret = k_msgq_get(&can_msgq, &msg_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, 0, "Receiving timeout");

	check_msg(&msg_buffer, msg1);
	can_detach(can_dev, filter_id_1);

	k_sem_reset(&tx_cb_sem);

	if (msg1->fd) {
		filter_id_1 = attach_isr(can_dev, filter1, rx_std_isr_fd_1);
	} else {
		filter_id_1 = attach_isr(can_dev, filter1, rx_std_isr_1);
	}

	if (msg2->fd) {
		filter_id_2 = attach_isr(can_dev, filter2, rx_std_isr_fd_2);
	} else {
		filter_id_2 = attach_isr(can_dev, filter2, rx_std_isr_2);
	}

	send_test_msg_nowait(can_dev, msg1, tx_std_isr_1);
	send_test_msg_nowait(can_dev, msg2, tx_std_isr_2);

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

	if (msg1->fd) {
		filter_id_1 = attach_workq(can_dev, filter1, &can_work_1,
					   rx_std_fd_cb_1);
	} else {
		filter_id_1 = attach_workq(can_dev, filter1, &can_work_1,
					   rx_std_cb_1);
	}

	if (msg2->fd) {
		filter_id_2 = attach_workq(can_dev, filter2, &can_work_2,
					   rx_std_fd_cb_2);
	} else {
		filter_id_2 = attach_workq(can_dev, filter2, &can_work_2,
					   rx_std_cb_2);
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
 * Attach to a filter that should pass the message and send the message.
 * The massage should be received within a small timeout.
 * Standard identifier, classic frame
 */
void test_send_receive_std(void)
{
	send_receive(&test_std_filter_1, &test_std_filter_2,
		     &test_std_msg_1, &test_std_msg_2);
}

/*
 * Attach to a filter that should pass the message and send the message.
 * The massage should be received within a small timeout.
 * Standard identifier, fd frame
 */
void test_send_receive_fd(void)
{
	send_receive(&test_std_filter_1, &test_std_filter_2,
		     &test_std_msg_fd_1, &test_std_msg_fd_2);
}

/*
 * Attach to a filter that should pass the message and send the message.
 * The massage should be received within a small timeout.
 * Standard identifier, classic frame and fd
 */
void test_send_receive_mix(void)
{
	send_receive(&test_std_filter_1, &test_std_filter_2,
		     &test_std_msg_fd_1, &test_std_msg_2);
}

void test_main(void)
{
	k_sem_init(&rx_isr_sem, 0, 2);
	k_sem_init(&rx_cb_sem, 0, INT_MAX);
	k_sem_init(&tx_cb_sem, 0, 2);
	can_dev = device_get_binding(CAN_DEVICE_NAME);
	zassert_not_null(can_dev, "Device not found");

	ztest_test_suite(canfd_driver,
			 ztest_unit_test(test_set_loopback),
			 ztest_unit_test(test_send_receive_std),
			 ztest_unit_test(test_send_receive_fd),
			 ztest_unit_test(test_send_receive_mix));
	ztest_run_test_suite(canfd_driver);
}
