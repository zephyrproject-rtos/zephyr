/*
 * Copyright 2024 - 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LIB_SBC_H_
#define ZEPHYR_INCLUDE_LIB_SBC_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sbc_encoder.h"
#include "oi_codec_sbc.h"
#include "oi_status.h"

/** @brief SBC channel mode */
enum __packed sbc_ch_mode {
	/** Mono channel */
	SBC_CH_MODE_MONO,
	/** Dual channel */
	SBC_CH_MODE_DUAL_CHANNEL,
	/** Stereo channel */
	SBC_CH_MODE_STEREO,
	/** Joint stereo channel */
	SBC_CH_MODE_JOINT_STEREO,
};

/** @brief SBC allocation method */
enum __packed sbc_alloc_mthd {
	/** Loudness allocation method */
	SBC_ALLOC_MTHD_LOUDNESS,
	/** SNR allocation method */
	SBC_ALLOC_MTHD_SNR,
};

/** @brief SBC encoder */
struct sbc_encoder {
	/** @cond INTERNAL_HIDDEN */
	/** Internally used field for encoder */
	SBC_ENC_PARAMS sbc_encoder_params;
	/** @endcond */
};

/** @brief Encoder initialization parameters */
struct sbc_encoder_init_param {
	/** Bit rate after encoded */
	uint32_t bit_rate;
	/** Sample frequency */
	uint32_t samp_freq;
	/** Block length */
	uint8_t blk_len;
	/** Number of subbands */
	uint8_t subband;
	/** Allocation method */
	enum sbc_alloc_mthd alloc_mthd;
	/** Channel mode */
	enum sbc_ch_mode ch_mode;
	/** Number of channels */
	uint8_t ch_num;
	/** Minimum bitpool */
	uint8_t min_bitpool;
	/** Maximum bitpool */
	uint8_t max_bitpool;
};

/** @brief Setup encoder.
 *
 * @param encoder  Handle of the encoder.
 * @param param The parameters to initialize encoder @ref sbc_encoder_init_param.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int sbc_setup_encoder(struct sbc_encoder *encoder, struct sbc_encoder_init_param *param);

/** @brief Encode a frame.
 *
 * @param encoder  Handle of the encoder.
 * @param in_data  Input PCM samples.
 * @param out_data Encoded SBC frame.
 *
 * @return The encoded size of the frame in bytes.
 */
uint32_t sbc_encode(struct sbc_encoder *encoder, const void *in_data, void *out_data);

/** @brief Return the number of PCM samples in a frame.
 *
 * @param encoder Handle of the encoder.
 *
 * @return Number of PCM samples or (negative) error code otherwise.
 */
int sbc_frame_samples(struct sbc_encoder *encoder);

/** @brief Return the number of PCM bytes in a frame.
 *
 * @param encoder Handle of the encoder.
 *
 * @return Number of PCM bytes or (negative) error code otherwise
 */
int sbc_frame_bytes(struct sbc_encoder *encoder);

/** @brief Return the encoded size of one frame.
 *
 *  @param encoder Handle of the encoder.
 *
 *  @return The encoded size of one frame in bytes or (negative) error code otherwise.
 */
int sbc_frame_encoded_bytes(struct sbc_encoder *encoder);

/** @brief SBC decoder */
struct sbc_decoder {
	/** @cond INTERNAL_HIDDEN */
	/** Internally used fields for decoder */
	OI_CODEC_SBC_DECODER_CONTEXT context;
	uint32_t context_data[CODEC_DATA_WORDS(2, SBC_CODEC_FAST_FILTER_BUFFERS)];
	/** @endcond */
};

/** @brief Setup the SBC decoder.
 *
 *  @param decoder Handle of the decoder.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int sbc_setup_decoder(struct sbc_decoder *decoder);

/** @brief Decode a frame.
 *
 *  @param decoder  Handle of the decoder.
 *  @param in_data  Input bitstream, it is increased after decode one frame
 *  @param in_size  Input data size in bytes, it is decreased after decode one frame
 *  @param out_data Output PCM samples
 *  @param out_size Output data size in bytes
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int sbc_decode(struct sbc_decoder *decoder, const void **in_data, uint32_t *in_size,
		void *out_data, uint32_t *out_size);
#endif /* ZEPHYR_INCLUDE_LIB_SBC_H_ */
