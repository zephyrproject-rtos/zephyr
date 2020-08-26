/*
 * Copyright (c) 2018-2020 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "updatehub_device.h"

#include <string.h>

bool updatehub_get_device_identity(char *id, int id_max_len)
{
	uint8_t hwinfo_id[DEVICE_ID_BIN_MAX_SIZE];
	size_t length;

	length = hwinfo_get_device_id(hwinfo_id, DEVICE_ID_BIN_MAX_SIZE);
	if (length <= 0) {
		return false;
	}

	memset(id, 0, id_max_len);
	length = bin2hex(hwinfo_id, length, id, id_max_len - 1);

	return length > 0;
}
