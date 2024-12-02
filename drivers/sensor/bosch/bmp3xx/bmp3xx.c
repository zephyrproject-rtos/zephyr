/* Bosch BMP388 / BMP390 pressure sensor
 *
 * Copyright (c) 2020 Facebook, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/pm/device.h>

#include "bmp3xx.h"

LOG_MODULE_REGISTER(BMP3XX, CONFIG_SENSOR_LOG_LEVEL);

enum chipset_id {
	BMP388_ID = 0x50,
	BMP390_ID = 0x60,
};

#if defined(CONFIG_BMP3XX_ODR_RUNTIME)
static const struct {
	uint16_t freq_int;
	uint16_t freq_milli;
} bmp3xx_odr_map[] = {
	{0, 3},	    /* 25/8192 - 327.68s */
	{0, 6},	    /* 25/4096 - 163.84s */
	{0, 12},    /* 25/2048 - 81.92s */
	{0, 24},    /* 25/1024 - 40.96s */
	{0, 49},    /* 25/512 - 20.48s */
	{0, 98},    /* 25/256 - 10.24s */
	{0, 195},   /* 25/128 - 5.12s */
	{0, 391},   /* 25/64 - 2.56s */
	{0, 781},   /* 25/32 - 1.28s */
	{1, 563},   /* 25/16 - 640ms */
	{3, 125},   /* 25/8 - 320ms */
	{6, 250},   /* 25/4 - 160ms */
	{12, 500},  /* 25/2 - 80ms */
	{25, 0},    /* 25 - 40ms */
	{50, 0},    /* 50 - 20ms */
	{100, 0},   /* 100 - 10ms */
	{200, 0},   /* 200 - 5ms */
};
#endif

static inline int bmp3xx_bus_check(const struct device *dev)
{
	const struct bmp3xx_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static inline int bmp3xx_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size)
{
	const struct bmp3xx_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, start, buf, size);
}

static inline int bmp3xx_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct bmp3xx_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, reg, val);
}

int bmp3xx_reg_field_update(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
	int rc = 0;
	uint8_t old_value, new_value;
	const struct bmp3xx_config *cfg = dev->config;

	rc = cfg->bus_io->read(&cfg->bus, reg, &old_value, 1);
	if (rc != 0) {
		return rc;
	}

	new_value = (old_value & ~mask) | (val & mask);
	if (new_value == old_value) {
		return 0;
	}

	return cfg->bus_io->write(&cfg->bus, reg, new_value);
}

#ifdef CONFIG_BMP3XX_ODR_RUNTIME
static int bmp3xx_freq_to_odr_val(uint16_t freq_int, uint16_t freq_milli)
{
	size_t i;

	/* An ODR of 0 Hz is not allowed */
	if (freq_int == 0U && freq_milli == 0U) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(bmp3xx_odr_map); i++) {
		if (freq_int < bmp3xx_odr_map[i].freq_int ||
		    (freq_int == bmp3xx_odr_map[i].freq_int &&
		     freq_milli <= bmp3xx_odr_map[i].freq_milli)) {
			return (ARRAY_SIZE(bmp3xx_odr_map) - 1) - i;
		}
	}

	return -EINVAL;
}

static int bmp3xx_attr_set_odr(const struct device *dev, uint16_t freq_int, uint16_t freq_milli)
{
	int err;
	struct bmp3xx_data *data = dev->data;
	int odr = bmp3xx_freq_to_odr_val(freq_int, freq_milli);

	if (odr < 0) {
		return odr;
	}

	err = bmp3xx_reg_field_update(dev, BMP3XX_REG_ODR, BMP3XX_ODR_MASK, (uint8_t)odr);
	if (err == 0) {
		data->odr = odr;
	}

	return err;
}
#endif

#ifdef CONFIG_BMP3XX_OSR_RUNTIME
static int bmp3xx_attr_set_oversampling(const struct device *dev, enum sensor_channel chan,
					int16_t val)
{
	uint8_t reg_val = 0;
	uint32_t pos, mask;
	int err;

	struct bmp3xx_data *data = dev->data;

	/* Value must be a positive power of 2 <= 32. */
	if ((val <= 0) || (val > 32) || ((val & (val - 1)) != 0)) {
		return -EINVAL;
	}

	if (chan == SENSOR_CHAN_PRESS) {
		pos = BMP3XX_OSR_PRESSURE_POS;
		mask = BMP3XX_OSR_PRESSURE_MASK;
	} else if ((chan == SENSOR_CHAN_AMBIENT_TEMP) || (chan == SENSOR_CHAN_DIE_TEMP)) {
		pos = BMP3XX_OSR_TEMP_POS;
		mask = BMP3XX_OSR_TEMP_MASK;
	} else {
		return -EINVAL;
	}

	/* Determine exponent: this corresponds to register setting. */
	while ((val % 2) == 0) {
		val >>= 1;
		++reg_val;
	}

	err = bmp3xx_reg_field_update(dev, BMP3XX_REG_OSR, mask, reg_val << pos);
	if (err < 0) {
		return err;
	}

	/* Store for future use in converting RAW values. */
	if (chan == SENSOR_CHAN_PRESS) {
		data->osr_pressure = reg_val;
	} else {
		data->osr_temp = reg_val;
	}

	return err;
}
#endif

static int bmp3xx_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	int ret;

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return -EBUSY;
	}
#endif

	switch (attr) {
#ifdef CONFIG_BMP3XX_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = bmp3xx_attr_set_odr(dev, val->val1, val->val2 / 1000);
		break;
#endif

#ifdef CONFIG_BMP3XX_OSR_RUNTIME
	case SENSOR_ATTR_OVERSAMPLING:
		ret = bmp3xx_attr_set_oversampling(dev, chan, val->val1);
		break;
#endif

	default:
		ret = -EINVAL;
	}

	return ret;
}

static int bmp3xx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bmp3xx_data *bmp3xx = dev->data;
	uint8_t raw[BMP3XX_SAMPLE_BUFFER_SIZE];
	int ret = 0;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return -EBUSY;
	}
#endif

	pm_device_busy_set(dev);

	/* Wait for status to indicate that data is ready. */
	raw[0] = 0U;
	while ((raw[0] & BMP3XX_STATUS_DRDY_PRESS) == 0U) {
		ret = bmp3xx_reg_read(dev, BMP3XX_REG_STATUS, raw, 1);
		if (ret < 0) {
			goto error;
		}
	}

	ret = bmp3xx_reg_read(dev, BMP3XX_REG_DATA0, raw, BMP3XX_SAMPLE_BUFFER_SIZE);
	if (ret < 0) {
		goto error;
	}

	/* convert samples to 32bit values */
	bmp3xx->sample.press = sys_get_le24(&raw[0]);
	bmp3xx->sample.raw_temp = sys_get_le24(&raw[3]);
	bmp3xx->sample.comp_temp = 0;

error:
	pm_device_busy_clear(dev);
	return ret;
}

static void bmp3xx_compensate_temp(struct bmp3xx_data *data)
{
	/* Adapted from:
	 * https://github.com/BoschSensortec/BMP3-Sensor-API/blob/master/bmp3.c
	 */

	int64_t partial_data1;
	int64_t partial_data2;
	int64_t partial_data3;
	int64_t partial_data4;
	int64_t partial_data5;

	struct bmp3xx_cal_data *cal = &data->cal;

	partial_data1 = ((int64_t)data->sample.raw_temp - (256 * cal->t1));
	partial_data2 = cal->t2 * partial_data1;
	partial_data3 = (partial_data1 * partial_data1);
	partial_data4 = (int64_t)partial_data3 * cal->t3;
	partial_data5 = ((int64_t)(partial_data2 * 262144) + partial_data4);

	/* Store for pressure calculation */
	data->sample.comp_temp = partial_data5 / 4294967296;
}

static int bmp3xx_temp_channel_get(const struct device *dev, struct sensor_value *val)
{
	struct bmp3xx_data *data = dev->data;

	if (data->sample.comp_temp == 0) {
		bmp3xx_compensate_temp(data);
	}

	int64_t tmp = (data->sample.comp_temp * 250000) / 16384;

	val->val1 = tmp / 1000000;
	val->val2 = tmp % 1000000;

	return 0;
}

static uint64_t bmp3xx_compensate_press(struct bmp3xx_data *data)
{
	/* Adapted from:
	 * https://github.com/BoschSensortec/BMP3-Sensor-API/blob/master/bmp3.c
	 */

	int64_t partial_data1;
	int64_t partial_data2;
	int64_t partial_data3;
	int64_t partial_data4;
	int64_t partial_data5;
	int64_t partial_data6;
	int64_t offset;
	int64_t sensitivity;
	uint64_t comp_press;

	struct bmp3xx_cal_data *cal = &data->cal;

	int64_t t_lin = data->sample.comp_temp;
	uint32_t raw_pressure = data->sample.press;

	partial_data1 = t_lin * t_lin;
	partial_data2 = partial_data1 / 64;
	partial_data3 = (partial_data2 * t_lin) / 256;
	partial_data4 = (cal->p8 * partial_data3) / 32;
	partial_data5 = (cal->p7 * partial_data1) * 16;
	partial_data6 = (cal->p6 * t_lin) * 4194304;
	offset = (cal->p5 * 140737488355328) + partial_data4 + partial_data5 + partial_data6;
	partial_data2 = (cal->p4 * partial_data3) / 32;
	partial_data4 = (cal->p3 * partial_data1) * 4;
	partial_data5 = (cal->p2 - 16384) * t_lin * 2097152;
	sensitivity = ((cal->p1 - 16384) * 70368744177664) + partial_data2 + partial_data4 +
		      partial_data5;
	partial_data1 = (sensitivity / 16777216) * raw_pressure;
	partial_data2 = cal->p10 * t_lin;
	partial_data3 = partial_data2 + (65536 * cal->p9);
	partial_data4 = (partial_data3 * raw_pressure) / 8192;
	/* Dividing by 10 followed by multiplying by 10 to avoid overflow caused
	 * (raw_pressure * partial_data4)
	 */
	partial_data5 = (raw_pressure * (partial_data4 / 10)) / 512;
	partial_data5 = partial_data5 * 10;
	partial_data6 = ((int64_t)raw_pressure * (int64_t)raw_pressure);
	partial_data2 = (cal->p11 * partial_data6) / 65536;
	partial_data3 = (partial_data2 * raw_pressure) / 128;
	partial_data4 = (offset / 4) + partial_data1 + partial_data5 + partial_data3;

	comp_press = (((uint64_t)partial_data4 * 25) / (uint64_t)1099511627776);

	/* returned value is in hundredths of Pa. */
	return comp_press;
}

static int bmp3xx_press_channel_get(const struct device *dev, struct sensor_value *val)
{
	struct bmp3xx_data *data = dev->data;

	if (data->sample.comp_temp == 0) {
		bmp3xx_compensate_temp(data);
	}

	uint64_t tmp = bmp3xx_compensate_press(data);

	/* tmp is in hundredths of Pa. Convert to kPa as specified in sensor
	 * interface.
	 */
	val->val1 = tmp / 100000;
	val->val2 = (tmp % 100000) * 10;

	return 0;
}

static int bmp3xx_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_PRESS:
		bmp3xx_press_channel_get(dev, val);
		break;

	case SENSOR_CHAN_DIE_TEMP:
	case SENSOR_CHAN_AMBIENT_TEMP:
		bmp3xx_temp_channel_get(dev, val);
		break;

	default:
		LOG_DBG("Channel not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int bmp3xx_get_calibration_data(const struct device *dev)
{
	struct bmp3xx_data *data = dev->data;
	struct bmp3xx_cal_data *cal = &data->cal;

	if (bmp3xx_reg_read(dev, BMP3XX_REG_CALIB0, (uint8_t *)cal, sizeof(*cal)) < 0) {
		return -EIO;
	}

	cal->t1 = sys_le16_to_cpu(cal->t1);
	cal->t2 = sys_le16_to_cpu(cal->t2);
	cal->p1 = (int16_t)sys_le16_to_cpu(cal->p1);
	cal->p2 = (int16_t)sys_le16_to_cpu(cal->p2);
	cal->p5 = sys_le16_to_cpu(cal->p5);
	cal->p6 = sys_le16_to_cpu(cal->p6);
	cal->p9 = (int16_t)sys_le16_to_cpu(cal->p9);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int bmp3xx_pm_action(const struct device *dev, enum pm_device_action action)
{
	uint8_t reg_val;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		reg_val = BMP3XX_PWR_CTRL_MODE_NORMAL;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		reg_val = BMP3XX_PWR_CTRL_MODE_SLEEP;
		break;
	default:
		return -ENOTSUP;
	}

	if (bmp3xx_reg_field_update(dev, BMP3XX_REG_PWR_CTRL, BMP3XX_PWR_CTRL_MODE_MASK, reg_val) <
	    0) {
		LOG_DBG("Failed to set power mode.");
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static const struct sensor_driver_api bmp3xx_api = {
	.attr_set = bmp3xx_attr_set,
#ifdef CONFIG_BMP3XX_TRIGGER
	.trigger_set = bmp3xx_trigger_set,
#endif
	.sample_fetch = bmp3xx_sample_fetch,
	.channel_get = bmp3xx_channel_get,
};

static int bmp3xx_init(const struct device *dev)
{
	struct bmp3xx_data *bmp3xx = dev->data;
	const struct bmp3xx_config *cfg = dev->config;
	uint8_t val = 0U;

	if (bmp3xx_bus_check(dev) < 0) {
		LOG_DBG("bus check failed");
		return -ENODEV;
	}

	/* reboot the chip */
	if (bmp3xx_reg_write(dev, BMP3XX_REG_CMD, BMP3XX_CMD_SOFT_RESET) < 0) {
		LOG_ERR("Cannot reboot chip.");
		return -EIO;
	}

	k_busy_wait(2000);

	if (bmp3xx_reg_read(dev, BMP3XX_REG_CHIPID, &val, 1) < 0) {
		LOG_ERR("Failed to read chip id.");
		return -EIO;
	}

	if (val != bmp3xx->chip_id) {
		LOG_ERR("Unsupported chip detected (0x%x)!", val);
		return -ENODEV;
	}

	/* Read calibration data */
	if (bmp3xx_get_calibration_data(dev) < 0) {
		LOG_ERR("Failed to read calibration data.");
		return -EIO;
	}

	/* Set ODR */
	if (bmp3xx_reg_field_update(dev, BMP3XX_REG_ODR, BMP3XX_ODR_MASK, bmp3xx->odr) < 0) {
		LOG_ERR("Failed to set ODR.");
		return -EIO;
	}

	/* Set OSR */
	val = (bmp3xx->osr_pressure << BMP3XX_OSR_PRESSURE_POS);
	val |= (bmp3xx->osr_temp << BMP3XX_OSR_TEMP_POS);
	if (bmp3xx_reg_write(dev, BMP3XX_REG_OSR, val) < 0) {
		LOG_ERR("Failed to set OSR.");
		return -EIO;
	}

	/* Set IIR filter coefficient */
	val = (cfg->iir_filter << BMP3XX_IIR_FILTER_POS) & BMP3XX_IIR_FILTER_MASK;
	if (bmp3xx_reg_write(dev, BMP3XX_REG_CONFIG, val) < 0) {
		LOG_ERR("Failed to set IIR coefficient.");
		return -EIO;
	}

	/* Enable sensors and normal mode*/
	if (bmp3xx_reg_write(dev, BMP3XX_REG_PWR_CTRL, BMP3XX_PWR_CTRL_ON) < 0) {
		LOG_ERR("Failed to enable sensors.");
		return -EIO;
	}

	/* Read error register */
	if (bmp3xx_reg_read(dev, BMP3XX_REG_ERR_REG, &val, 1) < 0) {
		LOG_ERR("Failed get sensors error register.");
		return -EIO;
	}

	/* OSR and ODR config not proper */
	if (val & BMP3XX_STATUS_CONF_ERR) {
		LOG_ERR("OSR and ODR configuration is not proper");
		return -EINVAL;
	}

#ifdef CONFIG_BMP3XX_TRIGGER
	if (cfg->gpio_int.port != NULL && bmp3xx_trigger_mode_init(dev) < 0) {
		LOG_ERR("Cannot set up trigger mode.");
		return -EINVAL;
	}
#endif

	return 0;
}

/* Initializes a struct bmp3xx_config for an instance on a SPI bus. */
#define BMP3XX_CONFIG_SPI(inst)				\
	.bus.spi = SPI_DT_SPEC_INST_GET(inst, BMP3XX_SPI_OPERATION, 0),	\
	.bus_io = &bmp3xx_bus_io_spi,

/* Initializes a struct bmp3xx_config for an instance on an I2C bus. */
#define BMP3XX_CONFIG_I2C(inst)			       \
	.bus.i2c = I2C_DT_SPEC_INST_GET(inst),	       \
	.bus_io = &bmp3xx_bus_io_i2c,

#define BMP3XX_BUS_CFG(inst)			\
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c),	\
		    (BMP3XX_CONFIG_I2C(inst)),	\
		    (BMP3XX_CONFIG_SPI(inst)))

#if defined(CONFIG_BMP3XX_TRIGGER)
#define BMP3XX_INT_CFG(inst) \
	.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),
#else
#define BMP3XX_INT_CFG(inst)
#endif

#define BMP3XX_INST(inst, chipid)\
	static struct bmp3xx_data bmp3xx_data_##inst##chipid = {\
		.odr = DT_INST_ENUM_IDX(inst, odr),\
		.osr_pressure = DT_INST_ENUM_IDX(inst, osr_press),\
		.osr_temp = DT_INST_ENUM_IDX(inst, osr_temp),\
		.chip_id = chipid,\
	};\
	static const struct bmp3xx_config bmp3xx_config_##inst##chipid = {\
		BMP3XX_BUS_CFG(inst)\
		BMP3XX_INT_CFG(inst)\
		.iir_filter = DT_INST_ENUM_IDX(inst, iir_filter),\
	};\
	PM_DEVICE_DT_INST_DEFINE(inst, bmp3xx_pm_action);\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bmp3xx_init, PM_DEVICE_DT_INST_GET(inst),\
				    &bmp3xx_data_##inst##chipid, &bmp3xx_config_##inst##chipid,\
					POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &bmp3xx_api);

#define DT_DRV_COMPAT bosch_bmp388
DT_INST_FOREACH_STATUS_OKAY_VARGS(BMP3XX_INST, BMP388_ID)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT bosch_bmp390
DT_INST_FOREACH_STATUS_OKAY_VARGS(BMP3XX_INST, BMP390_ID)
#undef DT_DRV_COMPAT
