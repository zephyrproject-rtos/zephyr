/**
 * \file
 *
 * \brief Instance description for TWIHS0
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

#ifndef _SAME70_TWIHS0_INSTANCE_H_
#define _SAME70_TWIHS0_INSTANCE_H_

/* ========== Register definition for TWIHS0 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_TWIHS0_CR           (0x40018000) /**< (TWIHS0) Control Register */
#define REG_TWIHS0_MMR          (0x40018004) /**< (TWIHS0) Master Mode Register */
#define REG_TWIHS0_SMR          (0x40018008) /**< (TWIHS0) Slave Mode Register */
#define REG_TWIHS0_IADR         (0x4001800C) /**< (TWIHS0) Internal Address Register */
#define REG_TWIHS0_CWGR         (0x40018010) /**< (TWIHS0) Clock Waveform Generator Register */
#define REG_TWIHS0_SR           (0x40018020) /**< (TWIHS0) Status Register */
#define REG_TWIHS0_IER          (0x40018024) /**< (TWIHS0) Interrupt Enable Register */
#define REG_TWIHS0_IDR          (0x40018028) /**< (TWIHS0) Interrupt Disable Register */
#define REG_TWIHS0_IMR          (0x4001802C) /**< (TWIHS0) Interrupt Mask Register */
#define REG_TWIHS0_RHR          (0x40018030) /**< (TWIHS0) Receive Holding Register */
#define REG_TWIHS0_THR          (0x40018034) /**< (TWIHS0) Transmit Holding Register */
#define REG_TWIHS0_SMBTR        (0x40018038) /**< (TWIHS0) SMBus Timing Register */
#define REG_TWIHS0_FILTR        (0x40018044) /**< (TWIHS0) Filter Register */
#define REG_TWIHS0_SWMR         (0x4001804C) /**< (TWIHS0) SleepWalking Matching Register */
#define REG_TWIHS0_WPMR         (0x400180E4) /**< (TWIHS0) Write Protection Mode Register */
#define REG_TWIHS0_WPSR         (0x400180E8) /**< (TWIHS0) Write Protection Status Register */

#else

#define REG_TWIHS0_CR           (*(__O  uint32_t*)0x40018000U) /**< (TWIHS0) Control Register */
#define REG_TWIHS0_MMR          (*(__IO uint32_t*)0x40018004U) /**< (TWIHS0) Master Mode Register */
#define REG_TWIHS0_SMR          (*(__IO uint32_t*)0x40018008U) /**< (TWIHS0) Slave Mode Register */
#define REG_TWIHS0_IADR         (*(__IO uint32_t*)0x4001800CU) /**< (TWIHS0) Internal Address Register */
#define REG_TWIHS0_CWGR         (*(__IO uint32_t*)0x40018010U) /**< (TWIHS0) Clock Waveform Generator Register */
#define REG_TWIHS0_SR           (*(__I  uint32_t*)0x40018020U) /**< (TWIHS0) Status Register */
#define REG_TWIHS0_IER          (*(__O  uint32_t*)0x40018024U) /**< (TWIHS0) Interrupt Enable Register */
#define REG_TWIHS0_IDR          (*(__O  uint32_t*)0x40018028U) /**< (TWIHS0) Interrupt Disable Register */
#define REG_TWIHS0_IMR          (*(__I  uint32_t*)0x4001802CU) /**< (TWIHS0) Interrupt Mask Register */
#define REG_TWIHS0_RHR          (*(__I  uint32_t*)0x40018030U) /**< (TWIHS0) Receive Holding Register */
#define REG_TWIHS0_THR          (*(__O  uint32_t*)0x40018034U) /**< (TWIHS0) Transmit Holding Register */
#define REG_TWIHS0_SMBTR        (*(__IO uint32_t*)0x40018038U) /**< (TWIHS0) SMBus Timing Register */
#define REG_TWIHS0_FILTR        (*(__IO uint32_t*)0x40018044U) /**< (TWIHS0) Filter Register */
#define REG_TWIHS0_SWMR         (*(__IO uint32_t*)0x4001804CU) /**< (TWIHS0) SleepWalking Matching Register */
#define REG_TWIHS0_WPMR         (*(__IO uint32_t*)0x400180E4U) /**< (TWIHS0) Write Protection Mode Register */
#define REG_TWIHS0_WPSR         (*(__I  uint32_t*)0x400180E8U) /**< (TWIHS0) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for TWIHS0 peripheral ========== */
#define TWIHS0_INSTANCE_ID                       19        
#define TWIHS0_DMAC_ID_TX                        14        
#define TWIHS0_DMAC_ID_RX                        15        

#endif /* _SAME70_TWIHS0_INSTANCE_ */
