/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/fpga.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fpga_slg471x5);

#define SLG471X5_NREG 256

#define SLG471X5_I2C_RST_REG 0xF5U
#define SLG471X5_I2C_RST_BIT BIT(0)

#define SLG471X5_ADDR_UNCONFIGURED 0x00U

/*
 * mem_region_t - Memory Region
 *
 * @addr starting address of memory region
 * @len size of memory region
 */
typedef union {
	uint16_t mem_region;
	struct {
		uint8_t addr;
		uint8_t len;
	} __packed;
} mem_region_t;

struct fpga_slg471x5_data {
	bool loaded;
	struct k_spinlock lock;
};

struct fpga_slg471x5_config {
	struct i2c_dt_spec bus;
	mem_region_t *verify_list;
	int verify_list_len;
	bool try_unconfigured;
};

static enum FPGA_status fpga_slg471x5_get_status(const struct device *dev)
{
	enum FPGA_status status;
	k_spinlock_key_t key;
	struct fpga_slg471x5_data *data = dev->data;

	key = k_spin_lock(&data->lock);

	if (data->loaded) {
		status = FPGA_STATUS_ACTIVE;
	} else {
		status = FPGA_STATUS_INACTIVE;
	}

	k_spin_unlock(&data->lock, key);

	return status;
}

static int fpga_slg471x5_verify(const struct device *dev, uint8_t *img, uint32_t img_size)
{
	const struct fpga_slg471x5_config *config = dev->config;
	uint8_t buf[SLG471X5_NREG] = {0}, addr, len;
	int i;

	i2c_read_dt(&config->bus, buf, SLG471X5_NREG);

	for (i = 0; i < config->verify_list_len; i++) {
		addr = config->verify_list[i].addr;
		len = config->verify_list[i].len;
		if (memcmp(&img[addr], &buf[addr], len) != 0) {
			return -EIO;
		}
	}

	return 0;
}

static int fpga_slg471x5_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
	const struct fpga_slg471x5_config *config = dev->config;
	struct fpga_slg471x5_data *data = dev->data;
	uint8_t buf[SLG471X5_NREG + 1];
	int ret;

	if (img_size > SLG471X5_NREG) {
		img_size = SLG471X5_NREG;
	}

	buf[0] = 0;
	memcpy(buf + 1, image_ptr, img_size);

	if (config->try_unconfigured) {
		ret = i2c_write(config->bus.bus, buf, img_size + 1, SLG471X5_ADDR_UNCONFIGURED);
		if (ret == 0) {
			ret = fpga_slg471x5_verify(dev, buf + 1, img_size);
			if (ret == 0) {
				data->loaded = true;
				return 0;
			}
		}
	}

	ret = i2c_write_dt(&config->bus, buf, img_size + 1);
	if (ret < 0) {
		LOG_ERR("Loading bitstream failed");
		return ret;
	}

	ret = fpga_slg471x5_verify(dev, buf + 1, img_size);
	if (ret < 0) {
		LOG_ERR("Verification failed");
		return ret;
	}

	data->loaded = true;

	return 0;
}

static int fpga_slg471x5_reset(const struct device *dev)
{
	const struct fpga_slg471x5_config *config = dev->config;
	struct fpga_slg471x5_data *data = dev->data;
	int ret;

	ret = i2c_reg_update_byte_dt(&config->bus, SLG471X5_I2C_RST_REG, SLG471X5_I2C_RST_BIT,
				     SLG471X5_I2C_RST_BIT);
	if (ret < 0) {
		return ret;
	}

	data->loaded = false;

	return 0;
}

static const struct fpga_driver_api fpga_slg471x5_api = {
	.get_status = fpga_slg471x5_get_status,
	.reset = fpga_slg471x5_reset,
	.load = fpga_slg471x5_load,
};

static int fpga_slg471x5_init(const struct device *dev)
{
	const struct fpga_slg471x5_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	return 0;
}

#define SLG471X5_INIT(type, inst)                                                                  \
	static struct fpga_slg471x5_data fpga_slg##type##_data_##inst;                             \
                                                                                                   \
	static mem_region_t fpga_slg##type##_verify_list[] = FPGA_SLG##type##_VERIFY_LIST;         \
                                                                                                   \
	static const struct fpga_slg471x5_config fpga_slg##type##_config_##inst = {                \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.verify_list = fpga_slg##type##_verify_list,                                       \
		.verify_list_len = sizeof(fpga_slg##type##_verify_list) / sizeof(mem_region_t),    \
		.try_unconfigured = DT_INST_NODE_HAS_PROP(inst, try_unconfigured),                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fpga_slg471x5_init, NULL, &fpga_slg##type##_data_##inst,       \
			      &fpga_slg##type##_config_##inst, POST_KERNEL,                        \
			      CONFIG_FPGA_INIT_PRIORITY, &fpga_slg471x5_api)

#define FPGA_SLG47105_VERIFY_LIST                                                                  \
	{                                                                                          \
		{.addr = 0x00, .len = 0x47},                                                       \
		{.addr = 0x4C, .len = 0x01},                                                       \
		{.addr = 0xFD, .len = 0x01},                                                       \
	}

#define SLG47105_INIT(inst) SLG471X5_INIT(47105, inst)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_slg47105
DT_INST_FOREACH_STATUS_OKAY(SLG47105_INIT)

#define FPGA_SLG47115_VERIFY_LIST                                                                  \
	{                                                                                          \
		{.addr = 0x00, .len = 0x47},                                                       \
		{.addr = 0x4C, .len = 0x01},                                                       \
		{.addr = 0xFD, .len = 0x01},                                                       \
	}

#define SLG47115_INIT(inst) SLG471X5_INIT(47115, inst)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_slg47115
DT_INST_FOREACH_STATUS_OKAY(SLG47115_INIT)
