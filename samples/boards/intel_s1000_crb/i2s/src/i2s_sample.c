/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/i2s.h>
#include <audio/codec.h>

#include <string.h>
#include <math.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_REGISTER(i2s_sample);

#define AUDIO_SAMPLE_FREQ		(48000)
#define AUDIO_SAMPLES_PER_CH_PER_FRAME	(64)
#define AUDIO_NUM_CHANNELS		(2)
#define AUDIO_SAMPLES_PER_FRAME		\
	(AUDIO_SAMPLES_PER_CH_PER_FRAME * AUDIO_NUM_CHANNELS)
#define AUDIO_SAMPLE_BYTES		(4)
#define AUDIO_SAMPLE_BIT_WIDTH		(32)

#define AUDIO_FRAME_BUF_BYTES		\
	(AUDIO_SAMPLES_PER_FRAME * AUDIO_SAMPLE_BYTES)

#define I2S_PLAYBACK_DEV		"I2S_1"
#define I2S_HOST_DEV			"I2S_2"

#define I2S_PLAY_BUF_COUNT		(6)
#define I2S_TX_PRELOAD_BUF_COUNT	(2)

#define BASE_TONE_FREQ_HZ		1046.502 /* Hz */
#define FLOAT_VALUE_OF_2PI		(2 * 3.1415926535897932384626433832795)
#define BASE_TONE_FREQ_RAD		(BASE_TONE_FREQ_HZ * FLOAT_VALUE_OF_2PI)
#define BASE_TONE_PHASE_DELTA		(BASE_TONE_FREQ_RAD / AUDIO_SAMPLE_FREQ)

#define SECONDS_PER_TONE		(1) /* second(s) */
#define TONES_TO_PLAY			\
	{0, 2, 4, 5, 7, 9, 10, 12, 12, 10, 9, 7, 5, 4, 2, 0}

#define AUDIO_FRAMES_PER_SECOND		\
	(AUDIO_SAMPLE_FREQ / AUDIO_SAMPLES_PER_CH_PER_FRAME)
#define AUDIO_FRAMES_PER_TONE_DURATION	\
	(SECONDS_PER_TONE * AUDIO_FRAMES_PER_SECOND)

#define SIGNAL_AMPLITUDE_DBFS		(-36)
#define SIGNAL_AMPLITUDE_BITS		(31 + (SIGNAL_AMPLITUDE_DBFS / 6))
#define SIGNAL_AMPLITUDE_SCALE		(1 << SIGNAL_AMPLITUDE_BITS)

#ifdef AUDIO_PLAY_FROM_HOST
#define APP_MODE_STRING			"host playback"
#else
#define APP_MODE_STRING			"tone playback"
#endif

static struct k_mem_slab i2s_mem_slab;
__attribute__((section(".dma_buffers")))
static char audio_buffers[AUDIO_FRAME_BUF_BYTES][I2S_PLAY_BUF_COUNT];
static const struct device *spk_i2s_dev;
static const struct device *host_i2s_dev;
static const struct device *codec_device;

#ifndef AUDIO_PLAY_FROM_HOST
static inline int audio_playback_buffer_fill(float phase_delta, int32_t *buffer,
		int channels, int samples)
{
	int channel;
	int32_t sample;
	int32_t *wr_ptr;
	int sample_index;
	static float phase;

	wr_ptr = buffer;
	sample_index = 0;
	while ((sample_index < samples) && (phase_delta != 0.0)) {
		/* get sine(phase) and scale it */
		sample = (int32_t)(SIGNAL_AMPLITUDE_SCALE * sinf(phase));
		/* update phase for next sample */
		phase = fmodf(phase + phase_delta, FLOAT_VALUE_OF_2PI);
		/* write same sample value to all channels */
		for (channel = 0; channel < channels; channel++) {
			*wr_ptr++ = sample;
		}
		sample_index++;
	}

	return sample_index;
}

static float audio_playback_tone_get_next(void)
{
	static int index;
	static int frame;
	char tones[] = TONES_TO_PLAY;

	if (frame == 0) {
		LOG_INF("Tone %u Hz", (unsigned int)(BASE_TONE_FREQ_HZ *
					powf(2.0, (float)tones[index]/12.0)));
	}

	if (++frame == AUDIO_FRAMES_PER_TONE_DURATION) {
		frame = 0;
		index++;
	}

	if (index == ARRAY_SIZE(tones)) {
		index = 0;

		/* all tones returned */
		return 0.0f;
	}

	return (BASE_TONE_PHASE_DELTA * powf(2.0, (float)tones[index]/12.0));
}
#endif

static void i2s_audio_init(void)
{
	int ret;
	struct i2s_config i2s_cfg;
	struct audio_codec_cfg codec_cfg;

	k_mem_slab_init(&i2s_mem_slab, audio_buffers, AUDIO_FRAME_BUF_BYTES,
			I2S_PLAY_BUF_COUNT);

	spk_i2s_dev = device_get_binding(I2S_PLAYBACK_DEV);

	if (!spk_i2s_dev) {
		LOG_ERR("unable to find " I2S_PLAYBACK_DEV " device");
		return;
	}

	host_i2s_dev = device_get_binding(I2S_HOST_DEV);

	if (!host_i2s_dev) {
		LOG_ERR("unable to find " I2S_HOST_DEV " device");
		return;
	}

	codec_device = device_get_binding(DT_LABEL(DT_INST(0, ti_tlv320dac)));
	if (!codec_device) {
		LOG_ERR("unable to find " DT_LABEL(DT_INST(0, ti_tlv320dac)) " device");
		return;
	}

	/* configure i2s for audio playback */
	i2s_cfg.word_size = AUDIO_SAMPLE_BIT_WIDTH;
	i2s_cfg.channels = AUDIO_NUM_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S | I2S_FMT_CLK_NF_NB;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER |
		I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg.frame_clk_freq = AUDIO_SAMPLE_FREQ;
	i2s_cfg.block_size = AUDIO_FRAME_BUF_BYTES;
	i2s_cfg.mem_slab = &i2s_mem_slab;

	/* make the transmit interface non-blocking */
	i2s_cfg.timeout = 0;
	ret = i2s_configure(spk_i2s_dev, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		LOG_ERR("dmic_configure failed with %d error", ret);
		return;
	}

	/* make the receive interface blocking */
	i2s_cfg.timeout = SYS_FOREVER_MS;
	ret = i2s_configure(host_i2s_dev, I2S_DIR_RX, &i2s_cfg);
	if (ret != 0) {
		LOG_ERR("dmic_configure failed with %d error", ret);
		return;
	}

	/* configure codec */
	codec_cfg.dai_type = AUDIO_DAI_TYPE_I2S,
	codec_cfg.dai_cfg.i2s = i2s_cfg;
	codec_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_SLAVE |
				I2S_OPT_BIT_CLK_SLAVE;
	codec_cfg.dai_cfg.i2s.mem_slab = NULL;
	codec_cfg.mclk_freq = soc_get_ref_clk_freq();

	audio_codec_configure(codec_device, &codec_cfg);
}

static void i2s_start_audio(void)
{
	int ret;

	LOG_DBG("Starting audio playback...");
	/* start codec output */
	audio_codec_start_output(codec_device);

	/* start i2s */
	ret = i2s_trigger(spk_i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret) {
		LOG_ERR("spk_i2s_dev TX start failed. code %d", ret);
	}

	ret = i2s_trigger(host_i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret) {
		LOG_ERR("host_i2s_dev TX start failed. code %d", ret);
	}

	ret = i2s_trigger(host_i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
	if (ret) {
		LOG_ERR("host_i2s_dev RX start failed. code %d", ret);
	}
}

static void i2s_prepare_audio(const struct device *dev)
{
	int frame_counter = 0;
	void *buffer;
	int ret;

	LOG_DBG("Preloading silence...");
	while (frame_counter++ < I2S_TX_PRELOAD_BUF_COUNT) {
		ret = k_mem_slab_alloc(&i2s_mem_slab, &buffer, K_NO_WAIT);
		if (ret) {
			LOG_ERR("buffer alloc failed %d", ret);
			return;
		}

		LOG_DBG("allocated buffer %p frame %d",
				buffer, frame_counter);

		/* fill the buffer with zeros (silence) */
		memset(buffer, 0, AUDIO_FRAME_BUF_BYTES);

		ret = i2s_write(dev, buffer, AUDIO_FRAME_BUF_BYTES);
		if (ret) {
			LOG_ERR("i2s_write failed %d", ret);
			k_mem_slab_free(&i2s_mem_slab, &buffer);
		}
	}
}

static void i2s_play_audio(void)
{
	void *in_buf;
	void *copy_buf;
	size_t size;
	int ret;

	while (true) {
		/* read from host I2S interface */
		ret = i2s_read(host_i2s_dev, &in_buf, &size);
		if (ret) {
			LOG_ERR("host_i2s_dev i2s_read failed %d", ret);
			return;
		}

		/* make a copy of the audio to send back to the host */
		ret = k_mem_slab_alloc(&i2s_mem_slab, &copy_buf, K_NO_WAIT);
		if (ret) {
			LOG_ERR("buffer alloc failed %d", ret);
			k_mem_slab_free(&i2s_mem_slab, &in_buf);
			return;
		}

		memcpy(copy_buf, in_buf, AUDIO_FRAME_BUF_BYTES);

		/* loop the audio back to the host */
		ret = i2s_write(host_i2s_dev, copy_buf, AUDIO_FRAME_BUF_BYTES);
		if (ret) {
			k_mem_slab_free(&i2s_mem_slab, &copy_buf);
			LOG_ERR("host_i2s_dev i2s_write failed %d", ret);
			return;
		}

#ifndef AUDIO_PLAY_FROM_HOST
		/* fill buffer with audio samples */
		if (audio_playback_buffer_fill(audio_playback_tone_get_next(),
					(int32_t *)in_buf, AUDIO_NUM_CHANNELS,
					size) < size) {
			/* break if all tones are exhausted */
			k_mem_slab_free(&i2s_mem_slab, &in_buf);
			break;
		}
#endif
		ret = i2s_write(spk_i2s_dev, in_buf, AUDIO_FRAME_BUF_BYTES);
		if (ret) {
			k_mem_slab_free(&i2s_mem_slab, &in_buf);
			LOG_ERR("spk_i2s_dev i2s_write failed %d", ret);
		}
	}
}

#ifndef AUDIO_PLAY_FROM_HOST
static void i2s_stop_audio(void)
{
	int ret;

	ret = i2s_trigger(spk_i2s_dev, I2S_DIR_TX, I2S_TRIGGER_STOP);
	if (ret) {
		LOG_ERR("spk_i2s_dev stop failed with code %d", ret);
	}

	ret = i2s_trigger(host_i2s_dev, I2S_DIR_RX, I2S_TRIGGER_STOP);
	if (ret) {
		LOG_ERR("host_i2s_dev stop failed with code %d", ret);
	}

	LOG_DBG("Stopping audio playback...");
}
#endif

static void i2s_audio_sample_app(void *p1, void *p2, void *p3)
{
	i2s_audio_init();

	LOG_INF("Starting I2S audio sample app in " APP_MODE_STRING " mode...");
	i2s_prepare_audio(spk_i2s_dev);
	i2s_prepare_audio(host_i2s_dev);
	i2s_start_audio();
#ifdef AUDIO_PLAY_FROM_HOST
	LOG_WRN("Play audio from the host over I2S using");
	LOG_WRN("aplay -f S32_LE -r 48000 -c 2 -D <Device> <WAV file>");
#endif
	i2s_play_audio();
#ifndef AUDIO_PLAY_FROM_HOST
	i2s_stop_audio();
	LOG_INF("Exiting I2S audio sample app ...");
	k_thread_suspend(k_current_get());
#endif
}

K_THREAD_DEFINE(i2s_sample, 1024, i2s_audio_sample_app, NULL, NULL, NULL,
		10, 0, 0);
