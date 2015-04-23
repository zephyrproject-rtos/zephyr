/* x86 implementation of k_memset, k_memcpy, etc */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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
The conditional definitions of k_memcpy() and k_memset() exist because it
has been found that depending upon the choice of compiler, calls to memcpy()
and memset() may or may not be size-optimized to "rep movsb" or "rep stosb".
Consequently, without these conditional defines, there could be unresolved
symbols to memcpy() and memset().

This file is included indirectly by a number of HostServer source files
which include microkernel/k_struct.h.  These files define VXMICRO_HOST_TOOLS
when including target header files.   The Microsoft Visual C++ compiler
used to build the HostServer does not handle the "static inline"
declaration, so the HostServer uses the memcpy/memset implementations.

Typical use of the inline function has the size (_s) and value (_v) parameters
defined as immediate value and therefore the compiler optimization resolves the
if and shift operations in order to generate minimal code with maximum
performance.
*/

#include <toolchain.h>
#include <sections.h>
#include <stddef.h>

#if defined(__GNUC__)
	#include <nanokernel/x86/k_mem-gcc.h>
#else
	#include <nanokernel/x86/k_mem-other.h>
#endif
