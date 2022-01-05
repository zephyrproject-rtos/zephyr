/*
 * Copyright (c) 2021 Advanced Climate Systems
 *
 * SPDX-License-Identiier: Apache-2.0
 */

#include <errno.h>
#include "hawkbit_config_data.h"

#if !defined(CONFIG_HAWKBIT_CUSTOM_CONFIG_DATA)
#include <string.h>
#include "hawkbit_device.h"
#endif

#if defined(CONFIG_HAWKBIT_CUSTOM_CONFIG_DATA)
int __attribute__((__weak__)) hawkbit_get_config_data(struct hawkbit_cfg_data *data)
{
	return -ENOSYS;
}
#else
int hawkbit_get_config_data(struct hawkbit_cfg_data *data)
{
	static char device_id[32];

	if (strlen(device_id) == 0 && !hawkbit_get_device_identity(device_id, sizeof(device_id))) {
		return  -1;
	}
	data->VIN = device_id;
	data->hwRevision = "3";

	return 0;
}
#endif
