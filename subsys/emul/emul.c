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
#include <zephyr/sys/iterable_sections.h>
#include <string.h>

const struct emul *emul_get_binding(const char *name)
{
	STRUCT_SECTION_FOREACH(emul, emul_it) {
		if (strcmp(emul_it->dev->name, name) == 0) {
			return emul_it;
		}
	}

	return NULL;
}

int emul_init_for_bus(const struct device *dev)
{
	const struct emul_list_for_bus *cfg = dev->config;

	/*
	 * Walk the list of children, find the corresponding emulator and
	 * initialise it.
	 */
	const struct emul_link_for_bus *elp;
	const struct emul_link_for_bus *const end = cfg->children + cfg->num_children;

	LOG_INF("Registering %d emulator(s) for %s", cfg->num_children, dev->name);
	for (elp = cfg->children; elp < end; elp++) {
		const struct emul *emul = emul_get_binding(elp->dev->name);

		if (!emul) {
			LOG_WRN("Cannot find emulator for '%s'", elp->dev->name);
			continue;
		}

		switch (emul->bus_type) {
		case EMUL_BUS_TYPE_I2C:
			emul->bus.i2c->target = emul;
			break;
		case EMUL_BUS_TYPE_ESPI:
			emul->bus.espi->target = emul;
			break;
		case EMUL_BUS_TYPE_SPI:
			emul->bus.spi->target = emul;
			break;
		case EMUL_BUS_TYPE_NONE:
			break;
		}
		int rc = emul->init(emul, dev);

		if (rc != 0) {
			LOG_WRN("Init %s emulator failed: %d",
				 elp->dev->name, rc);
		}

		switch (emul->bus_type) {
#ifdef CONFIG_I2C_EMUL
		case EMUL_BUS_TYPE_I2C:
			rc = i2c_emul_register(dev, emul->bus.i2c);
			break;
#endif /* CONFIG_I2C_EMUL */
#ifdef CONFIG_ESPI_EMUL
		case EMUL_BUS_TYPE_ESPI:
			rc = espi_emul_register(dev, emul->bus.espi);
			break;
#endif /* CONFIG_ESPI_EMUL */
#ifdef CONFIG_SPI_EMUL
		case EMUL_BUS_TYPE_SPI:
			rc = spi_emul_register(dev, emul->bus.spi);
			break;
#endif /* CONFIG_SPI_EMUL */
		default:
			rc = -EINVAL;
			LOG_WRN("Found no emulated bus enabled to register emulator %s",
				elp->dev->name);
		}

		if (rc != 0) {
			LOG_WRN("Failed to register emulator for %s: %d", elp->dev->name, rc);
		}
	}

	return 0;
}
