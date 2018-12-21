/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <spi.h>

int _impl_spi_transceive(struct device *dev,
			 const struct spi_config *config,
			 const struct spi_buf_set *tx_bufs,
			 const struct spi_buf_set *rx_bufs)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->driver_api;

	return api->transceive(dev, config, tx_bufs, rx_bufs);
}

int _impl_spi_release(struct device *dev,
		      const struct spi_config *config)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->driver_api;

	return api->release(dev, config);
}
