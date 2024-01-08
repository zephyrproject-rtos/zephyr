/** @file
 * @brief Advance Audio Distribution Profile - SBC Codec header.
 */
/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2015-2016 Intel Corporation
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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Sampling Frequency
 * @{
 */
#define A2DP_SBC_SAMP_FREQ_16000 BIT(7) /**< 16 kHz */
#define A2DP_SBC_SAMP_FREQ_32000 BIT(6) /**< 32 kHz */
#define A2DP_SBC_SAMP_FREQ_44100 BIT(5) /**< 44.1 kHz */
#define A2DP_SBC_SAMP_FREQ_48000 BIT(4) /**< 48 kHz */
/** @} */

/**
 * @name Channel Mode
 * @{
 */
#define A2DP_SBC_CH_MODE_MONO  BIT(3) /**< Mono */
#define A2DP_SBC_CH_MODE_DUAL  BIT(2) /**< Dual Channel */
#define A2DP_SBC_CH_MODE_STREO BIT(1) /**< Stereo */
#define A2DP_SBC_CH_MODE_JOINT BIT(0) /**< Joint Stereo */
/** @} */

/**
 * @name Block Length
 * @{
 */
#define A2DP_SBC_BLK_LEN_4  BIT(7) /**< 4 blocks */
#define A2DP_SBC_BLK_LEN_8  BIT(6) /**< 8 blocks */
#define A2DP_SBC_BLK_LEN_12 BIT(5) /**< 12 blocks */
#define A2DP_SBC_BLK_LEN_16 BIT(4) /**< 16 blocks */
/** @} */

/**
 * @name Subbands
 * @{
 */
#define A2DP_SBC_SUBBAND_4 BIT(3) /**< 4 subbands */
#define A2DP_SBC_SUBBAND_8 BIT(2) /**< 8 subbands */
/** @} */

/**
 * @name Bit pool Allocation Method
 * @{
 */
#define A2DP_SBC_ALLOC_MTHD_SNR      BIT(1) /**< Allocate based on loudness of the subband signal */
#define A2DP_SBC_ALLOC_MTHD_LOUDNESS BIT(0) /**< Allocate based on the signal-to-noise ratio */
/** @} */

/**
 * Gets the sampling rate from a codec preset
 * @param preset Codec preset
 */
#define BT_A2DP_SBC_SAMP_FREQ(preset)    ((preset->config[0] >> 4) & 0x0f)
/**
 * Gets the channel mode from a codec preset
 * @param preset Codec preset
 */
#define BT_A2DP_SBC_CHAN_MODE(preset)    ((preset->config[0]) & 0x0f)
/**
 * Gets the block length from a codec preset
 * @param preset Codec preset
 */
#define BT_A2DP_SBC_BLK_LEN(preset)      ((preset->config[1] >> 4) & 0x0f)
/**
 * Gets the number subbands from a codec preset
 * @param preset Codec preset
 */
#define BT_A2DP_SBC_SUB_BAND(preset)     ((preset->config[1] >> 2) & 0x03)
/**
 * Gets the bitpool allocation method from a codec preset
 * @param preset Codec preset
 */
#define BT_A2DP_SBC_ALLOC_MTHD(preset)   ((preset->config[1]) & 0x03)

/** @brief SBC Codec */
struct bt_a2dp_codec_sbc_params {
	/** First two octets of configuration */
	uint8_t config[2];
	/** Minimum Bitpool Value */
	uint8_t min_bitpool;
	/** Maximum Bitpool Value */
	uint8_t max_bitpool;
} __packed;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_A2DP_CODEC_H_ */
