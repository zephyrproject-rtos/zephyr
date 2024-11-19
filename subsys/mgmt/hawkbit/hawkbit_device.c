/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "hawkbit_device.h"
#include <string.h>
#include <zephyr/mgmt/hawkbit/hawkbit.h>

static bool hawkbit_get_device_identity_default(char *id, int id_max_len);

static hawkbit_get_device_identity_cb_handler_t hawkbit_get_device_identity_cb_handler =
	hawkbit_get_device_identity_default;

bool hawkbit_get_device_identity(char *id, int id_max_len)
{
	return hawkbit_get_device_identity_cb_handler(id, id_max_len);
}

static bool hawkbit_get_device_identity_default(char *id, int id_max_len)
{
#ifdef CONFIG_HWINFO
	uint8_t hwinfo_id[DEVICE_ID_BIN_MAX_SIZE];
	ssize_t length;

	length = hwinfo_get_device_id(hwinfo_id, DEVICE_ID_BIN_MAX_SIZE);
	if (length <= 0) {
		return false;
	}

	memset(id, 0, id_max_len);
	length = bin2hex(hwinfo_id, (size_t)length, id, id_max_len);

	return length > 0;
#else /* CONFIG_HWINFO */
	ARG_UNUSED(id);
	ARG_UNUSED(id_max_len);

	return false;
#endif /* CONFIG_HWINFO */
}

#ifdef CONFIG_HAWKBIT_CUSTOM_DEVICE_ID
int hawkbit_set_device_identity_cb(hawkbit_get_device_identity_cb_handler_t cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	hawkbit_get_device_identity_cb_handler = cb;

	return 0;
}
#endif /* CONFIG_HAWKBIT_CUSTOM_DEVICE_ID */
