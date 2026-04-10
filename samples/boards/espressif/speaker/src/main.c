/*
 * Copyright (c) 2026 NotioNext LTD.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/audio/es8311.h>
#include <zephyr/sys/iterable_sections.h>
#include <math.h>
#include <string.h>

#ifdef TEST_AUDIO_AVAILABLE
#include "test_audio.h"
#endif

LOG_MODULE_REGISTER(esp32s3_box3_demo, LOG_LEVEL_INF);

/* ESP32-S3-BOX-3 Pin Definitions */
#define PA_CTRL_NODE DT_PATH(leds, pa_ctrl)
#define MUTE_BUTTON_NODE DT_PATH(buttons, button_1)
#define I2S_NODE DT_CHOSEN(zephyr_audio)
#define ES8311_NODE DT_ALIAS(audio_codec_spk)

/* Audio Configuration - Professional audio standard */
#define SAMPLE_RATE 48000     /* 48kHz sample rate (DVD/Professional quality) */
#define BLOCK_SIZE 2048       /* Larger buffer for smoother playback */
#define NUM_BLOCKS 8          /* More blocks for better buffering */

static const struct gpio_dt_spec pa_ctrl = GPIO_DT_SPEC_GET(PA_CTRL_NODE, gpios);
static const struct gpio_dt_spec mute_ctrl = GPIO_DT_SPEC_GET(MUTE_BUTTON_NODE, gpios);
static const struct device *i2s_dev = DEVICE_DT_GET(I2S_NODE);
static const struct device *es8311_dev = DEVICE_DT_GET(ES8311_NODE);

#ifdef CONFIG_NOCACHE_MEMORY
	#define MEM_SLAB_CACHE_ATTR __nocache
#else
	#define MEM_SLAB_CACHE_ATTR
#endif

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_tx_mem_slab[(NUM_BLOCKS) * WB_UP(BLOCK_SIZE)];

static STRUCT_SECTION_ITERABLE(k_mem_slab, tx_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(tx_mem_slab, _k_mem_slab_buf_tx_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS);

/* Audio test data - stereo, larger buffer */
static int16_t sine_wave_data[1024];  /* 512 samples * 2 channels */

void generate_sine_wave(void)
{
	for (int i = 0; i < 512; i++) {
		/* Generate 1kHz tone at 22.05kHz sample rate with moderate amplitude */
		float phase = 2.0f * 3.14159f * 1000.0f * i / 22050.0f;  /* 1kHz at 22.05kHz sample rate */
		int16_t sample = (int16_t)(20000 * sin(phase));  /* Moderate amplitude to avoid clipping */
		sine_wave_data[i * 2] = sample;     /* Left channel */
		sine_wave_data[i * 2 + 1] = sample; /* Right channel (same as left) */
	}
	LOG_INF("Generated stereo sine wave data (1kHz tone, 512 samples, 2 channels)");
}

void generate_test_melody(void)
{
	/* Generate a simple melody: 440Hz (A4) -> 523Hz (C5) -> 659Hz (E5) */
	int notes_per_sample = 512 / 3;  /* Divide into 3 notes */
	
	for (int i = 0; i < 512; i++) {
		float frequency;
		if (i < notes_per_sample) {
			frequency = 440.0f;  /* A4 */
		} else if (i < notes_per_sample * 2) {
			frequency = 523.0f;  /* C5 */
		} else {
			frequency = 659.0f;  /* E5 */
		}
		
		float phase = 2.0f * 3.14159f * frequency * (i % notes_per_sample) / (22050.0f / 3.0f);
		int16_t sample = (int16_t)(18000 * sin(phase));  /* Moderate amplitude */
		sine_wave_data[i * 2] = sample;     /* Left channel */
		sine_wave_data[i * 2 + 1] = sample; /* Right channel */
	}
	LOG_INF("Generated test melody (A4-C5-E5, 512 samples, 2 channels)");
}

#ifdef TEST_AUDIO_AVAILABLE
int play_test_audio(void)
{
	int ret;
	void *tx_block;
	const int16_t *test_data = test_audio_data;
	size_t total_samples = TEST_AUDIO_SAMPLES;
	size_t samples_per_block = BLOCK_SIZE / sizeof(int16_t);
	size_t current_sample = 0;
	
	LOG_INF("🎵 Starting test.mp3 playback...");
	LOG_INF("📊 Total samples: %d, Block size: %d samples", total_samples, samples_per_block);
	LOG_INF("📊 Sample rate: %dHz, Duration: ~%dms", TEST_AUDIO_SAMPLE_RATE, (total_samples / 2) * 1000 / TEST_AUDIO_SAMPLE_RATE);
	
	/* Configure I2S for 48kHz */
	struct i2s_config config = {
		.word_size = 16,
		.channels = 2,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
		.frame_clk_freq = TEST_AUDIO_SAMPLE_RATE,  /* Use the actual sample rate from the audio */
		.mem_slab = &tx_mem_slab,
		.block_size = BLOCK_SIZE,
		.timeout = 1000,
	};

	ret = i2s_configure(i2s_dev, I2S_DIR_TX, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2S for test audio: %d", ret);
		return ret;
	}

	/* Play audio in chunks */
	while (current_sample < total_samples) {
		size_t samples_to_copy = MIN(samples_per_block, total_samples - current_sample);
		size_t bytes_to_copy = samples_to_copy * sizeof(int16_t);
		
		/* Allocate memory block from slab */
		ret = k_mem_slab_alloc(&tx_mem_slab, &tx_block, K_MSEC(1000));
		if (ret < 0) {
			LOG_ERR("Failed to allocate TX block: %d", ret);
			break;
		}

		/* Copy test audio data to the allocated block */
		memcpy(tx_block, &test_data[current_sample], bytes_to_copy);
		
		/* If we have less data than block size, pad with zeros */
		if (bytes_to_copy < BLOCK_SIZE) {
			memset((uint8_t*)tx_block + bytes_to_copy, 0, BLOCK_SIZE - bytes_to_copy);
		}

		/* Send audio data */
		ret = i2s_write(i2s_dev, tx_block, BLOCK_SIZE);
		if (ret < 0) {
			LOG_ERR("Failed to write I2S data: %d", ret);
			k_mem_slab_free(&tx_mem_slab, tx_block);
			break;
		}

		/* Start I2S transmission if this is the first block */
		if (current_sample == 0) {
			ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
			if (ret < 0) {
				LOG_ERR("Failed to start I2S: %d", ret);
				k_mem_slab_free(&tx_mem_slab, tx_block);
				break;
			}
			LOG_INF("🎵 Test audio playback started!");
		}

		current_sample += samples_to_copy;
		
		/* Progress indicator */
		if (current_sample % (total_samples / 10) == 0) {
			int progress = (current_sample * 100) / total_samples;
			LOG_INF("🎶 Playback progress: %d%%", progress);
		}
		
		/* Small delay to prevent overwhelming the I2S buffer */
		k_sleep(K_MSEC(10));
	}

	/* Wait for playback to complete */
	k_sleep(K_MSEC(500));

	/* Stop I2S transmission */
	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (ret < 0) {
		LOG_DBG("Failed to drop I2S (may be normal): %d", ret);
	} else {
		LOG_INF("🎵 Test audio playback completed!");
	}

	return 0;
}
#endif

void generate_silence(void)
{
	/* Fill with zeros for silence */
	memset(sine_wave_data, 0, sizeof(sine_wave_data));
	LOG_INF("Generated silence data (512 samples, 2 channels)");
}

int init_pa_control(void)
{
	int ret;
	
	LOG_INF("Initializing PA control...");
	
	if (!gpio_is_ready_dt(&pa_ctrl)) {
		LOG_ERR("PA control GPIO not ready");
		return -1;
	}

	ret = gpio_pin_configure_dt(&pa_ctrl, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure PA control GPIO: %d", ret);
		return ret;
	}

	/* Enable PA */
	gpio_pin_set_dt(&pa_ctrl, 1);
	LOG_INF("✓ PA control enabled");
	
	return 0;
}

int init_mute_control(void)
{
	int ret;
	
	LOG_INF("Initializing audio mute control...");
	
	if (!gpio_is_ready_dt(&mute_ctrl)) {
		LOG_ERR("Mute control GPIO not ready");
		return -1;
	}

	/* Configure as output to control mute state */
	ret = gpio_pin_configure_dt(&mute_ctrl, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure mute control GPIO: %d", ret);
		return ret;
	}

	/* Unmute audio (set to high - assuming active low mute) */
	gpio_pin_set_dt(&mute_ctrl, 1);
	LOG_INF("✓ Audio unmuted");
	
	return 0;
}

int init_audio(void)
{
	int ret;
	
	LOG_INF("Initializing audio system...");

	/* Initialize PA control first */
	ret = init_pa_control();
	if (ret) {
		LOG_ERR("Failed to initialize PA control: %d", ret);
		return ret;
	}

	/* Initialize mute control */
	ret = init_mute_control();
	if (ret) {
		LOG_ERR("Failed to initialize mute control: %d", ret);
		return ret;
	}

	/* Check if ES8311 device is ready */
	if (!device_is_ready(es8311_dev)) {
		LOG_ERR("ES8311 device not ready");
		return -1;
	}
	LOG_INF("✓ ES8311 device ready");

	/* Initialize ES8311 codec using the driver */
	struct es8311_config codec_config = {
		.sample_rate = ES8311_SAMPLE_RATE_48KHZ,
		.bit_depth = ES8311_BIT_DEPTH_16,
		.format = ES8311_FORMAT_I2S,
		.channels = 2,
		.master_mode = false,  /* ESP32 is I2S master */
		.use_mclk = true,
		.invert_mclk = false,
		.invert_sclk = false,
	};

	ret = es8311_initialize(es8311_dev, &codec_config);
	if (ret) {
		LOG_ERR("Failed to initialize ES8311 codec: %d", ret);
		return ret;
	}
	LOG_INF("✓ ES8311 codec initialized (48kHz, 16-bit, stereo, I2S)");

	/* Set volume to 85% (217 out of 255) */
	ret = es8311_set_volume(es8311_dev, 217);
	if (ret) {
		LOG_WRN("Failed to set ES8311 volume: %d", ret);
	} else {
		LOG_INF("✓ Volume set to 85%% (217/255)");
	}

	/* Unmute codec */
	ret = es8311_set_mute(es8311_dev, false);
	if (ret) {
		LOG_WRN("Failed to unmute ES8311: %d", ret);
	}

	/* Enable codec */
	ret = es8311_enable(es8311_dev, true);
	if (ret) {
		LOG_WRN("Failed to enable ES8311: %d", ret);
	}

	/* Check if I2S device is ready */
	if (!device_is_ready(i2s_dev)) {
		LOG_ERR("I2S device not ready");
		return -1;
	}
	LOG_INF("✓ I2S device ready");

	/* Configure I2S with 48kHz to match codec configuration */
	struct i2s_config config = {
		.word_size = 16,
		.channels = 2,  /* ESP32 I2S driver requires 2 channels */
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
		.frame_clk_freq = SAMPLE_RATE,  /* 48kHz to match codec */
		.mem_slab = &tx_mem_slab,
		.block_size = BLOCK_SIZE,
		.timeout = 1000,
	};

	ret = i2s_configure(i2s_dev, I2S_DIR_TX, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2S: %d", ret);
		return ret;
	}
	LOG_INF("✓ I2S configured (48kHz, 16-bit, stereo, MCLK=12.288MHz)");

	return 0;
}

int test_speaker(void)
{
	int ret;
	void *tx_block;
	
	LOG_INF("Testing speaker with sine wave...");

	/* First, ensure I2S is in a clean state */
	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (ret < 0) {
		LOG_DBG("I2S drop failed (expected if not running): %d", ret);
	}

	/* Allocate memory block from slab */
	ret = k_mem_slab_alloc(&tx_mem_slab, &tx_block, K_MSEC(1000));
	if (ret < 0) {
		LOG_ERR("Failed to allocate TX block: %d", ret);
		return ret;
	}

	/* Copy sine wave data to the allocated block */
	memcpy(tx_block, sine_wave_data, sizeof(sine_wave_data));

	/* Send audio data first - this queues the data */
	ret = i2s_write(i2s_dev, tx_block, BLOCK_SIZE);
	if (ret < 0) {
		LOG_ERR("Failed to write I2S data: %d", ret);
		k_mem_slab_free(&tx_mem_slab, tx_block);
		return ret;
	}

	LOG_INF("Audio data queued to I2S");

	/* Now start I2S transmission */
	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("Failed to start I2S: %d", ret);
		k_mem_slab_free(&tx_mem_slab, tx_block);
		return ret;
	}

	LOG_INF("I2S transmission started - playing audio...");

	/* Let it play for longer to hear the full buffer */
	k_sleep(K_MSEC(3000));

	/* Stop I2S transmission cleanly */
	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (ret < 0) {
		LOG_DBG("Failed to drop I2S (may be normal): %d", ret);
	} else {
		LOG_INF("I2S stopped successfully");
	}

	LOG_INF("Speaker test completed");
	return 0;
}

int test_silence(void)
{
	int ret;
	void *tx_block;
	
	LOG_INF("Testing speaker with silence...");

	/* Generate silence */
	generate_silence();

	/* First, ensure I2S is in a clean state */
	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (ret < 0) {
		LOG_DBG("I2S drop failed (expected if not running): %d", ret);
	}

	/* Allocate memory block from slab */
	ret = k_mem_slab_alloc(&tx_mem_slab, &tx_block, K_MSEC(1000));
	if (ret < 0) {
		LOG_ERR("Failed to allocate TX block: %d", ret);
		return ret;
	}

	/* Copy silence data to the allocated block */
	memcpy(tx_block, sine_wave_data, sizeof(sine_wave_data));

	/* Send audio data first - this queues the data */
	ret = i2s_write(i2s_dev, tx_block, BLOCK_SIZE);
	if (ret < 0) {
		LOG_ERR("Failed to write I2S data: %d", ret);
		k_mem_slab_free(&tx_mem_slab, tx_block);
		return ret;
	}

	LOG_INF("Silence data queued to I2S");

	/* Now start I2S transmission */
	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("Failed to start I2S: %d", ret);
		k_mem_slab_free(&tx_mem_slab, tx_block);
		return ret;
	}

	LOG_INF("I2S transmission started - playing silence...");

	/* Let it play for a bit */
	k_sleep(K_MSEC(1000));

	/* Stop I2S transmission cleanly */
	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (ret < 0) {
		LOG_DBG("Failed to drop I2S (may be normal): %d", ret);
	} else {
		LOG_INF("I2S stopped successfully");
	}

	LOG_INF("Silence test completed");
	return 0;
}

int main(void)
{
	LOG_INF("========================================");
	LOG_INF("  ESP32-S3-BOX-3 Audio Demo");
	LOG_INF("  Board: %s", CONFIG_BOARD);
	LOG_INF("========================================");

	/* Initialize Audio */
	int ret = init_audio();
	if (ret) {
		LOG_ERR("Failed to initialize audio: %d", ret);
		return ret;
	}

	LOG_INF("");
	LOG_INF("🎵 Audio system initialized successfully!");
	LOG_INF("🔊 Ready for music playback at 48kHz, 85%% volume");
	LOG_INF("");

#ifdef TEST_AUDIO_AVAILABLE
	/* Play test audio */
	LOG_INF("▶ Playing test.mp3...");
	play_test_audio();
	LOG_INF("✓ Test audio playback completed");
#else
	LOG_INF("ℹ No audio file available");
	LOG_INF("ℹ Add test_audio.c to enable music playback");
#endif

	LOG_INF("");
	LOG_INF("🎉 Audio demo completed");
	LOG_INF("");

	return 0;
}