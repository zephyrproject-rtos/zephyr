/**
 * \file
 *
 * \brief Instance description for WDT
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

#ifndef _SAME70_WDT_INSTANCE_H_
#define _SAME70_WDT_INSTANCE_H_

/* ========== Register definition for WDT peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_WDT_CR              (0x400E1850) /**< (WDT) Control Register */
#define REG_WDT_MR              (0x400E1854) /**< (WDT) Mode Register */
#define REG_WDT_SR              (0x400E1858) /**< (WDT) Status Register */

#else

#define REG_WDT_CR              (*(__O  uint32_t*)0x400E1850U) /**< (WDT) Control Register */
#define REG_WDT_MR              (*(__IO uint32_t*)0x400E1854U) /**< (WDT) Mode Register */
#define REG_WDT_SR              (*(__I  uint32_t*)0x400E1858U) /**< (WDT) Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for WDT peripheral ========== */
#define WDT_INSTANCE_ID                          4         

#endif /* _SAME70_WDT_INSTANCE_ */
