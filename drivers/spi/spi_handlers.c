/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/internal/syscall_handler.h>
#include <string.h>

/* This assumes that bufs and buf_copy are copies from the values passed
 * as syscall arguments.
 */
static struct spi_buf_set *copy_and_check(struct spi_buf_set *bufs,
					  struct spi_buf *buf_copy,
					  int writable)
{
	size_t i;

	if (bufs->count == 0) {
		bufs->buffers = NULL;
		return NULL;
	}

	/* Validate the array of struct spi_buf instances */
	K_OOPS(K_SYSCALL_MEMORY_ARRAY_READ(bufs->buffers,
					   bufs->count,
					   sizeof(struct spi_buf)));

	/* Not worried about overflow here: _SYSCALL_MEMORY_ARRAY_READ()
	 * takes care of it.
	 */
	bufs->buffers = memcpy(buf_copy,
			       bufs->buffers,
			       bufs->count * sizeof(struct spi_buf));

	for (i = 0; i < bufs->count; i++) {
		/* Now for each array element, validate the memory buffers
		 * that they point to
		 */
		const struct spi_buf *buf = &bufs->buffers[i];

		K_OOPS(K_SYSCALL_MEMORY(buf->buf, buf->len, writable));
	}

	return bufs;
}

/* This function is only here so tx_buf_copy and rx_buf_copy can be allocated
 * using VLA.  It assumes that both tx_bufs and rx_bufs will receive a copy of
 * the values passed to the syscall as arguments.  It also assumes that the
 * count member has been verified and is a value that won't lead to stack
 * overflow.
 */
static uint32_t copy_bufs_and_transceive(const struct device *dev,
					 const struct spi_config *config,
					 struct spi_buf_set *tx_bufs,
					 struct spi_buf_set *rx_bufs)
{
	struct spi_buf tx_buf_copy[tx_bufs->count ? tx_bufs->count : 1];
	struct spi_buf rx_buf_copy[rx_bufs->count ? rx_bufs->count : 1];

	tx_bufs = copy_and_check(tx_bufs, tx_buf_copy, 0);
	rx_bufs = copy_and_check(rx_bufs, rx_buf_copy, 1);

	return z_impl_spi_transceive((const struct device *)dev, config,
				     tx_bufs, rx_bufs);
}

static inline int z_vrfy_spi_transceive(const struct device *dev,
					const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	struct spi_buf_set tx_bufs_copy;
	struct spi_buf_set rx_bufs_copy;
	struct spi_config config_copy;

	K_OOPS(K_SYSCALL_MEMORY_READ(config, sizeof(*config)));
	K_OOPS(K_SYSCALL_DRIVER_SPI(dev, transceive));

	if (tx_bufs) {
		const struct spi_buf_set *tx =
			(const struct spi_buf_set *)tx_bufs;

		K_OOPS(K_SYSCALL_MEMORY_READ(tx_bufs,
					     sizeof(struct spi_buf_set)));
		memcpy(&tx_bufs_copy, tx, sizeof(tx_bufs_copy));
		K_OOPS(K_SYSCALL_VERIFY(tx_bufs_copy.count < 32));
	} else {
		memset(&tx_bufs_copy, 0, sizeof(tx_bufs_copy));
	}

	if (rx_bufs) {
		const struct spi_buf_set *rx =
			(const struct spi_buf_set *)rx_bufs;

		K_OOPS(K_SYSCALL_MEMORY_READ(rx_bufs,
					     sizeof(struct spi_buf_set)));
		memcpy(&rx_bufs_copy, rx, sizeof(rx_bufs_copy));
		K_OOPS(K_SYSCALL_VERIFY(rx_bufs_copy.count < 32));
	} else {
		memset(&rx_bufs_copy, 0, sizeof(rx_bufs_copy));
	}

	memcpy(&config_copy, config, sizeof(*config));
	if (spi_cs_is_gpio(&config_copy)) {
		K_OOPS(K_SYSCALL_OBJ(config_copy.cs.gpio.port,
				     K_OBJ_DRIVER_GPIO));
	}

	return copy_bufs_and_transceive((const struct device *)dev,
					&config_copy,
					&tx_bufs_copy,
					&rx_bufs_copy);
}
#include <syscalls/spi_transceive_mrsh.c>

static inline int z_vrfy_spi_release(const struct device *dev,
				     const struct spi_config *config)
{
	K_OOPS(K_SYSCALL_MEMORY_READ(config, sizeof(*config)));
	K_OOPS(K_SYSCALL_DRIVER_SPI(dev, release));
	return z_impl_spi_release((const struct device *)dev, config);
}
#include <syscalls/spi_release_mrsh.c>
