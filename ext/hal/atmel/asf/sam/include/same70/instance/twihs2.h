/**
 * \file
 *
 * \brief Instance description for TWIHS2
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

#ifndef _SAME70_TWIHS2_INSTANCE_H_
#define _SAME70_TWIHS2_INSTANCE_H_

/* ========== Register definition for TWIHS2 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_TWIHS2_CR           (0x40060000) /**< (TWIHS2) Control Register */
#define REG_TWIHS2_MMR          (0x40060004) /**< (TWIHS2) Master Mode Register */
#define REG_TWIHS2_SMR          (0x40060008) /**< (TWIHS2) Slave Mode Register */
#define REG_TWIHS2_IADR         (0x4006000C) /**< (TWIHS2) Internal Address Register */
#define REG_TWIHS2_CWGR         (0x40060010) /**< (TWIHS2) Clock Waveform Generator Register */
#define REG_TWIHS2_SR           (0x40060020) /**< (TWIHS2) Status Register */
#define REG_TWIHS2_IER          (0x40060024) /**< (TWIHS2) Interrupt Enable Register */
#define REG_TWIHS2_IDR          (0x40060028) /**< (TWIHS2) Interrupt Disable Register */
#define REG_TWIHS2_IMR          (0x4006002C) /**< (TWIHS2) Interrupt Mask Register */
#define REG_TWIHS2_RHR          (0x40060030) /**< (TWIHS2) Receive Holding Register */
#define REG_TWIHS2_THR          (0x40060034) /**< (TWIHS2) Transmit Holding Register */
#define REG_TWIHS2_SMBTR        (0x40060038) /**< (TWIHS2) SMBus Timing Register */
#define REG_TWIHS2_FILTR        (0x40060044) /**< (TWIHS2) Filter Register */
#define REG_TWIHS2_SWMR         (0x4006004C) /**< (TWIHS2) SleepWalking Matching Register */
#define REG_TWIHS2_WPMR         (0x400600E4) /**< (TWIHS2) Write Protection Mode Register */
#define REG_TWIHS2_WPSR         (0x400600E8) /**< (TWIHS2) Write Protection Status Register */

#else

#define REG_TWIHS2_CR           (*(__O  uint32_t*)0x40060000U) /**< (TWIHS2) Control Register */
#define REG_TWIHS2_MMR          (*(__IO uint32_t*)0x40060004U) /**< (TWIHS2) Master Mode Register */
#define REG_TWIHS2_SMR          (*(__IO uint32_t*)0x40060008U) /**< (TWIHS2) Slave Mode Register */
#define REG_TWIHS2_IADR         (*(__IO uint32_t*)0x4006000CU) /**< (TWIHS2) Internal Address Register */
#define REG_TWIHS2_CWGR         (*(__IO uint32_t*)0x40060010U) /**< (TWIHS2) Clock Waveform Generator Register */
#define REG_TWIHS2_SR           (*(__I  uint32_t*)0x40060020U) /**< (TWIHS2) Status Register */
#define REG_TWIHS2_IER          (*(__O  uint32_t*)0x40060024U) /**< (TWIHS2) Interrupt Enable Register */
#define REG_TWIHS2_IDR          (*(__O  uint32_t*)0x40060028U) /**< (TWIHS2) Interrupt Disable Register */
#define REG_TWIHS2_IMR          (*(__I  uint32_t*)0x4006002CU) /**< (TWIHS2) Interrupt Mask Register */
#define REG_TWIHS2_RHR          (*(__I  uint32_t*)0x40060030U) /**< (TWIHS2) Receive Holding Register */
#define REG_TWIHS2_THR          (*(__O  uint32_t*)0x40060034U) /**< (TWIHS2) Transmit Holding Register */
#define REG_TWIHS2_SMBTR        (*(__IO uint32_t*)0x40060038U) /**< (TWIHS2) SMBus Timing Register */
#define REG_TWIHS2_FILTR        (*(__IO uint32_t*)0x40060044U) /**< (TWIHS2) Filter Register */
#define REG_TWIHS2_SWMR         (*(__IO uint32_t*)0x4006004CU) /**< (TWIHS2) SleepWalking Matching Register */
#define REG_TWIHS2_WPMR         (*(__IO uint32_t*)0x400600E4U) /**< (TWIHS2) Write Protection Mode Register */
#define REG_TWIHS2_WPSR         (*(__I  uint32_t*)0x400600E8U) /**< (TWIHS2) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for TWIHS2 peripheral ========== */
#define TWIHS2_INSTANCE_ID                       41        
#define TWIHS2_DMAC_ID_TX                        18        
#define TWIHS2_DMAC_ID_RX                        19        

#endif /* _SAME70_TWIHS2_INSTANCE_ */
