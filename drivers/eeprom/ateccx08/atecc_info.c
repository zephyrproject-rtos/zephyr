/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "atecc_priv.h"
LOG_MODULE_DECLARE(ateccx08);

#define INFO_MODE_REVISION 0x00U

static int atecc_info_base(const struct device *dev, uint8_t mode, uint16_t param2,
			   uint8_t *out_data)
{
	__ASSERT(dev != NULL, "device is NULL");
	__ASSERT(out_data != NULL, "out_data is NULL");

	struct ateccx08_packet packet = {0};
	int ret;

	packet.param1 = mode;
	packet.param2 = param2;

	atecc_command(ATECC_INFO, &packet);

	ret = atecc_execute_command_pm(dev, &packet);
	if (ret < 0) {
		LOG_ERR("%s: failed: %d", __func__, ret);
		return ret;
	}

	memcpy(out_data, &packet.data[ATECC_RSP_DATA_IDX], 4);

	return ret;
}

int atecc_info(const struct device *dev, uint8_t *revision)
{
	__ASSERT(revision != NULL, "revision is NULL");

	return atecc_info_base(dev, INFO_MODE_REVISION, 0, revision);
}
