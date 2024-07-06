/*
 * Copyright (c) 2018-2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/hwinfo.h>

#include "updatehub_device.h"

bool updatehub_get_device_identity(char *id, int id_max_len)
{
	uint8_t hwinfo_id[DEVICE_ID_BIN_MAX_SIZE];
	k_ssize_t length;

	length = hwinfo_get_device_id(hwinfo_id, DEVICE_ID_BIN_MAX_SIZE);
	if (length <= 0) {
		return false;
	}

	memset(id, 0, id_max_len);
	length = bin2hex(hwinfo_id, (size_t)length, id, id_max_len);

	return length > 0;
}
