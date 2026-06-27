/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/sys/byteorder.h>

#include "msbc.h"

#if defined(CONFIG_HFP_SCO_HCI_MSBC)

#include <oi_codec_sbc.h>
#include <oi_status.h>
#include <sbc_encoder.h>

#define MSBC_H2_SYNC     0x01U
#define MSBC_SBC_SYNC    0xADU
#define MSBC_H2_HDR_LEN  3U
#define MSBC_SBC_FRAME_LEN (MSBC_SCO_PKT_LEN - MSBC_H2_HDR_LEN)

static OI_CODEC_SBC_DECODER_CONTEXT dec_ctx;
static uint32_t dec_data[CODEC_DATA_WORDS(1, SBC_CODEC_FAST_FILTER_BUFFERS)];
static SBC_ENC_PARAMS enc_params;
static uint8_t msbc_seq;
static bool initialized;

int msbc_init(void)
{
	OI_STATUS status;

	if (initialized) {
		return 0;
	}

	status = OI_CODEC_SBC_DecoderReset(&dec_ctx, dec_data, sizeof(dec_data), 1, 1, FALSE);
	if (!OI_SUCCESS(status)) {
		return -EIO;
	}

	status = OI_CODEC_SBC_DecoderConfigureMSbc(&dec_ctx);
	if (!OI_SUCCESS(status)) {
		return -EIO;
	}

	memset(&enc_params, 0, sizeof(enc_params));
	enc_params.Format = SBC_FORMAT_MSBC;
	enc_params.s16SamplingFreq = SBC_sf16000;
	enc_params.s16ChannelMode = SBC_MONO;
	enc_params.s16NumOfSubBands = SUB_BANDS_8;
	enc_params.s16NumOfBlocks = 15;
	enc_params.s16AllocationMethod = SBC_LOUDNESS;
	enc_params.s16BitPool = 26;
	enc_params.u16BitRate = SBC_WBS_BITRATE / 1000U;
	SBC_Encoder_Init(&enc_params);
	enc_params.s16BitPool = 26;

	msbc_seq = 0;
	initialized = true;

	return 0;
}

int msbc_decode(const uint8_t *pkt, uint16_t len, int16_t *pcm_out, size_t pcm_count)
{
	const OI_BYTE *frame_data;
	uint32_t frame_bytes;
	uint32_t pcm_bytes;
	OI_STATUS status;

	if (!initialized || pkt == NULL || pcm_out == NULL) {
		return -EINVAL;
	}

	if (len < MSBC_H2_HDR_LEN + 1U) {
		return -EINVAL;
	}

	if (pcm_count < MSBC_PCM_SAMPLES) {
		return -ENOMEM;
	}

	frame_data = pkt;
	frame_bytes = len;
	pcm_bytes = pcm_count * sizeof(int16_t);

	status = OI_CODEC_SBC_DecodeFrame(&dec_ctx, &frame_data, &frame_bytes,
					  pcm_out, &pcm_bytes);
	if (!OI_SUCCESS(status)) {
		return -EIO;
	}

	return (int)(pcm_bytes / sizeof(int16_t));
}

int msbc_encode(const int16_t *pcm_in, size_t pcm_count, uint8_t *pkt_out, size_t pkt_size)
{
	uint8_t sbc_frame[MSBC_SBC_FRAME_LEN];
	uint32_t encoded;

	if (!initialized || pcm_in == NULL || pkt_out == NULL) {
		return -EINVAL;
	}

	if (pcm_count < MSBC_PCM_SAMPLES || pkt_size < MSBC_SCO_PKT_LEN) {
		return -ENOMEM;
	}

	encoded = SBC_Encode(&enc_params, (int16_t *)pcm_in, sbc_frame);
	if (encoded == 0U || encoded > MSBC_SBC_FRAME_LEN) {
		return -EIO;
	}

	pkt_out[0] = MSBC_H2_SYNC;
	pkt_out[1] = (uint8_t)(msbc_seq++ << 4);
	pkt_out[2] = 0x00;
	memcpy(&pkt_out[MSBC_H2_HDR_LEN], sbc_frame, encoded);

	if (encoded < MSBC_SBC_FRAME_LEN) {
		memset(&pkt_out[MSBC_H2_HDR_LEN + encoded], 0, MSBC_SBC_FRAME_LEN - encoded);
	}

	return MSBC_SCO_PKT_LEN;
}

#else

int msbc_init(void)
{
	return -ENOTSUP;
}

int msbc_decode(const uint8_t *pkt, uint16_t len, int16_t *pcm_out, size_t pcm_count)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(len);
	ARG_UNUSED(pcm_out);
	ARG_UNUSED(pcm_count);
	return -ENOTSUP;
}

int msbc_encode(const int16_t *pcm_in, size_t pcm_count, uint8_t *pkt_out, size_t pkt_size)
{
	ARG_UNUSED(pcm_in);
	ARG_UNUSED(pcm_count);
	ARG_UNUSED(pkt_out);
	ARG_UNUSED(pkt_size);
	return -ENOTSUP;
}

#endif /* CONFIG_HFP_SCO_HCI_MSBC */
