/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RTL87X2G_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RTL87X2G_CLOCKS_H_

/**
 * @file
 * @brief Realtek RTL87x2G Clock Controller Devicetree Bindings
 */

/**
 * @brief Helper macro to reference an APB peripheral clock.
 *
 * @param peri The peripheral name.
 *             The macro automatically expands to `APB_PERIPH_<peri>_CLOCK`.
 */
#define APB_CLK(peri)       APB_PERIPH_##peri##_CLOCK

/**
 * @name APB Peripheral Clock IDs
 * @brief Clock identifiers for APB peripherals.
 * @{
 */

#define APB_PERIPH_SPIC0_CLOCK              0U
#define APB_PERIPH_SPIC1_CLOCK              1U
#define APB_PERIPH_SPIC2_CLOCK              2U
#define APB_PERIPH_GDMA_CLOCK               3U
#define APB_PERIPH_SPI0_SLAVE_CLOCK         4U
#define APB_PERIPH_SPI1_CLOCK               5U
#define APB_PERIPH_SPI0_CLOCK               6U
#define APB_PERIPH_I2C3_CLOCK               7U
#define APB_PERIPH_I2C2_CLOCK               8U
#define APB_PERIPH_I2C1_CLOCK               9U
#define APB_PERIPH_I2C0_CLOCK               10U
#define APB_PERIPH_UART3_CLOCK              11U
#define APB_PERIPH_UART2_CLOCK              12U
#define APB_PERIPH_UART1_CLOCK              13U
#define APB_PERIPH_UART0_CLOCK              14U
#define APB_PERIPH_ACCXTAL_CLOCK            15U
#define APB_PERIPH_PDCK_CLOCK               16U
#define APB_PERIPH_ZBMAC_CLOCK              17U
#define APB_PERIPH_BTPHY_CLOCK              18U
#define APB_PERIPH_BTMAC_CLOCK              19U
#define APB_PERIPH_SEGCOM_CLOCK             20U
#define APB_PERIPH_SPI3W_CLOCK              21U
#define APB_PERIPH_ETH_CLOCK                22U
#define APB_PERIPH_PPE_CLOCK                23U
#define APB_PERIPH_KEYSCAN_CLOCK            24U
#define APB_PERIPH_24BADC_CLOCK             25U
#define APB_PERIPH_ADC_CLOCK                26U
#define APB_PERIPH_A2C_CLOCK                27U
#define APB_PERIPH_IR_CLOCK                 28U
#define APB_PERIPH_ISO7816_CLOCK            29U
#define APB_PERIPH_GPIOB_CLOCK              30U
#define APB_PERIPH_GPIOA_CLOCK              31U
#define APB_PERIPH_DISP_CLOCK               32U
#define APB_PERIPH_IMDC_CLOCK               33U
#define APB_PERIPH_TIMER_CLOCK              34U
#define APB_PERIPH_ENHTIMER_CLOCK           35U
#define APB_PERIPH_ENHTIMER_PWM1_CLOCK      36U
#define APB_PERIPH_ENHTIMER_PWM0_CLOCK      37U
#define APB_PERIPH_ENHTIMER_PWM3_CLOCK      38U
#define APB_PERIPH_ENHTIMER_PWM2_CLOCK      39U
#define APB_PERIPH_SDHC_CLOCK               40U
#define APB_PERIPH_UART5_CLOCK              41U
#define APB_PERIPH_UART4_CLOCK              42U
#define APB_PERIPH_CODEC_CLOCK              43U
#define APB_PERIPH_I2S1_CLOCK               44U
#define APB_PERIPH_I2S0_CLOCK               45U

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RTL87X2G_CLOCKS_H_ */
