/**
 * \file
 *
 * \brief Instance description for UART1
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

#ifndef _SAME70_UART1_INSTANCE_H_
#define _SAME70_UART1_INSTANCE_H_

/* ========== Register definition for UART1 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_UART1_CR            (0x400E0A00) /**< (UART1) Control Register */
#define REG_UART1_MR            (0x400E0A04) /**< (UART1) Mode Register */
#define REG_UART1_IER           (0x400E0A08) /**< (UART1) Interrupt Enable Register */
#define REG_UART1_IDR           (0x400E0A0C) /**< (UART1) Interrupt Disable Register */
#define REG_UART1_IMR           (0x400E0A10) /**< (UART1) Interrupt Mask Register */
#define REG_UART1_SR            (0x400E0A14) /**< (UART1) Status Register */
#define REG_UART1_RHR           (0x400E0A18) /**< (UART1) Receive Holding Register */
#define REG_UART1_THR           (0x400E0A1C) /**< (UART1) Transmit Holding Register */
#define REG_UART1_BRGR          (0x400E0A20) /**< (UART1) Baud Rate Generator Register */
#define REG_UART1_CMPR          (0x400E0A24) /**< (UART1) Comparison Register */
#define REG_UART1_WPMR          (0x400E0AE4) /**< (UART1) Write Protection Mode Register */

#else

#define REG_UART1_CR            (*(__O  uint32_t*)0x400E0A00U) /**< (UART1) Control Register */
#define REG_UART1_MR            (*(__IO uint32_t*)0x400E0A04U) /**< (UART1) Mode Register */
#define REG_UART1_IER           (*(__O  uint32_t*)0x400E0A08U) /**< (UART1) Interrupt Enable Register */
#define REG_UART1_IDR           (*(__O  uint32_t*)0x400E0A0CU) /**< (UART1) Interrupt Disable Register */
#define REG_UART1_IMR           (*(__I  uint32_t*)0x400E0A10U) /**< (UART1) Interrupt Mask Register */
#define REG_UART1_SR            (*(__I  uint32_t*)0x400E0A14U) /**< (UART1) Status Register */
#define REG_UART1_RHR           (*(__I  uint32_t*)0x400E0A18U) /**< (UART1) Receive Holding Register */
#define REG_UART1_THR           (*(__O  uint32_t*)0x400E0A1CU) /**< (UART1) Transmit Holding Register */
#define REG_UART1_BRGR          (*(__IO uint32_t*)0x400E0A20U) /**< (UART1) Baud Rate Generator Register */
#define REG_UART1_CMPR          (*(__IO uint32_t*)0x400E0A24U) /**< (UART1) Comparison Register */
#define REG_UART1_WPMR          (*(__IO uint32_t*)0x400E0AE4U) /**< (UART1) Write Protection Mode Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for UART1 peripheral ========== */
#define UART1_INSTANCE_ID                        8         
#define UART1_DMAC_ID_TX                         22        
#define UART1_DMAC_ID_RX                         23        

#endif /* _SAME70_UART1_INSTANCE_ */
