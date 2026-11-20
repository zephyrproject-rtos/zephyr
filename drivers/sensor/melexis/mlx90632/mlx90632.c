/*
 * SPDX-FileCopyrightText: Copyright (c) Michele Tavecchio <tavecchiomichele03@gmail.com>
 * SPDX-FileCopyrightText: Copyright (c) Alessandro Sola <alessandrosola03@gmail.com>
 * SPDX-FileCopyrightText: Copyright (c) 2017 Melexis N.V.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT melexis_mlx90632

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/logging/log.h>

#include "mlx90632.h"
#include <math.h>

LOG_MODULE_REGISTER(MLX90632, CONFIG_SENSOR_LOG_LEVEL);

static int mlx90632_reg_read(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct mlx90632_config *config = dev->config;
	uint8_t reg_addr[2] = {reg >> 8, reg & 0xFF};
	uint8_t data[2];
	int ret;

	ret = i2c_write_read_dt(&config->i2c, reg_addr, sizeof(reg_addr), data, sizeof(data));
	if (ret < 0) {
		LOG_ERR("Failed to read register 0x%04x (rc=%d)", reg, ret);
		return ret;
	}
	*val = sys_get_be16(data);
	return 0;
}

static int mlx90632_reg_read_s16(const struct device *dev, uint16_t reg, int16_t *val)
{
	uint16_t raw;
	int ret = mlx90632_reg_read(dev, reg, &raw);

	if (ret < 0) {
		return ret;
	}

	*val = (int16_t)raw;
	return 0;
}

static uint16_t mlx90632_get_control_bits(const struct device *dev)
{
	const struct mlx90632_config *cfg = dev->config;

	switch (cfg->power_mode) {
	case MLX90632_POWER_SLEEPING_STEP:
		return MLX90632_CTRL_PWR_SLEEP_STEP_BITS;
	case MLX90632_POWER_STEP:
		return MLX90632_CTRL_PWR_STEP_BITS;
	case MLX90632_POWER_CONTINUOUS:
	default:
		return MLX90632_CTRL_PWR_CONTINUOUS_BITS;
	}
}

static int mlx90632_read_32bit_param(const struct device *dev, uint16_t addr, int32_t *dest)
{
	uint16_t ls, ms;
	int ret;

	ret = mlx90632_reg_read(dev, addr, &ls);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_reg_read(dev, addr + 1, &ms);
	if (ret < 0) {
		return ret;
	}

	*dest = (int32_t)((uint32_t)ms << 16 | ls);
	return 0;
}

static int mlx90632_reg_write(const struct device *dev, uint16_t reg, uint16_t val)
{
	const struct mlx90632_config *config = dev->config;
	uint8_t tx_data[4];
	int ret;

	tx_data[0] = reg >> 8;
	tx_data[1] = reg & 0xFF;
	tx_data[2] = (val >> 8) & 0xFF;
	tx_data[3] = val & 0xFF;

	ret = i2c_write_dt(&config->i2c, tx_data, sizeof(tx_data));
	if (ret < 0) {
		LOG_ERR("Failed to write register 0x%04x (rc=%d)", reg, ret);
		return ret;
	}

	return 0;
}

static int mlx90632_load_calibration(const struct device *dev)
{
	struct mlx90632_data *data = dev->data;
	int ret;

	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_P_R, &data->p_r);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_P_G, &data->p_g);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_P_T, &data->p_t);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_P_O, &data->p_o);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Aa, &data->aa);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Ab, &data->ab);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Ba, &data->ba);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Bb, &data->bb);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Ca, &data->ca);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Cb, &data->cb);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Da, &data->da);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Db, &data->db);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Ea, &data->ea);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Eb, &data->eb);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Fa, &data->fa);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Fb, &data->fb);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_read_32bit_param(dev, MLX90632_EE_Ga, &data->ga);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_reg_read_s16(dev, MLX90632_EE_Gb, &data->gb);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_EE_Ka, &data->ka);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_EE_Ha, &data->ha);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_EE_Hb, &data->hb);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int mlx90632_set_refresh_rate(const struct device *dev)
{
	static const uint16_t medical_addrs[] = {
		MLX90632_EE_MEDICAL_MEAS1,
		MLX90632_EE_MEDICAL_MEAS2,
	};
	static const uint16_t extended_addrs[] = {
		MLX90632_EE_EXTENDED_MEAS1,
		MLX90632_EE_EXTENDED_MEAS2,
		MLX90632_EE_EXTENDED_MEAS3,
	};
	const struct mlx90632_config *cfg = dev->config;
	const uint16_t *addrs = cfg->extended_mode ? extended_addrs : medical_addrs;
	size_t num_regs =
		cfg->extended_mode ? ARRAY_SIZE(extended_addrs) : ARRAY_SIZE(medical_addrs);
	uint16_t current[ARRAY_SIZE(extended_addrs)];
	uint16_t target[ARRAY_SIZE(extended_addrs)];
	bool changed = false;
	uint16_t ctrl;
	int ret;

	for (size_t i = 0; i < num_regs; i++) {
		ret = mlx90632_reg_read(dev, addrs[i], &current[i]);
		if (ret < 0) {
			return ret;
		}

		target[i] = (current[i] & ~MLX90632_EE_RATE_MASK) |
			    FIELD_PREP(MLX90632_EE_RATE_MASK, cfg->refresh_rate);
		changed |= current[i] != target[i];
	}

	if (!changed) {
		return 0;
	}

	ret = mlx90632_reg_read(dev, MLX90632_REG_CTRL, &ctrl);
	if (ret < 0) {
		return ret;
	}

	ctrl = (ctrl & ~MLX90632_CTRL_PWR_MASK) | MLX90632_CTRL_PWR_HALT_BITS;
	ret = mlx90632_reg_write(dev, MLX90632_REG_CTRL, ctrl);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < num_regs; i++) {
		if (current[i] == target[i]) {
			continue;
		}

		ret = mlx90632_reg_write(dev, MLX90632_REG_UNLOCK, MLX90632_EE_UNLOCK_KEY);
		if (ret < 0) {
			return ret;
		}

		ret = mlx90632_reg_write(dev, addrs[i], 0U);
		if (ret < 0) {
			return ret;
		}

		k_msleep(MLX90632_EE_WRITE_TIME_MS);

		ret = mlx90632_reg_write(dev, MLX90632_REG_UNLOCK, MLX90632_EE_UNLOCK_KEY);
		if (ret < 0) {
			return ret;
		}

		ret = mlx90632_reg_write(dev, addrs[i], target[i]);
		if (ret < 0) {
			return ret;
		}

		k_msleep(MLX90632_EE_WRITE_TIME_MS);
	}

	ret = mlx90632_reg_write(dev, MLX90632_REG_UNLOCK, MLX90632_RESET_CMD);
	if (ret < 0) {
		return ret;
	}

	k_msleep(MLX90632_RESET_TIME_MS);
	return 0;
}

static int mlx90632_wait_for_data(const struct device *dev, uint16_t *status)
{
	/* Datasheet table 11 includes the extended sequence's rounding at high rates. */
	static const uint16_t extended_time_ms[] = {6000U, 3000U, 1500U, 750U,
						    375U,  200U,  100U,  50U};
	const struct mlx90632_config *cfg = dev->config;
	const struct mlx90632_data *data = dev->data;
	bool burst = cfg->power_mode != MLX90632_POWER_CONTINUOUS;
	uint32_t sequence_ms = cfg->extended_mode ? extended_time_ms[cfg->refresh_rate]
						  : DIV_ROUND_UP(2U * MLX90632_MEAS_MAX_TIME_MS,
								 BIT(cfg->refresh_rate));
	int64_t deadline = k_uptime_get() + sequence_ms + MLX90632_READY_MARGIN_MS;
	int ret;

	if (burst) {
		k_msleep(sequence_ms);
	}

	while (true) {
		ret = mlx90632_reg_read(dev, MLX90632_REG_STATUS, status);
		if (ret < 0) {
			return ret;
		}

		if ((*status & MLX90632_STAT_DATA_RDY) != 0U) {
			uint8_t cycle = FIELD_GET(MLX90632_STAT_CYCLE_MASK, *status);

			if (!cfg->extended_mode && cycle != 1U && cycle != 2U) {
				return -EINVAL;
			}

			if (burst) {
				if ((*status & MLX90632_STAT_BUSY) == 0U) {
					return 0;
				}
			} else if (cfg->extended_mode) {
				if (cycle == 19U) {
					return 0;
				}
			} else if (data->last_cycle != 0U || cycle == 2U) {
				return 0;
			}
		}

		int64_t remaining = deadline - k_uptime_get();

		if (remaining <= 0) {
			LOG_ERR("Timeout waiting for sensor data");
			return -ETIMEDOUT;
		}

		k_msleep(MIN(remaining, MLX90632_POLL_INTERVAL_MS));
	}
}

static int mlx90632_configure_measurements(const struct device *dev)
{
	const struct mlx90632_config *cfg = dev->config;
	uint16_t type = cfg->extended_mode ? MLX90632_MEAS_EXTENDED : MLX90632_MEAS_MEDICAL;
	uint16_t control;
	int ret;

	ret = mlx90632_reg_read(dev, MLX90632_REG_CTRL, &control);
	if (ret < 0) {
		return ret;
	}

	if ((control & MLX90632_CTRL_MEAS_MASK) != type) {
		if ((control & MLX90632_CTRL_PWR_MASK) == MLX90632_CTRL_PWR_SLEEP_STEP_BITS) {
			control = (control & ~MLX90632_CTRL_PWR_MASK) | MLX90632_CTRL_PWR_STEP_BITS;
			ret = mlx90632_reg_write(dev, MLX90632_REG_CTRL, control);
			if (ret < 0) {
				return ret;
			}
		}

		ret = mlx90632_reg_write(dev, MLX90632_REG_UNLOCK, MLX90632_RESET_CMD);
		if (ret < 0) {
			return ret;
		}
		k_busy_wait(MLX90632_RESET_DELAY_US);

		ret = mlx90632_reg_read(dev, MLX90632_REG_CTRL, &control);
		if (ret < 0) {
			return ret;
		}

		control &= ~(MLX90632_CTRL_PWR_MASK | MLX90632_CTRL_MEAS_MASK);
		control |= MLX90632_CTRL_PWR_HALT_BITS | type;
		ret = mlx90632_reg_write(dev, MLX90632_REG_CTRL, control);
		if (ret < 0) {
			return ret;
		}

		ret = mlx90632_reg_read(dev, MLX90632_REG_CTRL, &control);
		if (ret < 0) {
			return ret;
		}
	}

	control = (control & ~MLX90632_CTRL_PWR_MASK) | mlx90632_get_control_bits(dev);
	return mlx90632_reg_write(dev, MLX90632_REG_CTRL, control);
}

static int mlx90632_init(const struct device *dev)
{
	const struct mlx90632_config *cfg = dev->config;
	uint8_t dsp_version;
	uint16_t ee_version;
	uint16_t reg_status;
	uint16_t dummy;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	k_msleep(MLX90632_STARTUP_TIME_MS);

	/* dummy read required for 1V8 I2C communication variant */
	(void)mlx90632_reg_read(dev, MLX90632_REG_STATUS, &dummy);

	ret = mlx90632_reg_read(dev, MLX90632_EE_VERSION, &ee_version);
	if (ret < 0) {
		LOG_ERR("Device not responding at 0x%02x (rc=%d)", cfg->i2c.addr, ret);
		return -ENODEV;
	}

	dsp_version = FIELD_GET(MLX90632_EE_VERSION_MASK, ee_version);
	if (dsp_version != MLX90632_DSPv5) {
		LOG_ERR("Unsupported DSP version 0x%02x", (unsigned int)dsp_version);
		return -ENOTSUP;
	}

	ret = mlx90632_load_calibration(dev);
	if (ret < 0) {
		return ret;
	}

	int was_reset = mlx90632_set_refresh_rate(dev);

	if (was_reset < 0) {
		return was_reset;
	}

	ret = mlx90632_configure_measurements(dev);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_reg_read(dev, MLX90632_REG_STATUS, &reg_status);
	if (ret < 0) {
		return ret;
	}

	reg_status |= MLX90632_STAT_BROWN_OUT;
	reg_status &= ~MLX90632_STAT_DATA_RDY;

	ret = mlx90632_reg_write(dev, MLX90632_REG_STATUS, reg_status);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int mlx90632_trigger_measurement(const struct device *dev)
{
	uint16_t reg_ctrl;
	uint16_t status;
	int ret;

	ret = mlx90632_reg_read(dev, MLX90632_REG_STATUS, &status);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_reg_write(dev, MLX90632_REG_STATUS, status & ~MLX90632_STAT_DATA_RDY);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_reg_read(dev, MLX90632_REG_CTRL, &reg_ctrl);
	if (ret < 0) {
		return ret;
	}

	reg_ctrl |= MLX90632_BIT_SOB;
	return mlx90632_reg_write(dev, MLX90632_REG_CTRL, reg_ctrl);
}

static uint32_t int_sqrt(uint64_t x)
{
	uint64_t res = 0;
	uint64_t bit = 1ULL << 62;

	while (bit > x) {
		bit >>= 2;
	}

	while (bit != 0) {
		if (x >= res + bit) {
			x -= res + bit;
			res = (res >> 1) + bit;
		} else {
			res >>= 1;
		}
		bit >>= 2;
	}
	return (uint32_t)res;
}

static int mlx90632_read_extended_channels(const struct device *dev)
{
	struct mlx90632_data *data = dev->data;
	int ret;

	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_52, &data->ram_52);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_53, &data->ram_53);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_54, &data->ram_54);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_55, &data->ram_55);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_56, &data->ram_56);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_57, &data->ram_57);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_58, &data->ram_58);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_59, &data->ram_59);
	if (ret < 0) {
		return ret;
	}
	ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_60, &data->ram_60);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/*
 * Compute (a * b) / divisor by splitting the multiplication to limit
 * intermediate overflow. The division truncates toward zero. Returns
 * -EINVAL for a non-positive divisor and -ERANGE if a checked intermediate
 * overflows or the result magnitude exceeds INT64_MAX.
 */
static int mlx90632_mul_div(int64_t a, int64_t b, int64_t divisor, int64_t *result)
{
	bool negative = (a < 0) != (b < 0);
	uint64_t ua = a < 0 ? (uint64_t)(-(a + 1)) + 1U : (uint64_t)a;
	uint64_t ub = b < 0 ? (uint64_t)(-(b + 1)) + 1U : (uint64_t)b;
	uint64_t whole;
	uint64_t fraction;
	uint64_t magnitude;

	if (divisor <= 0) {
		return -EINVAL;
	}

	if (u64_mul_overflow(ua, ub / divisor, &whole) ||
	    u64_mul_overflow(ua, ub % divisor, &fraction) ||
	    u64_add_overflow(whole, fraction / divisor, &magnitude) ||
	    magnitude > (uint64_t)INT64_MAX) {
		return -ERANGE;
	}

	*result = negative ? -(int64_t)magnitude : (int64_t)magnitude;
	return 0;
}

/*
 * Normalize the ambient and object signals using the pre-calculations in
 * datasheet sections 11.1.1.1/11.1.1.2 and 11.2.1.1/11.2.1.2.
 */
static int mlx90632_normalize(int16_t ambient_new, int16_t ambient_old, int16_t coefficient,
			      int32_t twice_signal, int64_t *result)
{
	int64_t vr = (int64_t)ambient_old * (12LL * 1024LL) + (int64_t)coefficient * ambient_new;

	if (vr <= 0) {
		return -ERANGE;
	}

	*result = ((int64_t)twice_signal * ((1LL << 28) * 1000LL)) / vr;
	if (*result < INT32_MIN || *result > INT32_MAX) {
		return -ERANGE;
	}

	return 0;
}

/*
 * Compute ambient temperature from normalized AMB using the MLX90632
 * datasheet equations in sections 11.1.2 (medical) and 11.2.2 (extended).
 * Both `ambient` and `*temperature` are scaled by 1000.
 */
static int mlx90632_calc_ambient(struct mlx90632_data *data, int64_t ambient, int32_t *temperature)
{
	int64_t delta = ambient - (int64_t)data->p_r * 1000LL / 256LL;
	int64_t quadratic;
	int64_t linear;
	int64_t sum;
	int ret;

	if (data->p_g <= 0) {
		return -EINVAL;
	}

	ret = mlx90632_mul_div(data->p_t, delta, 1LL << 24, &quadratic);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_mul_div(quadratic, delta, (1LL << 20) * 1000LL, &quadratic);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_mul_div(delta, 1LL << 20, data->p_g, &linear);
	if (ret < 0) {
		return ret;
	}

	if (quadratic < INT32_MIN || quadratic > INT32_MAX || linear < INT32_MIN ||
	    linear > INT32_MAX) {
		return -ERANGE;
	}

	sum = quadratic + linear + (int64_t)data->p_o * 1000LL / 256LL;
	if (sum < INT32_MIN || sum > INT32_MAX) {
		return -ERANGE;
	}

	*temperature = (int32_t)sum;
	return 0;
}

/*
 * Datasheet rev. 13, sections 11.1.3 and 11.2.3; Melexis
 * mlx90632_calc_temp_object_iteration().
 */
static int mlx90632_calc_object(struct mlx90632_data *data, int64_t ambient, int64_t object,
				bool extended, int32_t *temperature)
{
	int64_t t_adut;
	int64_t alpha_base;
	int64_t alpha;
	int64_t ga;
	int64_t fb;
	int64_t correction;
	int64_t radiance;
	int64_t estimate = 25000;
	int64_t kelvin;
	uint64_t ta4;
	uint64_t radicand;
	int ret;

	if (data->ea <= 0 || data->fa <= 0 || data->ha <= 0) {
		return -EINVAL;
	}

	ret = mlx90632_mul_div(ambient * 256LL - (int64_t)data->eb * 1000LL, 256000LL, data->ea,
			       &t_adut);
	if (ret < 0) {
		return ret;
	}
	t_adut += 25000000LL;
	if (t_adut <= -273150000LL || t_adut > 382200000LL) {
		return -ERANGE;
	}

	kelvin = t_adut + 273150000LL;
	ta4 = ((uint64_t)kelvin * kelvin) / 100000000ULL;
	if (u64_mul_overflow(ta4, ta4, &ta4)) {
		return -ERANGE;
	}

	ret = mlx90632_mul_div(data->fa, data->ha, 1LL << 14, &alpha_base);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_mul_div(data->fb, t_adut - 25000000LL, 1LL << 36, &fb);
	if (ret < 0) {
		return ret;
	}

	for (uint8_t i = 0; i < 5U; i++) {
		ret = mlx90632_mul_div(data->ga, (estimate - 25000LL) * 1000LL, 1LL << 36, &ga);
		if (ret < 0) {
			return ret;
		}

		correction = 1000000LL + ga + fb;
		if (correction <= 0) {
			return -ERANGE;
		}

		ret = mlx90632_mul_div(alpha_base, correction, 1000000LL, &alpha);
		if (ret < 0) {
			return ret;
		}
		if (alpha <= 0) {
			return -ERANGE;
		}

		ret = mlx90632_mul_div(object * (extended ? 2LL : 1LL), (1LL << 46) * 100000LL,
				       alpha, &radiance);
		if (ret < 0) {
			return ret;
		}

		if (radiance < 0) {
			if ((uint64_t)-radiance >= ta4) {
				return -ERANGE;
			}
			radicand = ta4 - (uint64_t)-radiance;
		} else if (u64_add_overflow(ta4, (uint64_t)radiance, &radicand)) {
			return -ERANGE;
		}

		estimate = (int64_t)int_sqrt(int_sqrt(radicand)) * 10LL - 273150LL -
			   (int64_t)data->hb * 1000LL / 1024LL;
	}

	*temperature = (int32_t)estimate;
	return 0;
}

static int mlx90632_calculate_temperatures(struct mlx90632_data *data, bool extended)
{
	int16_t ambient_new = extended ? data->ram_54 : data->ram_6;
	int16_t ambient_old = extended ? data->ram_57 : data->ram_9;
	int32_t twice_signal;
	int32_t ambient_temperature;
	int32_t object_temperature;
	int64_t ambient;
	int64_t object;
	int ret;

	if (extended) {
		twice_signal = (int32_t)data->ram_52 - data->ram_53 - data->ram_55 + data->ram_56 +
			       2 * ((int32_t)data->ram_58 + data->ram_59);
	} else if (data->last_cycle == 1U) {
		twice_signal = (int32_t)data->ram_4 + data->ram_5;
	} else if (data->last_cycle == 2U) {
		twice_signal = (int32_t)data->ram_7 + data->ram_8;
	} else {
		return -EINVAL;
	}

	ret = mlx90632_normalize(ambient_new, ambient_old, data->gb, 2 * (int32_t)ambient_new,
				 &ambient);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_normalize(ambient_new, ambient_old, data->ka, twice_signal, &object);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_calc_ambient(data, ambient, &ambient_temperature);
	if (ret < 0) {
		return ret;
	}

	ret = mlx90632_calc_object(data, ambient, object, extended, &object_temperature);
	if (ret < 0) {
		return ret;
	}

	data->ambient_temp = ambient_temperature;
	data->object_temp = object_temperature;

	return 0;
}

static int mlx90632_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct mlx90632_config *cfg = dev->config;
	struct mlx90632_data *data = dev->data;
	uint16_t reg_status;
	int ret;

	ARG_UNUSED(chan);

	if (cfg->power_mode != MLX90632_POWER_CONTINUOUS) {
		ret = mlx90632_trigger_measurement(dev);
		if (ret < 0) {
			return ret;
		}
	}

	ret = mlx90632_wait_for_data(dev, &reg_status);
	if (ret < 0) {
		return ret;
	}

	uint8_t cycle_pos = FIELD_GET(MLX90632_STAT_CYCLE_MASK, reg_status);

	data->last_cycle = cycle_pos;

	if (cfg->extended_mode) {
		ret = mlx90632_read_extended_channels(dev);
		if (ret < 0) {
			return ret;
		}
	} else {
		ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_6, &data->ram_6);
		if (ret < 0) {
			return ret;
		}
		ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_9, &data->ram_9);
		if (ret < 0) {
			return ret;
		}
		if (cycle_pos == 1) {
			ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_4, &data->ram_4);
			if (ret < 0) {
				return ret;
			}
			ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_5, &data->ram_5);
			if (ret < 0) {
				return ret;
			}
		} else {
			ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_7, &data->ram_7);
			if (ret < 0) {
				return ret;
			}
			ret = mlx90632_reg_read_s16(dev, MLX90632_RAM_8, &data->ram_8);
			if (ret < 0) {
				return ret;
			}
		}
	}

	ret = mlx90632_reg_write(dev, MLX90632_REG_STATUS, reg_status & ~MLX90632_STAT_DATA_RDY);
	if (ret < 0) {
		return ret;
	}

	return mlx90632_calculate_temperatures(data, cfg->extended_mode);
}

static int mlx90632_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct mlx90632_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
		val->val1 = data->ambient_temp / 1000;
		val->val2 = (data->ambient_temp % 1000) * 1000;
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = data->object_temp / 1000;
		val->val2 = (data->object_temp % 1000) * 1000;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, mlx90632_driver_api) = {
	.sample_fetch = mlx90632_sample_fetch,
	.channel_get = mlx90632_channel_get,
};

#define MLX90632_DEFINE(inst)                                                                      \
	static struct mlx90632_data mlx90632_data_##inst;                                          \
	static const struct mlx90632_config mlx90632_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.power_mode = DT_INST_ENUM_IDX(inst, power_mode),                                  \
		.refresh_rate = DT_INST_PROP(inst, refresh_rate),                                  \
		.extended_mode = DT_INST_PROP(inst, extended_mode),                                \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, mlx90632_init, NULL, &mlx90632_data_##inst,             \
				     &mlx90632_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &mlx90632_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MLX90632_DEFINE)
