/**
 * \file
 *
 * \brief Instance description for UART2
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

#ifndef _SAME70_UART2_INSTANCE_H_
#define _SAME70_UART2_INSTANCE_H_

/* ========== Register definition for UART2 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_UART2_CR            (0x400E1A00) /**< (UART2) Control Register */
#define REG_UART2_MR            (0x400E1A04) /**< (UART2) Mode Register */
#define REG_UART2_IER           (0x400E1A08) /**< (UART2) Interrupt Enable Register */
#define REG_UART2_IDR           (0x400E1A0C) /**< (UART2) Interrupt Disable Register */
#define REG_UART2_IMR           (0x400E1A10) /**< (UART2) Interrupt Mask Register */
#define REG_UART2_SR            (0x400E1A14) /**< (UART2) Status Register */
#define REG_UART2_RHR           (0x400E1A18) /**< (UART2) Receive Holding Register */
#define REG_UART2_THR           (0x400E1A1C) /**< (UART2) Transmit Holding Register */
#define REG_UART2_BRGR          (0x400E1A20) /**< (UART2) Baud Rate Generator Register */
#define REG_UART2_CMPR          (0x400E1A24) /**< (UART2) Comparison Register */
#define REG_UART2_WPMR          (0x400E1AE4) /**< (UART2) Write Protection Mode Register */

#else

#define REG_UART2_CR            (*(__O  uint32_t*)0x400E1A00U) /**< (UART2) Control Register */
#define REG_UART2_MR            (*(__IO uint32_t*)0x400E1A04U) /**< (UART2) Mode Register */
#define REG_UART2_IER           (*(__O  uint32_t*)0x400E1A08U) /**< (UART2) Interrupt Enable Register */
#define REG_UART2_IDR           (*(__O  uint32_t*)0x400E1A0CU) /**< (UART2) Interrupt Disable Register */
#define REG_UART2_IMR           (*(__I  uint32_t*)0x400E1A10U) /**< (UART2) Interrupt Mask Register */
#define REG_UART2_SR            (*(__I  uint32_t*)0x400E1A14U) /**< (UART2) Status Register */
#define REG_UART2_RHR           (*(__I  uint32_t*)0x400E1A18U) /**< (UART2) Receive Holding Register */
#define REG_UART2_THR           (*(__O  uint32_t*)0x400E1A1CU) /**< (UART2) Transmit Holding Register */
#define REG_UART2_BRGR          (*(__IO uint32_t*)0x400E1A20U) /**< (UART2) Baud Rate Generator Register */
#define REG_UART2_CMPR          (*(__IO uint32_t*)0x400E1A24U) /**< (UART2) Comparison Register */
#define REG_UART2_WPMR          (*(__IO uint32_t*)0x400E1AE4U) /**< (UART2) Write Protection Mode Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for UART2 peripheral ========== */
#define UART2_INSTANCE_ID                        44        
#define UART2_DMAC_ID_TX                         24        
#define UART2_DMAC_ID_RX                         25        

#endif /* _SAME70_UART2_INSTANCE_ */
