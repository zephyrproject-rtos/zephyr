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

#ifndef FSL_FEATURE_SILICON_VERSION_A
#define FSL_FEATURE_SILICON_VERSION_A (1U)
#endif

/* XSPI memory config block related definitions */
#define FC_XSPI_CFG_BLK_TAG     (0x42464346UL) /* ascii "FCFB" Big Endian */
#define FC_XSPI_CFG_BLK_VERSION (0x56010400UL) /* V1.4.0 */

/*! @brief XSPI clock configuration - When clock source is PLL */
typedef enum fc_xspi_serial_clock_freq {
	fc_xspi_serial_clk_30mhz = 1,
	fc_xspi_serial_clk_50mhz = 2,
	fc_xspi_serial_clk_60mhz = 3,
	fc_xspi_serial_clk_80mhz = 4,
	fc_xspi_serial_clk_100mhz = 5,
	fc_xspi_serial_clk_120mhz = 6,
	fc_xspi_serial_clk_133mhz = 7,
	fc_xspi_serial_clk_166mhz = 8,
	fc_xspi_serial_clk_200mhz = 9,
} fc_xspi_serial_clk_freq_t;

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
typedef enum fc_xspi_read_sample_clk_source {
	fc_xspi_read_sample_clk_loopback_internally = 0,
	fc_xspi_read_sample_clk_loopback_from_dqs_pad = 2,
	fc_xspi_read_sample_clk_external_input_from_dqs_pad = 3,
} fc_xspi_read_sample_clk_t;

/* !@brief Misc feature bit definitions */
enum {
	/* !< Bit for Differential clock enable */
	fc_xspi_misc_offset_diff_clk_enable = 0,
	/* !< Bit for Word Addressable enable */
	fc_xspi_misc_offset_word_addressable_enable = 3,
	/* !< Bit for Safe Configuration Frequency enable */
	fc_xspi_misc_offset_safe_config_freq_enable = 4,
	/* !< Bit for DDR clock confiuration indication. */
	fc_xspi_misc_offset_ddr_mode_enable = 6,
};

typedef struct {
	uint8_t time_100ps;  /* !< Data valid time, in terms of 100ps */
	uint8_t delay_cells; /* !< Data valid time, in terms of delay cells */
} fc_xspi_dll_time_t;

/* !@brief XSPI LUT Sequence structure */
typedef struct _lut_sequence {
	uint8_t seq_num; /* !< Sequence Number, valid number: 1-16 */
	uint8_t seq_id;  /* !< Sequence Index, valid number: 0-15 */
	uint16_t reserved;
} fc_xspi_lut_seq_t;

/* !@brief XSPI Memory Configuration Block */
typedef struct xspi_config {
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
	uint8_t read_sample_clk_src;
	/* !< [0x00d-0x00d] CS hold time, default value: 3 */
	uint8_t cs_hold_time;
	/* !< [0x00e-0x00e] CS setup time, default value: 3 */
	uint8_t cs_setup_time;
	/* !< [0x00f-0x00f] Column Address with, for HyperBus protocol,
	 * it is fixed to 3, others to 0.
	 */
	uint8_t column_address_width;
	/* !< [0x010-0x010] Device Mode Configure enable flag,
	 * 1 - Enable, 0 - Disable
	 */
	uint8_t device_mode_cfg_enable;
	/* !< [0x011-0x011] Specify the configuration command type.
	 * 0: No mode change
	 * 1: Quad enable (switch from SPI to Quad mode)
	 * 2: Spi2Xpi (switch from SPI to DPI, QPI, or OPI mode)
	 * 3: Xpi2Spi (switch from DPI, QPI, or OPI to SPI mode)
	 */
	uint8_t device_mode_type;
	/* !< [0x012-0x013] Wait time for Device mode configuration
	 * command, unit: 100us
	 */
	uint16_t wait_time_cfg_commands;
	/* !< [0x014-0x017] Device mode sequence info
	 * [ 7:0] - Number of required sequences
	 * [15:8] - Sequence index
	 */
	fc_xspi_lut_seq_t device_mode_seq;
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
	fc_xspi_lut_seq_t config_cmd_seqs[3];
	/* !< [0x02c-0x02f] Reserved for future use */
	uint32_t reserved1;
	/* !< [0x030-0x03b] Arguments/Parameters for
	 * device Configuration commands
	 */
	uint32_t config_cmd_args[3];
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
	uint32_t controller_misc_option;
	/* !< [0x044-0x044] Device Type: 1 for Serial NOR flash memory */
	uint8_t device_type;
	/* !< [0x045-0x045] Serial Flash Pad Type:
	 *1 - Single, 2 - Dual, 4 - Quad, 8 - Octal
	 */
	uint8_t sflash_pad_type;
	/* !< [0x046-0x046] Serial Flash Frequencey
	 * 1: 30 mhz
	 * 2: 50 mhz
	 * 3: 60 mhz
	 * 4: 80 mhz
	 * 5: 100 mhz
	 * 6: 120 mhz
	 * 7: 133 mhz
	 * 8: 166 mhz
	 * 9: 200 mhz
	 */
	uint8_t serial_clk_freq;
	/* !< [0x047-0x047] LUT customization Enable, it is required if
	 * the program/erase cannot be done using 1 LUT sequence,
	 * currently, only applicable to HyperFLASH
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
	fc_xspi_dll_time_t data_valid_time[2];
	/* !< [0x07c-0x07d] Busy offset, valid value: 0-31 */
	uint16_t busy_offset;
	/* !< [0x07e-0x07f] Busy flag polarity, 0 - busy flag is 1 when
	 * flash device is busy, 1 - busy flag is 0 when flash device is busy
	 */
	uint16_t busy_bit_polarity;
#if defined(FSL_FEATURE_SILICON_VERSION_A)
	/* !< [0x080-0x1bf] Lookup table holds Flash command sequences */
	uint32_t lookup_table[80];
	/* !< [0x1c0-0x1ef] Customizable LUT Sequences */
	fc_xspi_lut_seq_t lut_custom_seq[12];
	/* !< [0x1f0-0x1f3] Customizable DLLCRA for SDR setting */
	uint32_t dll_cra_sdr_val;
	/* !< [0x1f4-0x1f7] Customizable SMPR SDR setting */
	uint32_t smpr_sdr_val;
	/* !< [0x1f8-0x1fb] Customizable DLLCRA for DDR setting */
	uint32_t dll_cra_ddr_val;
	/* !< [0x1fc-0x1ff] Customizable SMPR DDR setting */
	uint32_t smpr_ddr_val;
#else
	/* !< [0x080-0x1e7] B0 Lookup table holds Flash command sequences */
	uint32_t lookup_table[90];
	/* !< [0x1e8-0x217] Customizable LUT Sequences */
	fc_xspi_lut_seq_t lut_custom_seq[12];
	/* !< [0x218-0x21b] Customizable DLLCRA for SDR setting */
	uint32_t dll_cra_sdr_val;
	/* !< [0x21c-0x21f] Customizable SMPR SDR setting */
	uint32_t smpr_sdr_val;
	/* !< [0x220-0x223] Customizable DLLCRA for DDR setting */
	uint32_t dll_cra_ddr_val;
	/* !< [0x224-0x227] Customizable SMPR DDR setting */
	uint32_t smpr_ddr_val;
#endif
} fc_xspi_mem_config_t;
/*
 *  Serial NOR configuration block
 */
typedef struct _fc_xspi_nor_config {
	/* !< Common memory configuration info via XSPI */
	fc_xspi_mem_config_t mem_config;
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
	uint32_t xmcd_header;
	/* !< Simplified XSPI RAM Configuration Option 0 */
	uint32_t xmcd_opt0;
	/* !< Simplified XSPI RAM Configuration Option 1 */
	uint32_t xmcd_opt1;
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
