/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/libsbc/sbc.h>

#if defined(CONFIG_LIBSBC_ENCODER)
int sbc_setup_encoder(struct sbc_encoder *encoder, uint32_t samp_freq, enum sbc_ch_mode ch_mode,
		uint8_t blk_len, uint8_t subband, enum sbc_alloc_mthd alloc_mthd,
		uint32_t bit_rate)
{
	if (encoder == NULL) {
		return -1;
	}

	memset(encoder, 0, sizeof(struct sbc_encoder));
	encoder->sbc_encoder_params.s16ChannelMode = (SINT16)ch_mode;
	encoder->sbc_encoder_params.s16NumOfSubBands = (SINT16)subband;
	encoder->sbc_encoder_params.s16NumOfBlocks = (SINT16)blk_len;
	encoder->sbc_encoder_params.s16AllocationMethod = (SINT16)alloc_mthd;
	switch (samp_freq) {
	case 16000u:
		encoder->sbc_encoder_params.s16SamplingFreq = 0;
		break;
	case 32000u:
		encoder->sbc_encoder_params.s16SamplingFreq = 1;
		break;
	case 44100u:
		encoder->sbc_encoder_params.s16SamplingFreq = 2;
		break;
	case 48000u:
		encoder->sbc_encoder_params.s16SamplingFreq = 3;
		break;
	default:
		return -1;
	}
	encoder->sbc_encoder_params.u16BitRate = bit_rate;
	SBC_Encoder_Init(&encoder->sbc_encoder_params);

	return 0;
}

/**
 * Encode a SBC frame
 */
int sbc_encode(struct sbc_encoder *encoder, const void *pcm, uint32_t nbytes, void *out)
{
	if ((encoder == NULL) || (pcm == NULL) || (out == NULL)) {
		return -1;
	}

	encoder->sbc_encoder_params.ps16PcmBuffer = (int16_t *)pcm;
	encoder->sbc_encoder_params.pu8Packet = out;
	SBC_Encoder(&encoder->sbc_encoder_params);
	if (encoder->sbc_encoder_params.u16PacketLength != nbytes) {
		return -1;
	}
	return 0;
}

int sbc_frame_samples(struct sbc_encoder *encoder)
{
	if (encoder == NULL) {
		return -1;
	}

	return encoder->sbc_encoder_params.s16NumOfSubBands *
		encoder->sbc_encoder_params.s16NumOfBlocks;
}

int sbc_frame_bytes(struct sbc_encoder *encoder)
{
	if (encoder == NULL) {
		return -1;
	}

	return sbc_frame_samples(encoder) * 2 *
		(encoder->sbc_encoder_params.s16ChannelMode == SBC_CH_MODE_MONO ? 1 : 2);
}

int sbc_frame_encoded_bytes(struct sbc_encoder *encoder)
{
	int size = 4;
	int channel_num = 2;

	if (encoder == NULL) {
		return -1;
	}

	if (encoder->sbc_encoder_params.s16ChannelMode == SBC_CH_MODE_MONO) {
		channel_num = 1;
	}

	size += (4 * encoder->sbc_encoder_params.s16NumOfSubBands * channel_num) / 8;
	if ((encoder->sbc_encoder_params.s16ChannelMode == SBC_CH_MODE_MONO) ||
	    (encoder->sbc_encoder_params.s16ChannelMode == SBC_CH_MODE_DUAL_CHANNEL)) {
		size += ((encoder->sbc_encoder_params.s16NumOfBlocks * channel_num *
			encoder->sbc_encoder_params.s16BitPool + 7) / 8);
	} else if (encoder->sbc_encoder_params.s16ChannelMode == SBC_CH_MODE_STEREO) {
		size += ((encoder->sbc_encoder_params.s16NumOfBlocks *
			encoder->sbc_encoder_params.s16BitPool + 7) / 8);
	} else {
		size += ((encoder->sbc_encoder_params.s16NumOfSubBands +
			encoder->sbc_encoder_params.s16NumOfBlocks *
			encoder->sbc_encoder_params.s16BitPool + 7) / 8);
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
		return -1;
	}

	memset(decoder, 0, sizeof(struct sbc_decoder));

	status = OI_CODEC_SBC_DecoderReset(
			&decoder->context,
			(OI_UINT32 *)&decoder->context_data[0],
			sizeof(decoder->context_data),
			2, 2, FALSE);
	if (!OI_SUCCESS(status)) {
		return -1;
	}
	return 0;
}

/**
 * Decode a frame
 */
int sbc_decode(struct sbc_decoder *decoder, const void *in, uint32_t *in_nbytes,
		void *pcm, uint32_t *out_nbytes)
{
	OI_STATUS status;

	if (decoder == NULL) {
		return -1;
	}

	status = OI_CODEC_SBC_DecodeFrame(&decoder->context,
					(const OI_BYTE**)&in,
					(OI_UINT32 *)in_nbytes,
					pcm,
					(OI_UINT32 *)out_nbytes
					);
	if (!OI_SUCCESS(status)) {
		return -1;
	} else {
		return 0;
	}
}
#endif
