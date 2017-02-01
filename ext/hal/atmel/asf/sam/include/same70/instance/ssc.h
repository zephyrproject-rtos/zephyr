/**
 * \file
 *
 * \brief Instance description for SSC
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

#ifndef _SAME70_SSC_INSTANCE_H_
#define _SAME70_SSC_INSTANCE_H_

/* ========== Register definition for SSC peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_SSC_CR              (0x40004000) /**< (SSC) Control Register */
#define REG_SSC_CMR             (0x40004004) /**< (SSC) Clock Mode Register */
#define REG_SSC_RCMR            (0x40004010) /**< (SSC) Receive Clock Mode Register */
#define REG_SSC_RFMR            (0x40004014) /**< (SSC) Receive Frame Mode Register */
#define REG_SSC_TCMR            (0x40004018) /**< (SSC) Transmit Clock Mode Register */
#define REG_SSC_TFMR            (0x4000401C) /**< (SSC) Transmit Frame Mode Register */
#define REG_SSC_RHR             (0x40004020) /**< (SSC) Receive Holding Register */
#define REG_SSC_THR             (0x40004024) /**< (SSC) Transmit Holding Register */
#define REG_SSC_RSHR            (0x40004030) /**< (SSC) Receive Sync. Holding Register */
#define REG_SSC_TSHR            (0x40004034) /**< (SSC) Transmit Sync. Holding Register */
#define REG_SSC_RC0R            (0x40004038) /**< (SSC) Receive Compare 0 Register */
#define REG_SSC_RC1R            (0x4000403C) /**< (SSC) Receive Compare 1 Register */
#define REG_SSC_SR              (0x40004040) /**< (SSC) Status Register */
#define REG_SSC_IER             (0x40004044) /**< (SSC) Interrupt Enable Register */
#define REG_SSC_IDR             (0x40004048) /**< (SSC) Interrupt Disable Register */
#define REG_SSC_IMR             (0x4000404C) /**< (SSC) Interrupt Mask Register */
#define REG_SSC_WPMR            (0x400040E4) /**< (SSC) Write Protection Mode Register */
#define REG_SSC_WPSR            (0x400040E8) /**< (SSC) Write Protection Status Register */

#else

#define REG_SSC_CR              (*(__O  uint32_t*)0x40004000U) /**< (SSC) Control Register */
#define REG_SSC_CMR             (*(__IO uint32_t*)0x40004004U) /**< (SSC) Clock Mode Register */
#define REG_SSC_RCMR            (*(__IO uint32_t*)0x40004010U) /**< (SSC) Receive Clock Mode Register */
#define REG_SSC_RFMR            (*(__IO uint32_t*)0x40004014U) /**< (SSC) Receive Frame Mode Register */
#define REG_SSC_TCMR            (*(__IO uint32_t*)0x40004018U) /**< (SSC) Transmit Clock Mode Register */
#define REG_SSC_TFMR            (*(__IO uint32_t*)0x4000401CU) /**< (SSC) Transmit Frame Mode Register */
#define REG_SSC_RHR             (*(__I  uint32_t*)0x40004020U) /**< (SSC) Receive Holding Register */
#define REG_SSC_THR             (*(__O  uint32_t*)0x40004024U) /**< (SSC) Transmit Holding Register */
#define REG_SSC_RSHR            (*(__I  uint32_t*)0x40004030U) /**< (SSC) Receive Sync. Holding Register */
#define REG_SSC_TSHR            (*(__IO uint32_t*)0x40004034U) /**< (SSC) Transmit Sync. Holding Register */
#define REG_SSC_RC0R            (*(__IO uint32_t*)0x40004038U) /**< (SSC) Receive Compare 0 Register */
#define REG_SSC_RC1R            (*(__IO uint32_t*)0x4000403CU) /**< (SSC) Receive Compare 1 Register */
#define REG_SSC_SR              (*(__I  uint32_t*)0x40004040U) /**< (SSC) Status Register */
#define REG_SSC_IER             (*(__O  uint32_t*)0x40004044U) /**< (SSC) Interrupt Enable Register */
#define REG_SSC_IDR             (*(__O  uint32_t*)0x40004048U) /**< (SSC) Interrupt Disable Register */
#define REG_SSC_IMR             (*(__I  uint32_t*)0x4000404CU) /**< (SSC) Interrupt Mask Register */
#define REG_SSC_WPMR            (*(__IO uint32_t*)0x400040E4U) /**< (SSC) Write Protection Mode Register */
#define REG_SSC_WPSR            (*(__I  uint32_t*)0x400040E8U) /**< (SSC) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for SSC peripheral ========== */
#define SSC_INSTANCE_ID                          22        
#define SSC_DMAC_ID_TX                           32        
#define SSC_DMAC_ID_RX                           33        

#endif /* _SAME70_SSC_INSTANCE_ */
