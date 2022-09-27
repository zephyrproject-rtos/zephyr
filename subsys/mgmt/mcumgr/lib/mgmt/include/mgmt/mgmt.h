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
#include <smp/smp.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opcodes; encoded in first byte of header. */
#define MGMT_OP_READ		0
#define MGMT_OP_READ_RSP	1
#define MGMT_OP_WRITE		2
#define MGMT_OP_WRITE_RSP	3

/**
 * The first 64 groups are reserved for system level mcumgr commands.
 * Per-user commands are then defined after group 64.
 */
#define MGMT_GROUP_ID_OS	0
#define MGMT_GROUP_ID_IMAGE	1
#define MGMT_GROUP_ID_STAT	2
#define MGMT_GROUP_ID_CONFIG	3
#define MGMT_GROUP_ID_LOG	4
#define MGMT_GROUP_ID_CRASH	5
#define MGMT_GROUP_ID_SPLIT	6
#define MGMT_GROUP_ID_RUN	7
#define MGMT_GROUP_ID_FS	8
#define MGMT_GROUP_ID_SHELL	9
#define MGMT_GROUP_ID_PERUSER	64

/**
 * mcumgr error codes.
 */
#define MGMT_ERR_EOK		0
#define MGMT_ERR_EUNKNOWN	1
#define MGMT_ERR_ENOMEM		2
#define MGMT_ERR_EINVAL		3
#define MGMT_ERR_ETIMEOUT	4
#define MGMT_ERR_ENOENT		5
#define MGMT_ERR_EBADSTATE	6	/* Current state disallows command. */
#define MGMT_ERR_EMSGSIZE	7	/* Response too large. */
#define MGMT_ERR_ENOTSUP	8	/* Command not supported. */
#define MGMT_ERR_ECORRUPT	9	/* Corrupt */
#define MGMT_ERR_EBUSY		10	/* Command blocked by processing of other command */
#define MGMT_ERR_EACCESSDENIED	11	/* Access to specific function or resource denied */
#define MGMT_ERR_EPERUSER	256

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

#ifdef CONFIG_MGMT_VERBOSE_ERR_RESPONSE
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
 * @return 0 if a response was successfully encoded, MGMT_ERR_[...] code on failure.
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

#ifdef __cplusplus
}
#endif

#endif /* MGMT_MGMT_H_ */
