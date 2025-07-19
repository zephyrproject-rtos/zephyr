/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Generic commands for MSPI NOR devices */

#include "flash_mspi_nor.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(flash_mspi_nor, CONFIG_FLASH_LOG_LEVEL);

const struct flash_mspi_nor_cmds commands_single = {
	.id = {
		.dir = MSPI_RX,
		.cmd = JESD216_CMD_READ_ID,
		.cmd_length = 1,
	},
	.write_en = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_CMD_WREN,
		.cmd_length = 1,
	},
	.read = {
		.dir = MSPI_RX,
		.cmd = SPI_NOR_CMD_READ_FAST,
		.cmd_length = 1,
		.addr_length = 3,
		.rx_dummy = 8,
	},
	.status = {
		.dir = MSPI_RX,
		.cmd = SPI_NOR_CMD_RDSR,
		.cmd_length = 1,
	},
	.config = {
		.dir = MSPI_RX,
		.cmd = SPI_NOR_CMD_RDCR,
		.cmd_length = 1,
	},
	.page_program = {
		.dir  = MSPI_TX,
		.cmd = SPI_NOR_CMD_PP,
		.cmd_length = 1,
		.addr_length = 3,
	},
	.sector_erase = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_CMD_SE,
		.cmd_length = 1,
		.addr_length = 3,
	},
	.chip_erase = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_CMD_CE,
		.cmd_length = 1,
	},
	.sfdp = {
		.dir = MSPI_RX,
		.cmd = JESD216_CMD_READ_SFDP,
		.cmd_length = 1,
		.addr_length = 3,
		.rx_dummy = 8,
	},
};

const struct flash_mspi_nor_cmds commands_quad_1_4_4 = {
	.id = {
		.dir = MSPI_RX,
		.cmd = JESD216_CMD_READ_ID,
		.cmd_length = 1,
		.force_single = true,
	},
	.write_en = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_CMD_WREN,
		.cmd_length = 1,
	},
	.read = {
		.dir = MSPI_RX,
		.cmd = SPI_NOR_CMD_4READ,
		.cmd_length = 1,
		.addr_length = 3,
		.rx_dummy = 6,
	},
	.status = {
		.dir = MSPI_RX,
		.cmd = SPI_NOR_CMD_RDSR,
		.cmd_length = 1,
		.force_single = true,
	},
	.config = {
		.dir = MSPI_RX,
		.cmd = SPI_NOR_CMD_RDCR,
		.cmd_length = 1,
		.force_single = true,
	},
	.page_program = {
		.dir  = MSPI_TX,
		.cmd = SPI_NOR_CMD_PP_1_4_4,
		.cmd_length = 1,
		.addr_length = 3,
	},
	.sector_erase = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_CMD_SE,
		.cmd_length = 1,
		.addr_length = 3,
		.force_single = true,
	},
	.chip_erase = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_CMD_CE,
		.cmd_length = 1,
	},
	.sfdp = {
		.dir = MSPI_RX,
		.cmd = JESD216_CMD_READ_SFDP,
		.cmd_length = 1,
		.addr_length = 3,
		.rx_dummy = 8,
		.force_single = true,
	},
};

int flash_mspi_nor_probe_dev(const struct device *mspi,
			     struct flash_mspi_mode_data *flash_dev,
			     const struct flash_mspi_mode_data **vendor_devs,
			     uint32_t dev_count)
{
	int rc;
	uint8_t id[JESD216_READ_ID_LEN] = {0};

	rc = read_jedec_id(mspi, id);
	if (rc < 0) {
		LOG_ERR("Failed to read JEDEC ID: %d", rc);
		return rc;
	}
	for (int i = 0; i < dev_count; i++) {
		if (memcmp(id, &vendor_devs[i]->flash_data->jedec_id, sizeof(id)) == 0 &&
		    vendor_devs[i]->dev_cfg.io_mode == flash_dev->dev_cfg.io_mode) {
			/* Copy all data but the device configuration */
			flash_dev->flash_data = vendor_devs[i]->flash_data;
			flash_dev->jedec_cmds = vendor_devs[i]->jedec_cmds;
			flash_dev->quirks = vendor_devs[i]->quirks;
			LOG_DBG("Found device: %02x %02x %02x",
			 id[0], id[1], id[2]);
			return 0;
		}
	}
	LOG_ERR("Device not found: %02x %02x %02x", id[0], id[1], id[2]);
	return -ENODEV;
}
