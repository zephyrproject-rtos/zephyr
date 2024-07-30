/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/ztest.h>

#include "common.h"

/**
 * @addtogroup t_driver_can
 * @{
 * @defgroup t_can_utilities test_can_utilities
 * @}
 */

/**
 * @brief Test of @a can_dlc_to_bytes()
 */
ZTEST(can_utilities, test_can_dlc_to_bytes)
{
	uint8_t dlc;

	/* CAN 2.0B/CAN FD DLC, 0 to 8 data bytes */
	for (dlc = 0; dlc <= 8; dlc++) {
		zassert_equal(can_dlc_to_bytes(dlc), dlc, "wrong number of bytes for DLC %u", dlc);
	}

	/* CAN FD DLC, 12 to 64 data bytes in steps */
	zassert_equal(can_dlc_to_bytes(9),  12, "wrong number of bytes for DLC 9");
	zassert_equal(can_dlc_to_bytes(10), 16, "wrong number of bytes for DLC 10");
	zassert_equal(can_dlc_to_bytes(11), 20, "wrong number of bytes for DLC 11");
	zassert_equal(can_dlc_to_bytes(12), 24, "wrong number of bytes for DLC 12");
	zassert_equal(can_dlc_to_bytes(13), 32, "wrong number of bytes for DLC 13");
	zassert_equal(can_dlc_to_bytes(14), 48, "wrong number of bytes for DLC 14");
	zassert_equal(can_dlc_to_bytes(15), 64, "wrong number of bytes for DLC 15");
}

/**
 * @brief Test of @a can_bytes_to_dlc()
 */
ZTEST(can_utilities, test_can_bytes_to_dlc)
{
	uint8_t bytes;

	/* CAN 2.0B DLC, 0 to 8 data bytes */
	for (bytes = 0; bytes <= 8; bytes++) {
		zassert_equal(can_bytes_to_dlc(bytes), bytes, "wrong DLC for %u byte(s)", bytes);
	}

	/* CAN FD DLC, 12 to 64 data bytes in steps */
	zassert_equal(can_bytes_to_dlc(12), 9,  "wrong DLC for 12 bytes");
	zassert_equal(can_bytes_to_dlc(16), 10, "wrong DLC for 16 bytes");
	zassert_equal(can_bytes_to_dlc(20), 11, "wrong DLC for 20 bytes");
	zassert_equal(can_bytes_to_dlc(24), 12, "wrong DLC for 24 bytes");
	zassert_equal(can_bytes_to_dlc(32), 13, "wrong DLC for 32 bytes");
	zassert_equal(can_bytes_to_dlc(48), 14, "wrong DLC for 48 bytes");
	zassert_equal(can_bytes_to_dlc(64), 15, "wrong DLC for 64 bytes");
}

/**
 * @brief Test of @a can_frame_matches_filter()
 */
ZTEST(can_utilities, test_can_frame_matches_filter)
{
	const struct can_filter test_ext_filter_std_id_1 = {
		.flags = CAN_FILTER_IDE,
		.id = TEST_CAN_STD_ID_1,
		.mask = CAN_EXT_ID_MASK
	};

	/* Standard (11-bit) frames and filters */
	zassert_true(can_frame_matches_filter(&test_std_frame_1, &test_std_filter_1));
	zassert_true(can_frame_matches_filter(&test_std_frame_2, &test_std_filter_2));
	zassert_true(can_frame_matches_filter(&test_std_frame_1, &test_std_masked_filter_1));
	zassert_true(can_frame_matches_filter(&test_std_frame_2, &test_std_masked_filter_2));
	zassert_false(can_frame_matches_filter(&test_std_frame_1, &test_std_filter_2));
	zassert_false(can_frame_matches_filter(&test_std_frame_2, &test_std_filter_1));
	zassert_false(can_frame_matches_filter(&test_std_frame_1, &test_std_masked_filter_2));
	zassert_false(can_frame_matches_filter(&test_std_frame_2, &test_std_masked_filter_1));

	/* Extended (29-bit) frames and filters */
	zassert_true(can_frame_matches_filter(&test_ext_frame_1, &test_ext_filter_1));
	zassert_true(can_frame_matches_filter(&test_ext_frame_2, &test_ext_filter_2));
	zassert_true(can_frame_matches_filter(&test_ext_frame_1, &test_ext_masked_filter_1));
	zassert_true(can_frame_matches_filter(&test_ext_frame_2, &test_ext_masked_filter_2));
	zassert_false(can_frame_matches_filter(&test_ext_frame_1, &test_ext_filter_2));
	zassert_false(can_frame_matches_filter(&test_ext_frame_2, &test_ext_filter_1));
	zassert_false(can_frame_matches_filter(&test_ext_frame_1, &test_ext_masked_filter_2));
	zassert_false(can_frame_matches_filter(&test_ext_frame_2, &test_ext_masked_filter_1));

	/* Standard (11-bit) frames and extended (29-bit) filters */
	zassert_false(can_frame_matches_filter(&test_std_frame_1, &test_ext_filter_1));
	zassert_false(can_frame_matches_filter(&test_std_frame_2, &test_ext_filter_2));
	zassert_false(can_frame_matches_filter(&test_std_frame_1, &test_ext_masked_filter_1));
	zassert_false(can_frame_matches_filter(&test_std_frame_2, &test_ext_masked_filter_2));
	zassert_false(can_frame_matches_filter(&test_std_frame_1, &test_ext_filter_std_id_1));

	/* Extended (29-bit) frames and standard (11-bit) filters */
	zassert_false(can_frame_matches_filter(&test_ext_frame_1, &test_std_filter_1));
	zassert_false(can_frame_matches_filter(&test_ext_frame_2, &test_std_filter_2));
	zassert_false(can_frame_matches_filter(&test_ext_frame_1, &test_std_masked_filter_1));
	zassert_false(can_frame_matches_filter(&test_ext_frame_2, &test_std_masked_filter_2));

	/* Remote transmission request (RTR) frames */
	zassert_true(can_frame_matches_filter(&test_std_rtr_frame_1, &test_std_filter_1));
	zassert_true(can_frame_matches_filter(&test_ext_rtr_frame_1, &test_ext_filter_1));

#ifdef CONFIG_CAN_FD_MODE
	/* CAN FD format frames and filters */
	zassert_true(can_frame_matches_filter(&test_std_fdf_frame_1, &test_std_filter_1));
	zassert_true(can_frame_matches_filter(&test_std_fdf_frame_2, &test_std_filter_2));
#endif /* CONFIG_CAN_FD_MODE */
}

ZTEST_SUITE(can_utilities, NULL, NULL, NULL, NULL, NULL);
