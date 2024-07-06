/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_ll_utils.h>
#include <stm32_ll_rcc.h>
#if defined(CONFIG_SOC_SERIES_STM32H5X)
#include <stm32_ll_icache.h>
#endif /* CONFIG_SOC_SERIES_STM32H5X */
#include <stm32_ll_pwr.h>
#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

struct stm32_uid {
	uint32_t id[3];
};

int z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	struct stm32_uid dev_id;

#if defined(CONFIG_SOC_SERIES_STM32H5X)
	LL_ICACHE_Disable();
#endif /* CONFIG_SOC_SERIES_STM32H5X */

	dev_id.id[0] = sys_cpu_to_be32(LL_GetUID_Word2());
	dev_id.id[1] = sys_cpu_to_be32(LL_GetUID_Word1());
	dev_id.id[2] = sys_cpu_to_be32(LL_GetUID_Word0());

#if defined(CONFIG_SOC_SERIES_STM32H5X)
	LL_ICACHE_Enable();
#endif /* CONFIG_SOC_SERIES_STM32H5X */

	if (length > sizeof(dev_id.id)) {
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}

#if defined(CONFIG_SOC_SERIES_STM32WBAX) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
struct stm32_eui64 {
	uint32_t id[2];
};

int z_impl_hwinfo_get_device_eui64(uint8_t *buffer)
{
	struct stm32_eui64 dev_eui64;

	dev_eui64.id[0] = sys_cpu_to_be32(READ_REG(*((uint32_t *)UID64_BASE + 1U)));
	dev_eui64.id[1] = sys_cpu_to_be32(READ_REG(*((uint32_t *)UID64_BASE)));

	memcpy(buffer, dev_eui64.id, sizeof(dev_eui64));

	return 0;
}
#endif

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

#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CORE_CM4)
	if (LL_PWR_CPU2_IsActiveFlag_SB()) {
		flags |= RESET_LOW_POWER_WAKE;
	}
#elif defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CORE_CM7)
	if (LL_PWR_CPU_IsActiveFlag_SB()) {
		flags |= RESET_LOW_POWER_WAKE;
	}
#elif defined(CONFIG_SOC_SERIES_STM32MP1X)
	if (LL_PWR_MCU_IsActiveFlag_SB()) {
		flags |= RESET_LOW_POWER_WAKE;
	}
#elif defined(CONFIG_SOC_SERIES_STM32WLX) || defined(CONFIG_SOC_SERIES_STM32WBX)
	if (LL_PWR_IsActiveFlag_C1SB()) {
		flags |= RESET_LOW_POWER_WAKE;
	}
#elif defined(PWR_FLAG_SB) || defined(PWR_FLAG_SBF)
	if (LL_PWR_IsActiveFlag_SB()) {
		flags |= RESET_LOW_POWER_WAKE;
	}
#endif /* PWR_FLAG_SB */

	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	LL_RCC_ClearResetFlags();

#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CORE_CM4)
	LL_PWR_ClearFlag_CPU2();
#elif defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CORE_CM7)
	LL_PWR_ClearFlag_CPU();
#elif defined(CONFIG_SOC_SERIES_STM32H7RSX)
	LL_PWR_ClearFlag_STOP_SB();
#elif defined(CONFIG_SOC_SERIES_STM32MP1X)
	LL_PWR_ClearFlag_MCU();
#elif defined(CONFIG_SOC_SERIES_STM32WLX) || defined(CONFIG_SOC_SERIES_STM32WBX)
	LL_PWR_ClearFlag_C1STOP_C1STB();
#elif defined(PWR_FLAG_SB)
	LL_PWR_ClearFlag_SB();
#endif /* PWR_FLAG_SB */

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
