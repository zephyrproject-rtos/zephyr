/*
 * Copyright (c) 2025 sevenlab engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/byteorder.h>

#include "sen6x.h"
#include "sen6x_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SEN6X_DECODER, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT sensirion_sen6x

static int sen6x_get_channel_index_particle_number(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_PM_0_5_NUM:
		return MAX_MEASURED_VALUES_COUNT + 0;
	case SENSOR_CHAN_PM_1_0_NUM:
		return MAX_MEASURED_VALUES_COUNT + 1;
	case SENSOR_CHAN_PM_2_5_NUM:
		return MAX_MEASURED_VALUES_COUNT + 2;
	case SENSOR_CHAN_PM_4_0_NUM:
		return MAX_MEASURED_VALUES_COUNT + 3;
	case SENSOR_CHAN_PM_10_NUM:
		return MAX_MEASURED_VALUES_COUNT + 4;
	default:
		return -ENOTSUP;
	}
}

static int sen6x_get_channel_index_sen60(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_PM_1_0:
		return 0;
	case SENSOR_CHAN_PM_2_5:
		return 1;
	case SENSOR_CHAN_PM_4_0:
		return 2;
	case SENSOR_CHAN_PM_10:
		return 3;
	case SENSOR_CHAN_PM_0_5_NUM:
		return 4;
	case SENSOR_CHAN_PM_1_0_NUM:
		return 5;
	case SENSOR_CHAN_PM_2_5_NUM:
		return 6;
	case SENSOR_CHAN_PM_4_0_NUM:
		return 7;
	case SENSOR_CHAN_PM_10_NUM:
		return 8;
	default:
		return -ENOTSUP;
	}
}

static int sen6x_get_channel_index_sen63c(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_PM_1_0:
		return 0;
	case SENSOR_CHAN_PM_2_5:
		return 1;
	case SENSOR_CHAN_PM_4_0:
		return 2;
	case SENSOR_CHAN_PM_10:
		return 3;
	case SENSOR_CHAN_AMBIENT_HUMIDITY:
		return 4;
	case SENSOR_CHAN_AMBIENT_TEMP:
		return 5;
	case SENSOR_CHAN_CO2:
		return 6;
	default:
		return sen6x_get_channel_index_particle_number(chan);
	}
}

static int sen6x_get_channel_index_sen65(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_PM_1_0:
		return 0;
	case SENSOR_CHAN_PM_2_5:
		return 1;
	case SENSOR_CHAN_PM_4_0:
		return 2;
	case SENSOR_CHAN_PM_10:
		return 3;
	case SENSOR_CHAN_AMBIENT_HUMIDITY:
		return 4;
	case SENSOR_CHAN_AMBIENT_TEMP:
		return 5;
	case SENSOR_CHAN_VOC_INDEX:
		return 6;
	case SENSOR_CHAN_NOX_INDEX:
		return 7;
	default:
		return sen6x_get_channel_index_particle_number(chan);
	}
}

static int sen6x_get_channel_index_sen66(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_PM_1_0:
		return 0;
	case SENSOR_CHAN_PM_2_5:
		return 1;
	case SENSOR_CHAN_PM_4_0:
		return 2;
	case SENSOR_CHAN_PM_10:
		return 3;
	case SENSOR_CHAN_AMBIENT_HUMIDITY:
		return 4;
	case SENSOR_CHAN_AMBIENT_TEMP:
		return 5;
	case SENSOR_CHAN_VOC_INDEX:
		return 6;
	case SENSOR_CHAN_NOX_INDEX:
		return 7;
	case SENSOR_CHAN_CO2:
		return 8;
	default:
		return sen6x_get_channel_index_particle_number(chan);
	}
}

static int sen6x_get_channel_index_sen68(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_PM_1_0:
		return 0;
	case SENSOR_CHAN_PM_2_5:
		return 1;
	case SENSOR_CHAN_PM_4_0:
		return 2;
	case SENSOR_CHAN_PM_10:
		return 3;
	case SENSOR_CHAN_AMBIENT_HUMIDITY:
		return 4;
	case SENSOR_CHAN_AMBIENT_TEMP:
		return 5;
	case SENSOR_CHAN_VOC_INDEX:
		return 6;
	case SENSOR_CHAN_NOX_INDEX:
		return 7;
	case SENSOR_CHAN_HCHO:
		return 8;
	default:
		return sen6x_get_channel_index_particle_number(chan);
	}
}

static int sen6x_get_channel_index(const struct sen6x_encoded_data *edata, enum sensor_channel chan)
{
	switch (edata->header.model) {
	case SEN6X_MODEL_SEN60:
		return sen6x_get_channel_index_sen60(chan);
	case SEN6X_MODEL_SEN63C:
		return sen6x_get_channel_index_sen63c(chan);
	case SEN6X_MODEL_SEN65:
		return sen6x_get_channel_index_sen65(chan);
	case SEN6X_MODEL_SEN66:
		return sen6x_get_channel_index_sen66(chan);
	case SEN6X_MODEL_SEN68:
		return sen6x_get_channel_index_sen68(chan);
	default:
		return -ENOTSUP;
	}
}

static int sen6x_get_channel_format(enum sensor_channel chan, bool *out_is_signed,
				    size_t *out_divisor)
{
	bool is_signed;
	size_t divisor;

	switch (chan) {
	case SENSOR_CHAN_PM_1_0:
	case SENSOR_CHAN_PM_2_5:
	case SENSOR_CHAN_PM_4_0:
	case SENSOR_CHAN_PM_10:
	case SENSOR_CHAN_PM_0_5_NUM:
	case SENSOR_CHAN_PM_1_0_NUM:
	case SENSOR_CHAN_PM_2_5_NUM:
	case SENSOR_CHAN_PM_4_0_NUM:
	case SENSOR_CHAN_PM_10_NUM:
	case SENSOR_CHAN_HCHO:
		is_signed = false;
		divisor = 10;
		break;
	case SENSOR_CHAN_AMBIENT_HUMIDITY:
		is_signed = true;
		divisor = 100;
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		is_signed = true;
		divisor = 200;
		break;
	case SENSOR_CHAN_VOC_INDEX:
	case SENSOR_CHAN_NOX_INDEX:
		is_signed = true;
		divisor = 10;
		break;
	case SENSOR_CHAN_CO2:
		is_signed = false;
		divisor = 1;
		break;
	default:
		return -ENOTSUP;
	}

	if (out_is_signed) {
		*out_is_signed = is_signed;
	}
	if (out_divisor) {
		*out_divisor = divisor;
	}
	return 0;
}

int sen6x_encode(const struct device *dev, const struct sensor_chan_spec *const channels,
		 const size_t num_channels, uint8_t *buf)
{
	const struct sen6x_config *cfg = dev->config;
	struct sen6x_encoded_data *edata = (struct sen6x_encoded_data *)buf;
	uint64_t cycles;
	int ret;
	bool needs_measured_values = false;
	bool needs_separate_number_concentration = false;

	if (cfg->model != SEN6X_MODEL_SEN60) {
		for (size_t i = 0; i < num_channels; i++) {
			if (sen6x_get_channel_index(edata, channels[i].chan_type) >=
			    MAX_MEASURED_VALUES_COUNT) {
				needs_separate_number_concentration = true;
			} else {
				needs_measured_values = true;
			}
		}
	}

	ret = sensor_clock_get_cycles(&cycles);
	if (ret != 0) {
		return ret;
	}

	edata->header = (struct sen6x_encoded_header){
		.model = cfg->model,
		.timestamp = sensor_clock_cycles_to_ns(cycles),
		.has_data_ready = needs_measured_values || needs_separate_number_concentration,
		.has_measured_values = needs_measured_values,
		.has_number_concentration = needs_separate_number_concentration,
	};

	return 0;
}

static int sen6x_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					 uint16_t *frame_count)
{
	const struct sen6x_encoded_data *edata = (const struct sen6x_encoded_data *)buffer;
	int ret;
	const uint8_t *channel_data;
	uint16_t value;
	bool is_signed;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	ret = sen6x_get_channel_index(edata, chan_spec.chan_type);
	if (ret < 0) {
		return ret;
	}
	if (ret < MAX_MEASURED_VALUES_COUNT && !edata->header.has_measured_values) {
		return -ENODATA;
	}
	if (ret >= MAX_MEASURED_VALUES_COUNT && !edata->header.has_number_concentration) {
		return -ENODATA;
	}

	ret = sen6x_get_channel_format(chan_spec.chan_type, &is_signed, NULL);
	if (ret != 0) {
		return ret;
	}

	channel_data = &edata->channels[ret * 3];

	value = sys_get_be16(channel_data);

	if (is_signed) {
		if (value == INT16_MAX) {
			return -ENODATA;
		}
	} else {
		if (value == UINT16_MAX) {
			return -ENODATA;
		}
	}

	*frame_count = 1;
	return 0;
}

static int sen6x_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
				       size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_PM_1_0:
	case SENSOR_CHAN_PM_2_5:
	case SENSOR_CHAN_PM_4_0:
	case SENSOR_CHAN_PM_10:
	case SENSOR_CHAN_PM_0_5_NUM:
	case SENSOR_CHAN_PM_1_0_NUM:
	case SENSOR_CHAN_PM_2_5_NUM:
	case SENSOR_CHAN_PM_4_0_NUM:
	case SENSOR_CHAN_PM_10_NUM:
	case SENSOR_CHAN_AMBIENT_HUMIDITY:
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_VOC_INDEX:
	case SENSOR_CHAN_NOX_INDEX:
	case SENSOR_CHAN_CO2:
	case SENSOR_CHAN_HCHO:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int sen6x_one_shot_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				 uint32_t *fit, uint16_t max_count, void *data_out_)
{
	const struct sen6x_encoded_data *edata = (const struct sen6x_encoded_data *)buffer;
	struct sensor_q31_data *data_out = data_out_;
	int ret;
	const uint8_t *channel_data;
	uint16_t value;
	int16_t value_signed;
	size_t divisor;
	bool is_signed;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	ret = sen6x_get_channel_index(edata, chan_spec.chan_type);
	if (ret < 0) {
		return ret;
	}
	if (ret < MAX_MEASURED_VALUES_COUNT && !edata->header.has_measured_values) {
		return -ENODATA;
	}
	if (ret >= MAX_MEASURED_VALUES_COUNT && !edata->header.has_number_concentration) {
		return -ENODATA;
	}

	channel_data = &edata->channels[ret * 3];

	ret = sen6x_get_channel_format(chan_spec.chan_type, &is_signed, &divisor);
	if (ret != 0) {
		return ret;
	}

	if (!sen6x_u16_array_checksum_ok(channel_data, 3)) {
		LOG_WRN("CRC check failed on channel data.");
		return -EIO;
	}

	value = sys_get_be16(channel_data);
	value_signed = (int16_t)value;

	if (is_signed) {
		if (value_signed == INT16_MAX) {
			return -ENODATA;
		}
		data_out->shift = LOG2CEIL((INT16_MAX / divisor) + 1);
		data_out->readings[0].value =
			(((int64_t)value_signed) << (31 - data_out->shift)) / divisor;
	} else {
		if (value == UINT16_MAX) {
			return -ENODATA;
		}
		data_out->shift = LOG2CEIL((UINT16_MAX / divisor) + 1);
		data_out->readings[0].value =
			(((int64_t)value) << (31 - data_out->shift)) / divisor;
	}

	data_out->header.base_timestamp_ns = edata->header.timestamp;
	data_out->header.reading_count = 1;

	*fit = 1;
	return 1;
}

static int sen6x_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				uint32_t *fit, uint16_t max_count, void *data_out)
{
	return sen6x_one_shot_decode(buffer, chan_spec, fit, max_count, data_out);
}

static bool sen6x_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct sen6x_encoded_data *edata = (const struct sen6x_encoded_data *)buffer;

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:
		if (!edata->header.has_data_ready) {
			return false;
		}
		if (!sen6x_u16_array_checksum_ok(edata->data_ready, 3)) {
			LOG_WRN("CRC check failed on data-ready data.");
			return false;
		}

		if (edata->header.model == SEN6X_MODEL_SEN60) {
			return (edata->data_ready[1] & GENMASK(0, 11)) != 0;
		} else {
			return edata->data_ready[1] == 1;
		}
	default:
		break;
	}

	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = sen6x_decoder_get_frame_count,
	.get_size_info = sen6x_decoder_get_size_info,
	.decode = sen6x_decoder_decode,
	.has_trigger = sen6x_decoder_has_trigger,
};

int sen6x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
