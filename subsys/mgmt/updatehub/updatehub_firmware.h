/*
 * Copyright (c) 2018-2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UPDATEHUB_FIRMWARE_H__
#define __UPDATEHUB_FIRMWARE_H__

/* 255.255.65535\0 */
#define FIRMWARE_IMG_VER_STRLEN_MAX 14

bool updatehub_get_firmware_version(const uint32_t partition_id,
				    char *version, int version_len);

#endif /* __UPDATEHUB_FIRMWARE_H__ */
