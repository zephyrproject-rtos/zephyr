/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Independent Watchdog (IWDG) for STM32 MCUs
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 19: Independent watchdog (IWDG)
 *
 */

#include <watchdog.h>
#include <soc.h>
#include <errno.h>

#include "iwdg_stm32.h"

#define AS_IWDG(__base_addr) \
	(struct iwdg_stm32 *)(__base_addr)

static void iwdg_stm32_enable(struct device *dev)
{
	volatile struct iwdg_stm32 *iwdg = AS_IWDG(IWDG_BASE);

	ARG_UNUSED(dev);

	iwdg->kr.bit.key = STM32_IWDG_KR_START;
}

static void iwdg_stm32_disable(struct device *dev)
{
	/* watchdog cannot be stopped once started */
	ARG_UNUSED(dev);
}

static int iwdg_stm32_set_config(struct device *dev,
				struct wdt_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);

	/* no configuration */

	return -ENOTSUP;
}

static void iwdg_stm32_get_config(struct device *dev,
				struct wdt_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);
}

static void iwdg_stm32_reload(struct device *dev)
{
	volatile struct iwdg_stm32 *iwdg = AS_IWDG(IWDG_BASE);

	ARG_UNUSED(dev);

	iwdg->kr.bit.key = STM32_IWDG_KR_RELOAD;
}

static const struct wdt_driver_api iwdg_stm32_api = {
	.enable = iwdg_stm32_enable,
	.disable = iwdg_stm32_disable,
	.get_config = iwdg_stm32_get_config,
	.set_config = iwdg_stm32_set_config,
	.reload = iwdg_stm32_reload,
};

static inline int __iwdg_stm32_prescaler(int setting)
{
	int v;
	int i = 0;

	/* prescaler range 4 - 256 */
	for (v = 4; v < 256; v *= 2, i++) {
		if (v == setting)
			return i;
	}
	return i;
}

static int iwdg_stm32_init(struct device *dev)
{
	volatile struct iwdg_stm32 *iwdg = AS_IWDG(IWDG_BASE);

	/* clock setup is not required, once the watchdog is enabled
	 * LSI oscillator will be forced on and fed to IWD after
	 * stabilization period
	 */

	/* unlock access to configuration registers */
	iwdg->kr.bit.key = STM32_IWDG_KR_UNLOCK;

	iwdg->pr.bit.pr =
		__iwdg_stm32_prescaler(CONFIG_IWDG_STM32_PRESCALER);
	iwdg->rlr.bit.rl = CONFIG_IWDG_STM32_RELOAD_COUNTER;

#ifdef CONFIG_IWDG_STM32_START_AT_BOOT
	iwdg_stm32_enable(dev);
#endif

	return 0;
}

DEVICE_AND_API_INIT(iwdg_stm32, CONFIG_IWDG_STM32_DEVICE_NAME, iwdg_stm32_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &iwdg_stm32_api);
