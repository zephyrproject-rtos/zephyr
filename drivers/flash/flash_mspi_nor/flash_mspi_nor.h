/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FLASH_MSPI_NOR_H__
#define __FLASH_MSPI_NOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/mspi.h>
#include "../jesd216.h"
#include "../spi_nor.h"


#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define WITH_RESET_GPIO 1
#endif

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
#define FLASH_PAGE_LAYOUT_DEFINE(page_size, flash_size)                         \
	.layout = {                                                             \
		.pages_size = page_size,                                        \
		.pages_count = flash_size / page_size,                          \
	},
#else
#define FLASH_PAGE_LAYOUT_DEFINE(page_size, flash_size)
#endif

/*
 * Per device configuration. Stored in ROM unless
 * CONFIG_FLASH_MSPI_NOR_RUNTIME_PROBE is enabled.
 */
struct flash_mspi_device_data {
	uint8_t jedec_id[SPI_NOR_MAX_ID_LEN];
	uint8_t dw15_qer;
	uint32_t flash_size;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
};

/*
 * Per mode configuration. Stored in ROM unless
 * CONFIG_FLASH_MSPI_NOR_RUNTIME_PROBE is enabled.
 */
struct flash_mspi_mode_data{
	const struct flash_mspi_nor_cmds *jedec_cmds;
	const struct flash_mspi_nor_quirks *quirks;
	const struct flash_mspi_device_data *flash_data;
	struct mspi_dev_cfg dev_cfg;
};

struct flash_mspi_nor_config {
	const struct device *bus;
	struct mspi_dev_id mspi_id;
	struct mspi_dev_cfg mspi_nor_init_cfg;
	enum mspi_dev_cfg_mask mspi_nor_cfg_mask;
#if defined(CONFIG_MSPI_XIP)
	struct mspi_xip_cfg xip_cfg;
#endif
#if defined(WITH_RESET_GPIO)
	struct gpio_dt_spec reset;
	uint32_t reset_pulse_us;
	uint32_t reset_recovery_us;
#endif
	uint32_t transfer_timeout;
	const struct flash_mspi_mode_data *mode_data;
};

#define FLASH_MODE_DATA(dev) (((const struct flash_mspi_nor_config *)(dev->config))->mode_data)
#define FLASH_DATA(dev) (((const struct flash_mspi_nor_config *)(dev->config))->mode_data->flash_data)

struct flash_mspi_nor_data {
	struct k_sem acquired;
	struct mspi_xfer_packet packet;
	struct mspi_xfer xfer;
	struct mspi_dev_cfg *curr_cfg;
};

struct flash_mspi_nor_cmd {
	enum mspi_xfer_direction dir;
	uint32_t cmd;
	uint16_t tx_dummy;
	uint16_t rx_dummy;
	uint8_t cmd_length;
	uint8_t addr_length;
	bool force_single;
};

struct flash_mspi_nor_cmds {
	struct flash_mspi_nor_cmd id;
	struct flash_mspi_nor_cmd write_en;
	struct flash_mspi_nor_cmd read;
	struct flash_mspi_nor_cmd status;
	struct flash_mspi_nor_cmd config;
	struct flash_mspi_nor_cmd page_program;
	struct flash_mspi_nor_cmd sector_erase;
	struct flash_mspi_nor_cmd chip_erase;
	struct flash_mspi_nor_cmd sfdp;
};

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

const struct flash_mspi_nor_cmds commands_octal = {
	.id = {
		.dir = MSPI_RX,
		.cmd = JESD216_OCMD_READ_ID,
		.cmd_length = 2,
		.addr_length = 4,
		.rx_dummy = 4
	},
	.write_en = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_OCMD_WREN,
		.cmd_length = 2,
	},
	.read = {
		.dir = MSPI_RX,
		.cmd = SPI_NOR_OCMD_RD,
		.cmd_length = 2,
		.addr_length = 4,
		.rx_dummy = 20,
	},
	.status = {
		.dir = MSPI_RX,
		.cmd = SPI_NOR_OCMD_RDSR,
		.cmd_length = 2,
		.addr_length = 4,
		.rx_dummy = 4,
	},
	.page_program = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_OCMD_PAGE_PRG,
		.cmd_length = 2,
		.addr_length = 4,
	},
	.sector_erase = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_OCMD_SE,
		.cmd_length = 2,
		.addr_length = 4,
	},
	.chip_erase = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_OCMD_CE,
		.cmd_length = 2,
	},
	.sfdp = {
		.dir = MSPI_RX,
		.cmd = JESD216_OCMD_READ_SFDP,
		.cmd_length = 2,
		.addr_length = 4,
		.rx_dummy = 20,
	},
};

void flash_mspi_command_set(const struct device *dev, const struct flash_mspi_nor_cmd *cmd);

#ifdef __cplusplus
}
#endif

#endif /*__FLASH_MSPI_NOR_H__*/
