/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WATCHDOG_IWDG_STM32_H_
#define ZEPHYR_DRIVERS_WATCHDOG_IWDG_STM32_H_

#include <zephyr/types.h>

/**
 * @brief Driver for Independent Watchdog (IWDG) for STM32 MCUs
 *
 * The driver targets all STM32 SoCs. For details please refer to
 * an appropriate reference manual and look for chapter called:
 *
 *   Independent watchdog (IWDG)
 *
 *   Notice that the STM32 IWDG cannot pause in sleep from software.
 *   However, some newer implementations have the IWDG_STDBY and
 *   IWDG_STOP bits in the flash option bytes. Check an appropriate reference manual
 *   whether any specific SoC has these option bits. When unset (0), these freeze the watchdog
 *   in standby or stop mode, respectively. The factory default is to keep the
 *   watchdog running in standby and stop mode (both bits are set (1)).
 *   Lastly, be aware that option bytes do not reset to factory
 *   defaults on a mass erase.
 *
 */

struct iwdg_stm32_config {
	/* IWDG peripheral instance. */
	IWDG_TypeDef *instance;
};

struct iwdg_stm32_data {
	uint32_t prescaler;
	uint32_t reload;
	/* IWDG user defined callback on EWI */
	wdt_callback_t callback;
};

#endif	/* ZEPHYR_DRIVERS_WATCHDOG_IWDG_STM32_H_ */
