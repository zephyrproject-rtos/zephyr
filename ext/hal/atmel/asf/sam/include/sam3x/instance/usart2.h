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

#ifndef _SAM3XA_USART2_INSTANCE_
#define _SAM3XA_USART2_INSTANCE_

/* ========== Register definition for USART2 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_USART2_CR                      (0x400A0000U) /**< \brief (USART2) Control Register */
  #define REG_USART2_MR                      (0x400A0004U) /**< \brief (USART2) Mode Register */
  #define REG_USART2_IER                     (0x400A0008U) /**< \brief (USART2) Interrupt Enable Register */
  #define REG_USART2_IDR                     (0x400A000CU) /**< \brief (USART2) Interrupt Disable Register */
  #define REG_USART2_IMR                     (0x400A0010U) /**< \brief (USART2) Interrupt Mask Register */
  #define REG_USART2_CSR                     (0x400A0014U) /**< \brief (USART2) Channel Status Register */
  #define REG_USART2_RHR                     (0x400A0018U) /**< \brief (USART2) Receive Holding Register */
  #define REG_USART2_THR                     (0x400A001CU) /**< \brief (USART2) Transmit Holding Register */
  #define REG_USART2_BRGR                    (0x400A0020U) /**< \brief (USART2) Baud Rate Generator Register */
  #define REG_USART2_RTOR                    (0x400A0024U) /**< \brief (USART2) Receiver Time-out Register */
  #define REG_USART2_TTGR                    (0x400A0028U) /**< \brief (USART2) Transmitter Timeguard Register */
  #define REG_USART2_FIDI                    (0x400A0040U) /**< \brief (USART2) FI DI Ratio Register */
  #define REG_USART2_NER                     (0x400A0044U) /**< \brief (USART2) Number of Errors Register */
  #define REG_USART2_IF                      (0x400A004CU) /**< \brief (USART2) IrDA Filter Register */
  #define REG_USART2_MAN                     (0x400A0050U) /**< \brief (USART2) Manchester Configuration Register */
  #define REG_USART2_LINMR                   (0x400A0054U) /**< \brief (USART2) LIN Mode Register */
  #define REG_USART2_LINIR                   (0x400A0058U) /**< \brief (USART2) LIN Identifier Register */
  #define REG_USART2_LINBRR                  (0x400A005CU) /**< \brief (USART2) LIN Baud Rate Register */
  #define REG_USART2_WPMR                    (0x400A00E4U) /**< \brief (USART2) Write Protection Mode Register */
  #define REG_USART2_WPSR                    (0x400A00E8U) /**< \brief (USART2) Write Protection Status Register */
  #define REG_USART2_RPR                     (0x400A0100U) /**< \brief (USART2) Receive Pointer Register */
  #define REG_USART2_RCR                     (0x400A0104U) /**< \brief (USART2) Receive Counter Register */
  #define REG_USART2_TPR                     (0x400A0108U) /**< \brief (USART2) Transmit Pointer Register */
  #define REG_USART2_TCR                     (0x400A010CU) /**< \brief (USART2) Transmit Counter Register */
  #define REG_USART2_RNPR                    (0x400A0110U) /**< \brief (USART2) Receive Next Pointer Register */
  #define REG_USART2_RNCR                    (0x400A0114U) /**< \brief (USART2) Receive Next Counter Register */
  #define REG_USART2_TNPR                    (0x400A0118U) /**< \brief (USART2) Transmit Next Pointer Register */
  #define REG_USART2_TNCR                    (0x400A011CU) /**< \brief (USART2) Transmit Next Counter Register */
  #define REG_USART2_PTCR                    (0x400A0120U) /**< \brief (USART2) Transfer Control Register */
  #define REG_USART2_PTSR                    (0x400A0124U) /**< \brief (USART2) Transfer Status Register */
#else
  #define REG_USART2_CR     (*(__O  uint32_t*)0x400A0000U) /**< \brief (USART2) Control Register */
  #define REG_USART2_MR     (*(__IO uint32_t*)0x400A0004U) /**< \brief (USART2) Mode Register */
  #define REG_USART2_IER    (*(__O  uint32_t*)0x400A0008U) /**< \brief (USART2) Interrupt Enable Register */
  #define REG_USART2_IDR    (*(__O  uint32_t*)0x400A000CU) /**< \brief (USART2) Interrupt Disable Register */
  #define REG_USART2_IMR    (*(__I  uint32_t*)0x400A0010U) /**< \brief (USART2) Interrupt Mask Register */
  #define REG_USART2_CSR    (*(__I  uint32_t*)0x400A0014U) /**< \brief (USART2) Channel Status Register */
  #define REG_USART2_RHR    (*(__I  uint32_t*)0x400A0018U) /**< \brief (USART2) Receive Holding Register */
  #define REG_USART2_THR    (*(__O  uint32_t*)0x400A001CU) /**< \brief (USART2) Transmit Holding Register */
  #define REG_USART2_BRGR   (*(__IO uint32_t*)0x400A0020U) /**< \brief (USART2) Baud Rate Generator Register */
  #define REG_USART2_RTOR   (*(__IO uint32_t*)0x400A0024U) /**< \brief (USART2) Receiver Time-out Register */
  #define REG_USART2_TTGR   (*(__IO uint32_t*)0x400A0028U) /**< \brief (USART2) Transmitter Timeguard Register */
  #define REG_USART2_FIDI   (*(__IO uint32_t*)0x400A0040U) /**< \brief (USART2) FI DI Ratio Register */
  #define REG_USART2_NER    (*(__I  uint32_t*)0x400A0044U) /**< \brief (USART2) Number of Errors Register */
  #define REG_USART2_IF     (*(__IO uint32_t*)0x400A004CU) /**< \brief (USART2) IrDA Filter Register */
  #define REG_USART2_MAN    (*(__IO uint32_t*)0x400A0050U) /**< \brief (USART2) Manchester Configuration Register */
  #define REG_USART2_LINMR  (*(__IO uint32_t*)0x400A0054U) /**< \brief (USART2) LIN Mode Register */
  #define REG_USART2_LINIR  (*(__IO uint32_t*)0x400A0058U) /**< \brief (USART2) LIN Identifier Register */
  #define REG_USART2_LINBRR (*(__I  uint32_t*)0x400A005CU) /**< \brief (USART2) LIN Baud Rate Register */
  #define REG_USART2_WPMR   (*(__IO uint32_t*)0x400A00E4U) /**< \brief (USART2) Write Protection Mode Register */
  #define REG_USART2_WPSR   (*(__I  uint32_t*)0x400A00E8U) /**< \brief (USART2) Write Protection Status Register */
  #define REG_USART2_RPR    (*(__IO uint32_t*)0x400A0100U) /**< \brief (USART2) Receive Pointer Register */
  #define REG_USART2_RCR    (*(__IO uint32_t*)0x400A0104U) /**< \brief (USART2) Receive Counter Register */
  #define REG_USART2_TPR    (*(__IO uint32_t*)0x400A0108U) /**< \brief (USART2) Transmit Pointer Register */
  #define REG_USART2_TCR    (*(__IO uint32_t*)0x400A010CU) /**< \brief (USART2) Transmit Counter Register */
  #define REG_USART2_RNPR   (*(__IO uint32_t*)0x400A0110U) /**< \brief (USART2) Receive Next Pointer Register */
  #define REG_USART2_RNCR   (*(__IO uint32_t*)0x400A0114U) /**< \brief (USART2) Receive Next Counter Register */
  #define REG_USART2_TNPR   (*(__IO uint32_t*)0x400A0118U) /**< \brief (USART2) Transmit Next Pointer Register */
  #define REG_USART2_TNCR   (*(__IO uint32_t*)0x400A011CU) /**< \brief (USART2) Transmit Next Counter Register */
  #define REG_USART2_PTCR   (*(__O  uint32_t*)0x400A0120U) /**< \brief (USART2) Transfer Control Register */
  #define REG_USART2_PTSR   (*(__I  uint32_t*)0x400A0124U) /**< \brief (USART2) Transfer Status Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM3XA_USART2_INSTANCE_ */
