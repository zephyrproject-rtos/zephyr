/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/x86/mmustructs.h>
#include <sys/mem_manage.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <errno.h>
#include <ctype.h>
#include <spinlock.h>
#include <kernel_arch_func.h>
#include <x86_mmu.h>
#include <init.h>
#include <kernel_internal.h>

LOG_MODULE_DECLARE(os);

/* "dummy" pagetables for the first-phase build. The real page tables
 * are produced by gen-mmu.py based on data read in zephyr-prebuilt.elf,
 * and this dummy array is discarded.
 */
Z_GENERIC_SECTION(.dummy_pagetables)
char z_x86_dummy_pagetables[Z_X86_INITIAL_PAGETABLE_SIZE];

/*
 * Definitions for building an ontology of paging levels and capabilities
 * at each level
 */

/* Data structure describing the characteristics of a particular paging
 * level
 */
struct paging_level {
	/* What bits are used to store physical address */
	pentry_t mask;

	/* Number of entries in this paging structure */
	size_t entries;

	/* How many bits to right-shift a virtual address to obtain the
	 * appropriate entry within this table.
	 *
	 * The memory scope of each entry in this table is 1 << shift.
	 */
	unsigned int shift;
#ifdef CONFIG_EXCEPTION_DEBUG
	/* Name of this level, for debug purposes */
	const char *name;
#endif
};

/* Flags for all entries in intermediate paging levels.
 * Fortunately, the same bits are set for all intermediate levels for all
 * three paging modes.
 *
 * Obviously P is set.
 *
 * We want RW and US bit always set; actual access control will be
 * done at the leaf level.
 *
 * XD (if supported) always 0. Disabling execution done at leaf level.
 *
 * PCD/PWT always 0. Caching properties again done at leaf level.
 */
#define INT_FLAGS	(MMU_P | MMU_RW | MMU_US)

/* Paging level ontology for the selected paging mode.
 *
 * See Figures 4-4, 4-7, 4-11 in the Intel SDM, vol 3A
 */
static const struct paging_level paging_levels[] = {
#ifdef CONFIG_X86_64
	/* Page Map Level 4 */
	{
		.mask = 0x7FFFFFFFFFFFF000ULL,
		.entries = 512U,
		.shift = 39U,
#ifdef CONFIG_EXCEPTION_DEBUG
		.name = "PML4"
#endif
	},
#endif /* CONFIG_X86_64 */
#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
	/* Page Directory Pointer Table */
	{
		.mask = 0x7FFFFFFFFFFFF000ULL,
#ifdef CONFIG_X86_64
		.entries = 512U,
#else
		/* PAE version */
		.entries = 4U,
#endif
		.shift = 30U,
#ifdef CONFIG_EXCEPTION_DEBUG
		.name = "PDPT"
#endif
	},
#endif /* CONFIG_X86_64 || CONFIG_X86_PAE */
	/* Page Directory */
	{
#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
		.mask = 0x7FFFFFFFFFFFF000ULL,
		.entries = 512U,
		.shift = 21U,
#else
		/* 32-bit */
		.mask = 0xFFFFF000U,
		.entries = 1024U,
		.shift = 22U,
#endif /* CONFIG_X86_64 || CONFIG_X86_PAE */
#ifdef CONFIG_EXCEPTION_DEBUG
		.name = "PD"
#endif
	},
	/* Page Table */
	{
#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
		.mask = 0x07FFFFFFFFFFF000ULL,
		.entries = 512U,
		.shift = 12U,
#else
		/* 32-bit */
		.mask = 0xFFFFF000U,
		.entries = 1024U,
		.shift = 12U,
#endif /* CONFIG_X86_64 || CONFIG_X86_PAE */
#ifdef CONFIG_EXCEPTION_DEBUG
		.name = "PT"
#endif
	}
};

#define NUM_LEVELS	ARRAY_SIZE(paging_levels)

/*
 * Utility functions
 */

/* For a table at a particular level, get the entry index that corresponds to
 * the provided virtual address
 */
static inline int get_index(void *virt, int level)
{
	return (((uintptr_t)virt >> paging_levels[level].shift) %
		paging_levels[level].entries);
}

static inline pentry_t *get_entry_ptr(pentry_t *ptables, void *virt, int level)
{
	return &ptables[get_index(virt, level)];
}

static inline pentry_t get_entry(pentry_t *ptables, void *virt, int level)
{
	return ptables[get_index(virt, level)];
}

/* Get the physical memory address associated with this table entry */
static inline uintptr_t get_entry_phys(pentry_t entry, int level)
{
	return entry & paging_levels[level].mask;
}

/* Return the virtual address of a linked table stored in the provided entry */
static inline pentry_t *next_table(pentry_t entry, int level)
{
	return (pentry_t *)(get_entry_phys(entry, level));
}

/* 4K for everything except PAE PDPTs */
static inline size_t table_size(int level)
{
	return paging_levels[level].entries * sizeof(pentry_t);
}

/* For a table at a particular level, size of the amount of virtual memory
 * that an entry within the table covers
 */
static inline size_t get_entry_scope(int level)
{
	return (1UL << paging_levels[level].shift);
}

/* For a table at a particular level, size of the amount of virtual memory
 * that this entire table covers
 */
static inline size_t get_table_scope(int level)
{
	return get_entry_scope(level) * paging_levels[level].entries;
}

/* Must have checked Present bit first! Non-present entries may have OS data
 * stored in any other bits
 */
static inline bool is_leaf(int level, pentry_t entry)
{
	if (level == NUM_LEVELS - 1) {
		/* Always true for PTE */
		return true;
	}

	return ((entry & MMU_PS) != 0U);
}

static inline void tlb_flush_page(void *addr)
{
	/* Invalidate TLB entries corresponding to the page containing the
	 * specified address
	 */
	char *page = (char *)addr;

	__asm__ ("invlpg %0" :: "m" (*page));

	/* TODO: Need to implement TLB shootdown for SMP */
}

static inline void assert_addr_aligned(uintptr_t addr)
{
#if __ASSERT_ON
	__ASSERT((addr & (CONFIG_MMU_PAGE_SIZE - 1)) == 0U,
		 "unaligned address 0x%" PRIxPTR, addr);
#endif
}

static inline void assert_virt_addr_aligned(void *addr)
{
	assert_addr_aligned((uintptr_t)addr);
}

static inline void assert_region_page_aligned(void *addr, size_t size)
{
	assert_virt_addr_aligned(addr);
#if __ASSERT_ON
	__ASSERT((size & (CONFIG_MMU_PAGE_SIZE - 1)) == 0U,
		 "unaligned size %zu", size);
#endif
}

/*
 * Debug functions. All conditionally compiled with CONFIG_EXCEPTION_DEBUG.
 */
#ifdef CONFIG_EXCEPTION_DEBUG

/* Add colors to page table dumps to indicate mapping type */
#define COLOR_PAGE_TABLES	1

#if COLOR_PAGE_TABLES
#define ANSI_DEFAULT "\x1B[0m"
#define ANSI_RED     "\x1B[1;31m"
#define ANSI_GREEN   "\x1B[1;32m"
#define ANSI_YELLOW  "\x1B[1;33m"
#define ANSI_BLUE    "\x1B[1;34m"
#define ANSI_MAGENTA "\x1B[1;35m"
#define ANSI_CYAN    "\x1B[1;36m"
#define ANSI_GREY    "\x1B[1;90m"

#define COLOR(x)	printk(_CONCAT(ANSI_, x))
#else
#define COLOR(x)	do { } while (0)
#endif

static char get_entry_code(pentry_t value)
{
	char ret;

	if ((value & MMU_P) == 0U) {
		ret = '.';
	} else {
		if ((value & MMU_RW) != 0U) {
			/* Writable page */
			if ((value & MMU_XD) != 0U) {
				/* RW */
				ret = 'w';
			} else {
				/* RWX */
				ret = 'a';
			}
		} else {
			if ((value & MMU_XD) != 0U) {
				/* R */
				ret = 'r';
			} else {
				/* RX */
				ret = 'x';
			}
		}

		if ((value & MMU_US) != 0U) {
			/* Uppercase indicates user mode access */
			ret = toupper(ret);
		}
	}

	return ret;
}

static void print_entries(pentry_t entries_array[], uint8_t *base, int level,
			  size_t count)
{
	int column = 0;

	for (int i = 0; i < count; i++) {
		pentry_t entry = entries_array[i];

		uintptr_t phys = get_entry_phys(entry, level);
		uintptr_t virt =
			(uintptr_t)base + (get_entry_scope(level) * i);

		if (entry & MMU_P) {
			if (is_leaf(level, entry)) {
				if (phys == virt) {
					/* Identity mappings */
					COLOR(YELLOW);
				} else {
					/* Other mappings */
					COLOR(GREEN);
				}
			} else {
				COLOR(MAGENTA);
			}
		} else {
			COLOR(GREY);
		}

		printk("%c", get_entry_code(entry));

		column++;
		if (column == 64) {
			column = 0;
			printk("\n");
		}
	}
	COLOR(DEFAULT);

	if (column != 0) {
		printk("\n");
	}
}

static void dump_ptables(pentry_t *table, uint8_t *base, int level)
{
	const struct paging_level *info = &paging_levels[level];

#ifdef CONFIG_X86_64
	/* Account for the virtual memory "hole" with sign-extension */
	if (((uintptr_t)base & BITL(47)) != 0) {
		base = (uint8_t *)((uintptr_t)base | (0xFFFFULL << 48));
	}
#endif

	printk("%s at %p: ", info->name, table);
	if (level == 0) {
		printk("entire address space\n");
	} else {
		printk("for %p - %p\n", base,
		       base + get_table_scope(level) - 1);
	}

	print_entries(table, base, level, info->entries);

	/* Check if we're a page table */
	if (level == (NUM_LEVELS - 1)) {
		return;
	}

	/* Dump all linked child tables */
	for (int j = 0; j < info->entries; j++) {
		pentry_t entry = table[j];
		pentry_t *next;

		if ((entry & MMU_P) == 0U ||
			(entry & MMU_PS) != 0U) {
			/* Not present or big page, skip */
			continue;
		}

		next = next_table(entry, level);
		dump_ptables(next, base + (j * get_entry_scope(level)),
			     level + 1);
	}
}

void z_x86_dump_page_tables(pentry_t *ptables)
{
	dump_ptables(ptables, NULL, 0);
}

/* Enable to dump out the kernel's page table right before main() starts,
 * sometimes useful for deep debugging. May overwhelm sanitycheck.
 */
#define DUMP_PAGE_TABLES 0

#if DUMP_PAGE_TABLES
static int dump_kernel_tables(const struct device *unused)
{
	z_x86_dump_page_tables(&z_x86_kernel_ptables);

	return 0;
}

SYS_INIT(dump_kernel_tables, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif

static void str_append(char **buf, size_t *size, const char *str)
{
	int ret = snprintk(*buf, *size, "%s", str);

	if (ret >= *size) {
		/* Truncated */
		*size = 0U;
	} else {
		*size -= ret;
		*buf += ret;
	}

}

static void dump_entry(int level, void *virt, pentry_t entry)
{
	const struct paging_level *info = &paging_levels[level];
	char buf[24] = { 0 };
	char *pos = buf;
	size_t sz = sizeof(buf);
	uint8_t *virtmap = (uint8_t *)ROUND_DOWN(virt, get_entry_scope(level));

	#define DUMP_BIT(bit) do { \
			if ((entry & MMU_##bit) != 0U) { \
				str_append(&pos, &sz, #bit " "); \
			} \
		} while (0)

	DUMP_BIT(RW);
	DUMP_BIT(US);
	DUMP_BIT(PWT);
	DUMP_BIT(PCD);
	DUMP_BIT(A);
	DUMP_BIT(D);
	DUMP_BIT(G);
	DUMP_BIT(XD);

	LOG_ERR("%sE: %p -> " PRI_ENTRY ": %s", info->name,
		virtmap, entry & info->mask, log_strdup(buf));

	#undef DUMP_BIT
}

void z_x86_pentry_get(int *paging_level, pentry_t *val, pentry_t *ptables,
		      void *virt)
{
	pentry_t *table = ptables;

	for (int level = 0; level < NUM_LEVELS; level++) {
		pentry_t entry = get_entry(table, virt, level);

		if ((entry & MMU_P) == 0 || is_leaf(level, entry)) {
			*val = entry;
			*paging_level = level;
			break;
		} else {
			table = next_table(entry, level);
		}
	}
}

/*
 * Debug function for dumping out MMU table information to the LOG for a
 * specific virtual address, such as when we get an unexpected page fault.
 */
void z_x86_dump_mmu_flags(pentry_t *ptables, void *virt)
{
	pentry_t entry;
	int level;

	z_x86_pentry_get(&level, &entry, ptables, virt);

	if ((entry & MMU_P) == 0) {
		LOG_ERR("%sE: not present", paging_levels[level].name);
	} else {
		dump_entry(level, virt, entry);
	}
}
#endif /* CONFIG_EXCEPTION_DEBUG */

/* Page allocation function prototype, passed to map_page() */
typedef void * (*page_get_func_t)(void *);

/*
 * Pool of free memory pages for creating new page tables, as needed.
 *
 * This is very crude, once obtained, pages may not be returned. Fine for
 * permanent kernel mappings.
 */
static uint8_t __noinit
	page_pool[CONFIG_MMU_PAGE_SIZE * CONFIG_X86_MMU_PAGE_POOL_PAGES]
	__aligned(CONFIG_MMU_PAGE_SIZE);

static uint8_t *page_pos = page_pool + sizeof(page_pool);

static struct k_spinlock pool_lock;

/* Return a zeroed and suitably aligned memory page for page table data
 * from the global page pool
 */
static void *page_pool_get(void *context)
{
	void *ret;
	k_spinlock_key_t key;

	ARG_UNUSED(context);

	key = k_spin_lock(&pool_lock);
	if (page_pos == page_pool) {
		ret = NULL;
	} else {
		page_pos -= CONFIG_MMU_PAGE_SIZE;
		ret = page_pos;
	}
	k_spin_unlock(&pool_lock, key);

	if (ret != NULL) {
		memset(ret, 0, CONFIG_MMU_PAGE_SIZE);
	}

	return ret;
}

/**
 * Low-level mapping function
 *
 * Walk the provided page tables until we get to the PTE for the provided
 * virtual address, and set that to whatever is in 'entry_val'.
 *
 * If memory must be drawn to instantiate page table memory, it will be
 * obtained from the provided get_page() function. The function must
 * return a page-aligned pointer to a page-sized block of zeroed memory.
 * All intermediate tables have hard-coded flags of INT_FLAGS.
 *
 * Presumes we want to map a minimally sized page of CONFIG_MMU_PAGE_SIZE.
 * No support for mapping big pages yet; unclear if we will ever need it given
 * Zephyr's typical use-cases.
 *
 * TODO: There may be opportunities to optimize page table walks such as this
 * by using recusrive page table mappings, see for example
 * https://os.phil-opp.com/paging-implementation/#recursive-page-tables
 * May also help if we need fast virtual-to-physical translation outside of
 * the permanent memory mapping area.
 *
 * @param ptables Top-level page tables pointer
 * @param virt Virtual address to set mapping
 * @param entry_val Value to set PTE to
 * @param get_page Function to draw memory pages from
 * @param ctx Context pointer to pass to get_page()
 * @retval 0 success
 * @retval -ENOMEM get_page() failed
 */
static int page_map_set(pentry_t *ptables, void *virt, pentry_t entry_val,
			page_get_func_t get_page, void *ctx)
{
	pentry_t *table = ptables;

	for (int level = 0; level < NUM_LEVELS; level++) {
		int index;
		pentry_t *entryp;

		index = get_index(virt, level);
		entryp = &table[index];

		/* Check if we're a PTE */
		if (level == (NUM_LEVELS - 1)) {
			*entryp = entry_val;
			break;
		}

		/* This is a non-leaf entry */
		if ((*entryp & MMU_P) == 0U) {
			/* Not present. Never done a mapping here yet, need
			 * some RAM for linked tables
			 */
			void *new_table = get_page(ctx);

			if (new_table == NULL) {
				return -ENOMEM;
			}
			*entryp = ((uintptr_t)new_table) | INT_FLAGS;
			table = new_table;
		} else {
			/* We fail an assertion here due to no support for
			 * splitting existing bigpage mappings.
			 * If the PS bit is not supported at some level (like
			 * in a PML4 entry) it is always reserved and must be 0
			 */
			__ASSERT((*entryp & MMU_PS) == 0U,
				  "large page encountered");
			table = next_table(*entryp, level);
		}
	}

	return 0;
}

/* map region virt..virt+size to phys with provided arch-neutral flags */
int arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	pentry_t entry_flags = MMU_P;
	pentry_t *ptables;

	LOG_DBG("%s: %p -> %p (%zu) flags 0x%x",
		__func__, (void *)phys, virt, size, flags);

#ifdef CONFIG_X86_64
	/* There's a gap in the "64-bit" address space, as 4-level paging
	 * requires bits 48 to 63 to be copies of bit 47. Test this
	 * by treating as a signed value and shifting.
	 */
	__ASSERT(((((intptr_t)virt) << 16) >> 16) == (intptr_t)virt,
		 "non-canonical virtual address mapping %p (size %zu)",
		 virt, size);
#endif /* CONFIG_X86_64 */

	/* For now, always map in the kernel's page tables, we're just using
	 * this for driver mappings. User mode mappings
	 * (and interactions with KPTI) not implemented yet.
	 */
	ptables = (pentry_t *)&z_x86_kernel_ptables;

	/* Translate flags argument into HW-recognized entry flags.
	 *
	 * Support for PAT is not implemented yet. Many systems may have
	 * BIOS-populated MTRR values such that these cache settings are
	 * redundant.
	 */
	switch (flags & K_MEM_CACHE_MASK) {
	case K_MEM_CACHE_NONE:
		entry_flags |= MMU_PCD;
		break;
	case K_MEM_CACHE_WT:
		entry_flags |= MMU_PWT;
		break;
	case K_MEM_CACHE_WB:
		break;
	default:
		return -ENOTSUP;
	}
	if ((flags & K_MEM_PERM_RW) != 0U) {
		entry_flags |= MMU_RW;
	}
	if ((flags & K_MEM_PERM_USER) != 0U) {
		/* TODO: user mode support
		 * entry_flags |= MMU_US;
		 */
		return -ENOTSUP;
	}
	if ((flags & K_MEM_PERM_EXEC) == 0U) {
		entry_flags |= MMU_XD;
	}

	for (size_t offset = 0; offset < size; offset += CONFIG_MMU_PAGE_SIZE) {
		int ret;
		pentry_t entry_val = (phys + offset) | entry_flags;
		uint8_t *dest_virt = (uint8_t *)virt + offset;

		ret = page_map_set(ptables, dest_virt, entry_val,
				   page_pool_get, NULL);

		/* Currently used for new mappings, no TLB flush. Re-visit
		 * as capabilities increase
		 */

		if (ret != 0) {
			/* NOTE: Currently we do not un-map a partially
			 * completed mapping.
			 */
			return ret;
		}
	}

	return 0;
}

#if CONFIG_X86_STACK_PROTECTION
/* Legacy stack guard function. This will eventually be replaced in favor
 * of memory-mapping stacks (with a non-present mapping immediately below each
 * one to catch overflows) instead of using in-place
 */
static void stack_guard_set(void *guard_page)
{
	pentry_t pte = ((uintptr_t)guard_page) | MMU_P | MMU_XD;
	int ret;

	assert_virt_addr_aligned(guard_page);

	/* Always modify the kernel's page tables since this is for
	 * supervisor threads or handling syscalls
	 */
	ret = page_map_set(&z_x86_kernel_ptables, guard_page, pte,
			   page_pool_get, NULL);
	/* Literally should never happen */
	__ASSERT(ret == 0, "stack guard mapping failed for %p", guard_page);
	(void)ret;
}

void z_x86_set_stack_guard(k_thread_stack_t *stack)
{
#ifdef CONFIG_USERSPACE
	if (z_stack_is_user_capable(stack)) {
		struct z_x86_thread_stack_header *header =
			(struct z_x86_thread_stack_header *)stack;

		stack_guard_set(&header->guard_page);
	} else
#endif /* CONFIG_USERSPACE */
	{
		stack_guard_set(stack);
	}
}
#endif /* CONFIG_X86_STACK_PROTECTION */

#ifdef CONFIG_USERSPACE
/*
 * All of the code below will eventually be removed/replaced with a virtual
 * address space aware userspace that doesn't do a physical memory map with
 * memory domains.
 */
static bool page_validate(pentry_t *ptables, uint8_t *addr, bool write)
{
	pentry_t *table = (pentry_t *)ptables;

	for (int level = 0; level < NUM_LEVELS; level++) {
		pentry_t entry = get_entry(table, addr, level);

		if ((entry & MMU_P) == 0U) {
			/* Non-present, no access.
			 * TODO: will need re-visiting with demand paging
			 * implemented, could just be paged out
			 */
			return false;
		}

		if (is_leaf(level, entry)) {
			if (((entry & MMU_US) == 0U) ||
			    (write && ((entry & MMU_RW) == 0U))) {
				return false;
			}
		} else {
			table = next_table(entry, level);
		}
	}

	return true;
}

static inline void bcb_fence(void)
{
#ifdef CONFIG_X86_BOUNDS_CHECK_BYPASS_MITIGATION
	__asm__ volatile ("lfence" : : : "memory");
#endif
}

int arch_buffer_validate(void *addr, size_t size, int write)
{
	pentry_t *ptables = z_x86_thread_page_tables_get(_current);
	uint8_t *virt;
	size_t aligned_size;
	int ret = 0;

	/* addr/size arbitrary, fix this up into an aligned region */
	k_mem_region_align((uintptr_t *)&virt, &aligned_size,
			   (uintptr_t)addr, size, CONFIG_MMU_PAGE_SIZE);

	for (size_t offset = 0; offset < aligned_size;
	     offset += CONFIG_MMU_PAGE_SIZE) {
		if (!page_validate(ptables, virt + offset, write)) {
			ret = -1;
			break;
		}
	}

	bcb_fence();

	return ret;
}

/* Fetch pages for per-thread page tables from reserved space within the
 * thread stack object
 *
 * For the moment, re-use pool_lock for synchronization
 */
static void *thread_page_pool_get(void *context)
{
	struct k_thread *thread = context;
	uint8_t *stack_object = (uint8_t *)thread->stack_obj;
	void *ret;
	k_spinlock_key_t key;

	key = k_spin_lock(&pool_lock);
	ret = thread->arch.mmu_pos;

	if (thread->arch.mmu_pos >= stack_object + Z_X86_THREAD_PT_AREA) {
		ret = NULL;
	} else {
		thread->arch.mmu_pos += CONFIG_MMU_PAGE_SIZE;
		memset(ret, 0, CONFIG_MMU_PAGE_SIZE);
	}
	k_spin_unlock(&pool_lock, key);

	return ret;
}

#define RAM_BASE	((uintptr_t)CONFIG_SRAM_BASE_ADDRESS)
#define RAM_END		(RAM_BASE + (CONFIG_SRAM_SIZE * 1024UL))

/* Establish a mapping in the thread's page tables */
static void thread_map(struct k_thread *thread, void *ptr, size_t size,
		       pentry_t flags, bool flush)
{
	pentry_t *ptables = z_x86_thread_page_tables_get(thread);

	assert_region_page_aligned(ptr, size);

	/* Only mapping system RAM addresses is supported in thread page tables,
	 * as the thread does not have its own copies of tables outside of it
	 */
	__ASSERT((uintptr_t)ptr >= RAM_BASE,
		 "%p below system RAM", ptr);
	__ASSERT((uintptr_t)ptr < RAM_END,
		 "%p above system ram", ptr);

	for (size_t offset = 0; offset < size; offset += CONFIG_MMU_PAGE_SIZE) {
		pentry_t pte;
		uint8_t *pos;
		int ret;

		pos = (uint8_t *)ptr + offset;

		if ((flags & MMU_P) == 0U) {
			/* L1TF */
			pte = 0U;
		} else {
			pte = ((uintptr_t)pos) | flags;
		}

		ret = page_map_set(ptables, pos, pte, thread_page_pool_get,
				   thread);
		__ASSERT(ret == 0, "mapping failed for %p", pos);
		(void)ret;

		if (flush) {
			tlb_flush_page(pos);
		}
	}
}

/* Get the kernel's PTE value for a particular virtual address */
static pentry_t kernel_page_map_get(void *virt)
{
	pentry_t *table = &z_x86_kernel_ptables;

	for (int level = 0; level < NUM_LEVELS; level++) {
		pentry_t entry = get_entry(table, virt, level);

		if ((entry & MMU_P) == 0U) {
			break;
		}

		if (is_leaf(level, entry)) {
			__ASSERT((entry & MMU_PS) == 0, "bigpage found");
			return entry;
		}

		table = next_table(entry, level);
	}

	return 0;
}

/* In thread page tables, set mapping for a particular address to whatever
 * mapping is set up for that address in the kernel's page tables.
 */
static void page_reset(struct k_thread *thread, void *virt)
{
	pentry_t kern_pte = kernel_page_map_get(virt);
	pentry_t *thread_ptables = z_x86_thread_page_tables_get(thread);
	int ret;

#ifdef CONFIG_X86_KPTI
	/* Shared kernel page needs to be mapped in page tables as it contains
	 * trampoline stack and important data structures. Otherwise, non-User
	 * pages aren't present.
	 */
	if ((kern_pte & MMU_US) == 0U && virt != &z_shared_kernel_page_start) {
		kern_pte = 0;
	}
#endif /* CONFIG_X86_KPTI */

	ret = page_map_set(thread_ptables, virt,
			   kern_pte, thread_page_pool_get, thread);
	__ASSERT(ret == 0, "mapping failed for %p", virt);
	(void)ret;
}

#ifdef CONFIG_X86_KPTI
/* KPTI version. The thread-level page tables are ONLY used by user mode
 * and very briefly when changing privileges.
 *
 * We leave any memory addresses outside of system RAM unmapped.
 * Any addresses within system RAM are also unmapped unless they have the US
 * bit set, or are the trampoline page.
 */
static void setup_thread_tables(struct k_thread *thread,
				pentry_t *thread_ptables)
{
	ARG_UNUSED(thread_ptables);

	for (uint8_t *pos = (uint8_t *)RAM_BASE; pos < (uint8_t *)RAM_END;
	     pos += CONFIG_MMU_PAGE_SIZE) {
		page_reset(thread, pos);
	}
}
#else
/* get the Nth level paging structure for a particular virtual address */
static pentry_t *page_table_get(pentry_t *toplevel, void *virt, int level)
{
	pentry_t *table = toplevel;

	__ASSERT(level < NUM_LEVELS, "bad level argument %d", level);

	for (int i = 0; i < level; i++) {
		pentry_t entry = get_entry(table, virt, level);

		if ((entry & MMU_P) == 0U) {
			return NULL;
		}
		__ASSERT((entry & MMU_PS) == 0, "bigpage found");
		table = next_table(entry, i);
	}

	return table;
}

/* Get a pointer to the N-th level entry for a particular virtual address */
static pentry_t *page_entry_ptr_get(pentry_t *toplevel, void *virt, int level)
{
	pentry_t *table = page_table_get(toplevel, virt, level);

	__ASSERT(table != NULL, "no table mapping for %p at level %d",
		 virt, level);
	return get_entry_ptr(table, virt, level);
}

 /* Non-KPTI version. The thread-level page tables are used even during
  * interrupts, exceptions, and syscalls, so we need all mappings.
  * Copies will be made of all tables that provide mappings for system RAM,
  * otherwise the kernel table will just be linked instead.
  */
static void setup_thread_tables(struct k_thread *thread,
				pentry_t *thread_ptables)
{
	/* Copy top-level structure verbatim */
	(void)memcpy(thread_ptables, &z_x86_kernel_ptables, table_size(0));

	/* Proceed through linked structure levels, and for all system RAM
	 * virtual addresses, create copies of all relevant tables.
	 */
	for (int level = 1; level < NUM_LEVELS; level++) {
		uint8_t *start, *end;
		size_t increment;

		increment = get_entry_scope(level);
		start = (uint8_t *)ROUND_DOWN(RAM_BASE, increment);
		end = (uint8_t *)ROUND_UP(RAM_END, increment);

		for (uint8_t *virt = start; virt < end; virt += increment) {
			pentry_t *link, *master_table, *user_table;

			/* We're creating a new thread page table, so get the
			 * pointer to the entry in the previous table to have
			 * it point to the new location
			 */
			link = page_entry_ptr_get(thread_ptables, virt,
						  level - 1);

			/* Master table contents, which we make a copy of */
			master_table = page_table_get(&z_x86_kernel_ptables,
						      virt, level);

			/* Pulled out of reserved memory in the stack object */
			user_table = thread_page_pool_get(thread);
			__ASSERT(user_table != NULL, "out of memory")

			(void)memcpy(user_table, master_table,
				     table_size(level));

			*link = ((pentry_t)user_table) | INT_FLAGS;
		}
	}
}
#endif /* CONFIG_X86_KPTI */

/* Called on creation of a user thread or when a supervisor thread drops to
 * user mode.
 *
 * Sets up the per-thread page tables, such that when they are activated on
 * context switch, everything is rseady to go. thread->arch.ptables is updated
 * to the thread-level tables instead of the kernel's page tbales.
 *
 * Memory for the per-thread page table structures is drawn from the stack
 * object, a buffer of size Z_X86_THREAD_PT_AREA starting from the beginning
 * of the stack object.
 */
void z_x86_thread_pt_init(struct k_thread *thread)
{
	pentry_t *ptables;

	/* thread_page_pool_get() memory starts at the beginning of the
	 * stack object
	 */
	assert_virt_addr_aligned(thread->stack_obj);
	thread->arch.mmu_pos = (uint8_t *)thread->stack_obj;

	/* Get memory for the top-level structure */
#ifndef CONFIG_X86_PAE
	ptables = thread_page_pool_get(thread);
	__ASSERT(ptables != NULL, "out of memory");
#else
	struct z_x86_thread_stack_header *header =
		(struct z_x86_thread_stack_header *)thread->stack_obj;

	ptables = (pentry_t *)&header->kernel_data.ptables;
#endif
	thread->arch.ptables = ((uintptr_t)ptables);

	setup_thread_tables(thread, ptables);

	/* Enable access to the thread's own stack buffer */
	thread_map(thread, (void *)thread->stack_info.start,
		   ROUND_UP(thread->stack_info.size,
			    CONFIG_MMU_PAGE_SIZE),
		   MMU_P | MMU_RW | MMU_US | MMU_XD, false);
}

static inline void apply_mem_partition(struct k_thread *thread,
				       struct k_mem_partition *partition)
{
	thread_map(thread, (void *)partition->start, partition->size,
		   partition->attr | MMU_P, false);
}

static void reset_mem_partition(struct k_thread *thread,
				struct k_mem_partition *partition)
{
	uint8_t *addr = (uint8_t *)partition->start;
	size_t size = partition->size;

	assert_region_page_aligned(addr, size);
	for (size_t offset = 0; offset < size; offset += CONFIG_MMU_PAGE_SIZE) {
		page_reset(thread, addr + offset);
	}
}

void z_x86_apply_mem_domain(struct k_thread *thread,
			    struct k_mem_domain *mem_domain)
{
	for (int i = 0, pcount = 0; pcount < mem_domain->num_partitions; i++) {
		struct k_mem_partition *partition;

		partition = &mem_domain->partitions[i];
		if (partition->size == 0) {
			continue;
		}
		pcount++;

		apply_mem_partition(thread, partition);
	}
}

/*
 * Arch interface implementations for memory domains
 *
 * In all cases, if one of these arch_mem_domain_* APIs is called on a
 * supervisor thread, we don't need to do anything. If the thread later drops
 * into user mode the per-thread page tables will be generated and the memory
 * domain configuration applied.
 */
void arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				      uint32_t partition_id)
{
	sys_dnode_t *node, *next_node;

	/* Removing a partition. Need to reset the relevant memory range
	 * to the defaults in USER_PDPT for each thread.
	 */
	SYS_DLIST_FOR_EACH_NODE_SAFE(&domain->mem_domain_q, node, next_node) {
		struct k_thread *thread =
			CONTAINER_OF(node, struct k_thread, mem_domain_info);

		if ((thread->base.user_options & K_USER) == 0) {
			continue;
		}

		reset_mem_partition(thread, &domain->partitions[partition_id]);
	}
}

void arch_mem_domain_destroy(struct k_mem_domain *domain)
{
	for (int i = 0, pcount = 0; pcount < domain->num_partitions; i++) {
		struct k_mem_partition *partition;

		partition = &domain->partitions[i];
		if (partition->size == 0) {
			continue;
		}
		pcount++;

		arch_mem_domain_partition_remove(domain, i);
	}
}

void arch_mem_domain_thread_remove(struct k_thread *thread)
{
	struct k_mem_domain *domain = thread->mem_domain_info.mem_domain;

	/* Non-user threads don't have per-thread page tables set up */
	if ((thread->base.user_options & K_USER) == 0) {
		return;
	}

	for (int i = 0, pcount = 0; pcount < domain->num_partitions; i++) {
		struct k_mem_partition *partition;

		partition = &domain->partitions[i];
		if (partition->size == 0) {
			continue;
		}
		pcount++;

		reset_mem_partition(thread, partition);
	}
}

void arch_mem_domain_partition_add(struct k_mem_domain *domain,
				   uint32_t partition_id)
{
	sys_dnode_t *node, *next_node;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&domain->mem_domain_q, node, next_node) {
		struct k_thread *thread =
			CONTAINER_OF(node, struct k_thread, mem_domain_info);

		if ((thread->base.user_options & K_USER) == 0) {
			continue;
		}

		apply_mem_partition(thread, &domain->partitions[partition_id]);
	}
}

void arch_mem_domain_thread_add(struct k_thread *thread)
{
	if ((thread->base.user_options & K_USER) == 0) {
		return;
	}

	z_x86_apply_mem_domain(thread, thread->mem_domain_info.mem_domain);
}

int arch_mem_domain_max_partitions_get(void)
{
	return CONFIG_MAX_DOMAIN_PARTITIONS;
}
#endif /* CONFIG_USERSPACE */
