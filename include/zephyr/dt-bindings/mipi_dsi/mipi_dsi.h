/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MIPI_DSI_MIPI_DSI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MIPI_DSI_MIPI_DSI_H_

/**
 * @brief MIPI-DSI driver APIs
 * @defgroup mipi_dsi_interface MIPI-DSI driver APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @name MIPI-DSI Pixel formats.
 * @{
 */

/** RGB888 (24bpp). */
#define MIPI_DSI_PIXFMT_RGB888		0U
/** RGB666 (24bpp). */
#define MIPI_DSI_PIXFMT_RGB666		1U
/** Packed RGB666 (18bpp). */
#define MIPI_DSI_PIXFMT_RGB666_PACKED	2U
/** RGB565 (16bpp). */
#define MIPI_DSI_PIXFMT_RGB565		3U

/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MIPI_DSI_MIPI_DSI_H_ */
