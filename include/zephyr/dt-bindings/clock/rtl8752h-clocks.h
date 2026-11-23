/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RTL8752H_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RTL8752H_CLOCKS_H_

/**
 * @file
 * @brief Realtek RTL8752H Clock Controller Devicetree Bindings
 */

/**
 * @brief Helper macro to reference an APB peripheral clock.
 *
 * @param peri The peripheral name.
 *             The macro automatically expands to `APB_PERIPH_<peri>_CLOCK`.
 */
#define APB_CLK(peri) APB_PERIPH_##peri##_CLOCK

/**
 * @name APB Peripheral Clock IDs
 * @brief Clock identifiers for APB peripherals.
 * @{
 */

#define APB_PERIPH_I2S0_CLOCK     0U   /**< I2S0 clock */
#define APB_PERIPH_I2S1_CLOCK     1U   /**< I2S1 clock */
#define APB_PERIPH_CODEC_CLOCK    2U   /**< CODEC clock */
#define APB_PERIPH_GPIO_CLOCK     3U   /**< GPIO clock */
#define APB_PERIPH_GDMA_CLOCK     4U   /**< GDMA clock */
#define APB_PERIPH_TIMER_CLOCK    5U   /**< TIMER clock */
#define APB_PERIPH_ENHTIMER_CLOCK 6U   /**< ENHTIME clock */
#define APB_PERIPH_UART2_CLOCK    7U   /**< UART2 clock */
#define APB_PERIPH_UART0_CLOCK    8U   /**< UART0 clock */
#define APB_PERIPH_FLASH_CLOCK    9U   /**< FLASH clock */
#define APB_PERIPH_PKE_CLOCK      10U  /**< PKE clock */
#define APB_PERIPH_SHA256_CLOCK   11U  /**< SHA256 clock */
#define APB_PERIPH_FLASH1_CLOCK   12U  /**< FLASH1 clock */
#define APB_PERIPH_FLH_SEC_CLOCK  13U  /**< FLH_SEC clock */
#define APB_PERIPH_IR_CLOCK       14U  /**< IR clock */
#define APB_PERIPH_SPI1_CLOCK     15U  /**< SPI1 clock */
#define APB_PERIPH_SPI0_CLOCK     16U  /**< SPI0 clock */
#define APB_PERIPH_UART1_CLOCK    17U  /**< UART1 clock */
#define APB_PERIPH_IF8080_CLOCK   18U  /**< IF8080 clock */
#define APB_PERIPH_ADC_CLOCK      19U  /**< ADC clock */
#define APB_PERIPH_SPI2W_CLOCK    20U  /**< SPI2W clock */
#define APB_PERIPH_MODEM_CLOCK    21U  /**< MODEM clock */
#define APB_PERIPH_BLUEWIZ_CLOCK  22U  /**< BLUEWIZ clock */
#define APB_PERIPH_ZIGBEE_CLOCK   23U  /**< ZIGBEE clock */
#define APB_PERIPH_KEYSCAN_CLOCK  24U  /**< KEYSCAN clock */
#define APB_PERIPH_QDEC_CLOCK     25U  /**< QDEC clock */
#define APB_PERIPH_I2C1_CLOCK     26U  /**< I2C1 clock */
#define APB_PERIPH_I2C0_CLOCK     27U  /**< I2C0 clock */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RTL8752H_CLOCKS_H_ */
