/**
 * \file
 *
 * \brief Instance description for SPI0
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

#ifndef _SAME70_SPI0_INSTANCE_H_
#define _SAME70_SPI0_INSTANCE_H_

/* ========== Register definition for SPI0 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_SPI0_CR             (0x40008000) /**< (SPI0) Control Register */
#define REG_SPI0_MR             (0x40008004) /**< (SPI0) Mode Register */
#define REG_SPI0_RDR            (0x40008008) /**< (SPI0) Receive Data Register */
#define REG_SPI0_TDR            (0x4000800C) /**< (SPI0) Transmit Data Register */
#define REG_SPI0_SR             (0x40008010) /**< (SPI0) Status Register */
#define REG_SPI0_IER            (0x40008014) /**< (SPI0) Interrupt Enable Register */
#define REG_SPI0_IDR            (0x40008018) /**< (SPI0) Interrupt Disable Register */
#define REG_SPI0_IMR            (0x4000801C) /**< (SPI0) Interrupt Mask Register */
#define REG_SPI0_CSR            (0x40008030) /**< (SPI0) Chip Select Register 0 */
#define REG_SPI0_CSR0           (0x40008030) /**< (SPI0) Chip Select Register 0 */
#define REG_SPI0_CSR1           (0x40008034) /**< (SPI0) Chip Select Register 1 */
#define REG_SPI0_CSR2           (0x40008038) /**< (SPI0) Chip Select Register 2 */
#define REG_SPI0_CSR3           (0x4000803C) /**< (SPI0) Chip Select Register 3 */
#define REG_SPI0_WPMR           (0x400080E4) /**< (SPI0) Write Protection Mode Register */
#define REG_SPI0_WPSR           (0x400080E8) /**< (SPI0) Write Protection Status Register */

#else

#define REG_SPI0_CR             (*(__O  uint32_t*)0x40008000U) /**< (SPI0) Control Register */
#define REG_SPI0_MR             (*(__IO uint32_t*)0x40008004U) /**< (SPI0) Mode Register */
#define REG_SPI0_RDR            (*(__I  uint32_t*)0x40008008U) /**< (SPI0) Receive Data Register */
#define REG_SPI0_TDR            (*(__O  uint32_t*)0x4000800CU) /**< (SPI0) Transmit Data Register */
#define REG_SPI0_SR             (*(__I  uint32_t*)0x40008010U) /**< (SPI0) Status Register */
#define REG_SPI0_IER            (*(__O  uint32_t*)0x40008014U) /**< (SPI0) Interrupt Enable Register */
#define REG_SPI0_IDR            (*(__O  uint32_t*)0x40008018U) /**< (SPI0) Interrupt Disable Register */
#define REG_SPI0_IMR            (*(__I  uint32_t*)0x4000801CU) /**< (SPI0) Interrupt Mask Register */
#define REG_SPI0_CSR            (*(__IO uint32_t*)0x40008030U) /**< (SPI0) Chip Select Register 0 */
#define REG_SPI0_CSR0           (*(__IO uint32_t*)0x40008030U) /**< (SPI0) Chip Select Register 0 */
#define REG_SPI0_CSR1           (*(__IO uint32_t*)0x40008034U) /**< (SPI0) Chip Select Register 1 */
#define REG_SPI0_CSR2           (*(__IO uint32_t*)0x40008038U) /**< (SPI0) Chip Select Register 2 */
#define REG_SPI0_CSR3           (*(__IO uint32_t*)0x4000803CU) /**< (SPI0) Chip Select Register 3 */
#define REG_SPI0_WPMR           (*(__IO uint32_t*)0x400080E4U) /**< (SPI0) Write Protection Mode Register */
#define REG_SPI0_WPSR           (*(__I  uint32_t*)0x400080E8U) /**< (SPI0) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for SPI0 peripheral ========== */
#define SPI0_INSTANCE_ID                         21        
#define SPI0_DMAC_ID_TX                          1         
#define SPI0_DMAC_ID_RX                          2         

#endif /* _SAME70_SPI0_INSTANCE_ */
