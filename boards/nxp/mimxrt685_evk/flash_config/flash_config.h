/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
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
	/* Bit for DDR clock confiuration indication. */
	FLEXSPI_MISC_OFFSET_DDR_MODE_ENABLE = 6,
};

/*! @brief Flash Configuration Command Type */
enum {
	/* !< Generic command, for example:
	 * configure dummy cycles, drive strength, etc
	 */
	DEVICE_CONFIG_CMD_TYPE_GENERIC,
	/* !< Quad Enable command */
	DEVICE_CONFIG_CMD_TYPE_QUAD_ENABLE,
	/* !< Switch from SPI to DPI/QPI/OPI mode */
	DEVICE_CONFIG_CMD_TYPE_SPI2XPI,
	/* !< Switch from DPI/QPI/OPI to SPI mode */
	DEVICE_CONFIG_CMD_TYPE_XPI2SPI,
	/* !< Switch to 0-4-4/0-8-8 mode */
	DEVICE_CONFIG_CMD_TYPE_SPI2NOCMD,
	/* !< Reset device command */
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
	/* !< [0x00c-0x00c] Read Sample Clock Source, valid value: 0/1/3 */
	uint8_t read_sample_clk_src;
	/* !< [0x00d-0x00d] CS hold time, default value: 3 */
	uint8_t cs_hold_time;
	/* !< [0x00e-0x00e] CS setup time, default value: 3 */
	uint8_t cs_setup_time;
	/* !< [0x00f-0x00f] Column Address with, for HyperBus protocol,
	 * it is fixed to 3, For Serial NAND, need to refer to datasheet
	 */
	uint8_t column_address_width;
	/* !< [0x010-0x010] Device Mode Configure enable flag,
	 * 1 - Enable, 0 - Disable
	 */
	uint8_t device_mode_cfg_enable;
	/* !< [0x011-0x011] Specify the configuration command type:Quad
	 * Enable, DPI/QPI/OPI switch, Generic configuration, etc.
	 */
	uint8_t device_mode_type;
	/* !< [0x012-0x013] Wait time for all configuration commands,
	 * unit: 100us, Used for DPI/QPI/OPI switch or reset command
	 */
	uint16_t wait_time_cfg_commands;
	/* !< [0x014-0x017] Device mode sequence info,
	 * [7:0] - LUT sequence id,
	 * [15:8] - LUt sequence number,
	 * [31:16] Reserved
	 */
	flexspi_lut_seq_t device_mode_seq;
	/* !< [0x018-0x01b] Argument/Parameter for device configuration */
	uint32_t device_mode_arg;
	/* !< [0x01c-0x01c] Configure command Enable Flag,
	 * 1 - Enable, 0 - Disable
	 */
	uint8_t config_cmd_enable;
	/* !< [0x01d-0x01f] Configure Mode Type, similar as deviceModeTpe */
	uint8_t config_mode_type[3];
	/* !< [0x020-0x02b] Sequence info for Device Configuration command,
	 * similar as deviceModeSeq
	 */
	flexspi_lut_seq_t config_cmd_seqs[3];
	/* !< [0x02c-0x02f] Reserved for future use */
	uint32_t reserved1;
	/* !< [0x030-0x03b] Arguments/Parameters
	 * for device Configuration commands
	 */
	uint32_t config_cmd_args[3];
	/* !< [0x03c-0x03f] Reserved for future use */
	uint32_t reserved2;
	/* !< [0x040-0x043] Controller Misc Options, see Misc feature
	 * bit definitions for more details
	 */
	uint32_t controller_misc_option;
	/* !< [0x044-0x044] Device Type:
	 * See Flash Type Definition for more details
	 */
	uint8_t device_type;
	/* !< [0x045-0x045] Serial Flash Pad Type:
	 * 1 - Single, 2 - Dual, 4 - Quad, 8 - Octal
	 */
	uint8_t sflash_pad_type;
	/* !< [0x046-0x046] Serial Flash Frequencey,
	 * device specific definitions,
	 * See System Boot Chapter for more details
	 */
	uint8_t serial_clk_freq;
	/* !< [0x047-0x047] LUT customization Enable, it is required if
	 * the program/erase cannot be done using 1 LUT sequence, currently,
	 * only applicable to HyperFLASH
	 */
	uint8_t lut_custom_seq_enable;
	/* !< [0x048-0x04f] Reserved for future use */
	uint32_t reserved3[2];
	/* !< [0x050-0x053] Size of Flash connected to A1 */
	uint32_t sflash_a1_size;
	/* !< [0x054-0x057] Size of Flash connected to A2 */
	uint32_t sflash_a2_size;
	/* !< [0x058-0x05b] Size of Flash connected to B1 */
	uint32_t sflash_b1_size;
	/* !< [0x05c-0x05f] Size of Flash connected to B2 */
	uint32_t sflash_b2_size;
	/* !< [0x060-0x063] CS pad setting override value */
	uint32_t cs_pad_setting_override;
	/* !< [0x064-0x067] SCK pad setting override value */
	uint32_t sclk_pad_setting_override;
	/* !< [0x068-0x06b] data pad setting override value */
	uint32_t data_pad_setting_override;
	/* !< [0x06c-0x06f] DQS pad setting override value */
	uint32_t dqs_pad_setting_override;
	/* !< [0x070-0x073] Timeout threshold for read status command */
	uint32_t timeout_in_ms;
	/* !< [0x074-0x077] CS deselect interval between two commands */
	uint32_t command_interval;
	/* !< [0x078-0x07b] CLK edge to data valid time for
	 * PORT A and PORT B
	 */
	flexspi_dll_time_t data_valid_time[2];
	/* !< [0x07c-0x07d] Busy offset, valid value: 0-31 */
	uint16_t busy_offset;
	/* !< [0x07e-0x07f] Busy flag polarity, 0 - busy flag is 1 when
	 * flash device is busy, 1 - busy flag is 0 when flash device is busy
	 */
	uint16_t busy_bit_polarity;
	/* !< [0x080-0x17f] Lookup table holds Flash command sequences */
	uint32_t lookup_table[64];
	/* !< [0x180-0x1af] Customizable LUT Sequences */
	flexspi_lut_seq_t lut_custom_seq[12];
	/* !< [0x1b0-0x1bf] Reserved for future use */
	uint32_t reserved4[4];
} flexspi_mem_config_t;

/*
 *  Serial NOR configuration block
 */
typedef struct _flexspi_nor_config {
	/* !< Common memory configuration info via FlexSPI */
	flexspi_mem_config_t mem_config;
	/* !< Page size of Serial NOR */
	uint32_t page_size;
	/* !< Sector size of Serial NOR */
	uint32_t sector_size;
	/* !< Clock frequency for IP command */
	uint8_t ipcmd_serial_clk_freq;
	/* !< Sector/Block size is the same */
	uint8_t is_uniform_block_size;
	/* !< Data order (D0, D1, D2, D3) is swapped (D1, D0, D3, D2) */
	uint8_t is_data_order_swapped;
	/* !< Reserved for future use */
	uint8_t reserved0[1];
	/* !< Serial NOR Flash type: 0/1/2/3 */
	uint8_t serial_nor_type;
	/* !< Need to exit NoCmd mode before other IP command */
	uint8_t need_exit_nocmd_mode;
	/* !< Half the Serial Clock for non-read command: true/false */
	uint8_t half_clk_for_non_read_cmd;
	/* !< Need to Restore NoCmd mode after IP commmand execution */
	uint8_t need_restore_nocmd_mode;
	/* !< Block size */
	uint32_t block_size;
	/* !< Flash State Context */
	uint32_t flash_state_ctx;
	/* !< Reserved for future use */
	uint32_t reserve2[10];
} flexspi_nor_config_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif /* FLASH_CONFIG_ */
