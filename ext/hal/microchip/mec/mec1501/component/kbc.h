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

/** @file kbc.h
 *MEC1501 EM8042 Keyboard Controller Registers
 */
/** @defgroup MEC1501 Peripherals KBC
 */

#ifndef _KBC_H
#define _KBC_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

/* =========================================================================*/
/* ================   KBC				   ================ */
/* =========================================================================*/

#define MCHP_KBC_BASE_ADDR	0x400F0400ul

/*
 * KBC interrupts
 */
#define MCHP_KBC_GIRQ			15u
#define MCHP_KBC_GIRQ_NVIC		7u
#define MCHP_KBC_OBE_NVIC_DIRECT	58u
#define MCHP_KBC_IBF_NVIC_DIRECT	59u

#define MCHP_KBC_OBE_GIRQ_POS	18u
#define MCHP_KBC_IBF_GIRQ_POS	19u

#define MCHP_KBC_OBE_GIRQ	(1ul << 18)
#define MCHP_KBC_IBF_GIRQ	(1ul << 19)

/*
 * EC_KBC_STS and KBC_STS_RD bit definitions
 */
#define MCHP_KBC_STS_OBF_POS	0u
#define MCHP_KBC_STS_OBF	(1ul << (MCHP_KBC_STS_OBF_POS))
#define MCHP_KBC_STS_IBF_POS	1u
#define MCHP_KBC_STS_IBF	(1ul << (MCHP_KBC_STS_IBF_POS))
#define MCHP_KBC_STS_UD0_POS	2u
#define MCHP_KBC_STS_UD0	(1ul << (MCHP_KBC_STS_UD0_POS))
#define MCHP_KBC_STS_CD_POS	3u
#define MCHP_KBC_STS_CD		(1ul << (MCHP_KBC_STS_CD_POS))
#define MCHP_KBC_STS_UD1_POS	4u
#define MCHP_KBC_STS_UD1	(1ul << (MCHP_KBC_STS_UD1_POS))
#define MCHP_KBC_STS_AUXOBF_POS	5u
#define MCHP_KBC_STS_AUXOBF	(1ul << (MCHP_KBC_STS_AUXOBF_POS))
#define MCHP_KBC_STS_UD2_POS	6u
#define MCHP_KBC_STS_UD2_MASK0	0x03ul
#define MCHP_KBC_STS_UD2_MASK	(0x03ul << 6)
#define MCHP_KBC_STS_UD2_0_POS	6u
#define MCHP_KBC_STS_UD2_0	(1ul << 6)
#define MCHP_KBC_STS_UD2_1	(1ul << 7)

/*
 * KBC_CTRL bit definitions
 */
#define MCHP_KBC_CTRL_UD3_POS	0u
#define MCHP_KBC_CTRL_UD3	(1ul << (MCHP_KBC_CTRL_UD3_POS))
#define MCHP_KBC_CTRL_SAEN_POS	1u
#define MCHP_KBC_CTRL_SAEN	(1ul << (MCHP_KBC_CTRL_SAEN_POS))
#define MCHP_KBC_CTRL_PCOBFEN_POS	2u
#define MCHP_KBC_CTRL_PCOBFEN	(1ul << (MCHP_KBC_CTRL_PCOBFEN_POS))
#define MCHP_KBC_CTRL_UD4_POS	3u
#define MCHP_KBC_CTRL_UD4_MASK0	0x03ul
#define MCHP_KBC_CTRL_UD4_MASK	(0x03ul << (MCHP_KBC_CTRL_UD4_POS))
#define MCHP_KBC_CTRL_OBFEN_POS	5u
#define MCHP_KBC_CTRL_OBFEN	(1ul << (MCHP_KBC_CTRL_OBFEN_POS))
#define MCHP_KBC_CTRL_UD5_POS	6u
#define MCHP_KBC_CTRL_UD5	(1ul << (MCHP_KBC_CTRL_UD5_POS))
#define MCHP_KBC_CTRL_AUXH_POS	7u
#define MCHP_KBC_CTRL_AUXH	(1ul << (MCHP_KBC_CTRL_AUXH_POS))

/*
 * PCOBF register bit definitions
 */
#define MCHP_KBC_PCOBF_EN_POS	0u
#define MCHP_KBC_PCOBF_EN	(1ul << (MCHP_KBC_PCOBF_EN_POS))

/*
 * KBC_PORT92_EN register bit definitions
 */
#define MCHP_KBC_PORT92_EN_POS	0u
#define MCHP_KBC_PORT92_EN	(1ul << (MCHP_KBC_PORT92_EN_POS))

/**
  * @brief Keyboard Controller Registers (KBC)
  */
typedef struct kbc_regs
{
	__IOM uint32_t HOST_AUX_DATA;	/*!< (@ 0x0000) EC_HOST and AUX Data register */
	__IOM uint32_t KBC_STS_RD;	/*!< (@ 0x0004) Keyboard Status Read register */
	uint8_t RSVD1[0x100u - 0x08u];
	__IOM uint32_t EC_DATA;	/*!< (@ 0x0100) EC Data */
	__IOM uint32_t EC_KBC_STS;	/*!< (@ 0x0104) EC KBC Status */
	__IOM uint32_t KBC_CTRL;	/*!< (@ 0x0108) KBC Control */
	__IOM uint32_t EC_AUX_DATA;	/*!< (@ 0x010C) EC Aux Data */
	uint32_t RSVD2[1];
	__IOM uint32_t PCOBF;	/*!< (@ 0x0114) PCOBF register */
	uint8_t RSVD3[0x0330ul - 0x0118ul];
	__IOM uint32_t KBC_PORT92_EN;	/*!< (@ 0x0330) Port92h enable */
} KBC_Type;

#endif	/* #ifndef _KBC_H */
/* end kbc.h */
/**   @}
 */
