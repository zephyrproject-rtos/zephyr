/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Virtual (loopback) SDIO device controller. Implements the SDIO device
 * controller class in software and provides an in-process loopback transport
 * so that a host-role sdio_dev can exercise the device side without hardware.
 */

#define DT_DRV_COMPAT zephyr_sdio_device_virtual

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sdio_dc.h>
#include <zephyr/drivers/sdio_dc_virtual.h>
#include <zephyr/sdio/sdio_core.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sdio_dc_virtual, CONFIG_SDIO_LOG_LEVEL);

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

/* --- Loopback host transport --------------------------------------------- */

static int sdio_dc_virtual_dispatch(const struct device *dc,
				    struct sdio_dc_xfer *xfer)
{
	struct sdio_dc_virtual_data *data = dc->data;

	if (!data->enabled || data->xfer_cb == NULL) {
		return -EIO;
	}
	return data->xfer_cb(dc, xfer, data->xfer_user);
}

static int loopback_rw_direct(struct sdio_dev *dev, enum sdio_io_dir dir,
			      enum sdio_func_num func, uint32_t reg,
			      uint8_t data_in, uint8_t *data_out)
{
	const struct device *dc = dev->transport_ctx;
	struct sdio_dc_xfer xfer = {
		.func = func,
		.reg = reg,
		.increment = true,
		.len = 1,
	};
	uint8_t byte = data_in;
	int ret;

	if (dir == SDIO_IO_WRITE) {
		xfer.dir = SDIO_DC_DIR_WRITE;
		xfer.data = &byte;
		ret = sdio_dc_virtual_dispatch(dc, &xfer);
		if (ret) {
			return ret;
		}
		if (data_out == NULL) {
			return 0;
		}
		/* RAW: read the register back */
	}

	xfer.dir = SDIO_DC_DIR_READ;
	xfer.data = &byte;
	ret = sdio_dc_virtual_dispatch(dc, &xfer);
	if (ret) {
		return ret;
	}
	if (data_out) {
		*data_out = byte;
	}
	return 0;
}

static int loopback_rw_extended(struct sdio_dev *dev, enum sdio_io_dir dir,
				const struct sdio_xfer *xfer)
{
	const struct device *dc = dev->transport_ctx;
	struct sdio_dc_xfer dcx = {
		.func = xfer->func,
		.dir = (dir == SDIO_IO_WRITE) ? SDIO_DC_DIR_WRITE
					      : SDIO_DC_DIR_READ,
		.reg = xfer->reg,
		.increment = xfer->increment,
		.data = xfer->buf,
		.len = xfer->blocks ? (xfer->blocks * xfer->block_size)
				    : xfer->block_size,
	};

	return sdio_dc_virtual_dispatch(dc, &dcx);
}

static const struct sdio_transport_api sdio_dc_virtual_loopback = {
	.rw_direct = loopback_rw_direct,
	.rw_extended = loopback_rw_extended,
};

const struct sdio_transport_api *sdio_dc_virtual_loopback_api(void)
{
	return &sdio_dc_virtual_loopback;
}

const void *sdio_dc_virtual_loopback_ctx(const struct device *dc)
{
	return dc;
}

void sdio_dc_virtual_set_irq_cb(const struct device *dc,
				sdio_dc_virtual_irq_cb_t cb, void *user)
{
	struct sdio_dc_virtual_data *data = dc->data;

	data->irq_cb = cb;
	data->irq_user = user;
}

/* --- Instantiation -------------------------------------------------------- */

static int sdio_dc_virtual_init(const struct device *dev)
{
	struct sdio_dc_virtual_data *data = dev->data;

	k_mutex_init(&data->lock);
	data->enabled = false;
	return 0;
}

#define SDIO_DC_VIRTUAL_INIT(inst)							\
	static const struct sdio_dc_virtual_config sdio_dc_virtual_cfg_##inst = {	\
		.num_funcs = DT_INST_PROP_OR(inst, num_functions, 1),			\
		.max_blk_size = DT_INST_PROP_OR(inst, max_block_size, 512),		\
	};										\
	static struct sdio_dc_virtual_data sdio_dc_virtual_data_##inst;			\
	DEVICE_DT_INST_DEFINE(inst, sdio_dc_virtual_init, NULL,				\
			      &sdio_dc_virtual_data_##inst,				\
			      &sdio_dc_virtual_cfg_##inst, POST_KERNEL,			\
			      CONFIG_SDIO_DEVICE_INIT_PRIORITY,				\
			      &sdio_dc_virtual_api);

DT_INST_FOREACH_STATUS_OKAY(SDIO_DC_VIRTUAL_INIT)
