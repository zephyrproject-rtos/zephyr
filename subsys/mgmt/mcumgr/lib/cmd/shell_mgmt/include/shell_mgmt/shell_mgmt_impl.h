/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
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
 * @return int : 0 on success, -errno otherwise
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
