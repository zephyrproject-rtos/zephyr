/** @file
 * @brief Advance Audio Distribution Profile - SBC Codec header.
 */
/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2021 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_A2DP_CODEC_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_A2DP_CODEC_H_

#if defined(CONFIG_BT_A2DP_SOURCE)
#include "sbc_encoder.h"
#endif
#if defined(CONFIG_BT_A2DP_SINK)
#include "oi_codec_sbc.h"
#include "oi_status.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Sampling Frequency */
#define A2DP_SBC_SAMP_FREQ_16000 BIT(7)
#define A2DP_SBC_SAMP_FREQ_32000 BIT(6)
#define A2DP_SBC_SAMP_FREQ_44100 BIT(5)
#define A2DP_SBC_SAMP_FREQ_48000 BIT(4)

/* Channel Mode */
#define A2DP_SBC_CH_MODE_MONO  BIT(3)
#define A2DP_SBC_CH_MODE_DUAL  BIT(2)
#define A2DP_SBC_CH_MODE_STREO BIT(1)
#define A2DP_SBC_CH_MODE_JOINT BIT(0)

/* Block Length */
#define A2DP_SBC_BLK_LEN_4  BIT(7)
#define A2DP_SBC_BLK_LEN_8  BIT(6)
#define A2DP_SBC_BLK_LEN_12 BIT(5)
#define A2DP_SBC_BLK_LEN_16 BIT(4)

/* Subbands */
#define A2DP_SBC_SUBBAND_4 BIT(3)
#define A2DP_SBC_SUBBAND_8 BIT(2)

/* Allocation Method */
#define A2DP_SBC_ALLOC_MTHD_SNR      BIT(1)
#define A2DP_SBC_ALLOC_MTHD_LOUDNESS BIT(0)

#define BT_A2DP_SBC_SAMP_FREQ(cap)    ((cap->config[0] >> 4) & 0x0f)
#define BT_A2DP_SBC_CHAN_MODE(cap)    ((cap->config[0]) & 0x0f)
#define BT_A2DP_SBC_BLK_LEN(cap)      ((cap->config[1] >> 4) & 0x0f)
#define BT_A2DP_SBC_SUB_BAND(cap)     ((cap->config[1] >> 2) & 0x03)
#define BT_A2DP_SBC_ALLOC_MTHD(cap)   ((cap->config[1]) & 0x03)

/** @brief SBC Codec */
struct bt_a2dp_codec_sbc_params {
	/** First two octets of configuration */
	uint8_t config[2];
	/** Minimum Bitpool Value */
	uint8_t min_bitpool;
	/** Maximum Bitpool Value */
	uint8_t max_bitpool;
} __packed;

#if defined(CONFIG_BT_A2DP_SOURCE)
struct bt_a2dp_codec_sbc_encoder {
	SBC_ENC_PARAMS sbc_encoder_params;
	struct bt_a2dp_codec_sbc_params *sbc;
};
#endif

#if defined(CONFIG_BT_A2DP_SINK)
struct bt_a2dp_codec_sbc_decoder {
	OI_CODEC_SBC_DECODER_CONTEXT context;
	uint32_t context_data[CODEC_DATA_WORDS(2, SBC_CODEC_FAST_FILTER_BUFFERS)];
	struct bt_a2dp_codec_sbc_params *sbc;
};
#endif

struct bt_a2dp_codec_sbc_media_packet_hdr {
	uint8_t number_of_sbc_frames:4;
	uint8_t rfa:1;
	uint8_t L:1;
	uint8_t S:1;
	uint8_t F:1;
} __packed;

uint8_t bt_a2dp_sbc_get_subband_num(struct bt_a2dp_codec_sbc_params *sbc_codec);
uint8_t bt_a2dp_sbc_get_block_length(struct bt_a2dp_codec_sbc_params *sbc_codec);
uint8_t bt_a2dp_sbc_get_channel_num(struct bt_a2dp_codec_sbc_params *sbc_codec);
uint32_t bt_a2dp_sbc_get_sampling_frequency(struct bt_a2dp_codec_sbc_params *sbc_codec);
#if defined(CONFIG_BT_A2DP_SOURCE)
int bt_a2dp_sbc_encoder_init(struct bt_a2dp_codec_sbc_encoder *sbc_encoder);
int bt_a2dp_sbc_encode(struct bt_a2dp_codec_sbc_encoder *sbc_encoder, uint8_t
			*input_frame, uint8_t *output_buffer, uint32_t *output_len);
#endif
#if defined(CONFIG_BT_A2DP_SINK)
int bt_a2dp_sbc_decoder_init(struct bt_a2dp_codec_sbc_decoder *sbc_decoder);
int bt_a2dp_sbc_decode(struct bt_a2dp_codec_sbc_decoder *sbc_decoder,
				uint8_t **sbc_data, uint32_t *sbc_data_len,
				uint16_t *pcm_data, uint32_t *pcm_data_len);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_A2DP_CODEC_H_ */
