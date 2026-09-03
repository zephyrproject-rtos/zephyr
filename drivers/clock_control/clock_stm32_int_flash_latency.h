/*
 * Copyright 2026 Gowtham Palanichamy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Flash wait-state setter. Unconditionally includable from
 *        clock_stm32_ll_common.c: provides stm32_set_flash_latency()
 *        either way.
 *
 * Opt-in `st,flash-latency` on the flash node sets the wait-state count
 * directly (see the DT binding), bypassing the vendor VOS-only
 * LL_SetFlashLatency() helper -- untrusted beyond a field-width bound,
 * since LL_FLASH_SetLatency() doesn't mask before OR-ing into FLASH_ACR.
 */
#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_STM32_INT_FLASH_LATENCY_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_STM32_INT_FLASH_LATENCY_H_

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>
#include <stm32_ll_system.h>
#include <stm32_ll_utils.h>

#if defined(CONFIG_SOC_SERIES_STM32F4X) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(flash), st_flash_latency)

#define STM32_FLASH_LATENCY_DT DT_PROP(DT_NODELABEL(flash), st_flash_latency)

BUILD_ASSERT(STM32_FLASH_LATENCY_DT <= (FLASH_ACR_LATENCY >> FLASH_ACR_LATENCY_Pos),
	     "st,flash-latency exceeds this SoC's FLASH_ACR LATENCY field width");

/* Opt-in: wait-state count taken directly from devicetree. */
static inline void stm32_set_flash_latency(uint32_t freq)
{
	ARG_UNUSED(freq);
	LL_FLASH_SetLatency(STM32_FLASH_LATENCY_DT);
	while (LL_FLASH_GetLatency() != STM32_FLASH_LATENCY_DT) {
	}
}

#else /* !(CONFIG_SOC_SERIES_STM32F4X && st,flash-latency on flash) */

/* Default: unchanged, VOS-only vendor helper. */
static inline void stm32_set_flash_latency(uint32_t freq)
{
	LL_SetFlashLatency(freq);
}

#endif

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_STM32_INT_FLASH_LATENCY_H_ */
