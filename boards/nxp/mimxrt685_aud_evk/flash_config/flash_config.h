/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FLASH_CONFIG_
#define FLASH_CONFIG_

#include <stdint.h>
#include <stdbool.h>
#include "fsl_common.h"

/* FLEXSPI memory config block related definitions */
#define FLASH_CONFIG_BLOCK_TAG     (0x42464346)
#define FLASH_CONFIG_BLOCK_VERSION (0x56010400)

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

/*! @brief Data pad used in Read command */
enum {
	SERIAL_FLASH_1_PADS = 1,
	SERIAL_FLASH_2_PADS = 2,
	SERIAL_FLASH_4_PADS = 4,
	SERIAL_FLASH_8_PADS = 8,
};

/*! @brief FLEXSPI clock configuration - In High speed boot mode */
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

/*! @brief FLEXSPI clock configuration - In Normal boot SDR mode */
enum {
	FLEXSPI_SERIAL_CLK_SDR_24MHZ = 1,
	FLEXSPI_SERIAL_CLK_SDR_48MHZ = 2,
};

/*! @brief FLEXSPI clock configuration - In Normal boot DDR mode */
enum {
	FLEXSPI_SERIAL_CLK_DDR_48MHZ = 1,
};

/*! @brief Misc feature bit definitions */
enum {
	/* Bit for Differential clock enable */
	FLEXSPI_MISC_OFFSET_DIFF_CLK_ENABLE = 0,
	/* Bit for Word Addressable enable */
	FLEXSPI_MISC_OFFSET_WORD_ADDRESSABLE_ENABLE = 3,
	/* Bit for Safe Configuration Frequency enable */
	FLEXSPI_MISC_OFFSET_SAFE_CONFIG_FREQ_ENABLE = 4,
	/* Bit for DDR clock configuration indication. */
	FLEXSPI_MISC_OFFSET_DDR_MODE_ENABLE = 6,
};

/*! @brief Flash Configuration Command Type */
enum {
	DEVICE_CONFIG_CMD_TYPE_GENERIC,
	DEVICE_CONFIG_CMD_TYPE_QUAD_ENABLE,
	DEVICE_CONFIG_CMD_TYPE_SPI2XPI,
	DEVICE_CONFIG_CMD_TYPE_XPI2SPI,
	DEVICE_CONFIG_CMD_TYPE_SPI2NOCMD,
	DEVICE_CONFIG_CMD_TYPE_RESET,
};

typedef struct {
	uint8_t time_100ps;  /* !< Data valid time, in terms of 100ps */
	uint8_t delay_cells; /* !< Data valid time, in terms of delay cells */
} flexspi_dll_time_t;

/*! @brief FlexSPI LUT Sequence structure */
typedef struct _lut_sequence {
	uint8_t seq_num; /* !< Sequence Number, valid number: 1-16 */
	uint8_t seq_id;  /* !< Sequence Index, valid number: 0-15 */
	uint16_t reserved;
} flexspi_lut_seq_t;

/*! @brief FlexSPI Memory Configuration Block */
typedef struct flexspi_config {
	uint32_t tag;
	uint32_t version;
	uint32_t reserved0;
	uint8_t read_sample_clk_src;
	uint8_t cs_hold_time;
	uint8_t cs_setup_time;
	uint8_t column_address_width;
	uint8_t device_mode_cfg_enable;
	uint8_t device_mode_type;
	uint16_t wait_time_cfg_commands;
	flexspi_lut_seq_t device_mode_seq;
	uint32_t device_mode_arg;
	uint8_t config_cmd_enable;
	uint8_t config_mode_type[3];
	flexspi_lut_seq_t config_cmd_seqs[3];
	uint32_t reserved1;
	uint32_t config_cmd_args[3];
	uint32_t reserved2;
	uint32_t controller_misc_option;
	uint8_t device_type;
	uint8_t sflash_pad_type;
	uint8_t serial_clk_freq;
	uint8_t lut_custom_seq_enable;
	uint32_t reserved3[2];
	uint32_t sflash_a1_size;
	uint32_t sflash_a2_size;
	uint32_t sflash_b1_size;
	uint32_t sflash_b2_size;
	uint32_t cs_pad_setting_override;
	uint32_t sclk_pad_setting_override;
	uint32_t data_pad_setting_override;
	uint32_t dqs_pad_setting_override;
	uint32_t timeout_in_ms;
	uint32_t command_interval;
	flexspi_dll_time_t data_valid_time[2];
	uint16_t busy_offset;
	uint16_t busy_bit_polarity;
	uint32_t lookup_table[64];
	flexspi_lut_seq_t lut_custom_seq[12];
	uint32_t reserved4[4];
} flexspi_mem_config_t;

typedef struct _flexspi_nor_config {
	flexspi_mem_config_t mem_config;
	uint32_t page_size;
	uint32_t sector_size;
	uint8_t ipcmd_serial_clk_freq;
	uint8_t is_uniform_block_size;
	uint8_t is_data_order_swapped;
	uint8_t reserved0[1];
	uint8_t serial_nor_type;
	uint8_t need_exit_nocmd_mode;
	uint8_t half_clk_for_non_read_cmd;
	uint8_t need_restore_nocmd_mode;
	uint32_t block_size;
	uint32_t flash_state_ctx;
	uint32_t reserve2[10];
} flexspi_nor_config_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif /* FLASH_CONFIG_ */
