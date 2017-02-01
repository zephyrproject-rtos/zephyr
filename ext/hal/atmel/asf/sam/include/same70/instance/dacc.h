/**
 * \file
 *
 * \brief Instance description for DACC
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

#ifndef _SAME70_DACC_INSTANCE_H_
#define _SAME70_DACC_INSTANCE_H_

/* ========== Register definition for DACC peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_DACC_CR             (0x40040000) /**< (DACC) Control Register */
#define REG_DACC_MR             (0x40040004) /**< (DACC) Mode Register */
#define REG_DACC_TRIGR          (0x40040008) /**< (DACC) Trigger Register */
#define REG_DACC_CHER           (0x40040010) /**< (DACC) Channel Enable Register */
#define REG_DACC_CHDR           (0x40040014) /**< (DACC) Channel Disable Register */
#define REG_DACC_CHSR           (0x40040018) /**< (DACC) Channel Status Register */
#define REG_DACC_CDR            (0x4004001C) /**< (DACC) Conversion Data Register 0 */
#define REG_DACC_CDR0           (0x4004001C) /**< (DACC) Conversion Data Register 0 */
#define REG_DACC_CDR1           (0x40040020) /**< (DACC) Conversion Data Register 1 */
#define REG_DACC_IER            (0x40040024) /**< (DACC) Interrupt Enable Register */
#define REG_DACC_IDR            (0x40040028) /**< (DACC) Interrupt Disable Register */
#define REG_DACC_IMR            (0x4004002C) /**< (DACC) Interrupt Mask Register */
#define REG_DACC_ISR            (0x40040030) /**< (DACC) Interrupt Status Register */
#define REG_DACC_ACR            (0x40040094) /**< (DACC) Analog Current Register */
#define REG_DACC_WPMR           (0x400400E4) /**< (DACC) Write Protection Mode Register */
#define REG_DACC_WPSR           (0x400400E8) /**< (DACC) Write Protection Status Register */

#else

#define REG_DACC_CR             (*(__O  uint32_t*)0x40040000U) /**< (DACC) Control Register */
#define REG_DACC_MR             (*(__IO uint32_t*)0x40040004U) /**< (DACC) Mode Register */
#define REG_DACC_TRIGR          (*(__IO uint32_t*)0x40040008U) /**< (DACC) Trigger Register */
#define REG_DACC_CHER           (*(__O  uint32_t*)0x40040010U) /**< (DACC) Channel Enable Register */
#define REG_DACC_CHDR           (*(__O  uint32_t*)0x40040014U) /**< (DACC) Channel Disable Register */
#define REG_DACC_CHSR           (*(__I  uint32_t*)0x40040018U) /**< (DACC) Channel Status Register */
#define REG_DACC_CDR            (*(__O  uint32_t*)0x4004001CU) /**< (DACC) Conversion Data Register 0 */
#define REG_DACC_CDR0           (*(__O  uint32_t*)0x4004001CU) /**< (DACC) Conversion Data Register 0 */
#define REG_DACC_CDR1           (*(__O  uint32_t*)0x40040020U) /**< (DACC) Conversion Data Register 1 */
#define REG_DACC_IER            (*(__O  uint32_t*)0x40040024U) /**< (DACC) Interrupt Enable Register */
#define REG_DACC_IDR            (*(__O  uint32_t*)0x40040028U) /**< (DACC) Interrupt Disable Register */
#define REG_DACC_IMR            (*(__I  uint32_t*)0x4004002CU) /**< (DACC) Interrupt Mask Register */
#define REG_DACC_ISR            (*(__I  uint32_t*)0x40040030U) /**< (DACC) Interrupt Status Register */
#define REG_DACC_ACR            (*(__IO uint32_t*)0x40040094U) /**< (DACC) Analog Current Register */
#define REG_DACC_WPMR           (*(__IO uint32_t*)0x400400E4U) /**< (DACC) Write Protection Mode Register */
#define REG_DACC_WPSR           (*(__I  uint32_t*)0x400400E8U) /**< (DACC) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for DACC peripheral ========== */
#define DACC_INSTANCE_ID                         30        
#define DACC_DMAC_ID_TX                          30        

#endif /* _SAME70_DACC_INSTANCE_ */
