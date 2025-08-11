/*
 * Copyright 2018-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef __FLASH_CONFIG_H__
#define __FLASH_CONFIG_H__
#include <stdint.h>
#include "fsl_iap.h"

/*! @name Driver version */
/*@{*/
/*! @brief FLASH_CONFIG driver version 2.0.0. */
#define FSL_FLASH_CONFIG_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/* FLEXSPI memory config block related definitions */
#define FLEXSPI_CFG_BLK_TAG     (0x42464346UL) /* ascii "FCFB" Big Endian */
#define FLEXSPI_CFG_BLK_VERSION (0x56010400UL) /* V1.4.0 */

/*! @brief FLEXSPI clock configuration - When clock source is PLL */
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

/*! @brief LUT instructions supported by FLEXSPI */
#define CMD_SDR        0x01
#define CMD_DDR        0x21
#define RADDR_SDR      0x02
#define RADDR_DDR      0x22
#define CADDR_SDR      0x03
#define CADDR_DDR      0x23
#define MODE1_SDR      0x04
#define MODE1_DDR      0x24
#define MODE2_SDR      0x05
#define MODE2_DDR      0x25
#define MODE4_SDR      0x06
#define MODE4_DDR      0x26
#define MODE8_SDR      0x07
#define MODE8_DDR      0x27
#define WRITE_SDR      0x08
#define WRITE_DDR      0x28
#define READ_SDR       0x09
#define READ_DDR       0x29
#define LEARN_SDR      0x0A
#define LEARN_DDR      0x2A
#define DATSZ_SDR      0x0B
#define DATSZ_DDR      0x2B
#define DUMMY_SDR      0x0C
#define DUMMY_DDR      0x2C
#define DUMMY_RWDS_SDR 0x0D
#define DUMMY_RWDS_DDR 0x2D
#define JMP_ON_CS      0x1F
#define STOP_EXE       0

#define FLEXSPI_1PAD 0
#define FLEXSPI_2PAD 1
#define FLEXSPI_4PAD 2
#define FLEXSPI_8PAD 3

#define FLEXSPI_LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1)		\
	(FLEXSPI_LUT_OPERAND0(op0) | FLEXSPI_LUT_NUM_PADS0(pad0) |	\
	FLEXSPI_LUT_OPCODE0(cmd0) | FLEXSPI_LUT_OPERAND1(op1) |		\
	FLEXSPI_LUT_NUM_PADS1(pad1) | FLEXSPI_LUT_OPCODE1(cmd1))

/*! @brief FlexSPI Read Sample Clock Source definition */
typedef enum _FlashReadSampleClkSource {
	kFlexSPIReadSampleClk_LoopbackInternally = 0,
	kFlexSPIReadSampleClk_LoopbackFromDqsPad = 1,
	kFlexSPIReadSampleClk_LoopbackFromSckPad = 2,
	kFlexSPIReadSampleClk_ExternalInputFromDqsPad = 3,
} flexspi_read_sample_clk_t;

/*! @brief Misc feature bit definitions */
enum {
	/* Bit for Differential clock enable */
	kFlexSpiMiscOffset_DiffClkEnable = 0,
	/* Bit for Parallel mode enable */
	kFlexSpiMiscOffset_ParallelEnable = 2,
	/* Bit for Word Addressable enable */
	kFlexSpiMiscOffset_WordAddressableEnable = 3,
	/* Bit for Safe Configuration Frequency enable */
	kFlexSpiMiscOffset_SafeConfigFreqEnable = 4,
	/* Bit for Pad setting override enable */
	kFlexSpiMiscOffset_PadSettingOverrideEnable = 5,
	/* Bit for DDR clock confiuration indication. */
	kFlexSpiMiscOffset_DdrModeEnable = 6,
	/* Bit for DLLCR settings under all modes */
	kFlexSpiMiscOffset_UseValidTimeForAllFreq = 7,
};

#endif
