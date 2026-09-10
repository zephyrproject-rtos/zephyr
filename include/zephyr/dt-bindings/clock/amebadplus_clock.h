/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMEBADPLUS_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMEBADPLUS_CLOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Realtek Amebadplus clock Devicetree bindings
 */

/**
 * @name AON domain clocks
 * @{
 */

/** ATIM clock in AON domain */
#define AMEBA_ATIM_CLK 1

/** RTC clock in AON domain */
#define AMEBA_RTC_CLK  2

/** @} */

/**
 * @name SYSON domain clocks
 * @{
 */

/** PWM0 clock in SYSON domain */
#define AMEBA_PWM0_CLK    3

/** PWM1 clock in SYSON domain */
#define AMEBA_PWM1_CLK    4

/** HTIM0 clock in SYSON domain */
#define AMEBA_HTIM0_CLK   5

/** HTIM1 clock in SYSON domain */
#define AMEBA_HTIM1_CLK   6

/** LEDC clock in SYSON domain */
#define AMEBA_LEDC_CLK    7

/** UART0 clock in SYSON domain */
#define AMEBA_UART0_CLK   8

/** UART1 clock in SYSON domain */
#define AMEBA_UART1_CLK   9

/** UART2 clock in SYSON domain */
#define AMEBA_UART2_CLK   10

/** LOGUART clock in SYSON domain */
#define AMEBA_LOGUART_CLK 11

/** DTIM clock in SYSON domain */
#define AMEBA_DTIM_CLK    12

/** ADC clock in SYSON domain */
#define AMEBA_ADC_CLK     13

/** GPIO clock in SYSON domain */
#define AMEBA_GPIO_CLK    14

/** LTIM0 clock in SYSON domain */
#define AMEBA_LTIM0_CLK   15

/** LTIM1 clock in SYSON domain */
#define AMEBA_LTIM1_CLK   16

/** LTIM2 clock in SYSON domain */
#define AMEBA_LTIM2_CLK   17

/** LTIM3 clock in SYSON domain */
#define AMEBA_LTIM3_CLK   18

/** LTIM4 clock in SYSON domain */
#define AMEBA_LTIM4_CLK   19

/** LTIM5 clock in SYSON domain */
#define AMEBA_LTIM5_CLK   20

/** LTIM6 clock in SYSON domain */
#define AMEBA_LTIM6_CLK   21

/** LTIM7 clock in SYSON domain */
#define AMEBA_LTIM7_CLK   22

/** PTIM0 clock in SYSON domain */
#define AMEBA_PTIM0_CLK   23

/** PTIM1 clock in SYSON domain */
#define AMEBA_PTIM1_CLK   24

/** KSCAN clock in SYSON domain */
#define AMEBA_KSCAN_CLK   25

/** @} */

/**
 * @name SoC domain clocks
 * @{
 */

/** DMAC clock in SoC domain */
#define AMEBA_DMAC_CLK   26

/** SDIO clock in SoC domain */
#define AMEBA_SDIO_CLK   27

/** SPI0 clock in SoC domain */
#define AMEBA_SPI0_CLK   28

/** SPI1 clock in SoC domain */
#define AMEBA_SPI1_CLK   29

/** USB clock in SoC domain */
#define AMEBA_USB_CLK    30

/** Flash clock in SoC domain */
#define AMEBA_FLASH_CLK  31

/** PSRAM clock in SoC domain */
#define AMEBA_PSRAM_CLK  32

/** SPORT0 clock in SoC domain */
#define AMEBA_SPORT0_CLK 33

/** SPORT1 clock in SoC domain */
#define AMEBA_SPORT1_CLK 34

/** AC clock in SoC domain */
#define AMEBA_AC_CLK     35

/** IRDA clock in SoC domain */
#define AMEBA_IRDA_CLK   36

/** I2C0 clock in SoC domain */
#define AMEBA_I2C0_CLK   37

/** I2C1 clock in SoC domain */
#define AMEBA_I2C1_CLK   38

/** TRNG clock in SoC domain */
#define AMEBA_TRNG_CLK   39

/** @} */

/**
 * @name Misc clocks
 * @{
 */

/** BTON clock */
#define AMEBA_BTON_CLK 40

/** AES clock (misc domain) */
#define AMEBA_AES_CLK  42

/**
 * @brief Maximum clock index (one past the last valid index).
 */
#define AMEBA_CLK_MAX 43 /* clk idx max */

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
 * Used for peripherals that have only one instance, for example DMAC,
 * SDIO, USB, TRNG, etc.
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
 * @brief LTIM clock peripheral mappings.
 */
#define AMEBA_LTIM_PERIPHS                                                                 \
	AMEBA_NUMERICAL_PERIPH(LTIM, 0) /* AMEBA_LTIM0_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(LTIM, 1) /* AMEBA_LTIM1_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(LTIM, 2) /* AMEBA_LTIM2_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(LTIM, 3) /* AMEBA_LTIM3_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(LTIM, 4) /* AMEBA_LTIM4_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(LTIM, 5) /* AMEBA_LTIM5_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(LTIM, 6) /* AMEBA_LTIM6_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(LTIM, 7) /* AMEBA_LTIM7_CLK */

/**
 * @brief PTIM clock peripheral mappings.
 */
#define AMEBA_PTIM_PERIPHS                                                                 \
	AMEBA_NUMERICAL_PERIPH(PTIM, 0) /* AMEBA_PTIM0_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(PTIM, 1) /* AMEBA_PTIM1_CLK */

/**
 * @brief SPI clock peripheral mappings.
 */
#define AMEBA_SPI_PERIPHS                                                                  \
	AMEBA_NUMERICAL_PERIPH(SPI, 0) /* AMEBA_SPI0_CLK */                                \
	AMEBA_NUMERICAL_PERIPH(SPI, 1) /* AMEBA_SPI1_CLK */

/**
 * @brief SPORT clock peripheral mappings.
 */
#define AMEBA_SPORT_PERIPHS                                                                \
	AMEBA_NUMERICAL_PERIPH(SPORT, 0) /* AMEBA_SPORT0_CLK */                            \
	AMEBA_NUMERICAL_PERIPH(SPORT, 1) /* AMEBA_SPORT1_CLK */

/**
 * @brief I2C clock peripheral mappings.
 */
#define AMEBA_I2C_PERIPHS                                                                  \
	AMEBA_NUMERICAL_PERIPH(I2C, 0) /* AMEBA_I2C0_CLK */                                \
	AMEBA_NUMERICAL_PERIPH(I2C, 1) /* AMEBA_I2C1_CLK */

/**
 * @brief PWM clock peripheral mappings.
 */
#define AMEBA_PWM_PERIPHS                                                                  \
	AMEBA_NUMERICAL_PERIPH(PWM, 0) /* AMEBA_PWM0_CLK */                                \
	AMEBA_NUMERICAL_PERIPH(PWM, 1) /* AMEBA_PWM1_CLK */

/**
 * @brief HTIM clock peripheral mappings.
 */
#define AMEBA_HTIM_PERIPHS                                                                 \
	AMEBA_NUMERICAL_PERIPH(HTIM, 0) /* AMEBA_HTIM0_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(HTIM, 1) /* AMEBA_HTIM1_CLK */

/**
 * @brief UART clock peripheral mappings.
 */
#define AMEBA_UART_PERIPHS                                                                 \
	AMEBA_NUMERICAL_PERIPH(UART, 0) /* AMEBA_UART0_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(UART, 1) /* AMEBA_UART1_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(UART, 2) /* AMEBA_UART2_CLK */

/**
 * @brief LOGUART clock peripheral mapping.
 */
#define AMEBA_LOGUART_PERIPHS AMEBA_SINGLE_PERIPH(LOGUART) /* AMEBA_LOGUART_CLK */

/**
 * @brief KSCAN clock peripheral mapping.
 */
#define AMEBA_KSCAN_PERIPHS   AMEBA_SINGLE_PERIPH(KSCAN)   /* AMEBA_KSCAN_CLK */

/**
 * @brief DMAC clock peripheral mapping.
 */
#define AMEBA_DMAC_PERIPHS    AMEBA_SINGLE_PERIPH(DMAC)    /* AMEBA_DMAC_CLK */

/**
 * @brief SDIO clock peripheral mapping.
 */
#define AMEBA_SDIO_PERIPHS    AMEBA_SINGLE_PERIPH(SDIO)    /* AMEBA_SDIO_CLK */

/**
 * @brief USB clock peripheral mapping.
 */
#define AMEBA_USB_PERIPHS     AMEBA_SINGLE_PERIPH(USB)     /* AMEBA_USB_CLK */

/**
 * @brief Flash clock peripheral mapping.
 */
#define AMEBA_FLASH_PERIPHS   AMEBA_SINGLE_PERIPH(FLASH)   /* AMEBA_FLASH_CLK */

/**
 * @brief PSRAM clock peripheral mapping.
 */
#define AMEBA_PSRAM_PERIPHS   AMEBA_SINGLE_PERIPH(PSRAM)   /* AMEBA_PSRAM_CLK */

/**
 * @brief AC clock peripheral mapping.
 */
#define AMEBA_AC_PERIPHS      AMEBA_SINGLE_PERIPH(AC)      /* AMEBA_AC_CLK */

/**
 * @brief IRDA clock peripheral mapping.
 */
#define AMEBA_IRDA_PERIPHS    AMEBA_SINGLE_PERIPH(IRDA)    /* AMEBA_IRDA_CLK */

/**
 * @brief TRNG clock peripheral mapping.
 */
#define AMEBA_TRNG_PERIPHS    AMEBA_SINGLE_PERIPH(TRNG)    /* AMEBA_TRNG_CLK */

/**
 * @brief RTC clock peripheral mapping.
 */
#define AMEBA_RTC_PERIPHS     AMEBA_SINGLE_PERIPH(RTC)     /* AMEBA_RTC_CLK */

/**
 * @brief LEDC clock peripheral mapping.
 */
#define AMEBA_LEDC_PERIPHS    AMEBA_SINGLE_PERIPH(LEDC)    /* AMEBA_LEDC_CLK */

/**
 * @brief ADC clock peripheral mapping.
 */
#define AMEBA_ADC_PERIPHS     AMEBA_SINGLE_PERIPH(ADC)     /* AMEBA_ADC_CLK */

/**
 * @brief GPIO clock peripheral mapping.
 */
#define AMEBA_GPIO_PERIPHS    AMEBA_SINGLE_PERIPH(GPIO)    /* AMEBA_GPIO_CLK */

/**
 * @brief BTON clock peripheral mapping.
 */
#define AMEBA_BTON_PERIPHS    AMEBA_SINGLE_PERIPH(BTON)    /* AMEBA_BTON_CLK */

/**
 * @brief Aggregated core peripheral clock mappings.
 *
 * This macro expands to mappings of all core peripherals used by
 * the clock control implementation.
 */
#define AMEBA_CORE_PERIPHS                                                                 \
	AMEBA_RTC_PERIPHS                                                                  \
	AMEBA_PWM_PERIPHS                                                                  \
	AMEBA_HTIM_PERIPHS                                                                 \
	AMEBA_LEDC_PERIPHS                                                                 \
	AMEBA_UART_PERIPHS                                                                 \
	AMEBA_LOGUART_PERIPHS                                                              \
	AMEBA_ADC_PERIPHS                                                                  \
	AMEBA_GPIO_PERIPHS                                                                 \
	AMEBA_LTIM_PERIPHS                                                                 \
	AMEBA_PTIM_PERIPHS                                                                 \
	AMEBA_KSCAN_PERIPHS                                                                \
	AMEBA_DMAC_PERIPHS                                                                 \
	AMEBA_SDIO_PERIPHS                                                                 \
	AMEBA_SPI_PERIPHS                                                                  \
	AMEBA_USB_PERIPHS                                                                  \
	AMEBA_FLASH_PERIPHS                                                                \
	AMEBA_SPORT_PERIPHS                                                                \
	AMEBA_AC_PERIPHS                                                                   \
	AMEBA_I2C_PERIPHS                                                                  \
	AMEBA_TRNG_PERIPHS                                                                 \
	AMEBA_BTON_PERIPHS

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMEBADPLUS_CLOCK_H_ */
