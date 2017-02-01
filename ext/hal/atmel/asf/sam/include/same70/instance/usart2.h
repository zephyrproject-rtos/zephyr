/**
 * \file
 *
 * \brief Instance description for USART2
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

#ifndef _SAME70_USART2_INSTANCE_H_
#define _SAME70_USART2_INSTANCE_H_

/* ========== Register definition for USART2 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_USART2_US_CR        (0x4002C000) /**< (USART2) Control Register */
#define REG_USART2_US_MR        (0x4002C004) /**< (USART2) Mode Register */
#define REG_USART2_US_IER       (0x4002C008) /**< (USART2) Interrupt Enable Register */
#define REG_USART2_US_IDR       (0x4002C00C) /**< (USART2) Interrupt Disable Register */
#define REG_USART2_US_IMR       (0x4002C010) /**< (USART2) Interrupt Mask Register */
#define REG_USART2_US_CSR       (0x4002C014) /**< (USART2) Channel Status Register */
#define REG_USART2_US_RHR       (0x4002C018) /**< (USART2) Receive Holding Register */
#define REG_USART2_US_THR       (0x4002C01C) /**< (USART2) Transmit Holding Register */
#define REG_USART2_US_BRGR      (0x4002C020) /**< (USART2) Baud Rate Generator Register */
#define REG_USART2_US_RTOR      (0x4002C024) /**< (USART2) Receiver Time-out Register */
#define REG_USART2_US_TTGR      (0x4002C028) /**< (USART2) Transmitter Timeguard Register */
#define REG_USART2_US_FIDI      (0x4002C040) /**< (USART2) FI DI Ratio Register */
#define REG_USART2_US_NER       (0x4002C044) /**< (USART2) Number of Errors Register */
#define REG_USART2_US_IF        (0x4002C04C) /**< (USART2) IrDA Filter Register */
#define REG_USART2_US_MAN       (0x4002C050) /**< (USART2) Manchester Configuration Register */
#define REG_USART2_US_LINMR     (0x4002C054) /**< (USART2) LIN Mode Register */
#define REG_USART2_US_LINIR     (0x4002C058) /**< (USART2) LIN Identifier Register */
#define REG_USART2_US_LINBRR    (0x4002C05C) /**< (USART2) LIN Baud Rate Register */
#define REG_USART2_US_LONMR     (0x4002C060) /**< (USART2) LON Mode Register */
#define REG_USART2_US_LONPR     (0x4002C064) /**< (USART2) LON Preamble Register */
#define REG_USART2_US_LONDL     (0x4002C068) /**< (USART2) LON Data Length Register */
#define REG_USART2_US_LONL2HDR  (0x4002C06C) /**< (USART2) LON L2HDR Register */
#define REG_USART2_US_LONBL     (0x4002C070) /**< (USART2) LON Backlog Register */
#define REG_USART2_US_LONB1TX   (0x4002C074) /**< (USART2) LON Beta1 Tx Register */
#define REG_USART2_US_LONB1RX   (0x4002C078) /**< (USART2) LON Beta1 Rx Register */
#define REG_USART2_US_LONPRIO   (0x4002C07C) /**< (USART2) LON Priority Register */
#define REG_USART2_US_IDTTX     (0x4002C080) /**< (USART2) LON IDT Tx Register */
#define REG_USART2_US_IDTRX     (0x4002C084) /**< (USART2) LON IDT Rx Register */
#define REG_USART2_US_ICDIFF    (0x4002C088) /**< (USART2) IC DIFF Register */
#define REG_USART2_US_WPMR      (0x4002C0E4) /**< (USART2) Write Protection Mode Register */
#define REG_USART2_US_WPSR      (0x4002C0E8) /**< (USART2) Write Protection Status Register */

#else

#define REG_USART2_US_CR        (*(__O  uint32_t*)0x4002C000U) /**< (USART2) Control Register */
#define REG_USART2_US_MR        (*(__IO uint32_t*)0x4002C004U) /**< (USART2) Mode Register */
#define REG_USART2_US_IER       (*(__O  uint32_t*)0x4002C008U) /**< (USART2) Interrupt Enable Register */
#define REG_USART2_US_IDR       (*(__O  uint32_t*)0x4002C00CU) /**< (USART2) Interrupt Disable Register */
#define REG_USART2_US_IMR       (*(__I  uint32_t*)0x4002C010U) /**< (USART2) Interrupt Mask Register */
#define REG_USART2_US_CSR       (*(__I  uint32_t*)0x4002C014U) /**< (USART2) Channel Status Register */
#define REG_USART2_US_RHR       (*(__I  uint32_t*)0x4002C018U) /**< (USART2) Receive Holding Register */
#define REG_USART2_US_THR       (*(__O  uint32_t*)0x4002C01CU) /**< (USART2) Transmit Holding Register */
#define REG_USART2_US_BRGR      (*(__IO uint32_t*)0x4002C020U) /**< (USART2) Baud Rate Generator Register */
#define REG_USART2_US_RTOR      (*(__IO uint32_t*)0x4002C024U) /**< (USART2) Receiver Time-out Register */
#define REG_USART2_US_TTGR      (*(__IO uint32_t*)0x4002C028U) /**< (USART2) Transmitter Timeguard Register */
#define REG_USART2_US_FIDI      (*(__IO uint32_t*)0x4002C040U) /**< (USART2) FI DI Ratio Register */
#define REG_USART2_US_NER       (*(__I  uint32_t*)0x4002C044U) /**< (USART2) Number of Errors Register */
#define REG_USART2_US_IF        (*(__IO uint32_t*)0x4002C04CU) /**< (USART2) IrDA Filter Register */
#define REG_USART2_US_MAN       (*(__IO uint32_t*)0x4002C050U) /**< (USART2) Manchester Configuration Register */
#define REG_USART2_US_LINMR     (*(__IO uint32_t*)0x4002C054U) /**< (USART2) LIN Mode Register */
#define REG_USART2_US_LINIR     (*(__IO uint32_t*)0x4002C058U) /**< (USART2) LIN Identifier Register */
#define REG_USART2_US_LINBRR    (*(__I  uint32_t*)0x4002C05CU) /**< (USART2) LIN Baud Rate Register */
#define REG_USART2_US_LONMR     (*(__IO uint32_t*)0x4002C060U) /**< (USART2) LON Mode Register */
#define REG_USART2_US_LONPR     (*(__IO uint32_t*)0x4002C064U) /**< (USART2) LON Preamble Register */
#define REG_USART2_US_LONDL     (*(__IO uint32_t*)0x4002C068U) /**< (USART2) LON Data Length Register */
#define REG_USART2_US_LONL2HDR  (*(__IO uint32_t*)0x4002C06CU) /**< (USART2) LON L2HDR Register */
#define REG_USART2_US_LONBL     (*(__I  uint32_t*)0x4002C070U) /**< (USART2) LON Backlog Register */
#define REG_USART2_US_LONB1TX   (*(__IO uint32_t*)0x4002C074U) /**< (USART2) LON Beta1 Tx Register */
#define REG_USART2_US_LONB1RX   (*(__IO uint32_t*)0x4002C078U) /**< (USART2) LON Beta1 Rx Register */
#define REG_USART2_US_LONPRIO   (*(__IO uint32_t*)0x4002C07CU) /**< (USART2) LON Priority Register */
#define REG_USART2_US_IDTTX     (*(__IO uint32_t*)0x4002C080U) /**< (USART2) LON IDT Tx Register */
#define REG_USART2_US_IDTRX     (*(__IO uint32_t*)0x4002C084U) /**< (USART2) LON IDT Rx Register */
#define REG_USART2_US_ICDIFF    (*(__IO uint32_t*)0x4002C088U) /**< (USART2) IC DIFF Register */
#define REG_USART2_US_WPMR      (*(__IO uint32_t*)0x4002C0E4U) /**< (USART2) Write Protection Mode Register */
#define REG_USART2_US_WPSR      (*(__I  uint32_t*)0x4002C0E8U) /**< (USART2) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for USART2 peripheral ========== */
#define USART2_INSTANCE_ID                       15        
#define USART2_DMAC_ID_TX                        11        
#define USART2_DMAC_ID_RX                        12        

#endif /* _SAME70_USART2_INSTANCE_ */
