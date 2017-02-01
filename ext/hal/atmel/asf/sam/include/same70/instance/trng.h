/**
 * \file
 *
 * \brief Instance description for TRNG
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

#ifndef _SAME70_TRNG_INSTANCE_H_
#define _SAME70_TRNG_INSTANCE_H_

/* ========== Register definition for TRNG peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_TRNG_CR             (0x40070000) /**< (TRNG) Control Register */
#define REG_TRNG_IER            (0x40070010) /**< (TRNG) Interrupt Enable Register */
#define REG_TRNG_IDR            (0x40070014) /**< (TRNG) Interrupt Disable Register */
#define REG_TRNG_IMR            (0x40070018) /**< (TRNG) Interrupt Mask Register */
#define REG_TRNG_ISR            (0x4007001C) /**< (TRNG) Interrupt Status Register */
#define REG_TRNG_ODATA          (0x40070050) /**< (TRNG) Output Data Register */

#else

#define REG_TRNG_CR             (*(__O  uint32_t*)0x40070000U) /**< (TRNG) Control Register */
#define REG_TRNG_IER            (*(__O  uint32_t*)0x40070010U) /**< (TRNG) Interrupt Enable Register */
#define REG_TRNG_IDR            (*(__O  uint32_t*)0x40070014U) /**< (TRNG) Interrupt Disable Register */
#define REG_TRNG_IMR            (*(__I  uint32_t*)0x40070018U) /**< (TRNG) Interrupt Mask Register */
#define REG_TRNG_ISR            (*(__I  uint32_t*)0x4007001CU) /**< (TRNG) Interrupt Status Register */
#define REG_TRNG_ODATA          (*(__I  uint32_t*)0x40070050U) /**< (TRNG) Output Data Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for TRNG peripheral ========== */
#define TRNG_INSTANCE_ID                         57        

#endif /* _SAME70_TRNG_INSTANCE_ */
