/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This driver creates fake SPI buses which can contain emulated devices,
 * implemented by a separate emulation driver. The API between this driver and
 * its emulators is defined by struct spi_emul_driver_api.
 */

#define DT_DRV_COMPAT zephyr_spi_emul_controller

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_emul_ctlr);

#include <device.h>
#include <emul.h>
#include <drivers/spi.h>
#include <drivers/spi_emul.h>

/** Working data for the device */
struct spi_emul_data {
	/* List of struct spi_emul associated with the device */
	sys_slist_t emuls;
	/* SPI host configuration */
	uint32_t config;
};

uint32_t spi_emul_get_config(const struct device *dev)
{
	struct spi_emul_data *data = dev->data;

	return data->config;
}

/**
 * Find an emulator for a SPI bus
 *
 * At present only a single emulator is supported on the bus, since we do not
 * support chip selects, despite there being a chipsel field. It cannot be
 * implemented until we have a GPIO emulator.
 *
 * @param dev SPI emulation controller device
 * @param chipsel Chip-select value
 * @return emulator to use
 * @return NULL if not found
 */
static struct spi_emul *spi_emul_find(const struct device *dev,
				      unsigned int chipsel)
{
	struct spi_emul_data *data = dev->data;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&data->emuls, node) {
		struct spi_emul *emul;

		emul = CONTAINER_OF(node, struct spi_emul, node);
		if (emul->chipsel == chipsel) {
			return emul;
		}
	}

	return NULL;
}

static int spi_emul_io(const struct device *dev,
		       const struct spi_config *config,
		       const struct spi_buf_set *tx_bufs,
		       const struct spi_buf_set *rx_bufs)
{
	struct spi_emul *emul;
	const struct spi_emul_api *api;

	emul = spi_emul_find(dev, config->slave);
	if (!emul) {
		return -EIO;
	}

	api = emul->api;
	__ASSERT_NO_MSG(emul->api);
	__ASSERT_NO_MSG(emul->api->io);

	return api->io(emul, config, tx_bufs, rx_bufs);
}

/**
 * Set up a new emulator and add it to the list
 *
 * @param dev SPI emulation controller device
 */
static int spi_emul_init(const struct device *dev)
{
	struct spi_emul_data *data = dev->data;
	const struct emul_list_for_bus *list = dev->config;

	sys_slist_init(&data->emuls);

	return emul_init_for_bus_from_list(dev, list);
}

int spi_emul_register(const struct device *dev, const char *name,
		      struct spi_emul *emul)
{
	struct spi_emul_data *data = dev->data;

	sys_slist_append(&data->emuls, &emul->node);

	LOG_INF("Register emulator '%s' at cs %u\n", name, emul->chipsel);

	return 0;
}

/* Device instantiation */

static struct spi_driver_api spi_emul_api = {
	.transceive = spi_emul_io,
};

#define EMUL_LINK_AND_COMMA(node_id) {		\
	.label = DT_LABEL(node_id),		\
},

#define SPI_EMUL_INIT(n) \
	static const struct emul_link_for_bus emuls_##n[] = { \
		DT_FOREACH_CHILD(DT_DRV_INST(0), EMUL_LINK_AND_COMMA) \
	}; \
	static struct emul_list_for_bus spi_emul_cfg_##n = { \
		.children = emuls_##n, \
		.num_children = ARRAY_SIZE(emuls_##n), \
	}; \
	static struct spi_emul_data spi_emul_data_##n; \
	DEVICE_DT_INST_DEFINE(n, \
			    spi_emul_init, \
			    device_pm_control_nop, \
			    &spi_emul_data_##n, \
			    &spi_emul_cfg_##n, \
			    POST_KERNEL, \
			    CONFIG_SPI_INIT_PRIORITY, \
			    &spi_emul_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_EMUL_INIT)
