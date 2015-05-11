/* utils.h - utility functions used by latency measurement */

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
 * This file contains function declarations, macroses and inline functions
 * used in latency measurement.
 */

#define INT_IMM8_OFFSET   1
#define IRQ_PRIORITY      3

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#include <stdio.h>
#include "timestamp.h"
extern char tmpString[];
extern int errorCount;

#define TMP_STRING_SIZE  100

#define PRINT(fmt, ...) printk (fmt, ##__VA_ARGS__)
#define PRINTF(fmt, ...) printf (fmt, ##__VA_ARGS__)

#define PRINT_FORMAT(fmt, ...)                                       \
	do                                                               \
		{                                                            \
		snprintf (tmpString, TMP_STRING_SIZE, fmt, ##__VA_ARGS__);   \
		PRINTF ("|%-77s|\n", tmpString);                             \
		}                                                            \
	while (0)

/*******************************************************************************
 *
 * printDashLine - print dash line
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

static inline void printDashLine (void)
	{
	PRINTF ("|-----------------------------------------------------------------"
	    "------------|\n");
	}

#define PRINT_END_BANNER()                                                     \
	PRINTF ("|                                    E N D                       "\
	    "             |\n");                                               \
	printDashLine ();

#define PRINT_NANO_BANNER()                                                    \
	printDashLine ();                                                          \
	PRINTF ("|                    VxMicro Nanokernel Latency Benchmark        "\
	    "             |\n");                                               \
	printDashLine ();

#define PRINT_MICRO_BANNER()                                                   \
	printDashLine ();                                                          \
	PRINTF ("|                    VxMicro Microkernel Latency Benchmark       "\
	    "             |\n");                                               \
	printDashLine ();

#define PRINT_TIME_BANNER()                                                    \
	PRINT_FORMAT("  tcs = timer clock cycles: 1 tcs is %lu nsec", SYS_CLOCK_HW_CYCLES_TO_NS(1));\
	printDashLine ();

#define PRINT_OVERFLOW_ERROR() 	                                               \
	PRINT_FORMAT (" Error: tick occured")

#else
#error PRINTK needs to be enabled in VxMicro configuration
#endif

void raiseIntFunc (void);
extern void raiseInt(uint8_t id);

/* test the interrupt latency */
int nanoIntLatency (void);
int nanoIntToFiber (void);
int nanoIntToFiberSem (void);
int nanoCtxSwitch (void);
int nanoIntLockUnlock (void);

/* pointer to the ISR */
typedef void (*ptestIsr) (void *unused);

/*******************************************************************************
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

int initSwInterrupt (ptestIsr pIsrHdlr);

/*******************************************************************************
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

void setSwInterrupt (ptestIsr pIsrHdlr);
