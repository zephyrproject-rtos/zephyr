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

#ifndef _SAM3XA_ADC_INSTANCE_
#define _SAM3XA_ADC_INSTANCE_

/* ========== Register definition for ADC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_ADC_CR                     (0x400C0000U) /**< \brief (ADC) Control Register */
  #define REG_ADC_MR                     (0x400C0004U) /**< \brief (ADC) Mode Register */
  #define REG_ADC_SEQR1                  (0x400C0008U) /**< \brief (ADC) Channel Sequence Register 1 */
  #define REG_ADC_SEQR2                  (0x400C000CU) /**< \brief (ADC) Channel Sequence Register 2 */
  #define REG_ADC_CHER                   (0x400C0010U) /**< \brief (ADC) Channel Enable Register */
  #define REG_ADC_CHDR                   (0x400C0014U) /**< \brief (ADC) Channel Disable Register */
  #define REG_ADC_CHSR                   (0x400C0018U) /**< \brief (ADC) Channel Status Register */
  #define REG_ADC_LCDR                   (0x400C0020U) /**< \brief (ADC) Last Converted Data Register */
  #define REG_ADC_IER                    (0x400C0024U) /**< \brief (ADC) Interrupt Enable Register */
  #define REG_ADC_IDR                    (0x400C0028U) /**< \brief (ADC) Interrupt Disable Register */
  #define REG_ADC_IMR                    (0x400C002CU) /**< \brief (ADC) Interrupt Mask Register */
  #define REG_ADC_ISR                    (0x400C0030U) /**< \brief (ADC) Interrupt Status Register */
  #define REG_ADC_OVER                   (0x400C003CU) /**< \brief (ADC) Overrun Status Register */
  #define REG_ADC_EMR                    (0x400C0040U) /**< \brief (ADC) Extended Mode Register */
  #define REG_ADC_CWR                    (0x400C0044U) /**< \brief (ADC) Compare Window Register */
  #define REG_ADC_CGR                    (0x400C0048U) /**< \brief (ADC) Channel Gain Register */
  #define REG_ADC_COR                    (0x400C004CU) /**< \brief (ADC) Channel Offset Register */
  #define REG_ADC_CDR                    (0x400C0050U) /**< \brief (ADC) Channel Data Register */
  #define REG_ADC_ACR                    (0x400C0094U) /**< \brief (ADC) Analog Control Register */
  #define REG_ADC_WPMR                   (0x400C00E4U) /**< \brief (ADC) Write Protect Mode Register */
  #define REG_ADC_WPSR                   (0x400C00E8U) /**< \brief (ADC) Write Protect Status Register */
  #define REG_ADC_RPR                    (0x400C0100U) /**< \brief (ADC) Receive Pointer Register */
  #define REG_ADC_RCR                    (0x400C0104U) /**< \brief (ADC) Receive Counter Register */
  #define REG_ADC_RNPR                   (0x400C0110U) /**< \brief (ADC) Receive Next Pointer Register */
  #define REG_ADC_RNCR                   (0x400C0114U) /**< \brief (ADC) Receive Next Counter Register */
  #define REG_ADC_PTCR                   (0x400C0120U) /**< \brief (ADC) Transfer Control Register */
  #define REG_ADC_PTSR                   (0x400C0124U) /**< \brief (ADC) Transfer Status Register */
#else
  #define REG_ADC_CR    (*(__O  uint32_t*)0x400C0000U) /**< \brief (ADC) Control Register */
  #define REG_ADC_MR    (*(__IO uint32_t*)0x400C0004U) /**< \brief (ADC) Mode Register */
  #define REG_ADC_SEQR1 (*(__IO uint32_t*)0x400C0008U) /**< \brief (ADC) Channel Sequence Register 1 */
  #define REG_ADC_SEQR2 (*(__IO uint32_t*)0x400C000CU) /**< \brief (ADC) Channel Sequence Register 2 */
  #define REG_ADC_CHER  (*(__O  uint32_t*)0x400C0010U) /**< \brief (ADC) Channel Enable Register */
  #define REG_ADC_CHDR  (*(__O  uint32_t*)0x400C0014U) /**< \brief (ADC) Channel Disable Register */
  #define REG_ADC_CHSR  (*(__I  uint32_t*)0x400C0018U) /**< \brief (ADC) Channel Status Register */
  #define REG_ADC_LCDR  (*(__I  uint32_t*)0x400C0020U) /**< \brief (ADC) Last Converted Data Register */
  #define REG_ADC_IER   (*(__O  uint32_t*)0x400C0024U) /**< \brief (ADC) Interrupt Enable Register */
  #define REG_ADC_IDR   (*(__O  uint32_t*)0x400C0028U) /**< \brief (ADC) Interrupt Disable Register */
  #define REG_ADC_IMR   (*(__I  uint32_t*)0x400C002CU) /**< \brief (ADC) Interrupt Mask Register */
  #define REG_ADC_ISR   (*(__I  uint32_t*)0x400C0030U) /**< \brief (ADC) Interrupt Status Register */
  #define REG_ADC_OVER  (*(__I  uint32_t*)0x400C003CU) /**< \brief (ADC) Overrun Status Register */
  #define REG_ADC_EMR   (*(__IO uint32_t*)0x400C0040U) /**< \brief (ADC) Extended Mode Register */
  #define REG_ADC_CWR   (*(__IO uint32_t*)0x400C0044U) /**< \brief (ADC) Compare Window Register */
  #define REG_ADC_CGR   (*(__IO uint32_t*)0x400C0048U) /**< \brief (ADC) Channel Gain Register */
  #define REG_ADC_COR   (*(__IO uint32_t*)0x400C004CU) /**< \brief (ADC) Channel Offset Register */
  #define REG_ADC_CDR   (*(__I  uint32_t*)0x400C0050U) /**< \brief (ADC) Channel Data Register */
  #define REG_ADC_ACR   (*(__IO uint32_t*)0x400C0094U) /**< \brief (ADC) Analog Control Register */
  #define REG_ADC_WPMR  (*(__IO uint32_t*)0x400C00E4U) /**< \brief (ADC) Write Protect Mode Register */
  #define REG_ADC_WPSR  (*(__I  uint32_t*)0x400C00E8U) /**< \brief (ADC) Write Protect Status Register */
  #define REG_ADC_RPR   (*(__IO uint32_t*)0x400C0100U) /**< \brief (ADC) Receive Pointer Register */
  #define REG_ADC_RCR   (*(__IO uint32_t*)0x400C0104U) /**< \brief (ADC) Receive Counter Register */
  #define REG_ADC_RNPR  (*(__IO uint32_t*)0x400C0110U) /**< \brief (ADC) Receive Next Pointer Register */
  #define REG_ADC_RNCR  (*(__IO uint32_t*)0x400C0114U) /**< \brief (ADC) Receive Next Counter Register */
  #define REG_ADC_PTCR  (*(__O  uint32_t*)0x400C0120U) /**< \brief (ADC) Transfer Control Register */
  #define REG_ADC_PTSR  (*(__I  uint32_t*)0x400C0124U) /**< \brief (ADC) Transfer Status Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM3XA_ADC_INSTANCE_ */
