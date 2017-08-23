/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <mmustructs.h>
#include <linker/linker-defs.h>

/* Ref to _x86_mmu_buffer_validate documentation for details  */
#define USER_PERM_BIT_POS ((u32_t)0x1)
#define GET_RW_PERM(flags) (flags & BUFF_WRITEABLE)
#define GET_US_PERM(flags) ((flags & BUFF_USER) >> USER_PERM_BIT_POS)

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
		MMU_ENTRY_WRITE | MMU_ENTRY_USER);
#endif

/* __kernel_ram_size includes all unused memory, which is used for heaps.
 * User threads cannot access this unless granted at runtime. This is done
 * automatically for stacks.
 */
MMU_BOOT_REGION((u32_t)&__kernel_ram_start, (u32_t)&__kernel_ram_size,
		MMU_ENTRY_WRITE | MMU_ENTRY_RUNTIME_USER);

/**
 * brief check page directory flags
 *
 * This routine reads the flags of the pde and then compares it to the
 * rw_permission and us_permissions.
 *
 * return true-if the permissions of the pde matches the request
 */
static inline u32_t check_pde_flags(volatile union x86_mmu_pde_pt pde,
				    u32_t rw_permission,
				    u32_t us_permission)
{

	/*  If rw bit is 0 then read is enabled else if 1 then
	 *  read-write access is enabled. (flags & 0x1) returns a
	 *  RW permission required. if READ is requested and the rw bit says
	 *  it has write permission this passes through. But if a write
	 *  permission is requested and rw bit says only read is
	 *  allowed then this fails.
	 */

	/*  The privilage permisions possible combinations between request and
	 *  the the mmu configuraiont are
	 *  s s -> both supervisor                         = valid
	 *  s u -> PDE is supervisor and requested is user = invalid
	 *  u s -> PDE is user and requested is supervisor = valid
	 *  u u -> both user                               = valid
	 */

	return(pde.p &&
		(pde.rw >= rw_permission)  &&
		!(pde.us < us_permission));
}

/**
 * brief check page table entry flags
 *
 * This routine reads the flags of the pte and then compares it to the
 * rw_permission and us_permissions.
 *
 * return true-if the permissions of the pde matches the request
 */
static inline u32_t check_pte_flags(union x86_mmu_pte pte,
				    u32_t rw_permission,
				    u32_t us_permission)
{
	/* Ref to check_pde_flag for doc */
	return(pte.p &&
		(pte.rw >= rw_permission)  &&
		!(pte.us < us_permission));
}


void _x86_mmu_get_flags(void *addr, u32_t *pde_flags, u32_t *pte_flags)
{

	*pde_flags = X86_MMU_GET_PDE(addr)->value & ~MMU_PDE_PAGE_TABLE_MASK;
	*pte_flags = X86_MMU_GET_PTE(addr)->value & ~MMU_PTE_PAGE_MASK;
}


/**
 * @brief check page table entry flags
 *
 * This routine checks if the buffer is avaialable to the whoever calls
 * this API.
 * @a addr start address of the buffer
 * @a size the size of the buffer
 * @a flags permisions to check.
 *    Consists of 2 bits the bit0 represents the RW permissions
 *    The bit1 represents the user/supervisor permissions
 *    Use macro BUFF_READABLE/BUFF_WRITEABLE or BUFF_USER to build the flags
 *
 * @return true-if the permissions of the pde matches the request
 */
int _x86_mmu_buffer_validate(void *addr, size_t size, int flags)
{
	int status = 0;
	u32_t start_pde_num;
	u32_t end_pde_num;
	/* union x86_mmu_pde_pt pde_status; */
	volatile u32_t validity = 1;
	u32_t starting_pte_num;
	u32_t ending_pte_num;
	struct x86_mmu_page_table *pte_address;
	u32_t rw_permission;
	u32_t us_permission;
	u32_t pde;
	u32_t pte;
	union x86_mmu_pte pte_value;

	start_pde_num = MMU_PDE_NUM(addr);
	end_pde_num = MMU_PDE_NUM((char *)addr + size - 1);
	rw_permission = GET_RW_PERM(flags);
	us_permission = GET_US_PERM(flags);
	starting_pte_num = MMU_PAGE_NUM((char *)addr);

	/* Iterate for all the pde's the buffer might take up.
	 * (depends on the size of the buffer and start address of the buff)
	 */
	for (pde = start_pde_num; pde <= end_pde_num; pde++) {
		validity &= check_pde_flags(X86_MMU_PD->entry[pde].pt,
					   rw_permission,
					   us_permission);

		/* If pde is invalid exit immediately. */
		if (validity == 0) {
			break;
		}

		/* Get address of the page table from the pde.
		 * This is a 20 bit address, so shift it by 12.
		 * This gives us 4KB aligned page table.
		 */
		pte_address = (struct x86_mmu_page_table *)
			(X86_MMU_PD->entry[pde].pt.page_table << 12);

		/* loop over all the possible page tables for the required
		 * size. If the pde is not the last one then the last pte
		 * would be 1023. So each pde will be using all the
		 * page table entires except for the last pde.
		 * For the last pde, pte is calculated using the last
		 * memory address of the buffer.
		 */
		if (pde != end_pde_num) {
			ending_pte_num = 1023;
		} else {
			ending_pte_num = MMU_PAGE_NUM((char *)addr + size - 1);
		}

		/* For all the pde's appart from the starting pde, will have
		 * the start pte number as zero.
		 */
		if (pde != start_pde_num) {
			starting_pte_num = 0;
		}

		pte_value.value = 0xFFFFFFFF;

		/* Bitwise AND all the pte values. */
		for (pte = starting_pte_num; pte <= ending_pte_num; pte++) {

			pte_value.value &= pte_address->entry[pte].value;
		}

		validity &= check_pte_flags(pte_value,
					    rw_permission,
					    us_permission);
	}

	if (validity == 0) {
		status = -EPERM;
	}
	return status;
}


static inline void tlb_flush_page(void *addr)
{
	/* Invalidate TLB entries corresponding to the page containing the
	 * specified address
	 */
	char *page = (char *)addr;
	__asm__ ("invlpg %0" :: "m" (*page));
}


void _x86_mmu_set_flags(void *ptr, size_t size, u32_t flags, u32_t mask)
{
	union x86_mmu_pte *pte;

	u32_t addr = (u32_t)ptr;

	__ASSERT(!(addr & MMU_PAGE_MASK), "unaligned address provided");
	__ASSERT(!(size & MMU_PAGE_MASK), "unaligned size provided");

	while (size) {

		/* TODO we're not generating 4MB entries at the moment */
		__ASSERT(X86_MMU_GET_4MB_PDE(addr)->ps != 1, "4MB PDE found");

		pte = X86_MMU_GET_PTE(addr);

		pte->value = (pte->value & ~mask) | flags;
		tlb_flush_page((void *)addr);

		size -= MMU_PAGE_SIZE;
		addr += MMU_PAGE_SIZE;
	}
}
