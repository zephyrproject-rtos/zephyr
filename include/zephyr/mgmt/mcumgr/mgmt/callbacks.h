/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MCUMGR_CALLBACKS_
#define H_MCUMGR_CALLBACKS_

#include <inttypes.h>
#include <zephyr/sys/slist.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>

#ifdef CONFIG_MCUMGR_GRP_FS
#include <zephyr/mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_callbacks.h>
#endif

#ifdef CONFIG_MCUMGR_GRP_IMG
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt_callbacks.h>
#endif

#ifdef CONFIG_MCUMGR_GRP_OS
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt_callbacks.h>
#endif

#ifdef CONFIG_MCUMGR_GRP_SETTINGS
#include <zephyr/mgmt/mcumgr/grp/settings_mgmt/settings_mgmt_callbacks.h>
#endif

#ifdef CONFIG_MCUMGR_GRP_ENUM
#include <zephyr/mgmt/mcumgr/grp/enum_mgmt/enum_mgmt_callbacks.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCUmgr callback API
 * @defgroup mcumgr_callback_api MCUmgr callback API
 * @ingroup mcumgr
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/** Event which signifies that all event IDs for a particular group should be enabled. */
#define MGMT_EVT_OP_ID_ALL 0xffff

/** Get event for a particular group and event ID. */
#define MGMT_DEF_EVT_OP_ID(group, event_id) ((group << 16) | BIT(event_id))

/** Get event used for enabling all event IDs of a particular group. */
#define MGMT_DEF_EVT_OP_ALL(group) ((group << 16) | MGMT_EVT_OP_ID_ALL)
/** @endcond  */

/** Get group from event. */
#define MGMT_EVT_GET_GROUP(event) ((event >> 16) & MGMT_EVT_OP_ID_ALL)

/** Get event ID from event. */
#define MGMT_EVT_GET_ID(event) (event & MGMT_EVT_OP_ID_ALL)

/**
 * MGMT event callback return value.
 */
enum mgmt_cb_return {
	/** No error. */
	MGMT_CB_OK,

	/** SMP protocol error and ``err_rc`` contains the #mcumgr_err_t error code. */
	MGMT_CB_ERROR_RC,

	/**
	 * Group (application-level) error and ``err_group`` contains the group ID that caused
	 * the error and ``err_rc`` contains the error code of that group to return.
	 */
	MGMT_CB_ERROR_ERR,
};

/* Deprecated after Zephyr 3.4, use MGMT_CB_ERROR_ERR instead */
#define MGMT_CB_ERROR_RET __DEPRECATED_MACRO MGMT_CB_ERROR_ERR

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
 * MGMT event callback group IDs. Note that this is not a 1:1 mapping with #mcumgr_group_t values.
 */
enum mgmt_cb_groups {
	MGMT_EVT_GRP_ALL			= 0,
	MGMT_EVT_GRP_SMP,
	MGMT_EVT_GRP_OS,
	MGMT_EVT_GRP_IMG,
	MGMT_EVT_GRP_FS,
	MGMT_EVT_GRP_SETTINGS,
	MGMT_EVT_GRP_ENUM,

	MGMT_EVT_GRP_USER_CUSTOM_START		= MGMT_GROUP_ID_PERUSER,
};

/**
 * MGMT event opcodes for all command processing.
 */
enum smp_all_events {
	/** Used to enable all events. */
	MGMT_EVT_OP_ALL				= MGMT_DEF_EVT_OP_ALL(MGMT_EVT_GRP_ALL),
};

/**
 * MGMT event opcodes for base SMP command processing.
 */
enum smp_group_events {
	/** Callback when a command is received, data is mgmt_evt_op_cmd_arg(). */
	MGMT_EVT_OP_CMD_RECV			= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_SMP, 0),

	/** Callback when a status is updated, data is mgmt_evt_op_cmd_arg(). */
	MGMT_EVT_OP_CMD_STATUS			= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_SMP, 1),

	/** Callback when a command has been processed, data is mgmt_evt_op_cmd_arg(). */
	MGMT_EVT_OP_CMD_DONE			= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_SMP, 2),

	/** Used to enable all smp_group events. */
	MGMT_EVT_OP_CMD_ALL			= MGMT_DEF_EVT_OP_ALL(MGMT_EVT_GRP_SMP),
};

/**
 * MGMT event opcodes for filesystem management group.
 */
enum fs_mgmt_group_events {
	/** Callback when a file has been accessed, data is fs_mgmt_file_access(). */
	MGMT_EVT_OP_FS_MGMT_FILE_ACCESS		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_FS, 0),

	/** Used to enable all fs_mgmt_group events. */
	MGMT_EVT_OP_FS_MGMT_ALL			= MGMT_DEF_EVT_OP_ALL(MGMT_EVT_GRP_FS),
};

/**
 * MGMT event opcodes for image management group.
 */
enum img_mgmt_group_events {
	/** Callback when a client sends a file upload chunk, data is img_mgmt_upload_check(). */
	MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK			= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 0),

	/** Callback when a DFU operation is stopped. */
	MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 1),

	/** Callback when a DFU operation is started. */
	MGMT_EVT_OP_IMG_MGMT_DFU_STARTED		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 2),

	/** Callback when a DFU operation has finished being transferred. */
	MGMT_EVT_OP_IMG_MGMT_DFU_PENDING		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 3),

	/** Callback when an image has been confirmed. */
	MGMT_EVT_OP_IMG_MGMT_DFU_CONFIRMED		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 4),

	/** Callback when an image write command has finished writing to flash. */
	MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK_WRITE_COMPLETE	= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 5),

	/** Callback when an image slot's state is encoded for a response. */
	MGMT_EVT_OP_IMG_MGMT_IMAGE_SLOT_STATE		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 6),

	/** Callback when an slot list command outputs fields for an image. */
	MGMT_EVT_OP_IMG_MGMT_SLOT_INFO_IMAGE		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 7),

	/** Callback when an slot list command outputs fields for a slot of an image. */
	MGMT_EVT_OP_IMG_MGMT_SLOT_INFO_SLOT		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_IMG, 8),

	/** Used to enable all img_mgmt_group events. */
	MGMT_EVT_OP_IMG_MGMT_ALL			= MGMT_DEF_EVT_OP_ALL(MGMT_EVT_GRP_IMG),
};

/**
 * MGMT event opcodes for operating system management group.
 */
enum os_mgmt_group_events {
	/** Callback when a reset command has been received, data is os_mgmt_reset_data. */
	MGMT_EVT_OP_OS_MGMT_RESET		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_OS, 0),

	/** Callback when an info command is processed, data is os_mgmt_info_check. */
	MGMT_EVT_OP_OS_MGMT_INFO_CHECK		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_OS, 1),

	/** Callback when an info command needs to output data, data is os_mgmt_info_append. */
	MGMT_EVT_OP_OS_MGMT_INFO_APPEND		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_OS, 2),

	/** Callback when a datetime get command has been received. */
	MGMT_EVT_OP_OS_MGMT_DATETIME_GET	= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_OS, 3),

	/** Callback when a datetime set command has been received, data is struct rtc_time(). */
	MGMT_EVT_OP_OS_MGMT_DATETIME_SET	= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_OS, 4),

	/**
	 * Callback when a bootloader info command has been received, data is
	 * os_mgmt_bootloader_info_data.
	 */
	MGMT_EVT_OP_OS_MGMT_BOOTLOADER_INFO	= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_OS, 5),

	/** Used to enable all os_mgmt_group events. */
	MGMT_EVT_OP_OS_MGMT_ALL			= MGMT_DEF_EVT_OP_ALL(MGMT_EVT_GRP_OS),
};

/**
 * MGMT event opcodes for settings management group.
 */
enum settings_mgmt_group_events {
	/** Callback when a setting is read/written/deleted. */
	MGMT_EVT_OP_SETTINGS_MGMT_ACCESS	= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_SETTINGS, 0),

	/** Used to enable all settings_mgmt_group events. */
	MGMT_EVT_OP_SETTINGS_MGMT_ALL		= MGMT_DEF_EVT_OP_ALL(MGMT_EVT_GRP_SETTINGS),
};

/**
 * MGMT event opcodes for enumeration management group.
 */
enum enum_mgmt_group_events {
	/** Callback when fetching details on supported command groups. */
	MGMT_EVT_OP_ENUM_MGMT_DETAILS		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_ENUM, 0),

	/** Used to enable all enum_mgmt_group events. */
	MGMT_EVT_OP_ENUM_MGMT_ALL		= MGMT_DEF_EVT_OP_ALL(MGMT_EVT_GRP_ENUM),
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
