/**
 * \file
 *
 * \brief Instance description for RSTC
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

#ifndef _SAME70_RSTC_INSTANCE_H_
#define _SAME70_RSTC_INSTANCE_H_

/* ========== Register definition for RSTC peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_RSTC_CR             (0x400E1800) /**< (RSTC) Control Register */
#define REG_RSTC_SR             (0x400E1804) /**< (RSTC) Status Register */
#define REG_RSTC_MR             (0x400E1808) /**< (RSTC) Mode Register */

#else

#define REG_RSTC_CR             (*(__O  uint32_t*)0x400E1800U) /**< (RSTC) Control Register */
#define REG_RSTC_SR             (*(__I  uint32_t*)0x400E1804U) /**< (RSTC) Status Register */
#define REG_RSTC_MR             (*(__IO uint32_t*)0x400E1808U) /**< (RSTC) Mode Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for RSTC peripheral ========== */
#define RSTC_INSTANCE_ID                         1         

#endif /* _SAME70_RSTC_INSTANCE_ */
