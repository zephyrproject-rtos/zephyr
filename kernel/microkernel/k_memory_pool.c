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

#include <microkernel.h>
#include <micro_private.h>
#include <toolchain.h>
#include <sections.h>

/* Auto-Defrag settings */

#define AD_NONE 0
#define AD_BEFORE_SEARCH4BIGGERBLOCK 1
#define AD_AFTER_SEARCH4BIGGERBLOCK 2

#define AUTODEFRAG AD_AFTER_SEARCH4BIGGERBLOCK

/**
 *
 * @brief Initialize kernel memory pool subsystem
 *
 * Perform any initialization of memory pool that wasn't done at build time.
 *
 * @return N/A
 */
void _k_mem_pool_init(void)
{
	int i, j, k;
	struct pool_struct *P;
	char *memptr;

	/* for all pools initialise largest blocks */
	for (i = 0, P = _k_mem_pool_list; i < _k_mem_pool_count; i++, P++) {
		int remaining = P->nr_of_maxblocks;
		int t = 0;

		/* initialise block-arrays */
		for (k = 0; k < P->nr_of_frags; k++) {
			P->frag_tab[k].count = 0;
			for (j = 0; j < P->frag_tab[k].nr_of_entries; j++) {
				P->frag_tab[k].blocktable[j].mem_blocks = NULL;
				P->frag_tab[k].blocktable[j].mem_status =
					0xF; /* all blocks in use */
			}
		}

		memptr = P->bufblock;

		while (remaining >= 4) { /* while not all blocks allocated */
			/* - put pointer in table - */
			P->frag_tab[0].blocktable[t].mem_blocks = memptr;
			P->frag_tab[0].blocktable[t].mem_status =
				0; /* all blocks initial free */

			t++; /* next entry in table  */
			remaining = remaining - 4;
			memptr +=
				OCTET_TO_SIZEOFUNIT(P->frag_tab[0].block_size) *
				4;
		}

		if (remaining != 0) {

			/* - put pointer in table - */
			P->frag_tab[0].blocktable[t].mem_blocks = memptr;
			P->frag_tab[0].blocktable[t].mem_status =
				(0xF << remaining) &
				0xF; /* mark unaccessible blocks as used */
		}
	}
}

/**
 *
 * @brief ???
 *
 * marks ptr as free block in the given list [MYSTERIOUS LEGACY COMMENT]
 *
 * @return N/A
 */
static void search_bp(char *ptr, struct pool_struct *P, int index)
{
	int i = 0, j, block_found = 0;

	while ((P->frag_tab[index].blocktable[i].mem_blocks != NULL) &&
	       (!block_found)) {
		for (j = 0; j < 4; j++) {
			if (ptr ==
			    ((P->frag_tab[index].blocktable[i].mem_blocks) +
			     (OCTET_TO_SIZEOFUNIT(
				     j * P->frag_tab[index].block_size)))) {
				/* then we've found the right pointer */
				block_found = 1;

				/* so free it */
				P->frag_tab[index].blocktable[i].mem_status =
					P->frag_tab[index]
						.blocktable[i]
						.mem_status &
					(~(1 << j));
			}
		}
		i++;
	}
}

/**
 *
 * @brief Defragmentation algorithm for memory pool
 *
 * @return N/A
 */
static void defrag(struct pool_struct *P,
				  int ifraglevel_start,
				  int ifraglevel_stop)
{
	int i, j, k;

	j = ifraglevel_start;

	while (j > ifraglevel_stop) {
		i = 0;
		while (P->frag_tab[j].blocktable[i].mem_blocks != NULL) {
			if ((P->frag_tab[j].blocktable[i].mem_status & 0xF) ==
			    0) { /* blocks for defragmenting */
				search_bp(
					P->frag_tab[j].blocktable[i].mem_blocks,
					P,
					j - 1);

				/* remove blocks & compact list */
				k = i;
				while ((P->frag_tab[j]
						.blocktable[k]
						.mem_blocks != NULL) &&
				       (k <
					(P->frag_tab[j].nr_of_entries - 1))) {
					P->frag_tab[j]
						.blocktable[k]
						.mem_blocks =
						P->frag_tab[j]
							.blocktable[k + 1]
							.mem_blocks;
					P->frag_tab[j]
						.blocktable[k]
						.mem_status =
						P->frag_tab[j]
							.blocktable[k + 1]
							.mem_status;
					k++;
				}
				P->frag_tab[j].blocktable[k].mem_blocks = NULL;
				P->frag_tab[j].blocktable[k].mem_status = 0;
			} else {
				i++; /* take next block */
			}
		}
		j--;
	}
}

/**
 *
 * @brief Perform defragment memory pool request
 *
 * @return N/A
 */
void _k_defrag(struct k_args *A)
{
	struct pool_struct *P = _k_mem_pool_list + OBJ_INDEX(A->args.p1.pool_id);

	defrag(P,
	       P->nr_of_frags - 1, /* start from smallest blocks */
	       0		   /* and defragment till fragment level 0 */
	       );

	/* reschedule waiters */

	if (
		    P->waiters) {
		struct k_args *NewGet;

		/*
		 * get new command packet that calls the function
		 * that reallocate blocks for the waiting tasks
		 */
		GETARGS(NewGet);
		*NewGet = *A;
		NewGet->Comm = _K_SVC_BLOCK_WAITERS_GET;
		TO_ALIST(&_k_command_stack, NewGet); /*push on command stack */
	}
}


void task_mem_pool_defragment(kmemory_pool_t Pid)
{
	struct k_args A;

	A.Comm = _K_SVC_DEFRAG;
	A.args.p1.pool_id = Pid;
	KERNEL_ENTRY(&A);
}

/**
 *
 * @brief Allocate block using specified fragmentation level
 *
 * This routine attempts to allocate a free block. [NEED TO EXPAND THIS]
 *
 * @return pointer to allocated block, or NULL if none available
 */
static char *search_block_on_frag_level(struct pool_block *pfraglevelinfo,
						    int *piblockindex)
{
	int i, status, end_of_list;
	char *found;

	i = 0;
	end_of_list = 0;
	found = NULL;

	while (!end_of_list) {/* search list */

		if (pfraglevelinfo->blocktable[i].mem_blocks == NULL) {
			end_of_list = 1;
		} else {
			status = pfraglevelinfo->blocktable[i].mem_status;
			if (!(status & 1)) {
				found = pfraglevelinfo->blocktable[i]
						.mem_blocks;
				pfraglevelinfo->blocktable[i].mem_status |=
					1; /* set status used */
#ifdef CONFIG_OBJECT_MONITOR
				pfraglevelinfo->count++;
#endif
				break;
			} else if (!(status & 2)) {
				found = pfraglevelinfo->blocktable[i]
						.mem_blocks +
					(OCTET_TO_SIZEOFUNIT(
						pfraglevelinfo->block_size));
				pfraglevelinfo->blocktable[i].mem_status |=
					2; /* set status used */
#ifdef CONFIG_OBJECT_MONITOR
				pfraglevelinfo->count++;
#endif
				break;
			} else if (!(status & 4)) {
				found = pfraglevelinfo->blocktable[i]
						.mem_blocks +
					(OCTET_TO_SIZEOFUNIT(
						2 *
						pfraglevelinfo->block_size));
				pfraglevelinfo->blocktable[i].mem_status |=
					4; /* set status used */
#ifdef CONFIG_OBJECT_MONITOR
				pfraglevelinfo->count++;
#endif
				break;
			} else if (!(status & 8)) {
				found = pfraglevelinfo->blocktable[i]
						.mem_blocks +
					(OCTET_TO_SIZEOFUNIT(
						3 *
						pfraglevelinfo->block_size));
				pfraglevelinfo->blocktable[i].mem_status |=
					8; /* set status used */
#ifdef CONFIG_OBJECT_MONITOR
				pfraglevelinfo->count++;
#endif
				break;
			}

			i++;
			if (i >= pfraglevelinfo->nr_of_entries) {
				end_of_list = 1;
			}
		}
	} /* while (...) */

	*piblockindex = i;
	return found;
}

/**
 *
 * @brief Recursively get a block, doing fragmentation if necessary
 *
 * @file
 * @brief Memory pool kernel services
 *
 *
 * not implemented: check if we go below the minimal number of blocks with
 * the maximum size
 *
 * @return pointer to allocated block, or NULL if none available
 */
static char *get_block_recusive(struct pool_struct *P, int index, int startindex)
{
	int i;
	char *found, *larger_block;
	struct pool_block *fr_table;

	if (index < 0) {
		return NULL; /* no more free blocks in pool */
	}

	fr_table = P->frag_tab;
	i = 0;

	/* let's inspect the fragmentation level <index> */
	found = search_block_on_frag_level(&(fr_table[index]), &i);
	if (found != NULL) {
		return found;
	}

#if AUTODEFRAG == AD_BEFORE_SEARCH4BIGGERBLOCK
	/* maybe a partial defrag will free up a block? */
	if (index == startindex) { /* only for the original request */
		defrag(P,
		       P->nr_of_frags - 1, /* start from the smallest blocks */
		       startindex); /* but only until the requested blocksize
				     * (fragmentation level) !!
				     */

		found = search_block_on_frag_level(&(fr_table[index]), &i);
		if (found != NULL) {
			return found;
		}
	}
	/* partial defrag did not release a block of level <index> */
#endif

	/* end of list and i is index of first empty entry in blocktable */
	{
		/* get a block of one size larger */
		larger_block = get_block_recusive(
			P, index - 1, startindex);
	}

	if (larger_block != NULL) {
		/* insert 4 found blocks and mark one block as used */
		fr_table[index].blocktable[i].mem_blocks = larger_block;
		fr_table[index].blocktable[i].mem_status = 1;
#ifdef CONFIG_OBJECT_MONITOR
		fr_table[index].count++;
#endif
		return larger_block; /* return marked block */
	}

	/* trying to get a larger block did not succeed */

#if AUTODEFRAG == AD_AFTER_SEARCH4BIGGERBLOCK
	/* maybe a partial defrag will free up a block? */
	if (index == startindex) { /* only for the original request */
		defrag(P,
		       P->nr_of_frags - 1, /* start from the smallest blocks */
		       startindex); /* but only until the requested blocksize
				     * (fragmentation level) !!
				     */

		found = search_block_on_frag_level(&(fr_table[index]), &i);
		if (found != NULL) {
			return found;
		}
	}
   /* partial defrag did not release a block of level <index> */
#endif

	return NULL; /* now we have to report failure: no block available */
}

/**
 *
 * @brief Examine tasks that are waiting for memory pool blocks
 *
 * This routine attempts to satisfy any incomplete block allocation requests for
 * the specified memory pool. It can be invoked either by the explicit freeing
 * of a used block or as a result of defragmenting the pool  (which may create
 * one or more new, larger blocks).
 *
 * @return N/A
 */
void _k_block_waiters_get(struct k_args *A)
{
	struct pool_struct *P = _k_mem_pool_list + OBJ_INDEX(A->args.p1.pool_id);
	char *found_block;
	struct k_args *curr_task, *prev_task;
	int start_size, offset;

	curr_task = P->waiters;
	/* forw is first field in struct */
	prev_task = (struct k_args *)&(P->waiters);

	/* loop all waiters */
	while (curr_task != NULL) {

		/* calculate size & offset */
		start_size = P->minblock_size;
		offset = P->nr_of_frags - 1;
		while (curr_task->args.p1.req_size > start_size) {
			start_size = start_size << 2; /* try one larger */
			offset--;
		}

		/* allocate block */
		found_block = get_block_recusive(
			P, offset, offset); /* allocate and fragment blocks */

		/* if success : remove task from list and reschedule */
		if (found_block != NULL) {
			/* return found block */
			curr_task->args.p1.rep_poolptr = found_block;
			curr_task->args.p1.rep_dataptr = found_block;

			/* reschedule task */

#ifdef CONFIG_SYS_CLOCK_EXISTS
			if (curr_task->Time.timer) {
				_k_timeout_free(curr_task->Time.timer);
			}
#endif
			curr_task->Time.rcode = RC_OK;
			_k_state_bit_reset(curr_task->Ctxt.task, TF_GTBL);

			/* remove from list */
			prev_task->next = curr_task->next;

			/* and get next task */
			curr_task = curr_task->next;

		} else {
			/* else just get next task */
			prev_task = curr_task;
			curr_task = curr_task->next;
		}
	}

	/* put used command packet on the empty packet list */
	FREEARGS(A);
}

/**
 *
 * @brief Finish handling an allocate block request that timed out
 *
 * @return N/A
 */
void _k_mem_pool_block_get_timeout_handle(struct k_args *A)
{
	_k_timeout_free(A->Time.timer);
	REMOVE_ELM(A);
	A->Time.rcode = RC_TIME;
	_k_state_bit_reset(A->Ctxt.task, TF_GTBL);
}

/**
 *
 * @brief Perform allocate memory pool block request
 *
 * @return N/A
 */
void _k_mem_pool_block_get(struct k_args *A)
{
	struct pool_struct *P = _k_mem_pool_list + OBJ_INDEX(A->args.p1.pool_id);
	char *found_block;

	int start_size;
	int offset;

	/* calculate start size */
	start_size = P->minblock_size;
	offset = P->nr_of_frags - 1;

	while (A->args.p1.req_size > start_size) {
		start_size = start_size << 2; /*try one larger */
		offset--;
	}

	/* startsize==the available size that can contain the requeste block size */
	/* offset: index in fragtable of the minimal blocksize */

	found_block =
		get_block_recusive(P, offset, offset); /* allocate and fragment blocks */

	if (found_block != NULL) {
		A->args.p1.rep_poolptr = found_block;
		A->args.p1.rep_dataptr = found_block;
		A->Time.rcode = RC_OK;
		return; /* return found block */
	}

	if (likely(
		    (A->Time.ticks != TICKS_NONE) &&
		    (A->args.p1.req_size <=
		     P->maxblock_size))) {/* timeout?  but not block to large */
		A->priority = _k_current_task->priority;
		A->Ctxt.task = _k_current_task;
		_k_state_bit_set(_k_current_task, TF_GTBL); /* extra new statebit */

		/* INSERT_ELM (P->frag_tab[offset].waiters, A); */
		INSERT_ELM(P->waiters, A);

#ifdef CONFIG_SYS_CLOCK_EXISTS
		if (A->Time.ticks == TICKS_UNLIMITED) {
			A->Time.timer = NULL;
		} else {
			A->Comm = _K_SVC_MEM_POOL_BLOCK_GET_TIMEOUT_HANDLE;
			_k_timeout_alloc(A);
		}
#endif
	} else {
		A->Time.rcode =
			RC_FAIL; /* no blocks available or block too large */
	}
}

/**
 *
 * @brief Allocate memory pool block request
 *
 * This routine allocates a free block from the specified memory pool, ensuring
 * that its size is at least as big as the size requested (in bytes).
 *
 * @param blockptr poitner to requested block
 * @param pool_id pool from which to get block
 * @param reqsize requested block size
 * @param time maximum number of ticks to wait
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
int task_mem_pool_alloc(struct k_block *blockptr, kmemory_pool_t pool_id,
			 int reqsize, int32_t timeout)
{
	struct k_args A;

	A.Comm = _K_SVC_MEM_POOL_BLOCK_GET;
	A.Time.ticks = timeout;
	A.args.p1.pool_id = pool_id;
	A.args.p1.req_size = reqsize;

	KERNEL_ENTRY(&A);

	blockptr->pool_id = pool_id;
	blockptr->address_in_pool = A.args.p1.rep_poolptr;
	blockptr->pointer_to_data = A.args.p1.rep_dataptr;
	blockptr->req_size = reqsize;

	return A.Time.rcode;
}

/**
 *
 * @brief Perform return memory pool block request
 *
 * Marks a block belonging to a pool as free; if there are waiters that can use
 * the the block it is passed to a waiting task.
 *
 * @return N/A
 */
void _k_mem_pool_block_release(struct k_args *A)
{
	struct pool_struct *P;
	struct pool_block *block;
	struct block_stat *blockstat;
	int Pid;
	int start_size, offset;
	int i, j;

	Pid = A->args.p1.pool_id;


	P = _k_mem_pool_list + OBJ_INDEX(Pid);

	/* calculate size */
	start_size = P->minblock_size;
	offset = P->nr_of_frags - 1;

	while (A->args.p1.req_size > start_size) {
		start_size = start_size << 2; /* try one larger */
		offset--;
	}

	/* startsize==the available size that contains the requested block size */
	/* offset: index in fragtable of the block */

	j = 0;
	block = P->frag_tab + offset;

	while ((j < block->nr_of_entries) &&
	       ((blockstat = block->blocktable + j)->mem_blocks != 0)) {
		for (i = 0; i < 4; i++) {
			if (A->args.p1.rep_poolptr ==
			    (blockstat->mem_blocks +
			     (OCTET_TO_SIZEOFUNIT(i * block->block_size)))) {
				/* we've found the right pointer, so free it */
				blockstat->mem_status &= ~(1 << i);

				/* waiters? */
				if (P->waiters != NULL) {
					struct k_args *NewGet;
					/*
					 * get new command packet that calls
					 * the function that reallocate blocks
					 * for the waiting tasks
					 */
					GETARGS(NewGet);
					*NewGet = *A;
					NewGet->Comm = _K_SVC_BLOCK_WAITERS_GET;
					/* push on command stack */
					TO_ALIST(&_k_command_stack, NewGet);
				}
				if (A->alloc) {
					FREEARGS(A);
				}
				return;
			}
		}
		j++;
	}
}

void task_mem_pool_free(struct k_block *blockptr)
{
	struct k_args A;

	A.Comm = _K_SVC_MEM_POOL_BLOCK_RELEASE;
	A.args.p1.pool_id = blockptr->pool_id;
	A.args.p1.req_size = blockptr->req_size;
	A.args.p1.rep_poolptr = blockptr->address_in_pool;
	A.args.p1.rep_dataptr = blockptr->pointer_to_data;
	KERNEL_ENTRY(&A);
}
