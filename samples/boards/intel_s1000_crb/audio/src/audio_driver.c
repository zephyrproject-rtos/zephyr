/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_REGISTER(audio_io);

#include <zephyr.h>

#include <soc.h>
#include <device.h>
#include <drivers/i2s.h>

#include <audio/codec.h>
#include <audio/dmic.h>
#include "audio_core.h"

#define DMIC_DEV_NAME		"PDM"
#define SPK_OUT_DEV_NAME	"I2S_1"
#define HOST_INOUT_DEV_NAME	"I2S_2"

#define NUM_HOST_CHANNELS	2
#define NUM_SPK_CHANNELS	2
#define NUM_MIC_CHANNELS	8

#define AUDIO_SAMPLE_FREQ	48000
#define AUDIO_SAMPLE_WIDTH	32

#define HOST_FRAME_SAMPLES	(AUDIO_SAMPLES_PER_FRAME * NUM_HOST_CHANNELS)
#define MIC_FRAME_SAMPLES	(AUDIO_SAMPLES_PER_FRAME * NUM_MIC_CHANNELS)
#define SPK_FRAME_SAMPLES	(AUDIO_SAMPLES_PER_FRAME * NUM_SPK_CHANNELS)

#define HOST_FRAME_BYTES	(HOST_FRAME_SAMPLES * AUDIO_SAMPLE_WIDTH / 8)
#define MIC_FRAME_BYTES		(MIC_FRAME_SAMPLES * AUDIO_SAMPLE_WIDTH / 8)
#define SPK_FRAME_BYTES		(SPK_FRAME_SAMPLES * AUDIO_SAMPLE_WIDTH / 8)

#define SPK_OUT_BUF_COUNT	2
#define MIC_IN_BUF_COUNT	2
#define HOST_INOUT_BUF_COUNT	4 /* 2 for TX and 2 for RX */

static void audio_drv_thread(void);

__attribute__((section(".dma_buffers"))) static struct {
	s32_t	host_inout[HOST_INOUT_BUF_COUNT][HOST_FRAME_SAMPLES];
	s32_t	spk_out[SPK_OUT_BUF_COUNT][SPK_FRAME_SAMPLES];
	s32_t	mic_in[MIC_IN_BUF_COUNT][MIC_FRAME_SAMPLES];
} audio_buffers;

static struct device *codec_dev;
static struct device *i2s_spk_out_dev;
static struct device *i2s_host_dev;
static struct device *dmic_device;

static struct k_mem_slab mic_in_mem_slab;
static struct k_mem_slab host_inout_mem_slab;
static struct k_mem_slab spk_out_mem_slab;

K_SEM_DEFINE(audio_drv_sync_sem, 0, 1);
static bool audio_io_started;

K_THREAD_DEFINE(audio_drv_thread_id, AUDIO_DRIVER_THREAD_STACKSIZE,
		audio_drv_thread, NULL, NULL, NULL,
		AUDIO_DRIVER_THREAD_PRIORITY, 0, K_NO_WAIT);

static void audio_driver_process_audio_input(void)
{
	s32_t *host_in_buf;
	s32_t *mic_in_buf;
	size_t size;
	int ret;

	/* read capture input buffer */
	ret = dmic_read(dmic_device, 0, (void **)&mic_in_buf, &size, K_FOREVER);
	if (ret) {
		LOG_ERR("dmic_device read failed %d", ret);
		return;
	}

	/* read playback input buffer */
	ret = i2s_read(i2s_host_dev, (void **)&host_in_buf, &size);
	if (ret) {
		LOG_ERR("i2s_host_dev read failed %d", ret);
		return;
	}

	audio_core_process_mic_source(mic_in_buf, NUM_MIC_CHANNELS);
	audio_core_process_host_source(host_in_buf, NUM_HOST_CHANNELS);

	/* free the consumed buffers */
	k_mem_slab_free(&mic_in_mem_slab, (void **)&mic_in_buf);
	k_mem_slab_free(&host_inout_mem_slab, (void **)&host_in_buf);
}

static void audio_driver_process_audio_output(void)
{
	s32_t *spk_out_buf;
	s32_t *host_out_buf;
	int ret;

	ret = k_mem_slab_alloc(&spk_out_mem_slab, (void *)&spk_out_buf,
			K_NO_WAIT);
	if (ret) {
		LOG_ERR("speaker out buffer alloc failed %d mem_slab %p",
				ret, &spk_out_mem_slab);
	}

	ret = k_mem_slab_alloc(&host_inout_mem_slab, (void *)&host_out_buf,
			K_NO_WAIT);
	if (ret) {
		LOG_ERR("host out audio buffer alloc failed %d", ret);
	}

	audio_core_process_speaker_sink(spk_out_buf, NUM_SPK_CHANNELS);
	audio_core_process_host_sink(host_out_buf, NUM_HOST_CHANNELS);

	/* write buffer */
	ret = i2s_write(i2s_spk_out_dev, (void *)spk_out_buf, SPK_FRAME_BYTES);
	if (ret) {
		LOG_ERR("i2s_write for speaker failed %d", ret);
	}

	ret = i2s_write(i2s_host_dev, (void *)host_out_buf, HOST_FRAME_BYTES);
	if (ret) {
		LOG_ERR("i2s_write for host failed %d", ret);
	}
}

static int audio_driver_send_zeros_frame(void)
{
	int ret = 0;
	void *spk_out_buf;
	void *host_out_buf;

	/* allocate speaker output buffer */
	ret = k_mem_slab_alloc(&spk_out_mem_slab, &spk_out_buf, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Buffer alloc for spk output failed %d", ret);
		return ret;
	}

	/* allocate host output buffer */
	ret = k_mem_slab_alloc(&host_inout_mem_slab, &host_out_buf, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Buffer alloc for host output failed %d", ret);
		return ret;
	}

	/* fill buffer with zeros */
	memset(spk_out_buf, 0, SPK_FRAME_BYTES);
	memset(host_out_buf, 0, HOST_FRAME_BYTES);

	ret = i2s_write(i2s_spk_out_dev, spk_out_buf, SPK_FRAME_BYTES);
	if (ret) {
		LOG_ERR("i2s_write for spk output failed %d", ret);
		return ret;
	}

	ret = i2s_write(i2s_host_dev, host_out_buf, HOST_FRAME_BYTES);
	if (ret) {
		LOG_ERR("i2s_write for host output failed %d", ret);
		return ret;
	}

	return ret;
}

static int audio_driver_start_host_streams(void)
{
	int ret = 0;

	/* trigger transmission */
	ret = i2s_trigger(i2s_host_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret) {
		LOG_ERR("I2S TX failed with code %d", ret);
	}

	/* trigger reception */
	ret = i2s_trigger(i2s_host_dev, I2S_DIR_RX, I2S_TRIGGER_START);
	if (ret) {
		LOG_ERR("I2S RX failed with code %d", ret);
	}

	return ret;
}

static int audio_driver_stop_host_streams(void)
{
	int ret;

	/* stop transmission */
	ret = i2s_trigger(i2s_host_dev, I2S_DIR_TX, I2S_TRIGGER_STOP);
	if (ret) {
		LOG_ERR("I2S TX failed with code %d", ret);
	}

	/* stop reception */
	ret = i2s_trigger(i2s_host_dev, I2S_DIR_RX, I2S_TRIGGER_STOP);
	if (ret) {
		LOG_ERR("I2S RX failed with code %d", ret);
	}

	return ret;
}

static void audio_driver_config_host_streams(void)
{
	int ret;
	struct i2s_config i2s_cfg;

	i2s_host_dev = device_get_binding(HOST_INOUT_DEV_NAME);
	if (!i2s_host_dev) {
		LOG_ERR("unable to find device %s",  HOST_INOUT_DEV_NAME);
		return;
	}

	/* Configure */
	i2s_cfg.word_size = AUDIO_SAMPLE_WIDTH;
	i2s_cfg.channels = NUM_HOST_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S | I2S_FMT_CLK_NF_NB;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg.frame_clk_freq = AUDIO_SAMPLE_FREQ;
	i2s_cfg.block_size = HOST_FRAME_BYTES;
	i2s_cfg.mem_slab = &host_inout_mem_slab;
	i2s_cfg.timeout = K_NO_WAIT;

	k_mem_slab_init(&host_inout_mem_slab, &audio_buffers.host_inout[0][0],
			HOST_FRAME_BYTES, HOST_INOUT_BUF_COUNT);

	/* Configure host input/output I2S */
	ret = i2s_configure(i2s_host_dev, I2S_DIR_RX, &i2s_cfg);
	if (ret != 0) {
		LOG_ERR("i2s_configure RX failed with %d error", ret);
	}
}

static void audio_driver_config_periph_streams(void)
{
	int ret;
	struct i2s_config i2s_cfg;
	struct audio_codec_cfg codec = {
		.dai_type	= AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s	= {
			.word_size	= AUDIO_SAMPLE_WIDTH,
			.channels	= NUM_SPK_CHANNELS,
			.format		= I2S_FMT_DATA_FORMAT_I2S |
				I2S_FMT_CLK_NF_NB,
			.options	= I2S_OPT_FRAME_CLK_SLAVE |
				I2S_OPT_BIT_CLK_SLAVE,
			.frame_clk_freq	= AUDIO_SAMPLE_FREQ,
			.block_size	= SPK_FRAME_BYTES,
			.timeout	= K_NO_WAIT,
		},
	};
	struct pcm_stream_cfg stream = {
		.pcm_rate		= AUDIO_SAMPLE_FREQ,
		.pcm_width		= AUDIO_SAMPLE_WIDTH,
		.block_size		= MIC_FRAME_BYTES,
		.mem_slab		= &mic_in_mem_slab,
	};
	struct dmic_cfg cfg = {
		.io = {
			.min_pdm_clk_freq	= 1024000,
			.max_pdm_clk_freq	= 4800000,
			.min_pdm_clk_dc		= 48,
			.max_pdm_clk_dc		= 52,
			.pdm_clk_pol		= 0,
			.pdm_data_pol		= 0,
		},
		.streams			= &stream,
		.channel = {
			.req_chan_map_lo	=
				dmic_build_channel_map(0, 0, PDM_CHAN_RIGHT) |
				dmic_build_channel_map(1, 0, PDM_CHAN_LEFT) |
				dmic_build_channel_map(2, 2, PDM_CHAN_RIGHT) |
				dmic_build_channel_map(3, 2, PDM_CHAN_LEFT) |
				dmic_build_channel_map(4, 1, PDM_CHAN_RIGHT) |
				dmic_build_channel_map(5, 1, PDM_CHAN_LEFT) |
				dmic_build_channel_map(6, 3, PDM_CHAN_RIGHT) |
				dmic_build_channel_map(7, 3, PDM_CHAN_LEFT),
			.req_num_chan		= NUM_MIC_CHANNELS,
			.req_num_streams	= 1,
		},
	};

	k_mem_slab_init(&mic_in_mem_slab, &audio_buffers.mic_in[0][0],
			MIC_FRAME_BYTES, MIC_IN_BUF_COUNT);
	dmic_device = device_get_binding(DMIC_DEV_NAME);
	if (!dmic_device) {
		LOG_ERR("unable to find device %s", DMIC_DEV_NAME);
		return;
	}

	ret = dmic_configure(dmic_device, &cfg);
	if (ret != 0) {
		LOG_ERR("dmic_configure failed with %d error", ret);
	}

	i2s_spk_out_dev = device_get_binding(SPK_OUT_DEV_NAME);
	if (!i2s_spk_out_dev) {
		LOG_ERR("unable to find device %s",  SPK_OUT_DEV_NAME);
		return;
	}

	codec_dev = device_get_binding(DT_INST_0_TI_TLV320DAC_LABEL);
	if (!codec_dev) {
		LOG_ERR("unable to find device %s", DT_INST_0_TI_TLV320DAC_LABEL);
		return;
	}

	/* Configure */
	i2s_cfg.word_size = AUDIO_SAMPLE_WIDTH;
	i2s_cfg.channels = NUM_SPK_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S | I2S_FMT_CLK_NF_NB;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg.frame_clk_freq = AUDIO_SAMPLE_FREQ;
	i2s_cfg.block_size = SPK_FRAME_BYTES;
	i2s_cfg.mem_slab = &spk_out_mem_slab;
	i2s_cfg.timeout = K_NO_WAIT;
	k_mem_slab_init(&spk_out_mem_slab, &audio_buffers.spk_out[0][0],
			SPK_FRAME_BYTES, SPK_OUT_BUF_COUNT);

	ret = i2s_configure(i2s_spk_out_dev, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		LOG_ERR("i2s_configure TX failed with %d error", ret);
	}

	/* configure codec */
	codec.mclk_freq = soc_get_ref_clk_freq();
	audio_codec_configure(codec_dev, &codec);
}

static void audio_driver_start_periph_streams(void)
{
	int ret;

	/* start codec output */
	audio_codec_start_output(codec_dev);

	ret = i2s_trigger(i2s_spk_out_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret) {
		LOG_ERR("I2S TX failed with code %d", ret);
	}

	ret = dmic_trigger(dmic_device, DMIC_TRIGGER_START);
	if (ret) {
		LOG_ERR("dmic_trigger failed with code %d", ret);
	}
}

static void audio_driver_stop_periph_streams(void)
{
	int ret;

	ret = i2s_trigger(i2s_spk_out_dev, I2S_DIR_TX, I2S_TRIGGER_STOP);
	if (ret) {
		LOG_ERR("I2S TX failed with code %d", ret);
	}

	/* trigger transmission */
	ret = dmic_trigger(dmic_device, DMIC_TRIGGER_STOP);
	if (ret) {
		LOG_ERR("dmic_trigger failed with code %d", ret);
	}
}

int audio_driver_start(void)
{
	if (audio_io_started == true) {
		LOG_INF("Audio I/O already started ...");
		return 0;
	}

	LOG_INF("Starting Audio I/O...");

	/*
	 * start the playback path first followed by the capture path
	 * This ensures that by the time the capture frame tick is
	 * processed, the playback input samples are ready and playback
	 * output samples are transmitted
	 */
	audio_driver_send_zeros_frame();
	audio_driver_send_zeros_frame();

	audio_driver_start_host_streams();
	audio_driver_start_periph_streams();

	audio_io_started = true;
	k_sem_give(&audio_drv_sync_sem);

	return 0;
}

int audio_driver_stop(void)
{
	if (audio_io_started == false) {
		LOG_INF("Audio I/O already stopped ...");
		return 0;
	}

	k_sem_take(&audio_drv_sync_sem, K_FOREVER);
	audio_driver_stop_host_streams();
	audio_driver_stop_periph_streams();
	audio_io_started = false;
	LOG_INF("Stopped Audio I/O...");

	return 0;
}

static void audio_drv_thread(void)
{
	LOG_INF("Starting Audio Driver thread ,,,");

	LOG_INF("Configuring Host Audio Streams ...");
	audio_driver_config_host_streams();

	LOG_INF("Configuring Peripheral Audio Streams ...");
	audio_driver_config_periph_streams();

	LOG_INF("Initializing Audio Core Engine ...");
	audio_core_initialize();

	LOG_DBG("mic_in_mem_slab %p", &mic_in_mem_slab);
	LOG_DBG("spk_out_mem_slab %p", &spk_out_mem_slab);
	LOG_DBG("host_inout_mem_slab %p", &host_inout_mem_slab);

	audio_driver_start();

	while (true) {
		k_sem_take(&audio_drv_sync_sem, K_FOREVER);

		audio_driver_process_audio_input();

		audio_driver_process_audio_output();

		k_sem_give(&audio_drv_sync_sem);

		audio_core_notify_frame_tick();
	}
}
