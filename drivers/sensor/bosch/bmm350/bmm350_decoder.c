/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor_clock.h>

#include "bmm350.h"
#include "bmm350_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(BMM350_DECODER, CONFIG_SENSOR_LOG_LEVEL);

void bmm350_decoder_compensate_raw_data(const struct bmm350_raw_mag_data *raw_data,
					const struct mag_compensate *comp,
					struct bmm350_mag_temp_data *out)
{
	int32_t out_data[4];
	int32_t dut_offset_coef[3], dut_sensit_coef[3], dut_tco[3], dut_tcs[3];

	/* Convert mag lsb to uT and temp lsb to degC */
	out_data[0] = ((sign_extend(raw_data->magn_x, BMM350_SIGNED_24_BIT) *
			BMM350_LSB_TO_UT_XY_COEFF) /
		       BMM350_LSB_TO_UT_COEFF_DIV);
	out_data[1] = ((sign_extend(raw_data->magn_y, BMM350_SIGNED_24_BIT) *
			BMM350_LSB_TO_UT_XY_COEFF) /
		       BMM350_LSB_TO_UT_COEFF_DIV);
	out_data[2] = ((sign_extend(raw_data->magn_z, BMM350_SIGNED_24_BIT) *
			BMM350_LSB_TO_UT_Z_COEFF) /
		       BMM350_LSB_TO_UT_COEFF_DIV);
	out_data[3] = ((sign_extend(raw_data->temp, BMM350_SIGNED_24_BIT) *
			BMM350_LSB_TO_UT_TEMP_COEFF) /
		       BMM350_LSB_TO_UT_COEFF_DIV);

	if (out_data[3] > 0) {
		out_data[3] = (out_data[3] - (2549 / 100));
	} else if (out_data[3] < 0) {
		out_data[3] = (out_data[3] + (2549 / 100));
	}

	/* Apply compensation to temperature reading */
	out_data[3] = (((BMM350_MAG_COMP_COEFF_SCALING + comp->dut_sensit_coef.t_sens) *
			out_data[3]) +
		       comp->dut_offset_coef.t_offs) /
		      BMM350_MAG_COMP_COEFF_SCALING;

	/* Store magnetic compensation structure to an array */
	dut_offset_coef[0] = comp->dut_offset_coef.offset_x;
	dut_offset_coef[1] = comp->dut_offset_coef.offset_y;
	dut_offset_coef[2] = comp->dut_offset_coef.offset_z;

	dut_sensit_coef[0] = comp->dut_sensit_coef.sens_x;
	dut_sensit_coef[1] = comp->dut_sensit_coef.sens_y;
	dut_sensit_coef[2] = comp->dut_sensit_coef.sens_z;

	dut_tco[0] = comp->dut_tco.tco_x;
	dut_tco[1] = comp->dut_tco.tco_y;
	dut_tco[2] = comp->dut_tco.tco_z;

	dut_tcs[0] = comp->dut_tcs.tcs_x;
	dut_tcs[1] = comp->dut_tcs.tcs_y;
	dut_tcs[2] = comp->dut_tcs.tcs_z;

	/* Compensate raw magnetic data */
	for (size_t indx = 0; indx < 3; indx++) {
		out_data[indx] = (out_data[indx] *
				  (BMM350_MAG_COMP_COEFF_SCALING + dut_sensit_coef[indx])) /
				 BMM350_MAG_COMP_COEFF_SCALING;
		out_data[indx] = (out_data[indx] + dut_offset_coef[indx]);
		out_data[indx] = ((out_data[indx] * BMM350_MAG_COMP_COEFF_SCALING) +
				  (dut_tco[indx] * (out_data[3] - comp->dut_t0))) /
				 BMM350_MAG_COMP_COEFF_SCALING;
		out_data[indx] = (out_data[indx] * BMM350_MAG_COMP_COEFF_SCALING) /
				 (BMM350_MAG_COMP_COEFF_SCALING +
				  (dut_tcs[indx] * (out_data[3] - comp->dut_t0)));
	}

	out->mag[0] = ((((out_data[0] * BMM350_MAG_COMP_COEFF_SCALING) -
		    (comp->cross_axis.cross_x_y * out_data[1])) *
		   BMM350_MAG_COMP_COEFF_SCALING) /
		  ((BMM350_MAG_COMP_COEFF_SCALING * BMM350_MAG_COMP_COEFF_SCALING) -
		   (comp->cross_axis.cross_y_x * comp->cross_axis.cross_x_y)));

	out->mag[1] = ((((out_data[1] * BMM350_MAG_COMP_COEFF_SCALING) -
		    (comp->cross_axis.cross_y_x * out_data[0])) *
		   BMM350_MAG_COMP_COEFF_SCALING) /
		  ((BMM350_MAG_COMP_COEFF_SCALING * BMM350_MAG_COMP_COEFF_SCALING) -
		   (comp->cross_axis.cross_y_x * comp->cross_axis.cross_x_y)));

	out->mag[2] = (out_data[2] +
		  (((out_data[0] *
		     ((comp->cross_axis.cross_y_x * comp->cross_axis.cross_z_y) -
		      (comp->cross_axis.cross_z_x * BMM350_MAG_COMP_COEFF_SCALING))) -
		    (out_data[1] *
		     ((comp->cross_axis.cross_z_y * BMM350_MAG_COMP_COEFF_SCALING) -
		      (comp->cross_axis.cross_x_y * comp->cross_axis.cross_z_x))))) /
		  (((BMM350_MAG_COMP_COEFF_SCALING * BMM350_MAG_COMP_COEFF_SCALING) -
		    comp->cross_axis.cross_y_x * comp->cross_axis.cross_x_y)));

	LOG_DBG("mag data %d %d %d", out_data[0], out_data[1], out_data[2]);

	out->temperature = out_data[3];
}

#ifdef CONFIG_SENSOR_ASYNC_API

static uint8_t bmm350_encode_channel(enum sensor_channel chan)
{
	uint8_t encode_bmask = 0;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		encode_bmask |= BIT(0);
		break;
	case SENSOR_CHAN_MAGN_Y:
		encode_bmask |= BIT(1);
		break;
	case SENSOR_CHAN_MAGN_Z:
		encode_bmask |= BIT(2);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		encode_bmask |= BIT(0);
		encode_bmask |= BIT(1);
		encode_bmask |= BIT(2);
		break;
	default:
		break;
	}

	return encode_bmask;
}

int bmm350_encode(const struct device *dev,
		  const struct sensor_read_config *read_config,
		  bool is_trigger,
		  uint8_t *buf)
{
	struct bmm350_encoded_data *edata = (struct bmm350_encoded_data *)buf;
	struct bmm350_data *data = dev->data;
	uint64_t cycles;
	int err;

	edata->header.channels = 0;

	if (is_trigger) {
		edata->header.channels |= bmm350_encode_channel(SENSOR_CHAN_MAGN_XYZ);
	} else {
		const struct sensor_chan_spec *const channels = read_config->channels;
		size_t num_channels = read_config->count;

		for (size_t i = 0 ; i < num_channels ; i++) {
			edata->header.channels |= bmm350_encode_channel(channels[i].chan_type);
		}
	}

	err = sensor_clock_get_cycles(&cycles);
	if (err != 0) {
		return err;
	}

	edata->header.events = is_trigger ? BIT(0) : 0;
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	memcpy(&edata->comp, &data->mag_comp, sizeof(edata->comp));

	return 0;
}

static int bmm350_decoder_get_frame_count(const uint8_t *buffer,
					  struct sensor_chan_spec chan_spec,
					  uint16_t *frame_count)
{
	const struct bmm350_encoded_data *edata = (const struct bmm350_encoded_data *)buffer;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	uint8_t channel_request = bmm350_encode_channel(chan_spec.chan_type);

	/** Filter unknown channels and having no data. */
	if ((edata->header.channels & channel_request) != channel_request) {
		return -ENODATA;
	}

	*frame_count = 1;

	return 0;
}

static int bmm350_decoder_get_size_info(struct sensor_chan_spec chan_spec,
					size_t *base_size,
					size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int bmm350_decoder_decode(const uint8_t *buffer,
				 struct sensor_chan_spec chan_spec,
				 uint32_t *fit,
				 uint16_t max_count,
				 void *data_out)
{
	const struct bmm350_encoded_data *edata = (const struct bmm350_encoded_data *)buffer;
	uint8_t channel_request;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z: {

		channel_request = bmm350_encode_channel(chan_spec.chan_type);
		if ((channel_request & edata->header.channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		struct bmm350_mag_temp_data result;

		bmm350_decoder_compensate_raw_data(&edata->payload,
						   &edata->comp,
						   &result);

		/** Data compensation returns data in uT (x100 Gauss),
		 * so we're reserving 8 fractional bits.
		 */
		out->shift = (31 - 8);
		out->readings[0].value =
			(result.mag[chan_spec.chan_type - SENSOR_CHAN_MAGN_X] << 8) / 100;

		*fit = 1;
		return 1;
	}
	case SENSOR_CHAN_MAGN_XYZ: {

		channel_request = bmm350_encode_channel(chan_spec.chan_type);
		if ((channel_request & edata->header.channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		struct bmm350_mag_temp_data result;

		bmm350_decoder_compensate_raw_data(&edata->payload,
						   &edata->comp,
						   &result);

		/** Data compensation returns data in uT (x100 Gauss),
		 * so we're reserving 8 fractional bits.
		 */
		out->shift = (31 - 8);
		out->readings[0].values[0] = (result.mag[0] << 8) / 100;
		out->readings[0].values[1] = (result.mag[1] << 8) / 100;
		out->readings[0].values[2] = (result.mag[2] << 8) / 100;

		*fit = 1;
		return 1;
	}
	default:
		return -EINVAL;
	}
}

static bool bmm350_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct bmm350_encoded_data *edata = (const struct bmm350_encoded_data *)buffer;

	if ((trigger == SENSOR_TRIG_DATA_READY) && (edata->header.events != 0)) {
		return true;
	}

	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = bmm350_decoder_get_frame_count,
	.get_size_info = bmm350_decoder_get_size_info,
	.decode = bmm350_decoder_decode,
	.has_trigger = bmm350_decoder_has_trigger,
};

int bmm350_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}

#endif /* CONFIG_SENSOR_ASYNC_API */
