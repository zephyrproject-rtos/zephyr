/*
 * Copyright (c) 2026 A-Eberle <Daniel.Kampert@a-eberle.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Conversion between units and register codes of the ADE9000
 *        calibration registers.
 */

#include <math.h>

#include "ade9000.h"

#ifndef M_PI
#define ADE9000_PI 3.14159265358979323846
#else
#define ADE9000_PI M_PI
#endif

/* A gain factor of 1.0 in micro units. The factor is 1 + code * 2^-27 */
#define ADE9000_GAIN_ONE_MICRO 1000000

/* DSP rate of the phase compensation, f_DSP of data sheet equation 4 */
#define ADE9000_PHCAL_DSP_HZ 8000.0

/* Lower end of the phase compensation range, data sheet page 31 */
#define ADE9000_PHCAL_MIN_DEG -15.0

/**
 * @brief      Phase of one DSP sample at the line frequency, omega of data
 *             sheet equation 4.
 * @param dev  ADE9000 device
 * @return     Omega in rad
 */
static double ade9000_phcal_omega(const struct device *dev)
{
	const struct ade9000_data *data = dev->data;

	return (2.0 * ADE9000_PI * (double)data->line_frequency) / ADE9000_PHCAL_DSP_HZ;
}

int ade9000_gain_to_code(const struct sensor_value *val, int32_t *code)
{
	int64_t micro = sensor_value_to_micro(val) - ADE9000_GAIN_ONE_MICRO;
	int64_t raw;

	/* Limit first, so the multiplication below cannot overflow */
	if ((micro < (INT64_C(-16) * ADE9000_GAIN_ONE_MICRO)) ||
	    (micro >= (INT64_C(16) * ADE9000_GAIN_ONE_MICRO))) {
		return -EINVAL;
	}

	raw = (micro * (INT64_C(1) << 27)) / ADE9000_GAIN_ONE_MICRO;
	if ((raw < INT32_MIN) || (raw > INT32_MAX)) {
		return -EINVAL;
	}

	*code = (int32_t)raw;

	return 0;
}

int ade9000_gain_from_code(int32_t code, struct sensor_value *val)
{
	return sensor_value_from_micro(
		val, (((int64_t)code * ADE9000_GAIN_ONE_MICRO) / (INT64_C(1) << 27)) +
			     ADE9000_GAIN_ONE_MICRO);
}

int ade9000_phcal_to_code(const struct device *dev, const struct sensor_value *val, int32_t *code)
{
	const struct ade9000_data *data = dev->data;
	double omega = ade9000_phcal_omega(dev);
	double max_deg = (360.0 * (double)data->line_frequency) / ADE9000_PHCAL_DSP_HZ;
	double deg = sensor_value_to_double(val);
	double phi = (deg * ADE9000_PI) / 180.0;
	double factor;

	if ((deg < ADE9000_PHCAL_MIN_DEG) || (deg > max_deg)) {
		return -EINVAL;
	}

	factor = (sin(phi - omega) + sin(omega)) / sin((2.0 * omega) - phi);
	*code = (int32_t)lround(factor * (double)BIT(27));

	return 0;
}

int ade9000_phcal_from_code(const struct device *dev, int32_t code, struct sensor_value *val)
{
	double omega = ade9000_phcal_omega(dev);
	double factor = (double)code / (double)BIT(27);
	double a = (factor * sin(2.0 * omega)) + sin(omega);
	double b = (factor * cos(2.0 * omega)) + cos(omega);
	double c = CLAMP(sin(omega) / sqrt((a * a) + (b * b)), -1.0, 1.0);
	double phi = acos(c) - atan2(b, a);

	return sensor_value_from_double(val, (phi * 180.0) / ADE9000_PI);
}

int ade9000_mtthr_to_code(const struct device *dev, const struct sensor_value *val, uint32_t *code)
{
	const struct ade9000_config *cfg = dev->config;
	double raw;

	if (cfg->current_fs_ua == 0) {
		return -EINVAL;
	}

	raw = (sensor_value_to_double(val) * 1000000.0 * (double)ADE9000_DT_FULL_SCALE_RMS) /
	      (double)cfg->current_fs_ua;
	if ((raw < 0.0) || (raw > (double)INT32_MAX)) {
		return -EINVAL;
	}

	*code = (uint32_t)llround(raw);

	return 0;
}

int ade9000_mtthr_from_code(const struct device *dev, uint32_t code, struct sensor_value *val)
{
	const struct ade9000_config *cfg = dev->config;

	return sensor_value_from_double(val,
					((double)code * (double)cfg->current_fs_ua) /
						(1000000.0 * (double)ADE9000_DT_FULL_SCALE_RMS));
}
