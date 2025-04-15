/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>

#include <hal/nrf_gpio.h>

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_i2s) || defined(CONFIG_NRFX_I2S)
#include <hal/nrf_i2s.h>
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pdm) || defined(CONFIG_NRFX_PDM)
#include <hal/nrf_pdm.h>
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pwm) || defined(CONFIG_NRFX_PWM)
#include <hal/nrf_pwm.h>
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_qdec) || defined(CONFIG_NRFX_QDEC)
#include <hal/nrf_qdec.h>
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_qspi) || defined(CONFIG_NRFX_QSPI)
#include <hal/nrf_qspi.h>
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_spi) || defined(CONFIG_NRFX_SPI)
#include <hal/nrf_spi.h>
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_spim) || defined(CONFIG_NRFX_SPIM)
#include <hal/nrf_spim.h>
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_spis) || defined(CONFIG_NRFX_SPIS)
#include <hal/nrf_spis.h>
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_twi) || defined(CONFIG_NRFX_TWI)
#include <hal/nrf_twi.h>
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_twim) || defined(CONFIG_NRFX_TWIM)
#include <hal/nrf_twim.h>
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_twis) || defined(CONFIG_NRFX_TWIS)
#include <hal/nrf_twis.h>
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_uart) || defined(CONFIG_NRFX_UART)
#include <hal/nrf_uart.h>
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_uarte) || defined(CONFIG_NRFX_UARTE)
#include <hal/nrf_uarte.h>
#endif

#ifdef CONFIG_SOC_NRF54H20_GPD
#include <nrf/gpd.h>
#endif

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

#define PSEL_DISCONNECTED UINT32_MAX


#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_grtc) || defined(CONFIG_NRFX_GRTC)
#if DT_NODE_HAS_PROP(DT_NODELABEL(grtc), clkout_fast_frequency_hz)
#define NRF_GRTC_CLKOUT_FAST 1
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(grtc), clkout_32k)
#define NRF_GRTC_CLKOUT_SLOW 1
#endif
#endif

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
#ifdef CONFIG_SOC_NRF54H20_GPD
	bool gpd_requested = false;
#endif

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
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_uart) || defined(CONFIG_NRFX_UART)
		case NRF_FUN_UART_TX:
			nrf_uart_tx_pin_set((NRF_UART_Type *)reg, psel);
			write = 1U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_UART_RX:
			nrf_uart_rx_pin_set((NRF_UART_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_UART_RTS:
			nrf_uart_rts_pin_set((NRF_UART_Type *)reg, psel);
			write = 1U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_UART_CTS:
			nrf_uart_cts_pin_set((NRF_UART_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_uarte) || defined(CONFIG_NRFX_UARTE)

		case NRF_FUN_UART_TX:
			nrf_uarte_tx_pin_set((NRF_UARTE_Type *)reg, psel);
			write = 1U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_UART_RX:
			nrf_uarte_rx_pin_set((NRF_UARTE_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_UART_RTS:
			nrf_uarte_rts_pin_set((NRF_UARTE_Type *)reg, psel);
			write = 1U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_UART_CTS:
			nrf_uarte_cts_pin_set((NRF_UARTE_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_spi) || defined(CONFIG_NRFX_SPI)

		case NRF_FUN_SPIM_SCK:
			nrf_spi_sck_pin_set((NRF_SPI_Type *)reg, psel);
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_SPIM_MOSI:
			nrf_spi_mosi_pin_set((NRF_SPI_Type *)reg, psel);
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_SPIM_MISO:
			nrf_spi_miso_pin_set((NRF_SPI_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_spim) || defined(CONFIG_NRFX_SPIM)

		case NRF_FUN_SPIM_SCK:
			nrf_spim_sck_pin_set((NRF_SPIM_Type *)reg, psel);
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_SPIM_MOSI:
			nrf_spim_mosi_pin_set((NRF_SPIM_Type *)reg, psel);
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_SPIM_MISO:
			nrf_spim_miso_pin_set((NRF_SPIM_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_spis) || defined(CONFIG_NRFX_SPIS)

		case NRF_FUN_SPIS_SCK:
			nrf_spis_sck_pin_set((NRF_SPIS_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_SPIS_MOSI:
			nrf_spis_mosi_pin_set((NRF_SPIS_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_SPIS_MISO:
			nrf_spis_miso_pin_set((NRF_SPIS_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_SPIS_CSN:
			nrf_spis_csn_pin_set((NRF_SPIS_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_spis) || defined(CONFIG_NRFX_SPIS) */
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_twi) || defined(CONFIG_NRFX_TWI)

		case NRF_FUN_TWIM_SCL:
			nrf_twi_scl_pin_set((NRF_TWI_Type *)reg, psel);
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
			nrf_twi_sda_pin_set((NRF_TWI_Type *)reg, psel);
			if (drive == NRF_GPIO_PIN_S0S1) {
				drive = NRF_GPIO_PIN_S0D1;
			}
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_twim) || defined(CONFIG_NRFX_TWIM)

		case NRF_FUN_TWIM_SCL:
			nrf_twim_scl_pin_set((NRF_TWIM_Type *)reg, psel);
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
			nrf_twim_sda_pin_set((NRF_TWIM_Type *)reg, psel);
			if (drive == NRF_GPIO_PIN_S0S1) {
				drive = NRF_GPIO_PIN_S0D1;
			}
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_i2s) || defined(CONFIG_NRFX_I2S)

		case NRF_FUN_I2S_SCK_M:
			nrf_i2s_sck_pin_set((NRF_I2S_Type *)reg, psel);
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_I2S_SCK_S:
			nrf_i2s_sck_pin_set((NRF_I2S_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_I2S_LRCK_M:
			nrf_i2s_lrck_pin_set((NRF_I2S_Type *)reg, psel);
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_I2S_LRCK_S:
			nrf_i2s_lrck_pin_set((NRF_I2S_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_I2S_SDIN:
			nrf_i2s_sdin_pin_set((NRF_I2S_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_I2S_SDOUT:
			nrf_i2s_sdout_pin_set((NRF_I2S_Type *)reg, psel);
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_I2S_MCK:
			nrf_i2s_mck_pin_set((NRF_I2S_Type *)reg, psel);
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_i2s) || defined(CONFIG_NRFX_I2S) */
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pdm) || defined(CONFIG_NRFX_PDM)

		case NRF_FUN_PDM_CLK:
			nrf_pdm_clk_pin_set((NRF_PDM_Type *)reg, psel);
			write = 0U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_PDM_DIN:
			nrf_pdm_din_pin_set((NRF_PDM_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pdm) || defined(CONFIG_NRFX_PDM) */
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pwm) || defined(CONFIG_NRFX_PWM)

		case NRF_FUN_PWM_OUT0:
			nrf_pwm_pin_set((NRF_PWM_Type *)reg, 0, psel);
			write = NRF_GET_INVERT(pins[i]);
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_PWM_OUT1:
			nrf_pwm_pin_set((NRF_PWM_Type *)reg, 1, psel);
			write = NRF_GET_INVERT(pins[i]);
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_PWM_OUT2:
			nrf_pwm_pin_set((NRF_PWM_Type *)reg, 2, psel);
			write = NRF_GET_INVERT(pins[i]);
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_PWM_OUT3:
			nrf_pwm_pin_set((NRF_PWM_Type *)reg, 3, psel);
			write = NRF_GET_INVERT(pins[i]);
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pwm) || defined(CONFIG_NRFX_PWM) */
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_qdec) || defined(CONFIG_NRFX_QDEC)

		case NRF_FUN_QDEC_A:
			nrf_qdec_phase_a_pin_set((NRF_QDEC_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_QDEC_B:
			nrf_qdec_phase_b_pin_set((NRF_QDEC_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_QDEC_LED:
			nrf_qdec_phase_led_pin_set((NRF_QDEC_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_qdec) || defined(CONFIG_NRFX_QDEC) */
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_qspi) || defined(CONFIG_NRFX_QSPI)

		case NRF_FUN_QSPI_SCK:
			nrf_qspi_pin_sck_set((NRF_QSPI_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_QSPI_CSN:
			nrf_qspi_pin_csn_set((NRF_QSPI_Type *)reg, psel);
			write = 1U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_QSPI_IO0:
			nrf_qspi_pin_io0_set((NRF_QSPI_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_QSPI_IO1:
			nrf_qspi_pin_io1_set((NRF_QSPI_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_QSPI_IO2:
			nrf_qspi_pin_io2_set((NRF_QSPI_Type *)reg, psel);
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
		case NRF_FUN_QSPI_IO3:
			nrf_qspi_pin_io3_set((NRF_QSPI_Type *)reg, psel);
			write = 1U;
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_qspi) || defined(CONFIG_NRFX_QSPI) */
#if defined(NRF_GRTC_CLKOUT_FAST)
		case NRF_FUN_GRTC_CLKOUT_FAST:
#if NRF_GPIO_HAS_SEL && defined(GPIO_PIN_CNF_CTRLSEL_GRTC)
			nrf_gpio_pin_control_select(psel, NRF_GPIO_PIN_SEL_GRTC);
#endif
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
#endif /* defined(NRF_GRTC_CLKOUT_FAST) */
#if defined(NRF_GRTC_CLKOUT_SLOW)
		case NRF_FUN_GRTC_CLKOUT_32K:
#if NRF_GPIO_HAS_SEL && defined(GPIO_PIN_CNF_CTRLSEL_GRTC)
			nrf_gpio_pin_control_select(psel, NRF_GPIO_PIN_SEL_GRTC);
#endif
			dir = NRF_GPIO_PIN_DIR_OUTPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
#endif /* defined(NRF_GRTC_CLKOUT_SLOW) */
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
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_exmif)
		/* Pin routing is controlled by secure domain, via UICR */
		case NRF_FUN_EXMIF_CK:
		case NRF_FUN_EXMIF_DQ0:
		case NRF_FUN_EXMIF_DQ1:
		case NRF_FUN_EXMIF_DQ2:
		case NRF_FUN_EXMIF_DQ3:
		case NRF_FUN_EXMIF_DQ4:
		case NRF_FUN_EXMIF_DQ5:
		case NRF_FUN_EXMIF_DQ6:
		case NRF_FUN_EXMIF_DQ7:
		case NRF_FUN_EXMIF_CS0:
		case NRF_FUN_EXMIF_CS1:
		case NRF_FUN_EXMIF_RWDS:
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_DISCONNECT;
			break;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_exmif) */
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_twis) || defined(CONFIG_NRFX_TWIS)

		case NRF_FUN_TWIS_SCL:
			nrf_twis_scl_pin_set((NRF_TWIS_Type *)reg, psel);
			if (drive == NRF_GPIO_PIN_S0S1) {
				drive = NRF_GPIO_PIN_S0D1;
			}
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
		case NRF_FUN_TWIS_SDA:
			nrf_twis_sda_pin_set((NRF_TWIS_Type *)reg, psel);
			if (drive == NRF_GPIO_PIN_S0S1) {
				drive = NRF_GPIO_PIN_S0D1;
			}
			dir = NRF_GPIO_PIN_DIR_INPUT;
			input = NRF_GPIO_PIN_INPUT_CONNECT;
			break;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_twis) || defined(CONFIG_NRFX_TWIS) */
		default:
			return -ENOTSUP;
		}

		/* configure GPIO properties */
		if (psel != PSEL_DISCONNECTED) {
			uint32_t pin = psel;

#ifdef CONFIG_SOC_NRF54H20_GPD
			if (NRF_GET_GPD_FAST_ACTIVE1(pins[i]) == 1U) {
				if (!gpd_requested) {
					int ret;

					ret = nrf_gpd_request(NRF_GPD_SLOW_ACTIVE);
					if (ret < 0) {
						return ret;
					}
					gpd_requested = true;
				}

				nrf_gpio_pin_retain_disable(pin);
			}
#endif /* CONFIG_SOC_NRF54H20_GPD */

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
#ifdef CONFIG_SOC_NRF54H20_GPD
			if (NRF_GET_GPD_FAST_ACTIVE1(pins[i]) == 1U) {
				nrf_gpio_pin_retain_enable(pin);
			}
#endif /* CONFIG_SOC_NRF54H20_GPD */
		}
	}

#ifdef CONFIG_SOC_NRF54H20_GPD
	if (gpd_requested) {
		int ret;

		ret = nrf_gpd_release(NRF_GPD_SLOW_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}
#endif

	return 0;
}
