/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if !defined(CONFIG_DELTA_SAMPLE_TARGET_MODE)
#include "delta_apply.h"
#endif

LOG_MODULE_REGISTER(delta_sample, LOG_LEVEL_INF);

int main(void)
{
#if defined(CONFIG_DELTA_SAMPLE_TARGET_MODE)
	LOG_INF("Delta target firmware");
	return 0;
#else
	int ret;

	LOG_INF("Delta sample");

	ret = delta_apply_and_reboot();
	if (ret != 0) {
		LOG_ERR("Error while applying delta update: %d", ret);
	}

	return ret;
#endif
}
