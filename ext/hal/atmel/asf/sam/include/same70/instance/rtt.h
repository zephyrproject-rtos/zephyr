/**
 * \file
 *
 * \brief Instance description for RTT
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

#ifndef _SAME70_RTT_INSTANCE_H_
#define _SAME70_RTT_INSTANCE_H_

/* ========== Register definition for RTT peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_RTT_MR              (0x400E1830) /**< (RTT) Mode Register */
#define REG_RTT_AR              (0x400E1834) /**< (RTT) Alarm Register */
#define REG_RTT_VR              (0x400E1838) /**< (RTT) Value Register */
#define REG_RTT_SR              (0x400E183C) /**< (RTT) Status Register */

#else

#define REG_RTT_MR              (*(__IO uint32_t*)0x400E1830U) /**< (RTT) Mode Register */
#define REG_RTT_AR              (*(__IO uint32_t*)0x400E1834U) /**< (RTT) Alarm Register */
#define REG_RTT_VR              (*(__I  uint32_t*)0x400E1838U) /**< (RTT) Value Register */
#define REG_RTT_SR              (*(__I  uint32_t*)0x400E183CU) /**< (RTT) Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for RTT peripheral ========== */
#define RTT_INSTANCE_ID                          3         

#endif /* _SAME70_RTT_INSTANCE_ */
