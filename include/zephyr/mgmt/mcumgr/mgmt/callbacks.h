/*
 * Copyright (c) 2022-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MCUMGR_CALLBACKS_
#define H_MCUMGR_CALLBACKS_

#include <inttypes.h>
#include <zephyr/sys/slist.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include "callback_defines.h"

#if defined(CONFIG_MCUMGR_GRP_FS) || defined(__DOXYGEN__)
#include <zephyr/mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_callbacks.h>
#endif

#if defined(CONFIG_MCUMGR_GRP_IMG) || defined(__DOXYGEN__)
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt_callbacks.h>
#endif

#if defined(CONFIG_MCUMGR_GRP_OS) || defined(__DOXYGEN__)
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt_callbacks.h>
#endif

#if defined(CONFIG_MCUMGR_GRP_SETTINGS) || defined(__DOXYGEN__)
#include <zephyr/mgmt/mcumgr/grp/settings_mgmt/settings_mgmt_callbacks.h>
#endif

#if defined(CONFIG_MCUMGR_GRP_ENUM) || defined(__DOXYGEN__)
#include <zephyr/mgmt/mcumgr/grp/enum_mgmt/enum_mgmt_callbacks.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCUmgr Callback API
 * @defgroup mcumgr_callback_api Callbacks
 * @ingroup mcumgr
 * @{
 */

/**
 * @typedef mgmt_cb
 * @brief Function to be called on MGMT notification/event.
 *
 * This callback function is used to notify an application or system about a MCUmgr mgmt event.
 *
 * @param event		#mcumgr_op_t.
 * @param prev_status	#mgmt_cb_return of the previous handler calls, if it is an error then it
 *			will be the first error that was returned by a handler (i.e. this handler
 *			is being called for a notification only, the return code will be ignored).
 * @param rc		If ``prev_status`` is #MGMT_CB_ERROR_RC then this is the SMP error that
 *			was returned by the first handler that failed. If ``prev_status`` is
 *			#MGMT_CB_ERROR_ERR then this will be the group error rc code returned by
 *			the first handler that failed. If the handler wishes to raise an SMP
 *			error, this must be set to the #mcumgr_err_t status and #MGMT_CB_ERROR_RC
 *			must be returned by the function, if the handler wishes to raise a ret
 *			error, this must be set to the group ret status and #MGMT_CB_ERROR_ERR
 *			must be returned by the function.
 * @param group		If ``prev_status`` is #MGMT_CB_ERROR_ERR then this is the group of the
 *			ret error that was returned by the first handler that failed. If the
 *			handler wishes to raise a ret error, this must be set to the group ret
 *			status and #MGMT_CB_ERROR_ERR must be returned by the function.
 * @param abort_more	Set to true to abort further processing by additional handlers.
 * @param data		Optional event argument.
 * @param data_size	Size of optional event argument (0 if no data is provided).
 *
 * @return		#mgmt_cb_return indicating the status to return to the calling code (only
 *			checked when this is the first failure reported by a handler).
 */
typedef enum mgmt_cb_return (*mgmt_cb)(uint32_t event, enum mgmt_cb_return prev_status,
				       int32_t *rc, uint16_t *group, bool *abort_more, void *data,
				       size_t data_size);

/**
 * MGMT event opcodes for base SMP command processing.
 */
enum smp_group_events {
	/** Callback when a command is received, data is @ref mgmt_evt_op_cmd_arg. */
	MGMT_EVT_OP_CMD_RECV			= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_SMP, 0),

	/** Callback when a status is updated, data is @ref mgmt_evt_op_cmd_arg. */
	MGMT_EVT_OP_CMD_STATUS			= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_SMP, 1),

	/** Callback when a command has been processed, data is @ref mgmt_evt_op_cmd_arg. */
	MGMT_EVT_OP_CMD_DONE			= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_SMP, 2),

	/** Used to enable all smp_group events. */
	MGMT_EVT_OP_CMD_ALL			= MGMT_DEF_EVT_OP_ALL(MGMT_EVT_GRP_SMP),
};

/**
 * MGMT callback struct
 */
struct mgmt_callback {
	/** Entry list node. */
	sys_snode_t node;

	/** Callback that will be called. */
	mgmt_cb callback;

	/**
	 * MGMT_EVT_[...] Event ID for handler to be called on. This has special meaning if
	 * #MGMT_EVT_OP_ALL is used (which will cover all events for all groups), or
	 * MGMT_EVT_OP_*_MGMT_ALL (which will cover all events for a single group). For events
	 * that are part of a single group, they can be or'd together for this to have one
	 * registration trigger on multiple events, please note that this will only work for
	 * a single group, to register for events in different groups, they must be registered
	 * separately.
	 */
	uint32_t event_id;
};

/**
 * Arguments for #MGMT_EVT_OP_CMD_RECV, #MGMT_EVT_OP_CMD_STATUS and #MGMT_EVT_OP_CMD_DONE
 */
struct mgmt_evt_op_cmd_arg {
	/** #mcumgr_group_t */
	uint16_t group;

	/** Message ID within group */
	uint8_t id;

	union {
		/** #mcumgr_op_t used in #MGMT_EVT_OP_CMD_RECV */
		uint8_t op;

		/** #mcumgr_err_t, used in #MGMT_EVT_OP_CMD_DONE */
		int err;

		/** #img_mgmt_id_upload_t, used in #MGMT_EVT_OP_CMD_STATUS */
		int status;
	};
};

/**
 * @brief Get event ID index from event.
 *
 * @param event		Event to get ID index from.
 *
 * @return		Event index.
 */
uint8_t mgmt_evt_get_index(uint32_t event);

/**
 * @brief This function is called to notify registered callbacks about mcumgr notifications/events.
 *
 * @param event		#mcumgr_op_t.
 * @param data		Optional event argument.
 * @param data_size	Size of optional event argument (0 if none).
 * @param err_rc	Pointer to rc value.
 * @param err_group	Pointer to group value.
 *
 * @return		#mgmt_cb_return either #MGMT_CB_OK if all handlers returned it, or
 *			#MGMT_CB_ERROR_RC if the first failed handler returned an SMP error (in
 *			which case ``err_rc`` will be updated with the SMP error) or
 *			#MGMT_CB_ERROR_ERR if the first failed handler returned a ret group and
 *			error (in which case ``err_group`` will be updated with the failed group
 *			ID and ``err_rc`` will be updated with the group-specific error code).
 */
enum mgmt_cb_return mgmt_callback_notify(uint32_t event, void *data, size_t data_size,
					 int32_t *err_rc, uint16_t *err_group);

/**
 * @brief Register event callback function.
 *
 * @param callback Callback struct.
 */
void mgmt_callback_register(struct mgmt_callback *callback);

/**
 * @brief Unregister event callback function.
 *
 * @param callback Callback struct.
 */
void mgmt_callback_unregister(struct mgmt_callback *callback);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* H_MCUMGR_CALLBACKS_ */
