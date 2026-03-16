/*
 * Copyright (c) 2022 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_ESP32_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_ESP32_PRIV_H_

#include <hal/emac_periph.h>
#include <soc/gpio_periph.h>
#include <soc/io_mux_reg.h>

/*
 * ESP32 EMAC uses dedicated IOMUX pins, not GPIO matrix.
 * Only PIN_FUNC_SELECT is needed to route signals.
 */
static inline void esp32_emac_iomux_init_rmii(void)
{
	const emac_iomux_info_t *pin;

	pin = emac_rmii_iomux_pins.tx_en;
	if (pin != NULL) {
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	pin = emac_rmii_iomux_pins.txd0;
	if (pin != NULL) {
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	pin = emac_rmii_iomux_pins.txd1;
	if (pin != NULL) {
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	pin = emac_rmii_iomux_pins.crs_dv;
	if (pin != NULL) {
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	pin = emac_rmii_iomux_pins.rxd0;
	if (pin != NULL) {
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	pin = emac_rmii_iomux_pins.rxd1;
	if (pin != NULL) {
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}
}

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_ESP32_PRIV_H_ */
