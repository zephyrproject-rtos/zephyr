/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_NPCM_FIU_REGISTERS_H_
#define ZEPHYR_DRIVERS_FLASH_NPCM_FIU_REGISTERS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flash Interface Unit (FIU) device registers
 */
struct fiu_reg {
	volatile uint8_t reserved1;
	/* 0x001: Burst Configuration */
	volatile uint8_t BURST_CFG;
	/* 0x002: FIU Response Configuration */
	volatile uint8_t RESP_CFG;
	volatile uint8_t reserved2[17];
	/* 0x014: SPI Flash Configuration */
	volatile uint8_t SPI_FL_CFG;
	volatile uint8_t reserved3;
	/* 0x016: UMA Code Byte */
	volatile uint8_t UMA_CODE;
	/* 0x017: UMA Address Byte 0 */
	volatile uint8_t UMA_AB0;
	/* 0x018: UMA Address Byte 1 */
	volatile uint8_t UMA_AB1;
	/* 0x019: UMA Address Byte 2 */
	volatile uint8_t UMA_AB2;
	/* 0x01A: UMA Data Byte 0 */
	volatile uint8_t UMA_DB0;
	/* 0x01B: UMA Data Byte 1 */
	volatile uint8_t UMA_DB1;
	/* 0x01C: UMA Data Byte 2 */
	volatile uint8_t UMA_DB2;
	/* 0x01D: UMA Data Byte 3 */
	volatile uint8_t UMA_DB3;
	/* 0x01E: UMA Control and Status */
	volatile uint8_t UMA_CTS;
	/* 0x01F: UMA Extended Control and Status */
	volatile uint8_t UMA_ECTS;
	/* 0x020: UMA Data Bytes 0-3 */
	volatile uint32_t UMA_DB0_3;
	volatile uint8_t reserved4[2];
	/* 0x026: CRC Control Register */
	volatile uint8_t CRCCON;
	/* 0x027: CRC Entry Register */
	volatile uint8_t CRCENT;
	/* 0x028: CRC Initialization and Result Register */
	volatile uint32_t CRCRSLT;
	volatile uint8_t reserved5[2];
	/* 0x02E: FIU Read Command for Back-up flash */
	volatile uint8_t RD_CMD_BACK;
	volatile uint8_t reserved6;
	/* 0x030: FIU Read Command for private flash */
	volatile uint8_t RD_CMD_PVT;
	/* 0x031: FIU Read Command for shared flash */
	volatile uint8_t RD_CMD_SHD;
	volatile uint8_t reserved7;
	/* 0x033: FIU Extended Configuration */
	volatile uint8_t FIU_EXT_CFG;
	/* 0x034: UMA AB0~3 */
	volatile uint32_t UMA_AB0_3;
	volatile uint8_t reserved8[4];
	/* 0x03C: Set command enable in 4 Byte address mode */
	volatile uint8_t SET_CMD_EN;
	/* 0x03D: 4 Byte address mode Enable */
	volatile uint8_t ADDR_4B_EN;
	volatile uint8_t reserved9[3];
	/* 0x041: Master Inactive Counter Threshold */
	volatile uint8_t MI_CNT_THRSH;
	/* 0x042: FIU Master Status */
	volatile uint8_t FIU_MSR_STS;
	/* 0x043: FIU Master Interrupt Enable and Configuration */
	volatile uint8_t FIU_MSR_IE_CFG;
	/* 0x044: Quad Program Enable */
	volatile uint8_t Q_P_EN;
	volatile uint8_t reserved10[3];
	/* 0x048: Extended Data Byte Configuration */
	volatile uint8_t EXT_DB_CFG;
	/* 0x049: Direct Write Configuration */
	volatile uint8_t DIRECT_WR_CFG;
	volatile uint8_t reserved11[6];
	/* 0x050 ~ 0x060: Extended Data Byte F to 0 */
	volatile uint8_t EXT_DB_F_0[16];
};

/*
 * Register bit/field access helpers
 */
#define GET_POS_FIELD(pos, size)  pos
#define GET_SIZE_FIELD(pos, size) size
#define FIELD_POS(field)          GET_POS_##field
#define FIELD_SIZE(field)         GET_SIZE_##field

#define GET_FIELD(reg, field) \
	(((reg) >> FIELD_POS(field)) & ((1 << FIELD_SIZE(field)) - 1))
#define SET_FIELD(reg, field, value) \
	((reg) = ((reg) & (~(((1 << FIELD_SIZE(field)) - 1) << FIELD_POS(field)))) | \
	 ((value) << FIELD_POS(field)))

/* FIU register field definitions (from NPCM400F datasheet) */

/* 0x001: BURST_CFG */
#define NPCM_BURST_CFG_R_BURST			FIELD(0, 2)
#define NPCM_BURST_CFG_SLAVE			2
#define NPCM_BURST_CFG_UNLIM_BURST		3
#define NPCM_BURST_CFG_SPI_WR_EN		7

/* 0x002: RESP_CFG */
#define NPCM_RESP_CFG_QUAD_EN			3

/* 0x014: SPI_FL_CFG */
#define NPCM_SPI_FL_CFG_RD_MODE			FIELD(6, 2)
#define NPCM_SPI_FL_CFG_RD_MODE_NORMAL		0
#define NPCM_SPI_FL_CFG_RD_MODE_FAST		1
#define NPCM_SPI_FL_CFG_RD_MODE_FAST_DUAL	3

/* 0x01E: UMA_CTS */
#define NPCM_UMA_CTS_D_SIZE			FIELD(0, 3)
#define NPCM_UMA_CTS_A_SIZE			3
#define NPCM_UMA_CTS_C_SIZE			4
#define NPCM_UMA_CTS_RD_WR			5
#define NPCM_UMA_CTS_DEV_NUM			6
#define NPCM_UMA_CTS_EXEC_DONE			7

/* 0x01F: UMA_ECTS */
#define NPCM_UMA_ECTS_SW_CS0			0
#define NPCM_UMA_ECTS_SW_CS1			1
#define NPCM_UMA_ECTS_SW_CS2			2
#define NPCM_UMA_ECTS_DEV_NUM_BACK		3
#define NPCM_UMA_ECTS_UMA_ADDR_SIZE		FIELD(4, 3)

/* 0x026: CRCCON */
#define NPCM_CRCCON_CALCEN			0
#define NPCM_CRCCON_CKSMCRC			1
#define NPCM_CRCCON_UMAENT			2

/* 0x033: FIU_EXT_CFG */
#define NPCM_FIU_EXT_CFG_FOUR_BADDR		0

/* 0x03C: SET_CMD_EN */
#define NPCM_SET_CMD_EN_PVT_CMD_EN		4
#define NPCM_SET_CMD_EN_SHD_CMD_EN		5
#define NPCM_SET_CMD_EN_BACK_CMD_EN		6

/* 0x03D: ADDR_4B_EN */
#define NPCM_ADDR_4B_EN_PVT_4B			4
#define NPCM_ADDR_4B_EN_SHD_4B			5
#define NPCM_ADDR_4B_EN_BACK_4B			6

/* 0x042: FIU_MSR_STS */
#define NPCM_FIU_MSR_STS_RD_PEND_UMA		0
#define NPCM_FIU_MSR_STS_RD_PEND_FIU		1
#define NPCM_FIU_MSR_STS_MSTR_INACT		2

/* 0x043: FIU_MSR_IE_CFG */
#define NPCM_FIU_MSR_IE_CFG_RD_PEND_UMA_IE	0
#define NPCM_FIU_MSR_IE_CFG_RD_PEND_FIU_IE	1
#define NPCM_FIU_MSR_IE_CFG_MSTR_INACT_IE	2
#define NPCM_FIU_MSR_IE_CFG_UMA_BLOCK		3

/* 0x044: Q_P_EN */
#define NPCM_Q_P_EN_QUAD_P_EN			0
#define NPCM_Q_P_EN_UMA_QUADREAD		4
#define NPCM_Q_P_EN_ADDR_QUAD_EN		6
#define NPCM_Q_P_EN_CMD_QUAD_EN			7

/* 0x048: EXT_DB_CFG */
#define NPCM_EXT_DB_CFG_D_SIZE_DB		FIELD(0, 5)
#define NPCM_EXT_DB_CFG_EXT_DB_EN		5

/* 0x049: DIRECT_WR_CFG */
#define NPCM_DIRECT_WR_CFG_DIRECT_WR_BLOCK	1
#define NPCM_DIRECT_WR_CFG_DW_CS2		5
#define NPCM_DIRECT_WR_CFG_DW_CS1		6
#define NPCM_DIRECT_WR_CFG_DW_CS0		7

/* Burst read selections */
#define NPCM_BURST_CFG_R_BURST_1B		0
#define NPCM_BURST_CFG_R_BURST_16B		3

/* Address and data sizes */
#define NPCM_FIU_ADDR_3B_LENGTH			0x3
#define NPCM_FIU_ADDR_4B_LENGTH			0x4
#define NPCM_FIU_EXT_DB_SIZE			0x10

/* UMA field selections */
#define UMA_FLD_ADDR     BIT(NPCM_UMA_CTS_A_SIZE)
#define UMA_FLD_NO_CMD   BIT(NPCM_UMA_CTS_C_SIZE)
#define UMA_FLD_WRITE    BIT(NPCM_UMA_CTS_RD_WR)
#define UMA_FLD_SHD_SL   BIT(NPCM_UMA_CTS_DEV_NUM)
#define UMA_FLD_EXEC     BIT(NPCM_UMA_CTS_EXEC_DONE)

#define UMA_FIELD_ADDR_0 0x00
#define UMA_FIELD_ADDR_1 0x01
#define UMA_FIELD_ADDR_2 0x02
#define UMA_FIELD_ADDR_3 0x03
#define UMA_FIELD_ADDR_4 0x04

#define UMA_FIELD_DATA_0 0x00
#define UMA_FIELD_DATA_1 0x01
#define UMA_FIELD_DATA_2 0x02
#define UMA_FIELD_DATA_3 0x03
#define UMA_FIELD_DATA_4 0x04

/* UMA transaction codes */
#define UMA_CODE_ONLY_WRITE           (UMA_FLD_EXEC | UMA_FLD_WRITE)
#define UMA_CODE_ONLY_READ_BYTE(n)    (UMA_FLD_EXEC | UMA_FLD_NO_CMD | UMA_FIELD_DATA_##n)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_FLASH_NPCM_FIU_REGISTERS_H_ */
