/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include<kernel.h>
#include<mmustructs.h>

/* Linker variable. It needed to access the start of the Page directory */
extern u32_t __mmu_tables_start;

#define X86_MMU_PD ((struct x86_mmu_page_directory *)\
		    (void *)&__mmu_tables_start)

/* Ref to _x86_mmu_buffer_validate documentation for details  */
#define USER_PERM_BIT_POS ((u32_t)0x1)
#define GET_RW_PERM(flags) (flags & BUFF_WRITEABLE)
#define GET_US_PERM(flags) ((flags & BUFF_USER) >> USER_PERM_BIT_POS)

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
	starting_pte_num = MMU_PAGE_NUM((char *)addr + size - 1);

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
