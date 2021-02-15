/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/flash.h>
#include <fsl_flexspi.h>

int flash_flexspi_update_lut(const struct device *dev, uint32_t index,
		const uint32_t *cmd, uint32_t count);

int flash_flexspi_set_flash_config(const struct device *dev,
		const flexspi_device_config_t *device_config,
		flexspi_port_t port);

int flash_flexspi_reset(const struct device *dev);

int flash_flexspi_transfer(const struct device *dev,
		flexspi_transfer_t *transfer);

void *flash_flexspi_get_ahb_address(const struct device *dev,
		flexspi_port_t port, off_t offset);
