/**
 * @file
 * @brief BAP Broadcast Sink audio data path built on the mpipe framework.
 *
 * All mpipe related code for this sample lives here and is compiled only when
 * CONFIG_BAP_SINK_AUDIO_PATH_MPIPE is set.
 *
 * Pipeline topology (unchanged from the 1-BIS baseline):
 *
 *   bap_src (frame set) -> lc3_decoder (transform) -> hw_codec_sink (I2S)
 *
 * The source is pull-based: the pipeline thread calls the source buffer pool's
 * acquire_buffer(), which blocks on an internal semaphore fed by
 * mpipe_audio_push() from the ISO receive callbacks. Each acquired buffer holds
 * one mpipe_audio_frame_set carrying one LC3 frame per active channel.
 *
 * Multi-channel operation:
 *   One LC3 decoder and one output channel per synced BIS. ch 0 drives the
 *   source semaphore (max count 1); both BIS callbacks fire within the same
 *   ISO interval so ch 1's frame is ready before the source thread wakes.
 *   A two-frame-period timeout keeps the SAI running when a channel is lost.
 *   All channels are decoded into one interleaved PCM block; with 1 BIS the
 *   output is plain mono.
 *
 * Note on caps: the mpipe framework models only raw PCM audio; every pad
 * negotiates the same PCM format. The actual LC3 decode is in the transform.
 *
 * Copyright (c) 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpipe_audio.h"

#if defined(CONFIG_BAP_SINK_AUDIO_PATH_MPIPE)

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_pipeline.h>
#include <zephyr/mpipe/mpipe_sink.h>
#include <zephyr/mpipe/mpipe_src.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_transform.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/bluetooth/audio/bap.h>

#include <lc3.h>

#include "hw_codec.h"

LOG_MODULE_REGISTER(mpipe_audio, CONFIG_LOG_DEFAULT_LEVEL);

/* Worst-case sizes for static buffer sizing. */
#define MPIPE_AUDIO_MAX_FRAME_DUR_US 10000U  /* longest LC3 frame (7500 or 10000 us) */
#define MPIPE_AUDIO_MAX_LC3_FRAME    256U    /* max encoded frame bytes */
#define MPIPE_AUDIO_MAX_SAMPLES_MONO \
	((MPIPE_AUDIO_MAX_FRAME_DUR_US * 48000U) / USEC_PER_SEC)
#define MPIPE_AUDIO_MAX_CHANNELS CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT
#define MPIPE_AUDIO_FRAME_SET_SZ sizeof(struct mpipe_audio_frame_set)
#define MPIPE_AUDIO_PCM_BUF_SZ \
	(MPIPE_AUDIO_MAX_SAMPLES_MONO * MPIPE_AUDIO_MAX_CHANNELS * sizeof(int16_t))

/* Source pool: one frame-set per period, consumed immediately; 4 in-flight max. */
#define MPIPE_AUDIO_SRC_BUF_COUNT 4
#define MPIPE_AUDIO_PCM_BUF_COUNT 4


/* Unique element identifiers within the pipeline. */
enum {
	MPIPE_AUDIO_PIPE_ID,
	MPIPE_AUDIO_SRC_ID,
	MPIPE_AUDIO_DEC_ID,
	MPIPE_AUDIO_SINK_ID,
};

/* One LC3 frame per channel; newest-wins, no allocation on the RX path. */


struct mpipe_audio_frame_set {
	/*
	 * Encoded LC3 payload for each channel. len[ch] == 0 means no frame
	 * was received for that channel this period; the decoder runs PLC.
	 */
	uint8_t  data[MPIPE_AUDIO_MAX_CHANNELS][MPIPE_AUDIO_MAX_LC3_FRAME];
	uint16_t len[MPIPE_AUDIO_MAX_CHANNELS];
	/* SDU timestamp of the most recent frame in this set. */
	uint32_t ts;
};

/* -------------------------------------------------------------------------
 * net_buf pools
 * -------------------------------------------------------------------------
 */

NET_BUF_POOL_FIXED_DEFINE(mpipe_audio_src_pool, MPIPE_AUDIO_SRC_BUF_COUNT,
			  MPIPE_AUDIO_FRAME_SET_SZ, sizeof(struct mpipe_buffer_meta),
			  mpipe_buffer_destroy);

NET_BUF_POOL_FIXED_DEFINE(mpipe_audio_pcm_pool, MPIPE_AUDIO_PCM_BUF_COUNT,
			  MPIPE_AUDIO_PCM_BUF_SZ, sizeof(struct mpipe_buffer_meta),
			  mpipe_buffer_destroy);

/* -------------------------------------------------------------------------
 * Runtime state
 * -------------------------------------------------------------------------
 */

/* Number of BIS channels configured for this session (1 or 2). */
static uint8_t mpipe_audio_channels = 1U;

/* Output sample rate set by mpipe_audio_init(). */
static uint32_t mpipe_audio_samplerate;

/* LC3 frame duration set by mpipe_audio_init(). */
static uint32_t mpipe_audio_frame_dur_us;

/* Presentation delay recorded by mpipe_audio_set_presentation_delay(). */
static uint32_t mpipe_audio_pd_us = BT_BAP_PD_UNSET;

/* Set to non-zero while tearing down, so the source acquire hook unblocks. */
static atomic_t mpipe_audio_stopping;

/*
 * Started-stream refcount. The pipeline is built when the first stream starts
 * (0 -> 1) and torn down when the last stream stops (1 -> 0). This prevents
 * the first BIS stopping from killing playback while the second is still live.
 */
static atomic_t mpipe_audio_started_streams;

/* -------------------------------------------------------------------------
 * Per-channel ingress state (written from ISO RX callbacks)
 * -------------------------------------------------------------------------
 */

/*
 * Pending frame set: newest-wins staging area written by mpipe_audio_push()
 * and snapshotted by the source acquire hook. A spinlock keeps the snapshot
 * consistent without blocking the ISO callback.
 */
static struct mpipe_audio_frame_set mpipe_audio_pending;
static struct k_spinlock            mpipe_audio_lock;

/*
 * Semaphore given by channel 0 to pace the source once per frame period.
 * Max count is 1: a second k_sem_give() before the source runs is a no-op,
 * which also covers the teardown path that may give it to unblock the source.
 */
static struct k_sem mpipe_audio_frame_sem;

/* LC3 decoder instances (one per channel). */


static lc3_decoder_mem_48k_t mpipe_audio_dec_mem[MPIPE_AUDIO_MAX_CHANNELS];
static lc3_decoder_t         mpipe_audio_dec[MPIPE_AUDIO_MAX_CHANNELS];

/* -------------------------------------------------------------------------
 * Pipeline elements
 * -------------------------------------------------------------------------
 */

static struct mpipe mpipe_audio_pipeline;

struct bap_src {
	struct mpipe_src src;
	struct mpipe_buffer_pool pool;
};

struct lc3_decoder_elem {
	struct mpipe_transform transform;
	struct mpipe_buffer_pool out_pool;
};

static struct bap_src          mpipe_audio_src;
static struct lc3_decoder_elem mpipe_audio_dec_elem;
static struct mpipe_sink       mpipe_audio_sink;

static bool mpipe_audio_built;

/* -------------------------------------------------------------------------
 * Asynchronous work queue
 *
 * Both the bring-up and the teardown of the pipeline block for a significant
 * amount of time (hw_codec_cfg ~1.2 s, mpipe_thread_join K_FOREVER, and
 * i2s_play_stop joining the playback thread with a 2 s timeout). Neither
 * must run on the Bluetooth RX work queue thread: blocking that thread
 * starves the ISO RX buffer pool and stalls HCI command completions, which
 * leads to a controller-unresponsive assert.
 *
 * Both start and stop work items are submitted to this dedicated work queue,
 * which is started eagerly at boot so the submission itself is cheap.
 * -------------------------------------------------------------------------
 */

#define MPIPE_AUDIO_INIT_STACK_SIZE 2048
#define MPIPE_AUDIO_INIT_PRIO       K_PRIO_PREEMPT(10)

static K_THREAD_STACK_DEFINE(mpipe_audio_init_stack, MPIPE_AUDIO_INIT_STACK_SIZE);
static struct k_work_q mpipe_audio_init_workq;
static struct k_work   mpipe_audio_init_work;
static struct k_work   mpipe_audio_stop_work;
static uint32_t        mpipe_audio_init_freq_hz;
static uint32_t        mpipe_audio_init_frame_dur_us;

static void mpipe_audio_init_work_handler(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	err = mpipe_audio_init(mpipe_audio_init_freq_hz, mpipe_audio_init_frame_dur_us);
	if (err == -EALREADY) {
		/* A previous BIS already completed init; just start. */
	} else if (err < 0) {
		LOG_ERR("Cannot init mpipe audio: %d", err);
		/*
		 * Init failed after mpipe_audio_request_start() had already
		 * incremented started_streams. Restore the count so that the
		 * matching mpipe_audio_stream_stopped() call does not underflow
		 * and a future retry can reach prev == 1 again.
		 */
		(void)atomic_dec(&mpipe_audio_started_streams);
		return;
	}

	err = mpipe_audio_start();
	if (err < 0) {
		LOG_ERR("Cannot start mpipe audio: %d", err);
	}
}

static void mpipe_audio_stop_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	mpipe_audio_stop();
}

static int mpipe_audio_workq_init(void)
{
	struct k_work_queue_config cfg = {
		.name = "mpipe_init",
	};

	k_work_queue_start(&mpipe_audio_init_workq, mpipe_audio_init_stack,
			   K_THREAD_STACK_SIZEOF(mpipe_audio_init_stack),
			   MPIPE_AUDIO_INIT_PRIO, &cfg);
	k_work_init(&mpipe_audio_init_work, mpipe_audio_init_work_handler);
	k_work_init(&mpipe_audio_stop_work, mpipe_audio_stop_work_handler);

	return 0;
}

SYS_INIT(mpipe_audio_workq_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/* -------------------------------------------------------------------------
 * Shared PCM capability (source and sink pads report the same format)
 * -------------------------------------------------------------------------
 */

static int mpipe_audio_pcm_enum_caps(struct mpipe_pad *pad, uint32_t index,
				     const struct mpipe_structure *filter,
				     struct mpipe_structure *out)
{
	struct mpipe_structure caps;
	int ret;

	ARG_UNUSED(pad);

	if (index > 0U) {
		return -ENOENT;
	}

	ret = mpipe_structure_init_fields(&caps, MPIPE_MEDIA_AUDIO_PCM,
					  MPIPE_CAPS_SAMPLE_RATE, MPIPE_TYPE_UINT,
					  mpipe_audio_samplerate,
					  MPIPE_CAPS_BITWIDTH, MPIPE_TYPE_UINT, 16,
					  MPIPE_CAPS_NUM_OF_CHANNEL, MPIPE_TYPE_UINT,
					  (uint32_t)mpipe_audio_channels,
					  MPIPE_CAPS_END);
	if (ret != 0) {
		return ret;
	}

	return mpipe_pad_enum_filter(&caps, filter, out);
}

/* -------------------------------------------------------------------------
 * Source element: pulls frame-sets from the ingress slots one per period
 * -------------------------------------------------------------------------
 */

static int bap_src_pool_acquire(struct mpipe_buffer_pool *pool, struct net_buf **buf)
{
	struct net_buf *nb;
	struct mpipe_audio_frame_set *set;
	k_spinlock_key_t key;

	ARG_UNUSED(pool);

	/*
	 * Wait for a complete frame period, signalled once every active
	 * channel has deposited its frame (see mpipe_audio_push). On timeout
	 * (two frame periods) emit whatever is pending so the SAI cadence
	 * never stalls during a loss burst.
	 */

	while (atomic_get(&mpipe_audio_stopping) == 0) {
		(void)k_sem_take(&mpipe_audio_frame_sem,
				 K_USEC(2U * mpipe_audio_frame_dur_us));

		if (atomic_get(&mpipe_audio_stopping) != 0) {
			break;
		}

		nb = net_buf_alloc(&mpipe_audio_src_pool, K_NO_WAIT);
		if (nb == NULL) {
			/* Pool temporarily exhausted; wait for next period. */
			continue;
		}

		/*
		 * Snapshot the pending frame-set under the spinlock. Copy only
		 * the actual encoded data (set->len[ch] bytes per channel) to
		 * minimise the time IRQs are held off. The full data[] slots are
		 * never all valid at once: a typical LC3 frame at 16/24 kHz is
		 * 40-60 bytes, so the copy is far smaller than the worst-case
		 * 256 bytes per channel implied by the struct size.
		 */
		set = (struct mpipe_audio_frame_set *)net_buf_add(nb,
							sizeof(struct mpipe_audio_frame_set));
		key = k_spin_lock(&mpipe_audio_lock);
		set->ts = mpipe_audio_pending.ts;
		for (uint8_t ch = 0U; ch < mpipe_audio_channels; ch++) {
			set->len[ch] = mpipe_audio_pending.len[ch];
			if (set->len[ch] != 0U) {
				memcpy(set->data[ch], mpipe_audio_pending.data[ch],
				       set->len[ch]);
			}
			/* Reset so the next period starts with PLC if no push arrives. */
			mpipe_audio_pending.len[ch] = 0U;
		}
		k_spin_unlock(&mpipe_audio_lock, key);

		mpipe_buffer_get_meta(nb)->pool = &mpipe_audio_src.pool;
		mpipe_buffer_get_meta(nb)->bytes_used = sizeof(struct mpipe_audio_frame_set);
		mpipe_buffer_get_meta(nb)->timestamp = set->ts;


		*buf = nb;
		return 0;
	}

	return -EPIPE;
}

static int bap_src_pool_release(struct mpipe_buffer_pool *pool, struct net_buf *buf)
{
	struct mpipe_buffer_meta *meta = mpipe_buffer_get_meta(buf);

	ARG_UNUSED(pool);

	if (meta != NULL) {
		meta->bytes_used = 0;
		meta->timestamp = 0;
		meta->driver_buf = NULL;
		meta->priv = NULL;
	}
	buf->len = 0;

	return 0;
}

static int bap_src_init(struct bap_src *self, uint8_t id)
{
	const struct mpipe_buffer_pool_config pool_req = {
		.size = MPIPE_AUDIO_FRAME_SET_SZ,
		.min_buffers = 1,
	};
	int ret;

	ret = mpipe_src_init(&self->src, id);
	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(&self->src.element, "bap_src");

	mpipe_buffer_pool_init(&self->pool);
	self->pool.nb_pool = &mpipe_audio_src_pool;
	(void)mpipe_buffer_pool_set_req_config(&self->pool, &pool_req);
	self->pool.acquire_buffer = bap_src_pool_acquire;
	self->pool.release_buffer = bap_src_pool_release;

	self->src.pool = &self->pool;
	/* Run until stopped (num_buffers == 0). */
	(void)mpipe_object_set_properties(&self->src.element.object,
					  MPIPE_PROP_SRC_NUM_BUFS,
					  (const void *)(uintptr_t)0,
					  MPIPE_PROP_LIST_END);
	self->src.src_pad.enum_caps_fn = mpipe_audio_pcm_enum_caps;

	return 0;
}

/* -------------------------------------------------------------------------
 * Transform: LC3 decode, interleaved output
 * -------------------------------------------------------------------------
 */

static int lc3_dec_pool_acquire(struct mpipe_buffer_pool *pool, struct net_buf **buf)
{
	struct net_buf *nb;
	struct mpipe_buffer_meta *meta;

	if (pool->nb_pool == NULL) {
		return -EINVAL;
	}

	nb = net_buf_alloc_len(pool->nb_pool, pool->config.size, K_NO_WAIT);
	if (nb == NULL) {
		return -ENOBUFS;
	}

	meta = mpipe_buffer_get_meta(nb);
	meta->pool = pool;
	meta->bytes_used = 0;
	meta->timestamp = k_uptime_get_32();

	*buf = nb;

	return 0;
}

static int lc3_dec_pool_release(struct mpipe_buffer_pool *pool, struct net_buf *buf)
{
	struct mpipe_buffer_meta *meta = mpipe_buffer_get_meta(buf);

	ARG_UNUSED(pool);

	if (meta != NULL) {
		meta->bytes_used = 0;
		meta->timestamp = 0;
		meta->driver_buf = NULL;
		meta->priv = NULL;
	}
	buf->len = 0;

	return 0;
}

static int lc3_dec_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
			    struct net_buf **out_buf)
{
	struct mpipe_transform *transform =
		(struct mpipe_transform *)pad->object.container;
	const struct mpipe_audio_frame_set *set =
		(const struct mpipe_audio_frame_set *)in_buf->data;
	struct net_buf *out;
	int16_t *pcm;
	int samples;
	uint32_t pcm_bytes;

	*out_buf = NULL;

	samples = lc3_frame_samples(mpipe_audio_frame_dur_us, mpipe_audio_samplerate);
	if (samples < 0) {
		net_buf_unref(in_buf);
		return -EINVAL;
	}

	if (transform->out_pool->acquire_buffer(transform->out_pool, &out) != 0) {
		LOG_ERR("Failed to acquire PCM buffer");
		net_buf_unref(in_buf);
		return -ENOBUFS;
	}

	pcm_bytes = (uint32_t)samples * mpipe_audio_channels * sizeof(int16_t);
	pcm = (int16_t *)net_buf_add(out, pcm_bytes);

	/*
	 * Decode each active channel into interleaved positions.
	 *
	 * The lc3_decode() stride parameter is the count between two
	 * consecutive samples for the same channel. With stride equal to the
	 * channel count the decoder writes sample[i] to pcm[ch + i*channels],
	 * producing L0 R0 L1 R1 ... which is the standard interleaved layout.
	 *
	 * With channels == 1 the stride is 1 and the output is plain mono,
	 * identical to the original single-BIS behaviour.
	 *
	 * A len[ch] of 0 means no frame arrived for that channel this period;
	 * passing NULL to lc3_decode() runs Packet Loss Concealment so the
	 * decoder state advances correctly without a gap in the output.
	 */
	for (uint8_t ch = 0U; ch < mpipe_audio_channels; ch++) {
		const uint8_t *in = (set->len[ch] != 0U) ? set->data[ch] : NULL;

		if (mpipe_audio_dec[ch] == NULL) {
			/* Decoder not initialised; fill with silence. */
			for (int i = 0; i < samples; i++) {
				pcm[ch + i * mpipe_audio_channels] = 0;
			}
			continue;
		}

		(void)lc3_decode(mpipe_audio_dec[ch], in, (int)set->len[ch],
				 LC3_PCM_FORMAT_S16, &pcm[ch],
				 (int)mpipe_audio_channels);
	}

	mpipe_buffer_get_meta(out)->bytes_used = pcm_bytes;

	net_buf_unref(in_buf);
	*out_buf = out;

	return 0;
}

static int lc3_dec_init(struct lc3_decoder_elem *self, uint8_t id)
{
	const struct mpipe_buffer_pool_config pool_req = {
		.size = MPIPE_AUDIO_PCM_BUF_SZ,
		.min_buffers = 1,
	};
	int ret;

	ret = mpipe_transform_init(&self->transform, id);
	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(&self->transform.element, "lc3_decoder");

	mpipe_buffer_pool_init(&self->out_pool);
	self->out_pool.nb_pool = &mpipe_audio_pcm_pool;
	(void)mpipe_buffer_pool_set_req_config(&self->out_pool, &pool_req);
	self->out_pool.acquire_buffer = lc3_dec_pool_acquire;
	self->out_pool.release_buffer = lc3_dec_pool_release;

	self->transform.mode = MPIPE_MODE_NORMAL;
	self->transform.out_pool = &self->out_pool;
	self->transform.sink_pad.chain_fn = lc3_dec_chain_fn;

	return 0;
}

/* -------------------------------------------------------------------------
 * Sink: writes PCM to the hardware codec over I2S
 * -------------------------------------------------------------------------
 */

static int hw_codec_sink_set_caps(struct mpipe_sink *sink, const struct mpipe_structure *caps)
{
	/*
	 * Propagate the negotiated caps into the pad. The actual hardware codec
	 * configuration (hw_codec_cfg) is done once in mpipe_audio_init() before
	 * the pipeline starts playing, to avoid the blocking power-up sequence
	 * running while ISO data is already arriving.
	 */
	return mpipe_pad_set_caps(&sink->sink_pad, caps);
}

static int hw_codec_sink_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				  struct net_buf **out_buf)
{
	struct mpipe_buffer_meta *meta = mpipe_buffer_get_meta(in_buf);
	uint32_t len = (meta != NULL && meta->bytes_used != 0U) ?
		       meta->bytes_used : in_buf->len;

	ARG_UNUSED(pad);

	(void)hw_codec_write_data(in_buf->data, len);

	net_buf_unref(in_buf);
	*out_buf = NULL;

	return 0;
}

static int hw_codec_sink_init(struct mpipe_sink *self, uint8_t id)
{
	int ret;

	ret = mpipe_sink_init(self, id);
	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(&self->element, "hw_codec_sink");

	self->set_caps = hw_codec_sink_set_caps;
	self->sink_pad.chain_fn = hw_codec_sink_chain_fn;
	self->sink_pad.enum_caps_fn = mpipe_audio_pcm_enum_caps;

	return 0;
}

/* -------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------
 */

void mpipe_audio_set_channels(uint8_t channels)
{
	if (channels == 0U || channels > MPIPE_AUDIO_MAX_CHANNELS) {
		LOG_ERR("Unsupported channel count %u (max %u)", channels,
			MPIPE_AUDIO_MAX_CHANNELS);
		return;
	}

	/*
	 * Render mono only: a single BIS (channel 0) drives the pipeline and
	 * any further channels are dropped at the mpipe_audio_push() entry
	 * (ch >= mpipe_audio_channels), the same way the non-mpipe codec path
	 * in lc3.c forwards only channel 0. Two-channel rendering needs both
	 * BIS to arrive in lockstep every frame period; in practice their
	 * independent ISO arrival jitter and per-channel packet loss starve
	 * the SAI ring (drop/restart bursts and TX-queue-empty), so a single
	 * clock source is used to keep a steady one-block-per-frame cadence.
	 */
	if (channels > 1U) {
		LOG_INF("mpipe audio: %u channel(s) offered, rendering mono (channel 0)",
			channels);
		channels = 1U;
	}

	/*
	 * Always record the requested channel count. Calling this while the
	 * pipeline is still running (e.g. the previous session's async stop
	 * work has not yet executed) is safe: mpipe_audio_init() reads the
	 * value when the start work item runs, which is always submitted after
	 * the stop work item on the same FIFO work queue, so the latest count
	 * is always in place before init() sees it.
	 */
	mpipe_audio_channels = channels;
	LOG_INF("mpipe audio: %u channel(s) requested", channels);
}


int mpipe_audio_init(uint32_t samplerate, uint32_t frame_dur_us)
{
	int ret;

	if (mpipe_audio_built) {
		return -EALREADY;
	}

	if (frame_dur_us == 0U || frame_dur_us > MPIPE_AUDIO_MAX_FRAME_DUR_US) {
		LOG_ERR("Unsupported LC3 frame duration %u us", frame_dur_us);
		return -EINVAL;
	}

	mpipe_audio_samplerate = samplerate;
	mpipe_audio_frame_dur_us = frame_dur_us;
	atomic_set(&mpipe_audio_stopping, 0);

	/* Initialise the semaphore that paces the source (max count 1). */
	k_sem_init(&mpipe_audio_frame_sem, 0, 1);

	/* Reset the pending frame-set. */
	memset(&mpipe_audio_pending, 0, sizeof(mpipe_audio_pending));

	/* Set up one LC3 decoder per active channel. */
	for (uint8_t ch = 0U; ch < mpipe_audio_channels; ch++) {
		mpipe_audio_dec[ch] = lc3_setup_decoder(frame_dur_us, samplerate, 0,
							 &mpipe_audio_dec_mem[ch]);
		if (mpipe_audio_dec[ch] == NULL) {
			LOG_ERR("Failed to setup LC3 decoder for channel %u", ch);
			ret = -EINVAL;
			goto err;
		}
	}
	/* Clear any channels beyond the active count. */
	for (uint8_t ch = mpipe_audio_channels; ch < MPIPE_AUDIO_MAX_CHANNELS; ch++) {
		mpipe_audio_dec[ch] = NULL;
	}

	/*
	 * Configure the hardware codec before starting the pipeline so the
	 * blocking power-up stabilization runs before ISO data arrives. The
	 * channel count from mpipe_audio_set_channels() drives the I2S block
	 * size; one channel per BIS stream keeps the SAI rate balanced.
	 */
	ret = hw_codec_cfg(samplerate, frame_dur_us, mpipe_audio_channels);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed to configure hw codec (%d)", ret);
		goto err;
	}

	ret = mpipe_pipeline_init(&mpipe_audio_pipeline, MPIPE_AUDIO_PIPE_ID);
	if (ret != 0) {
		goto err;
	}

	ret = bap_src_init(&mpipe_audio_src, MPIPE_AUDIO_SRC_ID);
	if (ret != 0) {
		goto err;
	}

	ret = lc3_dec_init(&mpipe_audio_dec_elem, MPIPE_AUDIO_DEC_ID);
	if (ret != 0) {
		goto err;
	}

	ret = hw_codec_sink_init(&mpipe_audio_sink, MPIPE_AUDIO_SINK_ID);
	if (ret != 0) {
		goto err;
	}

	ret = mpipe_bin_add((struct mpipe_bin *)&mpipe_audio_pipeline,
			    (struct mpipe_element *)&mpipe_audio_src,
			    (struct mpipe_element *)&mpipe_audio_dec_elem,
			    (struct mpipe_element *)&mpipe_audio_sink, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to add elements to pipeline (%d)", ret);
		goto err;
	}

	ret = mpipe_element_link((struct mpipe_element *)&mpipe_audio_src,
				 (struct mpipe_element *)&mpipe_audio_dec_elem,
				 (struct mpipe_element *)&mpipe_audio_sink, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to link elements (%d)", ret);
		goto err;
	}

	if (mpipe_element_set_state((struct mpipe_element *)&mpipe_audio_pipeline,
				    MPIPE_STATE_PAUSED) != 0) {
		LOG_ERR("Failed to move pipeline to PAUSED");
		ret = -EIO;
		goto err;
	}

	mpipe_audio_built = true;
	LOG_INF("mpipe audio pipeline built: %u Hz, %u us frames, "
		"%u channel(s), pd %u us",
		samplerate, frame_dur_us, mpipe_audio_channels, mpipe_audio_pd_us);

	return 0;

err:
	/*
	 * Undo whatever was set up before the failure so that a subsequent
	 * attempt starts from a clean slate. A partially-configured codec
	 * would make hw_codec_cfg() return -EALREADY on the next try, and
	 * stale decoder pointers would cause the transform to use freed
	 * memory after a restart.
	 */
	for (uint8_t ch = 0U; ch < MPIPE_AUDIO_MAX_CHANNELS; ch++) {
		mpipe_audio_dec[ch] = NULL;
	}
	(void)hw_codec_close();

	return ret;
}

void mpipe_audio_request_start(uint32_t samplerate, uint32_t frame_dur_us)
{
	/*
	 * Increment the started-stream count before submitting the work item
	 * so the count is always accurate when the stop work handler checks it.
	 * Submitting for the second BIS is a no-op (the work item is already
	 * queued or running), but the count increment is still needed.
	 */
	(void)atomic_inc(&mpipe_audio_started_streams);

	mpipe_audio_init_freq_hz = samplerate;
	mpipe_audio_init_frame_dur_us = frame_dur_us;
	(void)k_work_submit_to_queue(&mpipe_audio_init_workq, &mpipe_audio_init_work);
}

int mpipe_audio_start(void)
{
	if (!mpipe_audio_built) {
		return -EINVAL;
	}

	if (mpipe_element_set_state((struct mpipe_element *)&mpipe_audio_pipeline,
				    MPIPE_STATE_PLAYING) != 0) {
		LOG_ERR("Failed to move pipeline to PLAYING");
		return -EIO;
	}

	return 0;
}

void mpipe_audio_set_presentation_delay(uint32_t pd_us)
{
	mpipe_audio_pd_us = pd_us;
}

void mpipe_audio_push(uint8_t ch, const uint8_t *data, size_t len,
		      uint32_t ts, bool ts_valid)
{
	k_spinlock_key_t key;

	if (!mpipe_audio_built || atomic_get(&mpipe_audio_stopping) != 0) {
		return;
	}

	if (ch >= mpipe_audio_channels) {
		/* Channel index beyond what is configured; discard silently. */
		return;
	}

	/*
	 * Copy the frame into the pending slot for this channel. Newest-wins:
	 * if the pipeline thread has not consumed the previous frame yet, it
	 * is simply overwritten. No allocation, no queuing, no drop counter.
	 */
	key = k_spin_lock(&mpipe_audio_lock);

	if (data != NULL && len > 0U && len <= MPIPE_AUDIO_MAX_LC3_FRAME) {
		memcpy(mpipe_audio_pending.data[ch], data, len);
		mpipe_audio_pending.len[ch] = (uint16_t)len;
	} else {
		/* NULL data or zero length: flag as PLC for this channel. */
		mpipe_audio_pending.len[ch] = 0U;
	}

	if (ts_valid) {
		mpipe_audio_pending.ts = ts;
	}

	k_spin_unlock(&mpipe_audio_lock, key);

	/*
	 * Only ch 0 gives the semaphore. Both BIS share the same anchor point
	 * so ch 1 has normally already deposited its frame before the source
	 * thread wakes. Signalling on every channel would double the production
	 * rate; requiring both would stall on lossy links. A two-period timeout
	 * covers the case where ch 0 is genuinely lost.
	 */
	if (ch == 0U) {

		k_sem_give(&mpipe_audio_frame_sem);
	}
}

void mpipe_audio_push_lost(uint8_t ch)
{
	/*
	 * A lost or errored SDU is signalled as a zero-length push. The
	 * source acquire hook sees len[ch] == 0 and passes NULL to lc3_decode,

	 * which runs Packet Loss Concealment so the decoder state stays valid.
	 */
	mpipe_audio_push(ch, NULL, 0U, 0U, false);
}

void mpipe_audio_stream_stopped(void)
{
	atomic_val_t prev = atomic_dec(&mpipe_audio_started_streams);

	if (prev <= 0) {
		/* Should not happen; reset to a sane state. */
		atomic_set(&mpipe_audio_started_streams, 0);
		return;
	}

	if (prev == 1) {
		/*
		 * Last stream stopped. Signal the source to exit immediately so
		 * the pipeline thread unblocks as soon as the work item runs,
		 * then offload the blocking teardown (mpipe_thread_join, codec
		 * close, I2S playback thread join) to the dedicated work queue
		 * so this BT RX work queue callback returns promptly.
		 */
		atomic_set(&mpipe_audio_stopping, 1);
		k_sem_give(&mpipe_audio_frame_sem);
		(void)k_work_submit_to_queue(&mpipe_audio_init_workq,
					     &mpipe_audio_stop_work);
	}
}

void mpipe_audio_stop(void)
{
	if (!mpipe_audio_built) {
		return;
	}

	/* Signal the source acquire hook to exit its wait loop. */
	atomic_set(&mpipe_audio_stopping, 1);
	/* Unblock the semaphore so the pipeline thread wakes and sees the flag. */
	k_sem_give(&mpipe_audio_frame_sem);

	(void)mpipe_element_set_state((struct mpipe_element *)&mpipe_audio_pipeline,
				      MPIPE_STATE_READY);

	/*
	 * Close the hardware codec. mpipe owns the codec for its entire
	 * session, so the codec is closed here rather than in the per-stream
	 * stopped callbacks, which fire once per BIS and would otherwise close
	 * the codec while the second BIS is still streaming.
	 */
	(void)hw_codec_close();

	atomic_set(&mpipe_audio_stopping, 0);
	/*
	 * Do NOT reset started_streams here. The BT callbacks may already have
	 * incremented it for the next session while this (async) teardown was
	 * in flight. The counter is managed exclusively by
	 * mpipe_audio_stream_started() and mpipe_audio_stream_stopped().
	 */
	mpipe_audio_built = false;
	mpipe_audio_pd_us = BT_BAP_PD_UNSET;

	for (uint8_t ch = 0U; ch < MPIPE_AUDIO_MAX_CHANNELS; ch++) {
		mpipe_audio_dec[ch] = NULL;
	}

	LOG_INF("mpipe audio pipeline stopped");
}

#endif /* CONFIG_BAP_SINK_AUDIO_PATH_MPIPE */
