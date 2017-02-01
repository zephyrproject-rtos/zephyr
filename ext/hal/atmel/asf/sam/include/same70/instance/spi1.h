/**
 * \file
 *
 * \brief Instance description for SPI1
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

#ifndef _SAME70_SPI1_INSTANCE_H_
#define _SAME70_SPI1_INSTANCE_H_

/* ========== Register definition for SPI1 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_SPI1_CR             (0x40058000) /**< (SPI1) Control Register */
#define REG_SPI1_MR             (0x40058004) /**< (SPI1) Mode Register */
#define REG_SPI1_RDR            (0x40058008) /**< (SPI1) Receive Data Register */
#define REG_SPI1_TDR            (0x4005800C) /**< (SPI1) Transmit Data Register */
#define REG_SPI1_SR             (0x40058010) /**< (SPI1) Status Register */
#define REG_SPI1_IER            (0x40058014) /**< (SPI1) Interrupt Enable Register */
#define REG_SPI1_IDR            (0x40058018) /**< (SPI1) Interrupt Disable Register */
#define REG_SPI1_IMR            (0x4005801C) /**< (SPI1) Interrupt Mask Register */
#define REG_SPI1_CSR            (0x40058030) /**< (SPI1) Chip Select Register 0 */
#define REG_SPI1_CSR0           (0x40058030) /**< (SPI1) Chip Select Register 0 */
#define REG_SPI1_CSR1           (0x40058034) /**< (SPI1) Chip Select Register 1 */
#define REG_SPI1_CSR2           (0x40058038) /**< (SPI1) Chip Select Register 2 */
#define REG_SPI1_CSR3           (0x4005803C) /**< (SPI1) Chip Select Register 3 */
#define REG_SPI1_WPMR           (0x400580E4) /**< (SPI1) Write Protection Mode Register */
#define REG_SPI1_WPSR           (0x400580E8) /**< (SPI1) Write Protection Status Register */

#else

#define REG_SPI1_CR             (*(__O  uint32_t*)0x40058000U) /**< (SPI1) Control Register */
#define REG_SPI1_MR             (*(__IO uint32_t*)0x40058004U) /**< (SPI1) Mode Register */
#define REG_SPI1_RDR            (*(__I  uint32_t*)0x40058008U) /**< (SPI1) Receive Data Register */
#define REG_SPI1_TDR            (*(__O  uint32_t*)0x4005800CU) /**< (SPI1) Transmit Data Register */
#define REG_SPI1_SR             (*(__I  uint32_t*)0x40058010U) /**< (SPI1) Status Register */
#define REG_SPI1_IER            (*(__O  uint32_t*)0x40058014U) /**< (SPI1) Interrupt Enable Register */
#define REG_SPI1_IDR            (*(__O  uint32_t*)0x40058018U) /**< (SPI1) Interrupt Disable Register */
#define REG_SPI1_IMR            (*(__I  uint32_t*)0x4005801CU) /**< (SPI1) Interrupt Mask Register */
#define REG_SPI1_CSR            (*(__IO uint32_t*)0x40058030U) /**< (SPI1) Chip Select Register 0 */
#define REG_SPI1_CSR0           (*(__IO uint32_t*)0x40058030U) /**< (SPI1) Chip Select Register 0 */
#define REG_SPI1_CSR1           (*(__IO uint32_t*)0x40058034U) /**< (SPI1) Chip Select Register 1 */
#define REG_SPI1_CSR2           (*(__IO uint32_t*)0x40058038U) /**< (SPI1) Chip Select Register 2 */
#define REG_SPI1_CSR3           (*(__IO uint32_t*)0x4005803CU) /**< (SPI1) Chip Select Register 3 */
#define REG_SPI1_WPMR           (*(__IO uint32_t*)0x400580E4U) /**< (SPI1) Write Protection Mode Register */
#define REG_SPI1_WPSR           (*(__I  uint32_t*)0x400580E8U) /**< (SPI1) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for SPI1 peripheral ========== */
#define SPI1_INSTANCE_ID                         42        
#define SPI1_DMAC_ID_TX                          3         
#define SPI1_DMAC_ID_RX                          4         

#endif /* _SAME70_SPI1_INSTANCE_ */
