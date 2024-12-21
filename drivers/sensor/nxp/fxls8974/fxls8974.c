/*
 * Copyright 2024 NXP
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_fxls8974

#include "fxls8974.h"
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(FXLS8974, CONFIG_SENSOR_LOG_LEVEL);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define DIR_READ(a)  ((a) & 0x7f)
#define DIR_WRITE(a) ((a) | BIT(7))
#define ADDR_7(a) ((a) & BIT(7))

int fxls8974_transceive(const struct device *dev,
				void *data, size_t length)
{
		const struct fxls8974_config *cfg = dev->config;
		const struct spi_buf buf = { .buf = data, .len = length };
		const struct spi_buf_set s = { .bufs = &buf, .count = 1 };

		return spi_transceive_dt(&cfg->bus_cfg.spi, &s, &s);
}

int fxls8974_read_spi(const struct device *dev,
				uint8_t reg,
				void *data,
				size_t length)
{
		const struct fxls8974_config *cfg = dev->config;

		/* Reads must clock out a dummy byte after sending the address. */
		uint8_t reg_buf[3] = { DIR_READ(reg), ADDR_7(reg), 0 };
		const struct spi_buf buf[2] = {
				{ .buf = reg_buf, .len = 3 },
				{ .buf = data, .len = length }
		};
		const struct spi_buf_set tx = { .bufs = buf, .count = 1 };
		const struct spi_buf_set rx = { .bufs = buf, .count = 2 };

		return spi_transceive_dt(&cfg->bus_cfg.spi, &tx, &rx);
}

int fxls8974_byte_read_spi(const struct device *dev,
				uint8_t reg,
				uint8_t *byte)
{
		/* Reads must clock out a dummy byte after sending the address. */
		uint8_t data[] = { DIR_READ(reg), ADDR_7(reg), 0};
		int ret;

		ret = fxls8974_transceive(dev, data, sizeof(data));

		*byte = data[2];

		return ret;
}

int fxls8974_byte_write_spi(const struct device *dev,
				uint8_t reg,
				uint8_t byte)
{
		uint8_t data[] = { DIR_WRITE(reg), ADDR_7(reg), byte };

		return fxls8974_transceive(dev, data, sizeof(data));
}

int fxls8974_reg_field_update_spi(const struct device *dev,
				uint8_t reg,
				uint8_t mask,
				uint8_t val)
{
		uint8_t old_val;

		if (fxls8974_byte_read_spi(dev, reg, &old_val) < 0) {
			return -EIO;
		}

		return fxls8974_byte_write_spi(dev, reg, (old_val & ~mask) | (val & mask));
}

static const struct fxls8974_io_ops fxls8974_spi_ops = {
		.read = fxls8974_read_spi,
		.byte_read = fxls8974_byte_read_spi,
		.byte_write = fxls8974_byte_write_spi,
		.reg_field_update = fxls8974_reg_field_update_spi,
};
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
int fxls8974_read_i2c(const struct device *dev,
				uint8_t reg,
				void *data,
				size_t length)
{
		const struct fxls8974_config *cfg = dev->config;

		return i2c_burst_read_dt(&cfg->bus_cfg.i2c, reg, data, length);
}

int fxls8974_byte_read_i2c(const struct device *dev,
				uint8_t reg,
				uint8_t *byte)
{
		const struct fxls8974_config *cfg = dev->config;

		return i2c_reg_read_byte_dt(&cfg->bus_cfg.i2c, reg, byte);
}

int fxls8974_byte_write_i2c(const struct device *dev,
				uint8_t reg,
				uint8_t byte)
{
		const struct fxls8974_config *cfg = dev->config;

		return i2c_reg_write_byte_dt(&cfg->bus_cfg.i2c, reg, byte);
}

int fxls8974_reg_field_update_i2c(const struct device *dev,
				uint8_t reg,
				uint8_t mask,
				uint8_t val)
{
		const struct fxls8974_config *cfg = dev->config;

		return i2c_reg_update_byte_dt(&cfg->bus_cfg.i2c, reg, mask, val);
}
static const struct fxls8974_io_ops fxls8974_i2c_ops = {
		.read = fxls8974_read_i2c,
		.byte_read = fxls8974_byte_read_i2c,
		.byte_write = fxls8974_byte_write_i2c,
		.reg_field_update = fxls8974_reg_field_update_i2c,
};
#endif

static int fxls8974_set_odr(const struct device *dev,
				const struct sensor_value *val, enum fxls8974_wake mode)
{
		const struct fxls8974_config *cfg = dev->config;
		uint8_t odr;
		/* val int32 */
		switch (val->val1) {
		case 3200:
			odr = FXLS8974_CTRLREG3_ODR_RATE_3200;
			break;
		case 800:
			odr = FXLS8974_CTRLREG3_ODR_RATE_800;
			break;
		case 400:
			odr = FXLS8974_CTRLREG3_ODR_RATE_400;
			break;
		case 200:
			odr = FXLS8974_CTRLREG3_ODR_RATE_200;
			break;
		case 100:
			odr = FXLS8974_CTRLREG3_ODR_RATE_100;
			break;
		case 50:
			odr = FXLS8974_CTRLREG3_ODR_RATE_50;
			break;
		case 25:
			odr = FXLS8974_CTRLREG3_ODR_RATE_25;
			break;
		case 12:
			if (val->val2 == 500000) {
				odr = FXLS8974_CTRLREG3_ODR_RATE_12_5;
				break;
			}
			return -EINVAL;
		case 6:
			if (val->val2 == 250000) {
				odr = FXLS8974_CTRLREG3_ODR_RATE_6_25;
				break;
			}
			return -EINVAL;
		case 3:
			if (val->val2 == 125000) {
				odr = FXLS8974_CTRLREG3_ODR_RATE_3_125;
				break;
			}
			return -EINVAL;
		case 1:
			if (val->val2 == 563000) {
				odr = FXLS8974_CTRLREG3_ODR_RATE_1_563;
				break;
			}
			return -EINVAL;
		case 0:
			if (val->val2 == 781000) {
				odr = FXLS8974_CTRLREG3_ODR_RATE_0_781;
				break;
			}
			return -EINVAL;
		default:
			return -EINVAL;
		}

		LOG_DBG("Set %s ODR to 0x%02x", (mode == FXLS8974_WAKE) ? "wake" : "sleep", odr);

		/* Change the attribute and restore active mode. */
		if (mode == FXLS8974_WAKE) {
			return cfg->ops->reg_field_update(dev, FXLS8974_REG_CTRLREG3,
				FXLS8974_CTRLREG3_WAKE_ODR_MASK,
				odr<<4);
		} else {
			return cfg->ops->reg_field_update(dev, FXLS8974_REG_CTRLREG3,
				FXLS8974_CTRLREG3_SLEEP_ODR_MASK,
				odr);
		}
}

static int fxls8974_attr_set(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
		if (chan != SENSOR_CHAN_ALL) {
			return -ENOTSUP;
		}

		switch (attr) {
		case SENSOR_ATTR_SAMPLING_FREQUENCY:
			return fxls8974_set_odr(dev, val, FXLS8974_WAKE);
		default:
			return -ENOTSUP;
		}
		return 0;
}

static int fxls8974_sample_fetch(const struct device *dev, enum sensor_channel ch)
{
		const struct fxls8974_config *cfg = dev->config;
		struct fxls8974_data *data = dev->data;
		uint8_t buf[FXLS8974_MAX_NUM_BYTES];
		int16_t *raw;
		int ret = 0;
		int i;

		k_sem_take(&data->sem, K_FOREVER);

		/* Read all the accel channels in one I2C/SPI transaction. */
		if (cfg->ops->read(dev, FXLS8974_REG_OUTXLSB, buf, FXLS8974_MAX_ACCEL_BYTES)) {
			LOG_ERR("Could not fetch accelerometer data");
			ret = -EIO;
			goto exit;
		}

		if (cfg->ops->byte_read(dev, FXLS8974_REG_OUTTEMP, buf+FXLS8974_DATA_TEMP_OFFSET)) {
			LOG_ERR("Could not fetch temperature");
			ret = -EIO;
			goto exit;
		}

		/* Parse the buf into raw channel data (16-bit integers). To save
		 * RAM, store the data in raw format and wait to convert to the
		 * normalized sensor_value type until later.
		 */
		__ASSERT(FXLS8974_MAX_NUM_CHANNELS <= ARRAY_SIZE(data->raw),
			"Too many channels");

		raw = &data->raw[FXLS8974_CHANNEL_ACCEL_X];

		for (i = 0; i < FXLS8974_MAX_ACCEL_BYTES; i += 2) {
			*raw++ = (buf[i+1] << 8) | (buf[i]);
		}

		*raw = *(buf+FXLS8974_MAX_ACCEL_BYTES);

exit:
		k_sem_give(&data->sem);

		return ret;
}

static void fxls8974_accel_convert(struct sensor_value *val, int16_t raw,
				uint8_t fsr)
{
		int64_t micro_ms2;

		/* Convert units to micro m/s^2. */
		micro_ms2 = (raw * SENSOR_G) >> fsr;

		/* The maximum possible value is 16g, which in units of micro m/s^2
		 * always fits into 32-bits. Cast down to int32_t so we can use a
		 * faster divide.
		 */
		val->val1 = (int32_t) micro_ms2 / 1000000;
		val->val2 = (int32_t) micro_ms2 % 1000000;
}

static int fxls8974_get_accel_data(const struct device *dev,
		struct sensor_value *val, enum sensor_channel ch)
{
		const struct fxls8974_config *cfg = dev->config;
		struct fxls8974_data *data = dev->data;
		int16_t *raw;
		uint8_t fsr;

		k_sem_take(&data->sem, K_FOREVER);

		if (cfg->ops->byte_read(dev, FXLS8974_REG_CTRLREG1, &fsr)) {
			LOG_ERR("Could not read scale settings");
			return -EIO;
		}

		fsr = (fsr & FXLS8974_CTRLREG1_FSR_MASK) >> 1;
		switch (fsr) {
		case 0b00:
			fsr = 10U;
			break;
		case 0b01:
			fsr = 9U;
			break;
		case 0b10:
			fsr = 8U;
			break;
		case 0b11:
			fsr = 7U;
			break;
		}

		if (ch == SENSOR_CHAN_ACCEL_XYZ) {
			raw = &data->raw[FXLS8974_CHANNEL_ACCEL_X];
			for (int i = 0; i < FXLS8974_MAX_ACCEL_CHANNELS; i++) {
				fxls8974_accel_convert(val++, *raw++, fsr);
			}
		} else {
			switch (ch) {
			case SENSOR_CHAN_ACCEL_X:
				raw = &data->raw[FXLS8974_CHANNEL_ACCEL_X];
				break;
			case SENSOR_CHAN_ACCEL_Y:
				raw = &data->raw[FXLS8974_CHANNEL_ACCEL_Y];
				break;
			case SENSOR_CHAN_ACCEL_Z:
				raw = &data->raw[FXLS8974_CHANNEL_ACCEL_Z];
				break;
			default:
				return -ENOTSUP;
			}
			fxls8974_accel_convert(val, *raw, fsr);
		}
		k_sem_give(&data->sem);

		return 0;
}

static int fxls8974_get_temp_data(const struct device *dev, struct sensor_value *val)
{
		struct fxls8974_data *data = dev->data;
		int16_t *raw;

		k_sem_take(&data->sem, K_FOREVER);
		raw = &data->raw[FXLS8974_CHANNEL_TEMP];
		val->val1 = *raw+FXLS8974_ZERO_TEMP;
		k_sem_give(&data->sem);

		return 0;
}

static int fxls8974_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{

		switch (chan) {
		case SENSOR_CHAN_ALL:
			if (fxls8974_get_accel_data(dev, val, SENSOR_CHAN_ACCEL_XYZ)) {
				return -EIO;
			}

			val += FXLS8974_MAX_ACCEL_CHANNELS;

			if (fxls8974_get_temp_data(dev, val)) {
				return -EIO;
			}
			break;
		case SENSOR_CHAN_ACCEL_XYZ:
			return fxls8974_get_accel_data(dev, val, SENSOR_CHAN_ACCEL_XYZ);
		case SENSOR_CHAN_ACCEL_X:
			__fallthrough;
		case SENSOR_CHAN_ACCEL_Y:
			__fallthrough;
		case SENSOR_CHAN_ACCEL_Z:
			return fxls8974_get_accel_data(dev, val, chan);
		case SENSOR_CHAN_AMBIENT_TEMP:
			return fxls8974_get_temp_data(dev, val);
		default:
			LOG_ERR("Unsupported channel");
			return -ENOTSUP;
		}

		return 0;
}

int fxls8974_get_active(const struct device *dev, enum fxls8974_active *active)
{
		const struct fxls8974_config *cfg = dev->config;
		uint8_t val;

		if (cfg->ops->byte_read(dev, FXLS8974_REG_CTRLREG1, &val)) {
			LOG_ERR("Could not get active setting");
			return -EIO;
		}
		val &= FXLS8974_CTRLREG1_ACTIVE_MASK;

		*active = val;

		return 0;
}

int fxls8974_set_active(const struct device *dev, enum fxls8974_active active)
{
		const struct fxls8974_config *cfg = dev->config;

		return cfg->ops->reg_field_update(dev, FXLS8974_REG_CTRLREG1,
			FXLS8974_CTRLREG1_ACTIVE_MASK, active);
}

static void fxls8974_print_config(const struct device *dev)
{
		const struct fxls8974_config *cfg = dev->config;
		uint8_t regVal[5];

		if (cfg->ops->read(dev, FXLS8974_REG_CTRLREG1, regVal, 5)) {
			LOG_ERR("Failed to read config registers");
		}
		LOG_DBG("Current config:\n\r"
			"CFG: 0x%02x CFG2: 0x%02x CFG3: 0x%02x CFG4: 0x%02x CFG5: 0x%02x",
			regVal[0], regVal[1], regVal[2], regVal[3], regVal[4]);
}

static int fxls8974_init(const struct device *dev)
{
		const struct fxls8974_config *cfg = dev->config;
		struct fxls8974_data *data = dev->data;
		struct sensor_value odr = {.val1 = 6, .val2 = 250000};
		uint8_t regVal;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c_spec = cfg->bus_cfg.i2c;

		if (cfg->inst_on_bus == FXLS8974_BUS_I2C) {
			if (!i2c_is_ready_dt(&i2c_spec)) {
				LOG_ERR("I2C bus device not ready");
				return -ENODEV;
			}
		}
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi_spec = cfg->bus_cfg.spi;

		if (cfg->inst_on_bus == FXLS8974_BUS_SPI) {
			if (!spi_is_ready_dt(&spi_spec)) {
				LOG_ERR("SPI bus device not ready");
				return -ENODEV;
			}
		}
#endif

		/* Software reset the sensor. Upon issuing a software
		 * reset command over the I2C interface, the sensor
		 * immediately resets and does not send any
		 * acknowledgment (ACK) of the written byte to the
		 * master. Therefore, do not check the return code of
		 * the I2C transaction.
		 */
		cfg->ops->byte_write(dev, FXLS8974_REG_CTRLREG1,
						FXLS8974_CTRLREG1_RST_MASK);

		/* The sensor requires us to wait 1 ms after a reset before
		 * attempting further communications.
		 */
		k_busy_wait(USEC_PER_MSEC);

		/*
		 * Read the WHOAMI register to make sure we are talking to FXLS8974 or
		 * compatible device and not some other type of device that happens to
		 * have the same I2C address.
		 */
		if (cfg->ops->byte_read(dev, FXLS8974_REG_WHOAMI,
			&data->whoami)) {
			LOG_ERR("Could not get WHOAMI value");
			return -EIO;
		}

		if (data->whoami == WHOAMI_ID_FXLS8974) {
			LOG_DBG("Device ID 0x%x, FXLS8974", data->whoami);
		} else {
			LOG_ERR("Unknown Device ID 0x%x", data->whoami);
			return -EIO;
		}

		if (fxls8974_get_active(dev, (enum fxls8974_active *)&regVal)) {
			LOG_ERR("Failed to set standby mode");
			return -EIO;
		}

		if (regVal != FXLS8974_ACTIVE_OFF) {
			LOG_ERR("Not in standby mode");
			return -EIO;
		}

		if (cfg->ops->byte_write(dev, FXLS8974_REG_CTRLREG4,
			FXLS8974_CTRLREG4_INT_POL_HIGH)) {
			LOG_ERR("Could not set up register 4");
			return -EIO;
		}
		if (cfg->ops->byte_read(dev, FXLS8974_REG_CTRLREG4, &regVal)) {
			LOG_ERR("Could not get CTRL_REG4 value");
			return -EIO;
		}

		if (regVal != FXLS8974_CTRLREG4_INT_POL_HIGH) {
			LOG_ERR("CTRLREG4 is not set up properly");
			return -EIO;
		}

		if (fxls8974_set_odr(dev, &odr, FXLS8974_WAKE)) {
			LOG_ERR("Could not set default data rate");
			return -EIO;
		}

		/* Set the +-2G mode */
		if (cfg->ops->byte_write(dev, FXLS8974_REG_CTRLREG1,
			FXLS8974_CTRLREG1_FSR_2G)) {
			LOG_ERR("Could not set range");
			return -EIO;
		}

		if (cfg->ops->byte_read(dev, FXLS8974_REG_CTRLREG1,
			&regVal)) {
			LOG_ERR("Could not ret CTRL_REG1 value");
			return -EIO;
		}

		if ((regVal & FXLS8974_CTRLREG1_FSR_MASK) != FXLS8974_CTRLREG1_FSR_2G) {
			LOG_ERR("Wrong range selected!");
			return -EIO;
		}

		k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

#if CONFIG_FXLS8974_TRIGGER
		if (fxls8974_trigger_init(dev)) {
			LOG_ERR("Could not initialize interrupts");
			return -EIO;
		}
#endif

		if (fxls8974_set_active(dev, FXLS8974_ACTIVE_ON)) {
			LOG_ERR("Could not set active mode");
			return -EIO;
		}

		if (fxls8974_get_active(dev, (enum fxls8974_active *)&regVal)) {
			LOG_ERR("Failed to get active mode");
			return -EIO;
		}

		if (regVal != FXLS8974_ACTIVE_ON) {
			LOG_ERR("Not in active mode");
			return -EIO;
		}

		fxls8974_print_config(dev);
		k_sem_give(&data->sem);

		LOG_DBG("Init complete");

		return 0;
}

static const struct sensor_driver_api fxls8974_driver_api = {
		.sample_fetch = fxls8974_sample_fetch,
		.channel_get = fxls8974_channel_get,
		.attr_set = fxls8974_attr_set,
#if CONFIG_FXLS8974_TRIGGER
		.trigger_set = fxls8974_trigger_set,
#endif
};

#define FXLS8974_CONFIG_I2C(n)									\
		.bus_cfg = { .i2c = I2C_DT_SPEC_INST_GET(n) },					\
		.ops = &fxls8974_i2c_ops,							\
		.range = DT_INST_PROP(n, range),						\
		.inst_on_bus = FXLS8974_BUS_I2C,

#define FXLS8974_CONFIG_SPI(n)									\
		.bus_cfg = { .spi = SPI_DT_SPEC_INST_GET(n,					\
						SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0) },	\
						.ops = &fxls8974_spi_ops,			\
						.range = DT_INST_PROP(n, range),		\
						.inst_on_bus = FXLS8974_BUS_SPI,		\

#define FXLS8974_SPI_OPERATION (SPI_WORD_SET(8) |	\
				SPI_OP_MODE_MASTER)	\

#define FXLS8974_INTM_PROPS(n, m)			\
	.int_gpio = GPIO_DT_SPEC_INST_GET(n, int##m##_gpios),

#define FXLS8974_INT_PROPS(n)				\
	COND_CODE_1(CONFIG_FXLS8974_DRDY_INT1,		\
			(FXLS8974_INTM_PROPS(n, 1)),	\
			(FXLS8974_INTM_PROPS(n, 2)))

#define FXLS8974_INT(n)					\
	COND_CODE_1(CONFIG_FXLS8974_TRIGGER,		\
			(FXLS8974_INT_PROPS(n)),	\
			())

#define FXLS8974_INIT(n)									\
		static const struct fxls8974_config fxls8974_config_##n = {			\
				COND_CODE_1(DT_INST_ON_BUS(n, spi),				\
				(FXLS8974_CONFIG_SPI(n)),					\
				(FXLS8974_CONFIG_I2C(n)))					\
				FXLS8974_INT(n)							\
		};										\
		\
		static struct fxls8974_data fxls8974_data_##n;					\
		\
		SENSOR_DEVICE_DT_INST_DEFINE(n,							\
						fxls8974_init,					\
						NULL,						\
						&fxls8974_data_##n,				\
						&fxls8974_config_##n,				\
						POST_KERNEL,					\
						CONFIG_SENSOR_INIT_PRIORITY,			\
						&fxls8974_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FXLS8974_INIT)
