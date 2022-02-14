/*
 * Copyright (c) 2018-2022 mcumgr authors
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
#define FS_MGMT_ID_FILE			 0
#define FS_MGMT_ID_STAT			 1
#define FS_MGMT_ID_HASH_CHECKSUM	 2

/**
 * @brief Registers the file system management command handler group.
 */
void fs_mgmt_register_group(void);

#ifdef __cplusplus
}
#endif

#endif
