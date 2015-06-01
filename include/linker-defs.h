/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
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
Platform independent, commonly used macros and defines related to linker script.

This file may be included by:
- Linker script files: for linker section declarations
- C files: for external declaration of address or size of linker section
- Assembly files: for external declaration of address or size of linker section
*/

#ifndef _LINKERDEFS_H
#define _LINKERDEFS_H

#include <toolchain.h>
#include <sections.h>

/* include platform dependent linker-defs */
#ifdef VXMICRO_ARCH_x86
#include <arch/x86/linker-defs-arch.h>
#elif defined(VXMICRO_ARCH_arm)
/* Nothing yet to include */
#elif defined(VXMICRO_ARCH_arc)
/* Nothing yet to include */
#else
#error Arch not supported.
#endif

#ifdef _LINKER
#ifdef VXMICRO_ARCH_x86 /* LINKER FILES: defines used by linker script */
/* Should be moved to linker-common-defs.h */
#if defined(CONFIG_XIP)
#define ROMABLE_REGION ROM
#else
#define ROMABLE_REGION RAM
#endif
#endif

/*
 * If image is loaded via kexec Linux system call, then program
 * headers need to be page aligned.
 * This can be done by section page aligning.
 */
#ifdef CONFIG_BOOTLOADER_KEXEC
#define KEXEC_PGALIGN_PAD(x) . = ALIGN(x);
#else
#define KEXEC_PGALIGN_PAD(x)
#endif

#elif defined(_ASMLANGUAGE)
/* Assembly FILES: declaration defined by the linker script */
GDATA(__bss_start)
GDATA(__bss_num_words)
#ifdef CONFIG_XIP
GDATA(__data_rom_start)
GDATA(__data_ram_start)
GDATA(__data_num_words)
#endif

#else

#include <stdint.h>
extern char __bss_start[];
extern int __bss_num_words[];
#ifdef CONFIG_XIP
extern char __data_rom_start[];
extern char __data_ram_start[];
extern int __data_num_words[];
#endif

/* end address of image. */
extern char _end[];
#define _END_VPAGE (VIRT_ADDR) _end

#endif /* ! _ASMLANGUAGE */

#endif /* _LINKERDEFS_H */
