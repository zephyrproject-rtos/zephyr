/* utils.c - utility functions used by latency measurement */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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
 * DESCRIPTION
 * this file contains commonly used functions
 */

#include "timestamp.h"
#include "utils.h"

#ifdef CONFIG_NANOKERNEL

#elif defined(CONFIG_MICROKERNEL)

#include <microkernel.h>

#endif /*  CONFIG_NANOKERNEL */

#include <arch/cpu.h>

static uint8_t vector; /* the interrupt vector we allocate */

/* current pointer to the ISR */
static ptestIsr pcurrIsrFunc;

/* scratchpad for the string used to print on console */
char tmpString[TMP_STRING_SIZE];

/**
 *
 * initSwInterrupt - initialize the interrupt handler
 *
 * Function initializes the interrupt handler with the pointer to the function
 * provided as an argument. It sets up allocated interrupt vector, pointer to
 * the current interrupt service routine and stub code memory block.
 *
 * RETURNS: the allocated interrupt vector
 *
 * \NOMANUAL
 */

int initSwInterrupt(ptestIsr pIsrHdlr)
{
	vector = irq_connect(NANO_SOFT_IRQ, IRQ_PRIORITY, pIsrHdlr,
			     (void *) 0);
	pcurrIsrFunc = pIsrHdlr;

	return vector;
}

/**
 *
 * setSwInterrupt - set the new ISR for software interrupt
 *
 * The routine shange the ISR for the fully connected interrupt to the routine
 * provided. This routine can be invoked only after the interrupt has been
 * initialized and connected by initSwInterrupt.
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void setSwInterrupt(ptestIsr pIsrHdlr)
{
	irq_handler_set(vector, pcurrIsrFunc, pIsrHdlr, (void *)0);
	pcurrIsrFunc = pIsrHdlr;
}

/**
 *
 * raiseIntFunc - generate a software interrupt
 *
 * This routine will call one of the generate SW interrupt functions based upon
 * the current vector assigned by the initSwInterrupt() function.
 * This routine can be invoked only after the interrupt has been
 * initialized and connected by initSwInterrupt.
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */
void raiseIntFunc(void)
{
	raiseInt(vector);
}
