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

#define APB_PERIPH_SPIC0_CLOCK              0U    /**< SPIC0 clock */
#define APB_PERIPH_SPIC1_CLOCK              1U    /**< SPIC1 clock */
#define APB_PERIPH_SPIC2_CLOCK              2U    /**< SPIC2 clock */
#define APB_PERIPH_GDMA_CLOCK               3U    /**< GDMA clock */
#define APB_PERIPH_SPI0_SLAVE_CLOCK         4U    /**< SPI0_SLAVE clock */
#define APB_PERIPH_SPI1_CLOCK               5U    /**< SPI1 clock */
#define APB_PERIPH_SPI0_CLOCK               6U    /**< SPI0 clock */
#define APB_PERIPH_I2C3_CLOCK               7U    /**< I2C3 clock */
#define APB_PERIPH_I2C2_CLOCK               8U    /**< I2C2 clock */
#define APB_PERIPH_I2C1_CLOCK               9U    /**< I2C1 clock */
#define APB_PERIPH_I2C0_CLOCK               10U   /**< I2C0 clock */
#define APB_PERIPH_UART3_CLOCK              11U   /**< UART3 clock */
#define APB_PERIPH_UART2_CLOCK              12U   /**< UART2 clock */
#define APB_PERIPH_UART1_CLOCK              13U   /**< UART1 clock */
#define APB_PERIPH_UART0_CLOCK              14U   /**< UART0 clock */
#define APB_PERIPH_ACCXTAL_CLOCK            15U   /**< ACCXTAL clock */
#define APB_PERIPH_PDCK_CLOCK               16U   /**< PDCK clock */
#define APB_PERIPH_ZBMAC_CLOCK              17U   /**< ZBMAC clock */
#define APB_PERIPH_BTPHY_CLOCK              18U   /**< BTPHY clock */
#define APB_PERIPH_BTMAC_CLOCK              19U   /**< BTMAC clock */
#define APB_PERIPH_SEGCOM_CLOCK             20U   /**< SEGCOM clock */
#define APB_PERIPH_SPI3W_CLOCK              21U   /**< SPI3W clock */
#define APB_PERIPH_ETH_CLOCK                22U   /**< ETH clock */
#define APB_PERIPH_PPE_CLOCK                23U   /**< PPE clock */
#define APB_PERIPH_KEYSCAN_CLOCK            24U   /**< KEYSCAN clock */
#define APB_PERIPH_24BADC_CLOCK             25U   /**< 24BADC clock */
#define APB_PERIPH_ADC_CLOCK                26U   /**< ADC clock */
#define APB_PERIPH_A2C_CLOCK                27U   /**< A2C clock */
#define APB_PERIPH_IR_CLOCK                 28U   /**< IR clock */
#define APB_PERIPH_ISO7816_CLOCK            29U   /**< ISO7816 clock */
#define APB_PERIPH_GPIOB_CLOCK              30U   /**< GPIOB clock */
#define APB_PERIPH_GPIOA_CLOCK              31U   /**< GPIOA clock */
#define APB_PERIPH_DISP_CLOCK               32U   /**< DISP clock */
#define APB_PERIPH_IMDC_CLOCK               33U   /**< IMDC clock */
#define APB_PERIPH_TIMER_CLOCK              34U   /**< TIMER clock */
#define APB_PERIPH_ENHTIMER_CLOCK           35U   /**< ENHTIMER clock */
#define APB_PERIPH_ENHTIMER_PWM1_CLOCK      36U   /**< ENHTIMER_PWM1 clock */
#define APB_PERIPH_ENHTIMER_PWM0_CLOCK      37U   /**< ENHTIMER_PWM0 clock */
#define APB_PERIPH_ENHTIMER_PWM3_CLOCK      38U   /**< ENHTIMER_PWM3 clock */
#define APB_PERIPH_ENHTIMER_PWM2_CLOCK      39U   /**< ENHTIMER_PWM2 clock */
#define APB_PERIPH_SDHC_CLOCK               40U   /**< SDHC clock */
#define APB_PERIPH_UART5_CLOCK              41U   /**< UART5 clock */
#define APB_PERIPH_UART4_CLOCK              42U   /**< UART4 clock */
#define APB_PERIPH_CODEC_CLOCK              43U   /**< CODEC clock */
#define APB_PERIPH_I2S1_CLOCK               44U   /**< I2S1 clock */
#define APB_PERIPH_I2S0_CLOCK               45U   /**< I2S0 clock */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RTL87X2G_CLOCKS_H_ */
