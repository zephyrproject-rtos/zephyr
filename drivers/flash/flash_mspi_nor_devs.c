/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/mspi.h>
#include "jesd216.h"
#include "spi_nor.h"
#include "flash_mspi_nor.h"

struct flash_mspi_nor_devs mspi_nor_devs[] = {
	{
		.jedec_id = {0x2C, 0x5B, 0x1A}, /* MT35XU02GCBA */
		.page_size = 4096,
		.flash_size = 0x4000000,
		.dev_cfg = {
			.io_mode = MSPI_IO_MODE_OCTAL_1_8_8,
			.data_rate = MSPI_DATA_RATE_SINGLE,
			.endian = MSPI_XFER_BIG_ENDIAN,
		},
		.jedec_cmds = {
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
				.force_single = true,
			},
			.read = {
				.dir = MSPI_RX,
				.cmd = 0xCC,
				.cmd_length = 1,
				.addr_length = 4,
				.rx_dummy = 16,
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
				.cmd = 0x8E,
				.cmd_length = 1,
				.addr_length = 4,
			},
			.sector_erase = {
				.dir = MSPI_TX,
				.cmd = 0x21,
				.cmd_length = 1,
				.addr_length = 4,
				.force_single = true,
			},
			.chip_erase = {
				.dir = MSPI_TX,
				.cmd = 0xC4,
				.cmd_length = 1,
			},
			.sfdp = {
				.dir = MSPI_RX,
				.cmd = JESD216_CMD_READ_SFDP,
				.cmd_length = 1,
				.addr_length = 3,
				.rx_dummy = 0,
				.force_single = true,
			},
		}
	},
	{
		.jedec_id = {0x20, 0xBB, 0x20}, /* MT25QU512ABB */
		.page_size = 4096,
		.flash_size = 0x4000000,
		.dev_cfg = {
			.io_mode = MSPI_IO_MODE_QUAD_1_4_4,
			.data_rate = MSPI_DATA_RATE_SINGLE,
			.endian = MSPI_XFER_BIG_ENDIAN,
		},
		.jedec_cmds = {
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
				.cmd = 0xEC,
				.cmd_length = 1,
				.addr_length = 4,
				.rx_dummy = 10,
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
				.cmd = 0x3E,
				.cmd_length = 1,
				.addr_length = 4,
			},
			.sector_erase = {
				.dir = MSPI_TX,
				.cmd = 0x21,
				.cmd_length = 1,
				.addr_length = 4,
				.force_single = true,
			},
			.chip_erase = {
				.dir = MSPI_TX,
				.cmd = 0xC7,
				.cmd_length = 1,
			},
			.sfdp = {
				.dir = MSPI_RX,
				.cmd = JESD216_CMD_READ_SFDP,
				.cmd_length = 1,
				.addr_length = 3,
				.rx_dummy = 0,
				.force_single = true,
			},
		}
	},
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
		.rx_dummy = 0,
	},
};

const uint32_t mspi_nor_devs_count = ARRAY_SIZE(mspi_nor_devs);

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
