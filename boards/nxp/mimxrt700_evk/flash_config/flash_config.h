/*
 * Copyright 2023, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef FLASH_CONFIG_H_
#define FLASH_CONFIG_H_
#include <stdint.h>
#include "fsl_common.h"

/*! @name Driver version */
/*@{*/
/*! @brief FLASH_CONFIG driver version 2.0.1. */
#define FSL_FLASH_CONFIG_DRIVER_VERSION (MAKE_VERSION(2, 0, 1))
/*@}*/

#ifndef FSL_FEATURE_SILICON_VERSION_A
#define FSL_FEATURE_SILICON_VERSION_A (1U)
#endif

/* XSPI memory config block related definitions */
#define FC_XSPI_CFG_BLK_TAG     (0x42464346UL) /* ascii "FCFB" Big Endian */
#define FC_XSPI_CFG_BLK_VERSION (0x56010400UL) /* V1.4.0 */

/*! @brief XSPI clock configuration - When clock source is PLL */
enum {
	Fc_XspiSerialClk_30MHz = 1,
	Fc_XspiSerialClk_50MHz = 2,
	Fc_XspiSerialClk_60MHz = 3,
	Fc_XspiSerialClk_80MHz = 4,
	Fc_XspiSerialClk_100MHz = 5,
	Fc_XspiSerialClk_120MHz = 6,
	Fc_XspiSerialClk_133MHz = 7,
	Fc_XspiSerialClk_166MHz = 8,
	Fc_XspiSerialClk_200MHz = 9,
};

/*! @brief LUT instructions supported by XSPI */
/*!< Stop execution, deassert CS. */
#define FC_CMD_STOP        0x00U
/*!< Transmit Command code to Flash, using SDR mode. */
#define FC_CMD_SDR         0x01U
/*!< Transmit Row Address to Flash, using SDR mode. */
#define FC_CMD_RADDR_SDR   0x02U
/*!< Leave data lines undriven by xSPI controller, using SDR mode. */
#define FC_CMD_DUMMY_SDR   0x03U
/*!< Transmit 8-bit Mode bits to Flash, using SDR mode. */
#define FC_CMD_MODE_SDR    0x04U
/*!< Transmit 2-bit Mode bits to Flash, using SDR mode. */
#define FC_CMD_MODE2_SDR   0x05U
/*!< Transmit 4-bit Mode bits to Flash, using SDR mode. */
#define FC_CMD_MODE4_SDR   0x06U
/*!< Receive Read Data from Flash, using SDR mode. */
#define FC_CMD_READ_SDR    0x07U
/*!< Transmit Programming Data to Flash, using SDR mode. */
#define FC_CMD_WRITE_SDR   0x08U
/*!< Stop execution, deassert CS and save operand[7:0] as
 * the instruction start pointer for next sequence
 */
#define FC_CMD_JMP_ON_CS   0x09U
/*!< Transmit Row Address to Flash, using DDR mode. */
#define FC_CMD_RADDR_DDR   0x0AU
/*!< Transmit 8-bit Mode bits to Flash, using DDR mode. */
#define FC_CMD_MODE_DDR    0x0BU
/*!< Transmit 2-bit Mode bits to Flash, using DDR mode. */
#define FC_CMD_MODE2_DDR   0x0CU
/*!< Transmit 4-bit Mode bits to Flash, using DDR mode. */
#define FC_CMD_MODE4_DDR   0x0DU
/*!< Receive Read Data from Flash, using DDR mode. */
#define FC_CMD_READ_DDR    0x0EU
/*!< Transmit Programming Data to Flash, using DDR mode. */
#define FC_CMD_WRITE_DDR   0x0FU
/*!< Receive Read Data or Preamble bit from Flash, DDR mode. */
#define FC_CMD_LEARN_DDR   0x10U
/*!< Transmit Command code to Flash, using DDR mode. */
#define FC_CMD_DDR         0x11U
/*!< Transmit Column Address to Flash, using SDR mode. */
#define FC_CMD_CADDR_SDR   0x12U
/*!< Transmit Column Address to Flash, using DDR mode. */
#define FC_CMD_CADDR_DDR   0x13U
#define FC_CMD_JUMP_TO_SEQ 0x14U

#define FC_XSPI_1PAD 0
#define FC_XSPI_2PAD 1
#define FC_XSPI_4PAD 2
#define FC_XSPI_8PAD 3

#define FC_XSPI_LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1)	\
	(XSPI_LUT_INSTR0(cmd0) | XSPI_LUT_PAD0(pad0) |		\
	XSPI_LUT_OPRND0(op0) | XSPI_LUT_INSTR1(cmd1) |		\
	XSPI_LUT_PAD1(pad1) | XSPI_LUT_OPRND1(op1))

/* !@brief XSPI Read Sample Clock Source definition */
typedef enum _FlashReadSampleClkSource {
	kXSPIReadSampleClk_LoopbackInternally = 0,
	kXSPIReadSampleClk_LoopbackFromDqsPad = 2,
	kXSPIReadSampleClk_ExternalInputFromDqsPad = 3,
} fc_xspi_read_sample_clk_t;

/* !@brief Misc feature bit definitions */
enum {
	/* !< Bit for Differential clock enable */
	Fc_XspiMiscOffset_DiffClkEnable = 0,
	/* !< Bit for Word Addressable enable */
	Fc_XspiMiscOffset_WordAddressableEnable = 3,
	/* !< Bit for Safe Configuration Frequency enable */
	Fc_XspiMiscOffset_SafeConfigFreqEnable = 4,
	/* !< Bit for DDR clock confiuration indication. */
	Fc_XspiMiscOffset_DdrModeEnable = 6,
};

typedef struct {
	uint8_t time_100ps;  /* !< Data valid time, in terms of 100ps */
	uint8_t delay_cells; /* !< Data valid time, in terms of delay cells */
} fc_xspi_dll_time_t;

/* !@brief XSPI LUT Sequence structure */
typedef struct _lut_sequence {
	uint8_t seqNum; /* !< Sequence Number, valid number: 1-16 */
	uint8_t seqId;  /* !< Sequence Index, valid number: 0-15 */
	uint16_t reserved;
} fc_xspi_lut_seq_t;

/* !@brief XSPI Memory Configuration Block */
typedef struct _XSPIConfig {
	/* !< [0x000-0x003] Tag, fixed value 0x42464346UL */
	uint32_t tag;
	/* !< [0x004-0x007] Version, [31:24] -'V',
	 * [23:16] - Major,
	 * [15:8] - Minor,
	 * [7:0] - bugfix
	 */
	uint32_t version;
	/* !< [0x008-0x00b] Reserved for future use */
	uint32_t reserved0;
	/* !< [0x00c-0x00c] Read Sample Clock Source, valid value:
	 * 0: internal sampling
	 * 2: DQS pad loopback
	 * 3: External DQS signal
	 */
	uint8_t readSampleClkSrc;
	/* !< [0x00d-0x00d] CS hold time, default value: 3 */
	uint8_t csHoldTime;
	/* !< [0x00e-0x00e] CS setup time, default value: 3 */
	uint8_t csSetupTime;
	/* !< [0x00f-0x00f] Column Address with, for HyperBus protocol,
	 * it is fixed to 3, others to 0.
	 */
	uint8_t columnAddressWidth;
	/* !< [0x010-0x010] Device Mode Configure enable flag,
	 * 1 - Enable, 0 - Disable
	 */
	uint8_t deviceModeCfgEnable;
	/* !< [0x011-0x011] Specify the configuration command type.
	 * 0: No mode change
	 * 1: Quad enable (switch from SPI to Quad mode)
	 * 2: Spi2Xpi (switch from SPI to DPI, QPI, or OPI mode)
	 * 3: Xpi2Spi (switch from DPI, QPI, or OPI to SPI mode)
	 */
	uint8_t deviceModeType;
	/* !< [0x012-0x013] Wait time for Device mode configuration
	 * command, unit: 100us
	 */
	uint16_t waitTimeCfgCommands;
	/* !< [0x014-0x017] Device mode sequence info
	 * [ 7:0] - Number of required sequences
	 * [15:8] - Sequence index
	 */
	fc_xspi_lut_seq_t deviceModeSeq;
	/* !< [0x018-0x01b] Argument/Parameter for device configuration */
	uint32_t deviceModeArg;
	/* !< [0x01c-0x01c] Configure command Enable Flag,
	 * 1 - Enable, 0 - Disable
	 */
	uint8_t configCmdEnable;
	/* !< [0x01d-0x01f] Configure Mode Type, similar as deviceModeTpe */
	uint8_t configModeType[3];
	/* !< [0x020-0x02b] Sequence info for Device Configuration command,
	 * similar as deviceModeSeq
	 */
	fc_xspi_lut_seq_t configCmdSeqs[3];
	/* !< [0x02c-0x02f] Reserved for future use */
	uint32_t reserved1;
	/* !< [0x030-0x03b] Arguments/Parameters for
	 * device Configuration commands
	 */
	uint32_t configCmdArgs[3];
	/* !< [0x03c-0x03f] Reserved for future use */
	uint32_t reserved2;
	/* !< [0x040-0x043] Controller Misc Options.
	 * Bit 0: Differential clock enable: 1 for HyperFlash NOR
	 * flash memory 1V8 device and 0 for other devices Bit 3:
	 * WordAddressableEnable: 1 for HyperFlash NOR flash memory
	 * and 0 for other devices Bit 4: SafeConfigFreqEnable: set
	 * to 1 if expecting to configure the chip with a safe
	 * frequency Bit 6: DDR mode enable: set to 1 if DDR read is
	 * expected Other bits Reserved; set to 0
	 */
	uint32_t controllerMiscOption;
	/* !< [0x044-0x044] Device Type: 1 for Serial NOR flash memory */
	uint8_t deviceType;
	/* !< [0x045-0x045] Serial Flash Pad Type:
	 *1 - Single, 2 - Dual, 4 - Quad, 8 - Octal
	 */
	uint8_t sflashPadType;
	/* !< [0x046-0x046] Serial Flash Frequencey
	 * 1: 30 MHz
	 * 2: 50 MHz
	 * 3: 60 MHz
	 * 4: 80 MHz
	 * 5: 100 MHz
	 * 6: 120 MHz
	 * 7: 133 MHz
	 * 8: 166 MHz
	 * 9: 200 MHz
	 */
	uint8_t serialClkFreq;
	/* !< [0x047-0x047] LUT customization Enable, it is required if
	 * the program/erase cannot be done using 1 LUT sequence,
	 * currently, only applicable to HyperFLASH
	 */
	uint8_t lutCustomSeqEnable;
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
	/* !< [0x078-0x07b] CLK edge to data valid time for
	 * PORT A and PORT B
	 */
	fc_xspi_dll_time_t dataValidTime[2];
	/* !< [0x07c-0x07d] Busy offset, valid value: 0-31 */
	uint16_t busyOffset;
	/* !< [0x07e-0x07f] Busy flag polarity, 0 - busy flag is 1 when
	 * flash device is busy, 1 - busy flag is 0 when flash device is busy
	 */
	uint16_t busyBitPolarity;
#if defined(FSL_FEATURE_SILICON_VERSION_A)
	/* !< [0x080-0x1bf] Lookup table holds Flash command sequences */
	uint32_t lookupTable[80];
	/* !< [0x1c0-0x1ef] Customizable LUT Sequences */
	fc_xspi_lut_seq_t lutCustomSeq[12];
	/* !< [0x1f0-0x1f3] Customizable DLLCRA for SDR setting */
	uint32_t dllCraSdrVal;
	/* !< [0x1f4-0x1f7] Customizable SMPR SDR setting */
	uint32_t smprSdrVal;
	/* !< [0x1f8-0x1fb] Customizable DLLCRA for DDR setting */
	uint32_t dllCraDdrVal;
	/* !< [0x1fc-0x1ff] Customizable SMPR DDR setting */
	uint32_t smprDdrVal;
#else
	/* !< [0x080-0x1e7] B0 Lookup table holds Flash command sequences */
	uint32_t lookupTable[90];
	/* !< [0x1e8-0x217] Customizable LUT Sequences */
	fc_xspi_lut_seq_t lutCustomSeq[12];
	/* !< [0x218-0x21b] Customizable DLLCRA for SDR setting */
	uint32_t dllCraSdrVal;
	/* !< [0x21c-0x21f] Customizable SMPR SDR setting */
	uint32_t smprSdrVal;
	/* !< [0x220-0x223] Customizable DLLCRA for DDR setting */
	uint32_t dllCraDdrVal;
	/* !< [0x224-0x227] Customizable SMPR DDR setting */
	uint32_t smprDdrVal;
#endif
} fc_xspi_mem_config_t;
/*
 *  Serial NOR configuration block
 */
typedef struct _fc_xspi_nor_config {
	/* !< Common memory configuration info via XSPI */
	fc_xspi_mem_config_t memConfig;
	/* !< Page size of Serial NOR */
	uint32_t pageSize;
	/* !< Sector size of Serial NOR */
	uint32_t sectorSize;
	/* !< Clock frequency for IP command */
	uint8_t ipcmdSerialClkFreq;
	/* !< Sector/Block size is the same */
	uint8_t isUniformBlockSize;
	/* !< Data order (D0, D1, D2, D3) is swapped (D1, D0, D3, D2) */
	uint8_t isDataOrderSwapped;
	/* !< Reserved for future use */
	uint8_t reserved0[1];
	/* !< Serial NOR Flash type: 0/1/2/3 */
	uint8_t serialNorType;
	/* !< Need to exit NoCmd mode before other IP command */
	uint8_t needExitNoCmdMode;
	/* !< Half the Serial Clock for non-read command: true/false */
	uint8_t halfClkForNonReadCmd;
	/* !< Need to Restore NoCmd mode after IP commmand execution */
	uint8_t needRestoreNoCmdMode;
	/* !< Block size */
	uint32_t blockSize;
	/* !< Flash State Context */
	uint32_t flashStateCtx;
#if defined(FSL_FEATURE_SILICON_VERSION_A)
	/* !< Reserved for future use */
	uint32_t reserved2[58];
#else
	/* !< Reserved for future use */
	uint32_t reserved2[48];
#endif
} fc_xspi_nor_config_t;

/*
 *  Serial PSRAM configuration block
 */
typedef struct _fc_xspi_psram_config {
	/* !< XMCD header */
	uint32_t xmcdHeader;
	/* !< Simplified XSPI RAM Configuration Option 0 */
	uint32_t xmcdOpt0;
	/* !< Simplified XSPI RAM Configuration Option 1 */
	uint32_t xmcdOpt1;
	/* !< Reserved for future use */
	uint32_t reserved2[189];
} fc_xspi_psram_config_t;

typedef struct {
	/* !< Configure structure for boot device connected
	 * to XSPI0/XSPI1 interface.
	 */
	fc_xspi_nor_config_t xspi_fcb_block;
	/* !< Configure structure for PSRAM device connected
	 * to XSPI0/XSPI1 interface.
	 */
	fc_xspi_psram_config_t psram_config_block;
	/* !< Configure structure for PSRAM device connected to XSPI2 interface.
	 * Only for users' usage, Boot ROM doesn't use this part
	 */
	uint8_t xspi2_fcb_block[768];
	/* !< Reserved for future usage */
	uint8_t reserved[1792];
} fc_static_platform_config_t;
#endif
