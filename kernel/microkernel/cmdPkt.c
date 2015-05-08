/* cmdPkt.c - library to manage statically allocated command packets */

/*
 * Copyright (c) 2012, 2014 Wind River Systems, Inc.
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
A command packet is an opaque data structure that maps to a k_args without
exposing its innards. It allows a subsystem, like a driver, that needs its
fibers or ISRs to communicate with tasks to define a set of such packets to
be passed to the isr_xxx routines, such as isr_sem_give(), that, in turn,
needs to obtain command packets to interact with the microkernel.

Each command packet set is created in global memory using ...
    CMD_PKT_SET_INSTANCE(set variable name, # of command packets in the set);

Once created, the command packet set is accessed using ...
    CMD_PKT_SET(set variable name).<member field name>

A command packet set is a simple ring buffer.  No error checking is performed
when a command packet is retrieved from the set.  Each obtained command packet
is implicitly released once the command packet has been processed.  Thus, it is
important that each command packet be processed in a near-FIFO order to prevent
corruption of command packets that are already in use.  To this end, drivers
that have an ISR component should use their own command packet set.
*/

/* includes */

#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <microkernel/k_struct.h>
#include <microkernel/cmdPkt.h>
#include <minik.h>
#include <sections.h>

/*******************************************************************************
 *
 * _cmd_pkt_get - get the next command packet
 *
 * This routine gets the next command packet from the specified set.
 *
 * RETURNS: pointer to the command packet
 */

cmdPkt_t *_cmd_pkt_get(
	struct cmd_pkt_set *pSet /* ptr to set of command packets */
	)
{
	uint32_t index; /* index into command packet array */
	int key;	/* interrupt lock level */

	key = irq_lock_inline();
	index = pSet->index;
	pSet->index++;
	if (pSet->index >= pSet->nPkts)
		pSet->index = 0;
	irq_unlock_inline(key);

	return &pSet->cmdPkt[index];
}

/*******************************************************************************
*
* _k_task_call - send command packet to be processed by K_swapper
*
* RETURNS: N/A
*/

void _k_task_call(struct k_args *cmdpacket)
{
	cmdpacket->alloc = false;
	_k_current_task->Args = cmdpacket;
	nano_task_stack_push(&_k_command_stack, (uint32_t)cmdpacket);
}
