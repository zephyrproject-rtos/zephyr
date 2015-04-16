/* nanokernel/private.h */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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

#ifndef __NANOPRIVATE_H__
#define __NANOPRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct s_CCS tCCS;

struct nano_sem {
	tCCS *proc;
	int nsig;
};

struct nano_lifo {
	void *list;
	tCCS *proc;
};

struct nano_fifo {
	void *head;
	void *tail;
	int stat;
};

struct nano_stack {
	tCCS *proc;
	uint32_t *base;
	uint32_t *next;
};

struct nano_timer {
	struct nano_timer *link;
	uint32_t ticks;
	struct nano_lifo lifo;
	void *userData;
};

/* context entry point function typedef */

typedef void *_ContextArg;
typedef void (*_ContextEntry)(_ContextArg arg1,
			      _ContextArg arg2,
			      _ContextArg arg3);

/* Private API to set and clear essential fiber/task flag */
extern void _NanoEssentialContextSet(void);
extern void _NanoEssentialContextClear(void);

/* Private API to clean up when a context is aborted */
#if defined(CONFIG_HOST_TOOLS_SUPPORT)
extern void _context_exit(tCCS *ccs);
#else
#define _context_exit(ccs) \
	do {/* nothing */    \
	} while (0)
#endif /* CONFIG_HOST_TOOLS_SUPPORT */

#ifdef __cplusplus
}
#endif

#endif /* __NANOPRIVATE_H__ */
