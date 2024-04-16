/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/i2s.h>


static inline int z_vrfy_i2s_configure(const struct device *dev,
				       enum i2s_dir dir,
				       const struct i2s_config *cfg_ptr)
{
	struct i2s_config config;
	int ret = -EINVAL;

	if (Z_SYSCALL_DRIVER_I2S(dev, configure)) {
		goto out;
	}

	Z_OOPS(z_user_from_copy(&config, (const void *)cfg_ptr,
				sizeof(struct i2s_config)));

	/* Check that the k_mem_slab provided is a valid pointer and that
	 * the caller has permission on it
	 */
	if (Z_SYSCALL_OBJ(config.mem_slab, K_OBJ_MEM_SLAB)) {
		goto out;
	}

	/* Ensure that the k_mem_slab's slabs are large enough for the
	 * specified block size
	 */
	if (config.block_size > config.mem_slab->info.block_size) {
		goto out;
	}

	ret = z_impl_i2s_configure((const struct device *)dev, dir, &config);
out:
	return ret;
}
#include <syscalls/i2s_configure_mrsh.c>

static inline int z_vrfy_i2s_buf_read(const struct device *dev,
				      void *buf, size_t *size)
{
	void *mem_block;
	size_t data_size;
	int ret;

	Z_OOPS(Z_SYSCALL_DRIVER_I2S(dev, read));

	ret = i2s_read((const struct device *)dev, &mem_block, &data_size);

	if (!ret) {
		const struct i2s_config *rx_cfg;
		int copy_success;

		/* Presumed to be configured otherwise the i2s_read() call
		 * would have failed.
		 */
		rx_cfg = i2s_config_get((const struct device *)dev, I2S_DIR_RX);

		copy_success = z_user_to_copy((void *)buf, mem_block,
					      data_size);

		k_mem_slab_free(rx_cfg->mem_slab, mem_block);
		Z_OOPS(copy_success);
		Z_OOPS(z_user_to_copy((void *)size, &data_size,
				      sizeof(data_size)));
	}

	return ret;
}
#include <syscalls/i2s_buf_read_mrsh.c>

static inline int z_vrfy_i2s_buf_write(const struct device *dev,
				       void *buf, size_t size)
{
	int ret;
	const struct i2s_config *tx_cfg;
	void *mem_block;

	Z_OOPS(Z_SYSCALL_DRIVER_I2S(dev, write));
	tx_cfg = i2s_config_get((const struct device *)dev, I2S_DIR_TX);
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

	ret = z_user_from_copy(mem_block, (void *)buf, size);
	if (ret) {
		k_mem_slab_free(tx_cfg->mem_slab, mem_block);
		Z_OOPS(ret);
	}

	ret = i2s_write((const struct device *)dev, mem_block, size);
	if (ret != 0) {
		k_mem_slab_free(tx_cfg->mem_slab, mem_block);
	}

	return ret;
}
#include <syscalls/i2s_buf_write_mrsh.c>

static inline int z_vrfy_i2s_trigger(const struct device *dev,
				     enum i2s_dir dir,
				     enum i2s_trigger_cmd cmd)
{
	Z_OOPS(Z_SYSCALL_DRIVER_I2S(dev, trigger));

	return z_impl_i2s_trigger((const struct device *)dev, dir, cmd);
}
#include <syscalls/i2s_trigger_mrsh.c>
