/*
 * Copyright 2018-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "evkmimxrt1064_flexspi_nor_config.h"

#if defined(XIP_BOOT_HEADER_ENABLE) && (XIP_BOOT_HEADER_ENABLE == 1)
__attribute__((section(".boot_hdr.conf"), used))

const flexspi_nor_config_t qspi_flash_config = {
	.mem_config = {
		.tag = FLEXSPI_CFG_BLK_TAG,
		.version = FLEXSPI_CFG_BLK_VERSION,
		.read_sample_clk_src =
			FLEXSPI_READ_SAMPLE_CLK_LOOPBACK_FROM_DQS_PAD,
		.cs_hold_time = 3u,
		.cs_setup_time = 3u,
		.controller_misc_option =
			(1u << FLEXSPI_MISC_OFFSET_SAFE_CONFIG_FREQ_ENABLE),
		.device_type = FLEXSPI_DEVICE_TYPE_SERIAL_NOR,
		.sflash_pad_type = SERIAL_FLASH_4_PADS,
		.serial_clk_freq = FLEXSPI_SERIAL_CLK_120MHZ,
		.sflash_a1_size = 4u * 1024u * 1024u,
		.lookup_table = {
			/* Read LUTs */
			[0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0xEB, RADDR_SDR, FLEXSPI_4PAD, 0x18),
			[1] = FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_4PAD,
				0x06, READ_SDR, FLEXSPI_4PAD, 0x04),

			/* Read Status LUTs */
			[4 * 1 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x05, READ_SDR, FLEXSPI_1PAD, 0x04),

			/* Write Enable LUTs */
			[4 * 3 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x06, STOP, FLEXSPI_1PAD, 0x0),

			/* Erase Sector LUTs */
			[4 * 5 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x20, RADDR_SDR, FLEXSPI_1PAD, 0x18),

			/* Erase Block LUTs */
			[4 * 8 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0xD8, RADDR_SDR, FLEXSPI_1PAD, 0x18),

			/* Pape Program LUTs */
			[4 * 9 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x02, RADDR_SDR, FLEXSPI_1PAD, 0x18),
			[4 * 9 + 1] = FLEXSPI_LUT_SEQ(WRITE_SDR, FLEXSPI_1PAD,
				0x04, STOP, FLEXSPI_1PAD, 0x0),

			/* Erase Chip LUTs */
			[4 * 11 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x60, STOP, FLEXSPI_1PAD, 0x0),
		},
	},
	.page_size = 256u,
	.sector_size = 4u * 1024u,
	.ipcmd_serial_clk_freq = 1u,
	.block_size = 64u * 1024u,
	.is_uniform_block_size = false,
};

#endif /* XIP_BOOT_HEADER_ENABLE */
