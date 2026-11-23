/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_BOARDS_NXP_MIMXRT2660_EVK_XIP_XSPI_NOR_CONFIG_H_
#define ZEPHYR_BOARDS_NXP_MIMXRT2660_EVK_XIP_XSPI_NOR_CONFIG_H_

#include <stdint.h>

/*
 * i.MX RT266x XSPI NOR flash configuration block (FCB).
 *
 * The boot ROM reads this block from the start of the XSPI0 NOR flash to learn
 * how to fetch the boot image over XSPI. The layout mirrors the ROM's own
 * interface, so the field offsets below are fixed by the ROM API and must not
 * be reordered.
 *
 * Ported from the MCUXpresso SDK
 * examples/_boards/mimxrt2660evk/xip/mimxrt2660evk_xspi_nor_config.h.
 * Only the definitions this board actually uses are carried over.
 */

#define XSPI_CFG_BLK_TAG     (0x42464346UL) /* "FCFB" */
#define XSPI_CFG_BLK_VERSION (0x56010400UL) /* V1.4.0 */

/* LUT sequence indices (5 LUT words per sequence). */
#define CMD_LUT_SEQ_IDX_READ        0
#define CMD_LUT_SEQ_IDX_READSTATUS  1
#define CMD_LUT_SEQ_IDX_WRITEENABLE 3
#define CMD_LUT_SEQ_IDX_ERASESECTOR 5
#define CMD_LUT_SEQ_IDX_ERASEBLOCK  8
#define CMD_LUT_SEQ_IDX_WRITE       9
#define CMD_LUT_SEQ_IDX_CHIPERASE   11

/* LUT instruction opcodes. */
#define STOP  0x00
#define CMD   0x01
#define ADDR  0x02
#define DUMMY 0x03
#define READ  0x07
#define WRITE 0x08

/* LUT pad counts. */
#define XSPI_1PAD 0
#define XSPI_4PAD 2

#define XSPI_LUT_OPERAND0(op) (((uint32_t)(op) & 0xFFU) << 0)
#define XSPI_LUT_NUM_PADS0(p) (((uint32_t)(p) & 0x3U) << 8)
#define XSPI_LUT_OPCODE0(c)   (((uint32_t)(c) & 0x3FU) << 10)
#define XSPI_LUT_OPERAND1(op) (((uint32_t)(op) & 0xFFU) << 16)
#define XSPI_LUT_NUM_PADS1(p) (((uint32_t)(p) & 0x3U) << 24)
#define XSPI_LUT_OPCODE1(c)   (((uint32_t)(c) & 0x3FU) << 26)

#define XSPI_LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1)                                             \
	(XSPI_LUT_OPERAND0(op0) | XSPI_LUT_NUM_PADS0(pad0) | XSPI_LUT_OPCODE0(cmd0) |              \
	 XSPI_LUT_OPERAND1(op1) | XSPI_LUT_NUM_PADS1(pad1) | XSPI_LUT_OPCODE1(cmd1))

enum {
	kxSpiSerialClk_120MHz = 6,
};

enum {
	kxSpiReadSampleClk_LoopbackFromDqsPad = 1,
};

enum {
	kxSpiMiscOffset_SafeConfigFreqEnable = 4,
};

enum {
	kxSpiDeviceType_SerialNOR = 1,
};

enum {
	kSerialFlash_4Pads = 4,
};

/* !@brief XSPI LUT sequence structure. */
struct xspi_lut_seq {
	uint8_t seq_num;
	uint8_t seq_id;
	uint16_t reserved;
};

/* !@brief XSPI memory configuration block (offsets fixed by the boot ROM). */
struct xspi_mem_config {
	uint32_t tag;                           /* [0x000] = XSPI_CFG_BLK_TAG */
	uint32_t version;                       /* [0x004] */
	uint32_t reserved0;                     /* [0x008] */
	uint8_t read_sample_clk_src;            /* [0x00c] */
	uint8_t cs_hold_time;                   /* [0x00d] */
	uint8_t cs_setup_time;                  /* [0x00e] */
	uint8_t column_address_width;           /* [0x00f] */
	uint8_t device_mode_cfg_enable;         /* [0x010] */
	uint8_t device_mode_type;               /* [0x011] */
	uint16_t wait_time_cfg_commands;        /* [0x012] */
	struct xspi_lut_seq device_mode_seq;    /* [0x014] */
	uint32_t device_mode_arg;               /* [0x018] */
	uint8_t config_cmd_enable;              /* [0x01c] */
	uint8_t reserved1[3];                   /* [0x01d] */
	struct xspi_lut_seq config_cmd_seqs[3]; /* [0x020] */
	uint8_t reserved2[2];                   /* [0x02c] */
	uint8_t max_cs_low_interval;            /* [0x02e] */
	uint8_t ahb_alignment;                  /* [0x02f] */
	uint32_t cfg_cmd_args[3];               /* [0x030] */
	uint8_t ahb_split_en;                   /* [0x03c] */
	uint8_t reserved3[3];                   /* [0x03d] */
	uint32_t controller_misc_option;        /* [0x040] */
	uint8_t device_type;                    /* [0x044] */
	uint8_t sflash_pad_type;                /* [0x045] */
	uint8_t serial_clk_freq;                /* [0x046] */
	uint8_t lut_custom_seq_enable;          /* [0x047] */
	uint32_t reserved4[2];                  /* [0x048] */
	uint32_t sflash_a1_size;                /* [0x050] */
	uint32_t sflash_a2_size;                /* [0x054] */
	uint32_t sflash_b1_size;                /* [0x058] */
	uint32_t sflash_b2_size;                /* [0x05c] */
	uint32_t cs_pad_setting_override;       /* [0x060] */
	uint32_t sclk_pad_setting_override;     /* [0x064] */
	uint32_t data_pad_setting_override;     /* [0x068] */
	uint32_t dqs_pad_setting_override;      /* [0x06c] */
	uint32_t timeout_in_ms;                 /* [0x070] */
	uint32_t command_interval;              /* [0x074] */
	uint16_t data_valid_time[2];            /* [0x078] */
	uint16_t busy_offset;                   /* [0x07c] */
	uint16_t busy_bit_polarity;             /* [0x07e] */
	uint32_t lookup_table[90];              /* [0x080-0x1e7] */
	struct xspi_lut_seq lut_custom_seq[12]; /* [0x1e8-0x217] */
	uint32_t dll_cr_val;                    /* [0x218] */
	uint32_t smpr_val;                      /* [0x21c] */
	uint32_t reserved5[2];                  /* [0x220-0x227] */
};

/* !@brief Serial NOR configuration block (offsets fixed by the boot ROM). */
struct xspi_nor_config {
	struct xspi_mem_config mem_config; /* [0x000-0x227] */
	uint32_t page_size;                /* [0x228] */
	uint32_t sector_size;              /* [0x22c] */
	uint8_t ipcmd_serial_clk_freq;     /* [0x230] */
	uint8_t is_uniform_block_size;     /* [0x231] */
	uint8_t is_data_order_swapped;     /* [0x232] */
	uint8_t reserved0;                 /* [0x233] */
	uint8_t serial_nor_type;           /* [0x234] */
	uint8_t need_exit_nocmd_mode;      /* [0x235] */
	uint8_t half_clk_for_non_read_cmd; /* [0x236] */
	uint8_t need_restore_nocmd_mode;   /* [0x237] */
	uint32_t block_size;               /* [0x238] */
	uint32_t flash_state_ctx;          /* [0x23c] */
	uint32_t reserved1[10];            /* [0x240-0x267] */
};

#endif /* ZEPHYR_BOARDS_NXP_MIMXRT2660_EVK_XIP_XSPI_NOR_CONFIG_H_ */
