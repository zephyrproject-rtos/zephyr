/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "audio_buf.h"
#include "i2s_play.h"
#include "stream_rx.h"

#if DT_HAS_ALIAS(i2s_codec_tx) && IS_ENABLED(CONFIG_I2S) && IS_ENABLED(CONFIG_AUDIO_CODEC)

#if CONFIG_NOCACHE_MEMORY
#define __NOCACHE __attribute__((__section__(".nocache")))
#elif defined(CONFIG_DT_DEFINED_NOCACHE)
#define __NOCACHE __attribute__((__section__(CONFIG_DT_DEFINED_NOCACHE_NAME)))
#else
#define __NOCACHE
#endif

#define I2S_CODEC_TX DT_ALIAS(i2s_codec_tx)
/*
 * Keep the write timeout short. i2s_buf_write() only blocks while the driver's
 * TX input queue is full; in the RUNNING state the DMA drains it continuously,
 * so a healthy write returns quickly. If the TX has paused (see i2s_prime_tx),
 * the queue fills and writes block for the full timeout before failing - a
 * short timeout lets the consumer detect the stall and re-prime in ~200 ms
 * instead of stalling for seconds and backing up the ISO RX buffers.
 */
#define I2S_TIMEOUT  (200U)

/* Dedicated thread that drives the I2S TX from the decoded PCM ring buffer. */
#define CODEC_PLAY_STACK_SIZE 2048
#define CODEC_PLAY_PRIORITY   5

static void i2s_play_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	i2s_keep_play();
}

K_THREAD_DEFINE(i2s_play_tid, CODEC_PLAY_STACK_SIZE, i2s_play_thread, NULL, NULL, NULL,
		CODEC_PLAY_PRIORITY, 0, 0);

/* Playback stream state. */
static volatile bool audio_start;

/*
 * PCM sample rate (Hz) of the current playback stream.
 *
 * This is the single gate that enables the LC3 producer path
 * (i2s_play_add_frame). It is written ONLY by the i2s_play thread
 * (i2s_keep_play), around the point where the ring buffer is reset and
 * re-sized, and cleared by i2s_play_stop(). It is deliberately NOT written
 * from i2s_play_start() so that the producer can never observe a non-zero
 * frequency while the buffer is being reset.
 */
static volatile uint32_t play_freq_hz;

/* Fields populated by i2s_play_configure() and consumed by the codec thread. */
static volatile bool configure_requested;
static uint32_t pending_sr;
static uint8_t pending_width;
static uint8_t pending_channels;

static uint32_t audio_sample_rate;
static uint32_t audio_block_size;
static uint8_t *audio_data_sync_buf[CONFIG_BAP_BROADCAST_SINK_BOARD_CODEC_PLAY_COUNT];
static uint32_t audio_data_sync_buf_size[CONFIG_BAP_BROADCAST_SINK_BOARD_CODEC_PLAY_COUNT];
static uint8_t audio_data_sync_buf_w;
static uint8_t audio_data_sync_buf_r;
static __NOCACHE __aligned(4) uint8_t bap_silence_data[BAP_PCM_DATA_PLAY_SIZE_48K];

const struct device *const codec_tx = DEVICE_DT_GET(I2S_CODEC_TX);

static __NOCACHE __aligned(4)
	uint8_t mem_slab_buffer[CONFIG_BAP_BROADCAST_SINK_BOARD_CODEC_PLAY_COUNT *
				BAP_PCM_DATA_PLAY_SIZE_48K];
static struct k_mem_slab mem_slab;

static uint32_t rate_to_block_size(uint32_t sample_rate)
{
	switch (sample_rate) {
	case 8000:
		return BAP_PCM_DATA_PLAY_SIZE_8K;
	case 16000:
		return BAP_PCM_DATA_PLAY_SIZE_16K;
	case 24000:
		return BAP_PCM_DATA_PLAY_SIZE_24K;
	case 32000:
		return BAP_PCM_DATA_PLAY_SIZE_32K;
	case 48000:
	default:
		return BAP_PCM_DATA_PLAY_SIZE_48K;
	}
}

int i2s_play_init(void)
{
	const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));

	if (!device_is_ready(codec_tx)) {
		printk("%s is not ready\n", codec_tx->name);
		return -EIO;
	}

	if (!device_is_ready(codec_dev)) {
		printk("%s is not ready\n", codec_dev->name);
		return -EIO;
	}

	return 0;
}

/*
 * Store the requested codec parameters and signal the codec thread to apply
 * them.
 */
void i2s_play_configure(uint32_t sample_rate, uint8_t sample_width, uint8_t channels)
{
	pending_sr = sample_rate;
	pending_width = sample_width;
	pending_channels = channels;
	configure_requested = true;
}

/*
 * Apply stored parameters to the codec and I2S peripheral.
 */
static void i2s_do_hw_configure(void)
{
	const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));
	struct audio_codec_cfg audio_cfg;
	struct i2s_config config;
	uint32_t sample_rate = pending_sr;
	uint8_t sample_width = pending_width;
	uint8_t channels = pending_channels;
	size_t block_size;

	audio_sample_rate = sample_rate;
	block_size = rate_to_block_size(sample_rate);
	audio_block_size = block_size;

	audio_cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = sample_width;
	audio_cfg.dai_cfg.i2s.channels = channels;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
#ifdef CONFIG_USE_CODEC_CLOCK
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;
#else
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET;
#endif
	audio_cfg.dai_cfg.i2s.frame_clk_freq = sample_rate;
	audio_cfg.dai_cfg.i2s.mem_slab = &mem_slab;
	audio_cfg.dai_cfg.i2s.block_size = block_size;
	audio_codec_configure(codec_dev, &audio_cfg);
	/* Allow the codec to settle after being configured. */
	k_msleep(1000);

	config.word_size = sample_width;
	config.channels = channels;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
#ifdef CONFIG_USE_CODEC_CLOCK
	config.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
#else
	config.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
#endif
	config.frame_clk_freq = sample_rate;
	config.mem_slab = &mem_slab;
	config.block_size = block_size;
	config.timeout = I2S_TIMEOUT;
	if (i2s_configure(codec_tx, I2S_DIR_TX, &config)) {
		printk("failure to config streams\n");
	}

	k_mem_slab_init(&mem_slab, mem_slab_buffer, block_size,
			CONFIG_BAP_BROADCAST_SINK_BOARD_CODEC_PLAY_COUNT);
}

/*
 * Write one PCM frame to the I2S driver.
 *
 * i2s_buf_write allocates a slab block (waiting up to I2S_TIMEOUT ms),
 * copies the data, and hands it to the DMA.
 */
static int i2s_play_to_dev(const uint8_t *data, uint32_t length)
{
	int ret;

	ret = i2s_buf_write(codec_tx, (void *)data, length);
	if (ret < 0) {
		printk("i2s_buf_write failed: %d\n", ret);
	}

	return ret;
}

/*
 * (Re)prime the I2S TX FIFO with silence blocks and (re)start the DMA.
 *
 * Used both for the initial start and to recover after a TX underrun. Returns 0
 * once the FIFO has been primed and START has been issued, or a negative errno
 * if priming failed (e.g. the write timed out) so the caller can retry later.
 */
static int i2s_prime_tx(void)
{
	int ret;

	for (uint8_t i = 0U; i < CONFIG_BAP_BROADCAST_SINK_BOARD_CODEC_PLAY_COUNT; i++) {
		ret = i2s_play_to_dev(bap_silence_data, audio_block_size);
		if (ret < 0) {
			return ret;
		}

		if (i == 0U) {
			ret = i2s_trigger(codec_tx, I2S_DIR_TX, I2S_TRIGGER_START);
			if (ret < 0) {
				printk("i2s_trigger START failed: %d\n", ret);
				return ret;
			}
		}
	}

	return 0;
}

/*
 * Derive PCM sample rate (Hz) from an LC3 codec configuration.
 */
static uint32_t i2s_cfg_sample_rate(const struct bt_audio_codec_cfg *codec_cfg)
{
	int ret;

	ret = bt_audio_codec_cfg_get_freq(codec_cfg);
	if (ret < 0) {
		return 0U;
	}

	ret = bt_audio_codec_cfg_freq_to_freq_hz(ret);
	if (ret <= 0) {
		return 0U;
	}

	return (uint32_t)ret;
}

int i2s_play_start(const struct bt_audio_codec_cfg *codec_cfg)
{
	uint32_t sample_rate = i2s_cfg_sample_rate(codec_cfg);

	if (sample_rate == 0U) {
		printk("Unsupported sample rate for playback\n");
		return -EINVAL;
	}

	/*
	 * i2s_play_start() is invoked from stream_started_cb() once per BIS
	 * stream. A broadcast may carry several BIS (e.g. front-left and
	 * front-right), but the codec/I2S hardware and the PCM playback
	 * pipeline must only be configured and started ONCE.
	 *
	 * If playback is already active, ignore the additional stream(s).
	 * Reconfiguring here would make the i2s_play thread run
	 * i2s_do_hw_configure() again - which calls k_mem_slab_init() and
	 * i2s_configure() - on a memory slab and I2S stream that the DMA is
	 * already using, corrupting the driver state.
	 *
	 * The ring-buffer reset and the producer-enable (play_freq_hz) are
	 * intentionally NOT done here. They are performed by the i2s_play
	 * thread immediately after the hardware is (re)configured so that the
	 * LC3 producer (audio_add_pcm_data) can never run concurrently with
	 * audio_buf_reset().
	 */
	if (audio_start) {
		return 0;
	}

	/* The playback path uses stereo interleaved samples. */
	i2s_play_configure(sample_rate, 16U, 2U);
	audio_start = true;

	return 0;
}

void i2s_play_stop(void)
{
	/*
	 * Disable the producer first so the LC3 decoder thread stops feeding
	 * the ring buffer, then stop the consumer. play_freq_hz is the gate
	 * checked by i2s_play_add_frame().
	 */
	play_freq_hz = 0U;
	audio_start = false;
	configure_requested = false;
}

int i2s_play_add_frame(const struct stream_rx *stream, int chn, const void *pcm)
{
	uint32_t freq_hz;
	size_t frame_bytes;

	if (chn != 0) {
		return 0;
	}

	/*
	 * Only accept data from the left-channel or mono BIS stream.  The
	 * mono-to-stereo expansion in audio_feed_pcm_lc3() duplicates the left
	 * sample to both output channels, producing correct mono playback.
	 * Right-only BIS streams are skipped.
	 */
	if ((stream->lc3_chan_allocation != BT_AUDIO_LOCATION_MONO_AUDIO) &&
	    ((stream->lc3_chan_allocation & BT_AUDIO_LOCATION_FRONT_LEFT) == 0U)) {
		return 0;
	}

	/*
	 * Sample play_freq_hz once. The i2s_play thread may clear it to 0 at
	 * any time (during reconfigure or stop); taking a local snapshot keeps
	 * the guard and the subsequent frame-size calculation consistent.
	 */
	freq_hz = play_freq_hz;
	if (freq_hz == 0U) {
		/* Playback not configured (or being reconfigured); drop frame. */
		return 0;
	}
	frame_bytes = (freq_hz / 100U) * sizeof(int16_t);

	audio_feed_pcm_lc3((const uint8_t *)pcm, frame_bytes, stream->lc3_chan_cnt);

	return 0;
}

void i2s_keep_play(void)
{
	bool i2s_primed = false;
	uint8_t *get_data;
	uint32_t length;
	int ret;

	while (true) {
		if (configure_requested) {
			configure_requested = false;
			i2s_primed = false;

			/*
			 * Stop the LC3 producer before touching the ring
			 * buffer. play_freq_hz is the single gate checked by
			 * i2s_play_add_frame(); clearing it here guarantees the
			 * decoder thread cannot push data into the buffer while
			 * we reset and re-size it below.
			 */
			play_freq_hz = 0U;

			i2s_do_hw_configure();
			/*
			 * Discard PCM data that accumulated while the hardware
			 * was settling so playback always starts with fresh
			 * audio.
			 */
			audio_buf_reset(audio_sample_rate);

			/*
			 * Re-enable the producer only now that the buffer has
			 * been reset and the block size matches the configured
			 * sample rate.
			 */
			play_freq_hz = audio_sample_rate;
		}

		if (!audio_start) {
			k_msleep(10);
			continue;
		}

		/*
		 * Prime the I2S TX FIFO with silence then start the DMA. This
		 * also recovers after a TX underrun: when the broadcast source
		 * stops or a link-loss burst starves the ring buffer, the DMA
		 * drains and the driver reports "TX input queue empty" / "TX is
		 * paused". In that state i2s_buf_write() fails, so we clear
		 * i2s_primed below and re-prime here rather than spinning.
		 */
		if (!i2s_primed) {
			if (i2s_prime_tx() < 0) {
				/*
				 * Priming failed (driver not ready yet). Yield
				 * so equal-priority threads - in particular the
				 * LC3 decoder that drains the ISO RX buffers -
				 * are never starved, then retry.
				 */
				k_msleep(10);
				continue;
			}
			i2s_primed = true;
		}

		length = audio_block_size;

		/*
		 * Fetch the next decoded PCM block pointer from the ring
		 * buffer. If the buffer is below threshold or empty, NULL
		 * is returned and silence is output instead.
		 */
		audio_get_pcm_data(&get_data, length);

		if (get_data != NULL) {
			ret = i2s_play_to_dev(get_data, length);
		} else {
			ret = i2s_play_to_dev(bap_silence_data, audio_block_size);
		}

		if (ret < 0) {
			/*
			 * The write failed, which means the TX has underrun and
			 * paused. Force a re-prime on the next iteration and
			 * yield so we do not busy-spin (which would starve the
			 * equal-priority LC3 decoder thread, back up the ISO RX
			 * buffers and eventually drop the broadcast). The PCM
			 * frame we just pulled is intentionally dropped.
			 */
			i2s_primed = false;
			k_msleep(10);
			continue;
		}

		audio_data_sync_buf[audio_data_sync_buf_w %
				    CONFIG_BAP_BROADCAST_SINK_BOARD_CODEC_PLAY_COUNT] = get_data;
		audio_data_sync_buf_size[audio_data_sync_buf_w %
					 CONFIG_BAP_BROADCAST_SINK_BOARD_CODEC_PLAY_COUNT] =
			length;
		audio_data_sync_buf_w++;

		audio_media_sync(
			audio_data_sync_buf[audio_data_sync_buf_r %
					    CONFIG_BAP_BROADCAST_SINK_BOARD_CODEC_PLAY_COUNT],
			audio_data_sync_buf_size[audio_data_sync_buf_r %
						 CONFIG_BAP_BROADCAST_SINK_BOARD_CODEC_PLAY_COUNT]);

		audio_data_sync_buf_r++;
	}
}

#else

void i2s_play_configure(uint32_t sample_rate, uint8_t sample_width, uint8_t channels)
{
	ARG_UNUSED(sample_rate);
	ARG_UNUSED(sample_width);
	ARG_UNUSED(channels);
	printk("Codec is unsupported\n");
}

int i2s_play_init(void)
{
	return 0;
}

int i2s_play_start(const struct bt_audio_codec_cfg *codec_cfg)
{
	ARG_UNUSED(codec_cfg);
	return 0;
}

void i2s_play_stop(void)
{
}

int i2s_play_add_frame(const struct stream_rx *stream, int chn, const void *pcm)
{
	ARG_UNUSED(stream);
	ARG_UNUSED(chn);
	ARG_UNUSED(pcm);
	return 0;
}

void i2s_keep_play(void)
{
}

#endif
