/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <spi.h>
#include <syscall_handler.h>

#ifndef CONFIG_SPI_LEGACY_API
static void verify_spi_buf_array(struct spi_buf *bufs, size_t len,
				 int writable, void *ssf)
{
	/* Empty array, nothing to do */
	if (!len) {
		return;
	}

	/* Validate the array of struct spi_buf instances */
	_SYSCALL_MEMORY_ARRAY_READ(bufs, len, sizeof(struct spi_buf));

	for ( ; len; --len, ++bufs) {
		/* Now for each array element, validate the memory buffers
		 * that they point to
		 */
		_SYSCALL_MEMORY(bufs->buf, bufs->len, writable);
	}
}

_SYSCALL_HANDLER(spi_transceive, config_p, tx_bufs, tx_count, rx_bufs,
		 rx_count)
{
	struct spi_config *config = (struct spi_config *)config_p;

	_SYSCALL_MEMORY_READ(config, sizeof(*config));
	_SYSCALL_OBJ(config->dev, K_OBJ_DRIVER_SPI);

	/* ssf is implicit system call stack frame parameter, used by
	 * _SYSCALL_* APIs when something goes wrong.
	 */
	verify_spi_buf_array((struct spi_buf *)tx_bufs, tx_count, 0, ssf);
	verify_spi_buf_array((struct spi_buf *)rx_bufs, rx_count, 1, ssf);

	if (config->cs) {
		struct spi_cs_control *cs = config->cs;

		_SYSCALL_MEMORY_READ(cs, sizeof(*cs));
		if (cs->gpio_dev) {
			_SYSCALL_OBJ(cs->gpio_dev, K_OBJ_DRIVER_GPIO);
		}
	}

	return _impl_spi_transceive(config, (const struct spi_buf *)tx_bufs,
				    tx_count, (struct spi_buf *)rx_bufs,
				    rx_count);
}

_SYSCALL_HANDLER(spi_release, config_p)
{
	struct spi_config *config = (struct spi_config *)config_p;

	_SYSCALL_MEMORY_READ(config, sizeof(*config));
	_SYSCALL_OBJ(config->dev, K_OBJ_DRIVER_SPI);
	return _impl_spi_release(config);
}
#endif
