/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Realtek RTS5817 GPIO controller definitions
 *
 * This file contains the GPIO name definitions and specific configuration flags
 * for the Realtek RTS5817 SoC.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RTS5817_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RTS5817_GPIO_H_

/**
 * @brief GPIO name definition.
 *
 * Define GPIO aliases for convenient access and usage.
 *
 * Example DT usage:
 *
 * @code{.dts}
 * gpios = <&gpio GPIO_AL0 0>;
 * @endcode
 *
 * @{
 */

#define GPIO_CACHE_CS2  0  /**< CS2 for flash controller */
#define GPIO_SSI_M_MISO 4  /**< MISO for SPI1 (master) */
#define GPIO_SSI_M_MOSI 5  /**< MOSI for SPI1 (master)*/
#define GPIO_SSI_M_CS   6  /**< CS for SPI1 (master) */
#define GPIO_SSI_M_SCK  7  /**< SCK for SPI1 (master) */
#define GPIO_SCL0       8  /**< SCL for I2C0 */
#define GPIO_SDA0       9  /**< SDA for I2C0 */
#define GPIO_SCL2       10 /**< SCL for I2C2 */
#define GPIO_SDA2       11 /**< SDA for I2C2 */
#define GPIO_SCL1       12 /**< SCL for I2C1 */
#define GPIO_SDA1       13 /**< SDA for I2C1 */
#define GPIO_SSI_S_MISO 14 /**< MISO for SPI2 (slave) */
#define GPIO_SSI_S_MOSI 15 /**< MOSI for SPI2 (slave)) */
#define GPIO_SSI_S_CS   16 /**< CS for SPI2 (slave) */
#define GPIO_SSI_S_SCK  17 /**< SCK for SSI (slave) */
#define GPIO_GPIO14     18 /**< GPIO14 */
#define GPIO_GPIO15     19 /**< GPIO15 */
#define GPIO_AL0        20 /**< GPIO AL0 */
#define GPIO_AL1        21 /**< GPIO AL1 */
#define GPIO_AL2        22 /**< GPIO AL2 */
#define GPIO_SNR_RST    23 /**< Reset pin for FP sensor */
#define GPIO_SNR_CS     24 /**< CS for SPI0 (FP sensor SPI) */
#define GPIO_SNR_GPIO   25 /**< GPIO for FP sensor */
#define GPI_WAKE1       26 /**< GPI1 for wake up */
#define GPI_WAKE2       27 /**< GPI2 for wake up */

/** @} */

/**
 * @brief Definition of GPIO power level flags.
 *
 * Some GPIOs of RTS5817 support two operating voltages: 1.8V or 3.3V..
 *
 * @{
 */

#define GPIO_POWER_3V3 (1 << 8) /**< 3.3V */
#define GPIO_POWER_1V8 (1 << 9) /**< 1.8V */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RTS5817_GPIO_H_ */
