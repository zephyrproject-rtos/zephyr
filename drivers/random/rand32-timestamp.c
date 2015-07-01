/* non-random number generator based on x86 CPU timestamp */

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
This module provides a non-random implementation of sys_rand32_get(), which is
not meant to be used in a final product as a truly random number generator. It
was provided to allow testing on a BSP that does not (yet) provide a random
number generator.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <drivers/rand32.h>

/**
 *
 * sys_rand32_init - initialize the random number generator
 *
 * The non-random number generator does not require any initialization.
 * Routine is automatically invoked by the kernel during system startup.
 *
 * RETURNS: N/A
 */

void sys_rand32_init(void)
{
}

/**
 *
 * sys_rand32_get - get a 32 bit random number
 *
 * The non-random number generator returns values that are based off the
 * CPU's timestamp counter, which means that successive calls will normally
 * display ever-increasing values.
 *
 * RETURNS: a 32-bit number
 */

uint32_t sys_rand32_get(void)
{
	return _do_read_cpu_timestamp32();
}
