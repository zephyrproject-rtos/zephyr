/* linker-defs-arch.h - Intel commonly used macro and defines for linker script */

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
Commonly used macros and defines for linker script.
*/
#ifndef _LINKERDEFSARCH_H
#define _LINKERDEFSARCH_H

#include <toolchain.h>

#define INT_STUB_NOINIT KEEP(*(.intStubSect))

#ifdef FINAL_LINK
  /* Use the real IDT */
  #define STATIC_IDT KEEP(*(staticIdt))
#else
  /*
   * Save space for the real IDT to prevent symbols from shifting. Note that
   * an IDT entry is 8 bytes in size.
   */
  #define STATIC_IDT . += (8 * CONFIG_IDT_NUM_VECTORS);
#endif

/*
 * The x86 manual recommends aligning the IDT on 8 byte boundary. This also
 * ensures that individual entries do not span a page boundary which the
 * interrupt management code relies on.
 */
#define IDT_MEMORY \
  . = ALIGN(8);\
  _idt_base_address = .;\
  STATIC_IDT

#endif   /* _LINKERDEFSARCH_H */
