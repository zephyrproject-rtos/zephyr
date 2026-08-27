/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <api/argus_api.h>

/** Used to get instance data from slave index */
#include <../drivers/sensor/broadcom/afbr_s50/platform.h>

argus_hnd_t *Argus_GetHandle(s2pi_slave_t spi_slave)
{
	struct afbr_s50_platform_data *data;
	int err;

	err = afbr_s50_platform_get_by_id(spi_slave, &data);
	if (err) {
		return NULL;
	}

	return data->argus.handle;
}
