/* memory map kernel services */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

#include "minik.h" /* _k_mem_map_list, _k_mem_map_count */
#include <sections.h>

/*******************************************************************************
*
* _k_mem_map_init - initialize kernel memory map subsystem
*
* Perform any initialization of memory maps that wasn't done at build time.
*
* RETURNS: N/A
*/

void _k_mem_map_init(void)
{
	int i, j, w;
	struct map_struct *M;

	for (i = 0, M = _k_mem_map_list; i < _k_mem_map_count; i++, M++) {
		char *p;
		char *q;

		M->Waiters = NULL;

		w = OCTET_TO_SIZEOFUNIT(M->Esize);

		p = M->Base;
		q = NULL;

		for (j = 0; j < M->Nelms; j++) {
			*(char **)p = q;
			q = p;
			p += w;
		}
		M->Free = q;
		M->Nused = 0;
		M->Hmark = 0;
		M->Count = 0;
	}
}

/*******************************************************************************
*
* _k_mem_map_alloc_timeout - finish handling a memory map block request that timed out
*
* RETURNS: N/A
*/

void _k_mem_map_alloc_timeout(struct k_args *A)
{
	delist_timeout(A->Time.timer);
	REMOVE_ELM(A);
	A->Time.rcode = RC_TIME;
	reset_state_bit(A->Ctxt.proc, TF_ALLO);
}

/*******************************************************************************
*
* _k_mem_map_alloc - perform allocate memory map block request
*
* RETURNS: N/A
*/

void _k_mem_map_alloc(struct k_args *A)
{
	struct map_struct *M = _k_mem_map_list + OBJ_INDEX(A->Args.a1.mmap);

	if (M->Free != NULL) {
		*(A->Args.a1.mptr) = M->Free;
		M->Free = *(char **)(M->Free);
		M->Nused++;

#ifdef CONFIG_OBJECT_MONITOR
		M->Count++;
		if (M->Hmark < M->Nused)
			M->Hmark = M->Nused;
#endif

		A->Time.rcode = RC_OK;
		return;
	}

	*(A->Args.a1.mptr) = NULL;

	if (likely(A->Time.ticks != TICKS_NONE)) {
		A->Prio = _k_current_task->Prio;
		A->Ctxt.proc = _k_current_task;
		set_state_bit(_k_current_task, TF_ALLO);
		INSERT_ELM(M->Waiters, A);
#ifdef CONFIG_SYS_CLOCK_EXISTS
		if (A->Time.ticks == TICKS_UNLIMITED)
			A->Time.timer = NULL;
		else {
			A->Comm = ALLOCTMO;
			enlist_timeout(A);
		}
#endif
	} else
		A->Time.rcode = RC_FAIL;
}

/*******************************************************************************
*
* _task_mem_map_alloc - allocate memory map block request
*
* This routine is used to request a block of memory from the memory map.
*
* RETURNS: RC_OK, RC_FAIL, RC_TIME on success, error, timeout respectively
*/

int _task_mem_map_alloc(kmemory_map_t mmap,  /* memory map from which to request block */
		    void **mptr, /* pointer to requested block of memory */
		    int32_t time /* maximum # of ticks for which to wait */
		    )
{
	struct k_args A;

	A.Comm = ALLOC;
	A.Time.ticks = time;
	A.Args.a1.mmap = mmap;
	A.Args.a1.mptr = mptr;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}

/*******************************************************************************
*
* _k_mem_map_dealloc - perform return memory map block request
*
* RETURNS: N/A
*/

void _k_mem_map_dealloc(struct k_args *A)
{
	struct map_struct *M = _k_mem_map_list + OBJ_INDEX(A->Args.a1.mmap);
	struct k_args *X;

	**(char ***)(A->Args.a1.mptr) = M->Free;
	M->Free = *(char **)(A->Args.a1.mptr);
	*(A->Args.a1.mptr) = NULL;

	X = M->Waiters;
	if (X) {
		M->Waiters = X->Forw;
		*(X->Args.a1.mptr) = M->Free;
		M->Free = *(char **)(M->Free);

#ifdef CONFIG_SYS_CLOCK_EXISTS
		if (X->Time.timer) {
			delist_timeout(X->Time.timer);
			X->Comm = NOP;
		}
#endif
		X->Time.rcode = RC_OK;
		reset_state_bit(X->Ctxt.proc, TF_ALLO);

#ifdef CONFIG_OBJECT_MONITOR
		M->Count++;
#endif

		return;
	}
	M->Nused--;
}

/*******************************************************************************
*
* _task_mem_map_free - return memory map block request
*
* This routine returns a block to the specified memory map. If a higher
* priority task is waiting for a block from the same map a task switch
* takes place.
*
* RETURNS: N/A
*/

void _task_mem_map_free(kmemory_map_t mmap, /* memory map */
		      void **mptr /* block of memory to return */
		      )
{
	struct k_args A;

	A.Comm = DEALLOC;
	A.Args.a1.mmap = mmap;
	A.Args.a1.mptr = mptr;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* task_mem_map_used_get - read the number of used blocks in a memory map
*
* This routine returns the number of blocks in use for the memory map.
*
* RETURNS: number of used blocks
*/

int task_mem_map_used_get(kmemory_map_t mmap /* memory map */
		 )
{
	return _k_mem_map_list[OBJ_INDEX(mmap)].Nused;
}
