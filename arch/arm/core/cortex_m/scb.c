/* scb.h - ARM CORTEX-M3 System Control Block interface */

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
