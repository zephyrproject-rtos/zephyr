/*
 * Copyright 2019 Broadcom
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_ARM_MMU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_ARM_MMU_H_

#ifndef _ASMLANGUAGE
#include <stdint.h>
#include <stdlib.h>
#endif

/* Following Memory types supported through MAIR encodings can be passed
 * by user through "attrs"(attributes) field of specified memory region.
 * As MAIR supports such 8 encodings, we will reserve attrs[2:0];
 * so that we can provide encodings upto 7 if needed in future.
 */
#define MT_TYPE_MASK		0x7U
#define MT_TYPE(attr)		(attr & MT_TYPE_MASK)
#define MT_DEVICE_nGnRnE	0U
#define MT_DEVICE_nGnRE		1U
#define MT_DEVICE_GRE		2U
#define MT_NORMAL_NC		3U
#define MT_NORMAL		4U
#define MT_NORMAL_WT		5U

#define MEMORY_ATTRIBUTES	((0x00 << (MT_DEVICE_nGnRnE * 8)) |	\
				(0x04 << (MT_DEVICE_nGnRE * 8))   |	\
				(0x0c << (MT_DEVICE_GRE * 8))     |	\
				(0x44 << (MT_NORMAL_NC * 8))      |	\
				(0xffUL << (MT_NORMAL * 8))	  |	\
				(0xbbUL << (MT_NORMAL_WT * 8)))

/* More flags from user's perspective are supported using remaining bits
 * of "attrs" field, i.e. attrs[31:3], underlying code will take care
 * of setting PTE fields correctly.
 *
 * current usage of attrs[31:3] is:
 * attrs[3] : Access Permissions
 * attrs[4] : Memory access from secure/ns state
 * attrs[5] : Execute Permissions privileged mode (PXN)
 * attrs[6] : Execute Permissions unprivileged mode (UXN)
 * attrs[7] : Mirror RO/RW permissions to EL0
 * attrs[8] : Overwrite existing mapping if any
 * attrs[9] : non-Global mapping (nG)
 *
 */
#define MT_PERM_SHIFT		3U
#define MT_SEC_SHIFT		4U
#define MT_P_EXECUTE_SHIFT	5U
#define MT_U_EXECUTE_SHIFT	6U
#define MT_RW_AP_SHIFT		7U
#define MT_NO_OVERWRITE_SHIFT	8U
#define MT_NON_GLOBAL_SHIFT	9U

#define MT_RO			(0U << MT_PERM_SHIFT)
#define MT_RW			(1U << MT_PERM_SHIFT)

#define MT_RW_AP_ELx		(1U << MT_RW_AP_SHIFT)
#define MT_RW_AP_EL_HIGHER	(0U << MT_RW_AP_SHIFT)

#define MT_SECURE		(0U << MT_SEC_SHIFT)
#define MT_NS			(1U << MT_SEC_SHIFT)

#define MT_P_EXECUTE		(0U << MT_P_EXECUTE_SHIFT)
#define MT_P_EXECUTE_NEVER	(1U << MT_P_EXECUTE_SHIFT)

#define MT_U_EXECUTE		(0U << MT_U_EXECUTE_SHIFT)
#define MT_U_EXECUTE_NEVER	(1U << MT_U_EXECUTE_SHIFT)

#define MT_NO_OVERWRITE		(1U << MT_NO_OVERWRITE_SHIFT)

#define MT_G			(0U << MT_NON_GLOBAL_SHIFT)
#define MT_NG			(1U << MT_NON_GLOBAL_SHIFT)

#define MT_P_RW_U_RW		(MT_RW | MT_RW_AP_ELx | MT_P_EXECUTE_NEVER | MT_U_EXECUTE_NEVER)
#define MT_P_RW_U_NA		(MT_RW | MT_RW_AP_EL_HIGHER  | MT_P_EXECUTE_NEVER | MT_U_EXECUTE_NEVER)
#define MT_P_RO_U_RO		(MT_RO | MT_RW_AP_ELx | MT_P_EXECUTE_NEVER | MT_U_EXECUTE_NEVER)
#define MT_P_RO_U_NA		(MT_RO | MT_RW_AP_EL_HIGHER  | MT_P_EXECUTE_NEVER | MT_U_EXECUTE_NEVER)
#define MT_P_RO_U_RX		(MT_RO | MT_RW_AP_ELx | MT_P_EXECUTE_NEVER | MT_U_EXECUTE)
#define MT_P_RX_U_RX		(MT_RO | MT_RW_AP_ELx | MT_P_EXECUTE | MT_U_EXECUTE)
#define MT_P_RX_U_NA		(MT_RO | MT_RW_AP_EL_HIGHER  | MT_P_EXECUTE | MT_U_EXECUTE_NEVER)

#ifdef CONFIG_ARMV8_A_NS
#define MT_DEFAULT_SECURE_STATE	MT_NS
#else
#define MT_DEFAULT_SECURE_STATE	MT_SECURE
#endif

#ifndef _ASMLANGUAGE

/* Region definition data structure */
struct arm_mmu_region {
	/* Region Base Physical Address */
	uintptr_t base_pa;
	/* Region Base Virtual Address */
	uintptr_t base_va;
	/* Region size */
	size_t size;
	/* Region Name */
	const char *name;
	/* Region Attributes */
	uint32_t attrs;
};

/* MMU configuration data structure */
struct arm_mmu_config {
	/* Number of regions */
	unsigned int num_regions;
	/* Regions */
	const struct arm_mmu_region *mmu_regions;
};

struct arm_mmu_ptables {
	uint64_t *base_xlat_table;
	uint64_t ttbr0;
};

/* Convenience macros to represent the ARMv8-A-specific
 * configuration for memory access permission and
 * cache-ability attribution.
 */

#define MMU_REGION_ENTRY(_name, _base_pa, _base_va, _size, _attrs) \
	{\
		.name = _name, \
		.base_pa = _base_pa, \
		.base_va = _base_va, \
		.size = _size, \
		.attrs = _attrs, \
	}

#define MMU_REGION_FLAT_ENTRY(name, adr, sz, attrs) \
	MMU_REGION_ENTRY(name, adr, adr, sz, attrs)

/*
 * @brief Auto generate mmu region entry for node_id
 *
 * Example usage:
 *
 * @code{.c}
 *      DT_FOREACH_STATUS_OKAY_VARGS(nxp_imx_gpio,
 *				  MMU_REGION_DT_FLAT_ENTRY,
 *				 (MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_NS))
 * @endcode
 *
 * @note  Since devicetree_generated.h does not include
 *        node_id##_P_reg_FOREACH_PROP_ELEM* definitions,
 *        we can't automate dts node with multiple reg
 *        entries.
 */
#define MMU_REGION_DT_FLAT_ENTRY(node_id, attrs)  \
	MMU_REGION_FLAT_ENTRY(DT_NODE_FULL_NAME(node_id), \
				  DT_REG_ADDR(node_id), \
				  DT_REG_SIZE(node_id), \
				  attrs),

/*
 * @brief Auto generate mmu region entry for status = "okay"
 *        nodes compatible to a driver
 *
 * Example usage:
 *
 * @code{.c}
 *      MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY(nxp_imx_gpio,
 *				 (MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_NS))
 * @endcode
 *
 * @note  This is a wrapper of @ref MMU_REGION_DT_FLAT_ENTRY
 */
#define MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY(compat, attr) \
	DT_FOREACH_STATUS_OKAY_VARGS(compat, \
	MMU_REGION_DT_FLAT_ENTRY, attr)

/* Kernel macros for memory attribution
 * (access permissions and cache-ability).
 *
 * The macros are to be stored in k_mem_partition_attr_t
 * objects. The format of a k_mem_partition_attr_t object
 * is an uint32_t composed by permission and attribute flags
 * located in include/arch/arm64/arm_mmu.h
 */

/* Read-Write access permission attributes */
#define K_MEM_PARTITION_P_RW_U_RW ((k_mem_partition_attr_t) \
			{MT_P_RW_U_RW})
#define K_MEM_PARTITION_P_RW_U_NA ((k_mem_partition_attr_t) \
			{MT_P_RW_U_NA})
#define K_MEM_PARTITION_P_RO_U_RO ((k_mem_partition_attr_t) \
			{MT_P_RO_U_RO})
#define K_MEM_PARTITION_P_RO_U_NA ((k_mem_partition_attr_t) \
			{MT_P_RO_U_NA})
/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RX_U_RX ((k_mem_partition_attr_t) \
			{MT_P_RX_U_RX})
/* Typedef for the k_mem_partition attribute */
typedef struct { uint32_t attrs; } k_mem_partition_attr_t;

/* Reference to the MMU configuration.
 *
 * This struct is defined and populated for each SoC (in the SoC definition),
 * and holds the build-time configuration information for the fixed MMU
 * regions enabled during kernel initialization.
 */
extern const struct arm_mmu_config mmu_config;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_ARM_MMU_H_ */
