/* unaligned.S - utility routines to perform unaligned reads/writes */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
This module contains utilities to perform unaligned reads/writes from/to a
32-bit quantity.  Some memory subsystems to not support the IA-32 byte
enable lines, and thus accessing an unaligned 32-bit quantity is
performed byte-by-byte.
 */

#define _ASMLANGUAGE

#ifdef CONFIG_UNALIGNED_WRITE_UNSUPPORTED

#include <arch/x86/asm.h>

	/* exports (internal APIs) */

	GTEXT(_Unaligned32Write)
	GTEXT(_Unaligned32Read)

/**
 *
 * @brief Perform an unaligned 32-bit write operation
 *
 * This function is used during the interrupt and exception stub code 
 * synthesis step when writing out the 32-bit relative jmp/branch
 * offsets.  
 *
 * Generally, the 32-bit offsets are located at an odd memory address.  For
 * target hardware that don't fully (or properly) decode the byte enable
 * lines from the IA-32 processor, this function shall be utilized to write out
 * the data byte-by-byte.
 *
 * The platform specific configuration option CONFIG_UNALIGNED_WRITE_UNSUPPORTED
 * shall be defined when the kernel is built for target hardware that cannot
 * support unaligned double word (32-bit) write operations.
 *
 * C function prototype:
 *
 *  void _Unaligned32Write
 *      (
 *      unsigned int * ptr, 
 *      unsigned int val
 *      );
 */

SECTION_FUNC(TEXT, _Unaligned32Write)
	movl	0x4(%esp), %edx		/* fetch ptr argument */
 	movl	0x8(%esp), %eax		/* fetch val argument */
	movb	%al, (%edx)
	movb	%ah, 1(%edx)
	shrl	$0x10, %eax
	movb	%al, 2(%edx)
	movb	%ah, 3(%edx)
	ret


/**
 *
 * @brief Perform an unaligned 32-bit read operation
 *
 * This function is used during the interrupt and exception stub code
 * synthesis step when reading the 32-bit relative jmp/branch
 * offsets.  
 *
 * Generally, the 32-bit offsets are located at an odd memory address.  For
 * target hardware that don't fully (or properly) decode the byte enable
 * lines from the IA-32 processor, this function shall be utilized to read
 * the data byte-by-byte.
 *
 * The platform specific configuration option CONFIG_UNALIGNED_WRITE_UNSUPPORTED
 * shall be defined when the kernel is built for target hardware that cannot
 * support unaligned double word (32-bit) write operations.
 *
 * C function prototype:
 *
 *  unsigned int _Unaligned32Read
 *      (
 *      unsigned int * ptr
 *      );
 */

SECTION_FUNC(TEXT, _Unaligned32Read)
	movl	0x4(%esp), %edx		/* fetch ptr argument */
	movb	3(%edx), %ah
	movb	2(%edx), %al
	shll	$0x10, %eax
	movb	1(%edx), %ah
	movb	(%edx), %al
	ret

#endif /* CONFIG_UNALIGNED_WRITE_UNSUPPORTED */
