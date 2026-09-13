/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ad5940.h"

#include <math.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ad5940.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ad5940, CONFIG_SENSOR_LOG_LEVEL);

static inline int32_t ad5940_sign_extend(uint32_t val, uint8_t bits)
{
	int32_t sval = (int32_t)val;

	if ((sval & BIT(bits - 1u)) != 0) {
		sval |= (int32_t)(~((BIT(bits)) - 1u));
	}

	return sval;
}

static int ad5940_decoder_get_frame_count(const uint8_t *buffer,
					  struct sensor_chan_spec chan_spec,
					  uint16_t *frame_count)
{
	const struct ad5940_fifo_hdr *hdr = (const struct ad5940_fifo_hdr *)buffer;

	if (hdr->fifo_byte_count == 0u || hdr->sample_set_size == 0u) {
		*frame_count = 0u;
		return 0;
	}

	switch ((int)chan_spec.chan_type) {
	case SENSOR_CHAN_AD5940_IMPEDANCE_MAGNITUDE:
	case SENSOR_CHAN_AD5940_IMPEDANCE_PHASE:
	case SENSOR_CHAN_AD5940_IMPEDANCE_REAL:
	case SENSOR_CHAN_AD5940_IMPEDANCE_IMAG:
	case SENSOR_CHAN_AD5940_DFT:
		*frame_count = (uint16_t)(hdr->fifo_byte_count / hdr->sample_set_size);
		return 0;

	case SENSOR_CHAN_AD5940_CURRENT:
	case SENSOR_CHAN_AD5940_ADC_RAW:
	case SENSOR_CHAN_AD5940_POTENTIAL:
	case SENSOR_CHAN_AD5940_SINC2:
		*frame_count = (uint16_t)(hdr->fifo_byte_count / AD5940_FIFO_WORD_BYTES);
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int ad5940_decoder_get_size_info(struct sensor_chan_spec chan_spec,
					size_t *base_size,
					size_t *frame_size)
{
	*base_size = sizeof(struct sensor_q31_data);

	switch ((int)chan_spec.chan_type) {
	case SENSOR_CHAN_AD5940_IMPEDANCE_MAGNITUDE:
	case SENSOR_CHAN_AD5940_IMPEDANCE_PHASE:
	case SENSOR_CHAN_AD5940_IMPEDANCE_REAL:
	case SENSOR_CHAN_AD5940_IMPEDANCE_IMAG:
	case SENSOR_CHAN_AD5940_DFT:
		*frame_size = sizeof(struct sensor_q31_sample_data) *
			      AD5940_EIS_WORDS_PER_FRAME;
		break;

	case SENSOR_CHAN_AD5940_CURRENT:
	case SENSOR_CHAN_AD5940_ADC_RAW:
	case SENSOR_CHAN_AD5940_POTENTIAL:
	case SENSOR_CHAN_AD5940_SINC2:
		*frame_size = sizeof(struct sensor_q31_sample_data);
		break;

	default:
		*frame_size = sizeof(struct sensor_q31_sample_data);
		break;
	}

	return 0;
}

static bool ad5940_decoder_has_trigger(const uint8_t *buffer,
				       enum sensor_trigger_type trigger)
{
	const struct ad5940_fifo_hdr *hdr = (const struct ad5940_fifo_hdr *)buffer;

	switch (trigger) {
	case SENSOR_TRIG_FIFO_WATERMARK:
		return (hdr->int_status & AFEINTSRC_DATAFIFOTHRESH) != 0u;

	default:
		return false;
	}
}

static int ad5940_decode_eis(const uint8_t *buffer,
			     struct sensor_chan_spec chan_spec,
			     uint32_t *fit,
			     uint16_t max_count,
			     void *data_out)
{
	const struct ad5940_fifo_hdr *hdr = (const struct ad5940_fifo_hdr *)buffer;
	const uint8_t *raw = buffer + sizeof(struct ad5940_fifo_hdr);
	struct sensor_q31_data *out = data_out;
	const uint32_t *words;
	uint32_t frame_offset;
	uint32_t raw_word;
	uint32_t chid_top;
	uint32_t w0, w1, w2, w3;
	int32_t v_real, v_imag, i_real, i_imag;
	int32_t z_real_q31, z_imag_q31, z_mag_q31, z_phase_q31;
	int64_t denom;
	int64_t zr_num, zi_num;
	float ratio_re, ratio_im;
	float mag_ohms, phase_f;
	float rtia_mag, rtia_phase;
	uint16_t total_frames;
	uint16_t frames_to_decode;
	uint32_t ns_per_frame;
	bool frame_valid;
	uint16_t i;
	uint8_t w;

	if (hdr->sample_set_size == 0u) {
		return -EINVAL;
	}

	total_frames = (uint16_t)(hdr->fifo_byte_count / hdr->sample_set_size);

	if (*fit >= total_frames) {
		return 0;
	}

	frames_to_decode = MIN((uint16_t)(total_frames - *fit), max_count);
	ns_per_frame = (hdr->odr > 0u) ? (AD5940_NSEC_PER_SEC / hdr->odr) : 0u;

	out->header.base_timestamp_ns = hdr->timestamp;
	out->header.reading_count     = frames_to_decode;
	out->shift = 0;

	for (i = 0u; i < frames_to_decode; i++) {
		frame_offset = (*fit + i) * hdr->sample_set_size;
		words = (const uint32_t *)(raw + frame_offset);
		frame_valid = true;

		for (w = 0u; w < AD5940_EIS_WORDS_PER_FRAME; w++) {
			raw_word = sys_le32_to_cpu(words[w]);
			chid_top = FIELD_GET(AD5940_DFT_CHID_TOP_MSK, raw_word);

			if (chid_top != AD5940_DFT_CHID_TOP_DFT) {
				frame_valid = false;
				break;
			}
		}

		w0 = sys_le32_to_cpu(words[0]);
		w1 = sys_le32_to_cpu(words[1]);
		w2 = sys_le32_to_cpu(words[2]);
		w3 = sys_le32_to_cpu(words[3]);

		v_real = frame_valid ?
		 ad5940_sign_extend(w0 & AD5940_DFTREAL_DATA_MSK,
				    AD5940_DFT_DATA_BITS) : 0;
	v_imag = frame_valid ?
		 -ad5940_sign_extend(w1 & AD5940_DFTREAL_DATA_MSK,
				     AD5940_DFT_DATA_BITS) : 0;
	i_real = frame_valid ?
		 -ad5940_sign_extend(w2 & AD5940_DFTREAL_DATA_MSK,
				     AD5940_DFT_DATA_BITS) : 0;
		i_imag = frame_valid ?
			 ad5940_sign_extend(w3 & AD5940_DFTREAL_DATA_MSK, AD5940_DFT_DATA_BITS) : 0;

		denom = (int64_t)i_real * i_real + (int64_t)i_imag * i_imag;

		if (denom == 0) {
			z_real_q31  = 0;
			z_imag_q31  = 0;
			z_mag_q31   = 0;
			z_phase_q31 = 0;
		} else {
			zr_num = ((int64_t)v_real * i_real +
				  (int64_t)v_imag * i_imag) << AD5940_DECODER_Q_SHIFT;
			zi_num = ((int64_t)v_imag * i_real -
				  (int64_t)v_real * i_imag) << AD5940_DECODER_Q_SHIFT;
			ratio_re = (float)(int32_t)(zr_num / denom) / AD5940_DECODER_Q_SCALE;
			ratio_im = (float)(int32_t)(zi_num / denom) / AD5940_DECODER_Q_SCALE;

			mag_ohms = sqrtf(ratio_re * ratio_re + ratio_im * ratio_im);
			phase_f  = atan2f(ratio_im, ratio_re);

			rtia_mag   = hdr->rtia_mag_ohms;
			rtia_phase = hdr->rtia_phase_rad;

			if (rtia_mag > 0.0f) {
				mag_ohms *= rtia_mag;
				phase_f  += rtia_phase;
			}

			while (phase_f >  AD5940_PI_F) {
				phase_f -= 2.0f * AD5940_PI_F;
			}
			while (phase_f <= -AD5940_PI_F) {
				phase_f += 2.0f * AD5940_PI_F;
			}

			z_real_q31 = (int32_t)(mag_ohms * cosf(phase_f));
			z_imag_q31 = (int32_t)(mag_ohms * sinf(phase_f));
			z_mag_q31  = (int32_t)mag_ohms;

			z_phase_q31 = (int32_t)(phase_f *
						((float)INT32_MAX / AD5940_PI_F));
		}

		out->readings[i].timestamp_delta = i * ns_per_frame;

		switch ((int)chan_spec.chan_type) {
		case SENSOR_CHAN_AD5940_IMPEDANCE_REAL:
			out->readings[i].value = z_real_q31;
			break;
		case SENSOR_CHAN_AD5940_IMPEDANCE_IMAG:
			out->readings[i].value = z_imag_q31;
			break;
		case SENSOR_CHAN_AD5940_IMPEDANCE_MAGNITUDE:
			out->readings[i].value = z_mag_q31;
			break;
		case SENSOR_CHAN_AD5940_IMPEDANCE_PHASE:
			out->readings[i].value = z_phase_q31;
			break;
		case SENSOR_CHAN_AD5940_DFT:
			/* Return raw DFT real */
			out->readings[i].value = v_real;
			break;
		default:
			out->readings[i].value = 0;
			break;
		}
	}

	*fit += frames_to_decode;

	return (int)frames_to_decode;
}

static int ad5940_decode_current(const uint8_t *buffer,
				 struct sensor_chan_spec chan_spec,
				 uint32_t *fit,
				 uint16_t max_count,
				 void *data_out)
{
	const struct ad5940_fifo_hdr *hdr = (const struct ad5940_fifo_hdr *)buffer;
	const uint8_t *raw = buffer + sizeof(struct ad5940_fifo_hdr);
	struct sensor_q31_data *out = data_out;
	uint32_t word_offset;
	uint32_t raw_word;
	uint32_t ns_per_frame;
	uint16_t total_words;
	uint16_t words_to_decode;
	uint16_t adc_code;
	uint16_t i;

	ARG_UNUSED(chan_spec);

	total_words = (uint16_t)(hdr->fifo_byte_count / AD5940_FIFO_WORD_BYTES);

	if (*fit >= total_words) {
		return 0;
	}

	words_to_decode = MIN((uint16_t)(total_words - *fit), max_count);
	ns_per_frame = (hdr->odr > 0u) ? (AD5940_NSEC_PER_SEC / hdr->odr) : 0u;

	out->header.base_timestamp_ns = hdr->timestamp;
	out->header.reading_count     = words_to_decode;
	out->shift = AD5940_CURRENT_Q31_SHIFT;

	for (i = 0u; i < words_to_decode; i++) {
		word_offset = (*fit + i) * AD5940_FIFO_WORD_BYTES;
		raw_word = ((uint32_t)raw[word_offset]     << 24u) |
			   ((uint32_t)raw[word_offset + 1u] << 16u) |
			   ((uint32_t)raw[word_offset + 2u] << 8u)  |
			   ((uint32_t)raw[word_offset + 3u]);

		adc_code = (uint16_t)(raw_word & AD5940_ADCDAT_DATA_MSK);

		out->readings[i].timestamp_delta = i * ns_per_frame;
		out->readings[i].value = (int32_t)adc_code << AD5940_CURRENT_CODE_SHIFT;
	}

	*fit += words_to_decode;

	return (int)words_to_decode;
}

static int ad5940_decode_adc_raw(const uint8_t *buffer,
				 struct sensor_chan_spec chan_spec,
				 uint32_t *fit,
				 uint16_t max_count,
				 void *data_out)
{
	const struct ad5940_fifo_hdr *hdr = (const struct ad5940_fifo_hdr *)buffer;
	const uint8_t *raw = buffer + sizeof(struct ad5940_fifo_hdr);
	struct sensor_q31_data *out = data_out;
	uint32_t word_offset;
	uint32_t raw_word;
	uint32_t ns_per_frame;
	uint16_t total_words;
	uint16_t words_to_decode;
	uint16_t adc_code;
	uint16_t i;

	ARG_UNUSED(chan_spec);

	total_words = (uint16_t)(hdr->fifo_byte_count / AD5940_FIFO_WORD_BYTES);

	if (*fit >= total_words) {
		return 0;
	}

	words_to_decode = MIN((uint16_t)(total_words - *fit), max_count);
	ns_per_frame = (hdr->odr > 0u) ? (AD5940_NSEC_PER_SEC / hdr->odr) : 0u;

	out->header.base_timestamp_ns = hdr->timestamp;
	out->header.reading_count     = words_to_decode;
	out->shift = 0;

	for (i = 0u; i < words_to_decode; i++) {
		word_offset = (*fit + i) * AD5940_FIFO_WORD_BYTES;
		raw_word = ((uint32_t)raw[word_offset]     << 24u) |
			   ((uint32_t)raw[word_offset + 1u] << 16u) |
			   ((uint32_t)raw[word_offset + 2u] << 8u)  |
			   ((uint32_t)raw[word_offset + 3u]);

		adc_code = (uint16_t)(raw_word & AD5940_ADCDAT_DATA_MSK);

		out->readings[i].timestamp_delta = i * ns_per_frame;
		out->readings[i].value = (int32_t)(uint32_t)adc_code;
	}

	*fit += words_to_decode;

	return (int)words_to_decode;
}


static int ad5940_decoder_decode(const uint8_t *buffer,
				 struct sensor_chan_spec chan_spec,
				 uint32_t *fit,
				 uint16_t max_count,
				 void *data_out)
{
	const struct ad5940_fifo_hdr *hdr = (const struct ad5940_fifo_hdr *)buffer;

	if (hdr->fifo_byte_count == 0u) {
		return 0;
	}

	switch ((int)chan_spec.chan_type) {
	case SENSOR_CHAN_AD5940_IMPEDANCE_REAL:
	case SENSOR_CHAN_AD5940_IMPEDANCE_IMAG:
	case SENSOR_CHAN_AD5940_IMPEDANCE_MAGNITUDE:
	case SENSOR_CHAN_AD5940_IMPEDANCE_PHASE:
	case SENSOR_CHAN_AD5940_DFT:
		return ad5940_decode_eis(buffer, chan_spec, fit, max_count, data_out);

	case SENSOR_CHAN_AD5940_CURRENT:
		return ad5940_decode_current(buffer, chan_spec, fit, max_count, data_out);

	case SENSOR_CHAN_AD5940_ADC_RAW:
	case SENSOR_CHAN_AD5940_POTENTIAL:
	case SENSOR_CHAN_AD5940_SINC2:

		return ad5940_decode_adc_raw(buffer, chan_spec, fit, max_count, data_out);

	default:
		return -ENOTSUP;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = ad5940_decoder_get_frame_count,
	.get_size_info   = ad5940_decoder_get_size_info,
	.decode          = ad5940_decoder_decode,
	.has_trigger     = ad5940_decoder_has_trigger,
};

/**
 * @brief Get the sensor decoder API for AD5940/AD5941
 *
 * @param dev Device pointer
 * @param decoder Decoder API pointer
 * @return int 0 on success, negative error code otherwise
 */
int ad5940_get_decoder(const struct device *dev,
		       const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
