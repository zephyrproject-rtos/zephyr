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
#include <zephyr/sys/util.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/drivers/sdio_dc.h>
#include <zephyr/drivers/sdio_dc_virtual.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sdio_dc_virtual, CONFIG_SDIO_DC_LOG_LEVEL);

#define SDIO_DC_VIRTUAL_NUM_Q (SDIO_MAX_IO_NUMS + 1)

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
	sdio_dc_completion_cb_t comp_cb;
	void *comp_user;
	/* Depth-1 zero-copy queues per function (loopback model). */
	uint8_t *rx_buf[SDIO_DC_VIRTUAL_NUM_Q];
	uint32_t rx_cap[SDIO_DC_VIRTUAL_NUM_Q];
	uint8_t *tx_buf[SDIO_DC_VIRTUAL_NUM_Q];
	uint32_t tx_len[SDIO_DC_VIRTUAL_NUM_Q];
	/* Last buffers handed through the zero-copy path (test inspection). */
	uint8_t *last_rx;
	uint8_t *last_tx;
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
	caps->zero_copy = true;
	return 0;
}

static int sdio_dc_virtual_set_completion_cb(const struct device *dev,
					     sdio_dc_completion_cb_t cb,
					     void *user)
{
	struct sdio_dc_virtual_data *data = dev->data;

	data->comp_cb = cb;
	data->comp_user = user;
	return 0;
}

static int sdio_dc_virtual_rx_post(const struct device *dev,
				   enum sdio_func_num func, uint8_t *buf,
				   uint32_t cap)
{
	struct sdio_dc_virtual_data *data = dev->data;

	if (func >= SDIO_DC_VIRTUAL_NUM_Q) {
		return -EINVAL;
	}
	if (data->rx_buf[func] != NULL) {
		return -EBUSY;
	}
	data->rx_buf[func] = buf;
	data->rx_cap[func] = cap;
	return 0;
}

static int sdio_dc_virtual_tx_submit(const struct device *dev,
				     enum sdio_func_num func, uint8_t *buf,
				     uint32_t len)
{
	struct sdio_dc_virtual_data *data = dev->data;

	if (func >= SDIO_DC_VIRTUAL_NUM_Q) {
		return -EINVAL;
	}
	if (data->tx_buf[func] != NULL) {
		return -EBUSY;
	}
	data->tx_buf[func] = buf;
	data->tx_len[func] = len;
	return 0;
}

static DEVICE_API(sdio_dc, sdio_dc_virtual_api) = {
	.enable = sdio_dc_virtual_enable,
	.disable = sdio_dc_virtual_disable,
	.set_xfer_callback = sdio_dc_virtual_set_xfer_callback,
	.raise_interrupt = sdio_dc_virtual_raise_interrupt,
	.get_caps = sdio_dc_virtual_get_caps,
	.set_completion_cb = sdio_dc_virtual_set_completion_cb,
	.rx_post = sdio_dc_virtual_rx_post,
	.tx_submit = sdio_dc_virtual_tx_submit,
};

/* --- Injected access hook ------------------------------------------------- */

/* Serve a fixed-address (data-port) access from the zero-copy queues if a
 * buffer is available; returns true if it was handled that way. The data copy
 * here models the controller's DMA between bus and device memory; the
 * subsystem/application never copies.
 */
static bool sdio_dc_virtual_zc(struct sdio_dc_virtual_data *data,
			       const struct device *dc, struct sdio_dc_xfer *xfer)
{
	enum sdio_func_num f = xfer->func;

	if (data->comp_cb == NULL || f >= SDIO_DC_VIRTUAL_NUM_Q) {
		return false;
	}

	if (xfer->dir == SDIO_DC_DIR_WRITE && data->rx_buf[f] != NULL) {
		uint8_t *buf = data->rx_buf[f];
		uint32_t n = MIN(xfer->len, data->rx_cap[f]);

		memcpy(buf, xfer->data, n);
		data->rx_buf[f] = NULL;
		data->last_rx = buf;
		data->comp_cb(dc, f, SDIO_DC_RX_DONE, buf, n, data->comp_user);
		return true;
	}

	if (xfer->dir == SDIO_DC_DIR_READ && data->tx_buf[f] != NULL) {
		uint8_t *buf = data->tx_buf[f];
		uint32_t n = MIN(xfer->len, data->tx_len[f]);

		memcpy(xfer->data, buf, n);
		if (n < xfer->len) {
			memset(xfer->data + n, 0, xfer->len - n);
		}
		data->tx_buf[f] = NULL;
		data->last_tx = buf;
		data->comp_cb(dc, f, SDIO_DC_TX_DONE, buf, n, data->comp_user);
		return true;
	}

	return false;
}

int sdio_dc_virtual_access(const struct device *dc, struct sdio_dc_xfer *xfer)
{
	struct sdio_dc_virtual_data *data = dc->data;

	if (!data->enabled) {
		return -EIO;
	}
	/* Fixed-address data-port accesses may be served zero-copy. */
	if (!xfer->increment && sdio_dc_virtual_zc(data, dc, xfer)) {
		return 0;
	}
	if (data->xfer_cb == NULL) {
		return -EIO;
	}
	return data->xfer_cb(dc, xfer, data->xfer_user);
}

uint8_t *sdio_dc_virtual_last_rx(const struct device *dc)
{
	struct sdio_dc_virtual_data *data = dc->data;

	return data->last_rx;
}

uint8_t *sdio_dc_virtual_last_tx(const struct device *dc)
{
	struct sdio_dc_virtual_data *data = dc->data;

	return data->last_tx;
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
