/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Audio playback pipeline: LC3 -> PCM -> I2S -> external codec.
 *
 * We use both the I2S and the audio_codec APIs on purpose. The
 * audio_codec_write/start/stop/register_done_callback path integrates data
 * transport into the codec driver, but it is only implemented by codec
 * drivers that embed the DMA/serializer engine themselves (see
 * samples/bluetooth/audio/bap_broadcast_sink/src/hw_codec.c). Codecs that
 * only program registers over I2C/SPI (WM8962, TLV320, etc.) rely on the
 * SoC's SAI/I2S peripheral to move samples, so the sample must drive I2S
 * directly for data transport and use audio_codec_configure() /
 * audio_codec_set_property() for codec-side setup and volume/mute.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "le_audio_playback.h"

#if !DT_HAS_ALIAS(i2s_codec_tx) || !DT_NODE_EXISTS(DT_NODELABEL(audio_codec)) || \
	!IS_ENABLED(CONFIG_I2S) || !IS_ENABLED(CONFIG_AUDIO_CODEC)

int le_audio_playback_init(void)
{
	printk("le_audio_playback: I2S/codec alias not present, playback disabled\n");
	return 0;
}

int le_audio_playback_stream_start(const struct le_audio_pcm_cfg *cfg)
{
	ARG_UNUSED(cfg);
	return 0;
}

void le_audio_playback_stream_stop(void) { }

void le_audio_playback_stream_recv(struct net_buf *buf)
{
	ARG_UNUSED(buf);
}

void le_audio_playback_set_volume(uint8_t vcs_volume) { ARG_UNUSED(vcs_volume); }

void le_audio_playback_set_mute(bool mute) { ARG_UNUSED(mute); }

#else

#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>

#include <lc3.h>

#define I2S_CODEC_TX_NODE DT_ALIAS(i2s_codec_tx)
#define I2S_TIMEOUT_MS    2000U
#define SAMPLE_BIT_WIDTH  16U
#define PCM_MAX_CHAN      2U

/* Worst-case block: 48 kHz, 10 ms, stereo, 16-bit = 480 * 2 * 2 = 1920 B. */
#define LC3_MAX_SAMPLES_PER_FRAME (48U * 10U)  /* 48 kHz * 10 ms per channel */
#define PCM_MAX_BLOCK_BYTES       (LC3_MAX_SAMPLES_PER_FRAME * PCM_MAX_CHAN * sizeof(int16_t))

/* Depth of the I2S DMA queue managed by i2s_configure(). */
#define I2S_MEM_SLAB_BLOCKS ((uint32_t)CONFIG_TMAP_PERIPHERAL_I2S_DMA_BLOCK_COUNT)

/* Depth of the decode -> I2S worker handoff queue. */
#define PCM_MSGQ_DEPTH ((uint32_t)CONFIG_TMAP_PERIPHERAL_PCM_QUEUE_DEPTH)

/* i2s_buf_write() allocates from this slab and copies our worker's block into it. */
#if CONFIG_NOCACHE_MEMORY
#define __I2S_NOCACHE __attribute__((__section__(".nocache")))
#else
#define __I2S_NOCACHE
#endif

static __I2S_NOCACHE __aligned(4) uint8_t
	i2s_slab_buffer[CONFIG_TMAP_PERIPHERAL_I2S_DMA_BLOCK_COUNT * PCM_MAX_BLOCK_BYTES];
static struct k_mem_slab i2s_mem_slab;

/* Producer/consumer queue between stream_recv() (BAP RX context) and the I2S worker. */
K_MSGQ_DEFINE(pcm_msgq, PCM_MAX_BLOCK_BYTES, PCM_MSGQ_DEPTH, 4);

static const struct device *const codec_tx  = DEVICE_DT_GET(I2S_CODEC_TX_NODE);
static const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));

/* Two LC3 decoder contexts for stereo. Sized for the worst-case 48 kHz / 10 ms. */
static lc3_decoder_mem_48k_t decoder_mem[PCM_MAX_CHAN];
static lc3_decoder_t         decoders[PCM_MAX_CHAN];

/* Shared state: active_cfg, samples_per_frame, pcm_block_bytes, out_chan_cnt
 * and decoders[] are mutated only by bringup_work_handler and only while
 * running == 0. Consumers (stream_recv, i2s_worker) atomic_get(&running)
 * before reading any of the cfg fields; the atomic_set release on the
 * producer publishes cfg to the consumers via matching acquire on the reader.
 */
static struct le_audio_pcm_cfg active_cfg;
static uint32_t               samples_per_frame; /* per-channel samples per LC3 frame */
static uint32_t               pcm_block_bytes;   /* per-SDU decoded PCM size */
static uint8_t                out_chan_cnt;      /* channels sent to I2S (always 2) */
static atomic_t               running;
static atomic_t               i2s_configured;    /* set after first successful i2s_configure() */

/* Cached VCS state re-applied after every codec (re)configure. VCS defines volume as
 * 0..255; most codec drivers expect a smaller linear range (WM8962 uses a 7-bit 0..127
 * output-volume register). Boards with a different codec should adjust apply_volume().
 */
static uint8_t                cached_vcs_volume = 100U;
static bool                   cached_mute;

static K_THREAD_STACK_DEFINE(i2s_worker_stack, CONFIG_TMAP_PERIPHERAL_I2S_WORKER_STACK_SIZE);
static struct k_thread i2s_worker_thread;
static k_tid_t         i2s_worker_tid;

/* Deferred bring-up: stream_start() runs in the BAP RX workqueue context; doing the
 * codec control-bus transactions and mem_slab reinit inline starves host callbacks and
 * causes some Centrals to abort the setup. stream_stop() is a flag flip and stays inline.
 */
static struct k_work           bringup_work;
static struct le_audio_pcm_cfg pending_cfg;
static bool                    pending_valid;

/* Wakes the i2s worker after stream_stop parked it on the sem. */
static K_SEM_DEFINE(resume_sem, 0, 1);

static void bringup_work_handler(struct k_work *work);

static void apply_volume(void)
{
	audio_property_value_t val;
	int ret;

	val.vol = (int)(((uint32_t)cached_vcs_volume * 127U) / 255U);
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
				       AUDIO_CHANNEL_ALL, val);
	if (ret != 0) {
		printk("codec volume set failed: %d\n", ret);
	}

	val.mute = cached_mute;
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_MUTE,
				       AUDIO_CHANNEL_ALL, val);
	if (ret != 0) {
		printk("codec mute set failed: %d\n", ret);
	}

	ret = audio_codec_apply_properties(codec_dev);
	if (ret != 0) {
		printk("codec apply_properties failed: %d\n", ret);
	}
}

static void i2s_worker(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	/* Static: sized to the worst-case block. Keeping this off the thread
	 * stack avoids overflowing i2s_worker_stack into adjacent .bss.
	 */
	static uint8_t block[PCM_MAX_BLOCK_BYTES];

	while (true) {
		if (!atomic_get(&running)) {
			/* While parked we stop feeding the driver's TX queue; the outstanding
			 * DMA blocks drain and the driver auto-transitions to READY. This lets
			 * us avoid I2S_TRIGGER_DROP, whose disable path in some driver
			 * implementations busy-waits on FIFO empty under irq_lock and hangs
			 * once the transmitter has already been disabled.
			 */
			(void)k_sem_take(&resume_sem, K_FOREVER);
			continue;
		}

		/* i2s_buf_write() is paced by the driver freeing a DMA block per transfer.
		 * On brief SDU gaps we push silence so the codec keeps seeing valid frames.
		 */
		if (k_msgq_get(&pcm_msgq, block, K_NO_WAIT) != 0) {
			memset(block, 0, pcm_block_bytes);
		}

		int ret = i2s_buf_write(codec_tx, block, pcm_block_bytes);

		if (ret < 0) {
			printk("i2s_buf_write failed: %d\n", ret);
		}
	}
}

static int configure_i2s_and_codec(uint32_t sample_rate)
{
	struct i2s_config i2s_cfg;
	struct audio_codec_cfg codec_cfg;

	i2s_cfg.word_size       = SAMPLE_BIT_WIDTH;
	i2s_cfg.channels        = out_chan_cnt;
	i2s_cfg.format          = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.options         = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
	i2s_cfg.frame_clk_freq  = sample_rate;
	i2s_cfg.mem_slab        = &i2s_mem_slab;
	i2s_cfg.block_size      = pcm_block_bytes;
	i2s_cfg.timeout         = I2S_TIMEOUT_MS;

	codec_cfg.dai_route                  = AUDIO_ROUTE_PLAYBACK;
	codec_cfg.dai_type                   = AUDIO_DAI_TYPE_I2S;
	codec_cfg.dai_cfg.i2s.word_size      = SAMPLE_BIT_WIDTH;
	codec_cfg.dai_cfg.i2s.channels       = out_chan_cnt;
	codec_cfg.dai_cfg.i2s.format         = I2S_FMT_DATA_FORMAT_I2S;
	codec_cfg.dai_cfg.i2s.options        = I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET;
	codec_cfg.dai_cfg.i2s.frame_clk_freq = sample_rate;
	codec_cfg.dai_cfg.i2s.mem_slab       = &i2s_mem_slab;
	codec_cfg.dai_cfg.i2s.block_size     = pcm_block_bytes;

	/* Configure the I2S controller (MCLK/BCLK source) before the codec so the codec
	 * sees a stable MCLK when it locks its internal PLL/FLL.
	 */
	int ret = i2s_configure(codec_tx, I2S_DIR_TX, &i2s_cfg);

	if (ret != 0) {
		printk("i2s_configure failed: %d\n", ret);
		return ret;
	}

	ret = audio_codec_configure(codec_dev, &codec_cfg);
	if (ret != 0) {
		printk("audio_codec_configure failed: %d\n", ret);
		return ret;
	}

	/* Give the codec time to lock its internal PLL/FLL onto MCLK after
	 * i2s_configure() enabled the SAI clocks. WM8962 needs ~30 ms; 50 ms
	 * covers similar codecs. Drivers that report ready state via a callback
	 * can skip this.
	 */
	k_msleep(50);

	ret = k_mem_slab_init(&i2s_mem_slab, i2s_slab_buffer, pcm_block_bytes,
			      I2S_MEM_SLAB_BLOCKS);
	if (ret < 0) {
		printk("k_mem_slab_init failed: %d\n", ret);
		return ret;
	}

	return 0;
}

int le_audio_playback_init(void)
{
	if (!device_is_ready(codec_tx)) {
		printk("%s not ready\n", codec_tx->name);
		return -EIO;
	}
	if (!device_is_ready(codec_dev)) {
		printk("%s not ready\n", codec_dev->name);
		return -EIO;
	}

	k_work_init(&bringup_work, bringup_work_handler);

	i2s_worker_tid = k_thread_create(&i2s_worker_thread, i2s_worker_stack,
					 K_THREAD_STACK_SIZEOF(i2s_worker_stack),
					 i2s_worker, NULL, NULL, NULL,
					 K_PRIO_COOP(CONFIG_TMAP_PERIPHERAL_I2S_WORKER_PRIORITY),
					 0, K_NO_WAIT);
	k_thread_name_set(i2s_worker_tid, "le_audio_i2s");

	return 0;
}

int le_audio_playback_stream_start(const struct le_audio_pcm_cfg *cfg)
{
	if (cfg->chan_cnt == 0U || cfg->chan_cnt > PCM_MAX_CHAN) {
		printk("Unsupported chan_cnt %u\n", cfg->chan_cnt);
		return -EINVAL;
	}
	if (cfg->frame_blocks_per_sdu != 1U) {
		printk("Unsupported frame_blocks_per_sdu %u\n", cfg->frame_blocks_per_sdu);
		return -EINVAL;
	}

	pending_cfg   = *cfg;
	pending_valid = true;
	(void)k_work_submit(&bringup_work);
	return 0;
}

static void bringup_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!pending_valid) {
		return;
	}
	pending_valid = false;

	if (atomic_get(&running)) {
		return;
	}

	const struct le_audio_pcm_cfg *cfg = &pending_cfg;

	const bool cfg_changed = !atomic_get(&i2s_configured) ||
		cfg->freq_hz != active_cfg.freq_hz ||
		cfg->chan_cnt != active_cfg.chan_cnt ||
		cfg->frame_duration_us != active_cfg.frame_duration_us ||
		cfg->octets_per_frame != active_cfg.octets_per_frame;

	active_cfg        = *cfg;
	samples_per_frame = (cfg->freq_hz / 1000U) * (cfg->frame_duration_us / 1000U);
	out_chan_cnt      = PCM_MAX_CHAN;
	pcm_block_bytes   = samples_per_frame * out_chan_cnt * sizeof(int16_t);
	if (pcm_block_bytes > PCM_MAX_BLOCK_BYTES) {
		printk("pcm_block_bytes %u exceeds max %u\n",
		       pcm_block_bytes, (unsigned int)PCM_MAX_BLOCK_BYTES);
		return;
	}

	for (uint8_t c = 0U; c < cfg->chan_cnt; c++) {
		decoders[c] = lc3_setup_decoder((int)cfg->frame_duration_us,
						(int)cfg->freq_hz, 0,
						&decoder_mem[c]);
		if (decoders[c] == NULL) {
			printk("lc3_setup_decoder ch%u failed\n", c);
			return;
		}
	}

	k_msgq_purge(&pcm_msgq);

	if (cfg_changed) {
		int ret = configure_i2s_and_codec(cfg->freq_hz);

		if (ret != 0) {
			return;
		}
		atomic_set(&i2s_configured, 1);
	}

	apply_volume();

	/* Prime the DMA queue with silence so START has valid frames to transmit
	 * from the moment the transmitter is enabled.
	 */
	static uint8_t silence[PCM_MAX_BLOCK_BYTES];

	memset(silence, 0, pcm_block_bytes);
	for (uint32_t i = 0; i < I2S_MEM_SLAB_BLOCKS - 1U; i++) {
		int wret = i2s_buf_write(codec_tx, silence, pcm_block_bytes);

		if (wret < 0) {
			printk("silence pre-fill %u failed: %d\n", i, wret);
			return;
		}
	}

	/* Best-effort START. -EIO is expected on quick pause/resume when the driver is
	 * still draining a previous session; the blocks we just queued flow through the
	 * existing DMA loop unchanged. Otherwise this transitions from READY to RUNNING.
	 */
	(void)i2s_trigger(codec_tx, I2S_DIR_TX, I2S_TRIGGER_START);

	/* Publish cfg to consumers: atomic_set release pairs with atomic_get
	 * acquire in stream_recv() / i2s_worker().
	 */
	atomic_set(&running, 1);
	k_sem_give(&resume_sem);

	printk("le_audio_playback: %u Hz, %u ch, %u us frame, %u B/frame\n",
	       cfg->freq_hz, cfg->chan_cnt, cfg->frame_duration_us,
	       cfg->octets_per_frame);
}

void le_audio_playback_stream_stop(void)
{
	/* Flag flip only: the worker parks on resume_sem, feeding stops, the driver
	 * drains and auto-transitions to READY. Never call I2S_TRIGGER_DROP - some
	 * driver implementations busy-wait on FIFO empty and hang once the transmitter
	 * has already been disabled.
	 */
	atomic_set(&running, 0);
}

void le_audio_playback_stream_recv(struct net_buf *buf)
{
	/* Called from the BT RX WQ context - keep the stack footprint small. */
	static int16_t pcm[LC3_MAX_SAMPLES_PER_FRAME * PCM_MAX_CHAN];

	if (!atomic_get(&running) || decoders[0] == NULL) {
		return;
	}

	const uint16_t opf = active_cfg.octets_per_frame;
	const uint16_t expected = (uint16_t)(opf * active_cfg.chan_cnt);

	if (buf->len == 0U) {
		return;
	}

	if (active_cfg.chan_cnt == 1U) {
		const void *in = (buf->len >= opf) ? net_buf_pull_mem(buf, opf) : NULL;
		int err = lc3_decode(decoders[0], in, in ? opf : 0,
				     LC3_PCM_FORMAT_S16, pcm, 1);
		if (err < 0) {
			return;
		}
		/* Duplicate mono into both I2S channels. */
		for (int i = (int)samples_per_frame - 1; i >= 0; i--) {
			pcm[i * 2 + 0] = pcm[i];
			pcm[i * 2 + 1] = pcm[i];
		}
	} else {
		const bool have_data = buf->len >= expected;
		const void *in_l = have_data ? net_buf_pull_mem(buf, opf) : NULL;
		const void *in_r = have_data ? net_buf_pull_mem(buf, opf) : NULL;

		int err = lc3_decode(decoders[0], in_l, in_l ? opf : 0,
				     LC3_PCM_FORMAT_S16, &pcm[0], 2);
		if (err < 0) {
			return;
		}
		err = lc3_decode(decoders[1], in_r, in_r ? opf : 0,
				 LC3_PCM_FORMAT_S16, &pcm[1], 2);
		if (err < 0) {
			return;
		}
	}

	(void)k_msgq_put(&pcm_msgq, pcm, K_NO_WAIT);
}

void le_audio_playback_set_volume(uint8_t vcs_volume)
{
	cached_vcs_volume = vcs_volume;
	if (atomic_get(&i2s_configured)) {
		apply_volume();
	}
}

void le_audio_playback_set_mute(bool mute)
{
	cached_mute = mute;
	if (atomic_get(&i2s_configured)) {
		apply_volume();
	}
}

#endif /* DT_HAS_ALIAS(i2s_codec_tx) && DT_NODE_EXISTS(DT_NODELABEL(audio_codec)) */
