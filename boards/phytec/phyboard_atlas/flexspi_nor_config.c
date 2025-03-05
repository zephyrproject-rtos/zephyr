/*
 * Copyright (c) 2025 PHYTEC America, LLC
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <flexspi_nor_config.h>

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.xip_board"
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
#if defined(XIP_BOOT_HEADER_ENABLE) && (XIP_BOOT_HEADER_ENABLE == 1)
#if defined(__CC_ARM) || defined(__ARMCC_VERSION) || defined(__GNUC__)
__attribute__((section(".boot_hdr.conf"), used))
#elif defined(__ICCARM__)
#pragma location = ".boot_hdr.conf"
#endif

#define FLASH_DUMMY_CYCLES 0x06
#define FLASH_DUMMY_VALUE  0x06

const struct flexspi_nor_config_t qspiflash_config = {
	.memConfig = {
			.tag = FLEXSPI_CFG_BLK_TAG,
			.version = FLEXSPI_CFG_BLK_VERSION,
			.readSampleClkSrc = kFlexSPIReadSampleClk_LoopbackFromDqsPad,
			.csHoldTime = 3u,
			.csSetupTime = 3u,
			.controllerMiscOption = 0x10,
			.deviceType = kFlexSpiDeviceType_SerialNOR,
			.sflashPadType = kSerialFlash_4Pads,
			.serialClkFreq = kFlexSpiSerialClk_100MHz,
			.sflashA1Size = 16u * 1024u * 1024u,
			.configCmdEnable = 1u,
			.configModeType[0] = kDeviceConfigCmdType_Generic,
			.configCmdSeqs[0] = {
					.seqNum = 1,
					.seqId = 12,
					.reserved = 0,
				},
			.configCmdArgs[0] = (FLASH_DUMMY_VALUE << 3),
			.lookupTable = {
					/* Fast read quad mode - SDR */
					[4 * CMD_LUT_SEQ_IDX_READ + 0] =
						FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0xEB,
								RADDR_SDR, FLEXSPI_4PAD, 0x18),
					[4 * CMD_LUT_SEQ_IDX_READ + 1] = FLEXSPI_LUT_SEQ(
						DUMMY_SDR, FLEXSPI_4PAD, FLASH_DUMMY_CYCLES,
						READ_SDR, FLEXSPI_4PAD, 0x04),

					/* Read Status LUTs */
					[4 * CMD_LUT_SEQ_IDX_READSTATUS + 0] =
						FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x05,
								READ_SDR, FLEXSPI_1PAD, 0x04),

					/* Write Enable LUTs */
					[4 * CMD_LUT_SEQ_IDX_WRITEENABLE + 0] =
						FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x06, STOP,
								FLEXSPI_1PAD, 0x0),

					/* Page Program LUTs */
					[4 * CMD_LUT_SEQ_IDX_WRITE + 0] =
						FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x02,
								RADDR_SDR, FLEXSPI_1PAD, 0x18),
					[4 * CMD_LUT_SEQ_IDX_WRITE + 1] =
						FLEXSPI_LUT_SEQ(WRITE_SDR, FLEXSPI_1PAD, 0x04, STOP,
								FLEXSPI_1PAD, 0x0),
				},
		},
	.pageSize = 256u,
	.sectorSize = 4u * 1024u,
	.ipcmdSerialClkFreq = 0x1,
	.blockSize = 64u * 1024u,
	.isUniformBlockSize = false,
};
#endif /* XIP_BOOT_HEADER_ENABLE */
