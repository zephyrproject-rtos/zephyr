/* frammb85rs64v.c - Driver for Fujitsu FRAMMB85RS64V temperature and pressure
 * sensor */

/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <drivers/fram.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>

#include "frammb85rs64v.h"

LOG_MODULE_REGISTER(FRAMMB85RS64V, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "FRAMMB85RS64V driver enabled without any devices"
#endif

struct frammb85rs64v_data {
	uint8_t chip_id[4];
};

struct frammb85rs64v_config {
	union frammb85rs64v_bus bus;
	const struct frammb85rs64v_bus_io *bus_io;
};

static inline int frammb85rs64v_bus_check(const struct device *dev)
{
	const struct frammb85rs64v_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static inline int frammb85rs64v_reg_read(const struct device *dev, uint16_t addr, uint8_t *data,
					 size_t len)
{
	const struct frammb85rs64v_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, addr, data, len);
}

static inline int frammb85rs64v_reg_write(const struct device *dev, uint16_t addr,
					  const uint8_t *data, size_t len)
{
	const struct frammb85rs64v_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, addr, data, len);
}

static inline int frammb85rs64v_read_id(const struct device *dev, uint8_t *data)
{
	const struct frammb85rs64v_config *cfg = dev->config;

	return cfg->bus_io->read_id(&cfg->bus, data);
}

static int frammb85rs64v_write_bytes(const struct device *dev, uint16_t addr, const uint8_t *data,
				     size_t len)
{
	return frammb85rs64v_reg_write(dev, addr, data, len);
}

static int frammb85rs64v_read_bytes(const struct device *dev, uint16_t addr, uint8_t *data,
				    size_t len)
{
	return frammb85rs64v_reg_read(dev, addr, data, len);
}

int frammb85rs64v_fram_read(const struct device *dev, uint16_t addr, uint8_t *data, size_t len)
{
	return frammb85rs64v_read_bytes(dev, addr, data, len);
}

int frammb85rs64v_fram_write(const struct device *dev, uint16_t addr, const uint8_t *data,
			     size_t len)
{
	return frammb85rs64v_write_bytes(dev, addr, data, len);
}

static const struct fram_driver_api frammb85rs64v_api_funcs = {
	.read = frammb85rs64v_fram_read,
	.write = frammb85rs64v_fram_write,
};

static int frammb85rs64v_chip_init(const struct device *dev)
{
	struct frammb85rs64v_data *data = dev->data;
	int err;
	uint8_t *id;
	err = frammb85rs64v_bus_check(dev);
	if (err < 0) {
		LOG_DBG("bus check failed: %d", err);
		return err;
	}
	err = frammb85rs64v_read_id(dev, data->chip_id);
	id = (data->chip_id);
	if (err) {
		LOG_DBG("Error during ID read\n");
		return -EIO;
	}
	if (id[0] != 0x04) {
		return -EIO;
	}

	if (id[1] != 0x7f) {
		return -EIO;
	}

	if (id[2] != 0x03) {
		return -EIO;
	}

	if (id[3] != 0x02) {
		return -EIO;
	}
	LOG_DBG("\n CHIP ID 0x%02X%02X%02X%02X\n", id[0], id[1], id[2], id[3]);
	LOG_DBG("\"%s\" OK", dev->name);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int frammb85rs64v_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Re-initialize the chip */
		LOG_DBG("In func %s", __func__);
		ret = frammb85rs64v_chip_init(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

/* Initializes a struct frammb85rs64v_config for an instance on a SPI bus. */
#define FRAMMB85RS64V_CONFIG_SPI(inst)                                                             \
	{                                                                                          \
		.bus.spi = SPI_DT_SPEC_INST_GET(inst, FRAMMB85RS64V_SPI_OPERATION, 0),             \
		.bus_io = &frammb85rs64v_bus_io_spi,                                               \
	}

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define FRAMMB85RS64V_DEFINE(inst)                                                                 \
	static struct frammb85rs64v_data frammb85rs64v_data_##inst;                                \
	static const struct frammb85rs64v_config frammb85rs64v_config_##inst =                     \
		FRAMMB85RS64V_CONFIG_SPI(inst);                                                    \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, frammb85rs64v_pm_action);                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, frammb85rs64v_chip_init, PM_DEVICE_DT_INST_GET(inst),          \
			      &frammb85rs64v_data_##inst, &frammb85rs64v_config_##inst,            \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &frammb85rs64v_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(FRAMMB85RS64V_DEFINE)
