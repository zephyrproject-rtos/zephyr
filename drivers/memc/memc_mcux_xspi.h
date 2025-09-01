/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <fsl_xspi.h>

/**
 * @brief Get XSPI root clock frequency with Hz.
 *
 * @return 0 on success, negative value on failure
 */
int memc_mcux_xspi_get_root_clock(const struct device *dev, uint32_t *clock_rate);

/**
 * @brief Wait the XSPI bus idle status.
 *
 * @param dev: XSPI device
 */
void memc_xspi_wait_bus_idle(const struct device *dev);

/**
 * @brief Check whether XSPI is running in XIP mode.
 *
 * @return 0 - In XIP mode, 1 - In XIP mode.
 */
bool memc_xspi_is_running_xip(const struct device *dev);

/**
 * @brief Configure new XSPI device
 *
 * Configures new device on the XSPI bus.
 * @param dev: XSPI device
 * @param device_config: External device configuration.
 * @param lut_array: Lookup table of XSPI flash commands for device
 * @param lut_count: number of LUT entries (4 bytes each) in lut array
 * @return 0 on success, negative value on failure
 */
int memc_xspi_set_device_config(const struct device *dev,
		const xspi_device_config_t *device_config,
		const uint32_t *lut_array,
		uint8_t lut_count);

/**
 * @brief Get AHB address for XSPI
 *
 * This address is memory mapped, and can be read and written as though it were internal memory.
 * @param dev: XSPI device
 * @return AHB access address for XSPI device
 */
uint32_t memc_mcux_xspi_get_ahb_address(const struct device *dev);

/**
 * @brief Send blocking IP transfer
 *
 * Send blocking IP transfer using XSPI.
 * @param dev: XSPI device
 * @param xfer: XSPI transfer structure
 * @return 0 on success, negative value on failure
 */
int memc_mcux_xspi_transfer(const struct device *dev, xspi_transfer_t *xfer);
