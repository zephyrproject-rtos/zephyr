/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/** @file global_cfg.h
 *MEC1501 Global Configuration Registers
 */
/** @defgroup MEC1501 Peripherals GlobalConfig
 */

#ifndef _GLOBAL_CFG_H
#define _GLOBAL_CFG_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

/* ===================================================================*/
/* ================	Global Config			============= */
/* ===================================================================*/

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
#define MCHP_GCFG_DEV_ID_REG32_OFS	0x1C
#define MCHP_GCFG_DEV_ID_REG_MASK	0xFFFFFFFFul
#define MCHP_GCFG_REV_ID_POS		0
#define MCHP_GCFG_DID_REV_MASK0		0xFFul
#define MCHP_GCFG_DID_REV_MASK		0xFFul
#define MCHP_GCFG_DID_SUB_ID_POS	8
#define MCHP_GCFG_DID_SUB_ID_MASK0	0xFFul
#define MCHP_GCFG_DID_SUB_ID_MASK	(0xFFul << 8)
#define MCHP_GCFG_DID_DEV_ID_POS	16
#define MCHP_GCFG_DID_DEV_ID_MASK0	0xFFFFul
#define MCHP_GCFG_DID_DEV_ID_MASK	(0xFFFFul << 16)

/* Byte[0] at offset 0x1C is the 8-bit revision ID */
#define MCHP_GCFG_REV_ID_REG_OFS	0x1C
#define MCHP_GCFG_REV_A1		0x02
#define MCHP_GCFG_REV_B0		0x03

/* 
 * Byte[1] at offset 0x1D is the 8-bit Sub-ID 
 * bits[3:0] = package type
 * bits[7:4] = chip family
 */
#define MCHP_GCFG_SUB_ID_OFS		0x1D
#define MCHP_GCFG_SUB_ID_PKG_POS	0
#define MCHP_GCFG_SUB_ID_PKG_MASK0	0x0F
#define MCHP_GCFG_SUB_ID_PKG_MASK	0x0F
#define MCHP_GCFG_SUB_ID_PKG_UNDEF	0x00
#define MCHP_GCFG_SUB_ID_PKG_64_PIN	0x01
#define MCHP_GCFG_SUB_ID_PKG_84_PIN	0x02
#define MCHP_GCFG_SUB_ID_PKG_128_PIN	0x03
#define MCHP_GCFG_SUB_ID_PKG_144_PIN	0x04
/* chip family field */
#define MCHP_GCFG_SUB_ID_FAM_POS	4
#define MCHP_GCFG_SUB_ID_FAM_MASK0	0x0F
#define MCHP_GCFG_SUB_ID_FAM_MASK	0xF0
#define MCHP_GCFG_SUB_ID_FAM_UNDEF	0x00
#define MCHP_GCFG_SUB_ID_FAM_MEC	0x01
#define MCHP_GCFG_SUB_ID_FAM_2		0x02
#define MCHP_GCFG_SUB_ID_FAM_3		0x03
#define MCHP_GCFG_SUB_ID_FAM_4		0x04
#define MCHP_GCFG_SUB_ID_FAM_5		0x05

#define MCHP_GCFG_DEV_ID_LSB_OFS	0x1E
#define MCHP_GCFG_DEV_ID_MSB_OFS	0x1F
#define MCHP_GCFG_DEV_ID_15XX		0x0020
#define MCHP_GCFG_DEV_ID_15XX_LSB	0x20
#define MCHP_GCFG_DEV_ID_15XX_MSB	0x00

/* Legacy Device ID value */
#define MCHP_CCFG_LEGACY_DID_REG_OFS	0x20
#define MCHP_GCFG_LEGACY_DEV_ID		0xFE

/*
 * Host access via configuration port (default I/O locations 0x2E/0x2F)
 */
#define MCHP_HOST_CFG_INDEX_IO_DFLT	0x2E
#define MCHP_HOST_CFG_DATA_IO_DFLT	0x2F
#define MCHP_HOST_CFG_UNLOCK	0x55
#define MCHP_HOST_CFG_LOCK	0xAA
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

/**
  * @brief Glocal Configuration Registers (GLOBAL_CFG)
  */
typedef struct global_cfg_regs
{
	__IOM uint8_t  RSVD0[2];
	__IOM uint8_t  TEST02;		/*!< (@ 0x0002) MCHP Test */
	__IOM uint8_t  RSVD1[4];
	__IOM uint8_t  LOG_DEV_NUM;	/*!< (@ 0x0007) Global Config Logical Device Number */
	__IOM uint8_t  RSVD2[20];
	__IOM uint32_t DEV_REV_ID;	/*!< (@ 0x001C) Device and revision ID */
	__IOM uint8_t  LEGACY_DEV_ID;	/*!< (@ 0x0020) Legacy Device ID */
	__IOM uint8_t  RSVD3[14];
} GLOBAL_CFG_Type;

#endif	/* #ifndef _GLOBAL_CFG_H */
/* end global_cfg.h */
/**   @}
 */
