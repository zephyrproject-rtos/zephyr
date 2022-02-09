/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <hal/nrf_gpio.h>

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#if defined(CONFIG_BUILD_WITH_TFM)
/* Use TF-M platform services */
#include "tfm_ioctl_api.h"
#include "hal/nrf_gpio.h"

static inline void soc_secure_gpio_pin_mcu_select(uint32_t pin_number, nrf_gpio_pin_mcusel_t mcu)
{
	uint32_t result;
	enum tfm_platform_err_t err;

	err = tfm_platform_gpio_pin_mcu_select(pin_number, mcu, &result);
	__ASSERT(err == TFM_PLATFORM_ERR_SUCCESS, "TFM platform error (%d)", err);
	__ASSERT(result == 0, "GPIO service error (%d)", result);
}

#if defined(CONFIG_SOC_HFXO_CAP_INTERNAL)
static inline uint32_t soc_secure_read_xosc32mtrim(void)
{
	uintptr_t ptr = (uintptr_t)&NRF_FICR_S->XOSC32MTRIM;
	enum tfm_platform_err_t err;
	uint32_t result;
	uint32_t xosc32mtrim;

	err = tfm_platform_mem_read(&xosc32mtrim, ptr, 4, &result);
	__ASSERT(err == TFM_PLATFORM_ERR_SUCCESS, "TFM platform error (%d)", err);
	__ASSERT(result == 0, "Read service error (%d)", result);

	return xosc32mtrim;
}
#endif /* defined(CONFIG_SOC_HFXO_CAP_INTERNAL) */
#else
#include <nrf.h>
/* Do this directly from secure processing environment. */
static inline void soc_secure_gpio_pin_mcu_select(uint32_t pin_number, nrf_gpio_pin_mcusel_t mcu)
{
	nrf_gpio_pin_mcu_select(pin_number, mcu);
}

static inline uint32_t soc_secure_read_xosc32mtrim(void)
{
	return NRF_FICR_S->XOSC32MTRIM;
}
#endif /* defined CONFIG_BUILD_WITH_TFM */
#endif /* defined(CONFIG_SOC_NRF5340_CPUAPP) */
