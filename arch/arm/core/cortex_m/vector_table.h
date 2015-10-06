/* vector_table.h - definitions for the boot vector table */

/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
DESCRIPTION

Definitions for the boot vector table.

System exception handler names all have the same format:

  __<exception name with underscores>

No other symbol has the same format, so they are easy to spot.
 */

#ifndef _VECTOR_TABLE__H_
#define _VECTOR_TABLE__H_

#ifdef _ASMLANGUAGE

#include <board.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/util.h>

/* location of MSP and PSP upon boot: at the end of SRAM */
.equ __CORTEXM_BOOT_MSP, (CONFIG_SRAM_BASE_ADDRESS + KB(CONFIG_SRAM_SIZE) - 8)
.equ __CORTEXM_BOOT_PSP, (__CORTEXM_BOOT_MSP - 0x100)

GTEXT(__start)
GTEXT(_VectorTableROM)

GTEXT(__reset)
GTEXT(__nmi)
GTEXT(__hard_fault)
GTEXT(__mpu_fault)
GTEXT(__bus_fault)
GTEXT(__usage_fault)
GTEXT(__svc)
GTEXT(__debug_monitor)
GTEXT(__pendsv)
GTEXT(__reserved)

GTEXT(_PrepC)
GTEXT(_isr_wrapper)

#endif /* _ASMLANGUAGE */

#endif /* _VECTOR_TABLE__H_ */
