/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STMod+ connector pin constants
 * @ingroup stmod-plus-connector
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_STMOD_PLUS_CONNECTOR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_STMOD_PLUS_CONNECTOR_H_

/**
 * @defgroup stmod-plus-connector STMod+ connector
 * @brief Constants for pins exposed on STMod+ connectors
 * @ingroup devicetree-gpio-pin-headers
 * @{
 */

#define STMOD_PLUS_NSS_CTS  1  /**< Pin 1: SPI NSS or UART CTS */
#define STMOD_PLUS_MOSI_TX  2  /**< Pin 2: SPI MOSI or UART TX */
#define STMOD_PLUS_MISO_RX  3  /**< Pin 3: SPI MISO or UART RX */
#define STMOD_PLUS_SCK_RTS  4  /**< Pin 4: SPI SCK or UART RTS */
#define STMOD_PLUS_SCL      7  /**< Pin 7: I2C SCL */
#define STMOD_PLUS_MOSIS    8  /**< Pin 8: Secondary SPI MOSI */
#define STMOD_PLUS_MISOS    9  /**< Pin 9: Secondary SPI MISO */
#define STMOD_PLUS_SDA      10 /**< Pin 10: I2C SDA */
#define STMOD_PLUS_INT      11 /**< Pin 11: Interrupt */
#define STMOD_PLUS_RESET    12 /**< Pin 12: Reset */
#define STMOD_PLUS_ADC      13 /**< Pin 13: ADC */
#define STMOD_PLUS_PWM      14 /**< Pin 14: PWM */
#define STMOD_PLUS_GPIO17   17 /**< Pin 17: GPIO */
#define STMOD_PLUS_GPIO18   18 /**< Pin 18: GPIO */
#define STMOD_PLUS_GPIO19   19 /**< Pin 19: GPIO */
#define STMOD_PLUS_GPIO20   20 /**< Pin 20: GPIO */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_STMOD_PLUS_CONNECTOR_H_ */
