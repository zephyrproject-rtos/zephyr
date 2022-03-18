/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <nrf.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_ficr.h>

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#if defined(GPIO_PIN_CNF_MCUSEL_Msk)
void soc_secure_gpio_pin_mcu_select(uint32_t pin_number, nrf_gpio_pin_mcusel_t mcu);
#endif

int soc_secure_mem_read(void *dst, void *src, size_t len);

#if defined(CONFIG_SOC_HFXO_CAP_INTERNAL)
static inline uint32_t soc_secure_read_xosc32mtrim(void)
{
	uint32_t xosc32mtrim;
	int err;

	err = soc_secure_mem_read(&xosc32mtrim,
				  (void *)&NRF_FICR_S->XOSC32MTRIM,
				  sizeof(xosc32mtrim));
	__ASSERT(err == 0, "Secure read error (%d)", err);

	return xosc32mtrim;
}
#endif /* defined(CONFIG_SOC_HFXO_CAP_INTERNAL) */

#else /* defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) */
#if defined(GPIO_PIN_CNF_MCUSEL_Msk)
static inline void soc_secure_gpio_pin_mcu_select(uint32_t pin_number,
						  nrf_gpio_pin_mcusel_t mcu)
{
	nrf_gpio_pin_mcu_select(pin_number, mcu);
}
#endif /* defined(GPIO_PIN_CNF_MCUSEL_Msk) */

#if defined(CONFIG_SOC_HFXO_CAP_INTERNAL)
static inline uint32_t soc_secure_read_xosc32mtrim(void)
{
	return NRF_FICR_S->XOSC32MTRIM;
}
#endif /* defined(CONFIG_SOC_HFXO_CAP_INTERNAL) */
#endif /* defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) */
