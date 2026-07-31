/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
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

#include <lc3_decode.h>
#include <pcm_sink.h>

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
			LOG_DBG("lc3_setup_decoder ch%u failed", c);
			return -EINVAL;
		}
	}

	return 0;
}

bool sample_bt_audio_lc3_decode_block(struct net_buf *buf, uint16_t octets_per_frame,
				      uint8_t chan_cnt, uint32_t samples_per_frame, bool do_plc,
				      int16_t *pcm)
{
	const uint32_t stride = SAMPLE_BT_AUDIO_PCM_SINK_MAX_CHAN;

	for (uint8_t c = 0U; c < chan_cnt; c++) {
		/* The SDU holds one frame per channel back to back; stride only
		 * interleaves the decoded PCM output.
		 */
		const void *in = do_plc ? NULL : net_buf_pull_mem(buf, octets_per_frame);
		int err = lc3_decode(decoders[c].decoder, in, (in != NULL) ? octets_per_frame : 0,
				     LC3_PCM_FORMAT_S16, &pcm[c], (int)stride);

		if (err < 0) {
			return false;
		}
	}

	if (chan_cnt == 1U) {
		/* Duplicate mono into the right output channel. */
		for (uint32_t i = 0U; i < samples_per_frame; i++) {
			pcm[(i * stride) + 1U] = pcm[i * stride];
		}
	}

	return true;
}
