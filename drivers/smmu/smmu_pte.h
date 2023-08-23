/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ARM64_SMMU_PTE_H__
#define __ARM64_SMMU_PTE_H__

#include <zephyr/types.h>
#include <zephyr/arch/arm64/arm_mmu.h>

typedef uint64_t pd_entry_t; /* Page directory entry */
typedef uint64_t pt_entry_t; /* Page table entry */

/* ARM64 architecture related properties */
#define VA_MAX_SIZE 48

#define MEMATTR_DEVICE_nGnRnE MT_DEVICE_nGnRnE
#define MEMATTR_DEVICE_nGnRE  MT_DEVICE_nGnRE
#define MEMATTR_DEVICE_GRE    MT_DEVICE_GRE
#define MEMATTR_NORMAL_NC     MT_NORMAL_NC
#define MEMATTR_NORMAL        MT_NORMAL
#define MEMATTR_NORMAL_WT     MT_NORMAL_WT

#define MEMATTR_DEVICE MEMATTR_DEVICE_nGnRnE

/* extra attrs */
#define SMMU_ATTRS_READ_ONLY BIT(0)

#define PAGE_4K_S          12
#define PAGE_4K_M          GENMASK64(11, 0)
#define PAGE_4K_Ln_VA_SIZE 9
#define PAGE_4K_ln_N       4
#define PAGE_S             PAGE_4K_S
#define PAGE_M             PAGE_4K_M
#define PAGE_Ln_N          PAGE_4K_ln_N
#define PAGE_Ln_VA_SIZE    PAGE_4K_Ln_VA_SIZE

#define SMMU_PAGE_SIZE (PAGE_4K_M + 1)

#define trunc_page(x) ((unsigned long)(x) & ~PAGE_M)

/*
 * 48-bit address with 4KB granule size:
 *
 * +------------+------------+------------+------------+-----------+
 * | VA [47:39] | VA [38:30] | VA [29:21] | VA [20:12] | VA [11:0] |
 * +---------------------------------------------------------------+
 * |  L0(512)   |  L1(512)   |  L2(512)   |  L3(512)   | block off |
 * +------------+------------+------------+------------+-----------+
 */

/* Level 0 table, 512GiB per entry */
#define SMMU_L0_S 39
#define SMMU_L0_M GENMASK64(47, 39)

/* Level 1 table, 1GiB per entry */
#define SMMU_L1_S 30
#define SMMU_L1_M GENMASK64(38, 30)

#define SMMU_L2_S 21
#define SMMU_L2_M GENMASK64(29, 21)

#define SMMU_L3_S 12
#define SMMU_L3_M GENMASK64(20, 12)

/* Table attributes */
#define ATTR_DESCR_VALID_B    BIT(0)
#define ATTR_DESCR_TYPE_M     GENMASK64(1, 0)
#define ATTR_DESCR_TYPE_TABLE 0x3
#define SMMU_L0_TABLE         ATTR_DESCR_TYPE_TABLE
#define SMMU_L1_TABLE         ATTR_DESCR_TYPE_TABLE
#define SMMU_L2_TABLE         ATTR_DESCR_TYPE_TABLE
#define SMMU_Ln_TABLE         ATTR_DESCR_TYPE_TABLE
#define SMMU_L3_PAGE          ATTR_DESCR_TYPE_TABLE

#define ATTR_S1_IDX_M  GENMASK64(4, 2)
#define ATTR_S1_IDX(x) FIELD_PREP(ATTR_S1_IDX_M, x)

#define ATTR_S1_NS BIT(5)

#define ATTR_S1_AP_M       GENMASK64(7, 6)
#define ATTR_S1_AP(x)      FIELD_PREP(ATTR_S1_AP_M, x)
#define ATTR_S1_AP_RO      0x2
#define ATTR_S1_AP_RW      0x0
#define ATTR_S1_AP_USER_RW 0x1

#define ATTR_SH_M  GENMASK64(9, 8)
#define ATTR_SH(x) FIELD_PREP(ATTR_SH_M, x)
#define ATTR_SH_NS 0
#define ATTR_SH_OS 2
#define ATTR_SH_IS 3

#define ATTR_AF    BIT(10)
#define ATTR_S1_nG BIT(11)

#define ATTR_S1_PXN BIT(53)
#define ATTR_S1_UXN BIT(54)
#define ATTR_S1_XN  (ATTR_S1_PXN | ATTR_S1_UXN)

#define ATTR_DEFAULT (ATTR_AF | ATTR_SH(ATTR_SH_IS) | ATTR_S1_NS)

/* Output address or next-level table address */
#define ADDR_Ln_NLTA GENMASK64(47, 12)

#define Ln_ENTRIES 512

#define smmu_l3_index(va) FIELD_GET(SMMU_L3_M, va)
#define smmu_xlat_index(va, level)                                                                 \
	((va >> (PAGE_S + PAGE_Ln_VA_SIZE * (PAGE_Ln_N - 1 - level))) & (Ln_ENTRIES - 1))

#endif /* end of include guard: __ARM64_SMMU_PTE_H__ */
