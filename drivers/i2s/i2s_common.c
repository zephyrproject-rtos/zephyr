/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include <i2s.h>

int _impl_i2s_buf_read(struct device *dev, void *buf, size_t *size)
{
	void *mem_block;
	int ret;

	ret = i2s_read((struct device *)dev, &mem_block, size);

	if (!ret) {
		struct i2s_config *rx_cfg =
			i2s_config_get((struct device *)dev, I2S_DIR_RX);

		memcpy(buf, mem_block, *size);
		k_mem_slab_free(rx_cfg->mem_slab, mem_block);
	}

	return ret;
}

int _impl_i2s_buf_write(struct device *dev, void *buf, size_t size)
{
	int ret;
	struct i2s_config *tx_cfg;
	void *mem_block;

	tx_cfg = i2s_config_get((struct device *)dev, I2S_DIR_TX);
	if (!tx_cfg) {
		return -EIO;
	}

	if (size > tx_cfg->block_size) {
		return -EINVAL;
	}

	ret = k_mem_slab_alloc(tx_cfg->mem_slab, &mem_block, K_FOREVER);
	if (ret < 0) {
		return -ENOMEM;
	}

	memcpy(mem_block, (void *)buf, size);

	return i2s_write((struct device *)dev, mem_block, size);
}
