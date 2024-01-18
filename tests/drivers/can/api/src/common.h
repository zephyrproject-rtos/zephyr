/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>

/**
 * Test bitrates in bits/second.
 */
#define TEST_BITRATE_1 125000
#define TEST_BITRATE_2 250000
#define TEST_BITRATE_3 1000000

/**
 * Test sample points in per mille.
 */
#define TEST_SAMPLE_POINT 875
#define TEST_SAMPLE_POINT_2 750

/**
 * @brief Test timeouts.
 */
#define TEST_SEND_TIMEOUT    K_MSEC(100)
#define TEST_RECEIVE_TIMEOUT K_MSEC(100)
#define TEST_RECOVER_TIMEOUT K_MSEC(100)

/**
 * @brief Standard (11-bit) CAN IDs and masks used for testing.
 */
#define TEST_CAN_STD_ID_1      0x555
#define TEST_CAN_STD_ID_2      0x565
#define TEST_CAN_STD_MASK_ID_1 0x55A
#define TEST_CAN_STD_MASK_ID_2 0x56A
#define TEST_CAN_STD_MASK      0x7F0
#define TEST_CAN_SOME_STD_ID   0x123

/**
 * @brief Extended (29-bit) CAN IDs and masks used for testing.
 */
#define TEST_CAN_EXT_ID_1      0x15555555
#define TEST_CAN_EXT_ID_2      0x15555565
#define TEST_CAN_EXT_MASK_ID_1 0x1555555A
#define TEST_CAN_EXT_MASK_ID_2 0x1555556A
#define TEST_CAN_EXT_MASK      0x1FFFFFF0

/**
 * @brief Common variables.
 */
extern ZTEST_DMEM const struct device *const can_dev;
extern struct k_sem rx_callback_sem;
extern struct k_sem tx_callback_sem;
extern struct k_msgq can_msgq;

/**
 * @brief Standard (11-bit) CAN ID frame 1.
 */
extern const struct can_frame test_std_frame_1;

/**
 * @brief Standard (11-bit) CAN ID frame 2.
 */
extern const struct can_frame test_std_frame_2;

/**
 * @brief Extended (29-bit) CAN ID frame 1.
 */
extern const struct can_frame test_ext_frame_1;

/**
 * @brief Extended (29-bit) CAN ID frame 1.
 */
extern const struct can_frame test_ext_frame_2;

/**
 * @brief Standard (11-bit) CAN ID RTR frame 1.
 */
extern const struct can_frame test_std_rtr_frame_1;

/**
 * @brief Extended (29-bit) CAN ID RTR frame 1.
 */
extern const struct can_frame test_ext_rtr_frame_1;

#ifdef CONFIG_CAN_FD_MODE
/**
 * @brief Standard (11-bit) CAN ID frame 1 with CAN FD payload.
 */
extern const struct can_frame test_std_fdf_frame_1;

/**
 * @brief Standard (11-bit) CAN ID frame 2 with CAN FD payload.
 */
extern const struct can_frame test_std_fdf_frame_2;
#endif /* CONFIG_CAN_FD_MODE */

/**
 * @brief Standard (11-bit) CAN ID filter 1. This filter matches
 * ``test_std_frame_1`` and ``test_std_fdf_frame_1``.
 */
extern const struct can_filter test_std_filter_1;

/**
 * @brief Standard (11-bit) CAN ID filter 2. This filter matches
 * ``test_std_frame_2`` and ``test_std_fdf_frame_2``.
 */
extern const struct can_filter test_std_filter_2;

/**
 * @brief Standard (11-bit) CAN ID masked filter 1. This filter matches
 * ``test_std_frame_1``.
 */
extern const struct can_filter test_std_masked_filter_1;

/**
 * @brief Standard (11-bit) CAN ID masked filter 2. This filter matches
 * ``test_std_frame_2``.
 */
extern const struct can_filter test_std_masked_filter_2;

/**
 * @brief Extended (29-bit) CAN ID filter 1. This filter matches
 * ``test_ext_frame_1``.
 */
extern const struct can_filter test_ext_filter_1;

/**
 * @brief Extended (29-bit) CAN ID filter 2. This filter matches
 * ``test_ext_frame_2``.
 */
extern const struct can_filter test_ext_filter_2;

/**
 * @brief Extended (29-bit) CAN ID masked filter 1. This filter matches
 * ``test_ext_frame_1``.
 */
extern const struct can_filter test_ext_masked_filter_1;

/**
 * @brief Extended (29-bit) CAN ID masked filter 2. This filter matches
 * ``test_ext_frame_2``.
 */
extern const struct can_filter test_ext_masked_filter_2;

/**
 * @brief Standard (11-bit) CAN ID filter. This filter matches
 * ``TEST_CAN_SOME_STD_ID``.
 */
extern const struct can_filter test_std_some_filter;

/**
 * @brief Assert that two CAN frames are equal given a CAN ID mask.
 *
 * @param frame1  First CAN frame.
 * @param frame2  Second CAN frame.
 * @param id_mask CAN ID mask.
 */
void assert_frame_equal(const struct can_frame *frame1,
			const struct can_frame *frame2,
			uint32_t id_mask);
