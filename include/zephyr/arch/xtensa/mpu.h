/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/util_macro.h>

#include <xtensa/config/core-isa.h>

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_MPU_H
#define ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_MPU_H

/**
 * @defgroup xtensa_mpu_apis Xtensa Memory Protection Unit (MPU) APIs
 * @ingroup xtensa_apis
 * @{
 */

/** Number of available entries in the MPU table. */
#define XTENSA_MPU_NUM_ENTRIES			XCHAL_MPU_ENTRIES

/**
 * @name MPU memory region access rights.
 *
 * @note These are NOT bit masks, and must be used as whole value.
 *
 * @{
 */

/** Kernel and user modes no access. */
#define XTENSA_MPU_ACCESS_P_NA_U_NA			(0)

/** Kernel mode execution only. */
#define XTENSA_MPU_ACCESS_P_X_U_NA			(2)

/** User mode execution only. */
#define XTENSA_MPU_ACCESS_P_NA_U_X			(3)

/** Kernel mode read only. */
#define XTENSA_MPU_ACCESS_P_RO_U_NA			(4)

/** Kernel mode read and execution. */
#define XTENSA_MPU_ACCESS_P_RX_U_NA			(5)

/** Kernel mode read and write. */
#define XTENSA_MPU_ACCESS_P_RW_U_NA			(6)

/** Kernel mode read, write and execution. */
#define XTENSA_MPU_ACCESS_P_RWX_U_NA			(7)

/** Kernel and user modes write only. */
#define XTENSA_MPU_ACCESS_P_WO_U_WO			(8)

/** Kernel mode read, write. User mode read, write and execution. */
#define XTENSA_MPU_ACCESS_P_RW_U_RWX			(9)

/** Kernel mode read and write. User mode read only. */
#define XTENSA_MPU_ACCESS_P_RW_U_RO			(10)

/** Kernel mode read, write and execution. User mode read and execution. */
#define XTENSA_MPU_ACCESS_P_RWX_U_RX			(11)

/** Kernel and user modes read only. */
#define XTENSA_MPU_ACCESS_P_RO_U_RO			(12)

/** Kernel and user modes read and execution. */
#define XTENSA_MPU_ACCESS_P_RX_U_RX			(13)

/** Kernel and user modes read and write. */
#define XTENSA_MPU_ACCESS_P_RW_U_RW			(14)

/** Kernel and user modes read, write and execution. */
#define XTENSA_MPU_ACCESS_P_RWX_U_RWX			(15)

/**
 * @}
 */

/**
 * @brief Foreground MPU Entry.
 *
 * This holds the as, at register values for one MPU entry which can be
 * used directly by WPTLB.
 */
struct xtensa_mpu_entry {
	/**
	 * Content of as register for WPTLB.
	 *
	 * This contains the start address, the enable bit, and the lock bit.
	 */
	union {
		/** Raw value. */
		uint32_t raw;

		/** Individual parts. */
		struct {
			/**
			 * Enable bit for this entry.
			 *
			 * Modifying this will also modify the corresponding bit of
			 * the MPUENB register.
			 */
			uint32_t enable:1;

			/**
			 * Lock bit for this entry.
			 *
			 * Usable only if MPULOCKABLE parameter is enabled in
			 * processor configuration.
			 *
			 * Once set:
			 * - This cannot be cleared until reset.
			 * - This entry can no longer be modified.
			 * - The start address of the next entry also
			 *   cannot be modified.
			 */
			uint32_t lock:1;

			/** Must be zero. */
			uint32_t mbz:3;

			/**
			 * Start address of this MPU entry.
			 *
			 * Effective bits in this portion are affected by the minimum
			 * segment size of each MPU entry, ranging from 32 bytes to 4GB.
			 */
			uint32_t start_addr:27;
		} p;
	} as;

	/**
	 * Content of at register for WPTLB.
	 *
	 * This contains the memory type, access rights, and the segment number.
	 */
	union {
		/** Raw value. */
		uint32_t raw;

		/** Individual parts. */
		struct {
			/** The segment number of this MPU entry. */
			uint32_t segment:5;

			/** Must be zero (part 1). */
			uint32_t mbz1:3;

			/**
			 * Access rights associated with this MPU entry.
			 *
			 * This dictates the access right from the start address of
			 * this entry, to the start address of next entry.
			 *
			 * Refer to XTENSA_MPU_ACCESS_* macros for available rights.
			 */
			uint32_t access_rights:4;

			/**
			 * Memory type associated with this MPU entry.
			 *
			 * This dictates the memory type from the start address of
			 * this entry, to the start address of next entry.
			 *
			 * This affects how the hardware treats the memory, for example,
			 * cacheable vs non-cacheable, shareable vs non-shareable.
			 * Refer to the Xtensa Instruction Set Architecture (ISA) manual
			 * for general description, and the processor manual for processor
			 * specific information.
			 */
			uint32_t memory_type:9;

			/** Must be zero (part 2). */
			uint32_t mbz2:11;
		} p;
	} at;
};

/**
 * @brief Struct to hold foreground MPU map and its entries.
 */
struct xtensa_mpu_map {
	/**
	 * Array of MPU entries.
	 */
	struct xtensa_mpu_entry entries[XTENSA_MPU_NUM_ENTRIES];
};

/**
 * @name Memory domain and partitions
 * @{
 */

typedef uint32_t k_mem_partition_attr_t;

static inline bool xtensa_mem_partition_is_executable(k_mem_partition_attr_t access_rights)
{
	bool is_exec;

	switch (access_rights) {
	case XTENSA_MPU_ACCESS_P_X_U_NA:
	case XTENSA_MPU_ACCESS_P_NA_U_X:
	case XTENSA_MPU_ACCESS_P_RX_U_NA:
	case XTENSA_MPU_ACCESS_P_RWX_U_NA:
	case XTENSA_MPU_ACCESS_P_RW_U_RWX:
	case XTENSA_MPU_ACCESS_P_RWX_U_RX:
	case XTENSA_MPU_ACCESS_P_RX_U_RX:
	case XTENSA_MPU_ACCESS_P_RWX_U_RWX:
		is_exec = true;
		break;
	default:
		is_exec = false;
		break;
	};

	return is_exec;
}

static inline bool xtensa_mem_partition_is_writable(k_mem_partition_attr_t access_rights)
{
	bool is_writable;

	switch (access_rights) {
	case XTENSA_MPU_ACCESS_P_RW_U_NA:
	case XTENSA_MPU_ACCESS_P_RWX_U_NA:
	case XTENSA_MPU_ACCESS_P_WO_U_WO:
	case XTENSA_MPU_ACCESS_P_RW_U_RWX:
	case XTENSA_MPU_ACCESS_P_RW_U_RO:
	case XTENSA_MPU_ACCESS_P_RWX_U_RX:
	case XTENSA_MPU_ACCESS_P_RW_U_RW:
	case XTENSA_MPU_ACCESS_P_RWX_U_RWX:
		is_writable = true;
		break;
	default:
		is_writable = false;
		break;
	};

	return is_writable;
}

#define K_MEM_PARTITION_IS_EXECUTABLE(access_rights) \
	(xtensa_mem_partition_is_executable(access_rights))

#define K_MEM_PARTITION_IS_WRITABLE(access_rights) \
	(xtensa_mem_partition_is_writable(access_rights))

/* Read-Write access permission attributes */
#define K_MEM_PARTITION_P_RW_U_RW \
	((k_mem_partition_attr_t) {XTENSA_MPU_ACCESS_P_RW_U_RW})
#define K_MEM_PARTITION_P_RW_U_NA \
	((k_mem_partition_attr_t) {XTENSA_MPU_ACCESS_P_RW_U_NA})
#define K_MEM_PARTITION_P_RO_U_RO \
	((k_mem_partition_attr_t) {XTENSA_MPU_ACCESS_P_RO_U_RO})
#define K_MEM_PARTITION_P_RO_U_NA \
	((k_mem_partition_attr_t) {XTENSA_MPU_ACCESS_P_RO_U_NA})
#define K_MEM_PARTITION_P_NA_U_NA \
	((k_mem_partition_attr_t) {XTENSA_MPU_ACCESS_P_NA_U_NA})

/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RX_U_RX \
	((k_mem_partition_attr_t) {XTENSA_MPU_ACCESS_P_RX_U_RX})

/**
 * @}
 */

/**
 * Struct to describe a memory region [start, end).
 */
struct xtensa_mpu_range {
	/** Start address (inclusive) of the memory region. */
	const uintptr_t start;

	/**
	 * End address (exclusive) of the memory region.
	 *
	 * Use 0xFFFFFFFF for the end of memory.
	 */
	const uintptr_t end;

	/** Access rights for the memory region. */
	const uint8_t access_rights:4;

	/**
	 * Memory type for the region.
	 *
	 * Refer to the Xtensa Instruction Set Architecture (ISA) manual
	 * for general description, and the processor manual for processor
	 * specific information.
	 */
	const uint16_t memory_type:9;
} __packed;

/**
 * @brief Additional memory regions required by SoC.
 *
 * These memory regions will be setup by MPU initialization code at boot.
 *
 * Must be defined in the SoC layer.
 */
extern const struct xtensa_mpu_range xtensa_soc_mpu_ranges[];

/**
 * @brief Number of SoC additional memory regions.
 *
 * Must be defined in the SoC layer.
 */
extern const int xtensa_soc_mpu_ranges_num;

/**
 * @brief Initialize hardware MPU.
 *
 * This initializes the MPU hardware and setup the memory regions at boot.
 */
void xtensa_mpu_init(void);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_MPU_H */
