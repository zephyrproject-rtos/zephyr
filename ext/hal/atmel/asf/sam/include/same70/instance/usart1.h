/**
 * \file
 *
 * \brief Instance description for USART1
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

#ifndef _SAME70_USART1_INSTANCE_H_
#define _SAME70_USART1_INSTANCE_H_

/* ========== Register definition for USART1 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_USART1_US_CR        (0x40028000) /**< (USART1) Control Register */
#define REG_USART1_US_MR        (0x40028004) /**< (USART1) Mode Register */
#define REG_USART1_US_IER       (0x40028008) /**< (USART1) Interrupt Enable Register */
#define REG_USART1_US_IDR       (0x4002800C) /**< (USART1) Interrupt Disable Register */
#define REG_USART1_US_IMR       (0x40028010) /**< (USART1) Interrupt Mask Register */
#define REG_USART1_US_CSR       (0x40028014) /**< (USART1) Channel Status Register */
#define REG_USART1_US_RHR       (0x40028018) /**< (USART1) Receive Holding Register */
#define REG_USART1_US_THR       (0x4002801C) /**< (USART1) Transmit Holding Register */
#define REG_USART1_US_BRGR      (0x40028020) /**< (USART1) Baud Rate Generator Register */
#define REG_USART1_US_RTOR      (0x40028024) /**< (USART1) Receiver Time-out Register */
#define REG_USART1_US_TTGR      (0x40028028) /**< (USART1) Transmitter Timeguard Register */
#define REG_USART1_US_FIDI      (0x40028040) /**< (USART1) FI DI Ratio Register */
#define REG_USART1_US_NER       (0x40028044) /**< (USART1) Number of Errors Register */
#define REG_USART1_US_IF        (0x4002804C) /**< (USART1) IrDA Filter Register */
#define REG_USART1_US_MAN       (0x40028050) /**< (USART1) Manchester Configuration Register */
#define REG_USART1_US_LINMR     (0x40028054) /**< (USART1) LIN Mode Register */
#define REG_USART1_US_LINIR     (0x40028058) /**< (USART1) LIN Identifier Register */
#define REG_USART1_US_LINBRR    (0x4002805C) /**< (USART1) LIN Baud Rate Register */
#define REG_USART1_US_LONMR     (0x40028060) /**< (USART1) LON Mode Register */
#define REG_USART1_US_LONPR     (0x40028064) /**< (USART1) LON Preamble Register */
#define REG_USART1_US_LONDL     (0x40028068) /**< (USART1) LON Data Length Register */
#define REG_USART1_US_LONL2HDR  (0x4002806C) /**< (USART1) LON L2HDR Register */
#define REG_USART1_US_LONBL     (0x40028070) /**< (USART1) LON Backlog Register */
#define REG_USART1_US_LONB1TX   (0x40028074) /**< (USART1) LON Beta1 Tx Register */
#define REG_USART1_US_LONB1RX   (0x40028078) /**< (USART1) LON Beta1 Rx Register */
#define REG_USART1_US_LONPRIO   (0x4002807C) /**< (USART1) LON Priority Register */
#define REG_USART1_US_IDTTX     (0x40028080) /**< (USART1) LON IDT Tx Register */
#define REG_USART1_US_IDTRX     (0x40028084) /**< (USART1) LON IDT Rx Register */
#define REG_USART1_US_ICDIFF    (0x40028088) /**< (USART1) IC DIFF Register */
#define REG_USART1_US_WPMR      (0x400280E4) /**< (USART1) Write Protection Mode Register */
#define REG_USART1_US_WPSR      (0x400280E8) /**< (USART1) Write Protection Status Register */

#else

#define REG_USART1_US_CR        (*(__O  uint32_t*)0x40028000U) /**< (USART1) Control Register */
#define REG_USART1_US_MR        (*(__IO uint32_t*)0x40028004U) /**< (USART1) Mode Register */
#define REG_USART1_US_IER       (*(__O  uint32_t*)0x40028008U) /**< (USART1) Interrupt Enable Register */
#define REG_USART1_US_IDR       (*(__O  uint32_t*)0x4002800CU) /**< (USART1) Interrupt Disable Register */
#define REG_USART1_US_IMR       (*(__I  uint32_t*)0x40028010U) /**< (USART1) Interrupt Mask Register */
#define REG_USART1_US_CSR       (*(__I  uint32_t*)0x40028014U) /**< (USART1) Channel Status Register */
#define REG_USART1_US_RHR       (*(__I  uint32_t*)0x40028018U) /**< (USART1) Receive Holding Register */
#define REG_USART1_US_THR       (*(__O  uint32_t*)0x4002801CU) /**< (USART1) Transmit Holding Register */
#define REG_USART1_US_BRGR      (*(__IO uint32_t*)0x40028020U) /**< (USART1) Baud Rate Generator Register */
#define REG_USART1_US_RTOR      (*(__IO uint32_t*)0x40028024U) /**< (USART1) Receiver Time-out Register */
#define REG_USART1_US_TTGR      (*(__IO uint32_t*)0x40028028U) /**< (USART1) Transmitter Timeguard Register */
#define REG_USART1_US_FIDI      (*(__IO uint32_t*)0x40028040U) /**< (USART1) FI DI Ratio Register */
#define REG_USART1_US_NER       (*(__I  uint32_t*)0x40028044U) /**< (USART1) Number of Errors Register */
#define REG_USART1_US_IF        (*(__IO uint32_t*)0x4002804CU) /**< (USART1) IrDA Filter Register */
#define REG_USART1_US_MAN       (*(__IO uint32_t*)0x40028050U) /**< (USART1) Manchester Configuration Register */
#define REG_USART1_US_LINMR     (*(__IO uint32_t*)0x40028054U) /**< (USART1) LIN Mode Register */
#define REG_USART1_US_LINIR     (*(__IO uint32_t*)0x40028058U) /**< (USART1) LIN Identifier Register */
#define REG_USART1_US_LINBRR    (*(__I  uint32_t*)0x4002805CU) /**< (USART1) LIN Baud Rate Register */
#define REG_USART1_US_LONMR     (*(__IO uint32_t*)0x40028060U) /**< (USART1) LON Mode Register */
#define REG_USART1_US_LONPR     (*(__IO uint32_t*)0x40028064U) /**< (USART1) LON Preamble Register */
#define REG_USART1_US_LONDL     (*(__IO uint32_t*)0x40028068U) /**< (USART1) LON Data Length Register */
#define REG_USART1_US_LONL2HDR  (*(__IO uint32_t*)0x4002806CU) /**< (USART1) LON L2HDR Register */
#define REG_USART1_US_LONBL     (*(__I  uint32_t*)0x40028070U) /**< (USART1) LON Backlog Register */
#define REG_USART1_US_LONB1TX   (*(__IO uint32_t*)0x40028074U) /**< (USART1) LON Beta1 Tx Register */
#define REG_USART1_US_LONB1RX   (*(__IO uint32_t*)0x40028078U) /**< (USART1) LON Beta1 Rx Register */
#define REG_USART1_US_LONPRIO   (*(__IO uint32_t*)0x4002807CU) /**< (USART1) LON Priority Register */
#define REG_USART1_US_IDTTX     (*(__IO uint32_t*)0x40028080U) /**< (USART1) LON IDT Tx Register */
#define REG_USART1_US_IDTRX     (*(__IO uint32_t*)0x40028084U) /**< (USART1) LON IDT Rx Register */
#define REG_USART1_US_ICDIFF    (*(__IO uint32_t*)0x40028088U) /**< (USART1) IC DIFF Register */
#define REG_USART1_US_WPMR      (*(__IO uint32_t*)0x400280E4U) /**< (USART1) Write Protection Mode Register */
#define REG_USART1_US_WPSR      (*(__I  uint32_t*)0x400280E8U) /**< (USART1) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for USART1 peripheral ========== */
#define USART1_INSTANCE_ID                       14        
#define USART1_DMAC_ID_TX                        9         
#define USART1_DMAC_ID_RX                        10        

#endif /* _SAME70_USART1_INSTANCE_ */
