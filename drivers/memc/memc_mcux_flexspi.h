/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <sys/types.h>
#include <fsl_flexspi.h>

enum memc_flexspi_clock_t {
	/* Flexspi clock 332M, DDR mode, internal clock 166M. */
	MEMC_FLEXSPI_CLOCK_166M,
	/* Flexspi clock 83M, DDR mode, internal clock 42M. */
	MEMC_FLEXSPI_CLOCK_42M,
};

void memc_flexspi_wait_bus_idle(const struct device *dev);

bool memc_flexspi_is_running_xip(const struct device *dev);

int memc_flexspi_update_lut(const struct device *dev, uint32_t index,
		const uint32_t *cmd, uint32_t count);

int memc_flexspi_update_clock(const struct device *dev,
		flexspi_device_config_t *device_config,
		flexspi_port_t port, enum memc_flexspi_clock_t clock);

int memc_flexspi_set_device_config(const struct device *dev,
		const flexspi_device_config_t *device_config,
		flexspi_port_t port);

int memc_flexspi_reset(const struct device *dev);

int memc_flexspi_transfer(const struct device *dev,
		flexspi_transfer_t *transfer);

void *memc_flexspi_get_ahb_address(const struct device *dev,
		flexspi_port_t port, off_t offset);
