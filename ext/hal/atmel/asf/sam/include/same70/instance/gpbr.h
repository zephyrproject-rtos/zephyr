/**
 * \file
 *
 * \brief Instance description for GPBR
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

#ifndef _SAME70_GPBR_INSTANCE_H_
#define _SAME70_GPBR_INSTANCE_H_

/* ========== Register definition for GPBR peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_GPBR_SYS_GPBR       (0x400E1890) /**< (GPBR) General Purpose Backup Register 0 */
#define REG_GPBR_SYS_GPBR0      (0x400E1890) /**< (GPBR) General Purpose Backup Register 0 */
#define REG_GPBR_SYS_GPBR1      (0x400E1894) /**< (GPBR) General Purpose Backup Register 1 */
#define REG_GPBR_SYS_GPBR2      (0x400E1898) /**< (GPBR) General Purpose Backup Register 2 */
#define REG_GPBR_SYS_GPBR3      (0x400E189C) /**< (GPBR) General Purpose Backup Register 3 */
#define REG_GPBR_SYS_GPBR4      (0x400E18A0) /**< (GPBR) General Purpose Backup Register 4 */
#define REG_GPBR_SYS_GPBR5      (0x400E18A4) /**< (GPBR) General Purpose Backup Register 5 */
#define REG_GPBR_SYS_GPBR6      (0x400E18A8) /**< (GPBR) General Purpose Backup Register 6 */
#define REG_GPBR_SYS_GPBR7      (0x400E18AC) /**< (GPBR) General Purpose Backup Register 7 */

#else

#define REG_GPBR_SYS_GPBR       (*(__IO uint32_t*)0x400E1890U) /**< (GPBR) General Purpose Backup Register 0 */
#define REG_GPBR_SYS_GPBR0      (*(__IO uint32_t*)0x400E1890U) /**< (GPBR) General Purpose Backup Register 0 */
#define REG_GPBR_SYS_GPBR1      (*(__IO uint32_t*)0x400E1894U) /**< (GPBR) General Purpose Backup Register 1 */
#define REG_GPBR_SYS_GPBR2      (*(__IO uint32_t*)0x400E1898U) /**< (GPBR) General Purpose Backup Register 2 */
#define REG_GPBR_SYS_GPBR3      (*(__IO uint32_t*)0x400E189CU) /**< (GPBR) General Purpose Backup Register 3 */
#define REG_GPBR_SYS_GPBR4      (*(__IO uint32_t*)0x400E18A0U) /**< (GPBR) General Purpose Backup Register 4 */
#define REG_GPBR_SYS_GPBR5      (*(__IO uint32_t*)0x400E18A4U) /**< (GPBR) General Purpose Backup Register 5 */
#define REG_GPBR_SYS_GPBR6      (*(__IO uint32_t*)0x400E18A8U) /**< (GPBR) General Purpose Backup Register 6 */
#define REG_GPBR_SYS_GPBR7      (*(__IO uint32_t*)0x400E18ACU) /**< (GPBR) General Purpose Backup Register 7 */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAME70_GPBR_INSTANCE_ */
