/* arch.h - ARM specific nanokernel interface header */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
This header contains the ARM specific nanokernel interface.  It is
included by the nanokernel interface architecture-abstraction header
(nanokernel/cpu.h)
 */

#ifndef _ARM_ARCH__H_
#define _ARM_ARCH__H_

#ifdef __cplusplus
extern "C" {
#endif

/* APIs need to support non-byte addressable architectures */

#define OCTET_TO_SIZEOFUNIT(X) (X)
#define SIZEOFUNIT_TO_OCTET(X) (X)

#ifdef CONFIG_CPU_CORTEX_M
#include <arch/arm/cortex_m/exc.h>
#include <arch/arm/cortex_m/irq.h>
#include <arch/arm/cortex_m/error.h>
#include <arch/arm/cortex_m/misc.h>
#include <arch/arm/cortex_m/scs.h>
#include <arch/arm/cortex_m/scb.h>
#include <arch/arm/cortex_m/nvic.h>
#include <arch/arm/cortex_m/memory_map.h>
#include <arch/arm/cortex_m/gdb_stub.h>
#include <arch/arm/cortex_m/asm_inline.h>
#endif

#define STACK_ALIGN  4

#ifdef __cplusplus
}
#endif

#endif /* _ARM_ARCH__H_ */
