/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include <drivers/entropy.h>
#include <logging/log.h>

#include <openthread/platform/entropy.h>

#include "platform-zephyr.h"

LOG_MODULE_REGISTER(net_otPlat_entropy, CONFIG_OPENTHREAD_L2_LOG_LEVEL);

#if !defined(CONFIG_ENTROPY_HAS_DRIVER)
#error OpenThread requires an entropy source for a TRNG
#endif

otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
{
	/* static to obtain it once in a first call */
	static const struct device *dev;
	int err;

	if ((aOutput == NULL) || (aOutputLength == 0)) {
		return OT_ERROR_INVALID_ARGS;
	}

	if (dev == NULL) {
		dev = device_get_binding(DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
		if (dev == NULL) {
			LOG_ERR("Failed to obtain entropy device");
			return OT_ERROR_FAILED;
		}
	}

	err = entropy_get_entropy(dev, aOutput, aOutputLength);
	if (err != 0) {
		LOG_ERR("Failed to obtain entropy, err %d", err);
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}
