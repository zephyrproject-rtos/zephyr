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
 * @defgroup t_can_stm32 test_stm32_can
 * @brief TestPurpose: verify correct filter handling of stm32 driver
 * @details
 * - Test Steps
 *   -# Set driver to loopback mode
 *   -# Attach a extended and masked filter (1 bank/filter)
 *   -# Attach a standard none masked filter (1/4 bank/filter)
 *   -# Detach the first filter (first bank gets free) an attach
 *      a different standard none masked filter (1/4 bank/filter).
 *      Bank 0 is extended to 4 filters/bank which leads to a left shift
 *      of the first filter by 3 and tests the corner case of the last filter
 *      is used.
 *   -# Test message sending and receiving
 *   -# Detach first fillter (gets free) and attach an extended filter.
 *      This shrinks bank 0 to 2 filters/bank which leads to a right shift
 *      of the first buffer by two.
 *   -# Test message sending and receiving.
 *   -# Detach both filters.
 * - Expected Results
 *   -# Message reception should work in any case
 * @}
 */

#if defined(DT_CAN_0_NAME)
#define CAN_NAME DT_CAN_0_NAME
#elif defined(DT_CAN_1_NAME)
#define CAN_NAME DT_CAN_1_NAME
#else
#error No CAN device available
#endif

#define TEST_SEND_TIMEOUT    K_MSEC(100)
#define TEST_RECEIVE_TIMEOUT K_MSEC(100)

#define TEST_CAN_STD_ID      0x555
#define TEST_CAN_SOME_STD_ID 0x123

#define TEST_CAN_EXT_ID      0x15555555
#define TEST_CAN_EXT_MASK    0x1FFFFFF0

CAN_DEFINE_MSGQ(can_msgq, 5);

struct zcan_frame test_std_msg = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.std_id  = TEST_CAN_STD_ID,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

const struct zcan_filter test_std_filter = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.std_id = TEST_CAN_STD_ID,
	.rtr_mask = 1,
	.std_id_mask = CAN_STD_ID_MASK
};

const struct zcan_filter test_ext_filter = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.ext_id = TEST_CAN_EXT_ID,
	.rtr_mask = 1,
	.ext_id_mask = CAN_EXT_ID_MASK
};

const struct zcan_filter test_ext_masked_filter = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.ext_id = TEST_CAN_EXT_ID,
	.rtr_mask = 1,
	.ext_id_mask = TEST_CAN_EXT_MASK
};

const struct zcan_filter test_std_some_filter = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.std_id = TEST_CAN_SOME_STD_ID,
	.rtr_mask = 1,
	.std_id_mask = CAN_STD_ID_MASK
};

static inline void check_msg(struct zcan_frame *msg1, struct zcan_frame *msg2)
{
	int cmp_res;

	zassert_equal(msg1->id_type, msg2->id_type,
		      "ID type does not match");

	zassert_equal(msg1->rtr, msg2->rtr,
		      "RTR bit does not match");

	if (msg2->id_type == CAN_STANDARD_IDENTIFIER) {
		zassert_equal(msg1->std_id, msg2->std_id,
			      "ID does not match");
	} else {
		zassert_equal(msg1->ext_id, msg2->ext_id,
			      "ID does not match");
	}

	zassert_equal(msg1->dlc, msg2->dlc,
		      "DLC does not match");

	cmp_res = memcmp(msg1->data, msg2->data, msg1->dlc);
	zassert_equal(cmp_res, 0, "Received data differ");
}

static void send_test_msg(struct device *can_dev, struct zcan_frame *msg)
{
	int ret;

	ret = can_send(can_dev, msg, TEST_SEND_TIMEOUT, NULL, NULL);
	zassert_not_equal(ret, CAN_TX_ARB_LOST,
			  "Arbitration though in loopback mode");
	zassert_equal(ret, CAN_TX_OK, "Can't send a message. Err: %d", ret);
}

/*
 * Test a more adcvanced filter handling. Attach more than one filter at
 * the same time, remove and change the filters before the message.
 * This tests the internals filter handling of the driver itself.
 */
static void test_filter_handling(void)
{
	struct device *can_dev;
	int ret, filter_id_1, filter_id_2;
	struct zcan_frame msg_buffer;

	can_dev = device_get_binding(CAN_NAME);

	ret = can_configure(can_dev, CAN_LOOPBACK_MODE, 0);

	filter_id_1 = can_attach_msgq(can_dev, &can_msgq, &test_ext_masked_filter);
	zassert_not_equal(filter_id_1, CAN_NO_FREE_FILTER,
			  "Filter full even for a single one");
	zassert_true((filter_id_1 >= 0), "Negative filter number");

	filter_id_2 = can_attach_msgq(can_dev, &can_msgq, &test_std_filter);
	zassert_not_equal(filter_id_2, CAN_NO_FREE_FILTER,
			  "Filter full when attaching the second one");
	zassert_true((filter_id_2 >= 0), "Negative filter number");

	can_detach(can_dev, filter_id_1);
	filter_id_1 = can_attach_msgq(can_dev, &can_msgq, &test_std_some_filter);
	zassert_not_equal(filter_id_1, CAN_NO_FREE_FILTER,
			  "Filter full when overriding the first one");
	zassert_true((filter_id_1 >= 0), "Negative filter number");

	send_test_msg(can_dev, &test_std_msg);

	ret = k_msgq_get(&can_msgq, &msg_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, 0, "Receiving timeout");
	check_msg(&test_std_msg, &msg_buffer);

	ret = k_msgq_get(&can_msgq, &msg_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, -EAGAIN, "There is more than one msg in the queue");

	can_detach(can_dev, filter_id_1);
	filter_id_1 = can_attach_msgq(can_dev, &can_msgq, &test_ext_filter);
	zassert_not_equal(filter_id_1, CAN_NO_FREE_FILTER,
			  "Filter full when overriding the first one");
	zassert_true((filter_id_1 >= 0), "Negative filter number");

	send_test_msg(can_dev, &test_std_msg);

	ret = k_msgq_get(&can_msgq, &msg_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, 0, "Receiving timeout");
	check_msg(&test_std_msg, &msg_buffer);

	can_detach(can_dev, filter_id_1);
	can_detach(can_dev, filter_id_2);
}

void test_main(void)
{
	ztest_test_suite(can_driver,
			 ztest_unit_test(test_filter_handling));
	ztest_run_test_suite(can_driver);
}
