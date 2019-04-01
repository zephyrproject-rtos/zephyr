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

/** @file port92.h
 *MEC1501 Fast Port92h Registers
 */
/** @defgroup MEC1501 Peripherals Fast Port92
 */

#ifndef _PORT92_H
#define _PORT92_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

/* =========================================================================*/
/* ================   PORT92				   ================ */
/* =========================================================================*/

#define MCHP_PORT92_BASE_ADDR	0x400F2000ul

/*
 * HOST_P92
 */
#define MCHP_PORT92_HOST_MASK			0x03ul
#define MCHP_PORT92_HOST_ALT_CPU_RST_POS	0u
#define MCHP_PORT92_HOST_ALT_CPU_RST		(1ul << 0)
#define MCHP_PORT92_HOST_ALT_GA20_POS		1u
#define MCHP_PORT92_HOST_ALT_GA20		(1ul << 1)

/*
 * GATEA20_CTRL
 */
#define MCHP_PORT92_GA20_CTRL_MASK	0x01ul
#define MCHP_PORT92_GA20_CTRL_VAL_POS	0u
#define MCHP_PORT92_GA20_CTRL_VAL_MASK	(1ul << 0)
#define MCHP_PORT92_GA20_CTRL_VAL_HI	(1ul << 0)
#define MCHP_PORT92_GA20_CTRL_VAL_LO	(0ul << 0)

/*
 * SETGA20L - writes of any data to this register causes
 * GATEA20 latch to be set.
 */
#define MCHP_PORT92_SETGA20L_MASK	0x01ul
#define MCHP_PORT92_SETGA20L_SET_POS	0u
#define MCHP_PORT92_SETGA20L_SET	(1ul << 0)

/*
 * RSTGA20L - writes of any data to this register causes
 * the GATEA20 latch to be reset
 */
#define MCHP_PORT92_RSTGA20L_MASK	0x01ul
#define MCHP_PORT92_RSTGA20L_SET_POS	0u
#define MCHP_PORT92_RSTGA20L_RST	(1ul << 0)

/*
 * ACTV
 */
#define MCHP_PORT92_ACTV_MASK		0x01ul
#define MCHP_PORT92_ACTV_ENABLE		0x01ul

/**
  * @brief Fast Port92h Registers (PORT92)
  */
typedef struct port92_regs
 {
	__IOM uint32_t HOST_P92;	/*!< (@ 0x0000) HOST Port92h register */
	uint8_t RSVD1[0x100u - 0x04u];
	__IOM uint32_t GATEA20_CTRL;	/*!< (@ 0x0100) Gate A20 Control */
	uint8_t RSVD2[4];
	__IOM uint32_t SETGA20L;	/*!< (@ 0x0108) Set Gate A20 */
	__IOM uint32_t RSTGA20L;	/*!< (@ 0x010C) Reset Gate A20 */
	uint8_t RSVD3[0x0330ul - 0x0110ul];
	__IOM uint32_t ACTV;	/*!< (@ 0x0330) Logical device Activate */
} PORT92_Type;

#endif	/* #ifndef _PORT92_H */
/* end port92.h */
/**   @}
 */
