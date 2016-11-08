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

#ifndef _SAM3XA_CAN0_INSTANCE_
#define _SAM3XA_CAN0_INSTANCE_

/* ========== Register definition for CAN0 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_CAN0_MR                       (0x400B4000U) /**< \brief (CAN0) Mode Register */
  #define REG_CAN0_IER                      (0x400B4004U) /**< \brief (CAN0) Interrupt Enable Register */
  #define REG_CAN0_IDR                      (0x400B4008U) /**< \brief (CAN0) Interrupt Disable Register */
  #define REG_CAN0_IMR                      (0x400B400CU) /**< \brief (CAN0) Interrupt Mask Register */
  #define REG_CAN0_SR                       (0x400B4010U) /**< \brief (CAN0) Status Register */
  #define REG_CAN0_BR                       (0x400B4014U) /**< \brief (CAN0) Baudrate Register */
  #define REG_CAN0_TIM                      (0x400B4018U) /**< \brief (CAN0) Timer Register */
  #define REG_CAN0_TIMESTP                  (0x400B401CU) /**< \brief (CAN0) Timestamp Register */
  #define REG_CAN0_ECR                      (0x400B4020U) /**< \brief (CAN0) Error Counter Register */
  #define REG_CAN0_TCR                      (0x400B4024U) /**< \brief (CAN0) Transfer Command Register */
  #define REG_CAN0_ACR                      (0x400B4028U) /**< \brief (CAN0) Abort Command Register */
  #define REG_CAN0_WPMR                     (0x400B40E4U) /**< \brief (CAN0) Write Protect Mode Register */
  #define REG_CAN0_WPSR                     (0x400B40E8U) /**< \brief (CAN0) Write Protect Status Register */
  #define REG_CAN0_MMR0                     (0x400B4200U) /**< \brief (CAN0) Mailbox Mode Register (MB = 0) */
  #define REG_CAN0_MAM0                     (0x400B4204U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 0) */
  #define REG_CAN0_MID0                     (0x400B4208U) /**< \brief (CAN0) Mailbox ID Register (MB = 0) */
  #define REG_CAN0_MFID0                    (0x400B420CU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 0) */
  #define REG_CAN0_MSR0                     (0x400B4210U) /**< \brief (CAN0) Mailbox Status Register (MB = 0) */
  #define REG_CAN0_MDL0                     (0x400B4214U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 0) */
  #define REG_CAN0_MDH0                     (0x400B4218U) /**< \brief (CAN0) Mailbox Data High Register (MB = 0) */
  #define REG_CAN0_MCR0                     (0x400B421CU) /**< \brief (CAN0) Mailbox Control Register (MB = 0) */
  #define REG_CAN0_MMR1                     (0x400B4220U) /**< \brief (CAN0) Mailbox Mode Register (MB = 1) */
  #define REG_CAN0_MAM1                     (0x400B4224U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 1) */
  #define REG_CAN0_MID1                     (0x400B4228U) /**< \brief (CAN0) Mailbox ID Register (MB = 1) */
  #define REG_CAN0_MFID1                    (0x400B422CU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 1) */
  #define REG_CAN0_MSR1                     (0x400B4230U) /**< \brief (CAN0) Mailbox Status Register (MB = 1) */
  #define REG_CAN0_MDL1                     (0x400B4234U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 1) */
  #define REG_CAN0_MDH1                     (0x400B4238U) /**< \brief (CAN0) Mailbox Data High Register (MB = 1) */
  #define REG_CAN0_MCR1                     (0x400B423CU) /**< \brief (CAN0) Mailbox Control Register (MB = 1) */
  #define REG_CAN0_MMR2                     (0x400B4240U) /**< \brief (CAN0) Mailbox Mode Register (MB = 2) */
  #define REG_CAN0_MAM2                     (0x400B4244U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 2) */
  #define REG_CAN0_MID2                     (0x400B4248U) /**< \brief (CAN0) Mailbox ID Register (MB = 2) */
  #define REG_CAN0_MFID2                    (0x400B424CU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 2) */
  #define REG_CAN0_MSR2                     (0x400B4250U) /**< \brief (CAN0) Mailbox Status Register (MB = 2) */
  #define REG_CAN0_MDL2                     (0x400B4254U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 2) */
  #define REG_CAN0_MDH2                     (0x400B4258U) /**< \brief (CAN0) Mailbox Data High Register (MB = 2) */
  #define REG_CAN0_MCR2                     (0x400B425CU) /**< \brief (CAN0) Mailbox Control Register (MB = 2) */
  #define REG_CAN0_MMR3                     (0x400B4260U) /**< \brief (CAN0) Mailbox Mode Register (MB = 3) */
  #define REG_CAN0_MAM3                     (0x400B4264U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 3) */
  #define REG_CAN0_MID3                     (0x400B4268U) /**< \brief (CAN0) Mailbox ID Register (MB = 3) */
  #define REG_CAN0_MFID3                    (0x400B426CU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 3) */
  #define REG_CAN0_MSR3                     (0x400B4270U) /**< \brief (CAN0) Mailbox Status Register (MB = 3) */
  #define REG_CAN0_MDL3                     (0x400B4274U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 3) */
  #define REG_CAN0_MDH3                     (0x400B4278U) /**< \brief (CAN0) Mailbox Data High Register (MB = 3) */
  #define REG_CAN0_MCR3                     (0x400B427CU) /**< \brief (CAN0) Mailbox Control Register (MB = 3) */
  #define REG_CAN0_MMR4                     (0x400B4280U) /**< \brief (CAN0) Mailbox Mode Register (MB = 4) */
  #define REG_CAN0_MAM4                     (0x400B4284U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 4) */
  #define REG_CAN0_MID4                     (0x400B4288U) /**< \brief (CAN0) Mailbox ID Register (MB = 4) */
  #define REG_CAN0_MFID4                    (0x400B428CU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 4) */
  #define REG_CAN0_MSR4                     (0x400B4290U) /**< \brief (CAN0) Mailbox Status Register (MB = 4) */
  #define REG_CAN0_MDL4                     (0x400B4294U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 4) */
  #define REG_CAN0_MDH4                     (0x400B4298U) /**< \brief (CAN0) Mailbox Data High Register (MB = 4) */
  #define REG_CAN0_MCR4                     (0x400B429CU) /**< \brief (CAN0) Mailbox Control Register (MB = 4) */
  #define REG_CAN0_MMR5                     (0x400B42A0U) /**< \brief (CAN0) Mailbox Mode Register (MB = 5) */
  #define REG_CAN0_MAM5                     (0x400B42A4U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 5) */
  #define REG_CAN0_MID5                     (0x400B42A8U) /**< \brief (CAN0) Mailbox ID Register (MB = 5) */
  #define REG_CAN0_MFID5                    (0x400B42ACU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 5) */
  #define REG_CAN0_MSR5                     (0x400B42B0U) /**< \brief (CAN0) Mailbox Status Register (MB = 5) */
  #define REG_CAN0_MDL5                     (0x400B42B4U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 5) */
  #define REG_CAN0_MDH5                     (0x400B42B8U) /**< \brief (CAN0) Mailbox Data High Register (MB = 5) */
  #define REG_CAN0_MCR5                     (0x400B42BCU) /**< \brief (CAN0) Mailbox Control Register (MB = 5) */
  #define REG_CAN0_MMR6                     (0x400B42C0U) /**< \brief (CAN0) Mailbox Mode Register (MB = 6) */
  #define REG_CAN0_MAM6                     (0x400B42C4U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 6) */
  #define REG_CAN0_MID6                     (0x400B42C8U) /**< \brief (CAN0) Mailbox ID Register (MB = 6) */
  #define REG_CAN0_MFID6                    (0x400B42CCU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 6) */
  #define REG_CAN0_MSR6                     (0x400B42D0U) /**< \brief (CAN0) Mailbox Status Register (MB = 6) */
  #define REG_CAN0_MDL6                     (0x400B42D4U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 6) */
  #define REG_CAN0_MDH6                     (0x400B42D8U) /**< \brief (CAN0) Mailbox Data High Register (MB = 6) */
  #define REG_CAN0_MCR6                     (0x400B42DCU) /**< \brief (CAN0) Mailbox Control Register (MB = 6) */
  #define REG_CAN0_MMR7                     (0x400B42E0U) /**< \brief (CAN0) Mailbox Mode Register (MB = 7) */
  #define REG_CAN0_MAM7                     (0x400B42E4U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 7) */
  #define REG_CAN0_MID7                     (0x400B42E8U) /**< \brief (CAN0) Mailbox ID Register (MB = 7) */
  #define REG_CAN0_MFID7                    (0x400B42ECU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 7) */
  #define REG_CAN0_MSR7                     (0x400B42F0U) /**< \brief (CAN0) Mailbox Status Register (MB = 7) */
  #define REG_CAN0_MDL7                     (0x400B42F4U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 7) */
  #define REG_CAN0_MDH7                     (0x400B42F8U) /**< \brief (CAN0) Mailbox Data High Register (MB = 7) */
  #define REG_CAN0_MCR7                     (0x400B42FCU) /**< \brief (CAN0) Mailbox Control Register (MB = 7) */
#else
  #define REG_CAN0_MR      (*(__IO uint32_t*)0x400B4000U) /**< \brief (CAN0) Mode Register */
  #define REG_CAN0_IER     (*(__O  uint32_t*)0x400B4004U) /**< \brief (CAN0) Interrupt Enable Register */
  #define REG_CAN0_IDR     (*(__O  uint32_t*)0x400B4008U) /**< \brief (CAN0) Interrupt Disable Register */
  #define REG_CAN0_IMR     (*(__I  uint32_t*)0x400B400CU) /**< \brief (CAN0) Interrupt Mask Register */
  #define REG_CAN0_SR      (*(__I  uint32_t*)0x400B4010U) /**< \brief (CAN0) Status Register */
  #define REG_CAN0_BR      (*(__IO uint32_t*)0x400B4014U) /**< \brief (CAN0) Baudrate Register */
  #define REG_CAN0_TIM     (*(__I  uint32_t*)0x400B4018U) /**< \brief (CAN0) Timer Register */
  #define REG_CAN0_TIMESTP (*(__I  uint32_t*)0x400B401CU) /**< \brief (CAN0) Timestamp Register */
  #define REG_CAN0_ECR     (*(__I  uint32_t*)0x400B4020U) /**< \brief (CAN0) Error Counter Register */
  #define REG_CAN0_TCR     (*(__O  uint32_t*)0x400B4024U) /**< \brief (CAN0) Transfer Command Register */
  #define REG_CAN0_ACR     (*(__O  uint32_t*)0x400B4028U) /**< \brief (CAN0) Abort Command Register */
  #define REG_CAN0_WPMR    (*(__IO uint32_t*)0x400B40E4U) /**< \brief (CAN0) Write Protect Mode Register */
  #define REG_CAN0_WPSR    (*(__I  uint32_t*)0x400B40E8U) /**< \brief (CAN0) Write Protect Status Register */
  #define REG_CAN0_MMR0    (*(__IO uint32_t*)0x400B4200U) /**< \brief (CAN0) Mailbox Mode Register (MB = 0) */
  #define REG_CAN0_MAM0    (*(__IO uint32_t*)0x400B4204U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 0) */
  #define REG_CAN0_MID0    (*(__IO uint32_t*)0x400B4208U) /**< \brief (CAN0) Mailbox ID Register (MB = 0) */
  #define REG_CAN0_MFID0   (*(__I  uint32_t*)0x400B420CU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 0) */
  #define REG_CAN0_MSR0    (*(__I  uint32_t*)0x400B4210U) /**< \brief (CAN0) Mailbox Status Register (MB = 0) */
  #define REG_CAN0_MDL0    (*(__IO uint32_t*)0x400B4214U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 0) */
  #define REG_CAN0_MDH0    (*(__IO uint32_t*)0x400B4218U) /**< \brief (CAN0) Mailbox Data High Register (MB = 0) */
  #define REG_CAN0_MCR0    (*(__O  uint32_t*)0x400B421CU) /**< \brief (CAN0) Mailbox Control Register (MB = 0) */
  #define REG_CAN0_MMR1    (*(__IO uint32_t*)0x400B4220U) /**< \brief (CAN0) Mailbox Mode Register (MB = 1) */
  #define REG_CAN0_MAM1    (*(__IO uint32_t*)0x400B4224U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 1) */
  #define REG_CAN0_MID1    (*(__IO uint32_t*)0x400B4228U) /**< \brief (CAN0) Mailbox ID Register (MB = 1) */
  #define REG_CAN0_MFID1   (*(__I  uint32_t*)0x400B422CU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 1) */
  #define REG_CAN0_MSR1    (*(__I  uint32_t*)0x400B4230U) /**< \brief (CAN0) Mailbox Status Register (MB = 1) */
  #define REG_CAN0_MDL1    (*(__IO uint32_t*)0x400B4234U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 1) */
  #define REG_CAN0_MDH1    (*(__IO uint32_t*)0x400B4238U) /**< \brief (CAN0) Mailbox Data High Register (MB = 1) */
  #define REG_CAN0_MCR1    (*(__O  uint32_t*)0x400B423CU) /**< \brief (CAN0) Mailbox Control Register (MB = 1) */
  #define REG_CAN0_MMR2    (*(__IO uint32_t*)0x400B4240U) /**< \brief (CAN0) Mailbox Mode Register (MB = 2) */
  #define REG_CAN0_MAM2    (*(__IO uint32_t*)0x400B4244U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 2) */
  #define REG_CAN0_MID2    (*(__IO uint32_t*)0x400B4248U) /**< \brief (CAN0) Mailbox ID Register (MB = 2) */
  #define REG_CAN0_MFID2   (*(__I  uint32_t*)0x400B424CU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 2) */
  #define REG_CAN0_MSR2    (*(__I  uint32_t*)0x400B4250U) /**< \brief (CAN0) Mailbox Status Register (MB = 2) */
  #define REG_CAN0_MDL2    (*(__IO uint32_t*)0x400B4254U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 2) */
  #define REG_CAN0_MDH2    (*(__IO uint32_t*)0x400B4258U) /**< \brief (CAN0) Mailbox Data High Register (MB = 2) */
  #define REG_CAN0_MCR2    (*(__O  uint32_t*)0x400B425CU) /**< \brief (CAN0) Mailbox Control Register (MB = 2) */
  #define REG_CAN0_MMR3    (*(__IO uint32_t*)0x400B4260U) /**< \brief (CAN0) Mailbox Mode Register (MB = 3) */
  #define REG_CAN0_MAM3    (*(__IO uint32_t*)0x400B4264U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 3) */
  #define REG_CAN0_MID3    (*(__IO uint32_t*)0x400B4268U) /**< \brief (CAN0) Mailbox ID Register (MB = 3) */
  #define REG_CAN0_MFID3   (*(__I  uint32_t*)0x400B426CU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 3) */
  #define REG_CAN0_MSR3    (*(__I  uint32_t*)0x400B4270U) /**< \brief (CAN0) Mailbox Status Register (MB = 3) */
  #define REG_CAN0_MDL3    (*(__IO uint32_t*)0x400B4274U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 3) */
  #define REG_CAN0_MDH3    (*(__IO uint32_t*)0x400B4278U) /**< \brief (CAN0) Mailbox Data High Register (MB = 3) */
  #define REG_CAN0_MCR3    (*(__O  uint32_t*)0x400B427CU) /**< \brief (CAN0) Mailbox Control Register (MB = 3) */
  #define REG_CAN0_MMR4    (*(__IO uint32_t*)0x400B4280U) /**< \brief (CAN0) Mailbox Mode Register (MB = 4) */
  #define REG_CAN0_MAM4    (*(__IO uint32_t*)0x400B4284U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 4) */
  #define REG_CAN0_MID4    (*(__IO uint32_t*)0x400B4288U) /**< \brief (CAN0) Mailbox ID Register (MB = 4) */
  #define REG_CAN0_MFID4   (*(__I  uint32_t*)0x400B428CU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 4) */
  #define REG_CAN0_MSR4    (*(__I  uint32_t*)0x400B4290U) /**< \brief (CAN0) Mailbox Status Register (MB = 4) */
  #define REG_CAN0_MDL4    (*(__IO uint32_t*)0x400B4294U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 4) */
  #define REG_CAN0_MDH4    (*(__IO uint32_t*)0x400B4298U) /**< \brief (CAN0) Mailbox Data High Register (MB = 4) */
  #define REG_CAN0_MCR4    (*(__O  uint32_t*)0x400B429CU) /**< \brief (CAN0) Mailbox Control Register (MB = 4) */
  #define REG_CAN0_MMR5    (*(__IO uint32_t*)0x400B42A0U) /**< \brief (CAN0) Mailbox Mode Register (MB = 5) */
  #define REG_CAN0_MAM5    (*(__IO uint32_t*)0x400B42A4U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 5) */
  #define REG_CAN0_MID5    (*(__IO uint32_t*)0x400B42A8U) /**< \brief (CAN0) Mailbox ID Register (MB = 5) */
  #define REG_CAN0_MFID5   (*(__I  uint32_t*)0x400B42ACU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 5) */
  #define REG_CAN0_MSR5    (*(__I  uint32_t*)0x400B42B0U) /**< \brief (CAN0) Mailbox Status Register (MB = 5) */
  #define REG_CAN0_MDL5    (*(__IO uint32_t*)0x400B42B4U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 5) */
  #define REG_CAN0_MDH5    (*(__IO uint32_t*)0x400B42B8U) /**< \brief (CAN0) Mailbox Data High Register (MB = 5) */
  #define REG_CAN0_MCR5    (*(__O  uint32_t*)0x400B42BCU) /**< \brief (CAN0) Mailbox Control Register (MB = 5) */
  #define REG_CAN0_MMR6    (*(__IO uint32_t*)0x400B42C0U) /**< \brief (CAN0) Mailbox Mode Register (MB = 6) */
  #define REG_CAN0_MAM6    (*(__IO uint32_t*)0x400B42C4U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 6) */
  #define REG_CAN0_MID6    (*(__IO uint32_t*)0x400B42C8U) /**< \brief (CAN0) Mailbox ID Register (MB = 6) */
  #define REG_CAN0_MFID6   (*(__I  uint32_t*)0x400B42CCU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 6) */
  #define REG_CAN0_MSR6    (*(__I  uint32_t*)0x400B42D0U) /**< \brief (CAN0) Mailbox Status Register (MB = 6) */
  #define REG_CAN0_MDL6    (*(__IO uint32_t*)0x400B42D4U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 6) */
  #define REG_CAN0_MDH6    (*(__IO uint32_t*)0x400B42D8U) /**< \brief (CAN0) Mailbox Data High Register (MB = 6) */
  #define REG_CAN0_MCR6    (*(__O  uint32_t*)0x400B42DCU) /**< \brief (CAN0) Mailbox Control Register (MB = 6) */
  #define REG_CAN0_MMR7    (*(__IO uint32_t*)0x400B42E0U) /**< \brief (CAN0) Mailbox Mode Register (MB = 7) */
  #define REG_CAN0_MAM7    (*(__IO uint32_t*)0x400B42E4U) /**< \brief (CAN0) Mailbox Acceptance Mask Register (MB = 7) */
  #define REG_CAN0_MID7    (*(__IO uint32_t*)0x400B42E8U) /**< \brief (CAN0) Mailbox ID Register (MB = 7) */
  #define REG_CAN0_MFID7   (*(__I  uint32_t*)0x400B42ECU) /**< \brief (CAN0) Mailbox Family ID Register (MB = 7) */
  #define REG_CAN0_MSR7    (*(__I  uint32_t*)0x400B42F0U) /**< \brief (CAN0) Mailbox Status Register (MB = 7) */
  #define REG_CAN0_MDL7    (*(__IO uint32_t*)0x400B42F4U) /**< \brief (CAN0) Mailbox Data Low Register (MB = 7) */
  #define REG_CAN0_MDH7    (*(__IO uint32_t*)0x400B42F8U) /**< \brief (CAN0) Mailbox Data High Register (MB = 7) */
  #define REG_CAN0_MCR7    (*(__O  uint32_t*)0x400B42FCU) /**< \brief (CAN0) Mailbox Control Register (MB = 7) */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM3XA_CAN0_INSTANCE_ */
