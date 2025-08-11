/*
 * Copyright 2018-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "evkmimxrt1060_flexspi_nor_config.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.xip_board"
#endif

#if defined(XIP_BOOT_HEADER_ENABLE) && (XIP_BOOT_HEADER_ENABLE == 1)
#if defined(__CC_ARM) || defined(__ARMCC_VERSION) || defined(__GNUC__)
__attribute__((section(".boot_hdr.conf"), used))
#elif defined(__ICCARM__)
#pragma location = ".boot_hdr.conf"
#endif

const flexspi_nor_config_t qspiflash_config = {
	.memConfig = {
		.tag = FLEXSPI_CFG_BLK_TAG,
		.version = FLEXSPI_CFG_BLK_VERSION,
		.readSampleClkSrc =
			kFlexSPIReadSampleClk_LoopbackFromDqsPad,
		.csHoldTime = 3u,
		.csSetupTime = 3u,
		.controllerMiscOption =
			(1u << kFlexSpiMiscOffset_SafeConfigFreqEnable),
		.deviceType = kFlexSpiDeviceType_SerialNOR,
		.sflashPadType = kSerialFlash_4Pads,
		.serialClkFreq = kFlexSpiSerialClk_120MHz,
		.sflashA1Size = 8u * 1024u * 1024u,
		.lookupTable = {
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
	.pageSize = 256u,
	.sectorSize = 4u * 1024u,
	.ipcmdSerialClkFreq = 1u,
	.blockSize = 64u * 1024u,
	.isUniformBlockSize = false,
};

#endif /* XIP_BOOT_HEADER_ENABLE */
