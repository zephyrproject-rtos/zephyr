/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ap_pwrseq/ap_pwrseq.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define DT_DRV_COMPAT        ap_pwrseq_state

bool success;

void entry_callback(const struct device *dev,
		enum ap_pwrseq_state state,
		enum ap_pwrseq_action action)
{
	LOG_INF("Callback triggered");
	if (action == AP_POWER_STATE_ACTION_ENTRY) {
		LOG_INF("Entering state %s", ap_pwrseq_get_state_str(state));
		switch(state) {
		case AP_POWER_STATE_S5:
		case AP_POWER_STATE_S4:
		case AP_POWER_STATE_S3:
			ap_pwrseq_post_event(dev, AP_PWRSEQ_EVENT_POWER_SIGNAL);
			break;
		case AP_POWER_STATE_S0:
		case AP_POWER_STATE_G3:
			break;
		default:
			zassert_unreachable("Spurious event");
			break;
		}
	} else if (action == AP_POWER_STATE_ACTION_EXIT) {
		LOG_INF("Exiting state %s", ap_pwrseq_get_state_str(state));
		switch(state) {
		case AP_POWER_STATE_S0IX_2: {
			enum ap_pwrseq_state cur_state;
			ap_pwrseq_post_event(dev, AP_PWRSEQ_EVENT_POWER_BUTTON);
			if (!ap_pwrseq_get_current_state(dev, &cur_state)) {
				success = true;
			}
			break;
		}
		default:
			zassert_unreachable("Spurious event");
			break;
		}
	}
}

void test_execute(void)
{
	static struct ap_pwrseq_state_callback entry_state_cb = {
		.cb = entry_callback,
		.states_bit_mask = BIT(AP_POWER_STATE_S0) |
				   BIT(AP_POWER_STATE_S4) |
				   BIT(AP_POWER_STATE_S5) |
				   BIT(AP_POWER_STATE_S3) |
				   BIT(AP_POWER_STATE_G3),
		.action = AP_POWER_STATE_ACTION_ENTRY,
	};
	static struct ap_pwrseq_state_callback exit_state_cb = {
		.cb = entry_callback,
		.states_bit_mask = BIT(AP_POWER_STATE_S0IX_2),
		.action = AP_POWER_STATE_ACTION_EXIT,
	};
	const struct device * dev = ap_pwrseq_get_instance();

	ap_pwrseq_register_state_callback(dev, &entry_state_cb);
	ap_pwrseq_register_state_callback(dev, &exit_state_cb);
	ap_pwrseq_start(dev);
	ap_pwrseq_post_event(dev, AP_PWRSEQ_EVENT_POWER_BUTTON);

	k_msleep(500);
	zassert_equal(1, success, "Test failed");
}

void test_main(void)
{
	ztest_test_suite(ap_pwrseq,
			ztest_1cpu_unit_test(test_execute));
	ztest_run_test_suite(ap_pwrseq);
}
