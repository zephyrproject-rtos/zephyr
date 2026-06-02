/*
 * Copyright (c) 2022 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_ESP32_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_ESP32_PRIV_H_

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/esp-pinctrl-common.h>

#include <hal/emac_periph.h>
#include <soc/gpio_periph.h>
#include <soc/io_mux_reg.h>

/*
 * Find the IOMUX entry matching a specific GPIO number.
 * Returns the entry pointer, or the first entry if gpio_num < 0.
 */
static inline const emac_iomux_info_t *esp32_emac_iomux_find(const emac_iomux_info_t *info,
							     int gpio_num)
{
	if (info == NULL) {
		return NULL;
	}

	if (gpio_num < 0) {
		return info;
	}

	while (info->gpio_num != GPIO_NUM_MAX) {
		if (info->gpio_num == gpio_num) {
			return info;
		}
		info++;
	}

	return NULL;
}

static inline void esp32_emac_iomux_output(const emac_iomux_info_t *pin)
{
	if (pin == NULL) {
		return;
	}
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
}

static inline void esp32_emac_iomux_input(const emac_iomux_info_t *pin)
{
	if (pin == NULL) {
		return;
	}
	PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
}

/*
 * Initialize RMII data plane IOMUX pins. Each signal is routed to its
 * dedicated EMAC IOMUX pad; pass the desired GPIO per signal, or -1 to
 * use the first available pad.
 */
static inline void esp32_emac_iomux_init_rmii(int gpio_tx_en, int gpio_txd0, int gpio_txd1,
					      int gpio_crs_dv, int gpio_rxd0, int gpio_rxd1)
{
	esp32_emac_iomux_output(esp32_emac_iomux_find(emac_rmii_iomux_pins.tx_en, gpio_tx_en));
	esp32_emac_iomux_output(esp32_emac_iomux_find(emac_rmii_iomux_pins.txd0, gpio_txd0));
	esp32_emac_iomux_output(esp32_emac_iomux_find(emac_rmii_iomux_pins.txd1, gpio_txd1));
	esp32_emac_iomux_input(esp32_emac_iomux_find(emac_rmii_iomux_pins.crs_dv, gpio_crs_dv));
	esp32_emac_iomux_input(esp32_emac_iomux_find(emac_rmii_iomux_pins.rxd0, gpio_rxd0));
	esp32_emac_iomux_input(esp32_emac_iomux_find(emac_rmii_iomux_pins.rxd1, gpio_rxd1));
}

/*
 * Read the RMII GPIOs from the pinctrl-0 "default" state and route them
 * through the dedicated EMAC IOMUX. RMII pads bypass the GPIO matrix, so
 * pinctrl_apply_state() is not used; the cells only carry (slot, GPIO)
 * pairs (see ESP32_RMII_PINMUX). REF_CLK is returned via clk_gpio.
 */
static inline int esp32_emac_iomux_init_rmii_pinctrl(const struct pinctrl_dev_config *pcfg,
						     int *clk_gpio)
{
	const struct pinctrl_state *state;
	int gpios[ESP_RMII_SLOT_COUNT];
	int res;

	BUILD_ASSERT(ESP_RMII_SLOT_COUNT == ESP_RMII_RXD1 + 1,
		     "ESP_RMII_SLOT_COUNT out of sync with the ESP_RMII_* slots");

	for (int i = 0; i < ESP_RMII_SLOT_COUNT; i++) {
		gpios[i] = -1;
	}

	res = pinctrl_lookup_state(pcfg, PINCTRL_STATE_DEFAULT, &state);
	if (res != 0) {
		return res;
	}

	for (uint8_t i = 0; i < state->pin_cnt; i++) {
		uint32_t cell = state->pins[i].pinmux;

		if ((cell & ESP32_RMII_MARKER) == 0) {
			continue;
		}

		uint32_t slot = (cell >> ESP32_RMII_SLOT_SHIFT) & ESP32_RMII_SLOT_MASK;
		int gpio = (cell >> ESP32_PIN_NUM_SHIFT) & ESP32_PIN_NUM_MASK;

		if (slot < ARRAY_SIZE(gpios)) {
			gpios[slot] = gpio;
		}
	}

	esp32_emac_iomux_init_rmii(gpios[ESP_RMII_TX_EN], gpios[ESP_RMII_TXD0],
				   gpios[ESP_RMII_TXD1], gpios[ESP_RMII_CRS_DV],
				   gpios[ESP_RMII_RXD0], gpios[ESP_RMII_RXD1]);

	if (clk_gpio != NULL) {
		*clk_gpio = gpios[ESP_RMII_CLK];
	}

	return 0;
}

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_ESP32_PRIV_H_ */
