/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max40109

#include <zephyr/device.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "max40109.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor/max40109.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_MAX40109_CALIBRATION
#include <math.h>
#endif

LOG_MODULE_REGISTER(max40109, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_MAX40109_CALIBRATION

/** Coefficients are stored in MTP as the raw IEEE-754 encoding of a float. */
union max40109_float_bits {
	float f;
	uint32_t u;
};

/** Reinterpret the IEEE-754 encoding of @p f as a raw 32-bit word. */
static inline uint32_t max40109_float_to_hex(float f)
{
	union max40109_float_bits bits = {.f = f};

	return bits.u;
}

/** Reinterpret the raw 32-bit word @p u as an IEEE-754 single-precision float. */
static inline float max40109_hex_to_float(uint32_t u)
{
	union max40109_float_bits bits = {.u = u};

	return bits.f;
}

#endif /* CONFIG_MAX40109_CALIBRATION */

static int max40109_reg_access(const struct device *dev, uint8_t addr_reg, uint8_t *data, bool read,
			       uint8_t length)
{

	const struct max40109_config *config = dev->config;
	int ret;

	if (read) {
		ret = i2c_burst_read_dt(&config->i2c, addr_reg, data, length);
	} else {
		ret = i2c_burst_write_dt(&config->i2c, addr_reg, data, length);
	}

	return ret;
}

int max40109_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *val, uint8_t length)
{
	return max40109_reg_access(dev, reg_addr, val, true, length);
}

int max40109_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t val)
{
	return max40109_reg_access(dev, reg_addr, &val, false, 1);
}

int max40109_reg_write_multiple(const struct device *dev, uint8_t reg_addr, const uint8_t *val,
				uint8_t length)
{
	const struct max40109_config *config = dev->config;

	if (length < 2) {
		return -EINVAL;
	}
	return i2c_burst_write_dt(&config->i2c, reg_addr, val, length);
}

int max40109_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t val)
{
	uint8_t reg_val = 0;
	int ret;

	ret = max40109_reg_read(dev, reg_addr, &reg_val, 1);
	if (ret < 0) {
		return ret;
	}

	reg_val &= ~mask;
	reg_val |= FIELD_PREP(mask, val);

	return max40109_reg_write(dev, reg_addr, reg_val);
}

int max40109_mtp_read(const struct device *dev, uint8_t mtp_addr, uint8_t *val)
{
	int ret = 0;

	ret = max40109_reg_write(dev, MAX40109_MTP_CONTROL, 0x10);
	if (ret < 0) {
		return ret;
	}
	ret = max40109_reg_write(dev, MAX40109_MTP_PROT_ADDR, mtp_addr);
	if (ret < 0) {
		return ret;
	}

	return max40109_reg_read(dev, MAX40109_MTP_PROT_RDATA_MSB, val, MAX40109_MTP_DATA_LENGTH);
}

int max40109_mtp_write(const struct device *dev, uint8_t mtp_addr, const uint8_t *val)
{
	int ret = 0;

	ret = max40109_reg_write(dev, MAX40109_MTP_CONTROL, 0x10);
	if (ret < 0) {
		return ret;
	}
	ret = max40109_reg_write(dev, MAX40109_MTP_PROT_ADDR, mtp_addr);
	if (ret < 0) {
		return ret;
	}

	return max40109_reg_write_multiple(dev, MAX40109_MTP_PROT_WDATA_MSB, val,
					   MAX40109_MTP_DATA_LENGTH);
}

int max40109_mtp_update(const struct device *dev, uint8_t mtp_addr, uint16_t mask, uint16_t val)
{
	uint8_t reg_val[2] = {0};
	uint16_t reg;
	int ret;

	ret = max40109_mtp_read(dev, mtp_addr, reg_val);
	if (ret < 0) {
		return ret;
	}
	reg = (reg_val[0] << 8) | reg_val[1];
	reg &= ~mask;
	reg |= FIELD_PREP(mask, val);
	reg_val[0] = (reg >> 8) & 0xFF;
	reg_val[1] = reg & 0xFF;

	return max40109_mtp_write(dev, mtp_addr, reg_val);
}

#ifdef CONFIG_MAX40109_CALIBRATION
/**
 * @brief Convert float coefficient to byte representation
 *
 * @param hex Float coefficient in hex format
 * @param buf_msb MSB byte buffer
 * @param buf_lsb LSB byte buffer
 */
static void coeff_hex_to_bytes(const uint32_t hex, uint8_t *buf_msb, uint8_t *buf_lsb)
{

	buf_msb[0] = (hex >> 24) & 0xFF;
	buf_msb[1] = (hex >> 16) & 0xFF;
	buf_lsb[0] = (hex >> 8) & 0xFF;
	buf_lsb[1] = hex & 0xFF;
}
#endif /* CONFIG_MAX40109_CALIBRATION */

/**
 * @brief Initialize MTP charge pump for burning operations
 *
 * Configures the charge pump control registers and slope parameters required
 * for MTP programming. This sequence must be executed before burning data to MTP.
 *
 * @note Requires minimum 12V applied at VDDHV supply pin for MTP burning.
 *       Per the datasheet this sequence is run once before a burn session, not
 *       once per burned register, and it is not needed at all for the volatile
 *       max40109_mtp_write() path, which never engages the charge pump.
 *
 * Register configuration sequence per datasheet:
 * - CP_Control_1: 0x80 - Charge pump control
 * - CP_Control_2: 0x1B - Charge pump timing
 * - SLP_MR:       0x0302 - Slope MR parameter
 * - SLP_MREF:     0x0200 - Slope MREF parameter
 * - SLP_MRV:      0x0300 - Slope MRV parameter
 * - SLP_MREFV:    0x0401 - Slope MREFV parameter
 *
 * @param dev Device structure
 * @return int 0 on success, negative error code on failure
 */
int max40109_mtp_initialize(const struct device *dev)
{
	/* Slope parameters for MTP charge pump - values from datasheet */
	static const uint8_t slp_mr[2] = {MAX40109_SLP_MR_MSB_INIT, MAX40109_SLP_MR_LSB_INIT};
	static const uint8_t slp_mref[2] = {MAX40109_SLP_MREF_MSB_INIT, MAX40109_SLP_MREF_LSB_INIT};
	static const uint8_t slp_mrv[2] = {MAX40109_SLP_MRV_MSB_INIT, MAX40109_SLP_MRV_LSB_INIT};
	static const uint8_t slp_mrefv[2] = {MAX40109_SLP_MREFV_MSB_INIT,
					     MAX40109_SLP_MREFV_LSB_INIT};
	int ret = 0;

	/* Configure charge pump control registers */
	ret = max40109_reg_write(dev, MAX40109_CP_CONTROL_1, MAX40109_CP_CONTROL_1_MTP_INIT);
	if (ret < 0) {
		return ret;
	}

	ret = max40109_reg_write(dev, MAX40109_CP_CONTROL_2, MAX40109_CP_CONTROL_2_MTP_INIT);
	if (ret < 0) {
		return ret;
	}

	/* Configure slope parameters for MTP programming */
	ret = max40109_reg_write_multiple(dev, MAX40109_SLP_MR, slp_mr, 2);
	if (ret < 0) {
		return ret;
	}

	ret = max40109_reg_write_multiple(dev, MAX40109_SLP_MREF, slp_mref, 2);
	if (ret < 0) {
		return ret;
	}

	ret = max40109_reg_write_multiple(dev, MAX40109_SLP_MRV_MSB, slp_mrv, 2);
	if (ret < 0) {
		return ret;
	}

	ret = max40109_reg_write_multiple(dev, MAX40109_SLP_MREFV_MSB, slp_mrefv, 2);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int max40109_is_burn_successful(const struct device *dev)
{
	uint8_t mtp_status = 0;
	int ret;

	ret = max40109_reg_read(dev, MAX40109_MTP_STATUS, &mtp_status, 1);
	if (ret < 0) {
		return ret;
	}

	if ((mtp_status & (MAX40109_MTP_ECC_ERR_2_BIT_MASK | MAX40109_MTP_VPP_INIT_FAIL_MASK |
			   MAX40109_MTP_FULL_MASK | MAX40109_MTP_VERIFICATION_FAIL_MASK |
			   MAX40109_MTP_VPP_ACT_MASK)) != 0U) {
		LOG_ERR("MTP burn failed: status 0x%02x", mtp_status);
		return -EIO;
	} else if ((mtp_status & MAX40109_MTP_BURN_DONE_MASK) != 0U) {
		LOG_INF("MTP burn successful");
	} else {
		LOG_ERR("MTP burn not completed successfully");
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_MAX40109_CALIBRATION

static const struct calibration_coeff_map cal_table[] = {
	{MAX40109_CALIBRATION_K0, MAX40109_MTP_REG_CAL_DATA0},
	{MAX40109_CALIBRATION_K1, MAX40109_MTP_REG_CAL_DATA2},
	{MAX40109_CALIBRATION_K2, MAX40109_MTP_REG_CAL_DATA4},
	{MAX40109_CALIBRATION_K3, MAX40109_MTP_REG_CAL_DATA6},
	{MAX40109_CALIBRATION_H0, MAX40109_MTP_REG_CAL_DATA8},
	{MAX40109_CALIBRATION_H1, MAX40109_MTP_REG_CAL_DATA10},
	{MAX40109_CALIBRATION_H2, MAX40109_MTP_REG_CAL_DATA12},
	{MAX40109_CALIBRATION_H3, MAX40109_MTP_REG_CAL_DATA14},
	{MAX40109_CALIBRATION_G0, MAX40109_MTP_REG_CAL_DATA16},
	{MAX40109_CALIBRATION_G1, MAX40109_MTP_REG_CAL_DATA18},
	{MAX40109_CALIBRATION_G2, MAX40109_MTP_REG_CAL_DATA20},
	{MAX40109_CALIBRATION_G3, MAX40109_MTP_REG_CAL_DATA22},
	{MAX40109_CALIBRATION_N0, MAX40109_MTP_REG_CAL_DATA24},
	{MAX40109_CALIBRATION_N1, MAX40109_MTP_REG_CAL_DATA26},
	{MAX40109_CALIBRATION_N2, MAX40109_MTP_REG_CAL_DATA28},
	{MAX40109_CALIBRATION_N3, MAX40109_MTP_REG_CAL_DATA30},
	{MAX40109_CALIBRATION_M0, MAX40109_MTP_REG_CAL_DATA32},
	{MAX40109_CALIBRATION_M1, MAX40109_MTP_REG_CAL_DATA34},
	{MAX40109_CALIBRATION_M2, MAX40109_MTP_REG_CAL_DATA36},
	{MAX40109_CALIBRATION_M3, MAX40109_MTP_REG_CAL_DATA38},
};

static int get_calibration_address(uint8_t cal_coeff, uint8_t *mtp_addr)
{
	for (size_t i = 0; i < ARRAY_SIZE(cal_table); i++) {
		if (cal_table[i].cal_coeff == cal_coeff) {
			*mtp_addr = cal_table[i].cal_start_addr;
			return 0;
		}
	}
	return -EINVAL;
}

int max40109_mtp_calibration(const struct device *dev, enum max40109_calibration_coefficients coeff,
			     float value, bool burn)
{
	uint8_t mtp_addr = 0;
	uint32_t hex = max40109_float_to_hex(value);
	uint8_t buf_msb[2] = {0, 0};
	uint8_t buf_lsb[2] = {0, 0};
	int ret = 0;

	if (coeff < MAX40109_CALIBRATION_K0 || coeff > MAX40109_CALIBRATION_M3) {
		LOG_ERR("Invalid calibration coefficient");
		return -EINVAL;
	}
	coeff_hex_to_bytes(hex, buf_msb, buf_lsb);

	ret = get_calibration_address(coeff, &mtp_addr);
	if (ret < 0) {
		LOG_ERR("Failed to get MTP address for coefficient %d", coeff);
		return ret;
	}

	if (burn) {
		/* Charge pump setup is a burn-session prerequisite, so it is done
		 * once here and covers both words of the coefficient. The volatile
		 * write path below does not need it.
		 */
		ret = max40109_mtp_initialize(dev);
		if (ret < 0) {
			return ret;
		}

		LOG_INF("Burning MTP data for coefficient %d", coeff);
		ret = max40109_mtp_burn(dev, mtp_addr, buf_lsb);
		if (ret < 0) {
			LOG_ERR("Failed to burn MTP data for coefficient %d", coeff);
			return ret;
		}
		ret = max40109_mtp_burn(dev, mtp_addr + 1, buf_msb);
		if (ret < 0) {
			LOG_ERR("Failed to burn MTP data for coefficient %d", coeff);
			return ret;
		}
	} else {
		LOG_INF("Writing MTP data for coefficient %d without burning", coeff);
		ret = max40109_mtp_write(dev, mtp_addr, buf_lsb);
		if (ret < 0) {
			LOG_ERR("Failed to write MTP data for coefficient %d", coeff);
			return ret;
		}
		ret = max40109_mtp_write(dev, mtp_addr + 1, buf_msb);
		if (ret < 0) {
			LOG_ERR("Failed to write MTP data for coefficient %d", coeff);
			return ret;
		}
		LOG_INF("MTP data for coefficient %d written successfully", coeff);
	}

	return 0;
}

#endif /* CONFIG_MAX40109_CALIBRATION */

static const struct temp_gain_map temp_gain_table[] = {
	{1, 500000, MAX40109_DT_PGA_TEMP_GAIN_1_5},
	{2, 0, MAX40109_DT_PGA_TEMP_GAIN_2},
	{3, 0, MAX40109_DT_PGA_TEMP_GAIN_3},
	{5, 0, MAX40109_DT_PGA_TEMP_GAIN_5},
	{6, 0, MAX40109_DT_PGA_TEMP_GAIN_6},
	{10, 0, MAX40109_DT_PGA_TEMP_GAIN_10},
	{15, 0, MAX40109_DT_PGA_TEMP_GAIN_15},
	{20, 0, MAX40109_DT_PGA_TEMP_GAIN_20},
	{24, 0, MAX40109_DT_PGA_TEMP_GAIN_24},
	{30, 0, MAX40109_DT_PGA_TEMP_GAIN_30},
	{36, 0, MAX40109_DT_PGA_TEMP_GAIN_36},
	{40, 0, MAX40109_DT_PGA_TEMP_GAIN_40},
	{45, 0, MAX40109_DT_PGA_TEMP_GAIN_45},
	{60, 0, MAX40109_DT_PGA_TEMP_GAIN_60},
	{72, 0, MAX40109_DT_PGA_TEMP_GAIN_72},
	{90, 0, MAX40109_DT_PGA_TEMP_GAIN_90},
};

static const struct pressure_gain_map pressure_gain_table[] = {
	{5, MAX40109_DT_PGA_PRESSURE_GAIN_5},
	{10, MAX40109_DT_PGA_PRESSURE_GAIN_10},
	{15, MAX40109_DT_PGA_PRESSURE_GAIN_15},
	{20, MAX40109_DT_PGA_PRESSURE_GAIN_20},
	{24, MAX40109_DT_PGA_PRESSURE_GAIN_24},
	{40, MAX40109_DT_PGA_PRESSURE_GAIN_40},
	{60, MAX40109_DT_PGA_PRESSURE_GAIN_60},
	{72, MAX40109_DT_PGA_PRESSURE_GAIN_72},
	{90, MAX40109_DT_PGA_PRESSURE_GAIN_90},
	{108, MAX40109_DT_PGA_PRESSURE_GAIN_108},
	{126, MAX40109_DT_PGA_PRESSURE_GAIN_126},
	{144, MAX40109_DT_PGA_PRESSURE_GAIN_144},
	{160, MAX40109_DT_PGA_PRESSURE_GAIN_160},
	{180, MAX40109_DT_PGA_PRESSURE_GAIN_180},
	{200, MAX40109_DT_PGA_PRESSURE_GAIN_200},
	{252, MAX40109_DT_PGA_PRESSURE_GAIN_252},
	{540, MAX40109_DT_PGA_PRESSURE_GAIN_540},
	{1080, MAX40109_DT_PGA_PRESSURE_GAIN_1080},
	{1440, MAX40109_DT_PGA_PRESSURE_GAIN_1440},
	{2520, MAX40109_DT_PGA_PRESSURE_GAIN_2520},
};

static const struct sampling_rate_entry sampling_rate_table[] = {
	{1000, 1, MAX40109_DT_SAMPLE_RATE_1KSPS_1SPS},
	{1000, 10, MAX40109_DT_SAMPLE_RATE_1KSPS_10SPS},
	{2000, 1, MAX40109_DT_SAMPLE_RATE_2KSPS_1SPS},
	{2000, 10, MAX40109_DT_SAMPLE_RATE_2KSPS_10SPS},
	{4000, 1, MAX40109_DT_SAMPLE_RATE_4KSPS_1SPS},
	{4000, 10, MAX40109_DT_SAMPLE_RATE_4KSPS_10SPS},
	{8000, 1, MAX40109_DT_SAMPLE_RATE_8KSPS_1SPS},
	{8000, 10, MAX40109_DT_SAMPLE_RATE_8KSPS_10SPS},
	{16000, 1, MAX40109_DT_SAMPLE_RATE_16KSPS_1SPS},
	{16000, 10, MAX40109_DT_SAMPLE_RATE_16KSPS_10SPS},
};

static const struct drv_scale_entry drv_scale_table[] = {
	{1, 0, MAX40109_DT_DRV_SCALE_1_0},
	{0, 500000, MAX40109_DT_DRV_SCALE_0_5},
	{0, 375000, MAX40109_DT_DRV_SCALE_0_375},
	{0, 250000, MAX40109_DT_DRV_SCALE_0_25},
};

#ifdef CONFIG_MAX40109_CALIBRATION
static const struct calib_map calib_map_table[] = {
	{SENSOR_CHAN_MAX40109_AMBIENT_TEMP_CALIB_COEFF_K0, MAX40109_CALIBRATION_K0},
	{SENSOR_CHAN_MAX40109_AMBIENT_TEMP_CALIB_COEFF_K1, MAX40109_CALIBRATION_K1},
	{SENSOR_CHAN_MAX40109_AMBIENT_TEMP_CALIB_COEFF_K2, MAX40109_CALIBRATION_K2},
	{SENSOR_CHAN_MAX40109_AMBIENT_TEMP_CALIB_COEFF_K3, MAX40109_CALIBRATION_K3},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_H0, MAX40109_CALIBRATION_H0},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_H1, MAX40109_CALIBRATION_H1},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_H2, MAX40109_CALIBRATION_H2},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_H3, MAX40109_CALIBRATION_H3},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_G0, MAX40109_CALIBRATION_G0},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_G1, MAX40109_CALIBRATION_G1},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_G2, MAX40109_CALIBRATION_G2},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_G3, MAX40109_CALIBRATION_G3},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_N0, MAX40109_CALIBRATION_N0},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_N1, MAX40109_CALIBRATION_N1},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_N2, MAX40109_CALIBRATION_N2},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_N3, MAX40109_CALIBRATION_N3},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_M0, MAX40109_CALIBRATION_M0},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_M1, MAX40109_CALIBRATION_M1},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_M2, MAX40109_CALIBRATION_M2},
	{SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_M3, MAX40109_CALIBRATION_M3},
};
#endif /* CONFIG_MAX40109_CALIBRATION */

static const struct analog_bw_map bw_table[] = {
	{1200, MAX40109_DT_ANALOG_FILTER_BW_1200HZ},
	{9000, MAX40109_DT_ANALOG_FILTER_BW_9000HZ},
	{37000, MAX40109_DT_ANALOG_FILTER_BW_37000HZ},
	{45000, MAX40109_DT_ANALOG_FILTER_BW_45000HZ},
};

static const struct digital_filter_map filter_table[] = {
	{0, MAX40109_DT_FILTER_AVG_NONE},
	{4, MAX40109_DT_FILTER_AVG_4},
	{8, MAX40109_DT_FILTER_AVG_8},
	{16, MAX40109_DT_FILTER_AVG_16},
	{32, MAX40109_DT_FILTER_AVG_32},
	{64, MAX40109_DT_FILTER_AVG_64},
	{128, MAX40109_DT_FILTER_AVG_128},
};

static int max40109_set_sampling_rate(const struct device *dev, int pressure_rate,
				      int temperature_rate)
{
	for (size_t i = 0; i < ARRAY_SIZE(sampling_rate_table); i++) {
		if (sampling_rate_table[i].pressure_rate == pressure_rate &&
		    sampling_rate_table[i].temperature_rate == temperature_rate) {
			return max40109_reg_write(dev, MAX40109_ADC_SAMPLE_RATE,
						  sampling_rate_table[i].reg_value);
		}
	}
	return -EINVAL;
}

static int max40109_set_pressure_gain(const struct device *dev, int val1)
{
	for (size_t i = 0; i < ARRAY_SIZE(pressure_gain_table); i++) {
		if (pressure_gain_table[i].val1 == val1) {
			return max40109_reg_write(dev, MAX40109_PGA_PRESSURE_GAIN,
						  pressure_gain_table[i].reg_value);
		}
	}
	return -EINVAL;
}

static int set_analog_output_stage(const struct device *dev, uint8_t mode)
{
	if (mode < MAX40109_AOS_SHUTDOWN || mode > MAX40109_AOS_RATIO_METRIC_VDD5V_1X_ALT3 ||
	    mode == 0x01) {
		return -EINVAL;
	}

	return max40109_reg_write(dev, MAX40109_ANALOG_OUTPUT_STAGE, mode);
}

static int max40109_set_temperature_gain(const struct device *dev, int val1, int val2)
{
	for (size_t i = 0; i < ARRAY_SIZE(temp_gain_table); i++) {
		if (temp_gain_table[i].val1 == val1 && temp_gain_table[i].val2 == val2) {
			return max40109_reg_write(dev, MAX40109_PGA_TEMPERATURE_GAIN,
						  temp_gain_table[i].reg_value);
		}
	}
	return -EINVAL;
}

static int max40109_set_alert_mode(const struct device *dev, int32_t mode)
{
	if (mode < MAX40109_ALERT_ISSUES_INTERRUPT ||
	    mode > MAX40109_ALERT_ISSUES_PRESSURE_DIGITAL_OUTPUT_CASE_4) {
		return -EINVAL;
	}
	return max40109_reg_update(dev, MAX40109_REG_CONFIG_LSB, MAX40109_ALERT_MODE_MASK,
				   (uint8_t)mode);
}

static int max40109_set_temp_mode(const struct device *dev, int32_t mode)
{
	if (mode < MAX40109_TEMP_MODE_INT_SE_GAIN_1 ||
	    mode > MAX40109_TEMP_MODE_DRV_DIFF_DAC_OFFSET) {
		return -EINVAL;
	}

	return max40109_reg_update(dev, MAX40109_TEMP_MODE, MAX40109_TEMP_MODE_MASK, (uint8_t)mode);
}

static int max40109_set_drv_scale(const struct device *dev, int val1, int val2)
{
	for (size_t i = 0; i < ARRAY_SIZE(drv_scale_table); i++) {
		if (drv_scale_table[i].val1 == val1 && drv_scale_table[i].val2 == val2) {
			return max40109_reg_update(dev, MAX40109_TEMP_MODE, MAX40109_DRV_SCALE_MASK,
						   drv_scale_table[i].reg_value);
		}
	}
	return -EINVAL;
}

static int set_analog_filter_bw(const struct device *dev, int analog_filter_bw)
{
	for (size_t i = 0; i < ARRAY_SIZE(bw_table); i++) {
		if (bw_table[i].bw == analog_filter_bw) {
			return max40109_reg_write(dev, MAX40109_ANALOG_FILTER_BW,
						  bw_table[i].reg_val);
		}
	}
	return -EINVAL;
}

static int set_digital_filter_setup(const struct device *dev, int digital_filter_setup)
{
	for (size_t i = 0; i < ARRAY_SIZE(filter_table); i++) {
		if (filter_table[i].setup == digital_filter_setup) {
			return max40109_reg_update(dev, MAX40109_REG_CONFIG_MSB,
						   MAX40109_DIGITAL_FILTER_MASK,
						   filter_table[i].reg_val);
		}
	}
	return -EINVAL;
}

#ifdef CONFIG_MAX40109_CALIBRATION

int max40109_set_calibration_coeff(const struct device *dev, enum sensor_channel chan,
				   const struct sensor_value *val)
{
	uint8_t coeff = 0;
	bool found = false;
	float calibration_value;
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(calib_map_table); i++) {
		if (calib_map_table[i].chan == chan) {
			coeff = calib_map_table[i].coeff;
			found = true;
			break;
		}
	}
	if (!found) {
		return -ENOTSUP;
	}
	calibration_value = val->val1 + (val->val2 / 1000000.0f);
	ret = max40109_mtp_calibration(dev, coeff, calibration_value, false);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int max40109_mtp_calibration_read(const struct device *dev,
				  enum max40109_calibration_coefficients coeff, float *value)
{
	uint8_t mtp_addr = 0;
	uint8_t buf_msb[2] = {0, 0};
	uint8_t buf_lsb[2] = {0, 0};
	int ret = 0;

	if (coeff < MAX40109_CALIBRATION_K0 || coeff > MAX40109_CALIBRATION_M3) {
		LOG_ERR("Invalid calibration coefficient");
		return -EINVAL;
	}

	ret = get_calibration_address(coeff, &mtp_addr);
	if (ret < 0) {
		LOG_ERR("Failed to get MTP address for coefficient %d", coeff);
		return ret;
	}

	ret = max40109_mtp_read(dev, mtp_addr, buf_lsb);
	if (ret < 0) {
		LOG_ERR("Failed to read MTP data for coefficient %d", coeff);
		return ret;
	}
	ret = max40109_mtp_read(dev, mtp_addr + 1, buf_msb);
	if (ret < 0) {
		LOG_ERR("Failed to read MTP data for coefficient %d", coeff);
		return ret;
	}
	uint32_t hex = ((uint32_t)buf_msb[0] << 24) | ((uint32_t)buf_msb[1] << 16) |
		       ((uint32_t)buf_lsb[0] << 8) | buf_lsb[1];

	*value = max40109_hex_to_float(hex);
	return 0;
}

#endif /* CONFIG_MAX40109_CALIBRATION */

int max40109_mtp_burn(const struct device *dev, uint8_t mtp_addr, const uint8_t *val)
{
	int ret = 0;

	ret = max40109_reg_write(dev, MAX40109_MTP_ADDR, mtp_addr);

	if (ret < 0) {
		return ret;
	}

	ret = max40109_reg_write_multiple(dev, MAX40109_MTP_DATA0_MSB, val,
					  MAX40109_MTP_DATA_LENGTH);
	if (ret < 0) {
		return ret;
	}

	ret = max40109_reg_write(dev, MAX40109_MTP_CONTROL, 0x01);

	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(50));
	ret = max40109_is_burn_successful(dev);
	if (ret < 0) {
		LOG_ERR("MTP burn failed, check MTP status");
		return -EIO;
	}

	LOG_INF("MTP burn successful");
	return 0;
}

/**
 * @brief Probe function to set initial attributes for MAX40109
 *
 * @param dev Device Structure
 * @return int 0 on success, negative error code on failure
 */
static int max40109_probe(const struct device *dev)
{
	const struct max40109_config *config = dev->config;
	int ret;

	/** Set ADC Sampling Rate */
	if (config->adc_sample_rate != UINT8_MAX) {
		ret = max40109_reg_write(dev, MAX40109_ADC_SAMPLE_RATE, config->adc_sample_rate);
		if (ret < 0) {
			return ret;
		}
	}

	/** Disable Interrupts at Startup */
	ret = max40109_reg_write_multiple(dev, MAX40109_INTERRUPT_ENABLE_MSB,
					  (uint8_t[]){0x00, 0x00}, 2);
	if (ret < 0) {
		return ret;
	}

	/** Set Pressure PGA Gain */
	if (config->pga_pressure_gain != UINT8_MAX) {
		ret = max40109_reg_write(dev, MAX40109_PGA_PRESSURE_GAIN,
					 config->pga_pressure_gain);
		if (ret < 0) {
			return ret;
		}
	}

	/** Set Temperature PGA Gain */
	if (config->pga_temperature_gain != UINT8_MAX) {
		ret = max40109_reg_write(dev, MAX40109_PGA_TEMPERATURE_GAIN,
					 config->pga_temperature_gain);
		if (ret < 0) {
			return ret;
		}
	}

	/** Set Alert Mode */
	if (config->alert_mode != UINT8_MAX) {
		ret = max40109_reg_update(dev, MAX40109_REG_CONFIG_LSB, MAX40109_ALERT_MODE_MASK,
					  config->alert_mode);
		if (ret < 0) {
			return ret;
		}
	}

	/** Set Temp Mode */
	if (config->temp_mode != UINT8_MAX) {
		ret = max40109_reg_update(dev, MAX40109_TEMP_MODE, MAX40109_TEMP_MODE_MASK,
					  config->temp_mode);
		if (ret < 0) {
			return ret;
		}
	}
	/** Set Temp Current */
	if (config->temp_current != UINT8_MAX) {
		ret = max40109_reg_update(dev, MAX40109_REG_CONFIG_LSB, MAX40109_TEMP_CURRENT_MASK,
					  config->temp_current);
		if (ret < 0) {
			return ret;
		}
	}

	/** Set DRV Scale */
	if (config->drv_scale != UINT8_MAX) {
		ret = max40109_reg_update(dev, MAX40109_TEMP_MODE, MAX40109_DRV_SCALE_MASK,
					  config->drv_scale);
		if (ret < 0) {
			return ret;
		}
	}

	/** Set Analog Filter Bandwidth */
	if (config->analog_filter_bw != UINT8_MAX) {
		ret = max40109_reg_write(dev, MAX40109_ANALOG_FILTER_BW, config->analog_filter_bw);
		if (ret < 0) {
			return ret;
		}
	}

	/** Set Digital Filter Setup */
	if (config->digital_filter_setup != UINT8_MAX) {
		ret = max40109_reg_update(dev, MAX40109_REG_CONFIG_MSB,
					  MAX40109_DIGITAL_FILTER_MASK,
					  config->digital_filter_setup);
		if (ret < 0) {
			return ret;
		}
	}

	/** Set Analog Output Stage */
	if (config->analog_output_stage != UINT8_MAX) {
		ret = max40109_reg_write(dev, MAX40109_ANALOG_OUTPUT_STAGE,
					 config->analog_output_stage);
		if (ret < 0) {
			return ret;
		}
	}

	/** Set Bridge Drive */
	if (config->bridge_drive != UINT8_MAX) {
		ret = max40109_reg_write(dev, MAX40109_BRIDGE_DRIVE, config->bridge_drive);
		if (ret < 0) {
			return ret;
		}
	}

	/** Set Current Source */
	if (config->current_source != UINT8_MAX) {
		ret = max40109_reg_write(dev, MAX40109_CURRENT_SOURCE, config->current_source);
		if (ret < 0) {
			return ret;
		}
	}
#ifdef CONFIG_MAX40109_CALIBRATION
	/** Set Calibration Coefficients */
	for (size_t i = 0; i < ARRAY_SIZE(config->calib_coeff); i++) {
		if (!isnan(config->calib_coeff[i])) {
			ret = max40109_mtp_calibration(dev,
						       (enum max40109_calibration_coefficients)i,
						       config->calib_coeff[i], false);
			if (ret < 0) {
				LOG_ERR("Failed to set calibration coefficient %zu", i);
				return ret;
			}
		}
	}
#endif /* CONFIG_MAX40109_CALIBRATION */
	return 0;
}

static int attr_set_analog_filter_bw(const struct device *dev, const struct sensor_value *val)
{
	if (val->val1 < 0) {
		return -EINVAL;
	}

	return set_analog_filter_bw(dev, val->val1);
}

static int attr_set_digital_filter(const struct device *dev, const struct sensor_value *val)
{
	if (val->val1 < 0) {
		return -EINVAL;
	}

	return set_digital_filter_setup(dev, val->val1);
}

static int attr_set_alert_mode(const struct device *dev, const struct sensor_value *val)
{
	return max40109_set_alert_mode(dev, val->val1);
}

static int attr_set_temp_mode(const struct device *dev, const struct sensor_value *val)
{
	return max40109_set_temp_mode(dev, val->val1);
}

static int attr_set_drv_scale(const struct device *dev, const struct sensor_value *val)
{
	if (val == NULL) {
		return -EINVAL;
	}
	if (val->val1 < 0 || val->val2 < 0) {
		return -EINVAL;
	}
	if (val->val1 == 1 && val->val2 != 0) {
		return -EINVAL;
	}

	return max40109_set_drv_scale(dev, val->val1, val->val2);
}

static int attr_set_analog_output_stage(const struct device *dev, const struct sensor_value *val)
{
	if (val->val1 < 0 || val->val1 > 0x0F || val->val1 == 0x01) {
		return -EINVAL;
	}

	return set_analog_output_stage(dev, val->val1);
}

/**
 * @brief MTP-backed threshold attributes.
 *
 * Each entry fully describes one attribute, so set and get share a single
 * implementation. SENSOR_CHAN_ALL means the attribute is not channel-specific.
 */
static const struct max40109_threshold_map threshold_table[] = {
	{SENSOR_ATTR_MAX40109_OVER_PRESSURE_THRESHOLD_POSITIVE, MAX40109_MTP_REG_DIAG_DATA0,
	 MAX40109_OVER_PRESSURE_POSITIVE_MASK, SENSOR_CHAN_PRESS},
	{SENSOR_ATTR_MAX40109_UNDER_PRESSURE_THRESHOLD_POSITIVE, MAX40109_MTP_REG_DIAG_DATA0,
	 MAX40109_UNDER_PRESSURE_POSITIVE_MASK, SENSOR_CHAN_PRESS},
	{SENSOR_ATTR_MAX40109_OVER_PRESSURE_THRESHOLD_NEGATIVE, MAX40109_MTP_REG_DIAG_DATA1,
	 MAX40109_OVER_PRESSURE_NEGATIVE_MASK, SENSOR_CHAN_PRESS},
	{SENSOR_ATTR_MAX40109_UNDER_PRESSURE_THRESHOLD_NEGATIVE, MAX40109_MTP_REG_DIAG_DATA1,
	 MAX40109_UNDER_PRESSURE_NEGATIVE_MASK, SENSOR_CHAN_PRESS},
	{SENSOR_ATTR_MAX40109_OVER_TEMPERATURE_VOLTAGE_THRESHOLD, MAX40109_MTP_REG_DIAG_DATA2,
	 MAX40109_OVER_TEMPERATURE_VOLTAGE_MASK, SENSOR_CHAN_AMBIENT_TEMP},
	{SENSOR_ATTR_MAX40109_UNDER_TEMPERATURE_VOLTAGE_THRESHOLD, MAX40109_MTP_REG_DIAG_DATA2,
	 MAX40109_UNDER_TEMPERATURE_VOLTAGE_MASK, SENSOR_CHAN_AMBIENT_TEMP},
	{SENSOR_ATTR_MAX40109_OVER_VOLTAGE_DRIVE_THRESHOLD, MAX40109_MTP_REG_DIAG_DATA3,
	 MAX40109_OVER_VOLTAGE_DRIVE_MASK, SENSOR_CHAN_ALL},
	{SENSOR_ATTR_MAX40109_UNDER_VOLTAGE_DRIVE_THRESHOLD, MAX40109_MTP_REG_DIAG_DATA3,
	 MAX40109_UNDER_VOLTAGE_DRIVE_MASK, SENSOR_CHAN_ALL},
	{SENSOR_ATTR_MAX40109_PRIMARY_THRESHOLD_PRESSURE_VALUE, MAX40109_MTP_REG_PRESSURE_THRESHOLD,
	 MAX40109_PRIMARY_THRESHOLD_PRESSURE_VALUE_MASK, SENSOR_CHAN_PRESS},
	{SENSOR_ATTR_MAX40109_HYSTERESIS_THRESHOLD_PRESSURE_VALUE,
	 MAX40109_MTP_REG_PRESSURE_THRESHOLD, MAX40109_HYSTERESIS_THRESHOLD_PRESSURE_VALUE_MASK,
	 SENSOR_CHAN_PRESS},
};

/**
 * @brief Look up a threshold attribute.
 *
 * @param attr Attribute to find
 * @param chan Channel the caller supplied
 * @return Matching entry, or NULL if @p attr is not a threshold attribute or
 *         does not apply to @p chan
 */
static const struct max40109_threshold_map *find_threshold(enum sensor_attribute attr,
							   enum sensor_channel chan)
{
	for (size_t i = 0; i < ARRAY_SIZE(threshold_table); i++) {
		if (threshold_table[i].attr != (uint16_t)attr) {
			continue;
		}
		if (threshold_table[i].chan != SENSOR_CHAN_ALL && threshold_table[i].chan != chan) {
			return NULL;
		}
		return &threshold_table[i];
	}

	return NULL;
}

static int attr_set_threshold(const struct device *dev, const struct max40109_threshold_map *entry,
			      const struct sensor_value *val)
{
	if (val->val1 < 0 || val->val1 > 0xFF) {
		return -EINVAL;
	}

	return max40109_mtp_update(dev, entry->mtp_addr, entry->mask, val->val1);
}

static int attr_get_threshold(const struct device *dev, const struct max40109_threshold_map *entry,
			      struct sensor_value *val)
{
	uint8_t buf[MAX40109_MTP_DATA_LENGTH] = {0};
	int ret;

	ret = max40109_mtp_read(dev, entry->mtp_addr, buf);
	if (ret < 0) {
		return ret;
	}

	val->val1 = FIELD_GET(entry->mask, sys_get_be16(buf));
	val->val2 = 0;

	return 0;
}

/**
 * @brief Look up the current ADC sample rate setting
 *
 * @param dev Device structure
 * @param entry Receives the table entry matching the register
 * @return int 0 on success, negative error code on failure
 */
static int max40109_get_sampling_rate(const struct device *dev,
				      const struct sampling_rate_entry **entry)
{
	uint8_t reg = 0;
	int ret;

	ret = max40109_reg_read(dev, MAX40109_ADC_SAMPLE_RATE, &reg, 1);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(sampling_rate_table); i++) {
		if (sampling_rate_table[i].reg_value == reg) {
			*entry = &sampling_rate_table[i];
			return 0;
		}
	}

	LOG_ERR("Unknown ADC sample rate encoding 0x%02X", reg);
	return -EIO;
}

/**
 * @brief Set the pressure or temperature sampling frequency
 *
 * Both rates share one register, so the rate of the other channel is kept.
 *
 * @param dev Device structure
 * @param chan SENSOR_CHAN_PRESS (1000, 2000, 4000, 8000 or 16000 Hz) or
 *             SENSOR_CHAN_AMBIENT_TEMP (1 or 10 Hz)
 * @param val Sampling frequency in Hz
 * @return int 0 on success, negative error code on failure
 */
static int attr_set_sampling_freq(const struct device *dev, enum sensor_channel chan,
				  const struct sensor_value *val)
{
	const struct sampling_rate_entry *current;
	int ret;

	if (chan != SENSOR_CHAN_PRESS && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}
	if (val->val2 != 0) {
		return -EINVAL;
	}

	ret = max40109_get_sampling_rate(dev, &current);
	if (ret < 0) {
		return ret;
	}

	if (chan == SENSOR_CHAN_PRESS) {
		return max40109_set_sampling_rate(dev, val->val1, current->temperature_rate);
	}

	return max40109_set_sampling_rate(dev, current->pressure_rate, val->val1);
}

/**
 * @brief Set PGA gain for pressure or temperature channel
 *
 * @param dev Device structure
 * @param chan Sensor channel (SENSOR_CHAN_PRESS or SENSOR_CHAN_AMBIENT_TEMP)
 * @param val Sensor value where:
 *            - For SENSOR_CHAN_PRESS:
 *              val->val1: Pressure gain (5, 10, 15, 20, 24, 40, 60, 72, 90, etc.)
 *            - For SENSOR_CHAN_AMBIENT_TEMP:
 *              val->val1: Temperature gain integer part (e.g., 1 for 1.5x)
 *              val->val2: Temperature gain decimal part (e.g., 500000 for 0.5, giving 1.5x)
 * @return int 0 on success, negative error code on failure
 */
static int attr_set_gain(const struct device *dev, enum sensor_channel chan,
			 const struct sensor_value *val)
{
	if (chan == SENSOR_CHAN_PRESS) {
		if (val->val1 < 5) {
			return -EINVAL;
		}
		return max40109_set_pressure_gain(dev, val->val1);
	}

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		if (val->val1 < 1) {
			return -EINVAL;
		}
		return max40109_set_temperature_gain(dev, val->val1, val->val2);
	}

	return -ENOTSUP;
}

static int max40109_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct max40109_threshold_map *threshold;

	if (val == NULL) {
		return -EINVAL;
	}

	threshold = find_threshold(attr, chan);
	if (threshold != NULL) {
		return attr_set_threshold(dev, threshold, val);
	}

	switch ((int)attr) {
	/* Standard sensor attributes */
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return attr_set_sampling_freq(dev, chan, val);
	case SENSOR_ATTR_GAIN:
		/* For SENSOR_CHAN_PRESS: val->val1 = pressure gain (5-2520)
		 * For SENSOR_CHAN_AMBIENT_TEMP: val->val1 = integer part, val->val2 = decimal part
		 */
		return attr_set_gain(dev, chan, val);
#ifdef CONFIG_MAX40109_CALIBRATION
	case SENSOR_ATTR_CALIB_TARGET:
		return max40109_set_calibration_coeff(dev, chan, val);
#endif /* CONFIG_MAX40109_CALIBRATION */

	/* MAX40109-specific attributes */
	case SENSOR_ATTR_MAX40109_ANALOG_FILTER_BW_SETUP:
		return attr_set_analog_filter_bw(dev, val);
	case SENSOR_ATTR_MAX40109_DIGITAL_FILTER_SETUP:
		return attr_set_digital_filter(dev, val);
	case SENSOR_ATTR_MAX40109_ALERT_MODE:
		return attr_set_alert_mode(dev, val);
	case SENSOR_ATTR_MAX40109_TEMP_MODE:
		return attr_set_temp_mode(dev, val);
	case SENSOR_ATTR_MAX40109_DRV_SCALE:
		return attr_set_drv_scale(dev, val);
	case SENSOR_ATTR_MAX40109_ANALOG_OUTPUT_STAGE:
		return attr_set_analog_output_stage(dev, val);
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Read a raw register field back as an attribute value.
 *
 * Used for the attributes whose value is the register encoding itself, with no
 * engineering-unit translation.
 *
 * @param dev Device structure
 * @param reg_addr Register holding the field
 * @param mask Field position within that register
 * @param val Destination, val1 receives the field, val2 is zeroed
 * @return int 0 on success, negative error code on failure
 */
static int attr_get_reg_field(const struct device *dev, uint8_t reg_addr, uint8_t mask,
			      struct sensor_value *val)
{
	uint8_t reg = 0;
	int ret;

	ret = max40109_reg_read(dev, reg_addr, &reg, 1);
	if (ret < 0) {
		return ret;
	}

	val->val1 = FIELD_GET(mask, reg);
	val->val2 = 0;

	return 0;
}

static int attr_get_sampling_freq(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	const struct sampling_rate_entry *current;
	int ret;

	if (chan != SENSOR_CHAN_PRESS && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	ret = max40109_get_sampling_rate(dev, &current);
	if (ret < 0) {
		return ret;
	}

	val->val1 =
		(chan == SENSOR_CHAN_PRESS) ? current->pressure_rate : current->temperature_rate;
	val->val2 = 0;

	return 0;
}

static int attr_get_gain(const struct device *dev, enum sensor_channel chan,
			 struct sensor_value *val)
{
	uint8_t reg = 0;
	int ret;

	if (chan == SENSOR_CHAN_PRESS) {
		ret = max40109_reg_read(dev, MAX40109_PGA_PRESSURE_GAIN, &reg, 1);
		if (ret < 0) {
			return ret;
		}

		for (size_t i = 0; i < ARRAY_SIZE(pressure_gain_table); i++) {
			if (pressure_gain_table[i].reg_value == reg) {
				val->val1 = pressure_gain_table[i].val1;
				val->val2 = 0;
				return 0;
			}
		}

		LOG_ERR("Unknown pressure gain encoding 0x%02X", reg);
		return -EIO;
	}

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		ret = max40109_reg_read(dev, MAX40109_PGA_TEMPERATURE_GAIN, &reg, 1);
		if (ret < 0) {
			return ret;
		}

		for (size_t i = 0; i < ARRAY_SIZE(temp_gain_table); i++) {
			if (temp_gain_table[i].reg_value == reg) {
				val->val1 = temp_gain_table[i].val1;
				val->val2 = temp_gain_table[i].val2;
				return 0;
			}
		}

		LOG_ERR("Unknown temperature gain encoding 0x%02X", reg);
		return -EIO;
	}

	return -ENOTSUP;
}

#ifdef CONFIG_MAX40109_CALIBRATION
static int attr_get_calib_target(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	float coeff_value = 0.0f;
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(calib_map_table); i++) {
		if (calib_map_table[i].chan != chan) {
			continue;
		}

		ret = max40109_mtp_calibration_read(dev, calib_map_table[i].coeff, &coeff_value);
		if (ret < 0) {
			return ret;
		}

		return sensor_value_from_float(val, coeff_value);
	}

	return -ENOTSUP;
}
#endif /* CONFIG_MAX40109_CALIBRATION */

static int attr_get_analog_filter_bw(const struct device *dev, struct sensor_value *val)
{
	uint8_t reg = 0;
	int ret;

	ret = max40109_reg_read(dev, MAX40109_ANALOG_FILTER_BW, &reg, 1);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(bw_table); i++) {
		if (bw_table[i].reg_val == reg) {
			val->val1 = bw_table[i].bw;
			val->val2 = 0;
			return 0;
		}
	}

	LOG_ERR("Unknown analog filter bandwidth encoding 0x%02X", reg);
	return -EIO;
}

static int attr_get_digital_filter(const struct device *dev, struct sensor_value *val)
{
	uint8_t reg = 0;
	uint8_t field;
	int ret;

	ret = max40109_reg_read(dev, MAX40109_REG_CONFIG_MSB, &reg, 1);
	if (ret < 0) {
		return ret;
	}

	field = FIELD_GET(MAX40109_DIGITAL_FILTER_MASK, reg);

	for (size_t i = 0; i < ARRAY_SIZE(filter_table); i++) {
		if (filter_table[i].reg_val == field) {
			val->val1 = filter_table[i].setup;
			val->val2 = 0;
			return 0;
		}
	}

	LOG_ERR("Unknown digital filter encoding 0x%02X", field);
	return -EIO;
}

static int attr_get_drv_scale(const struct device *dev, struct sensor_value *val)
{
	uint8_t reg = 0;
	uint8_t field;
	int ret;

	ret = max40109_reg_read(dev, MAX40109_TEMP_MODE, &reg, 1);
	if (ret < 0) {
		return ret;
	}

	field = FIELD_GET(MAX40109_DRV_SCALE_MASK, reg);

	for (size_t i = 0; i < ARRAY_SIZE(drv_scale_table); i++) {
		if (drv_scale_table[i].reg_value == field) {
			val->val1 = drv_scale_table[i].val1;
			val->val2 = drv_scale_table[i].val2;
			return 0;
		}
	}

	LOG_ERR("Unknown DRV scale encoding 0x%02X", field);
	return -EIO;
}

/**
 * @brief Read back an attribute previously set with max40109_attr_set().
 *
 * Values are reported in the same units attr_set() accepts, so a get/set round
 * trip is lossless.
 *
 * @param dev Device structure
 * @param chan Channel the attribute applies to
 * @param attr Attribute to read
 * @param val Destination value
 * @return int 0 on success, negative error code on failure
 */
static int max40109_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct max40109_threshold_map *threshold;

	if (val == NULL) {
		return -EINVAL;
	}

	threshold = find_threshold(attr, chan);
	if (threshold != NULL) {
		return attr_get_threshold(dev, threshold, val);
	}

	switch ((int)attr) {
	/* Standard sensor attributes */
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return attr_get_sampling_freq(dev, chan, val);
	case SENSOR_ATTR_GAIN:
		return attr_get_gain(dev, chan, val);
#ifdef CONFIG_MAX40109_CALIBRATION
	case SENSOR_ATTR_CALIB_TARGET:
		return attr_get_calib_target(dev, chan, val);
#endif /* CONFIG_MAX40109_CALIBRATION */

	/* MAX40109-specific attributes */
	case SENSOR_ATTR_MAX40109_ANALOG_FILTER_BW_SETUP:
		return attr_get_analog_filter_bw(dev, val);
	case SENSOR_ATTR_MAX40109_DIGITAL_FILTER_SETUP:
		return attr_get_digital_filter(dev, val);
	case SENSOR_ATTR_MAX40109_ALERT_MODE:
		return attr_get_reg_field(dev, MAX40109_REG_CONFIG_LSB, MAX40109_ALERT_MODE_MASK,
					  val);
	case SENSOR_ATTR_MAX40109_TEMP_MODE:
		return attr_get_reg_field(dev, MAX40109_TEMP_MODE, MAX40109_TEMP_MODE_MASK, val);
	case SENSOR_ATTR_MAX40109_DRV_SCALE:
		return attr_get_drv_scale(dev, val);
	case SENSOR_ATTR_MAX40109_ANALOG_OUTPUT_STAGE:
		return attr_get_reg_field(dev, MAX40109_ANALOG_OUTPUT_STAGE, GENMASK(7, 0), val);
	default:
		return -ENOTSUP;
	}
}

static int max40109_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct max40109_data *data = dev->data;
	bool fetch_pressure;
	bool fetch_temperature;
	int ret;

	uint8_t uncalibrated_data[2];
	uint8_t calibrated_data[2];

	fetch_pressure = (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_PRESS ||
			  chan == (enum sensor_channel)SENSOR_CHAN_MAX40109_PRESS_RAW ||
			  chan == (enum sensor_channel)SENSOR_CHAN_MAX40109_PRESS_COMPENSATED);
	fetch_temperature =
		(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP ||
		 chan == (enum sensor_channel)SENSOR_CHAN_MAX40109_AMBIENT_TEMP_RAW ||
		 chan == (enum sensor_channel)SENSOR_CHAN_MAX40109_AMBIENT_TEMP_COMPENSATED);

	if (!fetch_pressure && !fetch_temperature) {
		return -ENOTSUP;
	}

	if (fetch_pressure) {
		ret = max40109_reg_read(dev, MAX40109_UNCALIBRATED_PRESSURE_MSB, uncalibrated_data,
					2);
		if (ret < 0) {
			return ret;
		}
		data->uncalibrated_pressure = (int16_t)sys_get_be16(uncalibrated_data);

		ret = max40109_reg_read(dev, MAX40109_CALIBRATED_PRESSURE_MSB, calibrated_data, 2);
		if (ret < 0) {
			return ret;
		}
		data->calibrated_pressure = (int16_t)sys_get_be16(calibrated_data);
	}

	if (fetch_temperature) {
		ret = max40109_reg_read(dev, MAX40109_UNCALIBRATED_TEMPERATURE_MSB,
					uncalibrated_data, 2);
		if (ret < 0) {
			return ret;
		}
		data->uncalibrated_temperature = (int16_t)sys_get_be16(uncalibrated_data);

		ret = max40109_reg_read(dev, MAX40109_CALIBRATED_TEMPERATURE_MSB, calibrated_data,
					2);
		if (ret < 0) {
			return ret;
		}
		data->calibrated_temperature = (int16_t)sys_get_be16(calibrated_data);
	}

	return 0;
}

/**
 * @brief Scale a compensated measurement code to the channel unit
 *
 * @param val Destination, in kPa or degrees Celsius
 * @param code Signed fraction of full scale, MAX40109_CODE_FULL_SCALE being 1.0
 * @param full_scale Value at full scale relative to code 0, in thousandths of
 *                   the channel unit (Pa or millidegrees Celsius)
 * @param offset Value at code 0, in the same unit as @p full_scale
 * @return int 0 on success, negative error code on failure
 */
static int max40109_code_to_value(struct sensor_value *val, int16_t code, int32_t full_scale,
				  int32_t offset)
{
	int64_t micro = ((int64_t)offset * 1000) +
			(((int64_t)code * full_scale * 1000) / MAX40109_CODE_FULL_SCALE);

	return sensor_value_from_micro(val, micro);
}

static int max40109_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct max40109_config *config = dev->config;
	struct max40109_data *data = dev->data;
	int16_t code;

	switch ((int)chan) {
	case SENSOR_CHAN_PRESS:
		if (config->pressure_full_scale == 0) {
			return -ENOTSUP;
		}
		return max40109_code_to_value(val, data->calibrated_pressure,
					      config->pressure_full_scale, config->pressure_offset);
	case SENSOR_CHAN_AMBIENT_TEMP:
		if (config->temperature_full_scale == 0) {
			return -ENOTSUP;
		}
		return max40109_code_to_value(val, data->calibrated_temperature,
					      config->temperature_full_scale,
					      config->temperature_offset);
	case SENSOR_CHAN_MAX40109_PRESS_RAW:
		code = data->uncalibrated_pressure;
		break;
	case SENSOR_CHAN_MAX40109_AMBIENT_TEMP_RAW:
		code = data->uncalibrated_temperature;
		break;
	case SENSOR_CHAN_MAX40109_PRESS_COMPENSATED:
		code = data->calibrated_pressure;
		break;
	case SENSOR_CHAN_MAX40109_AMBIENT_TEMP_COMPENSATED:
		code = data->calibrated_temperature;
		break;
	default:
		return -ENOTSUP;
	}

	val->val1 = code;
	val->val2 = 0;

	return 0;
}

static int max40109_init(const struct device *dev)
{
	const struct max40109_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	/* Reset the STATUS Register */
	int ret = 0;
	uint8_t status_reg[2];

	ret = max40109_probe(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize the sensor (%d)", ret);
		return ret;
	}

	ret = max40109_reg_read(dev, MAX40109_REG_STATUS_MSB, status_reg, 2);
	if (ret < 0) {
		return ret;
	}

	ret = max40109_reg_write_multiple(dev, MAX40109_REG_STATUS_MSB, status_reg, 2);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_MAX40109_TRIGGER
	if (config->interrupt_gpio.port != NULL) {
		if (max40109_init_interrupt(dev) < 0) {
			LOG_ERR("Failed to initialize interrupts");
			return -EIO;
		}
	}
#endif

	LOG_INF("MAX40109 sensor initialized successfully");
	return 0;
}

static DEVICE_API(sensor, max40109_driver_api) = {
	.attr_set = max40109_attr_set,
	.attr_get = max40109_attr_get,
	.sample_fetch = max40109_sample_fetch,
	.channel_get = max40109_channel_get,
#ifdef CONFIG_MAX40109_TRIGGER
	.trigger_set = max40109_trigger_set,
#endif
};

#ifdef CONFIG_MAX40109_CALIBRATION

#define DT_INST_CALIB_COEFF_OR_NAN(inst, prop)                                                     \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, prop),                                             \
		    ((float)DT_INST_PROP(inst, prop) / 1000.0f), (NAN))

/** Coefficients left out of devicetree stay NAN and are skipped at init. */
#define MAX40109_CALIB_COEFF_CONFIG(inst)                                                          \
	.calib_coeff = {                                                                           \
		DT_INST_CALIB_COEFF_OR_NAN(inst, k0), DT_INST_CALIB_COEFF_OR_NAN(inst, k1),        \
		DT_INST_CALIB_COEFF_OR_NAN(inst, k2), DT_INST_CALIB_COEFF_OR_NAN(inst, k3),        \
		DT_INST_CALIB_COEFF_OR_NAN(inst, h0), DT_INST_CALIB_COEFF_OR_NAN(inst, h1),        \
		DT_INST_CALIB_COEFF_OR_NAN(inst, h2), DT_INST_CALIB_COEFF_OR_NAN(inst, h3),        \
		DT_INST_CALIB_COEFF_OR_NAN(inst, g0), DT_INST_CALIB_COEFF_OR_NAN(inst, g1),        \
		DT_INST_CALIB_COEFF_OR_NAN(inst, g2), DT_INST_CALIB_COEFF_OR_NAN(inst, g3),        \
		DT_INST_CALIB_COEFF_OR_NAN(inst, n0), DT_INST_CALIB_COEFF_OR_NAN(inst, n1),        \
		DT_INST_CALIB_COEFF_OR_NAN(inst, n2), DT_INST_CALIB_COEFF_OR_NAN(inst, n3),        \
		DT_INST_CALIB_COEFF_OR_NAN(inst, m0), DT_INST_CALIB_COEFF_OR_NAN(inst, m1),        \
		DT_INST_CALIB_COEFF_OR_NAN(inst, m2), DT_INST_CALIB_COEFF_OR_NAN(inst, m3)},

#else

#define MAX40109_CALIB_COEFF_CONFIG(inst)

#endif /* CONFIG_MAX40109_CALIBRATION */

#define MAX40109_CONFIG(inst)                                                                      \
	.digital_filter_setup = DT_INST_PROP_OR(inst, digital_filter, UINT8_MAX),                  \
	.alert_mode = DT_INST_PROP_OR(inst, alert_mode, UINT8_MAX),                                \
	.temp_current = DT_INST_PROP_OR(inst, temp_current, UINT8_MAX),                            \
	.pga_pressure_gain = DT_INST_PROP_OR(inst, pga_pressure_gain, UINT8_MAX),                  \
	.current_source = DT_INST_PROP_OR(inst, current_source, UINT8_MAX),                        \
	.adc_sample_rate = DT_INST_PROP_OR(inst, adc_sample_rate, UINT8_MAX),                      \
	.pga_temperature_gain = DT_INST_PROP_OR(inst, pga_temperature_gain, UINT8_MAX),            \
	.bridge_drive = DT_INST_PROP_OR(inst, bridge_drive, UINT8_MAX),                            \
	.temp_mode = DT_INST_PROP_OR(inst, temp_mode, UINT8_MAX),                                  \
	.drv_scale = DT_INST_PROP_OR(inst, drv_scale, UINT8_MAX),                                  \
	.analog_filter_bw = DT_INST_PROP_OR(inst, analog_filter_bw, UINT8_MAX),                    \
	.analog_output_stage = DT_INST_PROP_OR(inst, analog_output_stage, UINT8_MAX),              \
	.pressure_full_scale = DT_INST_PROP_OR(inst, pressure_full_scale_pascal, 0),               \
	.pressure_offset = DT_INST_PROP(inst, pressure_offset_pascal),                             \
	.temperature_full_scale = DT_INST_PROP_OR(inst, temperature_full_scale_millicelsius, 0),   \
	.temperature_offset = DT_INST_PROP(inst, temperature_offset_millicelsius),                 \
	MAX40109_CALIB_COEFF_CONFIG(inst)

/* The initializer is assembled from macros that expand to nothing when their feature is
 * disabled, so the element separators live inside those macros rather than between them.
 */
/* clang-format off */
#define MAX40109_DEFINE(inst)                                                                      \
	static struct max40109_data max40109_prv_data_##inst;                                      \
	static const struct max40109_config max40109_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		MAX40109_CONFIG(inst)                                                              \
		IF_ENABLED(CONFIG_MAX40109_TRIGGER, (.interrupt_gpio = GPIO_DT_SPEC_INST_GET_OR(   \
			inst, interrupt_gpios, {0}))) };                                           \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &max40109_init, NULL, &max40109_prv_data_##inst,        \
				     &max40109_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &max40109_driver_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(MAX40109_DEFINE)
