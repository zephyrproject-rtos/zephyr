/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MEMC_MCUX_QSPI_H_
#define ZEPHYR_DRIVERS_MEMC_MCUX_QSPI_H_

#include <zephyr/device.h>
#include <fsl_qspi.h>

/* QSPI transfer descriptor. */
struct memc_mcux_qspi_transfer {
	uint32_t device_address; /* Operation device address. */
	bool is_read;            /* Execution command type. */
	uint8_t seq_index;       /* LUT entry index where sequence starts. */
	void *data;              /* Data buffer. */
	size_t data_size;        /* Data size in bytes. */
};

/**
 * @brief Configure new QSPI device
 *
 * Configures new device on the QSPI bus.
 * @param dev: QSPI device
 * @param flash_config: External flash configuration.
 * @param lut_array: Lookup table of QSPI flash commands for device
 * @param lut_count: number of LUT entries (4 bytes each) in lut array
 * @return 0 on success, negative value on failure
 */
int memc_qspi_set_device_config(const struct device *dev,
				const qspi_flash_config_t *flash_config,
				const uint32_t *lut_array,
				size_t lut_count);

/**
 * @brief Get AHB address for QSPI
 *
 * This address is memory mapped, and can be read and written as though it were internal memory.
 * @param dev: QSPI device
 * @return AHB access address for QSPI device
 */
uint32_t memc_mcux_qspi_get_ahb_address(const struct device *dev);

/**
 * @brief Execute a QSPI transfer (read or write)
 *
 * @param dev: QSPI device
 * @param transfer: Transfer descriptor
 * @return 0 on success, negative value on failure
 */
int memc_mcux_qspi_transfer(const struct device *dev, struct memc_mcux_qspi_transfer *transfer);

#endif /* ZEPHYR_DRIVERS_MEMC_MCUX_QSPI_H_ */
