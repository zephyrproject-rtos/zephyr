/*
 * Copyright (c) 2018 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UPDATEHUB_DEVICE_H__
#define __UPDATEHUB_DEVICE_H__

#include <zephyr.h>
#include <drivers/hwinfo.h>

#define DEVICE_ID_MAX_SIZE 65

bool updatehub_get_device_identity(char *id, int id_max_len);

#endif /* __UPDATEHUB_DEVICE_H__ */
