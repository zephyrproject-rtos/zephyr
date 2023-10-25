/*
 * Xtensa MMU support
 *
 * Private data declarations
 *
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#ifndef ZEPHYR_ARCH_XTENSA_XTENSA_MMU_PRIV_H_
#define ZEPHYR_ARCH_XTENSA_XTENSA_MMU_PRIV_H_

#define Z_XTENSA_PTE_VPN_MASK 0xFFFFF000U
#define Z_XTENSA_PTE_PPN_MASK 0xFFFFF000U
#define Z_XTENSA_PTE_ATTR_MASK 0x0000000FU
#define Z_XTENSA_L1_MASK 0x3FF00000U
#define Z_XTENSA_L2_MASK 0x3FFFFFU

#define Z_XTENSA_PPN_SHIFT 12U

#define Z_XTENSA_PTE_RING_MASK 0x00000030U

#define Z_XTENSA_PTE(paddr, ring, attr) \
	(((paddr) & Z_XTENSA_PTE_PPN_MASK) | \
	(((ring) << 4) & Z_XTENSA_PTE_RING_MASK) | \
	((attr) & Z_XTENSA_PTE_ATTR_MASK))

#define Z_XTENSA_L2_POS(vaddr) \
	(((vaddr) & Z_XTENSA_L2_MASK) >> Z_XTENSA_PPN_SHIFT)

/* Kernel specific ASID. Ring field in the PTE */
#define Z_XTENSA_KERNEL_RING 0

void xtensa_init_paging(uint32_t *l1_page);
void xtensa_set_paging(uint32_t user_asid, uint32_t *l1_page);
void xtensa_invalidate_refill_tlb(void);

#endif /* ZEPHYR_ARCH_XTENSA_XTENSA_MMU_PRIV_H_ */
