/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Virtual (software) SDIO device controller. Implements the SDIO device
 * controller class and exposes an access hook used to inject decoded host
 * accesses to the registered device functions. Intended for tests and samples.
 */

#define DT_DRV_COMPAT zephyr_sdio_device_virtual

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sdio_dc.h>
#include <zephyr/drivers/sdio_dc_virtual.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sdio_dc_virtual, CONFIG_SDIO_DC_LOG_LEVEL);

struct sdio_dc_virtual_config {
	uint8_t num_funcs;
	uint16_t max_blk_size;
};

struct sdio_dc_virtual_data {
	struct k_mutex lock;
	bool enabled;
	sdio_dc_xfer_cb_t xfer_cb;
	void *xfer_user;
	sdio_dc_virtual_irq_cb_t irq_cb;
	void *irq_user;
};

/* --- SDIO device controller API ------------------------------------------ */

static int sdio_dc_virtual_enable(const struct device *dev)
{
	struct sdio_dc_virtual_data *data = dev->data;

	data->enabled = true;
	return 0;
}

static int sdio_dc_virtual_disable(const struct device *dev)
{
	struct sdio_dc_virtual_data *data = dev->data;

	data->enabled = false;
	return 0;
}

static int sdio_dc_virtual_set_xfer_callback(const struct device *dev,
					     sdio_dc_xfer_cb_t cb, void *user)
{
	struct sdio_dc_virtual_data *data = dev->data;

	data->xfer_cb = cb;
	data->xfer_user = user;
	return 0;
}

static int sdio_dc_virtual_raise_interrupt(const struct device *dev,
					   enum sdio_func_num func)
{
	struct sdio_dc_virtual_data *data = dev->data;

	if (data->irq_cb == NULL) {
		return -ENOSYS;
	}
	data->irq_cb(dev, func, data->irq_user);
	return 0;
}

static int sdio_dc_virtual_get_caps(const struct device *dev,
				    struct sdio_dc_caps *caps)
{
	const struct sdio_dc_virtual_config *cfg = dev->config;

	caps->num_funcs = cfg->num_funcs;
	caps->max_blk_size = cfg->max_blk_size;
	caps->interrupt_supported = true;
	return 0;
}

static DEVICE_API(sdio_dc, sdio_dc_virtual_api) = {
	.enable = sdio_dc_virtual_enable,
	.disable = sdio_dc_virtual_disable,
	.set_xfer_callback = sdio_dc_virtual_set_xfer_callback,
	.raise_interrupt = sdio_dc_virtual_raise_interrupt,
	.get_caps = sdio_dc_virtual_get_caps,
};

/* --- Injected access hook ------------------------------------------------- */

int sdio_dc_virtual_access(const struct device *dc, struct sdio_dc_xfer *xfer)
{
	struct sdio_dc_virtual_data *data = dc->data;

	if (!data->enabled || data->xfer_cb == NULL) {
		return -EIO;
	}
	return data->xfer_cb(dc, xfer, data->xfer_user);
}

void sdio_dc_virtual_set_irq_cb(const struct device *dc,
				sdio_dc_virtual_irq_cb_t cb, void *user)
{
	struct sdio_dc_virtual_data *data = dc->data;

	data->irq_cb = cb;
	data->irq_user = user;
}

static int sdio_dc_virtual_init(const struct device *dev)
{
	struct sdio_dc_virtual_data *data = dev->data;

	k_mutex_init(&data->lock);
	data->enabled = false;
	return 0;
}

#define SDIO_DC_VIRTUAL_INIT(inst)						\
	static const struct sdio_dc_virtual_config sdio_dc_virtual_cfg_##inst = { \
		.num_funcs = DT_INST_PROP_OR(inst, num_functions, 1),		\
		.max_blk_size = DT_INST_PROP_OR(inst, max_block_size, 512),	\
	};									\
	static struct sdio_dc_virtual_data sdio_dc_virtual_data_##inst;		\
	DEVICE_DT_INST_DEFINE(inst, sdio_dc_virtual_init, NULL,			\
			      &sdio_dc_virtual_data_##inst,			\
			      &sdio_dc_virtual_cfg_##inst, POST_KERNEL,		\
			      CONFIG_SDIO_DEVICE_INIT_PRIORITY,			\
			      &sdio_dc_virtual_api);

DT_INST_FOREACH_STATUS_OKAY(SDIO_DC_VIRTUAL_INIT)
