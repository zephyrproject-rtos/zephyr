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
 * @brief SPI duplex mode
 *
 * Some controllers support half duplex transfer, which results in 3-wire usage.
 * By default, full duplex will prevail.
 */
#define SPI_FULL_DUPLEX		(0U << 11)
#define SPI_HALF_DUPLEX		(1U << 11)

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SPI_SPI_H_ */
