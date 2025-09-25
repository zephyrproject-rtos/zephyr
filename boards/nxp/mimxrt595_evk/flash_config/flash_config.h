/*
 * Copyright 2018-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef FLASH_CONFIG_H_
#define FLASH_CONFIG_H_

#include <stdint.h>
#include "fsl_iap.h"

/* FLEXSPI memory config block related definitions */
#define FLEXSPI_CFG_BLK_TAG     (0x42464346UL) /* ascii "FCFB" Big Endian */
#define FLEXSPI_CFG_BLK_VERSION (0x56010400UL) /* V1.4.0 */

/*! @brief FLEXSPI clock configuration - When clock source is PLL */
enum {
	FLEXSPI_SERIAL_CLK_30MHZ = 1,
	FLEXSPI_SERIAL_CLK_50MHZ = 2,
	FLEXSPI_SERIAL_CLK_60MHZ = 3,
	FLEXSPI_SERIAL_CLK_80MHZ = 4,
	FLEXSPI_SERIAL_CLK_100MHZ = 5,
	FLEXSPI_SERIAL_CLK_120MHZ = 6,
	FLEXSPI_SERIAL_CLK_133MHZ = 7,
	FLEXSPI_SERIAL_CLK_166MHZ = 8,
	FLEXSPI_SERIAL_CLK_200MHZ = 9,
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
typedef enum flexspi_read_sample_clk_source {
	FLEXSPI_READ_SAMPLE_CLK_LOOPBACK_INTERNALLY = 0,
	FLEXSPI_READ_SAMPLE_CLK_LOOPBACK_FROM_DQS_PAD = 1,
	FLEXSPI_READ_SAMPLE_CLK_LOOPBACK_FROM_SCK_PAD = 2,
	FLEXSPI_READ_SAMPLE_CLK_EXTERNAL_INPUT_FROM_DQS_PAD = 3,
} flexspi_read_sample_clk_t;

/*! @brief Misc feature bit definitions */
enum {
	/* Bit for Differential clock enable */
	FLEXSPI_MISC_OFFSET_DIFF_CLK_ENABLE = 0,
	/* Bit for Parallel mode enable */
	FLEXSPI_MISC_OFFSET_PARALLEL_ENABLE = 2,
	/* Bit for Word Addressable enable */
	FLEXSPI_MISC_OFFSET_WORD_ADDRESSABLE_ENABLE = 3,
	/* Bit for Safe Configuration Frequency enable */
	FLEXSPI_MISC_OFFSET_SAFE_CONFIG_FREQ_ENABLE = 4,
	/* Bit for Pad setting override enable */
	FLEXSPI_MISC_OFFSET_PAD_SETTING_OVERRIDE_ENABLE = 5,
	/* Bit for DDR clock confiuration indication. */
	FLEXSPI_MISC_OFFSET_DDR_MODE_ENABLE = 6,
	/* Bit for DLLCR settings under all modes */
	FLEXSPI_MISC_OFFSET_USE_VALID_TIME_FOR_ALL_FREQ = 7,
};

#endif /* FLASH_CONFIG_H_ */
