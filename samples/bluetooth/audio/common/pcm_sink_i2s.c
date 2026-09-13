/*
 * SPDX-FileCopyrightText: Copyright 2026 Ezurio LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * I2S + external-codec PCM sink.
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

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <sample_bt_audio_pcm_sink.h>

LOG_MODULE_DECLARE(sample_bt_audio_playback, CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_LOG_LEVEL);

#define I2S_CODEC_TX_NODE DT_ALIAS(i2s_codec_tx)
#define I2S_TIMEOUT_MS    2000U
#define SAMPLE_BIT_WIDTH  16U

/* k_mem_slab_init() rejects a slab buffer or block size not aligned to sizeof(void *)
 * (8 bytes on 64-bit targets), so align the slab buffer to pointer width.
 */
#define PCM_BLOCK_ALIGN sizeof(void *)

/* Depth of the I2S DMA queue managed by i2s_configure(). */
#define I2S_MEM_SLAB_BLOCKS ((uint32_t)CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_I2S_DMA_BLOCK_COUNT)

/* Blocks queued before START. The I2S API only requires a single queued block before the
 * transmitter is enabled, and a driver's own TX queue may be shallow; queueing more blocks
 * than that depth while TX is stopped blocks in i2s_buf_write() until timeout and fails
 * startup. Prime exactly one block - the minimum the contract needs - and let the running
 * worker fill the remaining capacity.
 */
#define I2S_PREFILL_BLOCKS 1U

/* DMA-visible buffer: __nocache places it in non-cacheable memory on cached
 * targets and is a no-op elsewhere.
 */
static __nocache __aligned(PCM_BLOCK_ALIGN) uint8_t
	i2s_slab_buffer[I2S_MEM_SLAB_BLOCKS * SAMPLE_BT_AUDIO_PCM_SINK_MAX_BLOCK_BYTES];
static struct k_mem_slab i2s_mem_slab;

static const struct device *const codec_tx = DEVICE_DT_GET(I2S_CODEC_TX_NODE);
static const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));

static struct sample_bt_audio_pcm_sink_cfg sink_cfg;
static atomic_t configured; /* set after first successful sample_bt_audio_pcm_sink_configure() */

/* Cached VCS state re-applied after every codec (re)configure. VCS defines volume as
 * 0..255; codec drivers expect a codec-specific range (WM8962 uses a 7-bit 0..127
 * output-volume register, TLV320 uses a negative dB range). The VCS scale is mapped
 * linearly onto the CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_CODEC_VOL_MIN..VOL_MAX window, so
 * boards select the range in Kconfig rather than patching apply_volume().
 */
static uint8_t cached_vcs_volume = 100U;
static bool cached_mute;

/* Serialises codec control-bus access and the cached VCS state above.
 * sample_bt_audio_pcm_sink_set_volume()/set_mute() run in the Bluetooth RX context while
 * configure()/start()/stop() run on the system workqueue, so without this lock a VCS
 * update could interleave a codec register sequence on the shared control bus.
 */
static struct k_mutex codec_lock;

/* Caller must hold codec_lock. */
static void apply_volume(void)
{
	audio_property_value_t val;
	int ret;

	/* Map the 0..255 VCS scale onto the codec's output-volume range. The range is
	 * board/codec-specific and may be negative (e.g. a dB scale), so it is signed
	 * and comes from Kconfig.
	 */
	val.vol = CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_CODEC_VOL_MIN +
		  (int)cached_vcs_volume *
			  (CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_CODEC_VOL_MAX -
			   CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_CODEC_VOL_MIN) /
			  255;
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
				       AUDIO_CHANNEL_ALL, val);
	if (ret != 0) {
		LOG_ERR("codec volume set failed: %d", ret);
	}

	val.mute = cached_mute;
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_MUTE,
				       AUDIO_CHANNEL_ALL, val);
	if (ret != 0) {
		LOG_ERR("codec mute set failed: %d", ret);
	}

	ret = audio_codec_apply_properties(codec_dev);
	if (ret != 0) {
		LOG_ERR("codec apply_properties failed: %d", ret);
	}
}

/* I2S_TRIGGER_DRAIN is asynchronous: the transmitter keeps sending the blocks that are
 * already queued and only returns to READY once the TX queue empties. Block for the
 * worst-case queued duration so a subsequent bring-up cannot reinitialise the mem_slab
 * or push new blocks while the DMA still owns the old ones (or while the stream is still
 * STOPPING, which the SAI driver would reject).
 */
static void wait_for_tx_drain(void)
{
	uint32_t frame_us = sink_cfg.frame_duration_us;

	if (frame_us == 0U) {
		return;
	}
	k_sleep(K_USEC((uint64_t)I2S_MEM_SLAB_BLOCKS * frame_us));
}

static int configure_i2s_and_codec(void)
{
	/* const designated-init so every field is assigned and none is left
	 * indeterminate.
	 */
	const struct i2s_config i2s_cfg = {
		.word_size = SAMPLE_BIT_WIDTH,
		.channels = sink_cfg.channels,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER,
		.frame_clk_freq = sink_cfg.sample_rate_hz,
		.mem_slab = &i2s_mem_slab,
		.block_size = sink_cfg.block_bytes,
		.timeout = I2S_TIMEOUT_MS,
	};
	/* Zero-initialized: several codec drivers (e.g. WM8962, TLV320) read mclk_freq
	 * to compute PLL/clock dividers, so it must not be left indeterminate.
	 */
	struct audio_codec_cfg codec_cfg = {0};
	int ret;

	codec_cfg.mclk_freq = (uint32_t)CONFIG_SAMPLE_BT_AUDIO_PLAYBACK_MCLK_FREQUENCY;
	codec_cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	codec_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	codec_cfg.dai_cfg.i2s.word_size = SAMPLE_BIT_WIDTH;
	codec_cfg.dai_cfg.i2s.channels = sink_cfg.channels;
	codec_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	codec_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET;
	codec_cfg.dai_cfg.i2s.frame_clk_freq = sink_cfg.sample_rate_hz;
	codec_cfg.dai_cfg.i2s.mem_slab = &i2s_mem_slab;
	codec_cfg.dai_cfg.i2s.block_size = sink_cfg.block_bytes;

	ret = k_mem_slab_init(&i2s_mem_slab, i2s_slab_buffer, sink_cfg.block_bytes,
			      I2S_MEM_SLAB_BLOCKS);
	if (ret < 0) {
		LOG_ERR("k_mem_slab_init failed: %d", ret);
		return ret;
	}

	/* Configure the I2S controller (MCLK/BCLK source) before the codec so the codec
	 * sees a stable MCLK when it locks its internal PLL/FLL.
	 */
	ret = i2s_configure(codec_tx, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		LOG_ERR("i2s_configure failed: %d", ret);
		return ret;
	}

	ret = audio_codec_configure(codec_dev, &codec_cfg);
	if (ret != 0) {
		LOG_ERR("audio_codec_configure failed: %d", ret);
		return ret;
	}

	return 0;
}

int sample_bt_audio_pcm_sink_init(void)
{
	k_mutex_init(&codec_lock);

	if (!device_is_ready(codec_tx)) {
		LOG_ERR("%s not ready", codec_tx->name);
		return -EIO;
	}
	if (!device_is_ready(codec_dev)) {
		LOG_ERR("%s not ready", codec_dev->name);
		return -EIO;
	}

	return 0;
}

int sample_bt_audio_pcm_sink_configure(const struct sample_bt_audio_pcm_sink_cfg *cfg)
{
	const bool changed = atomic_get(&configured) == 0 ||
			     cfg->sample_rate_hz != sink_cfg.sample_rate_hz ||
			     cfg->frame_duration_us != sink_cfg.frame_duration_us ||
			     cfg->channels != sink_cfg.channels || cfg->bits != sink_cfg.bits ||
			     cfg->block_bytes != sink_cfg.block_bytes;
	int ret;

	if (!changed) {
		return 0;
	}

	/* configure_i2s_and_codec() programs the hardware from sink_cfg. A partial failure
	 * (e.g. slab reinit and i2s_configure() succeed but audio_codec_configure() fails)
	 * can leave the hardware half-reprogrammed, so clear configured on any failure to
	 * force the next call to reprogram all components regardless of the requested format.
	 */
	sink_cfg = *cfg;

	k_mutex_lock(&codec_lock, K_FOREVER);
	ret = configure_i2s_and_codec();
	k_mutex_unlock(&codec_lock);
	if (ret != 0) {
		atomic_set(&configured, 0);
		return ret;
	}

	atomic_set(&configured, 1);

	return 0;
}

int sample_bt_audio_pcm_sink_start(void)
{
	/* Static: sized to the worst-case block, off the caller stack. */
	static uint8_t silence[SAMPLE_BT_AUDIO_PCM_SINK_MAX_BLOCK_BYTES];
	int ret;

	k_mutex_lock(&codec_lock, K_FOREVER);

	apply_volume();

	/* Power up / unmute the codec's output path. Configuring the DAI alone is not
	 * enough - several audio_codec drivers gate the DAC on start_output().
	 */
	audio_codec_start_output(codec_dev);

	/* Recover from I2S_STATE_ERROR left over by a prior underrun/overrun before priming.
	 * PREPARE is only valid in ERROR state, and in ERROR state i2s_buf_write() itself
	 * fails with -EIO, so this must run before the pre-fill or recovery never happens. On
	 * the normal (non-recovery) path the driver is already READY and returns -EIO, which
	 * is the expected benign case; any other error is a genuine failure worth flagging.
	 */
	ret = i2s_trigger(codec_tx, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
	if (ret == -EIO) {
		LOG_DBG("i2s_trigger PREPARE: already READY");
	} else if (ret != 0) {
		LOG_ERR("i2s_trigger PREPARE failed: %d", ret);
		audio_codec_stop_output(codec_dev);
		k_mutex_unlock(&codec_lock);
		return ret;
	}

	/* Prime the DMA queue with silence so START has valid frames to transmit
	 * from the moment the transmitter is enabled. The running worker fills the rest.
	 */
	(void)memset(silence, 0, sink_cfg.block_bytes);
	for (uint32_t i = 0; i < I2S_PREFILL_BLOCKS; i++) {
		int wret = i2s_buf_write(codec_tx, silence, sink_cfg.block_bytes);

		if (wret < 0) {
			LOG_ERR("silence pre-fill %u failed: %d", i, wret);
			audio_codec_stop_output(codec_dev);
			k_mutex_unlock(&codec_lock);
			return wret;
		}
	}

	ret = i2s_trigger(codec_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret != 0) {
		LOG_ERR("i2s_trigger START failed: %d", ret);
		audio_codec_stop_output(codec_dev);
		k_mutex_unlock(&codec_lock);
		return ret;
	}

	k_mutex_unlock(&codec_lock);
	return 0;
}

void sample_bt_audio_pcm_sink_stop(void)
{
	int ret;

	k_mutex_lock(&codec_lock, K_FOREVER);
	audio_codec_stop_output(codec_dev);
	k_mutex_unlock(&codec_lock);

	/* Documented graceful shutdown: DRAIN flushes what is already queued and
	 * transitions RUNNING -> STOPPING -> READY. Never use I2S_TRIGGER_DROP - some
	 * driver implementations busy-wait on FIFO empty and hang once the transmitter
	 * has already been disabled.
	 */
	ret = i2s_trigger(codec_tx, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	if (ret != 0) {
		LOG_ERR("i2s_trigger DRAIN failed: %d", ret);
	} else {
		wait_for_tx_drain();
	}
}

int sample_bt_audio_pcm_sink_write(void *block, size_t bytes)
{
	return i2s_buf_write(codec_tx, block, bytes);
}

bool sample_bt_audio_pcm_sink_is_configured(void)
{
	return atomic_get(&configured) != 0;
}

void sample_bt_audio_pcm_sink_set_volume(uint8_t vcs_volume)
{
	k_mutex_lock(&codec_lock, K_FOREVER);
	cached_vcs_volume = vcs_volume;
	if (atomic_get(&configured) != 0) {
		apply_volume();
	}
	k_mutex_unlock(&codec_lock);
}

void sample_bt_audio_pcm_sink_set_mute(bool mute)
{
	k_mutex_lock(&codec_lock, K_FOREVER);
	cached_mute = mute;
	if (atomic_get(&configured) != 0) {
		apply_volume();
	}
	k_mutex_unlock(&codec_lock);
}
