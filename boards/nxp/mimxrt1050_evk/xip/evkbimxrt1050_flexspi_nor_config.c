/*
 * Copyright 2017-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "evkbimxrt1050_flexspi_nor_config.h"

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

const flexspi_nor_config_t hyperflash_config = {
	.memConfig = {
		.tag                = FLEXSPI_CFG_BLK_TAG,
		.version            = FLEXSPI_CFG_BLK_VERSION,
		.readSampleClkSrc
			= kFlexSPIReadSampleClk_ExternalInputFromDqsPad,
		.csHoldTime         = 3u,
		.csSetupTime        = 3u,
		.columnAddressWidth = 3u,
		/* Enable DDR mode, Wordaddassable,
		 * Safe configuration, Differential clock
		 */
		.controllerMiscOption =
			(1u << kFlexSpiMiscOffset_DdrModeEnable) |
			(1u << kFlexSpiMiscOffset_WordAddressableEnable) |
			(1u << kFlexSpiMiscOffset_SafeConfigFreqEnable) |
			(1u << kFlexSpiMiscOffset_DiffClkEnable),
		.deviceType         = kFlexSpiDeviceType_SerialNOR,
		.sflashPadType      = kSerialFlash_8Pads,
		.serialClkFreq      = kFlexSpiSerialClk_133MHz,
		.lutCustomSeqEnable = 0x1,
		.sflashA1Size       = 64u * 1024u * 1024u,
		.dataValidTime      = {15u, 0u},
		.busyOffset         = 15u,
		.busyBitPolarity    = 1u,
		.lookupTable = {
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
		.lutCustomSeq = {
			{
				.seqNum   = 0,
				.seqId    = 0,
				.reserved = 0,
			},
			{
				.seqNum   = 2,
				.seqId    = 1,
				.reserved = 0,
			},
			{
				.seqNum   = 2,
				.seqId    = 3,
				.reserved = 0,
			},
			{
				.seqNum   = 4,
				.seqId    = 5,
				.reserved = 0,
			},
			{
				.seqNum   = 2,
				.seqId    = 9,
				.reserved = 0,
			},
			{
				.seqNum   = 4,
				.seqId    = 11,
				.reserved = 0,
			}
		},
	},
	.pageSize           = 512u,
	.sectorSize         = 256u * 1024u,
	.ipcmdSerialClkFreq = 1u,
	.serialNorType      = 1u,
	.blockSize          = 256u * 1024u,
	.isUniformBlockSize = true,
};

#endif /* XIP_BOOT_HEADER_ENABLE */
