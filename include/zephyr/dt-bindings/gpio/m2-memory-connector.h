/*
 * Copyright (c) 2026 Filip Stojanovic <filipembedded@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief M.2 Key A serial memory connector pin constants
 * @ingroup m2-memory-connector
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_M2_MEMORY_CONNECTOR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_M2_MEMORY_CONNECTOR_H_

/**
 * @defgroup m2-memory-connector M.2 Key A serial memory connector
 * @brief Constants for non-power signals exposed on the ST M.2 serial memory connector
 * @ingroup devicetree-gpio-pin-headers
 * @{
 *
 * Values match the physical M.2 Key A connector pin numbers from TN1618.
 * Power, ground, reserved, and key pins are intentionally omitted.
 */

#define M2MEM_CONNECTOR_GPIO_SPI_SCK  16 /**< Optional auxiliary SPI clock */
#define M2MEM_CONNECTOR_GPIO_SPI_MOSI 17 /**< Optional auxiliary SPI controller-out */
#define M2MEM_CONNECTOR_GPIO_LDO_EN   18 /**< Mandatory open-drain LDO enable */
#define M2MEM_CONNECTOR_GPIO_SPI_MISO 19 /**< Optional auxiliary SPI controller-in */
#define M2MEM_CONNECTOR_XSPI_NCS2     20 /**< XSPI chip select 2 */
#define M2MEM_CONNECTOR_GPIO_SPI_NSS  21 /**< Optional auxiliary SPI chip select */
#define M2MEM_CONNECTOR_XSPI_NCS1     22 /**< XSPI chip select 1 */

#define M2MEM_CONNECTOR_XSPI_DQS0 25 /**< XSPI data strobe 0 */
#define M2MEM_CONNECTOR_XSPI_D6   26 /**< XSPI data 6 */
#define M2MEM_CONNECTOR_XSPI_D7   27 /**< XSPI data 7 */
#define M2MEM_CONNECTOR_XSPI_D5   28 /**< XSPI data 5 */
#define M2MEM_CONNECTOR_XSPI_D4   31 /**< XSPI data 4 */
#define M2MEM_CONNECTOR_XSPI_D3   32 /**< XSPI data 3 */
#define M2MEM_CONNECTOR_XSPI_D2   34 /**< XSPI data 2 */
#define M2MEM_CONNECTOR_XSPI_D1   35 /**< XSPI data 1 */
#define M2MEM_CONNECTOR_XSPI_D0   37 /**< XSPI data 0 */

#define M2MEM_CONNECTOR_LED2       38 /**< Red LED (LD2) */
#define M2MEM_CONNECTOR_INT1       40 /**< Optional GPIO, error, or interrupt 1 */
#define M2MEM_CONNECTOR_XSPI_CLK1  41 /**< XSPI clock 1 */
#define M2MEM_CONNECTOR_LED1       42 /**< Green LED (LD1) */
#define M2MEM_CONNECTOR_XSPI_NCLK1 43 /**< XSPI differential clock complement 1 */

#define M2MEM_CONNECTOR_XSPI_D15  47 /**< XSPI data 15 */
#define M2MEM_CONNECTOR_XSPI_D14  49 /**< XSPI data 14 */
#define M2MEM_CONNECTOR_XSPI_NCS4 52 /**< XSPI chip select 4 */
#define M2MEM_CONNECTOR_XSPI_D13  53 /**< XSPI data 13 */
#define M2MEM_CONNECTOR_XSPI_NCS3 54 /**< XSPI chip select 3 */
#define M2MEM_CONNECTOR_XSPI_D12  55 /**< XSPI data 12 */
#define M2MEM_CONNECTOR_XSPI_DQS1 56 /**< XSPI data strobe 1 */

#define M2MEM_CONNECTOR_I2C_SDA    58 /**< I2C data for identification EEPROM or auxiliaries */
#define M2MEM_CONNECTOR_XSPI_D11   59 /**< XSPI data 11 */
#define M2MEM_CONNECTOR_I2C_SCL    60 /**< I2C clock for identification EEPROM or auxiliaries */
#define M2MEM_CONNECTOR_XSPI_D10   61 /**< XSPI data 10 */
#define M2MEM_CONNECTOR_INT2       62 /**< Optional GPIO, error, or interrupt 2 */
#define M2MEM_CONNECTOR_XSPI_D9    65 /**< XSPI data 9 */
#define M2MEM_CONNECTOR_NRST       66 /**< Memory reset */
#define M2MEM_CONNECTOR_XSPI_D8    67 /**< XSPI data 8 */
#define M2MEM_CONNECTOR_XSPI_CLK2  71 /**< XSPI clock 2 */
#define M2MEM_CONNECTOR_XSPI_NCLK2 73 /**< XSPI differential clock complement 2 */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_M2_MEMORY_CONNECTOR_H_ */
