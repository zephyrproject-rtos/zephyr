/*
 * SPDX-FileCopyrightText: Copyright 2026 Ezurio LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LE Audio playback pump.
 *
 * Takes received LC3 SDUs off the Bluetooth RX context and hands them to a
 * worker thread that decodes them (lc3_decode.c) and writes the resulting PCM
 * to a transport-agnostic sink (sample_bt_audio_pcm_sink.h, backed by
 * pcm_sink_i2s.c). Keeping the decode and output stages behind those two
 * interfaces lets either be reused independently, e.g. a USB sink in place of
 * I2S.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <sample_bt_audio_lc3_decode.h>
#include <sample_bt_audio_pcm_sink.h>
#include <sample_bt_audio_playback.h>

LOG_MODULE_REGISTER(sample_bt_audio_playback, CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_LOG_LEVEL);

/* Depth of the BAP RX -> decode worker SDU handoff queue. */
#define SDU_QUEUE_DEPTH ((uint32_t)CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_PCM_QUEUE_DEPTH)

/* Compressed-SDU handoff between stream_recv() (BAP RX context) and the decode/sink
 * worker. Decoding LC3 is CPU-heavy, so it is deferred off the BT RX context: recv()
 * only takes a reference on the SDU and enqueues it, and the worker decodes it. See the
 * BAP Broadcast Sink sample (samples/bluetooth/audio/bap_broadcast_sink) for the same
 * offload pattern.
 */
struct sdu_item {
	void *fifo_reserved; /* first word reserved for the k_fifo */
	struct net_buf *buf;
	bool do_plc; /* decode as packet-loss concealment (invalid or wrong-size SDU) */
};

static K_FIFO_DEFINE(sdu_fifo);
K_MEM_SLAB_DEFINE_STATIC_TYPE(sdu_slab, struct sdu_item, SDU_QUEUE_DEPTH);

/* The worker decodes each LC3 frame block into an
 * int16_t[SAMPLE_BT_AUDIO_PCM_SINK_MAX_SAMPLES * SAMPLE_BT_AUDIO_PCM_SINK_MAX_CHAN]
 * buffer and writes it to the sink, so the worst-case decoded block must match the
 * sink block size. The runtime pcm_block_bytes bound in bringup only rejects remote
 * configs whose derived block would exceed this worst-case maximum.
 */
BUILD_ASSERT(sizeof(int16_t[SAMPLE_BT_AUDIO_PCM_SINK_MAX_SAMPLES *
			    SAMPLE_BT_AUDIO_PCM_SINK_MAX_CHAN]) ==
		     SAMPLE_BT_AUDIO_PCM_SINK_MAX_BLOCK_BYTES,
	     "PCM block size must match the worst-case decoded block");

/* Shared state: active_cfg, samples_per_frame and pcm_block_bytes are mutated only by
 * bringup_work_handler and only while running == 0. Consumers (stream_recv, the worker)
 * atomic_get(&running) before reading any of the cfg fields; the atomic_set release on
 * the producer publishes cfg to the consumers via matching acquire on the reader.
 */
static struct sample_bt_audio_pcm_cfg active_cfg;
static uint32_t samples_per_frame; /* per-channel samples per LC3 frame */
static uint32_t pcm_block_bytes;   /* per-SDU decoded PCM size */
static atomic_t running;

static K_THREAD_STACK_DEFINE(playback_worker_stack,
			     CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_WORKER_STACK_SIZE);
static struct k_thread playback_worker_thread;
static k_tid_t playback_worker_tid;

/* The worker is preemptible. The lowest preemptible priority is reserved for the idle
 * thread, so K_PRIO_PREEMPT() accepts application values only up to
 * CONFIG_NUM_PREEMPT_PRIORITIES - 1; reject the reserved endpoint at build time.
 */
BUILD_ASSERT(CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_WORKER_PRIORITY < CONFIG_NUM_PREEMPT_PRIORITIES,
	     "WORKER_PRIORITY must be a valid preemptible application priority");

/* Deferred bring-up and teardown: sample_bt_audio_playback_start() and
 * sample_bt_audio_playback_stop() both run in the BAP RX workqueue context, reached
 * synchronously from ASCS control-point handling. Bring-up does codec control-bus
 * transactions and mem_slab reinit; teardown waits for a blocked I2S write and a DMA
 * drain. Doing either inline stalls the Bluetooth RX workqueue and delays all host
 * processing. Both are therefore deferred onto the system workqueue, where they run
 * serialised (one queue, FIFO) so a bring-up and a teardown never overlap. pending_lock
 * serialises the producers (start/stop on the BT RX WQ) and the consumers
 * (bringup_work_handler / teardown_work_handler on the sysworkq) against torn struct
 * reads. active_intent tracks whether the most recent start/stop call wants playback
 * running; both handlers recheck it before touching hardware so a call that raced a
 * queued handler of the other kind cannot resurrect a stopped stream or tear down a
 * restarted one.
 */
static struct k_work bringup_work;
static struct k_work teardown_work;
static struct k_mutex pending_lock;
static struct sample_bt_audio_pcm_cfg pending_cfg;
static bool pending_valid;
static bool active_intent;

/* Wakes the worker after stream_stop parked it on the sem. */
static K_SEM_DEFINE(resume_sem, 0, 1);
/* Given by the worker once it has parked on resume_sem, so the deferred teardown can wait
 * for the worker to be done touching the sink before issuing the drain.
 */
static K_SEM_DEFINE(worker_parked_sem, 0, 1);

static void bringup_work_handler(struct k_work *work);
static void teardown_work_handler(struct k_work *work);

/* Free every SDU still queued for the worker, releasing the ISO RX buffer each holds.
 * The worker is either parked or is itself the caller (running == 0), so there is no
 * concurrent consumer. Callers that race stream_recv() (teardown_work_handler,
 * worker_handle_fault) hold pending_lock and clear running under it first, so no producer
 * can enqueue a fresh SDU after the drain.
 */
static void drain_sdu_fifo(void)
{
	struct sdu_item *item;

	while ((item = k_fifo_get(&sdu_fifo, K_NO_WAIT)) != NULL) {
		net_buf_unref(item->buf);
		k_mem_slab_free(&sdu_slab, item);
	}
}

/* Tear down after a transport fault, running on the worker itself. stop() cannot be reused
 * here because it waits for the worker to park, which would deadlock the worker on itself.
 * Clearing running parks the worker via the loop below; draining the queue and stopping the
 * sink here mean a later stop() correctly early-returns instead of leaving the codec/I2S
 * active with queued SDUs still holding net_buf references. The next start() performs a
 * fresh bring-up and recovers the transport.
 */
static void worker_handle_fault(void)
{
	/* Clear running and drain under pending_lock so a concurrent stream_recv() cannot
	 * enqueue an SDU after the drain (which would leak its net_buf reference).
	 */
	k_mutex_lock(&pending_lock, K_FOREVER);
	atomic_set(&running, 0);
	drain_sdu_fifo();
	k_mutex_unlock(&pending_lock);

	sample_bt_audio_pcm_sink_stop();
}

/* Write one worst-case-sized block to the sink. A negative return means the transport has
 * faulted (typically an I2S DMA underrun); retrying immediately would spin at 100% CPU and
 * flood the log while starving the Bluetooth host, so stop playback. The next start()
 * recovers the transport.
 */
static int worker_write(void *block)
{
	int ret = sample_bt_audio_pcm_sink_write(block, pcm_block_bytes);

	if (ret < 0) {
		LOG_ERR("sink write failed: %d; stopping playback", ret);
		worker_handle_fault();
	}

	return ret;
}

static void playback_worker(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	int ret;

	/* Static: sized to the worst-case block. Keeping these off the thread stack
	 * avoids overflowing playback_worker_stack into adjacent .bss.
	 */
	static int16_t pcm[SAMPLE_BT_AUDIO_PCM_SINK_MAX_SAMPLES *
			   SAMPLE_BT_AUDIO_PCM_SINK_MAX_CHAN];
	static uint8_t silence[SAMPLE_BT_AUDIO_PCM_SINK_MAX_BLOCK_BYTES];

	while (true) {
		struct sdu_item *item;

		if (atomic_get(&running) == 0) {
			/* Signal sample_bt_audio_playback_stop() that we are done touching the
			 * sink so it can safely drain.
			 */
			k_sem_give(&worker_parked_sem);
			ret = k_sem_take(&resume_sem, K_FOREVER);

			__ASSERT_NO_MSG(ret == 0);
			continue;
		}

		/* Decoding LC3 is CPU-heavy, so it runs here rather than in the BAP RX
		 * context. pcm_sink_write() paces the loop by blocking until the transport
		 * frees a block; on SDU gaps we push a silence block so the codec keeps
		 * seeing valid frames.
		 */
		item = k_fifo_get(&sdu_fifo, K_NO_WAIT);
		if (item == NULL) {
			(void)memset(silence, 0, pcm_block_bytes);
			(void)worker_write(silence);
			continue;
		}

		for (uint8_t blk = 0U; blk < active_cfg.frame_blocks_per_sdu; blk++) {
			if (!sample_bt_audio_lc3_decode_block(
				    item->buf, active_cfg.octets_per_frame,
				    active_cfg.chan_cnt, samples_per_frame,
				    item->do_plc, pcm)) {
				break;
			}
			if (worker_write(pcm) < 0) {
				break;
			}
		}

		net_buf_unref(item->buf);
		k_mem_slab_free(&sdu_slab, item);
	}
}

int sample_bt_audio_playback_init(void)
{
	int ret;

	ret = sample_bt_audio_pcm_sink_init();
	if (ret != 0) {
		return ret;
	}

	k_mutex_init(&pending_lock);
	k_work_init(&bringup_work, bringup_work_handler);
	k_work_init(&teardown_work, teardown_work_handler);

	playback_worker_tid = k_thread_create(
		&playback_worker_thread, playback_worker_stack,
		K_THREAD_STACK_SIZEOF(playback_worker_stack), playback_worker, NULL, NULL, NULL,
		K_PRIO_PREEMPT(CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_WORKER_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(playback_worker_tid, "sample_bt_audio_playback");

	return 0;
}

int sample_bt_audio_playback_start(const struct sample_bt_audio_pcm_cfg *cfg)
{
	int ret;

	if (cfg->chan_cnt == 0U || cfg->chan_cnt > SAMPLE_BT_AUDIO_PCM_SINK_MAX_CHAN) {
		LOG_ERR("Unsupported chan_cnt %u", cfg->chan_cnt);
		return -EINVAL;
	}
	if (cfg->frame_blocks_per_sdu == 0U) {
		LOG_ERR("Unsupported frame_blocks_per_sdu %u", cfg->frame_blocks_per_sdu);
		return -EINVAL;
	}

	k_mutex_lock(&pending_lock, K_FOREVER);
	pending_cfg = *cfg;
	pending_valid = true;
	active_intent = true;
	k_mutex_unlock(&pending_lock);

	ret = k_work_submit(&bringup_work);
	if (ret < 0) {
		LOG_ERR("k_work_submit failed: %d", ret);
		return ret;
	}

	return 0;
}

static void bringup_work_handler(struct k_work *work)
{
	struct sample_bt_audio_pcm_cfg local_cfg;
	bool valid;

	ARG_UNUSED(work);

	k_mutex_lock(&pending_lock, K_FOREVER);
	valid = pending_valid;
	if (valid) {
		local_cfg = pending_cfg;
		pending_valid = false;
	}
	/* A stop() may have raced this queued start before we got scheduled;
	 * bail out without touching hardware so we don't resurrect a session
	 * the caller already asked us to stop.
	 */
	valid &= active_intent;
	k_mutex_unlock(&pending_lock);

	if (!valid) {
		return;
	}

	if (atomic_get(&running) != 0) {
		return;
	}

	const struct sample_bt_audio_pcm_cfg *cfg = &local_cfg;

	samples_per_frame = (uint32_t)(((uint64_t)cfg->freq_hz * (uint64_t)cfg->frame_duration_us) /
				       USEC_PER_SEC);
	pcm_block_bytes = samples_per_frame * SAMPLE_BT_AUDIO_PCM_SINK_MAX_CHAN * sizeof(int16_t);
	if (pcm_block_bytes > SAMPLE_BT_AUDIO_PCM_SINK_MAX_BLOCK_BYTES) {
		LOG_ERR("pcm_block_bytes %u exceeds max %u", pcm_block_bytes,
			(unsigned int)SAMPLE_BT_AUDIO_PCM_SINK_MAX_BLOCK_BYTES);
		return;
	}

	active_cfg = *cfg;

	if (sample_bt_audio_lc3_decode_setup(cfg->freq_hz, cfg->frame_duration_us,
					     cfg->chan_cnt) != 0) {
		return;
	}

	drain_sdu_fifo();

	const struct sample_bt_audio_pcm_sink_cfg sink_cfg = {
		.sample_rate_hz = cfg->freq_hz,
		.frame_duration_us = cfg->frame_duration_us,
		.channels = SAMPLE_BT_AUDIO_PCM_SINK_MAX_CHAN,
		.bits = 16U,
		.block_bytes = pcm_block_bytes,
	};

	if (sample_bt_audio_pcm_sink_configure(&sink_cfg) != 0) {
		return;
	}

	if (sample_bt_audio_pcm_sink_start() != 0) {
		return;
	}

	/* Re-check intent: a stop() could have arrived while we were doing the
	 * (blocking) sink bring-up above. If so, undo what we just started
	 * instead of publishing running=1 for an already-stopped session.
	 */
	k_mutex_lock(&pending_lock, K_FOREVER);
	const bool still_wanted = active_intent;

	k_mutex_unlock(&pending_lock);
	if (!still_wanted) {
		sample_bt_audio_pcm_sink_stop();
		return;
	}
	/* Publish cfg to consumers: atomic_set release pairs with atomic_get
	 * acquire in stream_recv() / the worker.
	 */
	atomic_set(&running, 1);
	k_sem_give(&resume_sem);

	LOG_INF("%u Hz, %u ch, %u us frame, %u B/frame", cfg->freq_hz, cfg->chan_cnt,
		cfg->frame_duration_us, cfg->octets_per_frame);
}

void sample_bt_audio_playback_stop(void)
{
	int ret;

	k_mutex_lock(&pending_lock, K_FOREVER);
	active_intent = false;
	k_mutex_unlock(&pending_lock);

	/* Defer the blocking teardown (worker park + DMA drain) onto the system workqueue
	 * so the ASCS/BAP callback returns immediately instead of stalling the Bluetooth
	 * RX workqueue. Runs on the same queue as bringup_work, so the two never overlap.
	 */
	ret = k_work_submit(&teardown_work);
	if (ret < 0) {
		LOG_ERR("k_work_submit failed: %d", ret);
	}
}

static void teardown_work_handler(struct k_work *work)
{
	int ret;
	bool want_teardown;

	ARG_UNUSED(work);

	k_mutex_lock(&pending_lock, K_FOREVER);
	want_teardown = !active_intent;
	k_mutex_unlock(&pending_lock);

	/* A start() re-armed the stream after this teardown was queued; leave it running. */
	if (!want_teardown) {
		return;
	}

	if (atomic_get(&running) == 0) {
		return;
	}

	/* Discard any stale park signal from a previous stop before flipping running,
	 * so the wait below only completes once the worker has actually parked for
	 * *this* stop and stopped touching the sink.
	 */
	k_sem_reset(&worker_parked_sem);

	/* Publish the stop under pending_lock so a stream_recv() that has already observed
	 * running != 0 finishes its enqueue before the drain below, and none can enqueue
	 * afterwards. The park wait is deliberately kept outside the lock so the BT RX
	 * context is never blocked on it.
	 */
	k_mutex_lock(&pending_lock, K_FOREVER);
	atomic_set(&running, 0);
	k_mutex_unlock(&pending_lock);

	ret = k_sem_take(&worker_parked_sem, K_FOREVER);
	__ASSERT_NO_MSG(ret == 0);

	/* The worker has parked; release any SDUs it never got to so their ISO RX
	 * buffers return to the stack. Hold pending_lock so a concurrent stream_recv()
	 * cannot slip an SDU past the drain.
	 */
	k_mutex_lock(&pending_lock, K_FOREVER);
	drain_sdu_fifo();
	k_mutex_unlock(&pending_lock);

	sample_bt_audio_pcm_sink_stop();
}

void sample_bt_audio_playback_recv(const struct bt_iso_recv_info *info, struct net_buf *buf)
{
	struct sdu_item *item;

	if (!sample_bt_audio_lc3_decode_is_ready()) {
		return;
	}

	/* Serialise the running check and the enqueue with the teardown/fault drain via
	 * pending_lock: without it a recv that passed the running check could be preempted,
	 * let a teardown park the worker and drain the fifo, then enqueue an SDU that no
	 * consumer would ever free - leaking its net_buf reference and queueing stale audio.
	 */
	k_mutex_lock(&pending_lock, K_FOREVER);

	if (atomic_get(&running) == 0) {
		k_mutex_unlock(&pending_lock);
		return;
	}
	if (k_mem_slab_alloc(&sdu_slab, (void **)&item, K_NO_WAIT) != 0) {
		/* Worker is behind: drop this SDU (overrun) rather than block the BT RX
		 * context; the worker will catch up.
		 */
		k_mutex_unlock(&pending_lock);
		LOG_DBG("sdu_slab empty, dropping SDU");
		return;
	}

	const uint16_t opf = active_cfg.octets_per_frame;
	const uint32_t expected =
		(uint32_t)opf * active_cfg.chan_cnt * active_cfg.frame_blocks_per_sdu;

	/* Only decode SDUs the controller marked valid; treat wrong-size SDUs as PLC.
	 * The heavy LC3 decode itself runs on the worker, not this BT RX context.
	 */
	item->do_plc = (info->flags & BT_ISO_FLAGS_VALID) == 0U || buf->len != expected;
	item->buf = net_buf_ref(buf);

	k_fifo_put(&sdu_fifo, item);

	k_mutex_unlock(&pending_lock);
}

void sample_bt_audio_playback_set_volume(uint8_t vcs_volume)
{
	sample_bt_audio_pcm_sink_set_volume(vcs_volume);
}

void sample_bt_audio_playback_set_mute(bool mute)
{
	sample_bt_audio_pcm_sink_set_mute(mute);
}
