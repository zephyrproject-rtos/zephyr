/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/atomic.h>
#include <sys/util.h>
#include <sys/slist.h>
#include <sys/byteorder.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/a2dp.h>
#include <bluetooth/a2dp_codec_sbc.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_A2DP)
#define LOG_MODULE_NAME bt_a2dp_sbc
#include "common/log.h"

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

#if defined(CONFIG_BT_A2DP_SINK)
int bt_a2dp_sbc_decoder_init(struct bt_a2dp_codec_sbc_decoder *sbc_decoder)
{
	OI_STATUS status = OI_CODEC_SBC_DecoderReset(
					&sbc_decoder->context,
					sbc_decoder->context_data,
					sizeof(sbc_decoder->context_data),
					2, 2, FALSE);
	if (!OI_SUCCESS(status)) {
		BT_DBG("OI_CODEC_SBC_DecoderReset error\r\n");
	}
	return 0;
}

int bt_a2dp_sbc_decode(struct bt_a2dp_codec_sbc_decoder *sbc_decoder,
				uint8_t **sbc_data, uint32_t *sbc_data_len,
				uint16_t *pcm_data, uint32_t *pcm_data_len)
{
	OI_STATUS status;

	status = OI_CODEC_SBC_DecodeFrame(&sbc_decoder->context,
					(const OI_BYTE**)sbc_data,
					sbc_data_len,
					pcm_data,
					pcm_data_len
					);
	if (!OI_SUCCESS(status)) {
		return -1;
	} else {
		return 0;
	}
}
#endif

#if defined(CONFIG_BT_A2DP_SOURCE)
static uint8_t bt_a2dp_sbc_get_allocation_method_index(struct bt_a2dp_codec_sbc_params *sbc_codec)
{
	__ASSERT_NO_MSG(sbc_codec != NULL);

	if (sbc_codec->config[1] & A2DP_SBC_ALLOC_MTHD_LOUDNESS) {
		return 0U;
	} else if (sbc_codec->config[1] & A2DP_SBC_ALLOC_MTHD_SNR) {
		return 1U;
	} else {
		return 0U;
	}
}

static uint8_t bt_a2dp_sbc_get_channel_mode_index(struct bt_a2dp_codec_sbc_params *sbc_codec)
{
	__ASSERT_NO_MSG(sbc_codec != NULL);

	if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_MONO) {
		return 0U;
	} else if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_DUAL) {
		return 1U;
	} else if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_STREO) {
		return 2U;
	} else if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_JOINT) {
		return 3U;
	} else {
		return 0U;
	}
}

static uint8_t bt_a2dp_sbc_get_sampling_fequency_index(struct bt_a2dp_codec_sbc_params *sbc_codec)
{
	__ASSERT_NO_MSG(sbc_codec != NULL);

	if (sbc_codec->config[0] & A2DP_SBC_SAMP_FREQ_16000) {
		return 0U;
	} else if (sbc_codec->config[0] & A2DP_SBC_SAMP_FREQ_32000) {
		return 1U;
	} else if (sbc_codec->config[0] & A2DP_SBC_SAMP_FREQ_44100) {
		return 2U;
	} else if (sbc_codec->config[0] & A2DP_SBC_SAMP_FREQ_48000) {
		return 3U;
	} else {
		return 0U;
	}
}

int bt_a2dp_sbc_encoder_init(struct bt_a2dp_codec_sbc_encoder *sbc_encoder)
{
	memset(&sbc_encoder->sbc_encoder_params, 0, sizeof(sbc_encoder->sbc_encoder_params));

	sbc_encoder->sbc_encoder_params.s16ChannelMode =
		bt_a2dp_sbc_get_channel_mode_index(sbc_encoder->sbc);
	sbc_encoder->sbc_encoder_params.s16NumOfSubBands =
		bt_a2dp_sbc_get_subband_num(sbc_encoder->sbc);
	sbc_encoder->sbc_encoder_params.s16NumOfBlocks =
		bt_a2dp_sbc_get_block_length(sbc_encoder->sbc);
	sbc_encoder->sbc_encoder_params.s16AllocationMethod =
		bt_a2dp_sbc_get_allocation_method_index(sbc_encoder->sbc);
	sbc_encoder->sbc_encoder_params.s16SamplingFreq =
		bt_a2dp_sbc_get_sampling_fequency_index(sbc_encoder->sbc);
	sbc_encoder->sbc_encoder_params.u16BitRate = CONFIG_BT_A2DP_SBC_ENCODER_BIT_RATE;
	SBC_Encoder_Init(&sbc_encoder->sbc_encoder_params);

	return 0;
}

int bt_a2dp_sbc_encode(struct bt_a2dp_codec_sbc_encoder *sbc_encoder, uint8_t
	*input_frame, uint8_t *output_buffer, uint32_t *output_len)
{
	if ((sbc_encoder == NULL) || (input_frame == NULL) ||
		(output_buffer == NULL) || (output_len == NULL)) {
		return -EPERM;
	}
	*output_len = SBC_Encode(&sbc_encoder->sbc_encoder_params,
					(int16_t *)input_frame, output_buffer);
	return 0;
}
#endif
