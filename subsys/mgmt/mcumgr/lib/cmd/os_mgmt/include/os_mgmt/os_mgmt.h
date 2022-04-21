/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_OS_MGMT_
#define H_OS_MGMT_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command IDs for OS management group.
 */
#define OS_MGMT_ID_ECHO			0
#define OS_MGMT_ID_CONS_ECHO_CTRL	1
#define OS_MGMT_ID_TASKSTAT		2
#define OS_MGMT_ID_MPSTAT		3
#define OS_MGMT_ID_DATETIME_STR		4
#define OS_MGMT_ID_RESET		5
#define OS_MGMT_ID_MCUMGR_PARAMS	6

#ifdef CONFIG_OS_MGMT_RESET_HOOK
/** @typedef os_mgmt_on_reset_evt_cb
 * @brief Function to be called on os mgmt reset event.
 *
 * This callback function is used to notify the application about a pending
 * reset request and to authorise or deny it.
 *
 * @return 0 to allow reset, MGMT_ERR_[...] code to disallow reset.
 */
typedef int (*os_mgmt_on_reset_evt_cb)(void);
#endif

/**
 * @brief Registers the OS management command handler group.
 */
void os_mgmt_register_group(void);

#ifdef CONFIG_OS_MGMT_RESET_HOOK
/**
 * @brief Register os reset event callback function.
 *
 * @param cb Callback function or NULL to disable.
 */
void os_mgmt_register_reset_evt_cb(os_mgmt_on_reset_evt_cb cb);
#endif

#ifdef __cplusplus
}
#endif

#endif /* H_OS_MGMT_ */
