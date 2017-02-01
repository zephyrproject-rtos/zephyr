/**
 * \file
 *
 * \brief Instance description for TC0
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

#ifndef _SAME70_TC0_INSTANCE_H_
#define _SAME70_TC0_INSTANCE_H_

/* ========== Register definition for TC0 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_TC0_CCR0            (0x4000C000) /**< (TC0) Channel Control Register (channel = 0) 0 */
#define REG_TC0_CMR0            (0x4000C004) /**< (TC0) Channel Mode Register (channel = 0) 0 */
#define REG_TC0_SMMR0           (0x4000C008) /**< (TC0) Stepper Motor Mode Register (channel = 0) 0 */
#define REG_TC0_RAB0            (0x4000C00C) /**< (TC0) Register AB (channel = 0) 0 */
#define REG_TC0_CV0             (0x4000C010) /**< (TC0) Counter Value (channel = 0) 0 */
#define REG_TC0_RA0             (0x4000C014) /**< (TC0) Register A (channel = 0) 0 */
#define REG_TC0_RB0             (0x4000C018) /**< (TC0) Register B (channel = 0) 0 */
#define REG_TC0_RC0             (0x4000C01C) /**< (TC0) Register C (channel = 0) 0 */
#define REG_TC0_SR0             (0x4000C020) /**< (TC0) Status Register (channel = 0) 0 */
#define REG_TC0_IER0            (0x4000C024) /**< (TC0) Interrupt Enable Register (channel = 0) 0 */
#define REG_TC0_IDR0            (0x4000C028) /**< (TC0) Interrupt Disable Register (channel = 0) 0 */
#define REG_TC0_IMR0            (0x4000C02C) /**< (TC0) Interrupt Mask Register (channel = 0) 0 */
#define REG_TC0_EMR0            (0x4000C030) /**< (TC0) Extended Mode Register (channel = 0) 0 */
#define REG_TC0_CCR1            (0x4000C040) /**< (TC0) Channel Control Register (channel = 0) 1 */
#define REG_TC0_CMR1            (0x4000C044) /**< (TC0) Channel Mode Register (channel = 0) 1 */
#define REG_TC0_SMMR1           (0x4000C048) /**< (TC0) Stepper Motor Mode Register (channel = 0) 1 */
#define REG_TC0_RAB1            (0x4000C04C) /**< (TC0) Register AB (channel = 0) 1 */
#define REG_TC0_CV1             (0x4000C050) /**< (TC0) Counter Value (channel = 0) 1 */
#define REG_TC0_RA1             (0x4000C054) /**< (TC0) Register A (channel = 0) 1 */
#define REG_TC0_RB1             (0x4000C058) /**< (TC0) Register B (channel = 0) 1 */
#define REG_TC0_RC1             (0x4000C05C) /**< (TC0) Register C (channel = 0) 1 */
#define REG_TC0_SR1             (0x4000C060) /**< (TC0) Status Register (channel = 0) 1 */
#define REG_TC0_IER1            (0x4000C064) /**< (TC0) Interrupt Enable Register (channel = 0) 1 */
#define REG_TC0_IDR1            (0x4000C068) /**< (TC0) Interrupt Disable Register (channel = 0) 1 */
#define REG_TC0_IMR1            (0x4000C06C) /**< (TC0) Interrupt Mask Register (channel = 0) 1 */
#define REG_TC0_EMR1            (0x4000C070) /**< (TC0) Extended Mode Register (channel = 0) 1 */
#define REG_TC0_CCR2            (0x4000C080) /**< (TC0) Channel Control Register (channel = 0) 2 */
#define REG_TC0_CMR2            (0x4000C084) /**< (TC0) Channel Mode Register (channel = 0) 2 */
#define REG_TC0_SMMR2           (0x4000C088) /**< (TC0) Stepper Motor Mode Register (channel = 0) 2 */
#define REG_TC0_RAB2            (0x4000C08C) /**< (TC0) Register AB (channel = 0) 2 */
#define REG_TC0_CV2             (0x4000C090) /**< (TC0) Counter Value (channel = 0) 2 */
#define REG_TC0_RA2             (0x4000C094) /**< (TC0) Register A (channel = 0) 2 */
#define REG_TC0_RB2             (0x4000C098) /**< (TC0) Register B (channel = 0) 2 */
#define REG_TC0_RC2             (0x4000C09C) /**< (TC0) Register C (channel = 0) 2 */
#define REG_TC0_SR2             (0x4000C0A0) /**< (TC0) Status Register (channel = 0) 2 */
#define REG_TC0_IER2            (0x4000C0A4) /**< (TC0) Interrupt Enable Register (channel = 0) 2 */
#define REG_TC0_IDR2            (0x4000C0A8) /**< (TC0) Interrupt Disable Register (channel = 0) 2 */
#define REG_TC0_IMR2            (0x4000C0AC) /**< (TC0) Interrupt Mask Register (channel = 0) 2 */
#define REG_TC0_EMR2            (0x4000C0B0) /**< (TC0) Extended Mode Register (channel = 0) 2 */
#define REG_TC0_BCR             (0x4000C0C0) /**< (TC0) Block Control Register */
#define REG_TC0_BMR             (0x4000C0C4) /**< (TC0) Block Mode Register */
#define REG_TC0_QIER            (0x4000C0C8) /**< (TC0) QDEC Interrupt Enable Register */
#define REG_TC0_QIDR            (0x4000C0CC) /**< (TC0) QDEC Interrupt Disable Register */
#define REG_TC0_QIMR            (0x4000C0D0) /**< (TC0) QDEC Interrupt Mask Register */
#define REG_TC0_QISR            (0x4000C0D4) /**< (TC0) QDEC Interrupt Status Register */
#define REG_TC0_FMR             (0x4000C0D8) /**< (TC0) Fault Mode Register */
#define REG_TC0_WPMR            (0x4000C0E4) /**< (TC0) Write Protection Mode Register */

#else

#define REG_TC0_CCR0            (*(__O  uint32_t*)0x4000C000U) /**< (TC0) Channel Control Register (channel = 0) 0 */
#define REG_TC0_CMR0            (*(__IO uint32_t*)0x4000C004U) /**< (TC0) Channel Mode Register (channel = 0) 0 */
#define REG_TC0_SMMR0           (*(__IO uint32_t*)0x4000C008U) /**< (TC0) Stepper Motor Mode Register (channel = 0) 0 */
#define REG_TC0_RAB0            (*(__I  uint32_t*)0x4000C00CU) /**< (TC0) Register AB (channel = 0) 0 */
#define REG_TC0_CV0             (*(__I  uint32_t*)0x4000C010U) /**< (TC0) Counter Value (channel = 0) 0 */
#define REG_TC0_RA0             (*(__IO uint32_t*)0x4000C014U) /**< (TC0) Register A (channel = 0) 0 */
#define REG_TC0_RB0             (*(__IO uint32_t*)0x4000C018U) /**< (TC0) Register B (channel = 0) 0 */
#define REG_TC0_RC0             (*(__IO uint32_t*)0x4000C01CU) /**< (TC0) Register C (channel = 0) 0 */
#define REG_TC0_SR0             (*(__I  uint32_t*)0x4000C020U) /**< (TC0) Status Register (channel = 0) 0 */
#define REG_TC0_IER0            (*(__O  uint32_t*)0x4000C024U) /**< (TC0) Interrupt Enable Register (channel = 0) 0 */
#define REG_TC0_IDR0            (*(__O  uint32_t*)0x4000C028U) /**< (TC0) Interrupt Disable Register (channel = 0) 0 */
#define REG_TC0_IMR0            (*(__I  uint32_t*)0x4000C02CU) /**< (TC0) Interrupt Mask Register (channel = 0) 0 */
#define REG_TC0_EMR0            (*(__IO uint32_t*)0x4000C030U) /**< (TC0) Extended Mode Register (channel = 0) 0 */
#define REG_TC0_CCR1            (*(__O  uint32_t*)0x4000C040U) /**< (TC0) Channel Control Register (channel = 0) 1 */
#define REG_TC0_CMR1            (*(__IO uint32_t*)0x4000C044U) /**< (TC0) Channel Mode Register (channel = 0) 1 */
#define REG_TC0_SMMR1           (*(__IO uint32_t*)0x4000C048U) /**< (TC0) Stepper Motor Mode Register (channel = 0) 1 */
#define REG_TC0_RAB1            (*(__I  uint32_t*)0x4000C04CU) /**< (TC0) Register AB (channel = 0) 1 */
#define REG_TC0_CV1             (*(__I  uint32_t*)0x4000C050U) /**< (TC0) Counter Value (channel = 0) 1 */
#define REG_TC0_RA1             (*(__IO uint32_t*)0x4000C054U) /**< (TC0) Register A (channel = 0) 1 */
#define REG_TC0_RB1             (*(__IO uint32_t*)0x4000C058U) /**< (TC0) Register B (channel = 0) 1 */
#define REG_TC0_RC1             (*(__IO uint32_t*)0x4000C05CU) /**< (TC0) Register C (channel = 0) 1 */
#define REG_TC0_SR1             (*(__I  uint32_t*)0x4000C060U) /**< (TC0) Status Register (channel = 0) 1 */
#define REG_TC0_IER1            (*(__O  uint32_t*)0x4000C064U) /**< (TC0) Interrupt Enable Register (channel = 0) 1 */
#define REG_TC0_IDR1            (*(__O  uint32_t*)0x4000C068U) /**< (TC0) Interrupt Disable Register (channel = 0) 1 */
#define REG_TC0_IMR1            (*(__I  uint32_t*)0x4000C06CU) /**< (TC0) Interrupt Mask Register (channel = 0) 1 */
#define REG_TC0_EMR1            (*(__IO uint32_t*)0x4000C070U) /**< (TC0) Extended Mode Register (channel = 0) 1 */
#define REG_TC0_CCR2            (*(__O  uint32_t*)0x4000C080U) /**< (TC0) Channel Control Register (channel = 0) 2 */
#define REG_TC0_CMR2            (*(__IO uint32_t*)0x4000C084U) /**< (TC0) Channel Mode Register (channel = 0) 2 */
#define REG_TC0_SMMR2           (*(__IO uint32_t*)0x4000C088U) /**< (TC0) Stepper Motor Mode Register (channel = 0) 2 */
#define REG_TC0_RAB2            (*(__I  uint32_t*)0x4000C08CU) /**< (TC0) Register AB (channel = 0) 2 */
#define REG_TC0_CV2             (*(__I  uint32_t*)0x4000C090U) /**< (TC0) Counter Value (channel = 0) 2 */
#define REG_TC0_RA2             (*(__IO uint32_t*)0x4000C094U) /**< (TC0) Register A (channel = 0) 2 */
#define REG_TC0_RB2             (*(__IO uint32_t*)0x4000C098U) /**< (TC0) Register B (channel = 0) 2 */
#define REG_TC0_RC2             (*(__IO uint32_t*)0x4000C09CU) /**< (TC0) Register C (channel = 0) 2 */
#define REG_TC0_SR2             (*(__I  uint32_t*)0x4000C0A0U) /**< (TC0) Status Register (channel = 0) 2 */
#define REG_TC0_IER2            (*(__O  uint32_t*)0x4000C0A4U) /**< (TC0) Interrupt Enable Register (channel = 0) 2 */
#define REG_TC0_IDR2            (*(__O  uint32_t*)0x4000C0A8U) /**< (TC0) Interrupt Disable Register (channel = 0) 2 */
#define REG_TC0_IMR2            (*(__I  uint32_t*)0x4000C0ACU) /**< (TC0) Interrupt Mask Register (channel = 0) 2 */
#define REG_TC0_EMR2            (*(__IO uint32_t*)0x4000C0B0U) /**< (TC0) Extended Mode Register (channel = 0) 2 */
#define REG_TC0_BCR             (*(__O  uint32_t*)0x4000C0C0U) /**< (TC0) Block Control Register */
#define REG_TC0_BMR             (*(__IO uint32_t*)0x4000C0C4U) /**< (TC0) Block Mode Register */
#define REG_TC0_QIER            (*(__O  uint32_t*)0x4000C0C8U) /**< (TC0) QDEC Interrupt Enable Register */
#define REG_TC0_QIDR            (*(__O  uint32_t*)0x4000C0CCU) /**< (TC0) QDEC Interrupt Disable Register */
#define REG_TC0_QIMR            (*(__I  uint32_t*)0x4000C0D0U) /**< (TC0) QDEC Interrupt Mask Register */
#define REG_TC0_QISR            (*(__I  uint32_t*)0x4000C0D4U) /**< (TC0) QDEC Interrupt Status Register */
#define REG_TC0_FMR             (*(__IO uint32_t*)0x4000C0D8U) /**< (TC0) Fault Mode Register */
#define REG_TC0_WPMR            (*(__IO uint32_t*)0x4000C0E4U) /**< (TC0) Write Protection Mode Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for TC0 peripheral ========== */
#define TC0_INSTANCE_ID_CHANNEL1                 24        
#define TC0_INSTANCE_ID_CHANNEL0                 23        
#define TC0_INSTANCE_ID_CHANNEL2                 25        
#define TC0_DMAC_ID_RX                           40        

#endif /* _SAME70_TC0_INSTANCE_ */
