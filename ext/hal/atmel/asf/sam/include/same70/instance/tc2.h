/**
 * \file
 *
 * \brief Instance description for TC2
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

#ifndef _SAME70_TC2_INSTANCE_H_
#define _SAME70_TC2_INSTANCE_H_

/* ========== Register definition for TC2 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_TC2_CCR0            (0x40014000) /**< (TC2) Channel Control Register (channel = 0) 0 */
#define REG_TC2_CMR0            (0x40014004) /**< (TC2) Channel Mode Register (channel = 0) 0 */
#define REG_TC2_SMMR0           (0x40014008) /**< (TC2) Stepper Motor Mode Register (channel = 0) 0 */
#define REG_TC2_RAB0            (0x4001400C) /**< (TC2) Register AB (channel = 0) 0 */
#define REG_TC2_CV0             (0x40014010) /**< (TC2) Counter Value (channel = 0) 0 */
#define REG_TC2_RA0             (0x40014014) /**< (TC2) Register A (channel = 0) 0 */
#define REG_TC2_RB0             (0x40014018) /**< (TC2) Register B (channel = 0) 0 */
#define REG_TC2_RC0             (0x4001401C) /**< (TC2) Register C (channel = 0) 0 */
#define REG_TC2_SR0             (0x40014020) /**< (TC2) Status Register (channel = 0) 0 */
#define REG_TC2_IER0            (0x40014024) /**< (TC2) Interrupt Enable Register (channel = 0) 0 */
#define REG_TC2_IDR0            (0x40014028) /**< (TC2) Interrupt Disable Register (channel = 0) 0 */
#define REG_TC2_IMR0            (0x4001402C) /**< (TC2) Interrupt Mask Register (channel = 0) 0 */
#define REG_TC2_EMR0            (0x40014030) /**< (TC2) Extended Mode Register (channel = 0) 0 */
#define REG_TC2_CCR1            (0x40014040) /**< (TC2) Channel Control Register (channel = 0) 1 */
#define REG_TC2_CMR1            (0x40014044) /**< (TC2) Channel Mode Register (channel = 0) 1 */
#define REG_TC2_SMMR1           (0x40014048) /**< (TC2) Stepper Motor Mode Register (channel = 0) 1 */
#define REG_TC2_RAB1            (0x4001404C) /**< (TC2) Register AB (channel = 0) 1 */
#define REG_TC2_CV1             (0x40014050) /**< (TC2) Counter Value (channel = 0) 1 */
#define REG_TC2_RA1             (0x40014054) /**< (TC2) Register A (channel = 0) 1 */
#define REG_TC2_RB1             (0x40014058) /**< (TC2) Register B (channel = 0) 1 */
#define REG_TC2_RC1             (0x4001405C) /**< (TC2) Register C (channel = 0) 1 */
#define REG_TC2_SR1             (0x40014060) /**< (TC2) Status Register (channel = 0) 1 */
#define REG_TC2_IER1            (0x40014064) /**< (TC2) Interrupt Enable Register (channel = 0) 1 */
#define REG_TC2_IDR1            (0x40014068) /**< (TC2) Interrupt Disable Register (channel = 0) 1 */
#define REG_TC2_IMR1            (0x4001406C) /**< (TC2) Interrupt Mask Register (channel = 0) 1 */
#define REG_TC2_EMR1            (0x40014070) /**< (TC2) Extended Mode Register (channel = 0) 1 */
#define REG_TC2_CCR2            (0x40014080) /**< (TC2) Channel Control Register (channel = 0) 2 */
#define REG_TC2_CMR2            (0x40014084) /**< (TC2) Channel Mode Register (channel = 0) 2 */
#define REG_TC2_SMMR2           (0x40014088) /**< (TC2) Stepper Motor Mode Register (channel = 0) 2 */
#define REG_TC2_RAB2            (0x4001408C) /**< (TC2) Register AB (channel = 0) 2 */
#define REG_TC2_CV2             (0x40014090) /**< (TC2) Counter Value (channel = 0) 2 */
#define REG_TC2_RA2             (0x40014094) /**< (TC2) Register A (channel = 0) 2 */
#define REG_TC2_RB2             (0x40014098) /**< (TC2) Register B (channel = 0) 2 */
#define REG_TC2_RC2             (0x4001409C) /**< (TC2) Register C (channel = 0) 2 */
#define REG_TC2_SR2             (0x400140A0) /**< (TC2) Status Register (channel = 0) 2 */
#define REG_TC2_IER2            (0x400140A4) /**< (TC2) Interrupt Enable Register (channel = 0) 2 */
#define REG_TC2_IDR2            (0x400140A8) /**< (TC2) Interrupt Disable Register (channel = 0) 2 */
#define REG_TC2_IMR2            (0x400140AC) /**< (TC2) Interrupt Mask Register (channel = 0) 2 */
#define REG_TC2_EMR2            (0x400140B0) /**< (TC2) Extended Mode Register (channel = 0) 2 */
#define REG_TC2_BCR             (0x400140C0) /**< (TC2) Block Control Register */
#define REG_TC2_BMR             (0x400140C4) /**< (TC2) Block Mode Register */
#define REG_TC2_QIER            (0x400140C8) /**< (TC2) QDEC Interrupt Enable Register */
#define REG_TC2_QIDR            (0x400140CC) /**< (TC2) QDEC Interrupt Disable Register */
#define REG_TC2_QIMR            (0x400140D0) /**< (TC2) QDEC Interrupt Mask Register */
#define REG_TC2_QISR            (0x400140D4) /**< (TC2) QDEC Interrupt Status Register */
#define REG_TC2_FMR             (0x400140D8) /**< (TC2) Fault Mode Register */
#define REG_TC2_WPMR            (0x400140E4) /**< (TC2) Write Protection Mode Register */

#else

#define REG_TC2_CCR0            (*(__O  uint32_t*)0x40014000U) /**< (TC2) Channel Control Register (channel = 0) 0 */
#define REG_TC2_CMR0            (*(__IO uint32_t*)0x40014004U) /**< (TC2) Channel Mode Register (channel = 0) 0 */
#define REG_TC2_SMMR0           (*(__IO uint32_t*)0x40014008U) /**< (TC2) Stepper Motor Mode Register (channel = 0) 0 */
#define REG_TC2_RAB0            (*(__I  uint32_t*)0x4001400CU) /**< (TC2) Register AB (channel = 0) 0 */
#define REG_TC2_CV0             (*(__I  uint32_t*)0x40014010U) /**< (TC2) Counter Value (channel = 0) 0 */
#define REG_TC2_RA0             (*(__IO uint32_t*)0x40014014U) /**< (TC2) Register A (channel = 0) 0 */
#define REG_TC2_RB0             (*(__IO uint32_t*)0x40014018U) /**< (TC2) Register B (channel = 0) 0 */
#define REG_TC2_RC0             (*(__IO uint32_t*)0x4001401CU) /**< (TC2) Register C (channel = 0) 0 */
#define REG_TC2_SR0             (*(__I  uint32_t*)0x40014020U) /**< (TC2) Status Register (channel = 0) 0 */
#define REG_TC2_IER0            (*(__O  uint32_t*)0x40014024U) /**< (TC2) Interrupt Enable Register (channel = 0) 0 */
#define REG_TC2_IDR0            (*(__O  uint32_t*)0x40014028U) /**< (TC2) Interrupt Disable Register (channel = 0) 0 */
#define REG_TC2_IMR0            (*(__I  uint32_t*)0x4001402CU) /**< (TC2) Interrupt Mask Register (channel = 0) 0 */
#define REG_TC2_EMR0            (*(__IO uint32_t*)0x40014030U) /**< (TC2) Extended Mode Register (channel = 0) 0 */
#define REG_TC2_CCR1            (*(__O  uint32_t*)0x40014040U) /**< (TC2) Channel Control Register (channel = 0) 1 */
#define REG_TC2_CMR1            (*(__IO uint32_t*)0x40014044U) /**< (TC2) Channel Mode Register (channel = 0) 1 */
#define REG_TC2_SMMR1           (*(__IO uint32_t*)0x40014048U) /**< (TC2) Stepper Motor Mode Register (channel = 0) 1 */
#define REG_TC2_RAB1            (*(__I  uint32_t*)0x4001404CU) /**< (TC2) Register AB (channel = 0) 1 */
#define REG_TC2_CV1             (*(__I  uint32_t*)0x40014050U) /**< (TC2) Counter Value (channel = 0) 1 */
#define REG_TC2_RA1             (*(__IO uint32_t*)0x40014054U) /**< (TC2) Register A (channel = 0) 1 */
#define REG_TC2_RB1             (*(__IO uint32_t*)0x40014058U) /**< (TC2) Register B (channel = 0) 1 */
#define REG_TC2_RC1             (*(__IO uint32_t*)0x4001405CU) /**< (TC2) Register C (channel = 0) 1 */
#define REG_TC2_SR1             (*(__I  uint32_t*)0x40014060U) /**< (TC2) Status Register (channel = 0) 1 */
#define REG_TC2_IER1            (*(__O  uint32_t*)0x40014064U) /**< (TC2) Interrupt Enable Register (channel = 0) 1 */
#define REG_TC2_IDR1            (*(__O  uint32_t*)0x40014068U) /**< (TC2) Interrupt Disable Register (channel = 0) 1 */
#define REG_TC2_IMR1            (*(__I  uint32_t*)0x4001406CU) /**< (TC2) Interrupt Mask Register (channel = 0) 1 */
#define REG_TC2_EMR1            (*(__IO uint32_t*)0x40014070U) /**< (TC2) Extended Mode Register (channel = 0) 1 */
#define REG_TC2_CCR2            (*(__O  uint32_t*)0x40014080U) /**< (TC2) Channel Control Register (channel = 0) 2 */
#define REG_TC2_CMR2            (*(__IO uint32_t*)0x40014084U) /**< (TC2) Channel Mode Register (channel = 0) 2 */
#define REG_TC2_SMMR2           (*(__IO uint32_t*)0x40014088U) /**< (TC2) Stepper Motor Mode Register (channel = 0) 2 */
#define REG_TC2_RAB2            (*(__I  uint32_t*)0x4001408CU) /**< (TC2) Register AB (channel = 0) 2 */
#define REG_TC2_CV2             (*(__I  uint32_t*)0x40014090U) /**< (TC2) Counter Value (channel = 0) 2 */
#define REG_TC2_RA2             (*(__IO uint32_t*)0x40014094U) /**< (TC2) Register A (channel = 0) 2 */
#define REG_TC2_RB2             (*(__IO uint32_t*)0x40014098U) /**< (TC2) Register B (channel = 0) 2 */
#define REG_TC2_RC2             (*(__IO uint32_t*)0x4001409CU) /**< (TC2) Register C (channel = 0) 2 */
#define REG_TC2_SR2             (*(__I  uint32_t*)0x400140A0U) /**< (TC2) Status Register (channel = 0) 2 */
#define REG_TC2_IER2            (*(__O  uint32_t*)0x400140A4U) /**< (TC2) Interrupt Enable Register (channel = 0) 2 */
#define REG_TC2_IDR2            (*(__O  uint32_t*)0x400140A8U) /**< (TC2) Interrupt Disable Register (channel = 0) 2 */
#define REG_TC2_IMR2            (*(__I  uint32_t*)0x400140ACU) /**< (TC2) Interrupt Mask Register (channel = 0) 2 */
#define REG_TC2_EMR2            (*(__IO uint32_t*)0x400140B0U) /**< (TC2) Extended Mode Register (channel = 0) 2 */
#define REG_TC2_BCR             (*(__O  uint32_t*)0x400140C0U) /**< (TC2) Block Control Register */
#define REG_TC2_BMR             (*(__IO uint32_t*)0x400140C4U) /**< (TC2) Block Mode Register */
#define REG_TC2_QIER            (*(__O  uint32_t*)0x400140C8U) /**< (TC2) QDEC Interrupt Enable Register */
#define REG_TC2_QIDR            (*(__O  uint32_t*)0x400140CCU) /**< (TC2) QDEC Interrupt Disable Register */
#define REG_TC2_QIMR            (*(__I  uint32_t*)0x400140D0U) /**< (TC2) QDEC Interrupt Mask Register */
#define REG_TC2_QISR            (*(__I  uint32_t*)0x400140D4U) /**< (TC2) QDEC Interrupt Status Register */
#define REG_TC2_FMR             (*(__IO uint32_t*)0x400140D8U) /**< (TC2) Fault Mode Register */
#define REG_TC2_WPMR            (*(__IO uint32_t*)0x400140E4U) /**< (TC2) Write Protection Mode Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for TC2 peripheral ========== */
#define TC2_INSTANCE_ID_CHANNEL1                 48        
#define TC2_INSTANCE_ID_CHANNEL0                 47        
#define TC2_INSTANCE_ID_CHANNEL2                 49        
#define TC2_DMAC_ID_RX                           42        

#endif /* _SAME70_TC2_INSTANCE_ */
