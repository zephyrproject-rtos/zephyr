/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SHELL_MGMT_
#define H_SHELL_MGMT_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command IDs for shell management group.
 */
#define SHELL_MGMT_ID_EXEC   0

/**
 * @brief Registers the shell management command handler group.
 */
void
shell_mgmt_register_group(void);

#ifdef __cplusplus
}
#endif

#endif /* H_SHELL_MGMT_ */
