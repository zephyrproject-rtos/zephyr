/*
 * SPDX-FileCopyrightText: Copyright 2026 Ezurio LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LC3 decode stage: received LC3 frame blocks -> interleaved stereo 16-bit PCM.
 * Kept free of any output-transport knowledge so the same decoder can feed an
 * I2S/codec sink, a USB sink, etc.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <lc3.h>

#include <sample_bt_audio_lc3_decode.h>
#include <sample_bt_audio_pcm_sink.h>

LOG_MODULE_DECLARE(sample_bt_audio_playback, CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_LOG_LEVEL);

/* Two LC3 decoder contexts for stereo. Sized for the worst-case 48 kHz / 10 ms.
 * Memory and handle are kept together so a single array stays coupled per channel.
 */
struct lc3_decoder_ctx {
	lc3_decoder_mem_48k_t mem;
	lc3_decoder_t decoder;
};
static struct lc3_decoder_ctx decoders[SAMPLE_BT_AUDIO_PCM_SINK_MAX_CHAN];

int sample_bt_audio_lc3_decode_setup(uint32_t freq_hz, uint32_t frame_duration_us,
				     uint8_t chan_cnt)
{
	for (uint8_t c = 0U; c < chan_cnt; c++) {
		decoders[c].decoder = lc3_setup_decoder((int)frame_duration_us, (int)freq_hz, 0,
							&decoders[c].mem);
		if (decoders[c].decoder == NULL) {
			LOG_ERR("lc3_setup_decoder ch%u failed", c);
			return -EINVAL;
		}
	}

	return 0;
}

bool sample_bt_audio_lc3_decode_is_ready(void)
{
	return decoders[0].decoder != NULL;
}

bool sample_bt_audio_lc3_decode_block(struct net_buf *buf, uint16_t octets_per_frame,
				      uint8_t chan_cnt, uint32_t samples_per_frame, bool do_plc,
				      int16_t *pcm)
{
	const uint16_t opf = octets_per_frame;

	if (chan_cnt == 1U) {
		const void *in = do_plc ? NULL : net_buf_pull_mem(buf, opf);
		int err = lc3_decode(decoders[0].decoder, in, in != NULL ? opf : 0,
				     LC3_PCM_FORMAT_S16, pcm, 1);

		if (err < 0) {
			return false;
		}
		/* Duplicate mono into both output channels. */
		for (int i = (int)samples_per_frame - 1; i >= 0; i--) {
			pcm[i * 2 + 0] = pcm[i];
			pcm[i * 2 + 1] = pcm[i];
		}
	} else {
		const void *in_l = do_plc ? NULL : net_buf_pull_mem(buf, opf);
		const void *in_r = do_plc ? NULL : net_buf_pull_mem(buf, opf);
		int err = lc3_decode(decoders[0].decoder, in_l, in_l != NULL ? opf : 0,
				     LC3_PCM_FORMAT_S16, &pcm[0], 2);

		if (err < 0) {
			return false;
		}
		err = lc3_decode(decoders[1].decoder, in_r, in_r != NULL ? opf : 0,
				 LC3_PCM_FORMAT_S16, &pcm[1], 2);
		if (err < 0) {
			return false;
		}
	}

	return true;
}
