/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test.h"

#include <zephyr/logging/log.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys_clock.h>

LOG_MODULE_REGISTER(dev_test, CONFIG_APP_LOG_LEVEL);

struct dev_test_data {
	struct pm_policy_latency_subscription subs;
	struct pm_policy_latency_request req;
};

static void on_latency_changed(int32_t latency)
{
	if (latency == SYS_FOREVER_US) {
		LOG_INF("Latency constraint changed: none");
	} else {
		LOG_INF("Latency constraint changed: %" PRId32 "ms",
			latency / USEC_PER_MSEC);
	}
}

static void dev_test_open(const struct device *dev)
{
	struct dev_test_data *data = dev->data;

	LOG_INF("Adding latency constraint: 20ms");
	pm_policy_latency_request_add(&data->req, 20000);

	pm_policy_latency_changed_subscribe(&data->subs, on_latency_changed);
}

static void dev_test_close(const struct device *dev)
{
	struct dev_test_data *data = dev->data;

	LOG_INF("Removing latency constraint");
	pm_policy_latency_request_remove(&data->req);

	pm_policy_latency_changed_unsubscribe(&data->subs);
}

static const struct test_api dev_test_api = {
	.open = dev_test_open,
	.close = dev_test_close,
};

int dev_test_init(const struct device *dev)
{
	return 0;
}

static struct dev_test_data data;

DEVICE_DEFINE(dev_test, "dev_test", &dev_test_init, NULL, &data, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &dev_test_api);
