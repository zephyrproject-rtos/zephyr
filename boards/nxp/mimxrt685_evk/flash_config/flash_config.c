/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "flash_config.h"

#if defined(BOOT_HEADER_ENABLE) && (BOOT_HEADER_ENABLE == 1)
__attribute__((section(".flash_conf"), used))

const flexspi_nor_config_t flexspi_config = {
	.mem_config = {
		.tag = FLASH_CONFIG_BLOCK_TAG,
		.version = FLASH_CONFIG_BLOCK_VERSION,
		.cs_hold_time = 3,
		.cs_setup_time = 3,
		.device_mode_cfg_enable = 1,
		.device_mode_type = device_config_cmd_type_generic,
		.wait_time_cfg_commands = 1,
		.device_mode_seq = {
			.seq_num = 1,
			/* See Lookup table for more details */
			.seq_id = 6,
			.reserved = 0,
		},
		.device_mode_arg = 0,
		.config_cmd_enable = 1,
		.config_mode_type = {device_config_cmd_type_generic,
					device_config_cmd_type_spi2xpi,
					device_config_cmd_type_generic},
		.config_cmd_seqs = {
			{
				.seq_num = 1,
				.seq_id = 7,
				.reserved = 0,
			},
			{
				.seq_num = 1,
				.seq_id = 10,
				.reserved = 0,
			}
		},
		.config_cmd_args = {0x2, 0x1},
		.controller_misc_option =
			1u << flexspi_misc_offset_safe_config_freq_enable,
		.device_type = 0x1,
		.sflash_pad_type = serial_flash_8_pads,
		.serial_clk_freq = flexspi_serial_clk_sdr_48mhz,
		.sflash_a1_size = 0,
		.sflash_a2_size = 0,
		.sflash_b1_size = 0x4000000U,
		.sflash_b2_size = 0,
		.lookup_table = {
			/* Read */
			[0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_8PAD,
					0xEC, CMD_SDR, FLEXSPI_8PAD, 0x13),
			[1] = FLEXSPI_LUT_SEQ(RADDR_SDR, FLEXSPI_8PAD, 0x20,
					DUMMY_SDR, FLEXSPI_8PAD, 0x14),
			[2] = FLEXSPI_LUT_SEQ(READ_SDR, FLEXSPI_8PAD, 0x04,
					STOP_EXE, FLEXSPI_1PAD, 0x00),

			/* Read status SPI */
			[4 * 1 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x05, READ_SDR, FLEXSPI_1PAD, 0x04),

			/* Read Status OPI */
			[4 * 2 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_8PAD,
				0x05, CMD_SDR, FLEXSPI_8PAD, 0xFA),
			[4 * 2 + 1] = FLEXSPI_LUT_SEQ(RADDR_SDR, FLEXSPI_8PAD,
				0x20, DUMMY_SDR, FLEXSPI_8PAD, 0x14),
			[4 * 2 + 2] = FLEXSPI_LUT_SEQ(READ_SDR, FLEXSPI_8PAD,
				0x04, STOP_EXE, FLEXSPI_1PAD, 0x00),

			/* Write Enable */
			[4 * 3 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x06, STOP_EXE, FLEXSPI_1PAD, 0x00),

			/* Write Enable - OPI */
			[4 * 4 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_8PAD,
				0x06, CMD_SDR, FLEXSPI_8PAD, 0xF9),

			/* Erase Sector */
			[4 * 5 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_8PAD,
				0x21, CMD_SDR, FLEXSPI_8PAD, 0xDE),
			[4 * 5 + 1] = FLEXSPI_LUT_SEQ(RADDR_SDR, FLEXSPI_8PAD,
				0x20, STOP_EXE, FLEXSPI_1PAD, 0x00),

			/* Configure dummy cycles */
			[4 * 6 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x72, CMD_SDR, FLEXSPI_1PAD, 0x00),
			[4 * 6 + 1] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x00, CMD_SDR, FLEXSPI_1PAD, 0x03),
			[4 * 6 + 2] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x00, WRITE_SDR, FLEXSPI_1PAD, 0x01),

			/* Configure Register */
			[4 * 7 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x72, CMD_SDR, FLEXSPI_1PAD, 0x00),
			[4 * 7 + 1] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x00, CMD_SDR, FLEXSPI_1PAD, 0x02),
			[4 * 7 + 2] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				 0x00, WRITE_SDR, FLEXSPI_1PAD, 0x01),

			/* Erase Block */
			[4 * 8 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_8PAD,
				0xDC, CMD_SDR, FLEXSPI_8PAD, 0x23),
			[4 * 8 + 1] = FLEXSPI_LUT_SEQ(RADDR_SDR, FLEXSPI_8PAD,
				0x20, STOP_EXE, FLEXSPI_1PAD, 0x00),

			/* Page program */
			[4 * 9 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_8PAD,
				 0x12, CMD_SDR, FLEXSPI_8PAD, 0xED),
			[4 * 9 + 1] = FLEXSPI_LUT_SEQ(RADDR_SDR, FLEXSPI_8PAD,
				 0x20, WRITE_SDR, FLEXSPI_8PAD, 0x04),

			/* Enable OPI STR mode */
			[4 * 10 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				 0x72, CMD_SDR, FLEXSPI_1PAD, 0x00),
			[4 * 10 + 1] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				 0x00, CMD_SDR, FLEXSPI_1PAD, 0x00),
			[4 * 10 + 2] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				 0x00, WRITE_SDR, FLEXSPI_1PAD, 0x01),

			/* Erase Chip */
			[4 * 11 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_8PAD,
				 0x60, CMD_SDR, FLEXSPI_8PAD, 0x9F),
		},
	},
	.page_size = 0x100,
	.sector_size = 0x1000,
	.ipcmd_serial_clk_freq = 1u,
	.serial_nor_type = 2u,
	.block_size = 0x10000,
	.flash_state_ctx = 0x07008100u,
};

#endif /* BOOT_HEADER_ENABLE */
