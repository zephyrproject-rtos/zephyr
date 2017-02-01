/**
 * \file
 *
 * \brief Instance description for USART0
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

#ifndef _SAME70_USART0_INSTANCE_H_
#define _SAME70_USART0_INSTANCE_H_

/* ========== Register definition for USART0 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_USART0_US_CR        (0x40024000) /**< (USART0) Control Register */
#define REG_USART0_US_MR        (0x40024004) /**< (USART0) Mode Register */
#define REG_USART0_US_IER       (0x40024008) /**< (USART0) Interrupt Enable Register */
#define REG_USART0_US_IDR       (0x4002400C) /**< (USART0) Interrupt Disable Register */
#define REG_USART0_US_IMR       (0x40024010) /**< (USART0) Interrupt Mask Register */
#define REG_USART0_US_CSR       (0x40024014) /**< (USART0) Channel Status Register */
#define REG_USART0_US_RHR       (0x40024018) /**< (USART0) Receive Holding Register */
#define REG_USART0_US_THR       (0x4002401C) /**< (USART0) Transmit Holding Register */
#define REG_USART0_US_BRGR      (0x40024020) /**< (USART0) Baud Rate Generator Register */
#define REG_USART0_US_RTOR      (0x40024024) /**< (USART0) Receiver Time-out Register */
#define REG_USART0_US_TTGR      (0x40024028) /**< (USART0) Transmitter Timeguard Register */
#define REG_USART0_US_FIDI      (0x40024040) /**< (USART0) FI DI Ratio Register */
#define REG_USART0_US_NER       (0x40024044) /**< (USART0) Number of Errors Register */
#define REG_USART0_US_IF        (0x4002404C) /**< (USART0) IrDA Filter Register */
#define REG_USART0_US_MAN       (0x40024050) /**< (USART0) Manchester Configuration Register */
#define REG_USART0_US_LINMR     (0x40024054) /**< (USART0) LIN Mode Register */
#define REG_USART0_US_LINIR     (0x40024058) /**< (USART0) LIN Identifier Register */
#define REG_USART0_US_LINBRR    (0x4002405C) /**< (USART0) LIN Baud Rate Register */
#define REG_USART0_US_LONMR     (0x40024060) /**< (USART0) LON Mode Register */
#define REG_USART0_US_LONPR     (0x40024064) /**< (USART0) LON Preamble Register */
#define REG_USART0_US_LONDL     (0x40024068) /**< (USART0) LON Data Length Register */
#define REG_USART0_US_LONL2HDR  (0x4002406C) /**< (USART0) LON L2HDR Register */
#define REG_USART0_US_LONBL     (0x40024070) /**< (USART0) LON Backlog Register */
#define REG_USART0_US_LONB1TX   (0x40024074) /**< (USART0) LON Beta1 Tx Register */
#define REG_USART0_US_LONB1RX   (0x40024078) /**< (USART0) LON Beta1 Rx Register */
#define REG_USART0_US_LONPRIO   (0x4002407C) /**< (USART0) LON Priority Register */
#define REG_USART0_US_IDTTX     (0x40024080) /**< (USART0) LON IDT Tx Register */
#define REG_USART0_US_IDTRX     (0x40024084) /**< (USART0) LON IDT Rx Register */
#define REG_USART0_US_ICDIFF    (0x40024088) /**< (USART0) IC DIFF Register */
#define REG_USART0_US_WPMR      (0x400240E4) /**< (USART0) Write Protection Mode Register */
#define REG_USART0_US_WPSR      (0x400240E8) /**< (USART0) Write Protection Status Register */

#else

#define REG_USART0_US_CR        (*(__O  uint32_t*)0x40024000U) /**< (USART0) Control Register */
#define REG_USART0_US_MR        (*(__IO uint32_t*)0x40024004U) /**< (USART0) Mode Register */
#define REG_USART0_US_IER       (*(__O  uint32_t*)0x40024008U) /**< (USART0) Interrupt Enable Register */
#define REG_USART0_US_IDR       (*(__O  uint32_t*)0x4002400CU) /**< (USART0) Interrupt Disable Register */
#define REG_USART0_US_IMR       (*(__I  uint32_t*)0x40024010U) /**< (USART0) Interrupt Mask Register */
#define REG_USART0_US_CSR       (*(__I  uint32_t*)0x40024014U) /**< (USART0) Channel Status Register */
#define REG_USART0_US_RHR       (*(__I  uint32_t*)0x40024018U) /**< (USART0) Receive Holding Register */
#define REG_USART0_US_THR       (*(__O  uint32_t*)0x4002401CU) /**< (USART0) Transmit Holding Register */
#define REG_USART0_US_BRGR      (*(__IO uint32_t*)0x40024020U) /**< (USART0) Baud Rate Generator Register */
#define REG_USART0_US_RTOR      (*(__IO uint32_t*)0x40024024U) /**< (USART0) Receiver Time-out Register */
#define REG_USART0_US_TTGR      (*(__IO uint32_t*)0x40024028U) /**< (USART0) Transmitter Timeguard Register */
#define REG_USART0_US_FIDI      (*(__IO uint32_t*)0x40024040U) /**< (USART0) FI DI Ratio Register */
#define REG_USART0_US_NER       (*(__I  uint32_t*)0x40024044U) /**< (USART0) Number of Errors Register */
#define REG_USART0_US_IF        (*(__IO uint32_t*)0x4002404CU) /**< (USART0) IrDA Filter Register */
#define REG_USART0_US_MAN       (*(__IO uint32_t*)0x40024050U) /**< (USART0) Manchester Configuration Register */
#define REG_USART0_US_LINMR     (*(__IO uint32_t*)0x40024054U) /**< (USART0) LIN Mode Register */
#define REG_USART0_US_LINIR     (*(__IO uint32_t*)0x40024058U) /**< (USART0) LIN Identifier Register */
#define REG_USART0_US_LINBRR    (*(__I  uint32_t*)0x4002405CU) /**< (USART0) LIN Baud Rate Register */
#define REG_USART0_US_LONMR     (*(__IO uint32_t*)0x40024060U) /**< (USART0) LON Mode Register */
#define REG_USART0_US_LONPR     (*(__IO uint32_t*)0x40024064U) /**< (USART0) LON Preamble Register */
#define REG_USART0_US_LONDL     (*(__IO uint32_t*)0x40024068U) /**< (USART0) LON Data Length Register */
#define REG_USART0_US_LONL2HDR  (*(__IO uint32_t*)0x4002406CU) /**< (USART0) LON L2HDR Register */
#define REG_USART0_US_LONBL     (*(__I  uint32_t*)0x40024070U) /**< (USART0) LON Backlog Register */
#define REG_USART0_US_LONB1TX   (*(__IO uint32_t*)0x40024074U) /**< (USART0) LON Beta1 Tx Register */
#define REG_USART0_US_LONB1RX   (*(__IO uint32_t*)0x40024078U) /**< (USART0) LON Beta1 Rx Register */
#define REG_USART0_US_LONPRIO   (*(__IO uint32_t*)0x4002407CU) /**< (USART0) LON Priority Register */
#define REG_USART0_US_IDTTX     (*(__IO uint32_t*)0x40024080U) /**< (USART0) LON IDT Tx Register */
#define REG_USART0_US_IDTRX     (*(__IO uint32_t*)0x40024084U) /**< (USART0) LON IDT Rx Register */
#define REG_USART0_US_ICDIFF    (*(__IO uint32_t*)0x40024088U) /**< (USART0) IC DIFF Register */
#define REG_USART0_US_WPMR      (*(__IO uint32_t*)0x400240E4U) /**< (USART0) Write Protection Mode Register */
#define REG_USART0_US_WPSR      (*(__I  uint32_t*)0x400240E8U) /**< (USART0) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for USART0 peripheral ========== */
#define USART0_INSTANCE_ID                       13        
#define USART0_DMAC_ID_TX                        7         
#define USART0_DMAC_ID_RX                        8         

#endif /* _SAME70_USART0_INSTANCE_ */
