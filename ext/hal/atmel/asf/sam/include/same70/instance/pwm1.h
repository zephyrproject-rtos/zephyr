/**
 * \file
 *
 * \brief Instance description for PWM1
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

#ifndef _SAME70_PWM1_INSTANCE_H_
#define _SAME70_PWM1_INSTANCE_H_

/* ========== Register definition for PWM1 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_PWM1_CMPV0          (0x4005C130) /**< (PWM1) PWM Comparison 0 Value Register 0 */
#define REG_PWM1_CMPVUPD0       (0x4005C134) /**< (PWM1) PWM Comparison 0 Value Update Register 0 */
#define REG_PWM1_CMPM0          (0x4005C138) /**< (PWM1) PWM Comparison 0 Mode Register 0 */
#define REG_PWM1_CMPMUPD0       (0x4005C13C) /**< (PWM1) PWM Comparison 0 Mode Update Register 0 */
#define REG_PWM1_CMPV1          (0x4005C140) /**< (PWM1) PWM Comparison 0 Value Register 1 */
#define REG_PWM1_CMPVUPD1       (0x4005C144) /**< (PWM1) PWM Comparison 0 Value Update Register 1 */
#define REG_PWM1_CMPM1          (0x4005C148) /**< (PWM1) PWM Comparison 0 Mode Register 1 */
#define REG_PWM1_CMPMUPD1       (0x4005C14C) /**< (PWM1) PWM Comparison 0 Mode Update Register 1 */
#define REG_PWM1_CMPV2          (0x4005C150) /**< (PWM1) PWM Comparison 0 Value Register 2 */
#define REG_PWM1_CMPVUPD2       (0x4005C154) /**< (PWM1) PWM Comparison 0 Value Update Register 2 */
#define REG_PWM1_CMPM2          (0x4005C158) /**< (PWM1) PWM Comparison 0 Mode Register 2 */
#define REG_PWM1_CMPMUPD2       (0x4005C15C) /**< (PWM1) PWM Comparison 0 Mode Update Register 2 */
#define REG_PWM1_CMPV3          (0x4005C160) /**< (PWM1) PWM Comparison 0 Value Register 3 */
#define REG_PWM1_CMPVUPD3       (0x4005C164) /**< (PWM1) PWM Comparison 0 Value Update Register 3 */
#define REG_PWM1_CMPM3          (0x4005C168) /**< (PWM1) PWM Comparison 0 Mode Register 3 */
#define REG_PWM1_CMPMUPD3       (0x4005C16C) /**< (PWM1) PWM Comparison 0 Mode Update Register 3 */
#define REG_PWM1_CMPV4          (0x4005C170) /**< (PWM1) PWM Comparison 0 Value Register 4 */
#define REG_PWM1_CMPVUPD4       (0x4005C174) /**< (PWM1) PWM Comparison 0 Value Update Register 4 */
#define REG_PWM1_CMPM4          (0x4005C178) /**< (PWM1) PWM Comparison 0 Mode Register 4 */
#define REG_PWM1_CMPMUPD4       (0x4005C17C) /**< (PWM1) PWM Comparison 0 Mode Update Register 4 */
#define REG_PWM1_CMPV5          (0x4005C180) /**< (PWM1) PWM Comparison 0 Value Register 5 */
#define REG_PWM1_CMPVUPD5       (0x4005C184) /**< (PWM1) PWM Comparison 0 Value Update Register 5 */
#define REG_PWM1_CMPM5          (0x4005C188) /**< (PWM1) PWM Comparison 0 Mode Register 5 */
#define REG_PWM1_CMPMUPD5       (0x4005C18C) /**< (PWM1) PWM Comparison 0 Mode Update Register 5 */
#define REG_PWM1_CMPV6          (0x4005C190) /**< (PWM1) PWM Comparison 0 Value Register 6 */
#define REG_PWM1_CMPVUPD6       (0x4005C194) /**< (PWM1) PWM Comparison 0 Value Update Register 6 */
#define REG_PWM1_CMPM6          (0x4005C198) /**< (PWM1) PWM Comparison 0 Mode Register 6 */
#define REG_PWM1_CMPMUPD6       (0x4005C19C) /**< (PWM1) PWM Comparison 0 Mode Update Register 6 */
#define REG_PWM1_CMPV7          (0x4005C1A0) /**< (PWM1) PWM Comparison 0 Value Register 7 */
#define REG_PWM1_CMPVUPD7       (0x4005C1A4) /**< (PWM1) PWM Comparison 0 Value Update Register 7 */
#define REG_PWM1_CMPM7          (0x4005C1A8) /**< (PWM1) PWM Comparison 0 Mode Register 7 */
#define REG_PWM1_CMPMUPD7       (0x4005C1AC) /**< (PWM1) PWM Comparison 0 Mode Update Register 7 */
#define REG_PWM1_CMR0           (0x4005C200) /**< (PWM1) PWM Channel Mode Register (ch_num = 0) 0 */
#define REG_PWM1_CDTY0          (0x4005C204) /**< (PWM1) PWM Channel Duty Cycle Register (ch_num = 0) 0 */
#define REG_PWM1_CDTYUPD0       (0x4005C208) /**< (PWM1) PWM Channel Duty Cycle Update Register (ch_num = 0) 0 */
#define REG_PWM1_CPRD0          (0x4005C20C) /**< (PWM1) PWM Channel Period Register (ch_num = 0) 0 */
#define REG_PWM1_CPRDUPD0       (0x4005C210) /**< (PWM1) PWM Channel Period Update Register (ch_num = 0) 0 */
#define REG_PWM1_CCNT0          (0x4005C214) /**< (PWM1) PWM Channel Counter Register (ch_num = 0) 0 */
#define REG_PWM1_DT0            (0x4005C218) /**< (PWM1) PWM Channel Dead Time Register (ch_num = 0) 0 */
#define REG_PWM1_DTUPD0         (0x4005C21C) /**< (PWM1) PWM Channel Dead Time Update Register (ch_num = 0) 0 */
#define REG_PWM1_CMR1           (0x4005C220) /**< (PWM1) PWM Channel Mode Register (ch_num = 0) 1 */
#define REG_PWM1_CDTY1          (0x4005C224) /**< (PWM1) PWM Channel Duty Cycle Register (ch_num = 0) 1 */
#define REG_PWM1_CDTYUPD1       (0x4005C228) /**< (PWM1) PWM Channel Duty Cycle Update Register (ch_num = 0) 1 */
#define REG_PWM1_CPRD1          (0x4005C22C) /**< (PWM1) PWM Channel Period Register (ch_num = 0) 1 */
#define REG_PWM1_CPRDUPD1       (0x4005C230) /**< (PWM1) PWM Channel Period Update Register (ch_num = 0) 1 */
#define REG_PWM1_CCNT1          (0x4005C234) /**< (PWM1) PWM Channel Counter Register (ch_num = 0) 1 */
#define REG_PWM1_DT1            (0x4005C238) /**< (PWM1) PWM Channel Dead Time Register (ch_num = 0) 1 */
#define REG_PWM1_DTUPD1         (0x4005C23C) /**< (PWM1) PWM Channel Dead Time Update Register (ch_num = 0) 1 */
#define REG_PWM1_CMR2           (0x4005C240) /**< (PWM1) PWM Channel Mode Register (ch_num = 0) 2 */
#define REG_PWM1_CDTY2          (0x4005C244) /**< (PWM1) PWM Channel Duty Cycle Register (ch_num = 0) 2 */
#define REG_PWM1_CDTYUPD2       (0x4005C248) /**< (PWM1) PWM Channel Duty Cycle Update Register (ch_num = 0) 2 */
#define REG_PWM1_CPRD2          (0x4005C24C) /**< (PWM1) PWM Channel Period Register (ch_num = 0) 2 */
#define REG_PWM1_CPRDUPD2       (0x4005C250) /**< (PWM1) PWM Channel Period Update Register (ch_num = 0) 2 */
#define REG_PWM1_CCNT2          (0x4005C254) /**< (PWM1) PWM Channel Counter Register (ch_num = 0) 2 */
#define REG_PWM1_DT2            (0x4005C258) /**< (PWM1) PWM Channel Dead Time Register (ch_num = 0) 2 */
#define REG_PWM1_DTUPD2         (0x4005C25C) /**< (PWM1) PWM Channel Dead Time Update Register (ch_num = 0) 2 */
#define REG_PWM1_CMR3           (0x4005C260) /**< (PWM1) PWM Channel Mode Register (ch_num = 0) 3 */
#define REG_PWM1_CDTY3          (0x4005C264) /**< (PWM1) PWM Channel Duty Cycle Register (ch_num = 0) 3 */
#define REG_PWM1_CDTYUPD3       (0x4005C268) /**< (PWM1) PWM Channel Duty Cycle Update Register (ch_num = 0) 3 */
#define REG_PWM1_CPRD3          (0x4005C26C) /**< (PWM1) PWM Channel Period Register (ch_num = 0) 3 */
#define REG_PWM1_CPRDUPD3       (0x4005C270) /**< (PWM1) PWM Channel Period Update Register (ch_num = 0) 3 */
#define REG_PWM1_CCNT3          (0x4005C274) /**< (PWM1) PWM Channel Counter Register (ch_num = 0) 3 */
#define REG_PWM1_DT3            (0x4005C278) /**< (PWM1) PWM Channel Dead Time Register (ch_num = 0) 3 */
#define REG_PWM1_DTUPD3         (0x4005C27C) /**< (PWM1) PWM Channel Dead Time Update Register (ch_num = 0) 3 */
#define REG_PWM1_CLK            (0x4005C000) /**< (PWM1) PWM Clock Register */
#define REG_PWM1_ENA            (0x4005C004) /**< (PWM1) PWM Enable Register */
#define REG_PWM1_DIS            (0x4005C008) /**< (PWM1) PWM Disable Register */
#define REG_PWM1_SR             (0x4005C00C) /**< (PWM1) PWM Status Register */
#define REG_PWM1_IER1           (0x4005C010) /**< (PWM1) PWM Interrupt Enable Register 1 */
#define REG_PWM1_IDR1           (0x4005C014) /**< (PWM1) PWM Interrupt Disable Register 1 */
#define REG_PWM1_IMR1           (0x4005C018) /**< (PWM1) PWM Interrupt Mask Register 1 */
#define REG_PWM1_ISR1           (0x4005C01C) /**< (PWM1) PWM Interrupt Status Register 1 */
#define REG_PWM1_SCM            (0x4005C020) /**< (PWM1) PWM Sync Channels Mode Register */
#define REG_PWM1_DMAR           (0x4005C024) /**< (PWM1) PWM DMA Register */
#define REG_PWM1_SCUC           (0x4005C028) /**< (PWM1) PWM Sync Channels Update Control Register */
#define REG_PWM1_SCUP           (0x4005C02C) /**< (PWM1) PWM Sync Channels Update Period Register */
#define REG_PWM1_SCUPUPD        (0x4005C030) /**< (PWM1) PWM Sync Channels Update Period Update Register */
#define REG_PWM1_IER2           (0x4005C034) /**< (PWM1) PWM Interrupt Enable Register 2 */
#define REG_PWM1_IDR2           (0x4005C038) /**< (PWM1) PWM Interrupt Disable Register 2 */
#define REG_PWM1_IMR2           (0x4005C03C) /**< (PWM1) PWM Interrupt Mask Register 2 */
#define REG_PWM1_ISR2           (0x4005C040) /**< (PWM1) PWM Interrupt Status Register 2 */
#define REG_PWM1_OOV            (0x4005C044) /**< (PWM1) PWM Output Override Value Register */
#define REG_PWM1_OS             (0x4005C048) /**< (PWM1) PWM Output Selection Register */
#define REG_PWM1_OSS            (0x4005C04C) /**< (PWM1) PWM Output Selection Set Register */
#define REG_PWM1_OSC            (0x4005C050) /**< (PWM1) PWM Output Selection Clear Register */
#define REG_PWM1_OSSUPD         (0x4005C054) /**< (PWM1) PWM Output Selection Set Update Register */
#define REG_PWM1_OSCUPD         (0x4005C058) /**< (PWM1) PWM Output Selection Clear Update Register */
#define REG_PWM1_FMR            (0x4005C05C) /**< (PWM1) PWM Fault Mode Register */
#define REG_PWM1_FSR            (0x4005C060) /**< (PWM1) PWM Fault Status Register */
#define REG_PWM1_FCR            (0x4005C064) /**< (PWM1) PWM Fault Clear Register */
#define REG_PWM1_FPV1           (0x4005C068) /**< (PWM1) PWM Fault Protection Value Register 1 */
#define REG_PWM1_FPE            (0x4005C06C) /**< (PWM1) PWM Fault Protection Enable Register */
#define REG_PWM1_ELMR           (0x4005C07C) /**< (PWM1) PWM Event Line 0 Mode Register 0 */
#define REG_PWM1_ELMR0          (0x4005C07C) /**< (PWM1) PWM Event Line 0 Mode Register 0 */
#define REG_PWM1_ELMR1          (0x4005C080) /**< (PWM1) PWM Event Line 0 Mode Register 1 */
#define REG_PWM1_SSPR           (0x4005C0A0) /**< (PWM1) PWM Spread Spectrum Register */
#define REG_PWM1_SSPUP          (0x4005C0A4) /**< (PWM1) PWM Spread Spectrum Update Register */
#define REG_PWM1_SMMR           (0x4005C0B0) /**< (PWM1) PWM Stepper Motor Mode Register */
#define REG_PWM1_FPV2           (0x4005C0C0) /**< (PWM1) PWM Fault Protection Value 2 Register */
#define REG_PWM1_WPCR           (0x4005C0E4) /**< (PWM1) PWM Write Protection Control Register */
#define REG_PWM1_WPSR           (0x4005C0E8) /**< (PWM1) PWM Write Protection Status Register */
#define REG_PWM1_CMUPD0         (0x4005C400) /**< (PWM1) PWM Channel Mode Update Register (ch_num = 0) */
#define REG_PWM1_CMUPD1         (0x4005C420) /**< (PWM1) PWM Channel Mode Update Register (ch_num = 1) */
#define REG_PWM1_ETRG1          (0x4005C42C) /**< (PWM1) PWM External Trigger Register (trg_num = 1) */
#define REG_PWM1_LEBR1          (0x4005C430) /**< (PWM1) PWM Leading-Edge Blanking Register (trg_num = 1) */
#define REG_PWM1_CMUPD2         (0x4005C440) /**< (PWM1) PWM Channel Mode Update Register (ch_num = 2) */
#define REG_PWM1_ETRG2          (0x4005C44C) /**< (PWM1) PWM External Trigger Register (trg_num = 2) */
#define REG_PWM1_LEBR2          (0x4005C450) /**< (PWM1) PWM Leading-Edge Blanking Register (trg_num = 2) */
#define REG_PWM1_CMUPD3         (0x4005C460) /**< (PWM1) PWM Channel Mode Update Register (ch_num = 3) */

#else

#define REG_PWM1_CMPV0          (*(__IO uint32_t*)0x4005C130U) /**< (PWM1) PWM Comparison 0 Value Register 0 */
#define REG_PWM1_CMPVUPD0       (*(__O  uint32_t*)0x4005C134U) /**< (PWM1) PWM Comparison 0 Value Update Register 0 */
#define REG_PWM1_CMPM0          (*(__IO uint32_t*)0x4005C138U) /**< (PWM1) PWM Comparison 0 Mode Register 0 */
#define REG_PWM1_CMPMUPD0       (*(__O  uint32_t*)0x4005C13CU) /**< (PWM1) PWM Comparison 0 Mode Update Register 0 */
#define REG_PWM1_CMPV1          (*(__IO uint32_t*)0x4005C140U) /**< (PWM1) PWM Comparison 0 Value Register 1 */
#define REG_PWM1_CMPVUPD1       (*(__O  uint32_t*)0x4005C144U) /**< (PWM1) PWM Comparison 0 Value Update Register 1 */
#define REG_PWM1_CMPM1          (*(__IO uint32_t*)0x4005C148U) /**< (PWM1) PWM Comparison 0 Mode Register 1 */
#define REG_PWM1_CMPMUPD1       (*(__O  uint32_t*)0x4005C14CU) /**< (PWM1) PWM Comparison 0 Mode Update Register 1 */
#define REG_PWM1_CMPV2          (*(__IO uint32_t*)0x4005C150U) /**< (PWM1) PWM Comparison 0 Value Register 2 */
#define REG_PWM1_CMPVUPD2       (*(__O  uint32_t*)0x4005C154U) /**< (PWM1) PWM Comparison 0 Value Update Register 2 */
#define REG_PWM1_CMPM2          (*(__IO uint32_t*)0x4005C158U) /**< (PWM1) PWM Comparison 0 Mode Register 2 */
#define REG_PWM1_CMPMUPD2       (*(__O  uint32_t*)0x4005C15CU) /**< (PWM1) PWM Comparison 0 Mode Update Register 2 */
#define REG_PWM1_CMPV3          (*(__IO uint32_t*)0x4005C160U) /**< (PWM1) PWM Comparison 0 Value Register 3 */
#define REG_PWM1_CMPVUPD3       (*(__O  uint32_t*)0x4005C164U) /**< (PWM1) PWM Comparison 0 Value Update Register 3 */
#define REG_PWM1_CMPM3          (*(__IO uint32_t*)0x4005C168U) /**< (PWM1) PWM Comparison 0 Mode Register 3 */
#define REG_PWM1_CMPMUPD3       (*(__O  uint32_t*)0x4005C16CU) /**< (PWM1) PWM Comparison 0 Mode Update Register 3 */
#define REG_PWM1_CMPV4          (*(__IO uint32_t*)0x4005C170U) /**< (PWM1) PWM Comparison 0 Value Register 4 */
#define REG_PWM1_CMPVUPD4       (*(__O  uint32_t*)0x4005C174U) /**< (PWM1) PWM Comparison 0 Value Update Register 4 */
#define REG_PWM1_CMPM4          (*(__IO uint32_t*)0x4005C178U) /**< (PWM1) PWM Comparison 0 Mode Register 4 */
#define REG_PWM1_CMPMUPD4       (*(__O  uint32_t*)0x4005C17CU) /**< (PWM1) PWM Comparison 0 Mode Update Register 4 */
#define REG_PWM1_CMPV5          (*(__IO uint32_t*)0x4005C180U) /**< (PWM1) PWM Comparison 0 Value Register 5 */
#define REG_PWM1_CMPVUPD5       (*(__O  uint32_t*)0x4005C184U) /**< (PWM1) PWM Comparison 0 Value Update Register 5 */
#define REG_PWM1_CMPM5          (*(__IO uint32_t*)0x4005C188U) /**< (PWM1) PWM Comparison 0 Mode Register 5 */
#define REG_PWM1_CMPMUPD5       (*(__O  uint32_t*)0x4005C18CU) /**< (PWM1) PWM Comparison 0 Mode Update Register 5 */
#define REG_PWM1_CMPV6          (*(__IO uint32_t*)0x4005C190U) /**< (PWM1) PWM Comparison 0 Value Register 6 */
#define REG_PWM1_CMPVUPD6       (*(__O  uint32_t*)0x4005C194U) /**< (PWM1) PWM Comparison 0 Value Update Register 6 */
#define REG_PWM1_CMPM6          (*(__IO uint32_t*)0x4005C198U) /**< (PWM1) PWM Comparison 0 Mode Register 6 */
#define REG_PWM1_CMPMUPD6       (*(__O  uint32_t*)0x4005C19CU) /**< (PWM1) PWM Comparison 0 Mode Update Register 6 */
#define REG_PWM1_CMPV7          (*(__IO uint32_t*)0x4005C1A0U) /**< (PWM1) PWM Comparison 0 Value Register 7 */
#define REG_PWM1_CMPVUPD7       (*(__O  uint32_t*)0x4005C1A4U) /**< (PWM1) PWM Comparison 0 Value Update Register 7 */
#define REG_PWM1_CMPM7          (*(__IO uint32_t*)0x4005C1A8U) /**< (PWM1) PWM Comparison 0 Mode Register 7 */
#define REG_PWM1_CMPMUPD7       (*(__O  uint32_t*)0x4005C1ACU) /**< (PWM1) PWM Comparison 0 Mode Update Register 7 */
#define REG_PWM1_CMR0           (*(__IO uint32_t*)0x4005C200U) /**< (PWM1) PWM Channel Mode Register (ch_num = 0) 0 */
#define REG_PWM1_CDTY0          (*(__IO uint32_t*)0x4005C204U) /**< (PWM1) PWM Channel Duty Cycle Register (ch_num = 0) 0 */
#define REG_PWM1_CDTYUPD0       (*(__O  uint32_t*)0x4005C208U) /**< (PWM1) PWM Channel Duty Cycle Update Register (ch_num = 0) 0 */
#define REG_PWM1_CPRD0          (*(__IO uint32_t*)0x4005C20CU) /**< (PWM1) PWM Channel Period Register (ch_num = 0) 0 */
#define REG_PWM1_CPRDUPD0       (*(__O  uint32_t*)0x4005C210U) /**< (PWM1) PWM Channel Period Update Register (ch_num = 0) 0 */
#define REG_PWM1_CCNT0          (*(__I  uint32_t*)0x4005C214U) /**< (PWM1) PWM Channel Counter Register (ch_num = 0) 0 */
#define REG_PWM1_DT0            (*(__IO uint32_t*)0x4005C218U) /**< (PWM1) PWM Channel Dead Time Register (ch_num = 0) 0 */
#define REG_PWM1_DTUPD0         (*(__O  uint32_t*)0x4005C21CU) /**< (PWM1) PWM Channel Dead Time Update Register (ch_num = 0) 0 */
#define REG_PWM1_CMR1           (*(__IO uint32_t*)0x4005C220U) /**< (PWM1) PWM Channel Mode Register (ch_num = 0) 1 */
#define REG_PWM1_CDTY1          (*(__IO uint32_t*)0x4005C224U) /**< (PWM1) PWM Channel Duty Cycle Register (ch_num = 0) 1 */
#define REG_PWM1_CDTYUPD1       (*(__O  uint32_t*)0x4005C228U) /**< (PWM1) PWM Channel Duty Cycle Update Register (ch_num = 0) 1 */
#define REG_PWM1_CPRD1          (*(__IO uint32_t*)0x4005C22CU) /**< (PWM1) PWM Channel Period Register (ch_num = 0) 1 */
#define REG_PWM1_CPRDUPD1       (*(__O  uint32_t*)0x4005C230U) /**< (PWM1) PWM Channel Period Update Register (ch_num = 0) 1 */
#define REG_PWM1_CCNT1          (*(__I  uint32_t*)0x4005C234U) /**< (PWM1) PWM Channel Counter Register (ch_num = 0) 1 */
#define REG_PWM1_DT1            (*(__IO uint32_t*)0x4005C238U) /**< (PWM1) PWM Channel Dead Time Register (ch_num = 0) 1 */
#define REG_PWM1_DTUPD1         (*(__O  uint32_t*)0x4005C23CU) /**< (PWM1) PWM Channel Dead Time Update Register (ch_num = 0) 1 */
#define REG_PWM1_CMR2           (*(__IO uint32_t*)0x4005C240U) /**< (PWM1) PWM Channel Mode Register (ch_num = 0) 2 */
#define REG_PWM1_CDTY2          (*(__IO uint32_t*)0x4005C244U) /**< (PWM1) PWM Channel Duty Cycle Register (ch_num = 0) 2 */
#define REG_PWM1_CDTYUPD2       (*(__O  uint32_t*)0x4005C248U) /**< (PWM1) PWM Channel Duty Cycle Update Register (ch_num = 0) 2 */
#define REG_PWM1_CPRD2          (*(__IO uint32_t*)0x4005C24CU) /**< (PWM1) PWM Channel Period Register (ch_num = 0) 2 */
#define REG_PWM1_CPRDUPD2       (*(__O  uint32_t*)0x4005C250U) /**< (PWM1) PWM Channel Period Update Register (ch_num = 0) 2 */
#define REG_PWM1_CCNT2          (*(__I  uint32_t*)0x4005C254U) /**< (PWM1) PWM Channel Counter Register (ch_num = 0) 2 */
#define REG_PWM1_DT2            (*(__IO uint32_t*)0x4005C258U) /**< (PWM1) PWM Channel Dead Time Register (ch_num = 0) 2 */
#define REG_PWM1_DTUPD2         (*(__O  uint32_t*)0x4005C25CU) /**< (PWM1) PWM Channel Dead Time Update Register (ch_num = 0) 2 */
#define REG_PWM1_CMR3           (*(__IO uint32_t*)0x4005C260U) /**< (PWM1) PWM Channel Mode Register (ch_num = 0) 3 */
#define REG_PWM1_CDTY3          (*(__IO uint32_t*)0x4005C264U) /**< (PWM1) PWM Channel Duty Cycle Register (ch_num = 0) 3 */
#define REG_PWM1_CDTYUPD3       (*(__O  uint32_t*)0x4005C268U) /**< (PWM1) PWM Channel Duty Cycle Update Register (ch_num = 0) 3 */
#define REG_PWM1_CPRD3          (*(__IO uint32_t*)0x4005C26CU) /**< (PWM1) PWM Channel Period Register (ch_num = 0) 3 */
#define REG_PWM1_CPRDUPD3       (*(__O  uint32_t*)0x4005C270U) /**< (PWM1) PWM Channel Period Update Register (ch_num = 0) 3 */
#define REG_PWM1_CCNT3          (*(__I  uint32_t*)0x4005C274U) /**< (PWM1) PWM Channel Counter Register (ch_num = 0) 3 */
#define REG_PWM1_DT3            (*(__IO uint32_t*)0x4005C278U) /**< (PWM1) PWM Channel Dead Time Register (ch_num = 0) 3 */
#define REG_PWM1_DTUPD3         (*(__O  uint32_t*)0x4005C27CU) /**< (PWM1) PWM Channel Dead Time Update Register (ch_num = 0) 3 */
#define REG_PWM1_CLK            (*(__IO uint32_t*)0x4005C000U) /**< (PWM1) PWM Clock Register */
#define REG_PWM1_ENA            (*(__O  uint32_t*)0x4005C004U) /**< (PWM1) PWM Enable Register */
#define REG_PWM1_DIS            (*(__O  uint32_t*)0x4005C008U) /**< (PWM1) PWM Disable Register */
#define REG_PWM1_SR             (*(__I  uint32_t*)0x4005C00CU) /**< (PWM1) PWM Status Register */
#define REG_PWM1_IER1           (*(__O  uint32_t*)0x4005C010U) /**< (PWM1) PWM Interrupt Enable Register 1 */
#define REG_PWM1_IDR1           (*(__O  uint32_t*)0x4005C014U) /**< (PWM1) PWM Interrupt Disable Register 1 */
#define REG_PWM1_IMR1           (*(__I  uint32_t*)0x4005C018U) /**< (PWM1) PWM Interrupt Mask Register 1 */
#define REG_PWM1_ISR1           (*(__I  uint32_t*)0x4005C01CU) /**< (PWM1) PWM Interrupt Status Register 1 */
#define REG_PWM1_SCM            (*(__IO uint32_t*)0x4005C020U) /**< (PWM1) PWM Sync Channels Mode Register */
#define REG_PWM1_DMAR           (*(__O  uint32_t*)0x4005C024U) /**< (PWM1) PWM DMA Register */
#define REG_PWM1_SCUC           (*(__IO uint32_t*)0x4005C028U) /**< (PWM1) PWM Sync Channels Update Control Register */
#define REG_PWM1_SCUP           (*(__IO uint32_t*)0x4005C02CU) /**< (PWM1) PWM Sync Channels Update Period Register */
#define REG_PWM1_SCUPUPD        (*(__O  uint32_t*)0x4005C030U) /**< (PWM1) PWM Sync Channels Update Period Update Register */
#define REG_PWM1_IER2           (*(__O  uint32_t*)0x4005C034U) /**< (PWM1) PWM Interrupt Enable Register 2 */
#define REG_PWM1_IDR2           (*(__O  uint32_t*)0x4005C038U) /**< (PWM1) PWM Interrupt Disable Register 2 */
#define REG_PWM1_IMR2           (*(__I  uint32_t*)0x4005C03CU) /**< (PWM1) PWM Interrupt Mask Register 2 */
#define REG_PWM1_ISR2           (*(__I  uint32_t*)0x4005C040U) /**< (PWM1) PWM Interrupt Status Register 2 */
#define REG_PWM1_OOV            (*(__IO uint32_t*)0x4005C044U) /**< (PWM1) PWM Output Override Value Register */
#define REG_PWM1_OS             (*(__IO uint32_t*)0x4005C048U) /**< (PWM1) PWM Output Selection Register */
#define REG_PWM1_OSS            (*(__O  uint32_t*)0x4005C04CU) /**< (PWM1) PWM Output Selection Set Register */
#define REG_PWM1_OSC            (*(__O  uint32_t*)0x4005C050U) /**< (PWM1) PWM Output Selection Clear Register */
#define REG_PWM1_OSSUPD         (*(__O  uint32_t*)0x4005C054U) /**< (PWM1) PWM Output Selection Set Update Register */
#define REG_PWM1_OSCUPD         (*(__O  uint32_t*)0x4005C058U) /**< (PWM1) PWM Output Selection Clear Update Register */
#define REG_PWM1_FMR            (*(__IO uint32_t*)0x4005C05CU) /**< (PWM1) PWM Fault Mode Register */
#define REG_PWM1_FSR            (*(__I  uint32_t*)0x4005C060U) /**< (PWM1) PWM Fault Status Register */
#define REG_PWM1_FCR            (*(__O  uint32_t*)0x4005C064U) /**< (PWM1) PWM Fault Clear Register */
#define REG_PWM1_FPV1           (*(__IO uint32_t*)0x4005C068U) /**< (PWM1) PWM Fault Protection Value Register 1 */
#define REG_PWM1_FPE            (*(__IO uint32_t*)0x4005C06CU) /**< (PWM1) PWM Fault Protection Enable Register */
#define REG_PWM1_ELMR           (*(__IO uint32_t*)0x4005C07CU) /**< (PWM1) PWM Event Line 0 Mode Register 0 */
#define REG_PWM1_ELMR0          (*(__IO uint32_t*)0x4005C07CU) /**< (PWM1) PWM Event Line 0 Mode Register 0 */
#define REG_PWM1_ELMR1          (*(__IO uint32_t*)0x4005C080U) /**< (PWM1) PWM Event Line 0 Mode Register 1 */
#define REG_PWM1_SSPR           (*(__IO uint32_t*)0x4005C0A0U) /**< (PWM1) PWM Spread Spectrum Register */
#define REG_PWM1_SSPUP          (*(__O  uint32_t*)0x4005C0A4U) /**< (PWM1) PWM Spread Spectrum Update Register */
#define REG_PWM1_SMMR           (*(__IO uint32_t*)0x4005C0B0U) /**< (PWM1) PWM Stepper Motor Mode Register */
#define REG_PWM1_FPV2           (*(__IO uint32_t*)0x4005C0C0U) /**< (PWM1) PWM Fault Protection Value 2 Register */
#define REG_PWM1_WPCR           (*(__O  uint32_t*)0x4005C0E4U) /**< (PWM1) PWM Write Protection Control Register */
#define REG_PWM1_WPSR           (*(__I  uint32_t*)0x4005C0E8U) /**< (PWM1) PWM Write Protection Status Register */
#define REG_PWM1_CMUPD0         (*(__O  uint32_t*)0x4005C400U) /**< (PWM1) PWM Channel Mode Update Register (ch_num = 0) */
#define REG_PWM1_CMUPD1         (*(__O  uint32_t*)0x4005C420U) /**< (PWM1) PWM Channel Mode Update Register (ch_num = 1) */
#define REG_PWM1_ETRG1          (*(__IO uint32_t*)0x4005C42CU) /**< (PWM1) PWM External Trigger Register (trg_num = 1) */
#define REG_PWM1_LEBR1          (*(__IO uint32_t*)0x4005C430U) /**< (PWM1) PWM Leading-Edge Blanking Register (trg_num = 1) */
#define REG_PWM1_CMUPD2         (*(__O  uint32_t*)0x4005C440U) /**< (PWM1) PWM Channel Mode Update Register (ch_num = 2) */
#define REG_PWM1_ETRG2          (*(__IO uint32_t*)0x4005C44CU) /**< (PWM1) PWM External Trigger Register (trg_num = 2) */
#define REG_PWM1_LEBR2          (*(__IO uint32_t*)0x4005C450U) /**< (PWM1) PWM Leading-Edge Blanking Register (trg_num = 2) */
#define REG_PWM1_CMUPD3         (*(__O  uint32_t*)0x4005C460U) /**< (PWM1) PWM Channel Mode Update Register (ch_num = 3) */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for PWM1 peripheral ========== */
#define PWM1_INSTANCE_ID                         60        
#define PWM1_DMAC_ID_TX                          39        

#endif /* _SAME70_PWM1_INSTANCE_ */
