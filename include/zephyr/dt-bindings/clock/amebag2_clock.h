/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMEBAG2_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMEBAG2_CLOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Realtek Amebag2 clock Devicetree bindings
 */


/**
 * @name AON domain clocks
 * @{
 */

/** ATIM clock in AON domain */
#define AMEBA_ATIM_CLK 1

/** RTC clock in AON domain */
#define AMEBA_RTC_CLK  2

/** LEDC clock in AON domain */
#define AMEBA_LEDC_CLK 3

/** @} */

/**
 * @name SYSON domain clocks
 * @{
 */

/** PWM0 clock in SYSON domain */
#define AMEBA_PWM0_CLK 4

/** PWM1 clock in SYSON domain */
#define AMEBA_PWM1_CLK 5

/** PWM2 clock in SYSON domain */
#define AMEBA_PWM2_CLK 6

/** PWM3 clock in SYSON domain */
#define AMEBA_PWM3_CLK 7

/** PWM4 clock in SYSON domain */
#define AMEBA_PWM4_CLK 8

/** UART0 clock in SYSON domain */
#define AMEBA_UART0_CLK   9

/** UART1 clock in SYSON domain */
#define AMEBA_UART1_CLK   10

/** UART2 clock in SYSON domain */
#define AMEBA_UART2_CLK   11

/** UART3 clock in SYSON domain */
#define AMEBA_UART3_CLK   12

/** LOGUART clock in SYSON domain */
#define AMEBA_LOGUART_CLK 13

/** DTIM clock in SYSON domain */
#define AMEBA_DTIM_CLK    14

/** ADC clock in SYSON domain */
#define AMEBA_ADC_CLK     15

/** GPIO clock in SYSON domain */
#define AMEBA_GPIO_CLK    16

/** LTIM0 clock in SYSON domain */
#define AMEBA_LTIM0_CLK 17

/** LTIM1 clock in SYSON domain */
#define AMEBA_LTIM1_CLK 18

/** LTIM2 clock in SYSON domain */
#define AMEBA_LTIM2_CLK 19

/** LTIM3 clock in SYSON domain */
#define AMEBA_LTIM3_CLK 20

/** PTIM0 clock in SYSON domain */
#define AMEBA_PTIM0_CLK 21

/** PTIM1 clock in SYSON domain */
#define AMEBA_PTIM1_CLK 22

/** @} */

/**
 * @name SoC domain clocks
 * @{
 */

/** DMAC clock in SoC domain */
#define AMEBA_DMAC_CLK  23

/** SDH clock in SoC domain */
#define AMEBA_SDH_CLK   24

/** SDD clock in SoC domain */
#define AMEBA_SDD_CLK   25

/** SPI0 clock in SoC domain */
#define AMEBA_SPI0_CLK  26

/** SPI1 clock in SoC domain */
#define AMEBA_SPI1_CLK  27

/** USB clock in SoC domain */
#define AMEBA_USB_CLK   28

/** Flash clock in SoC domain */
#define AMEBA_FLASH_CLK 29

/** PSRAM clock in SoC domain */
#define AMEBA_PSRAM_CLK 30

/** SPORT clock in SoC domain */
#define AMEBA_SPORT_CLK 31

/** AC clock in SoC domain */
#define AMEBA_AC_CLK    32

/** IRDA clock in SoC domain */
#define AMEBA_IRDA_CLK  33

/** I2C0 clock in SoC domain */
#define AMEBA_I2C0_CLK  34

/** I2C1 clock in SoC domain */
#define AMEBA_I2C1_CLK  35

/** TRNG clock in SoC domain */
#define AMEBA_TRNG_CLK  36

/** LCDC clock in SoC domain */
#define AMEBA_LCDC_CLK  37

/** A2C0 clock in SoC domain */
#define AMEBA_A2C0_CLK  38

/** A2C1 clock in SoC domain */
#define AMEBA_A2C1_CLK  39

/** GMAC clock in SoC domain */
#define AMEBA_GMAC_CLK  40

/** PPE clock in SoC domain */
#define AMEBA_PPE_CLK   41

/** MJPEG clock in SoC domain */
#define AMEBA_MJPEG_CLK 42

/** @} */

/**
 * @name Misc clocks
 * @{
 */

/** BTON clock */
#define AMEBA_BTON_CLK 43

/** PKE clock (misc domain) */
#define AMEBA_PKE_CLK  44

/**
 * @brief Maximum clock index (one past the last valid index).
 */
#define AMEBA_CLK_MAX 45 /* clk idx max */

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
 * SDH, SDD, USB, TRNG, etc.
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
 * @brief Define a clock entry for a single-instance peripheral without FEN bit.
 *
 * Used when a peripheral clock has no separate enable bit.
 *
 * @param name Peripheral name
 */
#define AMEBA_SINGLE_PERIPH_NO_FEN(name)                                                   \
	[AMEBA_##name##_CLK] = {                                                           \
		.parent = AMEBA_RCC_NO_PARENT,                                             \
		.cke = APBPeriph_##name##_CLOCK,                                           \
		.fen = APBPeriph_NULL,                                                     \
	},

/**
 * @brief LTIM clock peripheral mappings.
 */
#define AMEBA_LTIM_PERIPHS                                                                 \
	AMEBA_NUMERICAL_PERIPH(LTIM, 0) /* AMEBA_LTIM0_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(LTIM, 1) /* AMEBA_LTIM1_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(LTIM, 2) /* AMEBA_LTIM2_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(LTIM, 3) /* AMEBA_LTIM3_CLK */

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
	AMEBA_NUMERICAL_PERIPH(PWM, 1) /* AMEBA_PWM1_CLK */                                \
	AMEBA_NUMERICAL_PERIPH(PWM, 2) /* AMEBA_PWM2_CLK */                                \
	AMEBA_NUMERICAL_PERIPH(PWM, 3) /* AMEBA_PWM3_CLK */                                \
	AMEBA_NUMERICAL_PERIPH(PWM, 4) /* AMEBA_PWM4_CLK */

/**
 * @brief UART clock peripheral mappings.
 */
#define AMEBA_UART_PERIPHS                                                                 \
	AMEBA_NUMERICAL_PERIPH(UART, 0) /* AMEBA_UART0_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(UART, 1) /* AMEBA_UART1_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(UART, 2) /* AMEBA_UART2_CLK */                              \
	AMEBA_NUMERICAL_PERIPH(UART, 3) /* AMEBA_UART3_CLK */

/**
 * @brief A2C clock peripheral mappings.
 */
#define AMEBA_A2C_PERIPHS                                                                  \
	AMEBA_NUMERICAL_PERIPH(A2C, 0) /* AMEBA_A2C0_CLK */                                \
	AMEBA_NUMERICAL_PERIPH(A2C, 1) /* AMEBA_A2C1_CLK */

/**
 * @brief LOGUART clock peripheral mapping.
 */
#define AMEBA_LOGUART_PERIPHS AMEBA_SINGLE_PERIPH(LOGUART)    /* AMEBA_LOGUART_CLK */

/**
 * @brief DMAC clock peripheral mapping.
 */
#define AMEBA_DMAC_PERIPHS    AMEBA_SINGLE_PERIPH(DMAC)       /* AMEBA_DMAC_CLK */

/**
 * @brief SDH clock peripheral mapping.
 */
#define AMEBA_SDH_PERIPHS     AMEBA_SINGLE_PERIPH(SDH)        /* AMEBA_SDH_CLK */

/**
 * @brief SDD clock peripheral mapping.
 */
#define AMEBA_SDD_PERIPHS     AMEBA_SINGLE_PERIPH(SDD)        /* AMEBA_SDD_CLK */

/**
 * @brief USB clock peripheral mapping.
 */
#define AMEBA_USB_PERIPHS     AMEBA_SINGLE_PERIPH(USB)        /* AMEBA_USB_CLK */

/**
 * @brief Flash clock peripheral mapping.
 */
#define AMEBA_FLASH_PERIPHS   AMEBA_SINGLE_PERIPH(FLASH)      /* AMEBA_FLASH_CLK */

/**
 * @brief PSRAM clock peripheral mapping.
 */
#define AMEBA_PSRAM_PERIPHS   AMEBA_SINGLE_PERIPH(PSRAM)      /* AMEBA_PSRAM_CLK */

/**
 * @brief AC clock peripheral mapping.
 */
#define AMEBA_AC_PERIPHS      AMEBA_SINGLE_PERIPH(AC)         /* AMEBA_AC_CLK */

/**
 * @brief IRDA clock peripheral mapping.
 */
#define AMEBA_IRDA_PERIPHS    AMEBA_SINGLE_PERIPH(IRDA)       /* AMEBA_IRDA_CLK */

/**
 * @brief TRNG clock peripheral mapping.
 */
#define AMEBA_TRNG_PERIPHS    AMEBA_SINGLE_PERIPH(TRNG)       /* AMEBA_TRNG_CLK */

/**
 * @brief LCDC clock peripheral mapping.
 */
#define AMEBA_LCDC_PERIPHS    AMEBA_SINGLE_PERIPH(LCDC)       /* AMEBA_LCDC_CLK */

/**
 * @brief RTC clock peripheral mapping (no FEN bit).
 */
#define AMEBA_RTC_PERIPHS     AMEBA_SINGLE_PERIPH_NO_FEN(RTC) /* AMEBA_RTC_CLK */

/**
 * @brief LEDC clock peripheral mapping.
 */
#define AMEBA_LEDC_PERIPHS    AMEBA_SINGLE_PERIPH(LEDC)       /* AMEBA_LEDC_CLK */

/**
 * @brief ADC clock peripheral mapping.
 */
#define AMEBA_ADC_PERIPHS     AMEBA_SINGLE_PERIPH(ADC)        /* AMEBA_ADC_CLK */

/**
 * @brief GPIO clock peripheral mapping.
 */
#define AMEBA_GPIO_PERIPHS    AMEBA_SINGLE_PERIPH(GPIO)       /* AMEBA_GPIO_CLK */

/**
 * @brief BTON clock peripheral mapping.
 */
#define AMEBA_BTON_PERIPHS    AMEBA_SINGLE_PERIPH(BTON)       /* AMEBA_BTON_CLK */

/**
 * @brief SPORT clock peripheral mapping.
 */
#define AMEBA_SPORT_PERIPHS   AMEBA_SINGLE_PERIPH(SPORT)      /* AMEBA_SPORT_CLK */

/**
 * @brief GMAC clock peripheral mapping.
 */
#define AMEBA_GMAC_PERIPHS    AMEBA_SINGLE_PERIPH(GMAC)       /* AMEBA_GMAC_CLK */

/**
 * @brief PPE clock peripheral mapping.
 */
#define AMEBA_PPE_PERIPHS     AMEBA_SINGLE_PERIPH(PPE)        /* AMEBA_PPE_CLK */

/**
 * @brief MJPEG clock peripheral mapping.
 */
#define AMEBA_MJPEG_PERIPHS   AMEBA_SINGLE_PERIPH(MJPEG)      /* AMEBA_MJPEG_CLK */

/**
 * @brief Aggregated core peripheral clock mappings.
 *
 * This macro expands to mappings of all core peripherals used by
 * the clock control implementation.
 */
#define AMEBA_CORE_PERIPHS                                                                 \
	AMEBA_RTC_PERIPHS                                                                  \
	AMEBA_PWM_PERIPHS                                                                  \
	AMEBA_LEDC_PERIPHS                                                                 \
	AMEBA_UART_PERIPHS                                                                 \
	AMEBA_LOGUART_PERIPHS                                                              \
	AMEBA_ADC_PERIPHS                                                                  \
	AMEBA_GPIO_PERIPHS                                                                 \
	AMEBA_LTIM_PERIPHS                                                                 \
	AMEBA_PTIM_PERIPHS                                                                 \
	AMEBA_DMAC_PERIPHS                                                                 \
	AMEBA_SDH_PERIPHS                                                                  \
	AMEBA_SDD_PERIPHS                                                                  \
	AMEBA_SPI_PERIPHS                                                                  \
	AMEBA_USB_PERIPHS                                                                  \
	AMEBA_FLASH_PERIPHS                                                                \
	AMEBA_SPORT_PERIPHS                                                                \
	AMEBA_AC_PERIPHS                                                                   \
	AMEBA_I2C_PERIPHS                                                                  \
	AMEBA_TRNG_PERIPHS                                                                 \
	AMEBA_LCDC_PERIPHS                                                                 \
	AMEBA_A2C_PERIPHS                                                                  \
	AMEBA_GMAC_PERIPHS                                                                 \
	AMEBA_PPE_PERIPHS                                                                  \
	AMEBA_MJPEG_PERIPHS                                                                \
	AMEBA_BTON_PERIPHS

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMEBAG2_CLOCK_H_ */
