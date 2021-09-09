/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_GLOBAL_CFG_H
#define _MEC_GLOBAL_CFG_H

#include <stdint.h>
#include <stddef.h>

/*
 * Device and Revision ID 32-bit register
 * b[7:0] = Revision
 * b[15:8] = Device Sub-ID
 * b[31:16] = Device ID
 * This register can be accesses as bytes or a single 32-bit read from
 * the EC. Host access byte access via the Host visible configuration
 * register space at 0x2E/0x2F (default).
 */
#define MCHP_GCFG_DEV_ID_REG32_OFS	28u
#define MCHP_GCFG_DEV_ID_REG_MASK	GENMASK(31, 0)
#define MCHP_GCFG_REV_ID_POS		0
#define MCHP_GCFG_DID_REV_MASK		GENMASK(7, 0)
#define MCHP_GCFG_DID_SUB_ID_POS	8
#define MCHP_GCFG_DID_SUB_ID_MASK	GENMASK(15, 8)
#define MCHP_GCFG_DID_DEV_ID_POS	16
#define MCHP_GCFG_DID_DEV_ID_MASK	GENMASK(31, 16)

/* Byte[0] at offset 0x1c is the 8-bit revision ID */
#define MCHP_GCFG_REV_A1		2u
#define MCHP_GCFG_REV_B0		3u

/*
 * Byte[1] at offset 0x1D is the 8-bit Sub-ID
 * bits[3:0] = package type
 * bits[7:4] = chip family
 */
#define MCHP_GCFG_SUB_ID_OFS		0x1du
#define MCHP_GCFG_SUB_ID_PKG_POS	0
#define MCHP_GCFG_SUB_ID_PKG_MASK	GENMASK(3, 0)
#define MCHP_GCFG_SUB_ID_PKG_UNDEF	0u
#define MCHP_GCFG_SUB_ID_PKG_64_PIN	1u
#define MCHP_GCFG_SUB_ID_PKG_84_PIN	2u
#define MCHP_GCFG_SUB_ID_PKG_128_PIN	3u
#define MCHP_GCFG_SUB_ID_PKG_144_PIN	4u
#define MCHP_GCFG_SUB_ID_PKG_176_PIN	7u
/* chip family field */
#define MCHP_GCFG_SUB_ID_FAM_POS	4u
#define MCHP_GCFG_SUB_ID_FAM_MASK	GENMASK(7, 4)
#define MCHP_GCFG_SUB_ID_FAM_UNDEF	0u
#define MCHP_GCFG_SUB_ID_FAM_1		0x10u
#define MCHP_GCFG_SUB_ID_FAM_2		0x20u
#define MCHP_GCFG_SUB_ID_FAM_3		0x30u
#define MCHP_GCFG_SUB_ID_FAM_4		0x40u
#define MCHP_GCFG_SUB_ID_FAM_5		0x50u
#define MCHP_GCFG_SUB_ID_FAM_6		0x60u
#define MCHP_GCFG_SUB_ID_FAM_7		0x70u

#define MCHP_GCFG_DEV_ID_LSB_OFS	0x1eu
#define MCHP_GCFG_DEV_ID_MSB_OFS	0x1fu
#define MCHP_GCFG_DEV_ID_172X		0x0022u
#define MCHP_GCFG_DEV_ID_172X_LSB	0x22u
#define MCHP_GCFG_DEV_ID_172X_MSB	0x00u

/* SZ 144-pin package parts */
#define MCHP_GCFG_DEVID_1723_144	0x00223400u
#define MCHP_GCFG_DEVID_1727_144	0x00227400u
/* LJ 176-pin package parts */
#define MCHP_GCFG_DID_1721_176		0x00222700u
#define MCHP_GCFG_DID_1723_176		0x00223700u
#define MCHP_GCFG_DID_1727_176		0x00227700u

/* Legacy Device ID value */
#define MCHP_CCFG_LEGACY_DID_REG_OFS	0x20u
#define MCHP_GCFG_LEGACY_DEV_ID		0xfeu

/* Host access via configuration port (default I/O locations 0x2E/0x2F) */
#define MCHP_HOST_CFG_INDEX_IO_DFLT	0x2eu
#define MCHP_HOST_CFG_DATA_IO_DFLT	0x2fu
#define MCHP_HOST_CFG_UNLOCK		0x55u
#define MCHP_HOST_CFG_LOCK		0xaau
/* Logical Device Configuration Indices */
#define MCHP_HOST_CFG_LDN_IDX		7u
#define MCHP_HOST_CFG_LD_ACTIVATE_IDX	0x30u
#define MCHP_HOST_CFG_LD_BASE_ADDR_IDX	0x34u
#define MCHP_HOST_CFG_LD_CFG_SEL_IDX	0xf0u

/* Global Configuration Registers */
struct global_cfg_regs {
	volatile uint8_t RSVD0[2];
	volatile uint8_t TEST02;
	volatile uint8_t RSVD1[4];
	volatile uint8_t LOG_DEV_NUM;
	volatile uint8_t RSVD2[20];
	volatile uint32_t DEV_REV_ID;
	volatile uint8_t LEGACY_DEV_ID;
	volatile uint8_t RSVD3[14];
};

#endif	/* #ifndef _MEC_GLOBAL_CFG_H */
