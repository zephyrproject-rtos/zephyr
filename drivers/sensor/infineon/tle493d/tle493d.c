/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tle493d

#include "tle493d_regs.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <assert.h>

LOG_MODULE_REGISTER(TLE493D, CONFIG_SENSOR_LOG_LEVEL);

#define TLE493D_ADDR_A0 0x35U
#define TLE493D_ADDR_A1 0x22U
#define TLE493D_ADDR_A2 0x78U
#define TLE493D_ADDR_A3 0x44U

struct tle493d_config {
	struct i2c_dt_spec i2c;
};

struct tle493d_data {
	uint8_t regs[6];
	struct sensor_value temp_gain;   /* in degrees Celsius per LSB */
	struct sensor_value temp_offset; /* in degrees Celsius */
};

static int tle493d_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	const struct tle493d_data *data = dev->data;

	if (val == NULL) {
		return -EINVAL;
	}

	if (chan == SENSOR_CHAN_AMBIENT_TEMP && attr == SENSOR_ATTR_GAIN) {
		*val = data->temp_gain;
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP && attr == SENSOR_ATTR_OFFSET) {
		*val = data->temp_offset;
	} else {
		return -ENOTSUP;
	}
	return 0;
}

static int tle493d_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	struct tle493d_data *data = dev->data;

	if (val == NULL) {
		return -EINVAL;
	}

	/* the temperature in C is calculated via
	 * temp = raw_value * temp_gain + temp_offset
	 *
	 * ranges from TLE493D-W2B6 DS Table 13:
	 *
	 * parameter              min     typ     max   unit
	 * temp_gain              0.21    0.24    0.27  K/LSB
	 * digital value at 25 C  1000    1180    1360  LSB
	 * temp_offset            -342.2  -258.2  -185  C      (derived from other parameter limits)
	 */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP && attr == SENSOR_ATTR_GAIN) {
		if (val->val1 != 0 || val->val2 < 210000 || val->val2 > 270000) {
			LOG_WRN("temp_gain %i.%06i out of datasheet range [0.21, 0.27]", val->val1,
				val->val2);
		}
		data->temp_gain = *val;
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP && attr == SENSOR_ATTR_OFFSET) {
		if (val->val1 < -342 || val->val1 > -185) {
			LOG_WRN("temp_offset %i.%06i out of datasheet range [-342, -185]",
				val->val1, val->val2);
		}
		data->temp_offset = *val;
	} else {
		return -ENOTSUP;
	}
	return 0;
}

static bool tle493d_channel_is_supported(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
	case SENSOR_CHAN_AMBIENT_TEMP:
		return true;
	default:
		return false;
	}
}

static int tle493d_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tle493d_config *config = dev->config;
	struct tle493d_data *data = dev->data;

	if (!tle493d_channel_is_supported(chan)) {
		return -ENOTSUP;
	}

	return i2c_read_dt(&config->i2c, data->regs, sizeof(data->regs));
}

static int32_t tle493d_signed_12bit_from_bits(uint8_t high_byte, uint8_t low_nibble)
{
	return ((int8_t)high_byte << 4) | (low_nibble & 0x0F);
}

static struct sensor_value tle493d_raw_magn_to_gauss(int32_t raw_val)
{
	/* resolution is 130 uT/LSB = 1.3 G/LSB */
	int32_t value = raw_val * 1300;

	return (struct sensor_value){.val1 = value / 1000, .val2 = (value % 1000) * 1000};
}

static int tle493d_raw_temp_to_celcius(int32_t raw_val, const struct sensor_value *temp_gain,
				       const struct sensor_value *temp_offset,
				       struct sensor_value *out)
{
	struct sensor_value temp = {.val1 = raw_val, .val2 = 0};
	int ret;

	ret = sensor_value_multiply(&temp, temp_gain, &temp);
	if (ret < 0) {
		return ret;
	}
	ret = sensor_value_add(&temp, temp_offset, &temp);
	if (ret < 0) {
		return ret;
	}
	*out = temp;
	return 0;
}

static int tle493d_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct tle493d_data *data = dev->data;

	if (chan == SENSOR_CHAN_ALL || !tle493d_channel_is_supported(chan)) {
		return -ENOTSUP;
	}

	const uint8_t *regs = data->regs;
	int32_t raw_magn_x = tle493d_signed_12bit_from_bits(regs[0], regs[4] >> 4);
	int32_t raw_magn_y = tle493d_signed_12bit_from_bits(regs[1], regs[4] & 0x0F);
	int32_t raw_magn_z = tle493d_signed_12bit_from_bits(regs[2], regs[5] & 0x0F);
	int32_t raw_temp = tle493d_signed_12bit_from_bits(regs[3], (regs[5] & 0xC0) >> 4);

	switch (chan) {
	case SENSOR_CHAN_MAGN_X: {
		*val = tle493d_raw_magn_to_gauss(raw_magn_x);
	} break;
	case SENSOR_CHAN_MAGN_Y: {
		*val = tle493d_raw_magn_to_gauss(raw_magn_y);
	} break;
	case SENSOR_CHAN_MAGN_Z: {
		*val = tle493d_raw_magn_to_gauss(raw_magn_z);
	} break;
	case SENSOR_CHAN_MAGN_XYZ: {
		val[0] = tle493d_raw_magn_to_gauss(raw_magn_x);
		val[1] = tle493d_raw_magn_to_gauss(raw_magn_y);
		val[2] = tle493d_raw_magn_to_gauss(raw_magn_z);
	} break;
	case SENSOR_CHAN_AMBIENT_TEMP: {
		return tle493d_raw_temp_to_celcius(raw_temp, &data->temp_gain, &data->temp_offset,
						   val);
	}
	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, tle493d_driver_api) = {
	.attr_get = tle493d_attr_get,
	.attr_set = tle493d_attr_set,
	.sample_fetch = tle493d_sample_fetch,
	.channel_get = tle493d_channel_get,
};

static void tle493d_reset(const struct device *dev)
{
	const struct tle493d_config *config = dev->config;
	uint8_t dummy;

	/* Reset by keeping SDA high then low.
	 * This is done by sending two 0xFF bytes followed by two 0x00 bytes.
	 */
	for (int i = 0; i < 4; i++) {
		struct i2c_msg msg = {
			.buf = &dummy,
			.len = 0,
			.flags = I2C_MSG_STOP | (i < 2 ? I2C_MSG_READ : I2C_MSG_WRITE),
		};
		(void)i2c_transfer(config->i2c.bus, &msg, 1, i < 2 ? 0x7f : 0x00);
	}

	k_msleep(CONFIG_TLE493D_RESET_DELAY);
}

static uint8_t tle493d_byte_parity(uint8_t data)
{
	data ^= data >> 4;
	data ^= data >> 2;
	data ^= data >> 1;
	return data & 1U;
}

static uint8_t tle493d_calculate_mod_reg_parity(uint8_t mod1_reg, uint8_t mod2_reg)
{
	return tle493d_byte_parity(mod1_reg & ~(TLE493D_MOD1_FP)) ^
	       tle493d_byte_parity(mod2_reg & TLE493D_MOD2_PRD) ^ 1U;
}

static uint8_t tle493d_get_initial_mod1_reg(uint16_t addr)
{
	uint8_t mod1_reg = TLE493D_MOD1_PR_1BYTE | TLE493D_MOD1_CA_ENABLE |
			   TLE493D_MOD1_INT_DISABLE | TLE493D_MOD1_MODE_MCM;
	switch (addr) {
	case TLE493D_ADDR_A0:
		mod1_reg |= TLE493D_MOD1_ADDR_A0;
		break;
	case TLE493D_ADDR_A1:
		mod1_reg |= TLE493D_MOD1_ADDR_A1;
		break;
	case TLE493D_ADDR_A2:
		mod1_reg |= TLE493D_MOD1_ADDR_A2;
		break;
	case TLE493D_ADDR_A3:
		mod1_reg |= TLE493D_MOD1_ADDR_A3;
		break;
	default:
		__ASSERT(false, "device address 0x%x not supported", addr);
		return 0;
	}
	if (tle493d_calculate_mod_reg_parity(mod1_reg, 0) != 0) {
		mod1_reg |= TLE493D_MOD1_FP;
	}
	return mod1_reg;
}

static int tle493d_init(const struct device *dev)
{
	const struct tle493d_config *config = dev->config;
	struct tle493d_data *data = dev->data;
	int ret;
	uint8_t dummy, mod1_reg, config_reg;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(config->i2c.bus);
		return -ENODEV;
	}

	/* see tle493d_attr_set */
	data->temp_gain = (struct sensor_value){.val1 = 0, .val2 = 240000};
	data->temp_offset = (struct sensor_value){.val1 = -258, .val2 = -200000};

	dummy = 0;
	if (i2c_read_dt(&config->i2c, &dummy, 1) != 0) {
		LOG_WRN("dummy read failed. resetting device");
		tle493d_reset(dev);
	}

	mod1_reg = tle493d_get_initial_mod1_reg(config->i2c.addr);
	ret = i2c_reg_write_byte_dt(&config->i2c, TLE493D_REG_MOD1, mod1_reg);
	if (ret < 0) {
		LOG_ERR("writing MOD1 failed (%d)", ret);
		return ret;
	}

	config_reg = TLE493D_CONFIG_TRIG_R6;
	ret = i2c_reg_write_byte_dt(&config->i2c, TLE493D_REG_CONFIG, config_reg);
	if (ret < 0) {
		LOG_ERR("writing CONFIG failed (%d)", ret);
		return ret;
	}

	return 0;
}

#define ADDR_EQ(i, addr) (addr == TLE493D_ADDR_A##i)

#define DEVICE_ADDR_SUPPORTED(inst) LISTIFY(4, ADDR_EQ, (||), DT_REG_ADDR(DT_DRV_INST(inst)))

#define TLE493D_DEFINE(inst)                                                                       \
	BUILD_ASSERT(DEVICE_ADDR_SUPPORTED(inst), "device address not supported");                 \
                                                                                                   \
	static struct tle493d_data tle493d_data_##inst;                                            \
                                                                                                   \
	static const struct tle493d_config tle493d_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tle493d_init, NULL, &tle493d_data_##inst,               \
				     &tle493d_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &tle493d_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TLE493D_DEFINE)
