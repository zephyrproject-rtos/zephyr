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

#define FLEXSPI_CFG_BLK_TAG     (0x42464346UL)
#define FLEXSPI_CFG_BLK_VERSION (0x56010400UL)
#define FLEXSPI_CFG_BLK_SIZE    (512)

#define FLEXSPI_FEATURE_HAS_PARALLEL_MODE 1

#define CMD_INDEX_READ        0
#define CMD_INDEX_READSTATUS  1
#define CMD_INDEX_WRITEENABLE 2
#define CMD_INDEX_WRITE       4

#define CMD_LUT_SEQ_IDX_READ        0
#define CMD_LUT_SEQ_IDX_READSTATUS  1
#define CMD_LUT_SEQ_IDX_WRITEENABLE 3
#define CMD_LUT_SEQ_IDX_WRITE       9

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
#define STOP           0

#define FLEXSPI_1PAD 0
#define FLEXSPI_2PAD 1
#define FLEXSPI_4PAD 2
#define FLEXSPI_8PAD 3

#define FLEXSPI_LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1)                                          \
	(FLEXSPI_LUT_OPERAND0(op0) | FLEXSPI_LUT_NUM_PADS0(pad0) | FLEXSPI_LUT_OPCODE0(cmd0) |     \
	 FLEXSPI_LUT_OPERAND1(op1) | FLEXSPI_LUT_NUM_PADS1(pad1) | FLEXSPI_LUT_OPCODE1(cmd1))

/* For flexspi_mem_config.serial_clk_freq */
#if defined(CONFIG_SOC_MIMXRT1011)
enum {
	flexspi_serial_clk_30mhz = 1,
	flexspi_serial_clk_50mhz = 2,
	flexspi_serial_clk_60mhz = 3,
	flexspi_serial_clk_75mhz = 4,
	flexspi_serial_clk_80mhz = 5,
	flexspi_serial_clk_100mhz = 6,
	flexspi_serial_clk_120mhz = 7,
	flexspi_serial_clk_133mhz = 8,
};
#elif defined(CONFIG_SOC_MIMXRT1015) || defined(CONFIG_SOC_MIMXRT1021) ||                          \
	defined(CONFIG_SOC_MIMXRT1024)
enum {
	flexspi_serial_clk_30mhz = 1,
	flexspi_serial_clk_50mhz = 2,
	flexspi_serial_clk_60mhz = 3,
	flexspi_serial_clk_75mhz = 4,
	flexspi_serial_clk_80mhz = 5,
	flexspi_serial_clk_100mhz = 6,
	flexspi_serial_clk_133mhz = 7,
};
#elif defined(CONFIG_SOC_MIMXRT1052) || defined(CONFIG_SOC_SERIES_IMXRT11XX)
enum {
	flexspi_serial_clk_30mhz = 1,
	flexspi_serial_clk_50mhz = 2,
	flexspi_serial_clk_60mhz = 3,
	flexspi_serial_clk_75mhz = 4,
	flexspi_serial_clk_80mhz = 5,
	flexspi_serial_clk_100mhz = 6,
	flexspi_serial_clk_133mhz = 7,
	flexspi_serial_clk_166mhz = 8,
	flexspi_serial_clk_200mhz = 9,
};
#elif defined(CONFIG_SOC_MIMXRT1062) || defined(CONFIG_SOC_MIMXRT1064)
enum {
	flexspi_serial_clk_30mhz = 1,
	flexspi_serial_clk_50mhz = 2,
	flexspi_serial_clk_60mhz = 3,
	flexspi_serial_clk_75mhz = 4,
	flexspi_serial_clk_80mhz = 5,
	flexspi_serial_clk_100mhz = 6,
	flexspi_serial_clk_120mhz = 7,
	flexspi_serial_clk_133mhz = 8,
	flexspi_serial_clk_166mhz = 9,
};
#else
#error "kFlexSpiSerialClk is not defined for this SoC"
#endif

/* For flexspi_mem_config.controller_misc_option */
enum {
	flexspi_clk_sdr,
	flexspi_clk_ddr,
};

/* For flexspi_mem_config.read_sample_clk_src */
enum {
	flexspi_read_sample_clk_loopback_internally = 0,
	flexspi_read_sample_clk_loopback_from_dqs_pad = 1,
	flexspi_read_sample_clk_loopback_from_sck_pad = 2,
	flexspi_read_sample_clk_external_input_from_dqs_pad = 3,
};

/* For flexspi_mem_config.controller_misc_option */
enum {
	/* !< Bit for Differential clock enable */
	flexspi_misc_offset_diff_clk_enable = 0,
	/* !< Bit for CK2 enable */
	flexspi_misc_offset_ck2_enable = 1,
	/* !< Bit for Parallel mode enable */
	flexspi_misc_offset_parallel_enable = 2,
	/* !< Bit for Word Addressable enable */
	flexspi_misc_offset_word_addressable_enable = 3,
	/* !< Bit for Safe Configuration Frequency enable */
	flexspi_misc_offset_safe_config_freq_enable = 4,
	/* !< Bit for Pad setting override enable */
	flexspi_misc_offset_pad_setting_override_enable = 5,
	/* !< Bit for DDR clock configuration indication. */
	flexspi_misc_offset_ddr_mode_enable = 6,
};

/* For flexspi_mem_config.device_type */
enum {
	/* !< Flash devices are Serial NOR */
	flexspi_device_type_serial_nor = 1,
	/* !< Flash devices are Serial NAND */
	flexspi_device_type_serial_nand = 2,
	/* !< Flash devices are Serial RAM/HyperFLASH */
	flexspi_device_type_serial_ram = 3,
	/* !< Flash device is MCP device, A1 is Serial NOR, A2 is Serial NAND */
	flexspi_device_type_mcp_nor_nand = 0x12,
	/* !< Flash device is MCP device, A1 is Serial NOR, A2 is Serial RAMs */
	flexspi_device_type_mcp_nor_ram = 0x13,
};

/* For flexspi_mem_config.sflash_pad_type */
enum {
	serial_flash_1_pad = 1,
	serial_flash_2_pads = 2,
	serial_flash_4_pads = 4,
	serial_flash_8_pads = 8,
};

enum {
	/* !< Generic command, for example: configure dummy cycles, drive strength, etc */
	device_config_cmd_type_generic,
	/* !< Quad Enable command */
	device_config_cmd_type_quad_enable,
	/* !< Switch from SPI to DPI/QPI/OPI mode */
	device_config_cmd_type_spi2xpi,
	/* !< Switch from DPI/QPI/OPI to SPI mode */
	device_config_cmd_type_xpi2spi,
	/* !< Switch to 0-4-4/0-8-8 mode */
	device_config_cmd_type_spi2nocmd,
	/* !< Reset device command */
	device_config_cmd_type_reset,
};

struct flexspi_lut_seq_t {
	uint8_t seq_num;
	uint8_t seq_id;
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
	uint8_t read_sample_clk_src;
	/* !< [0x00d-0x00d] CS hold time, default value: 3 */
	uint8_t cs_hold_time;
	/* !< [0x00e-0x00e] CS setup time, default value: 3 */
	uint8_t cs_setup_time;
	/* !< [0x00f-0x00f] Column Address with, for HyperBus protocol, it is fixed to 3, For */
	uint8_t column_address_width;
	/* ! Serial NAND, need to refer to datasheet */
	/* !< [0x010-0x010] Device Mode Configure enable flag, 1 - Enable, 0 - Disable */
	uint8_t device_mode_cfg_enable;
	/* !< [0x011-0x011] Specify the configuration command
	 * type:Quad Enable, DPI/QPI/OPI switch,
	 */
	uint8_t device_mode_type;
	/* ! Generic configuration, etc. */
	/* !< [0x012-0x013] Wait time for all configuration commands, unit: 100us, Used for */
	uint16_t wait_time_cfg_commands;
	/* ! DPI/QPI/OPI switch or reset command */
	/* !< [0x014-0x017] Device mode sequence info, [7:0] - LUT sequence id, [15:8] - LUt */
	struct flexspi_lut_seq_t device_mode_seq;
	/* ! sequence number, [31:16] Reserved */
	/* !< [0x018-0x01b] Argument/Parameter for device configuration */
	uint32_t device_mode_arg;
	/* !< [0x01c-0x01c] Configure command Enable Flag, 1 - Enable, 0 - Disable */
	uint8_t config_cmd_enable;
	/* !< [0x01d-0x01f] Configure Mode Type, similar as deviceModeTpe */
	uint8_t config_mode_type[3];
	/* !< [0x020-0x02b] Sequence info for Device Configuration command, similar as
	 * deviceModeSeq
	 */
	struct flexspi_lut_seq_t config_cmd_seqs[3];
	/* !< [0x02c-0x02f] Reserved for future use */
	uint32_t reserved1;
	/* !< [0x030-0x03b] Arguments/Parameters for device Configuration commands */
	uint32_t config_cmd_args[3];
	/* !< [0x03c-0x03f] Reserved for future use */
	uint32_t reserved2;
	/* !< [0x040-0x043] Controller Misc Options, see Misc feature bit definitions for more */
	uint32_t controller_misc_option;
	/* ! details */
	/* !< [0x044-0x044] Device Type:  See Flash Type Definition for more details */
	uint8_t device_type;
	/* !< [0x045-0x045] Serial Flash Pad Type: 1 - Single, 2 - Dual, 4 - Quad, 8 - Octal */
	uint8_t sflash_pad_type;
	/* !< [0x046-0x046] Serial Flash Frequency, device specific definitions, See System Boot */
	uint8_t serial_clk_freq;
	/* ! Chapter for more details */
	/* !< [0x047-0x047] LUT customization Enable, it is required if the program/erase cannot */
	uint8_t lut_custom_seq_enable;
	/* ! be done using 1 LUT sequence, currently, only applicable to HyperFLASH */
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
	/* !< [0x078-0x07b] CLK edge to data valid time for PORT A and PORT B, in terms of 0.1ns */
	uint16_t data_valid_time[2];
	/* !< [0x07c-0x07d] Busy offset, valid value: 0-31 */
	uint16_t busy_offset;
	/* !< [0x07e-0x07f] Busy flag polarity, 0 - busy flag is 1 when flash device is busy, 1 - */
	uint16_t busy_bit_polarity;
	/* ! busy flag is 0 when flash device is busy */
	/* !< [0x080-0x17f] Lookup table holds Flash command sequences */
	uint32_t lookup_table[64];
	/* !< [0x180-0x1af] Customizable LUT Sequences */
	struct flexspi_lut_seq_t lut_custom_seq[12];
	/* !< [0x1b0-0x1bf] Reserved for future use */
	uint32_t reserved4[4];
};

#define NOR_CMD_INDEX_READ        CMD_INDEX_READ
#define NOR_CMD_INDEX_READSTATUS  CMD_INDEX_READSTATUS
#define NOR_CMD_INDEX_WRITEENABLE CMD_INDEX_WRITEENABLE
#define NOR_CMD_INDEX_ERASESECTOR 3
#define NOR_CMD_INDEX_PAGEPROGRAM CMD_INDEX_WRITE
#define NOR_CMD_INDEX_CHIPERASE   5
#define NOR_CMD_INDEX_DUMMY       6
#define NOR_CMD_INDEX_ERASEBLOCK  7

#define NOR_CMD_LUT_SEQ_IDX_READ            CMD_LUT_SEQ_IDX_READ
#define NOR_CMD_LUT_SEQ_IDX_READSTATUS      CMD_LUT_SEQ_IDX_READSTATUS
#define NOR_CMD_LUT_SEQ_IDX_READSTATUS_XPI  2
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE     CMD_LUT_SEQ_IDX_WRITEENABLE
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE_XPI 4
#define NOR_CMD_LUT_SEQ_IDX_ERASESECTOR     5
#define NOR_CMD_LUT_SEQ_IDX_ERASEBLOCK      8
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM     CMD_LUT_SEQ_IDX_WRITE
#define NOR_CMD_LUT_SEQ_IDX_CHIPERASE       11
#define NOR_CMD_LUT_SEQ_IDX_READ_SFDP       13
#define NOR_CMD_LUT_SEQ_IDX_RESTORE_NOCMD   14
#define NOR_CMD_LUT_SEQ_IDX_EXIT_NOCMD      15

struct flexspi_nor_config_t {
	/* !< Common memory configuration info via FlexSPI */
	struct flexspi_mem_config_t mem_config;
	/* !< Page size of Serial NOR */
	uint32_t page_size;
	/* !< Sector size of Serial NOR */
	uint32_t sector_size;
	/* !< Clock frequency for IP command */
	uint8_t ipcmd_serial_clk_freq;
	/* !< Sector/Block size is the same */
	uint8_t is_uniform_block_size;
	/* !< Reserved for future use */
	uint8_t reserved0[2];
	/* !< Serial NOR Flash type: 0/1/2/3 */
	uint8_t serial_nor_type;
	/* !< Need to exit NoCmd mode before other IP command */
	uint8_t need_exit_nocmd_mode;
	/* !< Half the Serial Clock for non-read command: true/false */
	uint8_t half_clk_for_non_read_cmd;
	/* !< Need to Restore NoCmd mode after IP command execution */
	uint8_t need_restore_nocmd_mode;
	/* !< Block size */
	uint32_t block_size;
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
