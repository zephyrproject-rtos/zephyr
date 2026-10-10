/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Decode-only PCM sink: discards the decoded PCM so the LC3 decode path can run on boards
 * without an audio codec. write() blocks until the block's playout time, standing in for
 * the pacing a real transport provides, so the playback worker does not spin.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <pcm_sink.h>

static uint32_t frame_us;
static int64_t next_block_us; /* uptime at which the next block is due */

int sample_bt_audio_pcm_sink_init(void)
{
	return 0;
}

int sample_bt_audio_pcm_sink_configure(const struct sample_bt_audio_pcm_sink_cfg *cfg)
{
	frame_us = cfg->frame_duration_us;

	return 0;
}

int sample_bt_audio_pcm_sink_start(void)
{
	next_block_us = (int64_t)k_ticks_to_us_floor64(k_uptime_ticks());

	return 0;
}

void sample_bt_audio_pcm_sink_stop(void)
{
}

int sample_bt_audio_pcm_sink_write(void *block, size_t bytes)
{
	int64_t remaining_us;

	ARG_UNUSED(block);
	ARG_UNUSED(bytes);

	/* Pace against an absolute schedule so per-write overhead does not accumulate. */
	next_block_us += frame_us;
	remaining_us = next_block_us - (int64_t)k_ticks_to_us_floor64(k_uptime_ticks());
	if (remaining_us > 0) {
		k_sleep(K_USEC(remaining_us));
	}

	return 0;
}

void sample_bt_audio_pcm_sink_set_volume(uint8_t vcs_volume)
{
	ARG_UNUSED(vcs_volume);
}

void sample_bt_audio_pcm_sink_set_mute(bool mute)
{
	ARG_UNUSED(mute);
}
