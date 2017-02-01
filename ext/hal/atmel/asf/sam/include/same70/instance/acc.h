/**
 * \file
 *
 * \brief Instance description for ACC
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

#ifndef _SAME70_ACC_INSTANCE_H_
#define _SAME70_ACC_INSTANCE_H_

/* ========== Register definition for ACC peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_ACC_CR              (0x40044000) /**< (ACC) Control Register */
#define REG_ACC_MR              (0x40044004) /**< (ACC) Mode Register */
#define REG_ACC_IER             (0x40044024) /**< (ACC) Interrupt Enable Register */
#define REG_ACC_IDR             (0x40044028) /**< (ACC) Interrupt Disable Register */
#define REG_ACC_IMR             (0x4004402C) /**< (ACC) Interrupt Mask Register */
#define REG_ACC_ISR             (0x40044030) /**< (ACC) Interrupt Status Register */
#define REG_ACC_ACR             (0x40044094) /**< (ACC) Analog Control Register */
#define REG_ACC_WPMR            (0x400440E4) /**< (ACC) Write Protection Mode Register */
#define REG_ACC_WPSR            (0x400440E8) /**< (ACC) Write Protection Status Register */

#else

#define REG_ACC_CR              (*(__O  uint32_t*)0x40044000U) /**< (ACC) Control Register */
#define REG_ACC_MR              (*(__IO uint32_t*)0x40044004U) /**< (ACC) Mode Register */
#define REG_ACC_IER             (*(__O  uint32_t*)0x40044024U) /**< (ACC) Interrupt Enable Register */
#define REG_ACC_IDR             (*(__O  uint32_t*)0x40044028U) /**< (ACC) Interrupt Disable Register */
#define REG_ACC_IMR             (*(__I  uint32_t*)0x4004402CU) /**< (ACC) Interrupt Mask Register */
#define REG_ACC_ISR             (*(__I  uint32_t*)0x40044030U) /**< (ACC) Interrupt Status Register */
#define REG_ACC_ACR             (*(__IO uint32_t*)0x40044094U) /**< (ACC) Analog Control Register */
#define REG_ACC_WPMR            (*(__IO uint32_t*)0x400440E4U) /**< (ACC) Write Protection Mode Register */
#define REG_ACC_WPSR            (*(__I  uint32_t*)0x400440E8U) /**< (ACC) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for ACC peripheral ========== */
#define ACC_INSTANCE_ID                          33        

#endif /* _SAME70_ACC_INSTANCE_ */
