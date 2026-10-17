/*
 * Copyright (c) 2026 Darshan Maru <darshanmaru1411@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_opt4048

#include <errno.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/work.h>
#include <zephyr/sys/byteorder.h>

#include "opt4048.h"

LOG_MODULE_REGISTER(OPT4048, CONFIG_SENSOR_LOG_LEVEL);

static const uint32_t opt4048_conv_time_us[] = {
	OPT4048_CONV_TIME_600US, OPT4048_CONV_TIME_1MS,   OPT4048_CONV_TIME_1MS8,
	OPT4048_CONV_TIME_3MS4,  OPT4048_CONV_TIME_6MS5,  OPT4048_CONV_TIME_12MS7,
	OPT4048_CONV_TIME_25MS,  OPT4048_CONV_TIME_50MS,  OPT4048_CONV_TIME_100MS,
	OPT4048_CONV_TIME_200MS, OPT4048_CONV_TIME_400MS, OPT4048_CONV_TIME_800MS,
};

/*
 * Datasheet eq. 16 example conversion matrix, rows are CH0..CH3, columns are X, Y, Z.
 * Every coefficient is the datasheet value multiplied by 1e12 (see OPT4048_MATRIX_TO_MICRO).
 * The matrix can be tuned for a given cover glass and optical cavity.
 */
static const int32_t opt4048_matrix[OPT4048_NUM_CHANNELS][3] = {
	{OPT4048_MATRIX_CH0_X, OPT4048_MATRIX_CH0_Y, OPT4048_MATRIX_CH0_Z},
	{OPT4048_MATRIX_CH1_X, OPT4048_MATRIX_CH1_Y, OPT4048_MATRIX_CH1_Z},
	{OPT4048_MATRIX_CH2_X, OPT4048_MATRIX_CH2_Y, OPT4048_MATRIX_CH2_Z},
	{0, 0, 0}, /* CH3: not used by the XYZ calculation */
};

static int opt4048_reg_read(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct opt4048_config *cfg = dev->config;
	uint8_t buf[2];
	int ret;

	ret = i2c_burst_read_dt(&cfg->i2c, reg, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be16(buf);

	return 0;
}

static int opt4048_reg_write(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct opt4048_config *cfg = dev->config;
	uint8_t buf[3];

	buf[0] = reg;
	sys_put_be16(val, &buf[1]);

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

static int opt4048_cfg_update(const struct device *dev, uint16_t mask, uint16_t val)
{
	struct opt4048_data *data = dev->data;
	uint16_t new_cfg = (data->cfg & ~mask) | (val & mask);
	int ret;

	ret = opt4048_reg_write(dev, OPT4048_REG_CONFIG, new_cfg);
	if (ret == 0) {
		data->cfg = new_cfg;
	}

	return ret;
}

static int opt4048_cfg2_update(const struct device *dev, uint16_t mask, uint16_t val)
{
	struct opt4048_data *data = dev->data;
	uint16_t new_cfg = (data->cfg2 & ~mask) | (val & mask);
	int ret;

	ret = opt4048_reg_write(dev, OPT4048_REG_THRESHOLD_CFG, new_cfg);
	if (ret == 0) {
		data->cfg2 = new_cfg;
	}

	return ret;
}

static uint8_t opt4048_calc_crc(uint8_t exponent, uint32_t mant, uint8_t cnt)
{
	uint8_t x0, x1, x2, x3;

	/* Each CRC bit is the parity of a fixed subset of the exponent, mantissa and counter bits
	 */
	x0 = __builtin_parity(exponent & 0xF) ^ __builtin_parity(mant & 0xFFFFF) ^
	     __builtin_parity(cnt & 0xF);
	x1 = __builtin_parity(cnt & 0xA) ^ __builtin_parity(mant & 0xAAAAA) ^
	     __builtin_parity(exponent & 0xA);
	x2 = __builtin_parity(cnt & 0x8) ^ __builtin_parity(mant & 0x88888) ^
	     __builtin_parity(exponent & 0x8);
	x3 = __builtin_parity(mant & 0x80808);

	return (x0 & 1) | ((x1 & 1) << 1) | ((x2 & 1) << 2) | ((x3 & 1) << 3);
}

static int opt4048_encode_threshold(uint32_t codes, uint16_t *reg)
{
	uint8_t exponent;

	/* Try the smallest exponent first: it gives the finest threshold resolution */
	for (exponent = 0; exponent <= OPT4048_MAX_EXPONENT; exponent++) {
		uint32_t res = codes >> (8 + exponent);

		if (res <= OPT4048_THRESH_RES_MASK) {
			*reg = FIELD_PREP(OPT4048_THRESH_EXP_MASK, exponent) |
			       FIELD_PREP(OPT4048_THRESH_RES_MASK, res);
			return 0;
		}
	}

	return -ERANGE;
}

static uint32_t opt4048_decode_threshold(uint16_t reg)
{
	uint8_t exponent = FIELD_GET(OPT4048_THRESH_EXP_MASK, reg);
	uint32_t res = FIELD_GET(OPT4048_THRESH_RES_MASK, reg);

	return res << (8 + exponent);
}

static bool opt4048_chan_supported(int chan)
{
	return chan == SENSOR_CHAN_LIGHT ||
	       (chan >= SENSOR_CHAN_OPT4048_RAW_CH0 && chan <= SENSOR_CHAN_OPT4048_CCT);
}

static void opt4048_calc_xyz(const uint32_t *adc, int64_t xyz[3])
{
	int64_t acc[3] = {0, 0, 0};
	int i, j;

	/* adc < 2^28 and |coef| < 2^30, so the sums fit in 64 bits */
	for (i = 0; i < OPT4048_NUM_CHANNELS; i++) {
		for (j = 0; j < 3; j++) {
			acc[j] += (int64_t)adc[i] * opt4048_matrix[i][j];
		}
	}

	/* Matrix coefficients are scaled by 1e12, bring the result to micro units */
	for (j = 0; j < 3; j++) {
		xyz[j] = acc[j] / OPT4048_MATRIX_TO_MICRO;
	}
}

static int opt4048_calc_cct(int64_t cie_x, int64_t cie_y, int64_t *cct)
{
	int64_t den = OPT4048_CCT_YE_UL - cie_y;
	int64_t n, n2, n3, cct_fx;

	/* Chromaticity must be within 0..1 and the denominator must not be zero */
	if (cie_x < 0 || cie_x > OPT4048_MICRO || cie_y < 0 || cie_y > OPT4048_MICRO || den == 0) {
		return -ERANGE;
	}

	/* Datasheet eq. 17: n, limited so that n^3 stays within 64 bits */
	n = ((cie_x - OPT4048_CCT_XE_UL) * OPT4048_FX_ONE) / den;
	if (n > OPT4048_CCT_N_MAX_FX || n < -OPT4048_CCT_N_MAX_FX) {
		return -ERANGE;
	}

	/* Datasheet eq. 18: third order polynomial in n */
	n2 = (n * n) >> OPT4048_FX_SHIFT;
	n3 = (n2 * n) >> OPT4048_FX_SHIFT;
	cct_fx = OPT4048_CCT_A3 * n3 + OPT4048_CCT_A2 * n2 + OPT4048_CCT_A1 * n +
		 OPT4048_CCT_A0 * OPT4048_FX_ONE;

	if (cct_fx < 0) {
		return -ERANGE;
	}

	/* Round to the nearest kelvin and reject values above the valid range */
	*cct = (cct_fx + OPT4048_FX_ONE / 2) >> OPT4048_FX_SHIFT;
	if (*cct >= OPT4048_CCT_MAX) {
		return -ERANGE;
	}

	return 0;
}

static int opt4048_calc_value(const uint32_t *adc, int chan, int64_t *micro)
{
	int64_t xyz[3];
	int64_t sum, cie_x, cie_y;
	int ret;

	if (!opt4048_chan_supported(chan)) {
		return -ENOTSUP;
	}

	/* Raw ADC code of CH0..CH3 */
	if (chan >= SENSOR_CHAN_OPT4048_RAW_CH0 && chan <= SENSOR_CHAN_OPT4048_RAW_CH3) {
		*micro = (int64_t)adc[chan - SENSOR_CHAN_OPT4048_RAW_CH0] * OPT4048_MICRO;
		return 0;
	}

	if (chan == SENSOR_CHAN_LIGHT) {
		/* Datasheet eq. 9: lux = adc_ch1 * 2.15e-3 */
		*micro = (int64_t)adc[1] * OPT4048_LUX_PER_CODE_UL;
		return 0;
	}

	/* All remaining channels are derived from the XYZ matrix result */
	opt4048_calc_xyz(adc, xyz);

	if (chan >= SENSOR_CHAN_OPT4048_X && chan <= SENSOR_CHAN_OPT4048_Z) {
		/* The matrix output is unscaled: scale it so that Y equals the lux value */
		int64_t lux_micro = (int64_t)adc[1] * OPT4048_LUX_PER_CODE_UL;
		int64_t ratio_fx;

		if (xyz[1] <= 0) {
			return -ENODATA;
		}

		ratio_fx = CLAMP((lux_micro << OPT4048_FX_SHIFT) / xyz[1], 0,
				 100LL << OPT4048_FX_SHIFT);
		*micro = (xyz[chan - SENSOR_CHAN_OPT4048_X] * ratio_fx) >> OPT4048_FX_SHIFT;
		return 0;
	}

	/* CIE 1931 chromaticity: x = X / (X + Y + Z), y = Y / (X + Y + Z) */
	sum = xyz[0] + xyz[1] + xyz[2];
	if (sum <= 0) {
		return -ENODATA;
	}

	if (xyz[0] < 0 || xyz[1] < 0 || xyz[2] < 0) {
		return -ERANGE;
	}

	cie_x = (xyz[0] * OPT4048_MICRO) / sum;
	cie_y = (xyz[1] * OPT4048_MICRO) / sum;

	if (chan == SENSOR_CHAN_OPT4048_CIE_X) {
		*micro = cie_x;
		return 0;
	}

	if (chan == SENSOR_CHAN_OPT4048_CIE_Y) {
		*micro = cie_y;
		return 0;
	}

	/* Only the CCT channel is left */
	ret = opt4048_calc_cct(cie_x, cie_y, micro);
	if (ret < 0) {
		return ret;
	}

	*micro *= OPT4048_MICRO; /* kelvin to micro units */

	return 0;
}

static int opt4048_read_adc_unlocked(const struct device *dev, uint32_t adc[OPT4048_NUM_CHANNELS])
{
	const struct opt4048_config *cfg = dev->config;
	struct opt4048_data *data = dev->data;
	uint8_t mode = FIELD_GET(OPT4048_CFG_MODE_MASK, data->cfg);
	uint8_t raw[OPT4048_RAW_LEN];
	int ret;
	int i;

	/* Power-down: no conversion is running, there is nothing to read */
	if (mode == OPT4048_MODE_POWERDOWN) {
		return -ENODATA;
	}

	/* One-shot: a CONFIG write starts one conversion of all four channels */
	if (mode == OPT4048_MODE_ONESHOT || mode == OPT4048_MODE_AUTO_ONESHOT) {
		uint8_t ct = FIELD_GET(OPT4048_CFG_CONVTIME_MASK, data->cfg);

		if (ct >= ARRAY_SIZE(opt4048_conv_time_us)) {
			return -EINVAL;
		}

		ret = opt4048_reg_write(dev, OPT4048_REG_CONFIG, data->cfg);
		if (ret < 0) {
			return ret;
		}

		/* Wait for 4 channel conversions plus the wake-up and I2C margin */
		k_sleep(K_USEC(4U * opt4048_conv_time_us[ct] + OPT4048_ONESHOT_MARGIN_US));
	}

	/* Burst read CH0..CH3 (8 registers, I2C burst mode is enabled) */
	ret = i2c_burst_read_dt(&cfg->i2c, OPT4048_REG_CH0_MSB, raw, sizeof(raw));
	if (ret < 0) {
		LOG_ERR("Failed to read result registers (%d)", ret);
		return ret;
	}

	for (i = 0; i < OPT4048_NUM_CHANNELS; i++) {
		/* MSB register: exponent [15:12] + mantissa [11:0] */
		uint16_t hi = sys_get_be16(&raw[i * 4]);
		/* LSB register: mantissa [7:0] + counter [3:0] + CRC [3:0] */
		uint16_t lo = sys_get_be16(&raw[i * 4 + 2]);
		uint8_t exponent = hi >> OPT4048_RESULT_EXP_SHIFT;
		uint32_t mant = ((uint32_t)(hi & OPT4048_RESULT_MSB_MANT_MASK) << 8) | (lo >> 8);
		uint8_t cnt = (lo >> 4) & 0x0F;
		uint8_t crc = lo & 0x0F;

		if (exponent > OPT4048_MAX_EXPONENT) {
			LOG_ERR("CH%d invalid exponent %u", i, exponent);
			return -EIO;
		}

		if (IS_ENABLED(CONFIG_OPT4048_CRC_CHECK)) {
			uint8_t calc = opt4048_calc_crc(exponent, mant, cnt);

			if (calc != crc) {
				LOG_ERR("CH%d CRC mismatch (got 0x%x, calc 0x%x)", i, crc, calc);
				return -EIO;
			}
		}

		/* Linearise: ADC code = mantissa * 2^exponent */
		adc[i] = mant << exponent;
	}

	return 0;
}

/* Serialises the one-shot sequence and the cached CONFIG against attr_set() */
static int opt4048_read_adc(const struct device *dev, uint32_t adc[OPT4048_NUM_CHANNELS])
{
	struct opt4048_data *data = dev->data;
	int ret;

	k_sem_take(&data->lock, K_FOREVER);
	ret = opt4048_read_adc_unlocked(dev, adc);
	k_sem_give(&data->lock);

	return ret;
}

static int opt4048_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct opt4048_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	return opt4048_read_adc(dev, data->adc);
}

static int opt4048_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct opt4048_data *data = dev->data;
	int64_t micro;
	int ret;

	ret = opt4048_calc_value(data->adc, (int)chan, &micro);
	if (ret < 0) {
		return ret;
	}

	val->val1 = (int32_t)(micro / OPT4048_MICRO);
	val->val2 = (int32_t)(micro % OPT4048_MICRO);

	return 0;
}

#ifdef CONFIG_SENSOR_ASYNC_API

static int opt4048_chan_shift(int chan)
{
	if (chan >= SENSOR_CHAN_OPT4048_RAW_CH0 && chan <= SENSOR_CHAN_OPT4048_RAW_CH3) {
		return OPT4048_SHIFT_RAW;
	}

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		return OPT4048_SHIFT_LUX;
	case SENSOR_CHAN_OPT4048_X:
	case SENSOR_CHAN_OPT4048_Y:
	case SENSOR_CHAN_OPT4048_Z:
		return OPT4048_SHIFT_XYZ;
	case SENSOR_CHAN_OPT4048_CIE_X:
	case SENSOR_CHAN_OPT4048_CIE_Y:
		return OPT4048_SHIFT_CIE;
	case SENSOR_CHAN_OPT4048_CCT:
		return OPT4048_SHIFT_CCT;
	default:
		return -ENOTSUP;
	}
}

static void opt4048_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = read_cfg->sensor;
	struct opt4048_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;
	int ret;
	uint64_t cycles;

	/* Get the receive buffer provided by the caller of sensor_read() */
	ret = rtio_sqe_rx_buf(iodev_sqe, sizeof(*edata), sizeof(*edata), &buf, &buf_len);
	if (ret != 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	edata = (struct opt4048_encoded_data *)buf;

	ret = opt4048_read_adc(dev, edata->adc);
	if (ret < 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	/* Timestamp the sample right after the read */
	ret = sensor_clock_get_cycles(&cycles);
	if (ret < 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	edata->timestamp = sensor_clock_cycles_to_ns(cycles);

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static void opt4048_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	struct rtio_work_req *req;

	ARG_UNUSED(dev);

	/* Streaming (trigger based) reads are not implemented */
	if (read_cfg->is_streaming) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	/* Only channel index 0 of the channels supported by the decoder can be requested */
	for (size_t i = 0; i < read_cfg->count; i++) {
		if (read_cfg->channels[i].chan_idx != 0 ||
		    !opt4048_chan_supported(read_cfg->channels[i].chan_type)) {
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}

	req = rtio_work_req_alloc();
	if (req == NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, opt4048_submit_sync);
}

static int opt4048_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					   uint16_t *frame_count)
{
	ARG_UNUSED(buffer);

	if (chan_spec.chan_idx != 0 || opt4048_chan_shift(chan_spec.chan_type) < 0) {
		return -ENOTSUP;
	}

	*frame_count = 1;

	return 0;
}

static int opt4048_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					 size_t *frame_size)
{
	if (chan_spec.chan_idx != 0 || opt4048_chan_shift(chan_spec.chan_type) < 0) {
		return -ENOTSUP;
	}

	*base_size = sizeof(struct sensor_q31_data);
	*frame_size = sizeof(struct sensor_q31_sample_data);

	return 0;
}

static int opt4048_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				  uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct opt4048_encoded_data *edata = (const struct opt4048_encoded_data *)buffer;
	struct sensor_q31_data *out = data_out;
	int64_t micro;
	int64_t q;
	int shift;
	int ret;

	/* A buffer holds a single frame: the iterator is already past it */
	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	shift = opt4048_chan_shift(chan_spec.chan_type);
	if (shift < 0) {
		return -ENOTSUP;
	}

	/* Convert the encoded ADC codes into the value of the requested channel */
	ret = opt4048_calc_value(edata->adc, chan_spec.chan_type, &micro);
	if (ret < 0) {
		return ret;
	}

	out->header.base_timestamp_ns = edata->timestamp;
	out->header.reading_count = 1;
	out->shift = shift;
	out->readings[0].timestamp_delta = 0;

	/* q31 = value / 2^shift * 2^31, saturated to the q31 range */
	q = (micro * (1LL << (31 - shift))) / OPT4048_MICRO;

	out->readings[0].value = (q31_t)CLAMP(q, INT32_MIN, INT32_MAX);

	*fit = 1;

	return 1;
}

/* Decoder API used by the sensor subsystem to unpack encoded read buffers */
SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = opt4048_decoder_get_frame_count,
	.get_size_info = opt4048_decoder_get_size_info,
	.decode = opt4048_decoder_decode,
};

static int opt4048_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);

	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}

#endif /* CONFIG_SENSOR_ASYNC_API */

static int opt4048_attr_set_locked(const struct device *dev, enum sensor_attribute attr,
				   const struct sensor_value *val)
{
	int32_t v = val->val1;
	uint16_t reg;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
		/* Thresholds are given as linear ADC codes of the threshold channel */
		if (v < 0) {
			return -EINVAL;
		}
		ret = opt4048_encode_threshold((uint32_t)v, &reg);
		if (ret < 0) {
			return ret;
		}
		return opt4048_reg_write(dev,
					 (int)attr == SENSOR_ATTR_LOWER_THRESH
						 ? OPT4048_REG_THRESHOLD_LOW
						 : OPT4048_REG_THRESHOLD_HIGH,
					 reg);

	case SENSOR_ATTR_OPT4048_RANGE:
		/* Fixed ranges 2K..144K lux, or auto-range */
		if (!((v >= OPT4048_RANGE_2K_LUX && v <= OPT4048_RANGE_144K_LUX) ||
		      v == OPT4048_RANGE_AUTO)) {
			return -EINVAL;
		}
		return opt4048_cfg_update(dev, OPT4048_CFG_RANGE_MASK,
					  FIELD_PREP(OPT4048_CFG_RANGE_MASK, v));

	case SENSOR_ATTR_OPT4048_CONVERSION_TIME:
		/* Conversion time per channel, 600 us to 800 ms */
		if (v < OPT4048_CONVERSION_TIME_600US || v > OPT4048_CONVERSION_TIME_800MS) {
			return -EINVAL;
		}
		return opt4048_cfg_update(dev, OPT4048_CFG_CONVTIME_MASK,
					  FIELD_PREP(OPT4048_CFG_CONVTIME_MASK, v));

	case SENSOR_ATTR_OPT4048_MODE:
		/* Power-down, forced auto-range one-shot, one-shot or continuous */
		if (v < OPT4048_MODE_POWERDOWN || v > OPT4048_MODE_CONTINUOUS) {
			return -EINVAL;
		}
		return opt4048_cfg_update(dev, OPT4048_CFG_MODE_MASK,
					  FIELD_PREP(OPT4048_CFG_MODE_MASK, v));

	case SENSOR_ATTR_OPT4048_FAULT_COUNT:
		/* Number of consecutive faults needed to trigger the threshold interrupt */
		if (v < OPT4048_FAULT_COUNT_1 || v > OPT4048_FAULT_COUNT_8) {
			return -EINVAL;
		}
		return opt4048_cfg_update(dev, OPT4048_CFG_FAULT_MASK,
					  FIELD_PREP(OPT4048_CFG_FAULT_MASK, v));

	case SENSOR_ATTR_OPT4048_INT_LATCH:
		/* Any non-zero value enables the interrupt latch */
		return opt4048_cfg_update(dev, OPT4048_CFG_LATCH_MASK,
					  v ? OPT4048_CFG_LATCH_MASK : 0);

	case SENSOR_ATTR_OPT4048_INT_POLARITY:
		/* Any non-zero value selects active-high INT */
		return opt4048_cfg_update(dev, OPT4048_CFG_INTPOL_MASK,
					  v ? OPT4048_CFG_INTPOL_MASK : 0);

	case SENSOR_ATTR_OPT4048_QUICK_WAKE:
		/* Any non-zero value enables quick wake-up from standby */
		return opt4048_cfg_update(dev, OPT4048_CFG_QWAKE_MASK,
					  v ? OPT4048_CFG_QWAKE_MASK : 0);

	case SENSOR_ATTR_OPT4048_INT_DIRECTION:
		/* Any non-zero value makes INT an output */
		return opt4048_cfg2_update(dev, OPT4048_CFG2_INTDIR_MASK,
					   v ? OPT4048_CFG2_INTDIR_MASK : 0);

	case SENSOR_ATTR_OPT4048_INT_CONFIG:
		/* SMBus alert, or data-ready after the next or after all channels */
		if (v != OPT4048_INT_CFG_SMBUS_ALERT && v != OPT4048_INT_CFG_DATA_READY_NEXT &&
		    v != OPT4048_INT_CFG_DATA_READY_ALL) {
			return -EINVAL;
		}
		return opt4048_cfg2_update(dev, OPT4048_CFG2_INTCFG_MASK,
					   FIELD_PREP(OPT4048_CFG2_INTCFG_MASK, v));

	case SENSOR_ATTR_OPT4048_THRESHOLD_CHANNEL:
		/* Channel (CH0..CH3) compared against the thresholds */
		if (v < 0 || v >= OPT4048_NUM_CHANNELS) {
			return -EINVAL;
		}
		return opt4048_cfg2_update(dev, OPT4048_CFG2_THCH_MASK,
					   FIELD_PREP(OPT4048_CFG2_THCH_MASK, v));

	default:
		return -ENOTSUP;
	}
}

static int opt4048_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	struct opt4048_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);
	ret = opt4048_attr_set_locked(dev, attr, val);
	k_sem_give(&data->lock);

	return ret;
}

static int opt4048_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	struct opt4048_data *data = dev->data;
	uint16_t reg;
	int ret = 0;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	val->val2 = 0;

	k_sem_take(&data->lock, K_FOREVER);

	switch ((int)attr) {
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
		/* Thresholds are read back from the device and decoded to ADC codes */
		ret = opt4048_reg_read(dev,
				       (int)attr == SENSOR_ATTR_LOWER_THRESH
					       ? OPT4048_REG_THRESHOLD_LOW
					       : OPT4048_REG_THRESHOLD_HIGH,
				       &reg);
		if (ret == 0) {
			val->val1 = (int32_t)opt4048_decode_threshold(reg);
		}
		break;
	/* The attributes below are returned from the cached registers */
	case SENSOR_ATTR_OPT4048_RANGE:
		val->val1 = FIELD_GET(OPT4048_CFG_RANGE_MASK, data->cfg);
		break;
	case SENSOR_ATTR_OPT4048_CONVERSION_TIME:
		val->val1 = FIELD_GET(OPT4048_CFG_CONVTIME_MASK, data->cfg);
		break;
	case SENSOR_ATTR_OPT4048_MODE:
		val->val1 = FIELD_GET(OPT4048_CFG_MODE_MASK, data->cfg);
		break;
	case SENSOR_ATTR_OPT4048_FAULT_COUNT:
		val->val1 = FIELD_GET(OPT4048_CFG_FAULT_MASK, data->cfg);
		break;
	case SENSOR_ATTR_OPT4048_INT_LATCH:
		val->val1 = FIELD_GET(OPT4048_CFG_LATCH_MASK, data->cfg);
		break;
	case SENSOR_ATTR_OPT4048_INT_POLARITY:
		val->val1 = FIELD_GET(OPT4048_CFG_INTPOL_MASK, data->cfg);
		break;
	case SENSOR_ATTR_OPT4048_QUICK_WAKE:
		val->val1 = FIELD_GET(OPT4048_CFG_QWAKE_MASK, data->cfg);
		break;
	case SENSOR_ATTR_OPT4048_INT_DIRECTION:
		val->val1 = FIELD_GET(OPT4048_CFG2_INTDIR_MASK, data->cfg2);
		break;
	case SENSOR_ATTR_OPT4048_INT_CONFIG:
		val->val1 = FIELD_GET(OPT4048_CFG2_INTCFG_MASK, data->cfg2);
		break;
	case SENSOR_ATTR_OPT4048_THRESHOLD_CHANNEL:
		val->val1 = FIELD_GET(OPT4048_CFG2_THCH_MASK, data->cfg2);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_sem_give(&data->lock);

	return ret;
}

int opt4048_get_flags(const struct device *dev, uint8_t *flags)
{
	struct opt4048_data *data = dev->data;
	uint16_t reg;
	int ret;

	if (flags == NULL) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	ret = opt4048_reg_read(dev, OPT4048_REG_STATUS, &reg);
	k_sem_give(&data->lock);

	if (ret == 0) {
		*flags = reg & OPT4048_STATUS_FLAGS_MASK;
	}

	return ret;
}

/* Sensor driver API implemented by this driver */
static DEVICE_API(sensor, opt4048_driver_api) = {
	.attr_set = opt4048_attr_set,
	.attr_get = opt4048_attr_get,
	.sample_fetch = opt4048_sample_fetch,
	.channel_get = opt4048_channel_get,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = opt4048_submit,
	.get_decoder = opt4048_get_decoder,
#endif
};

static int opt4048_init(const struct device *dev)
{
	const struct opt4048_config *cfg = dev->config;
	struct opt4048_data *data = dev->data;
	uint16_t id;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

	/* Verify the device ID before touching any other register */
	ret = opt4048_reg_read(dev, OPT4048_REG_DEVICE_ID, &id);
	if (ret < 0) {
		LOG_ERR("Failed to read ID register (%d)", ret);
		return ret;
	}

	if ((id & OPT4048_DEVICE_ID_MASK) != OPT4048_DEVICE_ID) {
		LOG_ERR("Unexpected device ID 0x%04x", id);
		return -ENODEV;
	}

	/* THRESHOLD_CFG: reset defaults plus burst reads enabled (see OPT4048_CFG2_DEFAULT) */
	data->cfg2 = OPT4048_CFG2_DEFAULT;
	ret = opt4048_reg_write(dev, OPT4048_REG_THRESHOLD_CFG, data->cfg2);
	if (ret < 0) {
		return ret;
	}

	/* CONFIG: devicetree settings, interrupts latched and active-high */
	data->cfg = FIELD_PREP(OPT4048_CFG_RANGE_MASK, cfg->range) |
		    FIELD_PREP(OPT4048_CFG_CONVTIME_MASK, cfg->conv_time) |
		    FIELD_PREP(OPT4048_CFG_MODE_MASK, cfg->mode) | OPT4048_CFG_LATCH_MASK |
		    OPT4048_CFG_INTPOL_MASK;
	ret = opt4048_reg_write(dev, OPT4048_REG_CONFIG, data->cfg);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("OPT4048 initialized (ID=0x%04x)", id);

	return 0;
}

#define OPT4048_INIT(inst)                                                                         \
	static struct opt4048_data opt4048_data_##inst;                                            \
	static const struct opt4048_config opt4048_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.range = DT_INST_PROP(inst, range),                                                \
		.conv_time = DT_INST_PROP(inst, conversion_time),                                  \
		.mode = DT_INST_PROP(inst, operating_mode),                                        \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, opt4048_init, NULL, &opt4048_data_##inst,               \
				     &opt4048_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &opt4048_driver_api);

DT_INST_FOREACH_STATUS_OKAY(OPT4048_INIT)
