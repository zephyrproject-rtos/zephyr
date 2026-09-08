/*
 * SPDX-FileCopyrightText: Copyright Panoramix Labs
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
 * @defgroup dvp-24pin-connector 24-pin FFC connector for DVP cameras
 * @brief Mapping for pins exposed on the 24-pin FFC connector of multiple similar camera modules
 * with a paralle ldata interface.
 * @ingroup devicetree-gpio-pin-headers
 * @{
 */

#define DVP_24PIN_SDA		3	/**< I2C data pin */

#define DVP_24PIN_SCL		5	/**< I2C clock pin */

#define DVP_24PIN_RST		6	/**< Reset pin */

#define DVP_24PIN_VSYNC		7	/**< Vertical sync */

#define DVP_24PIN_PWDN		8	/**< Power Down */

#define DVP_24PIN_HSYNC		9	/**< Horizontal Sync */

#define DVP_24PIN_8BIT_D7	12	/**< Parallel port data for 8 bits wide bus */
#define DVP_24PIN_10BIT_D9	12	/**< Parallel port data for 10 bits wide bus */

#define DVP_24PIN_MCLK		13	/**< System/master clock provided to the sensor */

#define DVP_24PIN_8BIT_D6	14	/**< Parallel port data for 8 bits wide bus */
#define DVP_24PIN_10BIT_D8	14	/**< Parallel port data for 10 bits wide bus */

#define DVP_24PIN_8BIT_D5	16	/**< Parallel port data for 8 bits wide bus */
#define DVP_24PIN_10BIT_D7	16	/**< Parallel port data for 10 bits wide bus */

#define DVP_24PIN_PCLK		17	/**< Pixel clock synchronized with the data */

#define DVP_24PIN_8BIT_D4	18	/**< Parallel port data for 8 bits wide bus */
#define DVP_24PIN_10BIT_D6	18	/**< Parallel port data for 10 bits wide bus */

#define DVP_24PIN_8BIT_D0	19	/**< Parallel port data for 8 bits wide bus */
#define DVP_24PIN_10BIT_D2	19	/**< Parallel port data for 10 bits wide bus */

#define DVP_24PIN_8BIT_D3	20	/**< Parallel port data for 8 bits wide bus */
#define DVP_24PIN_10BIT_D5	20	/**< Parallel port data for 10 bits wide bus */

#define DVP_24PIN_8BIT_D1	21	/**< Parallel port data for 8 bits wide bus */
#define DVP_24PIN_10BIT_D3	21	/**< Parallel port data for 10 bits wide bus */

#define DVP_24PIN_8BIT_D2	22	/**< Parallel port data for 8 bits wide bus */
#define DVP_24PIN_10BIT_D4	22	/**< Parallel port data for 10 bits wide bus */

#define DVP_24PIN_10BIT_D1	23	/**< Parallel port data for 10 bits wide bus */

#define DVP_24PIN_10BIT_D0	24	/**< Parallel port data for 10 bits wide bus */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_DVP_24PIN_CONNECTOR_H_ */
