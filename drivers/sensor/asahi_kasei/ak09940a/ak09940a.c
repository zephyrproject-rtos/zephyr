/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT asahi_kasei_ak09940a

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "ak09940a.h"

LOG_MODULE_REGISTER(AK09940A, CONFIG_SENSOR_LOG_LEVEL);

/* Reset pulse width (tRSTL) and time until registers can be accessed after a reset */
#define AK09940A_RESET_PULSE_US 5
#define AK09940A_RESET_TIME_US  100

static const struct ak099xx_odr ak09940a_odr_table[] = {
	AK099XX_ODR_COMMON,
	{200, AK09940A_MODE_CONT_200HZ},
	{400, AK09940A_MODE_CONT_400HZ},
	{1000, AK09940A_MODE_CONT_1000HZ},
	{2500, AK09940A_MODE_CONT_2500HZ},
};

/* In the order of the sensor-drive devicetree enum */
static const struct ak09940a_drive ak09940a_drives[] = {
	/* Low power drive 1 */
	{0, FIELD_PREP(AK09940A_CNTL3_MT, 0), 700, 7},
	/* Low power drive 2 */
	{0, FIELD_PREP(AK09940A_CNTL3_MT, 1), 1000, 6},
	/* Low noise drive 1 */
	{0, FIELD_PREP(AK09940A_CNTL3_MT, 2), 1700, 5},
	/* Low noise drive 2 */
	{0, FIELD_PREP(AK09940A_CNTL3_MT, 3), 3100, 5},
	/* Ultra low power drive */
	{AK09940A_CNTL1_MT2, 0, 400, 8},
};

#if AK09940A_BUS_I2C
static int ak09940a_i2c_check(const union ak09940a_bus *bus)
{
	return i2c_is_ready_dt(&bus->i2c) ? 0 : -ENODEV;
}

static int ak09940a_i2c_read(const union ak09940a_bus *bus, uint8_t reg, uint8_t *buf, size_t len)
{
	return i2c_burst_read_dt(&bus->i2c, reg, buf, len);
}

static int ak09940a_i2c_write(const union ak09940a_bus *bus, uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte_dt(&bus->i2c, reg, val);
}

static const struct ak09940a_bus_io ak09940a_bus_io_i2c = {
	.check = ak09940a_i2c_check,
	.read = ak09940a_i2c_read,
	.write = ak09940a_i2c_write,
};
#endif /* AK09940A_BUS_I2C */

#if AK09940A_BUS_SPI
static int ak09940a_spi_check(const union ak09940a_bus *bus)
{
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

static int ak09940a_spi_read(const union ak09940a_bus *bus, uint8_t reg, uint8_t *buf, size_t len)
{
	uint8_t addr = reg | AK09940A_SPI_READ;
	const struct spi_buf tx_buf = {.buf = &addr, .len = 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf rx_bufs[] = {
		{.buf = NULL, .len = 1},
		{.buf = buf, .len = len},
	};
	const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

	return spi_transceive_dt(&bus->spi, &tx, &rx);
}

static int ak09940a_spi_write(const union ak09940a_bus *bus, uint8_t reg, uint8_t val)
{
	uint8_t cmd[] = {reg, val};
	const struct spi_buf tx_buf = {.buf = cmd, .len = sizeof(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(&bus->spi, &tx);
}

static const struct ak09940a_bus_io ak09940a_bus_io_spi = {
	.check = ak09940a_spi_check,
	.read = ak09940a_spi_read,
	.write = ak09940a_spi_write,
};
#endif /* AK09940A_BUS_SPI */

static inline int ak09940a_read(const struct device *dev, uint8_t reg, uint8_t *buf, size_t len)
{
	const struct ak09940a_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, reg, buf, len);
}

static inline int ak09940a_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct ak09940a_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, reg, val);
}

static int ak09940a_set_mode(const struct device *dev, uint8_t mode)
{
	const struct ak09940a_config *cfg = dev->config;
	struct ak09940a_data *data = dev->data;
	int ret;

	/* A mode change has to go through power-down mode */
	if (data->mode != AK099XX_MODE_POWER_DOWN) {
		ret = ak09940a_write(dev, AK09940A_REG_CNTL3,
				     cfg->drive->cntl3 | AK099XX_MODE_POWER_DOWN);
		if (ret != 0) {
			return ret;
		}
		data->mode = AK099XX_MODE_POWER_DOWN;
		k_usleep(AK099XX_MODE_CHANGE_WAIT_US);
	}

	if (mode != AK099XX_MODE_POWER_DOWN) {
		ret = ak09940a_write(dev, AK09940A_REG_CNTL3, cfg->drive->cntl3 | mode);
		if (ret != 0) {
			return ret;
		}
		data->mode = mode;
	}

	return 0;
}

static int ak09940a_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ak09940a_config *cfg = dev->config;
	struct ak09940a_data *data = dev->data;
	bool single = data->mode == AK099XX_MODE_POWER_DOWN;
	int ret;

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
	case SENSOR_CHAN_DIE_TEMP:
		break;
	default:
		return -ENOTSUP;
	}

	if (single) {
		ret = ak09940a_write(dev, AK09940A_REG_CNTL3,
				     cfg->drive->cntl3 | AK099XX_MODE_SINGLE);
		if (ret != 0) {
			LOG_ERR("Failed to start measurement (%d)", ret);
			return ret;
		}
		k_usleep(cfg->drive->measure_time_us);
	}

	/* Reading up to ST2 releases the data protection */
	ret = ak09940a_read(dev, AK09940A_REG_ST1, data->frame, sizeof(data->frame));
	if (ret != 0) {
		LOG_ERR("Failed to read measurement (%d)", ret);
		return ret;
	}

	/* In continuous mode, the data registers hold the last completed measurement */
	if (single && ((data->frame[AK09940A_FRAME_ST1] & AK099XX_ST1_DRDY) == 0U)) {
		LOG_DBG("Data not ready, st1=0x%02x", data->frame[AK09940A_FRAME_ST1]);
		return -EBUSY;
	}

	return 0;
}

static int ak09940a_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct ak09940a_data *data = dev->data;
	int32_t magn[3];

	for (uint8_t i = 0; i < ARRAY_SIZE(magn); i++) {
		magn[i] = ak09940a_frame_magn(data->frame, i) * AK09940A_MICRO_GAUSS_PER_LSB;
	}

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
		return sensor_value_from_micro(val, magn[chan - SENSOR_CHAN_MAGN_X]);
	case SENSOR_CHAN_MAGN_XYZ:
		for (uint8_t i = 0; i < ARRAY_SIZE(magn); i++) {
			(void)sensor_value_from_micro(&val[i], magn[i]);
		}
		return 0;
	case SENSOR_CHAN_DIE_TEMP:
		return sensor_value_from_micro(val, ak09940a_frame_temp_micro(data->frame));
	default:
		return -ENOTSUP;
	}
}

static bool ak09940a_is_odr_attr(enum sensor_channel chan, enum sensor_attribute attr)
{
	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		return attr == SENSOR_ATTR_SAMPLING_FREQUENCY;
	default:
		return false;
	}
}

static int ak09940a_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct ak09940a_config *cfg = dev->config;
	uint8_t mode;
	int ret;

	if (!ak09940a_is_odr_attr(chan, attr)) {
		return -ENOTSUP;
	}

	mode = ak099xx_odr_to_mode(ak09940a_odr_table, cfg->drive->odr_count, val);

	ret = ak09940a_set_mode(dev, mode);
	if (ret != 0) {
		LOG_ERR("Failed to set sampling frequency (%d)", ret);
	}

	return ret;
}

static int ak09940a_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	struct ak09940a_data *data = dev->data;

	if (!ak09940a_is_odr_attr(chan, attr)) {
		return -ENOTSUP;
	}

	ak099xx_mode_to_odr(ak09940a_odr_table, ARRAY_SIZE(ak09940a_odr_table), data->mode, val);

	return 0;
}

static int ak09940a_reset(const struct device *dev)
{
	const struct ak09940a_config *cfg = dev->config;
	int ret;

	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR_DEVICE_NOT_READY(cfg->reset_gpio.port);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			return ret;
		}
		k_busy_wait(AK09940A_RESET_PULSE_US);

		ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
		if (ret != 0) {
			return ret;
		}
	} else {
		ret = ak09940a_write(dev, AK09940A_REG_CNTL4, AK09940A_CNTL4_SRST);
		if (ret != 0) {
			return ret;
		}
	}

	k_usleep(AK09940A_RESET_TIME_US);

	return 0;
}

static int ak09940a_init(const struct device *dev)
{
	const struct ak09940a_config *cfg = dev->config;
	struct ak09940a_data *data = dev->data;
	uint8_t wia[2];
	int ret;

	ret = cfg->bus_io->check(&cfg->bus);
	if (ret != 0) {
		LOG_ERR("Bus not ready");
		return ret;
	}

	ret = ak09940a_reset(dev);
	if (ret != 0) {
		LOG_ERR("Failed to reset (%d)", ret);
		return ret;
	}

	/* Keep SPI traffic to other devices from being decoded as I2C */
	if (cfg->is_spi) {
		ret = ak09940a_write(dev, AK09940A_REG_I2CDIS, AK09940A_I2CDIS_DISABLE);
		if (ret != 0) {
			LOG_ERR("Failed to disable I2C interface (%d)", ret);
			return ret;
		}
	}

	ret = ak09940a_read(dev, AK09940A_REG_WIA1, wia, sizeof(wia));
	if (ret != 0) {
		LOG_ERR("Failed to read device ID (%d)", ret);
		return ret;
	}

	if ((wia[0] != AK099XX_WIA1_AKM) || (wia[1] != AK09940A_WIA2)) {
		LOG_ERR("Unexpected device ID 0x%02x%02x", wia[0], wia[1]);
		return -ENODEV;
	}

	ret = ak09940a_write(dev, AK09940A_REG_CNTL1, cfg->drive->cntl1);
	if (ret != 0) {
		LOG_ERR("Failed to set sensor drive (%d)", ret);
		return ret;
	}

	data->mode = AK099XX_MODE_POWER_DOWN;

	return 0;
}

static DEVICE_API(sensor, ak09940a_driver_api) = {
	.sample_fetch = ak09940a_sample_fetch,
	.channel_get = ak09940a_channel_get,
	.attr_set = ak09940a_attr_set,
	.attr_get = ak09940a_attr_get,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = ak09940a_submit,
	.get_decoder = ak09940a_get_decoder,
#endif
};

#define AK09940A_SPI_OPERATION                                                                     \
	(SPI_OP_MODE_CONTROLLER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) |                \
	 SPI_TRANSFER_MSB)

#define AK09940A_CONFIG_I2C(inst)                                                                  \
	.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)}, .bus_io = &ak09940a_bus_io_i2c, .is_spi = false,

#define AK09940A_CONFIG_SPI(inst)                                                                  \
	.bus = {.spi = SPI_DT_SPEC_INST_GET(inst, AK09940A_SPI_OPERATION)},                        \
	.bus_io = &ak09940a_bus_io_spi, .is_spi = true,

#define AK09940A_IODEV(inst)                                                                       \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                                     \
		    (SPI_DT_IODEV_DEFINE(ak09940a_iodev_##inst, DT_DRV_INST(inst),                 \
					 AK09940A_SPI_OPERATION)),                                 \
		    (I2C_DT_IODEV_DEFINE(ak09940a_iodev_##inst, DT_DRV_INST(inst))));              \
	RTIO_DEFINE(ak09940a_rtio_##inst, 8, 8);

#define AK09940A_DEFINE(inst)                                                                      \
	IF_ENABLED(CONFIG_SENSOR_ASYNC_API, (AK09940A_IODEV(inst)))                                \
                                                                                                   \
	static struct ak09940a_data ak09940a_data_##inst = {                                       \
		IF_ENABLED(CONFIG_SENSOR_ASYNC_API,                                                \
			   (.rtio_ctx = &ak09940a_rtio_##inst,                                     \
			    .iodev = &ak09940a_iodev_##inst,))                                     \
	};                                                                                         \
                                                                                                   \
	static const struct ak09940a_config ak09940a_config_##inst = {                             \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (AK09940A_CONFIG_SPI(inst)),                \
			    (AK09940A_CONFIG_I2C(inst)))                                           \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.drive = &ak09940a_drives[DT_INST_ENUM_IDX(inst, sensor_drive)],                   \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ak09940a_init, NULL, &ak09940a_data_##inst,             \
				     &ak09940a_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &ak09940a_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AK09940A_DEFINE)
