/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief mikroBUS socket pin constants
 * @ingroup mikro-bus
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_MIKRO_BUS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_MIKRO_BUS_H_

/**
 * @defgroup mikro-bus mikroBUS socket
 * @brief Constants for pins exposed on mikroBUS sockets
 * @ingroup devicetree-gpio-pin-headers
 * @{
 */

#define MIKROBUS_AN   0  /**< Analog input (AN) */
#define MIKROBUS_RST  1  /**< Reset (RST) */
#define MIKROBUS_CS   2  /**< SPI chip select (CS) */
#define MIKROBUS_SCK  3  /**< SPI clock (SCK) */
#define MIKROBUS_MISO 4  /**< SPI master input, slave output (MISO) */
#define MIKROBUS_MOSI 5  /**< SPI master output, slave input (MOSI) */
#define MIKROBUS_PWM  6  /**< PWM output (PWM) */
#define MIKROBUS_INT  7  /**< Hardware interrupt (INT) */
#define MIKROBUS_RX   8  /**< UART receive (RX) */
#define MIKROBUS_TX   9  /**< UART transmit (TX) */
#define MIKROBUS_SCL  10 /**< I2C clock (SCL) */
#define MIKROBUS_SDA  11 /**< I2C data (SDA) */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_MIKRO_BUS_H_ */
