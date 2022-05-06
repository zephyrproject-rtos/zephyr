/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_hts221

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <zephyr/logging/log.h>

#include "hts221.h"

LOG_MODULE_REGISTER(HTS221, CONFIG_SENSOR_LOG_LEVEL);

struct str2odr {
	const char *str;
	hts221_odr_t odr;
};

static const struct str2odr hts221_odrs[] = {
	{ "1", HTS221_ODR_1Hz },
	{ "7", HTS221_ODR_7Hz },
	{ "12.5", HTS221_ODR_12Hz5 },
};

static int hts221_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct hts221_data *data = dev->data;
	int32_t conv_val;

	/*
	 * see "Interpreting humidity and temperature readings" document
	 * for more details
	 */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		conv_val = (int32_t)(data->t1_degc_x8 - data->t0_degc_x8) *
			   (data->t_sample - data->t0_out) /
			   (data->t1_out - data->t0_out) +
			   data->t0_degc_x8;

		/* convert temperature x8 to degrees Celsius */
		val->val1 = conv_val / 8;
		val->val2 = (conv_val % 8) * (1000000 / 8);
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		conv_val = (int32_t)(data->h1_rh_x2 - data->h0_rh_x2) *
			   (data->rh_sample - data->h0_t0_out) /
			   (data->h1_t0_out - data->h0_t0_out) +
			   data->h0_rh_x2;

		/* convert humidity x2 to percent */
		val->val1 = conv_val / 2;
		val->val2 = (conv_val % 2) * 500000;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int hts221_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct hts221_data *data = dev->data;
	const struct hts221_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t buf[4];
	int status;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	status = hts221_read_reg(ctx, HTS221_HUMIDITY_OUT_L |
				 HTS221_AUTOINCREMENT_ADDR, buf, 4);
	if (status < 0) {
		LOG_ERR("Failed to fetch data sample.");
		return status;
	}

	data->rh_sample = sys_le16_to_cpu(buf[0] | (buf[1] << 8));
	data->t_sample = sys_le16_to_cpu(buf[2] | (buf[3] << 8));

	return 0;
}

static int hts221_read_conversion_data(const struct device *dev)
{
	struct hts221_data *data = dev->data;
	const struct hts221_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t buf[16];
	int status;

	status = hts221_read_reg(ctx, HTS221_H0_RH_X2 |
				 HTS221_AUTOINCREMENT_ADDR, buf, 16);
	if (status < 0) {
		LOG_ERR("Failed to read conversion data.");
		return status;
	}

	data->h0_rh_x2 = buf[0];
	data->h1_rh_x2 = buf[1];
	data->t0_degc_x8 = sys_le16_to_cpu(buf[2] | ((buf[5] & 0x3) << 8));
	data->t1_degc_x8 = sys_le16_to_cpu(buf[3] | ((buf[5] & 0xC) << 6));
	data->h0_t0_out = sys_le16_to_cpu(buf[6] | (buf[7] << 8));
	data->h1_t0_out = sys_le16_to_cpu(buf[10] | (buf[11] << 8));
	data->t0_out = sys_le16_to_cpu(buf[12] | (buf[13] << 8));
	data->t1_out = sys_le16_to_cpu(buf[14] | (buf[15] << 8));

	return 0;
}

static const struct sensor_driver_api hts221_driver_api = {
#if HTS221_TRIGGER_ENABLED
	.trigger_set = hts221_trigger_set,
#endif
	.sample_fetch = hts221_sample_fetch,
	.channel_get = hts221_channel_get,
};

int hts221_init(const struct device *dev)
{
	const struct hts221_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t id, idx;
	int status;

	/* check chip ID */

	status = hts221_device_id_get(ctx, &id);
	if (status < 0) {
		LOG_ERR("Failed to read chip ID.");
		return status;
	}

	if (id != HTS221_ID) {
		LOG_ERR("Invalid chip ID.");
		return -EINVAL;
	}

	/* check if CONFIG_HTS221_ODR is valid */
	for (idx = 0U; idx < ARRAY_SIZE(hts221_odrs); idx++) {
		if (!strcmp(hts221_odrs[idx].str, CONFIG_HTS221_ODR)) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(hts221_odrs)) {
		LOG_ERR("Invalid ODR value %s.", CONFIG_HTS221_ODR);
		return -EINVAL;
	}

	status = hts221_data_rate_set(ctx, hts221_odrs[idx].odr);
	if (status < 0) {
		LOG_ERR("Could not set output data rate");
		return status;
	}

	status = hts221_block_data_update_set(ctx, 1);
	if (status < 0) {
		LOG_ERR("Could not set BDU bit");
		return status;
	}

	status = hts221_power_on_set(ctx, 1);
	if (status < 0) {
		LOG_ERR("Could not set PD bit");
		return status;
	}

	/*
	 * the device requires about 2.2 ms to download the flash content
	 * into the volatile mem
	 */
	k_sleep(K_MSEC(3));

	status = hts221_read_conversion_data(dev);
	if (status < 0) {
		LOG_ERR("Failed to read conversion data.");
		return status;
	}

#if HTS221_TRIGGER_ENABLED
	status = hts221_init_interrupt(dev);
	if (status < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return status;
	}
#else
	LOG_INF("Cannot enable trigger without drdy-gpios");
#endif

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "HTS221 driver enabled without any devices"
#endif

/*
 * Device creation macros
 */

#define HTS221_DEVICE_INIT(inst)					\
	DEVICE_DT_INST_DEFINE(inst,					\
			      hts221_init,				\
			      NULL,					\
			      &hts221_data_##inst,			\
			      &hts221_config_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &hts221_driver_api);

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#ifdef CONFIG_HTS221_TRIGGER
#define HTS221_CFG_IRQ(inst)					\
	.gpio_drdy = GPIO_DT_SPEC_INST_GET(inst, irq_gpios)
#else
#define HTS221_CFG_IRQ(inst)
#endif /* CONFIG_HTS221_TRIGGER */

#define HTS221_SPI_OPERATION (SPI_WORD_SET(8) |				\
			      SPI_OP_MODE_MASTER |			\
			      SPI_MODE_CPOL |				\
			      SPI_MODE_CPHA |				\
			      SPI_HALF_DUPLEX)				\

#define HTS221_CONFIG_SPI(inst)						\
	{								\
		.ctx = {						\
			.read_reg =					\
			   (stmdev_read_ptr) stmemsc_spi_read,		\
			.write_reg =					\
			   (stmdev_write_ptr) stmemsc_spi_write,	\
			.handle =					\
			   (void *)&hts221_config_##inst.stmemsc_cfg,	\
		},							\
		.stmemsc_cfg = {					\
			.spi = SPI_DT_SPEC_INST_GET(inst,		\
						    HTS221_SPI_OPERATION, \
						    0),			\
		},							\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irq_gpios),	\
			(HTS221_CFG_IRQ(inst)), ())			\
	}

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define HTS221_CONFIG_I2C(inst)						\
	{								\
		.ctx = {						\
			.read_reg =					\
			   (stmdev_read_ptr) stmemsc_i2c_read,		\
			.write_reg =					\
			   (stmdev_write_ptr) stmemsc_i2c_write,	\
			.handle =					\
			   (void *)&hts221_config_##inst.stmemsc_cfg,	\
		},							\
		.stmemsc_cfg = {					\
			.i2c = I2C_DT_SPEC_INST_GET(inst),		\
		},							\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irq_gpios),	\
			(HTS221_CFG_IRQ(inst)), ())			\
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define HTS221_DEFINE(inst)						\
	static struct hts221_data hts221_data_##inst;			\
	static const struct hts221_config hts221_config_##inst =	\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),			\
			    (HTS221_CONFIG_SPI(inst)),			\
			    (HTS221_CONFIG_I2C(inst)));			\
	HTS221_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(HTS221_DEFINE)
