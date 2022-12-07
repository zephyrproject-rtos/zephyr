/*
 * Copyright (c) 2018 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UPDATEHUB_DEVICE_H__
#define __UPDATEHUB_DEVICE_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>

#define DEVICE_ID_BIN_MAX_SIZE	64
#define DEVICE_ID_HEX_MAX_SIZE	((DEVICE_ID_BIN_MAX_SIZE * 2) + 1)

bool updatehub_get_device_identity(char *id, int id_max_len);

#endif /* __UPDATEHUB_DEVICE_H__ */
