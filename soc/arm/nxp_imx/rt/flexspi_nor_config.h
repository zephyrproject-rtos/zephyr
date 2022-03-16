/*
 * Copyright (c) 2019, MADMACHINE LIMITED
 *
 * refer to hal_nxp board file
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FLEXSPI_NOR_CONFIG__
#define __FLEXSPI_NOR_CONFIG__

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

/* For flexspi_mem_config.serialClkFreq */
#if defined(CONFIG_SOC_MIMXRT1011)
enum {
	kFlexSpiSerialClk_30MHz  = 1,
	kFlexSpiSerialClk_50MHz  = 2,
	kFlexSpiSerialClk_60MHz  = 3,
	kFlexSpiSerialClk_75MHz  = 4,
	kFlexSpiSerialClk_80MHz  = 5,
	kFlexSpiSerialClk_100MHz = 6,
	kFlexSpiSerialClk_120MHz = 7,
	kFlexSpiSerialClk_133MHz = 8,
};
#elif defined(CONFIG_SOC_MIMXRT1015) || defined(CONFIG_SOC_MIMXRT1021) || \
	defined(CONFIG_SOC_MIMXRT1024)
enum {
	kFlexSpiSerialClk_30MHz  = 1,
	kFlexSpiSerialClk_50MHz  = 2,
	kFlexSpiSerialClk_60MHz  = 3,
	kFlexSpiSerialClk_75MHz  = 4,
	kFlexSpiSerialClk_80MHz  = 5,
	kFlexSpiSerialClk_100MHz = 6,
	kFlexSpiSerialClk_133MHz = 7,
};
#elif defined(CONFIG_SOC_MIMXRT1051) || defined(CONFIG_SOC_MIMXRT1052)
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
#elif defined(CONFIG_SOC_MIMXRT1061) || defined(CONFIG_SOC_MIMXRT1062) || \
	defined(CONFIG_SOC_MIMXRT1062) || defined(CONFIG_SOC_MIMXRT1064)
enum {
	kFlexSpiSerialClk_30MHz  = 1,
	kFlexSpiSerialClk_50MHz  = 2,
	kFlexSpiSerialClk_60MHz  = 3,
	kFlexSpiSerialClk_75MHz  = 4,
	kFlexSpiSerialClk_80MHz  = 5,
	kFlexSpiSerialClk_100MHz = 6,
	kFlexSpiSerialClk_120MHz = 7,
	kFlexSpiSerialClk_133MHz = 8,
	kFlexSpiSerialClk_166MHz = 9,
};
#else
#error "kFlexSpiSerialClk is not defined for this SoC"
#endif

/* For flexspi_mem_config.controllerMiscOption */
enum {
	kFlexSpiClk_SDR,
	kFlexSpiClk_DDR,
};

/* For flexspi_mem_config.readSampleClkSrc */
enum {
	kFlexSPIReadSampleClk_LoopbackInternally = 0,
	kFlexSPIReadSampleClk_LoopbackFromDqsPad = 1,
	kFlexSPIReadSampleClk_LoopbackFromSckPad = 2,
	kFlexSPIReadSampleClk_ExternalInputFromDqsPad   = 3,
};

/* For flexspi_mem_config.controllerMiscOption */
enum {
	/* !< Bit for Differential clock enable */
	kFlexSpiMiscOffset_DiffClkEnable = 0,
	/* !< Bit for CK2 enable */
	kFlexSpiMiscOffset_Ck2Enable	 = 1,
	/* !< Bit for Parallel mode enable */
	kFlexSpiMiscOffset_ParallelEnable = 2,
	/* !< Bit for Word Addressable enable */
	kFlexSpiMiscOffset_WordAddressableEnable  = 3,
	/* !< Bit for Safe Configuration Frequency enable */
	kFlexSpiMiscOffset_SafeConfigFreqEnable   = 4,
	/* !< Bit for Pad setting override enable */
	kFlexSpiMiscOffset_PadSettingOverrideEnable	  = 5,
	/* !< Bit for DDR clock configuration indication. */
	kFlexSpiMiscOffset_DdrModeEnable = 6,
};

/* For flexspi_mem_config.deviceType */
enum {
	/* !< Flash devices are Serial NOR */
	kFlexSpiDeviceType_SerialNOR	= 1,
	/* !< Flash devices are Serial NAND */
	kFlexSpiDeviceType_SerialNAND	= 2,
	/* !< Flash devices are Serial RAM/HyperFLASH */
	kFlexSpiDeviceType_SerialRAM	= 3,
	/* !< Flash device is MCP device, A1 is Serial NOR, A2 is Serial NAND */
	kFlexSpiDeviceType_MCP_NOR_NAND = 0x12,
	/* !< Flash device is MCP device, A1 is Serial NOR, A2 is Serial RAMs */
	kFlexSpiDeviceType_MCP_NOR_RAM  = 0x13,
};

/* For flexspi_mem_config.sflashPadType */
enum {
	kSerialFlash_1Pad  = 1,
	kSerialFlash_2Pads = 2,
	kSerialFlash_4Pads = 4,
	kSerialFlash_8Pads = 8,
};

enum {
	/* !< Generic command, for example: configure dummy cycles, drive strength, etc */
	kDeviceConfigCmdType_Generic,
	/* !< Quad Enable command */
	kDeviceConfigCmdType_QuadEnable,
	/* !< Switch from SPI to DPI/QPI/OPI mode */
	kDeviceConfigCmdType_Spi2Xpi,
	/* !< Switch from DPI/QPI/OPI to SPI mode */
	kDeviceConfigCmdType_Xpi2Spi,
	/* !< Switch to 0-4-4/0-8-8 mode */
	kDeviceConfigCmdType_Spi2NoCmd,
	/* !< Reset device command */
	kDeviceConfigCmdType_Reset,
};

struct flexspi_lut_seq_t {
	uint8_t seqNum;
	uint8_t seqId;
	uint16_t reserved;
};

struct flexspi_mem_config_t {
	/* !< [0x000-0x003] Tag, fixed value 0x42464346UL */
	uint32_t tag;
	/* !< [0x004-0x007] Version,[31:24] -'V', [23:16] - Major, [15:8] - Minor, [7:0] - bugfix */
	uint32_t version;
	/* !< [0x008-0x00b] Reserved for future use */
	uint32_t reserved0;
	/* !< [0x00c-0x00c] Read Sample Clock Source, valid value: 0/1/3 */
	uint8_t readSampleClkSrc;
	/* !< [0x00d-0x00d] CS hold time, default value: 3 */
	uint8_t csHoldTime;
	/* !< [0x00e-0x00e] CS setup time, default value: 3 */
	uint8_t csSetupTime;
	/* !< [0x00f-0x00f] Column Address with, for HyperBus protocol, it is fixed to 3, For */
	uint8_t columnAddressWidth;
	/* ! Serial NAND, need to refer to datasheet */
	/* !< [0x010-0x010] Device Mode Configure enable flag, 1 - Enable, 0 - Disable */
	uint8_t deviceModeCfgEnable;
	/* !< [0x011-0x011] Specify the configuration command
	 * type:Quad Enable, DPI/QPI/OPI switch,
	 */
	uint8_t deviceModeType;
	/* ! Generic configuration, etc. */
	/* !< [0x012-0x013] Wait time for all configuration commands, unit: 100us, Used for */
	uint16_t waitTimeCfgCommands;
	/* ! DPI/QPI/OPI switch or reset command */
	/* !< [0x014-0x017] Device mode sequence info, [7:0] - LUT sequence id, [15:8] - LUt */
	struct flexspi_lut_seq_t deviceModeSeq;
	/* ! sequence number, [31:16] Reserved */
	/* !< [0x018-0x01b] Argument/Parameter for device configuration */
	uint32_t deviceModeArg;
	/* !< [0x01c-0x01c] Configure command Enable Flag, 1 - Enable, 0 - Disable */
	uint8_t configCmdEnable;
	/* !< [0x01d-0x01f] Configure Mode Type, similar as deviceModeTpe */
	uint8_t configModeType[3];
	/* !< [0x020-0x02b] Sequence info for Device Configuration command, similar as
	 * deviceModeSeq
	 */
	struct flexspi_lut_seq_t configCmdSeqs[3];
	/* !< [0x02c-0x02f] Reserved for future use */
	uint32_t reserved1;
	/* !< [0x030-0x03b] Arguments/Parameters for device Configuration commands */
	uint32_t configCmdArgs[3];
	/* !< [0x03c-0x03f] Reserved for future use */
	uint32_t reserved2;
	/* !< [0x040-0x043] Controller Misc Options, see Misc feature bit definitions for more */
	uint32_t controllerMiscOption;
	/* ! details */
	/* !< [0x044-0x044] Device Type:  See Flash Type Definition for more details */
	uint8_t deviceType;
	/* !< [0x045-0x045] Serial Flash Pad Type: 1 - Single, 2 - Dual, 4 - Quad, 8 - Octal */
	uint8_t sflashPadType;
	/* !< [0x046-0x046] Serial Flash Frequency, device specific definitions, See System Boot */
	uint8_t serialClkFreq;
	/* ! Chapter for more details */
	/* !< [0x047-0x047] LUT customization Enable, it is required if the program/erase cannot */
	uint8_t lutCustomSeqEnable;
	/* ! be done using 1 LUT sequence, currently, only applicable to HyperFLASH */
	/* !< [0x048-0x04f] Reserved for future use */
	uint32_t reserved3[2];
	/* !< [0x050-0x053] Size of Flash connected to A1 */
	uint32_t sflashA1Size;
	/* !< [0x054-0x057] Size of Flash connected to A2 */
	uint32_t sflashA2Size;
	/* !< [0x058-0x05b] Size of Flash connected to B1 */
	uint32_t sflashB1Size;
	/* !< [0x05c-0x05f] Size of Flash connected to B2 */
	uint32_t sflashB2Size;
	/* !< [0x060-0x063] CS pad setting override value */
	uint32_t csPadSettingOverride;
	/* !< [0x064-0x067] SCK pad setting override value */
	uint32_t sclkPadSettingOverride;
	/* !< [0x068-0x06b] data pad setting override value */
	uint32_t dataPadSettingOverride;
	/* !< [0x06c-0x06f] DQS pad setting override value */
	uint32_t dqsPadSettingOverride;
	/* !< [0x070-0x073] Timeout threshold for read status command */
	uint32_t timeoutInMs;
	/* !< [0x074-0x077] CS deselect interval between two commands */
	uint32_t commandInterval;
	/* !< [0x078-0x07b] CLK edge to data valid time for PORT A and PORT B, in terms of 0.1ns */
	uint16_t dataValidTime[2];
	/* !< [0x07c-0x07d] Busy offset, valid value: 0-31 */
	uint16_t busyOffset;
	/* !< [0x07e-0x07f] Busy flag polarity, 0 - busy flag is 1 when flash device is busy, 1 - */
	uint16_t busyBitPolarity;
	/* ! busy flag is 0 when flash device is busy */
	/* !< [0x080-0x17f] Lookup table holds Flash command sequences */
	uint32_t lookupTable[64];
	/* !< [0x180-0x1af] Customizable LUT Sequences */
	struct flexspi_lut_seq_t lutCustomSeq[12];
	/* !< [0x1b0-0x1bf] Reserved for future use */
	uint32_t reserved4[4];
};

#define NOR_CMD_INDEX_READ CMD_INDEX_READ
#define NOR_CMD_INDEX_READSTATUS CMD_INDEX_READSTATUS
#define NOR_CMD_INDEX_WRITEENABLE CMD_INDEX_WRITEENABLE
#define NOR_CMD_INDEX_ERASESECTOR 3
#define NOR_CMD_INDEX_PAGEPROGRAM CMD_INDEX_WRITE
#define NOR_CMD_INDEX_CHIPERASE 5
#define NOR_CMD_INDEX_DUMMY 6
#define NOR_CMD_INDEX_ERASEBLOCK 7

#define NOR_CMD_LUT_SEQ_IDX_READ  CMD_LUT_SEQ_IDX_READ
#define NOR_CMD_LUT_SEQ_IDX_READSTATUS  CMD_LUT_SEQ_IDX_READSTATUS
#define NOR_CMD_LUT_SEQ_IDX_READSTATUS_XPI  2
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE	 CMD_LUT_SEQ_IDX_WRITEENABLE
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE_XPI  4
#define NOR_CMD_LUT_SEQ_IDX_ERASESECTOR	 5
#define NOR_CMD_LUT_SEQ_IDX_ERASEBLOCK  8
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM	 CMD_LUT_SEQ_IDX_WRITE
#define NOR_CMD_LUT_SEQ_IDX_CHIPERASE  11
#define NOR_CMD_LUT_SEQ_IDX_READ_SFDP		13
#define NOR_CMD_LUT_SEQ_IDX_RESTORE_NOCMD	14
#define NOR_CMD_LUT_SEQ_IDX_EXIT_NOCMD		15

struct flexspi_nor_config_t {
	/* !< Common memory configuration info via FlexSPI */
	struct flexspi_mem_config_t memConfig;
	/* !< Page size of Serial NOR */
	uint32_t pageSize;
	/* !< Sector size of Serial NOR */
	uint32_t sectorSize;
	/* !< Clock frequency for IP command */
	uint8_t ipcmdSerialClkFreq;
	/* !< Sector/Block size is the same */
	uint8_t isUniformBlockSize;
	/* !< Reserved for future use */
	uint8_t reserved0[2];
	/* !< Serial NOR Flash type: 0/1/2/3 */
	uint8_t serialNorType;
	/* !< Need to exit NoCmd mode before other IP command */
	uint8_t needExitNoCmdMode;
	/* !< Half the Serial Clock for non-read command: true/false */
	uint8_t halfClkForNonReadCmd;
	/* !< Need to Restore NoCmd mode after IP command execution */
	uint8_t needRestoreNoCmdMode;
	/* !< Block size */
	uint32_t blockSize;
	/* !< Reserved for future use */
	uint32_t reserve2[11];
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif
