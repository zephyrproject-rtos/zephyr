/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>

#include "icp201xx_drv.h"
#include "Icp201xxDriver.h"

LOG_MODULE_REGISTER(ICP201XX, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_FPU
#include <math.h>
#define ATMOSPHERICAL_PRESSURE_KPA ((float)101.325)
#define TO_KELVIN(temp_C)          (((float)273.15) + temp_C)
/* Constant in altitude formula:
 * M*g/R = (0,0289644 * 9,80665 / 8,31432)
 * with M the molar mass of air.
 * with g the gravitational force acceleration.
 * with R the universal gaz constant.
 */
#define HEIGHT_TO_PRESSURE_COEFF   ((float)0.03424)

/* Constant in altitude formula:
 * R / (M*g) = 8,31432 / (0,0289644 * 9,80665)
 * with M the molar mass of air.
 * with g the gravitational force acceleration.
 * with R the universal gaz constant.
 */
#define PRESSURE_TO_HEIGHT_COEFF   ((float)29.27127)
/* Constant for altitude formula:
 * logarithm of pressure at 0m
 * ln(101.325)
 */
#define LOG_ATMOSPHERICAL_PRESSURE ((float)4.61833)
#endif

#define IO_DRIVE_STRENGTH_ADDR 0x0D

void inv_icp201xx_sleep_us(int us)
{
	k_sleep(K_USEC(us));
}

static int inv_io_hal_read_reg(void *ctx, uint8_t reg, uint8_t *rbuffer, uint32_t rlen)
{
	struct device *dev = (struct device *)ctx;
	const struct icp201xx_config *cfg = (const struct icp201xx_config *)dev->config;

	return cfg->bus_io->read(&cfg->bus, reg, rbuffer, rlen);
}

static int inv_io_hal_write_reg(void *ctx, uint8_t reg, const uint8_t *wbuffer, uint32_t wlen)
{
	struct device *dev = (struct device *)ctx;
	const struct icp201xx_config *cfg = (const struct icp201xx_config *)dev->config;

	return cfg->bus_io->write(&cfg->bus, reg, (uint8_t *)wbuffer, wlen);
}

#ifdef CONFIG_FPU
static float convertToHeight(float pressure_kp, float temperature_C)
{
	return PRESSURE_TO_HEIGHT_COEFF * TO_KELVIN(temperature_C) *
	       (LOG_ATMOSPHERICAL_PRESSURE - logf(pressure_kp));
}
#endif

/*
 * ICP201xx warm up.
 * If FIR filter is enabled, it will cause a settling effect on the first 14 pressure values.
 * Therefore the first 14 pressure output values are discarded.
 *
 */
void inv_icp201xx_app_warmup(inv_icp201xx_t *icp_device, icp201xx_op_mode_t op_mode,
			     icp201xx_meas_mode_t meas_mode)
{
	volatile uint8_t fifo_packets = 0;
	uint8_t fifo_packets_to_skip = 14;

	do {
		fifo_packets = 0;
		if (!inv_icp201xx_get_fifo_count(icp_device, (uint8_t *)&fifo_packets) &&
		    (fifo_packets >= fifo_packets_to_skip)) {
			uint8_t i_status = 0;

			inv_icp201xx_flush_fifo(icp_device);

			inv_icp201xx_get_int_status(icp_device, &i_status);
			if (i_status) {
				inv_icp201xx_clear_int_status(icp_device, i_status);
			}
			break;
		}
		inv_icp201xx_sleep_us(2000);
	} while (1);
}

static int icp201xx_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	int err = 0;
	struct icp201xx_data *data = (struct icp201xx_data *)dev->data;

	__ASSERT_NO_MSG(val != NULL);
	icp201xx_mutex_lock(dev);
	if (chan == SENSOR_CHAN_PRESS) {
		if (attr == SENSOR_ATTR_CONFIGURATION) {
			if ((val->val1 >= ICP201XX_OP_MODE0) && (val->val1 <= ICP201XX_OP_MODE4)) {
				data->op_mode = val->val1;
				err = inv_icp201xx_soft_reset(&(data->icp_device));
				err = inv_icp201xx_config(&(data->icp_device), data->op_mode,
							  ICP201XX_FIFO_READOUT_MODE_PRES_TEMP);
				inv_icp201xx_app_warmup(&(data->icp_device), data->op_mode,
							ICP201XX_MEAS_MODE_CONTINUOUS);
			} else {
				LOG_ERR("Not supported ATTR value");
				icp201xx_mutex_unlock(dev);
				return -EINVAL;
			}
#ifdef CONFIG_ICP201XX_TRIGGER
		} else if (attr == SENSOR_ATTR_SLOPE_TH) {
			data->pressure_change = *val;
		} else if ((attr == SENSOR_ATTR_LOWER_THRESH) ||
			   (attr == SENSOR_ATTR_UPPER_THRESH)) {
			if (val->val1 > 0) {
				data->pressure_threshold = *val;
			} else {
				LOG_ERR("Not supported ATTR value");
				icp201xx_mutex_unlock(dev);
				return -EINVAL;
			}
#endif
		} else {
			LOG_ERR("Not supported ATTR");
			icp201xx_mutex_unlock(dev);
			return -EINVAL;
		}
	};
	icp201xx_mutex_unlock(dev);
	return err;
}

static int icp201xx_sample_fetch(const struct device *dev, const enum sensor_channel chan)
{
	struct icp201xx_data *data = (struct icp201xx_data *)dev->data;
	uint8_t fifo_packets;
	uint8_t fifo_data[6];

	icp201xx_mutex_lock(dev);

	/** Read measurements count in FIFO **/
	if (inv_icp201xx_get_fifo_count(&(data->icp_device), &fifo_packets)) {
		icp201xx_mutex_unlock(dev);
		return -2;
	}
	if (fifo_packets == 0) {
		icp201xx_mutex_unlock(dev);
		return -1;
	}

	for (int i = 0; i < fifo_packets; i++) {
		inv_icp201xx_get_fifo_data(&(data->icp_device), 1, fifo_data);
		if (data->icp_device.serif.if_mode == ICP201XX_IF_I2C) {
			/*
			 * Perform dummy read to 0x00 register as last transaction after FIFO read
			 * for I2C interface
			 */
			do {
				uint8_t dummy_reg = 0;

				inv_io_hal_read_reg((struct device *)dev, 0, &dummy_reg, 1);
			} while (0);
		}
	}
	inv_icp201xx_process_raw_data(&(data->icp_device), 1, fifo_data, &data->raw_pressure,
				      &data->raw_temperature);
	icp201xx_mutex_unlock(dev);
	return 0;
}

static void icp201xx_convert_pressure(struct sensor_value *val, int32_t raw_val)
{
	/* Raw value is coded on 20-bits */
	if (raw_val & 0x080000) {
		raw_val |= 0xFFF00000;
	}
	/* P = (POUT/2^17)*40kPa + 70kPa */
	val->val1 = (raw_val * 40) / 131072 + 70;
	val->val2 = ((uint64_t)(raw_val * 40) % 131072) * 1000000 / 131072;
}

static void icp201xx_convert_temperature(struct sensor_value *val, int32_t raw_val)
{
	/* Raw value is coded on 20-bits */
	if (raw_val & 0x080000) {
		raw_val |= 0xFFF00000;
	}
	/* T = (TOUT/2^18)*65C + 25C */
	val->val1 = (raw_val * 65) / 262144 + 25;
	val->val2 = ((uint64_t)(raw_val * 65) % 262144) * 1000000 / 262144;
}

static int icp201xx_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct icp201xx_data *data = (struct icp201xx_data *)dev->data;

	if (!(chan == SENSOR_CHAN_AMBIENT_TEMP || chan == SENSOR_CHAN_PRESS ||
	      chan == SENSOR_CHAN_ALTITUDE)) {
		return -ENOTSUP;
	}
	icp201xx_mutex_lock(dev);

	if (chan == SENSOR_CHAN_PRESS) {
		icp201xx_convert_pressure(val, data->raw_pressure);
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		icp201xx_convert_temperature(val, data->raw_temperature);
#ifdef CONFIG_FPU
	} else if (chan == SENSOR_CHAN_ALTITUDE) {
		struct sensor_value pressure_val, temp_val;
		float pressure, temperature, altitude;

		icp201xx_convert_pressure(&pressure_val, data->raw_pressure);
		icp201xx_convert_temperature(&temp_val, data->raw_pressure);
		pressure = pressure_val.val1 + ((float)pressure_val.val2 / 1000000);
		temperature = temp_val.val1 + ((float)temp_val.val2 / 1000000);
		altitude = convertToHeight(pressure, temperature);
		sensor_value_from_float(val, altitude);
#endif
	} else {
		icp201xx_mutex_unlock(dev);
		return -ENOTSUP;
	}

	icp201xx_mutex_unlock(dev);
	return 0;
}

static int icp201xx_set_drive_strength(const struct device *dev, uint8_t drive)
{
	int rc = 0;
	const uint8_t active_mode = 0x4;
	const uint8_t normal_mode = 0x0;
	const uint8_t unlock = 0x1F;
	const uint8_t lock = 0x0;

	inv_icp201xx_sleep_us(4000);

	/* Set Active power mode */
	rc |= inv_io_hal_write_reg((struct device *)dev, MPUREG_MODE_SELECT, &active_mode, 1);
	inv_icp201xx_sleep_us(4000);

	/* Unlock main register write access */
	rc |= inv_io_hal_write_reg((struct device *)dev, MPUREG_MASTER_LOCK, &unlock, 1);
	inv_icp201xx_sleep_us(4000);

	/* Write IO drive strength */
	rc |= inv_io_hal_write_reg((struct device *)dev, IO_DRIVE_STRENGTH_ADDR, &drive, 1);

	/* Lock main register write access */
	rc |= inv_io_hal_write_reg((struct device *)dev, MPUREG_MASTER_LOCK, &lock, 1);

	/* Set Normal power mode */
	rc |= inv_io_hal_write_reg((struct device *)dev, MPUREG_MODE_SELECT, &normal_mode, 1);

	inv_icp201xx_sleep_us(4000);

	return rc;
}

static int icp201xx_init(const struct device *dev)
{
	struct icp201xx_data *data = (struct icp201xx_data *)dev->data;
	struct icp201xx_config *config = (struct icp201xx_config *)dev->config;
	inv_icp201xx_serif_t icp_serif;
	int rc = 0;
	uint8_t who_am_i;
	uint8_t icp_version;

	icp_serif.if_mode = config->if_mode;

	/* Initialize serial interface between MCU and Icp201xx */
	icp_serif.context = (void *)dev;
	icp_serif.read_reg = inv_io_hal_read_reg;
	icp_serif.write_reg = inv_io_hal_write_reg;
	icp_serif.max_read = 2048;  /* maximum number of bytes allowed per serial read */
	icp_serif.max_write = 2048; /* maximum number of bytes allowed per serial write */

	data->op_mode = config->op_mode;

	rc = icp201xx_set_drive_strength(dev, config->drive_strength);
	if (rc != INV_ERROR_SUCCESS) {
		LOG_ERR("Drive strength error");
		return rc;
	}

	rc = inv_icp201xx_init(&(data->icp_device), &icp_serif);
	if (rc != INV_ERROR_SUCCESS) {
		LOG_ERR("Init error");
		return rc;
	}

	rc = inv_icp201xx_soft_reset(&(data->icp_device));
	if (rc != INV_ERROR_SUCCESS) {
		LOG_ERR("Reset error");
		return rc;
	}

	/* Check WHOAMI */
	rc = inv_icp201xx_get_devid_version(&(data->icp_device), &who_am_i, &icp_version);
	if (rc != 0) {
		LOG_ERR("Device id error");
		return -2;
	}

	if (who_am_i != EXPECTED_DEVICE_ID) {
		LOG_ERR("Wrong device id");
		return -3;
	}

	/* Boot up OTP config */
	rc = inv_icp201xx_OTP_bootup_cfg(&(data->icp_device));
	if (rc != 0) {
		LOG_ERR("Bootup error");
		return rc;
	}
	rc = inv_icp201xx_soft_reset(&(data->icp_device));
	if (rc != 0) {
		LOG_ERR("Reset error");
		return rc;
	}
	rc = inv_icp201xx_config(&(data->icp_device), data->op_mode,
				 ICP201XX_FIFO_READOUT_MODE_PRES_TEMP);
	if (rc != 0) {
		LOG_ERR("Config error");
		return rc;
	}
	inv_icp201xx_app_warmup(&(data->icp_device), data->op_mode, ICP201XX_MEAS_MODE_CONTINUOUS);

	if (IS_ENABLED(CONFIG_ICP201XX_TRIGGER)) {
		rc = icp201xx_trigger_init(dev);
		if (rc < 0) {
			LOG_ERR("Failed to initialize interrupt.");
			return rc;
		}
	}

	/* successful init, return 0 */
	return 0;
}

void icp201xx_mutex_lock(const struct device *dev)
{
#ifdef CONFIG_ICP201XX_TRIGGER
	const struct icp201xx_config *config = dev->config;

	if (config->gpio_int.port) {
		struct icp201xx_data *data = dev->data;

		k_mutex_lock(&data->mutex, K_FOREVER);
	}
#else
	ARG_UNUSED(dev);
#endif
}

void icp201xx_mutex_unlock(const struct device *dev)
{
#ifdef CONFIG_ICP201XX_TRIGGER
	const struct icp201xx_config *config = dev->config;

	if (config->gpio_int.port) {
		struct icp201xx_data *data = dev->data;

		k_mutex_unlock(&data->mutex);
	}
#else
	ARG_UNUSED(dev);
#endif
}

static DEVICE_API(sensor, icp201xx_api_funcs) = {.sample_fetch = icp201xx_sample_fetch,
							    .channel_get = icp201xx_channel_get,
							    .attr_set = icp201xx_attr_set,
#ifdef CONFIG_ICP201XX_TRIGGER
							    .trigger_set = icp201xx_trigger_set
#endif
};

/* Common initialization for icp201xx_config */
#define ICP201XX_CONFIG(inst)                                              \
	IF_ENABLED(CONFIG_ICP201XX_TRIGGER,                                  \
		(.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),)) \
		.op_mode = DT_INST_ENUM_IDX(inst, op_mode),                    \
		.drive_strength = DT_INST_ENUM_IDX(inst, drive_strength)

/* Initializes a struct icp201xx_config for an instance on an I2C bus. */
#define ICP201XX_CONFIG_I2C(inst)                          \
	{                                                      \
		.if_mode = ICP201XX_IF_I2C,                        \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst),                 \
		.bus_io = &icp201xx_bus_io_i2c,                    \
		ICP201XX_CONFIG(inst)                                  \
	}

/* Initializes a struct icp201xx_config for an instance on a SPI bus. */
#define ICP201XX_CONFIG_SPI(inst)                                  \
	{                                                              \
		.if_mode = ICP201XX_IF_4_WIRE_SPI,                         \
		.bus.spi = SPI_DT_SPEC_INST_GET(inst,                      \
			(SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |    \
			SPI_TRANSFER_MSB | SPI_FULL_DUPLEX |      \
			SPI_MODE_CPHA | SPI_MODE_CPOL),           \
			0),                                        \
		.bus_io = &icp201xx_bus_io_spi,                            \
		ICP201XX_CONFIG(inst)                                          \
	}

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define ICP201XX_DEFINE(inst)                                            \
	static struct icp201xx_data icp201xx_drv_##inst;                       \
	static const struct icp201xx_config icp201xx_config_##inst =           \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),                               \
		(ICP201XX_CONFIG_SPI(inst)),	                                       \
		(ICP201XX_CONFIG_I2C(inst)));                                        \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icp201xx_init, NULL, &icp201xx_drv_##inst, \
		&icp201xx_config_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,   \
		&icp201xx_api_funcs);

#define DT_DRV_COMPAT invensense_icp201xx
DT_INST_FOREACH_STATUS_OKAY(ICP201XX_DEFINE)
