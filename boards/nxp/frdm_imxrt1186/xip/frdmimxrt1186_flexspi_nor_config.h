/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FRDMIMXRT1186_FLEXSPI_NOR_CONFIG__
#define __FRDMIMXRT1186_FLEXSPI_NOR_CONFIG__

#include "fsl_common.h"

#define FLEXSPI_CFG_BLK_TAG     (0x42464346UL) /* ascii "FCFB" Big Endian */
#define FLEXSPI_CFG_BLK_VERSION (0x56010400UL) /* V1.4.0 */

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

#define FLEXSPI_LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1)                      \
	(FLEXSPI_LUT_OPERAND0(op0) | FLEXSPI_LUT_NUM_PADS0(pad0) |            \
	 FLEXSPI_LUT_OPCODE0(cmd0) | FLEXSPI_LUT_OPERAND1(op1) |              \
	 FLEXSPI_LUT_NUM_PADS1(pad1) | FLEXSPI_LUT_OPCODE1(cmd1))

/* FlexSPI Read Sample Clock Source definition */
typedef enum _FlashReadSampleClkSource {
	FLEXSPI_READ_SAMPLE_CLK_LOOPBACK_INTERNALLY = 0,
	FLEXSPI_READ_SAMPLE_CLK_LOOPBACK_FROM_DQS_PAD = 1,
	FLEXSPI_READ_SAMPLE_CLK_REVERSED = 2,
	FLEXSPI_READ_SAMPLE_CLK_FLASH_PROVIDED_DQS = 3,
} flexspi_read_sample_clk_t;

/* Flash Type Definition */
enum {
	FLEXSPI_DEVICE_TYPE_SERIAL_NOR = 1,  /* Flash devices are Serial NOR */
	FLEXSPI_DEVICE_TYPE_SERIAL_NAND = 2, /* Flash devices are Serial NAND */
	FLEXSPI_DEVICE_TYPE_SERIAL_RAM = 3   /* Flash devices are Serial RAM/HyperFLASH */
};

/* Flash Pad Definitions */
enum {
	SERIAL_FLASH_1_PAD = 1,
	SERIAL_FLASH_2_PADS = 2,
	SERIAL_FLASH_4_PADS = 4,
	SERIAL_FLASH_8_PADS = 8,
};

/* Definitions for FlexSPI Serial Clock Frequency */
typedef enum _FlexSpiSerialClockFreq {
	FLEXSPI_SERIAL_CLK_30MHZ = 1,
	FLEXSPI_SERIAL_CLK_50MHZ = 2,
	FLEXSPI_SERIAL_CLK_60MHZ = 3,
	FLEXSPI_SERIAL_CLK_80MHZ = 4,
	FLEXSPI_SERIAL_CLK_100MHZ = 5,
	FLEXSPI_SERIAL_CLK_120MHZ = 6,
	FLEXSPI_SERIAL_CLK_133MHZ = 7,
	FLEXSPI_SERIAL_CLK_166MHZ = 8,
} flexspi_serial_clk_freq_t;

/* Flash Configuration Command Type */
enum {
	DEVICE_CONFIG_CMD_TYPE_GENERIC,     /* Generic command */
	DEVICE_CONFIG_CMD_TYPE_QUAD_ENABLE, /* Quad Enable command */
	DEVICE_CONFIG_CMD_TYPE_SPI_2_XPI,   /* Switch from SPI to DPI/QPI/OPI mode */
	DEVICE_CONFIG_CMD_TYPE_XPI_2_SPI,   /* Switch from DPI/QPI/OPI to SPI mode */
	DEVICE_CONFIG_CMD_TYPE_SPI_2_NO_CMD, /* Switch to 0-4-4/0-8-8 mode */
	DEVICE_CONFIG_CMD_TYPE_RESET,       /* Reset device command */
};

/* FlexSPI LUT Sequence structure */
typedef struct _lut_sequence {
	uint8_t seq_num; /* Sequence Number, valid number: 1-16 */
	uint8_t seq_id;  /* Sequence Index, valid number: 0-15 */
	uint16_t reserved;
} flexspi_lut_seq_t;

/* FlexSPI Memory Configuration Block */
typedef struct _FlexSPIConfig {
	uint32_t tag;                       /* [0x000-0x003] Tag, fixed value 0x42464346UL */
	uint32_t version;                   /* [0x004-0x007] Version */
	uint32_t reserved0;                 /* [0x008-0x00b] Reserved for future use */
	uint8_t read_sample_clk_src;        /* [0x00c-0x00c] Read Sample Clock Source */
	uint8_t cs_hold_time;               /* [0x00d-0x00d] CS hold time, default: 3 */
	uint8_t cs_setup_time;              /* [0x00e-0x00e] CS setup time, default: 3 */
	uint8_t column_address_width;       /* [0x00f-0x00f] Column Address width */
	uint8_t device_mode_cfg_enable;     /* [0x010-0x010] Device Mode Configure enable */
	uint8_t device_mode_type;           /* [0x011-0x011] Device Mode Type */
	uint16_t wait_time_cfg_commands;    /* [0x012-0x013] Wait time for config commands */
	flexspi_lut_seq_t device_mode_seq;  /* [0x014-0x017] Device mode sequence info */
	uint32_t device_mode_arg;           /* [0x018-0x01b] Argument for device config */
	uint8_t config_cmd_enable;          /* [0x01c-0x01c] Configure command Enable */
	uint8_t config_mode_type[3];        /* [0x01d-0x01f] Configure Mode Type */
	flexspi_lut_seq_t config_cmd_seqs[3]; /* [0x020-0x02b] Config command sequences */
	uint32_t reserved1;                 /* [0x02c-0x02f] Reserved */
	uint32_t config_cmd_args[3];        /* [0x030-0x03b] Config command arguments */
	uint32_t reserved2;                 /* [0x03c-0x03f] Reserved */
	uint32_t controller_misc_option;    /* [0x040-0x043] Controller Misc Options */
	uint8_t device_type;                /* [0x044-0x044] Device Type */
	uint8_t sflash_pad_type;            /* [0x045-0x045] Serial Flash Pad Type */
	uint8_t serial_clk_freq;            /* [0x046-0x046] Serial Flash Frequency */
	uint8_t lut_custom_seq_enable;      /* [0x047-0x047] LUT customization Enable */
	uint32_t reserved3[2];              /* [0x048-0x04f] Reserved */
	uint32_t sflash_a1_size;            /* [0x050-0x053] Flash A1 size */
	uint32_t sflash_a2_size;            /* [0x054-0x057] Flash A2 size */
	uint32_t sflash_b1_size;            /* [0x058-0x05b] Flash B1 size */
	uint32_t sflash_b2_size;            /* [0x05c-0x05f] Flash B2 size */
	uint32_t cs_pad_setting_override;   /* [0x060-0x063] CS pad setting override */
	uint32_t sclk_pad_setting_override; /* [0x064-0x067] SCK pad setting override */
	uint32_t data_pad_setting_override; /* [0x068-0x06b] Data pad setting override */
	uint32_t dqs_pad_setting_override;  /* [0x06c-0x06f] DQS pad setting override */
	uint32_t timeout_in_ms;             /* [0x070-0x073] Timeout threshold */
	uint32_t command_interval;          /* [0x074-0x077] CS deselect interval */
	uint16_t data_valid_time[2];        /* [0x078-0x07b] Data valid time */
	uint16_t busy_offset;               /* [0x07c-0x07d] Busy offset */
	uint16_t busy_bit_polarity;         /* [0x07e-0x07f] Busy flag polarity */
	uint32_t lookup_table[64];          /* [0x080-0x17f] Lookup table */
	flexspi_lut_seq_t lut_custom_seq[12]; /* [0x180-0x1af] Customizable LUT Sequences */
	uint32_t reserved4[4];              /* [0x1b0-0x1bf] Reserved */
} flexspi_mem_config_t;

/* Serial NOR configuration block */
typedef struct _flexspi_nor_config {
	flexspi_mem_config_t mem_config; /* Common memory configuration info */
	uint32_t page_size;              /* Page size of Serial NOR */
	uint32_t sector_size;            /* Sector size of Serial NOR */
	uint8_t ipcmd_serial_clk_freq;   /* Clock frequency for IP command */
	uint8_t is_uniform_block_size;   /* Sector/Block size is the same */
	uint8_t is_data_order_swapped;   /* Data order is swapped in OPI DDR mode */
	uint8_t reserved0[5];            /* Reserved */
	uint32_t block_size;             /* Block size */
	uint32_t flash_state_ctx;        /* Flash State Context */
	uint32_t reserve1[10];           /* Reserved */
} flexspi_nor_config_t;

#endif /* __FRDMIMXRT1186_FLEXSPI_NOR_CONFIG__ */
