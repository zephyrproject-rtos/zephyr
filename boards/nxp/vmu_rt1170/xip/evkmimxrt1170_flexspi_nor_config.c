/*
 * Copyright 2018-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "evkmimxrt1170_flexspi_nor_config.h"

#if defined(XIP_BOOT_HEADER_ENABLE) && (XIP_BOOT_HEADER_ENABLE == 1)
__attribute__((section(".boot_hdr.conf"), used))

#define FLASH_DUMMY_CYCLES 0x09
#define FLASH_DUMMY_VALUE  0x09

const flexspi_nor_config_t qspi_flash_config = {
	.mem_config = {
		.tag = FLEXSPI_CFG_BLK_TAG,
		.version = FLEXSPI_CFG_BLK_VERSION,
		.read_sample_clk_src =
			flexspi_read_sample_clk_loopback_from_dqs_pad,
		.cs_hold_time = 3u,
		.cs_setup_time = 3u,
		/* Enable DDR mode, Wordaddassable, Safe configuration,
		 * Differential clock
		 */
		.controller_misc_option = 0x10,
		.device_type = flexspi_device_type_serial_nor,
		.sflash_pad_type = serial_flash_4_pads,
		.serial_clk_freq = flexspi_serial_clk_133mhz,
		.sflash_a1_size = 16u * 1024u * 1024u,
		/* Enable flash configuration feature */
		.config_cmd_enable = 1u,
		.config_mode_type[0] = device_config_cmd_type_generic,
		/* Set configuration command sequences */
		.config_cmd_seqs[0] = {
			.seq_num = 1,
			.seq_id = 12,
			.reserved = 0,
		},
		/* Prepare setting value for Read Register in flash */
		.config_cmd_args[0] = (FLASH_DUMMY_VALUE << 3),
		.lookup_table = {
			/* Read LUTs */
			[0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				 0xEB, RADDR_SDR, FLEXSPI_4PAD, 0x18),
			[1] = FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_4PAD,
			FLASH_DUMMY_CYCLES, READ_SDR, FLEXSPI_4PAD, 0x04),

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

			/* Set Read Register LUTs */
			[4 * 12 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				 0xC0, WRITE_SDR, FLEXSPI_1PAD, 0x01),
			[4 * 12 + 1] = FLEXSPI_LUT_SEQ(STOP, FLEXSPI_1PAD,
				 0x00, 0, 0, 0),
		},
	},
	.page_size = 256u,
	.sector_size = 4u * 1024u,
	.ipcmd_serial_clk_freq = 0x1,
	.block_size = 64u * 1024u,
	.is_uniform_block_size = false,
};
#endif /* XIP_BOOT_HEADER_ENABLE */
