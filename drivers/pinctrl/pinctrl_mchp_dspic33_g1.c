/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/pinctrl.h>
#include <pinctrl_soc.h>

#define DT_DRV_COMPAT microchip_dspic33_pinctrl


#define MCHP_DSPIC_GET_PORT_ADDR_OR_NONE(nodelabel)                                        \
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),                                        \
		   (DT_REG_ADDR(DT_NODELABEL(nodelabel))))

static int pinctrl_configure_pin(pinctrl_soc_pin_t soc_pin)
{
	volatile uint32_t port;
	volatile uint32_t pin;
	volatile uint32_t func;
	int ret = 0;

	/* GPIO port addresses */
	static const uint32_t gpios[] = {
		MCHP_DSPIC_GET_PORT_ADDR_OR_NONE(gpioa),
		MCHP_DSPIC_GET_PORT_ADDR_OR_NONE(gpiob),
		MCHP_DSPIC_GET_PORT_ADDR_OR_NONE(gpioc),
		MCHP_DSPIC_GET_PORT_ADDR_OR_NONE(gpiod),
#if defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK512MPS512)
		MCHP_DSPIC_GET_PORT_ADDR_OR_NONE(gpioe),
		MCHP_DSPIC_GET_PORT_ADDR_OR_NONE(gpiof),
		MCHP_DSPIC_GET_PORT_ADDR_OR_NONE(gpiog),
		MCHP_DSPIC_GET_PORT_ADDR_OR_NONE(gpioh),
#endif
	};

	port = DSPIC33_PINMUX_PORT(soc_pin.pinmux);
	pin = DSPIC33_PINMUX_PIN(soc_pin.pinmux);
	func = DSPIC33_PINMUX_FUNC(soc_pin.pinmux);

	if (gpios[port] != 0U) {

		if ((func & 0xFF00U) == 0U) {
			/* Output Remappable functionality pins */
			volatile uint32_t reg_shift =
				((pin  < 4U) ? (0U) : (pin / 4U));
			volatile uint32_t reg_index = pin % 4U;
			volatile uint32_t RPORx_BASE = gpios[0] + OFFSET_RPOR;

			func = (func << (reg_index * 8u));
			volatile uint32_t *RPORx =
				(void *)(RPORx_BASE + (port * 0x10U) + (reg_shift * 4U));
			*RPORx |= func;

			/* Setting latch bit */
			volatile uint32_t *latch = (void *)(gpios[port] + OFFSET_LATCH);
			*latch |= (1U << pin);

			/* Setting TRIS bit*/
			volatile uint32_t *tris = (void *)(gpios[port] + OFFSET_TRIS);
			*tris &= ~(1U << pin);

			/* setting ANSEl bit */
#if defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK128MC106)
			if ((port == PORT_A) || (port == PORT_B)) {
				volatile uint32_t *ansel =
					(void *)(gpios[0] + OFFSET_ANSEL + (port * 0x24U));
				*ansel &= ~(1U << pin);
			}
#elif defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK512MPS512)
				volatile uint32_t *ansel =
					(void *)(gpios[0] + OFFSET_ANSEL + (port * 0x24U));
				*ansel &= ~(1U << pin);
#endif
		} else {
			/* Input Remappable functionality pins */
			volatile int pin_rpin = (port * 0x10U) + pin + 1U;
			volatile uint8_t *RPINx = (void *)(DSPIC33_PINMUX_FUNC(soc_pin.pinmux));
			*RPINx |= pin_rpin;

			/* setting ANSEl bit */
#if defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK128MC106)
			if ((port == PORT_A) || (port == PORT_B)) {
				volatile uint32_t *ansel =
					(void *)(gpios[0] + OFFSET_ANSEL + (port * 0x24U));
				*ansel &= ~(1U << pin);
			}
#elif defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK512MPS512)
			volatile uint32_t *ansel =
				(void *)(gpios[0] + OFFSET_ANSEL + (port * 0x24U));
			*ansel &= ~(1U << pin);
#endif
		}
	} else {
		ret = -EINVAL;
	}

	return ret;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	int ret = 0;

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		ret = pinctrl_configure_pin(pins[i]);
		if (ret) {
			break;
		}
	}

	return ret;
}
