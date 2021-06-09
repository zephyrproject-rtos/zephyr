/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_OS_MGMT_
#define H_OS_MGMT_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command IDs for OS management group.
 */
#define OS_MGMT_ID_ECHO             0
#define OS_MGMT_ID_CONS_ECHO_CTRL   1
#define OS_MGMT_ID_TASKSTAT         2
#define OS_MGMT_ID_MPSTAT           3
#define OS_MGMT_ID_DATETIME_STR     4
#define OS_MGMT_ID_RESET            5

#define OS_MGMT_TASK_NAME_LEN       32

struct os_mgmt_task_info {
    uint8_t oti_prio;
    uint8_t oti_taskid;
    uint8_t oti_state;
    uint16_t oti_stkusage;
    uint16_t oti_stksize;
    uint32_t oti_cswcnt;
    uint32_t oti_runtime;
    uint32_t oti_last_checkin;
    uint32_t oti_next_checkin;

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
