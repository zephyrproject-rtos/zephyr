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

/** If the F bit is set to 0, this field indicates the number of frames contained
 *  in this packet. If the F bit is set to 1, this field indicates the number
 *  of remaining fragments, including the current fragment.
 *  Therefore, the last counter value shall be one.
 */
#define BT_A2DP_SBC_MEDIA_HDR_NUM_FRAMES_GET(hdr) FIELD_GET(GENMASK(3, 0), (hdr))
/** Set to 1 for the last packet of a fragmented SBC frame, otherwise set to 0. */
#define BT_A2DP_SBC_MEDIA_HDR_L_GET(hdr)          FIELD_GET(BIT(5), (hdr))
/** Set to 1 for the starting packet of a fragmented SBC frame, otherwise set to 0. */
#define BT_A2DP_SBC_MEDIA_HDR_S_GET(hdr)          FIELD_GET(BIT(6), (hdr))
/** Set to 1 if the SBC frame is fragmented, otherwise set to 0. */
#define BT_A2DP_SBC_MEDIA_HDR_F_GET(hdr)          FIELD_GET(BIT(7), (hdr))

/** If the F bit is set to 0, this field indicates the number of frames contained
 *  in this packet. If the F bit is set to 1, this field indicates the number
 *  of remaining fragments, including the current fragment.
 *  Therefore, the last counter value shall be one.
 */
#define BT_A2DP_SBC_MEDIA_HDR_NUM_FRAMES_SET(hdr, val)\
	hdr = ((hdr) & ~GENMASK(3, 0)) | FIELD_PREP(GENMASK(3, 0), (val))
/** Set to 1 for the last packet of a fragmented SBC frame, otherwise set to 0. */
#define BT_A2DP_SBC_MEDIA_HDR_L_SET(hdr, val)\
	hdr = ((hdr) & ~BIT(5)) | FIELD_PREP(BIT(5), (val))
/** Set to 1 for the starting packet of a fragmented SBC frame, otherwise set to 0. */
#define BT_A2DP_SBC_MEDIA_HDR_S_SET(hdr, val)\
	hdr = ((hdr) & ~BIT(6)) | FIELD_PREP(BIT(6), (val))
/** Set to 1 if the SBC frame is fragmented, otherwise set to 0. */
#define BT_A2DP_SBC_MEDIA_HDR_F_SET(hdr, val)\
	hdr = ((hdr) & ~BIT(7)) | FIELD_PREP(BIT(7), (val))

#define BT_A2DP_SBC_MEDIA_HDR_ENCODE(num_frames, l, s, f)\
	FIELD_PREP(GENMASK(3, 0), num_frames) | FIELD_PREP(BIT(5), l) |\
	FIELD_PREP(BIT(6), s) | FIELD_PREP(BIT(7), f)

/** @brief get channel num of a2dp sbc config.
 *
 *  @param sbc_codec The a2dp sbc parameter.
 *
 *  @return the channel num.
 */
uint8_t bt_a2dp_sbc_get_channel_num(struct bt_a2dp_codec_sbc_params *sbc_codec);

/** @brief get sample rate of a2dp sbc config.
 *
 *  @param sbc_codec The a2dp sbc parameter.
 *
 *  @return the sample rate.
 */
uint32_t bt_a2dp_sbc_get_sampling_frequency(struct bt_a2dp_codec_sbc_params *sbc_codec);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_A2DP_CODEC_H_ */
