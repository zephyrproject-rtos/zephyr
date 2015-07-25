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

/* @file
 * @brief Memory map kernel services.
 */

#include <micro_private.h>
#include <sections.h>

/**
 * @brief Initialize kernel memory map subsystem
 *
 * Perform any initialization of memory maps that wasn't done at build time.
 *
 * @return N/A
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

/**
 * @brief Finish handling a memory map block request that timed out
 *
 * @param A Command package with the memory map block request that timed out.
 *
 * @return N/A
 */
void _k_mem_map_alloc_timeout(struct k_args *A)
{
	_k_timeout_free(A->Time.timer);
	REMOVE_ELM(A);
	A->Time.rcode = RC_TIME;
	_k_state_bit_reset(A->Ctxt.proc, TF_ALLO);
}

/**
 * @brief Perform allocate memory map block request
 *
 * @param A Command package with the allocate memory map block request.
 *
 * @return N/A
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
		_k_state_bit_set(_k_current_task, TF_ALLO);
		INSERT_ELM(M->Waiters, A);
#ifdef CONFIG_SYS_CLOCK_EXISTS
		if (A->Time.ticks == TICKS_UNLIMITED)
			A->Time.timer = NULL;
		else {
			A->Comm = _K_SVC_MEM_MAP_ALLOC_TIMEOUT;
			_k_timeout_alloc(A);
		}
#endif
	} else
		A->Time.rcode = RC_FAIL;
}

/**
 * @brief Allocate memory map block request
 *
 * This routine is used to request a block of memory from the memory map.
 *
 * @param mmap Memory map from which to request block.
 * @param mptr Pointer to requested block of memory.
 * @param time Maximum number of ticks for which to wait.
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, error, timeout respectively
 */
int _task_mem_map_alloc(kmemory_map_t mmap, void **mptr, int32_t time)
{
	struct k_args A;

	A.Comm = _K_SVC_MEM_MAP_ALLOC;
	A.Time.ticks = time;
	A.Args.a1.mmap = mmap;
	A.Args.a1.mptr = mptr;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}

/**
 * @brief Perform return memory map block request
 *
 * @param A Command package with the return memory map block request.
 *
 * @return N/A
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
			_k_timeout_free(X->Time.timer);
			X->Comm = _K_SVC_NOP;
		}
#endif
		X->Time.rcode = RC_OK;
		_k_state_bit_reset(X->Ctxt.proc, TF_ALLO);

#ifdef CONFIG_OBJECT_MONITOR
		M->Count++;
#endif

		return;
	}
	M->Nused--;
}

/**
 * @brief Return memory map block request
 *
 * This routine returns a block to the specified memory map. If a higher
 * priority task is waiting for a block from the same map a task switch
 * takes place.
 *
 * @param mmap Memory map.
 * @param mptr Block of memory to return.
 *
 * @return N/A
 */
void _task_mem_map_free(kmemory_map_t mmap, void **mptr)
{
	struct k_args A;

	A.Comm = _K_SVC_MEM_MAP_DEALLOC;
	A.Args.a1.mmap = mmap;
	A.Args.a1.mptr = mptr;
	KERNEL_ENTRY(&A);
}

int task_mem_map_used_get(kmemory_map_t mmap)
{
	return _k_mem_map_list[OBJ_INDEX(mmap)].Nused;
}
