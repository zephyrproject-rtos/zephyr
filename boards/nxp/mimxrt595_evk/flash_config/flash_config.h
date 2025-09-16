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

/* FLEXSPI memory config block related definitions */
#define FLEXSPI_CFG_BLK_TAG     (0x42464346UL) /* ascii "FCFB" Big Endian */
#define FLEXSPI_CFG_BLK_VERSION (0x56010400UL) /* V1.4.0 */

/*! @brief FLEXSPI clock configuration - When clock source is PLL */
enum {
	flexspi_serial_clk_30mhz = 1,
	flexspi_serial_clk_50mhz = 2,
	flexspi_serial_clk_60mhz = 3,
	flexspi_serial_clk_80mhz = 4,
	flexspi_serial_clk_100mhz = 5,
	flexspi_serial_clk_120mhz = 6,
	flexspi_serial_clk_133mhz = 7,
	flexspi_serial_clk_166mhz = 8,
	flexspi_serial_clk_200mhz = 9,
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
	flexspi_read_sample_clk_loopback_internally = 0,
	flexspi_read_sample_clk_loopback_from_dqs_pad = 1,
	flexspi_read_sample_clk_loopback_from_sck_pad = 2,
	flexspi_read_sample_clk_external_input_from_dqs_pad = 3,
} flexspi_read_sample_clk_t;

/*! @brief Misc feature bit definitions */
enum {
	/* Bit for Differential clock enable */
	flexspi_misc_offset_diff_clk_enable = 0,
	/* Bit for Parallel mode enable */
	flexspi_misc_offset_parallel_enable = 2,
	/* Bit for Word Addressable enable */
	flexspi_misc_offset_word_addressable_enable = 3,
	/* Bit for Safe Configuration Frequency enable */
	flexspi_misc_offset_safe_config_freq_enable = 4,
	/* Bit for Pad setting override enable */
	flexspi_misc_offset_pad_setting_override_enable = 5,
	/* Bit for DDR clock confiuration indication. */
	flexspi_misc_offset_ddr_mode_enable = 6,
	/* Bit for DLLCR settings under all modes */
	flexspi_misc_offset_use_valid_time_for_all_freq = 7,
};

#endif
