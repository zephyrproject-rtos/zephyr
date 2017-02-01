/**
 * \file
 *
 * \brief Instance description for PWM0
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

#ifndef _SAME70_PWM0_INSTANCE_H_
#define _SAME70_PWM0_INSTANCE_H_

/* ========== Register definition for PWM0 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_PWM0_CMPV0          (0x40020130) /**< (PWM0) PWM Comparison 0 Value Register 0 */
#define REG_PWM0_CMPVUPD0       (0x40020134) /**< (PWM0) PWM Comparison 0 Value Update Register 0 */
#define REG_PWM0_CMPM0          (0x40020138) /**< (PWM0) PWM Comparison 0 Mode Register 0 */
#define REG_PWM0_CMPMUPD0       (0x4002013C) /**< (PWM0) PWM Comparison 0 Mode Update Register 0 */
#define REG_PWM0_CMPV1          (0x40020140) /**< (PWM0) PWM Comparison 0 Value Register 1 */
#define REG_PWM0_CMPVUPD1       (0x40020144) /**< (PWM0) PWM Comparison 0 Value Update Register 1 */
#define REG_PWM0_CMPM1          (0x40020148) /**< (PWM0) PWM Comparison 0 Mode Register 1 */
#define REG_PWM0_CMPMUPD1       (0x4002014C) /**< (PWM0) PWM Comparison 0 Mode Update Register 1 */
#define REG_PWM0_CMPV2          (0x40020150) /**< (PWM0) PWM Comparison 0 Value Register 2 */
#define REG_PWM0_CMPVUPD2       (0x40020154) /**< (PWM0) PWM Comparison 0 Value Update Register 2 */
#define REG_PWM0_CMPM2          (0x40020158) /**< (PWM0) PWM Comparison 0 Mode Register 2 */
#define REG_PWM0_CMPMUPD2       (0x4002015C) /**< (PWM0) PWM Comparison 0 Mode Update Register 2 */
#define REG_PWM0_CMPV3          (0x40020160) /**< (PWM0) PWM Comparison 0 Value Register 3 */
#define REG_PWM0_CMPVUPD3       (0x40020164) /**< (PWM0) PWM Comparison 0 Value Update Register 3 */
#define REG_PWM0_CMPM3          (0x40020168) /**< (PWM0) PWM Comparison 0 Mode Register 3 */
#define REG_PWM0_CMPMUPD3       (0x4002016C) /**< (PWM0) PWM Comparison 0 Mode Update Register 3 */
#define REG_PWM0_CMPV4          (0x40020170) /**< (PWM0) PWM Comparison 0 Value Register 4 */
#define REG_PWM0_CMPVUPD4       (0x40020174) /**< (PWM0) PWM Comparison 0 Value Update Register 4 */
#define REG_PWM0_CMPM4          (0x40020178) /**< (PWM0) PWM Comparison 0 Mode Register 4 */
#define REG_PWM0_CMPMUPD4       (0x4002017C) /**< (PWM0) PWM Comparison 0 Mode Update Register 4 */
#define REG_PWM0_CMPV5          (0x40020180) /**< (PWM0) PWM Comparison 0 Value Register 5 */
#define REG_PWM0_CMPVUPD5       (0x40020184) /**< (PWM0) PWM Comparison 0 Value Update Register 5 */
#define REG_PWM0_CMPM5          (0x40020188) /**< (PWM0) PWM Comparison 0 Mode Register 5 */
#define REG_PWM0_CMPMUPD5       (0x4002018C) /**< (PWM0) PWM Comparison 0 Mode Update Register 5 */
#define REG_PWM0_CMPV6          (0x40020190) /**< (PWM0) PWM Comparison 0 Value Register 6 */
#define REG_PWM0_CMPVUPD6       (0x40020194) /**< (PWM0) PWM Comparison 0 Value Update Register 6 */
#define REG_PWM0_CMPM6          (0x40020198) /**< (PWM0) PWM Comparison 0 Mode Register 6 */
#define REG_PWM0_CMPMUPD6       (0x4002019C) /**< (PWM0) PWM Comparison 0 Mode Update Register 6 */
#define REG_PWM0_CMPV7          (0x400201A0) /**< (PWM0) PWM Comparison 0 Value Register 7 */
#define REG_PWM0_CMPVUPD7       (0x400201A4) /**< (PWM0) PWM Comparison 0 Value Update Register 7 */
#define REG_PWM0_CMPM7          (0x400201A8) /**< (PWM0) PWM Comparison 0 Mode Register 7 */
#define REG_PWM0_CMPMUPD7       (0x400201AC) /**< (PWM0) PWM Comparison 0 Mode Update Register 7 */
#define REG_PWM0_CMR0           (0x40020200) /**< (PWM0) PWM Channel Mode Register (ch_num = 0) 0 */
#define REG_PWM0_CDTY0          (0x40020204) /**< (PWM0) PWM Channel Duty Cycle Register (ch_num = 0) 0 */
#define REG_PWM0_CDTYUPD0       (0x40020208) /**< (PWM0) PWM Channel Duty Cycle Update Register (ch_num = 0) 0 */
#define REG_PWM0_CPRD0          (0x4002020C) /**< (PWM0) PWM Channel Period Register (ch_num = 0) 0 */
#define REG_PWM0_CPRDUPD0       (0x40020210) /**< (PWM0) PWM Channel Period Update Register (ch_num = 0) 0 */
#define REG_PWM0_CCNT0          (0x40020214) /**< (PWM0) PWM Channel Counter Register (ch_num = 0) 0 */
#define REG_PWM0_DT0            (0x40020218) /**< (PWM0) PWM Channel Dead Time Register (ch_num = 0) 0 */
#define REG_PWM0_DTUPD0         (0x4002021C) /**< (PWM0) PWM Channel Dead Time Update Register (ch_num = 0) 0 */
#define REG_PWM0_CMR1           (0x40020220) /**< (PWM0) PWM Channel Mode Register (ch_num = 0) 1 */
#define REG_PWM0_CDTY1          (0x40020224) /**< (PWM0) PWM Channel Duty Cycle Register (ch_num = 0) 1 */
#define REG_PWM0_CDTYUPD1       (0x40020228) /**< (PWM0) PWM Channel Duty Cycle Update Register (ch_num = 0) 1 */
#define REG_PWM0_CPRD1          (0x4002022C) /**< (PWM0) PWM Channel Period Register (ch_num = 0) 1 */
#define REG_PWM0_CPRDUPD1       (0x40020230) /**< (PWM0) PWM Channel Period Update Register (ch_num = 0) 1 */
#define REG_PWM0_CCNT1          (0x40020234) /**< (PWM0) PWM Channel Counter Register (ch_num = 0) 1 */
#define REG_PWM0_DT1            (0x40020238) /**< (PWM0) PWM Channel Dead Time Register (ch_num = 0) 1 */
#define REG_PWM0_DTUPD1         (0x4002023C) /**< (PWM0) PWM Channel Dead Time Update Register (ch_num = 0) 1 */
#define REG_PWM0_CMR2           (0x40020240) /**< (PWM0) PWM Channel Mode Register (ch_num = 0) 2 */
#define REG_PWM0_CDTY2          (0x40020244) /**< (PWM0) PWM Channel Duty Cycle Register (ch_num = 0) 2 */
#define REG_PWM0_CDTYUPD2       (0x40020248) /**< (PWM0) PWM Channel Duty Cycle Update Register (ch_num = 0) 2 */
#define REG_PWM0_CPRD2          (0x4002024C) /**< (PWM0) PWM Channel Period Register (ch_num = 0) 2 */
#define REG_PWM0_CPRDUPD2       (0x40020250) /**< (PWM0) PWM Channel Period Update Register (ch_num = 0) 2 */
#define REG_PWM0_CCNT2          (0x40020254) /**< (PWM0) PWM Channel Counter Register (ch_num = 0) 2 */
#define REG_PWM0_DT2            (0x40020258) /**< (PWM0) PWM Channel Dead Time Register (ch_num = 0) 2 */
#define REG_PWM0_DTUPD2         (0x4002025C) /**< (PWM0) PWM Channel Dead Time Update Register (ch_num = 0) 2 */
#define REG_PWM0_CMR3           (0x40020260) /**< (PWM0) PWM Channel Mode Register (ch_num = 0) 3 */
#define REG_PWM0_CDTY3          (0x40020264) /**< (PWM0) PWM Channel Duty Cycle Register (ch_num = 0) 3 */
#define REG_PWM0_CDTYUPD3       (0x40020268) /**< (PWM0) PWM Channel Duty Cycle Update Register (ch_num = 0) 3 */
#define REG_PWM0_CPRD3          (0x4002026C) /**< (PWM0) PWM Channel Period Register (ch_num = 0) 3 */
#define REG_PWM0_CPRDUPD3       (0x40020270) /**< (PWM0) PWM Channel Period Update Register (ch_num = 0) 3 */
#define REG_PWM0_CCNT3          (0x40020274) /**< (PWM0) PWM Channel Counter Register (ch_num = 0) 3 */
#define REG_PWM0_DT3            (0x40020278) /**< (PWM0) PWM Channel Dead Time Register (ch_num = 0) 3 */
#define REG_PWM0_DTUPD3         (0x4002027C) /**< (PWM0) PWM Channel Dead Time Update Register (ch_num = 0) 3 */
#define REG_PWM0_CLK            (0x40020000) /**< (PWM0) PWM Clock Register */
#define REG_PWM0_ENA            (0x40020004) /**< (PWM0) PWM Enable Register */
#define REG_PWM0_DIS            (0x40020008) /**< (PWM0) PWM Disable Register */
#define REG_PWM0_SR             (0x4002000C) /**< (PWM0) PWM Status Register */
#define REG_PWM0_IER1           (0x40020010) /**< (PWM0) PWM Interrupt Enable Register 1 */
#define REG_PWM0_IDR1           (0x40020014) /**< (PWM0) PWM Interrupt Disable Register 1 */
#define REG_PWM0_IMR1           (0x40020018) /**< (PWM0) PWM Interrupt Mask Register 1 */
#define REG_PWM0_ISR1           (0x4002001C) /**< (PWM0) PWM Interrupt Status Register 1 */
#define REG_PWM0_SCM            (0x40020020) /**< (PWM0) PWM Sync Channels Mode Register */
#define REG_PWM0_DMAR           (0x40020024) /**< (PWM0) PWM DMA Register */
#define REG_PWM0_SCUC           (0x40020028) /**< (PWM0) PWM Sync Channels Update Control Register */
#define REG_PWM0_SCUP           (0x4002002C) /**< (PWM0) PWM Sync Channels Update Period Register */
#define REG_PWM0_SCUPUPD        (0x40020030) /**< (PWM0) PWM Sync Channels Update Period Update Register */
#define REG_PWM0_IER2           (0x40020034) /**< (PWM0) PWM Interrupt Enable Register 2 */
#define REG_PWM0_IDR2           (0x40020038) /**< (PWM0) PWM Interrupt Disable Register 2 */
#define REG_PWM0_IMR2           (0x4002003C) /**< (PWM0) PWM Interrupt Mask Register 2 */
#define REG_PWM0_ISR2           (0x40020040) /**< (PWM0) PWM Interrupt Status Register 2 */
#define REG_PWM0_OOV            (0x40020044) /**< (PWM0) PWM Output Override Value Register */
#define REG_PWM0_OS             (0x40020048) /**< (PWM0) PWM Output Selection Register */
#define REG_PWM0_OSS            (0x4002004C) /**< (PWM0) PWM Output Selection Set Register */
#define REG_PWM0_OSC            (0x40020050) /**< (PWM0) PWM Output Selection Clear Register */
#define REG_PWM0_OSSUPD         (0x40020054) /**< (PWM0) PWM Output Selection Set Update Register */
#define REG_PWM0_OSCUPD         (0x40020058) /**< (PWM0) PWM Output Selection Clear Update Register */
#define REG_PWM0_FMR            (0x4002005C) /**< (PWM0) PWM Fault Mode Register */
#define REG_PWM0_FSR            (0x40020060) /**< (PWM0) PWM Fault Status Register */
#define REG_PWM0_FCR            (0x40020064) /**< (PWM0) PWM Fault Clear Register */
#define REG_PWM0_FPV1           (0x40020068) /**< (PWM0) PWM Fault Protection Value Register 1 */
#define REG_PWM0_FPE            (0x4002006C) /**< (PWM0) PWM Fault Protection Enable Register */
#define REG_PWM0_ELMR           (0x4002007C) /**< (PWM0) PWM Event Line 0 Mode Register 0 */
#define REG_PWM0_ELMR0          (0x4002007C) /**< (PWM0) PWM Event Line 0 Mode Register 0 */
#define REG_PWM0_ELMR1          (0x40020080) /**< (PWM0) PWM Event Line 0 Mode Register 1 */
#define REG_PWM0_SSPR           (0x400200A0) /**< (PWM0) PWM Spread Spectrum Register */
#define REG_PWM0_SSPUP          (0x400200A4) /**< (PWM0) PWM Spread Spectrum Update Register */
#define REG_PWM0_SMMR           (0x400200B0) /**< (PWM0) PWM Stepper Motor Mode Register */
#define REG_PWM0_FPV2           (0x400200C0) /**< (PWM0) PWM Fault Protection Value 2 Register */
#define REG_PWM0_WPCR           (0x400200E4) /**< (PWM0) PWM Write Protection Control Register */
#define REG_PWM0_WPSR           (0x400200E8) /**< (PWM0) PWM Write Protection Status Register */
#define REG_PWM0_CMUPD0         (0x40020400) /**< (PWM0) PWM Channel Mode Update Register (ch_num = 0) */
#define REG_PWM0_CMUPD1         (0x40020420) /**< (PWM0) PWM Channel Mode Update Register (ch_num = 1) */
#define REG_PWM0_ETRG1          (0x4002042C) /**< (PWM0) PWM External Trigger Register (trg_num = 1) */
#define REG_PWM0_LEBR1          (0x40020430) /**< (PWM0) PWM Leading-Edge Blanking Register (trg_num = 1) */
#define REG_PWM0_CMUPD2         (0x40020440) /**< (PWM0) PWM Channel Mode Update Register (ch_num = 2) */
#define REG_PWM0_ETRG2          (0x4002044C) /**< (PWM0) PWM External Trigger Register (trg_num = 2) */
#define REG_PWM0_LEBR2          (0x40020450) /**< (PWM0) PWM Leading-Edge Blanking Register (trg_num = 2) */
#define REG_PWM0_CMUPD3         (0x40020460) /**< (PWM0) PWM Channel Mode Update Register (ch_num = 3) */

#else

#define REG_PWM0_CMPV0          (*(__IO uint32_t*)0x40020130U) /**< (PWM0) PWM Comparison 0 Value Register 0 */
#define REG_PWM0_CMPVUPD0       (*(__O  uint32_t*)0x40020134U) /**< (PWM0) PWM Comparison 0 Value Update Register 0 */
#define REG_PWM0_CMPM0          (*(__IO uint32_t*)0x40020138U) /**< (PWM0) PWM Comparison 0 Mode Register 0 */
#define REG_PWM0_CMPMUPD0       (*(__O  uint32_t*)0x4002013CU) /**< (PWM0) PWM Comparison 0 Mode Update Register 0 */
#define REG_PWM0_CMPV1          (*(__IO uint32_t*)0x40020140U) /**< (PWM0) PWM Comparison 0 Value Register 1 */
#define REG_PWM0_CMPVUPD1       (*(__O  uint32_t*)0x40020144U) /**< (PWM0) PWM Comparison 0 Value Update Register 1 */
#define REG_PWM0_CMPM1          (*(__IO uint32_t*)0x40020148U) /**< (PWM0) PWM Comparison 0 Mode Register 1 */
#define REG_PWM0_CMPMUPD1       (*(__O  uint32_t*)0x4002014CU) /**< (PWM0) PWM Comparison 0 Mode Update Register 1 */
#define REG_PWM0_CMPV2          (*(__IO uint32_t*)0x40020150U) /**< (PWM0) PWM Comparison 0 Value Register 2 */
#define REG_PWM0_CMPVUPD2       (*(__O  uint32_t*)0x40020154U) /**< (PWM0) PWM Comparison 0 Value Update Register 2 */
#define REG_PWM0_CMPM2          (*(__IO uint32_t*)0x40020158U) /**< (PWM0) PWM Comparison 0 Mode Register 2 */
#define REG_PWM0_CMPMUPD2       (*(__O  uint32_t*)0x4002015CU) /**< (PWM0) PWM Comparison 0 Mode Update Register 2 */
#define REG_PWM0_CMPV3          (*(__IO uint32_t*)0x40020160U) /**< (PWM0) PWM Comparison 0 Value Register 3 */
#define REG_PWM0_CMPVUPD3       (*(__O  uint32_t*)0x40020164U) /**< (PWM0) PWM Comparison 0 Value Update Register 3 */
#define REG_PWM0_CMPM3          (*(__IO uint32_t*)0x40020168U) /**< (PWM0) PWM Comparison 0 Mode Register 3 */
#define REG_PWM0_CMPMUPD3       (*(__O  uint32_t*)0x4002016CU) /**< (PWM0) PWM Comparison 0 Mode Update Register 3 */
#define REG_PWM0_CMPV4          (*(__IO uint32_t*)0x40020170U) /**< (PWM0) PWM Comparison 0 Value Register 4 */
#define REG_PWM0_CMPVUPD4       (*(__O  uint32_t*)0x40020174U) /**< (PWM0) PWM Comparison 0 Value Update Register 4 */
#define REG_PWM0_CMPM4          (*(__IO uint32_t*)0x40020178U) /**< (PWM0) PWM Comparison 0 Mode Register 4 */
#define REG_PWM0_CMPMUPD4       (*(__O  uint32_t*)0x4002017CU) /**< (PWM0) PWM Comparison 0 Mode Update Register 4 */
#define REG_PWM0_CMPV5          (*(__IO uint32_t*)0x40020180U) /**< (PWM0) PWM Comparison 0 Value Register 5 */
#define REG_PWM0_CMPVUPD5       (*(__O  uint32_t*)0x40020184U) /**< (PWM0) PWM Comparison 0 Value Update Register 5 */
#define REG_PWM0_CMPM5          (*(__IO uint32_t*)0x40020188U) /**< (PWM0) PWM Comparison 0 Mode Register 5 */
#define REG_PWM0_CMPMUPD5       (*(__O  uint32_t*)0x4002018CU) /**< (PWM0) PWM Comparison 0 Mode Update Register 5 */
#define REG_PWM0_CMPV6          (*(__IO uint32_t*)0x40020190U) /**< (PWM0) PWM Comparison 0 Value Register 6 */
#define REG_PWM0_CMPVUPD6       (*(__O  uint32_t*)0x40020194U) /**< (PWM0) PWM Comparison 0 Value Update Register 6 */
#define REG_PWM0_CMPM6          (*(__IO uint32_t*)0x40020198U) /**< (PWM0) PWM Comparison 0 Mode Register 6 */
#define REG_PWM0_CMPMUPD6       (*(__O  uint32_t*)0x4002019CU) /**< (PWM0) PWM Comparison 0 Mode Update Register 6 */
#define REG_PWM0_CMPV7          (*(__IO uint32_t*)0x400201A0U) /**< (PWM0) PWM Comparison 0 Value Register 7 */
#define REG_PWM0_CMPVUPD7       (*(__O  uint32_t*)0x400201A4U) /**< (PWM0) PWM Comparison 0 Value Update Register 7 */
#define REG_PWM0_CMPM7          (*(__IO uint32_t*)0x400201A8U) /**< (PWM0) PWM Comparison 0 Mode Register 7 */
#define REG_PWM0_CMPMUPD7       (*(__O  uint32_t*)0x400201ACU) /**< (PWM0) PWM Comparison 0 Mode Update Register 7 */
#define REG_PWM0_CMR0           (*(__IO uint32_t*)0x40020200U) /**< (PWM0) PWM Channel Mode Register (ch_num = 0) 0 */
#define REG_PWM0_CDTY0          (*(__IO uint32_t*)0x40020204U) /**< (PWM0) PWM Channel Duty Cycle Register (ch_num = 0) 0 */
#define REG_PWM0_CDTYUPD0       (*(__O  uint32_t*)0x40020208U) /**< (PWM0) PWM Channel Duty Cycle Update Register (ch_num = 0) 0 */
#define REG_PWM0_CPRD0          (*(__IO uint32_t*)0x4002020CU) /**< (PWM0) PWM Channel Period Register (ch_num = 0) 0 */
#define REG_PWM0_CPRDUPD0       (*(__O  uint32_t*)0x40020210U) /**< (PWM0) PWM Channel Period Update Register (ch_num = 0) 0 */
#define REG_PWM0_CCNT0          (*(__I  uint32_t*)0x40020214U) /**< (PWM0) PWM Channel Counter Register (ch_num = 0) 0 */
#define REG_PWM0_DT0            (*(__IO uint32_t*)0x40020218U) /**< (PWM0) PWM Channel Dead Time Register (ch_num = 0) 0 */
#define REG_PWM0_DTUPD0         (*(__O  uint32_t*)0x4002021CU) /**< (PWM0) PWM Channel Dead Time Update Register (ch_num = 0) 0 */
#define REG_PWM0_CMR1           (*(__IO uint32_t*)0x40020220U) /**< (PWM0) PWM Channel Mode Register (ch_num = 0) 1 */
#define REG_PWM0_CDTY1          (*(__IO uint32_t*)0x40020224U) /**< (PWM0) PWM Channel Duty Cycle Register (ch_num = 0) 1 */
#define REG_PWM0_CDTYUPD1       (*(__O  uint32_t*)0x40020228U) /**< (PWM0) PWM Channel Duty Cycle Update Register (ch_num = 0) 1 */
#define REG_PWM0_CPRD1          (*(__IO uint32_t*)0x4002022CU) /**< (PWM0) PWM Channel Period Register (ch_num = 0) 1 */
#define REG_PWM0_CPRDUPD1       (*(__O  uint32_t*)0x40020230U) /**< (PWM0) PWM Channel Period Update Register (ch_num = 0) 1 */
#define REG_PWM0_CCNT1          (*(__I  uint32_t*)0x40020234U) /**< (PWM0) PWM Channel Counter Register (ch_num = 0) 1 */
#define REG_PWM0_DT1            (*(__IO uint32_t*)0x40020238U) /**< (PWM0) PWM Channel Dead Time Register (ch_num = 0) 1 */
#define REG_PWM0_DTUPD1         (*(__O  uint32_t*)0x4002023CU) /**< (PWM0) PWM Channel Dead Time Update Register (ch_num = 0) 1 */
#define REG_PWM0_CMR2           (*(__IO uint32_t*)0x40020240U) /**< (PWM0) PWM Channel Mode Register (ch_num = 0) 2 */
#define REG_PWM0_CDTY2          (*(__IO uint32_t*)0x40020244U) /**< (PWM0) PWM Channel Duty Cycle Register (ch_num = 0) 2 */
#define REG_PWM0_CDTYUPD2       (*(__O  uint32_t*)0x40020248U) /**< (PWM0) PWM Channel Duty Cycle Update Register (ch_num = 0) 2 */
#define REG_PWM0_CPRD2          (*(__IO uint32_t*)0x4002024CU) /**< (PWM0) PWM Channel Period Register (ch_num = 0) 2 */
#define REG_PWM0_CPRDUPD2       (*(__O  uint32_t*)0x40020250U) /**< (PWM0) PWM Channel Period Update Register (ch_num = 0) 2 */
#define REG_PWM0_CCNT2          (*(__I  uint32_t*)0x40020254U) /**< (PWM0) PWM Channel Counter Register (ch_num = 0) 2 */
#define REG_PWM0_DT2            (*(__IO uint32_t*)0x40020258U) /**< (PWM0) PWM Channel Dead Time Register (ch_num = 0) 2 */
#define REG_PWM0_DTUPD2         (*(__O  uint32_t*)0x4002025CU) /**< (PWM0) PWM Channel Dead Time Update Register (ch_num = 0) 2 */
#define REG_PWM0_CMR3           (*(__IO uint32_t*)0x40020260U) /**< (PWM0) PWM Channel Mode Register (ch_num = 0) 3 */
#define REG_PWM0_CDTY3          (*(__IO uint32_t*)0x40020264U) /**< (PWM0) PWM Channel Duty Cycle Register (ch_num = 0) 3 */
#define REG_PWM0_CDTYUPD3       (*(__O  uint32_t*)0x40020268U) /**< (PWM0) PWM Channel Duty Cycle Update Register (ch_num = 0) 3 */
#define REG_PWM0_CPRD3          (*(__IO uint32_t*)0x4002026CU) /**< (PWM0) PWM Channel Period Register (ch_num = 0) 3 */
#define REG_PWM0_CPRDUPD3       (*(__O  uint32_t*)0x40020270U) /**< (PWM0) PWM Channel Period Update Register (ch_num = 0) 3 */
#define REG_PWM0_CCNT3          (*(__I  uint32_t*)0x40020274U) /**< (PWM0) PWM Channel Counter Register (ch_num = 0) 3 */
#define REG_PWM0_DT3            (*(__IO uint32_t*)0x40020278U) /**< (PWM0) PWM Channel Dead Time Register (ch_num = 0) 3 */
#define REG_PWM0_DTUPD3         (*(__O  uint32_t*)0x4002027CU) /**< (PWM0) PWM Channel Dead Time Update Register (ch_num = 0) 3 */
#define REG_PWM0_CLK            (*(__IO uint32_t*)0x40020000U) /**< (PWM0) PWM Clock Register */
#define REG_PWM0_ENA            (*(__O  uint32_t*)0x40020004U) /**< (PWM0) PWM Enable Register */
#define REG_PWM0_DIS            (*(__O  uint32_t*)0x40020008U) /**< (PWM0) PWM Disable Register */
#define REG_PWM0_SR             (*(__I  uint32_t*)0x4002000CU) /**< (PWM0) PWM Status Register */
#define REG_PWM0_IER1           (*(__O  uint32_t*)0x40020010U) /**< (PWM0) PWM Interrupt Enable Register 1 */
#define REG_PWM0_IDR1           (*(__O  uint32_t*)0x40020014U) /**< (PWM0) PWM Interrupt Disable Register 1 */
#define REG_PWM0_IMR1           (*(__I  uint32_t*)0x40020018U) /**< (PWM0) PWM Interrupt Mask Register 1 */
#define REG_PWM0_ISR1           (*(__I  uint32_t*)0x4002001CU) /**< (PWM0) PWM Interrupt Status Register 1 */
#define REG_PWM0_SCM            (*(__IO uint32_t*)0x40020020U) /**< (PWM0) PWM Sync Channels Mode Register */
#define REG_PWM0_DMAR           (*(__O  uint32_t*)0x40020024U) /**< (PWM0) PWM DMA Register */
#define REG_PWM0_SCUC           (*(__IO uint32_t*)0x40020028U) /**< (PWM0) PWM Sync Channels Update Control Register */
#define REG_PWM0_SCUP           (*(__IO uint32_t*)0x4002002CU) /**< (PWM0) PWM Sync Channels Update Period Register */
#define REG_PWM0_SCUPUPD        (*(__O  uint32_t*)0x40020030U) /**< (PWM0) PWM Sync Channels Update Period Update Register */
#define REG_PWM0_IER2           (*(__O  uint32_t*)0x40020034U) /**< (PWM0) PWM Interrupt Enable Register 2 */
#define REG_PWM0_IDR2           (*(__O  uint32_t*)0x40020038U) /**< (PWM0) PWM Interrupt Disable Register 2 */
#define REG_PWM0_IMR2           (*(__I  uint32_t*)0x4002003CU) /**< (PWM0) PWM Interrupt Mask Register 2 */
#define REG_PWM0_ISR2           (*(__I  uint32_t*)0x40020040U) /**< (PWM0) PWM Interrupt Status Register 2 */
#define REG_PWM0_OOV            (*(__IO uint32_t*)0x40020044U) /**< (PWM0) PWM Output Override Value Register */
#define REG_PWM0_OS             (*(__IO uint32_t*)0x40020048U) /**< (PWM0) PWM Output Selection Register */
#define REG_PWM0_OSS            (*(__O  uint32_t*)0x4002004CU) /**< (PWM0) PWM Output Selection Set Register */
#define REG_PWM0_OSC            (*(__O  uint32_t*)0x40020050U) /**< (PWM0) PWM Output Selection Clear Register */
#define REG_PWM0_OSSUPD         (*(__O  uint32_t*)0x40020054U) /**< (PWM0) PWM Output Selection Set Update Register */
#define REG_PWM0_OSCUPD         (*(__O  uint32_t*)0x40020058U) /**< (PWM0) PWM Output Selection Clear Update Register */
#define REG_PWM0_FMR            (*(__IO uint32_t*)0x4002005CU) /**< (PWM0) PWM Fault Mode Register */
#define REG_PWM0_FSR            (*(__I  uint32_t*)0x40020060U) /**< (PWM0) PWM Fault Status Register */
#define REG_PWM0_FCR            (*(__O  uint32_t*)0x40020064U) /**< (PWM0) PWM Fault Clear Register */
#define REG_PWM0_FPV1           (*(__IO uint32_t*)0x40020068U) /**< (PWM0) PWM Fault Protection Value Register 1 */
#define REG_PWM0_FPE            (*(__IO uint32_t*)0x4002006CU) /**< (PWM0) PWM Fault Protection Enable Register */
#define REG_PWM0_ELMR           (*(__IO uint32_t*)0x4002007CU) /**< (PWM0) PWM Event Line 0 Mode Register 0 */
#define REG_PWM0_ELMR0          (*(__IO uint32_t*)0x4002007CU) /**< (PWM0) PWM Event Line 0 Mode Register 0 */
#define REG_PWM0_ELMR1          (*(__IO uint32_t*)0x40020080U) /**< (PWM0) PWM Event Line 0 Mode Register 1 */
#define REG_PWM0_SSPR           (*(__IO uint32_t*)0x400200A0U) /**< (PWM0) PWM Spread Spectrum Register */
#define REG_PWM0_SSPUP          (*(__O  uint32_t*)0x400200A4U) /**< (PWM0) PWM Spread Spectrum Update Register */
#define REG_PWM0_SMMR           (*(__IO uint32_t*)0x400200B0U) /**< (PWM0) PWM Stepper Motor Mode Register */
#define REG_PWM0_FPV2           (*(__IO uint32_t*)0x400200C0U) /**< (PWM0) PWM Fault Protection Value 2 Register */
#define REG_PWM0_WPCR           (*(__O  uint32_t*)0x400200E4U) /**< (PWM0) PWM Write Protection Control Register */
#define REG_PWM0_WPSR           (*(__I  uint32_t*)0x400200E8U) /**< (PWM0) PWM Write Protection Status Register */
#define REG_PWM0_CMUPD0         (*(__O  uint32_t*)0x40020400U) /**< (PWM0) PWM Channel Mode Update Register (ch_num = 0) */
#define REG_PWM0_CMUPD1         (*(__O  uint32_t*)0x40020420U) /**< (PWM0) PWM Channel Mode Update Register (ch_num = 1) */
#define REG_PWM0_ETRG1          (*(__IO uint32_t*)0x4002042CU) /**< (PWM0) PWM External Trigger Register (trg_num = 1) */
#define REG_PWM0_LEBR1          (*(__IO uint32_t*)0x40020430U) /**< (PWM0) PWM Leading-Edge Blanking Register (trg_num = 1) */
#define REG_PWM0_CMUPD2         (*(__O  uint32_t*)0x40020440U) /**< (PWM0) PWM Channel Mode Update Register (ch_num = 2) */
#define REG_PWM0_ETRG2          (*(__IO uint32_t*)0x4002044CU) /**< (PWM0) PWM External Trigger Register (trg_num = 2) */
#define REG_PWM0_LEBR2          (*(__IO uint32_t*)0x40020450U) /**< (PWM0) PWM Leading-Edge Blanking Register (trg_num = 2) */
#define REG_PWM0_CMUPD3         (*(__O  uint32_t*)0x40020460U) /**< (PWM0) PWM Channel Mode Update Register (ch_num = 3) */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for PWM0 peripheral ========== */
#define PWM0_INSTANCE_ID                         31        
#define PWM0_DMAC_ID_TX                          13        

#endif /* _SAME70_PWM0_INSTANCE_ */
