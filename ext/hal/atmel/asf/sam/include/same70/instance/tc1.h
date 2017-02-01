/**
 * \file
 *
 * \brief Instance description for TC1
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

#ifndef _SAME70_TC1_INSTANCE_H_
#define _SAME70_TC1_INSTANCE_H_

/* ========== Register definition for TC1 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_TC1_CCR0            (0x40010000) /**< (TC1) Channel Control Register (channel = 0) 0 */
#define REG_TC1_CMR0            (0x40010004) /**< (TC1) Channel Mode Register (channel = 0) 0 */
#define REG_TC1_SMMR0           (0x40010008) /**< (TC1) Stepper Motor Mode Register (channel = 0) 0 */
#define REG_TC1_RAB0            (0x4001000C) /**< (TC1) Register AB (channel = 0) 0 */
#define REG_TC1_CV0             (0x40010010) /**< (TC1) Counter Value (channel = 0) 0 */
#define REG_TC1_RA0             (0x40010014) /**< (TC1) Register A (channel = 0) 0 */
#define REG_TC1_RB0             (0x40010018) /**< (TC1) Register B (channel = 0) 0 */
#define REG_TC1_RC0             (0x4001001C) /**< (TC1) Register C (channel = 0) 0 */
#define REG_TC1_SR0             (0x40010020) /**< (TC1) Status Register (channel = 0) 0 */
#define REG_TC1_IER0            (0x40010024) /**< (TC1) Interrupt Enable Register (channel = 0) 0 */
#define REG_TC1_IDR0            (0x40010028) /**< (TC1) Interrupt Disable Register (channel = 0) 0 */
#define REG_TC1_IMR0            (0x4001002C) /**< (TC1) Interrupt Mask Register (channel = 0) 0 */
#define REG_TC1_EMR0            (0x40010030) /**< (TC1) Extended Mode Register (channel = 0) 0 */
#define REG_TC1_CCR1            (0x40010040) /**< (TC1) Channel Control Register (channel = 0) 1 */
#define REG_TC1_CMR1            (0x40010044) /**< (TC1) Channel Mode Register (channel = 0) 1 */
#define REG_TC1_SMMR1           (0x40010048) /**< (TC1) Stepper Motor Mode Register (channel = 0) 1 */
#define REG_TC1_RAB1            (0x4001004C) /**< (TC1) Register AB (channel = 0) 1 */
#define REG_TC1_CV1             (0x40010050) /**< (TC1) Counter Value (channel = 0) 1 */
#define REG_TC1_RA1             (0x40010054) /**< (TC1) Register A (channel = 0) 1 */
#define REG_TC1_RB1             (0x40010058) /**< (TC1) Register B (channel = 0) 1 */
#define REG_TC1_RC1             (0x4001005C) /**< (TC1) Register C (channel = 0) 1 */
#define REG_TC1_SR1             (0x40010060) /**< (TC1) Status Register (channel = 0) 1 */
#define REG_TC1_IER1            (0x40010064) /**< (TC1) Interrupt Enable Register (channel = 0) 1 */
#define REG_TC1_IDR1            (0x40010068) /**< (TC1) Interrupt Disable Register (channel = 0) 1 */
#define REG_TC1_IMR1            (0x4001006C) /**< (TC1) Interrupt Mask Register (channel = 0) 1 */
#define REG_TC1_EMR1            (0x40010070) /**< (TC1) Extended Mode Register (channel = 0) 1 */
#define REG_TC1_CCR2            (0x40010080) /**< (TC1) Channel Control Register (channel = 0) 2 */
#define REG_TC1_CMR2            (0x40010084) /**< (TC1) Channel Mode Register (channel = 0) 2 */
#define REG_TC1_SMMR2           (0x40010088) /**< (TC1) Stepper Motor Mode Register (channel = 0) 2 */
#define REG_TC1_RAB2            (0x4001008C) /**< (TC1) Register AB (channel = 0) 2 */
#define REG_TC1_CV2             (0x40010090) /**< (TC1) Counter Value (channel = 0) 2 */
#define REG_TC1_RA2             (0x40010094) /**< (TC1) Register A (channel = 0) 2 */
#define REG_TC1_RB2             (0x40010098) /**< (TC1) Register B (channel = 0) 2 */
#define REG_TC1_RC2             (0x4001009C) /**< (TC1) Register C (channel = 0) 2 */
#define REG_TC1_SR2             (0x400100A0) /**< (TC1) Status Register (channel = 0) 2 */
#define REG_TC1_IER2            (0x400100A4) /**< (TC1) Interrupt Enable Register (channel = 0) 2 */
#define REG_TC1_IDR2            (0x400100A8) /**< (TC1) Interrupt Disable Register (channel = 0) 2 */
#define REG_TC1_IMR2            (0x400100AC) /**< (TC1) Interrupt Mask Register (channel = 0) 2 */
#define REG_TC1_EMR2            (0x400100B0) /**< (TC1) Extended Mode Register (channel = 0) 2 */
#define REG_TC1_BCR             (0x400100C0) /**< (TC1) Block Control Register */
#define REG_TC1_BMR             (0x400100C4) /**< (TC1) Block Mode Register */
#define REG_TC1_QIER            (0x400100C8) /**< (TC1) QDEC Interrupt Enable Register */
#define REG_TC1_QIDR            (0x400100CC) /**< (TC1) QDEC Interrupt Disable Register */
#define REG_TC1_QIMR            (0x400100D0) /**< (TC1) QDEC Interrupt Mask Register */
#define REG_TC1_QISR            (0x400100D4) /**< (TC1) QDEC Interrupt Status Register */
#define REG_TC1_FMR             (0x400100D8) /**< (TC1) Fault Mode Register */
#define REG_TC1_WPMR            (0x400100E4) /**< (TC1) Write Protection Mode Register */

#else

#define REG_TC1_CCR0            (*(__O  uint32_t*)0x40010000U) /**< (TC1) Channel Control Register (channel = 0) 0 */
#define REG_TC1_CMR0            (*(__IO uint32_t*)0x40010004U) /**< (TC1) Channel Mode Register (channel = 0) 0 */
#define REG_TC1_SMMR0           (*(__IO uint32_t*)0x40010008U) /**< (TC1) Stepper Motor Mode Register (channel = 0) 0 */
#define REG_TC1_RAB0            (*(__I  uint32_t*)0x4001000CU) /**< (TC1) Register AB (channel = 0) 0 */
#define REG_TC1_CV0             (*(__I  uint32_t*)0x40010010U) /**< (TC1) Counter Value (channel = 0) 0 */
#define REG_TC1_RA0             (*(__IO uint32_t*)0x40010014U) /**< (TC1) Register A (channel = 0) 0 */
#define REG_TC1_RB0             (*(__IO uint32_t*)0x40010018U) /**< (TC1) Register B (channel = 0) 0 */
#define REG_TC1_RC0             (*(__IO uint32_t*)0x4001001CU) /**< (TC1) Register C (channel = 0) 0 */
#define REG_TC1_SR0             (*(__I  uint32_t*)0x40010020U) /**< (TC1) Status Register (channel = 0) 0 */
#define REG_TC1_IER0            (*(__O  uint32_t*)0x40010024U) /**< (TC1) Interrupt Enable Register (channel = 0) 0 */
#define REG_TC1_IDR0            (*(__O  uint32_t*)0x40010028U) /**< (TC1) Interrupt Disable Register (channel = 0) 0 */
#define REG_TC1_IMR0            (*(__I  uint32_t*)0x4001002CU) /**< (TC1) Interrupt Mask Register (channel = 0) 0 */
#define REG_TC1_EMR0            (*(__IO uint32_t*)0x40010030U) /**< (TC1) Extended Mode Register (channel = 0) 0 */
#define REG_TC1_CCR1            (*(__O  uint32_t*)0x40010040U) /**< (TC1) Channel Control Register (channel = 0) 1 */
#define REG_TC1_CMR1            (*(__IO uint32_t*)0x40010044U) /**< (TC1) Channel Mode Register (channel = 0) 1 */
#define REG_TC1_SMMR1           (*(__IO uint32_t*)0x40010048U) /**< (TC1) Stepper Motor Mode Register (channel = 0) 1 */
#define REG_TC1_RAB1            (*(__I  uint32_t*)0x4001004CU) /**< (TC1) Register AB (channel = 0) 1 */
#define REG_TC1_CV1             (*(__I  uint32_t*)0x40010050U) /**< (TC1) Counter Value (channel = 0) 1 */
#define REG_TC1_RA1             (*(__IO uint32_t*)0x40010054U) /**< (TC1) Register A (channel = 0) 1 */
#define REG_TC1_RB1             (*(__IO uint32_t*)0x40010058U) /**< (TC1) Register B (channel = 0) 1 */
#define REG_TC1_RC1             (*(__IO uint32_t*)0x4001005CU) /**< (TC1) Register C (channel = 0) 1 */
#define REG_TC1_SR1             (*(__I  uint32_t*)0x40010060U) /**< (TC1) Status Register (channel = 0) 1 */
#define REG_TC1_IER1            (*(__O  uint32_t*)0x40010064U) /**< (TC1) Interrupt Enable Register (channel = 0) 1 */
#define REG_TC1_IDR1            (*(__O  uint32_t*)0x40010068U) /**< (TC1) Interrupt Disable Register (channel = 0) 1 */
#define REG_TC1_IMR1            (*(__I  uint32_t*)0x4001006CU) /**< (TC1) Interrupt Mask Register (channel = 0) 1 */
#define REG_TC1_EMR1            (*(__IO uint32_t*)0x40010070U) /**< (TC1) Extended Mode Register (channel = 0) 1 */
#define REG_TC1_CCR2            (*(__O  uint32_t*)0x40010080U) /**< (TC1) Channel Control Register (channel = 0) 2 */
#define REG_TC1_CMR2            (*(__IO uint32_t*)0x40010084U) /**< (TC1) Channel Mode Register (channel = 0) 2 */
#define REG_TC1_SMMR2           (*(__IO uint32_t*)0x40010088U) /**< (TC1) Stepper Motor Mode Register (channel = 0) 2 */
#define REG_TC1_RAB2            (*(__I  uint32_t*)0x4001008CU) /**< (TC1) Register AB (channel = 0) 2 */
#define REG_TC1_CV2             (*(__I  uint32_t*)0x40010090U) /**< (TC1) Counter Value (channel = 0) 2 */
#define REG_TC1_RA2             (*(__IO uint32_t*)0x40010094U) /**< (TC1) Register A (channel = 0) 2 */
#define REG_TC1_RB2             (*(__IO uint32_t*)0x40010098U) /**< (TC1) Register B (channel = 0) 2 */
#define REG_TC1_RC2             (*(__IO uint32_t*)0x4001009CU) /**< (TC1) Register C (channel = 0) 2 */
#define REG_TC1_SR2             (*(__I  uint32_t*)0x400100A0U) /**< (TC1) Status Register (channel = 0) 2 */
#define REG_TC1_IER2            (*(__O  uint32_t*)0x400100A4U) /**< (TC1) Interrupt Enable Register (channel = 0) 2 */
#define REG_TC1_IDR2            (*(__O  uint32_t*)0x400100A8U) /**< (TC1) Interrupt Disable Register (channel = 0) 2 */
#define REG_TC1_IMR2            (*(__I  uint32_t*)0x400100ACU) /**< (TC1) Interrupt Mask Register (channel = 0) 2 */
#define REG_TC1_EMR2            (*(__IO uint32_t*)0x400100B0U) /**< (TC1) Extended Mode Register (channel = 0) 2 */
#define REG_TC1_BCR             (*(__O  uint32_t*)0x400100C0U) /**< (TC1) Block Control Register */
#define REG_TC1_BMR             (*(__IO uint32_t*)0x400100C4U) /**< (TC1) Block Mode Register */
#define REG_TC1_QIER            (*(__O  uint32_t*)0x400100C8U) /**< (TC1) QDEC Interrupt Enable Register */
#define REG_TC1_QIDR            (*(__O  uint32_t*)0x400100CCU) /**< (TC1) QDEC Interrupt Disable Register */
#define REG_TC1_QIMR            (*(__I  uint32_t*)0x400100D0U) /**< (TC1) QDEC Interrupt Mask Register */
#define REG_TC1_QISR            (*(__I  uint32_t*)0x400100D4U) /**< (TC1) QDEC Interrupt Status Register */
#define REG_TC1_FMR             (*(__IO uint32_t*)0x400100D8U) /**< (TC1) Fault Mode Register */
#define REG_TC1_WPMR            (*(__IO uint32_t*)0x400100E4U) /**< (TC1) Write Protection Mode Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for TC1 peripheral ========== */
#define TC1_INSTANCE_ID_CHANNEL1                 27        
#define TC1_INSTANCE_ID_CHANNEL0                 26        
#define TC1_INSTANCE_ID_CHANNEL2                 28        
#define TC1_DMAC_ID_RX                           41        

#endif /* _SAME70_TC1_INSTANCE_ */
