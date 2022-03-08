/*
 * Copyright (c) 2018-2021 mcumgr authors
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

/**
 * @brief Registers the OS management command handler group.
 */
void os_mgmt_register_group(void);

#ifdef __cplusplus
}
#endif

#endif /* H_OS_MGMT_ */
