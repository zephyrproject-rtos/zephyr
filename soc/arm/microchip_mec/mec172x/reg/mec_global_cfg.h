/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_GLOBAL_CFG_H
#define _MEC_GLOBAL_CFG_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"
#include "mec_regaccess.h"

#define MCHP_GCFG_BASE_ADDR	0x400FFF00ul

/*
 * Device and Revision ID 32-bit register
 * b[7:0] = Revision
 * b[15:8] = Device Sub-ID
 * b[31:16] = Device ID
 * This register can be accesses as bytes or a single 32-bit read from
 * the EC. Host access byte access via the Host visible configuration
 * register space at 0x2E/0x2F(default).
 */
#define MCHP_GCFG_DEV_ID_REG32_OFS	0x1Cu
#define MCHP_GCFG_DEV_ID_REG_MASK	0xFFFFFFFFul
#define MCHP_GCFG_REV_ID_POS		0
#define MCHP_GCFG_DID_REV_MASK0		0xFFul
#define MCHP_GCFG_DID_REV_MASK		0xFFul
#define MCHP_GCFG_DID_SUB_ID_POS	8
#define MCHP_GCFG_DID_SUB_ID_MASK0	0xFFul
#define MCHP_GCFG_DID_SUB_ID_MASK	\
	SHLU32(MCHP_GCFG_DID_SUB_ID_MASK0, MCHP_GCFG_DID_SUB_ID_POS)
#define MCHP_GCFG_DID_DEV_ID_POS	16
#define MCHP_GCFG_DID_DEV_ID_MASK0	0xFFFFul
#define MCHP_GCFG_DID_DEV_ID_MASK	\
	SHLU32(MCHP_GCFG_DID_DEV_ID_MASK0, MCHP_GCFG_DID_DEV_ID_POS)

/* Byte[0] at offset 0x1C is the 8-bit revision ID */
#define MCHP_GCFG_REV_ID_REG_OFS	0x1Cu
#define MCHP_GCFG_REV_A1		0x02u
#define MCHP_GCFG_REV_B0		0x03u

/*
 * Byte[1] at offset 0x1D is the 8-bit Sub-ID
 * bits[3:0] = package type
 * bits[7:4] = chip family
 */
#define MCHP_GCFG_SUB_ID_OFS		0x1Du
#define MCHP_GCFG_SUB_ID_PKG_POS	0u
#define MCHP_GCFG_SUB_ID_PKG_MASK0	0x0Fu
#define MCHP_GCFG_SUB_ID_PKG_MASK	0x0Fu
#define MCHP_GCFG_SUB_ID_PKG_UNDEF	0x00u
#define MCHP_GCFG_SUB_ID_PKG_64_PIN	0x01u
#define MCHP_GCFG_SUB_ID_PKG_84_PIN	0x02u
#define MCHP_GCFG_SUB_ID_PKG_128_PIN	0x03u
#define MCHP_GCFG_SUB_ID_PKG_144_PIN	0x04u
#define MCHP_GCFG_SUB_ID_PKG_176_PIN	0x07u
/* chip family field */
#define MCHP_GCFG_SUB_ID_FAM_POS	4u
#define MCHP_GCFG_SUB_ID_FAM_MASK0	0x0Fu
#define MCHP_GCFG_SUB_ID_FAM_MASK	0xF0u
#define MCHP_GCFG_SUB_ID_FAM_UNDEF	0x00u
#define MCHP_GCFG_SUB_ID_FAM_1		0x01u
#define MCHP_GCFG_SUB_ID_FAM_2		0x02u
#define MCHP_GCFG_SUB_ID_FAM_3		0x03u
#define MCHP_GCFG_SUB_ID_FAM_4		0x04u
#define MCHP_GCFG_SUB_ID_FAM_5		0x05u
#define MCHP_GCFG_SUB_ID_FAM_6		0x06u
#define MCHP_GCFG_SUB_ID_FAM_7		0x07u

#define MCHP_GCFG_DEV_ID_LSB_OFS	0x1Eu
#define MCHP_GCFG_DEV_ID_MSB_OFS	0x1Fu
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
#define MCHP_CCFG_LEGACY_DID_REG_OFS	0x20
#define MCHP_GCFG_LEGACY_DEV_ID		0xFE

/*
 * Host access via configuration port (default I/O locations 0x2E/0x2F)
 */
#define MCHP_HOST_CFG_INDEX_IO_DFLT	0x2E
#define MCHP_HOST_CFG_DATA_IO_DFLT	0x2F
#define MCHP_HOST_CFG_UNLOCK		0x55
#define MCHP_HOST_CFG_LOCK		0xAA
/*
 * Logical Device Configuration Indices.
 */
#define MCHP_HOST_CFG_LDN_IDX		0x07
#define MCHP_HOST_CFG_LD_ACTIVATE_IDX	0x30
#define MCHP_HOST_CFG_LD_BASE_ADDR_IDX	0x34
#define MCHP_HOST_CFG_LD_CFG_SEL_IDX	0xF0


/* Read 32-bit Device, Sub, and Revision ID */
#define MCHP_DEVICE_REV_ID() \
	REG32(MCHP_GCFG_BASE_ADDR + MCHP_GCFG_DEV_ID_REG32_OFS)

/* Read 16-bit Device ID */
#define MCHP_DEVICE_ID() \
	REG16(MCHP_GCFG_BASE_ADDR + MCHP_GCFG_DEV_ID_LSB_OFS)

/* Read 8-bit Sub ID */
#define MCHP_DEV_SUB_ID() \
	REG8(MCHP_GCFG_BASE_ADDR + MCHP_GCFG_SUB_ID_OFS)

/* Read 8-bit Revision ID */
#define MCHP_REVISION_ID() \
	REG8(MCHP_GCFG_BASE_ADDR + MCHP_GCFG_REV_ID_REG_OFS)

#endif	/* #ifndef _MEC_GLOBAL_CFG_H */
