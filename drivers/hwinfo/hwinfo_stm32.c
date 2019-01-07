/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <hwinfo.h>
#include <string.h>

#if !defined(CONFIG_SOC_SERIES_STM32F0X) && \
	!defined(CONFIG_SOC_SERIES_STM32F3X) && \
	!defined(CONFIG_SOC_SERIES_STM32L4X) && \
	!defined(CONFIG_SOC_SERIES_STM32F7X)
#error ID only available on STM32F0, STM32F3, STM32L4 and STM32F7 series
#endif

struct stm32_uid {
	u32_t id[3];
};

ssize_t _impl_hwinfo_get_device_id(u8_t *buffer, size_t length)
{
	struct stm32_uid dev_id;

	dev_id.id[0] = HAL_GetUIDw0();
	dev_id.id[1] = HAL_GetUIDw1();
	dev_id.id[2] = HAL_GetUIDw2();

	if (length > sizeof(dev_id.id)) {
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}
