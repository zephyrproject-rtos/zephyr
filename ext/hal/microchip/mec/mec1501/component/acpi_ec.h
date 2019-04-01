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

/** @file acpi_ec.h
 *MEC1501 ACPI EC Registers
 */
/** @defgroup MEC1501 Peripherals ACPI_EC
 */

#ifndef _ACPI_EC_H
#define _ACPI_EC_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

/* =========================================================================*/
/* ================   ACPI_EC				   ================ */
/* =========================================================================*/

#define MCHP_ACPI_EC_BASE_ADDR		0x400F0800ul

#define MCHP_ACPI_EC_NUM_INSTANCES	4u
#define MCHP_ACPI_EC_SPACING		0x0400ul
#define MCHP_ACPI_EC_SPACING_PWROF2	10u

#define MCHP_ACPI_EC0_ADDR		0x400F0800ul
#define MCHP_ACPI_EC1_ADDR		0x400F0C00ul
#define MCHP_ACPI_EC2_ADDR		0x400F1000ul
#define MCHP_ACPI_EC3_ADDR		0x400F1400ul

/* 0 <= n < MCHP_ACPI_EC_NUM_INSTANCES */
#define MCHP_ACPI_EC_ADDR(n) (MCHP_ACPI_EC_BASE_ADDR +\
	((uint32_t)(n) << MCHP_ACPI_EC_SPACING_PWROF2))

/*
 * ACPI_EC interrupts
 */
#define MCHP_ACPI_EC_GIRQ		15u
#define MCHP_ACPI_EC_GIRQ_NVIC		7u

#define MCHP_ACPI_EC_0_IBF_NVIC		45u
#define MCHP_ACPI_EC_0_OBE_NVIC		46u
#define MCHP_ACPI_EC_0_IBF_GIRQ_POS	5u
#define MCHP_ACPI_EC_0_OBE_GIRQ_POS	6u
#define MCHP_ACPI_EC_0_IBF_GIRQ		(1ul << 5)
#define MCHP_ACPI_EC_0_OBE_GIRQ		(1ul << 6)

#define MCHP_ACPI_EC_1_IBF_NVIC		47u
#define MCHP_ACPI_EC_1_OBE_NVIC		48u
#define MCHP_ACPI_EC_1_IBF_GIRQ_POS	7u
#define MCHP_ACPI_EC_1_OBE_GIRQ_POS	8u
#define MCHP_ACPI_EC_1_IBF_GIRQ		(1ul << 7)
#define MCHP_ACPI_EC_1_OBE_GIRQ		(1ul << 8)

#define MCHP_ACPI_EC_2_IBF_NVIC		49u
#define MCHP_ACPI_EC_2_OBE_NVIC		50u
#define MCHP_ACPI_EC_2_IBF_GIRQ_POS	9u
#define MCHP_ACPI_EC_2_OBE_GIRQ_POS	10u
#define MCHP_ACPI_EC_2_IBF_GIRQ		(1ul << 9)
#define MCHP_ACPI_EC_2_OBE_GIRQ		(1ul << 10)

#define MCHP_ACPI_EC_3_IBF_NVIC		51u
#define MCHP_ACPI_EC_3_OBE_NVIC		52u
#define MCHP_ACPI_EC_3_IBF_GIRQ_POS	11u
#define MCHP_ACPI_EC_3_OBE_GIRQ_POS	12u
#define MCHP_ACPI_EC_3_IBF_GIRQ		(1ul << 11)
#define MCHP_ACPI_EC_3_OBE_GIRQ		(1ul << 12)

/* 0 <= n < MCHP_ACPI_EC_NUM_INSTANCES */
#define MCHP_ACPI_EC_IBF_NVIC(n)	(45ul + ((uint32_t)(n) << 1))
#define MCHP_ACPI_EC_OBE_NVIC(n)	(46ul + ((uint32_t)(n) << 1))
#define MCHP_ACPI_EC_IBF_GIRQ_POS(n)	(5ul + ((uint32_t)(n) << 1))
#define MCHP_ACPI_EC_OBE_GIRQ_POS(n)	(6ul + ((uint32_t)(n) << 1))
#define MCHP_ACPI_EC_IBF_GIRQ(n)	(1ul << MCHP_ACPI_EC_IBF_GIRQ_POS(n))
#define MCHP_ACPI_EC_OBE_GIRQ(n)	(1ul << MCHP_ACPI_EC_OBE_GIRQ_POS(n))


/*
 * EC_STS and OS_CMD_STS(read) bit definitions
 */
#define MCHP_ACPI_EC_STS_OBF_POS	0u
#define MCHP_ACPI_EC_STS_OBF		(1ul << (ACPI_EC_STS_OBF_POS))
#define MCHP_ACPI_EC_STS_IBF_POS	1u
#define MCHP_ACPI_EC_STS_IBF		(1ul << (ACPI_EC_STS_IBF_POS))
#define MCHP_ACPI_EC_STS_UD1A_POS	2u
#define MCHP_ACPI_EC_STS_UD1A		(1ul << (ACPI_EC_STS_UD1A_POS))
#define MCHP_ACPI_EC_STS_CMD_POS	3u
#define MCHP_ACPI_EC_STS_CMD		(1ul << (ACPI_EC_STS_CMD_POS))
#define MCHP_ACPI_EC_STS_BURST_POS	4u
#define MCHP_ACPI_EC_STS_BURST		(1ul << (ACPI_EC_STS_BURST_POS))
#define MCHP_ACPI_EC_STS_SCI_POS	5u
#define MCHP_ACPI_EC_STS_SCI		(1ul << (ACPI_EC_STS_SCI_POS))
#define MCHP_ACPI_EC_STS_SMI_POS	6u
#define MCHP_ACPI_EC_STS_SMI		(1ul << (ACPI_EC_STS_SMI_POS))
#define MCHP_ACPI_EC_STS_UD0A_POS	7u
#define MCHP_ACPI_EC_STS_UD0A		(1ul << (ACPI_EC_STS_UD0A_POS))

/*
 * EC_BYTE_CTRL and OS_BYTE_CTRL
 */
#define MCHP_ACPI_EC_BYTE_CTRL_4B_POS	0u
#define MCHP_ACPI_EC_BYTE_CTRL_4B_EN	(1ul << (ACPI_EC_BYTE_CTRL_4B_POS))

/**
  * @brief ACPI EC Registers (ACPI_EC)
  */
typedef struct acpi_ec_regs
 {
	__IOM uint32_t OS_DATA;		/*!< (@ 0x0000) OS Data */
	__IOM uint8_t OS_CMD_STS;	/*!< (@ 0x0004) OS Command(WO), Status(RO) */
	__IOM uint8_t OS_BYTE_CTRL;	/*!< (@ 0x0005) OS Byte Control */
	uint8_t RSVD1[0x100u - 0x06u];
	__IOM uint32_t EC2OS_DATA;	/*!< (@ 0x0100) EC to OS Data */
	__IOM uint8_t EC_STS;		/*!< (@ 0x0104) EC Status */
	__IOM uint8_t EC_BYTE_CTRL;	/*!< (@ 0x0105) EC Byte Control */
	uint8_t RSVD2[2];
	__IOM uint32_t OS2EC_DATA;	/*!< (@ 0x0108) OS to EC Data */
} ACPI_EC_Type;

#endif	/* #ifndef _ACPI_EC_H */
/* end acpi_ec.h */
/**   @}
 */
