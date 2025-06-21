/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FS_VIRTIOFS_H_
#define ZEPHYR_INCLUDE_FS_VIRTIOFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct virtiofs_fs_data {
	uint32_t max_write;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_VIRTIOFS_H_ */
