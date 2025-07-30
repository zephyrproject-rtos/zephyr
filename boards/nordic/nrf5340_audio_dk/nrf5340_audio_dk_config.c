/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <hal/nrf_gpiote.h>

LOG_MODULE_REGISTER(nrf5340_audio_dk_nrf5340_cpuapp, CONFIG_LOG_DEFAULT_LEVEL);

void board_late_init_hook(void)
{
	nrf_gpiote_latency_t latency;

	latency = nrf_gpiote_latency_get(NRF_GPIOTE);

	if (latency != NRF_GPIOTE_LATENCY_LOWPOWER) {
		LOG_DBG("Setting gpiote latency to low power");
		nrf_gpiote_latency_set(NRF_GPIOTE, NRF_GPIOTE_LATENCY_LOWPOWER);
	}
}
