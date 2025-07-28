/*
 * Copyright (c) 2025 Nordic Semiconductors ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys/byteorder.h>

#include "bme680.h"
#include "bme680_decoder.h"

static void bme680_calc_temp(struct bme680_compensated_data *data,
							 const struct bme680_comp_param *comp_param,
							 int32_t *t_fine,
							 uint32_t adc_temp)
{
	int64_t var1, var2, var3;

	var1 = ((int32_t)adc_temp >> 3) - ((int32_t)comp_param->par_t1 << 1);
	var2 = (var1 * (int32_t)comp_param->par_t2) >> 11;
	var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
	var3 = ((var3) * ((int32_t)comp_param->par_t3 << 4)) >> 14;
	*t_fine = var2 + var3;
	data->temp = ((*t_fine * 5) + 128) >> 8;
}

static void bme680_calc_press(struct bme680_compensated_data *data,
							const struct bme680_comp_param *comp_param,
							int32_t *t_fine,
							uint32_t adc_press)
{
	int32_t var1, var2, var3, calc_press;

	var1 = (((int32_t)*t_fine) >> 1) - 64000;
	var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) *
		(int32_t)comp_param->par_p6) >> 2;
	var2 = var2 + ((var1 * (int32_t)comp_param->par_p5) << 1);
	var2 = (var2 >> 2) + ((int32_t)comp_param->par_p4 << 16);
	var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) *
		 ((int32_t)comp_param->par_p3 << 5)) >> 3)
		   + (((int32_t)comp_param->par_p2 * var1) >> 1);
	var1 = var1 >> 18;
	var1 = ((32768 + var1) * (int32_t)comp_param->par_p1) >> 15;
	calc_press = 1048576 - adc_press;
	calc_press = (calc_press - (var2 >> 12)) * ((uint32_t)3125);
	/* This max value is used to provide precedence to multiplication or
	 * division in the pressure calculation equation to achieve least
	 * loss of precision and avoiding overflows.
	 * i.e Comparing value, signed int 32bit (1 << 30)
	 */
	if (calc_press >= (int32_t)0x40000000) {
		calc_press = ((calc_press / var1) << 1);
	} else {
		calc_press = ((calc_press << 1) / var1);
	}
	var1 = ((int32_t)comp_param->par_p9 *
		(int32_t)(((calc_press >> 3)
			 * (calc_press >> 3)) >> 13)) >> 12;
	var2 = ((int32_t)(calc_press >> 2) * (int32_t)comp_param->par_p8) >> 13;
	var3 = ((int32_t)(calc_press >> 8) * (int32_t)(calc_press >> 8)
		* (int32_t)(calc_press >> 8)
		* (int32_t)comp_param->par_p10) >> 17;

	data->press = calc_press
			   + ((var1 + var2 + var3
				   + ((int32_t)comp_param->par_p7 << 7)) >> 4);
}

static void bme680_calc_humidity(struct bme680_compensated_data *data,
							const struct bme680_comp_param *comp_param,
							int32_t *t_fine,
							uint16_t adc_humidity)
{
	int32_t var1, var2_1, var2_2, var2, var3, var4, var5, var6;
	int32_t temp_scaled, calc_hum;

	temp_scaled = (((int32_t)*t_fine * 5) + 128) >> 8;
	var1 = (int32_t)(adc_humidity - ((int32_t)((int32_t)comp_param->par_h1 * 16))) -
		   (((temp_scaled * (int32_t)comp_param->par_h3)
		 / ((int32_t)100)) >> 1);
	var2_1 = (int32_t)comp_param->par_h2;
	var2_2 = ((temp_scaled * (int32_t)comp_param->par_h4) / (int32_t)100)
		 + (((temp_scaled * ((temp_scaled * (int32_t)comp_param->par_h5)
					 / ((int32_t)100))) >> 6) / ((int32_t)100))
		 +  (int32_t)(1 << 14);
	var2 = (var2_1 * var2_2) >> 10;
	var3 = var1 * var2;
	var4 = (int32_t)comp_param->par_h6 << 7;
	var4 = ((var4) + ((temp_scaled * (int32_t)comp_param->par_h7) /
			  ((int32_t)100))) >> 4;
	var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
	var6 = (var4 * var5) >> 1;
	calc_hum = (((var3 + var6) >> 10) * ((int32_t)1000)) >> 12;

	if (calc_hum > 100000) { /* Cap at 100%rH */
		calc_hum = 100000;
	}

	if (calc_hum < 0) {
		calc_hum = 0;
	}

	data->humidity = calc_hum;
}

static void bme680_calc_gas_resistance(struct bme680_compensated_data *data,
							const struct bme680_comp_param *comp_param,
							uint8_t gas_range,
							uint16_t adc_gas_res)
{
	int64_t var1, var3;
	uint64_t var2;

	static const uint32_t look_up1[16] = { 2147483647, 2147483647, 2147483647,
				   2147483647, 2147483647, 2126008810, 2147483647,
				   2130303777, 2147483647, 2147483647, 2143188679,
				   2136746228, 2147483647, 2126008810, 2147483647,
				   2147483647 };

	static const uint32_t look_up2[16] = { 4096000000, 2048000000, 1024000000,
				   512000000, 255744255, 127110228, 64000000,
				   32258064, 16016016, 8000000, 4000000, 2000000,
				   1000000, 500000, 250000, 125000 };

	var1 = (int64_t)((1340 + (5 * (int64_t)comp_param->range_sw_err)) *
			   ((int64_t)look_up1[gas_range])) >> 16;
	var2 = (((int64_t)((int64_t)adc_gas_res << 15) - (int64_t)(16777216)) + var1);
	var3 = (((int64_t)look_up2[gas_range] * (int64_t)var1) >> 9);
	data->gas_resistance = (uint32_t)((var3 + ((int64_t)var2 >> 1))
						/ (int64_t)var2);
}

void bme680_compensate_raw_data(enum sensor_channel chan,
							const struct bme680_raw_data *raw_data,
							const struct bme680_comp_param *comp_param,
							struct bme680_compensated_data *data)
{
	uint8_t gas_range;
	uint32_t adc_temp, adc_press;
	uint16_t adc_hum, adc_gas_res;
	/* Carryover between temperature and pressure/humidity compensation. */
	int32_t t_fine;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		adc_temp = sys_get_be24(&raw_data->buf[3]) >> 4;
		bme680_calc_temp(data, comp_param, &t_fine, adc_temp);
		break;
	case SENSOR_CHAN_PRESS:
		/* We need to calculate always before t_fine (temperature compensation) */
		adc_temp = sys_get_be24(&raw_data->buf[3]) >> 4;
		bme680_calc_temp(data, comp_param, &t_fine, adc_temp);
		adc_press = sys_get_be24(&raw_data->buf[0]) >> 4;
		bme680_calc_press(data, comp_param, &t_fine, adc_press);
		break;
	case SENSOR_CHAN_HUMIDITY:
		/* We need to calculate always before t_fine (temperature compensation) */
		adc_temp = sys_get_be24(&raw_data->buf[3]) >> 4;
		bme680_calc_temp(data, comp_param, &t_fine, adc_temp);
		adc_hum = sys_get_be16(&raw_data->buf[6]);
		bme680_calc_humidity(data, comp_param, &t_fine, adc_hum);
		break;
	case SENSOR_CHAN_GAS_RES:
		gas_range = raw_data->buf[13] & BME680_MSK_GAS_RANGE;
		adc_gas_res = sys_get_be16(&raw_data->buf[12]) >> 6;
		bme680_calc_gas_resistance(data, comp_param, gas_range, adc_gas_res);
		break;
	case SENSOR_CHAN_ALL:
		adc_press = sys_get_be24(&raw_data->buf[0]) >> 4;
		adc_temp = sys_get_be24(&raw_data->buf[3]) >> 4;
		adc_hum = sys_get_be16(&raw_data->buf[6]);
		adc_gas_res = sys_get_be16(&raw_data->buf[12]) >> 6;
		gas_range = raw_data->buf[13] & BME680_MSK_GAS_RANGE;
		bme680_calc_temp(data, comp_param, &t_fine, adc_temp);
		bme680_calc_press(data, comp_param, &t_fine, adc_press);
		bme680_calc_humidity(data, comp_param, &t_fine, adc_hum);
		bme680_calc_gas_resistance(data, comp_param, gas_range, adc_gas_res);
		break;
	default:
		break;
	}
}

static uint8_t bme680_encode_channel(enum sensor_channel chan)
{
	uint8_t encode_bmask = 0;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		encode_bmask |= BIT(0);
		break;
	case SENSOR_CHAN_PRESS:
		encode_bmask |= BIT(1);
		break;
	case SENSOR_CHAN_HUMIDITY:
		encode_bmask |= BIT(2);
		break;
	case SENSOR_CHAN_GAS_RES:
		encode_bmask |= BIT(3);
		break;
	case SENSOR_CHAN_ALL:
		encode_bmask |= BIT(0) | BIT(1) | BIT(2) | BIT(3);
		break;
	default:
		break;
	}

	return encode_bmask;
}

int bme680_encode(const struct device *dev,
		  const struct sensor_read_config *read_config,
		  uint8_t *buf)
{
	struct bme680_encoded_data *edata = (struct bme680_encoded_data *)buf;
	struct bme680_data *data = dev->data;
	uint64_t cycles;
	int err;

	edata->header.channels = 0;

	const struct sensor_chan_spec *const channels = read_config->channels;
	size_t num_channels = read_config->count;

	for (size_t i = 0 ; i < num_channels ; i++) {
		edata->header.channels |= bme680_encode_channel(channels[i].chan_type);
	}

	err = sensor_clock_get_cycles(&cycles);
	if (err != 0) {
		return err;
	}

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	memcpy(&edata->comp_param, &data->comp_param, sizeof(edata->comp_param));

	return 0;
}

static int bme680_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					  uint16_t *frame_count)
{
	const struct bme680_encoded_data *edata = (const struct bme680_encoded_data *)buffer;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	uint8_t channel_request = bme680_encode_channel(chan_spec.chan_type);

	/** Filter unknown channels and having no data. */
	if ((edata->header.channels & channel_request) != channel_request) {
		return -ENODATA;
	}

	*frame_count = 1;

	return 0;
}

static int bme680_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_HUMIDITY:
	case SENSOR_CHAN_PRESS:
	case SENSOR_CHAN_GAS_RES:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int bme680_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				 uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct bme680_encoded_data *edata = (const struct bme680_encoded_data *)buffer;
	uint8_t channel_request;
	int32_t readq, convq;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP: {
		channel_request = bme680_encode_channel(chan_spec.chan_type);
		if ((channel_request & edata->header.channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		struct bme680_compensated_data result;

		bme680_compensate_raw_data(chan_spec.chan_type,
								   &edata->payload,
								   &edata->comp_param,
								   &result);

		readq = result.temp * (1 << (31 - BME680_TEMP_SHIFT));
		convq = BME680_TEMP_CONV * (1 << (31 - BME680_TEMP_SHIFT));

		out->readings[0].temperature =
			(int32_t)((((int64_t)readq) << (31 - BME680_TEMP_SHIFT)) /
					((int64_t)convq));
		out->shift = BME680_TEMP_SHIFT;
		break;
	}
	case SENSOR_CHAN_PRESS: {
		channel_request = bme680_encode_channel(chan_spec.chan_type);
		if ((channel_request & edata->header.channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		struct bme680_compensated_data result;

		bme680_compensate_raw_data(chan_spec.chan_type,
								   &edata->payload,
								   &edata->comp_param,
								   &result);

		readq = result.press * (1 << (31 - BME680_PRESS_SHIFT));
		convq = BME680_PRESS_CONV_KPA * (1 << (31 - BME680_PRESS_SHIFT));

		out->readings[0].pressure =
			(int32_t)((((int64_t)readq) << (31 - BME680_PRESS_SHIFT)) /
				  ((int64_t)convq));
		out->shift = BME680_PRESS_SHIFT;
		break;
	}
	case SENSOR_CHAN_HUMIDITY: {
		channel_request = bme680_encode_channel(chan_spec.chan_type);
		if ((channel_request & edata->header.channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		struct bme680_compensated_data result;

		bme680_compensate_raw_data(chan_spec.chan_type,
								   &edata->payload,
								   &edata->comp_param,
								   &result);

		out->readings[0].humidity = result.humidity;
		out->shift = BME680_HUM_SHIFT;
		break;
	}
	case SENSOR_CHAN_GAS_RES: {
		channel_request = bme680_encode_channel(chan_spec.chan_type);
		if ((channel_request & edata->header.channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		struct bme680_compensated_data result;

		bme680_compensate_raw_data(chan_spec.chan_type,
								   &edata->payload,
								   &edata->comp_param,
								   &result);

		out->readings[0].resistance = result.gas_resistance;
		out->shift = BME680_GAS_RES_SHIFT;
		break;
	}
	default:
		return -EINVAL;
	}

	*fit = 1;

	return 1;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = bme680_decoder_get_frame_count,
	.get_size_info = bme680_decoder_get_size_info,
	.decode = bme680_decoder_decode,
};

int bme680_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
