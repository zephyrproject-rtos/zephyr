/* arch.h - ARM specific nanokernel interface header */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

/* APIs need to support non-byte addressible architectures */

#define OCTET_TO_SIZEOFUNIT(X) (X)
#define SIZEOFUNIT_TO_OCTET(X) (X)

#ifdef CONFIG_CPU_CORTEXM
#include <arch/arm/CortexM/exc.h>
#include <arch/arm/CortexM/irq.h>
#include <arch/arm/CortexM/ffs.h>
#include <arch/arm/CortexM/error.h>
#include <arch/arm/CortexM/misc.h>
#include <arch/arm/CortexM/scs.h>
#include <arch/arm/CortexM/scb.h>
#include <arch/arm/CortexM/nvic.h>
#include <arch/arm/CortexM/memory_map.h>
#include <arch/arm/CortexM/gdb_stub.h>
#include <arch/arm/CortexM/asm_inline.h>
#endif

#define STACK_ALIGN  4

#ifdef __cplusplus
}
#endif

#endif /* _ARM_ARCH__H_ */
