/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "flash_config.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.flash_config"
#endif

#if defined(BOOT_HEADER_ENABLE) && (BOOT_HEADER_ENABLE == 1)
#if defined(__ARMCC_VERSION) || defined(__GNUC__)
__attribute__((section(".flash_conf"), used))
#elif defined(__ICCARM__)
#pragma location = ".flash_conf"
#endif

const flexspi_nor_config_t flexspi_config = {
	.memConfig = {
		.tag = FLASH_CONFIG_BLOCK_TAG,
		.version = FLASH_CONFIG_BLOCK_VERSION,
		.csHoldTime = 3,
		.csSetupTime = 3,
		.deviceModeCfgEnable = 1,
		.deviceModeType = kDeviceConfigCmdType_Generic,
		.waitTimeCfgCommands = 1,
		.deviceModeSeq = {
			.seqNum = 1,
			/* See Lookup table for more details */
			.seqId = 6,
			.reserved = 0,
		},
		.deviceModeArg = 0,
		.configCmdEnable = 1,
		.configModeType = {kDeviceConfigCmdType_Generic,
						kDeviceConfigCmdType_Spi2Xpi,
						kDeviceConfigCmdType_Generic},
		.configCmdSeqs = {
			{
				.seqNum = 1,
				.seqId = 7,
				.reserved = 0,
			},
			{
				.seqNum = 1,
				.seqId = 10,
				.reserved = 0,
			}
		},
		.configCmdArgs = {0x2, 0x1},
		.controllerMiscOption =
			1u << kFlexSpiMiscOffset_SafeConfigFreqEnable,
		.deviceType = 0x1,
		.sflashPadType = kSerialFlash_8Pads,
		.serialClkFreq = kFlexSpiSerialClk_SDR_48MHz,
		.sflashA1Size = 0,
		.sflashA2Size = 0,
		.sflashB1Size = 0x4000000U,
		.sflashB2Size = 0,
		.lookupTable = {
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
	.pageSize = 0x100,
	.sectorSize = 0x1000,
	.ipcmdSerialClkFreq = 1u,
	.serialNorType = 2u,
	.blockSize = 0x10000,
	.flashStateCtx = 0x07008100u,
};

#endif /* BOOT_HEADER_ENABLE */
