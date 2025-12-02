/*
 * Copyright (c) 2022 Badgerd Technologies B.V.
 * Copyright (c) 2023 Metratec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bmp581.h"
#include "bmp581_bus.h"
#include "bmp581_decoder.h"
#include "bmp581_stream.h"

#include <math.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>

LOG_MODULE_REGISTER(bmp581, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "BMP581 driver enabled without any devices"
#endif

static int power_up_check(const struct device *dev);
static int get_nvm_status(uint8_t *nvm_status, const struct device *dev);
static int get_interrupt_status(uint8_t *int_status, const struct device *dev);
static int validate_chip_id(struct bmp581_data *drv);
static int get_osr_odr_press_config(struct bmp581_osr_odr_press_config *osr_odr_press_cfg,
				    const struct device *dev);
static int set_osr_config(const struct sensor_value *osr, enum sensor_channel chan,
			  const struct device *dev);
static int set_odr_config(const struct sensor_value *odr, const struct device *dev);
static int soft_reset(const struct device *dev);
static int set_iir_config(const struct sensor_value *iir, const struct device *dev);
static int get_power_mode(enum bmp5_powermode *powermode, const struct device *dev);
static int set_power_mode(enum bmp5_powermode powermode, const struct device *dev);

static int set_power_mode(enum bmp5_powermode powermode, const struct device *dev)
{
	struct bmp581_config *conf = (struct bmp581_config *)dev->config;
	int ret = BMP5_OK;
	uint8_t odr = 0;
	enum bmp5_powermode current_powermode;

	CHECKIF(dev == NULL) {
		return -EINVAL;
	}

	ret = get_power_mode(&current_powermode, dev);
	if (ret != BMP5_OK) {
		LOG_ERR("Couldnt set the power mode because something went wrong when getting the "
			"current power mode.");
		return ret;
	}

	if (current_powermode != BMP5_POWERMODE_STANDBY) {
		/*
		 * Device should be set to standby before transitioning to forced mode or normal
		 * mode or continuous mode.
		 */
		ret = bmp581_reg_read_rtio(&conf->bus, BMP5_REG_ODR_CONFIG, &odr, 1);
		if (ret == BMP5_OK) {
			/* Setting deep_dis = 1(BMP5_DEEP_DISABLED) disables the deep standby mode
			 */
			odr = BMP5_SET_BITSLICE(odr, BMP5_DEEP_DISABLE, BMP5_DEEP_DISABLED);
			odr = BMP5_SET_BITS_POS_0(odr, BMP5_POWERMODE, BMP5_POWERMODE_STANDBY);
			ret = bmp581_reg_write_rtio(&conf->bus, BMP5_REG_ODR_CONFIG, &odr, 1);

			if (ret != BMP5_OK) {
				LOG_DBG("Failed to set power mode to BMP5_POWERMODE_STANDBY.");
				return ret;
			}
		}
	}

	/* lets update the power mode */
	switch (powermode) {
	case BMP5_POWERMODE_STANDBY:
		/* this change is already done so we can just return */
		ret = BMP5_OK;
		break;
	case BMP5_POWERMODE_DEEP_STANDBY:
		LOG_DBG("Setting power mode to DEEP STANDBY is not supported, current power mode "
			"is BMP5_POWERMODE_STANDBY.");
		ret = -ENOTSUP;
		break;
	case BMP5_POWERMODE_NORMAL:
	case BMP5_POWERMODE_FORCED:
	case BMP5_POWERMODE_CONTINUOUS:
		odr = BMP5_SET_BITSLICE(odr, BMP5_DEEP_DISABLE, BMP5_DEEP_DISABLED);
		odr = BMP5_SET_BITS_POS_0(odr, BMP5_POWERMODE, powermode);
		ret = bmp581_reg_write_rtio(&conf->bus, BMP5_REG_ODR_CONFIG, &odr, 1);
		break;
	default:
		/* invalid power mode */
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int get_power_mode(enum bmp5_powermode *powermode, const struct device *dev)
{
	const struct bmp581_config *conf;
	int ret = BMP5_OK;

	CHECKIF(powermode == NULL || dev == NULL) {
		return -EINVAL;
	}

	conf = (const struct bmp581_config *)dev->config;
	uint8_t reg = 0;
	uint8_t raw_power_mode = 0;

	ret = bmp581_reg_read_rtio(&conf->bus, BMP5_REG_ODR_CONFIG, &reg, 1);
	if (ret != BMP5_OK) {
		LOG_DBG("Failed to read odr config to get power mode!");
		return ret;
	}

	raw_power_mode = BMP5_GET_BITS_POS_0(reg, BMP5_POWERMODE);

	switch (raw_power_mode) {
	case BMP5_POWERMODE_STANDBY: {
		/* Getting deep disable status */
		uint8_t deep_dis = BMP5_GET_BITSLICE(reg, BMP5_DEEP_DISABLE);

		/* Checking deepstandby status only when powermode is in standby mode */

		/* If deep_dis = 0(BMP5_DEEP_ENABLED) then deepstandby mode is enabled.
		 * If deep_dis = 1(BMP5_DEEP_DISABLED) then deepstandby mode is disabled
		 */
		if (deep_dis == BMP5_DEEP_ENABLED) {
			/* TODO: check if it is really deep standby */
			*powermode = BMP5_POWERMODE_DEEP_STANDBY;
		} else {
			*powermode = BMP5_POWERMODE_STANDBY;
		}

		break;
	}
	case BMP5_POWERMODE_NORMAL:
		*powermode = BMP5_POWERMODE_NORMAL;
		break;
	case BMP5_POWERMODE_FORCED:
		*powermode = BMP5_POWERMODE_FORCED;
		break;
	case BMP5_POWERMODE_CONTINUOUS:
		*powermode = BMP5_POWERMODE_CONTINUOUS;
		break;
	default:
		/* invalid power mode */
		ret = -EINVAL;
		LOG_DBG("Something went wrong invalid powermode!");
		break;
	}

	return ret;
}

static int power_up_check(const struct device *dev)
{
	int8_t rslt = 0;
	uint8_t nvm_status = 0;

	CHECKIF(dev == NULL) {
		return -EINVAL;
	}

	rslt = get_nvm_status(&nvm_status, dev);

	if (rslt == BMP5_OK) {
		/* Check if nvm_rdy status = 1 and nvm_err status = 0 to proceed */
		if ((nvm_status & BMP5_INT_NVM_RDY) != 0 && (nvm_status & BMP5_INT_NVM_ERR) == 0) {
			rslt = BMP5_OK;
		} else {
			rslt = -EFAULT;
		}
	}

	return rslt;
}

static int get_interrupt_status(uint8_t *int_status, const struct device *dev)
{
	const struct bmp581_config *conf;

	CHECKIF(int_status == NULL || dev == NULL) {
		return -EINVAL;
	}

	conf = (const struct bmp581_config *)dev->config;

	return bmp581_reg_read_rtio(&conf->bus, BMP5_REG_INT_STATUS, int_status, 1);
}

static int get_nvm_status(uint8_t *nvm_status, const struct device *dev)
{
	const struct bmp581_config *conf;

	CHECKIF(nvm_status == NULL || dev == NULL) {
		return -EINVAL;
	}

	conf = (const struct bmp581_config *)dev->config;

	return bmp581_reg_read_rtio(&conf->bus, BMP5_REG_STATUS, nvm_status, 1);
}

static int validate_chip_id(struct bmp581_data *drv)
{
	int8_t rslt = 0;

	CHECKIF(drv == NULL) {
		return -EINVAL;
	}

	if (drv->chip_id == BMP5_CHIP_ID_PRIM || drv->chip_id == BMP5_CHIP_ID_SEC) {
		rslt = BMP5_OK;
	} else {
		drv->chip_id = 0;
		rslt = -ENODEV;
	}

	return rslt;
}

/*!
 *  This API gets the configuration for oversampling of temperature, oversampling of
 *  pressure and ODR configuration along with pressure enable.
 */
static int get_osr_odr_press_config(struct bmp581_osr_odr_press_config *osr_odr_press_cfg,
				    const struct device *dev)
{
	const struct bmp581_config *conf;

	/* Variable to store the function result */
	int8_t rslt = 0;

	/* Variable to store OSR and ODR config */
	uint8_t reg_data[2] = {0};

	CHECKIF(osr_odr_press_cfg == NULL || dev == NULL) {
		return -EINVAL;
	}

	conf = (const struct bmp581_config *)dev->config;

	/* Get OSR and ODR configuration in burst read */
	rslt = bmp581_reg_read_rtio(&conf->bus, BMP5_REG_OSR_CONFIG, reg_data, 2);

	if (rslt == BMP5_OK) {
		osr_odr_press_cfg->osr_t = BMP5_GET_BITS_POS_0(reg_data[0], BMP5_TEMP_OSR);
		osr_odr_press_cfg->osr_p = BMP5_GET_BITSLICE(reg_data[0], BMP5_PRESS_OSR);
		osr_odr_press_cfg->press_en = BMP5_GET_BITSLICE(reg_data[0], BMP5_PRESS_EN);
	}

	return rslt;
}

static int set_osr_odr_press_config(const struct bmp581_osr_odr_press_config *osr_odr_press_cfg,
				    const struct device *dev)
{
	const struct bmp581_config *cfg = (const struct bmp581_config *)dev->config;
	uint8_t reg_data[2] = {0};

	reg_data[0] = BMP5_SET_BITSLICE(reg_data[0], BMP5_TEMP_OSR, osr_odr_press_cfg->osr_t);
	reg_data[0] = BMP5_SET_BITSLICE(reg_data[0], BMP5_PRESS_OSR, osr_odr_press_cfg->osr_p);
	reg_data[0] = BMP5_SET_BITSLICE(reg_data[0], BMP5_PRESS_EN, osr_odr_press_cfg->press_en);

	reg_data[1] = BMP5_SET_BITSLICE(reg_data[1], BMP5_POWERMODE, osr_odr_press_cfg->power_mode);
	reg_data[1] = BMP5_SET_BITSLICE(reg_data[1], BMP5_ODR, osr_odr_press_cfg->odr);

	return bmp581_reg_write_rtio(&cfg->bus, BMP5_REG_OSR_CONFIG, reg_data, sizeof(reg_data));
}

static int set_iir_filters_config(const struct bmp581_osr_odr_press_config *osr_odr_press_cfg,
				  const struct device *dev)
{
	const struct bmp581_config *cfg = (const struct bmp581_config *)dev->config;
	uint8_t reg_data = 0;

	reg_data = BMP5_SET_BITSLICE(reg_data, BMP5_SET_IIR_TEMP, osr_odr_press_cfg->iir_t);
	reg_data = BMP5_SET_BITSLICE(reg_data, BMP5_SET_IIR_PRESS, osr_odr_press_cfg->iir_p);

	return bmp581_reg_write_rtio(&cfg->bus, BMP5_REG_DSP_IIR, &reg_data, 1);
}

static int set_osr_config(const struct sensor_value *osr, enum sensor_channel chan,
			  const struct device *dev)
{
	CHECKIF(osr == NULL || dev == NULL) {
		return -EINVAL;
	}

	struct bmp581_data *drv = (struct bmp581_data *)dev->data;
	struct bmp581_config *conf = (struct bmp581_config *)dev->config;
	int ret = 0;

	uint8_t oversampling = osr->val1;
	uint8_t press_en = osr->val2 != 0; /* if it is not 0 then pressure is enabled */
	uint8_t osr_val = 0;

	ret = bmp581_reg_read_rtio(&conf->bus, BMP5_REG_OSR_CONFIG, &osr_val, 1);
	if (ret == BMP5_OK) {
		switch (chan) {
		case SENSOR_CHAN_ALL:
			osr_val = BMP5_SET_BITS_POS_0(osr_val, BMP5_TEMP_OSR, oversampling);
			osr_val = BMP5_SET_BITSLICE(osr_val, BMP5_PRESS_OSR, oversampling);
			osr_val = BMP5_SET_BITSLICE(osr_val, BMP5_PRESS_EN, press_en);
			break;
		case SENSOR_CHAN_PRESS:
			osr_val = BMP5_SET_BITSLICE(osr_val, BMP5_PRESS_OSR, oversampling);
			osr_val = BMP5_SET_BITSLICE(osr_val, BMP5_PRESS_EN, press_en);
			break;
		case SENSOR_CHAN_AMBIENT_TEMP:
			osr_val = BMP5_SET_BITS_POS_0(osr_val, BMP5_TEMP_OSR, oversampling);
			break;
		default:
			ret = -ENOTSUP;
			break;
		}

		if (ret == BMP5_OK) {
			ret = bmp581_reg_write_rtio(&conf->bus, BMP5_REG_OSR_CONFIG, &osr_val, 1);
			get_osr_odr_press_config(&drv->osr_odr_press_config, dev);
		}
	}

	return ret;
}

static int set_odr_config(const struct sensor_value *odr, const struct device *dev)
{
	CHECKIF(odr == NULL || dev == NULL) {
		return -EINVAL;
	}

	struct bmp581_data *drv = (struct bmp581_data *)dev->data;
	struct bmp581_config *conf = (struct bmp581_config *)dev->config;
	int ret = 0;
	uint8_t odr_val = 0;

	ret = bmp581_reg_read_rtio(&conf->bus, BMP5_REG_ODR_CONFIG, &odr_val, 1);
	if (ret != BMP5_OK) {
		return ret;
	}
	odr_val = BMP5_SET_BITSLICE(odr_val, BMP5_ODR, odr->val1);
	ret = bmp581_reg_write_rtio(&conf->bus, BMP5_REG_ODR_CONFIG, &odr_val, 1);
	get_osr_odr_press_config(&drv->osr_odr_press_config, dev);

	return ret;
}

static int soft_reset(const struct device *dev)
{
	struct bmp581_config *conf = (struct bmp581_config *)dev->config;
	int ret = 0;
	const uint8_t reset_cmd = BMP5_SOFT_RESET_CMD;
	uint8_t int_status = 0;

	CHECKIF(dev == NULL) {
		return -EINVAL;
	}

	ret = bmp581_reg_write_rtio(&conf->bus, BMP5_REG_CMD, &reset_cmd, 1);

	if (ret == BMP5_OK) {
		k_usleep(BMP5_DELAY_US_SOFT_RESET);
		ret = get_interrupt_status(&int_status, dev);
		if (ret == BMP5_OK) {
			if ((int_status & BMP5_INT_ASSERTED_POR_SOFTRESET_COMPLETE) != 0) {
				ret = BMP5_OK;
			} else {
				ret = -EFAULT;
			}
		}
	} else {
		LOG_DBG("Failed perform soft-reset.");
	}

	return ret;
}

static int bmp581_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	CHECKIF(dev == NULL) {
		return -EINVAL;
	}

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	struct bmp581_data *drv = (struct bmp581_data *)dev->data;
	struct bmp581_config *conf = (struct bmp581_config *)dev->config;
	uint8_t data[6];
	int ret = 0;

	ret = bmp581_reg_read_rtio(&conf->bus, BMP5_REG_TEMP_DATA_XLSB, data, 6);
	if (ret == BMP5_OK) {
		/* convert raw sensor data to sensor_value. Shift the decimal part by 1 decimal
		 * place to compensate for the conversion in sensor_value_to_double()
		 */
		drv->last_sample.temperature.val1 = data[2];
		drv->last_sample.temperature.val2 = (data[1] << 8 | data[0]) * 10;

		if (drv->osr_odr_press_config.press_en == BMP5_ENABLE) {
			/* convert raw sensor data to sensor_value. Shift the decimal part by
			 * 4 decimal places to compensate for the conversion in
			 * sensor_value_to_double()
			 */
			uint32_t raw_pressure = (uint32_t)((uint32_t)(data[5] << 16) |
							   (uint16_t)(data[4] << 8) | data[3]) >> 6;
			drv->last_sample.pressure.val1 = raw_pressure / 1000;
			drv->last_sample.pressure.val2 = (raw_pressure % 1000) * 1000;
		} else {
			drv->last_sample.pressure.val1 = 0;
			drv->last_sample.pressure.val2 = 0;
		}
	}

	return ret;
}

static int bmp581_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	CHECKIF(dev == NULL || val == NULL) {
		return -EINVAL;
	}

	struct bmp581_data *drv = (struct bmp581_data *)dev->data;

	switch (chan) {
	case SENSOR_CHAN_PRESS:
		/* returns pressure in kPa */
		*val = drv->last_sample.pressure;
		return BMP5_OK;
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* returns temperature in Celsius */
		*val = drv->last_sample.temperature;
		return BMP5_OK;
	default:
		return -ENOTSUP;
	}
}

static int set_iir_config(const struct sensor_value *iir, const struct device *dev)
{
	struct bmp581_config *conf = (struct bmp581_config *)dev->config;
	int ret = BMP5_OK;

	CHECKIF(iir == NULL || dev == NULL) {
		return -EINVAL;
	}

	/* Variable to store existing powermode */
	enum bmp5_powermode prev_powermode;

	ret = get_power_mode(&prev_powermode, dev);
	if (ret != BMP5_OK) {
		LOG_DBG("Not able to get current power mode.");
		return ret;
	}
	/* IIR configuration is writable only during STANDBY mode(as per datasheet) */
	set_power_mode(BMP5_POWERMODE_STANDBY, dev);

	/* update IIR config */
	uint8_t dsp_config[2];

	ret = bmp581_reg_read_rtio(&conf->bus, BMP5_REG_DSP_CONFIG, dsp_config, 2);
	if (ret != BMP5_OK) {
		LOG_DBG("Failed to read dsp config register.");
		return ret;
	}
	/* Put IIR filtered values in data registers */
	dsp_config[0] = BMP5_SET_BITSLICE(dsp_config[0], BMP5_SHDW_SET_IIR_TEMP, BMP5_ENABLE);
	dsp_config[0] = BMP5_SET_BITSLICE(dsp_config[0], BMP5_SHDW_SET_IIR_PRESS, BMP5_ENABLE);

	/* Configure IIR filter */
	dsp_config[1] = iir->val1;
	dsp_config[1] = BMP5_SET_BITSLICE(dsp_config[1], BMP5_SET_IIR_PRESS, iir->val2);

	/* Set IIR configuration */
	ret = bmp581_reg_write_rtio(&conf->bus, BMP5_REG_DSP_CONFIG, dsp_config, 2);

	if (ret != BMP5_OK) {
		LOG_DBG("Failed to configure IIR filter.");
		return ret;
	}

	/* Restore previous power mode if it is not standby already */
	if (prev_powermode != BMP5_POWERMODE_STANDBY) {
		ret = set_power_mode(prev_powermode, dev);
	}

	return ret;
}

static int bmp581_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	CHECKIF(dev == NULL || val == NULL) {
		return -EINVAL;
	}

	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = set_odr_config(val, dev);
		break;
	case SENSOR_ATTR_OVERSAMPLING:
		ret = set_osr_config(val, chan, dev);
		break;
	case BMP5_ATTR_POWER_MODE: {
		enum bmp5_powermode powermode = (enum bmp5_powermode)val->val1;

		ret = set_power_mode(powermode, dev);
		break;
	}
	case BMP5_ATTR_IIR_CONFIG:
		ret = set_iir_config(val, dev);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}
	return ret;
}

static int bmp581_init(const struct device *dev)
{
	CHECKIF(dev == NULL) {
		return -EINVAL;
	}

	struct bmp581_data *drv = (struct bmp581_data *)dev->data;
	struct bmp581_config *conf = (struct bmp581_config *)dev->config;
	int ret = -1;

	/* Reset the chip id. */
	drv->chip_id = 0;
	memset(&drv->last_sample, 0, sizeof(drv->last_sample));

	ret = soft_reset(dev);
	if (ret != BMP5_OK) {
		LOG_ERR("Failed to perform soft-reset: %d", ret);
		return ret;
	}

	ret = bmp581_reg_read_rtio(&conf->bus, BMP5_REG_CHIP_ID, &drv->chip_id, 1);
	if (ret != BMP5_OK) {
		LOG_ERR("Failed to read chip ID: %d", ret);
		return ret;
	}

	if (drv->chip_id != 0) {
		ret = power_up_check(dev);
		if (ret == BMP5_OK) {
			ret = validate_chip_id(drv);
			if (ret != BMP5_OK) {
				LOG_ERR("Unexpected chip id (%x). Expected (%x or %x)",
					drv->chip_id, BMP5_CHIP_ID_PRIM, BMP5_CHIP_ID_SEC);
			}
		}
	} else {
		/* that means something went wrong */
		LOG_ERR("Unexpected chip id (%x). Expected (%x or %x)", drv->chip_id,
			BMP5_CHIP_ID_PRIM, BMP5_CHIP_ID_SEC);
		return -EINVAL;
	}

	ret = set_iir_filters_config(&drv->osr_odr_press_config, dev);
	if (ret != 0) {
		LOG_ERR("Failed to set initial IIR settings: %d", ret);
		return ret;
	}

	ret = set_osr_odr_press_config(&drv->osr_odr_press_config, dev);
	if (ret != 0) {
		LOG_ERR("Failed to set initial ODR OSR settings: %d", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_BMP581_STREAM)) {
		ret = bmp581_stream_init(dev);
		if (ret != 0) {
			LOG_ERR("Failed to initialize streaming support: %d", ret);
			return ret;
		}
	}

	return ret;
}

#ifdef CONFIG_SENSOR_ASYNC_API

static void bmp581_complete_result(struct rtio *ctx, const struct rtio_sqe *sqe,
				   int result, void *arg)
{
	ARG_UNUSED(result);

	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)arg;
	struct rtio_cqe *cqe;
	int err = 0;

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			err = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	if (err != 0) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void bmp581_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	uint32_t min_buf_len = sizeof(struct bmp581_encoded_data);
	int err;
	uint8_t *buf;
	uint32_t buf_len;
	struct bmp581_encoded_data *edata;
	const struct bmp581_config *conf = dev->config;

	err = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	CHECKIF(err < 0 || buf_len < min_buf_len || !buf) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	edata = (struct bmp581_encoded_data *)buf;

	err = bmp581_encode(dev, cfg, 0, buf);
	if (err != 0) {
		LOG_ERR("Failed to encode sensor data");
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	struct rtio_sqe *read_sqe;

	err = bmp581_prep_reg_read_rtio_async(&conf->bus, BMP5_REG_TEMP_DATA_XLSB,
					      edata->payload, sizeof(edata->payload),
					      &read_sqe);
	if (err < 0) {
		LOG_ERR("Failed to prepare async read operation");
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	read_sqe->flags |= RTIO_SQE_CHAINED;

	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(conf->bus.rtio.ctx);

	if (!complete_sqe) {
		LOG_ERR("Failed to acquire completion SQE");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		rtio_sqe_drop_all(conf->bus.rtio.ctx);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(complete_sqe,
				      bmp581_complete_result,
				      iodev_sqe,
				      (void *)dev);

	rtio_submit(conf->bus.rtio.ctx, 0);
}

static void bmp581_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		bmp581_submit_one_shot(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_BMP581_STREAM)) {
		bmp581_stream_submit(dev, iodev_sqe);
	} else {
		LOG_ERR("Streaming not supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

#endif /* #ifdef CONFIG_SENSOR_ASYNC_API */

static DEVICE_API(sensor, bmp581_driver_api) = {
	.sample_fetch = bmp581_sample_fetch,
	.channel_get = bmp581_channel_get,
	.attr_set = bmp581_attr_set,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = bmp581_submit,
	.get_decoder = bmp581_get_decoder,
#endif
};

#define BMP581_INIT(i)                                                                             \
                                                                                                   \
	BUILD_ASSERT(COND_CODE_1(DT_INST_NODE_HAS_PROP(i, fifo_watermark),                         \
				 (DT_INST_PROP(i, fifo_watermark) > 0 &&                           \
				  DT_INST_PROP(i, fifo_watermark) < 16),                           \
				 (true)),                                                          \
		     "fifo-watermark must be between 1 and 15. Please set it in "                  \
		     "the device-tree node properties");                                           \
                                                                                                   \
	RTIO_DEFINE(bmp581_rtio_ctx_##i, 16, 16);                                                  \
	I2C_DT_IODEV_DEFINE(bmp581_bus_##i, DT_DRV_INST(i));                                       \
                                                                                                   \
	static struct bmp581_data bmp581_data_##i = {                                              \
		.osr_odr_press_config = {                                                          \
			.press_en = 1,                                                             \
			.odr = DT_INST_PROP(i, odr),                                               \
			.osr_t = DT_INST_PROP(i, temp_osr),                                        \
			.osr_p = DT_INST_PROP(i, press_osr),                                       \
			.iir_t = DT_INST_PROP(i, temp_iir),                                        \
			.iir_p = DT_INST_PROP(i, press_iir),                                       \
			.power_mode = DT_INST_PROP(i, power_mode),                                 \
		},                                                                                 \
		.stream = {                                                                        \
			.fifo_thres = DT_INST_PROP_OR(i, fifo_watermark, 0),                       \
		},                                                                                 \
	};                                                                                         \
                                                                                                   \
	static const struct bmp581_config bmp581_config_##i = {                                    \
		.bus.rtio = {                                                                      \
			.ctx = &bmp581_rtio_ctx_##i,                                               \
			.iodev = &bmp581_bus_##i,                                                  \
			.type = BMP581_BUS_TYPE_I2C,                                               \
		},                                                                                 \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(i, int_gpios, {0}),                           \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(i, bmp581_init, NULL, &bmp581_data_##i, &bmp581_config_##i,   \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                     \
				     &bmp581_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BMP581_INIT)
