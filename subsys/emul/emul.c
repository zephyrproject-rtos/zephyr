/*
 * Copyright 2020 Google LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_EMUL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emul);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <string.h>

int emul_init_for_bus_from_list(const struct device *dev,
				const struct emul_list_for_bus *list)
{
	const struct emul_list_for_bus *cfg = dev->config;

	/*
	 * Walk the list of children, find the corresponding emulator and
	 * initialise it.
	 */
	LOG_INF("Registering %d emulator(s) for %s", cfg->num_children,
		dev->name);

	for (unsigned int i = 0U; i < cfg->num_children; i++) {
		const struct emul *emul = cfg->children[i];

		int rc = emul->init(emul, dev);

		if (rc != 0) {
			LOG_WRN("Init %s emulator failed: %d\n",
				emul->name, rc);
		}
	}

	return 0;
}
