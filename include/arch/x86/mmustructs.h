/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_MMU_H
#define ZEPHYR_INCLUDE_ARCH_X86_MMU_H

#include <sys/util.h>

/*
 * K_MEM_PARTITION_* defines
 *
 * Slated for removal when virtual memory is implemented, memory
 * mapping APIs will replace memory domains.
 */
#define Z_X86_MMU_RW		BIT64(1)	/** Read-Write */
#define Z_X86_MMU_US		BIT64(2)	/** User-Supervisor */
#if defined(CONFIG_X86_PAE) || defined(CONFIG_X86_64)
#define Z_X86_MMU_XD		BIT64(63)	/** Execute Disable */
#else
#define Z_X86_MMU_XD		0
#endif

/* Always true with 32-bit page tables, don't enable
 * CONFIG_EXECUTE_XOR_WRITE and expect it to work for you
 */
#define K_MEM_PARTITION_IS_EXECUTABLE(attr)	(((attr) & Z_X86_MMU_XD) == 0)
#define K_MEM_PARTITION_IS_WRITABLE(attr)	(((attr) & Z_X86_MMU_RW) != 0)

/* memory partition arch/soc independent attribute */
#define K_MEM_PARTITION_P_RW_U_RW	(Z_X86_MMU_RW | Z_X86_MMU_US | \
					 Z_X86_MMU_XD)
#define K_MEM_PARTITION_P_RW_U_NA	(Z_X86_MMU_RW | Z_X86_MMU_XD)
#define K_MEM_PARTITION_P_RO_U_RO	(Z_X86_MMU_US | Z_X86_MMU_XD)
#define K_MEM_PARTITION_P_RO_U_NA	Z_X86_MMU_XD
/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RWX_U_RWX	(Z_X86_MMU_RW | Z_X86_MMU_US)
#define K_MEM_PARTITION_P_RWX_U_NA	Z_X86_MMU_RW
#define K_MEM_PARTITION_P_RX_U_RX	Z_X86_MMU_US
#define K_MEM_PARTITION_P_RX_U_NA	(0)
 /* memory partition access permission mask */
#define K_MEM_PARTITION_PERM_MASK	(Z_X86_MMU_RW | Z_X86_MMU_US | \
					 Z_X86_MMU_XD)

#ifndef _ASMLANGUAGE
#include <sys/slist.h>

/* Page table entry data type at all levels. Defined here due to
 * k_mem_partition_attr_t, eventually move to private x86_mmu.h
 */
#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
typedef uint64_t pentry_t;
#else
typedef uint32_t pentry_t;
#endif
typedef pentry_t k_mem_partition_attr_t;

struct arch_mem_domain {
#ifdef CONFIG_X86_PAE
	/* 4-entry, 32-byte top-level PDPT */
	pentry_t pdpt[4];
#endif
	/* Pointer to top-level structure, either a PML4, PDPT, PD */
	pentry_t *ptables;

	/* Linked list of all active memory domains */
	sys_snode_t node;
#ifdef CONFIG_X86_PAE
} __aligned(32);
#else
};
#endif /* CONFIG_X86_PAE */
#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_X86_MMU_H */
