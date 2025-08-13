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

enum __packed sbc_ch_mode {
	SBC_CH_MODE_MONO,
	SBC_CH_MODE_DUAL_CHANNEL,
	SBC_CH_MODE_STEREO,
	SBC_CH_MODE_JOINT_STEREO,
};

enum __packed sbc_alloc_mthd {
	SBC_ALLOC_MTHD_LOUDNESS,
	SBC_ALLOC_MTHD_SNR,
};

#if defined(CONFIG_LIBSBC_ENCODER)

struct sbc_encoder {
	SBC_ENC_PARAMS sbc_encoder_params;
};

struct sbc_encoder_init_param {
	uint32_t bit_rate;
	uint32_t samp_freq;
	uint8_t blk_len;
	uint8_t subband;
	enum sbc_alloc_mthd alloc_mthd;
	enum sbc_ch_mode ch_mode;
	uint8_t ch_num;
	uint8_t min_bitpool;
	uint8_t max_bitpool;
};

/**
 * Setup encoder
 * param     The init parameters.
 * return    Zero on success or (negative) error code otherwise.
 */
int sbc_setup_encoder(struct sbc_encoder *encoder, struct sbc_encoder_init_param *param);

/**
 * Encode a frame
 * encoder     Handle of the encoder
 * in_data     Input PCM samples
 * nbytes      Target size, in bytes, of the frame
 * out_data    Output buffer of `nbytes` size
 * return      Return number of bytes output
 */
uint32_t sbc_encode(struct sbc_encoder *encoder, const void *in_data, void *out_data);

/**
 * Return the number of PCM samples in a frame
 * encoder     Handle of the encoder.
 * return      Number of PCM samples or (negative) error code otherwise
 */
int sbc_frame_samples(struct sbc_encoder *encoder);

/**
 * Return the number of PCM bytes in a frame
 * encoder     Handle of the encoder.
 * return      Number of PCM bytes or (negative) error code otherwise
 */
int sbc_frame_bytes(struct sbc_encoder *encoder);

/**
 * Return the encoded size of one frame
 * encoder     Handle of the encoder.
 * return      The encoded size of one frame in bytes or (negative) error code otherwise
 */
int sbc_frame_encoded_bytes(struct sbc_encoder *encoder);
#endif

#if defined(CONFIG_LIBSBC_DECODER)

struct sbc_decoder {
	OI_CODEC_SBC_DECODER_CONTEXT context;
	uint32_t context_data[CODEC_DATA_WORDS(2, SBC_CODEC_FAST_FILTER_BUFFERS)];
};

/**
 * Setup the SBC decoder.
 * decoder   Handle of the decoder
 *
 * return    Zero on success or (negative) error code otherwise.
 */
int sbc_setup_decoder(struct sbc_decoder *decoder);

/**
 * Decode a frame
 * decoder    Handle of the decoder
 * in_data    Input bitstream, it is increased after decode one frame
 * in_size    Input data size in bytes, it is decreased after decode one frame
 * out_data   Output PCM samples
 * out_size   Output data size in bytes
 * return     Zero on success or (negative) error code otherwise.
 */
int sbc_decode(struct sbc_decoder *decoder, const void **in_data, uint32_t *in_size,
		void *out_data, uint32_t *out_size);
#endif
