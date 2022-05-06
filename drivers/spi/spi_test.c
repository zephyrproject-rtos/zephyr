/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real SPI driver. It is used to instantiate struct
 * devices for the "vnd,spi" devicetree compatible used in test code.
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/spi.h>

#define DT_DRV_COMPAT vnd_spi

static int vnd_spi_transceive(const struct device *dev,
			      const struct spi_config *spi_cfg,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs)
{
	return -ENOTSUP;
}

#ifdef CONFIG_SPI_ASYNC
static int vnd_spi_transceive_async(const struct device *dev,
				    const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs,
				    struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif

static int vnd_spi_release(const struct device *dev,
			   const struct spi_config *spi_cfg)
{
	return -ENOTSUP;
}

static const struct spi_driver_api vnd_spi_api = {
	.transceive = vnd_spi_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = vnd_spi_transceive_async,
#endif
	.release = vnd_spi_release,
};

static int vnd_spi_init(const struct device *dev)
{
	return 0;
}

#define VND_SPI_INIT(n)						\
	DEVICE_DT_INST_DEFINE(n, &vnd_spi_init, NULL,			\
			      NULL, NULL, POST_KERNEL,			\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &vnd_spi_api);

DT_INST_FOREACH_STATUS_OKAY(VND_SPI_INIT)
