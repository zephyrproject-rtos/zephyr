/*
 * Copyright (c) 2024 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/ztest.h>

#include "common.h"

/**
 * @addtogroup t_can_driver
 * @{
 * @defgroup t_can_transceiver test_can_transceiver
 * @}
 */

/**
 * @brief Test getting CAN transceiver device pointer.
 */
ZTEST_USER(can_transceiver, test_get_transceiver)
{
	const struct device *phy = can_get_transceiver(can_dev);

	zassert_equal(phy, can_phy, "wrong CAN transceiver device pointer returned");
}

static bool can_transceiver_predicate(const void *state)
{
	ARG_UNUSED(state);

	if (!device_is_ready(can_dev)) {
		TC_PRINT("CAN device not ready");
		return false;
	}

	if (!device_is_ready(can_phy)) {
		TC_PRINT("CAN transceiver device not ready");
		return false;
	}

	return true;
}

void *can_transceiver_setup(void)
{
	k_object_access_grant(can_dev, k_current_get());
	k_object_access_grant(can_phy, k_current_get());

	zassert_true(device_is_ready(can_dev), "CAN device not ready");
	zassert_true(device_is_ready(can_phy), "CAN transceiver device not ready");

	return NULL;
}

ZTEST_SUITE(can_transceiver, can_transceiver_predicate, can_transceiver_setup, NULL, NULL, NULL);
