/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Pixhawk connector pin constants
 * @ingroup pixhawk-connector
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_PIXHAWK_CONNECTOR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_PIXHAWK_CONNECTOR_H_

/**
 * @defgroup pixhawk-connector Pixhawk connectors
 * @brief Constants for signal pins of Pixhawk Connector Standard ports
 *
 * Values are connector pin numbers. Power and ground pins are not mapped.
 *
 * @ingroup devicetree-gpio-pin-headers
 * @{
 */

#define PIXHAWK_TELEM_TX  2 /**< TELEM pin 2: UART TX */
#define PIXHAWK_TELEM_RX  3 /**< TELEM pin 3: UART RX */
#define PIXHAWK_TELEM_CTS 4 /**< TELEM pin 4: UART CTS */
#define PIXHAWK_TELEM_RTS 5 /**< TELEM pin 5: UART RTS */

#define PIXHAWK_GPS_TX  2 /**< GPS pin 2: UART TX */
#define PIXHAWK_GPS_RX  3 /**< GPS pin 3: UART RX */
#define PIXHAWK_GPS_SCL 4 /**< GPS pin 4: I2C SCL */
#define PIXHAWK_GPS_SDA 5 /**< GPS pin 5: I2C SDA */

#define PIXHAWK_FULL_GPS_TX         2 /**< Full GPS pin 2: UART TX */
#define PIXHAWK_FULL_GPS_RX         3 /**< Full GPS pin 3: UART RX */
#define PIXHAWK_FULL_GPS_SCL        4 /**< Full GPS pin 4: I2C SCL */
#define PIXHAWK_FULL_GPS_SDA        5 /**< Full GPS pin 5: I2C SDA */
#define PIXHAWK_FULL_GPS_SAFETY_SW  6 /**< Full GPS pin 6: safety switch input */
#define PIXHAWK_FULL_GPS_SAFETY_LED 7 /**< Full GPS pin 7: safety LED output */
#define PIXHAWK_FULL_GPS_BUZZER     9 /**< Full GPS pin 9: buzzer output */

#define PIXHAWK_I2C_SCL 2 /**< I2C pin 2: I2C SCL */
#define PIXHAWK_I2C_SDA 3 /**< I2C pin 3: I2C SDA */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_PIXHAWK_CONNECTOR_H_ */
