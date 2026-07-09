/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Role-neutral SDIO function I/O helpers. These are shared by every role and
 * transport backend: they translate the function I/O API into the CMD52/CMD53
 * primitives exposed by the bound transport vtable.
 */

#include <zephyr/kernel.h>
#include <zephyr/sdio/sdio_core.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sdio_core, CONFIG_SDIO_LOG_LEVEL);

#define SDIO_LOCK_TIMEOUT K_MSEC(CONFIG_SDIO_LOCK_TIMEOUT_MS)

static inline int sdio_rw_direct(struct sdio_dev *dev, enum sdio_io_dir dir,
				 enum sdio_func_num func, uint32_t reg,
				 uint8_t data_in, uint8_t *data_out)
{
	return dev->transport->rw_direct(dev, dir, func, reg, data_in, data_out);
}

/*
 * Split an extended transfer into the minimum number of block transfers,
 * then move any remaining data using byte mode.
 */
static int sdio_rw_extended_helper(struct sdio_function *func,
				   enum sdio_io_dir dir, uint32_t reg,
				   bool increment, uint8_t *buf, uint32_t len)
{
	struct sdio_dev *dev = func->dev;
	struct sdio_xfer xfer = {
		.func = func->num,
		.reg = reg,
		.increment = increment,
		.buf = buf,
	};
	int ret;
	int remaining = len;
	uint32_t size;
	uint16_t max_byte = func->cis.max_blk_size ? func->cis.max_blk_size
						   : dev->max_blk_size;

	if (func->num > SDIO_MAX_IO_NUMS) {
		return -EINVAL;
	}
	if (max_byte == 0U) {
		/* No CIS information: fall back to a single block worth */
		max_byte = func->block_size ? func->block_size : len;
	}

	if ((dev->caps & SDIO_CAP_MULTIBLOCK) && func->block_size &&
	    (len > func->block_size)) {
		while (remaining >= func->block_size) {
			xfer.blocks = remaining / func->block_size;
			xfer.block_size = func->block_size;
			size = xfer.blocks * func->block_size;
			ret = dev->transport->rw_extended(dev, dir, &xfer);
			if (ret) {
				return ret;
			}
			remaining -= size;
			xfer.buf += size;
			if (increment) {
				xfer.reg += size;
			}
		}
	}
	/* Remaining data goes out byte mode */
	while (remaining > 0) {
		size = MIN((uint32_t)remaining, (uint32_t)max_byte);
		xfer.blocks = 0;
		xfer.block_size = size;
		ret = dev->transport->rw_extended(dev, dir, &xfer);
		if (ret) {
			return ret;
		}
		remaining -= size;
		xfer.buf += size;
		if (increment) {
			xfer.reg += size;
		}
	}
	return 0;
}

int sdio_dev_init(struct sdio_dev *dev, enum sdio_role role,
		  const struct sdio_transport_api *transport, const void *ctx,
		  uint32_t caps)
{
	if (dev == NULL || transport == NULL ||
	    transport->rw_direct == NULL || transport->rw_extended == NULL) {
		return -EINVAL;
	}
	dev->role = role;
	dev->transport = transport;
	dev->transport_ctx = ctx;
	dev->caps = caps;
	dev->max_blk_size = 0;
	k_mutex_init(&dev->lock);
	return 0;
}

int sdio_func_bind(struct sdio_dev *dev, struct sdio_function *func,
		   enum sdio_func_num num)
{
	if (dev == NULL || func == NULL || num > SDIO_FUNC_MEMORY) {
		return -EINVAL;
	}
	func->dev = dev;
	func->num = num;
	func->block_size = 0;
	memset(&func->cis, 0, sizeof(func->cis));
	return 0;
}

int sdio_func_set_block_size(struct sdio_function *func, uint16_t bsize)
{
	struct sdio_dev *dev = func->dev;
	int ret;
	uint8_t reg;

	if (func->cis.max_blk_size && (func->cis.max_blk_size < bsize)) {
		return -EINVAL;
	}
	ret = k_mutex_lock(&dev->lock, SDIO_LOCK_TIMEOUT);
	if (ret) {
		return -EBUSY;
	}
	for (int i = 0; i < 2; i++) {
		reg = (bsize >> (i * 8));
		ret = sdio_rw_direct(dev, SDIO_IO_WRITE, SDIO_FUNC_NUM_0,
				     SDIO_FBR_BASE(func->num) +
					     SDIO_FBR_BLK_SIZE + i,
				     reg, NULL);
		if (ret) {
			goto unlock;
		}
	}
	func->block_size = bsize;
unlock:
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_func_read_reg(struct sdio_function *func, uint32_t reg, uint8_t *val)
{
	struct sdio_dev *dev = func->dev;
	int ret;

	ret = k_mutex_lock(&dev->lock, SDIO_LOCK_TIMEOUT);
	if (ret) {
		return -EBUSY;
	}
	ret = sdio_rw_direct(dev, SDIO_IO_READ, func->num, reg, 0, val);
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_func_write_reg(struct sdio_function *func, uint32_t reg, uint8_t val)
{
	struct sdio_dev *dev = func->dev;
	int ret;

	ret = k_mutex_lock(&dev->lock, SDIO_LOCK_TIMEOUT);
	if (ret) {
		return -EBUSY;
	}
	ret = sdio_rw_direct(dev, SDIO_IO_WRITE, func->num, reg, val, NULL);
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_func_rw_reg(struct sdio_function *func, uint32_t reg,
		     uint8_t write_val, uint8_t *read_val)
{
	struct sdio_dev *dev = func->dev;
	int ret;

	ret = k_mutex_lock(&dev->lock, SDIO_LOCK_TIMEOUT);
	if (ret) {
		return -EBUSY;
	}
	ret = sdio_rw_direct(dev, SDIO_IO_WRITE, func->num, reg, write_val,
			     read_val);
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_func_read_fifo(struct sdio_function *func, uint32_t reg, uint8_t *data,
			uint32_t len)
{
	struct sdio_dev *dev = func->dev;
	int ret;

	ret = k_mutex_lock(&dev->lock, SDIO_LOCK_TIMEOUT);
	if (ret) {
		return -EBUSY;
	}
	ret = sdio_rw_extended_helper(func, SDIO_IO_READ, reg, false, data, len);
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_func_write_fifo(struct sdio_function *func, uint32_t reg,
			 uint8_t *data, uint32_t len)
{
	struct sdio_dev *dev = func->dev;
	int ret;

	ret = k_mutex_lock(&dev->lock, SDIO_LOCK_TIMEOUT);
	if (ret) {
		return -EBUSY;
	}
	ret = sdio_rw_extended_helper(func, SDIO_IO_WRITE, reg, false, data,
				      len);
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_func_read_blocks_fifo(struct sdio_function *func, uint32_t reg,
			       uint8_t *data, uint32_t blocks)
{
	struct sdio_dev *dev = func->dev;
	struct sdio_xfer xfer = {
		.func = func->num,
		.reg = reg,
		.increment = false,
		.buf = data,
		.blocks = blocks,
		.block_size = func->block_size,
	};
	int ret;

	ret = k_mutex_lock(&dev->lock, SDIO_LOCK_TIMEOUT);
	if (ret) {
		return -EBUSY;
	}
	ret = dev->transport->rw_extended(dev, SDIO_IO_READ, &xfer);
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_func_write_blocks_fifo(struct sdio_function *func, uint32_t reg,
				uint8_t *data, uint32_t blocks)
{
	struct sdio_dev *dev = func->dev;
	struct sdio_xfer xfer = {
		.func = func->num,
		.reg = reg,
		.increment = false,
		.buf = data,
		.blocks = blocks,
		.block_size = func->block_size,
	};
	int ret;

	ret = k_mutex_lock(&dev->lock, SDIO_LOCK_TIMEOUT);
	if (ret) {
		return -EBUSY;
	}
	ret = dev->transport->rw_extended(dev, SDIO_IO_WRITE, &xfer);
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_func_read_addr(struct sdio_function *func, uint32_t reg, uint8_t *data,
			uint32_t len)
{
	struct sdio_dev *dev = func->dev;
	int ret;

	ret = k_mutex_lock(&dev->lock, SDIO_LOCK_TIMEOUT);
	if (ret) {
		return -EBUSY;
	}
	ret = sdio_rw_extended_helper(func, SDIO_IO_READ, reg, true, data, len);
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_func_write_addr(struct sdio_function *func, uint32_t reg,
			 uint8_t *data, uint32_t len)
{
	struct sdio_dev *dev = func->dev;
	int ret;

	ret = k_mutex_lock(&dev->lock, SDIO_LOCK_TIMEOUT);
	if (ret) {
		return -EBUSY;
	}
	ret = sdio_rw_extended_helper(func, SDIO_IO_WRITE, reg, true, data, len);
	k_mutex_unlock(&dev->lock);
	return ret;
}
