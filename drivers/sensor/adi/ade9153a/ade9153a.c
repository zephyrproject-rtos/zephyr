/*
 * Copyright (c) 2024 Plentify (Pty) Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ade9153a

#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>

#include "ade9153a.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ade9153a, CONFIG_SENSOR_LOG_LEVEL);

/* Register operations.
 * The bit[3] = 0 indicates a write action and[3] = 1 indicates a read
 */
#define ADE9153A_READ_REG  BIT(3)
#define ADE9153A_WRITE_REG (0)
#define RETURN_ON_ERR(_err)                                                                        \
	if (_err != 0) {                                                                           \
		return _err;                                                                       \
	}

static int ade9153a_reg_access(const struct device *dev, uint8_t cmd, uint16_t reg_addr, void *data,
			       size_t length)
{
	const struct ade9153a_config *cfg = dev->config;

	/* The bit [3] = 0 indicates a write action and [3] = 1 indicates a read.
	 * When reading the cmd value is 8 == BIT(3), when writing cmd is 0
	 */
	uint16_t access = (((uint16_t)(reg_addr << 4U) & 0xFFF0U) + cmd);

	access = (access << 8) | ((access >> 8) & 0x00FF);

	const struct spi_buf buf[2] = {{.buf = &access, .len = sizeof(access)},
				       {.buf = data, .len = length}};
	struct spi_buf_set tx = {
		.buffers = buf,
	};

	if (cmd == ADE9153A_READ_REG) {
		const struct spi_buf_set rx = {.buffers = buf, .count = 2};

		tx.count = 1;

		return spi_transceive_dt(&cfg->spi_dt_spec, &tx, &rx);
	}

	tx.count = 2;

	return spi_write_dt(&cfg->spi_dt_spec, &tx);
}

static inline int ade9153a_get_reg16(const struct device *dev, uint16_t reg_addr, uint16_t *data)
{
	uint8_t data_raw[2] = {0};

	int err = ade9153a_reg_access(dev, ADE9153A_READ_REG, reg_addr, data_raw, 2U);

	if (err) {
		return err;
	}

	uint16_t data_msb = (((uint16_t)data_raw[0]) << 8U) + (uint16_t)data_raw[1];

	*data = data_msb;

	return 0;
}

static inline int ade9153a_get_reg32(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	uint8_t data_raw[4] = {0};

	int err = ade9153a_reg_access(dev, ADE9153A_READ_REG, reg_addr, data_raw, 4U);

	if (err) {
		return err;
	}

	uint32_t data_msb = (((uint32_t)data_raw[0]) << 24U) + (((uint32_t)data_raw[1]) << 16U) +
			    (((uint32_t)data_raw[2]) << 8U) + (uint32_t)(data_raw[3]);

	*data = data_msb;

	return 0;
}
static inline int ade9153a_set_reg16(const struct device *dev, uint16_t reg_addr, uint16_t data)
{
	uint8_t data_raw[2] = {(uint8_t)data >> 8U, (uint8_t)data};

	return ade9153a_reg_access(dev, ADE9153A_WRITE_REG, reg_addr, data_raw, 2U);
}

static inline int ade9153a_set_reg32(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	uint8_t data_raw[4] = {(uint8_t)data >> 24U, (uint8_t)data >> 16U, (uint8_t)data >> 8U,
			       (uint8_t)data};

	return ade9153a_reg_access(dev, ADE9153A_WRITE_REG, reg_addr, data_raw, 4U);
}

int start_acal(const struct device *dev, uint32_t reg_ms_acal_cfg)
{
	int attempts = 0;
	uint32_t ms_status_current_reg;

	ade9153a_get_reg32(dev, ADE9153A_REG_MS_STATUS_CURRENT, &ms_status_current_reg);

	while ((ms_status_current_reg & 0x00000001) == 0) {
		ade9153a_get_reg32(dev, ADE9153A_REG_MS_STATUS_CURRENT, &ms_status_current_reg);
		if (attempts > 15) {
			return -EBUSY;
		}
		attempts++;
		k_msleep(100);
	}

	ade9153a_set_reg32(dev, ADE9153A_REG_MS_ACAL_CFG, reg_ms_acal_cfg); /* turbo */

	return 0;
}

void stop_acal(const struct device *dev)
{
	ade9153a_set_reg32(dev, ADE9153A_REG_MS_ACAL_CFG, 0x00000000);
}

void apply_acal(const struct device *dev, double AICC, double AVCC)
{
	int err;
	int32_t AIGAIN;
	int32_t AVGAIN;

	AIGAIN = (AICC / (CAL_IRMS_CC * 1000) - 1) * 134217728;
	AVGAIN = (AVCC / (CAL_VRMS_CC * 1000) - 1) * 134217728;

	LOG_DBG("AIGAIN: %d", AIGAIN);
	err = ade9153a_set_reg32(dev, ADE9153A_REG_AIGAIN, AIGAIN);
	__ASSERT(err == 0, "Could not set AIGAIN register. Err=%d", AIGAIN);

	LOG_DBG("AVGAIN: %d", AVGAIN);
	err = ade9153a_set_reg32(dev, ADE9153A_REG_AVGAIN, AVGAIN);
	__ASSERT(err == 0, "Could not set AVGAIN register. Err=%d", AIGAIN);
}

int __weak ade9153a_start_autocalibration(const struct device *dev)
{
	LOG_DBG("Autocalibrating Current Channel:");
	start_acal(dev, 0x00000017); /* AITurbo */

	uint32_t cert;

	for (int i = 0, iterations = 10; i < iterations; ++i) {
		ade9153a_get_reg32(dev, ADE9153A_REG_MS_ACAL_AICERT, &cert);
		LOG_DBG("[%d/%d] AICERT: %d.%d %%", i, iterations, cert / 10000, cert % 10000);
		k_msleep(CONFIG_ADE9153A_AI_TURBO_CAL_TIME / iterations);
	}

	stop_acal(dev);

	LOG_DBG("Autocalibrating Voltage Channel:");
	start_acal(dev, 0x00000043); /* AVTurbo */

	for (int i = 0, iterations = 100; i < iterations; ++i) {
		ade9153a_get_reg32(dev, ADE9153A_REG_MS_ACAL_AVCERT, &cert);
		LOG_DBG("[%d/%d] AVCERT: %d.%d %%", i, iterations, cert / 10000, cert % 10000);
		k_msleep(CONFIG_ADE9153A_AV_TURBO_CAL_TIME / iterations);
	}

	stop_acal(dev);

	double aicc, avcc;
	uint32_t temp_reg;

	ade9153a_get_reg32(dev, ADE9153A_REG_MS_ACAL_AICC, &temp_reg); /* turbo */

	aicc = temp_reg / (double)2048;

	ade9153a_get_reg32(dev, ADE9153A_REG_MS_ACAL_AVCC, &temp_reg); /* turbo */

	avcc = temp_reg / (double)2048;

	apply_acal(dev, aicc, avcc);

	LOG_DBG("Autocalibration...[ok]");
	k_msleep(100);

	return 0;
}

static int ade9153a_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct ade9153a_data *data = dev->data;
	double double_value;

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP: {
		uint16_t gain;
		uint16_t offset;

		gain = (data->temperature_trim & 0xFFFF);           /* Extract 16 LSB */
		offset = ((data->temperature_trim >> 16) & 0xFFFF); /* Extract 16 MSB */
		double_value = ((double)offset / 32.00) -
			       ((double)data->temperature_reg * (double)gain / (double)131072);
		break;
	}
	case SENSOR_CHAN_AC_ACTIVE_ENERGY: {
		/* Energy in mWhr */
		double_value = (double)data->active_energy_reg * CAL_ENERGY_CC / 1000;

		break;
	}
	case SENSOR_CHAN_AC_FUNDAMENTAL_REACTIVE_ENERGY: {
		double_value = (double)data->fund_reactive_energy_reg * CAL_ENERGY_CC / 1000;
		break;
	}
	case SENSOR_CHAN_AC_APPARENT_ENERGY: {
		double_value = (double)data->apparent_energy_reg * CAL_ENERGY_CC / 1000;
		break;
	}
	case SENSOR_CHAN_AC_ACTIVE_POWER: {
		double_value = (double)data->active_power_reg * CAL_POWER_CC / 1000;
		break;
	}
	case SENSOR_CHAN_AC_FUNDAMENTAL_REACTIVE_POWER: {
		double_value = (double)data->fund_reactive_power_reg * CAL_POWER_CC / 1000;
		break;
	}
	case SENSOR_CHAN_AC_APPARENT_POWER: {
		double_value = (double)data->apparent_power_reg * CAL_POWER_CC / 1000;
		break;
	}
	case SENSOR_CHAN_AC_CURRENT_RMS: {
		/* RMS in mA */
		double_value = (double)data->current_rms_reg * CAL_IRMS_CC / 1000;
		break;
	}
	case SENSOR_CHAN_AC_HALF_CURRENT_RMS: {
		/* RMS in mV */
		double_value = (double)data->half_current_rms_reg * CAL_IRMS_CC / 1000;
		break;
	}
	case SENSOR_CHAN_AC_VOLTAGE_RMS: {
		/* Half-RMS in mV */
		double_value = (double)data->voltage_rms_reg * CAL_VRMS_CC / 1000;
		break;
	}
	case SENSOR_CHAN_AC_HALF_VOLTAGE_RMS: {
		/* Half-RMS in mV */
		double_value = (double)data->half_voltage_rms_reg * CAL_VRMS_CC / 1000;
		break;
	}
	case SENSOR_CHAN_AC_POWER_FACTOR: {
		/* Calculate PF */
		double_value = (double)data->power_factor_reg / (double)134217728;
		break;
	}
	case SENSOR_CHAN_AC_PERIOD: {
		/* Calculate Line Period */
		double_value = (double)(data->period_reg + 1) / (double)(4000 * 65536);
		break;
	}
	case SENSOR_CHAN_AC_FREQUENCY: {
		/* Calculate Frequency in cHz*/
		double_value = (4000LU * 65536 * 100) / (double)(data->period_reg + 1);
		break;
	}
	case SENSOR_CHAN_AC_ANGLE: {
		double mulConstant;

		if ((data->acc_mode_reg & 0x0010) > 0) {
			mulConstant = 0.02109375; /* multiplier constant for 60Hz system */
		} else {
			mulConstant = 0.017578125; /* multiplier constant for 50Hz system */
		}

		/* Calculate Angle in degrees */
		double_value = data->angle_reg_av_ai_reg * mulConstant;
		break;
	}
	default:
		return -ENOTSUP;
	}

	sensor_value_from_double(val, double_value);

	return 0;
}

static int ade9153a_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	uint32_t attribute = attr;

	switch (attribute) {
	case SENSOR_ATTR_ADE9153A_REGISTER: {
		union ade9153a_register ade = {.as_sensor_value = *val};

		if (ade.size == sizeof(uint16_t)) {
			LOG_DBG("Data to write %X", (uint16_t)ade.value);
			ade9153a_set_reg16(dev, ade.addr, (uint16_t)ade.value);
		} else {
			LOG_DBG("Data to write %X", ade.value);
			ade9153a_set_reg32(dev, ade.addr, ade.value);
		}
		break;
	}
	case SENSOR_ATTR_ADE9153A_START_AUTOCALIBRATION:
		ade9153a_start_autocalibration(dev);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ade9153a_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);
	__ASSERT_NO_MSG(attr == SENSOR_ATTR_ADE9153A_REGISTER);

	union ade9153a_register ade = {.as_sensor_value = *val};

	if (ade.size == sizeof(uint16_t)) {
		uint16_t u16_value;

		ade9153a_get_reg16(dev, ade.addr, &u16_value);

		val->val1 = u16_value;
	} else if (ade.size == sizeof(uint32_t)) {
		ade9153a_get_reg32(dev, ade.addr, &(val->val1));
	} else {
		return -EINVAL;
	}

	return 0;
}

static int ade9153a_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	struct ade9153a_data *data = dev->data;

	RETURN_ON_ERR(ade9153a_get_reg32(dev, ADE9153A_REG_STATUS, (void *)&data->status_reg));

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_DIE_TEMP: {
		RETURN_ON_ERR(
			ade9153a_get_reg32(dev, ADE9153A_REG_TEMP_TRIM, &data->temperature_trim));

		/* Starting temperature reading */
		RETURN_ON_ERR(
			ade9153a_set_reg16(dev, ADE9153A_REG_TEMP_CFG, CONFIG_ADE9153A_TEMP_CFG));

		k_msleep(10);
		/* SENSOR_CHAN_DIE_TEMP */
		RETURN_ON_ERR(
			ade9153a_get_reg16(dev, ADE9153A_REG_TEMP_RSLT, &data->temperature_reg));

		if (chan != SENSOR_CHAN_ALL) {
			break;
		}
	}
	case SENSOR_CHAN_AC_ACTIVE_ENERGY:
	case SENSOR_CHAN_AC_FUNDAMENTAL_REACTIVE_ENERGY:
	case SENSOR_CHAN_AC_APPARENT_ENERGY:
		/* SENSOR_CHAN_AC_ACTIVE_ENERGY */
		RETURN_ON_ERR(
			ade9153a_get_reg32(dev, ADE9153A_REG_AWATTHR_HI, &data->active_energy_reg));

		/* SENSOR_CHAN_AC_FUNDAMENTAL_REACTIVE_ENERGY */
		RETURN_ON_ERR(ade9153a_get_reg32(dev, ADE9153A_REG_AFVARHR_HI,
						 &data->fund_reactive_energy_reg));

		/* SENSOR_CHAN_AC_APPARENT_ENERGY */
		RETURN_ON_ERR(
			ade9153a_get_reg32(dev, ADE9153A_REG_AVAHR_HI, &data->apparent_energy_reg));

		if (chan != SENSOR_CHAN_ALL) {
			break;
		}
	case SENSOR_CHAN_AC_ACTIVE_POWER:
	case SENSOR_CHAN_AC_FUNDAMENTAL_REACTIVE_POWER:
	case SENSOR_CHAN_AC_APPARENT_POWER:
		/* SENSOR_CHAN_AC_ACTIVE_POWER */
		RETURN_ON_ERR(ade9153a_get_reg32(dev, ADE9153A_REG_AWATT, &data->active_power_reg));

		/* SENSOR_CHAN_AC_FUNDAMENTAL_REACTIVE_POWER */
		RETURN_ON_ERR(ade9153a_get_reg32(dev, ADE9153A_REG_AFVAR,
						 &data->fund_reactive_power_reg));

		/* SENSOR_CHAN_AC_APPARENT_POWER */
		RETURN_ON_ERR(ade9153a_get_reg32(dev, ADE9153A_REG_AVA, &data->apparent_power_reg));

		if (chan != SENSOR_CHAN_ALL) {
			break;
		}
	case SENSOR_CHAN_AC_CURRENT_RMS:
	case SENSOR_CHAN_AC_VOLTAGE_RMS:
		/* SENSOR_CHAN_AC_CURRENT_RMS */
		RETURN_ON_ERR(ade9153a_get_reg32(dev, ADE9153A_REG_AIRMS, &data->current_rms_reg));

		/* SENSOR_CHAN_AC_VOLTAGE_RMS */
		RETURN_ON_ERR(ade9153a_get_reg32(dev, ADE9153A_REG_AVRMS, &data->voltage_rms_reg));

		if (chan != SENSOR_CHAN_ALL) {
			break;
		}
	case SENSOR_CHAN_AC_HALF_VOLTAGE_RMS:
	case SENSOR_CHAN_AC_HALF_CURRENT_RMS:
		/* SENSOR_CHAN_AC_HALF_CURRENT_RMS */
		RETURN_ON_ERR(ade9153a_get_reg32(dev, ADE9153A_REG_AIRMS_OC,
						 &data->half_current_rms_reg));
		/* SENSOR_CHAN_AC_HALF_VOLTAGE_RMS */
		RETURN_ON_ERR(ade9153a_get_reg32(dev, ADE9153A_REG_AVRMS_OC,
						 &data->half_voltage_rms_reg));

		if (chan != SENSOR_CHAN_ALL) {
			break;
		}
	case SENSOR_CHAN_AC_POWER_FACTOR:
	case SENSOR_CHAN_AC_PERIOD:
	case SENSOR_CHAN_AC_FREQUENCY:
	case SENSOR_CHAN_AC_ANGLE:
		/* SENSOR_CHAN_AC_POWER_FACTOR */
		RETURN_ON_ERR(ade9153a_get_reg32(dev, ADE9153A_REG_APF, &data->power_factor_reg));
		RETURN_ON_ERR(ade9153a_get_reg16(dev, ADE9153A_REG_ACCMODE, &data->acc_mode_reg));

		/* SENSOR_CHAN_AC_PERIOD */
		/* SENSOR_CHAN_AC_FREQUENCY */
		RETURN_ON_ERR(ade9153a_get_reg32(dev, ADE9153A_REG_APERIOD, &data->period_reg));

		/* SENSOR_CHAN_AC_ANGLE */
		RETURN_ON_ERR(ade9153a_get_reg32(dev, ADE9153A_REG_ANGL_AV_AI,
						 &data->angle_reg_av_ai_reg));

		break;

	default:
		return -ENOTSUP;
	}
	return 0;
}

static const struct sensor_driver_api ade9153a_driver_api = {
	.attr_set = ade9153a_attr_set,
	.attr_get = ade9153a_attr_get,
	.sample_fetch = ade9153a_sample_fetch,
	.channel_get = ade9153a_channel_get,
	IF_ENABLED(CONFIG_ADE9153A_TRIGGER, (.trigger_set = ade9153a_trigger_set))};

static void ade9153a_reset(const struct device *dev)
{
	const struct ade9153a_config *cfg = dev->config;

	gpio_pin_set_dt(&cfg->reset_gpio_dt_spec, 1);

	k_msleep(CONFIG_ADE9153A_RESET_ACTIVE_TIME);

	gpio_pin_set_dt(&cfg->reset_gpio_dt_spec, 0);

	k_msleep(CONFIG_ADE9153A_POST_RESET_DELAY);

	LOG_DBG("Reset Done");
}

static int ade9153a_probe(const struct device *dev)
{
	ade9153a_set_reg16(dev, ADE9153A_REG_RUN, CONFIG_ADE9153A_RUN_ON);

	k_msleep(100);

	uint32_t reg_value = 0;

	ade9153a_get_reg32(dev, ADE9153A_REG_VERSION_PRODUCT, &reg_value);

	if (reg_value != 0x9153a) {
		return -ENODEV;
	}

	LOG_DBG("Communication attempt...[ok]");
	return 0;
}

int __weak ade9153a_setup(const struct device *dev)
{
	ade9153a_set_reg16(dev, ADE9153A_REG_AI_PGAGAIN, CONFIG_ADE9153A_AI_PGAGAIN);

	ade9153a_set_reg32(dev, ADE9153A_REG_CONFIG0, CONFIG_ADE9153A_CONFIG0);

	ade9153a_set_reg16(dev, ADE9153A_REG_CONFIG1, CONFIG_ADE9153A_CONFIG1);

	ade9153a_set_reg16(dev, ADE9153A_REG_CONFIG2, CONFIG_ADE9153A_CONFIG2);

	ade9153a_set_reg16(dev, ADE9153A_REG_CONFIG3, CONFIG_ADE9153A_CONFIG3);

	ade9153a_set_reg16(dev, ADE9153A_REG_ACCMODE, CONFIG_ADE9153A_ACCMODE);

	ade9153a_set_reg32(dev, ADE9153A_REG_VLEVEL, CONFIG_ADE9153A_VLEVEL);

	ade9153a_set_reg16(dev, ADE9153A_REG_ZX_CFG, CONFIG_ADE9153A_ZX_CFG);

	ade9153a_set_reg32(dev, ADE9153A_REG_MASK, CONFIG_ADE9153A_MASK);

	ade9153a_set_reg32(dev, ADE9153A_REG_ACT_NL_LVL, CONFIG_ADE9153A_ACT_NL_LVL);

	ade9153a_set_reg32(dev, ADE9153A_REG_REACT_NL_LVL, CONFIG_ADE9153A_REACT_NL_LVL);

	ade9153a_set_reg32(dev, ADE9153A_REG_APP_NL_LVL, CONFIG_ADE9153A_APP_NL_LVL);

	ade9153a_set_reg16(dev, ADE9153A_REG_COMPMODE, CONFIG_ADE9153A_COMPMODE);

	ade9153a_set_reg32(dev, ADE9153A_REG_VDIV_RSMALL, CONFIG_ADE9153A_VDIV_RSMALL);

	ade9153a_set_reg16(dev, ADE9153A_REG_EP_CFG, CONFIG_ADE9153A_EP_CFG);

	ade9153a_set_reg16(dev, ADE9153A_REG_EGY_TIME, CONFIG_ADE9153A_EGY_TIME);

	LOG_DBG("Initial setup...[ok]");

	return 0;
}

static int ade9153a_init(const struct device *dev)
{
	const struct ade9153a_config *cfg = dev->config;

	if (!device_is_ready(cfg->spi_dt_spec.bus)) {
		LOG_DBG("Bus device is not ready");
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&cfg->reset_gpio_dt_spec)) {
		LOG_DBG("%s: device %s is not ready", dev->name,
			cfg->reset_gpio_dt_spec.port->name);
		return -ENODEV;
	}

	gpio_pin_configure_dt(&cfg->reset_gpio_dt_spec,
			      GPIO_OUTPUT | cfg->reset_gpio_dt_spec.dt_flags);

	gpio_pin_set_dt(&cfg->reset_gpio_dt_spec, 0);

	ade9153a_reset(dev);

	if (ade9153a_probe(dev) < 0) {
		return -ENODEV;
	}

#if defined(CONFIG_ADE9153A_SETUP_ON_STARTUP)
	ade9153a_setup(dev);
#endif /* CONFIG_ADE9153A_SETUP_ON_STARTUP */

#if defined(CONFIG_ADE9153A_ACAL_ON_STARTUP)
	ade9153a_start_autocalibration(dev);
#endif /* CONFIG_ADE9153A_ACAL_ON_STARTUP */

#if defined(CONFIG_ADE9153A_TRIGGER)
	ade9153a_init_interrupt(dev);
#endif /* CONFIG_ADE9153A_TRIGGER */

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "ADE9153A driver enabled without any devices"
#endif

/* clang-format off */
#define ADE9153A_DEFINE(inst)                                                                  \
	static struct ade9153a_data ade9153a_data_##inst;                                      \
                                                                                               \
	static const struct ade9153a_config ade9153a_config_##inst = {                         \
		.spi_dt_spec = SPI_DT_SPEC_INST_GET(                                           \
			inst, (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8)), 0),   \
		.reset_gpio_dt_spec = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),        \
	IF_ENABLED(CONFIG_ADE9153A_TRIGGER, (                                                  \
		.irq_gpio_dt_spec = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, {0}),            \
		.cf_gpio_dt_spec = GPIO_DT_SPEC_INST_GET_OR(inst, cf_gpios, {0}),              \
		)                                                                              \
	)};                                                                                    \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ade9153a_init, NULL, &ade9153a_data_##inst,         \
				     &ade9153a_config_##inst, POST_KERNEL,                     \
				     CONFIG_SENSOR_INIT_PRIORITY, &ade9153a_driver_api);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(ADE9153A_DEFINE)
