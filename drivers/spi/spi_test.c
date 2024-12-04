/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real SPI driver. It is used to instantiate struct
 * devices for the "vnd,spi" devicetree compatible used in test code.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>

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
				    spi_callback_t cb,
				    void *userdata)
{
	return -ENOTSUP;
}
#endif

static int vnd_spi_release(const struct device *dev,
			   const struct spi_config *spi_cfg)
{
	return -ENOTSUP;
}

static DEVICE_API(spi, vnd_spi_api) = {
	.transceive = vnd_spi_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = vnd_spi_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = vnd_spi_release,
};

#define VND_SPI_INIT(n)							\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,	\
			      CONFIG_SPI_INIT_PRIORITY,			\
			      &vnd_spi_api);

DT_INST_FOREACH_STATUS_OKAY(VND_SPI_INIT)
