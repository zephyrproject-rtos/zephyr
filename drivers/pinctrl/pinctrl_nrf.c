/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>

#include <hal/nrf_gpio.h>

BUILD_ASSERT(((NRF_PULL_NONE == NRF_GPIO_PIN_NOPULL) &&
	      (NRF_PULL_DOWN == NRF_GPIO_PIN_PULLDOWN) &&
	      (NRF_PULL_UP == NRF_GPIO_PIN_PULLUP)),
	      "nRF pinctrl pull settings do not match HAL values");

#if defined(GPIO_PIN_CNF_DRIVE_E0E1) || defined(GPIO_PIN_CNF_DRIVE0_E0)
#define NRF_DRIVE_COUNT (NRF_DRIVE_E0E1 + 1)
#else
#define NRF_DRIVE_COUNT (NRF_DRIVE_H0D1 + 1)
#endif
static const nrf_gpio_pin_drive_t drive_modes[NRF_DRIVE_COUNT] = {
	[NRF_DRIVE_S0S1] = NRF_GPIO_PIN_S0S1,
	[NRF_DRIVE_H0S1] = NRF_GPIO_PIN_H0S1,
	[NRF_DRIVE_S0H1] = NRF_GPIO_PIN_S0H1,
	[NRF_DRIVE_H0H1] = NRF_GPIO_PIN_H0H1,
	[NRF_DRIVE_D0S1] = NRF_GPIO_PIN_D0S1,
	[NRF_DRIVE_D0H1] = NRF_GPIO_PIN_D0H1,
	[NRF_DRIVE_S0D1] = NRF_GPIO_PIN_S0D1,
	[NRF_DRIVE_H0D1] = NRF_GPIO_PIN_H0D1,
#if defined(GPIO_PIN_CNF_DRIVE_E0E1) || defined(GPIO_PIN_CNF_DRIVE0_E0)
	[NRF_DRIVE_E0E1] = NRF_GPIO_PIN_E0E1,
#endif
};

/* value to indicate pin level doesn't need initialization */
#define NO_WRITE UINT32_MAX

#define PSEL_DISCONNECTED 0xFFFFFFFFUL

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_uart) || defined(CONFIG_NRFX_UART)
#define NRF_PSEL_UART(reg, line) ((NRF_UART_Type *)reg)->PSEL##line
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_uarte) || defined(CONFIG_NRFX_UARTE)
#include <hal/nrf_uarte.h>
#define NRF_PSEL_UART(reg, line) ((NRF_UARTE_Type *)reg)->PSEL.line
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_spi) || defined(CONFIG_NRFX_SPI)
#define NRF_PSEL_SPIM(reg, line) ((NRF_SPI_Type *)reg)->PSEL##line
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_spim) || defined(CONFIG_NRFX_SPIM)
#include <hal/nrf_spim.h>
#define NRF_PSEL_SPIM(reg, line) ((NRF_SPIM_Type *)reg)->PSEL.line
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_spis) || defined(CONFIG_NRFX_SPIS)
#include <hal/nrf_spis.h>
#if defined(NRF51)
#define NRF_PSEL_SPIS(reg, line) ((NRF_SPIS_Type *)reg)->PSEL##line
#else
#define NRF_PSEL_SPIS(reg, line) ((NRF_SPIS_Type *)reg)->PSEL.line
#endif
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_twi) || defined(CONFIG_NRFX_TWI)
#if !defined(TWI_PSEL_SCL_CONNECT_Pos)
#define NRF_PSEL_TWIM(reg, line) ((NRF_TWI_Type *)reg)->PSEL##line
#else
#define NRF_PSEL_TWIM(reg, line) ((NRF_TWI_Type *)reg)->PSEL.line
#endif
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_twim) || defined(CONFIG_NRFX_TWIM)
#include <hal/nrf_twim.h>
#define NRF_PSEL_TWIM(reg, line) ((NRF_TWIM_Type *)reg)->PSEL.line
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_i2s) || defined(CONFIG_NRFX_I2S)
#define NRF_PSEL_I2S(reg, line) ((NRF_I2S_Type *)reg)->PSEL.line
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pdm) || defined(CONFIG_NRFX_PDM)
#define NRF_PSEL_PDM(reg, line) ((NRF_PDM_Type *)reg)->PSEL.line
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pwm) || defined(CONFIG_NRFX_PWM)
#define NRF_PSEL_PWM(reg, line) ((NRF_PWM_Type *)reg)->PSEL.line
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_qdec) || defined(CONFIG_NRFX_QDEC)
#define NRF_PSEL_QDEC(reg, line) ((NRF_QDEC_Type *)reg)->PSEL.line
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_qspi) || defined(CONFIG_NRFX_QSPI)
#define NRF_PSEL_QSPI(reg, line) ((NRF_QSPI_Type *)reg)->PSEL.line
#endif

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		nrf_gpio_pin_drive_t drive;
		uint8_t drive_idx = NRF_GET_DRIVE(pins[i]);
		uint32_t psel = NRF_GET_PIN(pins[i]);
		uint32_t write = NO_WRITE;
		nrf_gpio_pin_dir_t dir;
		nrf_gpio_pin_input_t input;

		if (drive_idx < ARRAY_SIZE(drive_modes)) {
			drive = drive_modes[drive_idx];
		} else {
			return -EINVAL;
		}

		if (psel == NRF_PIN_DISCONNECTED) {
			psel = PSEL_DISCONNECTED;
		}

		switch (NRF_GET_FUN(pins[i])) {
#if defined(NRF_PSEL_UART)
		case NRF_FUN_UART_TX:
			NRF_PSEL_UART(reg, TXD) = psel;
			write = 1U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_UART_RX:
			NRF_PSEL_UART(reg, RXD) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_UART_RTS:
			NRF_PSEL_UART(reg, RTS) = psel;
			write = 1U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_UART_CTS:
			NRF_PSEL_UART(reg, CTS) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif /* defined(NRF_PSEL_UART) */
#if defined(NRF_PSEL_SPIM)
		case NRF_FUN_SPIM_SCK:
			NRF_PSEL_SPIM(reg, SCK) = psel;
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_SPIM_MOSI:
			NRF_PSEL_SPIM(reg, MOSI) = psel;
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_SPIM_MISO:
			NRF_PSEL_SPIM(reg, MISO) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif /* defined(NRF_PSEL_SPIM) */
#if defined(NRF_PSEL_SPIS)
		case NRF_FUN_SPIS_SCK:
			NRF_PSEL_SPIS(reg, SCK) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_SPIS_MOSI:
			NRF_PSEL_SPIS(reg, MOSI) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_SPIS_MISO:
			NRF_PSEL_SPIS(reg, MISO) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_SPIS_CSN:
			NRF_PSEL_SPIS(reg, CSN) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif /* defined(NRF_PSEL_SPIS) */
#if defined(NRF_PSEL_TWIM)
		case NRF_FUN_TWIM_SCL:
			NRF_PSEL_TWIM(reg, SCL) = psel;
			if (drive == NRF_GPIO_PIN_S0S1) {
				/* Override the default drive setting with one
				 * suitable for TWI/TWIM peripherals (S0D1).
				 * This drive cannot be used always so that
				 * users are able to select e.g. H0D1 or E0E1
				 * in devicetree.
				 */
				drive = NRF_GPIO_PIN_S0D1;
			}
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_TWIM_SDA:
			NRF_PSEL_TWIM(reg, SDA) = psel;
			if (drive == NRF_GPIO_PIN_S0S1) {
				drive = NRF_GPIO_PIN_S0D1;
			}
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif /* defined(NRF_PSEL_TWIM) */
#if defined(NRF_PSEL_I2S)
		case NRF_FUN_I2S_SCK_M:
			NRF_PSEL_I2S(reg, SCK) = psel;
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_I2S_SCK_S:
			NRF_PSEL_I2S(reg, SCK) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_I2S_LRCK_M:
			NRF_PSEL_I2S(reg, LRCK) = psel;
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_I2S_LRCK_S:
			NRF_PSEL_I2S(reg, LRCK) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_I2S_SDIN:
			NRF_PSEL_I2S(reg, SDIN) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_I2S_SDOUT:
			NRF_PSEL_I2S(reg, SDOUT) = psel;
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_I2S_MCK:
			NRF_PSEL_I2S(reg, MCK) = psel;
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
#endif /* defined(NRF_PSEL_I2S) */
#if defined(NRF_PSEL_PDM)
		case NRF_FUN_PDM_CLK:
			NRF_PSEL_PDM(reg, CLK) = psel;
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_PDM_DIN:
			NRF_PSEL_PDM(reg, DIN) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif /* defined(NRF_PSEL_PDM) */
#if defined(NRF_PSEL_PWM)
		case NRF_FUN_PWM_OUT0:
			NRF_PSEL_PWM(reg, OUT[0]) = psel;
			write = NRF_GET_INVERT(pins[i]);
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_PWM_OUT1:
			NRF_PSEL_PWM(reg, OUT[1]) = psel;
			write = NRF_GET_INVERT(pins[i]);
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_PWM_OUT2:
			NRF_PSEL_PWM(reg, OUT[2]) = psel;
			write = NRF_GET_INVERT(pins[i]);
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_PWM_OUT3:
			NRF_PSEL_PWM(reg, OUT[3]) = psel;
			write = NRF_GET_INVERT(pins[i]);
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
#endif /* defined(NRF_PSEL_PWM) */
#if defined(NRF_PSEL_QDEC)
		case NRF_FUN_QDEC_A:
			NRF_PSEL_QDEC(reg, A) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_QDEC_B:
			NRF_PSEL_QDEC(reg, B) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_QDEC_LED:
			NRF_PSEL_QDEC(reg, LED) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif /* defined(NRF_PSEL_QDEC) */
#if defined(NRF_PSEL_QSPI)
		case NRF_FUN_QSPI_SCK:
			NRF_PSEL_QSPI(reg, SCK) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_QSPI_CSN:
			NRF_PSEL_QSPI(reg, CSN) = psel;
			write = 1U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_QSPI_IO0:
			NRF_PSEL_QSPI(reg, IO0) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_QSPI_IO1:
			NRF_PSEL_QSPI(reg, IO1) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_QSPI_IO2:
			NRF_PSEL_QSPI(reg, IO2) = psel;
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_QSPI_IO3:
			NRF_PSEL_QSPI(reg, IO3) = psel;
			write = 1U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
#endif /* defined(NRF_PSEL_QSPI) */
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_can)
		/* Pin routing is controlled by secure domain, via UICR */
		case NRF_FUN_CAN_TX:
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_CAN_RX:
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_can) */
		default:
			return -ENOTSUP;
		}

		/* configure GPIO properties */
		if (psel != PSEL_DISCONNECTED) {
			uint32_t pin = psel;

			if (write != NO_WRITE) {
				nrf_gpio_pin_write(pin, write);
			}

			/* force input and disconnected buffer for low power */
			if (NRF_GET_LP(pins[i]) == NRF_LP_ENABLE) {
				dir = NRF_GPIO_PIN_DIR_INPUT;
				input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			}

			nrf_gpio_cfg(pin, dir, input, NRF_GET_PULL(pins[i]),
				     drive, NRF_GPIO_PIN_NOSENSE);
#if NRF_GPIO_HAS_CLOCKPIN
			nrf_gpio_pin_clock_set(pin, NRF_GET_CLOCKPIN_ENABLE(pins[i]));
#endif
		}
	}

	return 0;
}
