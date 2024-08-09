/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adxl34x.h>
#include <zephyr/drivers/sensor_attribute_types.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>

#include "adxl34x_reg.h"
#include "adxl34x_emul.h"
#include "adxl34x_private.h"
#include "adxl34x_convert.h"

LOG_MODULE_DECLARE(adxl34x, CONFIG_SENSOR_LOG_LEVEL);

#define Q31_SCALE               ((int64_t)INT32_MAX + 1)
#define DOUBLE_TO_Q31(x, shift) ((int64_t)((double)(x) * (double)Q31_SCALE) >> shift)
#define Q31_TO_DOUBLE(x, shift) ((double)((int64_t)(x) << shift) / (double)Q31_SCALE)
#define G_TO_MS2(g)             (g * SENSOR_G / 1000000LL)
#define MS2_TO_G(ms)            (ms / SENSOR_G * 1000000LL)

/**
 * @brief Conversion from register values to their lsb values in ug
 * @var adxl34x_lsb_conv
 */
static const uint16_t adxl34x_lsb_conv[] = {
	[ADXL34X_RANGE_2G] = 3900,
	[ADXL34X_RANGE_4G] = 7800,
	[ADXL34X_RANGE_8G] = 15600,
	[ADXL34X_RANGE_16G] = 31200,
};

/**
 * Convert a q31 type value to a raw register value.
 *
 * @param[in] value The q31 to convert
 * @param[in] shift The shift value to use for the q31 value provided
 * @param[in] range The accelero range used when the value was collected
 * @return 0 if successful, negative errno code if failure.
 */
static int32_t adxl34x_convert_q31_to_raw(const q31_t *value, uint8_t shift,
					  enum adxl34x_accel_range range)
{
	double ms2 = Q31_TO_DOUBLE(*value, shift);
	double u_g = MS2_TO_G(ms2) * 1000000;
	const uint16_t u_g_lsb = adxl34x_lsb_conv[range];
	const int32_t raw = (int32_t)(u_g / u_g_lsb);
	return raw;
}

/**
 * Read from the virtual device registery
 *
 * @param[in] target Pointer to the emulation device
 * @param[in] address The register address to read from
 * @return 0 if successful, negative errno code if failure.
 */
static uint8_t reg_read(const struct emul *target, uint8_t address)
{
	const struct adxl34x_emul_data *data = (struct adxl34x_emul_data *)target->data;
	uint8_t val = data->reg[address];

	switch (address) {
	case ADXL34X_REG_DEVID:
	case ADXL34X_REG_THRESH_TAP:
	case ADXL34X_REG_OFSX:
	case ADXL34X_REG_OFSY:
	case ADXL34X_REG_OFSZ:
	case ADXL34X_REG_DUR:
	case ADXL34X_REG_LATENT:
	case ADXL34X_REG_WINDOW:
	case ADXL34X_REG_THRESH_ACT:
	case ADXL34X_REG_THRESH_INACT:
	case ADXL34X_REG_TIME_INACT:
	case ADXL34X_REG_ACT_INACT_CTL:
	case ADXL34X_REG_THRESH_FF:
	case ADXL34X_REG_TIME_FF:
	case ADXL34X_REG_TAP_AXES:
	case ADXL34X_REG_ACT_TAP_STATUS:
	case ADXL34X_REG_BW_RATE:
	case ADXL34X_REG_POWER_CTL:
	case ADXL34X_REG_INT_ENABLE:
	case ADXL34X_REG_INT_MAP:
	case ADXL34X_REG_INT_SOURCE:
	case ADXL34X_REG_DATA_FORMAT:
	case ADXL34X_REG_DATA:
	case ADXL34X_REG_DATAX1:
	case ADXL34X_REG_DATAY0:
	case ADXL34X_REG_DATAY1:
	case ADXL34X_REG_DATAZ0:
	case ADXL34X_REG_DATAZ1:
	case ADXL34X_REG_FIFO_CTL:
	case ADXL34X_REG_FIFO_STATUS:
		break;

		/* Additional registers for the ADXL344 and ADXL346. */
	case ADXL34X_REG_TAP_SIGN:
	case ADXL34X_REG_ORIENT_CONF:
	case ADXL34X_REG_ORIENT: {
		const uint8_t devid = data->reg[ADXL34X_REG_DEVID];

		if (devid != ADXL344_DEVID && devid != ADXL346_DEVID) {
			LOG_WRN("Trying to read from unknown address 0x%02X", address);
			return -1;
		}
		break;
	}
	default:
		LOG_WRN("Trying to read from unknown address 0x%02X", address);
		return -1;
	}
	return val;
}

/**
 * Write from the virtual device registery
 *
 * @param[in] target Pointer to the emulation device
 * @param[in] address The register address to write to
 * @param[in] val The value of the register to write to
 */
static void reg_write(const struct emul *target, uint8_t address, uint8_t val)
{
	struct adxl34x_emul_data *data = (struct adxl34x_emul_data *)target->data;

	switch (address) {
	case ADXL34X_REG_THRESH_TAP:
	case ADXL34X_REG_OFSX:
	case ADXL34X_REG_OFSY:
	case ADXL34X_REG_OFSZ:
	case ADXL34X_REG_DUR:
	case ADXL34X_REG_LATENT:
	case ADXL34X_REG_WINDOW:
	case ADXL34X_REG_THRESH_ACT:
	case ADXL34X_REG_THRESH_INACT:
	case ADXL34X_REG_TIME_INACT:
	case ADXL34X_REG_ACT_INACT_CTL:
	case ADXL34X_REG_THRESH_FF:
	case ADXL34X_REG_TIME_FF:
	case ADXL34X_REG_TAP_AXES:
	case ADXL34X_REG_BW_RATE:
	case ADXL34X_REG_POWER_CTL:
	case ADXL34X_REG_INT_ENABLE:
	case ADXL34X_REG_INT_MAP:
	case ADXL34X_REG_DATA_FORMAT:
	case ADXL34X_REG_FIFO_CTL:
		break;

	case ADXL34X_REG_DEVID:
	case ADXL34X_REG_ACT_TAP_STATUS:
	case ADXL34X_REG_INT_SOURCE:
	case ADXL34X_REG_DATA:
	case ADXL34X_REG_DATAX1:
	case ADXL34X_REG_DATAY0:
	case ADXL34X_REG_DATAY1:
	case ADXL34X_REG_DATAZ0:
	case ADXL34X_REG_DATAZ1:
	case ADXL34X_REG_FIFO_STATUS:
		LOG_WRN("Trying to write to read only address 0x%02X", address);
		return;

		/* Additional registers for the ADXL344 and ADXL346. */
	case ADXL34X_REG_TAP_SIGN:
	case ADXL34X_REG_ORIENT:
		LOG_WRN("Trying to write to read only (and/or unknown) address 0x%02X", address);
		return;

	case ADXL34X_REG_ORIENT_CONF: {
		const uint8_t devid = data->reg[ADXL34X_REG_DEVID];

		if (devid != ADXL344_DEVID && devid != ADXL346_DEVID) {
			LOG_WRN("Trying to write to unknown address 0x%02X", address);
			return;
		}
		break;
	}
	default:
		LOG_WRN("Trying to write to unknown address 0x%02X", address);
		return;
	}
	data->reg[address] = val;
}

/**
 * Callback API for initialising the emulation device
 *
 * @param[in] target Pointer to the emulation device
 * @param[in] parent Device that is using the emulator
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_emul_init(const struct emul *target, const struct device *parent)
{
	struct adxl34x_emul_data *data = (struct adxl34x_emul_data *)target->data;
	uint8_t *reg = data->reg;

	LOG_DBG("Setting emulated device registers of %s/%s to default", parent->name,
		target->dev->name);
	/* Set the register defaults */
	reg[ADXL34X_REG_DEVID] = ADXL344_DEVID;
	reg[ADXL34X_REG_BW_RATE] = 0x0A;
	reg[ADXL34X_REG_INT_SOURCE] = 0x02;
	reg[ADXL34X_REG_ORIENT_CONF] = 0x25;
	return 0;
}

/**
 * Callback API for setting an expected value for a given channel
 *
 * @param[in] target Pointer to the emulation device
 * @param[in] ch Sensor channel to set expected value for
 * @param[in] value Expected value in fixed-point format using standard SI unit for sensor type
 * @param[in] shift Shift value (scaling factor) applied to @p value
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_emul_set_channel(const struct emul *target, struct sensor_chan_spec ch,
				    const q31_t *value, int8_t shift)
{
	struct adxl34x_emul_data *data = (struct adxl34x_emul_data *)target->data;
	uint8_t *reg = data->reg;

	if (reg == NULL || value == NULL) {
		return -ENOTSUP;
	}
	if (ch.chan_type != SENSOR_CHAN_ACCEL_X && ch.chan_type != SENSOR_CHAN_ACCEL_Y &&
	    ch.chan_type != SENSOR_CHAN_ACCEL_Z) {
		return -ENOTSUP;
	}

	const enum adxl34x_accel_range range =
		FIELD_GET(ADXL34X_REG_DATA_FORMAT_RANGE, reg[ADXL34X_REG_DATA_FORMAT]);
	__ASSERT_NO_MSG(range >= ADXL34X_RANGE_2G && range <= ADXL34X_RANGE_16G);
	const int16_t reg_value =
		CLAMP(adxl34x_convert_q31_to_raw(value, shift, range), INT16_MIN, INT16_MAX);
	const uint8_t base_address = ADXL34X_REG_DATA + (ch.chan_type - SENSOR_CHAN_ACCEL_X) * 2;

	if (base_address != ADXL34X_REG_DATAX0 && base_address != ADXL34X_REG_DATAY0 &&
	    base_address != ADXL34X_REG_DATAZ0) {
		return -ERANGE;
	}

	/* Set the FIFO value */
	sys_put_le16(reg_value, &reg[base_address]);
	/* Set the FIFO number of entries */
	reg[ADXL34X_REG_FIFO_STATUS] = FIELD_PREP(ADXL34X_REG_FIFO_STATUS_ENTRIES, 1);
	return 0;
}

/**
 * Callback API for getting the supported sample value range and tolerance for a given channel
 *
 * @param[in] target Pointer to the emulation device
 * @param[in] ch The channel to request info for. If @p ch is unsupported, return `-ENOTSUP`
 * @param[out] lower Minimum supported sample value in SI units, fixed-point format
 * @param[out] upper Maximum supported sample value in SI units, fixed-point format
 * @param[out] epsilon Tolerance to use comparing expected and actual values to account for rounding
 *             and sensor precision issues. This can usually be set to the minimum sample value step
 *             size. Uses SI units and fixed-point format.
 * @param[out] shift The shift value (scaling factor) associated with @p lower, @p upper, and
 *             @p epsilon.
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_emul_get_sample_range(const struct emul *target, struct sensor_chan_spec ch,
					 q31_t *lower, q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	struct adxl34x_emul_data *data = (struct adxl34x_emul_data *)target->data;
	uint8_t *reg = data->reg;

	if (reg == NULL) {
		return -ENOTSUP;
	}
	if (ch.chan_type != SENSOR_CHAN_ACCEL_X && ch.chan_type != SENSOR_CHAN_ACCEL_Y &&
	    ch.chan_type != SENSOR_CHAN_ACCEL_Z && ch.chan_type != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	const enum adxl34x_accel_range range =
		FIELD_GET(ADXL34X_REG_DATA_FORMAT_RANGE, reg[ADXL34X_REG_DATA_FORMAT]);
	__ASSERT_NO_MSG(range >= ADXL34X_RANGE_2G && range <= ADXL34X_RANGE_16G);
	*shift = adxl34x_shift_conv[range];
	double epsilon_ms2 = G_TO_MS2((double)adxl34x_lsb_conv[range] / 1000000LL);
	*epsilon = DOUBLE_TO_Q31(epsilon_ms2, *shift);
	double upper_ms2 = G_TO_MS2((double)adxl34x_max_g_conv[range]);
	*upper = DOUBLE_TO_Q31(upper_ms2, *shift);
	*lower = -*upper;
	return 0;
}

/**
 * Set the emulator's offset attribute value
 *
 * @param[in] target Pointer to the emulation device
 * @param[in] ch The channel to use
 * @param[in] value The offset to use
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_emul_set_attr_offset(const struct emul *target, struct sensor_chan_spec ch,
					const struct sensor_three_axis_attribute *value)
{
	struct adxl34x_emul_data *data = (struct adxl34x_emul_data *)target->data;
	struct adxl34x_dev_data *dev_data = target->dev->data;
	struct adxl34x_cfg *cfg = &dev_data->cfg;

	uint8_t *reg = data->reg;
	const uint8_t shift = value->shift;

	if (ch.chan_type == SENSOR_CHAN_ACCEL_X || ch.chan_type == SENSOR_CHAN_ACCEL_XYZ) {
		const int8_t offset_x =
			CLAMP(adxl34x_convert_q31_to_raw(&value->x, shift, ADXL34X_RANGE_8G),
			      INT8_MIN, INT8_MAX);
		reg[ADXL34X_REG_OFSX] = offset_x;
		cfg->ofsx = offset_x; /* Update cached value as well. */
	}
	if (ch.chan_type == SENSOR_CHAN_ACCEL_Y || ch.chan_type == SENSOR_CHAN_ACCEL_XYZ) {
		const uint8_t offset_y =
			CLAMP(adxl34x_convert_q31_to_raw(&value->y, shift, ADXL34X_RANGE_8G),
			      INT8_MIN, UINT8_MAX);
		reg[ADXL34X_REG_OFSY] = offset_y;
		cfg->ofsy = offset_y; /* Update cached value as well. */
	}
	if (ch.chan_type == SENSOR_CHAN_ACCEL_Z || ch.chan_type == SENSOR_CHAN_ACCEL_XYZ) {
		const uint8_t offset_z =
			CLAMP(adxl34x_convert_q31_to_raw(&value->z, shift, ADXL34X_RANGE_8G),
			      INT8_MIN, UINT8_MAX);
		reg[ADXL34X_REG_OFSZ] = offset_z;
		cfg->ofsz = offset_z; /* Update cached value as well. */
	}
	return 0;
}

/**
 * Get metadata about the offset attribute.
 *
 * @param[in] target Pointer to the emulation device
 * @param[out] min The minimum value the attribute can be set to
 * @param[out] max The maximum value the attribute can be set to
 * @param[out] increment The value that the attribute increases by for every LSB
 * @param[out] shift The shift for @p min, @p max, and @p increment
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_emul_get_attr_offset_metadata(const struct emul *target, q31_t *min, q31_t *max,
						 q31_t *increment, int8_t *shift)
{
	*shift = adxl34x_shift_conv[ADXL34X_RANGE_2G];
	double min_ms2 = G_TO_MS2(-1.9968); /* -128 * 0.0156 */
	*min = DOUBLE_TO_Q31(min_ms2, *shift);
	double max_ms2 = G_TO_MS2(1.9812); /* 127 * 0.0156 */
	*max = DOUBLE_TO_Q31(max_ms2, *shift);
	double increment_ms2 = G_TO_MS2(0.0156);
	*increment = DOUBLE_TO_Q31(increment_ms2, *shift);
	return 0;
}

/**
 * Callback API to set the attribute value(s) of a given chanel
 *
 * @param[in] target Pointer to the emulation device
 * @param[in] ch The channel to use. If @p ch is unsupported, return `-ENOTSUP`
 * @param[in] attribute The attribute to set
 * @param[in] value the value to use (cast according to the channel/attribute pair)
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_emul_set_attribute(const struct emul *target, struct sensor_chan_spec ch,
				      enum sensor_attribute attribute, const void *value)
{
	const struct adxl34x_emul_data *data = (struct adxl34x_emul_data *)target->data;
	const uint8_t *reg = data->reg;

	if (reg == NULL || value == NULL) {
		return -ENOTSUP;
	}
	if (ch.chan_type != SENSOR_CHAN_ACCEL_X && ch.chan_type != SENSOR_CHAN_ACCEL_Y &&
	    ch.chan_type != SENSOR_CHAN_ACCEL_Z && ch.chan_type != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	switch (attribute) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
	case SENSOR_ATTR_SLOPE_TH:
	case SENSOR_ATTR_SLOPE_DUR:
	case SENSOR_ATTR_HYSTERESIS:
	case SENSOR_ATTR_OVERSAMPLING:
	case SENSOR_ATTR_FULL_SCALE:
		break;
	case SENSOR_ATTR_OFFSET:
		return adxl34x_emul_set_attr_offset(target, ch, value);
	case SENSOR_ATTR_CALIB_TARGET:
	case SENSOR_ATTR_CONFIGURATION:
	case SENSOR_ATTR_CALIBRATION:
	case SENSOR_ATTR_FEATURE_MASK:
	case SENSOR_ATTR_ALERT:
	case SENSOR_ATTR_FF_DUR:
	case SENSOR_ATTR_BATCH_DURATION:
	case SENSOR_ATTR_COMMON_COUNT:
	case SENSOR_ATTR_GAIN:
	case SENSOR_ATTR_RESOLUTION:
		break;
	default:
		LOG_ERR("Unknown attribute");
		return -ENOTSUP;
	}
	return -ENOTSUP;
}

/**
 * Callback API to get metadata about an attribute
 *
 * @param[in] target Pointer to the emulation device
 * @param[in] ch The channel to request info for. If @p ch is unsupported, return '-ENOTSUP'
 * @param[in] attribute The attribute to request info for. If @p attribute is unsupported, return
 *                      '-ENOTSUP'
 * @param[out] min The minimum value the attribute can be set to
 * @param[out] max The maximum value the attribute can be set to
 * @param[out] increment The value that the attribute increases by for every LSB
 * @param[out] shift The shift for @p min, @p max, and @p increment
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_emul_get_attribute_metadata(const struct emul *target,
					       struct sensor_chan_spec ch,
					       enum sensor_attribute attribute, q31_t *min,
					       q31_t *max, q31_t *increment, int8_t *shift)
{
	const struct adxl34x_emul_data *data = (struct adxl34x_emul_data *)target->data;
	const uint8_t *reg = data->reg;

	if (reg == NULL) {
		return -ENOTSUP;
	}
	if (ch.chan_type != SENSOR_CHAN_ACCEL_X && ch.chan_type != SENSOR_CHAN_ACCEL_Y &&
	    ch.chan_type != SENSOR_CHAN_ACCEL_Z && ch.chan_type != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	switch (attribute) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
	case SENSOR_ATTR_SLOPE_TH:
	case SENSOR_ATTR_SLOPE_DUR:
	case SENSOR_ATTR_HYSTERESIS:
	case SENSOR_ATTR_OVERSAMPLING:
	case SENSOR_ATTR_FULL_SCALE:
		break;
	case SENSOR_ATTR_OFFSET:
		return adxl34x_emul_get_attr_offset_metadata(target, min, max, increment, shift);
	case SENSOR_ATTR_CALIB_TARGET:
	case SENSOR_ATTR_CONFIGURATION:
	case SENSOR_ATTR_CALIBRATION:
	case SENSOR_ATTR_FEATURE_MASK:
	case SENSOR_ATTR_ALERT:
	case SENSOR_ATTR_FF_DUR:
	case SENSOR_ATTR_BATCH_DURATION:
	case SENSOR_ATTR_COMMON_COUNT:
	case SENSOR_ATTR_GAIN:
	case SENSOR_ATTR_RESOLUTION:
		break;
	default:
		LOG_ERR("Unknown attribute");
		return -ENOTSUP;
	}
	return -ENOTSUP;
}

/**
 * @brief The sensor driver emulator API callbacks
 * @var adxl34x_emul_api
 */
static const struct emul_sensor_driver_api adxl34x_emul_api = {
	.set_channel = adxl34x_emul_set_channel,
	.get_sample_range = adxl34x_emul_get_sample_range,
	.set_attribute = adxl34x_emul_set_attribute,
	.get_attribute_metadata = adxl34x_emul_get_attribute_metadata,
};

#ifdef CONFIG_ADXL34X_BUS_SPI

/**
 * Callback API to emulate spi communication
 *
 * Passes SPI messages to the emulator. The emulator updates the data with what
 * was read back.
 *
 * @param[in] target Pointer to the emulation device
 * @param[in] config Pointer to a valid spi_config structure instance
 * @param[in] tx_bufs Buffer array where data to be sent originates from, or NULL if none.
 * @param[in] rx_bufs Buffer array where data to be read will be written to, or NULL if none.
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_spi_emul_io(const struct emul *target, const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	const struct spi_buf *tx, *txd, *rxd;
	unsigned int address, val;
	int count;
	bool is_read_cmd, is_multi_byte;

	ARG_UNUSED(config);

	__ASSERT_NO_MSG(tx_bufs || rx_bufs);
	__ASSERT_NO_MSG(!tx_bufs || !rx_bufs || tx_bufs->count == rx_bufs->count);
	count = tx_bufs ? tx_bufs->count : rx_bufs->count;

	if (count != 2) {
		LOG_DBG("Unsupported nr of packages (%d) in spi transaction", count);
		return -EIO;
	}
	tx = tx_bufs->buffers;
	txd = &tx_bufs->buffers[1];
	rxd = rx_bufs ? &rx_bufs->buffers[1] : NULL;

	if (tx->len != 1) {
		LOG_DBG("Unsupported nr of bytes (%d) in spi transaction", tx->len);
		return -EIO;
	}

	address = *(uint8_t *)tx->buf;
	is_read_cmd = address & ADXL34X_SPI_MSG_READ;
	is_multi_byte = address & ADXL34X_SPI_MULTI_BYTE;
	address &= ~ADXL34X_SPI_MSG_READ;
	address &= ~ADXL34X_SPI_MULTI_BYTE;

	if (is_read_cmd && rxd == NULL) {
		LOG_DBG("Spi read transaction, but no read buffer supplied");
		return -EINVAL;
	}
	if (is_multi_byte && txd->len <= 1) {
		LOG_DBG("Spi transaction contains single byte, but multi-bit is set");
		return -EINVAL;
	}
	if (!is_multi_byte && txd->len > 1) {
		LOG_DBG("Spi transaction contains multiple bytes, but multi-bit is not set");
		return -EINVAL;
	}

	if (is_read_cmd) {
		for (int i = 0; i < txd->len; ++i) {
			((uint8_t *)rxd->buf)[i] = reg_read(target, address + i);
			LOG_DBG("SPI read - address:0x%02X, value:0x%02X", address + i,
				((uint8_t *)rxd->buf)[i]);
		}
	} else if (txd->len == 1) {
		val = *(uint8_t *)txd->buf;
		LOG_DBG("SPI write - address:0x%02X, value:0x%02X", address, val);
		reg_write(target, address, val);
	} else {
		LOG_DBG("Unsupported nr of bytes (%d) in spi write transaction", txd->len);
		return -EIO;
	}
	return 0;
}

/**
 * @brief The sensor driver emulator spi API callbacks
 * @var adxl34x_spi_emul_api
 */
static struct spi_emul_api adxl34x_spi_emul_api = {
	.io = adxl34x_spi_emul_io,
};

#endif /* CONFIG_ADXL34X_BUS_SPI */

#ifdef CONFIG_ADXL34X_BUS_I2C

/**
 * Callback API to emulate a i2c transfer
 *
 * @param[in] target Pointer to the emulation device
 * @param[out] msgs Array of messages to transfer. For 'read' messages, this function
 *                  updates the 'buf' member with the data that was read.
 * @param[in] num_msgs Number of messages to transfer
 * @param[in] addr Address of the I2C target device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_i2c_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				     int addr)
{
	if (msgs == NULL || msgs[0].flags & I2C_MSG_READ) {
		LOG_ERR("Unexpected i2c message");
		return -EIO;
	}

	const uint8_t address = msgs[0].buf[0];

	if (num_msgs == 1 && ((msgs[0].flags & I2C_MSG_READ) == 0)) {
		/* I2C write transaction. */
		LOG_DBG("I2C write - address:0x%02X, value:0x%02X", address, msgs[0].buf[1]);
		if (msgs[0].len != 2) {
			LOG_ERR("Unexpected i2c message length %d", msgs[0].len);
			return -EIO;
		}
		reg_write(target, address, msgs[0].buf[1]);

	} else if (num_msgs == 2 && msgs[1].flags & I2C_MSG_READ) {
		/* I2C read transaction. */
		for (int i = 0; i < msgs[1].len; i++) {
			msgs[1].buf[i] = reg_read(target, address + i);
			LOG_DBG("I2C read - address:0x%02X, value:0x%02X", address + i,
				msgs[1].buf[i]);
		}
	} else {
		LOG_ERR("Unexpected i2c message - address:0x%02X", address);
		return -EIO;
	}
	return 0;
}

/**
 * @brief The sensor driver emulator i2c API callbacks
 * @var adxl34x_i2c_emul_api
 */
static struct i2c_emul_api adxl34x_i2c_emul_api = {
	.transfer = adxl34x_i2c_emul_transfer,
};

#endif /* CONFIG_ADXL34X_BUS_I2C */

#define ADXL34X_EMUL_DEVICE(i)                                                                     \
	static struct adxl34x_emul_data adxl34x_emul_data_##i;                                     \
                                                                                                   \
	static const struct adxl34x_emul_config adxl34x_emul_config_##i = {                        \
		.addr = DT_INST_REG_ADDR(i),                                                       \
	};                                                                                         \
                                                                                                   \
	EMUL_DT_INST_DEFINE(i, adxl34x_emul_init, &adxl34x_emul_data_##i,                          \
			    &adxl34x_emul_config_##i,                                              \
			    COND_CODE_1(DT_INST_ON_BUS(i, spi), (&adxl34x_spi_emul_api),           \
					(&adxl34x_i2c_emul_api)),                                  \
			    &adxl34x_emul_api)

DT_INST_FOREACH_STATUS_OKAY(ADXL34X_EMUL_DEVICE)
