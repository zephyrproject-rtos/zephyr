/* ---------------------------------------------------------------------------- */
/*                  Atmel Microcontroller Software Support                      */
/*                       SAM Software Package License                           */
/* ---------------------------------------------------------------------------- */
/* Copyright (c) %copyright_year%, Atmel Corporation                                        */
/*                                                                              */
/* All rights reserved.                                                         */
/*                                                                              */
/* Redistribution and use in source and binary forms, with or without           */
/* modification, are permitted provided that the following condition is met:    */
/*                                                                              */
/* - Redistributions of source code must retain the above copyright notice,     */
/* this list of conditions and the disclaimer below.                            */
/*                                                                              */
/* Atmel's name may not be used to endorse or promote products derived from     */
/* this software without specific prior written permission.                     */
/*                                                                              */
/* DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR   */
/* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE   */
/* DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,      */
/* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,  */
/* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    */
/* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING         */
/* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, */
/* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/* ---------------------------------------------------------------------------- */

#ifndef _SAM4S_TC0_INSTANCE_
#define _SAM4S_TC0_INSTANCE_

/* ========== Register definition for TC0 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_TC0_CCR0                   (0x40010000U) /**< \brief (TC0) Channel Control Register (channel = 0) */
  #define REG_TC0_CMR0                   (0x40010004U) /**< \brief (TC0) Channel Mode Register (channel = 0) */
  #define REG_TC0_SMMR0                  (0x40010008U) /**< \brief (TC0) Stepper Motor Mode Register (channel = 0) */
  #define REG_TC0_CV0                    (0x40010010U) /**< \brief (TC0) Counter Value (channel = 0) */
  #define REG_TC0_RA0                    (0x40010014U) /**< \brief (TC0) Register A (channel = 0) */
  #define REG_TC0_RB0                    (0x40010018U) /**< \brief (TC0) Register B (channel = 0) */
  #define REG_TC0_RC0                    (0x4001001CU) /**< \brief (TC0) Register C (channel = 0) */
  #define REG_TC0_SR0                    (0x40010020U) /**< \brief (TC0) Status Register (channel = 0) */
  #define REG_TC0_IER0                   (0x40010024U) /**< \brief (TC0) Interrupt Enable Register (channel = 0) */
  #define REG_TC0_IDR0                   (0x40010028U) /**< \brief (TC0) Interrupt Disable Register (channel = 0) */
  #define REG_TC0_IMR0                   (0x4001002CU) /**< \brief (TC0) Interrupt Mask Register (channel = 0) */
  #define REG_TC0_CCR1                   (0x40010040U) /**< \brief (TC0) Channel Control Register (channel = 1) */
  #define REG_TC0_CMR1                   (0x40010044U) /**< \brief (TC0) Channel Mode Register (channel = 1) */
  #define REG_TC0_SMMR1                  (0x40010048U) /**< \brief (TC0) Stepper Motor Mode Register (channel = 1) */
  #define REG_TC0_CV1                    (0x40010050U) /**< \brief (TC0) Counter Value (channel = 1) */
  #define REG_TC0_RA1                    (0x40010054U) /**< \brief (TC0) Register A (channel = 1) */
  #define REG_TC0_RB1                    (0x40010058U) /**< \brief (TC0) Register B (channel = 1) */
  #define REG_TC0_RC1                    (0x4001005CU) /**< \brief (TC0) Register C (channel = 1) */
  #define REG_TC0_SR1                    (0x40010060U) /**< \brief (TC0) Status Register (channel = 1) */
  #define REG_TC0_IER1                   (0x40010064U) /**< \brief (TC0) Interrupt Enable Register (channel = 1) */
  #define REG_TC0_IDR1                   (0x40010068U) /**< \brief (TC0) Interrupt Disable Register (channel = 1) */
  #define REG_TC0_IMR1                   (0x4001006CU) /**< \brief (TC0) Interrupt Mask Register (channel = 1) */
  #define REG_TC0_CCR2                   (0x40010080U) /**< \brief (TC0) Channel Control Register (channel = 2) */
  #define REG_TC0_CMR2                   (0x40010084U) /**< \brief (TC0) Channel Mode Register (channel = 2) */
  #define REG_TC0_SMMR2                  (0x40010088U) /**< \brief (TC0) Stepper Motor Mode Register (channel = 2) */
  #define REG_TC0_CV2                    (0x40010090U) /**< \brief (TC0) Counter Value (channel = 2) */
  #define REG_TC0_RA2                    (0x40010094U) /**< \brief (TC0) Register A (channel = 2) */
  #define REG_TC0_RB2                    (0x40010098U) /**< \brief (TC0) Register B (channel = 2) */
  #define REG_TC0_RC2                    (0x4001009CU) /**< \brief (TC0) Register C (channel = 2) */
  #define REG_TC0_SR2                    (0x400100A0U) /**< \brief (TC0) Status Register (channel = 2) */
  #define REG_TC0_IER2                   (0x400100A4U) /**< \brief (TC0) Interrupt Enable Register (channel = 2) */
  #define REG_TC0_IDR2                   (0x400100A8U) /**< \brief (TC0) Interrupt Disable Register (channel = 2) */
  #define REG_TC0_IMR2                   (0x400100ACU) /**< \brief (TC0) Interrupt Mask Register (channel = 2) */
  #define REG_TC0_BCR                    (0x400100C0U) /**< \brief (TC0) Block Control Register */
  #define REG_TC0_BMR                    (0x400100C4U) /**< \brief (TC0) Block Mode Register */
  #define REG_TC0_QIER                   (0x400100C8U) /**< \brief (TC0) QDEC Interrupt Enable Register */
  #define REG_TC0_QIDR                   (0x400100CCU) /**< \brief (TC0) QDEC Interrupt Disable Register */
  #define REG_TC0_QIMR                   (0x400100D0U) /**< \brief (TC0) QDEC Interrupt Mask Register */
  #define REG_TC0_QISR                   (0x400100D4U) /**< \brief (TC0) QDEC Interrupt Status Register */
  #define REG_TC0_FMR                    (0x400100D8U) /**< \brief (TC0) Fault Mode Register */
  #define REG_TC0_WPMR                   (0x400100E4U) /**< \brief (TC0) Write Protection Mode Register */
#else
  #define REG_TC0_CCR0  (*(__O  uint32_t*)0x40010000U) /**< \brief (TC0) Channel Control Register (channel = 0) */
  #define REG_TC0_CMR0  (*(__IO uint32_t*)0x40010004U) /**< \brief (TC0) Channel Mode Register (channel = 0) */
  #define REG_TC0_SMMR0 (*(__IO uint32_t*)0x40010008U) /**< \brief (TC0) Stepper Motor Mode Register (channel = 0) */
  #define REG_TC0_CV0   (*(__I  uint32_t*)0x40010010U) /**< \brief (TC0) Counter Value (channel = 0) */
  #define REG_TC0_RA0   (*(__IO uint32_t*)0x40010014U) /**< \brief (TC0) Register A (channel = 0) */
  #define REG_TC0_RB0   (*(__IO uint32_t*)0x40010018U) /**< \brief (TC0) Register B (channel = 0) */
  #define REG_TC0_RC0   (*(__IO uint32_t*)0x4001001CU) /**< \brief (TC0) Register C (channel = 0) */
  #define REG_TC0_SR0   (*(__I  uint32_t*)0x40010020U) /**< \brief (TC0) Status Register (channel = 0) */
  #define REG_TC0_IER0  (*(__O  uint32_t*)0x40010024U) /**< \brief (TC0) Interrupt Enable Register (channel = 0) */
  #define REG_TC0_IDR0  (*(__O  uint32_t*)0x40010028U) /**< \brief (TC0) Interrupt Disable Register (channel = 0) */
  #define REG_TC0_IMR0  (*(__I  uint32_t*)0x4001002CU) /**< \brief (TC0) Interrupt Mask Register (channel = 0) */
  #define REG_TC0_CCR1  (*(__O  uint32_t*)0x40010040U) /**< \brief (TC0) Channel Control Register (channel = 1) */
  #define REG_TC0_CMR1  (*(__IO uint32_t*)0x40010044U) /**< \brief (TC0) Channel Mode Register (channel = 1) */
  #define REG_TC0_SMMR1 (*(__IO uint32_t*)0x40010048U) /**< \brief (TC0) Stepper Motor Mode Register (channel = 1) */
  #define REG_TC0_CV1   (*(__I  uint32_t*)0x40010050U) /**< \brief (TC0) Counter Value (channel = 1) */
  #define REG_TC0_RA1   (*(__IO uint32_t*)0x40010054U) /**< \brief (TC0) Register A (channel = 1) */
  #define REG_TC0_RB1   (*(__IO uint32_t*)0x40010058U) /**< \brief (TC0) Register B (channel = 1) */
  #define REG_TC0_RC1   (*(__IO uint32_t*)0x4001005CU) /**< \brief (TC0) Register C (channel = 1) */
  #define REG_TC0_SR1   (*(__I  uint32_t*)0x40010060U) /**< \brief (TC0) Status Register (channel = 1) */
  #define REG_TC0_IER1  (*(__O  uint32_t*)0x40010064U) /**< \brief (TC0) Interrupt Enable Register (channel = 1) */
  #define REG_TC0_IDR1  (*(__O  uint32_t*)0x40010068U) /**< \brief (TC0) Interrupt Disable Register (channel = 1) */
  #define REG_TC0_IMR1  (*(__I  uint32_t*)0x4001006CU) /**< \brief (TC0) Interrupt Mask Register (channel = 1) */
  #define REG_TC0_CCR2  (*(__O  uint32_t*)0x40010080U) /**< \brief (TC0) Channel Control Register (channel = 2) */
  #define REG_TC0_CMR2  (*(__IO uint32_t*)0x40010084U) /**< \brief (TC0) Channel Mode Register (channel = 2) */
  #define REG_TC0_SMMR2 (*(__IO uint32_t*)0x40010088U) /**< \brief (TC0) Stepper Motor Mode Register (channel = 2) */
  #define REG_TC0_CV2   (*(__I  uint32_t*)0x40010090U) /**< \brief (TC0) Counter Value (channel = 2) */
  #define REG_TC0_RA2   (*(__IO uint32_t*)0x40010094U) /**< \brief (TC0) Register A (channel = 2) */
  #define REG_TC0_RB2   (*(__IO uint32_t*)0x40010098U) /**< \brief (TC0) Register B (channel = 2) */
  #define REG_TC0_RC2   (*(__IO uint32_t*)0x4001009CU) /**< \brief (TC0) Register C (channel = 2) */
  #define REG_TC0_SR2   (*(__I  uint32_t*)0x400100A0U) /**< \brief (TC0) Status Register (channel = 2) */
  #define REG_TC0_IER2  (*(__O  uint32_t*)0x400100A4U) /**< \brief (TC0) Interrupt Enable Register (channel = 2) */
  #define REG_TC0_IDR2  (*(__O  uint32_t*)0x400100A8U) /**< \brief (TC0) Interrupt Disable Register (channel = 2) */
  #define REG_TC0_IMR2  (*(__I  uint32_t*)0x400100ACU) /**< \brief (TC0) Interrupt Mask Register (channel = 2) */
  #define REG_TC0_BCR   (*(__O  uint32_t*)0x400100C0U) /**< \brief (TC0) Block Control Register */
  #define REG_TC0_BMR   (*(__IO uint32_t*)0x400100C4U) /**< \brief (TC0) Block Mode Register */
  #define REG_TC0_QIER  (*(__O  uint32_t*)0x400100C8U) /**< \brief (TC0) QDEC Interrupt Enable Register */
  #define REG_TC0_QIDR  (*(__O  uint32_t*)0x400100CCU) /**< \brief (TC0) QDEC Interrupt Disable Register */
  #define REG_TC0_QIMR  (*(__I  uint32_t*)0x400100D0U) /**< \brief (TC0) QDEC Interrupt Mask Register */
  #define REG_TC0_QISR  (*(__I  uint32_t*)0x400100D4U) /**< \brief (TC0) QDEC Interrupt Status Register */
  #define REG_TC0_FMR   (*(__IO uint32_t*)0x400100D8U) /**< \brief (TC0) Fault Mode Register */
  #define REG_TC0_WPMR  (*(__IO uint32_t*)0x400100E4U) /**< \brief (TC0) Write Protection Mode Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM4S_TC0_INSTANCE_ */
