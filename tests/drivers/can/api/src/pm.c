/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/state.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "common.h"

/**
 * @addtogroup t_can_driver
 * @{
 * @defgroup t_can_pm test_can_pm
 * @}
 */

/* Long enough for the idle thread to reach the power management policy. */
#define TEST_IDLE_WINDOW K_MSEC(100)

static atomic_t low_power_entries;

static void pm_state_entry(enum pm_state state)
{
	ARG_UNUSED(state);

	atomic_inc(&low_power_entries);
}

static struct pm_notifier test_pm_notifier = {
	.state_entry = pm_state_entry,
};

static void pm_tx_callback(const struct device *dev, int error, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	zassert_equal(error, 0, "transmit failed after a low power state (err %d)", error);
	k_sem_give(&tx_callback_sem);
}

/**
 * @brief Idle for a while and report how many low power states were entered.
 */
static int idle_and_count_low_power_entries(void)
{
	atomic_clear(&low_power_entries);
	k_sleep(TEST_IDLE_WINDOW);

	return (int)atomic_get(&low_power_entries);
}

static void send_and_loopback_after_sleep(const struct can_frame *expected)
{
	struct can_frame frame;
	int err;

	k_sem_reset(&tx_callback_sem);

	err = can_send(can_dev, expected, TEST_SEND_TIMEOUT, pm_tx_callback, NULL);
	zassert_ok(err, "failed to queue frame after a low power state (err %d)", err);

	err = k_sem_take(&tx_callback_sem, TEST_SEND_TIMEOUT);
	zassert_ok(err, "transmit callback not called after a low power state");

	err = k_msgq_get(&can_msgq, &frame, TEST_RECEIVE_TIMEOUT);
	zassert_ok(err, "receive timeout after a low power state");

	assert_frame_equal(expected, &frame, 0);
}

/**
 * @brief Test that no low power state is entered while the CAN controller is started.
 */
ZTEST(can_pm, test_started_blocks_low_power_states)
{
	zassert_equal(idle_and_count_low_power_entries(), 0,
		      "entered a low power state while the CAN controller was started");
}

/**
 * @brief Test that stopping the CAN controller lets the system enter low power states again.
 */
ZTEST(can_pm, test_stopped_allows_low_power_states)
{
	int err;

	err = can_stop(can_dev);
	zassert_ok(err, "failed to stop CAN controller (err %d)", err);

	zassert_true(idle_and_count_low_power_entries() > 0,
		     "never entered a low power state with the CAN controller stopped");
}

/**
 * @brief Test that a start reporting -EALREADY keeps low power states blocked.
 */
ZTEST(can_pm, test_redundant_start_keeps_low_power_states_blocked)
{
	int err;

	err = can_start(can_dev);
	zassert_equal(err, -EALREADY, "starting a started CAN controller did not report -EALREADY");

	zassert_equal(idle_and_count_low_power_entries(), 0,
		      "entered a low power state after a redundant start");

	err = can_stop(can_dev);
	zassert_ok(err, "failed to stop CAN controller (err %d)", err);

	zassert_true(idle_and_count_low_power_entries() > 0,
		     "never entered a low power state after a redundant start and a stop");
}

/**
 * @brief Test that a stop reporting -EALREADY keeps low power states available.
 */
ZTEST(can_pm, test_redundant_stop_keeps_low_power_states_allowed)
{
	int err;

	err = can_stop(can_dev);
	zassert_ok(err, "failed to stop CAN controller (err %d)", err);

	err = can_stop(can_dev);
	zassert_equal(err, -EALREADY, "stopping a stopped CAN controller did not report -EALREADY");

	zassert_true(idle_and_count_low_power_entries() > 0,
		     "never entered a low power state after a redundant stop");
}

/**
 * @brief Test sending and receiving after a low power state without reconfiguring.
 */
ZTEST(can_pm, test_configuration_survives_low_power_state)
{
	int filter_id;
	int err;

	err = can_stop(can_dev);
	zassert_ok(err, "failed to stop CAN controller (err %d)", err);

	filter_id = can_common_add_rx_msgq(can_dev, &test_std_filter_1);

	zassert_true(idle_and_count_low_power_entries() > 0,
		     "never entered a low power state, nothing was verified");

	err = can_start(can_dev);
	zassert_ok(err, "failed to start CAN controller after a low power state (err %d)", err);

	send_and_loopback_after_sleep(&test_std_frame_1);

	can_remove_rx_filter(can_dev, filter_id);
}

#ifdef CONFIG_CAN_FD_MODE
/**
 * @brief Test that CAN FD timing still works after a low power state without reconfiguring.
 */
ZTEST(can_pm, test_fd_configuration_survives_low_power_state)
{
	can_mode_t caps = 0;
	int filter_id;
	int err;

	err = can_get_capabilities(can_dev, &caps);
	zassert_ok(err, "failed to get CAN capabilities (err %d)", err);
	if ((caps & CAN_MODE_FD) == 0) {
		ztest_test_skip();
		return;
	}

	err = can_stop(can_dev);
	zassert_ok(err, "failed to stop CAN controller (err %d)", err);

	err = can_set_mode(can_dev, CAN_MODE_LOOPBACK | CAN_MODE_FD);
	zassert_ok(err, "failed to set CAN FD loopback mode (err %d)", err);

	filter_id = can_common_add_rx_msgq(can_dev, &test_std_filter_1);

	zassert_true(idle_and_count_low_power_entries() > 0,
		     "never entered a low power state, nothing was verified");

	err = can_start(can_dev);
	zassert_ok(err, "failed to start CAN controller after a low power state (err %d)", err);

	send_and_loopback_after_sleep(&test_std_fdf_frame_1);

	can_remove_rx_filter(can_dev, filter_id);

	err = can_stop(can_dev);
	zassert_ok(err, "failed to stop CAN controller (err %d)", err);
	err = can_set_mode(can_dev, CAN_MODE_LOOPBACK);
	zassert_ok(err, "failed to restore loopback mode (err %d)", err);
}
#endif /* CONFIG_CAN_FD_MODE */

static bool can_pm_predicate(const void *state)
{
	const struct pm_state_info *states;
	uint8_t count;

	ARG_UNUSED(state);

	if (!device_is_ready(can_dev)) {
		TC_PRINT("CAN device not ready\n");
		return false;
	}

	count = pm_state_cpu_get_all(0U, &states);
	if (count == 0U) {
		TC_PRINT("no low power states defined for this platform\n");
		return false;
	}

	return true;
}

static void *can_pm_setup(void)
{
	can_common_test_setup(CAN_MODE_LOOPBACK);
	pm_notifier_register(&test_pm_notifier);

	return NULL;
}

static void can_pm_before(void *fixture)
{
	int err;

	ARG_UNUSED(fixture);

	/* Each test starts from a started controller, whatever the previous one left behind. */
	err = can_start(can_dev);
	if (err != -EALREADY) {
		zassert_ok(err, "failed to start CAN controller (err %d)", err);
	}
}

static void can_pm_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	pm_notifier_unregister(&test_pm_notifier);
}

ZTEST_SUITE(can_pm, can_pm_predicate, can_pm_setup, can_pm_before, NULL, can_pm_teardown);
