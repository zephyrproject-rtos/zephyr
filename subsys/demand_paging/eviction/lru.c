/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Least Recently Used (LRU) eviction algorithm for demand paging.
 *
 * This is meant to be used with MMUs that need manual tracking of their
 * "accessed" page flag so this can be called at the same time.
 *
 * Theory of Operation:
 *
 * - Page frames made evictable are appended to the end of the LRU queue with
 *   k_mem_paging_eviction_add(). They are presumably made unaccessible in
 *   their corresponding MMU page table initially, but not a deal breaker
 *   if not.
 *
 * - When accessed, an unaccessible page causes a fault. The architecture
 *   fault handler makes the page accessible, marks it as accessed and calls
 *   k_mem_paging_eviction_accessed() which moves the corresponding page frame
 *   back to the end of the queue.
 *
 * - On page reclammation, the page at the head of the queue is removed for
 *   that purpose. The new head page is marked unaccessible.
 *
 * - If the new head page is actively used, it will cause a fault and be moved
 *   to the end of the queue, preventing it from being the next page
 *   reclamation victim. Then the new head page is made unaccessible.
 *
 * This way, unused pages will migrate toward the head of the queue, used
 * pages will tend to remain towards the end of the queue. And there won't be
 * any fault overhead while the set of accessed pages remain stable.
 * This algorithm's complexity is O(1).
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <zephyr/spinlock.h>
#include <mmu.h>
#include <kernel_arch_interface.h>

/*
 * Page frames are ordered according to their access pattern. Using a regular
 * doubly-linked list with actual pointers would be wasteful as all we need
 * is a previous PF index and a next PF index for each page frame number
 * which can be compactly represented in an array.
 */

/*
 * Number of bits needed to store a page frame index. Rounded up to a byte
 * boundary for best compromize between code performance and space saving.
 * The extra entry is used to store head and tail indexes.
 */
#define PF_IDX_BITS ROUND_UP(LOG2CEIL(K_MEM_NUM_PAGE_FRAMES + 1), 8)

/* For each page frame, track the previous and next page frame in the queue. */
struct lru_pf_idx {
	uint32_t next : PF_IDX_BITS;
	uint32_t prev : PF_IDX_BITS;
} __packed;

static struct lru_pf_idx lru_pf_queue[K_MEM_NUM_PAGE_FRAMES + 1];
static struct k_spinlock lru_lock;

/* Slot 0 is for head and tail indexes (actual indexes are offset by 1) */
#define LRU_PF_HEAD lru_pf_queue[0].next
#define LRU_PF_TAIL lru_pf_queue[0].prev

static inline uint32_t pf_to_idx(struct k_mem_page_frame *pf)
{
	return (pf - k_mem_page_frames) + 1;
}

static inline struct k_mem_page_frame *idx_to_pf(uint32_t idx)
{
	return &k_mem_page_frames[idx - 1];
}

static inline void lru_pf_append(uint32_t pf_idx)
{
	lru_pf_queue[pf_idx].next = 0;
	lru_pf_queue[pf_idx].prev = LRU_PF_TAIL;
	lru_pf_queue[LRU_PF_TAIL].next = pf_idx;
	LRU_PF_TAIL = pf_idx;
}

static inline void lru_pf_unlink(uint32_t pf_idx)
{
	uint32_t next = lru_pf_queue[pf_idx].next;
	uint32_t prev = lru_pf_queue[pf_idx].prev;

	lru_pf_queue[prev].next = next;
	lru_pf_queue[next].prev = prev;

	lru_pf_queue[pf_idx].next = 0;
	lru_pf_queue[pf_idx].prev = 0;
}

static inline bool lru_pf_in_queue(uint32_t pf_idx)
{
	bool unqueued = (lru_pf_queue[pf_idx].next == 0) &&
			(lru_pf_queue[pf_idx].prev == 0) &&
			(LRU_PF_HEAD != pf_idx);

	return !unqueued;
}

static void lru_pf_remove(uint32_t pf_idx)
{
	bool was_head = (pf_idx == LRU_PF_HEAD);

	lru_pf_unlink(pf_idx);

	/* make new head PF unaccessible if it exists and it is not alone */
	if (was_head &&
	    (LRU_PF_HEAD != 0) &&
	    (lru_pf_queue[LRU_PF_HEAD].next != 0)) {
		struct k_mem_page_frame *pf = idx_to_pf(LRU_PF_HEAD);
		uintptr_t flags = arch_page_info_get(k_mem_page_frame_to_virt(pf), NULL, true);

		/* clearing the accessed flag expected only on loaded pages */
		__ASSERT((flags & ARCH_DATA_PAGE_LOADED) != 0, "");
		ARG_UNUSED(flags);
	}
}

void k_mem_paging_eviction_add(struct k_mem_page_frame *pf)
{
	uint32_t pf_idx = pf_to_idx(pf);
	k_spinlock_key_t key = k_spin_lock(&lru_lock);

	__ASSERT(k_mem_page_frame_is_evictable(pf), "");
	__ASSERT(!lru_pf_in_queue(pf_idx), "");
	lru_pf_append(pf_idx);
	k_spin_unlock(&lru_lock, key);
}

void k_mem_paging_eviction_remove(struct k_mem_page_frame *pf)
{
	uint32_t pf_idx = pf_to_idx(pf);
	k_spinlock_key_t key = k_spin_lock(&lru_lock);

	__ASSERT(lru_pf_in_queue(pf_idx), "");
	lru_pf_remove(pf_idx);
	k_spin_unlock(&lru_lock, key);
}

void k_mem_paging_eviction_accessed(uintptr_t phys)
{
	struct k_mem_page_frame *pf = k_mem_phys_to_page_frame(phys);
	uint32_t pf_idx = pf_to_idx(pf);
	k_spinlock_key_t key = k_spin_lock(&lru_lock);

	if (lru_pf_in_queue(pf_idx)) {
		lru_pf_remove(pf_idx);
		lru_pf_append(pf_idx);
	}
	k_spin_unlock(&lru_lock, key);
}

struct k_mem_page_frame *k_mem_paging_eviction_select(bool *dirty_ptr)
{
	uint32_t head_pf_idx = LRU_PF_HEAD;

	if (head_pf_idx == 0) {
		return NULL;
	}

	struct k_mem_page_frame *pf = idx_to_pf(head_pf_idx);
	uintptr_t flags = arch_page_info_get(k_mem_page_frame_to_virt(pf), NULL, false);

	__ASSERT(k_mem_page_frame_is_evictable(pf), "");
	*dirty_ptr = ((flags & ARCH_DATA_PAGE_DIRTY) != 0);
	return pf;
}

void k_mem_paging_eviction_init(void)
{
}
