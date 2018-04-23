/*
 * Copyright (c) 2018 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "updatehub_device.h"

bool updatehub_get_device_identity(char *id, int id_max_len)
{
	int i, id_len = 0, buf_len = 0;
	char buf[3];
	u8_t hwinfo_id[id_max_len];
	size_t length;

	length = hwinfo_get_device_id(hwinfo_id, sizeof(hwinfo_id) - 1);
	if (length <= 0) {
		return false;
	}

	memset(id, 0, id_max_len);

	for (i = 0; i < length; i++) {
		snprintk(buf, sizeof(buf), "%02x", hwinfo_id[i]);
		id_len = strlen(id);
		strncat(id, buf, id_max_len - id_len);
	}

	return true;
}
