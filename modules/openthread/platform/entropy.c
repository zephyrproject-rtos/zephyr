/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>

#include <openthread/platform/entropy.h>

LOG_MODULE_REGISTER(net_otPlat_entropy, CONFIG_OPENTHREAD_L2_LOG_LEVEL);

#if !defined(CONFIG_CSPRNG_ENABLED)
#error OpenThread requires an entropy source for a TRNG
#endif

otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
{
	int err;

	if ((aOutput == NULL) || (aOutputLength == 0)) {
		return OT_ERROR_INVALID_ARGS;
	}

	err = sys_csrand_get(aOutput, aOutputLength);
	if (err != 0) {
		LOG_ERR("Failed to obtain entropy, err %d", err);
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}
