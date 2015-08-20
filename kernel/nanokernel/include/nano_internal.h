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

/* helper type alias for thread control structure */

typedef struct tcs tTCS;

/* thread entry point declarations */

typedef void *_thread_arg_t;
typedef void (*_thread_entry_t)(_thread_arg_t arg1,
							  _thread_arg_t arg2,
							  _thread_arg_t arg3);

extern void _thread_entry(_thread_entry_t,
			     _thread_arg_t,
			     _thread_arg_t,
			     _thread_arg_t);

extern void _new_thread(char *pStack, unsigned stackSize,
						_thread_entry_t pEntry, _thread_arg_t arg1,
						_thread_arg_t arg2, _thread_arg_t arg3,
						int prio, unsigned options);

/* context switching and scheduling-related routines */

extern void _nano_fiber_schedule(struct tcs *tcs);
extern void _nano_fiber_swap(void);

extern unsigned int _Swap(unsigned int);

/* set and clear essential fiber/task flag */

extern void _thread_essential_set(void);
extern void _thread_essential_clear(void);

/* clean up when a thread is aborted */

#if defined(CONFIG_THREAD_MONITOR)
extern void _thread_exit(struct tcs *tcs);
#else
#define _thread_exit(tcs) \
	do {/* nothing */    \
	} while (0)
#endif /* CONFIG_THREAD_MONITOR */

/* special nanokernel object APIs */

struct nano_lifo;

extern void *_nano_fiber_lifo_get_panic(struct nano_lifo *lifo);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* _NANO_INTERNAL__H_ */
