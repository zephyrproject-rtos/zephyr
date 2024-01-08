/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MGMT_MGMT_
#define H_MGMT_MGMT_

#include <inttypes.h>
#include <zephyr/sys/slist.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt_defines.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCUmgr mgmt API
 * @defgroup mcumgr_mgmt_api MCUmgr mgmt API
 * @ingroup mcumgr
 * @{
 */

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
#if IS_ENABLED(CONFIG_MCUMGR_MGMT_HANDLER_USER_DATA)
	void *user_data;
#endif
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

	/** The numeric ID of this group. */
	uint16_t mg_group_id;

#if IS_ENABLED(CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL)
	/** A function handler for translating version 2 SMP error codes to version 1 SMP error
	 * codes (optional)
	 */
	smp_translate_error_fn mg_translate_error;
#endif
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
 * @brief Finds a registered command group.
 *
 * @param group_id	The command group id to find.
 *
 * @return	The requested command group on success;
 *		NULL on failure.
 */
const struct mgmt_group *mgmt_find_group(uint16_t group_id);

#if IS_ENABLED(CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL)
/**
 * @brief		Finds a registered error translation function for converting from SMP
 *			version 2 error codes to legacy SMP version 1 error codes.
 *
 * @param group_id	The group of the translation function to find.
 *
 * @return		Requested lookup function on success.
 * @return		NULL on failure.
 */
smp_translate_error_fn mgmt_find_error_translation_function(uint16_t group_id);
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* MGMT_MGMT_H_ */
