/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys_clock.h>

LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

static void on_latency_changed(int32_t latency)
{
	if (latency == SYS_FOREVER_US) {
		LOG_INF("Latency constraint changed: none");
	} else {
		LOG_INF("Latency constraint changed: %" PRId32 "ms",
			latency / USEC_PER_MSEC);
	}
}

int main(void)
{
	struct pm_policy_latency_subscription subs;
	struct pm_policy_latency_request req;
	const struct device *dev;

	dev = device_get_binding("dev_test");
	if (!device_is_ready(dev)) {
		LOG_ERR("Device not ready");
		return 0;
	}

	pm_policy_latency_changed_subscribe(&subs, on_latency_changed);

	/* test without any latency constraint */
	LOG_INF("Sleeping for 1.1 seconds, we should enter RUNTIME_IDLE");
	k_msleep(1100);
	LOG_INF("Sleeping for 1.2 seconds, we should enter SUSPEND_TO_IDLE");
	k_msleep(1200);
	LOG_INF("Sleeping for 1.3 seconds, we should enter STANDBY");
	k_msleep(1300);

	/* add an application level latency constraint */
	LOG_INF("Setting latency constraint: 30ms");
	pm_policy_latency_request_add(&req, 30000);

	/* test with an application level constraint */
	LOG_INF("Sleeping for 1.1 seconds, we should enter RUNTIME_IDLE");
	k_msleep(1100);
	LOG_INF("Sleeping for 1.2 seconds, we should enter SUSPEND_TO_IDLE");
	k_msleep(1200);
	LOG_INF("Sleeping for 1.3 seconds, we should enter SUSPEND_TO_IDLE");
	k_msleep(1300);

	/* open test device (adds its own latency constraint) */
	LOG_INF("Opening test device");
	test_open(dev);

	/* test with application + driver latency constraints */
	LOG_INF("Sleeping for 1.1 seconds, we should enter RUNTIME_IDLE");
	k_msleep(1100);
	LOG_INF("Sleeping for 1.2 seconds, we should enter RUNTIME_IDLE");
	k_msleep(1200);
	LOG_INF("Sleeping for 1.3 seconds, we should enter RUNTIME_IDLE");
	k_msleep(1300);

	/* update application level latency constraint */
	LOG_INF("Updating latency constraint: 10ms");
	pm_policy_latency_request_update(&req, 10000);

	/* test with updated application + driver latency constraints */
	LOG_INF("Sleeping for 1.1 seconds, we should stay ACTIVE");
	k_msleep(1100);
	LOG_INF("Sleeping for 1.2 seconds, we should stay ACTIVE");
	k_msleep(1200);
	LOG_INF("Sleeping for 1.3 seconds, we should stay ACTIVE");
	k_msleep(1300);

	/* restore application level latency constraint */
	LOG_INF("Updating latency constraint: 30ms");
	pm_policy_latency_request_update(&req, 30000);

	/* close test device (removes its own latency constraint) */
	LOG_INF("Closing test device");
	test_close(dev);

	/* test again with an application level constraint */
	LOG_INF("Sleeping for 1.1 seconds, we should enter RUNTIME_IDLE");
	k_msleep(1100);
	LOG_INF("Sleeping for 1.2 seconds, we should enter SUSPEND_TO_IDLE");
	k_msleep(1200);
	LOG_INF("Sleeping for 1.3 seconds, we should enter SUSPEND_TO_IDLE");
	k_msleep(1300);

	/* remove application level constraint and terminate */
	LOG_INF("Removing latency constraints");
	pm_policy_latency_request_remove(&req);

	pm_policy_latency_changed_unsubscribe(&subs);

	LOG_INF("Finished, we should now enter STANDBY");
	return 0;
}
