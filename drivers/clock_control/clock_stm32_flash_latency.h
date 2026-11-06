/*
 * Copyright (c) 2026 Rapyuta Robotics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_STM32_FLASH_LATENCY_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_STM32_FLASH_LATENCY_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <stm32_ll_system.h>

/*
 * Opt-in `st,flash-latency` on the flash controller node sets the
 * wait-state count directly (see the DT binding), bypassing the vendor
 * algorithm's LL_SetFlashLatency() helper. CONCAT()-ing the value onto
 * LL_FLASH_LATENCY_ resolves to the matching enum constant, or fails to
 * compile if that constant isn't defined for the target SoC -- each
 * part's own vendored header only defines the enum values its real
 * FLASH_ACR LATENCY field width can hold, so this is a free, per-part
 * bounds check with no separate table or assert needed.
 */
#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_flash_controller), st_flash_latency)

#define STM32_FLASH_LATENCY_DT \
	CONCAT(LL_FLASH_LATENCY_, DT_PROP(DT_INST(0, st_stm32_flash_controller), st_flash_latency))

static inline void stm32_set_flash_latency(uint32_t freq)
{
	ARG_UNUSED(freq);
	LL_FLASH_SetLatency(STM32_FLASH_LATENCY_DT);
	while (LL_FLASH_GetLatency() != STM32_FLASH_LATENCY_DT) {
	}
}

#else /* !st,flash-latency on the flash controller */

static inline void stm32_set_flash_latency(uint32_t freq)
{
	LL_SetFlashLatency(freq);
}

#endif

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_STM32_FLASH_LATENCY_H_ */
