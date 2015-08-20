/* scb.h - ARM CORTEX-M3 System Control Block interface */

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

Most of the SCB interface consists of simple bit-flipping methods, and is
implemented as inline functions in scb.h. This module thus contains only data
definitions and more complex routines, if needed.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <misc/util.h>

#define SCB_AIRCR_VECTKEY_EN_W 0x05FA

/**
 *
 * @brief Reset the system
 *
 * This routine resets the processor.
 *
 * @return N/A
 */

void _ScbSystemReset(void)
{
	union __aircr reg;

	reg.val = __scs.scb.aircr.val;
	reg.bit.vectkey = SCB_AIRCR_VECTKEY_EN_W;
	reg.bit.sysresetreq = 1;
	__scs.scb.aircr.val = reg.val;
}

/**
 *
 * @brief Set the number of priority groups based on the number
 *                      of exception priorities desired
 *
 * Exception priorities can be divided in priority groups, inside which there is
 * no preemption. The priorities inside a group are only used to decide which
 * exception will run when more than one is ready to be handled.
 *
 * The number of priorities has to be a power of two, from 1 to 128.
 *
 * @return N/A
 */

void _ScbNumPriGroupSet(unsigned int n /* number of priorities */
			)
{
	unsigned int set;
	union __aircr reg;

	__ASSERT(is_power_of_two(n) && (n <= 128),
		 "invalid number of priorities");

	set = find_lsb_set(n);

	reg.val = __scs.scb.aircr.val;

	/* num pri    bit set   prigroup
	 * ---------------------------------
	 *      1        1          7
	 *      2        2          6
	 *      4        3          5
	 *      8        4          4
	 *     16        5          3
	 *     32        6          2
	 *     64        7          1
	 *    128        8          0
	 */

	reg.bit.prigroup = 8 - set;
	reg.bit.vectkey = SCB_AIRCR_VECTKEY_EN_W;

	__scs.scb.aircr.val = reg.val;
}
