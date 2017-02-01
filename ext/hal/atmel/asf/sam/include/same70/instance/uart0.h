/**
 * \file
 *
 * \brief Instance description for UART0
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

#ifndef _SAME70_UART0_INSTANCE_H_
#define _SAME70_UART0_INSTANCE_H_

/* ========== Register definition for UART0 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_UART0_CR            (0x400E0800) /**< (UART0) Control Register */
#define REG_UART0_MR            (0x400E0804) /**< (UART0) Mode Register */
#define REG_UART0_IER           (0x400E0808) /**< (UART0) Interrupt Enable Register */
#define REG_UART0_IDR           (0x400E080C) /**< (UART0) Interrupt Disable Register */
#define REG_UART0_IMR           (0x400E0810) /**< (UART0) Interrupt Mask Register */
#define REG_UART0_SR            (0x400E0814) /**< (UART0) Status Register */
#define REG_UART0_RHR           (0x400E0818) /**< (UART0) Receive Holding Register */
#define REG_UART0_THR           (0x400E081C) /**< (UART0) Transmit Holding Register */
#define REG_UART0_BRGR          (0x400E0820) /**< (UART0) Baud Rate Generator Register */
#define REG_UART0_CMPR          (0x400E0824) /**< (UART0) Comparison Register */
#define REG_UART0_WPMR          (0x400E08E4) /**< (UART0) Write Protection Mode Register */

#else

#define REG_UART0_CR            (*(__O  uint32_t*)0x400E0800U) /**< (UART0) Control Register */
#define REG_UART0_MR            (*(__IO uint32_t*)0x400E0804U) /**< (UART0) Mode Register */
#define REG_UART0_IER           (*(__O  uint32_t*)0x400E0808U) /**< (UART0) Interrupt Enable Register */
#define REG_UART0_IDR           (*(__O  uint32_t*)0x400E080CU) /**< (UART0) Interrupt Disable Register */
#define REG_UART0_IMR           (*(__I  uint32_t*)0x400E0810U) /**< (UART0) Interrupt Mask Register */
#define REG_UART0_SR            (*(__I  uint32_t*)0x400E0814U) /**< (UART0) Status Register */
#define REG_UART0_RHR           (*(__I  uint32_t*)0x400E0818U) /**< (UART0) Receive Holding Register */
#define REG_UART0_THR           (*(__O  uint32_t*)0x400E081CU) /**< (UART0) Transmit Holding Register */
#define REG_UART0_BRGR          (*(__IO uint32_t*)0x400E0820U) /**< (UART0) Baud Rate Generator Register */
#define REG_UART0_CMPR          (*(__IO uint32_t*)0x400E0824U) /**< (UART0) Comparison Register */
#define REG_UART0_WPMR          (*(__IO uint32_t*)0x400E08E4U) /**< (UART0) Write Protection Mode Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for UART0 peripheral ========== */
#define UART0_INSTANCE_ID                        7         
#define UART0_DMAC_ID_TX                         20        
#define UART0_DMAC_ID_RX                         21        

#endif /* _SAME70_UART0_INSTANCE_ */
