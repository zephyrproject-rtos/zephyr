/**
 * \file
 *
 * \brief Instance description for TC3
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

#ifndef _SAME70_TC3_INSTANCE_H_
#define _SAME70_TC3_INSTANCE_H_

/* ========== Register definition for TC3 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_TC3_CCR0            (0x40054000) /**< (TC3) Channel Control Register (channel = 0) 0 */
#define REG_TC3_CMR0            (0x40054004) /**< (TC3) Channel Mode Register (channel = 0) 0 */
#define REG_TC3_SMMR0           (0x40054008) /**< (TC3) Stepper Motor Mode Register (channel = 0) 0 */
#define REG_TC3_RAB0            (0x4005400C) /**< (TC3) Register AB (channel = 0) 0 */
#define REG_TC3_CV0             (0x40054010) /**< (TC3) Counter Value (channel = 0) 0 */
#define REG_TC3_RA0             (0x40054014) /**< (TC3) Register A (channel = 0) 0 */
#define REG_TC3_RB0             (0x40054018) /**< (TC3) Register B (channel = 0) 0 */
#define REG_TC3_RC0             (0x4005401C) /**< (TC3) Register C (channel = 0) 0 */
#define REG_TC3_SR0             (0x40054020) /**< (TC3) Status Register (channel = 0) 0 */
#define REG_TC3_IER0            (0x40054024) /**< (TC3) Interrupt Enable Register (channel = 0) 0 */
#define REG_TC3_IDR0            (0x40054028) /**< (TC3) Interrupt Disable Register (channel = 0) 0 */
#define REG_TC3_IMR0            (0x4005402C) /**< (TC3) Interrupt Mask Register (channel = 0) 0 */
#define REG_TC3_EMR0            (0x40054030) /**< (TC3) Extended Mode Register (channel = 0) 0 */
#define REG_TC3_CCR1            (0x40054040) /**< (TC3) Channel Control Register (channel = 0) 1 */
#define REG_TC3_CMR1            (0x40054044) /**< (TC3) Channel Mode Register (channel = 0) 1 */
#define REG_TC3_SMMR1           (0x40054048) /**< (TC3) Stepper Motor Mode Register (channel = 0) 1 */
#define REG_TC3_RAB1            (0x4005404C) /**< (TC3) Register AB (channel = 0) 1 */
#define REG_TC3_CV1             (0x40054050) /**< (TC3) Counter Value (channel = 0) 1 */
#define REG_TC3_RA1             (0x40054054) /**< (TC3) Register A (channel = 0) 1 */
#define REG_TC3_RB1             (0x40054058) /**< (TC3) Register B (channel = 0) 1 */
#define REG_TC3_RC1             (0x4005405C) /**< (TC3) Register C (channel = 0) 1 */
#define REG_TC3_SR1             (0x40054060) /**< (TC3) Status Register (channel = 0) 1 */
#define REG_TC3_IER1            (0x40054064) /**< (TC3) Interrupt Enable Register (channel = 0) 1 */
#define REG_TC3_IDR1            (0x40054068) /**< (TC3) Interrupt Disable Register (channel = 0) 1 */
#define REG_TC3_IMR1            (0x4005406C) /**< (TC3) Interrupt Mask Register (channel = 0) 1 */
#define REG_TC3_EMR1            (0x40054070) /**< (TC3) Extended Mode Register (channel = 0) 1 */
#define REG_TC3_CCR2            (0x40054080) /**< (TC3) Channel Control Register (channel = 0) 2 */
#define REG_TC3_CMR2            (0x40054084) /**< (TC3) Channel Mode Register (channel = 0) 2 */
#define REG_TC3_SMMR2           (0x40054088) /**< (TC3) Stepper Motor Mode Register (channel = 0) 2 */
#define REG_TC3_RAB2            (0x4005408C) /**< (TC3) Register AB (channel = 0) 2 */
#define REG_TC3_CV2             (0x40054090) /**< (TC3) Counter Value (channel = 0) 2 */
#define REG_TC3_RA2             (0x40054094) /**< (TC3) Register A (channel = 0) 2 */
#define REG_TC3_RB2             (0x40054098) /**< (TC3) Register B (channel = 0) 2 */
#define REG_TC3_RC2             (0x4005409C) /**< (TC3) Register C (channel = 0) 2 */
#define REG_TC3_SR2             (0x400540A0) /**< (TC3) Status Register (channel = 0) 2 */
#define REG_TC3_IER2            (0x400540A4) /**< (TC3) Interrupt Enable Register (channel = 0) 2 */
#define REG_TC3_IDR2            (0x400540A8) /**< (TC3) Interrupt Disable Register (channel = 0) 2 */
#define REG_TC3_IMR2            (0x400540AC) /**< (TC3) Interrupt Mask Register (channel = 0) 2 */
#define REG_TC3_EMR2            (0x400540B0) /**< (TC3) Extended Mode Register (channel = 0) 2 */
#define REG_TC3_BCR             (0x400540C0) /**< (TC3) Block Control Register */
#define REG_TC3_BMR             (0x400540C4) /**< (TC3) Block Mode Register */
#define REG_TC3_QIER            (0x400540C8) /**< (TC3) QDEC Interrupt Enable Register */
#define REG_TC3_QIDR            (0x400540CC) /**< (TC3) QDEC Interrupt Disable Register */
#define REG_TC3_QIMR            (0x400540D0) /**< (TC3) QDEC Interrupt Mask Register */
#define REG_TC3_QISR            (0x400540D4) /**< (TC3) QDEC Interrupt Status Register */
#define REG_TC3_FMR             (0x400540D8) /**< (TC3) Fault Mode Register */
#define REG_TC3_WPMR            (0x400540E4) /**< (TC3) Write Protection Mode Register */

#else

#define REG_TC3_CCR0            (*(__O  uint32_t*)0x40054000U) /**< (TC3) Channel Control Register (channel = 0) 0 */
#define REG_TC3_CMR0            (*(__IO uint32_t*)0x40054004U) /**< (TC3) Channel Mode Register (channel = 0) 0 */
#define REG_TC3_SMMR0           (*(__IO uint32_t*)0x40054008U) /**< (TC3) Stepper Motor Mode Register (channel = 0) 0 */
#define REG_TC3_RAB0            (*(__I  uint32_t*)0x4005400CU) /**< (TC3) Register AB (channel = 0) 0 */
#define REG_TC3_CV0             (*(__I  uint32_t*)0x40054010U) /**< (TC3) Counter Value (channel = 0) 0 */
#define REG_TC3_RA0             (*(__IO uint32_t*)0x40054014U) /**< (TC3) Register A (channel = 0) 0 */
#define REG_TC3_RB0             (*(__IO uint32_t*)0x40054018U) /**< (TC3) Register B (channel = 0) 0 */
#define REG_TC3_RC0             (*(__IO uint32_t*)0x4005401CU) /**< (TC3) Register C (channel = 0) 0 */
#define REG_TC3_SR0             (*(__I  uint32_t*)0x40054020U) /**< (TC3) Status Register (channel = 0) 0 */
#define REG_TC3_IER0            (*(__O  uint32_t*)0x40054024U) /**< (TC3) Interrupt Enable Register (channel = 0) 0 */
#define REG_TC3_IDR0            (*(__O  uint32_t*)0x40054028U) /**< (TC3) Interrupt Disable Register (channel = 0) 0 */
#define REG_TC3_IMR0            (*(__I  uint32_t*)0x4005402CU) /**< (TC3) Interrupt Mask Register (channel = 0) 0 */
#define REG_TC3_EMR0            (*(__IO uint32_t*)0x40054030U) /**< (TC3) Extended Mode Register (channel = 0) 0 */
#define REG_TC3_CCR1            (*(__O  uint32_t*)0x40054040U) /**< (TC3) Channel Control Register (channel = 0) 1 */
#define REG_TC3_CMR1            (*(__IO uint32_t*)0x40054044U) /**< (TC3) Channel Mode Register (channel = 0) 1 */
#define REG_TC3_SMMR1           (*(__IO uint32_t*)0x40054048U) /**< (TC3) Stepper Motor Mode Register (channel = 0) 1 */
#define REG_TC3_RAB1            (*(__I  uint32_t*)0x4005404CU) /**< (TC3) Register AB (channel = 0) 1 */
#define REG_TC3_CV1             (*(__I  uint32_t*)0x40054050U) /**< (TC3) Counter Value (channel = 0) 1 */
#define REG_TC3_RA1             (*(__IO uint32_t*)0x40054054U) /**< (TC3) Register A (channel = 0) 1 */
#define REG_TC3_RB1             (*(__IO uint32_t*)0x40054058U) /**< (TC3) Register B (channel = 0) 1 */
#define REG_TC3_RC1             (*(__IO uint32_t*)0x4005405CU) /**< (TC3) Register C (channel = 0) 1 */
#define REG_TC3_SR1             (*(__I  uint32_t*)0x40054060U) /**< (TC3) Status Register (channel = 0) 1 */
#define REG_TC3_IER1            (*(__O  uint32_t*)0x40054064U) /**< (TC3) Interrupt Enable Register (channel = 0) 1 */
#define REG_TC3_IDR1            (*(__O  uint32_t*)0x40054068U) /**< (TC3) Interrupt Disable Register (channel = 0) 1 */
#define REG_TC3_IMR1            (*(__I  uint32_t*)0x4005406CU) /**< (TC3) Interrupt Mask Register (channel = 0) 1 */
#define REG_TC3_EMR1            (*(__IO uint32_t*)0x40054070U) /**< (TC3) Extended Mode Register (channel = 0) 1 */
#define REG_TC3_CCR2            (*(__O  uint32_t*)0x40054080U) /**< (TC3) Channel Control Register (channel = 0) 2 */
#define REG_TC3_CMR2            (*(__IO uint32_t*)0x40054084U) /**< (TC3) Channel Mode Register (channel = 0) 2 */
#define REG_TC3_SMMR2           (*(__IO uint32_t*)0x40054088U) /**< (TC3) Stepper Motor Mode Register (channel = 0) 2 */
#define REG_TC3_RAB2            (*(__I  uint32_t*)0x4005408CU) /**< (TC3) Register AB (channel = 0) 2 */
#define REG_TC3_CV2             (*(__I  uint32_t*)0x40054090U) /**< (TC3) Counter Value (channel = 0) 2 */
#define REG_TC3_RA2             (*(__IO uint32_t*)0x40054094U) /**< (TC3) Register A (channel = 0) 2 */
#define REG_TC3_RB2             (*(__IO uint32_t*)0x40054098U) /**< (TC3) Register B (channel = 0) 2 */
#define REG_TC3_RC2             (*(__IO uint32_t*)0x4005409CU) /**< (TC3) Register C (channel = 0) 2 */
#define REG_TC3_SR2             (*(__I  uint32_t*)0x400540A0U) /**< (TC3) Status Register (channel = 0) 2 */
#define REG_TC3_IER2            (*(__O  uint32_t*)0x400540A4U) /**< (TC3) Interrupt Enable Register (channel = 0) 2 */
#define REG_TC3_IDR2            (*(__O  uint32_t*)0x400540A8U) /**< (TC3) Interrupt Disable Register (channel = 0) 2 */
#define REG_TC3_IMR2            (*(__I  uint32_t*)0x400540ACU) /**< (TC3) Interrupt Mask Register (channel = 0) 2 */
#define REG_TC3_EMR2            (*(__IO uint32_t*)0x400540B0U) /**< (TC3) Extended Mode Register (channel = 0) 2 */
#define REG_TC3_BCR             (*(__O  uint32_t*)0x400540C0U) /**< (TC3) Block Control Register */
#define REG_TC3_BMR             (*(__IO uint32_t*)0x400540C4U) /**< (TC3) Block Mode Register */
#define REG_TC3_QIER            (*(__O  uint32_t*)0x400540C8U) /**< (TC3) QDEC Interrupt Enable Register */
#define REG_TC3_QIDR            (*(__O  uint32_t*)0x400540CCU) /**< (TC3) QDEC Interrupt Disable Register */
#define REG_TC3_QIMR            (*(__I  uint32_t*)0x400540D0U) /**< (TC3) QDEC Interrupt Mask Register */
#define REG_TC3_QISR            (*(__I  uint32_t*)0x400540D4U) /**< (TC3) QDEC Interrupt Status Register */
#define REG_TC3_FMR             (*(__IO uint32_t*)0x400540D8U) /**< (TC3) Fault Mode Register */
#define REG_TC3_WPMR            (*(__IO uint32_t*)0x400540E4U) /**< (TC3) Write Protection Mode Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for TC3 peripheral ========== */
#define TC3_INSTANCE_ID_CHANNEL1                 51        
#define TC3_INSTANCE_ID_CHANNEL0                 50        
#define TC3_INSTANCE_ID_CHANNEL2                 52        
#define TC3_DMAC_ID_RX                           43        

#endif /* _SAME70_TC3_INSTANCE_ */
