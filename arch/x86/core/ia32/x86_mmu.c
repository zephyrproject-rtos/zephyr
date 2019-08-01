/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <ia32/mmustructs.h>
#include <linker/linker-defs.h>
#include <kernel_internal.h>
#include <kernel_structs.h>
#include <init.h>
#include <ctype.h>
#include <string.h>

/* Common regions for all x86 processors.
 * Peripheral I/O ranges configured at the SOC level
 */

/* Mark text and rodata as read-only.
 * Userspace may read all text and rodata.
 */
MMU_BOOT_REGION((u32_t)&_image_text_start, (u32_t)&_image_text_size,
		MMU_ENTRY_READ | MMU_ENTRY_USER);

MMU_BOOT_REGION((u32_t)&_image_rodata_start, (u32_t)&_image_rodata_size,
		MMU_ENTRY_READ | MMU_ENTRY_USER |
		MMU_ENTRY_EXECUTE_DISABLE);

#ifdef CONFIG_USERSPACE
MMU_BOOT_REGION((u32_t)&_app_smem_start, (u32_t)&_app_smem_size,
		MMU_ENTRY_WRITE | MMU_ENTRY_RUNTIME_USER |
		MMU_ENTRY_EXECUTE_DISABLE);
#endif

#ifdef CONFIG_COVERAGE_GCOV
MMU_BOOT_REGION((u32_t)&__gcov_bss_start, (u32_t)&__gcov_bss_size,
		MMU_ENTRY_WRITE | MMU_ENTRY_USER | MMU_ENTRY_EXECUTE_DISABLE);
#endif

/* __kernel_ram_size includes all unused memory, which is used for heaps.
 * User threads cannot access this unless granted at runtime. This is done
 * automatically for stacks.
 */
MMU_BOOT_REGION((u32_t)&__kernel_ram_start, (u32_t)&__kernel_ram_size,
		MMU_ENTRY_WRITE |
		MMU_ENTRY_RUNTIME_USER |
		MMU_ENTRY_EXECUTE_DISABLE);

/* Works for PDPT, PD, PT entries, the bits we check here are all the same.
 *
 * Not trying to capture every flag, just the most interesting stuff,
 * Present, write, XD, user, in typically encountered combinations.
 */
static char get_entry_code(u64_t value)
{
	char ret;

	if ((value & MMU_ENTRY_PRESENT) == 0) {
		ret = '.';
	} else {
		if ((value & MMU_ENTRY_WRITE) != 0) {
			/* Writable page */
			if ((value & MMU_ENTRY_EXECUTE_DISABLE) != 0) {
				/* RW */
				ret = 'w';
			} else {
				/* RWX */
				ret = 'a';
			}
		} else {
			if ((value & MMU_ENTRY_EXECUTE_DISABLE) != 0) {
				/* R */
				ret = 'r';
			} else {
				/* RX */
				ret = 'x';
			}
		}

		if ((value & MMU_ENTRY_USER) != 0) {
			/* Uppercase indicates user mode access */
			ret = toupper(ret);
		}
	}

	return ret;
}

static void z_x86_dump_pt(struct x86_mmu_pt *pt, uintptr_t base, int index)
{
	int column = 0;

	printk("Page table %d for 0x%08lX - 0x%08lX at %p\n",
	       index, base, base + Z_X86_PT_AREA - 1, pt);

	for (int i = 0; i < Z_X86_NUM_PT_ENTRIES; i++) {
		printk("%c", get_entry_code(pt->entry[i].value));

		column++;
		if (column == 64) {
			column = 0;
			printk("\n");
		}
	}
}

static void z_x86_dump_pd(struct x86_mmu_pd *pd, uintptr_t base, int index)
{
	int column = 0;

	printk("Page directory %d for 0x%08lX - 0x%08lX at %p\n",
	       index, base, base + Z_X86_PD_AREA - 1, pd);

	for (int i = 0; i < Z_X86_NUM_PD_ENTRIES; i++) {
		printk("%c", get_entry_code(pd->entry[i].pt.value));

		column++;
		if (column == 64) {
			column = 0;
			printk("\n");
		}
	}

	for (int i = 0; i < Z_X86_NUM_PD_ENTRIES; i++) {
		struct x86_mmu_pt *pt;
		union x86_mmu_pde_pt *pde = &pd->entry[i].pt;

		if (pde->p == 0 || pde->ps == 1) {
			/* Skip non-present, or 2MB directory entries, there's
			 * no page table to examine */
			continue;
		}
		pt = (struct x86_mmu_pt *)(pde->pt << MMU_PAGE_SHIFT);

		z_x86_dump_pt(pt, base + (i * Z_X86_PT_AREA), i);
	}
}

static void z_x86_dump_pdpt(struct x86_mmu_pdpt *pdpt, uintptr_t base,
			    int index)
{
	printk("Page directory pointer table %d for 0x%08lX - 0x%08lX at %p\n",
	       index, base, base + Z_X86_PDPT_AREA - 1, pdpt);

	for (int i = 0; i < Z_X86_NUM_PDPT_ENTRIES; i++) {
		printk("%c", get_entry_code(pdpt->entry[i].value));
	}
	printk("\n");
	for (int i = 0; i < Z_X86_NUM_PDPT_ENTRIES; i++) {
		struct x86_mmu_pd *pd;

		if (pdpt->entry[i].p == 0) {
			continue;
		}
		pd = (struct x86_mmu_pd *)(pdpt->entry[i].pd << MMU_PAGE_SHIFT);

		z_x86_dump_pd(pd, base + (i * Z_X86_PD_AREA), i);
	}
}

void z_x86_dump_page_tables(struct x86_mmu_pdpt *pdpt)
{
	z_x86_dump_pdpt(pdpt, 0, 0);
}

void z_x86_mmu_get_flags(struct x86_mmu_pdpt *pdpt, void *addr,
			x86_page_entry_data_t *pde_flags,
			x86_page_entry_data_t *pte_flags)
{
	*pde_flags =
		(x86_page_entry_data_t)(X86_MMU_GET_PDE(pdpt, addr)->value &
			~(x86_page_entry_data_t)MMU_PDE_PAGE_TABLE_MASK);

	if ((*pde_flags & MMU_ENTRY_PRESENT) != 0) {
		*pte_flags = (x86_page_entry_data_t)
			(X86_MMU_GET_PTE(pdpt, addr)->value &
			 ~(x86_page_entry_data_t)MMU_PTE_PAGE_MASK);
	} else {
		*pte_flags = 0;
	}
}

int z_x86_mmu_validate(struct x86_mmu_pdpt *pdpt, void *addr, size_t size,
		       int write)
{
	u32_t start_pde_num;
	u32_t end_pde_num;
	u32_t starting_pte_num;
	u32_t ending_pte_num;
	u32_t pde;
	u32_t pte;
	union x86_mmu_pte pte_value;
	u32_t start_pdpte_num = MMU_PDPTE_NUM(addr);
	u32_t end_pdpte_num = MMU_PDPTE_NUM((char *)addr + size - 1);
	u32_t pdpte;
	struct x86_mmu_pt *pte_address;
	int ret = -EPERM;

	start_pde_num = MMU_PDE_NUM(addr);
	end_pde_num = MMU_PDE_NUM((char *)addr + size - 1);
	starting_pte_num = MMU_PAGE_NUM((char *)addr);

	for (pdpte = start_pdpte_num; pdpte <= end_pdpte_num; pdpte++) {
		if (pdpte != start_pdpte_num) {
			start_pde_num = 0U;
		}

		if (pdpte != end_pdpte_num) {
			end_pde_num = 0U;
		} else {
			end_pde_num = MMU_PDE_NUM((char *)addr + size - 1);
		}

		/* Ensure page directory pointer table entry is present */
		if (X86_MMU_GET_PDPTE_INDEX(pdpt, pdpte)->p == 0) {
			goto out;
		}

		struct x86_mmu_pd *pd_address =
			X86_MMU_GET_PD_ADDR_INDEX(pdpt, pdpte);

		/* Iterate for all the pde's the buffer might take up.
		 * (depends on the size of the buffer and start address
		 * of the buff)
		 */
		for (pde = start_pde_num; pde <= end_pde_num; pde++) {
			union x86_mmu_pde_pt pde_value =
				pd_address->entry[pde].pt;

			if ((pde_value.p) == 0 ||
			    (pde_value.us) == 0 ||
			    ((write != 0) && (pde_value.rw == 0))) {
				goto out;
			}

			pte_address = (struct x86_mmu_pt *)
				(pde_value.pt << MMU_PAGE_SHIFT);

			/* loop over all the possible page tables for the
			 * required size. If the pde is not the last one
			 * then the last pte would be 511. So each pde
			 * will be using all the page table entries except
			 * for the last pde. For the last pde, pte is
			 * calculated using the last memory address
			 * of the buffer.
			 */
			if (pde != end_pde_num) {
				ending_pte_num = 511U;
			} else {
				ending_pte_num =
					MMU_PAGE_NUM((char *)addr + size - 1);
			}

			/* For all the pde's apart from the starting pde,
			 * will have the start pte number as zero.
			 */
			if (pde != start_pde_num) {
				starting_pte_num = 0U;
			}

			pte_value.value = 0xFFFFFFFFU;

			/* Bitwise AND all the pte values.
			 * An optimization done to make sure a compare is
			 * done only once.
			 */
			for (pte = starting_pte_num;
			     pte <= ending_pte_num;
			     pte++) {
				pte_value.value &=
					pte_address->entry[pte].value;
			}

			if ((pte_value.p) == 0 ||
			    (pte_value.us) == 0 ||
			    ((write != 0) && (pte_value.rw == 0))) {
				goto out;
			}
		}
	}
	ret = 0;
out:
#ifdef CONFIG_X86_BOUNDS_CHECK_BYPASS_MITIGATION
	__asm__ volatile ("lfence" : : : "memory");
#endif

	return ret;
}

static inline void tlb_flush_page(void *addr)
{
	/* Invalidate TLB entries corresponding to the page containing the
	 * specified address
	 */
	char *page = (char *)addr;

	__asm__ ("invlpg %0" :: "m" (*page));
}


void z_x86_mmu_set_flags(struct x86_mmu_pdpt *pdpt, void *ptr, size_t size,
			 x86_page_entry_data_t flags,
			 x86_page_entry_data_t mask, bool flush)
{
	union x86_mmu_pte *pte;

	u32_t addr = (u32_t)ptr;

	__ASSERT((addr & MMU_PAGE_MASK) == 0U, "unaligned address provided");
	__ASSERT((size & MMU_PAGE_MASK) == 0U, "unaligned size provided");

	/* L1TF mitigation: non-present PTEs will have address fields
	 * zeroed. Expand the mask to include address bits if we are changing
	 * the present bit.
	 */
	if ((mask & MMU_PTE_P_MASK) != 0) {
		mask |= MMU_PTE_PAGE_MASK;
	}

	while (size != 0) {
		x86_page_entry_data_t cur_flags = flags;

		/* TODO we're not generating 2MB entries at the moment */
		__ASSERT(X86_MMU_GET_PDE(pdpt, addr)->ps != 1, "2MB PDE found");
		pte = X86_MMU_GET_PTE(pdpt, addr);

		/* If we're setting the present bit, restore the address
		 * field. If we're clearing it, then the address field
		 * will be zeroed instead, mapping the PTE to the NULL page.
		 */
		if (((mask & MMU_PTE_P_MASK) != 0) &&
		    ((flags & MMU_ENTRY_PRESENT) != 0)) {
			cur_flags |= addr;
		}

		pte->value = (pte->value & ~mask) | cur_flags;
		if (flush) {
			tlb_flush_page((void *)addr);
		}

		size -= MMU_PAGE_SIZE;
		addr += MMU_PAGE_SIZE;
	}
}

#ifdef CONFIG_X86_USERSPACE
int z_arch_buffer_validate(void *addr, size_t size, int write)
{
	return z_x86_mmu_validate(z_x86_pdpt_get(_current), addr, size, write);
}

static uintptr_t thread_pd_create(uintptr_t pages,
				  struct x86_mmu_pdpt *thread_pdpt,
				  struct x86_mmu_pdpt *master_pdpt)
{
	uintptr_t pos = pages, phys_addr = Z_X86_PD_START;

	for (int i = 0; i < Z_X86_NUM_PD; i++, phys_addr += Z_X86_PD_AREA) {
		union x86_mmu_pdpte *pdpte;
		struct x86_mmu_pd *master_pd, *dest_pd;

		/* Obtain PD in master tables for the address range and copy
		 * into the per-thread PD for this range
		 */
		master_pd = X86_MMU_GET_PD_ADDR(master_pdpt, phys_addr);
		dest_pd = (struct x86_mmu_pd *)pos;

		(void)memcpy(dest_pd, master_pd, sizeof(struct x86_mmu_pd));

		/* Update pointer in per-thread pdpt to point to the per-thread
		 * directory we just copied
		 */
		pdpte = X86_MMU_GET_PDPTE(thread_pdpt, phys_addr);
		pdpte->pd = pos >> MMU_PAGE_SHIFT;
		pos += MMU_PAGE_SIZE;
	}

	return pos;
}

/* thread_pdpt must be initialized, as well as all the page directories */
static uintptr_t thread_pt_create(uintptr_t pages,
				  struct x86_mmu_pdpt *thread_pdpt,
				  struct x86_mmu_pdpt *master_pdpt)
{
	uintptr_t pos = pages, phys_addr = Z_X86_PT_START;

	for (int i = 0; i < Z_X86_NUM_PT; i++, phys_addr += Z_X86_PT_AREA) {
		union x86_mmu_pde_pt *pde;
		struct x86_mmu_pt *master_pt, *dest_pt;

		/* Same as we did with the directories, obtain PT in master
		 * tables for the address range and copy into per-thread PT
		 * for this range
		 */
		master_pt = X86_MMU_GET_PT_ADDR(master_pdpt, phys_addr);
		dest_pt = (struct x86_mmu_pt *)pos;
		(void)memcpy(dest_pt, master_pt, sizeof(struct x86_mmu_pd));

		/* And then wire this up to the relevant per-thread
		 * page directory entry
		 */
		pde = X86_MMU_GET_PDE(thread_pdpt, phys_addr);
		pde->pt = pos >> MMU_PAGE_SHIFT;
		pos += MMU_PAGE_SIZE;
	}

	return pos;
}

/* Initialize the page tables for a thread. This will contain, once done,
 * the boot-time configuration for a user thread page tables. There are
 * no pre-conditions on the existing state of the per-thread tables.
 */
static void copy_page_tables(struct k_thread *thread,
			     struct x86_mmu_pdpt *master_pdpt)
{
	uintptr_t pos, start;
	struct x86_mmu_pdpt *thread_pdpt = z_x86_pdpt_get(thread);
	struct z_x86_thread_stack_header *header =
		(struct z_x86_thread_stack_header *)thread->stack_obj;

	__ASSERT(thread->stack_obj != NULL, "no stack object assigned");
	__ASSERT(z_x86_page_tables_get() != thread_pdpt, "PDPT is active");
	__ASSERT(((uintptr_t)thread_pdpt & 0x1f) == 0, "unaligned pdpt at %p",
		 thread_pdpt);

	(void)memcpy(thread_pdpt, master_pdpt, sizeof(struct x86_mmu_pdpt));

	/* pos represents the page we are working with in the reserved area
	 * in the stack buffer for per-thread tables. As we create tables in
	 * this area, pos is incremented to the next free page.
	 *
	 * The layout of the stack object, when this is done:
	 *
	 * +---------------------------+  <- thread->stack_obj
	 * | PDE(0)                    |
	 * +---------------------------+
	 * | ...                       |
	 * +---------------------------+
	 * | PDE(Z_X86_NUM_PD - 1)     |
	 * +---------------------------+
	 * | PTE(0)                    |
	 * +---------------------------+
	 * | ...                       |
	 * +---------------------------+
	 * | PTE(Z_X86_NUM_PT - 1)     |
	 * +---------------------------+ <- pos once this logic completes
	 * | Stack guard               |
	 * +---------------------------+
	 * | Privilege elevation stack |
	 * | PDPT                      |
	 * +---------------------------+ <- thread->stack_info.start
	 * | Thread stack              |
	 * | ...                       |
	 *
	 */
	start = (uintptr_t)(&header->page_tables);
	pos = thread_pd_create(start, thread_pdpt, master_pdpt);
	pos = thread_pt_create(pos, thread_pdpt, master_pdpt);

	__ASSERT(pos == (start + Z_X86_THREAD_PT_AREA),
		 "wrong amount of stack object memory used");
}

static void reset_mem_partition(struct x86_mmu_pdpt *thread_pdpt,
				struct k_mem_partition *partition)
{
	uintptr_t addr = partition->start;
	size_t size = partition->size;

	__ASSERT((addr & MMU_PAGE_MASK) == 0U, "unaligned address provided");
	__ASSERT((size & MMU_PAGE_MASK) == 0U, "unaligned size provided");

	while (size != 0) {
		union x86_mmu_pte *thread_pte, *master_pte;

		thread_pte = X86_MMU_GET_PTE(thread_pdpt, addr);
		master_pte = X86_MMU_GET_PTE(&USER_PDPT, addr);

		(void)memcpy(thread_pte, master_pte, sizeof(union x86_mmu_pte));

		size -= MMU_PAGE_SIZE;
		addr += MMU_PAGE_SIZE;
	}
}

static void apply_mem_partition(struct x86_mmu_pdpt *pdpt,
				struct k_mem_partition *partition)
{
	x86_page_entry_data_t x86_attr;
	x86_page_entry_data_t mask;

	if (IS_ENABLED(CONFIG_X86_KPTI)) {
		x86_attr = partition->attr | MMU_ENTRY_PRESENT;
		mask = K_MEM_PARTITION_PERM_MASK | MMU_PTE_P_MASK;
	} else {
		x86_attr = partition->attr;
		mask = K_MEM_PARTITION_PERM_MASK;
	}

	__ASSERT(partition->start >= DT_PHYS_RAM_ADDR,
		 "region at %08lx[%u] extends below system ram start 0x%08x",
		 partition->start, partition->size, DT_PHYS_RAM_ADDR);
	__ASSERT(((partition->start + partition->size) <=
		  (DT_PHYS_RAM_ADDR + (DT_RAM_SIZE * 1024U))),
		 "region at %08lx[%u] end at %08lx extends beyond system ram end 0x%08x",
		 partition->start, partition->size,
		 partition->start + partition->size,
		 (DT_PHYS_RAM_ADDR + (DT_RAM_SIZE * 1024U)));

	z_x86_mmu_set_flags(pdpt, (void *)partition->start, partition->size,
			    x86_attr, mask, false);
}

void z_x86_apply_mem_domain(struct x86_mmu_pdpt *pdpt,
			    struct k_mem_domain *mem_domain)
{
	for (int i = 0, pcount = 0; pcount < mem_domain->num_partitions; i++) {
		struct k_mem_partition *partition;

		partition = &mem_domain->partitions[i];
		if (partition->size == 0) {
			continue;
		}
		pcount++;

		apply_mem_partition(pdpt, partition);
	}
}

/* Called on creation of a user thread or when a supervisor thread drops to
 * user mode.
 *
 * Sets up the per-thread page tables, such that when they are activated on
 * context switch, everything is ready to go.
 */
void z_x86_thread_pt_init(struct k_thread *thread)
{
	struct x86_mmu_pdpt *pdpt = z_x86_pdpt_get(thread);

	/* USER_PDPT contains the page tables with the boot time memory
	 * policy. We use it as a template to set up the per-thread page
	 * tables.
	 *
	 * With KPTI, this is a distinct set of tables z_x86_user_pdpt from the
	 * kernel page tables in z_x86_kernel_pdpt; it has all non user
	 * accessible pages except the trampoline page marked as non-present.
	 * Without KPTI, they are the same object.
	 */
	copy_page_tables(thread, &USER_PDPT);

	/* Enable access to the thread's own stack buffer */
	z_x86_mmu_set_flags(pdpt, (void *)thread->stack_info.start,
			    ROUND_UP(thread->stack_info.size, MMU_PAGE_SIZE),
			    MMU_ENTRY_PRESENT | K_MEM_PARTITION_P_RW_U_RW,
			    MMU_PTE_P_MASK | K_MEM_PARTITION_PERM_MASK,
			    false);
}

/*
 * Memory domain interface
 *
 * In all cases, if one of these APIs is called on a supervisor thread,
 * we don't need to do anything. If the thread later drops into supervisor
 * mode the per-thread page tables will be generated and the memory domain
 * configuration applied.
 */
void z_arch_mem_domain_partition_remove(struct k_mem_domain *domain,
					u32_t partition_id)
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

		reset_mem_partition(z_x86_pdpt_get(thread),
				    &domain->partitions[partition_id]);
	}
}

void z_arch_mem_domain_destroy(struct k_mem_domain *domain)
{
	for (int i = 0, pcount = 0; pcount < domain->num_partitions; i++) {
		struct k_mem_partition *partition;

		partition = &domain->partitions[i];
		if (partition->size == 0) {
			continue;
		}
		pcount++;

		z_arch_mem_domain_partition_remove(domain, i);
	}
}

void z_arch_mem_domain_thread_remove(struct k_thread *thread)
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

		reset_mem_partition(z_x86_pdpt_get(thread), partition);
	}
}

void z_arch_mem_domain_partition_add(struct k_mem_domain *domain,
				     u32_t partition_id)
{
	sys_dnode_t *node, *next_node;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&domain->mem_domain_q, node, next_node) {
		struct k_thread *thread =
			CONTAINER_OF(node, struct k_thread, mem_domain_info);

		if ((thread->base.user_options & K_USER) == 0) {
			continue;
		}

		apply_mem_partition(z_x86_pdpt_get(thread),
				    &domain->partitions[partition_id]);
	}
}

void z_arch_mem_domain_thread_add(struct k_thread *thread)
{
	if ((thread->base.user_options & K_USER) == 0) {
		return;
	}

	z_x86_apply_mem_domain(z_x86_pdpt_get(thread),
			       thread->mem_domain_info.mem_domain);
}

int z_arch_mem_domain_max_partitions_get(void)
{
	return CONFIG_MAX_DOMAIN_PARTITIONS;
}
#endif	/* CONFIG_X86_USERSPACE*/
