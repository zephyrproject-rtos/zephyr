/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MGMT_MGMT_
#define H_MGMT_MGMT_

#include <inttypes.h>
#include <zephyr/sys/slist.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCUmgr mgmt API
 * @defgroup mcumgr_mgmt_api MCUmgr mgmt API
 * @ingroup mcumgr
 * @{
 */

/** Opcodes; encoded in first byte of header. */
enum mcumgr_op_t {
	/** Read op-code */
	MGMT_OP_READ		= 0,

	/** Read response op-code */
	MGMT_OP_READ_RSP,

	/** Write op-code */
	MGMT_OP_WRITE,

	/** Write response op-code */
	MGMT_OP_WRITE_RSP,
};

/**
 * MCUmgr groups. The first 64 groups are reserved for system level mcumgr
 * commands. Per-user commands are then defined after group 64.
 */
enum mcumgr_group_t {
	/** OS (operating system) group */
	MGMT_GROUP_ID_OS	= 0,

	/** Image management group, used for uploading firmware images */
	MGMT_GROUP_ID_IMAGE,

	/** Statistic management group, used for retieving statistics */
	MGMT_GROUP_ID_STAT,

	/** System configuration group (unused) */
	MGMT_GROUP_ID_CONFIG,

	/** Log management group (unused) */
	MGMT_GROUP_ID_LOG,

	/** Crash group (unused) */
	MGMT_GROUP_ID_CRASH,

	/** Split image management group (unused) */
	MGMT_GROUP_ID_SPLIT,

	/** Run group (unused) */
	MGMT_GROUP_ID_RUN,

	/** FS (file system) group, used for performing file IO operations */
	MGMT_GROUP_ID_FS,

	/** Shell management group, used for executing shell commands */
	MGMT_GROUP_ID_SHELL,

	/** User groups defined from 64 onwards */
	MGMT_GROUP_ID_PERUSER	= 64,

	/** Zephyr-specific groups decrease from PERUSER to avoid collision with upstream and
	 *  user-defined groups.
	 *  Zephyr-specific: Basic group
	 */
	ZEPHYR_MGMT_GRP_BASIC	= (MGMT_GROUP_ID_PERUSER - 1),
};

/**
 * MCUmgr error codes.
 */
enum mcumgr_err_t {
	/** No error (success). */
	MGMT_ERR_EOK		= 0,

	/** Unknown error. */
	MGMT_ERR_EUNKNOWN,

	/** Insufficient memory (likely not enough space for CBOR object). */
	MGMT_ERR_ENOMEM,

	/** Error in input value. */
	MGMT_ERR_EINVAL,

	/** Operation timed out. */
	MGMT_ERR_ETIMEOUT,

	/** No such file/entry. */
	MGMT_ERR_ENOENT,

	/** Current state disallows command. */
	MGMT_ERR_EBADSTATE,

	/** Response too large. */
	MGMT_ERR_EMSGSIZE,

	/** Command not supported. */
	MGMT_ERR_ENOTSUP,

	/** Corrupt */
	MGMT_ERR_ECORRUPT,

	/** Command blocked by processing of other command */
	MGMT_ERR_EBUSY,

	/** Access to specific function, command or resource denied */
	MGMT_ERR_EACCESSDENIED,

	/** User errors defined from 256 onwards */
	MGMT_ERR_EPERUSER	= 256
};

#define MGMT_HDR_SIZE		8

/** @typedef mgmt_alloc_rsp_fn
 * @brief Allocates a buffer suitable for holding a response.
 *
 * If a source buf is provided, its user data is copied into the new buffer.
 *
 * @param src_buf	An optional source buffer to copy user data from.
 * @param arg		Optional streamer argument.
 *
 * @return Newly-allocated buffer on success NULL on failure.
 */
typedef void *(*mgmt_alloc_rsp_fn)(const void *src_buf, void *arg);

/** @typedef mgmt_reset_buf_fn
 * @brief Resets a buffer to a length of 0.
 *
 * The buffer's user data remains, but its payload is cleared.
 *
 * @param buf	The buffer to reset.
 * @param arg	Optional streamer argument.
 */
typedef void (*mgmt_reset_buf_fn)(void *buf, void *arg);

#ifdef CONFIG_MCUMGR_SMP_VERBOSE_ERR_RESPONSE
#define MGMT_CTXT_SET_RC_RSN(mc, rsn) ((mc->rc_rsn) = (rsn))
#define MGMT_CTXT_RC_RSN(mc) ((mc)->rc_rsn)
#else
#define MGMT_CTXT_SET_RC_RSN(mc, rsn)
#define MGMT_CTXT_RC_RSN(mc) NULL
#endif

/** @typedef mgmt_handler_fn
 * @brief Processes a request and writes the corresponding response.
 *
 * A separate handler is required for each supported op-ID pair.
 *
 * @param ctxt	The mcumgr context to use.
 *
 * @return 0 if a response was successfully encoded, #mcumgr_err_t code on failure.
 */
typedef int (*mgmt_handler_fn)(struct smp_streamer *ctxt);

/**
 * @brief Read handler and write handler for a single command ID.
 */
struct mgmt_handler {
	mgmt_handler_fn mh_read;
	mgmt_handler_fn mh_write;
};

/**
 * @brief A collection of handlers for an entire command group.
 */
struct mgmt_group {
	/** Entry list node. */
	sys_snode_t node;

	/** Array of handlers; one entry per command ID. */
	const struct mgmt_handler *mg_handlers;
	uint16_t mg_handlers_count;

	/* The numeric ID of this group. */
	uint16_t mg_group_id;
};

/**
 * @brief Registers a full command group.
 *
 * @param group The group to register.
 */
void mgmt_register_group(struct mgmt_group *group);

/**
 * @brief Unregisters a full command group.
 *
 * @param group	The group to register.
 */
void mgmt_unregister_group(struct mgmt_group *group);

/**
 * @brief Finds a registered command handler.
 *
 * @param group_id	The group of the command to find.
 * @param command_id	The ID of the command to find.
 *
 * @return	The requested command handler on success;
 *		NULL on failure.
 */
const struct mgmt_handler *mgmt_find_handler(uint16_t group_id, uint16_t command_id);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* MGMT_MGMT_H_ */
