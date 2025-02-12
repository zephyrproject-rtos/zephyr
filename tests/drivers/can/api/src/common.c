/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "common.h"

/**
 * @brief Global variables.
 */
ZTEST_DMEM const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
ZTEST_DMEM const struct device *const can_phy =
	DEVICE_DT_GET_OR_NULL(DT_PHANDLE(DT_CHOSEN(zephyr_canbus), phys));
struct k_sem rx_callback_sem;
struct k_sem tx_callback_sem;

CAN_MSGQ_DEFINE(can_msgq, 5);

/**
 * @brief Standard (11-bit) CAN ID frame 1.
 */
const struct can_frame test_std_frame_1 = {
	.flags   = 0,
	.id      = TEST_CAN_STD_ID_1,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

/**
 * @brief Standard (11-bit) CAN ID frame 2.
 */
const struct can_frame test_std_frame_2 = {
	.flags   = 0,
	.id      = TEST_CAN_STD_ID_2,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

/**
 * @brief Extended (29-bit) CAN ID frame 1.
 */
const struct can_frame test_ext_frame_1 = {
	.flags   = CAN_FRAME_IDE,
	.id      = TEST_CAN_EXT_ID_1,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

/**
 * @brief Extended (29-bit) CAN ID frame 1.
 */
const struct can_frame test_ext_frame_2 = {
	.flags   = CAN_FRAME_IDE,
	.id      = TEST_CAN_EXT_ID_2,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

/**
 * @brief Standard (11-bit) CAN ID RTR frame 1.
 */
const struct can_frame test_std_rtr_frame_1 = {
	.flags   = CAN_FRAME_RTR,
	.id      = TEST_CAN_STD_ID_1,
	.dlc     = 0,
	.data    = {0}
};

/**
 * @brief Extended (29-bit) CAN ID RTR frame 1.
 */
const struct can_frame test_ext_rtr_frame_1 = {
	.flags   = CAN_FRAME_IDE | CAN_FRAME_RTR,
	.id      = TEST_CAN_EXT_ID_1,
	.dlc     = 0,
	.data    = {0}
};

#ifdef CONFIG_CAN_FD_MODE
/**
 * @brief Standard (11-bit) CAN ID frame 1 with CAN FD payload.
 */
const struct can_frame test_std_fdf_frame_1 = {
	.flags   = CAN_FRAME_FDF | CAN_FRAME_BRS,
	.id      = TEST_CAN_STD_ID_1,
	.dlc     = 0xf,
	.data    = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
		    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
		    46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
		    61, 62, 63, 64 }
};

/**
 * @brief Standard (11-bit) CAN ID frame 1 with CAN FD payload.
 */
const struct can_frame test_std_fdf_frame_2 = {
	.flags   = CAN_FRAME_FDF | CAN_FRAME_BRS,
	.id      = TEST_CAN_STD_ID_2,
	.dlc     = 0xf,
	.data    = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
		    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
		    46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
		    61, 62, 63, 64 }
};
#endif /* CONFIG_CAN_FD_MODE */

/**
 * @brief Standard (11-bit) CAN ID filter 1. This filter matches
 * ``test_std_frame_1``.
 */
const struct can_filter test_std_filter_1 = {
	.flags = 0U,
	.id = TEST_CAN_STD_ID_1,
	.mask = CAN_STD_ID_MASK
};

/**
 * @brief Standard (11-bit) CAN ID filter 2. This filter matches
 * ``test_std_frame_2``.
 */
const struct can_filter test_std_filter_2 = {
	.flags = 0U,
	.id = TEST_CAN_STD_ID_2,
	.mask = CAN_STD_ID_MASK
};

/**
 * @brief Standard (11-bit) CAN ID masked filter 1. This filter matches
 * ``test_std_frame_1``.
 */
const struct can_filter test_std_masked_filter_1 = {
	.flags = 0U,
	.id = TEST_CAN_STD_MASK_ID_1,
	.mask = TEST_CAN_STD_MASK
};

/**
 * @brief Standard (11-bit) CAN ID masked filter 2. This filter matches
 * ``test_std_frame_2``.
 */
const struct can_filter test_std_masked_filter_2 = {
	.flags = 0U,
	.id = TEST_CAN_STD_MASK_ID_2,
	.mask = TEST_CAN_STD_MASK
};

/**
 * @brief Extended (29-bit) CAN ID filter 1. This filter matches
 * ``test_ext_frame_1``.
 */
const struct can_filter test_ext_filter_1 = {
	.flags = CAN_FILTER_IDE,
	.id = TEST_CAN_EXT_ID_1,
	.mask = CAN_EXT_ID_MASK
};

/**
 * @brief Extended (29-bit) CAN ID filter 2. This filter matches
 * ``test_ext_frame_2``.
 */
const struct can_filter test_ext_filter_2 = {
	.flags = CAN_FILTER_IDE,
	.id = TEST_CAN_EXT_ID_2,
	.mask = CAN_EXT_ID_MASK
};

/**
 * @brief Extended (29-bit) CAN ID masked filter 1. This filter matches
 * ``test_ext_frame_1``.
 */
const struct can_filter test_ext_masked_filter_1 = {
	.flags = CAN_FILTER_IDE,
	.id = TEST_CAN_EXT_MASK_ID_1,
	.mask = TEST_CAN_EXT_MASK
};

/**
 * @brief Extended (29-bit) CAN ID masked filter 2. This filter matches
 * ``test_ext_frame_2``.
 */
const struct can_filter test_ext_masked_filter_2 = {
	.flags = CAN_FILTER_IDE,
	.id = TEST_CAN_EXT_MASK_ID_2,
	.mask = TEST_CAN_EXT_MASK
};

/**
 * @brief Standard (11-bit) CAN ID filter. This filter matches
 * ``TEST_CAN_SOME_STD_ID``.
 */
const struct can_filter test_std_some_filter = {
	.flags = 0U,
	.id = TEST_CAN_SOME_STD_ID,
	.mask = CAN_STD_ID_MASK
};

/**
 * @brief Assert that two CAN frames are equal given a CAN ID mask.
 *
 * @param frame1  First CAN frame.
 * @param frame2  Second CAN frame.
 * @param id_mask CAN ID mask.
 */
void assert_frame_equal(const struct can_frame *frame1,
			const struct can_frame *frame2,
			uint32_t id_mask)
{
	zassert_equal(frame1->flags, frame2->flags, "Flags do not match");
	zassert_equal(frame1->id | id_mask, frame2->id | id_mask, "ID does not match");
	zassert_equal(frame1->dlc, frame2->dlc, "DLC does not match");

	if ((frame1->flags & CAN_FRAME_RTR) == 0U) {
		zassert_mem_equal(frame1->data, frame2->data, can_dlc_to_bytes(frame1->dlc),
				  "Received data differ");
	}
}

void can_common_test_setup(can_mode_t initial_mode)
{
	int err;

	k_sem_init(&rx_callback_sem, 0, 2);
	k_sem_init(&tx_callback_sem, 0, 2);

	k_object_access_grant(&can_msgq, k_current_get());
	k_object_access_grant(can_dev, k_current_get());

	zassert_true(device_is_ready(can_dev), "CAN device not ready");

	(void)can_stop(can_dev);

	err = can_set_mode(can_dev, initial_mode);
	zassert_equal(err, 0, "failed to set initial mode (err %d)", err);
	zassert_equal(initial_mode, can_get_mode(can_dev));

	err = can_start(can_dev);
	zassert_equal(err, 0, "failed to start CAN controller (err %d)", err);
}
