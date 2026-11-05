/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ISR_TABLE_METADATA_H_
#define ZEPHYR_INCLUDE_ISR_TABLE_METADATA_H_

#include <stdint.h>

#define Z_ISR_LOCAL_METADATA_VERSION 3U
#define Z_ISR_LOCAL_RECORD_ALIGNMENT 4U

/* IAR keeps build records in the ELF without allocating target memory. */
#if defined(__IAR_SYSTEMS_ICC__)
#define Z_ISR_LOCAL_METADATA_STORAGE __no_alloc
#define Z_ISR_LOCAL_METADATA_NONALLOC 1
#else
#define Z_ISR_LOCAL_METADATA_STORAGE
#endif

/* All fields use the target ELF byte order and fixed-width integer encoding. */
struct z_isr_local_header {
	uint32_t version;
	uint32_t header_size;
	uint32_t table_size;
	uint32_t offset;
	uint32_t entry_size;
	uint32_t entry_alignment;
	uint32_t arg_offset;
	uint32_t arg_size;
	uint32_t isr_offset;
	uint32_t isr_size;
	uint32_t shared_size;
	uint32_t shared_alignment;
	uint32_t shared_count_offset;
	uint32_t shared_count_size;
	uint32_t shared_count_alignment;
	uint32_t vector_size;
	uint32_t vector_alignment;
};

/* The section-name bytes follow this prefix, including the terminating NUL. */
struct z_isr_local_record {
	uint32_t record_size;
	int32_t irq;
	uint32_t flags;
	uint32_t name_size;
};

#endif /* ZEPHYR_INCLUDE_ISR_TABLE_METADATA_H_ */
