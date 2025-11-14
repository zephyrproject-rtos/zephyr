/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/libsbc/sbc.h>

#if defined(CONFIG_LIBSBC_ENCODER)

int sbc_setup_encoder(struct sbc_encoder *encoder, struct sbc_encoder_init_param *param)
{
	SBC_ENC_PARAMS *encoder_params;

	if (encoder == NULL) {
		return -EINVAL;
	}

	memset(encoder, 0, sizeof(struct sbc_encoder));

	encoder_params = &encoder->sbc_encoder_params;

	encoder_params->s16ChannelMode = (int16_t)param->ch_mode;
	encoder_params->s16NumOfSubBands = (int16_t)param->subband;
	if (!encoder_params->s16NumOfSubBands) {
		return -EINVAL;
	}
	encoder_params->s16NumOfBlocks = (int16_t)param->blk_len;
	if (!encoder_params->s16NumOfBlocks) {
		return -EINVAL;
	}
	encoder_params->s16AllocationMethod = (int16_t)param->alloc_mthd;
	encoder_params->s16NumOfChannels = param->ch_num;
	if (!encoder_params->s16NumOfChannels) {
		return -EINVAL;
	}

	switch (param->samp_freq) {
	case 16000u:
		encoder_params->s16SamplingFreq = 0;
		break;
	case 32000u:
		encoder_params->s16SamplingFreq = 1;
		break;
	case 44100u:
		encoder_params->s16SamplingFreq = 2;
		break;
	case 48000u:
		encoder_params->s16SamplingFreq = 3;
		break;
	default:
		return -EINVAL;
	}

	encoder_params->u16BitRate = param->bit_rate;

	SBC_Encoder_Init(encoder_params);

	if (encoder_params->s16BitPool < param->min_bitpool) {
		/* need to increase the `param->bit_rate` */
		return -EINVAL;
	} else if (encoder_params->s16BitPool > param->max_bitpool) {
		/* need to decrease the `param->bit_rate` */
		return -EOVERFLOW;
	}

	return 0;
}

/**
 * Encode a SBC frame
 */
uint32_t sbc_encode(struct sbc_encoder *encoder, const void *in_data, void *out_data)
{
	uint32_t ret;

	if ((encoder == NULL) || (in_data == NULL) || (out_data == NULL)) {
		return 0;
	}

	ret = SBC_Encode(&encoder->sbc_encoder_params, (int16_t *)in_data, out_data);

	return ret;
}

int sbc_frame_samples(struct sbc_encoder *encoder)
{
	if (encoder == NULL) {
		return -EINVAL;
	}

	return encoder->sbc_encoder_params.s16NumOfSubBands *
		encoder->sbc_encoder_params.s16NumOfBlocks;
}

int sbc_frame_bytes(struct sbc_encoder *encoder)
{
	if (encoder == NULL) {
		return -EINVAL;
	}

	return sbc_frame_samples(encoder) * 2 *
		(encoder->sbc_encoder_params.s16ChannelMode == SBC_CH_MODE_MONO ? 1 : 2);
}

int sbc_frame_encoded_bytes(struct sbc_encoder *encoder)
{
	int size = 4;
	int channel_num = 2;
	SBC_ENC_PARAMS *encoder_params;

	if (encoder == NULL) {
		return -EINVAL;
	}

	encoder_params = &encoder->sbc_encoder_params;

	if (encoder_params->s16ChannelMode == SBC_CH_MODE_MONO) {
		channel_num = 1;
	}

	size += (4 * encoder_params->s16NumOfSubBands * channel_num) / 8;
	if ((encoder_params->s16ChannelMode == SBC_CH_MODE_MONO) ||
	    (encoder_params->s16ChannelMode == SBC_CH_MODE_DUAL_CHANNEL)) {
		size += ((encoder_params->s16NumOfBlocks * channel_num *
			encoder_params->s16BitPool + 7) / 8);
	} else if (encoder_params->s16ChannelMode == SBC_CH_MODE_STEREO) {
		size += ((encoder_params->s16NumOfBlocks *
			encoder_params->s16BitPool + 7) / 8);
	} else {
		size += ((encoder_params->s16NumOfSubBands +
			encoder_params->s16NumOfBlocks *
			encoder_params->s16BitPool + 7) / 8);
	}

	return size;
}
#endif

#if defined(CONFIG_LIBSBC_DECODER)
/**
 * Setup decoder
 */
int sbc_setup_decoder(struct sbc_decoder *decoder)
{
	OI_STATUS status;

	if (decoder == NULL) {
		return -EINVAL;
	}

	memset(decoder, 0, sizeof(struct sbc_decoder));

	status = OI_CODEC_SBC_DecoderReset(
			&decoder->context,
			&decoder->context_data[0],
			sizeof(decoder->context_data),
			2, 2, FALSE);
	if (!OI_SUCCESS(status)) {
		return -EIO;
	}

	return 0;
}

/**
 * Decode a frame
 */
int sbc_decode(struct sbc_decoder *decoder, const void **in_data, uint32_t *in_size,
		void *out_data, uint32_t *out_size)
{
	OI_STATUS status;

	if (decoder == NULL || in_data == NULL || in_size == NULL ||
	    out_data == NULL || out_size == NULL) {
		return -EINVAL;
	}

	status = OI_CODEC_SBC_DecodeFrame(&decoder->context,
					  (const OI_BYTE**)in_data,
					  in_size,
					  out_data,
					  out_size);
	if (!OI_SUCCESS(status)) {
		return -EIO;
	} else {
		return 0;
	}
}
#endif
