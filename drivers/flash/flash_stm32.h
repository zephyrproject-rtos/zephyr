/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 BayLibre, SAS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_STM32_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_STM32_H_

#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
#include <drivers/clock_control.h>
#include <drivers/clock_control/stm32_clock_control.h>
#endif

struct flash_stm32_priv {
	FLASH_TypeDef *regs;
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	/* clock subsystem driving this peripheral */
	struct stm32_pclken pclken;
#endif
	struct k_sem sem;
};

#define FLASH_STM32_PRIV(dev) ((struct flash_stm32_priv *)((dev)->driver_data))
#define FLASH_STM32_REGS(dev) (FLASH_STM32_PRIV(dev)->regs)

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static inline bool flash_stm32_range_exists(struct device *dev,
					    off_t offset,
					    uint32_t len)
{
	struct flash_pages_info info;

	return !(flash_get_page_info_by_offs(dev, offset, &info) ||
		 flash_get_page_info_by_offs(dev, offset + len - 1, &info));
}
#endif	/* CONFIG_FLASH_PAGE_LAYOUT */

bool flash_stm32_valid_range(struct device *dev, off_t offset,
			     uint32_t len, bool write);

int flash_stm32_write_range(struct device *dev, unsigned int offset,
			    const void *data, unsigned int len);

int flash_stm32_block_erase_loop(struct device *dev, unsigned int offset,
				 unsigned int len);

int flash_stm32_wait_flash_idle(struct device *dev);

#ifdef CONFIG_SOC_SERIES_STM32WBX
int flash_stm32_check_status(struct device *dev);
#endif /* CONFIG_SOC_SERIES_STM32WBX */

#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_stm32_page_layout(struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size);
#endif

#endif /* ZEPHYR_DRIVERS_FLASH_FLASH_STM32_H_ */
