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
 * @brief Declares implementation-specific functions required by log
 *        management.  The default stubs can be overridden with functions that
 *        are compatible with the host OS.
 */

#ifndef H_LOG_MGMT_IMPL_
#define H_LOG_MGMT_IMPL_

#ifdef __cplusplus
extern "C" {
#endif

struct log_mgmt_filter;
struct log_mgmt_entry;
struct log_mgmt_log;

typedef int log_mgmt_foreach_entry_fn(const struct log_mgmt_entry *entry,
                                      void *arg);

/**
 * @brief Retrieves the log at the specified index.
 *
 * @param idx                   The index of the log to retrieve.
 * @param out_name              On success, the requested log gets written
 *                                   here.
 *
 * @return                      0 on success;
 *                              MGMT_ERR_ENOENT if no log with the specified
 *                                  index exists;
 *                              Other MGMT_ERR_[...] code on failure.
 */
int log_mgmt_impl_get_log(int idx, struct log_mgmt_log *out_log);

/**
 * @brief Retrieves the name of log module at the specified index.
 *
 * @param idx                   The index of the log module to retrieve.
 * @param out_name              On success, the requested module's name gets
 *                                  written here.
 *
 * @return                      0 on success;
 *                              MGMT_ERR_ENOENT if no log module with the
 *                                  specified index exists;
 *                              Other MGMT_ERR_[...] code on failure.
 */
int log_mgmt_impl_get_module(int idx, const char **out_module_name);

/**
 * @brief Retrieves the name of log level at the specified index.
 *
 * @param idx                   The index of the log level to retrieve.
 * @param out_name              On success, the requested level's name gets
 *                                  written here.
 *
 * @return                      0 on success;
 *                              MGMT_ERR_ENOENT if no log level with the
 *                                  specified index exists;
 *                              Other MGMT_ERR_[...] code on failure.
 */
int log_mgmt_impl_get_level(int idx, const char **out_level_name);

/**
 * @brief Retrieves the index that the next log entry will use.
 *
 * @param out_idx               On success, the next index gets written here.
 *
 * @return                      0 on success; MGMT_ERR_[...] code on failure.
 */
int log_mgmt_impl_get_next_idx(uint32_t *out_idx);

/**
 * @brief Applies a function to every matching entry in the specified log.
 *
 * @param log_name              The name of the log to operate on.
 * @param filter                Specifies which log entries to operate on.
 * @param cb                    The callback to apply to each log entry.
 * @param arg                   An optional argument to pass to the callback.
 *
 * @return                      0 on success; MGMT_ERR_[...] code on failure.
 */
int log_mgmt_impl_foreach_entry(const char *log_name,
                                const struct log_mgmt_filter *filter,
                                log_mgmt_foreach_entry_fn *cb,
                                void *arg);

/**
 * @brief Clear the log with the specified name.
 *
 * @param log_name              The name of the log to clear.
 *
 * @return                      0 on success; MGMT_ERR_[...] code on failure.
 */
int log_mgmt_impl_clear(const char *log_name);

#ifdef __cplusplus
}
#endif

#endif
