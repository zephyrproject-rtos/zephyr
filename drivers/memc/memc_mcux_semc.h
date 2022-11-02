/*
 * Copyright 2022 Macronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <sys/types.h>
#include <fsl_semc.h>

bool memc_semc_is_nand_ready(const struct device *dev);

status_t memc_semc_send_ipcommand(const struct device *dev, uint32_t address, uint32_t command, uint32_t write, uint32_t *read);

status_t memc_semc_ipcommand_nand_write(const struct device *dev, uint32_t address, uint8_t *buffer, uint32_t size_bytes);

status_t memc_semc_ipcommand_nand_read(const struct device *dev, uint32_t address, uint8_t *buffer, uint32_t size_bytes);

status_t memc_semc_configure_nand(const struct device *dev, onfi_nand_config_t *config);
