/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <mmustructs.h>
#include <linker/linker-defs.h>
#include <kernel_internal.h>
#include <init.h>

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


void _x86_mmu_get_flags(struct x86_mmu_pdpt *pdpt, void *addr,
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


int _arch_buffer_validate(void *addr, size_t size, int write)
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
		if (X86_MMU_GET_PDPTE_INDEX(&USER_PDPT, pdpte)->p == 0) {
			return -EPERM;
		}

		struct x86_mmu_pd *pd_address =
			X86_MMU_GET_PD_ADDR_INDEX(&USER_PDPT, pdpte);

		/* Iterate for all the pde's the buffer might take up.
		 * (depends on the size of the buffer and start address
		 * of the buff)
		 */
		for (pde = start_pde_num; pde <= end_pde_num; pde++) {
			union x86_mmu_pde_pt pde_value =
				pd_address->entry[pde].pt;

			if (!pde_value.p ||
			    !pde_value.us ||
			    (write && !pde_value.rw)) {
				return -EPERM;
			}

			pte_address = (struct x86_mmu_pt *)
				(pde_value.pt << MMU_PAGE_SHIFT);

			/* loop over all the possible page tables for the
			 * required size. If the pde is not the last one
			 * then the last pte would be 511. So each pde
			 * will be using all the page table entires except
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

			/* For all the pde's appart from the starting pde,
			 * will have the start pte number as zero.
			 */
			if (pde != start_pde_num) {
				starting_pte_num = 0U;
			}

			pte_value.value = 0xFFFFFFFF;

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

			if (!pte_value.p ||
			    !pte_value.us ||
			    (write && !pte_value.rw)) {
				return -EPERM;
			}
		}
	}

	return 0;
}

static inline void tlb_flush_page(void *addr)
{
	/* Invalidate TLB entries corresponding to the page containing the
	 * specified address
	 */
	char *page = (char *)addr;

	__asm__ ("invlpg %0" :: "m" (*page));
}


void _x86_mmu_set_flags(struct x86_mmu_pdpt *pdpt, void *ptr,
			size_t size,
			x86_page_entry_data_t flags,
			x86_page_entry_data_t mask)
{
	union x86_mmu_pte *pte;

	u32_t addr = (u32_t)ptr;

	__ASSERT(!(addr & MMU_PAGE_MASK), "unaligned address provided");
	__ASSERT(!(size & MMU_PAGE_MASK), "unaligned size provided");

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
		tlb_flush_page((void *)addr);

		size -= MMU_PAGE_SIZE;
		addr += MMU_PAGE_SIZE;
	}
}

#ifdef CONFIG_X86_USERSPACE
void z_x86_reset_pages(void *start, size_t size)
{
#ifdef CONFIG_X86_KPTI
	/* Clear both present bit and access flags. Only applies
	 * to threads running in user mode.
	 */
	_x86_mmu_set_flags(&z_x86_user_pdpt, start, size,
			   MMU_ENTRY_NOT_PRESENT,
			   K_MEM_PARTITION_PERM_MASK | MMU_PTE_P_MASK);
#else
	/* Mark as supervisor read-write, user mode no access */
	_x86_mmu_set_flags(&z_x86_kernel_pdpt, start, size,
			   K_MEM_PARTITION_P_RW_U_NA,
			   K_MEM_PARTITION_PERM_MASK);
#endif /* CONFIG_X86_KPTI */
}

static inline void activate_partition(struct k_mem_partition *partition)
{
	/* Set the partition attributes */
	u64_t attr, mask;

#if CONFIG_X86_KPTI
	attr = partition->attr | MMU_ENTRY_PRESENT;
	mask = K_MEM_PARTITION_PERM_MASK | MMU_PTE_P_MASK;
#else
	attr = partition->attr;
	mask = K_MEM_PARTITION_PERM_MASK;
#endif /* CONFIG_X86_KPTI */

	_x86_mmu_set_flags(&USER_PDPT,
			   (void *)partition->start,
			   partition->size, attr, mask);
}

/* Helper macros needed to be passed to x86_update_mem_domain_pages */
#define X86_MEM_DOMAIN_SET_PAGES   (0U)
#define X86_MEM_DOMAIN_RESET_PAGES (1U)
/* Pass 1 to page_conf if reset of mem domain pages is needed else pass a 0*/
static inline void _x86_mem_domain_pages_update(struct k_mem_domain *mem_domain,
						u32_t page_conf)
{
	u32_t partition_index;
	u32_t total_partitions;
	struct k_mem_partition *partition;
	u32_t partitions_count;

	/* If mem_domain doesn't point to a valid location return.*/
	if (mem_domain == NULL) {
		goto out;
	}

	/* Get the total number of partitions*/
	total_partitions = mem_domain->num_partitions;

	/* Iterate over all the partitions for the given mem_domain
	 * For x86: interate over all the partitions and set the
	 * required flags in the correct MMU page tables.
	 */
	partitions_count = 0U;
	for (partition_index = 0U;
	     partitions_count < total_partitions;
	     partition_index++) {

		/* Get the partition info */
		partition = &mem_domain->partitions[partition_index];
		if (partition->size == 0) {
			continue;
		}
		partitions_count++;
		if (page_conf == X86_MEM_DOMAIN_SET_PAGES) {
			activate_partition(partition);
		} else {
			z_x86_reset_pages((void *)partition->start,
					  partition->size);
		}
	}
out:
	return;
}

/* Load the partitions of the thread. */
void _arch_mem_domain_configure(struct k_thread *thread)
{
	_x86_mem_domain_pages_update(thread->mem_domain_info.mem_domain,
				     X86_MEM_DOMAIN_SET_PAGES);
}

/* Destroy or reset the mmu page tables when necessary.
 * Needed when either swap takes place or k_mem_domain_destroy is called.
 */
void _arch_mem_domain_destroy(struct k_mem_domain *domain)
{
	_x86_mem_domain_pages_update(domain, X86_MEM_DOMAIN_RESET_PAGES);
}

/* Reset/destroy one partition spcified in the argument of the API. */
void _arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				       u32_t partition_id)
{
	struct k_mem_partition *partition;

	__ASSERT_NO_MSG(domain != NULL);
	__ASSERT(partition_id <= domain->num_partitions,
		 "invalid partitions");

	partition = &domain->partitions[partition_id];
	z_x86_reset_pages((void *)partition->start, partition->size);
}

/* Reset/destroy one partition spcified in the argument of the API. */
void _arch_mem_domain_partition_add(struct k_mem_domain *domain,
				    u32_t partition_id)
{
	struct k_mem_partition *partition;

	__ASSERT_NO_MSG(domain != NULL);
	__ASSERT(partition_id <= domain->num_partitions,
		 "invalid partitions");

	partition = &domain->partitions[partition_id];
	activate_partition(partition);
}

int _arch_mem_domain_max_partitions_get(void)
{
	return CONFIG_MAX_DOMAIN_PARTITIONS;
}
#endif	/* CONFIG_X86_USERSPACE*/
