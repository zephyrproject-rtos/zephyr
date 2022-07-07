/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SPI_SPI_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SPI_SPI_EMUL_H_

/**
 * @file
 *
 * @brief Public APIs for the SPI emulation drivers.
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>

/**
 * @brief SPI Emulation Interface
 * @defgroup spi_emul_interface SPI Emulation Interface
 * @ingroup io_emulators
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

struct spi_msg;
struct spi_emul_api;

/** Node in a linked list of emulators for SPI devices */
struct spi_emul {
	sys_snode_t node;

	/** Parent emulator */
	const struct emul *parent;

	/* API provided for this device */
	const struct spi_emul_api *api;

	/* SPI chip-select of the emulated device */
	uint16_t chipsel;
};

/**
 * Passes SPI messages to the emulator. The emulator updates the data with what
 * was read back.
 *
 * @param emul Emulator instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
typedef int (*spi_emul_io_t)(struct spi_emul *emul,
			     const struct spi_config *config,
			     const struct spi_buf_set *tx_bufs,
			     const struct spi_buf_set *rx_bufs);

/**
 * Register an emulated device on the controller
 *
 * @param dev Device that will use the emulator
 * @param name User-friendly name for this emulator
 * @param emul SPI emulator to use
 * @return 0 indicating success (always)
 */
int spi_emul_register(const struct device *dev, const char *name,
		      struct spi_emul *emul);

/** Definition of the emulator API */
struct spi_emul_api {
	spi_emul_io_t io;
};

/**
 * Back door to allow an emulator to retrieve the host configuration.
 *
 * @param dev SPI device associated with the emulator
 * @return Bit-packed 32-bit value containing the device's runtime configuration
 */
uint32_t spi_emul_get_config(const struct device *dev);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SPI_SPI_EMUL_H_ */
