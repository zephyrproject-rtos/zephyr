/*
 * Copyright (c) 2019, MADMACHINE LIMITED
 *
 * refer to hal_nxp board file
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MMSWIFTIO_FLEXSPI_NOR_CONFIG__
#define __MMSWIFTIO_FLEXSPI_NOR_CONFIG__

#include <zephyr/types.h>
#include "fsl_common.h"

#define FLEXSPI_CFG_BLK_TAG (0x42464346UL)
#define FLEXSPI_CFG_BLK_VERSION (0x56010400UL)
#define FLEXSPI_CFG_BLK_SIZE (512)

#define FLEXSPI_FEATURE_HAS_PARALLEL_MODE 1

#define CMD_INDEX_READ 0
#define CMD_INDEX_READSTATUS 1
#define CMD_INDEX_WRITEENABLE 2
#define CMD_INDEX_WRITE 4

#define CMD_LUT_SEQ_IDX_READ 0
#define CMD_LUT_SEQ_IDX_READSTATUS 1
#define CMD_LUT_SEQ_IDX_WRITEENABLE 3
#define CMD_LUT_SEQ_IDX_WRITE 9

#define CMD_SDR 0x01
#define CMD_DDR 0x21
#define RADDR_SDR 0x02
#define RADDR_DDR 0x22
#define CADDR_SDR 0x03
#define CADDR_DDR 0x23
#define MODE1_SDR 0x04
#define MODE1_DDR 0x24
#define MODE2_SDR 0x05
#define MODE2_DDR 0x25
#define MODE4_SDR 0x06
#define MODE4_DDR 0x26
#define MODE8_SDR 0x07
#define MODE8_DDR 0x27
#define WRITE_SDR 0x08
#define WRITE_DDR 0x28
#define READ_SDR 0x09
#define READ_DDR 0x29
#define LEARN_SDR 0x0A
#define LEARN_DDR 0x2A
#define DATSZ_SDR 0x0B
#define DATSZ_DDR 0x2B
#define DUMMY_SDR 0x0C
#define DUMMY_DDR 0x2C
#define DUMMY_RWDS_SDR 0x0D
#define DUMMY_RWDS_DDR 0x2D
#define JMP_ON_CS 0x1F
#define STOP 0

#define FLEXSPI_1PAD 0
#define FLEXSPI_2PAD 1
#define FLEXSPI_4PAD 2
#define FLEXSPI_8PAD 3

#define FLEXSPI_LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1)	   \
	(FLEXSPI_LUT_OPERAND0(op0) | FLEXSPI_LUT_NUM_PADS0(pad0) | \
	 FLEXSPI_LUT_OPCODE0(cmd0) | FLEXSPI_LUT_OPERAND1(op1) |   \
	 FLEXSPI_LUT_NUM_PADS1(pad1) | FLEXSPI_LUT_OPCODE1(cmd1))

enum {
	kFlexSpiSerialClk_30MHz  = 1,
	kFlexSpiSerialClk_50MHz  = 2,
	kFlexSpiSerialClk_60MHz  = 3,
	kFlexSpiSerialClk_75MHz  = 4,
	kFlexSpiSerialClk_80MHz  = 5,
	kFlexSpiSerialClk_100MHz = 6,
	kFlexSpiSerialClk_133MHz = 7,
	kFlexSpiSerialClk_166MHz = 8,
	kFlexSpiSerialClk_200MHz = 9,
};

enum {
	kFlexSpiClk_SDR,
	kFlexSpiClk_DDR, };

enum {
	kFlexSPIReadSampleClk_LoopbackInternally = 0,
	kFlexSPIReadSampleClk_LoopbackFromDqsPad = 1,
	kFlexSPIReadSampleClk_LoopbackFromSckPad = 2,
	kFlexSPIReadSampleClk_ExternalInputFromDqsPad   = 3,
};


enum { kFlexSpiMiscOffset_DiffClkEnable = 0,
	   kFlexSpiMiscOffset_Ck2Enable	 = 1,
	   kFlexSpiMiscOffset_ParallelEnable = 2,
	   kFlexSpiMiscOffset_WordAddressableEnable  = 3,
	   kFlexSpiMiscOffset_SafeConfigFreqEnable   = 4,
	   kFlexSpiMiscOffset_PadSettingOverrideEnable	  = 5,
	   kFlexSpiMiscOffset_DdrModeEnable = 6, };


enum { kFlexSpiDeviceType_SerialNOR	 = 1,
	   kFlexSpiDeviceType_SerialNAND	= 2,
	   kFlexSpiDeviceType_SerialRAM	 = 3,
	   kFlexSpiDeviceType_MCP_NOR_NAND  = 0x12,
	   kFlexSpiDeviceType_MCP_NOR_RAM   = 0x13, };


enum { kSerialFlash_1Pad = 1,
	   kSerialFlash_2Pads	   = 2,
	   kSerialFlash_4Pads	   = 4,
	   kSerialFlash_8Pads	   = 8, };


struct flexspi_lut_seq_t {
	u8_t seqNum;
	u8_t seqId;
	u16_t reserved;
};


enum { kDeviceConfigCmdType_Generic,
	   kDeviceConfigCmdType_QuadEnable,
	   kDeviceConfigCmdType_Spi2Xpi,
	   kDeviceConfigCmdType_Xpi2Spi,
	   kDeviceConfigCmdType_Spi2NoCmd,
	   kDeviceConfigCmdType_Reset, };


struct flexspi_mem_config_t {
	u32_t tag;
	u32_t version;
	u32_t reserved0;
	u8_t readSampleClkSrc;
	u8_t csHoldTime;
	u8_t csSetupTime;
	u8_t columnAddressWidth;

	u8_t deviceModeCfgEnable;
	u8_t deviceModeType;

	u16_t waitTimeCfgCommands;

	struct flexspi_lut_seq_t deviceModeSeq;

	u32_t deviceModeArg;
	u8_t configCmdEnable;
	u8_t configModeType[3];
	struct flexspi_lut_seq_t configCmdSeqs[3];
	u32_t reserved1;
	u32_t configCmdArgs[3];
	u32_t reserved2;
	u32_t controllerMiscOption;

	u8_t deviceType;
	u8_t sflashPadType;
	u8_t serialClkFreq;

	u8_t lutCustomSeqEnable;

	u32_t reserved3[2];
	u32_t sflashA1Size;
	u32_t sflashA2Size;
	u32_t sflashB1Size;
	u32_t sflashB2Size;
	u32_t csPadSettingOverride;
	u32_t sclkPadSettingOverride;
	u32_t dataPadSettingOverride;
	u32_t dqsPadSettingOverride;
	u32_t timeoutInMs;
	u32_t commandInterval;
	u16_t dataValidTime[2];
	u16_t busyOffset;
	u16_t busyBitPolarity;

	u32_t lookupTable[64];
	struct flexspi_lut_seq_t lutCustomSeq[12];
	u32_t reserved4[4];
};


#define NOR_CMD_INDEX_READ CMD_INDEX_READ
#define NOR_CMD_INDEX_READSTATUS CMD_INDEX_READSTATUS
#define NOR_CMD_INDEX_WRITEENABLE CMD_INDEX_WRITEENABLE
#define NOR_CMD_INDEX_ERASESECTOR 3
#define NOR_CMD_INDEX_PAGEPROGRAM CMD_INDEX_WRITE
#define NOR_CMD_INDEX_CHIPERASE 5
#define NOR_CMD_INDEX_DUMMY 6
#define NOR_CMD_INDEX_ERASEBLOCK 7

#define NOR_CMD_LUT_SEQ_IDX_READ \
	CMD_LUT_SEQ_IDX_READ
#define NOR_CMD_LUT_SEQ_IDX_READSTATUS \
	CMD_LUT_SEQ_IDX_READSTATUS
#define NOR_CMD_LUT_SEQ_IDX_READSTATUS_XPI \
	2
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE	\
	CMD_LUT_SEQ_IDX_WRITEENABLE
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE_XPI \
	4
#define NOR_CMD_LUT_SEQ_IDX_ERASESECTOR	\
	5
#define NOR_CMD_LUT_SEQ_IDX_ERASEBLOCK \
	8
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM	\
	CMD_LUT_SEQ_IDX_WRITE
#define NOR_CMD_LUT_SEQ_IDX_CHIPERASE \
	11
#define NOR_CMD_LUT_SEQ_IDX_READ_SFDP \
	13
#define NOR_CMD_LUT_SEQ_IDX_RESTORE_NOCMD \
	14
#define NOR_CMD_LUT_SEQ_IDX_EXIT_NOCMD \
	15


struct flexspi_nor_config_t {
	struct flexspi_mem_config_t memConfig;
	u32_t pageSize;
	u32_t sectorSize;
	u8_t ipcmdSerialClkFreq;
	u8_t isUniformBlockSize;
	u8_t reserved0[2];
	u8_t serialNorType;
	u8_t needExitNoCmdMode;
	u8_t halfClkForNonReadCmd;
	u8_t needRestoreNoCmdMode;
	u32_t blockSize;
	u32_t reserve2[11];
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif
