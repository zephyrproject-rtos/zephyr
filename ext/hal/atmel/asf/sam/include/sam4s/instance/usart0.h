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

#ifndef _SAM4S_USART0_INSTANCE_
#define _SAM4S_USART0_INSTANCE_

/* ========== Register definition for USART0 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_USART0_CR                    (0x40024000U) /**< \brief (USART0) Control Register */
  #define REG_USART0_MR                    (0x40024004U) /**< \brief (USART0) Mode Register */
  #define REG_USART0_IER                   (0x40024008U) /**< \brief (USART0) Interrupt Enable Register */
  #define REG_USART0_IDR                   (0x4002400CU) /**< \brief (USART0) Interrupt Disable Register */
  #define REG_USART0_IMR                   (0x40024010U) /**< \brief (USART0) Interrupt Mask Register */
  #define REG_USART0_CSR                   (0x40024014U) /**< \brief (USART0) Channel Status Register */
  #define REG_USART0_RHR                   (0x40024018U) /**< \brief (USART0) Receive Holding Register */
  #define REG_USART0_THR                   (0x4002401CU) /**< \brief (USART0) Transmit Holding Register */
  #define REG_USART0_BRGR                  (0x40024020U) /**< \brief (USART0) Baud Rate Generator Register */
  #define REG_USART0_RTOR                  (0x40024024U) /**< \brief (USART0) Receiver Time-out Register */
  #define REG_USART0_TTGR                  (0x40024028U) /**< \brief (USART0) Transmitter Timeguard Register */
  #define REG_USART0_FIDI                  (0x40024040U) /**< \brief (USART0) FI DI Ratio Register */
  #define REG_USART0_NER                   (0x40024044U) /**< \brief (USART0) Number of Errors Register */
  #define REG_USART0_IF                    (0x4002404CU) /**< \brief (USART0) IrDA Filter Register */
  #define REG_USART0_MAN                   (0x40024050U) /**< \brief (USART0) Manchester Configuration Register */
  #define REG_USART0_WPMR                  (0x400240E4U) /**< \brief (USART0) Write Protection Mode Register */
  #define REG_USART0_WPSR                  (0x400240E8U) /**< \brief (USART0) Write Protection Status Register */
  #define REG_USART0_RPR                   (0x40024100U) /**< \brief (USART0) Receive Pointer Register */
  #define REG_USART0_RCR                   (0x40024104U) /**< \brief (USART0) Receive Counter Register */
  #define REG_USART0_TPR                   (0x40024108U) /**< \brief (USART0) Transmit Pointer Register */
  #define REG_USART0_TCR                   (0x4002410CU) /**< \brief (USART0) Transmit Counter Register */
  #define REG_USART0_RNPR                  (0x40024110U) /**< \brief (USART0) Receive Next Pointer Register */
  #define REG_USART0_RNCR                  (0x40024114U) /**< \brief (USART0) Receive Next Counter Register */
  #define REG_USART0_TNPR                  (0x40024118U) /**< \brief (USART0) Transmit Next Pointer Register */
  #define REG_USART0_TNCR                  (0x4002411CU) /**< \brief (USART0) Transmit Next Counter Register */
  #define REG_USART0_PTCR                  (0x40024120U) /**< \brief (USART0) Transfer Control Register */
  #define REG_USART0_PTSR                  (0x40024124U) /**< \brief (USART0) Transfer Status Register */
#else
  #define REG_USART0_CR   (*(__O  uint32_t*)0x40024000U) /**< \brief (USART0) Control Register */
  #define REG_USART0_MR   (*(__IO uint32_t*)0x40024004U) /**< \brief (USART0) Mode Register */
  #define REG_USART0_IER  (*(__O  uint32_t*)0x40024008U) /**< \brief (USART0) Interrupt Enable Register */
  #define REG_USART0_IDR  (*(__O  uint32_t*)0x4002400CU) /**< \brief (USART0) Interrupt Disable Register */
  #define REG_USART0_IMR  (*(__I  uint32_t*)0x40024010U) /**< \brief (USART0) Interrupt Mask Register */
  #define REG_USART0_CSR  (*(__I  uint32_t*)0x40024014U) /**< \brief (USART0) Channel Status Register */
  #define REG_USART0_RHR  (*(__I  uint32_t*)0x40024018U) /**< \brief (USART0) Receive Holding Register */
  #define REG_USART0_THR  (*(__O  uint32_t*)0x4002401CU) /**< \brief (USART0) Transmit Holding Register */
  #define REG_USART0_BRGR (*(__IO uint32_t*)0x40024020U) /**< \brief (USART0) Baud Rate Generator Register */
  #define REG_USART0_RTOR (*(__IO uint32_t*)0x40024024U) /**< \brief (USART0) Receiver Time-out Register */
  #define REG_USART0_TTGR (*(__IO uint32_t*)0x40024028U) /**< \brief (USART0) Transmitter Timeguard Register */
  #define REG_USART0_FIDI (*(__IO uint32_t*)0x40024040U) /**< \brief (USART0) FI DI Ratio Register */
  #define REG_USART0_NER  (*(__I  uint32_t*)0x40024044U) /**< \brief (USART0) Number of Errors Register */
  #define REG_USART0_IF   (*(__IO uint32_t*)0x4002404CU) /**< \brief (USART0) IrDA Filter Register */
  #define REG_USART0_MAN  (*(__IO uint32_t*)0x40024050U) /**< \brief (USART0) Manchester Configuration Register */
  #define REG_USART0_WPMR (*(__IO uint32_t*)0x400240E4U) /**< \brief (USART0) Write Protection Mode Register */
  #define REG_USART0_WPSR (*(__I  uint32_t*)0x400240E8U) /**< \brief (USART0) Write Protection Status Register */
  #define REG_USART0_RPR  (*(__IO uint32_t*)0x40024100U) /**< \brief (USART0) Receive Pointer Register */
  #define REG_USART0_RCR  (*(__IO uint32_t*)0x40024104U) /**< \brief (USART0) Receive Counter Register */
  #define REG_USART0_TPR  (*(__IO uint32_t*)0x40024108U) /**< \brief (USART0) Transmit Pointer Register */
  #define REG_USART0_TCR  (*(__IO uint32_t*)0x4002410CU) /**< \brief (USART0) Transmit Counter Register */
  #define REG_USART0_RNPR (*(__IO uint32_t*)0x40024110U) /**< \brief (USART0) Receive Next Pointer Register */
  #define REG_USART0_RNCR (*(__IO uint32_t*)0x40024114U) /**< \brief (USART0) Receive Next Counter Register */
  #define REG_USART0_TNPR (*(__IO uint32_t*)0x40024118U) /**< \brief (USART0) Transmit Next Pointer Register */
  #define REG_USART0_TNCR (*(__IO uint32_t*)0x4002411CU) /**< \brief (USART0) Transmit Next Counter Register */
  #define REG_USART0_PTCR (*(__O  uint32_t*)0x40024120U) /**< \brief (USART0) Transfer Control Register */
  #define REG_USART0_PTSR (*(__I  uint32_t*)0x40024124U) /**< \brief (USART0) Transfer Status Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM4S_USART0_INSTANCE_ */
