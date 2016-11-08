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

#ifndef _SAM3XA_DACC_INSTANCE_
#define _SAM3XA_DACC_INSTANCE_

/* ========== Register definition for DACC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_DACC_CR                    (0x400C8000U) /**< \brief (DACC) Control Register */
  #define REG_DACC_MR                    (0x400C8004U) /**< \brief (DACC) Mode Register */
  #define REG_DACC_CHER                  (0x400C8010U) /**< \brief (DACC) Channel Enable Register */
  #define REG_DACC_CHDR                  (0x400C8014U) /**< \brief (DACC) Channel Disable Register */
  #define REG_DACC_CHSR                  (0x400C8018U) /**< \brief (DACC) Channel Status Register */
  #define REG_DACC_CDR                   (0x400C8020U) /**< \brief (DACC) Conversion Data Register */
  #define REG_DACC_IER                   (0x400C8024U) /**< \brief (DACC) Interrupt Enable Register */
  #define REG_DACC_IDR                   (0x400C8028U) /**< \brief (DACC) Interrupt Disable Register */
  #define REG_DACC_IMR                   (0x400C802CU) /**< \brief (DACC) Interrupt Mask Register */
  #define REG_DACC_ISR                   (0x400C8030U) /**< \brief (DACC) Interrupt Status Register */
  #define REG_DACC_ACR                   (0x400C8094U) /**< \brief (DACC) Analog Current Register */
  #define REG_DACC_WPMR                  (0x400C80E4U) /**< \brief (DACC) Write Protect Mode register */
  #define REG_DACC_WPSR                  (0x400C80E8U) /**< \brief (DACC) Write Protect Status register */
  #define REG_DACC_TPR                   (0x400C8108U) /**< \brief (DACC) Transmit Pointer Register */
  #define REG_DACC_TCR                   (0x400C810CU) /**< \brief (DACC) Transmit Counter Register */
  #define REG_DACC_TNPR                  (0x400C8118U) /**< \brief (DACC) Transmit Next Pointer Register */
  #define REG_DACC_TNCR                  (0x400C811CU) /**< \brief (DACC) Transmit Next Counter Register */
  #define REG_DACC_PTCR                  (0x400C8120U) /**< \brief (DACC) Transfer Control Register */
  #define REG_DACC_PTSR                  (0x400C8124U) /**< \brief (DACC) Transfer Status Register */
#else
  #define REG_DACC_CR   (*(__O  uint32_t*)0x400C8000U) /**< \brief (DACC) Control Register */
  #define REG_DACC_MR   (*(__IO uint32_t*)0x400C8004U) /**< \brief (DACC) Mode Register */
  #define REG_DACC_CHER (*(__O  uint32_t*)0x400C8010U) /**< \brief (DACC) Channel Enable Register */
  #define REG_DACC_CHDR (*(__O  uint32_t*)0x400C8014U) /**< \brief (DACC) Channel Disable Register */
  #define REG_DACC_CHSR (*(__I  uint32_t*)0x400C8018U) /**< \brief (DACC) Channel Status Register */
  #define REG_DACC_CDR  (*(__O  uint32_t*)0x400C8020U) /**< \brief (DACC) Conversion Data Register */
  #define REG_DACC_IER  (*(__O  uint32_t*)0x400C8024U) /**< \brief (DACC) Interrupt Enable Register */
  #define REG_DACC_IDR  (*(__O  uint32_t*)0x400C8028U) /**< \brief (DACC) Interrupt Disable Register */
  #define REG_DACC_IMR  (*(__I  uint32_t*)0x400C802CU) /**< \brief (DACC) Interrupt Mask Register */
  #define REG_DACC_ISR  (*(__I  uint32_t*)0x400C8030U) /**< \brief (DACC) Interrupt Status Register */
  #define REG_DACC_ACR  (*(__IO uint32_t*)0x400C8094U) /**< \brief (DACC) Analog Current Register */
  #define REG_DACC_WPMR (*(__IO uint32_t*)0x400C80E4U) /**< \brief (DACC) Write Protect Mode register */
  #define REG_DACC_WPSR (*(__I  uint32_t*)0x400C80E8U) /**< \brief (DACC) Write Protect Status register */
  #define REG_DACC_TPR  (*(__IO uint32_t*)0x400C8108U) /**< \brief (DACC) Transmit Pointer Register */
  #define REG_DACC_TCR  (*(__IO uint32_t*)0x400C810CU) /**< \brief (DACC) Transmit Counter Register */
  #define REG_DACC_TNPR (*(__IO uint32_t*)0x400C8118U) /**< \brief (DACC) Transmit Next Pointer Register */
  #define REG_DACC_TNCR (*(__IO uint32_t*)0x400C811CU) /**< \brief (DACC) Transmit Next Counter Register */
  #define REG_DACC_PTCR (*(__O  uint32_t*)0x400C8120U) /**< \brief (DACC) Transfer Control Register */
  #define REG_DACC_PTSR (*(__I  uint32_t*)0x400C8124U) /**< \brief (DACC) Transfer Status Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM3XA_DACC_INSTANCE_ */
