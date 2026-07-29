/*
 * Copyright (c) 2026 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc_prep_hook, LOG_LEVEL_DBG);

void soc_prep_hook(void)
{
	const struct device *smc = DEVICE_DT_GET(DT_NODELABEL(smc));

	/* note: we are before bss/data preparation. memory can be dirty */
	if (device_is_ready(smc)) {
		/* if the device appears to be ready, we force it to be re-initialized. */
		smc->state->initialized = false;
	}

	/* initialize the SRAM controller before relocation/bss/data preparation. */
	(void)device_init(smc);
}
