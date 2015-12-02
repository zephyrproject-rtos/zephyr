/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

/* @file
 * @brief Memory map kernel services.
 */

#include <micro_private.h>
#include <sections.h>

#include <microkernel/memory_map.h>

extern kmemory_map_t _k_mem_map_ptr_start[];
extern kmemory_map_t _k_mem_map_ptr_end[];

/**
 * @brief Initialize kernel memory map subsystem
 *
 * Perform any initialization of memory maps that wasn't done at build time.
 */
void _k_mem_map_init(void)
{
	int j, w;
	kmemory_map_t *id;
	struct _k_mem_map_struct *M;

	for (id = _k_mem_map_ptr_start; id < _k_mem_map_ptr_end; id++) {
		char *p;
		char *q;

		M = (struct _k_mem_map_struct *)(*id);

		M->waiters = NULL;

		w = OCTET_TO_SIZEOFUNIT(M->element_size);

		p = M->base;
		q = NULL;

		for (j = 0; j < M->Nelms; j++) {
			*(char **)p = q;
			q = p;
			p += w;
		}
		M->free = q;
		M->num_used = 0;
		M->high_watermark = 0;
		M->count = 0;
	}
}

/**
 * @brief Finish handling a memory map block allocation request that timed out
 */
void _k_mem_map_alloc_timeout(struct k_args *A)
{
	_k_timeout_free(A->Time.timer);
	REMOVE_ELM(A);
	A->Time.rcode = RC_TIME;
	_k_state_bit_reset(A->Ctxt.task, TF_ALLO);
}

/**
 * @brief Handle a request to allocate a memory map block
 */
void _k_mem_map_alloc(struct k_args *A)
{
	struct _k_mem_map_struct *M =
	    (struct _k_mem_map_struct *)(A->args.a1.mmap);

	if (M->free != NULL) {
		*(A->args.a1.mptr) = M->free;
		M->free = *(char **)(M->free);
		M->num_used++;

#ifdef CONFIG_OBJECT_MONITOR
		M->count++;
		if (M->high_watermark < M->num_used)
			M->high_watermark = M->num_used;
#endif

		A->Time.rcode = RC_OK;
		return;
	}

	*(A->args.a1.mptr) = NULL;

	if (likely(A->Time.ticks != TICKS_NONE)) {
		A->priority = _k_current_task->priority;
		A->Ctxt.task = _k_current_task;
		_k_state_bit_set(_k_current_task, TF_ALLO);
		INSERT_ELM(M->waiters, A);
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

int task_mem_map_alloc(kmemory_map_t mmap, void **mptr, int32_t timeout)
{
	struct k_args A;

	A.Comm = _K_SVC_MEM_MAP_ALLOC;
	A.Time.ticks = timeout;
	A.args.a1.mmap = mmap;
	A.args.a1.mptr = mptr;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}

/**
 * @brief Handle a request to free a memory map block
 *
 * Give block to a waiting task, if there is one.
 */
void _k_mem_map_dealloc(struct k_args *A)
{
	struct _k_mem_map_struct *M =
	    (struct _k_mem_map_struct *)(A->args.a1.mmap);
	struct k_args *X;

	**(char ***)(A->args.a1.mptr) = M->free;
	M->free = *(char **)(A->args.a1.mptr);
	*(A->args.a1.mptr) = NULL;

	X = M->waiters;
	if (X) {
		M->waiters = X->next;
		*(X->args.a1.mptr) = M->free;
		M->free = *(char **)(M->free);

#ifdef CONFIG_SYS_CLOCK_EXISTS
		if (X->Time.timer) {
			_k_timeout_free(X->Time.timer);
			X->Comm = _K_SVC_NOP;
		}
#endif
		X->Time.rcode = RC_OK;
		_k_state_bit_reset(X->Ctxt.task, TF_ALLO);

#ifdef CONFIG_OBJECT_MONITOR
		M->count++;
#endif

		return;
	}
	M->num_used--;
}

void _task_mem_map_free(kmemory_map_t mmap, void **mptr)
{
	struct k_args A;

	A.Comm = _K_SVC_MEM_MAP_DEALLOC;
	A.args.a1.mmap = mmap;
	A.args.a1.mptr = mptr;
	KERNEL_ENTRY(&A);
}

int task_mem_map_used_get(kmemory_map_t mmap)
{
	struct _k_mem_map_struct *M = (struct _k_mem_map_struct *)mmap;

	return M->num_used;
}
