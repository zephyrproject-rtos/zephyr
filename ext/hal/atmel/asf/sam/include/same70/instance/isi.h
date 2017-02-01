/**
 * \file
 *
 * \brief Instance description for ISI
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

#ifndef _SAME70_ISI_INSTANCE_H_
#define _SAME70_ISI_INSTANCE_H_

/* ========== Register definition for ISI peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_ISI_CFG1            (0x4004C000) /**< (ISI) ISI Configuration 1 Register */
#define REG_ISI_CFG2            (0x4004C004) /**< (ISI) ISI Configuration 2 Register */
#define REG_ISI_PSIZE           (0x4004C008) /**< (ISI) ISI Preview Size Register */
#define REG_ISI_PDECF           (0x4004C00C) /**< (ISI) ISI Preview Decimation Factor Register */
#define REG_ISI_Y2R_SET0        (0x4004C010) /**< (ISI) ISI Color Space Conversion YCrCb To RGB Set 0 Register */
#define REG_ISI_Y2R_SET1        (0x4004C014) /**< (ISI) ISI Color Space Conversion YCrCb To RGB Set 1 Register */
#define REG_ISI_R2Y_SET0        (0x4004C018) /**< (ISI) ISI Color Space Conversion RGB To YCrCb Set 0 Register */
#define REG_ISI_R2Y_SET1        (0x4004C01C) /**< (ISI) ISI Color Space Conversion RGB To YCrCb Set 1 Register */
#define REG_ISI_R2Y_SET2        (0x4004C020) /**< (ISI) ISI Color Space Conversion RGB To YCrCb Set 2 Register */
#define REG_ISI_CR              (0x4004C024) /**< (ISI) ISI Control Register */
#define REG_ISI_SR              (0x4004C028) /**< (ISI) ISI Status Register */
#define REG_ISI_IER             (0x4004C02C) /**< (ISI) ISI Interrupt Enable Register */
#define REG_ISI_IDR             (0x4004C030) /**< (ISI) ISI Interrupt Disable Register */
#define REG_ISI_IMR             (0x4004C034) /**< (ISI) ISI Interrupt Mask Register */
#define REG_ISI_DMA_CHER        (0x4004C038) /**< (ISI) DMA Channel Enable Register */
#define REG_ISI_DMA_CHDR        (0x4004C03C) /**< (ISI) DMA Channel Disable Register */
#define REG_ISI_DMA_CHSR        (0x4004C040) /**< (ISI) DMA Channel Status Register */
#define REG_ISI_DMA_P_ADDR      (0x4004C044) /**< (ISI) DMA Preview Base Address Register */
#define REG_ISI_DMA_P_CTRL      (0x4004C048) /**< (ISI) DMA Preview Control Register */
#define REG_ISI_DMA_P_DSCR      (0x4004C04C) /**< (ISI) DMA Preview Descriptor Address Register */
#define REG_ISI_DMA_C_ADDR      (0x4004C050) /**< (ISI) DMA Codec Base Address Register */
#define REG_ISI_DMA_C_CTRL      (0x4004C054) /**< (ISI) DMA Codec Control Register */
#define REG_ISI_DMA_C_DSCR      (0x4004C058) /**< (ISI) DMA Codec Descriptor Address Register */
#define REG_ISI_WPMR            (0x4004C0E4) /**< (ISI) Write Protection Mode Register */
#define REG_ISI_WPSR            (0x4004C0E8) /**< (ISI) Write Protection Status Register */

#else

#define REG_ISI_CFG1            (*(__IO uint32_t*)0x4004C000U) /**< (ISI) ISI Configuration 1 Register */
#define REG_ISI_CFG2            (*(__IO uint32_t*)0x4004C004U) /**< (ISI) ISI Configuration 2 Register */
#define REG_ISI_PSIZE           (*(__IO uint32_t*)0x4004C008U) /**< (ISI) ISI Preview Size Register */
#define REG_ISI_PDECF           (*(__IO uint32_t*)0x4004C00CU) /**< (ISI) ISI Preview Decimation Factor Register */
#define REG_ISI_Y2R_SET0        (*(__IO uint32_t*)0x4004C010U) /**< (ISI) ISI Color Space Conversion YCrCb To RGB Set 0 Register */
#define REG_ISI_Y2R_SET1        (*(__IO uint32_t*)0x4004C014U) /**< (ISI) ISI Color Space Conversion YCrCb To RGB Set 1 Register */
#define REG_ISI_R2Y_SET0        (*(__IO uint32_t*)0x4004C018U) /**< (ISI) ISI Color Space Conversion RGB To YCrCb Set 0 Register */
#define REG_ISI_R2Y_SET1        (*(__IO uint32_t*)0x4004C01CU) /**< (ISI) ISI Color Space Conversion RGB To YCrCb Set 1 Register */
#define REG_ISI_R2Y_SET2        (*(__IO uint32_t*)0x4004C020U) /**< (ISI) ISI Color Space Conversion RGB To YCrCb Set 2 Register */
#define REG_ISI_CR              (*(__O  uint32_t*)0x4004C024U) /**< (ISI) ISI Control Register */
#define REG_ISI_SR              (*(__I  uint32_t*)0x4004C028U) /**< (ISI) ISI Status Register */
#define REG_ISI_IER             (*(__O  uint32_t*)0x4004C02CU) /**< (ISI) ISI Interrupt Enable Register */
#define REG_ISI_IDR             (*(__O  uint32_t*)0x4004C030U) /**< (ISI) ISI Interrupt Disable Register */
#define REG_ISI_IMR             (*(__I  uint32_t*)0x4004C034U) /**< (ISI) ISI Interrupt Mask Register */
#define REG_ISI_DMA_CHER        (*(__O  uint32_t*)0x4004C038U) /**< (ISI) DMA Channel Enable Register */
#define REG_ISI_DMA_CHDR        (*(__O  uint32_t*)0x4004C03CU) /**< (ISI) DMA Channel Disable Register */
#define REG_ISI_DMA_CHSR        (*(__I  uint32_t*)0x4004C040U) /**< (ISI) DMA Channel Status Register */
#define REG_ISI_DMA_P_ADDR      (*(__IO uint32_t*)0x4004C044U) /**< (ISI) DMA Preview Base Address Register */
#define REG_ISI_DMA_P_CTRL      (*(__IO uint32_t*)0x4004C048U) /**< (ISI) DMA Preview Control Register */
#define REG_ISI_DMA_P_DSCR      (*(__IO uint32_t*)0x4004C04CU) /**< (ISI) DMA Preview Descriptor Address Register */
#define REG_ISI_DMA_C_ADDR      (*(__IO uint32_t*)0x4004C050U) /**< (ISI) DMA Codec Base Address Register */
#define REG_ISI_DMA_C_CTRL      (*(__IO uint32_t*)0x4004C054U) /**< (ISI) DMA Codec Control Register */
#define REG_ISI_DMA_C_DSCR      (*(__IO uint32_t*)0x4004C058U) /**< (ISI) DMA Codec Descriptor Address Register */
#define REG_ISI_WPMR            (*(__IO uint32_t*)0x4004C0E4U) /**< (ISI) Write Protection Mode Register */
#define REG_ISI_WPSR            (*(__I  uint32_t*)0x4004C0E8U) /**< (ISI) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for ISI peripheral ========== */
#define ISI_INSTANCE_ID                          59        

#endif /* _SAME70_ISI_INSTANCE_ */
