/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/spi.h>
#include <syscall_handler.h>
#include <string.h>

/* This assumes that bufs and buf_copy are copies from the values passed
 * as syscall arguments.
 */
static void copy_and_check(struct spi_buf_set *bufs,
			   struct spi_buf *buf_copy,
			   int writable)
{
	size_t i;

	if (bufs->count == 0) {
		bufs->buffers = NULL;
		return;
	}

	/* Validate the array of struct spi_buf instances */
	Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_READ(bufs->buffers,
					   bufs->count,
					   sizeof(struct spi_buf)));

	/* Not worried abuot overflow here: _SYSCALL_MEMORY_ARRAY_READ()
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

		Z_OOPS(Z_SYSCALL_MEMORY(buf->buf, buf->len, writable));
	}
}

/* This function is only here so tx_buf_copy and rx_buf_copy can be allocated
 * using VLA.  It assumes that both tx_bufs and rx_bufs will receive a copy of
 * the values passed to the syscall as arguments.  It also assumes that the
 * count member has been verified and is a value that won't lead to stack
 * overflow.
 */
static u32_t copy_bufs_and_transceive(struct device *dev,
				      const struct spi_config *config,
				      struct spi_buf_set *tx_bufs,
				      struct spi_buf_set *rx_bufs)
{
	struct spi_buf tx_buf_copy[tx_bufs->count ? tx_bufs->count : 1];
	struct spi_buf rx_buf_copy[rx_bufs->count ? rx_bufs->count : 1];

	copy_and_check(tx_bufs, tx_buf_copy, 0);
	copy_and_check(rx_bufs, rx_buf_copy, 1);

	return z_impl_spi_transceive((struct device *)dev, config,
				    tx_bufs, rx_bufs);
}

static inline int z_vrfy_spi_transceive(struct device *dev,
				       const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs)
{
	struct spi_buf_set tx_bufs_copy;
	struct spi_buf_set rx_bufs_copy;
	struct spi_config config_copy;

	Z_OOPS(Z_SYSCALL_MEMORY_READ(config, sizeof(*config)));
	Z_OOPS(Z_SYSCALL_DRIVER_SPI(dev, transceive));

	if (tx_bufs) {
		const struct spi_buf_set *tx =
			(const struct spi_buf_set *)tx_bufs;

		Z_OOPS(Z_SYSCALL_MEMORY_READ(tx_bufs,
					     sizeof(struct spi_buf_set)));
		memcpy(&tx_bufs_copy, tx, sizeof(tx_bufs_copy));
		Z_OOPS(Z_SYSCALL_VERIFY(tx_bufs_copy.count < 32));
	} else {
		memset(&tx_bufs_copy, 0, sizeof(tx_bufs_copy));
	}

	if (rx_bufs) {
		const struct spi_buf_set *rx =
			(const struct spi_buf_set *)rx_bufs;

		Z_OOPS(Z_SYSCALL_MEMORY_READ(rx_bufs,
					     sizeof(struct spi_buf_set)));
		memcpy(&rx_bufs_copy, rx, sizeof(rx_bufs_copy));
		Z_OOPS(Z_SYSCALL_VERIFY(rx_bufs_copy.count < 32));
	} else {
		memset(&rx_bufs_copy, 0, sizeof(rx_bufs_copy));
	}

	memcpy(&config_copy, config, sizeof(*config));
	if (config_copy.cs) {
		const struct spi_cs_control *cs = config_copy.cs;

		Z_OOPS(Z_SYSCALL_MEMORY_READ(cs, sizeof(*cs)));
		if (cs->gpio_dev) {
			Z_OOPS(Z_SYSCALL_OBJ(cs->gpio_dev, K_OBJ_DRIVER_GPIO));
		}
	}

	return copy_bufs_and_transceive((struct device *)dev,
					&config_copy,
					&tx_bufs_copy,
					&rx_bufs_copy);
}
#include <syscalls/spi_transceive_mrsh.c>

static inline int z_vrfy_spi_release(struct device *dev,
				    const struct spi_config *config)
{
	Z_OOPS(Z_SYSCALL_MEMORY_READ(config, sizeof(*config)));
	Z_OOPS(Z_SYSCALL_DRIVER_SPI(dev, release));
	return z_impl_spi_release((struct device *)dev, config);
}
#include <syscalls/spi_release_mrsh.c>
