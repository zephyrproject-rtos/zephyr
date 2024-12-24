/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Driver for MAX31825 1-Wire temperature sensors
 * A datasheet is available at:
 * https://www.analog.com/en/products/max31825.html
 *
 * Parasite power configuration and alarm mode is not supported by the driver.
 */
#define DT_DRV_COMPAT adi_max31825

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/w1_sensor.h>

LOG_MODULE_REGISTER(MAX31825, CONFIG_SENSOR_LOG_LEVEL);

#define MAX31825_FAMILY_CODE 0x3B

/*
 *  MAX31825 Registers
 */
#define MAX31825_R_TEMP_LSB 0x00
#define MAX31825_R_TEMP_MSB 0x01
#define MAX31825_R_STATUS   0x02
#define MAX31825_R_CFG      0x03
#define MAX31825_R_TH_MSB   0x04
#define MAX31825_R_TH_LSB   0x05
#define MAX31825_R_TL_LSB   0x06
#define MAX31825_R_TL_MSB   0x07
#define MAX31825_R_CRC      0x08

/*
 *  Status Register
 */
#define MAX31825_F_STATUS_ADDR     (0x3F)
#define MAX31825_F_STATUS_TL_FAULT BIT(6)
#define MAX31825_F_STATUS_TH_FAULT BIT(7)

/*
 *  Configuration Register
 */
#define MAX31825_F_CFG_CONV_RATE  (7 << 0)
#define MAX31825_F_CFG_CMP_INT    BIT(4)
#define MAX31825_F_CFG_RESOLUTION (3 << 5)
#define MAX31825_F_CFG_FORMAT     BIT(7)

/* Number of count per 1 celsius
 *   - TEMP_RESOLUTION_FOR_12_BIT 0.0625
 *   - TEMP_RESOLUTION_FOR_10_BIT 0.25
 *   - TEMP_RESOLUTION_FOR_9_BIT  0.5
 *   - TEMP_RESOLUTION_FOR_8_BIT  1.0
 */
#define NUM_OF_BITS_PER_1_CELSIUS 4 /* 16 = 1/0.0625 */

#define TEMP_RESOLUTION_FOR_12_BIT (0.0625f)
#define TEMP_RESOLUTION_FOR_10_BIT (0.25f)
#define TEMP_RESOLUTION_FOR_9_BIT  (0.5f)
#define TEMP_RESOLUTION_FOR_8_BIT  (1.0f)

/* Function Commands */
#define MAX31825_CMD_CONVERT_T        0x44
#define MAX31825_CMD_READ_SCRATCHPAD  0xBE
#define MAX31825_CMD_WRITE_SCRATCHPAD 0x4E
#define MAX31825_CMD_DETECT_ADDR      0x88
#define MAX31825_CMD_SELECT_ADDR      0x70

#define SCRATCHPAD_GET_TEMP_COUNT(scpth)                                                           \
	((scpth[MAX31825_R_TEMP_MSB] << 8) | scpth[MAX31825_R_TEMP_LSB])
#define SCRATCHPAD_GET_STATUS(scpth)   (scpth[MAX31825_R_STATUS])
#define SCRATCHPAD_GET_CFG(scpth)      (scpth[MAX31825_R_CFG])
#define SCRATCHPAD_GET_TH_COUNT(scpth) ((scpth[MAX31825_R_TH_MSB] << 8) | scpth[MAX31825_R_TH_LSB])
#define SCRATCHPAD_GET_TL_COUNT(scpth) ((scpth[MAX31825_R_TL_MSB] << 8) | scpth[MAX31825_R_TL_LSB])

/* measure wait time for 8-bit, 9-bit, 10-bit, 12-bit resolution respectively */
static const uint16_t measure_wait_max31825_ms[4] = {30, 40, 70, 160};

/* TMP + STATUS + CFG + TH + TL = 8bytes */
#define MAX31825_SCRATCHPAD_SIZE 8
#define MAX31825_ROMCODE_SIZE    8

struct max31825_config {
	const struct device *bus;
	uint8_t conversion_rate;
	uint8_t comp_int;   /* 0:Compare, 1:Interrupt */
	uint8_t resolution; /* 0=>8bits, 1=>9bits, 2=>10bits or 3=>12bits as per of UG */
	uint8_t format;     /* 0: Normal mode (max 128C), 1: Extend mode (max 145C) */
};

struct max31825_data {
	struct w1_slave_config slave;
	uint8_t scratchpad[MAX31825_SCRATCHPAD_SIZE];
	uint8_t rom[MAX31825_ROMCODE_SIZE]; /* FamilyCode(1 byte) + Serial(6 bytes) + CRC(1 byte) */
	bool is_configured;
};

static void convert_temp_2_count(struct sensor_value val, uint16_t *count)
{
	float temp;

	temp = (float)val.val1 + ((float)val.val2 / ((float)1000000));

	if (temp < 0) {
		temp = 0 - temp;
		*count = temp / TEMP_RESOLUTION_FOR_12_BIT;
		*count = (*count - 1) ^ 0xFFFF;
	} else {
		*count = temp / TEMP_RESOLUTION_FOR_12_BIT;
	}
}

static void convert_count_2_temp(uint16_t count, struct sensor_value *temp)
{
	/* 16th bit is sign bit */
	if (count & (1 << 15)) {
		/* Two's complement */
		count = (count ^ 0xFFFF) + 1;

		temp->val1 = count >> NUM_OF_BITS_PER_1_CELSIUS;
		temp->val2 = ((count % (1 << NUM_OF_BITS_PER_1_CELSIUS)) * 1000000) >>
			     NUM_OF_BITS_PER_1_CELSIUS;

		/* convert to negative */
		temp->val1 = 0 - temp->val1;
	} else {
		temp->val1 = count >> NUM_OF_BITS_PER_1_CELSIUS;
		temp->val2 = ((count % (1 << NUM_OF_BITS_PER_1_CELSIUS)) * 1000000) >>
			     NUM_OF_BITS_PER_1_CELSIUS;
	}
}

static int max31825_write_scratchpad(const struct device *dev)
{
	struct max31825_data *data = dev->data;
	const struct max31825_config *cfg = dev->config;
	uint8_t buf[6];

	buf[0] = MAX31825_CMD_WRITE_SCRATCHPAD;
	/* 1byte CFG, 2bytes TH, 2bytes TL = 5bytes */
	memcpy(&buf[1], &data->scratchpad[MAX31825_R_CFG], 5);

	return w1_write_read(cfg->bus, &data->slave, buf, sizeof(buf), NULL, 0);
}

static int max31825_read_scratchpad(const struct device *dev)
{
	struct max31825_data *data = dev->data;
	const struct max31825_config *cfg = dev->config;
	uint8_t cmd = MAX31825_CMD_READ_SCRATCHPAD;

	return w1_write_read(cfg->bus, &data->slave, &cmd, 1, data->scratchpad,
			     MAX31825_SCRATCHPAD_SIZE);
}

static int max31825_configure(const struct device *dev)
{
	int ret = 0;
	struct max31825_data *data = dev->data;
	const struct max31825_config *cfg = dev->config;

	if (w1_reset_bus(cfg->bus) <= 0) {
		LOG_ERR("No 1-Wire slaves connected");
		return -ENODEV;
	}

	/* In single drop configurations the rom can be read from device */
	if (w1_get_slave_count(cfg->bus) == 1) {
		if (w1_rom_to_uint64(&data->slave.rom) == 0ULL) {
			(void)w1_read_rom(cfg->bus, &data->slave.rom);
		}
	} else if (w1_rom_to_uint64(&data->slave.rom) == 0ULL) {
		LOG_DBG("nr: %d", w1_get_slave_count(cfg->bus));
		LOG_ERR("ROM required, because multiple slaves are on the bus");
		return -EINVAL;
	}

	if (data->slave.rom.family != MAX31825_FAMILY_CODE) {
		LOG_ERR("Found 1-Wire slave is not a MAX31825");
		return -EINVAL;
	}

	/* set configuration */
	uint8_t val8 = 0;

	val8 |= FIELD_PREP(MAX31825_F_CFG_CONV_RATE, cfg->conversion_rate);
	val8 |= FIELD_PREP(MAX31825_F_CFG_CMP_INT, cfg->comp_int);
	val8 |= FIELD_PREP(MAX31825_F_CFG_RESOLUTION, cfg->resolution);
	val8 |= FIELD_PREP(MAX31825_F_CFG_FORMAT, cfg->format);

	data->scratchpad[MAX31825_R_CFG] = val8;

	ret = max31825_write_scratchpad(dev);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("%s ROM info, family:%02X serial:%012llX crc:%02X", dev->name,
		(uint32_t)(data->slave.rom.family), sys_get_be48(data->slave.rom.serial),
		(uint32_t)(data->slave.rom.crc));

	/* Start automatic conversion if enabled */
	if (cfg->conversion_rate != 0) {
		/* When the conversion rate bits are set to 000, the ConvertT command
		 * initiates a single conversion and a return to shutdown.
		 * When the bits are set to a different value, the ConvertT command initiates
		 * continuous conversions. Continuous conversions may be stopped or the rate may be
		 * changed by changing the value of the conversion rate bits.
		 */
		ret = w1_reset_select(cfg->bus, &data->slave);
		if (ret == 0) {
			(void)w1_lock_bus(cfg->bus);
			ret = w1_write_byte(cfg->bus, MAX31825_CMD_CONVERT_T);
			(void)w1_unlock_bus(cfg->bus);
			if (ret < 0) {
				return ret;
			}
		}

		k_msleep(measure_wait_max31825_ms[cfg->resolution]);
	}

	data->is_configured = true;
	return 0;
}

static int api_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret;
	const struct max31825_config *cfg = dev->config;
	struct max31825_data *data = dev->data;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_AMBIENT_TEMP)) {
		return -ENOTSUP;
	}

	if (data->is_configured == false) {
		ret = max31825_configure(dev);
		if (ret < 0) {
			return ret;
		}
	}

	/* Execute convert to if NOT automatic conversion */
	if (cfg->conversion_rate == 0) {
		/* When the conversion rate bits are set to 000, the ConvertT command
		 * initiates a single conversion and a return to shutdown.
		 */
		ret = w1_reset_select(cfg->bus, &data->slave);
		if (ret == 0) {
			(void)w1_lock_bus(cfg->bus);
			ret = w1_write_byte(cfg->bus, MAX31825_CMD_CONVERT_T);
			(void)w1_unlock_bus(cfg->bus);

			if (ret < 0) {
				LOG_DBG("W1 fetch error");
				return ret;
			}
		}

		/* wait until conversion complate */
		k_msleep(measure_wait_max31825_ms[cfg->resolution]);
	}

	/* Update scratchpad */
	ret = max31825_read_scratchpad(dev);

	return ret;
}

static int api_channel_get(const struct device *dev, enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct max31825_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	convert_count_2_temp(SCRATCHPAD_GET_TEMP_COUNT(data->scratchpad), val);

	return 0;
}

static int api_attr_set(const struct device *dev, enum sensor_channel chan,
			enum sensor_attribute attr, const struct sensor_value *thr)
{
	struct max31825_data *data = dev->data;
	uint16_t count;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_W1_ROM:
		data->is_configured = false;
		w1_sensor_value_to_rom(thr, &data->slave.rom);
		break;
	case SENSOR_ATTR_LOWER_THRESH:
		data->is_configured = false;
		convert_temp_2_count(*thr, &count);
		data->scratchpad[MAX31825_R_TL_LSB] = count & 0xff;
		data->scratchpad[MAX31825_R_TL_MSB] = (count >> 8) & 0xff;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		data->is_configured = false;
		convert_temp_2_count(*thr, &count);
		data->scratchpad[MAX31825_R_TH_LSB] = count & 0xff;
		data->scratchpad[MAX31825_R_TH_MSB] = (count >> 8) & 0xff;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api max31825_driver_api = {
	.attr_set = api_attr_set,
	.sample_fetch = api_sample_fetch,
	.channel_get = api_channel_get,
};

static int max31825_init(const struct device *dev)
{
	int ret = 0;
	struct max31825_data *data = dev->data;
	const struct max31825_config *cfg = dev->config;

	if (device_is_ready(cfg->bus) == 0) {
		LOG_DBG("1-Wire bus is not ready");
		return -ENODEV;
	}

	/* Configure target after get ROM */
	data->is_configured = false;

	uint64_t rom_u64;

	rom_u64 = sys_get_be64(data->rom);
	w1_uint64_to_rom(rom_u64, &data->slave.rom);

	/* Already ROM specified, configure sensor */
	if (rom_u64 != 0ULL) {
		ret = max31825_configure(dev);
		if (ret < 0) {
			LOG_DBG("1-Wire bus cofigure failed");
		}
	}

	return ret;
}

#define MAX31825_DEFINE(inst)                                                                      \
	static struct max31825_data max31825_data_##inst = {                                       \
		.rom = DT_INST_PROP_OR(inst, rom, {0}),                                            \
	};                                                                                         \
	static const struct max31825_config max31825_config_##inst = {                             \
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),                                           \
		.conversion_rate = DT_INST_ENUM_IDX_OR(inst, conversions_per_second, 0),           \
		.resolution = DT_INST_ENUM_IDX_OR(inst, resolution, 3),                            \
		.format = DT_INST_PROP(inst, extended_mode),                                       \
		.comp_int = DT_INST_PROP_OR(inst, alarm_output_mode, 0),                           \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, max31825_init, NULL, &max31825_data_##inst,             \
				     &max31825_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &max31825_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31825_DEFINE)
