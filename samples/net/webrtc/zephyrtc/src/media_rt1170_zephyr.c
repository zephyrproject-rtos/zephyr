/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include <errno.h>
#include <fsl_dcdc.h>
#include <fsl_sai.h>
#include <fsl_soc_src.h>
#include <fsl_tempsensor.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/audio/codec.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/i2s.h>

#include "media_stats_zephyr.h"
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
#include <zephyr/drivers/display.h>
#include <zephyr/video/video.h>
#endif
#include <sys_malloc.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/mem_stats.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/util.h>

#include "media.h"
#include "platform.h"
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
#include "video_codec_zephyr.h"
#include "video_color_zephyr.h"
#endif

#define AUDIO_CAPTURE_NODE          DT_ALIAS(dmic0)
#define AUDIO_CODEC_NODE            DT_NODELABEL(audio_codec)
#define AUDIO_I2S_NODE              DT_ALIAS(i2s_codec_tx)
#define AUDIO_SAMPLE_RATE           16000U
#define AUDIO_CAPTURE_CHANNELS      2U
#define AUDIO_PLAYBACK_CHANNELS     2U
#define AUDIO_CAPTURE_SAMPLE_BITS   32U
#define AUDIO_CAPTURE_SAMPLE_BYTES  4U
#define AUDIO_PLAYBACK_SAMPLE_BITS  16U
#define AUDIO_PLAYBACK_SAMPLE_BYTES 2U
#define AUDIO_FRAME_MS              20U
#define AUDIO_CAPTURE_TIMEOUT_MS    25U
#define AUDIO_INPUT_SAMPLES         (AUDIO_SAMPLE_RATE * AUDIO_FRAME_MS / 1000U)
#define AUDIO_PCMU_SAMPLES          (AUDIO_INPUT_SAMPLES / 2U)
#define AUDIO_CAPTURE_BLOCK_BYTES                                                                  \
	(AUDIO_INPUT_SAMPLES * AUDIO_CAPTURE_CHANNELS * AUDIO_CAPTURE_SAMPLE_BYTES)
#define AUDIO_PLAYBACK_BLOCK_BYTES                                                                 \
	(AUDIO_INPUT_SAMPLES * AUDIO_PLAYBACK_CHANNELS * AUDIO_PLAYBACK_SAMPLE_BYTES)
#define AUDIO_CAPTURE_BLOCKS           16U
#define AUDIO_PLAYBACK_BLOCKS          16U
#define AUDIO_PLAYBACK_QUEUE_DEPTH     8U
#define AUDIO_PLAYBACK_WORKER_STACK    8192U
#define AUDIO_PLAYBACK_WORKER_PRIORITY K_PRIO_PREEMPT(4)
#define AUDIO_DURATION_NS              UINT64_C(20000000)

#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
#define VIDEO_CAPTURE_BUFFERS 2U
#define VIDEO_CAPTURE_BYTES   (VIDEO_CODEC_WIDTH * VIDEO_CODEC_HEIGHT * 4U)
#if CONFIG_ZEPHYRTC_DISPLAY_ROTATION == 90 || CONFIG_ZEPHYRTC_DISPLAY_ROTATION == 270
#define VIDEO_DISPLAY_BASE_WIDTH  VIDEO_CODEC_HEIGHT
#define VIDEO_DISPLAY_BASE_HEIGHT VIDEO_CODEC_WIDTH
#else
#define VIDEO_DISPLAY_BASE_WIDTH  VIDEO_CODEC_WIDTH
#define VIDEO_DISPLAY_BASE_HEIGHT VIDEO_CODEC_HEIGHT
#endif
#define VIDEO_DISPLAY_WIDTH  (VIDEO_DISPLAY_BASE_WIDTH * CONFIG_ZEPHYRTC_DISPLAY_SCALE)
#define VIDEO_DISPLAY_HEIGHT (VIDEO_DISPLAY_BASE_HEIGHT * CONFIG_ZEPHYRTC_DISPLAY_SCALE)
#define VIDEO_RGB565_BYTES   (VIDEO_DISPLAY_WIDTH * VIDEO_DISPLAY_HEIGHT * sizeof(uint16_t))
#if defined(ZEPHYRTC_DIAGNOSTIC_VIDEO_FRAME_MS)
#define VIDEO_FRAME_MS ZEPHYRTC_DIAGNOSTIC_VIDEO_FRAME_MS
#else
#define VIDEO_FRAME_MS 200U
#endif
#define VIDEO_DURATION_NS     (UINT64_C(1000000) * VIDEO_FRAME_MS)
#define VIDEO_RX_QUEUE_DEPTH  8U
#define VIDEO_WORKER_STACK    65536U
#define VIDEO_WORKER_PRIORITY K_PRIO_PREEMPT(6)

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_camera), "zephyr,camera must be chosen");
BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_display), "zephyr,display must be chosen");
#endif

BUILD_ASSERT(DT_NODE_HAS_STATUS(AUDIO_CAPTURE_NODE, okay), "dmic0 must be enabled");
BUILD_ASSERT(DT_NODE_HAS_STATUS(AUDIO_CODEC_NODE, okay), "audio_codec must be enabled");
BUILD_ASSERT(DT_NODE_HAS_STATUS(AUDIO_I2S_NODE, okay), "i2s_codec_tx must be enabled");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(AUDIO_CODEC_NODE, wolfson_wm8962), "audio_codec must be WM8962");
BUILD_ASSERT(DT_PROP(AUDIO_CAPTURE_NODE, sample_rate) == AUDIO_SAMPLE_RATE,
	     "MICFIL sample rate must be 16 kHz");

#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
struct encoded_video_nal {
	uint64_t sequence;
	uint32_t checksum;
	size_t size;
	uint8_t data[VIDEO_CODEC_MAX_NAL_BYTES];
};
#endif

struct Media {
	const struct device *codec;
	const struct device *dmic;
	const struct device *i2s;
	uint8_t encoded[AUDIO_PCMU_SAMPLES];
	uint64_t capture_index;
	uint64_t generated_audio;
	uint64_t late_audio;
	uint64_t received_audio;
	uint64_t decoded_audio;
	uint64_t decoded_video;
	uint64_t dropped_audio;
	uint64_t dropped_video;
	struct k_thread playback_worker;
	atomic_t playback_worker_stop;
	bool playback_worker_started;
	bool sai_started;
	atomic_t failure;
	bool started;
	bool borrowed[2];
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	const struct device *camera;
	const struct device *display;
	uint16_t display_x;
	uint16_t display_y;
	struct video_format camera_format;
	struct video_buffer *camera_buffers[VIDEO_CAPTURE_BUFFERS];
	uint8_t camera_memory[VIDEO_CAPTURE_BUFFERS][VIDEO_CAPTURE_BYTES] __aligned(64);
	uint8_t video_i420[VIDEO_CODEC_I420_BYTES] __aligned(64);
#if defined(ZEPHYRTC_DIAGNOSTIC_ENCODER_FIXED_FIRST_FRAME)
	uint8_t diagnostic_encoder_i420[VIDEO_CODEC_I420_BYTES] __aligned(64);
#endif
	uint16_t display_rgb565[VIDEO_DISPLAY_WIDTH * VIDEO_DISPLAY_HEIGHT] __aligned(64);
	uint64_t next_video_ms;
	uint64_t video_index;
	uint64_t video_nal_sequence;
#if defined(ZEPHYRTC_DIAGNOSTIC_DISABLE_ENCODER_PROCESSING)
	const uint8_t *diagnostic_encoded_video;
	size_t diagnostic_encoded_video_size;
#endif
	struct k_thread video_worker;
	atomic_t video_worker_stop;
	bool video_worker_started;
#endif
};

K_MEM_SLAB_DEFINE_STATIC(capture_slab, AUDIO_CAPTURE_BLOCK_BYTES, AUDIO_CAPTURE_BLOCKS, 4);
K_MEM_SLAB_DEFINE_IN_SECT_STATIC(playback_slab, __nocache, AUDIO_PLAYBACK_BLOCK_BYTES,
				 AUDIO_PLAYBACK_BLOCKS, 4);
K_MSGQ_DEFINE(playback_pcmu_queue, AUDIO_PCMU_SAMPLES, AUDIO_PLAYBACK_QUEUE_DEPTH, 4);
K_THREAD_STACK_DEFINE(playback_worker_stack, AUDIO_PLAYBACK_WORKER_STACK);
static int16_t playback_samples[AUDIO_INPUT_SAMPLES * AUDIO_PLAYBACK_CHANNELS] __nocache
	__aligned(32);
K_SEM_DEFINE(playback_dma_done, 0, 1);
static atomic_t playback_dma_status;
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
K_MEM_SLAB_DEFINE_STATIC(video_rx_slab, ROUND_UP(sizeof(struct encoded_video_nal), 64),
			 VIDEO_RX_QUEUE_DEPTH, 64);
K_MSGQ_DEFINE(video_rx_queue, sizeof(struct encoded_video_nal *), VIDEO_RX_QUEUE_DEPTH,
	      sizeof(void *));
Z_KERNEL_STACK_DEFINE_IN(video_worker_stack, VIDEO_WORKER_STACK, __dtcm_noinit_section);
extern struct k_heap h264swdec_heap;
#endif

static struct Media hardware_media;
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
static bool video_cleanup_failed;
#endif

static int32_t read_die_temperature_millidegrees(void)
{
	float temperature;

	TMPSNS_StopMeasure(TMPSNS);
	TMPSNS_StartMeasure(TMPSNS);
	temperature = TMPSNS_GetCurrentTemperature(TMPSNS);
	return (int32_t)(temperature * 1000.0f);
}

static void configure_die_temperature_monitor(void)
{
	tmpsns_config_t config;

	TMPSNS_GetDefaultConfig(&config);
	config.lowAlarmTemp = 0;
	config.highAlarmTemp = 85;
	config.panicAlarmTemp = 95;
	TMPSNS_Init(TMPSNS, &config);
	SRC_SetGlobalSystemResetMode(SRC, kSRC_TempsenseReset, kSRC_ResetSystem);
	platform_log("info", "die_temperature", "millidegrees=%d high_alarm=%d panic_alarm=%d",
		     read_die_temperature_millidegrees(), config.highAlarmTemp,
		     config.panicAlarmTemp);
}

static uint8_t linear_to_mulaw(int16_t input)
{
	static const int16_t segment_end[] = {0x00ff, 0x01ff, 0x03ff, 0x07ff,
					      0x0fff, 0x1fff, 0x3fff, 0x7fff};
	int32_t sample = input;
	int32_t mask;
	int32_t segment;
	uint8_t value;

	if (sample < 0) {
		sample = 0x84 - sample;
		mask = 0x7f;
	} else {
		sample += 0x84;
		mask = 0xff;
	}
	if (sample > 32635) {
		sample = 32635;
	}
	for (segment = 0; segment < 8; segment++) {
		if (sample <= segment_end[segment]) {
			break;
		}
	}
	if (segment >= 8) {
		return (uint8_t)(0x7f ^ mask);
	}
	value = (uint8_t)((segment << 4) | ((sample >> (segment + 3)) & 0x0f));
	return (uint8_t)(value ^ mask);
}

static int16_t mulaw_to_linear(uint8_t input)
{
	uint8_t value = (uint8_t)~input;
	int32_t sample = ((int32_t)(value & 0x0f) << 3) + 0x84;

	sample <<= (value & 0x70) >> 4;
	return (int16_t)((value & 0x80) != 0U ? 0x84 - sample : sample - 0x84);
}

static int configure_codec(struct Media *media)
{
	struct audio_codec_cfg config = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
		.dai_cfg.i2s.word_size = AUDIO_PLAYBACK_SAMPLE_BITS,
		.dai_cfg.i2s.channels = AUDIO_PLAYBACK_CHANNELS,
		.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S,
		.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET,
		.dai_cfg.i2s.frame_clk_freq = AUDIO_SAMPLE_RATE,
		.dai_cfg.i2s.mem_slab = &playback_slab,
		.dai_cfg.i2s.block_size = AUDIO_PLAYBACK_BLOCK_BYTES,
		.mclk_freq = 0U,
	};

	return audio_codec_configure(media->codec, &config);
}

static int configure_capture(struct Media *media)
{
	static struct pcm_stream_cfg stream = {
		.pcm_width = AUDIO_CAPTURE_SAMPLE_BITS,
		.mem_slab = &capture_slab,
	};
	struct dmic_cfg config = {
		.io.min_pdm_clk_freq = 1000000U,
		.io.max_pdm_clk_freq = 3500000U,
		.io.min_pdm_clk_dc = 40U,
		.io.max_pdm_clk_dc = 60U,
		.streams = &stream,
		.channel.req_num_streams = 1U,
		.channel.req_num_chan = AUDIO_CAPTURE_CHANNELS,
		.channel.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT) |
					   dmic_build_channel_map(1, 0, PDM_CHAN_RIGHT),
	};

	stream.pcm_rate = AUDIO_SAMPLE_RATE;
	stream.block_size = AUDIO_CAPTURE_BLOCK_BYTES;
	return dmic_configure(media->dmic, &config);
}

static int configure_playback(struct Media *media)
{
	struct i2s_config config = {
		.word_size = AUDIO_PLAYBACK_SAMPLE_BITS,
		.channels = AUDIO_PLAYBACK_CHANNELS,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER,
		.frame_clk_freq = AUDIO_SAMPLE_RATE,
		.mem_slab = &playback_slab,
		.block_size = AUDIO_PLAYBACK_BLOCK_BYTES,
		.timeout = 0,
	};

	return i2s_configure(media->i2s, I2S_DIR_TX, &config);
}

static void decode_playback(const uint8_t *encoded, int16_t *samples)
{
	for (size_t index = 0U; index < AUDIO_PCMU_SAMPLES; index++) {
		int16_t current = mulaw_to_linear(encoded[index]);
		int16_t next = index + 1U < AUDIO_PCMU_SAMPLES
				       ? mulaw_to_linear(encoded[index + 1U])
				       : current;
		int16_t interpolated = (int16_t)(((int32_t)current + next) / 2);
		size_t output = index * 4U;

		samples[output] = current;
		samples[output + 1U] = current;
		samples[output + 2U] = interpolated;
		samples[output + 3U] = interpolated;
	}
}

static void playback_dma_callback(const struct device *dma, void *user_data, uint32_t channel,
				  int status)
{
	ARG_UNUSED(dma);
	ARG_UNUSED(user_data);
	ARG_UNUSED(channel);
	atomic_set(&playback_dma_status, status);
	k_sem_give(&playback_dma_done);
}

static void playback_worker_entry(void *arg1, void *arg2, void *arg3)
{
	struct Media *media = arg1;
	I2S_Type *base = (I2S_Type *)DT_REG_ADDR(AUDIO_I2S_NODE);
	const struct device *dma = DEVICE_DT_GET(DT_DMAS_CTLR_BY_NAME(AUDIO_I2S_NODE, tx));
	const uint32_t dma_channel = DT_PROP(AUDIO_I2S_NODE, nxp_tx_dma_channel);
	struct dma_block_config block = {
		.source_address = (uintptr_t)playback_samples,
		.dest_address = (uintptr_t)&base->TDR[0],
		.block_size = AUDIO_PLAYBACK_BLOCK_BYTES,
		.dest_scatter_en = 1,
	};
	struct dma_config config = {
		.dma_slot = DT_DMAS_CELL_BY_NAME(AUDIO_I2S_NODE, tx, source),
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.source_data_size = AUDIO_PLAYBACK_SAMPLE_BYTES,
		.dest_data_size = AUDIO_PLAYBACK_SAMPLE_BYTES,
		.source_burst_length = AUDIO_PLAYBACK_SAMPLE_BYTES,
		.dest_burst_length = AUDIO_PLAYBACK_SAMPLE_BYTES,
		.block_count = 1,
		.head_block = &block,
		.dma_callback = playback_dma_callback,
		.user_data = media,
		.complete_callback_en = 1,
	};
	uint8_t encoded[AUDIO_PCMU_SAMPLES];
	int result;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	if (!device_is_ready(dma)) {
		atomic_set(&media->failure, 1);
		return;
	}
	while (atomic_get(&media->playback_worker_stop) == 0) {
		if (k_msgq_get(&playback_pcmu_queue, encoded, K_MSEC(50)) != 0) {
			continue;
		}
		decode_playback(encoded, playback_samples);
		k_sem_reset(&playback_dma_done);
		atomic_set(&playback_dma_status, 0);
		result = dma_config(dma, dma_channel, &config);
		if (result != 0) {
			platform_log("error", "playback_dma_config",
				     "status=fail result=%d decoded=%llu", result,
				     (unsigned long long)media->decoded_audio);
		} else {
			result = dma_start(dma, dma_channel);
			if (result != 0) {
				platform_log("error", "playback_dma_start",
					     "status=fail result=%d decoded=%llu", result,
					     (unsigned long long)media->decoded_audio);
			}
		}
		if (result != 0) {
			atomic_set(&media->failure, 1);
			return;
		}
		if (!media->sai_started) {
			base->TCSR |= I2S_TCSR_FR_MASK;
			SAI_TxEnableDMA(base, kSAI_FIFORequestDMAEnable, true);
			SAI_TxSetChannelFIFOMask(base, DT_PROP(AUDIO_I2S_NODE, nxp_tx_channel));
			SAI_TxEnable(base, true);
			media->sai_started = true;
		}
		result = k_sem_take(&playback_dma_done, K_MSEC(50));
		if (result != 0 || atomic_get(&playback_dma_status) != 0) {
			(void)dma_stop(dma, dma_channel);
			atomic_set(&media->failure, 1);
			return;
		}
		media->decoded_audio++;
	}
}

static void start_playback_worker(struct Media *media)
{
	k_msgq_purge(&playback_pcmu_queue);
	atomic_set(&media->playback_worker_stop, 0);
	(void)k_thread_create(&media->playback_worker, playback_worker_stack,
			      K_THREAD_STACK_SIZEOF(playback_worker_stack), playback_worker_entry,
			      media, NULL, NULL, AUDIO_PLAYBACK_WORKER_PRIORITY, 0, K_NO_WAIT);
	media->playback_worker_started = true;
}

static int stop_playback_worker(struct Media *media)
{
	I2S_Type *base = (I2S_Type *)DT_REG_ADDR(AUDIO_I2S_NODE);
	const struct device *dma = DEVICE_DT_GET(DT_DMAS_CTLR_BY_NAME(AUDIO_I2S_NODE, tx));
	const uint32_t dma_channel = DT_PROP(AUDIO_I2S_NODE, nxp_tx_dma_channel);
	int result;

	if (!media->playback_worker_started) {
		k_msgq_purge(&playback_pcmu_queue);
		return 0;
	}
	atomic_set(&media->playback_worker_stop, 1);
	(void)dma_stop(dma, dma_channel);
	k_sem_give(&playback_dma_done);
	result = k_thread_join(&media->playback_worker, K_SECONDS(1));
	if (result != 0) {
		return result;
	}
	if (media->sai_started) {
		SAI_TxEnableDMA(base, kSAI_FIFORequestDMAEnable, false);
		SAI_TxEnable(base, false);
		media->sai_started = false;
	}
	k_msgq_purge(&playback_pcmu_queue);
	media->playback_worker_started = false;
	return 0;
}

static int queue_playback(struct Media *media, const uint8_t *encoded)
{
	ARG_UNUSED(media);
	return k_msgq_put(&playback_pcmu_queue, encoded, K_NO_WAIT) == 0 ? 0 : -EAGAIN;
}

#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
static int receive_video(struct Media *media, const struct encoded_video_nal *queued);

static void drain_video_rx_queue(void)
{
	struct encoded_video_nal *queued;

	while (k_msgq_get(&video_rx_queue, &queued, K_NO_WAIT) == 0) {
		k_mem_slab_free(&video_rx_slab, queued);
	}
}

static void video_worker_entry(void *arg1, void *arg2, void *arg3)
{
	struct Media *media = arg1;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	for (;;) {
		struct encoded_video_nal *queued;
		int result;

		if (k_msgq_get(&video_rx_queue, &queued, K_MSEC(50)) != 0) {
			if (atomic_get(&media->video_worker_stop) != 0) {
				break;
			}
			continue;
		}
		result = receive_video(media, queued);
		k_mem_slab_free(&video_rx_slab, queued);
		if (result != 0) {
			platform_log("error", "video_worker", "status=fail result=%d", result);
			atomic_set(&media->failure, 1);
		}
	}
}

static void start_video_worker(struct Media *media)
{
	drain_video_rx_queue();
	atomic_set(&media->video_worker_stop, 0);
	(void)k_thread_create(&media->video_worker, video_worker_stack,
			      K_THREAD_STACK_SIZEOF(video_worker_stack), video_worker_entry, media,
			      NULL, NULL, VIDEO_WORKER_PRIORITY, 0, K_NO_WAIT);
	media->video_worker_started = true;
}

static int stop_video_worker(struct Media *media)
{
	int result;

	if (!media->video_worker_started) {
		drain_video_rx_queue();
		return 0;
	}
	atomic_set(&media->video_worker_stop, 1);
	result = k_thread_join(&media->video_worker, K_SECONDS(5));
	if (result != 0) {
		return result;
	}
	media->video_worker_started = false;
	drain_video_rx_queue();
	return 0;
}

static void release_camera_buffer(struct Media *media, struct video_buffer *buffer)
{
	for (size_t index = 0U; index < VIDEO_CAPTURE_BUFFERS; index++) {
		if (media->camera_buffers[index] == buffer) {
			media->camera_buffers[index] = NULL;
			break;
		}
	}
	if (video_buffer_release(buffer) != 0) {
		platform_log("error", "camera_buffer_release", "status=fail");
	}
}

static int cleanup_video(struct Media *media)
{
	bool has_camera_buffer = false;
	int result;

	result = stop_video_worker(media);
	if (result != 0) {
		platform_log("error", "video_worker_stop", "status=fail result=%d", result);
		video_cleanup_failed = true;
		return result;
	}
	for (size_t index = 0U; index < VIDEO_CAPTURE_BUFFERS; index++) {
		has_camera_buffer = has_camera_buffer || media->camera_buffers[index] != NULL;
	}
	if (media->camera != NULL && has_camera_buffer) {
		struct video_buffer *buffer;

		result = video_stream_stop(media->camera, VIDEO_BUF_TYPE_OUTPUT);
		if (result != 0) {
			platform_log("error", "camera_stop", "status=fail result=%d", result);
			video_cleanup_failed = true;
			goto finish;
		}
		while (video_dequeue(media->camera, &buffer, K_NO_WAIT) == 0) {
			release_camera_buffer(media, buffer);
		}
		for (size_t index = 0U; index < VIDEO_CAPTURE_BUFFERS; index++) {
			if (media->camera_buffers[index] != NULL) {
				release_camera_buffer(media, media->camera_buffers[index]);
			}
		}
	}
	video_cleanup_failed = false;
	result = 0;

finish:
	if (media->display != NULL && device_is_ready(media->display)) {
		int blanking_result = display_blanking_on(media->display);

		if (blanking_result != 0 && blanking_result != -ENOSYS) {
			platform_log("error", "display_blanking", "status=fail result=%d",
				     blanking_result);
		}
	}
	video_codec_deinit();
	return result;
}

static uint32_t video_checksum(const uint8_t *data, size_t size)
{
	uint32_t hash = 2166136261U;

	for (size_t index = 0U; index < size; index++) {
		hash = (hash ^ data[index]) * 16777619U;
	}
	return hash;
}

static int queue_received_video(struct Media *media, const MediaFrame *frame)
{
	struct encoded_video_nal *queued;
	int result;

	if (frame->size > VIDEO_CODEC_MAX_NAL_BYTES) {
		return -E2BIG;
	}
	result = k_mem_slab_alloc(&video_rx_slab, (void **)&queued, K_NO_WAIT);
	if (result != 0) {
		return result;
	}
	queued->sequence = media->video_nal_sequence++;
	queued->checksum = video_checksum(frame->data, frame->size);
	queued->size = frame->size;
	memcpy(queued->data, frame->data, frame->size);
	result = k_msgq_put(&video_rx_queue, &queued, K_NO_WAIT);
	if (result != 0) {
		k_mem_slab_free(&video_rx_slab, queued);
	}
	return result;
}

static int configure_video(struct Media *media)
{
	struct display_capabilities display_caps;
	struct video_caps camera_caps = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
	};
	int result;

	media->camera = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));
	media->display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(media->camera) || !device_is_ready(media->display)) {
		return -ENODEV;
	}
	result = video_codec_init();
	if (result != 0) {
		return result;
	}
#if !defined(ZEPHYRTC_DIAGNOSTIC_DISABLE_CAMERA)
	result = video_get_caps(media->camera, &camera_caps);
	if (result != 0 || camera_caps.min_vbuf_count > VIDEO_CAPTURE_BUFFERS) {
		result = result != 0 ? result : -ENOMEM;
		goto fail;
	}
	media->camera_format.type = VIDEO_BUF_TYPE_OUTPUT;
	result = video_get_format(media->camera, &media->camera_format);
	if (result != 0) {
		goto fail;
	}
	media->camera_format.pixelformat = VIDEO_PIX_FMT_BGRX32;
	media->camera_format.width = VIDEO_CODEC_WIDTH;
	media->camera_format.height = VIDEO_CODEC_HEIGHT;
	result = video_set_compose_format(media->camera, &media->camera_format);
	if (result != 0) {
		goto fail;
	}
	if (media->camera_format.size != VIDEO_CAPTURE_BYTES ||
	    media->camera_format.pitch != VIDEO_CODEC_WIDTH * 4U) {
		result = -ENOTSUP;
		goto fail;
	}
	for (size_t index = 0U; index < VIDEO_CAPTURE_BUFFERS; index++) {
		media->camera_buffers[index] =
			video_import_buffer(media->camera_memory[index], VIDEO_CAPTURE_BYTES);
		if (media->camera_buffers[index] == NULL) {
			result = -ENOMEM;
			goto fail;
		}
		media->camera_buffers[index]->type = VIDEO_BUF_TYPE_OUTPUT;
		result = video_enqueue(media->camera, media->camera_buffers[index]);
		if (result != 0) {
			goto fail;
		}
	}
#endif

	display_get_capabilities(media->display, &display_caps);
	if (display_caps.x_resolution < VIDEO_DISPLAY_WIDTH ||
	    display_caps.y_resolution < VIDEO_DISPLAY_HEIGHT) {
		platform_log("error", "video_display_geometry",
			     "status=fail panel=%ux%u output=%ux%u rotation=%d scale=%d",
			     display_caps.x_resolution, display_caps.y_resolution,
			     VIDEO_DISPLAY_WIDTH, VIDEO_DISPLAY_HEIGHT,
			     CONFIG_ZEPHYRTC_DISPLAY_ROTATION, CONFIG_ZEPHYRTC_DISPLAY_SCALE);
		result = -ENOTSUP;
		goto fail;
	}
	if (display_caps.current_pixel_format != PIXEL_FORMAT_RGB_565) {
		result = display_set_pixel_format(media->display, PIXEL_FORMAT_RGB_565);
		if (result != 0) {
			goto fail;
		}
	}
	media->display_x = (display_caps.x_resolution - VIDEO_DISPLAY_WIDTH) / 2U;
	media->display_y = (display_caps.y_resolution - VIDEO_DISPLAY_HEIGHT) / 2U;
#if defined(ZEPHYRTC_DIAGNOSTIC_DISABLE_DISPLAY)
	result = display_blanking_on(media->display);
#else
	result = display_blanking_off(media->display);
#endif
	if (result == -ENOSYS) {
		result = 0;
	}
	if (result != 0) {
		goto fail;
	}
	start_video_worker(media);
	platform_log("info", "video_devices",
		     "camera=%s format=BGRX32 size=%u display=%s origin=%u,%u "
		     "output=%ux%u rotation=%d scale=%d",
		     media->camera->name, media->camera_format.size, media->display->name,
		     media->display_x, media->display_y, VIDEO_DISPLAY_WIDTH, VIDEO_DISPLAY_HEIGHT,
		     CONFIG_ZEPHYRTC_DISPLAY_ROTATION, CONFIG_ZEPHYRTC_DISPLAY_SCALE);
	return 0;

fail:
	platform_log("error", "video_device_config", "status=fail result=%d", result);
	(void)cleanup_video(media);
	return result;
}

static bool pull_video(struct Media *media, MediaFrame *frame)
{
	const uint8_t *encoded = NULL;
	size_t encoded_size = 0U;
	size_t source_bytes = 0U;
	uint64_t now;
	int result;

	if (media->borrowed[MEDIA_VIDEO]) {
		return false;
	}
#if defined(ZEPHYRTC_DIAGNOSTIC_DISABLE_CAMERA)
	now = platform_now_ms();
	if (now < media->next_video_ms) {
		return false;
	}
	if (media->video_index == 0U) {
		memset(media->video_i420, 128, VIDEO_CODEC_I420_BYTES);
	}
#if defined(ZEPHYRTC_DIAGNOSTIC_DISABLE_ENCODER_PROCESSING)
	if (media->video_index == 0U) {
		result = video_codec_encode(media->video_i420, &encoded, &encoded_size);
		media->diagnostic_encoded_video = encoded;
		media->diagnostic_encoded_video_size = encoded_size;
	} else {
		encoded = media->diagnostic_encoded_video;
		encoded_size = media->diagnostic_encoded_video_size;
		result = 0;
	}
#else
	result = video_codec_encode(media->video_i420, &encoded, &encoded_size);
#endif
#else
	struct video_buffer *capture;
	bool emit;

	result = video_dequeue(media->camera, &capture, K_NO_WAIT);
	if (result != 0) {
		if (result != -EAGAIN && result != -EBUSY) {
			platform_log("error", "camera_dequeue", "status=fail result=%d", result);
			atomic_set(&media->failure, 1);
		}
		return false;
	}
	now = platform_now_ms();
	emit = now >= media->next_video_ms;
	source_bytes = capture->bytesused;
	if (emit) {
		if (capture->bytesused != VIDEO_CAPTURE_BYTES) {
			result = -EMSGSIZE;
		} else {
#if defined(ZEPHYRTC_DIAGNOSTIC_CAMERA_NO_CONVERSION)
			if (media->video_index == 0U) {
				memset(media->video_i420, 128, VIDEO_CODEC_I420_BYTES);
			}
			result = video_codec_encode(media->video_i420, &encoded, &encoded_size);
#else
			video_color_bgrx32_to_i420(capture->buffer, media->video_i420);
#if defined(ZEPHYRTC_DIAGNOSTIC_ENCODER_FIXED_FIRST_FRAME)
			if (media->video_index == 0U) {
				memcpy(media->diagnostic_encoder_i420, media->video_i420,
				       VIDEO_CODEC_I420_BYTES);
			}
			result = video_codec_encode(media->diagnostic_encoder_i420, &encoded,
						    &encoded_size);
#elif defined(ZEPHYRTC_DIAGNOSTIC_DISABLE_ENCODER_PROCESSING)
			if (media->video_index == 0U) {
				result = video_codec_encode(media->video_i420, &encoded,
							    &encoded_size);
				media->diagnostic_encoded_video = encoded;
				media->diagnostic_encoded_video_size = encoded_size;
			} else {
				encoded = media->diagnostic_encoded_video;
				encoded_size = media->diagnostic_encoded_video_size;
				result = 0;
			}
#else
			result = video_codec_encode(media->video_i420, &encoded, &encoded_size);
#endif
#endif
		}
	}
	if (video_enqueue(media->camera, capture) != 0) {
		platform_log("error", "camera_enqueue", "status=fail");
		atomic_set(&media->failure, 1);
		return false;
	}
	if (!emit) {
		return false;
	}
#endif
	if (result != 0) {
		platform_log("error", "camera_encode", "status=fail result=%d bytes=%zu", result,
			     source_bytes);
		atomic_set(&media->failure, 1);
		return false;
	}
	*frame = (MediaFrame){
		.data = encoded,
		.size = encoded_size,
		.timestamp_ns = media->video_index * VIDEO_DURATION_NS,
		.duration_ns = VIDEO_DURATION_NS,
	};
	media->video_index++;
	media->next_video_ms = now + VIDEO_FRAME_MS;
	media->borrowed[MEDIA_VIDEO] = true;
	return true;
}

static void log_video_memory_stats(const struct Media *media)
{
	struct video_codec_stats codec_stats;
	struct sys_memory_stats decoder_stats;
	struct sys_memory_stats malloc_stats;
	int decoder_result = sys_heap_runtime_stats_get(&h264swdec_heap.heap, &decoder_stats);
	int malloc_result = malloc_runtime_stats_get(&malloc_stats);

	video_codec_get_stats(&codec_stats);
	platform_log(decoder_result == 0 && malloc_result == 0 ? "info" : "warning", "video_memory",
		     "decoded=%llu decoder_alloc=%zu decoder_free=%zu decoder_peak=%zu "
		     "malloc_alloc=%zu malloc_free=%zu malloc_peak=%zu results=%d/%d "
		     "decode_errors=%llu resets=%llu awaiting_idr=%llu recoveries=%llu "
		     "concealed=%llu replays=%llu",
		     (unsigned long long)media->decoded_video,
		     decoder_result == 0 ? decoder_stats.allocated_bytes : 0U,
		     decoder_result == 0 ? decoder_stats.free_bytes : 0U,
		     decoder_result == 0 ? decoder_stats.max_allocated_bytes : 0U,
		     malloc_result == 0 ? malloc_stats.allocated_bytes : 0U,
		     malloc_result == 0 ? malloc_stats.free_bytes : 0U,
		     malloc_result == 0 ? malloc_stats.max_allocated_bytes : 0U, decoder_result,
		     malloc_result, (unsigned long long)codec_stats.decode_errors,
		     (unsigned long long)codec_stats.decoder_resets,
		     (unsigned long long)codec_stats.awaiting_idr_drops,
		     (unsigned long long)codec_stats.recovered_idrs,
		     (unsigned long long)codec_stats.concealed_pictures,
		     (unsigned long long)codec_stats.parameter_set_replays);
	platform_log("info", "die_temperature", "millidegrees=%d decoded=%llu",
		     read_die_temperature_millidegrees(), (unsigned long long)media->decoded_video);
	platform_log("info", "dcdc_status",
		     "reg0=%08x reg1=%08x reg2=%08x reg3=%08x ctrl0=%08x "
		     "ctrl1=%08x",
		     (unsigned int)DCDC->REG0, (unsigned int)DCDC->REG1, (unsigned int)DCDC->REG2,
		     (unsigned int)DCDC->REG3, (unsigned int)DCDC->CTRL0,
		     (unsigned int)DCDC->CTRL1);
}

static int receive_video(struct Media *media, const struct encoded_video_nal *queued)
{
#if defined(ZEPHYRTC_DIAGNOSTIC_DISABLE_DECODER_PROCESSING)
	ARG_UNUSED(queued);
	media->decoded_video++;
	if (media->decoded_video % 100U == 0U) {
		platform_log("info", "video_worker_progress", "decoded=%llu dropped=%llu",
			     (unsigned long long)media->decoded_video,
			     (unsigned long long)media->dropped_video);
		log_video_memory_stats(media);
	}
	return 0;
#else
	struct display_buffer_descriptor descriptor = {
		.buf_size = VIDEO_RGB565_BYTES,
		.width = VIDEO_DISPLAY_WIDTH,
		.pitch = VIDEO_DISPLAY_WIDTH,
		.height = VIDEO_DISPLAY_HEIGHT,
	};
	const uint8_t *i420;
	size_t i420_size;
	bool picture_ready;
	int result;

	result = video_codec_decode(queued->data, queued->size, queued->sequence, &i420, &i420_size,
				    &picture_ready);
	if (result != 0) {
		media->dropped_video++;
		platform_log("warning", "video_decode_drop",
			     "sequence=%llu size=%zu checksum=%08x result=%d dropped=%llu",
			     (unsigned long long)queued->sequence, queued->size, queued->checksum,
			     result, (unsigned long long)media->dropped_video);
		return 0;
	}
	if (!picture_ready) {
		return 0;
	}
	if (i420 == NULL || i420_size != VIDEO_CODEC_I420_BYTES) {
		return -EBADMSG;
	}
	result = video_color_i420_to_rgb565(i420, media->display_rgb565,
					    CONFIG_ZEPHYRTC_DISPLAY_ROTATION,
					    CONFIG_ZEPHYRTC_DISPLAY_SCALE);
	if (result != 0) {
		return result;
	}
#if defined(ZEPHYRTC_DIAGNOSTIC_DISABLE_DISPLAY)
	result = 0;
#else
	result = display_write(media->display, media->display_x, media->display_y, &descriptor,
			       media->display_rgb565);
#endif
	if (result == 0) {
		media->decoded_video++;
		if (media->decoded_video % 100U == 0U) {
			struct video_codec_stats codec_stats;

			video_codec_get_stats(&codec_stats);
			platform_log("info", "video_worker_progress",
				     "decoded=%llu dropped=%llu decode_errors=%llu resets=%llu "
				     "awaiting_idr=%llu recoveries=%llu concealed=%llu",
				     (unsigned long long)media->decoded_video,
				     (unsigned long long)media->dropped_video,
				     (unsigned long long)codec_stats.decode_errors,
				     (unsigned long long)codec_stats.decoder_resets,
				     (unsigned long long)codec_stats.awaiting_idr_drops,
				     (unsigned long long)codec_stats.recovered_idrs,
				     (unsigned long long)codec_stats.concealed_pictures);
			log_video_memory_stats(media);
		}
	}
	return result;
#endif
}
#endif

Media *media_create(const MediaConfig *config)
{
	struct Media *media = &hardware_media;

	if (config == NULL || config->test_sources || config->fake_sinks ||
	    config->audio_only == IS_ENABLED(CONFIG_ZEPHYRTC_VIDEO_H264)) {
		return NULL;
	}
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	if (video_cleanup_failed && cleanup_video(media) != 0) {
		return NULL;
	}
#endif
	memset(media, 0, sizeof(*media));
	configure_die_temperature_monitor();
	media->codec = DEVICE_DT_GET(AUDIO_CODEC_NODE);
	media->dmic = DEVICE_DT_GET(AUDIO_CAPTURE_NODE);
	media->i2s = DEVICE_DT_GET(AUDIO_I2S_NODE);
	if (!device_is_ready(media->codec) || !device_is_ready(media->dmic) ||
	    !device_is_ready(media->i2s)) {
		return NULL;
	}
	if (configure_codec(media) != 0 || configure_capture(media) != 0 ||
	    configure_playback(media) != 0) {
		return NULL;
	}
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	if (configure_video(media) != 0) {
		return NULL;
	}
#endif
	return media;
}

bool media_enabled(const Media *media, MediaKind kind)
{
	return media != NULL && (kind == MEDIA_AUDIO ||
				 (kind == MEDIA_VIDEO && IS_ENABLED(CONFIG_ZEPHYRTC_VIDEO_H264)));
}

int media_start(Media *media)
{
	int result;

	if (media == NULL || media->started) {
		return -EINVAL;
	}
	audio_codec_start_output(media->codec);
	start_playback_worker(media);
	result = dmic_trigger(media->dmic, DMIC_TRIGGER_START);
	if (result != 0) {
		goto stop_playback;
	}
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
#if !defined(ZEPHYRTC_DIAGNOSTIC_DISABLE_CAMERA)
	result = video_stream_start(media->camera, VIDEO_BUF_TYPE_OUTPUT);
	if (result != 0) {
		goto stop_capture;
	}
#endif
	media->next_video_ms = platform_now_ms();
#endif
	media->started = true;
	return 0;

#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
stop_capture:
	(void)dmic_trigger(media->dmic, DMIC_TRIGGER_STOP);
#endif
stop_playback:
	(void)stop_playback_worker(media);
	(void)audio_codec_stop_output(media->codec);
	return result;
}

int media_poll(Media *media)
{
	if (media == NULL || !media->started) {
		return -EINVAL;
	}
	if (atomic_get(&media->failure) != 0) {
		platform_log("error", "media_poll", "status=failure_latched");
		return -EIO;
	}
	return 0;
}

static bool block_is_zero(const void *block, size_t size)
{
	const uint8_t *bytes = block;

	for (size_t index = 0U; index < size; index++) {
		if (bytes[index] != 0U) {
			return false;
		}
	}
	return true;
}

bool media_pull(Media *media, MediaKind kind, MediaFrame *frame)
{
	uint32_t block_size = AUDIO_CAPTURE_BLOCK_BYTES;
	const int32_t *samples;
	void *block;
	int result;

	if (media == NULL || frame == NULL || !media->started) {
		return false;
	}
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	if (kind == MEDIA_VIDEO) {
		return pull_video(media, frame);
	}
#endif
	if (kind != MEDIA_AUDIO || media->borrowed[MEDIA_AUDIO]) {
		return false;
	}
	result = dmic_read(media->dmic, 0U, &block, &block_size, AUDIO_CAPTURE_TIMEOUT_MS);
	if (result != 0) {
		if (result != -EAGAIN && result != -EBUSY) {
			platform_log("error", "audio_capture", "status=read_failed result=%d",
				     result);
			atomic_set(&media->failure, 1);
		}
		return false;
	}
	if (block_size != AUDIO_CAPTURE_BLOCK_BYTES || block_is_zero(block, block_size)) {
		k_mem_slab_free(&capture_slab, block);
		if (block_size != AUDIO_CAPTURE_BLOCK_BYTES) {
			platform_log("error", "audio_capture",
				     "status=invalid_block_size size=%u expected=%u", block_size,
				     AUDIO_CAPTURE_BLOCK_BYTES);
			atomic_set(&media->failure, 1);
		}
		return false;
	}
	samples = block;
	for (size_t index = 0U; index < AUDIO_PCMU_SAMPLES; index++) {
		size_t input = index * 4U;
		int32_t sum = (samples[input] >> 16) + (samples[input + 1U] >> 16) +
			      (samples[input + 2U] >> 16) + (samples[input + 3U] >> 16);

		media->encoded[index] = linear_to_mulaw((int16_t)(sum / 4));
	}
	k_mem_slab_free(&capture_slab, block);
	*frame = (MediaFrame){
		.data = media->encoded,
		.size = sizeof(media->encoded),
		.timestamp_ns = media->capture_index * AUDIO_DURATION_NS,
		.duration_ns = AUDIO_DURATION_NS,
	};
	media->capture_index++;
	media->generated_audio++;
	media->borrowed[MEDIA_AUDIO] = true;
	return true;
}

void media_release(Media *media, MediaKind kind)
{
	if (media != NULL && (kind == MEDIA_AUDIO || kind == MEDIA_VIDEO)) {
		media->borrowed[kind] = false;
	}
}

int media_receive(Media *media, MediaKind kind, const MediaFrame *frame)
{
	if (media == NULL || frame == NULL || frame->data == NULL || frame->size == 0U) {
		return -EINVAL;
	}
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	if (kind == MEDIA_VIDEO) {
		int result = queue_received_video(media, frame);

		if (result != 0) {
			atomic_set(&media->failure, 1);
		}
		return result;
	}
#endif
	if (kind != MEDIA_AUDIO || frame->size != AUDIO_PCMU_SAMPLES) {
		return -EINVAL;
	}
	int result = queue_playback(media, frame->data);

	media->received_audio++;
	if (result == -ENOMEM || result == -EAGAIN) {
		media->dropped_audio++;
		result = 0;
	} else if (result != 0) {
		atomic_set(&media->failure, 1);
	}
	if (media->received_audio % 250U == 0U) {
		platform_log("info", "audio_playback_progress",
			     "received=%llu dropped=%llu queued=%u",
			     (unsigned long long)media->received_audio,
			     (unsigned long long)media->dropped_audio,
			     k_msgq_num_used_get(&playback_pcmu_queue));
	}
	return result;
}

void media_request_keyframe(Media *media)
{
	(void)media;
}

bool media_take_keyframe_request(Media *media)
{
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	return media != NULL && video_codec_take_keyframe_request();
#else
	(void)media;
	return false;
#endif
}

uint64_t media_decoded(Media *media, MediaKind kind)
{
	if (media == NULL) {
		return 0U;
	}
	return kind == MEDIA_AUDIO   ? media->decoded_audio
	       : kind == MEDIA_VIDEO ? media->decoded_video
				     : 0U;
}

uint64_t media_zephyr_generated_audio(void)
{
	return hardware_media.generated_audio;
}

uint64_t media_zephyr_late_audio(void)
{
	return hardware_media.late_audio;
}

int media_zephyr_quiesce(void)
{
	int result = stop_playback_worker(&hardware_media);

	if (result != 0) {
		return result;
	}
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	return stop_video_worker(&hardware_media);
#else
	return 0;
#endif
}

int media_destroy(Media *media)
{
	int audio_cleanup_result;
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	int cleanup_result;
#endif

	if (media == NULL) {
		return 0;
	}
	audio_cleanup_result = stop_playback_worker(media);
	if (audio_cleanup_result != 0) {
		return audio_cleanup_result;
	}
	if (media->started) {
		(void)dmic_trigger(media->dmic, DMIC_TRIGGER_STOP);
		(void)audio_codec_stop_output(media->codec);
	}
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	cleanup_result = cleanup_video(media);
#endif
	platform_log(
		"info", "audio_device_result", "generated=%llu played=%llu dropped=%llu late=%llu",
		(unsigned long long)media->generated_audio,
		(unsigned long long)media->decoded_audio, (unsigned long long)media->dropped_audio,
		(unsigned long long)media->late_audio);
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	platform_log("info", "video_device_result", "decoded=%llu dropped=%llu",
		     (unsigned long long)media->decoded_video,
		     (unsigned long long)media->dropped_video);
	if (cleanup_result != 0) {
		return cleanup_result;
	}
#endif
	memset(media, 0, sizeof(*media));
	return 0;
}
