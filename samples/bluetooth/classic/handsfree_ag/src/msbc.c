/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/sys/util.h>

#include "msbc.h"

#if defined(CONFIG_HFP_SCO_HCI_MSBC)

#include <oi_codec_sbc.h>
#include <oi_status.h>
#include <sbc_encoder.h>

/* HFP H2 header (HFP 1.7 Vol 4, Part B): 0x01 followed by sequence nibble pattern */
#define MSBC_H2_SYNC       0x01U
#define MSBC_SBC_SYNC      0xADU
#define MSBC_H2_HDR_LEN    2U
#define MSBC_SBC_FRAME_LEN 57U
#define MSBC_PAD_LEN       1U

BUILD_ASSERT(MSBC_H2_HDR_LEN + MSBC_SBC_FRAME_LEN + MSBC_PAD_LEN == MSBC_SCO_PKT_LEN,
	     "mSBC SCO packet layout must be 60 bytes");

static OI_CODEC_SBC_DECODER_CONTEXT dec_ctx;
static uint32_t dec_data[CODEC_DATA_WORDS(1, SBC_CODEC_FAST_FILTER_BUFFERS)];
static SBC_ENC_PARAMS enc_params;
static uint8_t msbc_seq;
static bool initialized;

/* H2 second-byte sequence values: SN bits cycle 00, 01, 10, 11 */
static const uint8_t msbc_h2_sn[] = {0x08U, 0x38U, 0xC8U, 0xF8U};

static const uint8_t *msbc_find_frame(const uint8_t *pkt, uint16_t len, uint16_t *frame_len)
{
	uint16_t i;

	if (len >= (MSBC_H2_HDR_LEN + MSBC_SBC_FRAME_LEN) && pkt[0] == MSBC_H2_SYNC &&
	    pkt[MSBC_H2_HDR_LEN] == MSBC_SBC_SYNC) {
		*frame_len = MSBC_SBC_FRAME_LEN;
		return &pkt[MSBC_H2_HDR_LEN];
	}

	/* Tolerate missing/corrupt H2 by scanning for the mSBC sync word */
	for (i = 0; i + MSBC_SBC_FRAME_LEN <= len; i++) {
		if (pkt[i] == MSBC_SBC_SYNC) {
			*frame_len = MSBC_SBC_FRAME_LEN;
			return &pkt[i];
		}
	}

	return NULL;
}

int msbc_frame_sync(const uint8_t *data, uint16_t len)
{
	uint16_t i;
	uint8_t sn;

	if (data == NULL) {
		return -EINVAL;
	}

	for (i = 0; (i + MSBC_H2_HDR_LEN) <= len; i++) {
		if (data[i] != MSBC_H2_SYNC) {
			continue;
		}

		for (sn = 0; sn < ARRAY_SIZE(msbc_h2_sn); sn++) {
			if (data[i + 1U] == msbc_h2_sn[sn]) {
				return (int)i;
			}
		}
	}

	return -ENOENT;
}

int msbc_init(void)
{
	if (initialized) {
		return 0;
	}

	if (!OI_SUCCESS(
		    OI_CODEC_SBC_DecoderReset(&dec_ctx, dec_data, sizeof(dec_data), 1, 1, FALSE))) {
		return -EIO;
	}

	if (!OI_SUCCESS(OI_CODEC_SBC_DecoderConfigureMSbc(&dec_ctx))) {
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
	const uint8_t *frame;
	const uint8_t *frame_data;
	uint32_t frame_bytes;
	uint32_t pcm_bytes;
	uint16_t frame_len;

	if (!initialized || pkt == NULL || pcm_out == NULL) {
		return -EINVAL;
	}

	if (pcm_count < MSBC_PCM_SAMPLES) {
		return -ENOMEM;
	}

	frame = msbc_find_frame(pkt, len, &frame_len);
	if (frame == NULL) {
		return -EINVAL;
	}

	frame_data = frame;
	frame_bytes = frame_len;
	pcm_bytes = pcm_count * sizeof(int16_t);

	if (!OI_SUCCESS(OI_CODEC_SBC_DecodeFrame(&dec_ctx, &frame_data, &frame_bytes, pcm_out,
						 &pcm_bytes))) {
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
	pkt_out[1] = msbc_h2_sn[msbc_seq & 0x03U];
	msbc_seq = (msbc_seq + 1U) & 0x03U;

	memcpy(&pkt_out[MSBC_H2_HDR_LEN], sbc_frame, encoded);
	if (encoded < MSBC_SBC_FRAME_LEN) {
		memset(&pkt_out[MSBC_H2_HDR_LEN + encoded], 0, MSBC_SBC_FRAME_LEN - encoded);
	}
	pkt_out[MSBC_H2_HDR_LEN + MSBC_SBC_FRAME_LEN] = 0x00;

	return MSBC_SCO_PKT_LEN;
}

#else

int msbc_init(void)
{
	return -ENOTSUP;
}

int msbc_frame_sync(const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(data);
	ARG_UNUSED(len);
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
