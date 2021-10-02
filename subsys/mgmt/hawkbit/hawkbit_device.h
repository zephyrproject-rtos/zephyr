/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identiier: Apache-2.0
 */

#ifndef __HAWKBIT_DEVICE_H__
#define __HAWKBIT_DEVICE_H__

#include <zephyr.h>
#include <drivers/hwinfo.h>

#define DEVICE_ID_HEX_MAX_SIZE ((CONFIG_HAWKBIT_DEVID_BIN_MAX_SIZE * 2) + 1)

/**
 * @brief Function to define the device identity for which the hawkbit uses
 * when it connects to the server.
 *
 * @note This is a weak-linked function, and can be overridden if desired.
 *
 * @param id Pointer to the devid buffer
 * @param id_max_len Maximum size of the devid buffer defined as
 * ((CONFIG_HAWKBIT_DEVID_BIN_MAX_SIZE * 2) + 1)
 *
 * @return true if length of devid > 0
 */
bool hawkbit_get_device_identity(char *id, int id_max_len);

#endif /* __HAWKBIT_DEVICE_H__ */
