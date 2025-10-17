/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Arducam DVP 20-pin connector pin constants
 * @ingroup dvp-20pin-connector
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_DVP_20PIN_CONNECTOR_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_DVP_20PIN_CONNECTOR_H_

/**
 * @defgroup dvp-20pin-connector Arducam DVP 20-pin connector
 * @brief Constants for pins exposed on Arducam DVP 20-pin or 18-pin connector
 * @ingroup devicetree-gpio-pin-headers
 * @{
 */
#define DVP_20PIN_SCL	3	/**< I2C clock pin */
#define DVP_20PIN_SDA	4	/**< I2C data pin */
#define DVP_20PIN_VS	5	/**< Vertical sync */
#define DVP_20PIN_HS	6	/**< Horizontal sync */
#define DVP_20PIN_PCLK	7	/**< Pixel clock used to transmit the data */
#define DVP_20PIN_XCLK	8	/**< System clock often needed for I2C communication */
#define DVP_20PIN_D7	9	/**< Parallel port data */
#define DVP_20PIN_D6	10	/**< Parallel port data */
#define DVP_20PIN_D5	11	/**< Parallel port data */
#define DVP_20PIN_D4	12	/**< Parallel port data */
#define DVP_20PIN_D3	13	/**< Parallel port data */
#define DVP_20PIN_D2	14	/**< Parallel port data */
#define DVP_20PIN_D1	15	/**< Parallel port data */
#define DVP_20PIN_D0	16	/**< Parallel port data */
#define DVP_20PIN_PEN	17	/**< Power Enable */
#define DVP_20PIN_PDN	18	/**< Power Down */
#define DVP_20PIN_GPIO0	19	/**< Extra GPIO pin present on some modules */
#define DVP_20PIN_GPIO1	20	/**< Extra GPIO pin present on some modules */
/** @} */

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_DVP_20PIN_CONNECTOR_H_ */
