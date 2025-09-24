/*
 * Copyright 2023, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "flash_config.h"

#if defined(BOOT_HEADER_ENABLE) && (BOOT_HEADER_ENABLE == 1)
__attribute__((section(".flash_conf"), used))

const fc_static_platform_config_t flash_config = {
	.xspi_fcb_block = {
		.mem_config = {
			.tag = FC_XSPI_CFG_BLK_TAG,
			.version = FC_XSPI_CFG_BLK_VERSION,
			.read_sample_clk_src =
				fc_xspi_read_sample_clk_external_input_from_dqs_pad,
			.cs_hold_time = 3,
			.cs_setup_time = 3,
			.device_mode_cfg_enable = 1,
			.device_mode_type = 2,
			.wait_time_cfg_commands = 1,
			.device_mode_seq = {
				.seq_num = 1,
				.seq_id = 6,
				/* SeeLookup table for more details */
				.reserved = 0,
			},
			.device_mode_arg = 2, /* Enable OPI DDR mode */
			.controller_misc_option =
			(1u << fc_xspi_misc_offset_safe_config_freq_enable) |
			(1u << fc_xspi_misc_offset_ddr_mode_enable),
			.device_type = 1,
			.sflash_pad_type = 8,
			.serial_clk_freq = fc_xspi_serial_clk_200mhz,
			.sflash_a1_size = 64ul * 1024u * 1024u,
			.busy_offset = 0u,
			.busy_bit_polarity = 0u,
#if defined(FSL_FEATURE_SILICON_VERSION_A)
			.lut_custom_seq_enable = 0u,
#else
			.lut_custom_seq_enable = 1u,
#endif
			.lookup_table = {
				/*Read*/
				[0] = FC_XSPI_LUT_SEQ(FC_CMD_DDR, FC_XSPI_8PAD,
					0xEE, FC_CMD_DDR, FC_XSPI_8PAD, 0x11),
				[1] = FC_XSPI_LUT_SEQ(FC_CMD_RADDR_DDR, FC_XSPI_8PAD,
					0x20, FC_CMD_DUMMY_SDR, FC_XSPI_8PAD, 0x12),
				[2] = FC_XSPI_LUT_SEQ(FC_CMD_DUMMY_SDR, FC_XSPI_8PAD,
					0x2, FC_CMD_READ_DDR, FC_XSPI_8PAD, 0x4),
				[3] = FC_XSPI_LUT_SEQ(FC_CMD_STOP, FC_XSPI_8PAD,
					0x0, 0, 0, 0),

				/*Read status SPI*/
				[5 * 1 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD,
					0x05, FC_CMD_READ_SDR, FC_XSPI_1PAD, 0x04),

				/* Read Status OPI */
				[5 * 2 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_DDR, FC_XSPI_8PAD,
					0x05, FC_CMD_DDR, FC_XSPI_8PAD, 0xFA),
				[5 * 2 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_RADDR_DDR,
					FC_XSPI_8PAD, 0x20, FC_CMD_DUMMY_SDR,
					FC_XSPI_8PAD, 0x12),
				[5 * 2 + 2] = FC_XSPI_LUT_SEQ(FC_CMD_DUMMY_SDR,
					FC_XSPI_8PAD, 0x2, FC_CMD_READ_DDR,
					FC_XSPI_8PAD, 0x4),
				[5 * 2 + 3] = FC_XSPI_LUT_SEQ(FC_CMD_STOP, FC_XSPI_8PAD,
					0x0, 0, 0, 0),

				/*Write enable*/
				[5 * 3 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD,
					0x06, FC_CMD_STOP, FC_XSPI_1PAD, 0x04),

				/* Write Enable - OPI */
				[5 * 4 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_DDR, FC_XSPI_8PAD,
					0x06, FC_CMD_DDR, FC_XSPI_8PAD, 0xF9),

				/* Erase Sector */
				[5 * 5 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_DDR, FC_XSPI_8PAD,
					0x21, FC_CMD_DDR, FC_XSPI_8PAD, 0xDE),
				[5 * 5 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_RADDR_DDR,
					FC_XSPI_8PAD, 0x20, FC_CMD_STOP, FC_XSPI_8PAD,
					0x0),

				/* Enable OPI DDR mode */
				[5 * 6 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD,
					0x72, FC_CMD_SDR, FC_XSPI_1PAD, 0x00),
				[5 * 6 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD,
					0x00, FC_CMD_SDR, FC_XSPI_1PAD, 0x00),
				[5 * 6 + 2] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD,
					0x00, FC_CMD_WRITE_SDR, FC_XSPI_1PAD, 0x01),
#if defined(FSL_FEATURE_SILICON_VERSION_A)
				/* Page program */
				[5 * 9 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_DDR, FC_XSPI_8PAD,
					0x12, FC_CMD_DDR, FC_XSPI_8PAD, 0xED),
				[5 * 9 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_RADDR_DDR,
					FC_XSPI_8PAD, 0x20, FC_CMD_WRITE_DDR,
					FC_XSPI_8PAD, 0x4),

				/* Erase Chip */
				[5 * 11 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_DDR, FC_XSPI_8PAD,
					0x60, FC_CMD_DDR, FC_XSPI_8PAD, 0x9F),
			},
#else
				/* Page program */
				[5 * 9 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_DDR, FC_XSPI_8PAD,
					0x06, FC_CMD_DDR, FC_XSPI_8PAD, 0xF9),
				[5 * 9 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_JUMP_TO_SEQ,
					FC_XSPI_8PAD, 0x2U, FC_CMD_STOP, FC_XSPI_8PAD,
					0x0),
				[5 * 10 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_DDR, FC_XSPI_8PAD,
					0x12, FC_CMD_DDR, FC_XSPI_8PAD, 0xED),
				[5 * 10 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_RADDR_DDR,
					FC_XSPI_8PAD, 0x20, FC_CMD_WRITE_DDR,
					FC_XSPI_8PAD, 0x4),
				/* Erase Chip */
				[5 * 13 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_DDR, FC_XSPI_8PAD,
					0x60, FC_CMD_DDR, FC_XSPI_8PAD, 0x9F),
		},

		/* For PAGEPROGRAM custom LUT, uses joined LUT. */
		.lut_custom_seq[4].seq_num = 2U,
		.lut_custom_seq[4].seq_id = 9U,
#endif
		},
		.page_size = 256u,
		.sector_size = 4u * 1024u,
		.ipcmd_serial_clk_freq = 1u,
		.serial_nor_type = 2u,
		.block_size = 64u * 1024u,
		.flash_state_ctx = 0x07008200u,
	},
#ifdef BOOT_ENABLE_XSPI1_PSRAM
	.psram_config_block = {
		.xmcd_header = 0xC0010008,
		.xmcd_opt0 = 0xC0000700,
	},
#endif
};

#endif /* BOOT_HEADER_ENABLE */
