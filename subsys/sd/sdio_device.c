/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Device/slave-role SDIO subsystem. Demultiplexes host-initiated accesses
 * delivered by an SDIO device controller to the registered functions, backing
 * incrementing-address accesses with a buffer and forwarding fixed-address
 * (FIFO/data-port) accesses to a per-function handler.
 *
 * When a function-0 configuration is supplied, the subsystem also serves the
 * function-0 register file (CCCR/FBR/CIS) and tracks enable/interrupt state, so
 * vendors only provide configuration rather than implementing function 0.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sdio_dc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sd/sdio_device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sdio_device, CONFIG_SDIO_DEVICE_LOG_LEVEL);

/* Base address of the generated CIS tuple chains in function-0 space, with one
 * chain slot per function (function 0 at CIS_BASE, function n at n*STRIDE).
 */
#define SDIO_DEV_CIS_BASE   0x1000U
#define SDIO_DEV_CIS_STRIDE 0x100U
#define SDIO_DEV_CIS_END    (SDIO_DEV_CIS_BASE + (SDIO_MAX_IO_NUMS + 1) * SDIO_DEV_CIS_STRIDE)
/* Upper bound on a generated tuple chain. */
#define SDIO_DEV_CIS_MAX    64U

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

/* Build the CIS tuple chain for a function into buf; returns the length.
 * func == NULL builds the function-0 chain from the device configuration.
 */
static size_t sdio_dev_build_cis(struct sdio_device *dev,
				 struct sdio_device_function *func, uint8_t *buf)
{
	const struct sdio_device_config *cfg = dev->config;
	size_t i = 0;

	if (func == NULL) {
		/* MANFID */
		buf[i++] = SDIO_TPL_CODE_MANIFID;
		buf[i++] = 0x04;
		buf[i++] = cfg->manf_id & 0xFF;
		buf[i++] = (cfg->manf_id >> 8) & 0xFF;
		buf[i++] = cfg->manf_code & 0xFF;
		buf[i++] = (cfg->manf_code >> 8) & 0xFF;
		/* FUNCID */
		buf[i++] = SDIO_TPL_CODE_FUNCID;
		buf[i++] = 0x02;
		buf[i++] = cfg->func0_id;
		buf[i++] = 0x00;
		/* FUNCE (function 0) */
		buf[i++] = SDIO_TPL_CODE_FUNCE;
		buf[i++] = 0x04;
		buf[i++] = 0x00; /* extended data type: function 0 */
		buf[i++] = cfg->max_blk_size & 0xFF;
		buf[i++] = (cfg->max_blk_size >> 8) & 0xFF;
		buf[i++] = cfg->max_speed;
		buf[i++] = SDIO_TPL_CODE_END;
		return i;
	}

	/* FUNCID */
	buf[i++] = SDIO_TPL_CODE_FUNCID;
	buf[i++] = 0x02;
	buf[i++] = func->func_code;
	buf[i++] = 0x00;
	/* FUNCE (function n): a type-1 extension tuple; the standard layout
	 * places the max block size at body offset 12 and the ready timeout at
	 * body offset 28.
	 */
	buf[i++] = SDIO_TPL_CODE_FUNCE;
	buf[i++] = 0x2A;
	memset(&buf[i], 0, 0x2A);
	buf[i + 0] = 0x01; /* extended data type: function n */
	buf[i + 12] = func->max_blk_size & 0xFF;
	buf[i + 13] = (func->max_blk_size >> 8) & 0xFF;
	buf[i + 28] = func->rdy_timeout & 0xFF;
	buf[i + 29] = (func->rdy_timeout >> 8) & 0xFF;
	i += 0x2A;
	buf[i++] = SDIO_TPL_CODE_END;
	return i;
}

static uint8_t sdio_dev_cis_read(struct sdio_device *dev, uint32_t addr)
{
	uint8_t chain[SDIO_DEV_CIS_MAX];
	uint32_t idx = (addr - SDIO_DEV_CIS_BASE) / SDIO_DEV_CIS_STRIDE;
	uint32_t off = (addr - SDIO_DEV_CIS_BASE) % SDIO_DEV_CIS_STRIDE;
	struct sdio_device_function *func = NULL;
	size_t len;

	if (idx != 0) {
		func = sdio_device_find(dev, (enum sdio_func_num)idx);
		if (func == NULL) {
			return 0;
		}
	}
	len = sdio_dev_build_cis(dev, func, chain);
	return (off < len) ? chain[off] : 0;
}

static uint8_t sdio_dev_cccr_read(struct sdio_device *dev, uint32_t reg)
{
	const struct sdio_device_config *cfg = dev->config;

	switch (reg) {
	case SDIO_CCCR_CCCR:
		return cfg->cccr_revision & SDIO_CCCR_CCCR_REV_MASK;
	case SDIO_CCCR_SD:
		return cfg->sd_spec & SDIO_CCCR_SD_SPEC_MASK;
	case SDIO_CCCR_IO_EN:
		return dev->io_enable;
	case SDIO_CCCR_IO_RD:
		return dev->io_ready;
	case SDIO_CCCR_INT_EN:
		return dev->int_enable;
	case SDIO_CCCR_INT_P:
		return dev->int_pending;
	case SDIO_CCCR_BUS_IF:
		return dev->bus_width;
	case SDIO_CCCR_CAPS:
		return cfg->caps;
	case SDIO_CCCR_CIS:
		return SDIO_DEV_CIS_BASE & 0xFF;
	case SDIO_CCCR_CIS + 1:
		return (SDIO_DEV_CIS_BASE >> 8) & 0xFF;
	case SDIO_CCCR_CIS + 2:
		return (SDIO_DEV_CIS_BASE >> 16) & 0xFF;
	case SDIO_CCCR_SPEED:
		return dev->speed_sel;
	default:
		return 0;
	}
}

static void sdio_dev_cccr_write(struct sdio_device *dev, uint32_t reg,
				uint8_t val)
{
	switch (reg) {
	case SDIO_CCCR_IO_EN:
		/* Functions become ready as soon as they are enabled. */
		dev->io_enable = val;
		dev->io_ready = val;
		break;
	case SDIO_CCCR_INT_EN:
		dev->int_enable = val;
		break;
	case SDIO_CCCR_BUS_IF:
		dev->bus_width = val & SDIO_CCCR_BUS_IF_WIDTH_MASK;
		break;
	case SDIO_CCCR_SPEED:
		dev->speed_sel = val & SDIO_CCCR_SPEED_MASK;
		break;
	default:
		/* Read-only or unimplemented register: ignore. */
		break;
	}
}

static uint8_t sdio_dev_fbr_read(struct sdio_device *dev, uint32_t fbr_base,
				 uint32_t off)
{
	enum sdio_func_num num = (enum sdio_func_num)(fbr_base / 0x100U);
	struct sdio_device_function *func = sdio_device_find(dev, num);
	uint32_t cis_ptr = SDIO_DEV_CIS_BASE + num * SDIO_DEV_CIS_STRIDE;

	if (func == NULL) {
		return 0;
	}
	switch (off) {
	case 0x00: /* standard function interface code */
		return func->func_code & 0x0F;
	case SDIO_FBR_CIS:
		return cis_ptr & 0xFF;
	case SDIO_FBR_CIS + 1:
		return (cis_ptr >> 8) & 0xFF;
	case SDIO_FBR_CIS + 2:
		return (cis_ptr >> 16) & 0xFF;
	case SDIO_FBR_BLK_SIZE:
		return func->block_size & 0xFF;
	case SDIO_FBR_BLK_SIZE + 1:
		return (func->block_size >> 8) & 0xFF;
	default:
		return 0;
	}
}

static void sdio_dev_fbr_write(struct sdio_device *dev, uint32_t fbr_base,
			       uint32_t off, uint8_t val)
{
	enum sdio_func_num num = (enum sdio_func_num)(fbr_base / 0x100U);
	struct sdio_device_function *func = sdio_device_find(dev, num);

	if (func == NULL) {
		return;
	}
	switch (off) {
	case SDIO_FBR_BLK_SIZE:
		func->block_size = (func->block_size & 0xFF00) | val;
		break;
	case SDIO_FBR_BLK_SIZE + 1:
		func->block_size = (func->block_size & 0x00FF) |
				   ((uint16_t)val << 8);
		break;
	default:
		break;
	}
}

static uint8_t sdio_dev_reg_read(struct sdio_device *dev, uint32_t addr)
{
	if (addr < 0x100U) {
		return sdio_dev_cccr_read(dev, addr);
	}
	if (addr < 0x800U) {
		return sdio_dev_fbr_read(dev, addr & ~0xFFU, addr & 0xFFU);
	}
	if (addr >= SDIO_DEV_CIS_BASE && addr < SDIO_DEV_CIS_END) {
		return sdio_dev_cis_read(dev, addr);
	}
	return 0;
}

static void sdio_dev_reg_write(struct sdio_device *dev, uint32_t addr,
			       uint8_t val)
{
	if (addr < 0x100U) {
		sdio_dev_cccr_write(dev, addr, val);
	} else if (addr < 0x800U) {
		sdio_dev_fbr_write(dev, addr & ~0xFFU, addr & 0xFFU, val);
	}
	/* CIS is read-only. */
}

/* Serve a host access to function 0 (CCCR/FBR/CIS) from subsystem state. */
static int sdio_device_func0(struct sdio_device *dev, struct sdio_dc_xfer *xfer)
{
	for (uint32_t i = 0; i < xfer->len; i++) {
		uint32_t addr = xfer->reg + i;

		if (xfer->dir == SDIO_DC_DIR_WRITE) {
			sdio_dev_reg_write(dev, addr, xfer->data[i]);
		} else {
			xfer->data[i] = sdio_dev_reg_read(dev, addr);
		}
	}
	return 0;
}

/* Controller callback: a host access arrived. Route it to a function. */
static int sdio_device_xfer(const struct device *controller,
			    struct sdio_dc_xfer *xfer, void *user)
{
	struct sdio_device *dev = user;
	struct sdio_device_function *func;
	enum sdio_io_dir dir;

	ARG_UNUSED(controller);

	/* Function 0 (CCCR/FBR/CIS) is served by the subsystem when configured. */
	if (xfer->func == SDIO_FUNC_NUM_0 && dev->config != NULL) {
		return sdio_device_func0(dev, xfer);
	}

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

int sdio_device_init(struct sdio_device *dev, const struct device *controller,
		     const struct sdio_device_config *config)
{
	if (dev == NULL || controller == NULL) {
		return -EINVAL;
	}
	if (!device_is_ready(controller)) {
		return -ENODEV;
	}
	dev->controller = controller;
	dev->config = config;
	dev->io_enable = 0;
	dev->io_ready = 0;
	dev->int_enable = 0;
	dev->int_pending = 0;
	dev->bus_width = SDIO_CCCR_BUS_IF_WIDTH_1_BIT;
	dev->speed_sel = 0;
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
	func->block_size = 0;
	sys_slist_append(&dev->functions, &func->node);
unlock:
	k_mutex_unlock(&dev->lock);
	return ret;
}

/* Controller callback: a posted/submitted zero-copy buffer completed. */
static void sdio_device_completion(const struct device *controller,
				   enum sdio_func_num num, enum sdio_dc_evt evt,
				   uint8_t *buf, uint32_t len, void *user)
{
	struct sdio_device *dev = user;
	struct sdio_device_function *func;

	ARG_UNUSED(controller);

	func = sdio_device_find(dev, num);
	if (func == NULL) {
		return;
	}
	if (evt == SDIO_DC_RX_DONE && func->rx_done != NULL) {
		func->rx_done(func, buf, len);
	} else if (evt == SDIO_DC_TX_DONE && func->tx_done != NULL) {
		func->tx_done(func, buf);
	}
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
	if (sdio_device_is_zero_copy(dev)) {
		(void)sdio_dc_set_completion_cb(dev->controller,
						sdio_device_completion, dev);
	}
	return sdio_dc_enable(dev->controller);
}

bool sdio_device_is_zero_copy(struct sdio_device *dev)
{
	struct sdio_dc_caps caps;

	if (dev == NULL || sdio_dc_get_caps(dev->controller, &caps) != 0) {
		return false;
	}
	return caps.zero_copy;
}

int sdio_device_rx_post(struct sdio_device_function *func, uint8_t *buf,
			uint32_t cap)
{
	if (func == NULL || func->parent == NULL) {
		return -EINVAL;
	}
	return sdio_dc_rx_post(func->parent->controller, func->num, buf, cap);
}

int sdio_device_tx_submit(struct sdio_device_function *func, uint8_t *buf,
			  uint32_t len)
{
	if (func == NULL || func->parent == NULL) {
		return -EINVAL;
	}
	return sdio_dc_tx_submit(func->parent->controller, func->num, buf, len);
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
	struct sdio_device *dev;

	if (func == NULL || func->parent == NULL) {
		return -EINVAL;
	}
	dev = func->parent;
	dev->int_pending |= BIT(func->num);
	return sdio_dc_raise_interrupt(dev->controller, func->num);
}

int sdio_device_clear_interrupt(struct sdio_device_function *func)
{
	if (func == NULL || func->parent == NULL) {
		return -EINVAL;
	}
	func->parent->int_pending &= ~BIT(func->num);
	return 0;
}
