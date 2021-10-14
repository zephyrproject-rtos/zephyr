/*
 * Copyright (c) 2019, MADMACHINE LIMITED
 * Copyright (c) 2021, Bernhard Kraemer
 *
 * refer to hal_nxp board file
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <flexspi_nor_config.h>

#ifdef CONFIG_NXP_IMX_RT_BOOT_HEADER
#if defined(__CC_ARM) || defined(__ARMCC_VERSION) || defined(__GNUC__)
__attribute__((section(".boot_hdr.conf")))
#elif defined(__ICCARM__)
#pragma location = ".boot_hdr.conf"
#endif

const struct flexspi_nor_config_t Qspiflash_config = {
	.memConfig = {
		.tag = FLEXSPI_CFG_BLK_TAG,
		.version = FLEXSPI_CFG_BLK_VERSION,
		.readSampleClkSrc =
			kFlexSPIReadSampleClk_LoopbackFromDqsPad,
		.csHoldTime = 3u,
		.csSetupTime = 3u,
		.sflashPadType = kSerialFlash_4Pads,
		.serialClkFreq = kFlexSpiSerialClk_100MHz,
		.sflashA1Size = 8u * 1024u * 1024u,
		.lookupTable = {
			FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
					0xEB, RADDR_SDR,
					FLEXSPI_4PAD, 0x18),
			FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_4PAD,
					0x06, READ_SDR,
					FLEXSPI_4PAD, 0x04),
		},
	},
	.pageSize = 256u,
	.sectorSize = 4u * 1024u,
	.blockSize = 256u * 1024u,
	.isUniformBlockSize = false,
};
#endif /* CONFIG_NXP_IMX_RT_BOOT_HEADER */
