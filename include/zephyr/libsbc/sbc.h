/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#if defined(CONFIG_LIBSBC_ENCODER)
#include "sbc_encoder.h"
#endif
#if defined(CONFIG_LIBSBC_DECODER)
#include "oi_codec_sbc.h"
#include "oi_status.h"
#endif

enum sbc_ch_mode {
	SBC_CH_MODE_MONO,
	SBC_CH_MODE_DUAL_CHANNEL,
	SBC_CH_MODE_STEREO,
	SBC_CH_MODE_JOINT_STEREO,
};

enum sbc_alloc_mthd {
	SBC_ALLOC_MTHD_LOUDNESS,
	SBC_ALLOC_MTHD_SNR,
};

#if defined(CONFIG_LIBSBC_ENCODER)
struct sbc_encoder {
	SBC_ENC_PARAMS sbc_encoder_params;
};
#endif

#if defined(CONFIG_LIBSBC_DECODER)
struct sbc_decoder {
	OI_CODEC_SBC_DECODER_CONTEXT context;
	uint32_t context_data[CODEC_DATA_WORDS(2, SBC_CODEC_FAST_FILTER_BUFFERS)];
};
#endif

#if defined(CONFIG_LIBSBC_ENCODER)
/**
 * Setup encoder
 * encoder     Handle of the encoder.
 * samp_freq   sampling frequency in Hz, 16000, 32000, 44100 or 48000.
 * ch_mode     channel mode, enum sbc_ch_mode.
 * blk_len     block length, 4, 8, 12 or 16.
 * subband     number of subbands, 4 or 8.
 * alloc_mthd  allocation method, enum sbc_alloc_mthd.
 * bit_rate    the resulting bit rate, kb/s
 * return	  0: On success  -1: Wrong parameters
 */
int sbc_setup_encoder(struct sbc_encoder *encoder, uint32_t samp_freq, enum sbc_ch_mode ch_mode,
		uint8_t blk_len, uint8_t subband, enum sbc_alloc_mthd alloc_mthd,
		uint32_t bit_rate);

/**
 * Encode a frame
 * encoder     Handle of the encoder
 * pcm         Input PCM samples
 * nbytes      Target size, in bytes, of the frame
 * out         Output buffer of `nbytes` size
 * return      0: On success  -1: Wrong parameters
 */
int sbc_encode(struct sbc_encoder *encoder, const void *pcm, uint32_t nbytes, void *out);

/**
 * Return the number of PCM samples in a frame
 * encoder     Handle of the encoder.
 * return      Number of PCM samples, -1 on bad parameters
 */
int sbc_frame_samples(struct sbc_encoder *encoder);

/**
 * Return the number of PCM bytes in a frame
 * encoder     Handle of the encoder.
 * return      Number of PCM bytes, -1 on bad parameters
 */
int sbc_frame_bytes(struct sbc_encoder *encoder);

/**
 * Return the encoded size of one frame
 * encoder     Handle of the encoder.
 * return      The encoded size in bytes of one frame, -1 on bad parameters
 */
int sbc_frame_encoded_bytes(struct sbc_encoder *encoder);
#endif

#if defined(CONFIG_LIBSBC_DECODER)
/**
 * Setup decoder
 * decoder     Handle of the decoder
 *
 * return	  0: On success  -1: Wrong parameters
 */
int sbc_setup_decoder(struct sbc_decoder *decoder);

/**
 * Decode a frame
 * decoder         Handle of the decoder
 * in              Input bitstream
 * in_nbytes       Input data size in bytes, it is decreased after decode one frame
 * pcm             Output PCM samples
 * out_nbytes      Output data size in bytes
 * return          0: On success  -1: Wrong parameters
 */
int sbc_decode(struct sbc_decoder *decoder, const void *in, uint32_t *in_nbytes,
		void *pcm, uint32_t *out_nbytes);
#endif
