/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 BayLibre, SAS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_FLASH_STM32_H
#define DRIVERS_FLASH_STM32_H

#include <flash_registers.h>

#if defined(CONFIG_SOC_SERIES_STM32F4X)
#include <flash_map.h>
#endif

#if defined(CONFIG_SOC_SERIES_STM32L4X)
#include <clock_control.h>
#include <clock_control/stm32_clock_control.h>
#endif

struct flash_stm32_priv {
#if defined(CONFIG_SOC_SERIES_STM32F4X)
	struct stm32f4x_flash *regs;
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
	struct stm32l4x_flash *regs;
	/* clock subsystem driving this peripheral */
	struct stm32_pclken pclken;
#endif
	struct k_sem sem;
};

#define FLASH_STM32_PRIV(dev) ((struct flash_stm32_priv *)((dev)->driver_data))
#define FLASH_STM32_REGS(dev) (FLASH_STM32_PRIV(dev)->regs)

bool flash_stm32_valid_range(struct device *dev, off_t offset,
			     u32_t len, bool write);

int flash_stm32_write_range(struct device *dev, unsigned int offset,
			    const void *data, unsigned int len);

int flash_stm32_block_erase_loop(struct device *dev, unsigned int offset,
				 unsigned int len);

int flash_stm32_wait_flash_idle(struct device *dev);


#endif /* DRIVERS_FLASH_STM32_H */
