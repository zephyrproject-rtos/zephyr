/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMEBAD_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMEBAD_CLOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Realtek Amebad clock Devicetree bindings
 */

/**
 * @name AON domain clocks
 * @{
 */

/** ATIM clock in AON domain */
#define AMEBA_ATIM_CLK 0

/** RTC clock in AON domain */
#define AMEBA_RTC_CLK  1

/** @} */

/**
 * @name SYSON domain clocks
 * @{
 */

/** PWM0 clock in SYSON domain */
#define AMEBA_PWM0_CLK    2

/** UART0 clock in SYSON domain */
#define AMEBA_UART0_CLK   3

/** LOGUART clock in SYSON domain */
#define AMEBA_LOGUART_CLK 4

/** UART3 clock in SYSON domain */
#define AMEBA_UART3_CLK   5

/** ADC clock in SYSON domain */
#define AMEBA_ADC_CLK     6

/** GPIO clock in SYSON domain */
#define AMEBA_GPIO_CLK    7

/** LTIM0 clock in SYSON domain */
#define AMEBA_LTIM0_CLK   8

/** LTIM1 clock in SYSON domain */
#define AMEBA_LTIM1_CLK   9

/** LTIM2 clock in SYSON domain */
#define AMEBA_LTIM2_CLK   10

/** LTIM3 clock in SYSON domain */
#define AMEBA_LTIM3_CLK   11

/** @} */

/**
 * @name SoC domain clocks
 * @{
 */

/** Peripheral HCLK in SoC domain */
#define AMEBA_PERI_HCLK 12

/** GDMA0 clock in SoC domain */
#define AMEBA_GDMA0_CLK 13

/** SPI0 clock in SoC domain */
#define AMEBA_SPI0_CLK  14

/** SPI1 clock in SoC domain */
#define AMEBA_SPI1_CLK  15

/** Flash clock in SoC domain */
#define AMEBA_FLASH_CLK 16

/** PSRAM clock in SoC domain */
#define AMEBA_PSRAM_CLK 17

/** I2C0 clock in SoC domain */
#define AMEBA_I2C0_CLK  18

/** PRNG clock in SoC domain */
#define AMEBA_PRNG_CLK  19

/**
 * @brief Maximum clock index (one past the last valid index).
 */
#define AMEBA_CLK_MAX 20 /* clk idx max */

/** @} */

/**
 * @name Peripheral clock helper macros
 * @{
 */

/**
 * @brief Define a clock entry for a peripheral with numerical suffix.
 *
 * Used for peripherals with an index, for example SPI0, SPI1, UART0.
 *
 * @param name Peripheral base name
 * @param n    Peripheral index
 */
#define AMEBA_NUMERICAL_PERIPH(name, n)                                                    \
	[AMEBA_##name##n##_CLK] = {                                                        \
		.parent = AMEBA_RCC_NO_PARENT,                                             \
		.cke = APBPeriph_##name##n##_CLOCK,                                        \
		.fen = APBPeriph_##name##n,                                                \
	},

/**
 * @brief Define a clock entry for a single-instance peripheral.
 *
 * Used for peripherals that have only one instance, for example GDMA0,
 * PSRAM, RTC.
 *
 * @param name Peripheral name
 */
#define AMEBA_SINGLE_PERIPH(name)                                                          \
	[AMEBA_##name##_CLK] = {                                                           \
		.parent = AMEBA_RCC_NO_PARENT,                                             \
		.cke = APBPeriph_##name##_CLOCK,                                           \
		.fen = APBPeriph_##name,                                                   \
	},

/**
 * @brief Define a clock entry with a remapped peripheral name.
 *
 * Used when a clock index is mapped to another peripheral name.
 *
 * @param clk_index Clock index
 * @param remap_name Target peripheral name
 */
#define AMEBA_REMAP_PERIPH(clk_index, remap_name)                                          \
	[clk_index] = {                                                                    \
		.parent = AMEBA_RCC_NO_PARENT,                                             \
		.cke = APBPeriph_##remap_name##_CLOCK,                                     \
		.fen = APBPeriph_##remap_name,                                             \
	},

/**
 * @brief LTIM clock mappings to GTIMER peripheral.
 */
#define AMEBA_LTIM_PERIPHS                                                                 \
	AMEBA_REMAP_PERIPH(AMEBA_LTIM0_CLK, GTIMER)                                        \
	AMEBA_REMAP_PERIPH(AMEBA_LTIM1_CLK, GTIMER)                                        \
	AMEBA_REMAP_PERIPH(AMEBA_LTIM2_CLK, GTIMER)                                        \
	AMEBA_REMAP_PERIPH(AMEBA_LTIM3_CLK, GTIMER)

/**
 * @brief PWM clock mapping to GTIMER peripheral.
 */
#define AMEBA_PWM_PERIPHS AMEBA_REMAP_PERIPH(AMEBA_PWM0_CLK, GTIMER)

/**
 * @brief SPI clock peripheral mappings.
 */
#define AMEBA_SPI_PERIPHS                                                                  \
	AMEBA_NUMERICAL_PERIPH(SPI, 0) /* AMEBA_SPI0_CLK */                                \
	AMEBA_NUMERICAL_PERIPH(SPI, 1) /* AMEBA_SPI1_CLK */

/**
 * @brief I2C clock peripheral mappings.
 */
#define AMEBA_I2C_PERIPHS AMEBA_NUMERICAL_PERIPH(I2C, 0) /* AMEBA_I2C0_CLK */

/**
 * @brief UART clock peripheral mappings.
 */
#define AMEBA_UART_PERIPHS                                                                 \
	AMEBA_NUMERICAL_PERIPH(UART, 0) /* AMEBA_UART0_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(UART, 3) /* AMEBA_UART3_CLK */

/**
 * @brief GDMA0 clock peripheral mapping.
 */
#define AMEBA_GDMA0_PERIPHS AMEBA_SINGLE_PERIPH(GDMA0) /* AMEBA_GDMA0_CLK */

/**
 * @brief PSRAM clock peripheral mapping.
 */
#define AMEBA_PSRAM_PERIPHS AMEBA_SINGLE_PERIPH(PSRAM) /* AMEBA_PSRAM_CLK */

/**
 * @brief RTC clock peripheral mapping.
 */
#define AMEBA_RTC_PERIPHS   AMEBA_SINGLE_PERIPH(RTC)   /* AMEBA_RTC_CLK */

/**
 * @brief LOGUART clock peripheral mapping.
 *
 * TODO: Enabled in KM0.
 */
#define AMEBA_LOGUART_PERIPHS AMEBA_REMAP_PERIPH(AMEBA_LOGUART_CLK, NULL)

/**
 * @brief Flash clock peripheral mapping.
 */
#define AMEBA_FLASH_PERIPHS   AMEBA_REMAP_PERIPH(AMEBA_FLASH_CLK, NULL)

/**
 * @brief GPIO clock peripheral mapping.
 */
#define AMEBA_GPIO_PERIPHS    AMEBA_REMAP_PERIPH(AMEBA_GPIO_CLK, NULL)

/**
 * @brief ADC clock peripheral mapping.
 */
#define AMEBA_ADC_PERIPHS     AMEBA_REMAP_PERIPH(AMEBA_ADC_CLK, NULL)

/**
 * @brief PRNG clock peripheral mapping.
 */
#define AMEBA_PRNG_PERIPHS    AMEBA_REMAP_PERIPH(AMEBA_PRNG_CLK, NULL)

/**
 * @brief Null APB peripheral clock definition.
 *
 * Used as a placeholder for an invalid or unused APB clock.
 */
#define APBPeriph_NULL_CLOCK APBPeriph_CLOCK_NULL

/**
 * @brief Aggregated core peripheral clock mappings.
 *
 * This macro expands to mappings of all core peripherals used by
 * the clock control implementation.
 */
#define AMEBA_CORE_PERIPHS                                                                 \
	AMEBA_RTC_PERIPHS                                                                  \
	AMEBA_PWM_PERIPHS                                                                  \
	AMEBA_UART_PERIPHS                                                                 \
	AMEBA_LOGUART_PERIPHS                                                              \
	AMEBA_ADC_PERIPHS                                                                  \
	AMEBA_GPIO_PERIPHS                                                                 \
	AMEBA_LTIM_PERIPHS                                                                 \
	AMEBA_GDMA0_PERIPHS                                                                \
	AMEBA_SPI_PERIPHS                                                                  \
	AMEBA_FLASH_PERIPHS                                                                \
	AMEBA_I2C_PERIPHS                                                                  \
	AMEBA_PRNG_PERIPHS

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMEBAD_CLOCK_H_ */
