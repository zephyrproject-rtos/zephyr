/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>

#include <openthread/platform/random.h>

#include "platform-zephyr.h"

uint32_t otPlatRandomGet(void)
{
	return sys_rand32_get();
}

otError otPlatRandomGetTrue(u8_t *aOutput, u16_t aOutputLength)
{
	int i;
	u32_t random;

	if (!aOutput) {
		return OT_ERROR_INVALID_ARGS;
	}

	for (i = 0; i < (aOutputLength & 0xFFFC); i += 4) {
		random = sys_rand32_get();
		memcpy(&aOutput[i], &random, sizeof(random));
	}

	random = sys_rand32_get();
	memcpy(&aOutput[i], &random, aOutputLength & 0x0003);

	return OT_ERROR_NONE;
}
