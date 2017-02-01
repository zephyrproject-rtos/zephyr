/**
 * \file
 *
 * \brief Instance description for UART3
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

#ifndef _SAME70_UART3_INSTANCE_H_
#define _SAME70_UART3_INSTANCE_H_

/* ========== Register definition for UART3 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_UART3_CR            (0x400E1C00) /**< (UART3) Control Register */
#define REG_UART3_MR            (0x400E1C04) /**< (UART3) Mode Register */
#define REG_UART3_IER           (0x400E1C08) /**< (UART3) Interrupt Enable Register */
#define REG_UART3_IDR           (0x400E1C0C) /**< (UART3) Interrupt Disable Register */
#define REG_UART3_IMR           (0x400E1C10) /**< (UART3) Interrupt Mask Register */
#define REG_UART3_SR            (0x400E1C14) /**< (UART3) Status Register */
#define REG_UART3_RHR           (0x400E1C18) /**< (UART3) Receive Holding Register */
#define REG_UART3_THR           (0x400E1C1C) /**< (UART3) Transmit Holding Register */
#define REG_UART3_BRGR          (0x400E1C20) /**< (UART3) Baud Rate Generator Register */
#define REG_UART3_CMPR          (0x400E1C24) /**< (UART3) Comparison Register */
#define REG_UART3_WPMR          (0x400E1CE4) /**< (UART3) Write Protection Mode Register */

#else

#define REG_UART3_CR            (*(__O  uint32_t*)0x400E1C00U) /**< (UART3) Control Register */
#define REG_UART3_MR            (*(__IO uint32_t*)0x400E1C04U) /**< (UART3) Mode Register */
#define REG_UART3_IER           (*(__O  uint32_t*)0x400E1C08U) /**< (UART3) Interrupt Enable Register */
#define REG_UART3_IDR           (*(__O  uint32_t*)0x400E1C0CU) /**< (UART3) Interrupt Disable Register */
#define REG_UART3_IMR           (*(__I  uint32_t*)0x400E1C10U) /**< (UART3) Interrupt Mask Register */
#define REG_UART3_SR            (*(__I  uint32_t*)0x400E1C14U) /**< (UART3) Status Register */
#define REG_UART3_RHR           (*(__I  uint32_t*)0x400E1C18U) /**< (UART3) Receive Holding Register */
#define REG_UART3_THR           (*(__O  uint32_t*)0x400E1C1CU) /**< (UART3) Transmit Holding Register */
#define REG_UART3_BRGR          (*(__IO uint32_t*)0x400E1C20U) /**< (UART3) Baud Rate Generator Register */
#define REG_UART3_CMPR          (*(__IO uint32_t*)0x400E1C24U) /**< (UART3) Comparison Register */
#define REG_UART3_WPMR          (*(__IO uint32_t*)0x400E1CE4U) /**< (UART3) Write Protection Mode Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for UART3 peripheral ========== */
#define UART3_INSTANCE_ID                        45        
#define UART3_DMAC_ID_TX                         26        
#define UART3_DMAC_ID_RX                         27        

#endif /* _SAME70_UART3_INSTANCE_ */
