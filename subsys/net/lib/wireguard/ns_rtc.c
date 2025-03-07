/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Get time from host system when running on native_sim board */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(wireguard, CONFIG_WIREGUARD_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/wireguard.h>

#include "native_rtc.h"

int wireguard_get_current_time(uint64_t *seconds, uint32_t *nanoseconds)
{
	uint64_t timeus;

	timeus = native_rtc_gettime_us(RTC_CLOCK_PSEUDOHOSTREALTIME);

	*seconds = timeus / USEC_PER_SEC;
	*nanoseconds = (timeus % USEC_PER_SEC) * NSEC_PER_USEC;

	NET_DBG("Current time: %llu.%09u", *seconds, *nanoseconds);

	return 0;
}
