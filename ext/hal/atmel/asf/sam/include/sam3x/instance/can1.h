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

#ifndef _SAM3XA_CAN1_INSTANCE_
#define _SAM3XA_CAN1_INSTANCE_

/* ========== Register definition for CAN1 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_CAN1_MR                       (0x400B8000U) /**< \brief (CAN1) Mode Register */
  #define REG_CAN1_IER                      (0x400B8004U) /**< \brief (CAN1) Interrupt Enable Register */
  #define REG_CAN1_IDR                      (0x400B8008U) /**< \brief (CAN1) Interrupt Disable Register */
  #define REG_CAN1_IMR                      (0x400B800CU) /**< \brief (CAN1) Interrupt Mask Register */
  #define REG_CAN1_SR                       (0x400B8010U) /**< \brief (CAN1) Status Register */
  #define REG_CAN1_BR                       (0x400B8014U) /**< \brief (CAN1) Baudrate Register */
  #define REG_CAN1_TIM                      (0x400B8018U) /**< \brief (CAN1) Timer Register */
  #define REG_CAN1_TIMESTP                  (0x400B801CU) /**< \brief (CAN1) Timestamp Register */
  #define REG_CAN1_ECR                      (0x400B8020U) /**< \brief (CAN1) Error Counter Register */
  #define REG_CAN1_TCR                      (0x400B8024U) /**< \brief (CAN1) Transfer Command Register */
  #define REG_CAN1_ACR                      (0x400B8028U) /**< \brief (CAN1) Abort Command Register */
  #define REG_CAN1_WPMR                     (0x400B80E4U) /**< \brief (CAN1) Write Protect Mode Register */
  #define REG_CAN1_WPSR                     (0x400B80E8U) /**< \brief (CAN1) Write Protect Status Register */
  #define REG_CAN1_MMR0                     (0x400B8200U) /**< \brief (CAN1) Mailbox Mode Register (MB = 0) */
  #define REG_CAN1_MAM0                     (0x400B8204U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 0) */
  #define REG_CAN1_MID0                     (0x400B8208U) /**< \brief (CAN1) Mailbox ID Register (MB = 0) */
  #define REG_CAN1_MFID0                    (0x400B820CU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 0) */
  #define REG_CAN1_MSR0                     (0x400B8210U) /**< \brief (CAN1) Mailbox Status Register (MB = 0) */
  #define REG_CAN1_MDL0                     (0x400B8214U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 0) */
  #define REG_CAN1_MDH0                     (0x400B8218U) /**< \brief (CAN1) Mailbox Data High Register (MB = 0) */
  #define REG_CAN1_MCR0                     (0x400B821CU) /**< \brief (CAN1) Mailbox Control Register (MB = 0) */
  #define REG_CAN1_MMR1                     (0x400B8220U) /**< \brief (CAN1) Mailbox Mode Register (MB = 1) */
  #define REG_CAN1_MAM1                     (0x400B8224U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 1) */
  #define REG_CAN1_MID1                     (0x400B8228U) /**< \brief (CAN1) Mailbox ID Register (MB = 1) */
  #define REG_CAN1_MFID1                    (0x400B822CU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 1) */
  #define REG_CAN1_MSR1                     (0x400B8230U) /**< \brief (CAN1) Mailbox Status Register (MB = 1) */
  #define REG_CAN1_MDL1                     (0x400B8234U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 1) */
  #define REG_CAN1_MDH1                     (0x400B8238U) /**< \brief (CAN1) Mailbox Data High Register (MB = 1) */
  #define REG_CAN1_MCR1                     (0x400B823CU) /**< \brief (CAN1) Mailbox Control Register (MB = 1) */
  #define REG_CAN1_MMR2                     (0x400B8240U) /**< \brief (CAN1) Mailbox Mode Register (MB = 2) */
  #define REG_CAN1_MAM2                     (0x400B8244U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 2) */
  #define REG_CAN1_MID2                     (0x400B8248U) /**< \brief (CAN1) Mailbox ID Register (MB = 2) */
  #define REG_CAN1_MFID2                    (0x400B824CU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 2) */
  #define REG_CAN1_MSR2                     (0x400B8250U) /**< \brief (CAN1) Mailbox Status Register (MB = 2) */
  #define REG_CAN1_MDL2                     (0x400B8254U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 2) */
  #define REG_CAN1_MDH2                     (0x400B8258U) /**< \brief (CAN1) Mailbox Data High Register (MB = 2) */
  #define REG_CAN1_MCR2                     (0x400B825CU) /**< \brief (CAN1) Mailbox Control Register (MB = 2) */
  #define REG_CAN1_MMR3                     (0x400B8260U) /**< \brief (CAN1) Mailbox Mode Register (MB = 3) */
  #define REG_CAN1_MAM3                     (0x400B8264U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 3) */
  #define REG_CAN1_MID3                     (0x400B8268U) /**< \brief (CAN1) Mailbox ID Register (MB = 3) */
  #define REG_CAN1_MFID3                    (0x400B826CU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 3) */
  #define REG_CAN1_MSR3                     (0x400B8270U) /**< \brief (CAN1) Mailbox Status Register (MB = 3) */
  #define REG_CAN1_MDL3                     (0x400B8274U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 3) */
  #define REG_CAN1_MDH3                     (0x400B8278U) /**< \brief (CAN1) Mailbox Data High Register (MB = 3) */
  #define REG_CAN1_MCR3                     (0x400B827CU) /**< \brief (CAN1) Mailbox Control Register (MB = 3) */
  #define REG_CAN1_MMR4                     (0x400B8280U) /**< \brief (CAN1) Mailbox Mode Register (MB = 4) */
  #define REG_CAN1_MAM4                     (0x400B8284U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 4) */
  #define REG_CAN1_MID4                     (0x400B8288U) /**< \brief (CAN1) Mailbox ID Register (MB = 4) */
  #define REG_CAN1_MFID4                    (0x400B828CU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 4) */
  #define REG_CAN1_MSR4                     (0x400B8290U) /**< \brief (CAN1) Mailbox Status Register (MB = 4) */
  #define REG_CAN1_MDL4                     (0x400B8294U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 4) */
  #define REG_CAN1_MDH4                     (0x400B8298U) /**< \brief (CAN1) Mailbox Data High Register (MB = 4) */
  #define REG_CAN1_MCR4                     (0x400B829CU) /**< \brief (CAN1) Mailbox Control Register (MB = 4) */
  #define REG_CAN1_MMR5                     (0x400B82A0U) /**< \brief (CAN1) Mailbox Mode Register (MB = 5) */
  #define REG_CAN1_MAM5                     (0x400B82A4U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 5) */
  #define REG_CAN1_MID5                     (0x400B82A8U) /**< \brief (CAN1) Mailbox ID Register (MB = 5) */
  #define REG_CAN1_MFID5                    (0x400B82ACU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 5) */
  #define REG_CAN1_MSR5                     (0x400B82B0U) /**< \brief (CAN1) Mailbox Status Register (MB = 5) */
  #define REG_CAN1_MDL5                     (0x400B82B4U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 5) */
  #define REG_CAN1_MDH5                     (0x400B82B8U) /**< \brief (CAN1) Mailbox Data High Register (MB = 5) */
  #define REG_CAN1_MCR5                     (0x400B82BCU) /**< \brief (CAN1) Mailbox Control Register (MB = 5) */
  #define REG_CAN1_MMR6                     (0x400B82C0U) /**< \brief (CAN1) Mailbox Mode Register (MB = 6) */
  #define REG_CAN1_MAM6                     (0x400B82C4U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 6) */
  #define REG_CAN1_MID6                     (0x400B82C8U) /**< \brief (CAN1) Mailbox ID Register (MB = 6) */
  #define REG_CAN1_MFID6                    (0x400B82CCU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 6) */
  #define REG_CAN1_MSR6                     (0x400B82D0U) /**< \brief (CAN1) Mailbox Status Register (MB = 6) */
  #define REG_CAN1_MDL6                     (0x400B82D4U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 6) */
  #define REG_CAN1_MDH6                     (0x400B82D8U) /**< \brief (CAN1) Mailbox Data High Register (MB = 6) */
  #define REG_CAN1_MCR6                     (0x400B82DCU) /**< \brief (CAN1) Mailbox Control Register (MB = 6) */
  #define REG_CAN1_MMR7                     (0x400B82E0U) /**< \brief (CAN1) Mailbox Mode Register (MB = 7) */
  #define REG_CAN1_MAM7                     (0x400B82E4U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 7) */
  #define REG_CAN1_MID7                     (0x400B82E8U) /**< \brief (CAN1) Mailbox ID Register (MB = 7) */
  #define REG_CAN1_MFID7                    (0x400B82ECU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 7) */
  #define REG_CAN1_MSR7                     (0x400B82F0U) /**< \brief (CAN1) Mailbox Status Register (MB = 7) */
  #define REG_CAN1_MDL7                     (0x400B82F4U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 7) */
  #define REG_CAN1_MDH7                     (0x400B82F8U) /**< \brief (CAN1) Mailbox Data High Register (MB = 7) */
  #define REG_CAN1_MCR7                     (0x400B82FCU) /**< \brief (CAN1) Mailbox Control Register (MB = 7) */
#else
  #define REG_CAN1_MR      (*(__IO uint32_t*)0x400B8000U) /**< \brief (CAN1) Mode Register */
  #define REG_CAN1_IER     (*(__O  uint32_t*)0x400B8004U) /**< \brief (CAN1) Interrupt Enable Register */
  #define REG_CAN1_IDR     (*(__O  uint32_t*)0x400B8008U) /**< \brief (CAN1) Interrupt Disable Register */
  #define REG_CAN1_IMR     (*(__I  uint32_t*)0x400B800CU) /**< \brief (CAN1) Interrupt Mask Register */
  #define REG_CAN1_SR      (*(__I  uint32_t*)0x400B8010U) /**< \brief (CAN1) Status Register */
  #define REG_CAN1_BR      (*(__IO uint32_t*)0x400B8014U) /**< \brief (CAN1) Baudrate Register */
  #define REG_CAN1_TIM     (*(__I  uint32_t*)0x400B8018U) /**< \brief (CAN1) Timer Register */
  #define REG_CAN1_TIMESTP (*(__I  uint32_t*)0x400B801CU) /**< \brief (CAN1) Timestamp Register */
  #define REG_CAN1_ECR     (*(__I  uint32_t*)0x400B8020U) /**< \brief (CAN1) Error Counter Register */
  #define REG_CAN1_TCR     (*(__O  uint32_t*)0x400B8024U) /**< \brief (CAN1) Transfer Command Register */
  #define REG_CAN1_ACR     (*(__O  uint32_t*)0x400B8028U) /**< \brief (CAN1) Abort Command Register */
  #define REG_CAN1_WPMR    (*(__IO uint32_t*)0x400B80E4U) /**< \brief (CAN1) Write Protect Mode Register */
  #define REG_CAN1_WPSR    (*(__I  uint32_t*)0x400B80E8U) /**< \brief (CAN1) Write Protect Status Register */
  #define REG_CAN1_MMR0    (*(__IO uint32_t*)0x400B8200U) /**< \brief (CAN1) Mailbox Mode Register (MB = 0) */
  #define REG_CAN1_MAM0    (*(__IO uint32_t*)0x400B8204U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 0) */
  #define REG_CAN1_MID0    (*(__IO uint32_t*)0x400B8208U) /**< \brief (CAN1) Mailbox ID Register (MB = 0) */
  #define REG_CAN1_MFID0   (*(__I  uint32_t*)0x400B820CU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 0) */
  #define REG_CAN1_MSR0    (*(__I  uint32_t*)0x400B8210U) /**< \brief (CAN1) Mailbox Status Register (MB = 0) */
  #define REG_CAN1_MDL0    (*(__IO uint32_t*)0x400B8214U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 0) */
  #define REG_CAN1_MDH0    (*(__IO uint32_t*)0x400B8218U) /**< \brief (CAN1) Mailbox Data High Register (MB = 0) */
  #define REG_CAN1_MCR0    (*(__O  uint32_t*)0x400B821CU) /**< \brief (CAN1) Mailbox Control Register (MB = 0) */
  #define REG_CAN1_MMR1    (*(__IO uint32_t*)0x400B8220U) /**< \brief (CAN1) Mailbox Mode Register (MB = 1) */
  #define REG_CAN1_MAM1    (*(__IO uint32_t*)0x400B8224U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 1) */
  #define REG_CAN1_MID1    (*(__IO uint32_t*)0x400B8228U) /**< \brief (CAN1) Mailbox ID Register (MB = 1) */
  #define REG_CAN1_MFID1   (*(__I  uint32_t*)0x400B822CU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 1) */
  #define REG_CAN1_MSR1    (*(__I  uint32_t*)0x400B8230U) /**< \brief (CAN1) Mailbox Status Register (MB = 1) */
  #define REG_CAN1_MDL1    (*(__IO uint32_t*)0x400B8234U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 1) */
  #define REG_CAN1_MDH1    (*(__IO uint32_t*)0x400B8238U) /**< \brief (CAN1) Mailbox Data High Register (MB = 1) */
  #define REG_CAN1_MCR1    (*(__O  uint32_t*)0x400B823CU) /**< \brief (CAN1) Mailbox Control Register (MB = 1) */
  #define REG_CAN1_MMR2    (*(__IO uint32_t*)0x400B8240U) /**< \brief (CAN1) Mailbox Mode Register (MB = 2) */
  #define REG_CAN1_MAM2    (*(__IO uint32_t*)0x400B8244U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 2) */
  #define REG_CAN1_MID2    (*(__IO uint32_t*)0x400B8248U) /**< \brief (CAN1) Mailbox ID Register (MB = 2) */
  #define REG_CAN1_MFID2   (*(__I  uint32_t*)0x400B824CU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 2) */
  #define REG_CAN1_MSR2    (*(__I  uint32_t*)0x400B8250U) /**< \brief (CAN1) Mailbox Status Register (MB = 2) */
  #define REG_CAN1_MDL2    (*(__IO uint32_t*)0x400B8254U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 2) */
  #define REG_CAN1_MDH2    (*(__IO uint32_t*)0x400B8258U) /**< \brief (CAN1) Mailbox Data High Register (MB = 2) */
  #define REG_CAN1_MCR2    (*(__O  uint32_t*)0x400B825CU) /**< \brief (CAN1) Mailbox Control Register (MB = 2) */
  #define REG_CAN1_MMR3    (*(__IO uint32_t*)0x400B8260U) /**< \brief (CAN1) Mailbox Mode Register (MB = 3) */
  #define REG_CAN1_MAM3    (*(__IO uint32_t*)0x400B8264U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 3) */
  #define REG_CAN1_MID3    (*(__IO uint32_t*)0x400B8268U) /**< \brief (CAN1) Mailbox ID Register (MB = 3) */
  #define REG_CAN1_MFID3   (*(__I  uint32_t*)0x400B826CU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 3) */
  #define REG_CAN1_MSR3    (*(__I  uint32_t*)0x400B8270U) /**< \brief (CAN1) Mailbox Status Register (MB = 3) */
  #define REG_CAN1_MDL3    (*(__IO uint32_t*)0x400B8274U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 3) */
  #define REG_CAN1_MDH3    (*(__IO uint32_t*)0x400B8278U) /**< \brief (CAN1) Mailbox Data High Register (MB = 3) */
  #define REG_CAN1_MCR3    (*(__O  uint32_t*)0x400B827CU) /**< \brief (CAN1) Mailbox Control Register (MB = 3) */
  #define REG_CAN1_MMR4    (*(__IO uint32_t*)0x400B8280U) /**< \brief (CAN1) Mailbox Mode Register (MB = 4) */
  #define REG_CAN1_MAM4    (*(__IO uint32_t*)0x400B8284U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 4) */
  #define REG_CAN1_MID4    (*(__IO uint32_t*)0x400B8288U) /**< \brief (CAN1) Mailbox ID Register (MB = 4) */
  #define REG_CAN1_MFID4   (*(__I  uint32_t*)0x400B828CU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 4) */
  #define REG_CAN1_MSR4    (*(__I  uint32_t*)0x400B8290U) /**< \brief (CAN1) Mailbox Status Register (MB = 4) */
  #define REG_CAN1_MDL4    (*(__IO uint32_t*)0x400B8294U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 4) */
  #define REG_CAN1_MDH4    (*(__IO uint32_t*)0x400B8298U) /**< \brief (CAN1) Mailbox Data High Register (MB = 4) */
  #define REG_CAN1_MCR4    (*(__O  uint32_t*)0x400B829CU) /**< \brief (CAN1) Mailbox Control Register (MB = 4) */
  #define REG_CAN1_MMR5    (*(__IO uint32_t*)0x400B82A0U) /**< \brief (CAN1) Mailbox Mode Register (MB = 5) */
  #define REG_CAN1_MAM5    (*(__IO uint32_t*)0x400B82A4U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 5) */
  #define REG_CAN1_MID5    (*(__IO uint32_t*)0x400B82A8U) /**< \brief (CAN1) Mailbox ID Register (MB = 5) */
  #define REG_CAN1_MFID5   (*(__I  uint32_t*)0x400B82ACU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 5) */
  #define REG_CAN1_MSR5    (*(__I  uint32_t*)0x400B82B0U) /**< \brief (CAN1) Mailbox Status Register (MB = 5) */
  #define REG_CAN1_MDL5    (*(__IO uint32_t*)0x400B82B4U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 5) */
  #define REG_CAN1_MDH5    (*(__IO uint32_t*)0x400B82B8U) /**< \brief (CAN1) Mailbox Data High Register (MB = 5) */
  #define REG_CAN1_MCR5    (*(__O  uint32_t*)0x400B82BCU) /**< \brief (CAN1) Mailbox Control Register (MB = 5) */
  #define REG_CAN1_MMR6    (*(__IO uint32_t*)0x400B82C0U) /**< \brief (CAN1) Mailbox Mode Register (MB = 6) */
  #define REG_CAN1_MAM6    (*(__IO uint32_t*)0x400B82C4U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 6) */
  #define REG_CAN1_MID6    (*(__IO uint32_t*)0x400B82C8U) /**< \brief (CAN1) Mailbox ID Register (MB = 6) */
  #define REG_CAN1_MFID6   (*(__I  uint32_t*)0x400B82CCU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 6) */
  #define REG_CAN1_MSR6    (*(__I  uint32_t*)0x400B82D0U) /**< \brief (CAN1) Mailbox Status Register (MB = 6) */
  #define REG_CAN1_MDL6    (*(__IO uint32_t*)0x400B82D4U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 6) */
  #define REG_CAN1_MDH6    (*(__IO uint32_t*)0x400B82D8U) /**< \brief (CAN1) Mailbox Data High Register (MB = 6) */
  #define REG_CAN1_MCR6    (*(__O  uint32_t*)0x400B82DCU) /**< \brief (CAN1) Mailbox Control Register (MB = 6) */
  #define REG_CAN1_MMR7    (*(__IO uint32_t*)0x400B82E0U) /**< \brief (CAN1) Mailbox Mode Register (MB = 7) */
  #define REG_CAN1_MAM7    (*(__IO uint32_t*)0x400B82E4U) /**< \brief (CAN1) Mailbox Acceptance Mask Register (MB = 7) */
  #define REG_CAN1_MID7    (*(__IO uint32_t*)0x400B82E8U) /**< \brief (CAN1) Mailbox ID Register (MB = 7) */
  #define REG_CAN1_MFID7   (*(__I  uint32_t*)0x400B82ECU) /**< \brief (CAN1) Mailbox Family ID Register (MB = 7) */
  #define REG_CAN1_MSR7    (*(__I  uint32_t*)0x400B82F0U) /**< \brief (CAN1) Mailbox Status Register (MB = 7) */
  #define REG_CAN1_MDL7    (*(__IO uint32_t*)0x400B82F4U) /**< \brief (CAN1) Mailbox Data Low Register (MB = 7) */
  #define REG_CAN1_MDH7    (*(__IO uint32_t*)0x400B82F8U) /**< \brief (CAN1) Mailbox Data High Register (MB = 7) */
  #define REG_CAN1_MCR7    (*(__O  uint32_t*)0x400B82FCU) /**< \brief (CAN1) Mailbox Control Register (MB = 7) */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM3XA_CAN1_INSTANCE_ */
