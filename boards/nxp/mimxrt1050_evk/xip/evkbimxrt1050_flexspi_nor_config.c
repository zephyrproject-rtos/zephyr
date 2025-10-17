/*
 * Copyright 2017-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "evkbimxrt1050_flexspi_nor_config.h"

#if defined(XIP_BOOT_HEADER_ENABLE) && (XIP_BOOT_HEADER_ENABLE == 1)
__attribute__((section(".boot_hdr.conf"), used))

const flexspi_nor_config_t hyperflash_config = {
	.mem_config = {
		.tag                = FLEXSPI_CFG_BLK_TAG,
		.version            = FLEXSPI_CFG_BLK_VERSION,
		.read_sample_clk_src
			= FLEXSPI_READ_SAMPLE_CLK_EXTERNAL_INPUT_FROM_DQS_PAD,
		.cs_hold_time         = 3u,
		.cs_setup_time        = 3u,
		.column_address_width = 3u,
		/* Enable DDR mode, Wordaddassable,
		 * Safe configuration, Differential clock
		 */
		.controller_misc_option =
			(1u << FLEXSPI_MISC_OFFSET_DDR_MODE_ENABLE) |
			(1u << FLEXSPI_MISC_OFFSET_WORD_ADDRESSABLE_ENABLE) |
			(1u << FLEXSPI_MISC_OFFSET_SAFE_CONFIG_FREQ_ENABLE) |
			(1u << FLEXSPI_MISC_OFFSET_DIFF_CLK_ENABLE),
		.device_type         = FLEXSPI_DEVICE_TYPE_SERIAL_NOR,
		.sflash_pad_type      = SERIAL_FLASH_8_PADS,
		.serial_clk_freq      = FLEXSPI_SERIAL_CLK_133MHZ,
		.lut_custom_seq_enable = 0x1,
		.sflash_a1_size       = 64u * 1024u * 1024u,
		.data_valid_time      = {15u, 0u},
		.busy_offset         = 15u,
		.busy_bit_polarity    = 1u,
		.lookup_table = {
			/* Read LUTs */
			[0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD, 0xA0,
				RADDR_DDR, FLEXSPI_8PAD, 0x18),
			[1] = FLEXSPI_LUT_SEQ(CADDR_DDR, FLEXSPI_8PAD, 0x10,
				DUMMY_DDR, FLEXSPI_8PAD, 0x0C),
			[2] = FLEXSPI_LUT_SEQ(READ_DDR, FLEXSPI_8PAD, 0x04,
				STOP, FLEXSPI_1PAD, 0x0),

			/* Read Status LUTs */
			/* 0 */
			[4 * 1 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x0),
			[4 * 1 + 1] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0xAA),
			[4 * 1 + 2] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x05),
			[4 * 1 + 3] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x70),

			/* 1 */
			[4 * 2 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0xA0, RADDR_DDR, FLEXSPI_8PAD, 0x18),
			[4 * 2 + 1] =
			    FLEXSPI_LUT_SEQ(CADDR_DDR, FLEXSPI_8PAD, 0x10,
					DUMMY_RWDS_DDR, FLEXSPI_8PAD, 0x0B),
			[4 * 2 + 2] = FLEXSPI_LUT_SEQ(READ_DDR, FLEXSPI_8PAD,
				0x4, STOP, FLEXSPI_1PAD, 0x0),

			/* Write Enable LUTs */
			/* 0 */
			[4 * 3 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x0),
			[4 * 3 + 1] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0xAA),
			[4 * 3 + 2] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x05),
			[4 * 3 + 3] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0xAA),

			/* 1 */
			[4 * 4 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x0),
			[4 * 4 + 1] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x55),
			[4 * 4 + 2] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x02),
			[4 * 4 + 3] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x55),

			/* Erase Sector LUTs */
			/* 0 */
			[4 * 5 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x0),
			[4 * 5 + 1] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0xAA),
			[4 * 5 + 2] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x05),
			[4 * 5 + 3] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x80),

			/* 1 */
			[4 * 6 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x0),
			[4 * 6 + 1] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0xAA),
			[4 * 6 + 2] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x05),
			[4 * 6 + 3] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0xAA),

			/* 2 */
			[4 * 7 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x0),
			[4 * 7 + 1] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x55),
			[4 * 7 + 2] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x02),
			[4 * 7 + 3] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x55),

			/* 3 */
			[4 * 8 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, RADDR_DDR, FLEXSPI_8PAD, 0x18),
			[4 * 8 + 1] = FLEXSPI_LUT_SEQ(CADDR_DDR, FLEXSPI_8PAD,
				0x10, CMD_DDR, FLEXSPI_8PAD, 0x0),
			[4 * 8 + 2] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x30, STOP, FLEXSPI_1PAD, 0x0),

			/* Page Program LUTs */
			/* 0 */
			[4 * 9 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x0),
			[4 * 9 + 1] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0xAA),
			[4 * 9 + 2] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x05),
			[4 * 9 + 3] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0xA0),

			/* 1 */
			[4 * 10 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, RADDR_DDR, FLEXSPI_8PAD, 0x18),
			[4 * 10 + 1] = FLEXSPI_LUT_SEQ(CADDR_DDR, FLEXSPI_8PAD,
				0x10, WRITE_DDR, FLEXSPI_8PAD, 0x80),

			/* Erase Chip LUTs */
			/* 0 */
			[4 * 11 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x0),
			[4 * 11 + 1] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0xAA),
			[4 * 11 + 2] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x05),
			[4 * 11 + 3] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x80),

			/* 1 */
			[4 * 12 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x0),
			[4 * 12 + 1] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0xAA),
			[4 * 12 + 2] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x05),
			[4 * 12 + 3] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0xAA),

			/* 2 */
			[4 * 13 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x0),
			[4 * 13 + 1] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x55),
			[4 * 13 + 2] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x02),
			[4 * 13 + 3] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x55),

			/* 3 */
			[4 * 14 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x0),
			[4 * 14 + 1] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0xAA),
			[4 * 14 + 2] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x05),
			[4 * 14 + 3] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0x0, CMD_DDR, FLEXSPI_8PAD, 0x10),
		},
		/* LUT customized sequence */
		.lut_custom_seq = {
			{
				.seq_num   = 0,
				.seq_id    = 0,
				.reserved = 0,
			},
			{
				.seq_num   = 2,
				.seq_id    = 1,
				.reserved = 0,
			},
			{
				.seq_num   = 2,
				.seq_id    = 3,
				.reserved = 0,
			},
			{
				.seq_num   = 4,
				.seq_id    = 5,
				.reserved = 0,
			},
			{
				.seq_num   = 2,
				.seq_id    = 9,
				.reserved = 0,
			},
			{
				.seq_num   = 4,
				.seq_id    = 11,
				.reserved = 0,
			}
		},
	},
	.page_size           = 512u,
	.sector_size         = 256u * 1024u,
	.ipcmd_serial_clk_freq = 1u,
	.serial_nor_type      = 1u,
	.block_size          = 256u * 1024u,
	.is_uniform_block_size = true,
};

#endif /* XIP_BOOT_HEADER_ENABLE */
