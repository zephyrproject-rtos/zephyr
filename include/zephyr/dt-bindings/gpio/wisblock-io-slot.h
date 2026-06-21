/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WisBlock IO slot connector pin constants
 * @ingroup wisblock-io-slot
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_WISBLOCK_IO_SLOT_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_WISBLOCK_IO_SLOT_H_

/**
 * @defgroup wisblock-io-slot WisBlock IO slot connector
 * @brief Constants for pins exposed on WisBlock IO slot connector
 * @ingroup devicetree-gpio-pin-headers
 * @{
 */

#define WISBLOCK_IO_SW1      0  /**< User button SW1 */
#define WISBLOCK_IO_TXD0     1  /**< UART0 TX */
#define WISBLOCK_IO_RXD0     2  /**< UART0 RX */
#define WISBLOCK_IO_LED1     3  /**< LED1 */
#define WISBLOCK_IO_LED2     4  /**< LED2 */
#define WISBLOCK_IO_LED3     5  /**< LED3 */
#define WISBLOCK_IO_I2C1_SDA 6  /**< I2C1 SDA */
#define WISBLOCK_IO_I2C1_SCL 7  /**< I2C1 SCL */
#define WISBLOCK_IO_AIN0     8  /**< Analog input 0 */
#define WISBLOCK_IO_AIN1     9  /**< Analog input 1 */
#define WISBLOCK_IO_IO7      10 /**< General-purpose IO7 */
#define WISBLOCK_IO_SPI_CS   11 /**< SPI chip select */
#define WISBLOCK_IO_SPI_CLK  12 /**< SPI clock */
#define WISBLOCK_IO_SPI_MISO 13 /**< SPI MISO */
#define WISBLOCK_IO_SPI_MOSI 14 /**< SPI MOSI */
#define WISBLOCK_IO_IO1      15 /**< General-purpose IO1 */
#define WISBLOCK_IO_IO2      16 /**< General-purpose IO2 */
#define WISBLOCK_IO_IO3      17 /**< General-purpose IO3 */
#define WISBLOCK_IO_IO4      18 /**< General-purpose IO4 */
#define WISBLOCK_IO_TXD1     19 /**< UART1 TX */
#define WISBLOCK_IO_RXD1     20 /**< UART1 RX */
#define WISBLOCK_IO_I2C2_SDA 21 /**< I2C2 SDA */
#define WISBLOCK_IO_I2C2_SCL 22 /**< I2C2 SCL */
#define WISBLOCK_IO_IO5      23 /**< General-purpose IO5 */
#define WISBLOCK_IO_IO6      24 /**< General-purpose IO6 */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_WISBLOCK_IO_SLOT_H_ */
