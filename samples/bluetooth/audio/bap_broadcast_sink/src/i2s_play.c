/**
 * @file
 * @brief BAP Broadcast Sink I2S playback logic.
 *
 * Software playback layer on top of hw_codec_i2s.c. i2s_play_write() copies
 * decoded PCM into an elastic ring; a dedicated thread feeds the SAI DMA,
 * pre-buffers silence on startup/underrun, and logs ring depth + clock drift.
 *
 * Copyright (c) 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/toolchain.h>

#include "i2s_play.h"

LOG_MODULE_REGISTER(i2s_play, CONFIG_LOG_DEFAULT_LEVEL);

/* Fallback defaults in case the Kconfig options are not selected. */
#ifndef CONFIG_BAP_BROADCAST_SINK_PCM_BUFFER_PLAY_COUNT
#define CONFIG_BAP_BROADCAST_SINK_PCM_BUFFER_PLAY_COUNT 12
#endif
#ifndef CONFIG_BAP_BROADCAST_SINK_PCM_BUFFER_PRELOAD_COUNT
#define CONFIG_BAP_BROADCAST_SINK_PCM_BUFFER_PRELOAD_COUNT 2
#endif
#ifndef CONFIG_BAP_BROADCAST_SINK_PCM_PREBUFFER_COUNT
#define CONFIG_BAP_BROADCAST_SINK_PCM_PREBUFFER_COUNT 4
#endif

/* Number of silence blocks primed into the SAI DMA before it is started. */
#define I2S_TX_PRELOAD_BLOCKS CONFIG_BAP_BROADCAST_SINK_PCM_BUFFER_PRELOAD_COUNT

/* Depth of the elastic PCM ring between producer and playback thread. */
#define PCM_RING_DEPTH CONFIG_BAP_BROADCAST_SINK_PCM_BUFFER_PLAY_COUNT

/* Blocks the ring must hold before playout starts (or resumes after a stall). */
#define PCM_PREBUFFER_BLOCKS CONFIG_BAP_BROADCAST_SINK_PCM_PREBUFFER_COUNT

#define I2S_TX_TIMEOUT_MS (2000U)

#define I2S_PLAY_THREAD_STACK_SIZE 2048
#define I2S_PLAY_THREAD_PRIO 5

/* Hardware handles and stream timing supplied via i2s_play_setup(). */
static const struct device *i2s_tx_dev;
static struct k_mem_slab *tx_mem_slab;
static uint32_t tx_block_size;
static uint32_t tx_frame_dur_us;

/* Ring of PCM block pointers handed from producer to the playback thread. */
static void *pcm_ring_storage[PCM_RING_DEPTH];
static struct k_msgq pcm_ring;

/* Cleared by i2s_play_stop(); producer also checks to skip new blocks. */
static atomic_t play_running;


static K_THREAD_STACK_DEFINE(i2s_play_stack, I2S_PLAY_THREAD_STACK_SIZE);
static struct k_thread i2s_play_thread_data;
static k_tid_t i2s_play_tid;

/* One-time initialisation of the PCM ring (idempotent). */
static bool pcm_ring_ready;

/* Playback statistics reported once per second. */


#define I2S_PLAY_STATS_PERIOD_MS 1000U

static struct {
	uint32_t blocks_written;  /* blocks handed to the SAI */
	uint32_t silence_cnt;     /* silence blocks substituted for missing PCM */
	uint32_t relatch_cnt;     /* PLAYING -> PREBUFFER transitions (underruns) */
	uint32_t drop_cnt;        /* blocks discarded because the ring was full */
	uint32_t alloc_fail_cnt;  /* slab exhausted in the producer */
	uint32_t restart_cnt;     /* SAI restarts after a write error */
	uint32_t depth_min;       /* ring depth watermarks over the period */
	uint32_t depth_max;
	uint32_t report_ms;       /* uptime of the last report */
	uint32_t period_blocks;   /* blocks written during the period */
} play_stats;

static void stats_reset_period(uint32_t now_ms)
{
	play_stats.report_ms = now_ms;
	play_stats.period_blocks = 0U;
	play_stats.depth_min = UINT32_MAX;
	play_stats.depth_max = 0U;
}

static void stats_note_depth(uint32_t depth)
{
	if (depth < play_stats.depth_min) {
		play_stats.depth_min = depth;
	}
	if (depth > play_stats.depth_max) {
		play_stats.depth_max = depth;
	}
}

/*
 * Report once per period. The drift figure compares the audio time actually
 * played out (blocks * frame duration) with the elapsed wall time: a positive
 * value means the SAI is slower than the source, a negative value means it is
 * faster. Sustained non-zero drift is what eventually forces a relatch or a
 * drop, so seeing it here distinguishes a clock problem from a scheduling one.
 */
static void stats_report(uint32_t depth)
{
	uint32_t now_ms = k_uptime_get_32();
	uint32_t elapsed_ms = now_ms - play_stats.report_ms;
	int64_t played_us;
	int64_t elapsed_us;
	int32_t drift_ppm = 0;

	if (elapsed_ms < I2S_PLAY_STATS_PERIOD_MS) {
		return;
	}

	played_us = (int64_t)play_stats.period_blocks * (int64_t)tx_frame_dur_us;
	elapsed_us = (int64_t)elapsed_ms * 1000;

	if (elapsed_us > 0) {
		drift_ppm = (int32_t)(((played_us - elapsed_us) * 1000000) / elapsed_us);
	}

	LOG_INF("ring=%u (min %u max %u) relatch=%u sil=%u drop=%u allocfail=%u "
		"restart=%u blks=%u drift=%d ppm",
		depth, (play_stats.depth_min == UINT32_MAX) ? 0U : play_stats.depth_min,
		play_stats.depth_max, play_stats.relatch_cnt, play_stats.silence_cnt,
		play_stats.drop_cnt, play_stats.alloc_fail_cnt, play_stats.restart_cnt,
		play_stats.blocks_written, drift_ppm);

	stats_reset_period(now_ms);
}

#define STATS_INC(field) (play_stats.field++)
#define STATS_NOTE_DEPTH(d) stats_note_depth(d)
#define STATS_REPORT(d) stats_report(d)
#define STATS_BLOCK_WRITTEN()\
	do {\
		play_stats.blocks_written++;\
		play_stats.period_blocks++;\
	} while (0)
#define STATS_START() stats_reset_period(k_uptime_get_32())

/* Allocate a slab block and fill it with silence (zeros). */
static int alloc_silence_block(void **out, k_timeout_t timeout)
{
	void *blk;
	int ret;

	ret = k_mem_slab_alloc(tx_mem_slab, &blk, timeout);
	if (ret != 0) {
		return ret;
	}

	memset(blk, 0, tx_block_size);
	*out = blk;

	return 0;
}

/* Hand one block to the SAI, recovering the block and the stream on failure. */
static bool play_write_block(void *blk, bool *started)
{
	if (i2s_write(i2s_tx_dev, blk, tx_block_size) != 0) {
		k_mem_slab_free(tx_mem_slab, blk);

		/*
		 * The MCUX SAI driver disables the transmitter and returns to
		 * READY when its queue runs dry, so a failed write means the
		 * stream has to be triggered again rather than just retried.
		 */
		if (i2s_trigger(i2s_tx_dev, I2S_DIR_TX, I2S_TRIGGER_START) != 0) {
			*started = false;
		}
		STATS_INC(restart_cnt);

		return false;
	}

	STATS_BLOCK_WRITTEN();

	return true;
}

static void i2s_play_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Two frame periods: tolerates jitter while still detecting a real stall. */
	const k_timeout_t play_wait = K_USEC(2U * tx_frame_dur_us);
	bool prebuffering = true;
	bool started = false;
	uint32_t preloaded = 0U;

	STATS_START();

	/*
	 * Prime the SAI DMA with silence and start it immediately, so the frame
	 * clock is running before the first real PCM arrives and the stream
	 * does not have to be started from the data path.
	 */
	while (preloaded < I2S_TX_PRELOAD_BLOCKS) {
		void *blk;

		if (alloc_silence_block(&blk, K_MSEC(I2S_TX_TIMEOUT_MS)) != 0) {
			LOG_ERR("Preload alloc failed");
			break;
		}
		if (i2s_write(i2s_tx_dev, blk, tx_block_size) != 0) {
			k_mem_slab_free(tx_mem_slab, blk);
			break;
		}
		preloaded++;
	}

	if (i2s_trigger(i2s_tx_dev, I2S_DIR_TX, I2S_TRIGGER_START) != 0) {
		LOG_ERR("Failed to start I2S tx");
	} else {
		started = true;
		LOG_INF("I2S tx started (prebuffer %u blocks, %u us frames)",
			PCM_PREBUFFER_BLOCKS, tx_frame_dur_us);
	}

	while (atomic_get(&play_running) != 0) {
		uint32_t depth = k_msgq_num_used_get(&pcm_ring);
		void *blk = NULL;

		STATS_NOTE_DEPTH(depth);
		STATS_REPORT(depth);

		if (!started) {
			if (i2s_trigger(i2s_tx_dev, I2S_DIR_TX, I2S_TRIGGER_START) == 0) {
				started = true;
			} else {
				k_sleep(K_MSEC(10));
				continue;
			}
		}

		if (prebuffering) {
			/*
			 * Keep the frame clock fed with silence while the
			 * cushion fills. This is not an underrun: the stream is
			 * deliberately not playing yet.
			 */
			if (depth < PCM_PREBUFFER_BLOCKS) {
				if (alloc_silence_block(&blk, K_MSEC(I2S_TX_TIMEOUT_MS)) != 0) {
					continue;
				}
				STATS_INC(silence_cnt);
				(void)play_write_block(blk, &started);
				continue;
			}

			prebuffering = false;
		}

		if (k_msgq_get(&pcm_ring, &blk, play_wait) != 0) {
			/*
			 * Nothing for two frame periods: the producer really has
			 * stalled. Emit one silence block to keep the SAI clocked
			 * and go back to prebuffering so the cushion is rebuilt
			 * instead of running at the edge of empty.
			 */
			if (alloc_silence_block(&blk, K_MSEC(I2S_TX_TIMEOUT_MS)) != 0) {
				continue;
			}
			STATS_INC(silence_cnt);
			STATS_INC(relatch_cnt);
			prebuffering = true;
		}

		(void)play_write_block(blk, &started);
	}

	/*
	 * Leave the SAI stopped and its queued blocks reclaimed, so the stream
	 * can be reconfigured after this thread exits.
	 */
	(void)i2s_trigger(i2s_tx_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);

	LOG_INF("I2S tx playback stopped");
}

void i2s_play_setup(const struct device *dev, struct k_mem_slab *slab, uint32_t block_size,
		    uint32_t frame_dur_us)
{
	/* Initialise the PCM ring once, lazily, on the first setup. */
	if (!pcm_ring_ready) {
		k_msgq_init(&pcm_ring, (char *)pcm_ring_storage, sizeof(void *), PCM_RING_DEPTH);
		pcm_ring_ready = true;
	}

	i2s_tx_dev = dev;
	tx_mem_slab = slab;
	tx_block_size = block_size;
	tx_frame_dur_us = frame_dur_us;

	/*
	 * Start the playback thread for this stream. i2s_play_stop() joins the
	 * previous one, so at most one thread is ever running.
	 */
	if (i2s_play_tid == NULL) {
		atomic_set(&play_running, 1);
		i2s_play_tid = k_thread_create(&i2s_play_thread_data, i2s_play_stack,
					       K_THREAD_STACK_SIZEOF(i2s_play_stack),
					       i2s_play_thread_fn, NULL, NULL, NULL,
					       I2S_PLAY_THREAD_PRIO, 0, K_NO_WAIT);
		k_thread_name_set(i2s_play_tid, "i2s_play");
	}
}

uint32_t i2s_play_write(const uint8_t *data, uint32_t len)
{
	uint32_t written = 0U;

	if (tx_block_size == 0U) {
		LOG_ERR("I2S tx not configured");
		return 0U;
	}

	if (atomic_get(&play_running) == 0) {
		/* Tearing down; do not queue blocks the consumer will not take. */
		return 0U;
	}

	/*
	 * Producer: split the incoming PCM into block-sized chunks, copy each
	 * into a slab block and enqueue the block pointer into the ring. Never
	 * calls i2s_write(); the playback thread drains the ring. If the pool
	 * or ring is full the oldest queued block is recycled (bounded latency,
	 * overrun protection) so this call never blocks the decode thread.
	 */
	while (written < len) {
		void *mem_block;
		uint32_t chunk = len - written;

		if (chunk > tx_block_size) {
			chunk = tx_block_size;
		}

		if (k_mem_slab_alloc(tx_mem_slab, &mem_block, K_NO_WAIT) != 0) {
			/*
			 * Pool exhausted: reuse the oldest block still waiting
			 * in the ring. Only ring-owned blocks are recycled -
			 * blocks already handed to i2s_write() belong to the
			 * driver until it frees them, so they are never touched
			 * here.
			 */
			void *old;

			STATS_INC(alloc_fail_cnt);

			if (k_msgq_get(&pcm_ring, &old, K_NO_WAIT) == 0) {
				mem_block = old;
				STATS_INC(drop_cnt);
			} else {
				LOG_WRN("No PCM buffer, dropped %u/%u bytes", written, len);
				break;
			}
		}

		memcpy(mem_block, &data[written], chunk);
		if (chunk < tx_block_size) {
			memset((uint8_t *)mem_block + chunk, 0, tx_block_size - chunk);
		}

		if (k_msgq_put(&pcm_ring, &mem_block, K_NO_WAIT) != 0) {
			/* Ring full: drop the oldest block, then retry once. */
			void *old;

			if (k_msgq_get(&pcm_ring, &old, K_NO_WAIT) == 0) {
				k_mem_slab_free(tx_mem_slab, old);
				STATS_INC(drop_cnt);
			}
			if (k_msgq_put(&pcm_ring, &mem_block, K_NO_WAIT) != 0) {
				k_mem_slab_free(tx_mem_slab, mem_block);
				STATS_INC(drop_cnt);
				break;
			}
		}

		written += chunk;
	}

	return written;
}

void i2s_play_stop(void)
{
	void *blk;

	if (i2s_play_tid == NULL) {
		return;
	}

	/*
	 * Ask the playback thread to leave its loop and wait for it. Joining
	 * before draining guarantees the thread no longer holds a block, so
	 * every block can be returned to the slab and the caller may
	 * reconfigure the stream afterwards.
	 */
	atomic_set(&play_running, 0);
	(void)k_thread_join(&i2s_play_thread_data, K_MSEC(I2S_TX_TIMEOUT_MS));
	i2s_play_tid = NULL;

	while (k_msgq_get(&pcm_ring, &blk, K_NO_WAIT) == 0) {
		k_mem_slab_free(tx_mem_slab, blk);
	}

	tx_block_size = 0U;
}
