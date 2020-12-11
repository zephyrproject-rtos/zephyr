/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identiier: Apache-2.0
 */

#ifndef __HAWKBIT_DEVICE_H__
#define __HAWKBIT_DEVICE_H__

#include <zephyr.h>
#include <drivers/hwinfo.h>

#define DEVICE_ID_BIN_MAX_SIZE	16
#define DEVICE_ID_HEX_MAX_SIZE	((DEVICE_ID_BIN_MAX_SIZE * 2) + 1)

bool hawkbit_get_device_identity(char *id, int id_max_len);

#endif /* __HAWKBIT_DEVICE_H__ */
