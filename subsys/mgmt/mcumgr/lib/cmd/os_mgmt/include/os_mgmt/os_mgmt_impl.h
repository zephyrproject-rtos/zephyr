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

/**
 * @file
 * @brief Declares implementation-specific functions required by OS management.
 *        The default stubs can be overridden with functions that are
 *        compatible with the host OS.
 */

#ifndef H_OS_MGMT_IMPL_
#define H_OS_MGMT_IMPL_

#ifdef __cplusplus
extern "C" {
#endif

struct os_mgmt_task_info;

/**
 * @brief Retrieves information about the specified task.  
 *
 * @param idx                   The index of the task to query.
 * @param out_info              On success, the requested information gets
 *                                  written here.
 *
 * @return                      0 on success;
 *                              MGMT_ERR_ENOENT if no such task exists;
 *                              Other MGMT_ERR_[...] code on failure.
 */
int os_mgmt_impl_task_info(int idx, struct os_mgmt_task_info *out_info);

/**
 * @brief Schedules a near-immediate system reset.  There must be a slight
 * delay before the reset occurs to allow time for the mgmt response to be
 * delivered.
 *
 * @return                      0 on success, MGMT_ERR_[...] code on failure.
 */
int os_mgmt_impl_reset(unsigned int delay_ms);

#ifdef __cplusplus
}
#endif

#endif
