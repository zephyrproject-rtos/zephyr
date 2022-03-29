/*
 * ARMv7 MMU support
 *
 * Copyright (c) 2021 Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_AARCH32_ARM_MMU_H_
#define ZEPHYR_INCLUDE_ARCH_AARCH32_ARM_MMU_H_

#ifndef _ASMLANGUAGE

/*
 * Comp.:
 * ARM Architecture Reference Manual, ARMv7-A and ARMv7-R edition,
 * ARM document ID DDI0406C Rev. d, March 2018
 * Memory type definitions:
 * Table B3-10, chap. B3.8.2, p. B3-1363f.
 * Outer / inner cache attributes for cacheable memory:
 * Table B3-11, chap. B3.8.2, p. B3-1364
 */

/*
 * The following definitions are used when specifying a memory
 * range to be mapped at boot time using the MMU_REGION_ENTRY
 * macro.
 */
#define MT_STRONGLY_ORDERED		BIT(0)
#define MT_DEVICE			BIT(1)
#define MT_NORMAL			BIT(2)
#define MT_MASK				0x7

#define MPERM_R				BIT(3)
#define MPERM_W				BIT(4)
#define MPERM_X				BIT(5)
#define MPERM_UNPRIVILEGED		BIT(6)

#define MATTR_NON_SECURE		BIT(7)
#define MATTR_NON_GLOBAL		BIT(8)
#define MATTR_SHARED			BIT(9)
#define MATTR_CACHE_OUTER_WB_WA		BIT(10)
#define MATTR_CACHE_OUTER_WT_nWA	BIT(11)
#define MATTR_CACHE_OUTER_WB_nWA	BIT(12)
#define MATTR_CACHE_INNER_WB_WA		BIT(13)
#define MATTR_CACHE_INNER_WT_nWA	BIT(14)
#define MATTR_CACHE_INNER_WB_nWA	BIT(15)

#define MATTR_MAY_MAP_L1_SECTION	BIT(16)

/*
 * The following macros are used for adding constant entries
 * mmu_regions array of the mmu_config struct. Use MMU_REGION_ENTRY
 * for the specification of mappings whose PA and VA differ,
 * the use of MMU_REGION_FLAT_ENTRY always results in an identity
 * mapping, which are used for the mappings of the Zephyr image's
 * code and data.
 */
#define MMU_REGION_ENTRY(_name, _base_pa, _base_va, _size, _attrs) \
	{\
		.name    = _name, \
		.base_pa = _base_pa, \
		.base_va = _base_va, \
		.size    = _size, \
		.attrs   = _attrs, \
	}

#define MMU_REGION_FLAT_ENTRY(name, adr, sz, attrs) \
	MMU_REGION_ENTRY(name, adr, adr, sz, attrs)

/* Region definition data structure */
struct arm_mmu_region {
	/* Region Base Physical Address */
	uintptr_t  base_pa;
	/* Region Base Virtual Address */
	uintptr_t  base_va;
	/* Region size */
	size_t     size;
	/* Region Name */
	const char *name;
	/* Region Attributes */
	uint32_t   attrs;
};

/* MMU configuration data structure */
struct arm_mmu_config {
	/* Number of regions */
	uint32_t		    num_regions;
	/* Regions */
	const struct arm_mmu_region *mmu_regions;
};

/*
 * Reference to the MMU configuration.
 *
 * This struct is defined and populated for each SoC (in the SoC definition),
 * and holds the build-time configuration information for the fixed MMU
 * regions enabled during kernel initialization.
 */
extern const struct arm_mmu_config mmu_config;

int z_arm_mmu_init(void);

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_AARCH32_ARM_MMU_H_ */
