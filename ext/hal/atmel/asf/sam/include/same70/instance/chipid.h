/**
 * \file
 *
 * \brief Instance description for CHIPID
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

#ifndef _SAME70_CHIPID_INSTANCE_H_
#define _SAME70_CHIPID_INSTANCE_H_

/* ========== Register definition for CHIPID peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_CHIPID_CIDR         (0x400E0940) /**< (CHIPID) Chip ID Register */
#define REG_CHIPID_EXID         (0x400E0944) /**< (CHIPID) Chip ID Extension Register */

#else

#define REG_CHIPID_CIDR         (*(__I  uint32_t*)0x400E0940U) /**< (CHIPID) Chip ID Register */
#define REG_CHIPID_EXID         (*(__I  uint32_t*)0x400E0944U) /**< (CHIPID) Chip ID Extension Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAME70_CHIPID_INSTANCE_ */
