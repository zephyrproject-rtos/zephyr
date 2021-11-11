/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SPI_SPI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SPI_SPI_H_

/**
 * @brief SPI Interface
 * @defgroup spi_interface SPI Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @name SPI duplex mode
 * @{
 *
 * Some controllers support half duplex transfer, which results in 3-wire usage.
 * By default, full duplex will prevail.
 */
#define SPI_FULL_DUPLEX		(0U << 11)
#define SPI_HALF_DUPLEX		(1U << 11)
/** @} */

/**
 * @name SPI Frame Format
 * @{
 *
 * 2 frame formats are exposed: Motorola and TI.
 * The main difference is the behavior of the CS line. In Motorala it stays
 * active the whole transfer. In TI, it's active only one serial clock period
 * prior to actually make the transfer, it is thus inactive during the transfer,
 * which ends when the clocks ends as well.
 * By default, as it is the most commonly used, the Motorola frame format
 * will prevail.
 */
#define SPI_FRAME_FORMAT_MOTOROLA	(0U << 15)
#define SPI_FRAME_FORMAT_TI		(1U << 15)
/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SPI_SPI_H_ */
