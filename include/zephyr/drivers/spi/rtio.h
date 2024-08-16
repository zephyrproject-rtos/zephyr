/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_RTIO_H_
#define ZEPHYR_DRIVERS_SPI_RTIO_H_

/**
 * @brief Fallback SPI RTIO submit implementation.
 *
 * Default RTIO SPI implementation for drivers who do no yet have
 * native support. For details, see @ref spi_iodev_submit.
 */
void spi_rtio_iodev_default_submit(const struct device *dev,
				   struct rtio_iodev_sqe *iodev_sqe);

#endif /* ZEPHYR_DRIVERS_SPI_RTIO_H_ */
