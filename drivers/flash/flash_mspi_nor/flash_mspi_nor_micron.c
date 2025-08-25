/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "flash_mspi_nor.h"

const struct flash_mspi_device_data mt35xu02gcba_data = {
	.jedec_id = {0x2C, 0x5B, 0x1A}, /* MT35XU02GCBA */
	FLASH_PAGE_LAYOUT_DEFINE(4096, 0x4000000)
	.flash_size = 0x4000000,
};

const struct flash_mspi_nor_cmds mt35xu02gcba_octal_cmds = {
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
};

struct flash_mspi_mode_data mt35xu02gcba_octal = {
	.dev_cfg = {
		.io_mode = MSPI_IO_MODE_OCTAL_1_8_8,
		.data_rate = MSPI_DATA_RATE_SINGLE,
		.endian = MSPI_XFER_BIG_ENDIAN,
	},
	.jedec_cmds = &mt35xu02gcba_octal_cmds,
	.flash_data = &mt35xu02gcba_data,
};

const struct flash_mspi_device_data mt25qu512abb_data = {
	.jedec_id = {0x20, 0xBB, 0x20}, /* MT25QU512ABB */
	FLASH_PAGE_LAYOUT_DEFINE(4096, 0x4000000)
	.flash_size = 0x4000000,
};

const struct flash_mspi_nor_cmds mt25qu512abb_quad_cmds = {
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
};

struct flash_mspi_mode_data mt25qu512abb_quad = {
	.dev_cfg = {
		.io_mode = MSPI_IO_MODE_QUAD_1_4_4,
		.data_rate = MSPI_DATA_RATE_SINGLE,
		.endian = MSPI_XFER_BIG_ENDIAN,
	},
	.jedec_cmds = &mt25qu512abb_quad_cmds,
	.flash_data = &mt25qu512abb_data,
};

#define MXICY_MX25R_LH_MASK BIT(1)
#define MXICY_MX25R_QE_MASK BIT(6)
#define MXICY_MX25R_REGS_LEN 3

static uint8_t mxicy_mx25r_hp_payload[MXICY_MX25R_REGS_LEN] = {
	MXICY_MX25R_QE_MASK, 0x0, MXICY_MX25R_LH_MASK
};

/* For quad io mode above 8 MHz and single io mode above 33 MHz,
 * high performance mode needs to be enabled.
 */
static bool needs_hp(enum mspi_io_mode io_mode, uint32_t freq)
{
	if ((io_mode == MSPI_IO_MODE_QUAD_1_1_4) || (io_mode == MSPI_IO_MODE_QUAD_1_4_4)) {
		if (freq > MHZ(8)) {
			return true;
		}
	} else if (io_mode == MSPI_IO_MODE_SINGLE) {
		if (freq > MHZ(33)) {
			return true;
		}
	}

	return false;
}

static int mxicy_mx25r_post_switch_mode(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	enum mspi_io_mode io_mode = FLASH_MODE_DATA(dev)->dev_cfg.io_mode;
	uint32_t freq = FLASH_MODE_DATA(dev)->dev_cfg.freq;
	int rc;
	uint8_t status;
	uint8_t config[MXICY_MX25R_REGS_LEN - 1];

	if (!needs_hp(io_mode, freq)) {
		return 0;
	}

	/* Wait for previous write to finish */
	do {
		flash_mspi_command_set(dev, &FLASH_MODE_DATA(dev)->jedec_cmds->status);
		dev_data->packet.data_buf  = &status;
		dev_data->packet.num_bytes = sizeof(status);
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);
		if (rc < 0) {
			return rc;
		}
	} while (status & SPI_NOR_WIP_BIT);

	/* Write enable */
	flash_mspi_command_set(dev, &commands_single.write_en);
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);
	if (rc < 0) {
		return rc;
	}

	/* Write status and config registers */
	const struct flash_mspi_nor_cmd cmd_status = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_CMD_WRSR,
		.cmd_length = 1,
	};

	flash_mspi_command_set(dev, &cmd_status);
	dev_data->packet.data_buf  = mxicy_mx25r_hp_payload;
	dev_data->packet.num_bytes = sizeof(mxicy_mx25r_hp_payload);
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);
	if (rc < 0) {
		return rc;
	}

	/* Wait for write to end and verify status register */
	do {
		flash_mspi_command_set(dev, &FLASH_MODE_DATA(dev)->jedec_cmds->status);
		dev_data->packet.data_buf  = &status;
		dev_data->packet.num_bytes = sizeof(status);
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);
		if (rc < 0) {
			return rc;
		}
	} while (status & SPI_NOR_WIP_BIT);

	if (status != mxicy_mx25r_hp_payload[0]) {
		return -EIO;
	}

	/* Verify configuration registers */
	flash_mspi_command_set(dev, &FLASH_MODE_DATA(dev)->jedec_cmds->config);
	dev_data->packet.data_buf  = config;
	dev_data->packet.num_bytes = sizeof(config);
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);
	if (rc < 0) {
		return rc;
	}

	for (uint8_t i = 0; i < MXICY_MX25R_REGS_LEN - 1; i++) {
		if (config[i] != mxicy_mx25r_hp_payload[i + 1]) {
			return -EIO;
		}
	}

	return 0;
}

#define MXICY_MX25R_OE_MASK BIT(0)

static uint8_t mxicy_mx25u_oe_payload = MXICY_MX25R_OE_MASK;

static inline int mxicy_mx25u_post_switch_mode(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	enum mspi_io_mode io_mode = FLASH_MODE_DATA(dev)->dev_cfg.io_mode;
	int rc;

	if (io_mode != MSPI_IO_MODE_OCTAL) {
		return 0;
	}

	/* Write enable */
	flash_mspi_command_set(dev, &commands_single.write_en);
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);
	if (rc < 0) {
		return rc;
	}

	/* Write config register 2 */
	const struct flash_mspi_nor_cmd cmd_status = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_CMD_WR_CFGREG2,
		.cmd_length = 1,
		.addr_length = 4,
	};

	flash_mspi_command_set(dev, &cmd_status);
	dev_data->packet.data_buf  = &mxicy_mx25u_oe_payload;
	dev_data->packet.num_bytes = sizeof(mxicy_mx25u_oe_payload);
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);
	return rc;
}

struct flash_mspi_nor_quirks flash_quirks_mxicy_mx25u = {
	.post_switch_mode = mxicy_mx25u_post_switch_mode,
};

const struct flash_mspi_device_data mxicy_mx25u_data = {
	.jedec_id = {0xc2, 0x84, 0x37}, /* MT25QU512ABB */
	FLASH_PAGE_LAYOUT_DEFINE(CONFIG_FLASH_MSPI_NOR_LAYOUT_PAGE_SIZE, 0x100000)
	.flash_size = 0x100000,
};

struct flash_mspi_mode_data mxicy_mx25u_single = {
	.dev_cfg = {
		.io_mode = MSPI_IO_MODE_SINGLE,
		.data_rate = MSPI_DATA_RATE_SINGLE,
		.endian = MSPI_XFER_BIG_ENDIAN,
	},
	.jedec_cmds = &commands_single,
	.quirks = &flash_quirks_mxicy_mx25u,
	.flash_data = &mxicy_mx25u_data,
};

const struct flash_mspi_nor_cmds mxicy_mx25u_commands_octal = {
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

struct flash_mspi_mode_data mxicy_mx25u_octal = {
	.dev_cfg = {
		.io_mode = MSPI_IO_MODE_OCTAL,
		.data_rate = MSPI_DATA_RATE_SINGLE,
		.endian = MSPI_XFER_BIG_ENDIAN,
	},
	.jedec_cmds = &mxicy_mx25u_commands_octal,
	.quirks = &flash_quirks_mxicy_mx25u,
	.flash_data = &mxicy_mx25u_data,
};

struct flash_mspi_nor_quirks flash_quirks_mxicy_mx25r = {
	.post_switch_mode = mxicy_mx25r_post_switch_mode,
};

const struct flash_mspi_device_data mxicy_mx25r_data = {
	.jedec_id = {0xc2, 0x28, 0x17}, /* MX25R6435F */
	FLASH_PAGE_LAYOUT_DEFINE(CONFIG_FLASH_MSPI_NOR_LAYOUT_PAGE_SIZE, 0x800000)
	.flash_size = 0x800000,
};

struct flash_mspi_mode_data mxicy_mx25r_single = {
	.dev_cfg = {
		.io_mode = MSPI_IO_MODE_SINGLE,
		.data_rate = MSPI_DATA_RATE_SINGLE,
		.endian = MSPI_XFER_BIG_ENDIAN,
	},
	.jedec_cmds = &commands_single,
	.quirks = &flash_quirks_mxicy_mx25r,
	.flash_data = &mxicy_mx25r_data,
};

struct flash_mspi_mode_data mxicy_mx25r_quad = {
	.dev_cfg = {
		.io_mode = MSPI_IO_MODE_QUAD_1_4_4,
		.data_rate = MSPI_DATA_RATE_SINGLE,
		.endian = MSPI_XFER_BIG_ENDIAN,
	},
	.jedec_cmds = &commands_quad_1_4_4,
	.quirks = &flash_quirks_mxicy_mx25r,
	.flash_data = &mxicy_mx25r_data,
};

/* Micron flash devices known to this driver */
const struct flash_mspi_mode_data *micron_flash_devs[] = {
	&mt35xu02gcba_octal,
	&mt25qu512abb_quad,
	&mxicy_mx25u_single,
	&mxicy_mx25u_octal,
	&mxicy_mx25r_single,
	&mxicy_mx25r_quad,
};

const struct flash_mspi_nor_vendor micron_vendor = {
	.vendor_devs = micron_flash_devs,
	.dev_count = ARRAY_SIZE(micron_flash_devs),
	.probe_dev = flash_mspi_nor_probe_dev,
};
