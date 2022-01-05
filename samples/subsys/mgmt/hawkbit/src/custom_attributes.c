/*
 * Copyright (c) 2021 Advanced Climate Systems
 *
 * SPDX-License-Identiier: Apache-2.0
 */
#include <mgmt/hawkbit/custom_config_data.h>

int hawkbit_get_config_data(struct hawkbit_cfg_data *data)
{
	data->hwRevision = "v1.0";
	data->customInt = 123456;
	return 0;
}
