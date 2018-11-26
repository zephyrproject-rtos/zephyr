/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <entropy.h>
#include <init.h>
#include <soc.h>
#include <crc32.h>
#include <errno.h>
#include <string.h>

#if !defined(CONFIG_SOC_SERIES_STM32F0X) && \
	!defined(CONFIG_SOC_SERIES_STM32F3X) && \
	!defined(SOC_SERIES_STM32L4X) && \
	!defined(CONFIG_SOC_SERIES_STM32F7X)
#error UNIQUE ID only available on STM32F0, STM32F3, STM32l4 and STM32F7 series
#endif

static int entropy_pseudo_stm32_init(struct device *dev)
{
	return 0;
}

static int entropy_pseudo_stm32_get_entropy(struct device *dev,
						   u8_t *buffer,
						   u16_t length)
{
	u32_t dev_id[4];

	/*
	 * This function is only used to seed xoroshiro128 which only use a
	 * length of 4 bytes
	 */
	if (length != 16) {
		return -ENOTSUP;
	}

	dev_id[0] = HAL_GetUIDw0();
	dev_id[1] = HAL_GetUIDw1();
	dev_id[2] = HAL_GetUIDw2();
	dev_id[3] = 0;

	memcpy(buffer, dev_id, length);

	return 0;
}

static inline int entropy_pseudo_stm32_get_entropy_isr(struct device *dev,
					  u8_t *buffer,
					  u16_t length,
					  u32_t flags)
{
	return entropy_pseudo_stm32_get_entropy(dev, buffer, length);
}

static const struct entropy_driver_api entropy_pseudo_stm32_api = {
	.get_entropy = entropy_pseudo_stm32_get_entropy,
	.get_entropy_isr = entropy_pseudo_stm32_get_entropy_isr
};

DEVICE_AND_API_INIT(entropy_pseudo_stm32, CONFIG_ENTROPY_NAME,
		    entropy_pseudo_stm32_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &entropy_pseudo_stm32_api);
