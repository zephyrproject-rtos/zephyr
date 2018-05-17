/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <spi.h>
#include <syscall_handler.h>

static void verify_spi_buf_array(const struct spi_buf *bufs, size_t len,
				 int writable, void *ssf)
{
	/* Empty array, nothing to do */
	if (!len) {
		return;
	}

	/* Validate the array of struct spi_buf instances */
	Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_READ(bufs, len, sizeof(struct spi_buf)));

	for ( ; len; --len, ++bufs) {
		/* Now for each array element, validate the memory buffers
		 * that they point to
		 */
		Z_OOPS(Z_SYSCALL_MEMORY(bufs->buf, bufs->len, writable));
	}
}

Z_SYSCALL_HANDLER(spi_transceive, dev, config_p, tx_bufs, rx_bufs)
{
	const struct spi_config *config = (const struct spi_config *)config_p;

	Z_OOPS(Z_SYSCALL_MEMORY_READ(config, sizeof(*config)));
	Z_OOPS(Z_SYSCALL_DRIVER_SPI(dev, transceive));

	/* ssf is implicit system call stack frame parameter, used by
	 * _SYSCALL_* APIs when something goes wrong.
	 */
	if (tx_bufs) {
		const struct spi_buf_set *tx =
			(const struct spi_buf_set *)tx_bufs;

		Z_OOPS(Z_SYSCALL_MEMORY_READ(tx_bufs,
					     sizeof(struct spi_buf_set)));
		verify_spi_buf_array(tx->buffers, tx->count, 0, ssf);
	}

	if (rx_bufs) {
		const struct spi_buf_set *rx =
			(const struct spi_buf_set *)rx_bufs;

		Z_OOPS(Z_SYSCALL_MEMORY_READ(rx_bufs,
					     sizeof(struct spi_buf_set));
		verify_spi_buf_array(rx->buffers, rx->count, 1, ssf);
	}

	if (config->cs) {
		const struct spi_cs_control *cs = config->cs;

		Z_OOPS(Z_SYSCALL_MEMORY_READ(cs, sizeof(*cs)));
		if (cs->gpio_dev) {
			Z_OOPS(Z_SYSCALL_OBJ(cs->gpio_dev, K_OBJ_DRIVER_GPIO));
		}
	}

	return _impl_spi_transceive((struct device *)dev, config,
				    (const struct spi_buf_set *)tx_bufs,
				    (const struct spi_buf_set *)rx_bufs);
}

Z_SYSCALL_HANDLER(spi_release, dev, config_p)
{
	const struct spi_config *config = (const struct spi_config *)config_p;

	Z_OOPS(Z_SYSCALL_MEMORY_READ(config, sizeof(*config)));
	Z_OOPS(Z_SYSCALL_DRIVER_SPI(dev, release));
	return _impl_spi_release((struct device *)dev, config);
}
