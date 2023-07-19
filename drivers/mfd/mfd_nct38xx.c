/*
 * Copyright (c) 2023 Google, LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct38xx

#include <zephyr/drivers/i2c.h>

struct mfd_nct38xx_config {
	const struct i2c_dt_spec i2c_dev;
};

struct mfd_nct38xx_data {
	/* lock NC38xx register access */
	struct k_sem lock;
};

static int mfd_nct38xx_init(const struct device *dev)
{
	const struct mfd_nct38xx_config *config = dev->config;
	struct mfd_nct38xx_data *data = dev->data;

	if (!device_is_ready(config->i2c_dev.bus)) {
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

	return 0;
}

struct k_sem *mfd_nct38xx_get_lock_reference(const struct device *dev)
{
	struct mfd_nct38xx_data *data = dev->data;

	return &data->lock;
}

int nct38xx_reg_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *val)
{
	const struct mfd_nct38xx_config *const config = dev->config;

	return i2c_reg_read_byte_dt(&config->i2c_dev, reg_addr, val);
}

int nct38xx_reg_burst_read(const struct device *dev, uint8_t start_addr, uint8_t *buf,
			   uint32_t num_bytes)
{
	const struct mfd_nct38xx_config *const config = dev->config;

	return i2c_burst_read_dt(&config->i2c_dev, start_addr, buf, num_bytes);
}

int nct38xx_reg_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t val)
{
	const struct mfd_nct38xx_config *const config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c_dev, reg_addr, val);
}

int nct38xx_reg_burst_write(const struct device *dev, uint8_t start_addr, uint8_t *buf,
			    uint32_t num_bytes)
{
	const struct mfd_nct38xx_config *const config = dev->config;

	return i2c_burst_write_dt(&config->i2c_dev, start_addr, buf, num_bytes);
}

int nct38xx_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t reg_val, uint8_t new_val)
{
	if (reg_val == new_val) {
		return 0;
	}

	return nct38xx_reg_write_byte(dev, reg_addr, new_val);
}

#define MFD_NCT38XX_DEFINE(inst)                                                                   \
	static struct mfd_nct38xx_data nct38xx_data_##inst;                                        \
	static const struct mfd_nct38xx_config nct38xx_cfg_##inst = {                              \
		.i2c_dev = I2C_DT_SPEC_INST_GET(inst),                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_nct38xx_init, NULL, &nct38xx_data_##inst,                  \
			      &nct38xx_cfg_##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_NCT38XX_DEFINE)
