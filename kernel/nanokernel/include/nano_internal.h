/* architecture-independent private nanokernel APIs */

/*
 * Copyright (c) 2010-2012, 2014-2015 Wind River Systems, Inc.
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
This file contains private nanokernel APIs that are not architecture-specific.
*/

#ifndef _NANO_INTERNAL__H_
#define _NANO_INTERNAL__H_

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/* context entry point declarations */

typedef void *_ContextArg;
typedef void (*_ContextEntry)(_ContextArg arg1,
							  _ContextArg arg2,
							  _ContextArg arg3);

extern void _context_entry(_ContextEntry,
			     _ContextArg,
			     _ContextArg,
			     _ContextArg);

extern void _NewContext(char *pStack, unsigned stackSize,
						_ContextEntry pEntry, _ContextArg arg1,
						_ContextArg arg2, _ContextArg arg3,
						int prio, unsigned options);

/* context switching and scheduling-related routines */

extern void _nano_fiber_schedule(tCCS *ccs);
extern void _nano_fiber_swap(void);

extern unsigned int _Swap(unsigned int);

/* set and clear essential fiber/task flag */

extern void _context_essential_set(void);
extern void _context_essential_clear(void);

/* clean up when a context is aborted */

#if defined(CONFIG_CONTEXT_MONITOR)
extern void _context_exit(tCCS *ccs);
#else
#define _context_exit(ccs) \
	do {/* nothing */    \
	} while (0)
#endif /* CONFIG_CONTEXT_MONITOR */

/* special nanokernel object APIs */

struct nano_lifo;

extern void *_nano_fiber_lifo_get_panic(struct nano_lifo *lifo);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* _NANO_INTERNAL__H_ */
