/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_NVS_MGMT_
#define H_NVS_MGMT_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command IDs for non-volatile storage management group.
 */
#define NVS_MGMT_ID_READ_WRITE			0
#define NVS_MGMT_ID_DELETE			1
#define NVS_MGMT_ID_FREE_SPACE			2
#define NVS_MGMT_ID_CLEAR			3

/**
 * @brief Registers the file system management command handler group.
 */
void nvs_mgmt_register_group(void);

#ifdef __cplusplus
}
#endif

#endif
