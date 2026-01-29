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
 *             The macro automatically expands to `APBPeriph_<peri>_CLOCK`.
 */
#define APB_CLK(peri)       APBPeriph_##peri##_CLOCK

/**
 * @name APB Peripheral Clock IDs
 * @brief Clock identifiers for APB peripherals.
 * @{
 */

#define APBPeriph_SPIC0_CLOCK              0U
#define APBPeriph_SPIC1_CLOCK              1U
#define APBPeriph_SPIC2_CLOCK              2U
#define APBPeriph_GDMA_CLOCK               3U
#define APBPeriph_SPI0_SLAVE_CLOCK         4U
#define APBPeriph_SPI1_CLOCK               5U
#define APBPeriph_SPI0_CLOCK               6U
#define APBPeriph_I2C3_CLOCK               7U
#define APBPeriph_I2C2_CLOCK               8U
#define APBPeriph_I2C1_CLOCK               9U
#define APBPeriph_I2C0_CLOCK               10U
#define APBPeriph_UART3_CLOCK              11U
#define APBPeriph_UART2_CLOCK              12U
#define APBPeriph_UART1_CLOCK              13U
#define APBPeriph_UART0_CLOCK              14U
#define APBPeriph_ACCXTAL_CLOCK            15U
#define APBPeriph_PDCK_CLOCK               16U
#define APBPeriph_ZBMAC_CLOCK              17U
#define APBPeriph_BTPHY_CLOCK              18U
#define APBPeriph_BTMAC_CLOCK              19U
#define APBPeriph_SEGCOM_CLOCK             20U
#define APBPeriph_SPI3W_CLOCK              21U
#define APBPeriph_ETH_CLOCK                22U
#define APBPeriph_PPE_CLOCK                23U
#define APBPeriph_KEYSCAN_CLOCK            24U
#define APBPeriph_24BADC_CLOCK             25U
#define APBPeriph_ADC_CLOCK                26U
#define APBPeriph_A2C_CLOCK                27U
#define APBPeriph_IR_CLOCK                 28U
#define APBPeriph_ISO7816_CLOCK            29U
#define APBPeriph_GPIOB_CLOCK              30U
#define APBPeriph_GPIOA_CLOCK              31U
#define APBPeriph_DISP_CLOCK               32U
#define APBPeriph_IMDC_CLOCK               33U
#define APBPeriph_TIMER_CLOCK              34U
#define APBPeriph_ENHTIMER_CLOCK           35U
#define APBPeriph_ENHTIMER_PWM1_CLOCK      36U
#define APBPeriph_ENHTIMER_PWM0_CLOCK      37U
#define APBPeriph_ENHTIMER_PWM3_CLOCK      38U
#define APBPeriph_ENHTIMER_PWM2_CLOCK      39U
#define APBPeriph_SDHC_CLOCK               40U
#define APBPeriph_UART5_CLOCK              41U
#define APBPeriph_UART4_CLOCK              42U
#define APBPeriph_CODEC_CLOCK              43U
#define APBPeriph_I2S1_CLOCK               44U
#define APBPeriph_I2S0_CLOCK               45U

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RTL87X2G_CLOCKS_H_ */
