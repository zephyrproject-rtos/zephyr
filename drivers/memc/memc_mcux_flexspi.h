/*
 * Copyright 2020,2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <fsl_flexspi.h>

/* Size of a command in the LUT table */
#define MEMC_FLEXSPI_CMD_SIZE 4U
/* Number of commands in an instruction sequence */
#define MEMC_FLEXSPI_CMD_PER_SEQ 4U

/**
 * @brief Wait for the FlexSPI bus to be idle
 *
 * Waits for the FlexSPI bus to be idle. Can be used when reconfiguring
 * the FlexSPI to make sure no flash access is occurring before changing
 * settings.
 *
 * @param dev: FlexSPI device
 */
void memc_flexspi_wait_bus_idle(const struct device *dev);

/**
 * @brief Check if FlexSPI is being used in XIP mode.
 *
 * Checks if the FlexSPI is being used for code execution in the current
 * application.
 *
 * @param dev: FlexSPI device
 * @retval true if FlexSPI being used for XIP
 */
bool memc_flexspi_is_running_xip(const struct device *dev);

/**
 * @brief Update clock selection of the FlexSPI device
 *
 * Updates clock frequency of FlexSPI to new clock speed.
 *
 * @param dev: FlexSPI device
 * @param device_config: External device configuration.
 * @param port: FlexSPI port to use for this external device
 * @param freq_hz: new clock frequency to apply
 * @return 0 on success, negative value on failure
 */
int memc_flexspi_update_clock(const struct device *dev,
		flexspi_device_config_t *device_config,
		flexspi_port_t port, uint32_t freq_hz);

/**
 * @brief configure new FlexSPI device
 *
 * Configures new device on the FlexSPI bus.
 * @param dev: FlexSPI device
 * @param device_config: External device configuration.
 * @param lut_array: Lookup table of FlexSPI flash commands for device
 * @param lut_count: number of LUT entries (4 bytes each) in lut array
 * @param port: FlexSPI port to use for this external device
 * @return 0 on success, negative value on failure
 */
int memc_flexspi_set_device_config(const struct device *dev,
		const flexspi_device_config_t *device_config,
		const uint32_t *lut_array,
		uint8_t lut_count,
		flexspi_port_t port);


/**
 * @brief Perform software reset of FlexSPI
 *
 * Software reset of FlexSPI. Does not clear configuration registers.
 * @param dev: FlexSPI device
 * @return 0 on success, negative value on failure
 */
int memc_flexspi_reset(const struct device *dev);

/**
 * @brief Send blocking IP transfer
 *
 * Send blocking IP transfer using FlexSPI.
 * @param dev: FlexSPI device
 * @param transfer: FlexSPI transfer. seqIndex should be set  as though the
 *                  LUT array was loaded at offset 0.
 * @return 0 on success, negative value on failure
 */
int memc_flexspi_transfer(const struct device *dev,
		flexspi_transfer_t *transfer);

/**
 * @brief Get AHB address for FlexSPI port
 *
 * Get AHB address for FlexSPI port. This address is memory mapped, and can be
 * read from (and written to, for PSRAM) as though it were internal memory.
 * @param dev: FlexSPI device
 * @param port: FlexSPI port external device is on
 * @param offset: byte offset from start of device to get AHB address for
 * @return 0 on success, negative value on failure
 */
void *memc_flexspi_get_ahb_address(const struct device *dev, flexspi_port_t port, k_off_t offset);
