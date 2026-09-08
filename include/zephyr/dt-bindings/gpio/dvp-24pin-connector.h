/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Panoramix Labs.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DVP 24-pin connector pin constants
 * @ingroup dvp-24pin-connector
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_DVP_24PIN_CONNECTOR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_DVP_24PIN_CONNECTOR_H_

/**
 * @defgroup dvp-24pin-connector DVP 24-pin connector
 * @brief Constants for pins exposed on Arducam DVP 24-pin or 18-pin connector
 * @ingroup devicetree-gpio-pin-headers
 * @{
 */

#define DVP_24PIN_SDA		3	/**< I2C data pin */
#define DVP_24PIN_SCL		5	/**< I2C clock pin */
#define DVP_24PIN_PEN		6	/**< Reset / Power Enable pin */
#define DVP_24PIN_VS		7	/**< Vertical sync */
#define DVP_24PIN_PDN		8	/**< Power Down */
#define DVP_24PIN_HS		9	/**< Horizontal sync */
#define DVP_24PIN_8BIT_D7	12	/**< Parallel port data for 8-bit variants */
#define DVP_24PIN_10BIT_D9	12	/**< Parallel port data for 10-bit variants */
#define DVP_24PIN_XCLK		13	/**< System clock, also known as XCLK */
#define DVP_24PIN_8BIT_D6	14	/**< Parallel port data for 8-bit variants */
#define DVP_24PIN_10BIT_D8	14	/**< Parallel port data for 10-bit variants */
#define DVP_24PIN_8BIT_D5	16	/**< Parallel port data for 8-bit variants */
#define DVP_24PIN_10BIT_D7	16	/**< Parallel port data for 10-bit variants */
#define DVP_24PIN_PCLK		17	/**< Pixel clock used to transmit the data */
#define DVP_24PIN_8BIT_D4	18	/**< Parallel port data for 8-bit variants */
#define DVP_24PIN_10BIT_D6	18	/**< Parallel port data for 10-bit variants */
#define DVP_24PIN_8BIT_D0	19	/**< Parallel port data for 8-bit variants */
#define DVP_24PIN_10BIT_D2	19	/**< Parallel port data for 10-bit variants */
#define DVP_24PIN_8BIT_D3	20	/**< Parallel port data for 8-bit variants */
#define DVP_24PIN_10BIT_D5	20	/**< Parallel port data for 10-bit variants */
#define DVP_24PIN_8BIT_D1	21	/**< Parallel port data for 8-bit variants */
#define DVP_24PIN_10BIT_D3	21	/**< Parallel port data for 10-bit variants */
#define DVP_24PIN_8BIT_D2	22	/**< Parallel port data for 8-bit variants */
#define DVP_24PIN_10BIT_D4	22	/**< Parallel port data for 10-bit variants */
#define DVP_24PIN_10BIT_D1	23	/**< Parallel port data for 10-bit variants */
#define DVP_24PIN_10BIT_D0	24	/**< Parallel port data for 10-bit variants */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_DVP_24PIN_CONNECTOR_H_ */
