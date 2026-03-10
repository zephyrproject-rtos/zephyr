/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Henrik Brix Andersen <henrik@brixandersen.dk>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/pm/device.h>
#include <zephyr/ztest.h>

#include "common.h"

/**
 * @addtogroup t_can_driver
 * @{
 * @defgroup t_can_powermgmt test_can_powermgmt
 * @}
 */

/**
 * @brief Test suspending and resuming the CAN controller device.
 */
ZTEST(can_powermgmt, test_suspend_resume)
{
	enum pm_device_state pm_state;
	struct can_frame frame;
	int filter_id;
	int err;

	err = can_stop(can_dev);
	zassert_ok(err, "failed to stop CAN controller (err %d)", err);

	err = pm_device_action_run(can_dev, PM_DEVICE_ACTION_SUSPEND);
	zassert_ok(err, "failed to suspend CAN controller (err %d)", err);

	err = pm_device_state_get(can_dev, &pm_state);
	zassert_ok(err, "failed to get CAN controller power management state (err %d)", err);
	zassert_equal(pm_state, PM_DEVICE_STATE_SUSPENDED, "CAN controller not suspended (%s)",
		      pm_device_state_str(pm_state));

	err = pm_device_action_run(can_dev, PM_DEVICE_ACTION_RESUME);
	zassert_ok(err, "failed to resume CAN controller (err %d)", err);

	err = pm_device_state_get(can_dev, &pm_state);
	zassert_ok(err, "failed to get CAN controller power management state (err %d)", err);
	zassert_equal(pm_state, PM_DEVICE_STATE_ACTIVE, "CAN controller not active (%s)",
		      pm_device_state_str(pm_state));

	err = can_set_mode(can_dev, CAN_MODE_LOOPBACK);
	zassert_ok(err, "failed to set loopback mode (err %d)", err);
	zassert_equal(CAN_MODE_LOOPBACK, can_get_mode(can_dev));

	err = can_start(can_dev);
	zassert_ok(err, "failed to start CAN controller (err %d)", err);

	filter_id = can_common_add_rx_msgq(can_dev, &test_std_filter_1);

	err = can_send(can_dev, &test_std_frame_1, TEST_SEND_TIMEOUT, NULL, NULL);
	zassert_ok(err, "failed to send frame without data (err %d)", err);

	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_ok(err, "receive timeout");

	assert_frame_equal(&test_std_frame_1, &frame, 0);

	can_remove_rx_filter(can_dev, filter_id);
}

/**
 * @brief Test suspending the CAN controller device while started.
 */
ZTEST(can_powermgmt, test_suspend_while_started)
{
	int err;

	err = pm_device_action_run(can_dev, PM_DEVICE_ACTION_SUSPEND);
	zassert_equal(err, -EBUSY, "suspended CAN controller while started");
}

/**
 * @brief Test suspending the CAN controller device while RX filters are added.
 */
ZTEST(can_powermgmt, test_suspend_while_filters_added)
{
	int filter_id;
	int err;

	err = can_stop(can_dev);
	zassert_ok(err, "failed to stop CAN controller (err %d)", err);

	filter_id = can_add_rx_filter_msgq(can_dev, &can_msgq, &test_std_filter_1);
	zassert_not_equal(filter_id, -ENOSPC, "no filters available");
	zassert_true(filter_id >= 0, "negative filter number");

	err = pm_device_action_run(can_dev, PM_DEVICE_ACTION_SUSPEND);
	zassert_equal(err, -EBUSY, "suspended CAN controller while RX filters added");

	can_remove_rx_filter(can_dev, filter_id);

	err = can_start(can_dev);
	zassert_ok(err, "failed to start CAN controller (err %d)", err);
}

static bool can_powermgmt_predicate(const void *state)
{
	enum pm_device_state pm_state;
	int err;

	ARG_UNUSED(state);

	if (!device_is_ready(can_dev)) {
		TC_PRINT("CAN device not ready");
		return false;
	}

	err = pm_device_state_get(can_dev, &pm_state);
	if (err == -ENOSYS) {
		TC_PRINT("CAN controller does not support device power management");
		return false;
	}

	return true;
}

void *can_powermgmt_setup(void)
{
	can_common_test_setup(CAN_MODE_LOOPBACK);

	return NULL;
}

ZTEST_SUITE(can_powermgmt, can_powermgmt_predicate, can_powermgmt_setup, NULL, NULL, NULL);
