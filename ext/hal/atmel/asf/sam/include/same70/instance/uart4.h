/**
 * \file
 *
 * \brief Instance description for UART4
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

#ifndef _SAME70_UART4_INSTANCE_H_
#define _SAME70_UART4_INSTANCE_H_

/* ========== Register definition for UART4 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_UART4_CR            (0x400E1E00) /**< (UART4) Control Register */
#define REG_UART4_MR            (0x400E1E04) /**< (UART4) Mode Register */
#define REG_UART4_IER           (0x400E1E08) /**< (UART4) Interrupt Enable Register */
#define REG_UART4_IDR           (0x400E1E0C) /**< (UART4) Interrupt Disable Register */
#define REG_UART4_IMR           (0x400E1E10) /**< (UART4) Interrupt Mask Register */
#define REG_UART4_SR            (0x400E1E14) /**< (UART4) Status Register */
#define REG_UART4_RHR           (0x400E1E18) /**< (UART4) Receive Holding Register */
#define REG_UART4_THR           (0x400E1E1C) /**< (UART4) Transmit Holding Register */
#define REG_UART4_BRGR          (0x400E1E20) /**< (UART4) Baud Rate Generator Register */
#define REG_UART4_CMPR          (0x400E1E24) /**< (UART4) Comparison Register */
#define REG_UART4_WPMR          (0x400E1EE4) /**< (UART4) Write Protection Mode Register */

#else

#define REG_UART4_CR            (*(__O  uint32_t*)0x400E1E00U) /**< (UART4) Control Register */
#define REG_UART4_MR            (*(__IO uint32_t*)0x400E1E04U) /**< (UART4) Mode Register */
#define REG_UART4_IER           (*(__O  uint32_t*)0x400E1E08U) /**< (UART4) Interrupt Enable Register */
#define REG_UART4_IDR           (*(__O  uint32_t*)0x400E1E0CU) /**< (UART4) Interrupt Disable Register */
#define REG_UART4_IMR           (*(__I  uint32_t*)0x400E1E10U) /**< (UART4) Interrupt Mask Register */
#define REG_UART4_SR            (*(__I  uint32_t*)0x400E1E14U) /**< (UART4) Status Register */
#define REG_UART4_RHR           (*(__I  uint32_t*)0x400E1E18U) /**< (UART4) Receive Holding Register */
#define REG_UART4_THR           (*(__O  uint32_t*)0x400E1E1CU) /**< (UART4) Transmit Holding Register */
#define REG_UART4_BRGR          (*(__IO uint32_t*)0x400E1E20U) /**< (UART4) Baud Rate Generator Register */
#define REG_UART4_CMPR          (*(__IO uint32_t*)0x400E1E24U) /**< (UART4) Comparison Register */
#define REG_UART4_WPMR          (*(__IO uint32_t*)0x400E1EE4U) /**< (UART4) Write Protection Mode Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for UART4 peripheral ========== */
#define UART4_INSTANCE_ID                        46        
#define UART4_DMAC_ID_TX                         28        
#define UART4_DMAC_ID_RX                         29        

#endif /* _SAME70_UART4_INSTANCE_ */
