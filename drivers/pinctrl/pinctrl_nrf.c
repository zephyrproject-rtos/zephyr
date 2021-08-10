/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pinctrl.h>

#include <hal/nrf_gpio.h>

BUILD_ASSERT(((NRF_PULL_NONE == NRF_GPIO_PIN_NOPULL) &&
	      (NRF_PULL_DOWN == NRF_GPIO_PIN_PULLDOWN) &&
	      (NRF_PULL_UP == NRF_GPIO_PIN_PULLUP)),
	      "nRF pinctrl pull settings do not match HAL values");

BUILD_ASSERT(((NRF_DRIVE_S0S1 == NRF_GPIO_PIN_S0S1) &&
	      (NRF_DRIVE_H0S1 == NRF_GPIO_PIN_H0S1) &&
	      (NRF_DRIVE_S0H1 == NRF_GPIO_PIN_S0H1) &&
	      (NRF_DRIVE_H0H1 == NRF_GPIO_PIN_H0H1) &&
	      (NRF_DRIVE_D0S1 == NRF_GPIO_PIN_D0S1) &&
	      (NRF_DRIVE_D0H1 == NRF_GPIO_PIN_D0H1) &&
	      (NRF_DRIVE_S0D1 == NRF_GPIO_PIN_S0D1) &&
	      (NRF_DRIVE_H0D1 == NRF_GPIO_PIN_H0D1) &&
#if defined(GPIO_PIN_CNF_DRIVE_E0E1)
	      (NRF_DRIVE_E0E1 == NRF_GPIO_PIN_E0E1) &&
#endif /* defined(GPIO_PIN_CNF_DRIVE_E0E1) */
	      (1U)),
	     "nRF pinctrl drive settings do not match HAL values");

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_uart)
#define NRF_PSEL_UART(reg, line) ((NRF_UART_Type *)reg)->PSEL##line
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_uarte)
#define NRF_PSEL_UART(reg, line) ((NRF_UARTE_Type *)reg)->PSEL.line
#endif

/**
 * @brief Configure pin settings.
 *
 * @param pin Pin configuration.
 * @param dir Pin direction.
 * @param input Pin input buffer connection.
 */
__unused static void nrf_pin_configure(pinctrl_soc_pin_t pin,
				       nrf_gpio_pin_dir_t dir,
				       nrf_gpio_pin_input_t input)
{
	/* force input direction and disconnected buffer for low power */
	if (NRF_GET_LP(pin) == NRF_LP_ENABLE) {
		dir = NRF_GPIO_PIN_DIR_INPUT;
		input = NRF_GPIO_PIN_INPUT_DISCONNECT;
	}

	nrf_gpio_cfg(NRF_GET_PIN(pin), dir, input, NRF_GET_PULL(pin),
		     NRF_GET_DRIVE(pin), NRF_GPIO_PIN_NOSENSE);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		switch (NRF_GET_FUN(pins[i])) {
#if defined(NRF_PSEL_UART)
		case NRF_FUN_UART_TX:
			NRF_PSEL_UART(reg, TXD) = NRF_GET_PIN(pins[i]);
			nrf_gpio_pin_write(NRF_GET_PIN(pins[i]), 1);
			nrf_pin_configure(pins[i], NRF_GPIO_PIN_DIR_OUTPUT,
					  NRF_GPIO_PIN_INPUT_DISCONNECT);
			break;
		case NRF_FUN_UART_RX:
			NRF_PSEL_UART(reg, RXD) = NRF_GET_PIN(pins[i]);
			nrf_pin_configure(pins[i], NRF_GPIO_PIN_DIR_INPUT,
					  NRF_GPIO_PIN_INPUT_CONNECT);
			break;
		case NRF_FUN_UART_RTS:
			NRF_PSEL_UART(reg, RTS) = NRF_GET_PIN(pins[i]);
			nrf_gpio_pin_write(NRF_GET_PIN(pins[i]), 1);
			nrf_pin_configure(pins[i], NRF_GPIO_PIN_DIR_OUTPUT,
					  NRF_GPIO_PIN_INPUT_DISCONNECT);
			break;
		case NRF_FUN_UART_CTS:
			NRF_PSEL_UART(reg, CTS) = NRF_GET_PIN(pins[i]);
			nrf_pin_configure(pins[i], NRF_GPIO_PIN_DIR_INPUT,
					  NRF_GPIO_PIN_INPUT_CONNECT);
			break;
#endif /* defined(NRF_PSEL_UART) */
		default:
			return -ENOTSUP;
		}
	}

	return 0;
}
