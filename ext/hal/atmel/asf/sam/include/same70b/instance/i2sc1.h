/**
 * \file
 *
 * \brief Instance description for I2SC1
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
#ifndef _SAME70_I2SC1_INSTANCE_H_
#define _SAME70_I2SC1_INSTANCE_H_

/* ========== Register definition for I2SC1 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_I2SC1_CR            (0x40090000) /**< (I2SC1) Control Register */
#define REG_I2SC1_MR            (0x40090004) /**< (I2SC1) Mode Register */
#define REG_I2SC1_SR            (0x40090008) /**< (I2SC1) Status Register */
#define REG_I2SC1_SCR           (0x4009000C) /**< (I2SC1) Status Clear Register */
#define REG_I2SC1_SSR           (0x40090010) /**< (I2SC1) Status Set Register */
#define REG_I2SC1_IER           (0x40090014) /**< (I2SC1) Interrupt Enable Register */
#define REG_I2SC1_IDR           (0x40090018) /**< (I2SC1) Interrupt Disable Register */
#define REG_I2SC1_IMR           (0x4009001C) /**< (I2SC1) Interrupt Mask Register */
#define REG_I2SC1_RHR           (0x40090020) /**< (I2SC1) Receiver Holding Register */
#define REG_I2SC1_THR           (0x40090024) /**< (I2SC1) Transmitter Holding Register */

#else

#define REG_I2SC1_CR            (*(__O  uint32_t*)0x40090000U) /**< (I2SC1) Control Register */
#define REG_I2SC1_MR            (*(__IO uint32_t*)0x40090004U) /**< (I2SC1) Mode Register */
#define REG_I2SC1_SR            (*(__I  uint32_t*)0x40090008U) /**< (I2SC1) Status Register */
#define REG_I2SC1_SCR           (*(__O  uint32_t*)0x4009000CU) /**< (I2SC1) Status Clear Register */
#define REG_I2SC1_SSR           (*(__O  uint32_t*)0x40090010U) /**< (I2SC1) Status Set Register */
#define REG_I2SC1_IER           (*(__O  uint32_t*)0x40090014U) /**< (I2SC1) Interrupt Enable Register */
#define REG_I2SC1_IDR           (*(__O  uint32_t*)0x40090018U) /**< (I2SC1) Interrupt Disable Register */
#define REG_I2SC1_IMR           (*(__I  uint32_t*)0x4009001CU) /**< (I2SC1) Interrupt Mask Register */
#define REG_I2SC1_RHR           (*(__I  uint32_t*)0x40090020U) /**< (I2SC1) Receiver Holding Register */
#define REG_I2SC1_THR           (*(__O  uint32_t*)0x40090024U) /**< (I2SC1) Transmitter Holding Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

/* ========== Instance Parameter definitions for I2SC1 peripheral ========== */
#define I2SC1_INSTANCE_ID                        70        
#define I2SC1_CLOCK_ID                           70        

#endif /* _SAME70_I2SC1_INSTANCE_ */
