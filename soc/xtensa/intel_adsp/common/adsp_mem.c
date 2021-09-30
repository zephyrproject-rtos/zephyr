/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <adsp_mem.h>
#include <sys/util_macro.h>
#include <spinlock.h>
#include <soc/memory.h>

/* Start of the region protection option memory region dedicated to
 * RAM on these devices (this is the uncached mapping, there is
 * another at 0xa0000000, but this one is normative)
 */
#define ADSP_MEM_RPO_BASE 0x80000000

#define PAGEID_FLAG BIT(18)
#define PAGEID_MASK (PAGEID_FLAG - 1)

/* Header on each block in a page free list, stored at the first
 * address of the block of pages
 */
struct pagerec {
	size_t num_pages;
	struct pagerec *next;
};

struct pagepool {
	struct pagerec *head;
	uint32_t idx_start;
	uint32_t idx_end;
};

static struct pagepool pools[SOC_ADSP_MEM_NUM_TYPES];

static struct k_spinlock pagelock;

void *soc_adsp_page_addr(soc_adsp_page_id_t pid)
{
	/* The page ID is the index of the page within the 512MB
	 * Xtensa RPO region, with BIT(18) masked in above it to make
	 * it positive-definite.  (One bit larger than space to allow
	 * an index "just past the end" to be legal for comparison).
	 * That allows us to reserve the "negative" region for error
	 * codes and have a zero available as a null.
	 */
	return (void *)(ADSP_MEM_RPO_BASE + (pid & PAGEID_MASK) * SOC_ADSP_PAGE_SZ);
}

soc_adsp_page_id_t soc_adsp_page_id(void *addr)
{
	uintptr_t idx = (((uintptr_t)addr) - ADSP_MEM_RPO_BASE) / SOC_ADSP_PAGE_SZ;

	return (idx & PAGEID_MASK) | PAGEID_FLAG;
}

static void *page_alloc(struct pagepool *pool)
{
	void *ret = NULL;

	if (pool->head != NULL) {
		if (pool->head->num_pages == 1) {
			/* Single page record */
			ret = pool->head;
			pool->head = pool->head->next;
		} else {
			/* Multipage block, use the last one */
			pool->head->num_pages--;
			ret = (void *)((size_t)pool->head
				       + (pool->head->num_pages * SOC_ADSP_PAGE_SZ));
		}
	}
	return ret;
}

static void page_free(struct pagepool *pool, void *page)
{
	struct pagerec *pr = page;

	pr->num_pages = 1;
	pr->next = pool->head;
	pool->head = pr;
}

soc_adsp_page_id_t soc_adsp_page_alloc(enum soc_adsp_mem_t type)
{
	k_spinlock_key_t k = k_spin_lock(&pagelock);
	void *page = page_alloc(&pools[type]);

	k_spin_unlock(&pagelock, k);
	return page == NULL ? -ENOMEM : soc_adsp_page_id(page);
}

int32_t soc_adsp_page_free(soc_adsp_page_id_t page)
{
	k_spinlock_key_t k = k_spin_lock(&pagelock);

	for (int i = 0; i < ARRAY_SIZE(pools); i++) {
		if (page >= pools[i].idx_start && page < pools[i].idx_end) {
			page_free(&pools[i], soc_adsp_page_addr(page));
		}
	}
	k_spin_unlock(&pagelock, k);
	return -EINVAL;
}

int32_t soc_adsp_pages_alloc(enum soc_adsp_mem_t type, size_t count,
			     soc_adsp_page_id_t *pages_out)
{
	int i;

	for (i = 0; i < count; i++) {
		pages_out[i] = soc_adsp_page_alloc(type);
		if (pages_out[i] < 0) {
			break;
		}
	}

	/* Clean up on failure */
	if (i < count) {
		(void)soc_adsp_pages_free(pages_out, i);
		return -ENOMEM;
	}

	return 0;
}

int32_t soc_adsp_pages_free(soc_adsp_page_id_t *pages, size_t count)
{
	for (int i = 0; i < count; i++) {
		soc_adsp_page_free(pages[i]);
	}
	return 0;
}

int32_t soc_adsp_pages_map(size_t count, soc_adsp_page_id_t *pages, void *vaddr)
{
	k_spinlock_key_t k = k_spin_lock(&pagelock);
	int32_t ret = 0;

	for (int i = 0; i < count; i++) {
		void *va = (char *)vaddr + (i * SOC_ADSP_PAGE_SZ);
		void *pa = soc_adsp_page_addr(pages[i]);

		/* Only HPSRAM can be mapped */
		if (pages[i] < pools[SOC_ADSP_MEM_HP_SRAM].idx_start ||
		    pages[i] >= pools[SOC_ADSP_MEM_HP_SRAM].idx_end) {
			ret = -EINVAL;
		}

		z_soc_mem_map(z_soc_cached_ptr(va),
			      (uintptr_t) z_soc_cached_ptr(pa),
			      SOC_ADSP_PAGE_SZ, 0);
	}

	k_spin_unlock(&pagelock, k);
	return ret;
}

int32_t soc_adsp_pages_unmap(size_t count, void *vaddr)
{
	k_spinlock_key_t k = k_spin_lock(&pagelock);

	for (int i = 0; i < count; i++) {
		void *va = (char *)vaddr + (i * SOC_ADSP_PAGE_SZ);

		z_soc_mem_unmap(z_soc_cached_ptr(va), SOC_ADSP_PAGE_SZ);
	}

	k_spin_unlock(&pagelock, k);
	return 0;
}

#endif /* ZEPHYR_SOC_ADSP_MEM_H_ */

static void pool_init(struct pagepool *pool, uintptr_t start, uintptr_t end)
{
	/* Force the input addresses to the lower (uncached) of the
	 * two mapped RPO regions.
	 */
	start &= ~BIT(29);
	end &= ~BIT(29);

	start = ROUND_UP(start, SOC_ADSP_PAGE_SZ);
	end = ROUND_DOWN(end, SOC_ADSP_PAGE_SZ);

	struct pagerec *pr = (void *)start;

	pr->num_pages = (end - start) / SOC_ADSP_PAGE_SZ;
	pr->next = NULL;
	pool->head = pr;
	pool->idx_start = soc_adsp_page_id((void *)start);
	pool->idx_end = soc_adsp_page_id((void *)end);
}

void soc_adsp_mem_init(void)
{
	/* Usable HP SRAM ("normal" memory) starts with a
	 * linker-maintained symbol marking the end of the linked area
	 */
	extern char _end;

	pool_init(&pools[SOC_ADSP_MEM_HP_SRAM], (uintptr_t)&_end,
		  HP_SRAM_BASE + HP_SRAM_SIZE);

	uintptr_t lpbase = LP_SRAM_BASE;

	/* cAVS 2.5 hardware uses the bottom of LPSRAM as its boot
	 * vector, so we can't touch that page
	 */
	if (IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V25)) {
		lpbase += SOC_ADSP_PAGE_SZ;
	}

	pool_init(&pools[SOC_ADSP_MEM_LP_SRAM], lpbase,
		  LP_SRAM_BASE + LP_SRAM_SIZE);

	/* IMR memory is poorly exposed by Zephyr's integration right
	 * now.  The base doesn't appear in dts or SOF-derived
	 * headers, the size is actually requested at runtime via the
	 * bootloader ROM.  This works for now, needs a framework.
	 */
	if (IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V25)) {
		pool_init(&pools[SOC_ADSP_MEM_IMR], 0x90000000, 0x90400000);
	}
}
