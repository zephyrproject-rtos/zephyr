/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TriCore MPU definitions
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_MPU_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_MPU_H_

#include <stdint.h>
#include <zephyr/sys/dlist.h>

/** Read access in supervisor mode */
#define TRICORE_MPU_ACCESS_P_R (1 << 0)
/** Read access in user mode */
#define TRICORE_MPU_ACCESS_U_R (1 << 1)
/** Write access in supervisor mode */
#define TRICORE_MPU_ACCESS_P_W (1 << 2)
/** Write access in user mode */
#define TRICORE_MPU_ACCESS_U_W (1 << 3)
/** Execute access in supervisor mode */
#define TRICORE_MPU_ACCESS_P_X (1 << 4)
/** Execute access in user mode */
#define TRICORE_MPU_ACCESS_U_X (1 << 5)

/** No access */
#define TRICORE_MPU_ACCESS_P_NA_U_NA (0)
/** Supervisor read only, no user access */
#define TRICORE_MPU_ACCESS_P_RO_U_NA (TRICORE_MPU_ACCESS_P_R)
/** Read only in both modes */
#define TRICORE_MPU_ACCESS_P_RO_U_RO (TRICORE_MPU_ACCESS_P_R | TRICORE_MPU_ACCESS_U_R)
/** Supervisor read and write, no user access */
#define TRICORE_MPU_ACCESS_P_RW_U_NA (TRICORE_MPU_ACCESS_P_R | TRICORE_MPU_ACCESS_P_W)
/** Supervisor read and write, user read only */
#define TRICORE_MPU_ACCESS_P_RW_U_RO                                                               \
	(TRICORE_MPU_ACCESS_P_R | TRICORE_MPU_ACCESS_P_W | TRICORE_MPU_ACCESS_U_R)
/** Read and write in both modes */
#define TRICORE_MPU_ACCESS_P_RW_U_RW                                                               \
	(TRICORE_MPU_ACCESS_P_R | TRICORE_MPU_ACCESS_P_W | TRICORE_MPU_ACCESS_U_R |                \
	 TRICORE_MPU_ACCESS_U_W)
/** Read and execute in both modes */
#define TRICORE_MPU_ACCESS_P_RX_U_RX                                                               \
	(TRICORE_MPU_ACCESS_P_R | TRICORE_MPU_ACCESS_P_X | TRICORE_MPU_ACCESS_U_R |                \
	 TRICORE_MPU_ACCESS_U_X)
/** Read, write and execute in both modes */
#define TRICORE_MPU_ACCESS_P_RWX_U_RWX                                                             \
	(TRICORE_MPU_ACCESS_P_R | TRICORE_MPU_ACCESS_P_W | TRICORE_MPU_ACCESS_P_X |                \
	 TRICORE_MPU_ACCESS_U_R | TRICORE_MPU_ACCESS_U_W | TRICORE_MPU_ACCESS_U_X)

/** True if the access rights allow execution in either mode */
#define K_MEM_PARTITION_IS_EXECUTABLE(access_rights)                                               \
	((access_rights) & (TRICORE_MPU_ACCESS_P_X | TRICORE_MPU_ACCESS_U_X))

/** True if the access rights allow writing in either mode */
#define K_MEM_PARTITION_IS_WRITABLE(access_rights)                                                 \
	((access_rights) & (TRICORE_MPU_ACCESS_P_W | TRICORE_MPU_ACCESS_U_W))

/** Memory partition attribute: read and write in both modes */
#define K_MEM_PARTITION_P_RW_U_RW ((k_mem_partition_attr_t){TRICORE_MPU_ACCESS_P_RW_U_RW})
/** Memory partition attribute: supervisor read and write, no user access */
#define K_MEM_PARTITION_P_RW_U_NA ((k_mem_partition_attr_t){TRICORE_MPU_ACCESS_P_RW_U_NA})
/** Memory partition attribute: read only in both modes */
#define K_MEM_PARTITION_P_RO_U_RO ((k_mem_partition_attr_t){TRICORE_MPU_ACCESS_P_RO_U_RO})
/** Memory partition attribute: supervisor read only, no user access */
#define K_MEM_PARTITION_P_RO_U_NA ((k_mem_partition_attr_t){TRICORE_MPU_ACCESS_P_RO_U_NA})
/** Memory partition attribute: no access */
#define K_MEM_PARTITION_P_NA_U_NA ((k_mem_partition_attr_t){TRICORE_MPU_ACCESS_P_NA_U_NA})

/** Memory partition attribute: read and execute in both modes */
#define K_MEM_PARTITION_P_RX_U_RX   ((k_mem_partition_attr_t){TRICORE_MPU_ACCESS_P_RX_U_RX})
/** Memory partition attribute: read, write and execute in both modes */
#define K_MEM_PARTITION_P_RWX_U_RWX ((k_mem_partition_attr_t){TRICORE_MPU_ACCESS_P_RWX_U_RWX})

/** Memory partition attribute */
struct tricore_mem_partition_attr {
	/** TRICORE_MPU_ACCESS_* flags of the partition */
	uint32_t access_rights: 6;
	/** Data protection range index, not used yet */
	uint32_t dpr: 5;
	/** Code protection range index, not used yet */
	uint32_t cpr: 5;
};

/** Memory partition attribute type of the kernel memory domain API */
typedef struct tricore_mem_partition_attr k_mem_partition_attr_t;

/** @cond INTERNAL_HIDDEN */

struct arch_mem_domain {
	sys_dnode_t loaded_node;
	uint32_t cpxe;
	uint32_t dpre;
	uint32_t dpwe;
};

#define Z_TRICORE_STACK_GUARD_SIZE                                                                 \
	Z_POW2_CEIL(MAX(ARCH_STACK_PTR_ALIGN, CONFIG_MPU_STACK_GUARD_MIN_SIZE))

#define Z_TRICORE_USER_STACK_SYSCALL_SLACK 64U

/** @endcond */

/** Fixed MPU region set up at boot */
struct tricore_mpu_region {
	/** First address of the region */
	uint32_t start;
	/** Address after the last byte of the region */
	uint32_t end;
	/** TRICORE_MPU_ACCESS_* flags of the region */
	uint32_t flags;
	/** Name of the region */
	const char *name;
};

/** Fixed MPU regions of a SoC */
struct tricore_mpu_config {
	/** Number of entries in regions */
	size_t num_regions;
	/** Region table */
	const struct tricore_mpu_region *regions;
};

/** Fixed MPU regions, provided by the SoC or the default region table */
extern const struct tricore_mpu_config mpu_config;

/** @cond INTERNAL_HIDDEN */

void z_tricore_mpu_init(void);

/** @endcond */

#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_MPU_H_ */
