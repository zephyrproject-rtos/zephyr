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

const struct emul *emul_get_binding(const char *name)
{
	const struct emul *emul_it;

	for (emul_it = __emul_list_start; emul_it < __emul_list_end; emul_it++) {
		if (strcmp(emul_it->dev_label, name) == 0) {
			return emul_it;
		}
	}

	return NULL;
}

int emul_init_for_bus_from_list(const struct device *dev,
				const struct emul_list_for_bus *list)
{
	const struct emul_list_for_bus *cfg = dev->config;

	/*
	 * Walk the list of children, find the corresponding emulator and
	 * initialise it.
	 */
	const struct emul_link_for_bus *elp;
	const struct emul_link_for_bus *const end =
		cfg->children + cfg->num_children;

	LOG_INF("Registering %d emulator(s) for %s", cfg->num_children,
		dev->name);
	for (elp = cfg->children; elp < end; elp++) {
		const struct emul *emul = emul_get_binding(elp->label);

		__ASSERT(emul, "Cannot find emulator for '%s'", elp->label);

		int rc = emul->init(emul, dev);

		if (rc != 0) {
			LOG_WRN("Init %s emulator failed: %d\n",
				 elp->label, rc);
		}
	}

	return 0;
}
