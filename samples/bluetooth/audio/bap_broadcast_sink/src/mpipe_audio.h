/**
 * @file
 * @brief BAP Broadcast Sink audio data path built on the mpipe framework.
 *
 * All mpipe related code for this sample lives in mpipe_audio.c and is guarded
 * by CONFIG_BAP_SINK_AUDIO_PATH_MPIPE, which requires both the mpipe framework
 * and codec audio output. This header exposes a small, pipeline-agnostic API to
 * the rest of the sample (stream_rx.c and main.c). When that path is not
 * selected the API collapses to inline no-ops so callers do not need their own
 * #ifdefs.
 *
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BAP_BROADCAST_SINK_MPIPE_AUDIO_H_
#define BAP_BROADCAST_SINK_MPIPE_AUDIO_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>

#if defined(CONFIG_BAP_SINK_AUDIO_PATH_MPIPE)

/**
 * @brief Set the number of output channels for the next pipeline build.
 *
 * Must be called once the number of BIS streams that will be synced is known,
 * before @ref mpipe_audio_request_start. The channel count drives the I2S
 * block size and the number of LC3 decoders allocated: one BIS stream per
 * channel, rendered as interleaved PCM.
 *
 * The value is recorded immediately and applied by the next pipeline build.
 * Calling this while a previous session is still tearing down is safe: the
 * build work item always runs after the teardown work item on the same FIFO
 * work queue, so the most recent value is the one that takes effect.
 *
 * @param channels Number of BIS streams (output channels), 1 or 2.
 */
void mpipe_audio_set_channels(uint8_t channels);

/**
 * @brief Build and configure the audio pipeline for a given stream format.
 *
 * Initializes the pipeline elements (BAP source, LC3 decoder transform and
 * hardware codec sink), sets their properties, links them and moves the
 * pipeline to PAUSED so the format and buffer pools are negotiated. Safe to
 * call again after @ref mpipe_audio_stop.
 *
 * @param samplerate   PCM output sample rate in Hz.
 * @param frame_dur_us LC3 frame duration in microseconds (7500 or 10000), as
 *                     negotiated in the codec configuration.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_audio_init(uint32_t samplerate, uint32_t frame_dur_us);

/**
 * @brief Start streaming (move the pipeline to PLAYING).
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_audio_start(void);

/**
 * @brief Request an asynchronous pipeline build and start.
 *
 * Submits the (blocking) hardware codec bring-up and pipeline build/start to a
 * dedicated work queue and returns immediately. This must be used from the
 * stream "started" callback, which runs on the Bluetooth RX work queue thread:
 * running the ~1.2 s blocking codec bring-up on that thread would starve the
 * transport RX buffer pool and trigger a controller-unresponsive assert.
 *
 * The work queue is started eagerly at boot, so this call only enqueues work.
 * The call is idempotent: if the pipeline is already starting or running it
 * returns immediately without enqueueing duplicate work.
 *
 * @param samplerate   PCM output sample rate in Hz.
 * @param frame_dur_us LC3 frame duration in microseconds (7500 or 10000).
 */
void mpipe_audio_request_start(uint32_t samplerate, uint32_t frame_dur_us);

/**
 * @brief Feed one LC3 frame from a BIS stream into the pipeline.
 *
 * Maps @p ch to a PCM output channel. The pipeline decodes all active channels
 * once per LC3 frame period and writes them interleaved to the hardware codec.
 * Frames from a channel index that equals or exceeds the configured channel
 * count are silently discarded.
 *
 * @param ch           Zero-based output channel index of the originating BIS.
 * @param data         Pointer to the LC3 frame payload.
 * @param len          Length of the payload in bytes.
 * @param ts           Controller timestamp of the SDU.
 * @param ts_valid     True if @p ts holds a timestamp from the controller.
 */
void mpipe_audio_push(uint8_t ch, const uint8_t *data, size_t len,
		      uint32_t ts, bool ts_valid);

/**
 * @brief Feed a lost-frame marker for a BIS channel into the pipeline.
 *
 * Keeps the channel's LC3 decoder advancing with Packet Loss Concealment when
 * an SDU is lost, so audio degrades gracefully rather than stalling.
 *
 * @param ch Zero-based output channel index of the originating BIS.
 */
void mpipe_audio_push_lost(uint8_t ch);

/**
 * @brief Record the presentation delay negotiated for the stream.
 *
 * @param pd_us Presentation delay in microseconds, or BT_BAP_PD_UNSET.
 */
void mpipe_audio_set_presentation_delay(uint32_t pd_us);

/**
 * @brief Notify the audio layer that one BIS stream has stopped.
 *
 * The pipeline is torn down only when all started streams have stopped, so
 * calling this for the first of two BIS streams leaves the second one playing.
 * Must be called once per @ref mpipe_audio_request_start call.
 */
void mpipe_audio_stream_stopped(void);

/**
 * @brief Stop streaming and tear the pipeline back down to READY.
 *
 * Unconditional teardown regardless of the started-stream count. Use
 * @ref mpipe_audio_stream_stopped for the per-BIS lifecycle instead.
 */
void mpipe_audio_stop(void);

#else /* !CONFIG_BAP_SINK_AUDIO_PATH_MPIPE */

static inline void mpipe_audio_set_channels(uint8_t channels)
{
	(void)channels;
}

static inline int mpipe_audio_init(uint32_t samplerate, uint32_t frame_dur_us)
{
	(void)samplerate;
	(void)frame_dur_us;
	return 0;
}

static inline int mpipe_audio_start(void)
{
	return 0;
}

static inline void mpipe_audio_request_start(uint32_t samplerate, uint32_t frame_dur_us)
{
	(void)samplerate;
	(void)frame_dur_us;
}

static inline void mpipe_audio_push(uint8_t ch, const uint8_t *data,
				    size_t len, uint32_t ts, bool ts_valid)
{
	(void)ch;
	(void)data;
	(void)len;
	(void)ts;
	(void)ts_valid;
}

static inline void mpipe_audio_push_lost(uint8_t ch)
{
	(void)ch;
}

static inline void mpipe_audio_set_presentation_delay(uint32_t pd_us)
{
	(void)pd_us;
}

static inline void mpipe_audio_stream_stopped(void)
{
}

static inline void mpipe_audio_stop(void)
{
}

#endif /* CONFIG_BAP_SINK_AUDIO_PATH_MPIPE */

#endif /* BAP_BROADCAST_SINK_MPIPE_AUDIO_H_ */
