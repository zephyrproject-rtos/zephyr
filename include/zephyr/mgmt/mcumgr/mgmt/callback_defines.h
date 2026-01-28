/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MCUMGR_CALLBACK_DEFINES_
#define H_MCUMGR_CALLBACK_DEFINES_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCUmgr Callback API
 * @defgroup mcumgr_callback_api Callbacks
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
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* H_MCUMGR_CALLBACK_DEFINES_ */
