/**
 * \file
 *
 * \brief Instance description for SDRAMC
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

#ifndef _SAME70_SDRAMC_INSTANCE_H_
#define _SAME70_SDRAMC_INSTANCE_H_

/* ========== Register definition for SDRAMC peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_SDRAMC_MR           (0x40084000) /**< (SDRAMC) SDRAMC Mode Register */
#define REG_SDRAMC_TR           (0x40084004) /**< (SDRAMC) SDRAMC Refresh Timer Register */
#define REG_SDRAMC_CR           (0x40084008) /**< (SDRAMC) SDRAMC Configuration Register */
#define REG_SDRAMC_LPR          (0x40084010) /**< (SDRAMC) SDRAMC Low Power Register */
#define REG_SDRAMC_IER          (0x40084014) /**< (SDRAMC) SDRAMC Interrupt Enable Register */
#define REG_SDRAMC_IDR          (0x40084018) /**< (SDRAMC) SDRAMC Interrupt Disable Register */
#define REG_SDRAMC_IMR          (0x4008401C) /**< (SDRAMC) SDRAMC Interrupt Mask Register */
#define REG_SDRAMC_ISR          (0x40084020) /**< (SDRAMC) SDRAMC Interrupt Status Register */
#define REG_SDRAMC_MDR          (0x40084024) /**< (SDRAMC) SDRAMC Memory Device Register */
#define REG_SDRAMC_CFR1         (0x40084028) /**< (SDRAMC) SDRAMC Configuration Register 1 */
#define REG_SDRAMC_OCMS         (0x4008402C) /**< (SDRAMC) SDRAMC OCMS Register */
#define REG_SDRAMC_OCMS_KEY1    (0x40084030) /**< (SDRAMC) SDRAMC OCMS KEY1 Register */
#define REG_SDRAMC_OCMS_KEY2    (0x40084034) /**< (SDRAMC) SDRAMC OCMS KEY2 Register */

#else

#define REG_SDRAMC_MR           (*(__IO uint32_t*)0x40084000U) /**< (SDRAMC) SDRAMC Mode Register */
#define REG_SDRAMC_TR           (*(__IO uint32_t*)0x40084004U) /**< (SDRAMC) SDRAMC Refresh Timer Register */
#define REG_SDRAMC_CR           (*(__IO uint32_t*)0x40084008U) /**< (SDRAMC) SDRAMC Configuration Register */
#define REG_SDRAMC_LPR          (*(__IO uint32_t*)0x40084010U) /**< (SDRAMC) SDRAMC Low Power Register */
#define REG_SDRAMC_IER          (*(__O  uint32_t*)0x40084014U) /**< (SDRAMC) SDRAMC Interrupt Enable Register */
#define REG_SDRAMC_IDR          (*(__O  uint32_t*)0x40084018U) /**< (SDRAMC) SDRAMC Interrupt Disable Register */
#define REG_SDRAMC_IMR          (*(__I  uint32_t*)0x4008401CU) /**< (SDRAMC) SDRAMC Interrupt Mask Register */
#define REG_SDRAMC_ISR          (*(__I  uint32_t*)0x40084020U) /**< (SDRAMC) SDRAMC Interrupt Status Register */
#define REG_SDRAMC_MDR          (*(__IO uint32_t*)0x40084024U) /**< (SDRAMC) SDRAMC Memory Device Register */
#define REG_SDRAMC_CFR1         (*(__IO uint32_t*)0x40084028U) /**< (SDRAMC) SDRAMC Configuration Register 1 */
#define REG_SDRAMC_OCMS         (*(__IO uint32_t*)0x4008402CU) /**< (SDRAMC) SDRAMC OCMS Register */
#define REG_SDRAMC_OCMS_KEY1    (*(__O  uint32_t*)0x40084030U) /**< (SDRAMC) SDRAMC OCMS KEY1 Register */
#define REG_SDRAMC_OCMS_KEY2    (*(__O  uint32_t*)0x40084034U) /**< (SDRAMC) SDRAMC OCMS KEY2 Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for SDRAMC peripheral ========== */
#define SDRAMC_INSTANCE_ID                       62        

#endif /* _SAME70_SDRAMC_INSTANCE_ */
