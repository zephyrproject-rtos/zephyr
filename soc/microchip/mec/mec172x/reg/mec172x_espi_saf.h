/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC172X_ESPI_SAF_H_
#define _MEC172X_ESPI_SAF_H_

#include <stdint.h>
#include <stddef.h>

#define MCHP_ESPI_SAF_BASE_ADDR		0x40008000u
#define MCHP_ESPI_SAF_COMM_BASE_ADDR	0x40071000u

/* SAF hardware supports up to 2 external SPI flash devices */
#define MCHP_ESPI_SAF_CS_MAX		2

/* Three TAG Map registers */
#define MCHP_ESPI_SAF_TAGMAP_MAX	3
/* 17 protection regions */
#define MCHP_ESPI_SAF_PR_MAX		17

#define MCHP_SAF_FL_CM_PRF_CS0_OFS	0x1b0u
#define MCHP_SAF_FL_CM_PRF_CS1_OFS	0x1b2u

#define MCHP_ESPI_SAF_BASE		0x40008000u
#define MCHP_ESPI_SAF_COMM_BASE		0x40071000u
#define MCHP_ESPI_SAF_COMM_MODE_OFS	0x2b8u
#define MCHP_ESPI_SAF_COMM_MODE_ADDR	(MCHP_ESPI_SAF_COMM_BASE_ADDR +	\
					 MCHP_ESPI_SAF_COMM_MODE_OFS)

/* SAF Protection region described by 4 32-bit registers. 17 regions */
#define MCHP_ESPI_SAF_PROT_MAX		17u

/* Register bit definitions */

/* SAF EC Portal Command register */
#define MCHP_SAF_ECP_CMD_OFS			0x18u
#define MCHP_SAF_ECP_CMD_MASK			0xff00ffffu
#define MCHP_SAF_ECP_CMD_PUT_POS		0
#define MCHP_SAF_ECP_CMD_PUT_MASK		0xffu
#define MCHP_SAF_ECP_CMD_PUT_FLASH_NP		0x0au
#define MCHP_SAF_ECP_CMD_CTYPE_POS		8
#define MCHP_SAF_ECP_CMD_CTYPE_MSK0		0xffu
#define MCHP_SAF_ECP_CMD_CTYPE_MASK		0xff00u
#define MCHP_SAF_ECP_CMD_CTYPE_READ		0x0000u
#define MCHP_SAF_ECP_CMD_CTYPE_WRITE		0x0100u
#define MCHP_SAF_ECP_CMD_CTYPE_ERASE		0x0200u
#define MCHP_SAF_ECP_CMD_CTYPE_RPMC_OP1_CS0	0x0300u
#define MCHP_SAF_ECP_CMD_CTYPE_RPMC_OP2_CS0	0x0400u
#define MCHP_SAF_ECP_CMD_CTYPE_RPMC_OP1_CS1	0x8300u
#define MCHP_SAF_ECP_CMD_CTYPE_RPMC_OP2_CS1	0x8400u
#define MCHP_SAF_ECP_CMD_LEN_POS		24
#define MCHP_SAF_ECP_CMD_LEN_MASK0		0xffu
#define MCHP_SAF_ECP_CMD_LEN_MASK		0xff000000ul
/* Read/Write request size (1 <= reqlen <= 64) bytes */
#define MCHP_SAF_ECP_CMD_RW_LEN_MIN		1u
#define MCHP_SAF_ECP_CMD_RW_LEN_MAX		64u
/* Only three erase sizes are supported encoded as */
#define MCHP_SAF_ECP_CMD_ERASE_4K		0u
#define MCHP_SAF_ECP_CMD_ERASE_32K		BIT(24)
#define MCHP_SAF_ECP_CMD_ERASE_64K		BIT(25)

/* Zero based command values */
#define MCHP_SAF_ECP_CMD_READ			0x00u
#define MCHP_SAF_ECP_CMD_WRITE			0x01u
#define MCHP_SAF_ECP_CMD_ERASE			0x02u
#define MCHP_SAF_ECP_CMD_RPMC_OP1_CS0		0x03u
#define MCHP_SAF_ECP_CMD_RPMC_OP2_CS0		0x04u
#define MCHP_SAF_ECP_CMD_RPMC_OP1_CS1		0x83u
#define MCHP_SAF_ECP_CMD_RPMC_OP2_CS1		0x84u

/* SAF EC Portal Flash Address register */
#define MCHP_SAF_ECP_FLAR_OFS		0x1cu
#define MCHP_SAF_ECP_FLAR_MASK		0xffffffffu

/* SAF EC Portal Start register */
#define MCHP_SAF_ECP_START_OFS		0x20u
#define MCHP_SAF_ECP_START_MASK		0x01u
#define MCHP_SAF_ECP_START_POS		0
#define MCHP_SAF_ECP_START		BIT(0)

/* SAF EC Portal Buffer Address register */
#define MCHP_SAF_ECP_BFAR_OFS		0x24u
#define MCHP_SAF_ECP_BFAR_MASK		0xfffffffcu

/* SAF EC Portal Status register */
#define MCHP_SAF_ECP_STS_OFS		0x28u
#define MCHP_SAF_ECP_STS_MASK		0x1ffu
#define MCHP_SAF_ECP_STS_ERR_MASK	0x1fcu
#define MCHP_SAF_ECP_STS_DONE_POS	0
#define MCHP_SAF_ECP_STS_DONE_TST_POS	1
#define MCHP_SAF_ECP_STS_TMOUT_POS	2
#define MCHP_SAF_ECP_STS_OOR_POS	3
#define MCHP_SAF_ECP_STS_AV_POS		4
#define MCHP_SAF_ECP_STS_BND_4K_POS	5
#define MCHP_SAF_ECP_STS_ERSZ_POS	6
#define MCHP_SAF_ECP_STS_ST_OVFL_POS	7
#define MCHP_SAF_ECP_STS_BAD_REQ_POS	8
#define MCHP_SAF_ECP_STS_DONE		BIT(0)
#define MCHP_SAF_ECP_STS_DONE_TST	BIT(1)
#define MCHP_SAF_ECP_STS_TMOUT		BIT(2)
#define MCHP_SAF_ECP_STS_OOR		BIT(3)
#define MCHP_SAF_ECP_STS_AV		BIT(4)
#define MCHP_SAF_ECP_STS_BND_4K		BIT(5)
#define MCHP_SAF_ECP_STS_ERSZ		BIT(6)
#define MCHP_SAF_ECP_STS_ST_OVFL	BIT(7)
#define MCHP_SAF_ECP_STS_BAD_REQ	BIT(8)

/* SAF EC Portal Interrupt Enable register */
#define MCHP_SAF_ECP_INTEN_OFS		0x2cu
#define MCHP_SAF_ECP_INTEN_MASK		0x01u
#define MCHP_SAF_ECP_INTEN_DONE_POS	0
#define MCHP_SAF_ECP_INTEN_DONE		BIT(0)

/* SAF Flash Configuration Size Limit register */
#define MCHP_SAF_FL_CFG_SIZE_LIM_OFS	0x30u
#define MCHP_SAF_FL_CFG_SIZE_LIM_MASK	0xffffffffu

/* SAF Flash Configuration Threshold register */
#define MCHP_SAF_FL_CFG_THRH_OFS	0x34u
#define MCHP_SAF_FL_CFG_THRH_MASK	0xffffffffu

/* SAF Flash Configuration Miscellaneous register */
#define MCHP_SAF_FL_CFG_MISC_OFS		0x38u
#define MCHP_SAF_FL_CFG_MISC_MASK		0x000030f3u
#define MCHP_SAF_FL_CFG_MISC_PFOE_MASK		0x3u
#define MCHP_SAF_FL_CFG_MISC_PFOE_DFLT		0u
#define MCHP_SAF_FL_CFG_MISC_PFOE_EXP		0x3u
#define MCHP_SAF_FL_CFG_MISC_CS0_4BM_POS	4
#define MCHP_SAF_FL_CFG_MISC_CS1_4BM_POS	5
#define MCHP_SAF_FL_CFG_MISC_CS0_CPE_POS	6
#define MCHP_SAF_FL_CFG_MISC_CS1_CPE_POS	7
#define MCHP_SAF_FL_CFG_MISC_SAF_EN_POS		12
#define MCHP_SAF_FL_CFG_MISC_SAF_LOCK_POS	13
#define MCHP_SAF_FL_CFG_MISC_CS0_4BM		BIT(4)
#define MCHP_SAF_FL_CFG_MISC_CS1_4BM		BIT(5)
#define MCHP_SAF_FL_CFG_MISC_CS0_CPE		BIT(6)
#define MCHP_SAF_FL_CFG_MISC_CS1_CPE		BIT(7)
#define MCHP_SAF_FL_CFG_MISC_SAF_EN		BIT(12)
#define MCHP_SAF_FL_CFG_MISC_SAF_LOCK		BIT(13)

/* SAF eSPI Monitor Status and Interrupt enable registers */
#define MCHP_SAF_ESPI_MON_STATUS_OFS		0x3cu
#define MCHP_SAF_ESPI_MON_INTEN_OFS		0x40u
#define MCHP_SAF_ESPI_MON_STS_IEN_MSK		0x1fu
#define MCHP_SAF_ESPI_MON_STS_IEN_TMOUT_POS	0
#define MCHP_SAF_ESPI_MON_STS_IEN_OOR_POS	1
#define MCHP_SAF_ESPI_MON_STS_IEN_AV_POS	2
#define MCHP_SAF_ESPI_MON_STS_IEN_BND_4K_POS	3
#define MCHP_SAF_ESPI_MON_STS_IEN_ERSZ_POS	4
#define MCHP_SAF_ESPI_MON_STS_IEN_TMOUT		BIT(0)
#define MCHP_SAF_ESPI_MON_STS_IEN_OOR		BIT(1)
#define MCHP_SAF_ESPI_MON_STS_IEN_AV		BIT(2)
#define MCHP_SAF_ESPI_MON_STS_IEN_BND_4K	BIT(3)
#define MCHP_SAF_ESPI_MON_STS_IEN_ERSZ		BIT(4)

/* SAF EC Portal Busy register */
#define MCHP_SAF_ECP_BUSY_OFS		0x44u
#define MCHP_SAF_ECP_BUSY_MASK		0x01u
#define MCHP_SAF_ECP_EC0_BUSY_POS	0
#define MCHP_SAF_ECP_EC1_BUSY_POS	1
#define MCHP_SAF_ECP_EC0_BUSY		BIT(0)
#define MCHP_SAF_ECP_EC1_BUSY		BIT(1)

/* SAF CS0/CS1 Opcode A registers */
#define MCHP_SAF_CS0_OPA_OFS		0x4cu
#define MCHP_SAF_CS1_OPA_OFS		0x5cu
#define MCHP_SAF_CS_OPA_MASK		0xffffffffu
#define MCHP_SAF_CS_OPA_WE_POS		0
#define MCHP_SAF_CS_OPA_WE_MASK		0xfful
#define MCHP_SAF_CS_OPA_SUS_POS		8
#define MCHP_SAF_CS_OPA_SUS_MASK	0xff00ul
#define MCHP_SAF_CS_OPA_RSM_POS		16
#define MCHP_SAF_CS_OPA_RSM_MASK	0xff0000ul
#define MCHP_SAF_CS_OPA_POLL1_POS	24
#define MCHP_SAF_CS_OPA_POLL1_MASK	0xff000000ul

/* SAF CS0/CS1 Opcode B registers */
#define MCHP_SAF_CS0_OPB_OFS		0x50u
#define MCHP_SAF_CS1_OPB_OFS		0x60u
#define MCHP_SAF_CS_OPB_OFS		0xffffffffu
#define MCHP_SAF_CS_OPB_ER0_POS		0
#define MCHP_SAF_CS_OPB_ER0_MASK	0xffu
#define MCHP_SAF_CS_OPB_ER1_POS		8
#define MCHP_SAF_CS_OPB_ER1_MASK	0xff00ul
#define MCHP_SAF_CS_OPB_ER2_POS		16
#define MCHP_SAF_CS_OPB_ER2_MASK	0xff0000ul
#define MCHP_SAF_CS_OPB_PGM_POS		24
#define MCHP_SAF_CS_OPB_PGM_MASK	0xff000000ul

/* SAF CS0/CS1 Opcode C registers */
#define MCHP_SAF_CS0_OPC_OFS		0x54u
#define MCHP_SAF_CS1_OPC_OFS		0x64u
#define MCHP_SAF_CS_OPC_MASK		0xffffffffu
#define MCHP_SAF_CS_OPC_RD_POS		0
#define MCHP_SAF_CS_OPC_RD_MASK		0xffu
#define MCHP_SAF_CS_OPC_MNC_POS		8
#define MCHP_SAF_CS_OPC_MNC_MASK	0xff00ul
#define MCHP_SAF_CS_OPC_MC_POS		16
#define MCHP_SAF_CS_OPC_MC_MASK		0xff0000ul
#define MCHP_SAF_CS_OPC_POLL2_POS	24
#define MCHP_SAF_CS_OPC_POLL2_MASK	0xff000000ul

/* SAF CS0/CS1 registers */
#define MCHP_SAF_CS0_DESCR_OFS		0x58u
#define MCHP_SAF_CS1_DESCR_OFS		0x68u
#define MCHP_SAF_CS_DESCR_MASK		0x0000ff0fu
#define MCHP_SAF_CS_DESCR_ENTC_POS	0
#define MCHP_SAF_CS_DESCR_ENTC_MASK	0x0fu
#define MCHP_SAF_CS_DESCR_RDC_POS	8
#define MCHP_SAF_CS_DESCR_RDC_MASK	0x0f00ul
#define MCHP_SAF_CS_DESCR_SZC_POS	12
#define MCHP_SAF_CS_DESCR_SZC_MASK	0xf000ul

/* SAF Flash Configuration General Descriptors register */
#define MCHP_SAF_FL_CFG_GEN_DESCR_OFS		0x6cu
#define MCHP_SAF_FL_CFG_GEN_DESCR_MASK		0x0000ff0fu
/* value for standard 16 descriptor programming */
#define MCHP_SAF_FL_CFG_GEN_DESCR_STD		0x0000ee0cu
#define MCHP_SAF_FL_CFG_GEN_DESCR_EXC_POS	0
#define MCHP_SAF_FL_CFG_GEN_DESCR_EXC_MASK	0x0fu
#define MCHP_SAF_FL_CFG_GEN_DESCR_POLL1_POS	8
#define MCHP_SAF_FL_CFG_GEN_DESCR_POLL1_MASK	\
	SHLU32(0x0Fu, MCHP_SAF_FL_CFG_GEN_DESCR_POLL1_POS)

#define MCHP_SAF_FL_CFG_GEN_DESCR_POLL2_POS	12
#define MCHP_SAF_FL_CFG_GEN_DESCR_POLL2_MASK	\
	SHLU32(0x0Fu, MCHP_SAF_FL_CFG_GEN_DESCR_POLL2_POS)

/* SAF Protection Lock register */
#define MCHP_SAF_PROT_LOCK_OFS		0x70u
#define MCHP_SAF_PROT_LOCK_MASK		0x1ffffu
#define MCHP_SAF_PROT_LOCK0		BIT(0)
#define MCHP_SAF_PROT_LOCK1		BIT(1)
#define MCHP_SAF_PROT_LOCK2		BIT(2)
#define MCHP_SAF_PROT_LOCK3		BIT(3)
#define MCHP_SAF_PROT_LOCK4		BIT(4)
#define MCHP_SAF_PROT_LOCK5		BIT(5)
#define MCHP_SAF_PROT_LOCK6		BIT(6)
#define MCHP_SAF_PROT_LOCK7		BIT(7)
#define MCHP_SAF_PROT_LOCK8		BIT(8)
#define MCHP_SAF_PROT_LOCK9		BIT(9)
#define MCHP_SAF_PROT_LOCK10		BIT(10)
#define MCHP_SAF_PROT_LOCK11		BIT(11)
#define MCHP_SAF_PROT_LOCK12		BIT(12)
#define MCHP_SAF_PROT_LOCK13		BIT(13)
#define MCHP_SAF_PROT_LOCK14		BIT(14)
#define MCHP_SAF_PROT_LOCK15		BIT(15)
#define MCHP_SAF_PROT_LOCK16		BIT(16)

/* SAF Protection Dirty register */
#define MCHP_SAF_PROT_DIRTY_OFS		0x74u
#define MCHP_SAF_PROT_DIRTY_MASK	0xfffu
#define MCHP_SAF_PROT_DIRTY0		BIT(0)
#define MCHP_SAF_PROT_DIRTY1		BIT(1)
#define MCHP_SAF_PROT_DIRTY2		BIT(2)
#define MCHP_SAF_PROT_DIRTY3		BIT(3)
#define MCHP_SAF_PROT_DIRTY4		BIT(4)
#define MCHP_SAF_PROT_DIRTY5		BIT(5)
#define MCHP_SAF_PROT_DIRTY6		BIT(6)
#define MCHP_SAF_PROT_DIRTY7		BIT(7)
#define MCHP_SAF_PROT_DIRTY8		BIT(8)
#define MCHP_SAF_PROT_DIRTY9		BIT(9)
#define MCHP_SAF_PROT_DIRTY10		BIT(10)
#define MCHP_SAF_PROT_DIRTY11		BIT(11)

/* SAF Tag Map 0 register */
#define MCHP_SAF_TAG_MAP0_OFS		0x78u
#define MCHP_SAF_TAG_MAP0_MASK		0x77777777u
#define MCHP_SAF_TAG_MAP0_DFLT_VAL	0x23221100u
#define MCHP_SAF_TAG_MAP0_STM0_POS	0
#define MCHP_SAF_TAG_MAP0_STM0_MASK	0x07u
#define MCHP_SAF_TAG_MAP0_STM1_POS	4
#define MCHP_SAF_TAG_MAP0_STM1_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP0_STM0_MASK, MCHP_SAF_TAG_MAP0_STM1_POS)
#define MCHP_SAF_TAG_MAP0_STM2_POS	8
#define MCHP_SAF_TAG_MAP0_STM2_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP0_STM0_MASK, MCHP_SAF_TAG_MAP1_STM2_POS)
#define MCHP_SAF_TAG_MAP0_STM3_POS	12
#define MCHP_SAF_TAG_MAP0_STM3_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP0_STM0_MASK, MCHP_SAF_TAG_MAP1_STM3_POS)
#define MCHP_SAF_TAG_MAP0_STM4_POS	16
#define MCHP_SAF_TAG_MAP0_STM4_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP0_STM0_MASK, MCHP_SAF_TAG_MAP1_STM4_POS)
#define MCHP_SAF_TAG_MAP0_STM5_POS	20
#define MCHP_SAF_TAG_MAP0_STM5_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP0_STM0_MASK, MCHP_SAF_TAG_MAP1_STM5_POS)
#define MCHP_SAF_TAG_MAP0_STM6_POS	24
#define MCHP_SAF_TAG_MAP0_STM6_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP0_STM0_MASK, MCHP_SAF_TAG_MAP1_STM6_POS)
#define MCHP_SAF_TAG_MAP0_STM7_POS	28
#define MCHP_SAF_TAG_MAP0_STM7_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP0_STM0_MASK, MCHP_SAF_TAG_MAP1_STM7_POS)

/* SAF Tag Map 1 register */
#define MCHP_SAF_TAG_MAP1_OFS		0x7Cu
#define MCHP_SAF_TAG_MAP1_MASK		0x77777777u
#define MCHP_SAF_TAG_MAP1_DFLT_VAL	0x77677767u
#define MCHP_SAF_TAG_MAP1_STM8_POS	0
#define MCHP_SAF_TAG_MAP1_STM8_MASK	0x07u
#define MCHP_SAF_TAG_MAP1_STM9_POS	4
#define MCHP_SAF_TAG_MAP1_STM9_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP1_STM8_MASK, MCHP_SAF_TAG_MAP1_STM9_POS)
#define MCHP_SAF_TAG_MAP1_STMA_POS	8
#define MCHP_SAF_TAG_MAP1_STMA_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP1_STM8_MASK, MCHP_SAF_TAG_MAP1_STMA_POS)
#define MCHP_SAF_TAG_MAP1_STMB_POS	12
#define MCHP_SAF_TAG_MAP1_STMB_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP1_STM8_MASK, MCHP_SAF_TAG_MAP1_STMB_POS)
#define MCHP_SAF_TAG_MAP1_STMC_POS	16
#define MCHP_SAF_TAG_MAP1_STMC_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP1_STM8_MASK, MCHP_SAF_TAG_MAP1_STMC_POS)
#define MCHP_SAF_TAG_MAP1_STMD_POS	20
#define MCHP_SAF_TAG_MAP1_STMD_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP1_STM8_MASK, MCHP_SAF_TAG_MAP1_STMD_POS)
#define MCHP_SAF_TAG_MAP1_STME_POS	24
#define MCHP_SAF_TAG_MAP1_STME_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP1_STM8_MASK, MCHP_SAF_TAG_MAP1_STME_POS)
#define MCHP_SAF_TAG_MAP1_STMF_POS	28
#define MCHP_SAF_TAG_MAP1_STMF_MASK	\
	SHLU32(MCHP_SAF_TAG_MAP1_STM8_MASK, MCHP_SAF_TAG_MAP1_STMF_POS)

/* SAF Tag Map 2 register */
#define MCHP_SAF_TAG_MAP2_OFS		0x80u
#define MCHP_SAF_TAG_MAP2_MASK		0x80000007u
#define MCHP_SAF_TAG_MAP2_DFLT_VAL	0x00000005u
#define MCHP_SAF_TAG_MAP2_SM_EC_POS	0
#define MCHP_SAF_TAG_MAP2_SM_EC_MASK	0x07u
#define MCHP_SAF_TAG_MAP2_LOCK_POS	31
#define MCHP_SAF_TAG_MAP2_LOCK		BIT(MCHP_SAF_TAG_MAP2_LOCK_POS)

/* SAF Protection Region Start registers */
#define MCHP_SAF_PROT_RG0_START_OFS	0x84u
#define MCHP_SAF_PROT_RG1_START_OFS	0x94u
#define MCHP_SAF_PROT_RG2_START_OFS	0xA4u
#define MCHP_SAF_PROT_RG3_START_OFS	0xB4u
#define MCHP_SAF_PROT_RG4_START_OFS	0xC4u
#define MCHP_SAF_PROT_RG5_START_OFS	0xD4u
#define MCHP_SAF_PROT_RG6_START_OFS	0xE4u
#define MCHP_SAF_PROT_RG7_START_OFS	0xF4u
#define MCHP_SAF_PROT_RG8_START_OFS	0x104u
#define MCHP_SAF_PROT_RG9_START_OFS	0x114u
#define MCHP_SAF_PROT_RG10_START_OFS	0x124u
#define MCHP_SAF_PROT_RG11_START_OFS	0x134u
#define MCHP_SAF_PROT_RG12_START_OFS	0x144u
#define MCHP_SAF_PROT_RG13_START_OFS	0x154u
#define MCHP_SAF_PROT_RG14_START_OFS	0x164u
#define MCHP_SAF_PROT_RG15_START_OFS	0x174u
#define MCHP_SAF_PROT_RG16_START_OFS	0x184u
#define MCHP_SAF_PROT_RG_START_MASK	0xfffffu
#define MCHP_SAF_PROT_RG_START_DFLT	0x07fffu

/* SAF Protection Region Limit registers */
#define MCHP_SAF_PROT_RG0_LIMIT_OFS	0x88u
#define MCHP_SAF_PROT_RG1_LIMIT_OFS	0x98u
#define MCHP_SAF_PROT_RG2_LIMIT_OFS	0xa8u
#define MCHP_SAF_PROT_RG3_LIMIT_OFS	0xb8u
#define MCHP_SAF_PROT_RG4_LIMIT_OFS	0xc8u
#define MCHP_SAF_PROT_RG5_LIMIT_OFS	0xd8u
#define MCHP_SAF_PROT_RG6_LIMIT_OFS	0xe8u
#define MCHP_SAF_PROT_RG7_LIMIT_OFS	0xf8u
#define MCHP_SAF_PROT_RG8_LIMIT_OFS	0x108u
#define MCHP_SAF_PROT_RG9_LIMIT_OFS	0x118u
#define MCHP_SAF_PROT_RG10_LIMIT_OFS	0x128u
#define MCHP_SAF_PROT_RG11_LIMIT_OFS	0x138u
#define MCHP_SAF_PROT_RG12_LIMIT_OFS	0x148u
#define MCHP_SAF_PROT_RG13_LIMIT_OFS	0x158u
#define MCHP_SAF_PROT_RG14_LIMIT_OFS	0x168u
#define MCHP_SAF_PROT_RG15_LIMIT_OFS	0x178u
#define MCHP_SAF_PROT_RG16_LIMIT_OFS	0x188u
#define MCHP_SAF_PROT_RG_LIMIT_MASK	0xfffffu
#define MCHP_SAF_PROT_RG_LIMIT_DFLT	0

/* SAF Protection Region Write Bitmap registers */
#define MCHP_SAF_PROT_RG0_WBM_OFS	0x8cu
#define MCHP_SAF_PROT_RG1_WBM_OFS	0x9cu
#define MCHP_SAF_PROT_RG2_WBM_OFS	0xacu
#define MCHP_SAF_PROT_RG3_WBM_OFS	0xbcu
#define MCHP_SAF_PROT_RG4_WBM_OFS	0xccu
#define MCHP_SAF_PROT_RG5_WBM_OFS	0xdcu
#define MCHP_SAF_PROT_RG6_WBM_OFS	0xefu
#define MCHP_SAF_PROT_RG7_WBM_OFS	0xfcu
#define MCHP_SAF_PROT_RG8_WBM_OFS	0x10cu
#define MCHP_SAF_PROT_RG9_WBM_OFS	0x11cu
#define MCHP_SAF_PROT_RG10_WBM_OFS	0x12cu
#define MCHP_SAF_PROT_RG11_WBM_OFS	0x13cu
#define MCHP_SAF_PROT_RG12_WBM_OFS	0x14cu
#define MCHP_SAF_PROT_RG13_WBM_OFS	0x15cu
#define MCHP_SAF_PROT_RG14_WBM_OFS	0x16cu
#define MCHP_SAF_PROT_RG15_WBM_OFS	0x17cu
#define MCHP_SAF_PROT_RG16_WBM_OFS	0x18cu
#define MCHP_SAF_PROT_RG_WBM_MASK	0xffu
#define MCHP_SAF_PROT_RG_WBM0		BIT(0)
#define MCHP_SAF_PROT_RG_WBM1		BIT(1)
#define MCHP_SAF_PROT_RG_WBM2		BIT(2)
#define MCHP_SAF_PROT_RG_WBM3		BIT(3)
#define MCHP_SAF_PROT_RG_WBM4		BIT(4)
#define MCHP_SAF_PROT_RG_WBM5		BIT(5)
#define MCHP_SAF_PROT_RG_WBM6		BIT(6)
#define MCHP_SAF_PROT_RG_WBM7		BIT(7)

/* SAF Protection Region Read Bitmap registers */
#define MCHP_SAF_PROT_RG0_RBM_OFS	0x90u
#define MCHP_SAF_PROT_RG1_RBM_OFS	0xa0u
#define MCHP_SAF_PROT_RG2_RBM_OFS	0xb0u
#define MCHP_SAF_PROT_RG3_RBM_OFS	0xc0u
#define MCHP_SAF_PROT_RG4_RBM_OFS	0xd0u
#define MCHP_SAF_PROT_RG5_RBM_OFS	0xe0u
#define MCHP_SAF_PROT_RG6_RBM_OFS	0xf0u
#define MCHP_SAF_PROT_RG7_RBM_OFS	0x100u
#define MCHP_SAF_PROT_RG8_RBM_OFS	0x110u
#define MCHP_SAF_PROT_RG9_RBM_OFS	0x120u
#define MCHP_SAF_PROT_RG10_RBM_OFS	0x130u
#define MCHP_SAF_PROT_RG11_RBM_OFS	0x140u
#define MCHP_SAF_PROT_RG12_RBM_OFS	0x150u
#define MCHP_SAF_PROT_RG13_RBM_OFS	0x160u
#define MCHP_SAF_PROT_RG14_RBM_OFS	0x170u
#define MCHP_SAF_PROT_RG15_RBM_OFS	0x180u
#define MCHP_SAF_PROT_RG16_RBM_OFS	0x190u
#define MCHP_SAF_PROT_RG_RBM_MASK	0xffu
#define MCHP_SAF_PROT_RG_RBM0		BIT(0)
#define MCHP_SAF_PROT_RG_RBM1		BIT(1)
#define MCHP_SAF_PROT_RG_RBM2		BIT(2)
#define MCHP_SAF_PROT_RG_RBM3		BIT(3)
#define MCHP_SAF_PROT_RG_RBM4		BIT(4)
#define MCHP_SAF_PROT_RG_RBM5		BIT(5)
#define MCHP_SAF_PROT_RG_RBM6		BIT(6)
#define MCHP_SAF_PROT_RG_RBM7		BIT(7)

/* SAF Poll Timeout register */
#define MCHP_SAF_POLL_TMOUT_OFS		0x194u
#define MCHP_SAF_POLL_TMOUT_MASK	0x3ffffu
#define MCHP_SAF_POLL_TMOUT_5S		0x28000u

/* SAF Poll Interval register */
#define MCHP_SAF_POLL_INTRVL_OFS	0x198u
#define MCHP_SAF_POLL_INTRVL_MASK	0xffffu

/* SAF Suspend Resume Interval register */
#define MCHP_SAF_SUS_RSM_INTRVL_OFS	0x19Cu
#define MCHP_SAF_SUS_RSM_INTRVL_MASK	0xffffu

/* SAF Consecutive Read Timeout register */
#define MCHP_SAF_CRD_TMOUT_OFS		0x1a0u
#define MCHP_SAF_CRD_TMOUT_MASK		0xfffffu

/* SAF Flash CS0/CS1 Configuration Poll2 Mask registers */
#define MCHP_SAF_FL0_CFG_P2M_OFS	0x1a4u
#define MCHP_SAF_FL1_CFG_P2M_OFS	0x1a6u
#define MCHP_SAF_FL_CFG_P2M_MASK	0xffffu

/* SAF Flash Configuration Special Mode register */
#define MCHP_SAF_FL_CFG_SPM_OFS		0x1a8u
#define MCHP_SAF_FL_CFG_SPM_MASK	0x01u
#define MCHP_SAF_FL_CFG_SPM_DIS_SUSP	BIT(0)

/* SAF Suspend Check Delay register */
#define MCHP_SAF_SUS_CHK_DLY_OFS	0x1acu
#define MCHP_SAF_SUS_CHK_DLY_MASK	0xfffffu

/* SAF Flash 0/1 Continuous Mode Prefix registers */
#define MCHP_SAF_FL_CM_PRF_OFS		0x1b0u
#define MCHP_SAF_FL_CM_PRF_MASK		0xffffu
#define MCHP_SAF_FL_CM_PRF_CS_OP_POS	0
#define MCHP_SAF_FL_CM_PRF_CS_OP_MASK	0xffu
#define MCHP_SAF_FL_CM_PRF_CS_DAT_POS	8
#define MCHP_SAF_FL_CM_PRF_CS_DAT_MASK \
	SHLU32(MCHP_SAF_FL_CM_PRF_CS_OP_MASK, MCHP_SAF_FL_CM_PRF_CS_DAT_POS)

/* SAF DnX Protection Bypass register */
#define MCHP_SAF_DNX_PROT_BYP_OFS	0x1b4u
#define MCHP_SAF_DNX_PROT_BYP_MASK	0x1110ffffu
#define MCHP_SAF_DNX_PB_TAG_POS(n)	((uint32_t)(n) & 0xfu)
#define MCHP_SAF_DNX_PB_TAG(n)		BIT(((n) & 0xfu))
#define MCHP_SAF_DNX_DS_RO_POS		20
#define MCHP_SAF_DNX_DS_RO		BIT(20)
#define MCHP_SAF_DNX_DM_POS		24
#define MCHP_SAF_DNX_DM			BIT(24)
#define MCHP_SAF_DNX_LK_POS		28
#define MCHP_SAF_DNX_LK			BIT(28)

/* SAF Activity Count Reload Valud register */
#define MCHP_SAF_AC_RELOAD_OFS		0x1b8u
#define MCHP_SAF_AC_RELOAD_REG_MSK	0xffffu

/* SAF Power Down Control register */
#define SAF_PWRDN_CTRL_OFS		0x1bcu
#define SAF_PWRDN_CTRL_REG_MSK		0x0fu
#define SAF_PWRDN_CTRL_CS0_PD_EN_POS	0
#define SAF_PWRDN_CTRL_CS1_PD_EN_POS	1
#define SAF_PWRDN_CTRL_CS0_WPA_EN_POS	2
#define SAF_PWRDN_CTRL_CS1_WPA_EN_POS	3

/* SAF Memory Power Status register (RO) */
#define SAF_MEM_PWR_STS_OFS		0x1c0u
#define SAF_MEM_PWR_STS_REG_MSK		0x03u

/* SAF Config CS0 and CS1 Opcode registers */
#define SAF_CFG_CS0_OPC_OFS		0x1c4u
#define SAF_CFG_CS1_OPC_OFS		0x1c8u
#define SAF_CFG_CS_OPC_REG_MSK		0x00ffffffu
#define SAF_CFG_CS_OPC_ENTER_PD_POS	0
#define SAF_CFG_CS_OPC_ENTER_PD_MSK0	0xffu
#define SAF_CFG_CS_OPC_ENTER_PD_MSK	0xffu
#define SAF_CFG_CS_OPC_EXIT_PD_POS	8
#define SAF_CFG_CS_OPC_EXIT_PD_MSK0	0xffu
#define SAF_CFG_CS_OPC_EXIT_PD_MSK	0xff00u
#define SAF_CFG_CS_OPC_RPMC_OP2_POS	16
#define SAF_CFG_CS_OPC_RPMC_OP2_MSK0	0xffu
#define SAF_CFG_CS_OPC_RPMC_OP2_MSK	0xff0000u


/* SAF Flash Power Down/Up Timerout register */
#define SAF_FL_PWR_TMOUT_OFS		0x1ccu
#define SAF_FL_PWR_TMOUT_REG_MSK	0xffffu

/* SAF Clock Divider CS0 and CS1 registers */
#define SAF_CLKDIV_CS0_OFS		0x200u
#define SAF_CLKDIV_CS1_OFS		0x204u
#define SAF_CLKDIV_CS_REG_MSK		0xffffffffu
#define SAF_CLKDIV_CS_READ_POS		0
#define SAF_CLKDIV_CS_REST_POS		16
#define SAF_CLKDIV_CS_MSK0		0xffffu

/* SAF RPMC OP2 eSPI, EC0, and EC1 Result Address register */
#define	SAF_RPMC_OP2_ESPI_RES_OFS	0x208u
#define SAF_RPMC_OP2_EC0_RES_OFS	0x20cu
#define SAF_RPMC_OP2_EC1_RES_OFS	0x210u
#define	SAF_RPMC_OP2_RES_REG_MSK	0xffffffffu

/* SAF Communication Mode */
#define MCHP_SAF_COMM_MODE_MASK		0x01u
/* Allow pre-fetch from flash devices */
#define MCHP_SAF_COMM_MODE_PF_EN	BIT(0)

/* SAF TAG numbers[0:0xF] */
#define MCHP_SAF_TAG_M0T0	0u
#define MCHP_SAF_TAG_M0T1	1u
#define MCHP_SAF_TAG_M1T0	2u
#define MCHP_SAF_TAG_M1T1	3u
#define MCHP_SAF_TAG_M2T0	4u
#define MCHP_SAF_TAG_M2T1	5u
#define MCHP_SAF_TAG_M3T0	6u
#define MCHP_SAF_TAG_M2T2	7u
#define MCHP_SAF_TAG_M6T0	9u
#define MCHP_SAF_TAG_M6T1	0x0du
#define MCHP_SAF_TAG_MAX	0x10u

/* SAF Master numbers */
#define MCHP_SAF_MSTR_CS_INIT	0u
#define MCHP_SAF_MSTR_CPU	1u
#define MCHP_SAF_MSTR_CS_ME	2u
#define MCHP_SAF_MSTR_CS_LAN	3u
#define MCHP_SAF_MSTR_UNUSED4	4u
#define MCHP_SAF_MSTR_EC_FW	5u
#define MCHP_SAF_MSTR_CS_IE	6u
#define MCHP_SAF_MSTR_UNUSED7	7u
#define MCHP_SAF_MSTR_MAX	8u
#define MCHP_SAF_MSTR_ALL	0xffu

/* eSPI SAF */
/** @brief SAF SPI Opcodes and descriptor indices */
struct mchp_espi_saf_op {
	volatile uint32_t OPA;
	volatile uint32_t OPB;
	volatile uint32_t OPC;
	volatile uint32_t OP_DESCR;
};

/** @brief SAF protection regions contain 4 32-bit registers. */
struct mchp_espi_saf_pr {
	volatile uint32_t START;
	volatile uint32_t LIMIT;
	volatile uint32_t WEBM;
	volatile uint32_t RDBM;
};

/** @brief eSPI SAF configuration and control registers at 0x40008000 */
struct mchp_espi_saf {
	uint32_t RSVD1[6];
	volatile uint32_t SAF_ECP_CMD;			/* 0x18 */
	volatile uint32_t SAF_ECP_FLAR;			/* 0x1c */
	volatile uint32_t SAF_ECP_START;		/* 0x20 */
	volatile uint32_t SAF_ECP_BFAR;			/* 0x24 */
	volatile uint32_t SAF_ECP_STATUS;		/* 0x28 */
	volatile uint32_t SAF_ECP_INTEN;		/* 0x2c */
	volatile uint32_t SAF_FL_CFG_SIZE_LIM;		/* 0x30 */
	volatile uint32_t SAF_FL_CFG_THRH;		/* 0x34 */
	volatile uint32_t SAF_FL_CFG_MISC;		/* 0x38 */
	volatile uint32_t SAF_ESPI_MON_STATUS;		/* 0x3c */
	volatile uint32_t SAF_ESPI_MON_INTEN;		/* 0x40 */
	volatile uint32_t SAF_ECP_BUSY;			/* 0x44 */
	uint32_t RSVD2[1];
	struct mchp_espi_saf_op SAF_CS_OP[2];		/* 0x4c - 0x6b */
	volatile uint32_t SAF_FL_CFG_GEN_DESCR;		/* 0x6c */
	volatile uint32_t SAF_PROT_LOCK;		/* 0x70 */
	volatile uint32_t SAF_PROT_DIRTY;		/* 0x74 */
	volatile uint32_t SAF_TAG_MAP[3];		/* 0x78 - 0x83 */
	struct mchp_espi_saf_pr SAF_PROT_RG[17];	/* 0x84 - 0x193 */
	volatile uint32_t SAF_POLL_TMOUT;		/* 0x194 */
	volatile uint32_t SAF_POLL_INTRVL;		/* 0x198 */
	volatile uint32_t SAF_SUS_RSM_INTRVL;		/* 0x19c */
	volatile uint32_t SAF_CONSEC_RD_TMOUT;		/* 0x1a0 */
	volatile uint16_t SAF_CS0_CFG_P2M;		/* 0x1a4 */
	volatile uint16_t SAF_CS1_CFG_P2M;		/* 0x1a6 */
	volatile uint32_t SAF_FL_CFG_SPM;		/* 0x1a8 */
	volatile uint32_t SAF_SUS_CHK_DLY;		/* 0x1ac */
	volatile uint16_t SAF_CS0_CM_PRF;		/* 0x1b0 */
	volatile uint16_t SAF_CS1_CM_PRF;		/* 0x1b2 */
	volatile uint32_t SAF_DNX_PROT_BYP;		/* 0x1b4 */
	volatile uint32_t SAF_AC_RELOAD;		/* 0x1b8 */
	volatile uint32_t SAF_PWRDN_CTRL;		/* 0x1bc */
	volatile uint32_t SAF_MEM_PWR_STS;		/* 0x1c0 */
	volatile uint32_t SAF_CFG_CS0_OPD;		/* 0x1c4 */
	volatile uint32_t SAF_CFG_CS1_OPD;		/* 0x1c8 */
	volatile uint32_t SAF_FL_PWR_TMOUT;		/* 0x1cc */
	uint32_t RSVD[12];
	volatile uint32_t SAF_CLKDIV_CS0;		/* 0x200 */
	volatile uint32_t SAF_CLKDIV_CS1;		/* 0x204 */
	volatile uint32_t SAF_RPMC_OP2_ESPI_RES;	/* 0x208 */
	volatile uint32_t SAF_RPMC_OP2_EC0_RES;		/* 0x20c */
	volatile uint32_t SAF_RPMC_OP2_EC1_RES;		/* 0x210 */
};

struct mchp_espi_saf_comm { /* @ 0x40071000 */
	uint32_t TEST0;
	uint32_t TEST1;
	uint32_t TEST2;
	uint32_t TEST3;
	uint32_t TEST4;
	uint32_t TEST5;
	uint32_t TEST6;
	uint32_t RSVD1[(0x2b8 - 0x01c) / 4];
	uint32_t SAF_COMM_MODE; /* @ 0x400712b8 */
	uint32_t TEST7;
};

#endif /* _MEC172X_ESPI_SAF_H_ */
