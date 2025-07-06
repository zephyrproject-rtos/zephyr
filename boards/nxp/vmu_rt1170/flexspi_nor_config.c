/*
 * Copyright (c) 2019, MADMACHINE LIMITED
 * Copyright 2024 NXP
 *
 * refer to hal_nxp board file
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <flexspi_nor_config.h>

/*!
 * @brief ROM API init
 *
 * Get the bootloader api entry address.
 */
void ROM_API_Init(void);

/*!
 * @brief Initialize Serial NOR devices via FLEXSPI
 *
 * This function checks and initializes the FLEXSPI module for the other FLEXSPI APIs.
 *
 * @param instance storage the instance of FLEXSPI.
 * @param config A pointer to the storage for the driver runtime state.
 *
 * @retval kStatus_Success Api was executed successfully.
 * @retval kStatus_InvalidArgument A invalid argument is provided.
 * @retval kStatus_ROM_FLEXSPI_InvalidSequence A invalid Sequence is provided.
 * @retval kStatus_ROM_FLEXSPI_SequenceExecutionTimeout Sequence Execution timeout.
 * @retval kStatus_ROM_FLEXSPI_DeviceTimeout the device timeout
 */
status_t ROM_FLEXSPI_NorFlash_Init(uint32_t instance, struct flexspi_nor_config_t *config);



#ifdef CONFIG_NXP_IMXRT_BOOT_HEADER
#if defined(__CC_ARM) || defined(__ARMCC_VERSION) || defined(__GNUC__)
__attribute__((section(".boot_hdr.conf")))
#elif defined(__ICCARM__)
#pragma location = ".boot_hdr.conf"
#endif

/* Config used for booting */

const struct flexspi_nor_config_t Qspiflash_config = {
	.memConfig = {
		.tag = FLEXSPI_CFG_BLK_TAG,
		.version = FLEXSPI_CFG_BLK_VERSION,
		.readSampleClkSrc =
			kFlexSPIReadSampleClk_LoopbackInternally,
		.csHoldTime = 1u,
		.csSetupTime = 1u,
		.sflashPadType = kSerialFlash_1Pad,
		.serialClkFreq = kFlexSpiSerialClk_80MHz,
		.sflashA1Size = 64u * 1024u * 1024u,
		.lookupTable = {
			FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
					0x03, RADDR_SDR,
					FLEXSPI_1PAD, 0x18),
			FLEXSPI_LUT_SEQ(READ_SDR, FLEXSPI_1PAD,
					0x04, STOP,
					FLEXSPI_1PAD, 0),
		},
	},
	.pageSize = 256u,
	.sectorSize = 4u * 1024u,
	.blockSize = 64u * 1024u,
	.isUniformBlockSize = false,
};
#endif /* CONFIG_NXP_IMXRT_BOOT_HEADER */

/* Config used for code execution */
const struct flexspi_nor_config_t g_flash_fast_config = {
	.memConfig = {
		.tag                 = FLEXSPI_CFG_BLK_TAG,
		.version             = FLEXSPI_CFG_BLK_VERSION,
		.readSampleClkSrc    = kFlexSPIReadSampleClk_ExternalInputFromDqsPad,
		.csHoldTime          = 1,
		.csSetupTime         = 1,
		.deviceModeCfgEnable = 1,
		.deviceModeType      = kDeviceConfigCmdType_Spi2Xpi,
		.waitTimeCfgCommands = 1,
		.deviceModeSeq = {
			.seqNum   = 1,
			.seqId    = 6, /* See Lookup table for more details */
			.reserved = 0,
		},
		.deviceModeArg = 2, /* Enable OPI DDR mode */
		.controllerMiscOption =
		(1u << kFlexSpiMiscOffset_SafeConfigFreqEnable)
			| (1u << kFlexSpiMiscOffset_DdrModeEnable),
		.deviceType    = kFlexSpiDeviceType_SerialNOR,
		.sflashPadType = kSerialFlash_8Pads,
		.serialClkFreq = kFlexSpiSerialClk_200MHz,
		.sflashA1Size  = 64ul * 1024u * 1024u,
		.busyOffset      = 0u,
		.busyBitPolarity = 0u,
		.lookupTable = {
			/* Read */
			[0 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0xEE, CMD_DDR, FLEXSPI_8PAD, 0x11),
			[0 + 1] = FLEXSPI_LUT_SEQ(RADDR_DDR, FLEXSPI_8PAD,
				0x20, DUMMY_DDR, FLEXSPI_8PAD, 0x28),
			[0 + 2] = FLEXSPI_LUT_SEQ(READ_DDR, FLEXSPI_8PAD,
				0x04, STOP, FLEXSPI_1PAD, 0x00),

			/* Write enable SPI */
			[4 * 3 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x06, STOP, FLEXSPI_1PAD, 0x00),

			/*Write Configuration Register 2 =01, Enable OPI DDR mode*/
			[4 * 6 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x72, CMD_SDR, FLEXSPI_1PAD, 0x00),
			[4 * 6 + 1] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x00, CMD_SDR, FLEXSPI_1PAD, 0x00),
			[4 * 6 + 2] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x00, WRITE_SDR, FLEXSPI_1PAD, 0x01),

		},
	},
	.pageSize           = 256u,
	.sectorSize         = 4u * 1024u,
	.blockSize          = 64u * 1024u,
	.isUniformBlockSize = false,
	.ipcmdSerialClkFreq = 1,
	.serialNorType = 2,
	.reserve2[0] = 0x7008200,
};


__ramfunc int imxrt_reclock_initialize(void)
{
	const uint32_t instance =  1;

	volatile struct flexspi_nor_config_t bootConfig;

	memcpy((struct flexspi_nor_config_t *)&bootConfig, &g_flash_fast_config,
	       sizeof(struct flexspi_nor_config_t));
	bootConfig.memConfig.tag = FLEXSPI_CFG_BLK_TAG;

	ROM_API_Init();

	ROM_FLEXSPI_NorFlash_Init(instance, (struct flexspi_nor_config_t *)&bootConfig);

	return 0;
}

SYS_INIT(imxrt_reclock_initialize,  PRE_KERNEL_1, 0);
