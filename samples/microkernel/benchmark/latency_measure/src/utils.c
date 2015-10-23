/* utils.c - utility functions used by latency measurement */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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
 * DESCRIPTION
 * this file contains commonly used functions
 */

#include <zephyr.h>

#include "timestamp.h"
#include "utils.h"


static uint8_t vector; /* the interrupt vector we allocate */

/* current pointer to the ISR */
static ptestIsr pcurrIsrFunc;

/* scratchpad for the string used to print on console */
char tmpString[TMP_STRING_SIZE];

/**
 *
 * @brief Initialize the interrupt handler
 *
 * Function initializes the interrupt handler with the pointer to the function
 * provided as an argument. It sets up allocated interrupt vector, pointer to
 * the current interrupt service routine and stub code memory block.
 *
 * @return the allocated interrupt vector
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
 * @brief Set the new ISR for software interrupt
 *
 * The routine shange the ISR for the fully connected interrupt to the routine
 * provided. This routine can be invoked only after the interrupt has been
 * initialized and connected by initSwInterrupt.
 *
 * @return N/A
 */
void setSwInterrupt(ptestIsr pIsrHdlr)
{
	extern void _irq_handler_set(unsigned int irq, void (*old)(void *arg),
									void (*new)(void *arg), void *arg);
	_irq_handler_set(vector, pcurrIsrFunc, pIsrHdlr, (void *)0);
	pcurrIsrFunc = pIsrHdlr;
}

/**
 *
 * @brief Generate a software interrupt
 *
 * This routine will call one of the generate SW interrupt functions based upon
 * the current vector assigned by the initSwInterrupt() function.
 * This routine can be invoked only after the interrupt has been
 * initialized and connected by initSwInterrupt.
 *
 * @return N/A
 */
void raiseIntFunc(void)
{
	raiseInt(vector);
}
