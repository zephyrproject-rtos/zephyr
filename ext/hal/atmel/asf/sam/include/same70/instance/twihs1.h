/**
 * \file
 *
 * \brief Instance description for TWIHS1
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

#ifndef _SAME70_TWIHS1_INSTANCE_H_
#define _SAME70_TWIHS1_INSTANCE_H_

/* ========== Register definition for TWIHS1 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_TWIHS1_CR           (0x4001C000) /**< (TWIHS1) Control Register */
#define REG_TWIHS1_MMR          (0x4001C004) /**< (TWIHS1) Master Mode Register */
#define REG_TWIHS1_SMR          (0x4001C008) /**< (TWIHS1) Slave Mode Register */
#define REG_TWIHS1_IADR         (0x4001C00C) /**< (TWIHS1) Internal Address Register */
#define REG_TWIHS1_CWGR         (0x4001C010) /**< (TWIHS1) Clock Waveform Generator Register */
#define REG_TWIHS1_SR           (0x4001C020) /**< (TWIHS1) Status Register */
#define REG_TWIHS1_IER          (0x4001C024) /**< (TWIHS1) Interrupt Enable Register */
#define REG_TWIHS1_IDR          (0x4001C028) /**< (TWIHS1) Interrupt Disable Register */
#define REG_TWIHS1_IMR          (0x4001C02C) /**< (TWIHS1) Interrupt Mask Register */
#define REG_TWIHS1_RHR          (0x4001C030) /**< (TWIHS1) Receive Holding Register */
#define REG_TWIHS1_THR          (0x4001C034) /**< (TWIHS1) Transmit Holding Register */
#define REG_TWIHS1_SMBTR        (0x4001C038) /**< (TWIHS1) SMBus Timing Register */
#define REG_TWIHS1_FILTR        (0x4001C044) /**< (TWIHS1) Filter Register */
#define REG_TWIHS1_SWMR         (0x4001C04C) /**< (TWIHS1) SleepWalking Matching Register */
#define REG_TWIHS1_WPMR         (0x4001C0E4) /**< (TWIHS1) Write Protection Mode Register */
#define REG_TWIHS1_WPSR         (0x4001C0E8) /**< (TWIHS1) Write Protection Status Register */

#else

#define REG_TWIHS1_CR           (*(__O  uint32_t*)0x4001C000U) /**< (TWIHS1) Control Register */
#define REG_TWIHS1_MMR          (*(__IO uint32_t*)0x4001C004U) /**< (TWIHS1) Master Mode Register */
#define REG_TWIHS1_SMR          (*(__IO uint32_t*)0x4001C008U) /**< (TWIHS1) Slave Mode Register */
#define REG_TWIHS1_IADR         (*(__IO uint32_t*)0x4001C00CU) /**< (TWIHS1) Internal Address Register */
#define REG_TWIHS1_CWGR         (*(__IO uint32_t*)0x4001C010U) /**< (TWIHS1) Clock Waveform Generator Register */
#define REG_TWIHS1_SR           (*(__I  uint32_t*)0x4001C020U) /**< (TWIHS1) Status Register */
#define REG_TWIHS1_IER          (*(__O  uint32_t*)0x4001C024U) /**< (TWIHS1) Interrupt Enable Register */
#define REG_TWIHS1_IDR          (*(__O  uint32_t*)0x4001C028U) /**< (TWIHS1) Interrupt Disable Register */
#define REG_TWIHS1_IMR          (*(__I  uint32_t*)0x4001C02CU) /**< (TWIHS1) Interrupt Mask Register */
#define REG_TWIHS1_RHR          (*(__I  uint32_t*)0x4001C030U) /**< (TWIHS1) Receive Holding Register */
#define REG_TWIHS1_THR          (*(__O  uint32_t*)0x4001C034U) /**< (TWIHS1) Transmit Holding Register */
#define REG_TWIHS1_SMBTR        (*(__IO uint32_t*)0x4001C038U) /**< (TWIHS1) SMBus Timing Register */
#define REG_TWIHS1_FILTR        (*(__IO uint32_t*)0x4001C044U) /**< (TWIHS1) Filter Register */
#define REG_TWIHS1_SWMR         (*(__IO uint32_t*)0x4001C04CU) /**< (TWIHS1) SleepWalking Matching Register */
#define REG_TWIHS1_WPMR         (*(__IO uint32_t*)0x4001C0E4U) /**< (TWIHS1) Write Protection Mode Register */
#define REG_TWIHS1_WPSR         (*(__I  uint32_t*)0x4001C0E8U) /**< (TWIHS1) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for TWIHS1 peripheral ========== */
#define TWIHS1_INSTANCE_ID                       20        
#define TWIHS1_DMAC_ID_TX                        16        
#define TWIHS1_DMAC_ID_RX                        17        

#endif /* _SAME70_TWIHS1_INSTANCE_ */
