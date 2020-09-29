/*
 * Copyright 2020 Google LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_EMUL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(emul);

#include <device.h>
#include <emul.h>
#include <string.h>

/**
 * Find a an emulator using its link information
 *
 * @param emul Emulator info to find
 * @return pointer to emulator, or NULL if not found
 */
static const struct emul *
emul_find_by_link(const struct emul_link_for_bus *emul)
{
	const struct emul *erp;

	for (erp = __emul_list_start; erp < __emul_list_end; erp++) {
		if (strcmp(erp->dev_label, emul->label) == 0) {
			return erp;
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

	for (elp = cfg->children; elp < end; elp++) {
		const struct emul *emul = emul_find_by_link(elp);

		__ASSERT_NO_MSG(emul);

		int rc = emul->init(emul, dev);

		if (rc != 0) {
			LOG_WRN("Init %s emulator failed: %d\n",
				 elp->label, rc);
		}
	}

	return 0;
}
