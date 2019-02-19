/**
 * \file
 *
 * \brief Instance description for RFCTRL
 *
 * Copyright (c) 2016 Atmel Corporation,
 *                    a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#ifndef _SAMR21_RFCTRL_INSTANCE_
#define _SAMR21_RFCTRL_INSTANCE_

/* ========== Register definition for RFCTRL peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_RFCTRL_FECFG           (0x42005400) /**< \brief (RFCTRL) Front-end control bus configuration */
#else
#define REG_RFCTRL_FECFG           (*(RwReg16*)0x42005400UL) /**< \brief (RFCTRL) Front-end control bus configuration */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* ========== Instance parameters for RFCTRL peripheral ========== */
#define RFCTRL_FBUSMSB              5       

#endif /* _SAMR21_RFCTRL_INSTANCE_ */
