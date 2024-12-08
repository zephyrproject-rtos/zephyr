/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <flash_config.h>

__attribute__((section(".flash_conf"), used))
const fc_flexspi_nor_config_t flexspi_config = {
	.memConfig = {
			.tag = FC_BLOCK_TAG,
			.version = FC_BLOCK_VERSION,
			.readSampleClkSrc = 1,
			.csHoldTime = 3,
			.csSetupTime = 3,
			.deviceModeCfgEnable = 1,
			.deviceModeSeq = {.seqNum = 1, .seqId = 2},
			.deviceModeArg = 0xC740,
			.configCmdEnable = 0,
			.deviceType = 0x1,
			.sflashPadType = kSerialFlash_4Pads,
			.serialClkFreq = 7,
			.sflashA1Size = 0x4000000U,
			.sflashA2Size = 0,
			.sflashB1Size = 0,
			.sflashB2Size = 0,
			.lookupTable = {
					/* Read */
					[0] = FC_FLEXSPI_LUT_SEQ(
						FC_CMD_SDR, FC_FLEXSPI_1PAD,
						0xEC, FC_RADDR_SDR,
						FC_FLEXSPI_4PAD, 0x20),
					[1] = FC_FLEXSPI_LUT_SEQ(
						FC_DUMMY_SDR,
						FC_FLEXSPI_4PAD, 0x0A,
						FC_READ_SDR,
						FC_FLEXSPI_4PAD, 0x04),

					/* Read Status */
					[4 * 1 + 0] = FC_FLEXSPI_LUT_SEQ(
						FC_CMD_SDR, FC_FLEXSPI_1PAD,
						0x05, FC_READ_SDR,
						FC_FLEXSPI_1PAD, 0x04),

					/* Write Status */
					[4 * 2 + 0] = FC_FLEXSPI_LUT_SEQ(
						FC_CMD_SDR, FC_FLEXSPI_1PAD,
						0x01, FC_WRITE_SDR,
						FC_FLEXSPI_1PAD, 0x02),

					/* Write Enable */
					[4 * 3 + 0] = FC_FLEXSPI_LUT_SEQ(
						FC_CMD_SDR, FC_FLEXSPI_1PAD,
						0x06, FC_STOP_EXE,
						FC_FLEXSPI_1PAD, 0x00),

					/* Sector erase */
					[4 * 5 + 0] = FC_FLEXSPI_LUT_SEQ(
						FC_CMD_SDR, FC_FLEXSPI_1PAD,
						0x21, FC_RADDR_SDR,
						FC_FLEXSPI_1PAD, 0x20),

					/* Block erase */
					[4 * 8 + 0] =
						FC_FLEXSPI_LUT_SEQ(FC_CMD_SDR,
								   FC_FLEXSPI_1PAD,
								   0x5C, FC_RADDR_SDR,
								   FC_FLEXSPI_1PAD,
								   0x20),

					/* Page program */
					[4 * 9 + 0] =
						FC_FLEXSPI_LUT_SEQ(FC_CMD_SDR,
								   FC_FLEXSPI_1PAD,
								   0x12, FC_RADDR_SDR,
								   FC_FLEXSPI_1PAD,
								   0x20),
					[4 * 9 + 1] =
						FC_FLEXSPI_LUT_SEQ(FC_WRITE_SDR,
								   FC_FLEXSPI_1PAD,
								   0x00,
								   FC_STOP_EXE, FC_FLEXSPI_1PAD,
								   0x00),

					/* chip erase */
					[4 * 11 + 0] = FC_FLEXSPI_LUT_SEQ(FC_CMD_SDR,
									  FC_FLEXSPI_1PAD,
									  0x60, FC_STOP_EXE,
									  FC_FLEXSPI_1PAD,
									  0x00),
				},
	},
	.pageSize = 0x100,
	.sectorSize = 0x1000,
	.ipcmdSerialClkFreq = 0,
	.blockSize = 0x8000,
	.fcb_fill[0] = 0xFFFFFFFF,
};
