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
 * @brief Declares implementation-specific functions required by shell
 *        management.  The default stubs can be overridden with functions that
 *        are compatible with the host OS.
 */

#ifndef H_SHELL_MGMT_IMPL_
#define H_SHELL_MGMT_IMPL_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Execute `line` as a shell command
 *
 * @param line : shell command to be executed
 * @return int : 0 on success, -errno otherwire
 */
int
shell_mgmt_impl_exec(const char *line);

/**
 * @brief Capture the output of the shell
 *
 * @return const char* : shell output. This is not the return code, it is
 * the string output of the shell command if it exists. If the shell provided no
 * output, this will be an empty string
 */
const char *
shell_mgmt_impl_get_output();

#ifdef __cplusplus
}
#endif

#endif
