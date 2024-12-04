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
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_emul_ctlr);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/drivers/spi_emul.h>

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
static struct spi_emul *spi_emul_find(const struct device *dev, unsigned int chipsel)
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

static int spi_emul_io(const struct device *dev, const struct spi_config *config,
		       const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	struct spi_emul *emul;
	const struct spi_emul_api *api;
	int ret;

	emul = spi_emul_find(dev, config->slave);
	if (!emul) {
		return -EIO;
	}

	api = emul->api;
	__ASSERT_NO_MSG(emul->api);
	__ASSERT_NO_MSG(emul->api->io);

	if (emul->mock_api != NULL && emul->mock_api->io != NULL) {
		ret = emul->mock_api->io(emul->target, config, tx_bufs, rx_bufs);
		if (ret != -ENOSYS) {
			return ret;
		}
	}

	return api->io(emul->target, config, tx_bufs, rx_bufs);
}

/**
 * @brief This is a no-op stub of the SPI API's `release` method to protect drivers under test
 *        from hitting a segmentation fault when using SPI_LOCK_ON plus spi_release()
 */
static int spi_emul_release(const struct device *dev, const struct spi_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);

	return 0;
}

/**
 * Set up a new emulator and add it to the list
 *
 * @param dev SPI emulation controller device
 */
static int spi_emul_init(const struct device *dev)
{
	struct spi_emul_data *data = dev->data;

	sys_slist_init(&data->emuls);

	return emul_init_for_bus(dev);
}

int spi_emul_register(const struct device *dev, struct spi_emul *emul)
{
	struct spi_emul_data *data = dev->data;
	const char *name = emul->target->dev->name;

	sys_slist_append(&data->emuls, &emul->node);

	LOG_INF("Register emulator '%s' at cs %u\n", name, emul->chipsel);

	return 0;
}

/* Device instantiation */

static DEVICE_API(spi, spi_emul_api) = {
	.transceive = spi_emul_io,
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_emul_release,
};

#define EMUL_LINK_AND_COMMA(node_id)                                                               \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
	},

#define SPI_EMUL_INIT(n)                                                                           \
	static const struct emul_link_for_bus emuls_##n[] = {                                      \
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), EMUL_LINK_AND_COMMA)};                \
	static struct emul_list_for_bus spi_emul_cfg_##n = {                                       \
		.children = emuls_##n,                                                             \
		.num_children = ARRAY_SIZE(emuls_##n),                                             \
	};                                                                                         \
	static struct spi_emul_data spi_emul_data_##n;                                             \
	DEVICE_DT_INST_DEFINE(n, spi_emul_init, NULL, &spi_emul_data_##n, &spi_emul_cfg_##n,       \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_emul_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_EMUL_INIT)
