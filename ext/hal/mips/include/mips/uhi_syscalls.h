/*
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
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


#ifndef _UHI_SYSCALLS_
#define _UHI_SYSCALLS_

#define __MIPS_UHI_EXIT        1
#define __MIPS_UHI_OPEN        2
#define __MIPS_UHI_CLOSE       3
#define __MIPS_UHI_READ        4
#define __MIPS_UHI_WRITE       5
#define __MIPS_UHI_LSEEK       6
#define __MIPS_UHI_UNLINK      7
#define __MIPS_UHI_FSTAT       8
#define __MIPS_UHI_ARGC        9
#define __MIPS_UHI_ARGLEN      10
#define __MIPS_UHI_ARGN        11
#define __MIPS_UHI_RAMRANGE    12
#define __MIPS_UHI_LOG         13
#define __MIPS_UHI_ASSERT      14
#define __MIPS_UHI_EXCEPTION   15
#define __MIPS_UHI_PREAD       19
#define __MIPS_UHI_PWRITE      20
#define __MIPS_UHI_LINK        22
#define __MIPS_UHI_BOOTFAIL    23

#define __MIPS_UHI_BF_CACHE    1

#define __xstr(s) __str(s)
#define __str(s) #s
#define __MIPS_UHI_SYSCALL_NUM 1

#ifdef __MIPS_SDBBP__
	#define SYSCALL(NUM) "\tsdbbp " __xstr (NUM)
	#define ASM_SYSCALL(NUM) sdbbp NUM
#else
	#define SYSCALL(NUM) "\tsyscall " __xstr (NUM)
	#define ASM_SYSCALL(NUM) syscall NUM
#endif

#endif // _UHI_SYSCALLS_
