/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/prometheus/counter.h>

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pm_counter, CONFIG_PROMETHEUS_LOG_LEVEL);

void prometheus_counter_inc(struct prometheus_counter *counter)
{
	if (counter) {
		counter->value++;
	}
}
