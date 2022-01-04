/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_FS_MGMT_
#define H_FS_MGMT_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command IDs for file system management group.
 */
#define FS_MGMT_ID_FILE	 0

/**
 * @brief Registers the file system management command handler group.
 */
void fs_mgmt_register_group(void);

#ifdef __cplusplus
}
#endif

#endif
