/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for SPI driver APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SPI_H_
#error "Should only be included by zephyr/drivers/spi.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_SPI_INTERNAL_SPI_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SPI_INTERNAL_SPI_IMPL_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_spi_transceive(const struct device *dev,
					const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->api;
	int ret;

	ret = api->transceive(dev, config, tx_bufs, rx_bufs);
	spi_transceive_stats(dev, ret, tx_bufs, rx_bufs);

	return ret;
}

static inline int z_impl_spi_release(const struct device *dev,
				     const struct spi_config *config)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->api;

	return api->release(dev, config);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SPI_INTERNAL_SPI_IMPL_H_ */
