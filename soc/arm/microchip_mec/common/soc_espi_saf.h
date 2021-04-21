/*
 * Copyright (c) 2020 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Header containing definitions for MCHP eSPI SAF
 */

#ifndef _SOC_ESPI_SAF_H_
#define _SOC_ESPI_SAF_H_

#include <stdint.h>
#include <sys/util.h>
#include <soc.h>

#define MCHP_SAF_MAX_FLASH_DEVICES 2U

/*
 * SAF hardware state machine timings
 * poll timeout is in 32KHz clock periods
 * poll interval is in AHB clock(48MHz) units.
 * suspend resume interval is in 32KHz clock periods.
 * consecutive read timeout is in AHB clock periods.
 * suspend check delay is in AHB clock(48MHz) periods.
 */
#define MCHP_SAF_FLASH_POLL_TIMEOUT 0x28000U
#define MCHP_SAF_FLASH_POLL_INTERVAL 0U
#define MCHP_SAF_FLASH_SUS_RSM_INTERVAL 8U
#define MCHP_SAF_FLASH_CONSEC_READ_TIMEOUT 2U
#define MCHP_SAF_FLASH_SUS_CHK_DELAY 0U

/* Default SAF Map of eSPI TAG numbers to master numbers */
#define MCHP_SAF_TAG_MAP0_DFLT 0x23221100
#define MCHP_SAF_TAG_MAP1_DFLT 0x77677767
#define MCHP_SAF_TAG_MAP2_DFLT 0x00000005

/*
 * Default QMSPI clock divider and chip select timing.
 * QMSPI master clock is 48MHz AHB clock.
 */
#define MCHP_SAF_QMSPI_CLK_DIV 2U
#define MCHP_SAF_QMSPI_CS_TIMING 0x03000101U

/* SAF QMSPI programming */

#define MCHP_SAF_QMSPI_NUM_FLASH_DESCR 6U
#define MCHP_SAF_QMSPI_CS0_START_DESCR 0U
#define MCHP_SAF_QMSPI_CS1_START_DESCR \
	(MCHP_SAF_QMSPI_CS0_START_DESCR + MCHP_SAF_QMSPI_NUM_FLASH_DESCR)

/* SAF engine requires start indices of descriptor chains */
#define MCHP_SAF_CM_EXIT_START_DESCR  12U
#define MCHP_SAF_CM_EXIT_LAST_DESCR   13U
#define MCHP_SAF_POLL_STS_START_DESCR 14U
#define MCHP_SAF_POLL_STS_END_DESCR   15U
#define MCHP_SAF_NUM_GENERIC_DESCR    4U

/* QMSPI descriptors 12-15 for all SPI flash devices */
/* #define SAF_QMSPI_DESCR12 0x0002D40E */

/*
 * QMSPI descriptors 12-13 are exit continuous mode
 */
#define MCHP_SAF_EXIT_CM_DESCR12 \
		(MCHP_QMSPI_C_IFM_4X | MCHP_QMSPI_C_TX_ONES | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_DIS | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_NO_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | \
		 MCHP_QMSPI_C_NEXT_DESCR(13) | \
		 MCHP_QMSPI_C_XFR_NUNITS(1))

#define MCHP_SAF_EXIT_CM_DESCR13 \
		(MCHP_QMSPI_C_IFM_4X | MCHP_QMSPI_C_TX_DIS | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_EN | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | \
		 MCHP_QMSPI_C_NEXT_DESCR(0) | \
		 MCHP_QMSPI_C_XFR_NUNITS(9) | \
		 MCHP_QMSPI_C_DESCR_LAST)

/*
 * QMSPI descriptors 14-15 are poll 16-bit flash status
 * Transmit one byte opcode at 1X (no DMA).
 * Receive two bytes at 1X (no DMA).
 */
#define MCHP_SAF_POLL_DESCR14 \
		(MCHP_QMSPI_C_IFM_1X | MCHP_QMSPI_C_TX_DATA | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_DIS | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_NO_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | \
		 MCHP_QMSPI_C_NEXT_DESCR(15) | \
		 MCHP_QMSPI_C_XFR_NUNITS(1))

#define MCHP_SAF_POLL_DESCR15 \
		(MCHP_QMSPI_C_IFM_1X | MCHP_QMSPI_C_TX_DIS | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_EN | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | \
		 MCHP_QMSPI_C_NEXT_DESCR(0) | \
		 MCHP_QMSPI_C_XFR_NUNITS(2) | \
		 MCHP_QMSPI_C_DESCR_LAST)


/* SAF Pre-fetch optimization mode */
#define MCHP_SAF_PREFETCH_MODE MCHP_SAF_FL_CFG_MISC_PFOE_DFLT

#define MCHP_SAF_CFG_MISC_PREFETCH_EXPEDITED 0x03U

/*
 * SAF Opcode 32-bit register value.
 * Each byte contain a SPI flash 8-bit opcode.
 * NOTE1: opcode value of 0 = flash does not support this operation
 * NOTE2:
 * SAF Opcode A
 *	op0 = SPI flash write-enable opcode
 *	op1 = SPI flash program/erase suspend opcode
 *	op2 = SPI flash program/erase resume opcode
 *	op3 = SPI flash read STATUS1 opcode
 * SAF Opcode B
 *	op0 = SPI flash erase 4KB sector opcode
 *	op1 = SPI flash erase 32KB sector opcode
 *	op2 = SPI flash erase 64KB sector opcode
 *	op3 = SPI flash page program opcode
 * SAF Opcode C
 *	op0 = SPI flash read 1-4-4 continuous mode opcode
 *	op1 = SPI flash op0 mode byte value for non-continuous mode
 *	op2 = SPI flash op0 mode byte value for continuous mode
 *	op3 = SPI flash read STATUS2 opcode
 */
#define MCHP_SAF_OPCODE_REG_VAL(op0, op1, op2, op3) \
	(((uint32_t)(op0)&0xffU) | (((uint32_t)(op1)&0xffU) << 8) | \
	 (((uint32_t)(op2)&0xffU) << 16) | (((uint32_t)(op3)&0xffU) << 24))

/*
 * SAF Flash Config CS0/CS1 QMSPI descriptor indices register value
 * e = First QMSPI descriptor index for enter continuous mode chain
 * r = First QMSPI descriptor index for continuous mode read chain
 * s = Index of QMSPI descriptor in continuous mode read chain that
 *     contains the data length field.
 */
#define MCHP_SAF_CS_CFG_DESCR_IDX_REG_VAL(e, r, s) (((uint32_t)(e)&0xfU) | \
	(((uint32_t)(r)&0xfU) << 8) | (((uint32_t)(s)&0xfU) << 12))

/* W25Q128 SPI flash device connected size in bytes */
#define MCHP_W25Q128_SIZE (16U * 1024U * 1024U)

/*
 * Six QMSPI descriptors describe SPI flash opcode protocols.
 * Example: W25Q128
 */
/* Continuous mode read: transmit-quad 24-bit address and mode byte */
#define MCHP_W25Q128_CM_RD_D0 \
		(MCHP_QMSPI_C_IFM_4X | MCHP_QMSPI_C_TX_DATA | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_DIS | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_NO_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | MCHP_QMSPI_C_XFR_NUNITS(4))

/* Continuous mode read: transmit-quad 4 dummy clocks with I/O tri-stated */
#define MCHP_W25Q128_CM_RD_D1 \
		(MCHP_QMSPI_C_IFM_4X | MCHP_QMSPI_C_TX_DIS | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_DIS | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_NO_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | MCHP_QMSPI_C_XFR_NUNITS(2))

/* Continuous mode read: read N bytes */
#define MCHP_W25Q128_CM_RD_D2 \
		(MCHP_QMSPI_C_IFM_4X | MCHP_QMSPI_C_TX_DIS | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_EN | \
		 MCHP_QMSPI_C_RX_DMA_4B | MCHP_QMSPI_C_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | MCHP_QMSPI_C_XFR_NUNITS(0) | \
		 MCHP_QMSPI_C_DESCR_LAST)

/* Enter Continuous mode: transmit-single CM quad read opcode */
#define MCHP_W25Q128_ENTER_CM_D0 \
		(MCHP_QMSPI_C_IFM_1X | MCHP_QMSPI_C_TX_DATA | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_DIS | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_NO_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | MCHP_QMSPI_C_XFR_NUNITS(1))

/* Enter Continuous mode: transmit-quad 24-bit address and mode byte  */
#define MCHP_W25Q128_ENTER_CM_D1 \
		(MCHP_QMSPI_C_IFM_4X | MCHP_QMSPI_C_TX_DATA | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_DIS | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_NO_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | MCHP_QMSPI_C_XFR_NUNITS(4))

/* Enter Continuous mode: read-quad 3 bytes */
#define MCHP_W25Q128_ENTER_CM_D2 \
		(MCHP_QMSPI_C_IFM_4X | MCHP_QMSPI_C_TX_DIS | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_DIS | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | MCHP_QMSPI_C_XFR_NUNITS(3) | \
		 MCHP_QMSPI_C_DESCR_LAST)

#define MCHP_W25Q128_OPA MCHP_SAF_OPCODE_REG_VAL(0x06U, 0x75U, 0x7aU, 0x05U)
#define MCHP_W25Q128_OPB MCHP_SAF_OPCODE_REG_VAL(0x20U, 0x52U, 0xd8U, 0x02U)
#define MCHP_W25Q128_OPC MCHP_SAF_OPCODE_REG_VAL(0xebU, 0xffU, 0xa5U, 0x35U)

/* W25Q128 STATUS2 bit[7] == 0 part is NOT in suspend state */
#define MCHP_W25Q128_POLL2_MASK 0xff7fU

/*
 * SAF Flash Continuous Mode Prefix register value
 * b[7:0] = continuous mode prefix opcode
 * b[15:8] = continuous mode prefix opcode data
 * Some SPI flash devices require a prefix command before
 * they will enter continuous mode.
 * A zero value means the SPI flash does not require a prefix
 * command.
 */
#define MCHP_W25Q128_CONT_MODE_PREFIX_VAL 0U

#define MCHP_W25Q128_FLAGS 0U


/* W25Q256 SPI flash device connected size in bytes */
#define MCHP_W25Q256_SIZE (32U * 1024U * 1024U)

/*
 * Six QMSPI descriptors describe SPI flash opcode protocols.
 * W25Q256 device.
 */

/* Continuous Mode Read: Transmit-quad opcode plus 32-bit address */
#define MCHP_W25Q256_CM_RD_D0 \
		(MCHP_QMSPI_C_IFM_4X | MCHP_QMSPI_C_TX_DATA | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_DIS | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_NO_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | MCHP_QMSPI_C_XFR_NUNITS(5))

#define MCHP_W25Q256_CM_RD_D1 \
		(MCHP_QMSPI_C_IFM_4X | MCHP_QMSPI_C_TX_DIS | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_DIS | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_NO_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | MCHP_QMSPI_C_XFR_NUNITS(2))

#define MCHP_W25Q256_CM_RD_D2 \
		(MCHP_QMSPI_C_IFM_4X | MCHP_QMSPI_C_TX_DIS | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_EN | \
		 MCHP_QMSPI_C_RX_DMA_4B | MCHP_QMSPI_C_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | MCHP_QMSPI_C_XFR_NUNITS(0) | \
		 MCHP_QMSPI_C_DESCR_LAST)

/* Enter Continuous mode: transmit-single CM quad read opcode */
#define MCHP_W25Q256_ENTER_CM_D0 \
		(MCHP_QMSPI_C_IFM_1X | MCHP_QMSPI_C_TX_DATA | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_DIS | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_NO_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | MCHP_QMSPI_C_XFR_NUNITS(1))

/* Enter Continuous mode: transmit-quad 32-bit address and mode byte  */
#define MCHP_W25Q256_ENTER_CM_D1 \
		(MCHP_QMSPI_C_IFM_4X | MCHP_QMSPI_C_TX_DATA | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_DIS | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_NO_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | MCHP_QMSPI_C_XFR_NUNITS(5))

/* Enter Continuous mode: read-quad 3 bytes */
#define MCHP_W25Q256_ENTER_CM_D2 \
		(MCHP_QMSPI_C_IFM_4X | MCHP_QMSPI_C_TX_DIS | \
		 MCHP_QMSPI_C_TX_DMA_DIS | MCHP_QMSPI_C_RX_DIS | \
		 MCHP_QMSPI_C_RX_DMA_DIS | MCHP_QMSPI_C_CLOSE | \
		 MCHP_QMSPI_C_XFR_UNITS_1 | MCHP_QMSPI_C_XFR_NUNITS(3) | \
		 MCHP_QMSPI_C_DESCR_LAST)

#define MCHP_W25Q256_OPA SAF_OPCODE_REG_VAL(0x06U, 0x75U, 0x7aU, 0x05U)
#define MCHP_W25Q256_OPB SAF_OPCODE_REG_VAL(0x20U, 0x52U, 0xd8U, 0x02U)
#define MCHP_W25Q256_OPC SAF_OPCODE_REG_VAL(0xebU, 0xffU, 0xa5U, 0x35U)

#define MCHP_W25Q256_POLL2_MASK 0xff7fU

#define MCHP_W25Q256_CONT_MODE_PREFIX_VAL 0U

#define MCHP_W25Q256_FLAGS 0U

/* SAF Flash Config CS0 QMSPI descriptor indices */
#define MCHP_CS0_CFG_DESCR_IDX_REG_VAL \
		MCHP_SAF_CS_CFG_DESCR_IDX_REG_VAL(3U, 0U, 2U)

/* SAF Flash Config CS1 QMSPI descriptor indices */
#define MCHP_CS1_CFG_DESCR_IDX_REG_VAL \
		MCHP_SAF_CS_CFG_DESCR_IDX_REG_VAL(9U, 6U, 8U)

#define MCHP_SAF_HW_CFG_FLAG_FREQ 0x01U
#define MCHP_SAF_HW_CFG_FLAG_CSTM 0x02U
#define MCHP_SAF_HW_CFG_FLAG_CPHA 0x04U

/* enable SAF prefetch */
#define MCHP_SAF_HW_CFG_FLAG_PFEN 0x10U
/* Use expedited prefetch instead of default */
#define MCHP_SAF_HW_CFG_FLAG_PFEXP 0x20U

/*
 * Override the default tag map value when this bit is set
 * in a tag_map[].
 */
#define MCHP_SAF_HW_CFG_TAGMAP_USE BIT(31)

struct espi_saf_hw_cfg {
	uint32_t qmspi_freq_hz;
	uint32_t qmspi_cs_timing;
	uint8_t  qmspi_cpha;
	uint8_t  flags;
	uint32_t generic_descr[MCHP_SAF_NUM_GENERIC_DESCR];
	uint32_t tag_map[MCHP_ESPI_SAF_TAGMAP_MAX];
};

/*
 * SAF local flash configuration.
 * SPI flash device size in bytes
 * SPI opcodes for SAF Opcode A register
 * SPI opcodes for SAF Opcode B register
 * SPI opcodes for SAF Opcode C register
 * QMSPI descriptors describing SPI opcode transmit and
 * data read.
 * SAF controller Poll2 Mast value specific for this flash device
 * SAF continuous mode prefix register value for those flashes requireing
 * a prefix byte transmitted before the enter continuous mode command.
 * Start QMSPI descriptor numbers.
 * miscellaneous flags.
 */

/* Flags */
#define MCHP_FLASH_FLAG_ADDR32 BIT(0)

struct espi_saf_flash_cfg {
	uint32_t flashsz;
	uint32_t opa;
	uint32_t opb;
	uint32_t opc;
	uint16_t poll2_mask;
	uint16_t cont_prefix;
	uint16_t cs_cfg_descr_ids;
	uint16_t flags;
	uint32_t descr[MCHP_SAF_QMSPI_NUM_FLASH_DESCR];
};


/*
 * 17 flash protection regions
 * Each region is described by:
 * SPI start address. 20-bits = bits[31:12] of SPI address
 * SPI limit address. 20-bits = bits[31:12] of last SPI address
 * 8-bit bit map of eSPI master write-erase permission
 * 8-bit bit map of eSPI maste read permission
 * eSPI master numbers 0 - 7 correspond to bits 0 - 7.
 *
 * Protection region lock:
 *   One 32-bit register with bits[16:0] -> protection regions 16:0
 *
 * eSPI Host maps threads by a tag number to master numbers.
 * Thread numbers are 4-bit
 * Master numbers are 3-bit
 * Master number    Thread numbers    Description
 *     0                0h, 1h        Host PCH HW init
 *     1                2h, 3h        Host CPU access(HW/BIOS/SMM/SW)
 *     2                4h, 5h        Host PCH ME
 *     3                6h            Host PCH LAN
 *     4                N/A           Not defined/used
 *     5                N/A           EC Firmware portal access
 *     6                9h, Dh        Host PCH IE
 *     7                N/A           Not defined/used
 *
 * NOTE: eSPI SAF specification allows master 0 (Host PCH HW) full
 * access to all protection regions.
 *
 * SAF TAG Map registers 0 - 2 map eSPI TAG values 0h - Fh to
 * the three bit master number. Each 32-bit register contains 3-bit
 * fields aligned on nibble boundaries holding the master number
 * associated with the eSPI tag (thread) number.
 * A master value of 7h in a field indicates a non-existent map entry.
 *
 * bit map of registers to program
 * b[2:0] = TAG Map[2:0]
 * b[20:4] = ProtectionRegions[16:0]
 * bit map of PR's to lock
 * b[20:4] = ProtectionRegions[16:0]
 *
 */
#define MCHP_SAF_PR_FLAG_ENABLE 0x01U
#define MCHP_SAF_PR_FLAG_LOCK 0x02U

#define MCHP_SAF_MSTR_HOST_PCH		0U
#define MCHP_SAF_MSTR_HOST_CPU		1U
#define MCHP_SAF_MSTR_HOST_PCH_ME	2U
#define MCHP_SAF_MSTR_HOST_PCH_LAN	3U
#define MCHP_SAF_MSTR_RSVD4		4U
#define MCHP_SAF_MSTR_EC		5U
#define MCHP_SAF_MSTR_HOST_PCH_IE	6U

struct espi_saf_pr {
	uint32_t start;
	uint32_t size;
	uint8_t  master_bm_we;
	uint8_t  master_bm_rd;
	uint8_t  pr_num;
	uint8_t  flags; /* bit[0]==1 is lock the region */
};

struct espi_saf_protection {
	size_t nregions;
	const struct espi_saf_pr *pregions;
};

#endif /* _SOC_ESPI_SAF_H_ */
