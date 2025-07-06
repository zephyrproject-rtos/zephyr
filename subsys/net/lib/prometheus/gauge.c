/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/prometheus/gauge.h>

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pm_gauge, CONFIG_PROMETHEUS_LOG_LEVEL);

int prometheus_gauge_set(struct prometheus_gauge *gauge, double value)
{
	if (value < 0) {
		LOG_ERR("Invalid value");
		return -EINVAL;
	}

	if (gauge) {
		gauge->value = value;
	}

	return 0;
}
