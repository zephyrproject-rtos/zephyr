/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CPU frequency boost for nRF53 series
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <nrfx_clock.h>

LOG_MODULE_DECLARE(zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

static int nrf53_cpu_boost(void)
{
	int err;

	/* For optimal performance, the CPU frequency should be set to 128 MHz */
	err = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
	err -= NRFX_ERROR_BASE_NUM;
	if (err != 0) {
		LOG_WRN("Failed to set 128 MHz: %d", err);
	}

	LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock/MHZ(1));

	return err;
}

SYS_INIT(nrf53_cpu_boost, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
