/**
 * \file
 *
 * \brief Instance description for I2SC0
 *
 * Copyright (c) 2018 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
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

/* file generated from device description version 2017-09-13T14:00:00Z */
#ifndef _SAME70_I2SC0_INSTANCE_H_
#define _SAME70_I2SC0_INSTANCE_H_

/* ========== Register definition for I2SC0 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_I2SC0_CR            (0x4008C000) /**< (I2SC0) Control Register */
#define REG_I2SC0_MR            (0x4008C004) /**< (I2SC0) Mode Register */
#define REG_I2SC0_SR            (0x4008C008) /**< (I2SC0) Status Register */
#define REG_I2SC0_SCR           (0x4008C00C) /**< (I2SC0) Status Clear Register */
#define REG_I2SC0_SSR           (0x4008C010) /**< (I2SC0) Status Set Register */
#define REG_I2SC0_IER           (0x4008C014) /**< (I2SC0) Interrupt Enable Register */
#define REG_I2SC0_IDR           (0x4008C018) /**< (I2SC0) Interrupt Disable Register */
#define REG_I2SC0_IMR           (0x4008C01C) /**< (I2SC0) Interrupt Mask Register */
#define REG_I2SC0_RHR           (0x4008C020) /**< (I2SC0) Receiver Holding Register */
#define REG_I2SC0_THR           (0x4008C024) /**< (I2SC0) Transmitter Holding Register */

#else

#define REG_I2SC0_CR            (*(__O  uint32_t*)0x4008C000U) /**< (I2SC0) Control Register */
#define REG_I2SC0_MR            (*(__IO uint32_t*)0x4008C004U) /**< (I2SC0) Mode Register */
#define REG_I2SC0_SR            (*(__I  uint32_t*)0x4008C008U) /**< (I2SC0) Status Register */
#define REG_I2SC0_SCR           (*(__O  uint32_t*)0x4008C00CU) /**< (I2SC0) Status Clear Register */
#define REG_I2SC0_SSR           (*(__O  uint32_t*)0x4008C010U) /**< (I2SC0) Status Set Register */
#define REG_I2SC0_IER           (*(__O  uint32_t*)0x4008C014U) /**< (I2SC0) Interrupt Enable Register */
#define REG_I2SC0_IDR           (*(__O  uint32_t*)0x4008C018U) /**< (I2SC0) Interrupt Disable Register */
#define REG_I2SC0_IMR           (*(__I  uint32_t*)0x4008C01CU) /**< (I2SC0) Interrupt Mask Register */
#define REG_I2SC0_RHR           (*(__I  uint32_t*)0x4008C020U) /**< (I2SC0) Receiver Holding Register */
#define REG_I2SC0_THR           (*(__O  uint32_t*)0x4008C024U) /**< (I2SC0) Transmitter Holding Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

/* ========== Instance Parameter definitions for I2SC0 peripheral ========== */
#define I2SC0_INSTANCE_ID                        69        
#define I2SC0_CLOCK_ID                           69        

#endif /* _SAME70_I2SC0_INSTANCE_ */
