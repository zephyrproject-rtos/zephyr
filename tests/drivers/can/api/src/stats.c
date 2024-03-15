/*
 * Copyright (c) 2023 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/ztest.h>

#include "common.h"

/**
 * @addtogroup t_driver_can
 * @{
 * @defgroup t_can_stats test_can_stats
 * @}
 */

/**
 * @brief Test that CAN statistics can be accessed from user mode threads.
 */
ZTEST_USER(can_stats, test_can_stats_accessors)
{
	uint32_t val;

	val = can_stats_get_bit_errors(can_dev);
	val = can_stats_get_bit0_errors(can_dev);
	val = can_stats_get_bit1_errors(can_dev);
	val = can_stats_get_stuff_errors(can_dev);
	val = can_stats_get_crc_errors(can_dev);
	val = can_stats_get_form_errors(can_dev);
	val = can_stats_get_ack_errors(can_dev);
	val = can_stats_get_rx_overruns(can_dev);
}

void *can_stats_setup(void)
{
	k_object_access_grant(can_dev, k_current_get());

	zassert_true(device_is_ready(can_dev), "CAN device not ready");

	return NULL;
}

ZTEST_SUITE(can_stats, NULL, can_stats_setup, NULL, NULL, NULL);
