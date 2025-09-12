/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ArduCam FFC 40-pin camera connector constants.
 * @ingroup arducam-ffc-40pin
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_ARDUCAM_FFC_40PIN_CONNECTOR_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_ARDUCAM_FFC_40PIN_CONNECTOR_H_

/**
 * @defgroup arducam-ffc-40pin ArduCam FFC 40-pin camera connector.
 * @brief Constants for pins exposed on ArduCam FFC 40-pin camera connector.
 * @ingroup devicetree-gpio-pin-headers
 * @{
 */

/**
 * @name MIPI-CSI2 interface pins
 * @{
 */
#define ARDUCAM_FFC_40PIN_MIPI_CSI_DL1_P 5  /**< MIPI‑CSI2 data lane 1 positive */
#define ARDUCAM_FFC_40PIN_MIPI_CSI_DL1_N 6  /**< MIPI‑CSI2 data lane 1 negative */
#define ARDUCAM_FFC_40PIN_MIPI_CSI_CL_P  8  /**< MIPI‑CSI2 clock lane positive */
#define ARDUCAM_FFC_40PIN_MIPI_CSI_CL_N  9  /**< MIPI‑CSI2 clock lane negative */
#define ARDUCAM_FFC_40PIN_MIPI_CSI_DL0_P 11 /**< MIPI‑CSI2 data lane 0 positive */
#define ARDUCAM_FFC_40PIN_MIPI_CSI_DL0_N 12 /**< MIPI‑CSI2 data lane 0 negative */
/** @} */

/**
 * @name DVP interface pins
 * @{
 */
#define ARDUCAM_FFC_40PIN_D11   2  /**< Parallel port data bit 11 */
#define ARDUCAM_FFC_40PIN_D10   3  /**< Parallel port data bit 10 */
#define ARDUCAM_FFC_40PIN_D9    5  /**< Parallel port data bit 9 */
#define ARDUCAM_FFC_40PIN_D8    6  /**< Parallel port data bit 8 */
#define ARDUCAM_FFC_40PIN_D7    8  /**< Parallel port data bit 7 */
#define ARDUCAM_FFC_40PIN_D6    9  /**< Parallel port data bit 6 */
#define ARDUCAM_FFC_40PIN_D5    11 /**< Parallel port data bit 5 */
#define ARDUCAM_FFC_40PIN_D4    12 /**< Parallel port data bit 4 */
#define ARDUCAM_FFC_40PIN_D3    14 /**< Parallel port data bit 3 */
#define ARDUCAM_FFC_40PIN_D2    15 /**< Parallel port data bit 2 */
#define ARDUCAM_FFC_40PIN_D1    17 /**< Parallel port data bit 1 */
#define ARDUCAM_FFC_40PIN_D0    18 /**< Parallel port data bit 0 */
#define ARDUCAM_FFC_40PIN_VSYNC 23 /**< Vertical sync */
#define ARDUCAM_FFC_40PIN_HSYNC 24 /**< Horizontal sync */
/** @} */

/**
 * @name Common pins
 * @{
 */
#define ARDUCAM_FFC_40PIN_SCL   20 /**< I2C clock line */
#define ARDUCAM_FFC_40PIN_SDA   21 /**< I2C data line */
#define ARDUCAM_FFC_40PIN_RESET 25 /**< Reset */
#define ARDUCAM_FFC_40PIN_XCLK  26 /**< External clock */
#define ARDUCAM_FFC_40PIN_PCLK  27 /**< Pixel clock */
#define ARDUCAM_FFC_40PIN_INT   28 /**< Interrupt from sensor */
/** @} */

/** @} */

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_ARDUCAM_FFC_40PIN_CONNECTOR_H_ */
