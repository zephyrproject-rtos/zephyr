/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_ll_utils.h>
#include <stm32_ll_rcc.h>
#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

struct stm32_uid {
	uint32_t id[3];
};

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	struct stm32_uid dev_id;

	dev_id.id[0] = sys_cpu_to_be32(LL_GetUID_Word2());
	dev_id.id[1] = sys_cpu_to_be32(LL_GetUID_Word1());
	dev_id.id[2] = sys_cpu_to_be32(LL_GetUID_Word0());

	if (length > sizeof(dev_id.id)) {
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;

#if defined(RCC_FLAG_SFTRST)
	if (LL_RCC_IsActiveFlag_SFTRST()) {
		flags |= RESET_SOFTWARE;
	}
#endif
#if defined(RCC_FLAG_PINRST)
	if (LL_RCC_IsActiveFlag_PINRST()) {
		flags |= RESET_PIN;
	}
#endif
#if defined(RCC_FLAG_IWDGRST)
	if (LL_RCC_IsActiveFlag_IWDGRST()) {
		flags |= RESET_WATCHDOG;
	}
#endif
#if defined(RCC_RSR_IWDG1RSTF)
	if (LL_RCC_IsActiveFlag_IWDG1RST()) {
		flags |= RESET_WATCHDOG;
	}
#endif
#if defined(RCC_RSR_IWDG2RSTF)
	if (LL_RCC_IsActiveFlag_IWDG2RST()) {
		flags |= RESET_WATCHDOG;
	}
#endif
#if defined(RCC_FLAG_WWDGRST)
	if (LL_RCC_IsActiveFlag_WWDGRST()) {
		flags |= RESET_WATCHDOG;
	}
#endif
#if defined(RCC_RSR_WWDG1RSTF)
	if (LL_RCC_IsActiveFlag_WWDG1RST()) {
		flags |= RESET_WATCHDOG;
	}
#endif
#if defined(RCC_RSR_WWDG2RSTF)
	if (LL_RCC_IsActiveFlag_WWDG2RST()) {
		flags |= RESET_WATCHDOG;
	}
#endif
#if defined(RCC_FLAG_FWRST)
	if (LL_RCC_IsActiveFlag_FWRST()) {
		flags |= RESET_SECURITY;
	}
#endif
#if defined(RCC_FLAG_BORRST)
	if (LL_RCC_IsActiveFlag_BORRST()) {
		flags |= RESET_BROWNOUT;
	}
#endif
#if defined(RCC_FLAG_PWRRST)
	if (LL_RCC_IsActiveFlag_PWRRST()) {
		flags |= RESET_POR;
	}
#endif
#if defined(RCC_FLAG_PORRST)
	if (LL_RCC_IsActiveFlag_PORRST()) {
		flags |= RESET_POR;
	}
#endif
#if defined(RCC_FLAG_LPWRRST)
	if (LL_RCC_IsActiveFlag_LPWRRST()) {
		flags |= RESET_LOW_POWER_WAKE;
	}
#endif

	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	LL_RCC_ClearResetFlags();

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_PIN
		      | RESET_WATCHDOG
		      | RESET_SOFTWARE
		      | RESET_SECURITY
		      | RESET_LOW_POWER_WAKE
		      | RESET_POR
		      | RESET_BROWNOUT);

	return 0;
}
