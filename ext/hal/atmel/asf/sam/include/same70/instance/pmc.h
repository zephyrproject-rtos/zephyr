/**
 * \file
 *
 * \brief Instance description for PMC
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

#ifndef _SAME70_PMC_INSTANCE_H_
#define _SAME70_PMC_INSTANCE_H_

/* ========== Register definition for PMC peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_PMC_SCER            (0x400E0600) /**< (PMC) System Clock Enable Register */
#define REG_PMC_SCDR            (0x400E0604) /**< (PMC) System Clock Disable Register */
#define REG_PMC_SCSR            (0x400E0608) /**< (PMC) System Clock Status Register */
#define REG_PMC_PCER0           (0x400E0610) /**< (PMC) Peripheral Clock Enable Register 0 */
#define REG_PMC_PCDR0           (0x400E0614) /**< (PMC) Peripheral Clock Disable Register 0 */
#define REG_PMC_PCSR0           (0x400E0618) /**< (PMC) Peripheral Clock Status Register 0 */
#define REG_CKGR_UCKR           (0x400E061C) /**< (PMC) UTMI Clock Register */
#define REG_CKGR_MOR            (0x400E0620) /**< (PMC) Main Oscillator Register */
#define REG_CKGR_MCFR           (0x400E0624) /**< (PMC) Main Clock Frequency Register */
#define REG_CKGR_PLLAR          (0x400E0628) /**< (PMC) PLLA Register */
#define REG_PMC_MCKR            (0x400E0630) /**< (PMC) Master Clock Register */
#define REG_PMC_USB             (0x400E0638) /**< (PMC) USB Clock Register */
#define REG_PMC_PCK             (0x400E0640) /**< (PMC) Programmable Clock 0 Register 0 */
#define REG_PMC_PCK0            (0x400E0640) /**< (PMC) Programmable Clock 0 Register 0 */
#define REG_PMC_PCK1            (0x400E0644) /**< (PMC) Programmable Clock 0 Register 1 */
#define REG_PMC_PCK2            (0x400E0648) /**< (PMC) Programmable Clock 0 Register 2 */
#define REG_PMC_PCK3            (0x400E064C) /**< (PMC) Programmable Clock 0 Register 3 */
#define REG_PMC_PCK4            (0x400E0650) /**< (PMC) Programmable Clock 0 Register 4 */
#define REG_PMC_PCK5            (0x400E0654) /**< (PMC) Programmable Clock 0 Register 5 */
#define REG_PMC_PCK6            (0x400E0658) /**< (PMC) Programmable Clock 0 Register 6 */
#define REG_PMC_PCK7            (0x400E065C) /**< (PMC) Programmable Clock 0 Register 7 */
#define REG_PMC_IER             (0x400E0660) /**< (PMC) Interrupt Enable Register */
#define REG_PMC_IDR             (0x400E0664) /**< (PMC) Interrupt Disable Register */
#define REG_PMC_SR              (0x400E0668) /**< (PMC) Status Register */
#define REG_PMC_IMR             (0x400E066C) /**< (PMC) Interrupt Mask Register */
#define REG_PMC_FSMR            (0x400E0670) /**< (PMC) Fast Startup Mode Register */
#define REG_PMC_FSPR            (0x400E0674) /**< (PMC) Fast Startup Polarity Register */
#define REG_PMC_FOCR            (0x400E0678) /**< (PMC) Fault Output Clear Register */
#define REG_PMC_WPMR            (0x400E06E4) /**< (PMC) Write Protection Mode Register */
#define REG_PMC_WPSR            (0x400E06E8) /**< (PMC) Write Protection Status Register */
#define REG_PMC_PCER1           (0x400E0700) /**< (PMC) Peripheral Clock Enable Register 1 */
#define REG_PMC_PCDR1           (0x400E0704) /**< (PMC) Peripheral Clock Disable Register 1 */
#define REG_PMC_PCSR1           (0x400E0708) /**< (PMC) Peripheral Clock Status Register 1 */
#define REG_PMC_PCR             (0x400E070C) /**< (PMC) Peripheral Control Register */
#define REG_PMC_OCR             (0x400E0710) /**< (PMC) Oscillator Calibration Register */
#define REG_PMC_SLPWK_ER0       (0x400E0714) /**< (PMC) SleepWalking Enable Register 0 */
#define REG_PMC_SLPWK_DR0       (0x400E0718) /**< (PMC) SleepWalking Disable Register 0 */
#define REG_PMC_SLPWK_SR0       (0x400E071C) /**< (PMC) SleepWalking Status Register 0 */
#define REG_PMC_SLPWK_ASR0      (0x400E0720) /**< (PMC) SleepWalking Activity Status Register 0 */
#define REG_PMC_PMMR            (0x400E0730) /**< (PMC) PLL Maximum Multiplier Value Register */
#define REG_PMC_SLPWK_ER1       (0x400E0734) /**< (PMC) SleepWalking Enable Register 1 */
#define REG_PMC_SLPWK_DR1       (0x400E0738) /**< (PMC) SleepWalking Disable Register 1 */
#define REG_PMC_SLPWK_SR1       (0x400E073C) /**< (PMC) SleepWalking Status Register 1 */
#define REG_PMC_SLPWK_ASR1      (0x400E0740) /**< (PMC) SleepWalking Activity Status Register 1 */
#define REG_PMC_SLPWK_AIPR      (0x400E0744) /**< (PMC) SleepWalking Activity In Progress Register */

#else

#define REG_PMC_SCER            (*(__O  uint32_t*)0x400E0600U) /**< (PMC) System Clock Enable Register */
#define REG_PMC_SCDR            (*(__O  uint32_t*)0x400E0604U) /**< (PMC) System Clock Disable Register */
#define REG_PMC_SCSR            (*(__I  uint32_t*)0x400E0608U) /**< (PMC) System Clock Status Register */
#define REG_PMC_PCER0           (*(__O  uint32_t*)0x400E0610U) /**< (PMC) Peripheral Clock Enable Register 0 */
#define REG_PMC_PCDR0           (*(__O  uint32_t*)0x400E0614U) /**< (PMC) Peripheral Clock Disable Register 0 */
#define REG_PMC_PCSR0           (*(__I  uint32_t*)0x400E0618U) /**< (PMC) Peripheral Clock Status Register 0 */
#define REG_CKGR_UCKR           (*(__IO uint32_t*)0x400E061CU) /**< (PMC) UTMI Clock Register */
#define REG_CKGR_MOR            (*(__IO uint32_t*)0x400E0620U) /**< (PMC) Main Oscillator Register */
#define REG_CKGR_MCFR           (*(__IO uint32_t*)0x400E0624U) /**< (PMC) Main Clock Frequency Register */
#define REG_CKGR_PLLAR          (*(__IO uint32_t*)0x400E0628U) /**< (PMC) PLLA Register */
#define REG_PMC_MCKR            (*(__IO uint32_t*)0x400E0630U) /**< (PMC) Master Clock Register */
#define REG_PMC_USB             (*(__IO uint32_t*)0x400E0638U) /**< (PMC) USB Clock Register */
#define REG_PMC_PCK             (*(__IO uint32_t*)0x400E0640U) /**< (PMC) Programmable Clock 0 Register 0 */
#define REG_PMC_PCK0            (*(__IO uint32_t*)0x400E0640U) /**< (PMC) Programmable Clock 0 Register 0 */
#define REG_PMC_PCK1            (*(__IO uint32_t*)0x400E0644U) /**< (PMC) Programmable Clock 0 Register 1 */
#define REG_PMC_PCK2            (*(__IO uint32_t*)0x400E0648U) /**< (PMC) Programmable Clock 0 Register 2 */
#define REG_PMC_PCK3            (*(__IO uint32_t*)0x400E064CU) /**< (PMC) Programmable Clock 0 Register 3 */
#define REG_PMC_PCK4            (*(__IO uint32_t*)0x400E0650U) /**< (PMC) Programmable Clock 0 Register 4 */
#define REG_PMC_PCK5            (*(__IO uint32_t*)0x400E0654U) /**< (PMC) Programmable Clock 0 Register 5 */
#define REG_PMC_PCK6            (*(__IO uint32_t*)0x400E0658U) /**< (PMC) Programmable Clock 0 Register 6 */
#define REG_PMC_PCK7            (*(__IO uint32_t*)0x400E065CU) /**< (PMC) Programmable Clock 0 Register 7 */
#define REG_PMC_IER             (*(__O  uint32_t*)0x400E0660U) /**< (PMC) Interrupt Enable Register */
#define REG_PMC_IDR             (*(__O  uint32_t*)0x400E0664U) /**< (PMC) Interrupt Disable Register */
#define REG_PMC_SR              (*(__I  uint32_t*)0x400E0668U) /**< (PMC) Status Register */
#define REG_PMC_IMR             (*(__I  uint32_t*)0x400E066CU) /**< (PMC) Interrupt Mask Register */
#define REG_PMC_FSMR            (*(__IO uint32_t*)0x400E0670U) /**< (PMC) Fast Startup Mode Register */
#define REG_PMC_FSPR            (*(__IO uint32_t*)0x400E0674U) /**< (PMC) Fast Startup Polarity Register */
#define REG_PMC_FOCR            (*(__O  uint32_t*)0x400E0678U) /**< (PMC) Fault Output Clear Register */
#define REG_PMC_WPMR            (*(__IO uint32_t*)0x400E06E4U) /**< (PMC) Write Protection Mode Register */
#define REG_PMC_WPSR            (*(__I  uint32_t*)0x400E06E8U) /**< (PMC) Write Protection Status Register */
#define REG_PMC_PCER1           (*(__O  uint32_t*)0x400E0700U) /**< (PMC) Peripheral Clock Enable Register 1 */
#define REG_PMC_PCDR1           (*(__O  uint32_t*)0x400E0704U) /**< (PMC) Peripheral Clock Disable Register 1 */
#define REG_PMC_PCSR1           (*(__I  uint32_t*)0x400E0708U) /**< (PMC) Peripheral Clock Status Register 1 */
#define REG_PMC_PCR             (*(__IO uint32_t*)0x400E070CU) /**< (PMC) Peripheral Control Register */
#define REG_PMC_OCR             (*(__IO uint32_t*)0x400E0710U) /**< (PMC) Oscillator Calibration Register */
#define REG_PMC_SLPWK_ER0       (*(__O  uint32_t*)0x400E0714U) /**< (PMC) SleepWalking Enable Register 0 */
#define REG_PMC_SLPWK_DR0       (*(__O  uint32_t*)0x400E0718U) /**< (PMC) SleepWalking Disable Register 0 */
#define REG_PMC_SLPWK_SR0       (*(__I  uint32_t*)0x400E071CU) /**< (PMC) SleepWalking Status Register 0 */
#define REG_PMC_SLPWK_ASR0      (*(__I  uint32_t*)0x400E0720U) /**< (PMC) SleepWalking Activity Status Register 0 */
#define REG_PMC_PMMR            (*(__IO uint32_t*)0x400E0730U) /**< (PMC) PLL Maximum Multiplier Value Register */
#define REG_PMC_SLPWK_ER1       (*(__O  uint32_t*)0x400E0734U) /**< (PMC) SleepWalking Enable Register 1 */
#define REG_PMC_SLPWK_DR1       (*(__O  uint32_t*)0x400E0738U) /**< (PMC) SleepWalking Disable Register 1 */
#define REG_PMC_SLPWK_SR1       (*(__I  uint32_t*)0x400E073CU) /**< (PMC) SleepWalking Status Register 1 */
#define REG_PMC_SLPWK_ASR1      (*(__I  uint32_t*)0x400E0740U) /**< (PMC) SleepWalking Activity Status Register 1 */
#define REG_PMC_SLPWK_AIPR      (*(__I  uint32_t*)0x400E0744U) /**< (PMC) SleepWalking Activity In Progress Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for PMC peripheral ========== */
#define PMC_INSTANCE_ID                          5         

#endif /* _SAME70_PMC_INSTANCE_ */
