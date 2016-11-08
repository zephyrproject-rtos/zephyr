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

#ifndef _SAM3XA_USART1_INSTANCE_
#define _SAM3XA_USART1_INSTANCE_

/* ========== Register definition for USART1 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_USART1_CR                      (0x4009C000U) /**< \brief (USART1) Control Register */
  #define REG_USART1_MR                      (0x4009C004U) /**< \brief (USART1) Mode Register */
  #define REG_USART1_IER                     (0x4009C008U) /**< \brief (USART1) Interrupt Enable Register */
  #define REG_USART1_IDR                     (0x4009C00CU) /**< \brief (USART1) Interrupt Disable Register */
  #define REG_USART1_IMR                     (0x4009C010U) /**< \brief (USART1) Interrupt Mask Register */
  #define REG_USART1_CSR                     (0x4009C014U) /**< \brief (USART1) Channel Status Register */
  #define REG_USART1_RHR                     (0x4009C018U) /**< \brief (USART1) Receive Holding Register */
  #define REG_USART1_THR                     (0x4009C01CU) /**< \brief (USART1) Transmit Holding Register */
  #define REG_USART1_BRGR                    (0x4009C020U) /**< \brief (USART1) Baud Rate Generator Register */
  #define REG_USART1_RTOR                    (0x4009C024U) /**< \brief (USART1) Receiver Time-out Register */
  #define REG_USART1_TTGR                    (0x4009C028U) /**< \brief (USART1) Transmitter Timeguard Register */
  #define REG_USART1_FIDI                    (0x4009C040U) /**< \brief (USART1) FI DI Ratio Register */
  #define REG_USART1_NER                     (0x4009C044U) /**< \brief (USART1) Number of Errors Register */
  #define REG_USART1_IF                      (0x4009C04CU) /**< \brief (USART1) IrDA Filter Register */
  #define REG_USART1_MAN                     (0x4009C050U) /**< \brief (USART1) Manchester Configuration Register */
  #define REG_USART1_LINMR                   (0x4009C054U) /**< \brief (USART1) LIN Mode Register */
  #define REG_USART1_LINIR                   (0x4009C058U) /**< \brief (USART1) LIN Identifier Register */
  #define REG_USART1_LINBRR                  (0x4009C05CU) /**< \brief (USART1) LIN Baud Rate Register */
  #define REG_USART1_WPMR                    (0x4009C0E4U) /**< \brief (USART1) Write Protection Mode Register */
  #define REG_USART1_WPSR                    (0x4009C0E8U) /**< \brief (USART1) Write Protection Status Register */
  #define REG_USART1_RPR                     (0x4009C100U) /**< \brief (USART1) Receive Pointer Register */
  #define REG_USART1_RCR                     (0x4009C104U) /**< \brief (USART1) Receive Counter Register */
  #define REG_USART1_TPR                     (0x4009C108U) /**< \brief (USART1) Transmit Pointer Register */
  #define REG_USART1_TCR                     (0x4009C10CU) /**< \brief (USART1) Transmit Counter Register */
  #define REG_USART1_RNPR                    (0x4009C110U) /**< \brief (USART1) Receive Next Pointer Register */
  #define REG_USART1_RNCR                    (0x4009C114U) /**< \brief (USART1) Receive Next Counter Register */
  #define REG_USART1_TNPR                    (0x4009C118U) /**< \brief (USART1) Transmit Next Pointer Register */
  #define REG_USART1_TNCR                    (0x4009C11CU) /**< \brief (USART1) Transmit Next Counter Register */
  #define REG_USART1_PTCR                    (0x4009C120U) /**< \brief (USART1) Transfer Control Register */
  #define REG_USART1_PTSR                    (0x4009C124U) /**< \brief (USART1) Transfer Status Register */
#else
  #define REG_USART1_CR     (*(__O  uint32_t*)0x4009C000U) /**< \brief (USART1) Control Register */
  #define REG_USART1_MR     (*(__IO uint32_t*)0x4009C004U) /**< \brief (USART1) Mode Register */
  #define REG_USART1_IER    (*(__O  uint32_t*)0x4009C008U) /**< \brief (USART1) Interrupt Enable Register */
  #define REG_USART1_IDR    (*(__O  uint32_t*)0x4009C00CU) /**< \brief (USART1) Interrupt Disable Register */
  #define REG_USART1_IMR    (*(__I  uint32_t*)0x4009C010U) /**< \brief (USART1) Interrupt Mask Register */
  #define REG_USART1_CSR    (*(__I  uint32_t*)0x4009C014U) /**< \brief (USART1) Channel Status Register */
  #define REG_USART1_RHR    (*(__I  uint32_t*)0x4009C018U) /**< \brief (USART1) Receive Holding Register */
  #define REG_USART1_THR    (*(__O  uint32_t*)0x4009C01CU) /**< \brief (USART1) Transmit Holding Register */
  #define REG_USART1_BRGR   (*(__IO uint32_t*)0x4009C020U) /**< \brief (USART1) Baud Rate Generator Register */
  #define REG_USART1_RTOR   (*(__IO uint32_t*)0x4009C024U) /**< \brief (USART1) Receiver Time-out Register */
  #define REG_USART1_TTGR   (*(__IO uint32_t*)0x4009C028U) /**< \brief (USART1) Transmitter Timeguard Register */
  #define REG_USART1_FIDI   (*(__IO uint32_t*)0x4009C040U) /**< \brief (USART1) FI DI Ratio Register */
  #define REG_USART1_NER    (*(__I  uint32_t*)0x4009C044U) /**< \brief (USART1) Number of Errors Register */
  #define REG_USART1_IF     (*(__IO uint32_t*)0x4009C04CU) /**< \brief (USART1) IrDA Filter Register */
  #define REG_USART1_MAN    (*(__IO uint32_t*)0x4009C050U) /**< \brief (USART1) Manchester Configuration Register */
  #define REG_USART1_LINMR  (*(__IO uint32_t*)0x4009C054U) /**< \brief (USART1) LIN Mode Register */
  #define REG_USART1_LINIR  (*(__IO uint32_t*)0x4009C058U) /**< \brief (USART1) LIN Identifier Register */
  #define REG_USART1_LINBRR (*(__I  uint32_t*)0x4009C05CU) /**< \brief (USART1) LIN Baud Rate Register */
  #define REG_USART1_WPMR   (*(__IO uint32_t*)0x4009C0E4U) /**< \brief (USART1) Write Protection Mode Register */
  #define REG_USART1_WPSR   (*(__I  uint32_t*)0x4009C0E8U) /**< \brief (USART1) Write Protection Status Register */
  #define REG_USART1_RPR    (*(__IO uint32_t*)0x4009C100U) /**< \brief (USART1) Receive Pointer Register */
  #define REG_USART1_RCR    (*(__IO uint32_t*)0x4009C104U) /**< \brief (USART1) Receive Counter Register */
  #define REG_USART1_TPR    (*(__IO uint32_t*)0x4009C108U) /**< \brief (USART1) Transmit Pointer Register */
  #define REG_USART1_TCR    (*(__IO uint32_t*)0x4009C10CU) /**< \brief (USART1) Transmit Counter Register */
  #define REG_USART1_RNPR   (*(__IO uint32_t*)0x4009C110U) /**< \brief (USART1) Receive Next Pointer Register */
  #define REG_USART1_RNCR   (*(__IO uint32_t*)0x4009C114U) /**< \brief (USART1) Receive Next Counter Register */
  #define REG_USART1_TNPR   (*(__IO uint32_t*)0x4009C118U) /**< \brief (USART1) Transmit Next Pointer Register */
  #define REG_USART1_TNCR   (*(__IO uint32_t*)0x4009C11CU) /**< \brief (USART1) Transmit Next Counter Register */
  #define REG_USART1_PTCR   (*(__O  uint32_t*)0x4009C120U) /**< \brief (USART1) Transfer Control Register */
  #define REG_USART1_PTSR   (*(__I  uint32_t*)0x4009C124U) /**< \brief (USART1) Transfer Status Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM3XA_USART1_INSTANCE_ */
