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

#define OS_MGMT_TASK_NAME_LEN		32

struct os_mgmt_task_info {
	uint8_t oti_prio;
	uint8_t oti_taskid;
	uint8_t oti_state;
	uint16_t oti_stkusage;
	uint16_t oti_stksize;
#ifndef CONFIG_OS_MGMT_TASKSTAT_ONLY_SUPPORTED_STATS
	uint32_t oti_cswcnt;
	uint32_t oti_runtime;
	uint32_t oti_last_checkin;
	uint32_t oti_next_checkin;
#endif

	char oti_name[OS_MGMT_TASK_NAME_LEN];
};

/**
 * @brief Registers the OS management command handler group.
 */
void os_mgmt_register_group(void);

#ifdef __cplusplus
}
#endif

#endif /* H_OS_MGMT_ */
