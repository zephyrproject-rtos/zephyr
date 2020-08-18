/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_COREDUMP_H_
#define ZEPHYR_INCLUDE_DEBUG_COREDUMP_H_

#ifdef CONFIG_DEBUG_COREDUMP

#include <toolchain.h>
#include <arch/cpu.h>
#include <sys/byteorder.h>

#define Z_COREDUMP_HDR_VER		1

#define	Z_COREDUMP_ARCH_HDR_ID		'A'

#define	Z_COREDUMP_MEM_HDR_ID		'M'
#define Z_COREDUMP_MEM_HDR_VER		1

/* Target code */
enum z_coredump_tgt_code {
	COREDUMP_TGT_UNKNOWN = 0,
	COREDUMP_TGT_X86,
	COREDUMP_TGT_X86_64,
	COREDUMP_TGT_ARM_CORTEX_M,
};

/* Coredump header */
struct z_coredump_hdr_t {
	/* 'Z', 'E' */
	char		id[2];

	/* Header version */
	uint16_t	hdr_version;

	/* Target code */
	uint16_t	tgt_code;

	/* Pointer size in Log2 */
	uint8_t		ptr_size_bits;

	uint8_t		flag;

	/* Coredump Reason given */
	unsigned int	reason;
} __packed;

/* Architecture-specific block header */
struct z_coredump_arch_hdr_t {
	/* Z_COREDUMP_ARCH_HDR_ID */
	char		id;

	/* Header version */
	uint16_t	hdr_version;

	/* Number of bytes in this block (excluding header) */
	uint16_t	num_bytes;
} __packed;

/* Memory block header */
struct z_coredump_mem_hdr_t {
	/* Z_COREDUMP_MEM_HDR_ID */
	char		id;

	/* Header version */
	uint16_t	hdr_version;

	/* Address of start of memory region */
	uintptr_t	start;

	/* Address of end of memory region */
	uintptr_t	end;
} __packed;

void z_coredump(unsigned int reason, const z_arch_esf_t *esf,
		struct k_thread *thread);
void z_coredump_memory_dump(uintptr_t start_addr, uintptr_t end_addr);
int z_coredump_buffer_output(uint8_t *buf, size_t buflen);

#else

void z_coredump(unsigned int reason, const z_arch_esf_t *esf,
		struct k_thread *thread)
{
}

void z_coredump_memory_dump(uintptr_t start_addr, uintptr_t end_addr)
{
}

int z_coredump_buffer_output(uint8_t *buf, size_t buflen)
{
	return 0;
}

#endif /* CONFIG_DEBUG_COREDUMP */

/**
 * @defgroup coredump_apis Coredump APIs
 * @brief Coredump APIs
 * @{
 */

/**
 * @fn void z_coredump(unsigned int reason, const z_arch_esf_t *esf, struct k_thread *thread);
 * @brief Perform coredump.
 *
 * Normally, this is called inside z_fatal_error() to generate coredump
 * when a fatal error is encountered. This can also be called on demand
 * whenever a coredump is desired.
 *
 * @param reason Reason for the fatal error
 * @param esf Exception context
 * @param thread Thread information to dump
 */

/**
 * @fn void z_coredump_memory_dump(uintptr_t start_addr, uintptr_t end_addr);
 * @brief Dump memory region
 *
 * @param start_addr Start address of memory region to be dumped
 * @param emd_addr End address of memory region to be dumped
 */

/**
 * @fn int z_coredump_buffer_output(uint8_t *buf, size_t buflen);
 * @brief Output the buffer via coredump
 *
 * This outputs the buffer of byte array to the coredump backend.
 * For example, this can be called to output the coredump section
 * containing registers, or a section for memory dump.
 *
 * @param buf Buffer to be send to coredump output
 * @param buflen Buffer length
 */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DEBUG_COREDUMP_H_ */
