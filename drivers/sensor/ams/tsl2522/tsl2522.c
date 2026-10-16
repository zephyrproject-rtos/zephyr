/*
 * Copyright (c) 2026 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Driver for the TSL2522, an ambient light sensor from ams OSRAM that integrates both
 * photopic and infrared photodiodes with peak wavelengths of 560nm and 880nm. The sensor supports
 * dual concurrent light sensing channels, invisible ALS operation under any glass type,
 * programmable gain and integration time, and a wide 4096x dynamic range, with detection
 * capabilities down to 1mlux.
 *
 * This driver configures the TSL2522 to sample continuously without a waiting period after
 * sampling. A SENSOR_TRIG_DATA_READY handler can be registered to get a notification when a
 * conversion has been performed. A SENSOR_TRIG_OVERFLOW handler can also be registered to receive
 * notifications when either an analog or digital saturation occurred in one of the modulators.
 */

#define DT_DRV_COMPAT ams_tsl2522

#include "tsl2522.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(tsl2522, CONFIG_SENSOR_LOG_LEVEL);

#define FIXED_ATTENUATION_TO_DBL(x)  ((x) / 100000.0)
#define STEP_TIME_IN_US              (1.388889)
#define FROM_TIME_STEPS_TO_MS_DBL(x) ((double)(x) * STEP_TIME_IN_US / 1000.0)
#define COEFA_L                      (0.6132)
#define COEFB_L                      (-0.1557)
#define DGF_L                        (2.4529)
#define COEFA_H                      (0.6099)
#define COEFB_H                      (-0.1133)
#define DGF_H                        (2.4398)

/*
 * Lux = DGFn * (CoefAn * Ch0 + CoefBn * Ch1) / (ATime * AGain)   (information from ams OSRAM)
 *   n = 1 (L) if IR / PHO < 1.074, else n = 2 (H)
 *            n=1 (L)   n=2 (H)
 *   CoefA    0.6132    0.6099
 *   CoefB   -0.1557   -0.1133
 *   DGF      2.4529    2.4398
 *   ATime in [ms], Ch0: photopic, Ch1: IR
 */

static int calc_lux(const struct tsl2522_dts_config *cfg, const struct tsl2522_measurement *m,
		    struct sensor_value *val)
{
	double pho = (double)m->photopic_channel * FIXED_ATTENUATION_TO_DBL(cfg->glass_attenuation);
	double ir = (double)m->ir_channel * FIXED_ATTENUATION_TO_DBL(cfg->glass_ir_attenuation);
	double denominator = m->atime_ms * m->gain;
	double lux;

	if (denominator <= 0.0) {
		return -ENODATA;
	}

	if (ir < pho * 1.074) {
		lux = DGF_L * (COEFA_L * pho + COEFB_L * ir) / denominator;
	} else {
		lux = DGF_H * (COEFA_H * pho + COEFB_H * ir) / denominator;
	}

	return sensor_value_from_double(val, MAX(lux, 0.0));
}

/*
 * IR channel: returns the normalized count rate in counts / (ms * gain), corrected by the
 * glass attenuation. Not a photometric unit, the value is only proportional to the IR
 * irradiance.
 */
static int calc_ir(const struct tsl2522_dts_config *cfg, const struct tsl2522_measurement *m,
		   struct sensor_value *val)
{
	double ir = (double)m->ir_channel * FIXED_ATTENUATION_TO_DBL(cfg->glass_ir_attenuation);
	double denominator = m->atime_ms * m->gain;

	if (denominator <= 0.0) {
		return -ENODATA;
	}

	return sensor_value_from_double(val, ir / denominator);
}

static double tsl2522_convert_gain_enum_to_value(enum sensor_gain_tsl2522 again)
{
	if (again >= TSL2522_GAIN_MOD_1X && again <= TSL2522_GAIN_MOD_4096X) {
		return (double)(1U << (again - 1));
	} else if (again == TSL2522_GAIN_MOD_HALF) {
		return 0.5;
	}
	LOG_ERR("Invalid again value: %d!", again);
	return -1.0;
}

static void log_state(const struct tsl2522_status_regs *status_registers)
{
	LOG_DBG("status2 (0x%02x): als data valid %d, dig sat %d, flicker det sat %d, mod sat1 %d, "
		"mod sat0 %d",
		status_registers->status2,
		(bool)FIELD_GET(TSL2522_STATUS2_ALS_DATA_VALID, status_registers->status2),
		(bool)FIELD_GET(TSL2522_STATUS2_ALS_DIG_SAT, status_registers->status2),
		(bool)FIELD_GET(TSL2522_STATUS2_ALS_FD_DIG_SAT, status_registers->status2),
		(bool)FIELD_GET(TSL2522_STATUS2_MOD_ANA_SAT1, status_registers->status2),
		(bool)FIELD_GET(TSL2522_STATUS2_MOD_ANA_SAT0, status_registers->status2));
	LOG_DBG("status3 (0x%02x): aint hyst state valid %d, aint hyst state read %d, aint high %d,"
		" aint low thres %d, osc cal sat %d, osc cal finished %d",
		status_registers->status3,
		(bool)FIELD_GET(TSL2522_STATUS3_AINT_HYST_STATE_VALID, status_registers->status3),
		(bool)FIELD_GET(TSL2522_STATUS3_AINT_HYST_STATE_RD, status_registers->status3),
		(bool)FIELD_GET(TSL2522_STATUS3_AINT_AIHT, status_registers->status3),
		(bool)FIELD_GET(TSL2522_STATUS3_AINT_AILT, status_registers->status3),
		(bool)FIELD_GET(TSL2522_STATUS3_OSC_CALIB_SATURATION, status_registers->status3),
		(bool)FIELD_GET(TSL2522_STATUS3_OSC_CALIB_FINISHED, status_registers->status3));
	LOG_DBG("status4 (0x%02x): mod sample trigger error %d, mod trigger error %d, sai_active "
		"%d, init busy %d",
		status_registers->status4,
		(bool)FIELD_GET(TSL2522_STATUS4_MOD_SAMPLE_TRIGGER_ERROR,
				status_registers->status4),
		(bool)FIELD_GET(TSL2522_STATUS4_MOD_TRIGGER_ERROR, status_registers->status4),
		(bool)FIELD_GET(TSL2522_STATUS4_SAI_ACTIVE, status_registers->status4),
		(bool)FIELD_GET(TSL2522_STATUS4_INIT_BUSY, status_registers->status4));
	LOG_DBG("status5 (0x%02x): meas seq sys int %d, vsync lost %d", status_registers->status5,
		(bool)FIELD_GET(TSL2522_STATUS5_SINT_MEASUREMENT_SEQUENCER,
				status_registers->status5),
		(bool)FIELD_GET(TSL2522_STATUS5_SINT_VSYNC, status_registers->status5));
}

static void clear_modulator_saturation(const struct device *dev)
{
	const struct tsl2522_dts_config *cfg = dev->config;
	(void)i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_STATUS, TSL2522_STATUS_MINT);
}

static int enable_ambient_light_sensing(const struct device *dev)
{
	const struct tsl2522_dts_config *cfg = dev->config;

	/* Enable ALS and device. */
	return i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_ENABLE,
				     TSL2522_ENABLE_PON | TSL2522_ENABLE_AEN);
}

static int disable_ambient_light_sensing(const struct device *dev)
{
	const struct tsl2522_dts_config *cfg = dev->config;

	/* Disable ALS and device. */
	return i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_ENABLE, TSL2522_ENABLE_DISABLE);
}

static int internal_sample_fetch(const struct device *dev)
{
	int rc = 0;
	const struct tsl2522_dts_config *cfg = dev->config;
	struct tsl2522_data *data = dev->data;
	struct tsl2522_status_regs status_registers;
	bool als_data_valid = false;
	bool measured_data_valid = false;
	bool saturation_detected = false;

	/* Read the status fields 2...5 in one read. */
	rc = i2c_burst_read_dt(&cfg->i2c, TSL2522_REG_STATUS2, (uint8_t *)&status_registers,
			       sizeof(status_registers));
	if (rc < 0) {
		goto exit;
	}

	als_data_valid = (bool)FIELD_GET(TSL2522_STATUS2_ALS_DATA_VALID, status_registers.status2);
	measured_data_valid = !(bool)FIELD_GET(TSL2522_STATUS4_MOD_SAMPLE_TRIGGER_ERROR,
					       status_registers.status4);

	log_state(&status_registers);

	if (als_data_valid && measured_data_valid) {
		struct {
			uint8_t status;
			uint8_t data0[2];
			uint8_t data1[2];
			uint8_t reserved[2];
			uint8_t status2;
		} als_regs;

		BUILD_ASSERT(sizeof(als_regs) == 8);

		/* Fetch ALS data in one burst. */
		rc = i2c_burst_read_dt(&cfg->i2c, TSL2522_REG_ALS_STATUS, (uint8_t *)&als_regs,
				       sizeof(als_regs));
		if (rc < 0) {
			LOG_ERR("Could not read als-status registers!");
			goto exit;
		}

		LOG_DBG("als_status (0x%02x): seq step %lu, ana_sat_dat0 %d, ana_sat_dat1 %d, "
			"dat0_scaled %d, dat1_scaled %d",
			als_regs.status,
			FIELD_GET(TSL2522_ALS_STATUS_MEAS_SEQR_STEP, als_regs.status),
			(bool)FIELD_GET(TSL2522_ALS_STATUS_DATA0_ANA_SAT, als_regs.status),
			(bool)FIELD_GET(TSL2522_ALS_STATUS_DATA1_ANA_SAT, als_regs.status),
			(bool)FIELD_GET(TSL2522_ALS_STATUS_DATA0_SCALED, als_regs.status),
			(bool)FIELD_GET(TSL2522_ALS_STATUS_DATA1_SCALED, als_regs.status));

		saturation_detected =
#ifdef CONFIG_TSL2522_TRIGGER
			/* Use the information stored in the interrupt handling. */
			(bool)FIELD_GET(TSL2522_STATUS2_ALS_DIG_SAT, data->saved_status2) ||
			(bool)FIELD_GET(TSL2522_STATUS2_MOD_ANA_SAT1, data->saved_status2) ||
			(bool)FIELD_GET(TSL2522_STATUS2_MOD_ANA_SAT0, data->saved_status2) ||
#endif
			(bool)FIELD_GET(TSL2522_ALS_STATUS_DATA0_ANA_SAT, als_regs.status) ||
			(bool)FIELD_GET(TSL2522_ALS_STATUS_DATA1_ANA_SAT, als_regs.status) ||
			(bool)FIELD_GET(TSL2522_STATUS2_ALS_DIG_SAT, status_registers.status2) ||
			(bool)FIELD_GET(TSL2522_STATUS2_MOD_ANA_SAT1, status_registers.status2) ||
			(bool)FIELD_GET(TSL2522_STATUS2_MOD_ANA_SAT0, status_registers.status2);

#ifdef CONFIG_TSL2522_TRIGGER
		/* Clear it after it has been evaluated. */
		data->saved_status2 = 0;
#endif

		data->measurement.photopic_channel = (uint32_t)sys_get_le16(als_regs.data0);
		if (!FIELD_GET(TSL2522_ALS_STATUS_DATA0_SCALED, als_regs.status)) {
			data->measurement.photopic_channel = data->measurement.photopic_channel
							     << data->als_scale;
		}
		data->measurement.ir_channel = (uint32_t)sys_get_le16(als_regs.data1);
		if (!FIELD_GET(TSL2522_ALS_STATUS_DATA1_SCALED, als_regs.status)) {
			data->measurement.ir_channel = data->measurement.ir_channel
						       << data->als_scale;
		}

		data->measurement.atime_ms = FROM_TIME_STEPS_TO_MS_DBL(
			data->number_of_samples * data->measurement_time_steps);
		/* Take the gain for the converted data from the device. */
		data->measurement.gain = tsl2522_convert_gain_enum_to_value(
			FIELD_GET(TSL2522_ALS_STATUS2_DATA0_GAIN, als_regs.status2));
	} else if (!measured_data_valid) {
		LOG_ERR("Measured data corrupted!");
		/* Clear all bits in status4. */
		(void)i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_STATUS4,
					    status_registers.status4);
		/* When measured_data_valid is false we have to enable ALS again.
		 * See 10.2.21 in the datasheet: "Indicates that measured data is corrupted.
		 * For a valid measurement, this bit must not be asserted. This error
		 * condition does not trigger an interrupt, however AEN and FDEN will be
		 * cleared"
		 */
		rc = enable_ambient_light_sensing(dev);
		if (rc == 0) {
			rc = -ENODATA;
		} else {
			LOG_ERR("Could not re-enable light sensing!");
		}
	} else {
		LOG_DBG("als data not available!");
		rc = -EAGAIN;
	}

exit:
	if (rc == 0) {
		data->measurement.saturation = saturation_detected;
		if (saturation_detected) {
			clear_modulator_saturation(dev);
		}
	}

	return rc;
}

static int tsl2522_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int rc = 0;
	struct tsl2522_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_LIGHT &&
	    chan != SENSOR_CHAN_LIGHT && chan != SENSOR_CHAN_IR) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	rc = internal_sample_fetch(dev);

	k_mutex_unlock(&data->mutex);

	return rc;
}

static int tsl2522_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct tsl2522_data *data = dev->data;
	const struct tsl2522_dts_config *cfg = dev->config;
	int rc;

	k_mutex_lock(&data->mutex, K_FOREVER);

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_LIGHT:
	case SENSOR_CHAN_LIGHT:
		rc = calc_lux(cfg, &data->measurement, val);
		break;
	case SENSOR_CHAN_IR:
		rc = calc_ir(cfg, &data->measurement, val);
		break;
	default:
		rc = -ENOTSUP;
		break;
	}

	if (rc == 0 && data->measurement.saturation) {
		rc = -EOVERFLOW;
	}

	k_mutex_unlock(&data->mutex);

	return rc;
}

static int set_modulator_gain(const struct device *dev, enum sensor_gain_tsl2522 gain)
{
	const struct tsl2522_dts_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_MEAS_SEQR_STEP0_MOD_GAINX_L,
				     FIELD_PREP(TSL2522_MEAS_SEQR_STEP0_MOD_GAIN1, gain) |
					     FIELD_PREP(TSL2522_MEAS_SEQR_STEP0_MOD_GAIN0, gain));
}

static int set_measurement_time_ticks(const struct device *dev, uint16_t measurement_time_steps)
{
	const struct tsl2522_dts_config *cfg = dev->config;
	uint8_t sample_time_ticks[2];

	sys_put_le16(measurement_time_steps - 1U, sample_time_ticks);
	return i2c_burst_write_dt(&cfg->i2c, TSL2522_REG_SAMPLE_TIME0, sample_time_ticks,
				  sizeof(sample_time_ticks));
}

static int set_number_of_samples(const struct device *dev, uint16_t number_of_samples)
{
	const struct tsl2522_dts_config *cfg = dev->config;
	uint8_t nr_samples[2];

	sys_put_le16(number_of_samples - 1U, nr_samples);
	return i2c_burst_write_dt(&cfg->i2c, TSL2522_REG_ALS_NR_SAMPLES0, nr_samples,
				  sizeof(nr_samples));
}

static int tsl2522_attribute_get(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr, struct sensor_value *val)
{
	int rc = 0;
	struct tsl2522_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_LIGHT &&
	    chan != SENSOR_CHAN_LIGHT && chan != SENSOR_CHAN_IR) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	if (attr == SENSOR_ATTR_GAIN) {
		val->val1 = data->gain;
		val->val2 = 0;
	} else {
		switch ((enum sensor_attribute_tsl2522)attr) {
		case SENSOR_ATTR_MEASUREMENT_TIME_STEPS:
			val->val1 = data->measurement_time_steps;
			val->val2 = 0;
			break;
		case SENSOR_ATTR_NUMBER_OF_SAMPLES:
			val->val1 = data->number_of_samples;
			val->val2 = 0;
			break;
		default:
			rc = -ENOTSUP;
			break;
		}
	}
	k_mutex_unlock(&data->mutex);

	return rc;
}

static int tsl2522_attribute_set(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr, const struct sensor_value *val)
{
	int rc = 0;
	struct tsl2522_data *data = dev->data;
	bool enable_sensing = false;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_LIGHT &&
	    chan != SENSOR_CHAN_LIGHT && chan != SENSOR_CHAN_IR) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (attr == SENSOR_ATTR_GAIN) {
		if (IN_RANGE(val->val1, (int32_t)TSL2522_GAIN_MOD_HALF,
			     (int32_t)TSL2522_GAIN_MOD_4096X)) {
			enum sensor_gain_tsl2522 gain = (enum sensor_gain_tsl2522)val->val1;

			if (gain != data->gain) {
				rc = disable_ambient_light_sensing(dev);
				if (rc < 0) {
					goto unlock;
				}
				enable_sensing = true;
				rc = set_modulator_gain(dev, gain);
				if (rc == 0) {
					data->gain = gain;
				}
			}
		} else {
			rc = -EINVAL;
		}
	} else {
		switch ((enum sensor_attribute_tsl2522)attr) {
		case SENSOR_ATTR_MEASUREMENT_TIME_STEPS:
			if (IN_RANGE(val->val1, (int32_t)TSL2522_MEASUREMENT_TIME_STEPS_MIN,
				     (int32_t)TSL2522_MEASUREMENT_TIME_STEPS_MAX)) {
				uint16_t measurement_time_steps = (uint16_t)val->val1;

				if (measurement_time_steps != data->measurement_time_steps) {
					rc = disable_ambient_light_sensing(dev);
					if (rc < 0) {
						goto unlock;
					}
					enable_sensing = true;
					rc = set_measurement_time_ticks(dev,
									measurement_time_steps);
					if (rc == 0) {
						data->measurement_time_steps =
							measurement_time_steps;
					}
				}
			} else {
				rc = -EINVAL;
			}
			break;
		case SENSOR_ATTR_NUMBER_OF_SAMPLES:
			if (IN_RANGE(val->val1, (int32_t)TSL2522_NUMBER_OF_SAMPLES_MIN,
				     (int32_t)TSL2522_NUMBER_OF_SAMPLES_MAX)) {
				uint16_t number_of_samples = (uint16_t)val->val1;

				if (number_of_samples != data->number_of_samples) {
					rc = disable_ambient_light_sensing(dev);
					if (rc < 0) {
						goto unlock;
					}
					enable_sensing = true;
					rc = set_number_of_samples(dev, number_of_samples);
					if (rc == 0) {
						data->number_of_samples = number_of_samples;
					}
				}
			} else {
				rc = -EINVAL;
			}
			break;
		default:
			rc = -ENOTSUP;
			break;
		}
	}

	if (enable_sensing) {
		int rc2 = enable_ambient_light_sensing(dev);

		if (rc == 0) {
			rc = rc2;
		}
	}

unlock:
	k_mutex_unlock(&data->mutex);

	return rc;
}

static DEVICE_API(sensor, tsl2522_driver_api) = {
	.sample_fetch = tsl2522_sample_fetch,
	.channel_get = tsl2522_channel_get,
	.attr_get = tsl2522_attribute_get,
	.attr_set = tsl2522_attribute_set,
#ifdef CONFIG_TSL2522_TRIGGER
	.trigger_set = tsl2522_trigger_set,
#endif
};

static int reset_device(const struct device *dev)
{
	int rc = 0;
	const struct tsl2522_dts_config *cfg = dev->config;

	rc = i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_ENABLE, TSL2522_ENABLE_PON);
	if (rc < 0) {
		return rc;
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_CONTROL, TSL2522_CONTROL_SOFT_RESET);
	if (rc < 0) {
		return rc;
	}

	k_busy_wait(TSL2522_SOFTRESET_WAIT_US);

	return 0;
}

static int setup_device(const struct device *dev)
{
	int rc = 0;
	const struct tsl2522_dts_config *cfg = dev->config;
	struct tsl2522_data *data = dev->data;
	uint8_t measure_mode = 0;

	rc = i2c_reg_read_byte_dt(&cfg->i2c, TSL2522_REG_MEAS_MODE, &measure_mode);
	if (rc < 0) {
		return rc;
	}

	data->als_scale = FIELD_GET(TSL2522_MEAS_MODE_ALS_SCALE, measure_mode);
	if (data->als_scale != TSL2522_DEFAULT_ALS_SCALE) {
		LOG_ERR("We expect the default scaling from the device in all calculations.");
		return -ENOTSUP;
	}

	rc = set_measurement_time_ticks(dev, data->measurement_time_steps);
	if (rc < 0) {
		return rc;
	}

	rc = set_number_of_samples(dev, data->number_of_samples);
	if (rc < 0) {
		return rc;
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_TRIGGER_MODE,
				   TSL2522_TRIGGER_MODE_NORMAL);
	if (rc < 0) {
		return rc;
	}

	rc = set_modulator_gain(dev, cfg->gain_enum);
	if (rc < 0) {
		return rc;
	}
	data->gain = cfg->gain_enum;

	/* Assign photopic diodes to modulator 0 and the IR diodes to modulator 1. */
	rc = i2c_reg_write_byte_dt(
		&cfg->i2c, TSL2522_REG_MEAS_SEQR_STEP0_MOD_PHDX_SMUX_L,
		FIELD_PREP(TSL2522_MEAS_SEQR_STEP0_MOD_PHD3, TSL2522_MOD_SEL_MOD_0) |
			FIELD_PREP(TSL2522_MEAS_SEQR_STEP0_MOD_PHD2, TSL2522_MOD_SEL_MOD_0) |
			FIELD_PREP(TSL2522_MEAS_SEQR_STEP0_MOD_PHD1, TSL2522_MOD_SEL_MOD_0) |
			FIELD_PREP(TSL2522_MEAS_SEQR_STEP0_MOD_PHD0, TSL2522_MOD_SEL_MOD_1));
	if (rc < 0) {
		return rc;
	}

	rc = i2c_reg_write_byte_dt(
		&cfg->i2c, TSL2522_REG_MEAS_SEQR_STEP0_MOD_PHDX_SMUX_H,
		FIELD_PREP(TSL2522_MEAS_SEQR_STEP0_MOD_PHD5, TSL2522_MOD_SEL_MOD_1) |
			FIELD_PREP(TSL2522_MEAS_SEQR_STEP0_MOD_PHD4, TSL2522_MOD_SEL_MOD_0));
	if (rc < 0) {
		return rc;
	}

	/* Enable ALS and device. */
	rc = enable_ambient_light_sensing(dev);
	if (rc < 0) {
		return rc;
	}

	uint8_t status4 = 0;

	rc = i2c_reg_read_byte_dt(&cfg->i2c, TSL2522_REG_STATUS4, &status4);
	if (rc < 0) {
		return rc;
	}

	/* Clear all bits in status4. */
	return i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_STATUS4, status4);
}

static int tsl2522_init(const struct device *dev)
{
	const struct tsl2522_dts_config *cfg = dev->config;
	struct tsl2522_data *data = dev->data;
	int rc = -EAGAIN;
	uint8_t devid = 0;

	k_mutex_init(&data->mutex);

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	/* Try to access device. */
	for (int i = 0; i < TSL2522_MAX_INIT_RETRY && rc != 0; i++) {
		rc = i2c_reg_read_byte_dt(&cfg->i2c, TSL2522_REG_DEVICE_ID, &devid);
		if (rc < 0) {
			k_busy_wait(TSL2522_RETRY_ACCESS_US);
		}
	}

	if (rc < 0) {
		return rc;
	}

	if (devid != TSL2522_DEVICE_ID) {
		LOG_ERR("Invalid chip ID (was 0x%02x, expected 0x%02x)", devid, TSL2522_DEVICE_ID);
		return -EIO;
	}

	rc = reset_device(dev);
	if (rc < 0) {
		return rc;
	}

	rc = setup_device(dev);
	if (rc < 0) {
		return rc;
	}

#ifdef CONFIG_TSL2522_TRIGGER
	rc = tsl2522_trigger_init(dev);
	if (rc < 0) {
		return rc;
	}
#endif

	return 0;
}

/* clang-format off */
#define TSL2522_DEFINE(inst)	\
	static const struct tsl2522_dts_config tsl2522_config_##inst = {	\
		.i2c = I2C_DT_SPEC_INST_GET(inst),	\
		.glass_attenuation = DT_INST_PROP(inst, glass_attenuation),	\
		.glass_ir_attenuation = DT_INST_PROP(inst, glass_ir_attenuation),	\
		.gain_enum = (enum sensor_gain_tsl2522)DT_INST_ENUM_IDX(inst, modulator_gain),	\
		IF_ENABLED(CONFIG_TSL2522_TRIGGER,	\
			(.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),))	\
	};	\
	\
	static struct tsl2522_data tsl2522_data_##inst = {	\
		.measurement_time_steps = (uint16_t)DT_INST_PROP(inst, measurement_time_steps),	\
		.number_of_samples = (uint16_t)DT_INST_PROP(inst, number_of_samples),	\
	};	\
	\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tsl2522_init, NULL, &tsl2522_data_##inst,	\
				     &tsl2522_config_##inst, POST_KERNEL,	\
				     CONFIG_SENSOR_INIT_PRIORITY, &tsl2522_driver_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(TSL2522_DEFINE)
