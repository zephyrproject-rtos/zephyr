/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_COREDUMP_H_
#define ZEPHYR_INCLUDE_DEBUG_COREDUMP_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>


/**
 * @file
 *
 * @defgroup coredump_apis Coredump APIs
 * @ingroup os_services
 * @brief Coredump APIs
 * @{
 */


/** Query ID */
enum coredump_query_id {
	/**
	 * Returns error code from backend.
	 */
	COREDUMP_QUERY_GET_ERROR,

	/**
	 * Check if there is a stored coredump from backend.
	 *
	 * Returns:
	 * - 1 if there is a stored coredump, 0 if none.
	 * - -ENOTSUP if this query is not supported.
	 * - Otherwise, error code from backend.
	 */
	COREDUMP_QUERY_HAS_STORED_DUMP,

	/**
	 * Returns:
	 * - coredump raw size from backend, 0 if none.
	 * - -ENOTSUP if this query is not supported.
	 * - Otherwise, error code from backend.
	 */
	COREDUMP_QUERY_GET_STORED_DUMP_SIZE,

	/**
	 * Max value for query ID.
	 */
	COREDUMP_QUERY_MAX
};

/** Command ID */
enum coredump_cmd_id {
	/**
	 * Clear error code from backend.
	 *
	 * Returns 0 if successful, failed otherwise.
	 */
	COREDUMP_CMD_CLEAR_ERROR,

	/**
	 * Verify that the stored coredump is valid.
	 *
	 * Returns:
	 * - 1 if valid.
	 * - 0 if not valid or no stored coredump.
	 * - -ENOTSUP if this command is not supported.
	 * - Otherwise, error code from backend.
	 */
	COREDUMP_CMD_VERIFY_STORED_DUMP,

	/**
	 * Erase the stored coredump.
	 *
	 * Returns:
	 * - 0 if successful.
	 * - -ENOTSUP if this command is not supported.
	 * - Otherwise, error code from backend.
	 */
	COREDUMP_CMD_ERASE_STORED_DUMP,

	/**
	 * Copy the raw stored coredump.
	 *
	 * Returns:
	 * - copied size if successful
	 * - 0 if stored coredump is not found
	 * - -ENOTSUP if this command is not supported.
	 * - Otherwise, error code from backend.
	 */
	COREDUMP_CMD_COPY_STORED_DUMP,

	/**
	 * Invalidate the stored coredump. This is faster than
	 * erasing the whole partition.
	 *
	 * Returns:
	 * - 0 if successful.
	 * - -ENOTSUP if this command is not supported.
	 * - Otherwise, error code from backend.
	 */
	COREDUMP_CMD_INVALIDATE_STORED_DUMP,

	/**
	 * Max value for command ID.
	 */
	COREDUMP_CMD_MAX
};

/** Coredump copy command (@ref COREDUMP_CMD_COPY_STORED_DUMP) argument definition */
struct coredump_cmd_copy_arg {
	/** Copy offset */
	off_t offset;

	/** Copy destination buffer */
	uint8_t *buffer;

	/** Copy length */
	size_t length;
};

#ifdef CONFIG_DEBUG_COREDUMP

#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/byteorder.h>

#define COREDUMP_HDR_VER		1

#define	COREDUMP_ARCH_HDR_ID		'A'

#define	COREDUMP_MEM_HDR_ID		'M'
#define COREDUMP_MEM_HDR_VER		1

/* Target code */
enum coredump_tgt_code {
	COREDUMP_TGT_UNKNOWN = 0,
	COREDUMP_TGT_X86,
	COREDUMP_TGT_X86_64,
	COREDUMP_TGT_ARM_CORTEX_M,
	COREDUMP_TGT_RISC_V,
	COREDUMP_TGT_XTENSA,
	COREDUMP_TGT_ARM64,
};

/* Coredump header */
struct coredump_hdr_t {
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
struct coredump_arch_hdr_t {
	/* COREDUMP_ARCH_HDR_ID */
	char		id;

	/* Header version */
	uint16_t	hdr_version;

	/* Number of bytes in this block (excluding header) */
	uint16_t	num_bytes;
} __packed;

/* Memory block header */
struct coredump_mem_hdr_t {
	/* COREDUMP_MEM_HDR_ID */
	char		id;

	/* Header version */
	uint16_t	hdr_version;

	/* Address of start of memory region */
	uintptr_t	start;

	/* Address of end of memory region */
	uintptr_t	end;
} __packed;

typedef void (*coredump_backend_start_t)(void);
typedef void (*coredump_backend_end_t)(void);
typedef void (*coredump_backend_buffer_output_t)(uint8_t *buf, size_t buflen);
typedef int (*coredump_backend_query_t)(enum coredump_query_id query_id,
					void *arg);
typedef int (*coredump_backend_cmd_t)(enum coredump_cmd_id cmd_id,
				      void *arg);

struct coredump_backend_api {
	/* Signal to backend of the start of coredump. */
	coredump_backend_start_t		start;

	/* Signal to backend of the end of coredump. */
	coredump_backend_end_t		end;

	/* Raw buffer output */
	coredump_backend_buffer_output_t	buffer_output;

	/* Perform query on backend */
	coredump_backend_query_t		query;

	/* Perform command on backend */
	coredump_backend_cmd_t			cmd;
};

void coredump(unsigned int reason, const z_arch_esf_t *esf,
	      struct k_thread *thread);
void coredump_memory_dump(uintptr_t start_addr, uintptr_t end_addr);
void coredump_buffer_output(uint8_t *buf, size_t buflen);

int coredump_query(enum coredump_query_id query_id, void *arg);
int coredump_cmd(enum coredump_cmd_id cmd_id, void *arg);

#else

void coredump(unsigned int reason, const z_arch_esf_t *esf,
	      struct k_thread *thread)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(esf);
	ARG_UNUSED(thread);
}

void coredump_memory_dump(uintptr_t start_addr, uintptr_t end_addr)
{
	ARG_UNUSED(start_addr);
	ARG_UNUSED(end_addr);
}

void coredump_buffer_output(uint8_t *buf, size_t buflen)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(buflen);
}

int coredump_query(enum coredump_query_id query_id, void *arg)
{
	ARG_UNUSED(query_id);
	ARG_UNUSED(arg);
	return -ENOTSUP;
}

int coredump_cmd(enum coredump_cmd_id query_id, void *arg)
{
	ARG_UNUSED(query_id);
	ARG_UNUSED(arg);
	return -ENOTSUP;
}

#endif /* CONFIG_DEBUG_COREDUMP */

/**
 * @fn void coredump(unsigned int reason, const z_arch_esf_t *esf, struct k_thread *thread);
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
 * @fn void coredump_memory_dump(uintptr_t start_addr, uintptr_t end_addr);
 * @brief Dump memory region
 *
 * @param start_addr Start address of memory region to be dumped
 * @param end_addr End address of memory region to be dumped
 */

/**
 * @fn int coredump_buffer_output(uint8_t *buf, size_t buflen);
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
 * @fn int coredump_query(enum coredump_query_id query_id, void *arg);
 * @brief Perform query on coredump subsystem.
 *
 * Query the coredump subsystem for information, for example, if there is
 * an error.
 *
 * @param[in] query_id Query ID
 * @param[in,out] arg Pointer to argument for exchanging information
 * @return Depends on the query
 */

/**
 * @fn int coredump_cmd(enum coredump_cmd_id cmd_id, void *arg);
 * @brief Perform command on coredump subsystem.
 *
 * Perform command on coredump subsystem, for example, output the stored
 * coredump via logging.
 *
 * @param[in] cmd_id Command ID
 * @param[in,out] arg Pointer to argument for exchanging information
 * @return Depends on the command
 */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DEBUG_COREDUMP_H_ */
