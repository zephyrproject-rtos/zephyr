/*
 * Copyright (c) 2026 Google, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/pac194x.h>
#include <zephyr/dt-bindings/sensor/pac194x.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/math_extras.h>
#include "pac194x.h"

LOG_MODULE_REGISTER(PAC194X, CONFIG_SENSOR_LOG_LEVEL);

struct pac194x_channel_config {
	bool configured;
	uint32_t shunt_resistor_uohm;
	uint8_t vbus_mode;
	uint8_t vsense_mode;
	uint8_t accumulation_mode;
	const char *label;
};

struct pac194x_chip_info {
	const uint8_t product_ids[2];
	const uint8_t phys_channels;
	const uint64_t vbus_fsr_uv;
};

static const struct pac194x_chip_info pac194x_chip_info_table[] = {
	{
		.product_ids = { PAC194X_PRODUCT_ID_1941, PAC194X_PRODUCT_ID_1941_2 },
		.phys_channels = 1,
		.vbus_fsr_uv = 9000000ULL,
	},
	{
		.product_ids = { PAC194X_PRODUCT_ID_1942, PAC194X_PRODUCT_ID_1942_2 },
		.phys_channels = 2,
		.vbus_fsr_uv = 9000000ULL,
	},
	{
		.product_ids = { PAC194X_PRODUCT_ID_1943 },
		.phys_channels = 3,
		.vbus_fsr_uv = 9000000ULL,
	},
	{
		.product_ids = { PAC194X_PRODUCT_ID_1944 },
		.phys_channels = 4,
		.vbus_fsr_uv = 9000000ULL,
	},
	{
		.product_ids = { PAC194X_PRODUCT_ID_1951, PAC194X_PRODUCT_ID_1951_2 },
		.phys_channels = 1,
		.vbus_fsr_uv = 32000000ULL,
	},
	{
		.product_ids = { PAC194X_PRODUCT_ID_1952, PAC194X_PRODUCT_ID_1952_2 },
		.phys_channels = 2,
		.vbus_fsr_uv = 32000000ULL,
	},
	{
		.product_ids = { PAC194X_PRODUCT_ID_1953 },
		.phys_channels = 3,
		.vbus_fsr_uv = 32000000ULL,
	},
	{
		.product_ids = { PAC194X_PRODUCT_ID_1954 },
		.phys_channels = 4,
		.vbus_fsr_uv = 32000000ULL,
	},
};

struct pac194x_config {
	struct i2c_dt_spec i2c;
	uint32_t shunt_micro_ohms;
	struct pac194x_channel_config channels[4];
};

struct pac194x_data_channel {
	uint64_t vacc;
	int16_t vbus;
	int16_t vsense;
	bool enabled;
};

struct pac194x_data {
	struct pac194x_data_channel channels[PAC194X_MAX_CHANNELS];
	const struct pac194x_chip_info *info;
	uint32_t acc_count;
	uint32_t sampling_freq;
	uint8_t refresh_mode;
};

static char *pac194x_sense_mode_name_get(uint8_t mode)
{
	switch (mode) {
	case PAC_UNIPOLAR_FSR:
		return "PAC_UNIPOLAR_FSR";
	case PAC_BIPOLAR_FSR:
		return "PAC_BIPOLAR_FSR";
	case PAC_BIPOLAR_HALF_FSR:
		return "PAC_BIPOLAR_HALF_FSR";
	default:
		return "UNKNOWN";
	}
}

static char *pac194x_accum_mode_name_get(uint8_t mode)
{
	switch (mode) {
	case PAC_ACCUM_VPOWER:
		return "PAC_ACCUM_VPOWER";
	case PAC_ACCUM_VSENSE:
		return "PAC_ACCUM_VSENSE";
	case PAC_ACCUM_VBUS:
		return "PAC_ACCUM_VBUS";
	default:
		return "UNKNOWN";
	}
}

static int pac194x_cmd_refresh(const struct device *dev)
{
	const struct pac194x_config *config = dev->config;
	uint8_t cmd = PAC194X_REG_REFRESH;

	return i2c_write_dt(&config->i2c, &cmd, sizeof(cmd));
}

/*
 * WARNING: This is BROADCAST call to address 0x00. It might
 *          not work on every I2C controller. Some of them
 *          block this address. Make sure the host support it
 *          before you use it.
 */
static int pac194x_cmd_refresh_all(const struct device *dev)
{
	const struct pac194x_config *config = dev->config;
	uint8_t cmd = PAC194X_REG_REFRESH;
	int ret;

	/* This command may fail as PACs do not send ACKs on broadcast. */
	ret = i2c_write(config->i2c.bus, &cmd, sizeof(cmd), 0x00);
	if (ret < 0) {
		LOG_DBG("i2c_write not ACKed");
	}

	return 0;
}

static int pac194x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct pac194x_config *config = dev->config;
	struct pac194x_data *data = dev->data;
	uint8_t buf_4[4];
	int ret;
	int i;

	/* Trigger REFRESH in non-default AUTO_WAIT mode. Specification requires
	 * to wait at least 1ms before latched values are valid. We don't want to
	 * call msleep inside the sample_fetch routine unless the user explicitly
	 * choose to wait. This is the only way to gather data from a current
	 * measurement window automatically.
	 */
	if (data->refresh_mode == PAC194X_SENSOR_ATTR_REFRESH_MODE_AUTO_WAIT) {
		/* Trigger REFRESH to latch current data into readable registers */
		pac194x_cmd_refresh(dev);
		k_msleep(2);
	}

	for (i = 0; i < data->info->phys_channels; i++) {
		/* Check if the channel is enabled */
		if (!data->channels[i].enabled) {
			continue;
		}

		uint8_t buf_2[2];
		uint8_t buf_8[8];

		/* Read VBUS (Bus Voltage) - 2 bytes */
		ret = i2c_burst_read_dt(&config->i2c, PAC194X_REG_VBUS1 + i, buf_2, sizeof(buf_2));
		if (ret < 0) {
			return ret;
		}
		data->channels[i].vbus = sys_get_be16(buf_2);

		/* Read VSENSE (Sense Resistor Voltage) - 2 bytes */
		ret = i2c_burst_read_dt(&config->i2c, PAC194X_REG_VSENSE1 + i,
					buf_2, sizeof(buf_2));
		if (ret < 0) {
			return ret;
		}
		data->channels[i].vsense = sys_get_be16(buf_2);

		/* Read VACC (Accumulator) - 56 bits (7 bytes) */
		buf_8[0] = 0;
		ret = i2c_burst_read_dt(&config->i2c, PAC194X_REG_VACC1 + i, &buf_8[1], 7);
		if (ret < 0) {
			return ret;
		}
		data->channels[i].vacc = sys_get_be64(buf_8);
	}

	/* Store Accumulator counter */
	ret = i2c_burst_read_dt(&config->i2c, PAC194X_REG_ACC_COUNT, buf_4, sizeof(buf_4));
	if (ret < 0) {
		return ret;
	}
	data->acc_count = sys_get_be32(buf_4);

	/* DEFAULT:
	 * Trigger REFRESH in AUTO_NOWAIT mode. PAC needs to wait 1ms after the data
	 * is latched and available. Call refresh at the end of sample_fetch so it
	 * will available on the next iteration. This way we gather data from the
	 * last measurement window (not current) but we don't need to wait and
	 * block the calling thread.
	 */
	if (data->refresh_mode == PAC194X_SENSOR_ATTR_REFRESH_MODE_AUTO_NOWAIT) {
		/*
		 * Trigger REFRESH to latch current data into readable registers,
		 * will be available on the next sample fetch.
		 */
		pac194x_cmd_refresh(dev);
	}

	return 0;
}

static int64_t pac194x_calculate_fsr_raw_16(uint64_t fsr, uint8_t mode, int16_t value)
{
	int64_t value_ret;

	if (mode == PAC194X_NEG_PWR_FSR_UNIPOLAR_FSR) {
		value_ret = ((uint16_t)value * (uint64_t)fsr) >> 16;
	} else {
		if (mode == PAC194X_NEG_PWR_FSR_BIPOLAR_HALF_FSR) {
			value_ret = ((int16_t)value * (int64_t)fsr) >> 16;
		} else {
			value_ret = ((int16_t)value * (int64_t)fsr) >> 15;
		}
	}

	return value_ret;
}

static int64_t pac194x_calculate_fsr_raw_64(uint64_t fsr, uint8_t mode, int64_t value)
{
	int64_t value_ret;

	if (mode == PAC194X_NEG_PWR_FSR_UNIPOLAR_FSR) {
		value_ret = ((uint64_t)value * (uint64_t)fsr) >> 16;
	} else {
		if (mode == PAC194X_NEG_PWR_FSR_BIPOLAR_HALF_FSR) {
			value_ret = ((int64_t)value * (int64_t)fsr) >> 16;
		} else {
			value_ret = ((int64_t)value * (int64_t)fsr) >> 15;
		}
	}

	return value_ret;
}

static int pac194x_parse_sensor_vbus(const struct device *dev, uint16_t ch_idx,
				     struct sensor_value *val)
{
	const struct pac194x_config *config = dev->config;
	struct pac194x_data *data = dev->data;
	int64_t vbus_uv;

	if (!data->channels[ch_idx].enabled) {
		return -ENODATA;
	}

	vbus_uv = pac194x_calculate_fsr_raw_16(data->info->vbus_fsr_uv,
					       config->channels[ch_idx].vbus_mode,
					data->channels[ch_idx].vbus);
	val->val1 = (int32_t)(vbus_uv / 1000000LL);
	val->val2 = (int32_t)(vbus_uv % 1000000LL);

	return 0;
}

static int pac194x_parse_sensor_vsense(const struct device *dev, uint16_t ch_idx,
				       struct sensor_value *val)
{
	const struct pac194x_config *config = dev->config;
	struct pac194x_data *data = dev->data;
	int64_t current_ua;

	if (!data->channels[ch_idx].enabled) {
		return -ENODATA;
	}

	current_ua = pac194x_calculate_fsr_raw_16(100000LL,
						  config->channels[ch_idx].vsense_mode,
						  data->channels[ch_idx].vsense);
	current_ua = (current_ua * 1000000LL) /
		     (int64_t)config->channels[ch_idx].shunt_resistor_uohm;

	val->val1 = (int32_t)(current_ua / 1000000LL);
	val->val2 = (int32_t)(current_ua % 1000000LL);

	return 0;
}

static int pac194x_parse_sensor_acc_avg(const struct device *dev, uint16_t ch_idx,
					struct sensor_value *val, uint8_t is_avg)
{
	const struct pac194x_config *config = dev->config;
	struct pac194x_data *data = dev->data;
	int64_t res_uv;

	if (!data->channels[ch_idx].enabled) {
		return -ENODATA;
	}
	if (data->acc_count == 0) {
		return -EAGAIN;
	}

	/*
	 * Treat vacc as 56-bit signed integer, extend it to 64-bit first
	 * WARNING: In unipolar mode the 56th bit is actucally unsigned.
	 *          Ignore that for the sake of a simple code, the number is
	 *          so large anyway that it will take 10^9 years for the
	 *          issue to appear.
	 */
	int64_t vacc = (int64_t)data->channels[ch_idx].vacc;

	if (vacc & (1ULL << 55)) {
		vacc |= (0xffULL << 56);
	}

	if (is_avg) {
		if (data->acc_count == 0) {
			return -EINVAL;
		}
		vacc = vacc / (int64_t)data->acc_count;
	}

	switch (config->channels[ch_idx].accumulation_mode) {
	case PAC194X_ACCUM_CONFIG_VBUS:
		res_uv = pac194x_calculate_fsr_raw_64(data->info->vbus_fsr_uv,
						      config->channels[ch_idx].vbus_mode,
						      vacc);
		break;
	case PAC194X_ACCUM_CONFIG_VSENSE:
		res_uv = pac194x_calculate_fsr_raw_64(100000LL,
						      config->channels[ch_idx].vsense_mode,
						      vacc);
		res_uv = (res_uv * 1000000LL) /
			 (int64_t)config->channels[ch_idx].shunt_resistor_uohm;
		break;
	case PAC194X_ACCUM_CONFIG_VPOWER:
	{
		/* FSR Power in microwatts (uW) */
		int64_t power_fsr_uw;
		uint64_t denominator;
		const struct pac194x_channel_config *channel = &config->channels[ch_idx];

		/*
		 * Denominator matrix:
		 *
		 * VBUS_MODE      VSENSE_MODE    POWER_DENOMINATOR
		 * -----------------------------------------------
		 * UNIPOLAR        UNIPOLAR           2^30
		 * UNIPOLAR        BIPOLAR_HALF       2^30
		 * UNIPOLAR        BIPOLAR              2^29
		 * BIPOLAR_HALF    UNIPOLAR           2^30
		 * BIPOLAR_HALF    BIPOLAR_HALF       2^30
		 * BIPOLAR_HALF    BIPOLAR            2^30
		 * UNIPOLAR        BIPOLAR              2^29
		 * BIPOLAR         UNIPOLAR           2^30
		 * BIPOLAR         BIPOLAR_HALF       2^30
		 * BIPOLAR         BIPOLAR              2^29
		 */
		if (channel->vbus_mode == PAC194X_NEG_PWR_FSR_BIPOLAR_HALF_FSR ||
		    channel->vsense_mode == PAC194X_NEG_PWR_FSR_BIPOLAR_HALF_FSR) {
			denominator = 30;
		} else if (channel->vbus_mode == PAC194X_NEG_PWR_FSR_BIPOLAR_FSR ||
			   channel->vsense_mode == PAC194X_NEG_PWR_FSR_BIPOLAR_FSR) {
			denominator = 29;
		} else {
			denominator = 30;
		}

		power_fsr_uw = (data->info->vbus_fsr_uv * 100000LL) /
			       (int64_t)channel->shunt_resistor_uohm;

		/*
		 * If is_avg then there is no chance for overflow as the precision is
		 * the same as typical Power precision.
		 * Use 128 bit multiplication if power data is accumulated.
		 */
		if (is_avg) {
			res_uv = ((int64_t)vacc * power_fsr_uw) >> denominator;
		} else {
			int128_t i128;

			i128_multiply_i64_i64(vacc, power_fsr_uw, &i128);
			res_uv = i128.low >> denominator;
			res_uv |= i128.high << (64 - denominator);

			if (i128.high >> denominator) {
				/* This is VERY unlikely but take care of the corner case */
				LOG_ERR("accumulated power exceeds 64-bit");
				return -E2BIG;
			}
		}

		break;
	}
	default:
		return -ENOTSUP;
	}

	val->val1 = (int32_t)(res_uv / 1000000LL);
	val->val2 = (int32_t)(res_uv % 1000000LL);
	return 0;
}

static int pac194x_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	enum pac194x_sensor_channel chan_priv = (enum pac194x_sensor_channel)chan;
	struct pac194x_data *data = dev->data;
	uint8_t action;
	int ch_idx;

	/* Determine channel index and type of measurement */
	if (chan_priv >= PAC194X_CHAN_VBUS1 && chan_priv <= PAC194X_CHAN_VBUS4) {
		ch_idx = chan_priv - PAC194X_CHAN_VBUS1;
		action = SENSOR_CHAN_VOLTAGE;
	} else if (chan_priv >= PAC194X_CHAN_CURR1 && chan_priv <= PAC194X_CHAN_CURR4) {
		ch_idx = chan_priv - PAC194X_CHAN_CURR1;
		action = SENSOR_CHAN_CURRENT;
	} else if (chan_priv >= PAC194X_CHAN_ACC1_AVG && chan_priv <= PAC194X_CHAN_ACC4_AVG) {
		ch_idx = chan_priv - PAC194X_CHAN_ACC1_AVG;
		action = PAC194X_CHAN_ACC1_AVG;
	} else if (chan_priv >= PAC194X_CHAN_ACC1 && chan_priv <= PAC194X_CHAN_ACC4) {
		ch_idx = chan_priv - PAC194X_CHAN_ACC1;
		action = PAC194X_CHAN_ACC1;
	} else {
		action = chan;
		ch_idx = 0;
	}

	if ((action != PAC194X_CHAN_ACC_COUNT) && !data->channels[ch_idx].enabled) {
		return -ENODATA;
	}

	switch (action) {
	case SENSOR_CHAN_VOLTAGE:
		return pac194x_parse_sensor_vbus(dev, ch_idx, val);
	case SENSOR_CHAN_CURRENT:
		return pac194x_parse_sensor_vsense(dev, ch_idx, val);
	case PAC194X_CHAN_ACC1:
		return pac194x_parse_sensor_acc_avg(dev, ch_idx, val, 0);
	case PAC194X_CHAN_ACC1_AVG:
		return pac194x_parse_sensor_acc_avg(dev, ch_idx, val, 1);
	case PAC194X_CHAN_ACC_COUNT:
		/* ACC_COUNT is a 24-bit counter, it fits easily in int32_t val1 */
		val->val1 = (int32_t)data->acc_count;
		val->val2 = 0;
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

static int freq_to_mode(int freq)
{
	switch (freq) {
	case 1024:
		return PAC194X_CTRL_SAMPLE_1024_ADAPTIVE;
	case 256:
		return PAC194X_CTRL_SAMPLE_256_ADAPTIVE;
	case 64:
		return PAC194X_CTRL_SAMPLE_64_ADAPTIVE;
	case 8:
		return PAC194X_CTRL_SAMPLE_8_ADAPTIVE;
	default:
		return -EINVAL;
	}
}

static int pac194_reg_mask_write_16(const struct device *dev, uint8_t reg,
				    uint16_t mask, uint16_t val)
{
	const struct pac194x_config *config = dev->config;
	int ret;
	uint8_t buf[2];
	uint16_t regval;

	ret = i2c_burst_read_dt(&config->i2c, reg, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	regval = sys_get_be16(buf);
	regval &= ~mask;
	regval |= (val & mask);
	sys_put_be16(regval, buf);

	ret = i2c_burst_write_dt(&config->i2c, reg, buf, sizeof(buf));

	return ret;
}

static int pac194x_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	const struct pac194x_config *config = dev->config;
	struct pac194x_data *data = dev->data;
	int mode;
	int ret;
	uint8_t chan_id;

	/* Convert channel to channel ID */
	switch ((enum pac194x_sensor_channel)chan) {
	case PAC194X_CHAN_VBUS1:
	case PAC194X_CHAN_CURR1:
	case PAC194X_CHAN_ACC1:
	case PAC194X_CHAN_ACC1_AVG:
		chan_id = 0;
		break;
	case PAC194X_CHAN_VBUS2:
	case PAC194X_CHAN_CURR2:
	case PAC194X_CHAN_ACC2:
	case PAC194X_CHAN_ACC2_AVG:
		chan_id = 1;
		break;
	case PAC194X_CHAN_VBUS3:
	case PAC194X_CHAN_CURR3:
	case PAC194X_CHAN_ACC3:
	case PAC194X_CHAN_ACC3_AVG:
		chan_id = 2;
		break;
	case PAC194X_CHAN_VBUS4:
	case PAC194X_CHAN_CURR4:
	case PAC194X_CHAN_ACC4:
	case PAC194X_CHAN_ACC4_AVG:
		chan_id = 3;
		break;
	default:
		chan_id = 0xff;
	}

	mode = val->val1;
	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		mode = freq_to_mode(mode);
		/* fallthrough */
	case SENSOR_ATTR_SAMPLING_FREQUENCY_RAW:
		if (chan != SENSOR_CHAN_ALL) {
			return -ENOTSUP;
		}
		if (mode < 0 || mode > 0xF) {
			return -EINVAL;
		}

		ret = pac194_reg_mask_write_16(dev, PAC194X_REG_CTRL,
					       PAC194X_CTRL_SAMPLE_MODE_MASK,
					       mode << PAC194X_CTRL_SAMPLE_MODE_POS);
		if (ret < 0) {
			return ret;
		}

		/*
		 * INFO:
		 * It is necessary to manually refresh ACC counters on every
		 * PAC194X_CTRL_REG change.
		 */
		return pac194x_cmd_refresh(dev);
	case SENSOR_ATTR_CHANNEL_ENABLED:
	{
		uint8_t regval;
		bool enable = (val->val1 > 0);

		if (chan_id >= data->info->phys_channels ||
		    chan_id >= PAC194X_MAX_CHANNELS ||
		    config->channels[chan_id].configured == 0) {
			return -ENOTSUP;
		}

		if (enable) {
			regval = 0;
		} else {
			regval = PAC194X_CTRL_CHANNEL_N_OFF(chan_id);
		}


		ret = pac194_reg_mask_write_16(dev, PAC194X_REG_CTRL,
					       PAC194X_CTRL_CHANNEL_N_OFF(chan_id),
					       regval);
		if (ret < 0) {
			return ret;
		}

		data->channels[chan_id].enabled = enable;

		/*
		 * INFO:
		 * It is necessary to manually refresh ACC counters on every
		 * PAC194X_CTRL_REG change.
		 */
		return pac194x_cmd_refresh(dev);
	}
	case SENSOR_ATTR_REFRESH_MODE:
		switch (val->val1) {
		case PAC194X_SENSOR_ATTR_REFRESH_MODE_AUTO_NOWAIT:
		case PAC194X_SENSOR_ATTR_REFRESH_MODE_AUTO_WAIT:
		case PAC194X_SENSOR_ATTR_REFRESH_MODE_MANUAL:
			data->refresh_mode = (val->val1);
			return 0;
		default:
			return -EINVAL;
		}
	case SENSOR_ATTR_FORCE_REFRESH_CMD:
		switch (val->val1) {
		case PAC194X_SENSOR_ATTR_FORCE_REFRESH_CMD_SINGLE:
			return pac194x_cmd_refresh(dev);
		case PAC194X_SENSOR_ATTR_FORCE_REFRESH_CMD_ALL:
			return pac194x_cmd_refresh_all(dev);
		default:
			return -EINVAL;
		}
		return 0;
	default:
		return -ENOTSUP;
	}

	return -EINVAL;
}

static DEVICE_API(sensor, pac194x_api_funcs) = {
	.sample_fetch = pac194x_sample_fetch,
	.channel_get = pac194x_channel_get,
	.attr_set = pac194x_attr_set,
};

static int pac194x_chip_identify(const struct device *dev)
{
	const struct pac194x_config *config = dev->config;
	struct pac194x_data *data = dev->data;
	const struct pac194x_chip_info *info;
	int ret;
	uint8_t product_id;

	ret = i2c_reg_read_byte_dt(&config->i2c, PAC194X_REG_PRODUCT_ID, &product_id);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("found: %02x", product_id);

	for (uint8_t a = 0; a < ARRAY_SIZE(pac194x_chip_info_table); a++) {
		info = &pac194x_chip_info_table[a];
		for (uint8_t b = 0; b < ARRAY_SIZE(info->product_ids); b++) {
			if ((info->product_ids[b] != 0) && (info->product_ids[b] == product_id)) {
				data->info = info;
				return 0;
			}
		}
	}

	LOG_ERR("unknown product_id = 0x%02X", product_id);
	return -EINVAL;
}

static void pac194x_dump_config(const struct device *dev)
{
	const struct pac194x_config *config = dev->config;

	LOG_DBG("--- PAC194X/195X Channel Configuration Dump ---");
	LOG_DBG("Device: PAC194X/195X at I2C 0x%02x", config->i2c.addr);

	for (int i = 0; i < PAC194X_MAX_CHANNELS; i++) {
		const struct pac194x_channel_config *ch = &config->channels[i];

		if (!ch->configured) {
			LOG_DBG("Channel %d: [DISABLED]", i + 1);
			continue;
		}

		LOG_DBG("Channel %d: [CONFIGURED]", i + 1);
		LOG_DBG("  Label:      %s", ch->label ? ch->label : "N/A");
		LOG_DBG("  Shunt:      %u uOhm (%u.%03u mOhm)",
			ch->shunt_resistor_uohm,
			ch->shunt_resistor_uohm / 1000,
			ch->shunt_resistor_uohm % 1000);
		LOG_DBG("  VBUS Mode:  %s", pac194x_sense_mode_name_get(ch->vbus_mode));
		LOG_DBG("  VSENSE Mode:%s", pac194x_sense_mode_name_get(ch->vsense_mode));
		LOG_DBG("  ACCUMULATION Mode:%s",
			pac194x_accum_mode_name_get(ch->accumulation_mode));
	}
	LOG_DBG("-------------------------------------------");
}

static int pac194x_configure_channels(const struct device *dev)
{
	const struct pac194x_config *config = dev->config;
	int ret;
	uint8_t a, vsense, vbus, accum;
	uint8_t buf[2];

	if (IS_ENABLED(CONFIG_SENSOR_LOG_LEVEL_DBG)) {
		pac194x_dump_config(dev);
	}

	/* Configure VSENSE and VBUS modes */
	vsense = 0;
	vbus = 0;
	for (a = 0; a < PAC194X_MAX_CHANNELS; a++) {
		uint16_t tmp;

		if (!config->channels[a].configured) {
			continue;
		}

		tmp = config->channels[a].vbus_mode;
		switch (tmp) {
		case PAC_UNIPOLAR_FSR:
		case PAC_BIPOLAR_FSR:
		case PAC_BIPOLAR_HALF_FSR:
			break;
		default:
			tmp = 0;
			break;
		}
		vsense |= tmp << (2 * (3 - a));

		tmp = config->channels[a].vsense_mode;
		switch (tmp) {
		case PAC_UNIPOLAR_FSR:
		case PAC_BIPOLAR_FSR:
		case PAC_BIPOLAR_HALF_FSR:
			break;
		default:
			tmp = 0;
			break;
		}
		vbus |= tmp << (2 * (3 - a));
	}

	/* MSB->LSB order */
	buf[0] = vsense;
	buf[1] = vbus;
	ret = i2c_burst_write_dt(&config->i2c, PAC194X_REG_NEG_PWR_FSR, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	/* Configure ACCUMULATOR mode */
	accum = 0;
	for (a = 0; a < PAC194X_MAX_CHANNELS; a++) {
		uint16_t tmp;

		if (!config->channels[a].configured) {
			continue;
		}

		tmp = config->channels[a].accumulation_mode;
		switch (tmp) {
		case PAC_ACCUM_VPOWER:
		case PAC_ACCUM_VSENSE:
		case PAC_ACCUM_VBUS:
			break;
		default:
			tmp = 0;
			break;
		}
		accum |= tmp << (2 * (3 - a));
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, PAC194X_REG_ACCUM_CONFIG, accum);
	if (ret < 0) {
		return ret;
	}

	/* Disable all channels */
	ret = pac194_reg_mask_write_16(dev, PAC194X_REG_CTRL,
				       PAC194X_CTRL_CHANNEL_N_OFF_MASK,
				       PAC194X_CTRL_CHANNEL_N_OFF_MASK);

	if (ret < 0) {
		return ret;
	}

	/*
	 * INFO:
	 * It is necessary to manually refresh ACC counters on every PAC194X_CTRL_REG change.
	 */
	return pac194x_cmd_refresh(dev);
}

static int pac194x_init(const struct device *dev)
{
	const struct pac194x_config *config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		return -ENODEV;
	}

	if (pac194x_chip_identify(dev)) {
		return -ENODEV;
	}

	if (pac194x_configure_channels(dev)) {
		LOG_ERR("failed to configure channels");
		return -EINVAL;
	}

	LOG_DBG("initialized at 0x%02x", config->i2c.addr);

	return 0;
}

#define PAC194X_SET_CHANNEL(node_id)								\
	[DT_REG_ADDR(node_id) - 1] = {								\
		.configured = 1,								\
		.shunt_resistor_uohm = DT_PROP(node_id, microchip_shunt_resistor_micro_ohms),	\
		.label = DT_PROP_OR(node_id, label, NULL),					\
		.vbus_mode = DT_PROP(node_id, microchip_vbus_mode),				\
		.vsense_mode = DT_PROP(node_id, microchip_vsense_mode),				\
		.accumulation_mode = DT_PROP(node_id, microchip_accumulation_mode),		\
	},											\

#define PAC194X_DEFINE(inst)									\
	static struct pac194x_data pac194x_data_##inst;						\
												\
	static const struct pac194x_config pac194x_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		.channels = {									\
			DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, PAC194X_SET_CHANNEL)		\
		},										\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,							\
				     pac194x_init,						\
				     NULL,							\
				     &pac194x_data_##inst,					\
				     &pac194x_config_##inst,					\
				     POST_KERNEL,						\
				     CONFIG_SENSOR_INIT_PRIORITY,				\
				     &pac194x_api_funcs);

#define DT_DRV_COMPAT microchip_pac194x
DT_INST_FOREACH_STATUS_OKAY(PAC194X_DEFINE)
