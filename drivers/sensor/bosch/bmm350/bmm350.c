/*
 * Copyright (c) 2024 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMM350s accessed via I2C.
 * version 1.0.0
 */

#include <zephyr/logging/log.h>
#include "bmm350.h"

LOG_MODULE_REGISTER(BMM350, CONFIG_SENSOR_LOG_LEVEL);

static inline int bmm350_bus_check(const struct device *dev)
{
	const struct bmm350_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static inline int bmm350_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size)
{
	const struct bmm350_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, start, buf, size);
}

int bmm350_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct bmm350_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, reg, val);
}

static int8_t bmm350_read_otp_word(const struct device *dev, uint8_t addr, uint16_t *lsb_msb)
{
	int8_t ret = 0;
	uint8_t tx_buf = 0;
	uint8_t rx_buf[3] = {0x00};
	uint8_t otp_status = 0;
	uint8_t otp_err = BMM350_OTP_STATUS_NO_ERROR;
	uint8_t lsb = 0, msb = 0;

	if (lsb_msb) {
		/* Set OTP command at specified address */
		tx_buf = BMM350_OTP_CMD_DIR_READ | (addr & BMM350_OTP_WORD_ADDR_MSK);
		ret = bmm350_reg_write(dev, BMM350_REG_OTP_CMD_REG, tx_buf);
		if (ret) {
			LOG_ERR("i2c xfer failed! read addr = 0x%02x, ret = %d\n", tx_buf, ret);
			return ret;
		}

		do {
			/* Get OTP status */
			ret += bmm350_reg_read(dev, BMM350_REG_OTP_STATUS_REG, &rx_buf[0], 3);
			otp_status = rx_buf[2];
			otp_err = BMM350_OTP_STATUS_ERROR(otp_status);
			if (otp_err != BMM350_OTP_STATUS_NO_ERROR) {
				break;
			}
		} while ((!(otp_status & BMM350_OTP_STATUS_CMD_DONE)) && (ret == BMM350_OK));

		if (otp_err != BMM350_OTP_STATUS_NO_ERROR) {
			LOG_ERR("OTP error code: 0x%02x\n", otp_err);
			return -EIO;
		}

		/* Get OTP L/MSB data */
		ret += bmm350_reg_read(dev, BMM350_REG_OTP_DATA_MSB_REG, &rx_buf[0], 3);
		msb = rx_buf[2];
		ret += bmm350_reg_read(dev, BMM350_REG_OTP_DATA_LSB_REG, &rx_buf[0], 3);
		lsb = rx_buf[2];
		*lsb_msb = ((uint16_t)(msb << 8) | lsb) & 0xFFFF;
	}

	return ret;
}

static int32_t fix_sign(uint32_t inval, int8_t number_of_bits)
{
	int32_t ret = 0;
	int32_t power = 0;

	switch ((enum bmm350_signed_bit)number_of_bits) {
	case BMM350_SIGNED_8_BIT:
		power = 128; /* 2^7 */
		break;
	case BMM350_SIGNED_12_BIT:
		power = 2048; /* 2^11 */
		break;
	case BMM350_SIGNED_16_BIT:
		power = 32768; /* 2^15 */
		break;
	case BMM350_SIGNED_21_BIT:
		power = 1048576; /* 2^20 */
		break;
	case BMM350_SIGNED_24_BIT:
		power = 8388608; /* 2^23 */
		break;
	default:
		power = 0;
		break;
	}

	ret = (int32_t)inval;
	if (ret >= power) {
		ret = ret - (power * 2);
	}

	return ret;
}

static void bmm350_update_mag_off_sens(struct bmm350_data *data)
{
	uint16_t off_x_lsb_msb, off_y_lsb_msb, off_z_lsb_msb, t_off = 0;
	uint8_t sens_x, sens_y, sens_z, t_sens = 0;
	uint8_t tco_x, tco_y, tco_z = 0;
	uint8_t tcs_x, tcs_y, tcs_z = 0;
	uint8_t cross_x_y, cross_y_x, cross_z_x, cross_z_y = 0;

	off_x_lsb_msb = data->otp_data[BMM350_MAG_OFFSET_X] & 0x0FFF;
	off_y_lsb_msb = ((data->otp_data[BMM350_MAG_OFFSET_X] & 0xF000) >> 4) +
			(data->otp_data[BMM350_MAG_OFFSET_Y] & BMM350_LSB_MASK);
	off_z_lsb_msb = (data->otp_data[BMM350_MAG_OFFSET_Y] & 0x0F00) +
			(data->otp_data[BMM350_MAG_OFFSET_Z] & BMM350_LSB_MASK);
	t_off = data->otp_data[BMM350_TEMP_OFF_SENS] & BMM350_LSB_MASK;

	data->mag_comp.dut_offset_coef.offset_x = fix_sign(off_x_lsb_msb, BMM350_SIGNED_12_BIT);
	data->mag_comp.dut_offset_coef.offset_y = fix_sign(off_y_lsb_msb, BMM350_SIGNED_12_BIT);
	data->mag_comp.dut_offset_coef.offset_z = fix_sign(off_z_lsb_msb, BMM350_SIGNED_12_BIT);
	data->mag_comp.dut_offset_coef.t_offs =
		fix_sign(t_off, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;
	data->mag_comp.dut_offset_coef.t_offs = data->mag_comp.dut_offset_coef.t_offs / 5;

	sens_x = (data->otp_data[BMM350_MAG_SENS_X] & BMM350_MSB_MASK) >> 8;
	sens_y = (data->otp_data[BMM350_MAG_SENS_Y] & BMM350_LSB_MASK);
	sens_z = (data->otp_data[BMM350_MAG_SENS_Z] & BMM350_MSB_MASK) >> 8;
	t_sens = (data->otp_data[BMM350_TEMP_OFF_SENS] & BMM350_MSB_MASK) >> 8;

	data->mag_comp.dut_sensit_coef.sens_x =
		fix_sign(sens_x, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;
	data->mag_comp.dut_sensit_coef.sens_y =
		fix_sign(sens_y, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;
	data->mag_comp.dut_sensit_coef.sens_z =
		fix_sign(sens_z, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;
	data->mag_comp.dut_sensit_coef.t_sens =
		fix_sign(t_sens, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;

	data->mag_comp.dut_sensit_coef.sens_x = (data->mag_comp.dut_sensit_coef.sens_x / 256);
	data->mag_comp.dut_sensit_coef.sens_y = (data->mag_comp.dut_sensit_coef.sens_y / 256);
	data->mag_comp.dut_sensit_coef.sens_z = (data->mag_comp.dut_sensit_coef.sens_z / 256);
	data->mag_comp.dut_sensit_coef.t_sens = (data->mag_comp.dut_sensit_coef.t_sens / 512);

	tco_x = (data->otp_data[BMM350_MAG_TCO_X] & BMM350_LSB_MASK);
	tco_y = (data->otp_data[BMM350_MAG_TCO_Y] & BMM350_LSB_MASK);
	tco_z = (data->otp_data[BMM350_MAG_TCO_Z] & BMM350_LSB_MASK);

	data->mag_comp.dut_tco.tco_x =
		fix_sign(tco_x, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;
	data->mag_comp.dut_tco.tco_y =
		fix_sign(tco_y, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;
	data->mag_comp.dut_tco.tco_z =
		fix_sign(tco_z, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;

	data->mag_comp.dut_tco.tco_x = (data->mag_comp.dut_tco.tco_x / 32);
	data->mag_comp.dut_tco.tco_y = (data->mag_comp.dut_tco.tco_y / 32);
	data->mag_comp.dut_tco.tco_z = (data->mag_comp.dut_tco.tco_z / 32);

	tcs_x = (data->otp_data[BMM350_MAG_TCS_X] & BMM350_MSB_MASK) >> 8;
	tcs_y = (data->otp_data[BMM350_MAG_TCS_Y] & BMM350_MSB_MASK) >> 8;
	tcs_z = (data->otp_data[BMM350_MAG_TCS_Z] & BMM350_MSB_MASK) >> 8;

	data->mag_comp.dut_tcs.tcs_x =
		fix_sign(tcs_x, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;
	data->mag_comp.dut_tcs.tcs_y =
		fix_sign(tcs_y, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;
	data->mag_comp.dut_tcs.tcs_z =
		fix_sign(tcs_z, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;

	data->mag_comp.dut_tcs.tcs_x = (data->mag_comp.dut_tcs.tcs_x / 16384);
	data->mag_comp.dut_tcs.tcs_y = (data->mag_comp.dut_tcs.tcs_y / 16384);
	data->mag_comp.dut_tcs.tcs_z = (data->mag_comp.dut_tcs.tcs_z / 16384);

	data->mag_comp.dut_t0 =
		(fix_sign(data->otp_data[BMM350_MAG_DUT_T_0], BMM350_SIGNED_16_BIT) / 512) + 23;

	cross_x_y = (data->otp_data[BMM350_CROSS_X_Y] & BMM350_LSB_MASK);
	cross_y_x = (data->otp_data[BMM350_CROSS_Y_X] & BMM350_MSB_MASK) >> 8;
	cross_z_x = (data->otp_data[BMM350_CROSS_Z_X] & BMM350_LSB_MASK);
	cross_z_y = (data->otp_data[BMM350_CROSS_Z_Y] & BMM350_MSB_MASK) >> 8;

	data->mag_comp.cross_axis.cross_x_y =
		fix_sign(cross_x_y, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;
	data->mag_comp.cross_axis.cross_y_x =
		fix_sign(cross_y_x, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;
	data->mag_comp.cross_axis.cross_z_x =
		fix_sign(cross_z_x, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;
	data->mag_comp.cross_axis.cross_z_y =
		fix_sign(cross_z_y, BMM350_SIGNED_8_BIT) * BMM350_MAG_COMP_COEFF_SCALING;

	data->mag_comp.cross_axis.cross_x_y = (data->mag_comp.cross_axis.cross_x_y / 800);
	data->mag_comp.cross_axis.cross_y_x = (data->mag_comp.cross_axis.cross_y_x / 800);
	data->mag_comp.cross_axis.cross_z_x = (data->mag_comp.cross_axis.cross_z_x / 800);
	data->mag_comp.cross_axis.cross_z_y = (data->mag_comp.cross_axis.cross_z_y / 800);
}

static int bmm350_otp_dump_after_boot(const struct device *dev)
{
	struct bmm350_data *data = dev->data;
	int ret = 0;
	uint16_t otp_word = 0;

	for (uint8_t idx = 0; idx < BMM350_OTP_DATA_LENGTH; idx++) {
		ret = bmm350_read_otp_word(dev, idx, &otp_word);
		data->otp_data[idx] = otp_word;
	}

	data->var_id = (data->otp_data[30] & 0x7f00) >> 9;
	/* Set the default auto bit reset configuration */
	data->enable_auto_br = ((data->var_id > BMM350_CURRENT_SHUTTLE_VARIANT_ID) ? BMM350_DISABLE
										   : BMM350_ENABLE);

	LOG_DBG("bmm350 Find the var id %d\n", data->var_id);
	/* Update magnetometer offset and sensitivity data. */
	bmm350_update_mag_off_sens(data);

	if (ret) {
		LOG_ERR("i2c xfer failed, ret = %d\n", ret);
	}

	return ret;
}

/*!
 * @brief This API gets the PMU command status 0 value
 */
static int8_t bmm350_get_pmu_cmd_status_0(const struct device *dev,
					  struct bmm350_pmu_cmd_status_0 *pmu_cmd_stat_0)
{
	/* Variable to store the function result */
	int8_t ret;
	uint8_t rx_buf[3] = {0x00};

	if (pmu_cmd_stat_0 != NULL) {
		/* Get PMU command status 0 data */
		ret = bmm350_reg_read(dev, BMM350_REG_PMU_CMD_STATUS_0, &rx_buf[0], 3);
		LOG_DBG("pmu cmd status 0:0x%x\n", rx_buf[2]);
		if (ret == BMM350_OK) {
			pmu_cmd_stat_0->pmu_cmd_busy =
				BMM350_GET_BITS_POS_0(rx_buf[2], BMM350_PMU_CMD_BUSY);
			pmu_cmd_stat_0->odr_ovwr = BMM350_GET_BITS(rx_buf[2], BMM350_ODR_OVWR);
			pmu_cmd_stat_0->avr_ovwr = BMM350_GET_BITS(rx_buf[2], BMM350_AVG_OVWR);
			pmu_cmd_stat_0->pwr_mode_is_normal =
				BMM350_GET_BITS(rx_buf[2], BMM350_PWR_MODE_IS_NORMAL);
			pmu_cmd_stat_0->cmd_is_illegal =
				BMM350_GET_BITS(rx_buf[2], BMM350_CMD_IS_ILLEGAL);
			pmu_cmd_stat_0->pmu_cmd_value =
				BMM350_GET_BITS(rx_buf[2], BMM350_PMU_CMD_VALUE);
		}
	} else {
		ret = -EINVAL;
	}
	return ret;
}

/*!
 * @brief This internal API is used to switch from suspend mode to normal mode or forced mode.
 */
static int8_t set_powermode(const struct device *dev, enum bmm350_power_modes powermode)
{
	/* Variable to store the function result */
	int8_t ret = 0;
	uint8_t rx_buf[3] = {0x00};
	uint8_t reg_data = powermode;

	/* Array to store suspend to forced mode delay */
	uint32_t sus_to_forced_mode[4] = {
		BMM350_SUS_TO_FORCEDMODE_NO_AVG_DELAY, BMM350_SUS_TO_FORCEDMODE_AVG_2_DELAY,
		BMM350_SUS_TO_FORCEDMODE_AVG_4_DELAY, BMM350_SUS_TO_FORCEDMODE_AVG_8_DELAY};

	/* Array to store suspend to forced mode fast delay */
	uint32_t sus_to_forced_mode_fast[4] = {BMM350_SUS_TO_FORCEDMODE_FAST_NO_AVG_DELAY,
					       BMM350_SUS_TO_FORCEDMODE_FAST_AVG_2_DELAY,
					       BMM350_SUS_TO_FORCEDMODE_FAST_AVG_4_DELAY,
					       BMM350_SUS_TO_FORCEDMODE_FAST_AVG_8_DELAY};

	uint8_t avg = 0;
	uint32_t delay_us = 0;

	if (ret == BMM350_OK) {
		/* Get average configuration */
		ret = bmm350_reg_read(dev, BMM350_REG_PMU_CMD_AGGR_SET, &rx_buf[0], 3);
		/* Mask the average value */
		avg = ((rx_buf[2] & BMM350_AVG_MSK) >> BMM350_AVG_POS);
		if (ret == BMM350_OK) {
			/* Get average configuration */
			if (powermode == BMM350_NORMAL_MODE) {
				delay_us = BMM350_SUSPEND_TO_NORMAL_DELAY;
			}

			/* Check if desired power mode is forced mode */
			if (powermode == BMM350_FORCED_MODE) {
				/* Store delay based on averaging mode */
				delay_us = sus_to_forced_mode[avg];
			}

			/* Check if desired power mode is forced mode fast */
			if (powermode == BMM350_FORCED_MODE_FAST) {
				/* Store delay based on averaging mode */
				delay_us = sus_to_forced_mode_fast[avg];
			}

			/* Set PMU command configuration to desired power mode */
			ret = bmm350_reg_write(dev, BMM350_REG_PMU_CMD, reg_data);
			k_usleep(delay_us);
		}
	}

	LOG_DBG("pmu cmd agget set powermode %d\n", powermode);

	return ret;
}

/*!
 * @brief This API is used to set the power mode of the sensor
 */
static int8_t bmm350_set_powermode(const struct device *dev, enum bmm350_power_modes powermode)
{
	/* Variable to store the function result */
	int8_t ret = 0;
	uint8_t rx_buf[3] = {0x00};

	if (ret == BMM350_OK) {
		ret = bmm350_reg_read(dev, BMM350_REG_PMU_CMD, &rx_buf[0], 3);
		if (ret == BMM350_OK) {
			if (rx_buf[2] > BMM350_PMU_CMD_NM_TC) {
				ret = -EINVAL;
			}

			if ((ret == BMM350_OK) && ((rx_buf[2] == BMM350_PMU_CMD_NM) ||
						   (rx_buf[2] == BMM350_PMU_CMD_UPD_OAE))) {
				/* Set PMU command configuration */
				ret = bmm350_reg_write(dev, BMM350_REG_PMU_CMD, BMM350_PMU_CMD_SUS);
			}

			if (ret == BMM350_OK) {
				ret = set_powermode(dev, powermode);
			}
		}
	}

	return ret;
}

/*!
 * used to perform the magnetic reset of the sensor
 * which is necessary after a field shock ( 400mT field applied to sensor )
 */
static int8_t bmm350_magnetic_reset_and_wait(const struct device *dev)
{
	/* Variable to store the function result */
	int8_t ret = 0;
	struct bmm350_pmu_cmd_status_0 pmu_cmd_stat_0 = {0};
	uint8_t restore_normal = BMM350_DISABLE;

	/* Read PMU CMD status */
	ret = bmm350_get_pmu_cmd_status_0(dev, &pmu_cmd_stat_0);
	LOG_DBG("get status result 0:%d\n", ret);

	/* Check the powermode is normal before performing magnetic reset */
	if ((ret == BMM350_OK) && (pmu_cmd_stat_0.pwr_mode_is_normal == BMM350_ENABLE)) {
		restore_normal = BMM350_ENABLE;
		/* Reset can only be triggered in suspend */
		ret = bmm350_set_powermode(dev, BMM350_SUSPEND_MODE);
		LOG_DBG("set power mode 0:%d\n", ret);
	}
	if (ret == BMM350_OK) {
		/* Set BR to PMU_CMD register */
		ret = bmm350_reg_write(dev, BMM350_REG_PMU_CMD, BMM350_PMU_CMD_BR);
		k_usleep(BMM350_BR_DELAY);
	}

	if (ret == BMM350_OK) {
		/* Verify if PMU_CMD_STATUS_0 register has BR set */
		ret = bmm350_get_pmu_cmd_status_0(dev, &pmu_cmd_stat_0);
		LOG_DBG("get status result 1:%d\n", ret);
		if ((ret == BMM350_OK) &&
		    (pmu_cmd_stat_0.pmu_cmd_value != BMM350_PMU_CMD_STATUS_0_BR)) {
			ret = -EIO;
		}
	}

	if (ret == BMM350_OK) {
		/* Set FGR to PMU_CMD register */
		ret = bmm350_reg_write(dev, BMM350_REG_PMU_CMD, BMM350_PMU_CMD_FGR);
		k_usleep(BMM350_FGR_DELAY);
	}
	if (ret == BMM350_OK) {
		/* Verify if PMU_CMD_STATUS_0 register has FGR set */
		ret = bmm350_get_pmu_cmd_status_0(dev, &pmu_cmd_stat_0);
		LOG_DBG("get status result 2:%d\n", ret);

		if ((ret == BMM350_OK) &&
		    (pmu_cmd_stat_0.pmu_cmd_value != BMM350_PMU_CMD_STATUS_0_FGR)) {
			ret = -EIO;
		}
	}

	if ((ret == BMM350_OK) && (restore_normal == BMM350_ENABLE)) {
		ret = bmm350_set_powermode(dev, BMM350_NORMAL_MODE);
		LOG_DBG("set power mode 1:%d\n", ret);
	} else {
		if (ret == BMM350_OK) {
			/* Reset PMU_CMD register */
			ret = bmm350_reg_write(dev, BMM350_REG_PMU_CMD, 0x00);
		}
	}
	return ret;
}
/*!
 * @brief This API is used to read uncompensated mag and temperature data.
 */
int8_t bmm350_read_uncomp_mag_temp_data(const struct device *dev,
					struct bmm350_raw_mag_data *raw_data)
{
	struct bmm350_data *data = dev->data;
	int8_t rslt = BMM350_OK;
	uint8_t mag_data[14] = {0};
	uint32_t raw_mag_x, raw_mag_y, raw_mag_z, raw_temp;

	if (raw_data != NULL) {
		/* Get uncompensated mag data */
		rslt = bmm350_reg_read(dev, BMM350_REG_MAG_X_XLSB, mag_data, 14);

		if (rslt == BMM350_OK) {
			raw_mag_x = (uint32_t)mag_data[2] + ((uint32_t)mag_data[3] << 8) +
				    ((uint32_t)mag_data[4] << 16);
			raw_mag_y = (uint32_t)mag_data[5] + ((uint32_t)mag_data[6] << 8) +
				    ((uint32_t)mag_data[7] << 16);
			raw_mag_z = (uint32_t)mag_data[8] + ((uint32_t)mag_data[9] << 8) +
				    ((uint32_t)mag_data[10] << 16);
			raw_temp = (uint32_t)mag_data[11] + ((uint32_t)mag_data[12] << 8) +
				   ((uint32_t)mag_data[13] << 16);

			if ((data->axis_en & BMM350_EN_X_MSK) == BMM350_DISABLE) {
				raw_data->raw_xdata = BMM350_DISABLE;
			} else {
				raw_data->raw_xdata = fix_sign(raw_mag_x, BMM350_SIGNED_24_BIT);
			}

			if ((data->axis_en & BMM350_EN_Y_MSK) == BMM350_DISABLE) {
				raw_data->raw_ydata = BMM350_DISABLE;
			} else {
				raw_data->raw_ydata = fix_sign(raw_mag_y, BMM350_SIGNED_24_BIT);
			}

			if ((data->axis_en & BMM350_EN_Z_MSK) == BMM350_DISABLE) {
				raw_data->raw_zdata = BMM350_DISABLE;
			} else {
				raw_data->raw_zdata = fix_sign(raw_mag_z, BMM350_SIGNED_24_BIT);
			}

			raw_data->raw_data_temp = fix_sign(raw_temp, BMM350_SIGNED_24_BIT);
		}
	} else {
		rslt = -EINVAL;
	}

	return rslt;
}
static int8_t read_out_raw_data(const struct device *dev, int32_t *out_data)
{
	int8_t rslt = BMM350_OK;
	int32_t temp = 0;
	struct bmm350_raw_mag_data raw_data = {0};

	if (out_data != NULL) {
		rslt = bmm350_read_uncomp_mag_temp_data(dev, &raw_data);

		if (rslt == BMM350_OK) {
			/* Convert mag lsb to uT and temp lsb to degC */
			out_data[0] = ((raw_data.raw_xdata * BMM350_LSB_TO_UT_XY_COEFF) /
				       BMM350_LSB_TO_UT_COEFF_DIV);
			out_data[1] = ((raw_data.raw_ydata * BMM350_LSB_TO_UT_XY_COEFF) /
				       BMM350_LSB_TO_UT_COEFF_DIV);
			out_data[2] = ((raw_data.raw_zdata * BMM350_LSB_TO_UT_Z_COEFF) /
				       BMM350_LSB_TO_UT_COEFF_DIV);
			out_data[3] = ((raw_data.raw_data_temp * BMM350_LSB_TO_UT_TEMP_COEFF) /
				       BMM350_LSB_TO_UT_COEFF_DIV);

			if (out_data[3] > 0) {
				temp = (out_data[3] - (2549 / 100));
			} else if (out_data[3] < 0) {
				temp = (out_data[3] + (2549 / 100));
			} else {
				temp = out_data[3];
			}

			out_data[3] = temp;
		}
	} else {
		rslt = -EINVAL;
	}

	return rslt;
}

int8_t bmm350_get_compensated_mag_xyz_temp_data_fixed(const struct device *dev,
						      struct bmm350_mag_temp_data *mag_temp_data)
{
	struct bmm350_data *data = dev->data;
	int8_t rslt = BMM350_OK;
	uint8_t indx;
	int32_t out_data[4] = {0};
	int32_t dut_offset_coef[3], dut_sensit_coef[3], dut_tco[3], dut_tcs[3];
	int32_t cr_ax_comp_x, cr_ax_comp_y, cr_ax_comp_z;

	if (mag_temp_data != NULL) {
		/* Reads raw magnetic x,y and z axis along with temperature */
		rslt = read_out_raw_data(dev, out_data);

		if (rslt == BMM350_OK) {
			/* Apply compensation to temperature reading */
			out_data[3] = (((BMM350_MAG_COMP_COEFF_SCALING +
					 data->mag_comp.dut_sensit_coef.t_sens) *
					out_data[3]) +
				       data->mag_comp.dut_offset_coef.t_offs) /
				      BMM350_MAG_COMP_COEFF_SCALING;

			/* Store magnetic compensation structure to an array */
			dut_offset_coef[0] = data->mag_comp.dut_offset_coef.offset_x;
			dut_offset_coef[1] = data->mag_comp.dut_offset_coef.offset_y;
			dut_offset_coef[2] = data->mag_comp.dut_offset_coef.offset_z;

			dut_sensit_coef[0] = data->mag_comp.dut_sensit_coef.sens_x;
			dut_sensit_coef[1] = data->mag_comp.dut_sensit_coef.sens_y;
			dut_sensit_coef[2] = data->mag_comp.dut_sensit_coef.sens_z;

			dut_tco[0] = data->mag_comp.dut_tco.tco_x;
			dut_tco[1] = data->mag_comp.dut_tco.tco_y;
			dut_tco[2] = data->mag_comp.dut_tco.tco_z;

			dut_tcs[0] = data->mag_comp.dut_tcs.tcs_x;
			dut_tcs[1] = data->mag_comp.dut_tcs.tcs_y;
			dut_tcs[2] = data->mag_comp.dut_tcs.tcs_z;

			/* Compensate raw magnetic data */
			for (indx = 0; indx < 3; indx++) {
				out_data[indx] = (out_data[indx] * (BMM350_MAG_COMP_COEFF_SCALING +
								    dut_sensit_coef[indx])) /
						 BMM350_MAG_COMP_COEFF_SCALING;
				out_data[indx] = (out_data[indx] + dut_offset_coef[indx]);
				out_data[indx] =
					((out_data[indx] * BMM350_MAG_COMP_COEFF_SCALING) +
					 (dut_tco[indx] * (out_data[3] - data->mag_comp.dut_t0))) /
					BMM350_MAG_COMP_COEFF_SCALING;
				out_data[indx] =
					(out_data[indx] * BMM350_MAG_COMP_COEFF_SCALING) /
					(BMM350_MAG_COMP_COEFF_SCALING +
					 (dut_tcs[indx] * (out_data[3] - data->mag_comp.dut_t0)));
			}

			cr_ax_comp_x =
				((((out_data[0] * BMM350_MAG_COMP_COEFF_SCALING) -
				   (data->mag_comp.cross_axis.cross_x_y * out_data[1])) *
				  BMM350_MAG_COMP_COEFF_SCALING) /
				 ((BMM350_MAG_COMP_COEFF_SCALING * BMM350_MAG_COMP_COEFF_SCALING) -
				  (data->mag_comp.cross_axis.cross_y_x *
				   data->mag_comp.cross_axis.cross_x_y)));

			cr_ax_comp_y =
				((((out_data[1] * BMM350_MAG_COMP_COEFF_SCALING) -
				   (data->mag_comp.cross_axis.cross_y_x * out_data[0])) *
				  BMM350_MAG_COMP_COEFF_SCALING) /
				 ((BMM350_MAG_COMP_COEFF_SCALING * BMM350_MAG_COMP_COEFF_SCALING) -
				  (data->mag_comp.cross_axis.cross_y_x *
				   data->mag_comp.cross_axis.cross_x_y)));

			cr_ax_comp_z =
				(out_data[2] +
				 (((out_data[0] * ((data->mag_comp.cross_axis.cross_y_x *
						    data->mag_comp.cross_axis.cross_z_y) -
						   (data->mag_comp.cross_axis.cross_z_x *
						    BMM350_MAG_COMP_COEFF_SCALING))) -
				   (out_data[1] * ((data->mag_comp.cross_axis.cross_z_y *
						    BMM350_MAG_COMP_COEFF_SCALING) -
						   (data->mag_comp.cross_axis.cross_x_y *
						    data->mag_comp.cross_axis.cross_z_x))))) /
					 (((BMM350_MAG_COMP_COEFF_SCALING *
					    BMM350_MAG_COMP_COEFF_SCALING) -
					   data->mag_comp.cross_axis.cross_y_x *
						   data->mag_comp.cross_axis.cross_x_y)));

			out_data[0] = (int32_t)cr_ax_comp_x;
			out_data[1] = (int32_t)cr_ax_comp_y;
			out_data[2] = (int32_t)cr_ax_comp_z;
		}
		LOG_DBG("mag data %d %d %d\n", out_data[0], out_data[1], out_data[2]);

		if (rslt == BMM350_OK) {
			if ((data->axis_en & BMM350_EN_X_MSK) == BMM350_DISABLE) {
				mag_temp_data->x = BMM350_DISABLE;
			} else {
				mag_temp_data->x = out_data[0];
			}

			if ((data->axis_en & BMM350_EN_Y_MSK) == BMM350_DISABLE) {
				mag_temp_data->y = BMM350_DISABLE;
			} else {
				mag_temp_data->y = out_data[1];
			}

			if ((data->axis_en & BMM350_EN_Z_MSK) == BMM350_DISABLE) {
				mag_temp_data->z = BMM350_DISABLE;
			} else {
				mag_temp_data->z = out_data[2];
			}
			mag_temp_data->temperature = out_data[3];
		}
	} else {
		rslt = -EINVAL;
	}
	return rslt;
}

static int bmm350_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bmm350_data *drv_data = dev->data;
	struct bmm350_mag_temp_data mag_temp_data;

	if (bmm350_get_compensated_mag_xyz_temp_data_fixed(dev, &mag_temp_data) < 0) {
		LOG_ERR("failed to read sample");
		return -EIO;
	}
	drv_data->mag_temp_data.x = mag_temp_data.x;
	drv_data->mag_temp_data.y = mag_temp_data.y;
	drv_data->mag_temp_data.z = mag_temp_data.z;
	return 0;
}

/*
 * ut change to Gauss
 */
static void bmm350_convert(struct sensor_value *val, int raw_val)
{
	val->val1 = raw_val / 100;
	val->val2 = (raw_val % 100) * 10000;
}

static int bmm350_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bmm350_data *drv_data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		bmm350_convert(val, drv_data->mag_temp_data.x);
		break;
	case SENSOR_CHAN_MAGN_Y:
		bmm350_convert(val, drv_data->mag_temp_data.y);
		break;
	case SENSOR_CHAN_MAGN_Z:
		bmm350_convert(val, drv_data->mag_temp_data.z);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		bmm350_convert(val, drv_data->mag_temp_data.x);
		bmm350_convert(val + 1, drv_data->mag_temp_data.y);
		bmm350_convert(val + 2, drv_data->mag_temp_data.z);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
static uint8_t acc_odr_to_reg(const struct sensor_value *val)
{
	double odr = sensor_value_to_double((struct sensor_value *)val);

	uint8_t reg = BMM350_DATA_RATE_100HZ;

	if ((odr >= 0.78125) && (odr <= 1.5625)) {
		reg = BMM350_DATA_RATE_1_5625HZ;
	} else if ((odr > 1.5625) && (odr <= 3.125)) {
		reg = BMM350_DATA_RATE_3_125HZ;
	} else if ((odr > 3.125) && (odr <= 6.25)) {
		reg = BMM350_DATA_RATE_6_25HZ;
	} else if ((odr > 6.25) && (odr <= 12.5)) {
		reg = BMM350_DATA_RATE_12_5HZ;
	} else if ((odr > 12.5) && (odr <= 25.0)) {
		reg = BMM350_DATA_RATE_25HZ;
	} else if ((odr > 25.0) && (odr <= 50.0)) {
		reg = BMM350_DATA_RATE_50HZ;
	} else if ((odr > 50.0) && (odr <= 100.0)) {
		reg = BMM350_DATA_RATE_100HZ;
	} else if ((odr > 100.0) && (odr <= 200.0)) {
		reg = BMM350_DATA_RATE_200HZ;
	} else if ((odr > 200.0) && (odr <= 400.0)) {
		reg = BMM350_DATA_RATE_400HZ;
	} else if (odr > 400.0) {
		reg = BMM350_DATA_RATE_400HZ;
	}
	return reg;
}

static uint8_t acc_osr_to_reg(const struct sensor_value *val)
{
	switch (val->val1) {
	case 0:
		return BMM350_NO_AVERAGING;
	case 2:
		return BMM350_AVERAGING_2;
	case 4:
		return BMM350_AVERAGING_4;
	case 8:
		return BMM350_AVERAGING_8;
	default:
		return 0xFF;
	}
}

/*!
 * @brief This API sets the ODR and averaging factor.
 */
int8_t bmm350_set_odr_performance(enum bmm350_data_rates odr,
				  enum bmm350_performance_parameters performance,
				  const struct device *dev)
{
	/* Variable to store the function result */
	int8_t rslt = 0x00;
	/* Variable to get PMU command */
	uint8_t reg_data = 0;
	enum bmm350_performance_parameters performance_fix = performance;

	/* Reduce the performance setting when too high for the chosen ODR */
	if ((odr == BMM350_DATA_RATE_400HZ) && (performance >= BMM350_AVERAGING_2)) {
		performance_fix = BMM350_NO_AVERAGING;
	} else if ((odr == BMM350_DATA_RATE_200HZ) && (performance >= BMM350_AVERAGING_4)) {
		performance_fix = BMM350_AVERAGING_2;
	} else if ((odr == BMM350_DATA_RATE_100HZ) && (performance >= BMM350_AVERAGING_8)) {
		performance_fix = BMM350_AVERAGING_4;
	}
	if (performance_fix != performance) {
		LOG_WRN("performance adjusted to %d", performance_fix);
	}

	/* ODR is an enum taking the generated constants from the register map */
	reg_data = ((uint8_t)odr & BMM350_ODR_MSK);
	/* AVG / performance is an enum taking the generated constants from the register map */
	reg_data = BMM350_SET_BITS(reg_data, BMM350_AVG, (uint8_t)performance_fix);
	/* Set PMU command configurations for ODR and performance */
	rslt = bmm350_reg_write(dev, BMM350_REG_PMU_CMD_AGGR_SET, reg_data);
	LOG_DBG("odr index %d odr_reg_data 0x%x", odr, reg_data);

	if (rslt == BMM350_OK) {
		/* Set PMU command configurations to update odr and average */
		reg_data = BMM350_PMU_CMD_UPD_OAE;
		/* Set PMU command configuration */
		rslt = bmm350_reg_write(dev, BMM350_REG_PMU_CMD, reg_data);
		if (rslt == BMM350_OK) {
			k_usleep(BMM350_UPD_OAE_DELAY);
		}
	}

	return rslt;
}
static int set_mag_odr_osr(const struct device *dev, const struct sensor_value *odr,
			   const struct sensor_value *osr)
{
	int ret;
	uint8_t rx_buf[3] = {0x00};
	uint8_t osr_bits;
	uint8_t odr_bits;

	/* read current state */
	ret = bmm350_reg_read(dev, BMM350_REG_PMU_CMD_AGGR_SET, &rx_buf[0], 3);
	if (ret < 0) {
		LOG_ERR("failed to read PMU_CMD_AGGR_SET");
		return -EIO;
	}
	osr_bits = ((rx_buf[2] & BMM350_AVG_MSK) >> BMM350_AVG_POS);
	odr_bits = ((rx_buf[2] & BMM350_ODR_MSK) >> BMM350_ODR_POS);

	/* to change sampling rate, device needs to suspend first */
	ret = bmm350_set_powermode(dev, BMM350_SUSPEND_MODE);
	if (ret < 0) {
		LOG_ERR("failed to set suspend mode");
		return -EIO;
	}

	if (odr) {
		odr_bits = acc_odr_to_reg(odr);
	}
	if (osr) {
		osr_bits = acc_osr_to_reg(osr);
		if (osr_bits == 0xFF) {
			LOG_ERR("unsupported oversampling rate");
			return -EINVAL;
		}
	}
	if (bmm350_set_odr_performance((enum bmm350_data_rates)odr_bits, osr_bits, dev) < 0) {
		LOG_ERR("bmm350_set_odr_performance failed");
		return -EIO;
	}

	/* go to normal mode now, measurements are requested. */
	ret = bmm350_set_powermode(dev, BMM350_NORMAL_MODE);
	if (ret < 0) {
		LOG_ERR("failed to set suspend mode");
		return -EIO;
	}

	return 0;
}

static int bmm350_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (set_mag_odr_osr(dev, val, NULL) < 0) {
			return -EIO;
		}
		break;
	case SENSOR_ATTR_OVERSAMPLING:
		if (set_mag_odr_osr(dev, NULL, val) < 0) {
			return -EIO;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
static DEVICE_API(sensor, bmm350_api_funcs) = {
	.attr_set = bmm350_attr_set,
	.sample_fetch = bmm350_sample_fetch,
	.channel_get = bmm350_channel_get,
#ifdef CONFIG_BMM350_TRIGGER
	.trigger_set = bmm350_trigger_set,
#endif
};

static int bmm350_init_chip(const struct device *dev)
{
	struct bmm350_pmu_cmd_status_0 pmu_cmd_stat_0 = {0};
	/* Variable to store soft-reset command */
	uint8_t soft_reset;
	uint8_t rx_buf[3] = {0x00};
	uint8_t chip_id[3] = {0x00};
	int ret = 0;
	/* Read chip ID (can only be read in sleep mode)*/
	if (bmm350_reg_read(dev, BMM350_REG_CHIP_ID, &chip_id[0], 3) < 0) {
		LOG_ERR("failed reading chip id");
		goto err_poweroff;
	}
	if (chip_id[2] != BMM350_CHIP_ID) {
		LOG_ERR("invalid chip id 0x%x", chip_id[2]);
		goto err_poweroff;
	}
	/* Soft-reset */
	soft_reset = BMM350_CMD_SOFTRESET;

	/* Set the command in the command register */
	ret = bmm350_reg_write(dev, BMM350_REG_CMD, soft_reset);
	k_usleep(BMM350_SOFT_RESET_DELAY);
	/* Read chip ID (can only be read in sleep mode)*/
	if (bmm350_reg_read(dev, BMM350_REG_CHIP_ID, &chip_id[0], 3) < 0) {
		LOG_ERR("failed reading chip id");
		goto err_poweroff;
	}
	if (chip_id[2] != BMM350_CHIP_ID) {
		LOG_ERR("invalid chip id 0x%x", chip_id[2]);
		goto err_poweroff;
	}
	ret = bmm350_otp_dump_after_boot(dev);
	LOG_DBG("bmm350 chip_id 0x%x otp dump after boot %d\n", chip_id[2], ret);

	if (bmm350_reg_write(dev, BMM350_REG_OTP_CMD_REG, BMM350_OTP_CMD_PWR_OFF_OTP) < 0) {
		LOG_ERR("failed to set REP");
		goto err_poweroff;
	}

	ret += bmm350_magnetic_reset_and_wait(dev);

	LOG_DBG("bmm350 setup result %d\n", ret);

	ret += bmm350_get_pmu_cmd_status_0(dev, &pmu_cmd_stat_0);
	ret += bmm350_reg_read(dev, BMM350_REG_ERR_REG, &rx_buf[0], 3);
	if (ret != 0) {
		LOG_ERR("%s %d", __func__, ret);
	}

	return 0;

err_poweroff:
	ret = bmm350_set_powermode(dev, BMM350_SUSPEND_MODE);
	if (ret != 0) {
		return -EIO;
	}
	return -EIO;
}

#ifdef CONFIG_PM_DEVICE
static int pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = bmm350_set_powermode(dev, BMM350_NORMAL_MODE);
		if (ret != 0) {
			LOG_ERR("failed to enter normal mode: %d", ret);
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = bmm350_set_powermode(dev, BMM350_SUSPEND_MODE);
		if (ret != 0) {
			LOG_ERR("failed to enter suspend mode: %d", ret);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif

static int bmm350_init(const struct device *dev)
{
	int err = 0;
	struct bmm350_data *data = dev->data;
	const struct sensor_value odr = {100, 0};
	const struct sensor_value osr = {2, 0};

	err = bmm350_bus_check(dev);
	if (err < 0) {
		LOG_ERR("bus check failed: %d", err);
		return err;
	}

	if (bmm350_init_chip(dev) < 0) {
		LOG_ERR("failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_BMM350_TRIGGER
	if (bmm350_trigger_mode_init(dev) < 0) {
		LOG_ERR("Cannot set up trigger mode.");
		return -EINVAL;
	}
#endif

	/* Assign axis_en with all axis enabled (BMM350_EN_XYZ_MSK) */
	data->axis_en = BMM350_EN_XYZ_MSK;

	/* Initialize to 100Hz, averaging between 2 samples by default */
	if (set_mag_odr_osr(dev, &odr, &osr) < 0) {
		return -EIO;
	}

	return 0;
}

/* Initializes a struct bmm350_config for an instance on an I2C bus. */
#define BMM350_CONFIG_I2C(inst) .bus.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_io = &bmm350_bus_io_i2c,
#define BMM350_INT_CFG(inst)    .drdy_int = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, {0}),

#define BMM350_DEFINE(inst)                                                                        \
	static struct bmm350_data bmm350_data_##inst;                                              \
	static const struct bmm350_config bmm350_config_##inst = {                                 \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst),                                             \
		.bus_io = &bmm350_bus_io_i2c,                                                      \
		BMM350_INT_CFG(inst)};                                                             \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, pm_action);                                                 \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bmm350_init, PM_DEVICE_DT_INST_GET(inst),               \
				     &bmm350_data_##inst, &bmm350_config_##inst, POST_KERNEL,      \
				     CONFIG_SENSOR_INIT_PRIORITY, &bmm350_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(BMM350_DEFINE)
