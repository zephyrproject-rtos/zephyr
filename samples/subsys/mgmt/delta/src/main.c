/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if !defined(CONFIG_DELTA_SAMPLE_TARGET_MODE)
#include <zephyr/mgmt/delta/delta.h>
#include "delta_apply.h"
#endif

LOG_MODULE_REGISTER(delta_sample, LOG_LEVEL_INF);

#if !defined(CONFIG_DELTA_SAMPLE_TARGET_MODE)
static struct delta_ctx ctx;
#endif

int main(void)
{
#if defined(CONFIG_DELTA_SAMPLE_TARGET_MODE)
	LOG_INF("DFOTA target firmware");
	return 0;
#else
	int ret;

	LOG_INF("Delta sample");

	ret = delta_apply(&ctx);
	if (ret != 0) {
		LOG_ERR("Error while applying delta update: %d", ret);
	}

	return ret;
#endif
}
