/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NRF_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NRF_PINCTRL_H_

/*
 * The whole nRF pin configuration information is encoded in a 32-bit bitfield
 * organized as follows:
 *
 * - 31..24: Pin function.
 * - 19-23:  Reserved.
 * - 18:     Associated peripheral belongs to GD FAST ACTIVE1 (nRF54H only)
 * - 17:     Clockpin enable.
 * - 16:     Pin inversion mode.
 * - 15:     Pin low power mode.
 * - 14..11: Pin output drive configuration.
 * - 10..9:  Pin pull configuration.
 * - 8..0:   Pin number (combination of port and pin).
 */

/**
 * @name nRF pin configuration bit field positions and masks.
 * @{
 */

/** Position of the function field. */
#define NRF_FUN_POS 24U
/** Mask for the function field. */
#define NRF_FUN_MSK 0xFFU
/** Position of the GPD FAST ACTIVE1 */
#define NRF_GPD_FAST_ACTIVE1_POS 18U
/** Mask for the GPD FAST ACTIVE1 */
#define NRF_GPD_FAST_ACTIVE1_MSK 0x1U
/** Position of the clockpin enable field. */
#define NRF_CLOCKPIN_ENABLE_POS 17U
/** Mask for the clockpin enable field. */
#define NRF_CLOCKPIN_ENABLE_MSK 0x1U
/** Position of the invert field. */
#define NRF_INVERT_POS 16U
/** Mask for the invert field. */
#define NRF_INVERT_MSK 0x1U
/** Position of the low power field. */
#define NRF_LP_POS 15U
/** Mask for the low power field. */
#define NRF_LP_MSK 0x1U
/** Position of the drive configuration field. */
#define NRF_DRIVE_POS 11U
/** Mask for the drive configuration field. */
#define NRF_DRIVE_MSK 0xFU
/** Position of the pull configuration field. */
#define NRF_PULL_POS 9U
/** Mask for the pull configuration field. */
#define NRF_PULL_MSK 0x3U
/** Position of the pin field. */
#define NRF_PIN_POS 0U
/** Mask for the pin field. */
#define NRF_PIN_MSK 0x1FFU

/** @} */

/**
 * @name nRF pinctrl pin functions.
 * @{
 */

/** UART TX */
#define NRF_FUN_UART_TX 0U
/** UART RX */
#define NRF_FUN_UART_RX 1U
/** UART RTS */
#define NRF_FUN_UART_RTS 2U
/** UART CTS */
#define NRF_FUN_UART_CTS 3U
/** SPI master SCK */
#define NRF_FUN_SPIM_SCK 4U
/** SPI master MOSI */
#define NRF_FUN_SPIM_MOSI 5U
/** SPI master MISO */
#define NRF_FUN_SPIM_MISO 6U
/** SPI slave SCK */
#define NRF_FUN_SPIS_SCK 7U
/** SPI slave MOSI */
#define NRF_FUN_SPIS_MOSI 8U
/** SPI slave MISO */
#define NRF_FUN_SPIS_MISO 9U
/** SPI slave CSN */
#define NRF_FUN_SPIS_CSN 10U
/** TWI master SCL */
#define NRF_FUN_TWIM_SCL 11U
/** TWI master SDA */
#define NRF_FUN_TWIM_SDA 12U
/** I2S SCK in master mode */
#define NRF_FUN_I2S_SCK_M 13U
/** I2S SCK in slave mode */
#define NRF_FUN_I2S_SCK_S 14U
/** I2S LRCK in master mode */
#define NRF_FUN_I2S_LRCK_M 15U
/** I2S LRCK in slave mode */
#define NRF_FUN_I2S_LRCK_S 16U
/** I2S SDIN */
#define NRF_FUN_I2S_SDIN 17U
/** I2S SDOUT */
#define NRF_FUN_I2S_SDOUT 18U
/** I2S MCK */
#define NRF_FUN_I2S_MCK 19U
/** PDM CLK */
#define NRF_FUN_PDM_CLK 20U
/** PDM DIN */
#define NRF_FUN_PDM_DIN 21U
/** PWM OUT0 */
#define NRF_FUN_PWM_OUT0 22U
/** PWM OUT1 */
#define NRF_FUN_PWM_OUT1 23U
/** PWM OUT2 */
#define NRF_FUN_PWM_OUT2 24U
/** PWM OUT3 */
#define NRF_FUN_PWM_OUT3 25U
/** QDEC A */
#define NRF_FUN_QDEC_A 26U
/** QDEC B */
#define NRF_FUN_QDEC_B 27U
/** QDEC LED */
#define NRF_FUN_QDEC_LED 28U
/** QSPI SCK */
#define NRF_FUN_QSPI_SCK 29U
/** QSPI CSN */
#define NRF_FUN_QSPI_CSN 30U
/** QSPI IO0 */
#define NRF_FUN_QSPI_IO0 31U
/** QSPI IO1 */
#define NRF_FUN_QSPI_IO1 32U
/** QSPI IO2 */
#define NRF_FUN_QSPI_IO2 33U
/** QSPI IO3 */
#define NRF_FUN_QSPI_IO3 34U
/** EXMIF CK */
#define NRF_FUN_EXMIF_CK 35U
/** EXMIF DQ0 */
#define NRF_FUN_EXMIF_DQ0 36U
/** EXMIF DQ1 */
#define NRF_FUN_EXMIF_DQ1 37U
/** EXMIF DQ2 */
#define NRF_FUN_EXMIF_DQ2 38U
/** EXMIF DQ3 */
#define NRF_FUN_EXMIF_DQ3 39U
/** EXMIF DQ4 */
#define NRF_FUN_EXMIF_DQ4 40U
/** EXMIF DQ5 */
#define NRF_FUN_EXMIF_DQ5 41U
/** EXMIF DQ6 */
#define NRF_FUN_EXMIF_DQ6 42U
/** EXMIF DQ7 */
#define NRF_FUN_EXMIF_DQ7 43U
/** EXMIF CS0 */
#define NRF_FUN_EXMIF_CS0 44U
/** EXMIF CS1 */
#define NRF_FUN_EXMIF_CS1 45U
/** CAN TX */
#define NRF_FUN_CAN_TX 46U
/** CAN RX */
#define NRF_FUN_CAN_RX 47U
/** TWIS SCL */
#define NRF_FUN_TWIS_SCL 48U
/** TWIS SDA */
#define NRF_FUN_TWIS_SDA 49U
/** EXMIF RWDS */
#define NRF_FUN_EXMIF_RWDS 50U
/** GRTC fast clock output */
#define NRF_FUN_GRTC_CLKOUT_FAST 55U
/** GRTC slow clock output */
#define NRF_FUN_GRTC_CLKOUT_32K  56U

/** @} */

/**
 * @name nRF pinctrl output drive.
 * @{
 */

/** Standard '0', standard '1'. */
#define NRF_DRIVE_S0S1 0U
/** High drive '0', standard '1'. */
#define NRF_DRIVE_H0S1 1U
/** Standard '0', high drive '1'. */
#define NRF_DRIVE_S0H1 2U
/** High drive '0', high drive '1'. */
#define NRF_DRIVE_H0H1 3U
/** Disconnect '0' standard '1'. */
#define NRF_DRIVE_D0S1 4U
/** Disconnect '0', high drive '1'. */
#define NRF_DRIVE_D0H1 5U
/** Standard '0', disconnect '1'. */
#define NRF_DRIVE_S0D1 6U
/** High drive '0', disconnect '1'. */
#define NRF_DRIVE_H0D1 7U
/** Extra high drive '0', extra high drive '1'. */
#define NRF_DRIVE_E0E1 8U

/** @} */

/**
 * @name nRF pinctrl pull-up/down.
 * @note Values match nrf_gpio_pin_pull_t constants.
 * @{
 */

/** Pull-up disabled. */
#define NRF_PULL_NONE 0U
/** Pull-down enabled. */
#define NRF_PULL_DOWN 1U
/** Pull-up enabled. */
#define NRF_PULL_UP 3U

/** @} */

/**
 * @name nRF pinctrl low power mode.
 * @{
 */

/** Input. */
#define NRF_LP_DISABLE 0U
/** Output. */
#define NRF_LP_ENABLE 1U

/** @} */

/**
 * @name nRF pinctrl helpers to indicate disconnected pins.
 * @{
 */

/** Indicates that a pin is disconnected */
#define NRF_PIN_DISCONNECTED NRF_PIN_MSK

/** @} */

/**
 * @brief Utility macro to build nRF psels property entry.
 *
 * @param fun Pin function configuration (see NRF_FUNC_{name} macros).
 * @param port Port (0 or 15).
 * @param pin Pin (0..31).
 */
#define NRF_PSEL(fun, port, pin)						       \
	((((((port) * 32U) + (pin)) & NRF_PIN_MSK) << NRF_PIN_POS) |		       \
	 ((NRF_FUN_ ## fun & NRF_FUN_MSK) << NRF_FUN_POS))

/**
 * @brief Utility macro to build nRF psels property entry when a pin is disconnected.
 *
 * This can be useful in situations where code running before Zephyr, e.g. a bootloader
 * configures pins that later needs to be disconnected.
 *
 * @param fun Pin function configuration (see NRF_FUN_{name} macros).
 */
#define NRF_PSEL_DISCONNECTED(fun)						       \
	(NRF_PIN_DISCONNECTED |							       \
	 ((NRF_FUN_ ## fun & NRF_FUN_MSK) << NRF_FUN_POS))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NRF_PINCTRL_H_ */
