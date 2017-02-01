/**
 * \file
 *
 * \brief Instance description for QSPI
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

#ifndef _SAME70_QSPI_INSTANCE_H_
#define _SAME70_QSPI_INSTANCE_H_

/* ========== Register definition for QSPI peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_QSPI_CR             (0x4007C000) /**< (QSPI) Control Register */
#define REG_QSPI_MR             (0x4007C004) /**< (QSPI) Mode Register */
#define REG_QSPI_RDR            (0x4007C008) /**< (QSPI) Receive Data Register */
#define REG_QSPI_TDR            (0x4007C00C) /**< (QSPI) Transmit Data Register */
#define REG_QSPI_SR             (0x4007C010) /**< (QSPI) Status Register */
#define REG_QSPI_IER            (0x4007C014) /**< (QSPI) Interrupt Enable Register */
#define REG_QSPI_IDR            (0x4007C018) /**< (QSPI) Interrupt Disable Register */
#define REG_QSPI_IMR            (0x4007C01C) /**< (QSPI) Interrupt Mask Register */
#define REG_QSPI_SCR            (0x4007C020) /**< (QSPI) Serial Clock Register */
#define REG_QSPI_IAR            (0x4007C030) /**< (QSPI) Instruction Address Register */
#define REG_QSPI_ICR            (0x4007C034) /**< (QSPI) Instruction Code Register */
#define REG_QSPI_IFR            (0x4007C038) /**< (QSPI) Instruction Frame Register */
#define REG_QSPI_SMR            (0x4007C040) /**< (QSPI) Scrambling Mode Register */
#define REG_QSPI_SKR            (0x4007C044) /**< (QSPI) Scrambling Key Register */
#define REG_QSPI_WPMR           (0x4007C0E4) /**< (QSPI) Write Protection Mode Register */
#define REG_QSPI_WPSR           (0x4007C0E8) /**< (QSPI) Write Protection Status Register */

#else

#define REG_QSPI_CR             (*(__O  uint32_t*)0x4007C000U) /**< (QSPI) Control Register */
#define REG_QSPI_MR             (*(__IO uint32_t*)0x4007C004U) /**< (QSPI) Mode Register */
#define REG_QSPI_RDR            (*(__I  uint32_t*)0x4007C008U) /**< (QSPI) Receive Data Register */
#define REG_QSPI_TDR            (*(__O  uint32_t*)0x4007C00CU) /**< (QSPI) Transmit Data Register */
#define REG_QSPI_SR             (*(__I  uint32_t*)0x4007C010U) /**< (QSPI) Status Register */
#define REG_QSPI_IER            (*(__O  uint32_t*)0x4007C014U) /**< (QSPI) Interrupt Enable Register */
#define REG_QSPI_IDR            (*(__O  uint32_t*)0x4007C018U) /**< (QSPI) Interrupt Disable Register */
#define REG_QSPI_IMR            (*(__I  uint32_t*)0x4007C01CU) /**< (QSPI) Interrupt Mask Register */
#define REG_QSPI_SCR            (*(__IO uint32_t*)0x4007C020U) /**< (QSPI) Serial Clock Register */
#define REG_QSPI_IAR            (*(__IO uint32_t*)0x4007C030U) /**< (QSPI) Instruction Address Register */
#define REG_QSPI_ICR            (*(__IO uint32_t*)0x4007C034U) /**< (QSPI) Instruction Code Register */
#define REG_QSPI_IFR            (*(__IO uint32_t*)0x4007C038U) /**< (QSPI) Instruction Frame Register */
#define REG_QSPI_SMR            (*(__IO uint32_t*)0x4007C040U) /**< (QSPI) Scrambling Mode Register */
#define REG_QSPI_SKR            (*(__O  uint32_t*)0x4007C044U) /**< (QSPI) Scrambling Key Register */
#define REG_QSPI_WPMR           (*(__IO uint32_t*)0x4007C0E4U) /**< (QSPI) Write Protection Mode Register */
#define REG_QSPI_WPSR           (*(__I  uint32_t*)0x4007C0E8U) /**< (QSPI) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for QSPI peripheral ========== */
#define QSPI_INSTANCE_ID                         43        
#define QSPI_DMAC_ID_TX                          5         
#define QSPI_DMAC_ID_RX                          6         

#endif /* _SAME70_QSPI_INSTANCE_ */
