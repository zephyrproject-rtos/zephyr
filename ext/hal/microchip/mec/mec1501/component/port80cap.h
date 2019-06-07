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

/** @file port80cap.h
 *MEC1501 Port80 Capture Registers
 */
/** @defgroup MEC1501 Peripherals Port80 Capture
 */

#ifndef _PORT80CAP_H
#define _PORT80CAP_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

/* ========================================================================*/
/* ================		PORT80 Capture			===========*/
/* ========================================================================*/

#define MCHP_PORT80_CAP_0_BASE_ADDR		0x400F8000ul
#define MCHP_PORT80_CAP_1_BASE_ADDR		0x400F8000ul

#define MCHP_PORT80_CAP_0_GIRQ			15u
#define MCHP_PORT80_CAP_0_GIRQ_POS		22u
#define MCHP_PORT80_CAP_0_GIRQ_VAL		(1ul << 22)
#define MCHP_PORT80_CAP_0_NVIC_AGGR		7u
#define MCHP_PORT80_CAP_0_NVIC_DIRECT		62u

#define MCHP_PORT80_CAP_1_GIRQ			15u
#define MCHP_PORT80_CAP_1_GIRQ_POS		23u
#define MCHP_PORT80_CAP_1_GIRQ_VAL		(1ul << 23)
#define MCHP_PORT80_CAP_1_NVIC_AGGR		7u
#define MCHP_PORT80_CAP_1_NVIC_DIRECT		63u

/* Port80 Capture receive FIFO number of entries */
#define MCHP_PORT80_CAP_MAX_FIFO_ENTRIES	16u

/*
 * HOST_DATA - Write-Only
 */
#define MCHP_PORT80_CAP_HOST_DATA_REG_MASK	0xFFul

/*
 * EC_DATA - Read-Only. Read as 32-bit.
 * b[7:0]  = Read captured data byte from FIFO
 * b[31:8] = Timestamp if enabled.
 */
#define MCHP_PORT80_CAP_EC_DATA_REG_MASK	0xFFFFFFFFul
#define MCHP_PORT80_CAP_EC_DATA_POS		0u
#define MCHP_PORT80_CAP_EC_DATA_MASK		0xFFul
#define MCHP_PORT80_CAP_EC_DATA_TIMESTAMP_POS	8u
#define MCHP_PORT80_CAP_EC_DATA_TIMESTAMP_MASK0	0x00FFFFFFul
#define MCHP_PORT80_CAP_EC_DATA_TIMESTAMP_MASK	0xFFFFFF00ul

/*
 * Configuration
 */
#define MCHP_PORT80_CAP_CFG_REG_MASK		0xFFul
/* Flush FIFO (Write-Only) */
#define MCHP_PORT80_CAP_CFG_FLUSH_POS		1u
#define MCHP_PORT80_CAP_CFG_FLUSH		(1ul << 1)
/* Reset Timestamp (Write-Only) */
#define MCHP_PORT80_CAP_CFG_TSRST_POS		2u
#define MCHP_PORT80_CAP_CFG_TSRST		(1ul << 2u)
/* Timestamp clock divider */
#define MCHP_PORT80_CAP_CFG_TSDIV_POS		3u
#define MCHP_PORT80_CAP_CFG_TSDIV_MASK0		0x03ul
#define MCHP_PORT80_CAP_CFG_TSDIV_MASK		(0x03ul << 3)
#define MCHP_PORT80_CAP_CFG_TSDIV_6MHZ		(0x00ul << 3)
#define MCHP_PORT80_CAP_CFG_TSDIV_3MHZ		(0x01ul << 3)
#define MCHP_PORT80_CAP_CFG_TSDIV_1P5MHZ	(0x02ul << 3)
#define MCHP_PORT80_CAP_CFG_TSDIV_750KHZ	(0x03ul << 3)
/* Timestamp Enable */
#define MCHP_PORT80_CAP_CFG_TSEN_POS		5u
#define MCHP_PORT80_CAP_CFG_TSEN_MASK		(1ul << 5)
#define MCHP_PORT80_CAP_CFG_TSEN_ENABLE		(1ul << 5)
/* FIFO threshold */
#define MCHP_PORT80_CAP_CFG_FIFO_THR_POS	6u
#define MCHP_PORT80_CAP_CFG_FIFO_THR_MASK0	0x03ul
#define MCHP_PORT80_CAP_CFG_FIFO_THR_MASK	(0x03ul << 6)
#define MCHP_PORT80_CAP_CFG_FIFO_THR_1		(0x00ul << 6)
#define MCHP_PORT80_CAP_CFG_FIFO_THR_4		(0x01ul << 6)
#define MCHP_PORT80_CAP_CFG_FIFO_THR_8		(0x02ul << 6)
#define MCHP_PORT80_CAP_CFG_FIFO_THR_14		(0x03ul << 6)

/*
 * Status - Read-only does not clear status on read.
 */
#define MCHP_PORT80_CAP_STS_REG_MASK		0x03ul;
/* Bit[0] FIFO not empty. Cleared by FW reading all content from FIFO */
#define MCHP_PORT80_CAP_STS_NOT_EMPTY_POS	0u
#define MCHP_PORT80_CAP_STS_NOT_EMPTY		(1ul << 0)
/* Bit[1] Overrun. Host wrote data when FIFO is full */
#define MCHP_PORT80_CAP_STS_OVERRUN_POS		1u
#define MCHP_PORT80_CAP_STS_OVERRUN		(1u << 1)

/*
 * Count - R/W access to Port 80 counter
 */
#define MCHP_PORT80_CAP_CNT_REG_MASK		0xFFFFFF00ul
#define MCHP_PORT80_CAP_CNT_POS			8u

/*
 * Port80 Capture Logical Device Activate register
 */
#define MCHP_PORT80_CAP_ACTV_MASK		0x01ul
#define MCHP_PORT80_CAP_ACTV_ENABLE		0x01ul

/**
  * @brief Fast Port80 Capture Registers (PORT80_CAP_Type)
  */
typedef struct port80cap_regs
 {
 	__OM  uint32_t HOST_DATA;	/*!< (@ 0x0000) Host Data b[7:0] write-only */
	uint8_t RSVD1[0x100u - 0x04u];
	__IM  uint32_t EC_DATA;		/*!< (@ 0x0100) EC Data. Read-only. */
	__IOM uint32_t CONFIG;		/*!< (@ 0x0104) Configuration Mix of R/W and WO */
	__IOM uint32_t STATUS;		/*!< (@ 0x0108) Status. Read-only */
	__IOM uint32_t COUNT;		/*!< (@ 0x010C) Counter. R/W */
	uint8_t RSVD3[0x0330ul - 0x0110ul];
	__IOM uint32_t ACTV;		/*!< (@ 0x0330) Logical device Activate */
} PORT80_CAP_Type;

#endif	/* #ifndef _PORT80CAP_H */
/* end port80cap.h */
/**   @}
 */
