/* Intelprc.h - IA-32 specific definitions for cputype.h */

/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
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
This file is included by cputype.h when the VXMICRO_ARCH_x86 macro is defined,
i.e. whenever a build for the IA-32 architecture is being performed.  This
file shall only contain the CPU/compiler specific definitions that are
necessary to build the EMBEDDED kernel library.
*/

#ifndef _INTELPRC_H
#define _INTELPRC_H

#ifdef __cplusplus
extern "C"
	{
#endif

#define OCTET_TO_SIZEOFUNIT(X) (X)	  /* byte addressing */
#define SIZEOFUNIT_TO_OCTET(X) (X)

#include <stdint.h>
#include <toolchain.h>
#include <misc/util.h>
#include <arch/x86/addr_types.h>
#include <arch/x86/asm_inline.h>
#include <drivers/system_timer.h> /* timer_driver() needed by kernel_main.c */

#ifdef __cplusplus
	}
#endif

#endif /* _INTELPRC_H */
