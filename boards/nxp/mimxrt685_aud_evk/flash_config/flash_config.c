/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#include "flash_config.h"

#if defined(CONFIG_NXP_IMXRT_BOOT_HEADER) && (CONFIG_NXP_IMXRT_BOOT_HEADER == 1)
__attribute__((section(".flash_conf"), used))

const flexspi_nor_config_t flexspi_config = {
	.mem_config = {
		.tag = FLASH_CONFIG_BLOCK_TAG,
		.version = FLASH_CONFIG_BLOCK_VERSION,
		.cs_hold_time = 3,
		.cs_setup_time = 3,
		/*
		 * Program the flash status/configuration register (WRSR 0x01,
		 * LUT seq 2) with 0xC740: QE = 1 and the configuration-register
		 * dummy-cycle bits set for a 10-cycle 1S-4S-4S (0xEC) read. This
		 * matches the read latency the Zephyr flash driver installs for the
		 * MX25U51245G, so AHB/XIP reads keep working after the driver
		 * reinstalls the LUT during initialization.
		 */
		.device_mode_cfg_enable = 1,
		.device_mode_seq = {.seq_num = 1, .seq_id = 2},
		.device_mode_arg = 0xC740,
		.controller_misc_option =
			1u << FLEXSPI_MISC_OFFSET_SAFE_CONFIG_FREQ_ENABLE,
		.device_type = 0x1,
		.sflash_pad_type = SERIAL_FLASH_4_PADS,
		.serial_clk_freq = FLEXSPI_SERIAL_CLK_SDR_24MHZ,
		.sflash_a1_size = 0,
		.sflash_a2_size = 0,
		.sflash_b1_size = 0x4000000U,
		.sflash_b2_size = 0,
		.lookup_table = {
			/* Read (4READ4B: 0xEC), 10 dummy cycles (CR DC bits) */
			[0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0xEC, RADDR_SDR, FLEXSPI_4PAD, 0x20),
			[1] = FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_4PAD,
				0x0A, READ_SDR, FLEXSPI_4PAD, 0x04),

			/* Read Status */
			[4 * 1 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x05, READ_SDR, FLEXSPI_1PAD, 0x04),

			/* Write Status/Config Register (device mode cfg) */
			[4 * 2 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x01, WRITE_SDR, FLEXSPI_1PAD, 0x02),

			/* Write Enable */
			[4 * 3 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x06, STOP_EXE, FLEXSPI_1PAD, 0x00),

			/* Sector Erase (SE4B: 0x21) */
			[4 * 5 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x21, RADDR_SDR, FLEXSPI_1PAD, 0x20),

			/* Block Erase (BE4B: 0xDC) */
			[4 * 8 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0xDC, RADDR_SDR, FLEXSPI_1PAD, 0x20),

			/* Page Program (4PP4B: 0x3E) */
			[4 * 9 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x3E, RADDR_SDR, FLEXSPI_4PAD, 0x20),
			[4 * 9 + 1] = FLEXSPI_LUT_SEQ(WRITE_SDR, FLEXSPI_4PAD,
				0x04, STOP_EXE, FLEXSPI_1PAD, 0x00),

			/* Chip Erase */
			[4 * 11 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x60, STOP_EXE, FLEXSPI_1PAD, 0x00),
		},
	},
	.page_size = 0x100,
	.sector_size = 0x1000,
	.ipcmd_serial_clk_freq = 1u,
	.block_size = 0x10000,
};

#endif /* defined(CONFIG_NXP_IMXRT_BOOT_HEADER) && (CONFIG_NXP_IMXRT_BOOT_HEADER == 1) */
