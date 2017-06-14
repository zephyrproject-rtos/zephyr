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

bool flash_stm32_valid_range(off_t offset, u32_t len, bool write);

int flash_stm32_write_range(unsigned int offset, const void *data,
			    unsigned int len, struct flash_stm32_priv *p);

int flash_stm32_block_erase_loop(unsigned int offset, unsigned int len,
				 struct flash_stm32_priv *p);

void flash_stm32_flush_caches(struct flash_stm32_priv *p);

int flash_stm32_wait_flash_idle(struct flash_stm32_priv *p);

int flash_stm32_check_status(struct flash_stm32_priv *p);

int flash_stm32_erase(struct device *dev, off_t offset, size_t len);

int flash_stm32_write(struct device *dev, off_t offset,
		      const void *data, size_t len);

#endif /* DRIVERS_FLASH_STM32_H */
