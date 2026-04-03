/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FLASH_CONFIG__
#define __FLASH_CONFIG__

#include <stdint.h>
#include <stdbool.h>
#include "fsl_common.h"


#define FC_BLOCK_TAG     (0x42464346)
#define FC_BLOCK_VERSION (0x00000000)

#define FC_CMD_SDR        0x01
#define FC_CMD_DDR        0x21
#define FC_RADDR_SDR      0x02
#define FC_RADDR_DDR      0x22
#define FC_CADDR_SDR      0x03
#define FC_CADDR_DDR      0x23
#define FC_MODE1_SDR      0x04
#define FC_MODE1_DDR      0x24
#define FC_MODE2_SDR      0x05
#define FC_MODE2_DDR      0x25
#define FC_MODE4_SDR      0x06
#define FC_MODE4_DDR      0x26
#define FC_MODE8_SDR      0x07
#define FC_MODE8_DDR      0x27
#define FC_WRITE_SDR      0x08
#define FC_WRITE_DDR      0x28
#define FC_READ_SDR       0x09
#define FC_READ_DDR       0x29
#define FC_LEARN_SDR      0x0A
#define FC_LEARN_DDR      0x2A
#define FC_DATSZ_SDR      0x0B
#define FC_DATSZ_DDR      0x2B
#define FC_DUMMY_SDR      0x0C
#define FC_DUMMY_DDR      0x2C
#define FC_DUMMY_RWDS_SDR 0x0D
#define FC_DUMMY_RWDS_DDR 0x2D
#define FC_JMP_ON_CS      0x1F
#define FC_STOP_EXE       0

#define FC_FLEXSPI_1PAD 0
#define FC_FLEXSPI_2PAD 1
#define FC_FLEXSPI_4PAD 2
#define FC_FLEXSPI_8PAD 3

#define FC_FLEXSPI_LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1)                                       \
	(FLEXSPI_LUT_OPERAND0(op0) | FLEXSPI_LUT_NUM_PADS0(pad0) | FLEXSPI_LUT_OPCODE0(cmd0) |     \
	 FLEXSPI_LUT_OPERAND1(op1) | FLEXSPI_LUT_NUM_PADS1(pad1) | FLEXSPI_LUT_OPCODE1(cmd1))


enum {
	kSerialFlash_1Pads = 1,
	kSerialFlash_2Pads = 2,
	kSerialFlash_4Pads = 4,
	kSerialFlash_8Pads = 8,
};


enum {
	kFlexSpiSerialClk_30MHz = 1,
	kFlexSpiSerialClk_50MHz = 2,
	kFlexSpiSerialClk_60MHz = 3,
	kFlexSpiSerialClk_80MHz = 4,
	kFlexSpiSerialClk_100MHz = 5,
	kFlexSpiSerialClk_120MHz = 6,
	kFlexSpiSerialClk_133MHz = 7,
	kFlexSpiSerialClk_166MHz = 8,
	kFlexSpiSerialClk_200MHz = 9,
};


enum {
	kFlexSpiSerialClk_SDR_24MHz = 1,
	kFlexSpiSerialClk_SDR_48MHz = 2,
};


enum {
	kFlexSpiSerialClk_DDR_48MHz = 1,
};


enum {
	kFlexSpiMiscOffset_DiffClkEnable = 0,
	kFlexSpiMiscOffset_WordAddressableEnable = 3,
	kFlexSpiMiscOffset_SafeConfigFreqEnable =
		4,
	kFlexSpiMiscOffset_DdrModeEnable = 6,
};


enum {
	kDeviceConfigCmdType_Generic,
	kDeviceConfigCmdType_QuadEnable,
	kDeviceConfigCmdType_Spi2Xpi,
	kDeviceConfigCmdType_Xpi2Spi,
	kDeviceConfigCmdType_Spi2NoCmd,
	kDeviceConfigCmdType_Reset,
};

typedef struct _fc_flexspi_dll_time {
	uint8_t time_100ps;
	uint8_t delay_cells;
} fc_flexspi_dll_time_t;


typedef struct _fc_flexspi_lut_seq {
	uint8_t seqNum;
	uint8_t seqId;
	uint16_t reserved;
} fc_flexspi_lut_seq_t;


typedef struct _fc_flexspi_mem_config {
	uint32_t tag;
	uint32_t version;
	uint32_t reserved0;
	uint8_t readSampleClkSrc;
	uint8_t csHoldTime;
	uint8_t csSetupTime;
	uint8_t columnAddressWidth;
	uint8_t deviceModeCfgEnable;
	uint8_t deviceModeType;
	uint16_t waitTimeCfgCommands;
	fc_flexspi_lut_seq_t deviceModeSeq;
	uint32_t deviceModeArg;
	uint8_t configCmdEnable;
	uint8_t configModeType[3];
	fc_flexspi_lut_seq_t configCmdSeqs[3];
	uint32_t reserved1;
	uint32_t configCmdArgs[3];
	uint32_t reserved2;
	uint32_t controllerMiscOption;
	uint8_t deviceType;
	uint8_t sflashPadType;
	uint8_t serialClkFreq;
	uint8_t lutCustomSeqEnable;
	uint32_t reserved3[2];
	uint32_t sflashA1Size;
	uint32_t sflashA2Size;
	uint32_t sflashB1Size;
	uint32_t sflashB2Size;
	uint32_t csPadSettingOverride;
	uint32_t sclkPadSettingOverride;
	uint32_t dataPadSettingOverride;
	uint32_t dqsPadSettingOverride;
	uint32_t timeoutInMs;
	uint32_t commandInterval;
	fc_flexspi_dll_time_t dataValidTime[2];
	uint16_t busyOffset;
	uint16_t busyBitPolarity;
	uint32_t lookupTable[64];
	fc_flexspi_lut_seq_t lutCustomSeq[12];
	uint32_t reserved4[4];
} fc_flexspi_mem_config_t;

typedef struct _fc_flexspi_nor_config {
#if defined(__ARMCC_VERSION) || defined(__ICCARM__)
	uint8_t padding[0x400];
#endif
	fc_flexspi_mem_config_t memConfig;
	uint32_t pageSize;
	uint32_t sectorSize;
	uint8_t ipcmdSerialClkFreq;
	uint8_t isUniformBlockSize;
	uint8_t isDataOrderSwapped;
	uint8_t reserved0[1];
	uint8_t serialNorType;
	uint8_t needExitNoCmdMode;
	uint8_t halfClkForNonReadCmd;
	uint8_t needRestoreNoCmdMode;
	uint32_t blockSize;
	uint32_t flashStateCtx;
	uint32_t reserve2[10];
	uint32_t fcb_fill[0x280];
} fc_flexspi_nor_config_t;
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif
