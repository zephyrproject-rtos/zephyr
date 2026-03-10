/**
 * @file ad2s1210.c
 * @brief ad2s1210 driver module
 *
 * @copyright Copyright (c) 2025, Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT adi_ad2s1210

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ad2s1210, CONFIG_SENSOR_LOG_LEVEL);

/** Position register address */
#define AD2S1210_REG_POSITION         0x80
/** Velocity register address */
#define AD2S1210_REG_VELOCITY         0x82
/** Loss of Signal threshold register address */
#define AD2S1210_REG_LOS_THRD         0x88
/** Degradation of Signal overrange threshold register address */
#define AD2S1210_REG_DOS_OVR_THRD     0x89
/** Degradation of Signal mismatch threshold register address */
#define AD2S1210_REG_DOS_MIS_THRD     0x8A
/** Degradation of Signal reset maximum threshold register address */
#define AD2S1210_REG_DOS_RST_MAX_THRD 0x8B
/** Degradation of Signal reset minimum threshold register address */
#define AD2S1210_REG_DOS_RST_MIN_THRD 0x8C
/** Loss of Tracking high threshold register address */
#define AD2S1210_REG_LOT_HIGH_THRD    0x8D
/** Loss of Tracking low threshold register address */
#define AD2S1210_REG_LOT_LOW_THRD     0x8E
/** Excitation frequency register address */
#define AD2S1210_REG_EXCIT_FREQ       0x91
/** Control register address */
#define AD2S1210_REG_CONTROL          0x92
/** Resolution bit 0 control mask */
#define AD2S1210_CONTROL_RES0_MASK    BIT(0)
/** Resolution bit 1 control mask */
#define AD2S1210_CONTROL_RES1_MASK    BIT(1)
/** Combined resolution control mask */
#define AD2S1210_CONTROL_RES_MASK     (AD2S1210_CONTROL_RES0_MASK | AD2S1210_CONTROL_RES1_MASK)
/** Hysteresis enable control bit */
#define AD2S1210_ENABLE_HYSTERESIS    BIT(4)

/** Software reset register address */
#define AD2S1210_REG_SOFT_RESET 0xF0
/** Fault register address */
#define AD2S1210_REG_FAULT      0xFF

/** Minimum valid register address */
#define AD2S1210_REG_MIN AD2S1210_REG_POSITION

/** Minimum input clock frequency in Hz */
#define AD2S1210_MIN_CLKIN  6144000
/** Maximum input clock frequency in Hz */
#define AD2S1210_MAX_CLKIN  10240000
/** Minimum excitation frequency in Hz */
#define AD2S1210_MIN_EXCIT  2000
/** Maximum excitation frequency in Hz */
#define AD2S1210_MAX_EXCIT  20000
/** Excitation frequency step size in Hz */
#define AD2S1210_STEP_EXCIT 250
/** Minimum frequency control word value */
#define AD2S1210_MIN_FCW    0x4
/** Maximum frequency control word value */
#define AD2S1210_MAX_FCW    0x50

/** Maximum number of resolution bits */
#define AD2S1210_MAX_RESOLUTION_BITS 16

/** AD2S1210 modes */
enum ad2s1210_mode {
	/** Normal position mode */
	AD2S1210_MODE_POSITION = 0,
	/** Reserved mode (unused) */
	AD2S1210_MODE_RESERVED,
	/** Normal velocity mode */
	AD2S1210_MODE_VELOCITY,
	/** Configuration mode */
	AD2S1210_MODE_CONFIG,
};

/** Analog resolution */
enum ad2s1210_res {
	/** Data resolution 10 bits */
	AD2S1210_RES_10BIT,
	/** Data resolution 12 bits */
	AD2S1210_RES_12BIT,
	/** Data resolution 14 bits */
	AD2S1210_RES_14BIT,
	/** Data resolution 16 bits */
	AD2S1210_RES_16BIT,
	/** Maximum number of resolution values, used for lookup tables and loops */
	AD2S1210_RES_MAX_VAL,
};

/** AD2S1210 channels */
enum ad2s1210_channel {
	/** Position channel */
	AD2S1210_POS,
	/** Velocity channel */
	AD2S1210_VEL,
};

/** Enumeration for array of mode pins */
enum ad2s1210_mode_pin {
	/** Mode pin A0 */
	AD2S1210_MODE_PIN_A0 = 0,
	/** Mode pin A1 */
	AD2S1210_MODE_PIN_A1,
	/** Number of mode pin values in enum */
	AD2S1210_MODE_PIN_MAX_VAL,
};

/** Enumeration for array of resolution pins */
enum ad2s1210_res_pin {
	/** Resolution pin RES0 */
	AD2S1210_RES_PIN_RES0 = 0,
	/** Resolution pin RES1 */
	AD2S1210_RES_PIN_RES1,
	/** Number of resolution pin values in enum */
	AD2S1210_RES_PIN_MAX_VAL,
};

/** Enumeration for array of fault pins */
enum ad2s1210_fault_pin {
	/** Fault pin LOT */
	AD2S1210_FAULT_PIN_LOT = 0,
	/** Fault pin DOS */
	AD2S1210_FAULT_PIN_DOS,
	/** Number of fault pin values in enum */
	AD2S1210_FAULT_PIN_MAX_VAL,
};

/** Enumeration for supported clock in frequency */
enum ad2s1210_clockin_frequency {
	/** Clock In frequency 8.192MHz */
	AD2S1210_CLOCKIN_8MHZ192,
	/** Clock In frequency 10.24MHz  */
	AD2S1210_CLOCKIN_10MHZ24,
	/** Unknown clock in frequency */
	AD2S1210_CLOCKIN_UNKNOWN,
};

/** ad2s1210 data structure */
struct ad2s1210_data {
	/** Clock In frequency enumeration */
	enum ad2s1210_clockin_frequency clock;
	/** Configuration mode */
	enum ad2s1210_mode mode;
	/** Resolution */
	enum ad2s1210_res resolution;
	/** Position data */
	uint16_t position;
	/** Velocity data */
	int16_t velocity;
};

/** ad2s1210 configuration structure */
struct ad2s1210_config {
	/** Sample gpio pin */
	struct gpio_dt_spec sample_gpio;
	/** Mode selection gpio pins (A0 and A1) */
	struct gpio_dt_spec mode_gpios[AD2S1210_MODE_PIN_MAX_VAL];
	/** Reset gpio pin */
	struct gpio_dt_spec reset_gpio;
	/** Resolution selection gpio pins (RES0 and RES1) */
	struct gpio_dt_spec resolution_gpios[AD2S1210_RES_PIN_MAX_VAL];
	/** Fault indication gpio pins (LOT and DOS) */
	struct gpio_dt_spec fault_gpios[AD2S1210_FAULT_PIN_MAX_VAL];
	/** SPI configuration */
	struct spi_dt_spec spi;
	/** Clock frequency in Hz */
	uint32_t clock_frequency;
	/** Assigned resolution in bits */
	uint8_t assigned_resolution_bits;
	/** Flag indicating if resolution pins are defined in devicetree */
	bool have_resolution_pins;
};

/**
 * @brief Lookup table for maximum velocity range in RPM based on clock and resolution
 *
 * @see See Tracking Rate table in ad2s1210 datasheet
 */
static const int32_t table_velocity_range_rpm[AD2S1210_RES_MAX_VAL][AD2S1210_CLOCKIN_UNKNOWN] = {
	[AD2S1210_RES_16BIT] = {
			[AD2S1210_CLOCKIN_8MHZ192] = 125 * 60,
			[AD2S1210_CLOCKIN_10MHZ24] = 9375,
		},
	[AD2S1210_RES_14BIT] = {
			[AD2S1210_CLOCKIN_8MHZ192] = 500 * 60,
			[AD2S1210_CLOCKIN_10MHZ24] = 625 * 60,
		},
	[AD2S1210_RES_12BIT] = {
			[AD2S1210_CLOCKIN_8MHZ192] = 1000 * 60,
			[AD2S1210_CLOCKIN_10MHZ24] = 1250 * 60,
		},
	[AD2S1210_RES_10BIT] = {
		[AD2S1210_CLOCKIN_8MHZ192] = 2500 * 60,
		[AD2S1210_CLOCKIN_10MHZ24] = 3125 * 60,
	}};

/**
 * @brief Set the operation mode of the device.
 *
 * @param dev ad2s1210 device
 * @param mode Operation mode to set the device
 * @return 0 if successful, negative error code otherwise
 */
static int ad2s1210_set_mode_pins(const struct device *dev, enum ad2s1210_mode mode);

/**
 * @brief Get data from a specific channel of the device
 *
 * @param dev ad2s1210 device
 * @param chn Channel to read from (position or velocity)
 * @param data Pointer to store the read data
 * @return 0 if successful, negative error code otherwise
 */
static int ad2s1210_get_channel_data(const struct device *dev, enum ad2s1210_channel chn,
				     uint16_t *data);

/**
 * @brief Fetch samples from the sensor
 *
 * @param dev ad2s1210 device
 * @param chan Channel to sample
 * @return 0 if successful, negative error code otherwise
 */
static int ad2s1210_sample_fetch(const struct device *dev, enum sensor_channel chan);

/**
 * @brief Get channel values from the sensor
 *
 * @param dev ad2s1210 device
 * @param chan Channel to get value from
 * @param val Pointer to store the channel value
 * @return 0 if successful, negative error code otherwise
 */
static int ad2s1210_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val);

/**
 * @brief Write a value to a device register
 *
 * @param dev ad2s1210 device
 * @param addr Register address
 * @param val Value to write
 * @return 0 if successful, negative error code otherwise
 */
static int ad2s1210_reg_write(const struct device *dev, uint8_t addr, uint8_t val);

/**
 * @brief Read a value from a device register
 *
 * @param dev ad2s1210 device
 * @param addr Register address
 * @param val Pointer to store the read value
 * @return 0 if successful, negative error code otherwise
 */
static int ad2s1210_reg_read(const struct device *dev, uint8_t addr, uint8_t *val);

/**
 * @brief Set the resolution of the device
 *
 * @param dev ad2s1210 device
 * @param resolution Resolution to set (10, 12, 14 or 16 bits)
 * @return 0 if successful, negative error code otherwise
 */
static int ad2s1210_set_resolution(const struct device *dev, enum ad2s1210_res resolution);

/**
 * @brief Enable or disable hysteresis
 *
 * @param dev ad2s1210 device
 * @param enable True to enable hysteresis, false to disable
 * @return 0 if successful, negative error code otherwise
 */
static int ad2s1210_set_hysteresis(const struct device *dev, bool enable);

/**
 * @brief Check if hysteresis is enabled
 *
 * @param dev ad2s1210 device
 * @return 1 if enabled, 0 if disabled, negative error code otherwise
 */
static __maybe_unused int ad2s1210_hysteresis_is_enabled(const struct device *dev);

/**
 * @brief Reinitialize the excitation frequency
 *
 * @param dev ad2s1210 device
 * @param fexcit Excitation frequency to set
 * @return 0 if successful, negative error code otherwise
 */
static __maybe_unused int ad2s1210_reinit_excitation_frequency(const struct device *dev,
							       uint16_t fexcit);

/**
 * @brief Initialize the ad2s1210 device
 *
 * @param dev ad2s1210 device
 * @return 0 if successful, negative error code otherwise
 */
static int ad2s1210_init(const struct device *dev);

static int ad2s1210_get_channel_data(const struct device *dev, enum ad2s1210_channel chn,
				     uint16_t *data)
{
	const struct ad2s1210_config *config = dev->config;
	int ret;
	enum ad2s1210_mode mode = AD2S1210_MODE_POSITION;
	uint8_t rx_buf[2] = {0};

	if (chn == AD2S1210_VEL) {
		mode = AD2S1210_MODE_VELOCITY;
	}

	ret = ad2s1210_set_mode_pins(dev, mode);
	if (ret < 0) {
		return ret;
	}

	/* Read data over SPI */
	const struct spi_buf rx_buf_arr = {
		.buf = rx_buf,
		.len = sizeof(rx_buf),
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf_arr,
		.count = 1,
	};

	ret = spi_read_dt(&config->spi, &rx);
	if (ret < 0) {
		return ret;
	}

	/* Combine bytes into 16-bit value */
	*data = sys_get_be16(rx_buf);
	return 0;
}

static int ad2s1210_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ad2s1210_config *config = dev->config;
	struct ad2s1210_data *data = dev->data;
	uint16_t raw_data;
	int ret;

	/* Pull sample pin to trigger measurement */
	ret = gpio_pin_set_dt(&config->sample_gpio, 1);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_set_dt(&config->sample_gpio, 0);
	if (ret < 0) {
		return ret;
	}

	switch (chan) {
	case SENSOR_CHAN_ROTATION:
		ret = ad2s1210_get_channel_data(dev, AD2S1210_POS, &raw_data);
		if (ret) {
			return ret;
		}
		data->position = raw_data;
		break;
	case SENSOR_CHAN_RPM:
		ret = ad2s1210_get_channel_data(dev, AD2S1210_VEL, &raw_data);
		if (ret) {
			return ret;
		}
		data->velocity = (int16_t)raw_data;
		break;
	case SENSOR_CHAN_ALL:
		ret = ad2s1210_get_channel_data(dev, AD2S1210_POS, &raw_data);
		if (ret) {
			return ret;
		}
		data->position = raw_data;

		ret = ad2s1210_get_channel_data(dev, AD2S1210_VEL, &raw_data);
		if (ret) {
			return ret;
		}
		data->velocity = (int16_t)raw_data;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ad2s1210_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct ad2s1210_data *data = dev->data;

	switch ((int16_t)chan) {
	case SENSOR_CHAN_ROTATION: {
		/* Scale factor of 1000000 for decimal places */
		/* position = (data->position * 360 * 1000000) / 65536 */
		uint64_t scaled_pos =
			((uint64_t)data->position * 360 * 1000000) >> AD2S1210_MAX_RESOLUTION_BITS;

		val->val1 = scaled_pos / 1000000;
		val->val2 = scaled_pos % 1000000;
	} break;

	case SENSOR_CHAN_RPM: {
		/* Get max range from lookup table */
		int32_t range = table_velocity_range_rpm[data->resolution][data->clock];
		/* Convert raw velocity to RPM */
		int rpm = ((int32_t)data->velocity * range) /
			  ((1 << (AD2S1210_MAX_RESOLUTION_BITS - 1)) - 1);

		val->val1 = rpm;
		val->val2 = 0;
	} break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

/** ad2s1210 driver API interface */
static DEVICE_API(sensor, ad2s1210_api) = {
	.sample_fetch = &ad2s1210_sample_fetch,
	.channel_get = &ad2s1210_channel_get,
};

static int ad2s1210_set_mode_pins(const struct device *dev, enum ad2s1210_mode mode)
{
	const struct ad2s1210_config *config = dev->config;
	struct ad2s1210_data *data = dev->data;
	int ret;

	if (data->mode == mode) {
		return 0;
	}

	/* Set A0 pin (bit 0 of mode) */
	ret = gpio_pin_set_dt(&config->mode_gpios[AD2S1210_MODE_PIN_A0], mode & BIT(0));
	if (ret < 0) {
		LOG_ERR("Could not set A0 pin (%d)", ret);
		return ret;
	}

	/* Set A1 pin (bit 1 of mode) */
	ret = gpio_pin_set_dt(&config->mode_gpios[AD2S1210_MODE_PIN_A1], !!(mode & BIT(1)));
	if (ret < 0) {
		LOG_ERR("Could not set A1 pin (%d)", ret);
		return ret;
	}

	data->mode = mode;
	return 0;
}

static int ad2s1210_reg_read(const struct device *dev, uint8_t addr, uint8_t *val)
{
	const struct ad2s1210_config *config = dev->config;
	int ret;
	uint8_t tx_buf;
	uint8_t tx_buf2 = 0;
	uint8_t rx_buf = 0;

	/* Validate register address */
	if (addr < AD2S1210_REG_MIN) {
		return -EINVAL;
	}

	/* Set device to CONFIG mode */
	ret = ad2s1210_set_mode_pins(dev, AD2S1210_MODE_CONFIG);
	if (ret < 0) {
		return ret;
	}

	/* Write register address */
	tx_buf = addr;
	const struct spi_buf tx_buf_arr = {
		.buf = &tx_buf,
		.len = 1,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf_arr,
		.count = 1,
	};

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Read register value while writing valid address
	 *
	 * While ad2s1210 will present data of the previous cycle on the SDO pins
	 * it will try to read from the address currently on the SDI pins.
	 * An invalid address might cause undefined behavior so better to have
	 * some valid address in buf while we read the result.
	 */
	tx_buf2 = addr;
	const struct spi_buf tx_buf_arr2 = {
		.buf = &tx_buf2,
		.len = 1,
	};
	const struct spi_buf rx_buf_arr = {
		.buf = &rx_buf,
		.len = 1,
	};
	const struct spi_buf_set tx2 = {
		.buffers = &tx_buf_arr2,
		.count = 1,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf_arr,
		.count = 1,
	};

	ret = spi_transceive_dt(&config->spi, &tx2, &rx);
	if (ret < 0) {
		return ret;
	}

	*val = rx_buf;
	return 0;
}

static int ad2s1210_reg_write(const struct device *dev, uint8_t addr, uint8_t val)
{
	const struct ad2s1210_config *config = dev->config;
	int ret;
	uint8_t tx_buf[2];

	/* Validate register address */
	if (addr < AD2S1210_REG_MIN) {
		return -EINVAL;
	}

	/* Set device to CONFIG mode */
	ret = ad2s1210_set_mode_pins(dev, AD2S1210_MODE_CONFIG);
	if (ret < 0) {
		return ret;
	}

	/* Prepare data to send */
	tx_buf[0] = addr;
	tx_buf[1] = val;

	/* Write register value */
	const struct spi_buf tx_buf_arr = {
		.buf = tx_buf,
		.len = sizeof(tx_buf),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf_arr,
		.count = 1,
	};

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ad2s1210_set_resolution(const struct device *dev, enum ad2s1210_res resolution)
{
	const struct ad2s1210_config *config = dev->config;
	struct ad2s1210_data *data = dev->data;
	int ret;
	uint8_t control;

	ret = ad2s1210_reg_read(dev, AD2S1210_REG_CONTROL, &control);
	if (ret < 0) {
		return ret;
	}

	control &= ~(AD2S1210_CONTROL_RES_MASK);

	switch (resolution) {
	case AD2S1210_RES_10BIT:
		break;
	case AD2S1210_RES_12BIT:
		control |= AD2S1210_CONTROL_RES1_MASK;
		break;
	case AD2S1210_RES_14BIT:
		control |= AD2S1210_CONTROL_RES0_MASK;
		break;
	case AD2S1210_RES_16BIT:
		control |= (AD2S1210_CONTROL_RES1_MASK | AD2S1210_CONTROL_RES0_MASK);
		break;
	default:
		LOG_ERR("Invalid resolution: %d", resolution);
		return -EINVAL;
	}

	ret = ad2s1210_reg_write(dev, AD2S1210_REG_CONTROL, control);
	if (ret < 0) {
		return ret;
	}

	if (!config->have_resolution_pins) {
		data->resolution = resolution;
		return 0;
	}

	/* Set RES0 pin */
	ret = gpio_pin_set_dt(&config->resolution_gpios[AD2S1210_RES_PIN_RES0],
			      !!(control & AD2S1210_CONTROL_RES0_MASK));
	if (ret < 0) {
		return ret;
	}

	/* Set RES1 pin */
	ret = gpio_pin_set_dt(&config->resolution_gpios[AD2S1210_RES_PIN_RES1],
			      !!(control & AD2S1210_CONTROL_RES1_MASK));
	if (ret < 0) {
		return ret;
	}

	data->resolution = resolution;
	return 0;
}

static int ad2s1210_set_hysteresis(const struct device *dev, bool enable)
{
	int ret;
	uint8_t control;

	ret = ad2s1210_reg_read(dev, AD2S1210_REG_CONTROL, &control);
	if (ret < 0) {
		return ret;
	}

	control &= ~AD2S1210_ENABLE_HYSTERESIS;
	if (enable) {
		control |= AD2S1210_ENABLE_HYSTERESIS;
	}

	return ad2s1210_reg_write(dev, AD2S1210_REG_CONTROL, control);
}

static __maybe_unused int ad2s1210_hysteresis_is_enabled(const struct device *dev)
{
	int ret;
	uint8_t control;

	ret = ad2s1210_reg_read(dev, AD2S1210_REG_CONTROL, &control);
	if (ret < 0) {
		return ret;
	}

	if (control & AD2S1210_ENABLE_HYSTERESIS) {
		return 1;
	}

	return 0;
}

static __maybe_unused int ad2s1210_reinit_excitation_frequency(const struct device *dev,
							       uint16_t frequency)
{
	const struct ad2s1210_config *config = dev->config;
	int ret;
	uint32_t fcw;

	/* Calculate the frequency control word */
	fcw = (frequency * BIT(15)) / config->clock_frequency;
	if (fcw < AD2S1210_MIN_FCW || fcw > AD2S1210_MAX_FCW) {
		return -EINVAL;
	}

	/* Write the frequency control word */
	ret = ad2s1210_reg_write(dev, AD2S1210_REG_EXCIT_FREQ, (int8_t)fcw);
	if (ret < 0) {
		return ret;
	}

	/* Software reset to reinitialize excitation frequency output */
	return ad2s1210_reg_write(dev, AD2S1210_REG_SOFT_RESET, 0);
}

static int ad2s1210_init(const struct device *dev) /* cppcheck-suppress unusedFunction */
{
	const struct ad2s1210_config *config = dev->config;
	struct ad2s1210_data *data = dev->data;
	int ret;
	enum ad2s1210_res resolution;

	/* Check if SPI bus is ready */
	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	/* Check if sample GPIO port is ready */
	if (!gpio_is_ready_dt(&config->sample_gpio)) {
		LOG_ERR("Sample GPIO port not ready");
		return -ENODEV;
	}

	/* Check if mode GPIO ports are ready (required) */
	for (uint8_t idx = 0; idx < (uint8_t)AD2S1210_MODE_PIN_MAX_VAL; idx++) {
		if (!gpio_is_ready_dt(&config->mode_gpios[idx])) {
			LOG_ERR("Mode GPIO %d port not ready", idx);
			return -ENODEV;
		}
	}

	/* Check if resolution GPIO ports are ready */

	if (config->have_resolution_pins) {
		for (uint8_t idx = 0; idx < (uint8_t)AD2S1210_RES_PIN_MAX_VAL; idx++) {
			if (!gpio_is_ready_dt(&config->resolution_gpios[idx])) {
				LOG_ERR("Resolution GPIO %d port not ready", idx);
				return -ENODEV;
			}
		}
	}

	/* Check if reset GPIO port is ready */
	if (config->reset_gpio.port && !gpio_is_ready_dt(&config->reset_gpio)) {
		LOG_ERR("Reset GPIO port not ready");
		return -ENODEV;
	}

	/* Check if fault GPIO ports are ready */
	for (uint8_t idx = 0; idx < (uint8_t)AD2S1210_FAULT_PIN_MAX_VAL; idx++) {
		if (config->fault_gpios[idx].port && !gpio_is_ready_dt(&config->fault_gpios[idx])) {
			LOG_ERR("Fault GPIO %d port not ready", idx);
			return -ENODEV;
		}
	}

	/* Configure sample pin as output */
	ret = gpio_pin_configure_dt(&config->sample_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure sample GPIO (%d)", ret);
		return ret;
	}

	/* Configure mode selection pins (required), default to CONFIG */
	for (uint8_t i = 0; i < (uint8_t)AD2S1210_MODE_PIN_MAX_VAL; i++) {
		ret = gpio_pin_configure_dt(&config->mode_gpios[i], GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure mode GPIO %d (%d)", i, ret);
			return ret;
		}
	}

	/* Configure resolution selection pins */
	if (config->have_resolution_pins) {
		for (uint8_t i = 0; i < (uint8_t)AD2S1210_RES_PIN_MAX_VAL; i++) {
			ret = gpio_pin_configure_dt(&config->resolution_gpios[i],
						    GPIO_OUTPUT_INACTIVE);
			if (ret < 0) {
				LOG_ERR("Could not configure resolution GPIO %d (%d)", i, ret);
				return ret;
			}
		}
	}

	/* Configure reset pin */
	if (config->reset_gpio.port) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}
	}

	/* Configure fault indication pins as inputs */
	for (uint8_t i = 0; i < (uint8_t)AD2S1210_FAULT_PIN_MAX_VAL; i++) {
		if (config->fault_gpios[i].port) {
			ret = gpio_pin_configure_dt(&config->fault_gpios[i], GPIO_INPUT);
			if (ret < 0) {
				LOG_ERR("Could not configure fault GPIO %d (%d)", i, ret);
				return ret;
			}
		}
	}

	data->mode = AD2S1210_MODE_CONFIG;

	/* Set initial mode to CONFIG */
	ret = ad2s1210_set_mode_pins(dev, AD2S1210_MODE_CONFIG);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Check clock frequency is 8 or 10 MHz as these are the only values we
	 * have in the velocity lookup table.
	 */
	switch (config->clock_frequency) {
	case 8192000:
		data->clock = AD2S1210_CLOCKIN_8MHZ192;
		break;
	case 10240000:
		data->clock = AD2S1210_CLOCKIN_10MHZ24;
		break;
	default:
		data->clock = AD2S1210_CLOCKIN_UNKNOWN;
		return -EIO;
	}

	/* Set resolution based on configuration */
	if (!config->have_resolution_pins) {
		/* Convert assigned resolution bits to enum */
		switch (config->assigned_resolution_bits) {
		case 10:
			resolution = AD2S1210_RES_10BIT;
			break;
		case 12:
			resolution = AD2S1210_RES_12BIT;
			break;
		case 14:
			resolution = AD2S1210_RES_14BIT;
			break;
		case 16:
			resolution = AD2S1210_RES_16BIT;
			break;
		default:
			LOG_ERR("Invalid assigned resolution bits: %d",
				config->assigned_resolution_bits);
			return -EINVAL;
		}
	} else {
		/* Default to 16 bits when using GPIO control */
		resolution = AD2S1210_RES_16BIT;
	}

	/* Set resolution */
	ret = ad2s1210_set_resolution(dev, resolution);
	if (ret < 0) {
		LOG_ERR("Could not set resolution (%d)", ret);
		return ret;
	}

	/* Enable hysteresis by default */
	ret = ad2s1210_set_hysteresis(dev, true);
	if (ret < 0) {
		LOG_ERR("Could not set hysteresis");
		return ret;
	}
	return 0;
}

/** Macro used to initialize one ad2s1210 driver instance */
#define AD2S1210_INIT(i)                                                                           \
	static struct ad2s1210_data ad2s1210_data_##i;                                             \
                                                                                                   \
	static const struct ad2s1210_config ad2s1210_config_##i = {                                \
		.spi = SPI_DT_SPEC_INST_GET(i, SPI_WORD_SET(8)),                                   \
		.sample_gpio = GPIO_DT_SPEC_INST_GET(i, sample_gpios),                             \
		.mode_gpios =                                                                      \
			{                                                                          \
				GPIO_DT_SPEC_INST_GET_BY_IDX(i, mode_gpios, 0),                    \
				GPIO_DT_SPEC_INST_GET_BY_IDX(i, mode_gpios, 1),                    \
			},                                                                         \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(i, reset_gpios),                               \
		.resolution_gpios =                                                                \
			{                                                                          \
				GPIO_DT_SPEC_INST_GET_BY_IDX_OR(i, resolution_gpios, 0, {0}),      \
				GPIO_DT_SPEC_INST_GET_BY_IDX_OR(i, resolution_gpios, 1, {0}),      \
			},                                                                         \
		.fault_gpios =                                                                     \
			{                                                                          \
				GPIO_DT_SPEC_INST_GET_BY_IDX_OR(i, fault_gpios, 0, {0}),           \
				GPIO_DT_SPEC_INST_GET_BY_IDX_OR(i, fault_gpios, 1, {0}),           \
			},                                                                         \
		.clock_frequency = DT_INST_PROP(i, clock_frequency),                               \
		.assigned_resolution_bits = DT_INST_PROP(i, assigned_resolution_bits),             \
		.have_resolution_pins = (DT_INST_NODE_HAS_PROP(i, resolution_gpios)),              \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(i, ad2s1210_init, NULL, &ad2s1210_data_##i,                   \
				     &ad2s1210_config_##i, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &ad2s1210_api);

DT_INST_FOREACH_STATUS_OKAY(AD2S1210_INIT)
