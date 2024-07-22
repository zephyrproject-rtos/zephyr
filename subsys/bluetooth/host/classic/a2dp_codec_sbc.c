/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/libsbc/sbc.h>

uint8_t bt_a2dp_sbc_get_channel_num(struct bt_a2dp_codec_sbc_params *sbc_codec)
{
	__ASSERT_NO_MSG(sbc_codec != NULL);

	if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_MONO) {
		return 1U;
	} else if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_DUAL) {
		return 2U;
	} else if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_STREO) {
		return 2U;
	} else if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_JOINT) {
		return 2U;
	} else {
		return 0U;
	}
}

uint32_t bt_a2dp_sbc_get_sampling_frequency(struct bt_a2dp_codec_sbc_params *sbc_codec)
{
	__ASSERT_NO_MSG(sbc_codec != NULL);

	if (sbc_codec->config[0] & A2DP_SBC_SAMP_FREQ_16000) {
		return 16000U;
	} else if (sbc_codec->config[0] & A2DP_SBC_SAMP_FREQ_32000) {
		return 32000U;
	} else if (sbc_codec->config[0] & A2DP_SBC_SAMP_FREQ_44100) {
		return 44100U;
	} else if (sbc_codec->config[0] & A2DP_SBC_SAMP_FREQ_48000) {
		return 48000U;
	} else {
		return 0U;
	}
}

uint8_t bt_a2dp_sbc_get_subband_num(struct bt_a2dp_codec_sbc_params *sbc_codec)
{
	__ASSERT_NO_MSG(sbc_codec != NULL);

	if (sbc_codec->config[1] & A2DP_SBC_SUBBAND_4) {
		return 4U;
	} else if (sbc_codec->config[1] & A2DP_SBC_SUBBAND_8) {
		return 8U;
	} else {
		return 0U;
	}
}

uint8_t bt_a2dp_sbc_get_block_length(struct bt_a2dp_codec_sbc_params *sbc_codec)
{
	__ASSERT_NO_MSG(sbc_codec != NULL);

	if (sbc_codec->config[1] & A2DP_SBC_BLK_LEN_4) {
		return 4U;
	} else if (sbc_codec->config[1] & A2DP_SBC_BLK_LEN_8) {
		return 8U;
	} else if (sbc_codec->config[1] & A2DP_SBC_BLK_LEN_12) {
		return 12U;
	} else if (sbc_codec->config[1] & A2DP_SBC_BLK_LEN_16) {
		return 16U;
	} else {
		return 0U;
	}
}

enum sbc_ch_mode bt_a2dp_sbc_get_channel_mode(struct bt_a2dp_codec_sbc_params *sbc_codec)
{
	__ASSERT_NO_MSG(sbc_codec != NULL);

	if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_MONO) {
		return SBC_CH_MODE_MONO;
	} else if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_DUAL) {
		return SBC_CH_MODE_DUAL_CHANNEL;
	} else if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_STREO) {
		return SBC_CH_MODE_STEREO;
	} else if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_JOINT) {
		return SBC_CH_MODE_JOINT_STEREO;
	} else {
		return SBC_CH_MODE_MONO;
	}
}

enum sbc_alloc_mthd bt_a2dp_sbc_get_allocation_method(struct bt_a2dp_codec_sbc_params *sbc_codec)
{
	__ASSERT_NO_MSG(sbc_codec != NULL);

	if (sbc_codec->config[1] & A2DP_SBC_ALLOC_MTHD_SNR) {
		return SBC_ALLOC_MTHD_SNR;
	} else {
		return SBC_ALLOC_MTHD_LOUDNESS;
	}
}
