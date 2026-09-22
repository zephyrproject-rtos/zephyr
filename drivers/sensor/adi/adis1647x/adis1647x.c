/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <stddef.h>

#include "adis1647x.h"

LOG_MODULE_REGISTER(adis1647x, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT adi_adis1647x

int adis1647x_verify_burst(const struct adis1647x_burst_data *b)
{
	/* Verify checksum: sum of all bytes from diag_stat to data_cntr */
	uint16_t sum = 0;
	const uint8_t *base = (const uint8_t *)b;
	const size_t start = offsetof(struct adis1647x_burst_data, diag_stat);
	const size_t end = offsetof(struct adis1647x_burst_data, spi_chksum);

	for (size_t i = start; i < end; i++) {
		sum += base[i];
	}
	if (sum != sys_be16_to_cpu(b->spi_chksum)) {
		LOG_ERR("Checksum mismatch: calc=0x%04X recv=0x%04X", sum,
			sys_be16_to_cpu(b->spi_chksum));
		return -EIO;
	}

	/* DIAG_STAT register logs hardware faults */
	uint16_t diag = sys_be16_to_cpu(b->diag_stat);

	if (diag != 0) {
		LOG_ERR("DIAG_STAT: 0x%04X", diag);
		return -EIO;
	}

	return 0;
}

int adis1647x_get_data(const struct device *dev, struct adis1647x_sample_data *sample_data)
{
	struct adis1647x_data *device_data = dev->data;

	sample_data->accel_scale_num = device_data->accel_scale_num;
	sample_data->gyro_scale_num = device_data->gyro_scale_num;

	int ret = adis1647x_burst_read(dev, &sample_data->burst_data);

	if (ret != 0) {
		return ret;
	}

	return adis1647x_verify_burst(&sample_data->burst_data);
}

static int adis1647x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(chan);

	struct adis1647x_data *data = dev->data;
	struct adis1647x_sample_data sample = {0};
	int ret;

	ret = adis1647x_get_data(dev, &sample);
	if (ret != 0) {
		return ret;
	}

	data->gyro_x = (int16_t)sys_be16_to_cpu(sample.burst_data.x_gyro_out);
	data->gyro_y = (int16_t)sys_be16_to_cpu(sample.burst_data.y_gyro_out);
	data->gyro_z = (int16_t)sys_be16_to_cpu(sample.burst_data.z_gyro_out);
	data->accel_x = (int16_t)sys_be16_to_cpu(sample.burst_data.x_accel_out);
	data->accel_y = (int16_t)sys_be16_to_cpu(sample.burst_data.y_accel_out);
	data->accel_z = (int16_t)sys_be16_to_cpu(sample.burst_data.z_accel_out);
	data->temp = (int16_t)sys_be16_to_cpu(sample.burst_data.temp_out);

	return 0;
}

static int adis1647x_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct adis1647x_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adis1647x_accel_convert(val, data->accel_x, data->accel_scale_num);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adis1647x_accel_convert(val, data->accel_y, data->accel_scale_num);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adis1647x_accel_convert(val, data->accel_z, data->accel_scale_num);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adis1647x_accel_convert(val++, data->accel_x, data->accel_scale_num);
		adis1647x_accel_convert(val++, data->accel_y, data->accel_scale_num);
		adis1647x_accel_convert(val, data->accel_z, data->accel_scale_num);
		break;
	case SENSOR_CHAN_GYRO_X:
		adis1647x_gyro_convert(val, data->gyro_x, data->gyro_scale_num);
		break;
	case SENSOR_CHAN_GYRO_Y:
		adis1647x_gyro_convert(val, data->gyro_y, data->gyro_scale_num);
		break;
	case SENSOR_CHAN_GYRO_Z:
		adis1647x_gyro_convert(val, data->gyro_z, data->gyro_scale_num);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		adis1647x_gyro_convert(val++, data->gyro_x, data->gyro_scale_num);
		adis1647x_gyro_convert(val++, data->gyro_y, data->gyro_scale_num);
		adis1647x_gyro_convert(val, data->gyro_z, data->gyro_scale_num);
		break;
	case SENSOR_CHAN_DIE_TEMP: {
		int64_t micro_celsius = (int64_t)data->temp * 100000;

		val->val1 = (int32_t)(micro_celsius / 1000000);
		val->val2 = (int32_t)(micro_celsius % 1000000);
		break;
	}
	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, adis1647x_api) = {
	.sample_fetch = adis1647x_sample_fetch,
	.channel_get = adis1647x_channel_get,
	.get_decoder = adis1647x_get_decoder,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = adis1647x_submit,
#endif
#ifdef CONFIG_ADIS1647X_TRIGGER
	.trigger_set = adis1647x_trigger_set,
#endif
};

/* Order must match the enum in adi,adis1647x.yaml */
static const struct adis1647x_model_cfg adis1647x_model_table[] = {
	[ADIS1647X_MODEL_ADIS16470] = {.accel_scale_num = 125,
				       .gyro_scale_num = 10000,
				       .prod_id = 0x4056,
				       .rang_mdl = 0x03},
	[ADIS1647X_MODEL_ADIS16475_1] = {.accel_scale_num = 25,
					 .gyro_scale_num = 625,
					 .prod_id = 0x405B,
					 .rang_mdl = 0x00},
	[ADIS1647X_MODEL_ADIS16475_2] = {.accel_scale_num = 25,
					 .gyro_scale_num = 2500,
					 .prod_id = 0x405B,
					 .rang_mdl = 0x01},
	[ADIS1647X_MODEL_ADIS16475_3] = {.accel_scale_num = 25,
					 .gyro_scale_num = 10000,
					 .prod_id = 0x405B,
					 .rang_mdl = 0x03},
	[ADIS1647X_MODEL_ADIS16477_1] = {.accel_scale_num = 125,
					 .gyro_scale_num = 625,
					 .prod_id = 0x405D,
					 .rang_mdl = 0x00},
	[ADIS1647X_MODEL_ADIS16477_2] = {.accel_scale_num = 125,
					 .gyro_scale_num = 2500,
					 .prod_id = 0x405D,
					 .rang_mdl = 0x01},
	[ADIS1647X_MODEL_ADIS16477_3] = {.accel_scale_num = 125,
					 .gyro_scale_num = 10000,
					 .prod_id = 0x405D,
					 .rang_mdl = 0x03},
};

static int adis1647x_init(const struct device *dev)
{
	const struct adis1647x_config *cfg = dev->config;
	struct adis1647x_data *data = dev->data;
	int ret;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->spi.bus);
		return -ENODEV;
	}

	/* Hardware reset if pin is wired */
	if (cfg->reset.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO: %d", ret);
			return ret;
		}
		gpio_pin_set_dt(&cfg->reset, 1);
		k_msleep(ADIS1647X_HW_RESET_PULSE_MS);
		gpio_pin_set_dt(&cfg->reset, 0);
		k_msleep(ADIS1647X_HW_RESET_WAIT_MS);
	}

	ret = adis1647x_reg_write_no_verify(dev, ADIS1647X_REG_GLOBAL_CMD,
					    ADIS1647X_REG_GLOBAL_CMD_SW_RESET);
	if (ret != 0) {
		LOG_ERR("Failed to send SW reset: %d", ret);
		return ret;
	}
	k_sleep(K_MSEC(ADIS1647X_SW_RESET_MS));

	const struct adis1647x_model_cfg *mcfg = &adis1647x_model_table[cfg->model_idx];

	uint16_t prod_id;

	ret = adis1647x_reg_read(dev, ADIS1647X_REG_PROD_ID, &prod_id);
	if (ret != 0) {
		LOG_ERR("Failed to read PROD_ID: %d", ret);
		return ret;
	}
	if (prod_id != mcfg->prod_id) {
		LOG_ERR("PROD_ID mismatch: expected 0x%04X got 0x%04X", mcfg->prod_id, prod_id);
		return -ENODEV;
	}

	uint16_t rang_mdl_reg;

	ret = adis1647x_reg_read(dev, ADIS1647X_REG_RANG_MDL, &rang_mdl_reg);
	if (ret != 0) {
		LOG_ERR("Failed to read RANG_MDL: %d", ret);
		return ret;
	}
	if (((rang_mdl_reg >> 2) & 0x03) != mcfg->rang_mdl) {
		LOG_ERR("RANG_MDL mismatch: expected 0x%02X got 0x%02X", mcfg->rang_mdl,
			(rang_mdl_reg >> 2) & 0x03);
		return -ENODEV;
	}

	LOG_INF("Device validated: PROD_ID=0x%04X RANG_MDL=0x%02X", prod_id, mcfg->rang_mdl);

	data->accel_scale_num = mcfg->accel_scale_num;
	data->gyro_scale_num = mcfg->gyro_scale_num;

	uint16_t firm_rev;

	ret = adis1647x_reg_read(dev, ADIS1647X_REG_FIRM_REV, &firm_rev);
	if (ret == 0) {
		LOG_INF("Firmware revision: %d.%d", (firm_rev >> 8) & 0xFF, firm_rev & 0xFF);
	}

	ret = adis1647x_reg_write(dev, ADIS1647X_REG_MSC_CTRL, ADIS1647X_REG_MSC_CTRL_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	k_busy_wait(ADIS1647X_SPI_STALL_US);

	ret = adis1647x_reg_write(dev, ADIS1647X_REG_DEC_RATE, cfg->dec_rate);
	if (ret != 0) {
		LOG_ERR("Failed to write decimation rate");
		return ret;
	}

#ifdef CONFIG_ADIS1647X_TRIGGER
	ret = adis1647x_init_interrupt(dev);
	if (ret != 0) {
		return ret;
	}
#endif

	LOG_INF("initialized: accel_scale_num=%u, gyro_scale_num=%u", data->accel_scale_num,
		data->gyro_scale_num);

	return 0;
}

#define ADIS1647X_SPI_CFG                                                                          \
	(SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA)

#ifdef CONFIG_SENSOR_ASYNC_API
#define ADIS1647X_RTIO_DEFINE(inst)                                                                \
	SPI_DT_IODEV_DEFINE(adis1647x_iodev_##inst, DT_DRV_INST(inst), ADIS1647X_SPI_CFG);         \
	RTIO_DEFINE(adis1647x_rtio_ctx_##inst, 8, 8);

#define ADIS1647X_RTIO_INIT(inst)                                                                  \
	.rtio_ctx = &adis1647x_rtio_ctx_##inst,                                                    \
	.iodev = &adis1647x_iodev_##inst,
#else
#define ADIS1647X_RTIO_DEFINE(inst)
#define ADIS1647X_RTIO_INIT(inst)
#endif /* CONFIG_SENSOR_ASYNC_API */

#define ADIS1647X_DEFINE(inst)                                                                     \
	ADIS1647X_RTIO_DEFINE(inst);                                                               \
	static struct adis1647x_data adis1647x_data_##inst = {                                     \
		ADIS1647X_RTIO_INIT(inst)                                                          \
	};                                                                                        \
	static const struct adis1647x_config adis1647x_config_##inst = {                           \
		.model_idx = DT_INST_ENUM_IDX(inst, model),                                        \
		.dec_rate = DT_INST_PROP(inst, dec_rate),                                          \
		.spi = SPI_DT_SPEC_INST_GET(inst, ADIS1647X_SPI_CFG),                              \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
		IF_ENABLED(CONFIG_ADIS1647X_TRIGGER,                                              \
			(.interrupt = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, {0}),))          \
	};                                                                                        \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adis1647x_init, NULL, &adis1647x_data_##inst,           \
				     &adis1647x_config_##inst, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &adis1647x_api);

DT_INST_FOREACH_STATUS_OKAY(ADIS1647X_DEFINE)
