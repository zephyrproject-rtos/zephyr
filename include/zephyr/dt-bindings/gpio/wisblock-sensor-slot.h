/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WisBlock Sensor slot connector pin constants
 * @ingroup wisblock-sensor-slot
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_WISBLOCK_SENSOR_SLOT_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_WISBLOCK_SENSOR_SLOT_H_

/**
 * @defgroup wisblock-sensor-slot WisBlock Sensor Slot connector
 * @brief Constants for pins exposed on RAKwireless WisBlock Sensor Slots
 * @ingroup devicetree-gpio-pin-headers
 * @{
 */

#define WISBLOCK_SENSOR_TXD      0 /**< UART TX */
#define WISBLOCK_SENSOR_SPI_CS   1 /**< SPI chip select */
#define WISBLOCK_SENSOR_SPI_CLK  2 /**< SPI clock */
#define WISBLOCK_SENSOR_SPI_MISO 3 /**< SPI MISO */
#define WISBLOCK_SENSOR_SPI_MOSI 4 /**< SPI MOSI */
#define WISBLOCK_SENSOR_I2C_SCL  5 /**< I2C SCL */
#define WISBLOCK_SENSOR_I2C_SDA  6 /**< I2C SDA */
#define WISBLOCK_SENSOR_GPIO1    7 /**< General-purpose GPIO1 */
#define WISBLOCK_SENSOR_GPIO2    8 /**< General-purpose GPIO2 */
#define WISBLOCK_SENSOR_RXD      9 /**< UART RX */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_WISBLOCK_SENSOR_SLOT_H_ */
