/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief VirtioFS file system data structures.
 * @ingroup file_system_api
 */

#ifndef ZEPHYR_INCLUDE_FS_VIRTIOFS_H_
#define ZEPHYR_INCLUDE_FS_VIRTIOFS_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief File system data for a VirtioFS mount */
struct virtiofs_fs_data {
	/** Maximum size of a single write request, in bytes */
	uint32_t max_write;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_VIRTIOFS_H_ */
