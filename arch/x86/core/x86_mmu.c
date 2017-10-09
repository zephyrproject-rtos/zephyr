/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <mmustructs.h>
#include <linker/linker-defs.h>

/* Common regions for all x86 processors.
 * Peripheral I/O ranges configured at the SOC level
 */

/* Mark text and rodata as read-only.
 * Userspace may read all text and rodata.
 */
MMU_BOOT_REGION((u32_t)&_image_rom_start, (u32_t)&_image_rom_size,
		MMU_ENTRY_READ | MMU_ENTRY_USER);

#ifdef CONFIG_APPLICATION_MEMORY
/* User threads by default can read/write app-level memory. */
MMU_BOOT_REGION((u32_t)&__app_ram_start, (u32_t)&__app_ram_size,
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


void _x86_mmu_get_flags(void *addr,
			x86_page_entry_data_t *pde_flags,
			x86_page_entry_data_t *pte_flags)
{
	*pde_flags = (x86_page_entry_data_t)(X86_MMU_GET_PDE(addr)->value &
				~(x86_page_entry_data_t)MMU_PDE_PAGE_TABLE_MASK);

	if (*pde_flags & MMU_ENTRY_PRESENT) {
		*pte_flags = (x86_page_entry_data_t)
			(X86_MMU_GET_PTE(addr)->value &
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
#ifdef CONFIG_X86_PAE_MODE
	union x86_mmu_pae_pte pte_value;
	u32_t start_pdpte_num = MMU_PDPTE_NUM(addr);
	u32_t end_pdpte_num = MMU_PDPTE_NUM((char *)addr + size - 1);
	u32_t pdpte;
#else
	union x86_mmu_pte pte_value;
#endif
	struct x86_mmu_page_table *pte_address;


	start_pde_num = MMU_PDE_NUM(addr);
	end_pde_num = MMU_PDE_NUM((char *)addr + size - 1);
	starting_pte_num = MMU_PAGE_NUM((char *)addr);

#ifdef CONFIG_X86_PAE_MODE
	for (pdpte = start_pdpte_num; pdpte <= end_pdpte_num; pdpte++) {
		if (pdpte != start_pdpte_num) {
			start_pde_num = 0;
		}

		if (pdpte != end_pdpte_num) {
			end_pde_num = 0;
		} else {
			end_pde_num = MMU_PDE_NUM((char *)addr + size - 1);
		}

		struct x86_mmu_page_directory *pd_address =
			X86_MMU_GET_PD_ADDR_INDEX(pdpte);
#endif
		/* Iterate for all the pde's the buffer might take up.
		 * (depends on the size of the buffer and start address
		 * of the buff)
		 */
		for (pde = start_pde_num; pde <= end_pde_num; pde++) {
#ifdef CONFIG_X86_PAE_MODE
			union x86_mmu_pae_pde pde_value =
				pd_address->entry[pde];
#else
			union x86_mmu_pde_pt pde_value =
				X86_MMU_PD->entry[pde].pt;
#endif

			if (!pde_value.p ||
			    !pde_value.us ||
			    (write && !pde_value.rw)) {
				return -EPERM;
			}

			pte_address = (struct x86_mmu_page_table *)
				(pde_value.page_table << MMU_PAGE_SHIFT);

			/* loop over all the possible page tables for the
			 * required size. If the pde is not the last one
			 * then the last pte would be 1023. So each pde
			 * will be using all the page table entires except
			 * for the last pde. For the last pde, pte is
			 * calculated using the last memory address
			 * of the buffer.
			 */
			if (pde != end_pde_num) {
				ending_pte_num = 1023;
			} else {
				ending_pte_num =
					MMU_PAGE_NUM((char *)addr + size - 1);
			}

			/* For all the pde's appart from the starting pde,
			 * will have the start pte number as zero.
			 */
			if (pde != start_pde_num) {
				starting_pte_num = 0;
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
#ifdef CONFIG_X86_PAE_MODE
	}
#endif

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


void _x86_mmu_set_flags(void *ptr,
			size_t size,
			x86_page_entry_data_t flags,
			x86_page_entry_data_t mask)
{
#ifdef CONFIG_X86_PAE_MODE
	union x86_mmu_pae_pte *pte;
#else
	union x86_mmu_pte *pte;
#endif

	u32_t addr = (u32_t)ptr;

	__ASSERT(!(addr & MMU_PAGE_MASK), "unaligned address provided");
	__ASSERT(!(size & MMU_PAGE_MASK), "unaligned size provided");

	while (size) {

#ifdef CONFIG_X86_PAE_MODE
		/* TODO we're not generating 2MB entries at the moment */
		__ASSERT(X86_MMU_GET_PDE(addr)->ps != 1, "2MB PDE found");
#else
		/* TODO we're not generating 4MB entries at the moment */
		__ASSERT(X86_MMU_GET_4MB_PDE(addr)->ps != 1, "4MB PDE found");
#endif
		pte = X86_MMU_GET_PTE(addr);

		pte->value = (pte->value & ~mask) | flags;
		tlb_flush_page((void *)addr);

		size -= MMU_PAGE_SIZE;
		addr += MMU_PAGE_SIZE;
	}
}

#ifdef CONFIG_X86_USERSPACE

/* Helper macros needed to be passed to x86_update_mem_domain_pages */
#define X86_MEM_DOMAIN_SET_PAGES   (0U)
#define X86_MEM_DOMAIN_RESET_PAGES (1U)
/* Pass 1 to page_conf if reset of mem domain pages is needed else pass a 0*/
static inline void _x86_mem_domain_pages_update(struct k_mem_domain *mem_domain,
						u32_t page_conf)
{
	u32_t partition_index;
	u32_t total_partitions;
	struct k_mem_partition partition;
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
	partitions_count = 0;
	for (partition_index = 0;
	     partitions_count < total_partitions;
	     partition_index++) {

		/* Get the partition info */
		partition = mem_domain->partitions[partition_index];
		if (partition.size == 0) {
			continue;
		}
		partitions_count++;
		if (page_conf == X86_MEM_DOMAIN_SET_PAGES) {
			/* Set the partition attributes */
			_x86_mmu_set_flags((void *)partition.start,
					   partition.size,
					   partition.attr,
					   K_MEM_PARTITION_PERM_MASK);
		} else {
			/* Reset the pages to supervisor RW only */
			_x86_mmu_set_flags((void *)partition.start,
					   partition.size,
					   K_MEM_PARTITION_P_RW_U_NA,
					   K_MEM_PARTITION_PERM_MASK);
		}
	}
 out:
	return;
}


/* Load the required parttions of the new incoming thread */
void _x86_mmu_mem_domain_load(struct k_thread *thread)
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
				       u32_t  partition_id)
{
	u32_t total_partitions;
	struct k_mem_partition partition;

	if (domain == NULL) {
		goto out;
	}

	total_partitions = domain->num_partitions;
	__ASSERT(partition_id <= total_partitions,
		 "invalid partitions");

	partition = domain->partitions[partition_id];

	_x86_mmu_set_flags((void *)partition.start,
			   partition.size,
			   K_MEM_PARTITION_P_RW_U_NA,
			   K_MEM_PARTITION_PERM_MASK);

 out:
	return;
}

u8_t _arch_mem_domain_max_partitions_get(void)
{
	return CONFIG_MAX_DOMAIN_PARTITIONS;
}

#endif	/* CONFIG_X86_USERSPACE*/
