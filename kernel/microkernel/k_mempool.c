/* memory pool kernel services */

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

#include <microkernel.h>
#include <minik.h>
#include <toolchain.h>
#include <sections.h>

/* Auto-Defrag settings */

#define AD_NONE 0
#define AD_BEFORE_SEARCH4BIGGERBLOCK 1
#define AD_AFTER_SEARCH4BIGGERBLOCK 2

#define AUTODEFRAG AD_AFTER_SEARCH4BIGGERBLOCK

/*******************************************************************************
*
* _k_mem_pool_init - initialize kernel memory pool subsystem
*
* Perform any initialization of memory pool that wasn't done at build time.
*
* RETURNS: N/A
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
			P->frag_tab[k].Count = 0;
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

/*******************************************************************************
*
* search_bp - ???
*
* marks ptr as free block in the given list [MYSTERIOUS LEGACY COMMENT]
*
* RETURNS: N/A
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

/*******************************************************************************
*
* defrag - defragmentation algorithm for memory pool
*
* RETURNS: N/A
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

/*******************************************************************************
*
* _k_defrag - perform defragment memory pool request
*
* RETURNS: N/A
*/

void _k_defrag(struct k_args *A)
{
	struct pool_struct *P = _k_mem_pool_list + OBJ_INDEX(A->Args.p1.poolid);

	defrag(P,
	       P->nr_of_frags - 1, /* start from smallest blocks */
	       0		   /* and defragment till fragment level 0 */
	       );

	/* reschedule waiters */

	if (
		    P->Waiters) {
		struct k_args *NewGet;

		/*
		 * get new command packet that calls the function
		 * that reallocate blocks for the waiting tasks
		 */
		GETARGS(NewGet);
		*NewGet = *A;
		NewGet->Comm = GET_BLOCK_WAIT;
		TO_ALIST(&_k_command_stack, NewGet); /*push on command stack */
	}
}

/*******************************************************************************
*
* task_mem_pool_defragment - defragment memory pool request
*
* This routine concatenates unused memory in a memory pool.
*
* RETURNS: N/A
*/

void task_mem_pool_defragment(kmemory_pool_t Pid /* pool to defragment */
		       )
{
	struct k_args A;

	A.Comm = POOL_DEFRAG;
	A.Args.p1.poolid = Pid;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* search_block_on_frag_level - allocate block using specified fragmentation level
*
* This routine attempts to allocate a free block. [NEED TO EXPAND THIS]
*
* RETURNS: pointer to allocated block, or NULL if none available
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
				pfraglevelinfo->Count++;
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
				pfraglevelinfo->Count++;
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
				pfraglevelinfo->Count++;
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
				pfraglevelinfo->Count++;
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

/*******************************************************************************
*
* get_block_recusive - recursively get a block, doing fragmentation if necessary
*
* [NEED A BETTER DESCRIPTION HERE]
*
* not implemented: check if we go below the minimal number of blocks with
* the maximum size
*
* RETURNS: pointer to allocated block, or NULL if none available
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
				       (fragmentation level) !! */

		found = search_block_on_frag_level(&(fr_table[index]), &i);
		if (found != NULL) {
			return found;
		}
	}
	/* partial defrag did not release a block of level <index> */
#endif

	/* end of list and i is index of first empty entry in blocktable */
	{
		larger_block = get_block_recusive(
			P, index - 1, startindex); /* get a block of one size larger */
	}

	if (larger_block != NULL) {
		/* insert 4 found blocks and mark one block as used */
		fr_table[index].blocktable[i].mem_blocks = larger_block;
		fr_table[index].blocktable[i].mem_status = 1;
#ifdef CONFIG_OBJECT_MONITOR
		fr_table[index].Count++;
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
				       (fragmentation level) !! */

		found = search_block_on_frag_level(&(fr_table[index]), &i);
		if (found != NULL) {
			return found;
		}
	}
   /* partial defrag did not release a block of level <index> */
#endif

	return NULL; /* now we have to report failure: no block available */
}

/*******************************************************************************
*
* _k_block_waiters_get - examine tasks that are waiting for memory pool blocks
*
* This routine attempts to satisfy any incomplete block allocation requests for
* the specified memory pool. It can be invoked either by the explicit freeing
* of a used block or as a result of defragmenting the pool  (which may create
* one or more new, larger blocks).
*
* RETURNS: N/A
*/

void _k_block_waiters_get(struct k_args *A)
{
	struct pool_struct *P = _k_mem_pool_list + OBJ_INDEX(A->Args.p1.poolid);
	char *found_block;
	struct k_args *curr_task, *prev_task;
	int start_size, offset;

	curr_task = P->Waiters;
	prev_task = (struct k_args *)&(P->Waiters); /* forw is first field in struct */

	/* loop all waiters */
	while (curr_task != NULL) {

		/* calculate size & offset */
		start_size = P->minblock_size;
		offset = P->nr_of_frags - 1;
		while (curr_task->Args.p1.req_size > start_size) {
			start_size = start_size << 2; /* try one larger */
			offset--;
		}

		/* allocate block */
		found_block = get_block_recusive(
			P, offset, offset); /* allocate and fragment blocks */

		/* if success : remove task from list and reschedule */
		if (found_block != NULL) {
			/* return found block */
			curr_task->Args.p1.rep_poolptr = found_block;
			curr_task->Args.p1.rep_dataptr = found_block;

			/* reschedule task */

#ifdef CONFIG_SYS_CLOCK_EXISTS
			if (curr_task->Time.timer) {
				delist_timeout(curr_task->Time.timer);
			}
#endif
			curr_task->Time.rcode = RC_OK;
			_k_state_bit_reset(curr_task->Ctxt.proc, TF_GTBL);

			/* remove from list */
			prev_task->Forw = curr_task->Forw;

			/* and get next task */
			curr_task = curr_task->Forw;

		} else {
			/* else just get next task */
			prev_task = curr_task;
			curr_task = curr_task->Forw;
		}
	}

	/* put used command packet on the empty packet list */
	FREEARGS(A);
}

/*******************************************************************************
*
* _k_mem_pool_block_get_timeout_handle - finish handling an allocate block request that timed out
*
* RETURNS: N/A
*/

void _k_mem_pool_block_get_timeout_handle(struct k_args *A)
{
	delist_timeout(A->Time.timer);
	REMOVE_ELM(A);
	A->Time.rcode = RC_TIME;
	_k_state_bit_reset(A->Ctxt.proc, TF_GTBL);
}

/*******************************************************************************
*
* _k_mem_pool_block_get - perform allocate memory pool block request
*
* RETURNS: N/A
*/

void _k_mem_pool_block_get(struct k_args *A)
{
	struct pool_struct *P = _k_mem_pool_list + OBJ_INDEX(A->Args.p1.poolid);
	char *found_block;

	int start_size;
	int offset;

	/* calculate start size */
	start_size = P->minblock_size;
	offset = P->nr_of_frags - 1;

	while (A->Args.p1.req_size > start_size) {
		start_size = start_size << 2; /*try one larger */
		offset--;
	}

	/* startsize==the available size that can contain the requeste block size */
	/* offset: index in fragtable of the minimal blocksize */

	found_block =
		get_block_recusive(P, offset, offset); /* allocate and fragment blocks */

	if (found_block != NULL) {
		A->Args.p1.rep_poolptr = found_block;
		A->Args.p1.rep_dataptr = found_block;
		A->Time.rcode = RC_OK;
		return; /* return found block */
	}

	if (likely(
		    (A->Time.ticks != TICKS_NONE) &&
		    (A->Args.p1.req_size <=
		     P->maxblock_size))) {/* timeout?  but not block to large */
		A->Prio = _k_current_task->Prio;
		A->Ctxt.proc = _k_current_task;
		_k_state_bit_set(_k_current_task, TF_GTBL); /* extra new statebit */

		/* INSERT_ELM (P->frag_tab[offset].Waiters, A); */
		INSERT_ELM(P->Waiters, A);

#ifdef CONFIG_SYS_CLOCK_EXISTS
		if (A->Time.ticks == TICKS_UNLIMITED) {
			A->Time.timer = NULL;
		} else {
			A->Comm = GTBLTMO;
			enlist_timeout(A);
		}
#endif
	} else {
		A->Time.rcode =
			RC_FAIL; /* no blocks available or block too large */
	}
}

/*******************************************************************************
*
* _task_mem_pool_alloc - allocate memory pool block request
*
* This routine allocates a free block from the specified memory pool, ensuring
* that its size is at least as big as the size requested (in bytes).
*
* RETURNS: RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
*/

int _task_mem_pool_alloc(struct k_block *blockptr, /* ptr to requested block */
		     kmemory_pool_t poolid,     /* pool from which to get block */
		     int reqsize,       /* requested block size */
		     int32_t time       /* maximum number of ticks to wait */
		     )
{
	struct k_args A;


	A.Comm = GET_BLOCK;
	A.Time.ticks = time;
	A.Args.p1.poolid = poolid;
	A.Args.p1.req_size = reqsize;

	KERNEL_ENTRY(&A);

	blockptr->poolid = poolid;
	blockptr->address_in_pool = A.Args.p1.rep_poolptr;
	blockptr->pointer_to_data = A.Args.p1.rep_dataptr;
	blockptr->req_size = reqsize;

	return A.Time.rcode;
}

/*******************************************************************************
*
* _k_mem_pool_block_release - perform return memory pool block request
*
* Marks a block belonging to a pool as free; if there are waiters that can use
* the the block it is passed to a waiting task.
*
* RETURNS: N/A
*/

void _k_mem_pool_block_release(struct k_args *A)
{
	struct pool_struct *P;
	struct pool_block *block;
	struct block_stat *blockstat;
	int Pid;
	int start_size, offset;
	int i, j;

	Pid = A->Args.p1.poolid;


	P = _k_mem_pool_list + OBJ_INDEX(Pid);

	/* calculate size */
	start_size = P->minblock_size;
	offset = P->nr_of_frags - 1;

	while (A->Args.p1.req_size > start_size) {
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
			if (A->Args.p1.rep_poolptr ==
			    (blockstat->mem_blocks +
			     (OCTET_TO_SIZEOFUNIT(i * block->block_size)))) {
				/* we've found the right pointer, so free it */
				blockstat->mem_status &= ~(1 << i);

				/* waiters? */
				if (P->Waiters != NULL) {
					struct k_args *NewGet;
					/*
					 * get new command packet that calls the function
					 * that reallocate blocks for the waiting tasks
					 */
					GETARGS(NewGet);
					*NewGet = *A;
					NewGet->Comm = GET_BLOCK_WAIT;
					TO_ALIST(&_k_command_stack, NewGet); /* push on command stack */
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

/*******************************************************************************
*
* task_mem_pool_free - return memory pool block request
*
* This routine returns a block to a memory pool.
*
* The struct k_block structure contains the block details, including the pool to
* which it should be returned.
*
* RETURNS: N/A
*/

void task_mem_pool_free(struct k_block *blockptr /* pointer to block to free */
		      )
{
	struct k_args A;

	A.Comm = REL_BLOCK;
	A.Args.p1.poolid = blockptr->poolid;
	A.Args.p1.req_size = blockptr->req_size;
	A.Args.p1.rep_poolptr = blockptr->address_in_pool;
	A.Args.p1.rep_dataptr = blockptr->pointer_to_data;
	KERNEL_ENTRY(&A);
}
