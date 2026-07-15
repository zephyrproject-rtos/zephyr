/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Device/slave-role SDIO subsystem. Demultiplexes host-initiated accesses
 * delivered by an SDIO device controller to the registered functions, backing
 * incrementing-address accesses with a buffer and forwarding fixed-address
 * (FIFO/data-port) accesses to a per-function handler.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sdio_dc.h>
#include <zephyr/sdio/sdio_device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(sdio_core, CONFIG_SDIO_LOG_LEVEL);

static struct sdio_device_function *sdio_device_find(struct sdio_device *dev,
						     enum sdio_func_num num)
{
	struct sdio_device_function *func;

	SYS_SLIST_FOR_EACH_CONTAINER(&dev->functions, func, node) {
		if (func->num == num) {
			return func;
		}
	}
	return NULL;
}

/* Controller callback: a host access arrived. Route it to a function. */
static int sdio_device_xfer(const struct device *controller,
			    struct sdio_dc_xfer *xfer, void *user)
{
	struct sdio_device *dev = user;
	struct sdio_device_function *func;
	enum sdio_io_dir dir;

	ARG_UNUSED(controller);

	func = sdio_device_find(dev, xfer->func);
	if (func == NULL) {
		LOG_DBG("access to unregistered function %d", xfer->func);
		return -ENODEV;
	}

	dir = (xfer->dir == SDIO_DC_DIR_WRITE) ? SDIO_IO_WRITE : SDIO_IO_READ;

	if (!xfer->increment) {
		/* Fixed-address: FIFO / data port */
		if (func->fifo_cb == NULL || xfer->reg != func->fifo_reg) {
			LOG_DBG("no FIFO handler for func %d reg 0x%x",
				xfer->func, xfer->reg);
			return -EIO;
		}
		return func->fifo_cb(func, dir, xfer->data, xfer->len,
				     func->user);
	}

	/* Incrementing-address: register window */
	if (func->regs == NULL ||
	    (xfer->reg + xfer->len) > func->regs_size) {
		LOG_DBG("register window OOB func %d reg 0x%x len %u",
			xfer->func, xfer->reg, xfer->len);
		return -EIO;
	}
	if (dir == SDIO_IO_WRITE) {
		memcpy(&func->regs[xfer->reg], xfer->data, xfer->len);
	} else {
		memcpy(xfer->data, &func->regs[xfer->reg], xfer->len);
	}
	return 0;
}

int sdio_device_init(struct sdio_device *dev, const struct device *controller)
{
	if (dev == NULL || controller == NULL) {
		return -EINVAL;
	}
	if (!device_is_ready(controller)) {
		return -ENODEV;
	}
	dev->controller = controller;
	sys_slist_init(&dev->functions);
	k_mutex_init(&dev->lock);
	return 0;
}

int sdio_device_register_function(struct sdio_device *dev,
				  struct sdio_device_function *func)
{
	int ret = 0;

	if (dev == NULL || func == NULL || func->num > SDIO_MAX_IO_NUMS) {
		return -EINVAL;
	}
	if (func->regs == NULL && func->fifo_cb == NULL) {
		/* A function must expose at least one access type */
		return -EINVAL;
	}

	k_mutex_lock(&dev->lock, K_FOREVER);
	if (sdio_device_find(dev, func->num) != NULL) {
		ret = -EALREADY;
		goto unlock;
	}
	func->parent = dev;
	sys_slist_append(&dev->functions, &func->node);
unlock:
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_device_enable(struct sdio_device *dev)
{
	int ret;

	if (dev == NULL) {
		return -EINVAL;
	}
	ret = sdio_dc_set_xfer_callback(dev->controller, sdio_device_xfer, dev);
	if (ret) {
		return ret;
	}
	return sdio_dc_enable(dev->controller);
}

int sdio_device_disable(struct sdio_device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}
	return sdio_dc_disable(dev->controller);
}

int sdio_device_raise_interrupt(struct sdio_device_function *func)
{
	if (func == NULL || func->parent == NULL) {
		return -EINVAL;
	}
	return sdio_dc_raise_interrupt(func->parent->controller, func->num);
}
