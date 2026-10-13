/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink sample stereo output buffer
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>

#include "lc3.h"
#include "stereo_out.h"

LOG_MODULE_REGISTER(stereo_out, CONFIG_LOG_DEFAULT_LEVEL);

#define STEREO_OUT_RING_BUF_SIZE (CONFIG_BT_ISO_RX_BUF_COUNT * LC3_MAX_NUM_SAMPLES_STEREO)

static struct decoded_sdu {
	int16_t right_frames[CONFIG_MAX_CODEC_FRAMES_PER_SDU][LC3_MAX_NUM_SAMPLES_MONO];
	int16_t left_frames[CONFIG_MAX_CODEC_FRAMES_PER_SDU][LC3_MAX_NUM_SAMPLES_MONO];
	size_t right_frames_cnt;
	size_t left_frames_cnt;
	size_t mono_frames_cnt;
	uint32_t ts;
} decoded_sdu;

RING_BUF_DECLARE(stereo_out_ring_buf, STEREO_OUT_RING_BUF_SIZE);

static void stereo_out_send_frames(void)
{
	const bool is_left_only =
		decoded_sdu.right_frames_cnt == 0U && decoded_sdu.mono_frames_cnt == 0U;
	const bool is_right_only =
		decoded_sdu.left_frames_cnt == 0U && decoded_sdu.mono_frames_cnt == 0U;
	const bool is_mono_only =
		decoded_sdu.left_frames_cnt == 0U && decoded_sdu.right_frames_cnt == 0U;
	const bool is_single_channel = is_left_only || is_right_only || is_mono_only;
	const size_t frame_cnt =
		MAX(decoded_sdu.mono_frames_cnt,
		    MAX(decoded_sdu.left_frames_cnt, decoded_sdu.right_frames_cnt));
	static size_t cnt;

	/* Send frames to the output - If we only have a single channel we mix it to stereo */
	for (size_t i = 0U; i < frame_cnt; i++) {
		static int16_t stereo_frame[LC3_MAX_NUM_SAMPLES_STEREO];
		const int16_t *right_frame = decoded_sdu.right_frames[i];
		const int16_t *left_frame = decoded_sdu.left_frames[i];
		const int16_t *mono_frame = decoded_sdu.left_frames[i]; /* use left as mono */
		static size_t fail_cnt;
		uint32_t rb_size;

		/* Not enough space to store data */
		if (ring_buf_space_get(&stereo_out_ring_buf) < sizeof(stereo_frame)) {
			if (CONFIG_INFO_REPORTING_INTERVAL > 0 &&
			    (fail_cnt % CONFIG_INFO_REPORTING_INTERVAL) == 0U) {
				LOG_WRN("[%zu] Could not send more than %zu frames to the output",
					fail_cnt, i);
			}

			fail_cnt++;

			break;
		}

		fail_cnt = 0U;

		/* Generate the stereo frame
		 *
		 * If we only have single channel then we mix that to stereo
		 */
		for (int j = 0; j < LC3_MAX_NUM_SAMPLES_MONO; j++) {
			if (is_single_channel) {
				int16_t sample = 0;

				/* Mix to stereo as LRLRLRLR */
				if (is_left_only) {
					sample = left_frame[j];
				} else if (is_right_only) {
					sample = right_frame[j];
				} else if (is_mono_only) {
					sample = mono_frame[j];
				}

				stereo_frame[j * 2] = sample;
				stereo_frame[j * 2 + 1] = sample;
			} else {
				stereo_frame[j * 2] = left_frame[j];
				stereo_frame[j * 2 + 1] = right_frame[j];
			}
		}

		rb_size = ring_buf_put(&stereo_out_ring_buf, (uint8_t *)stereo_frame,
				       sizeof(stereo_frame));
		if (rb_size != sizeof(stereo_frame)) {
			LOG_WRN("Failed to put frame on the output ring buf");

			break;
		}
	}

	if (CONFIG_INFO_REPORTING_INTERVAL > 0 && (++cnt % CONFIG_INFO_REPORTING_INTERVAL) == 0U) {
		LOG_INF("[%zu]: Sending %u audio frame", cnt, frame_cnt);
	}

	stereo_out_clear_frames();
}

static bool ts_overflowed(uint32_t ts)
{
	/* If the timestamp is a factor of 10 in difference, then we assume that TS overflowed
	 * We cannot simply check if `ts < decoded_sdu.ts` as that could also indicate old data
	 */
	return ((uint64_t)ts * 10 < decoded_sdu.ts);
}

int stereo_out_add_frame(enum bt_audio_location chan_allocation, const int16_t *frame,
			 size_t frame_size, uint32_t ts)
{
	const bool is_left = (chan_allocation & BT_AUDIO_LOCATION_FRONT_LEFT) != 0;
	const bool is_right = (chan_allocation & BT_AUDIO_LOCATION_FRONT_RIGHT) != 0;
	const bool is_mono = chan_allocation == BT_AUDIO_LOCATION_MONO_AUDIO;
	const uint8_t ts_jitter_us = 100U; /* timestamps may have jitter */

	if (frame_size > LC3_MAX_NUM_SAMPLES_MONO * sizeof(int16_t) || frame_size == 0U) {
		LOG_DBG("Invalid frame of size %zu", frame_size);

		return -EINVAL;
	}

	if (bt_audio_get_chan_count(chan_allocation) != 1) {
		LOG_DBG("Invalid channel allocation %d", chan_allocation);

		return -EINVAL;
	}

	if (((is_left || is_right) && decoded_sdu.mono_frames_cnt != 0U) ||
	    (is_mono &&
	     (decoded_sdu.left_frames_cnt != 0U || decoded_sdu.right_frames_cnt != 0U))) {
		LOG_WRN("Cannot mix and match mono with left or right: %s: %u | %u | %u",
			is_left    ? "is_left"
			: is_right ? "is_right"
				   : "is_mono",
			decoded_sdu.mono_frames_cnt, decoded_sdu.left_frames_cnt,
			decoded_sdu.right_frames_cnt);

		return -EINVAL;
	}

	/* Check if the frame can be combined with a previous frame from another channel, of if
	 * we have to send previous data to the output and then store the current frame
	 *
	 * This is done by comparing the timestamps of the frames, and in the case that they are the
	 * same, there are additional checks to see if we have received more left than right frames,
	 * in which case we also send existing data
	 */

	if (ts + ts_jitter_us < decoded_sdu.ts && !ts_overflowed(ts)) {
		/* Old data, discard */
		return -ENOEXEC;
	} else if (ts > decoded_sdu.ts + ts_jitter_us || ts_overflowed(ts)) {
		/* We are getting new data - Send existing data to ring buffer */
		stereo_out_send_frames();
	} else { /* same timestamp */
		bool send = false;

		if (is_left && decoded_sdu.left_frames_cnt > decoded_sdu.right_frames_cnt) {
			/* We are receiving left again before a right, send to the output */
			send = true;
		} else if (is_right && decoded_sdu.right_frames_cnt > decoded_sdu.left_frames_cnt) {
			/* We are receiving right again before a left, send to the output */
			send = true;
		} else if (is_mono) {
			/* always send mono as it comes */
			send = true;
		}

		if (send) {
			stereo_out_send_frames();
		}
	}

	if (is_left) {
		if (decoded_sdu.left_frames_cnt >= ARRAY_SIZE(decoded_sdu.left_frames)) {
			LOG_WRN("Could not add more left frames");

			return -ENOMEM;
		}

		(void)memcpy(decoded_sdu.left_frames[decoded_sdu.left_frames_cnt], frame,
			     frame_size);
		decoded_sdu.left_frames_cnt++;
	} else if (is_right) {
		if (decoded_sdu.right_frames_cnt >= ARRAY_SIZE(decoded_sdu.right_frames)) {
			LOG_WRN("Could not add more right frames");

			return -ENOMEM;
		}

		(void)memcpy(decoded_sdu.right_frames[decoded_sdu.right_frames_cnt], frame,
			     frame_size);
		decoded_sdu.right_frames_cnt++;
	} else if (is_mono) {
		/* Use left as mono*/
		if (decoded_sdu.mono_frames_cnt >= ARRAY_SIZE(decoded_sdu.left_frames)) {
			LOG_WRN("Could not add more mono frames");

			return -ENOMEM;
		}

		(void)memcpy(decoded_sdu.left_frames[decoded_sdu.mono_frames_cnt], frame,
			     frame_size);
		decoded_sdu.mono_frames_cnt++;
	} else {
		/* Unsupported channel */
		LOG_DBG("Unsupported channel %d", chan_allocation);

		return -EINVAL;
	}

	decoded_sdu.ts = ts;

	return 0;
}

void stereo_out_clear_frames(void)
{
	decoded_sdu.mono_frames_cnt = 0U;
	decoded_sdu.right_frames_cnt = 0U;
	decoded_sdu.left_frames_cnt = 0U;
	decoded_sdu.ts = 0U;
}

uint32_t stereo_out_read(uint8_t *dst, uint32_t size)
{
	if (dst == NULL) {
		const uint32_t discard = MIN(size, ring_buf_size_get(&stereo_out_ring_buf));

		ring_buf_consume(&stereo_out_ring_buf, discard);

		return discard;
	}

	return ring_buf_get(&stereo_out_ring_buf, dst, size);
}
