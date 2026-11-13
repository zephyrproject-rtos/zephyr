/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief 3DR Control N1 module connector pin constants
 * @ingroup control-n1-connector
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_3DR_CONTROL_N1_CONNECTOR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_3DR_CONTROL_N1_CONNECTOR_H_

/**
 * @defgroup control-n1-connector 3DR Control N1 connector
 * @brief Constants for pins exposed on the 3DR Control N1 module connector
 *
 * The Control N1 module exposes its interfaces on two 30-pin (J100, J200) and one 80-pin (J300)
 * board-to-board connectors. Module boards provide the @c cn1_connector GPIO nexus node, indexed
 * by these constants. Power, FD-CAN bus, USB, SWD, reset and boot pins are not mapped.
 *
 * The @c cn1_pwm PWM nexus node is indexed by output number (1 for CH1 to 12 for CH12) and the
 * @c cn1_adc io-channel nexus node by analog input number (1 for ADC1 on J300 pin 30, 2 for ADC2
 * on J300 pin 28).
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/gpio/3dr-control-n1-connector.h>
 *
 * gpios = <&cn1_connector CN1_CH5 GPIO_ACTIVE_HIGH>;
 * pwms = <&cn1_pwm 5 PWM_MSEC(20) PWM_POLARITY_NORMAL>;
 * @endcode
 *
 * @ingroup devicetree-gpio-pin-headers
 * @{
 */

#define CN1_SG1_TX   0  /**< SG1 TX, J100 pin 2 */
#define CN1_SG1_RX   1  /**< SG1 RX, J100 pin 4 */
#define CN1_SG1_CTS  2  /**< SG1 CTS, J100 pin 6 */
#define CN1_SG1_RTS  3  /**< SG1 RTS, J100 pin 8 */
#define CN1_SG2_TX   4  /**< SG2 TX, J100 pin 12 */
#define CN1_SG2_RX   5  /**< SG2 RX, J100 pin 14 */
#define CN1_SG2_CTS  6  /**< SG2 CTS, J100 pin 16 */
#define CN1_SG2_RTS  7  /**< SG2 RTS, J100 pin 18 */
#define CN1_SG3_TX   8  /**< SG3 TX, J300 pin 47 */
#define CN1_SG3_RX   9  /**< SG3 RX, J300 pin 45 */
#define CN1_SG3_CTS  10 /**< SG3 CTS, J300 pin 43 */
#define CN1_SG3_RTS  11 /**< SG3 RTS, J300 pin 41 */
#define CN1_SG4_TX   12 /**< SG4 TX, J100 pin 22 */
#define CN1_SG4_RX   13 /**< SG4 RX, J100 pin 24 */
#define CN1_SG5_TX   14 /**< SG5 TX, J100 pin 28 */
#define CN1_SG5_RX   15 /**< SG5 RX, J100 pin 30 */
#define CN1_SG6_TX   16 /**< SG6 TX, J200 pin 2 */
#define CN1_SG6_RX   17 /**< SG6 RX, J200 pin 4 */
#define CN1_SG7_TX   18 /**< SG7 TX, J300 pin 37 */
#define CN1_SG7_RX   19 /**< SG7 RX, J300 pin 35 */
#define CN1_I2C1_SCL 20 /**< I2C1 SCL, J300 pin 31 */
#define CN1_I2C1_SDA 21 /**< I2C1 SDA, J300 pin 29 */
#define CN1_I2C2_SCL 22 /**< I2C2 SCL, J300 pin 25 */
#define CN1_I2C2_SDA 23 /**< I2C2 SDA, J300 pin 23 */
#define CN1_SPI_SCK  24 /**< SPI clock, J200 pin 22 */
#define CN1_SPI_SDI  25 /**< SPI data into the module (MISO), J200 pin 24 */
#define CN1_SPI_SDO  26 /**< SPI data out of the module (MOSI), J200 pin 26 */
#define CN1_SPI_NCS  27 /**< SPI chip select, J200 pin 28 */
#define CN1_SD_DET   28 /**< SD card detect, J100 pin 9 */
#define CN1_SDIO_CK  29 /**< SDIO clock, J100 pin 15 */
#define CN1_SDIO_CMD 30 /**< SDIO command, J100 pin 19 */
#define CN1_SDIO_D0  31 /**< SDIO data 0, J100 pin 13 */
#define CN1_SDIO_D1  32 /**< SDIO data 1, J100 pin 11 */
#define CN1_SDIO_D2  33 /**< SDIO data 2, J100 pin 23 */
#define CN1_SDIO_D3  34 /**< SDIO data 3, J100 pin 21 */
#define CN1_ALARM    35 /**< Buzzer output, J100 pin 27 */
#define CN1_GP1      36 /**< GPIO 1, J300 pin 50 */
#define CN1_GP2      37 /**< GPIO 2, J300 pin 48 */
#define CN1_GP3      38 /**< GPIO 3, J300 pin 46 */
#define CN1_GP4      39 /**< GPIO 4, J300 pin 44 */
#define CN1_CH1      40 /**< Output CH1, J300 pin 80 */
#define CN1_CH2      41 /**< Output CH2, J300 pin 78 */
#define CN1_CH3      42 /**< Output CH3, J300 pin 76 */
#define CN1_CH4      43 /**< Output CH4, J300 pin 74 */
#define CN1_CH5      44 /**< Output CH5, J300 pin 70 */
#define CN1_CH6      45 /**< Output CH6, J300 pin 68 */
#define CN1_CH7      46 /**< Output CH7, J300 pin 66 */
#define CN1_CH8      47 /**< Output CH8, J300 pin 64 */
#define CN1_CH9      48 /**< Output CH9, J300 pin 60 */
#define CN1_CH10     49 /**< Output CH10, J300 pin 58 */
#define CN1_CH11     50 /**< Output CH11, J300 pin 56 */
#define CN1_CH12     51 /**< Output CH12, J300 pin 54 */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_3DR_CONTROL_N1_CONNECTOR_H_ */
