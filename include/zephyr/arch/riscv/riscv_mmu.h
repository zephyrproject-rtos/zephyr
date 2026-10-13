/*
 * Copyright (c) 2026 Synaptics Incorporated
 * Author: Jisheng Zhang <jszhang@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_RISCV_MMU_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_RISCV_MMU_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

/* Status bits returned by arch_page_info_get(). */
#define ARCH_DATA_PAGE_LOADED     BIT(0)
#define ARCH_DATA_PAGE_ACCESSED   BIT(1)
#define ARCH_DATA_PAGE_DIRTY      BIT(2)
#define ARCH_DATA_PAGE_NOT_MAPPED BIT(3)

/*
 * Anonymous demand-mapped pages have no backing-store location. Keep their
 * tags page aligned and within the PTE PPN field so a paged-out PTE can
 * return the original tag through arch_page_location_get().
 * RV64 PTEs have a 44-bit PPN (56-bit physical address); Sv32 also needs
 * these values to fit in uintptr_t.
 */
#if defined(CONFIG_64BIT)
#define ARCH_UNPAGED_ANON_ZERO   ((uintptr_t)0x00FFFFFFFFFFF000ULL)
#define ARCH_UNPAGED_ANON_UNINIT ((uintptr_t)0x00FFFFFFFFFFE000ULL)
#else
#define ARCH_UNPAGED_ANON_ZERO   ((uintptr_t)0xFFFFF000U)
#define ARCH_UNPAGED_ANON_UNINIT ((uintptr_t)0xFFFFE000U)
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_RISCV_MMU_H_ */
