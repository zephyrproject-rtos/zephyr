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
#include <zephyr/audio/codec.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/shell/shell.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef TEST_AUDIO_AVAILABLE
#include "test_audio.h"
#endif

LOG_MODULE_REGISTER(esp32s3_box3_demo, LOG_LEVEL_INF);

/* ------------------------------------------------------------------ */
/*  Device-tree node references                                         */
/* ------------------------------------------------------------------ */

#define ES8311_NODE   DT_ALIAS(audio_codec_spk)
#define ES7210_NODE   DT_ALIAS(audio_codec_mic)
#define I2S_NODE      DT_CHOSEN(zephyr_audio)
#define PA_CTRL_NODE    DT_PATH(leds, pa_ctrl)
#define MUTE_CTRL_NODE  DT_PATH(mute_ctrl_gpio, mute_ctrl)

/* ------------------------------------------------------------------ */
/*  Audio parameters                                                    */
/* ------------------------------------------------------------------ */

#define SAMPLE_RATE          16000
#define ES8311_MCLK_FREQ     6144000
#define ES7210_MCLK_FREQ     4096000

/* I2S memory slab — TX path (speaker) */
#define NUM_TX_BLOCKS  8          /* more blocks = less chance of underrun */
#define TX_BLOCK_SIZE  1024

/* I2S memory slab — RX path (microphone) */
#define NUM_RX_BLOCKS  8
#define RX_BLOCK_SIZE  1024

/* Recording: 5 seconds at 16kHz, 2ch, 16-bit = 320 kB */
#define REC_DURATION_MS      5000
#define BYTES_PER_SEC        (SAMPLE_RATE * 2 * sizeof(int16_t))
#define MAX_REC_BYTES        (BYTES_PER_SEC * (REC_DURATION_MS / 1000) + RX_BLOCK_SIZE)

/* ------------------------------------------------------------------ */
/*  Static device handles                                               */
/* ------------------------------------------------------------------ */

static const struct gpio_dt_spec pa_ctrl   = GPIO_DT_SPEC_GET(PA_CTRL_NODE,   gpios);
static const struct gpio_dt_spec mute_ctrl = GPIO_DT_SPEC_GET(MUTE_CTRL_NODE, gpios);

static const struct device *i2s_dev    = DEVICE_DT_GET(I2S_NODE);
static const struct device *es8311_dev = DEVICE_DT_GET(ES8311_NODE);
static const struct device *es7210_dev = DEVICE_DT_GET(ES7210_NODE);

/* ------------------------------------------------------------------ */
/*  Memory slabs                                                        */
/* ------------------------------------------------------------------ */

#ifdef CONFIG_NOCACHE_MEMORY
#define MEM_SLAB_CACHE_ATTR __nocache
#else
#define MEM_SLAB_CACHE_ATTR
#endif

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_tx_slab_buf[NUM_TX_BLOCKS * WB_UP(TX_BLOCK_SIZE)];

static STRUCT_SECTION_ITERABLE(k_mem_slab, tx_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(tx_mem_slab, _tx_slab_buf,
			       WB_UP(TX_BLOCK_SIZE), NUM_TX_BLOCKS);

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_rx_slab_buf[NUM_RX_BLOCKS * WB_UP(RX_BLOCK_SIZE)];

static STRUCT_SECTION_ITERABLE(k_mem_slab, rx_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(rx_mem_slab, _rx_slab_buf,
			       WB_UP(RX_BLOCK_SIZE), NUM_RX_BLOCKS);

/* ------------------------------------------------------------------ */
/*  Audio test data                                                     */
/* ------------------------------------------------------------------ */

static int16_t sine_wave_data[TX_BLOCK_SIZE / sizeof(int16_t)];

void generate_sine_wave(void)
{
	int n_samples = ARRAY_SIZE(sine_wave_data) / 2;
	for (int i = 0; i < n_samples; i++) {
		float phase = 2.0f * 3.14159f * 1000.0f * i / (float)SAMPLE_RATE;
		int16_t sample = (int16_t)(25000 * sinf(phase));
		sine_wave_data[i * 2]     = sample;
		sine_wave_data[i * 2 + 1] = sample;
	}
	LOG_INF("Generated stereo 1 kHz sine wave (%d samples)", n_samples);
}

void generate_test_melody(void)
{
	int n_samples = ARRAY_SIZE(sine_wave_data) / 2;
	int seg = n_samples / 3;
	const float freqs[3] = { 440.0f, 523.0f, 659.0f };
	for (int i = 0; i < n_samples; i++) {
		float f = freqs[i / seg < 3 ? i / seg : 2];
		float phase = 2.0f * 3.14159f * f * (i % seg) / (float)SAMPLE_RATE;
		int16_t sample = (int16_t)(20000 * sinf(phase));
		sine_wave_data[i * 2]     = sample;
		sine_wave_data[i * 2 + 1] = sample;
	}
	LOG_INF("Generated melody (A4-C5-E5)");
}

void generate_silence(void)
{
	memset(sine_wave_data, 0, sizeof(sine_wave_data));
}

/* ------------------------------------------------------------------ */
/*  PA / Mute helpers                                                   */
/* ------------------------------------------------------------------ */

static int init_pa_control(void)
{
	if (!gpio_is_ready_dt(&pa_ctrl)) {
		LOG_ERR("PA control GPIO not ready");
		return -ENODEV;
	}
	int ret = gpio_pin_configure_dt(&pa_ctrl, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) return ret;
	gpio_pin_set_dt(&pa_ctrl, 1);
	LOG_INF("PA enabled (GPIO46 HIGH)");
	return 0;
}

static int init_mute_control(void)
{
	if (!gpio_is_ready_dt(&mute_ctrl)) {
		LOG_ERR("Mute control GPIO not ready");
		return -ENODEV;
	}
	int ret = gpio_pin_configure_dt(&mute_ctrl, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) return ret;
	gpio_pin_set_dt(&mute_ctrl, 0);
	LOG_INF("Audio unmuted");
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Codec and I2S initialisation                                        */
/* ------------------------------------------------------------------ */

static int init_es8311(void)
{
	if (!device_is_ready(es8311_dev)) {
		LOG_ERR("ES8311 device not ready");
		return -ENODEV;
	}

	struct es8311_config codec_cfg = {
		.sample_rate  = ES8311_SAMPLE_RATE_16KHZ,
		.bit_depth    = ES8311_BIT_DEPTH_16,
		.format       = ES8311_FORMAT_I2S,
		.channels     = 2,
		.master_mode  = false,
		.use_mclk     = true,
		.invert_mclk  = false,
		.invert_sclk  = false,
	};

	int ret = es8311_initialize(es8311_dev, &codec_cfg);
	if (ret < 0) {
		LOG_ERR("ES8311 init failed: %d", ret);
		return ret;
	}

	es8311_set_volume(es8311_dev, 232);  /* 91% volume (217 + 6% = 232) */
	es8311_set_mute(es8311_dev, false);
	es8311_enable(es8311_dev, true);

	LOG_INF("ES8311 ready");
	return 0;
}

static int init_es7210(void)
{
	if (!device_is_ready(es7210_dev)) {
		LOG_ERR("ES7210 device not ready");
		return -ENODEV;
	}

	struct audio_codec_cfg cfg = {
		.mclk_freq            = ES7210_MCLK_FREQ,
		.dai_type             = AUDIO_DAI_TYPE_I2S,
		.dai_route            = AUDIO_ROUTE_CAPTURE,
		.dai_cfg.i2s = {
			.word_size      = AUDIO_PCM_WIDTH_16_BITS,
			.channels       = 2,
			.format         = I2S_FMT_DATA_FORMAT_I2S,
			.frame_clk_freq = SAMPLE_RATE,
			.options        = I2S_OPT_BIT_CLK_SLAVE |
					  I2S_OPT_FRAME_CLK_SLAVE,
		},
	};

	int ret = audio_codec_configure(es7210_dev, &cfg);
	if (ret < 0) {
		LOG_ERR("ES7210 configure failed: %d", ret);
		return ret;
	}

	audio_codec_start_output(es7210_dev);
	LOG_INF("ES7210 ready");
	return 0;
}

static int init_i2s(void)
{
	if (!device_is_ready(i2s_dev)) {
		LOG_ERR("I2S device not ready");
		return -ENODEV;
	}

	struct i2s_config tx_cfg = {
		.word_size      = 16,
		.channels       = 2,
		.format         = I2S_FMT_DATA_FORMAT_I2S,
		.options        = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
		.frame_clk_freq = SAMPLE_RATE,
		.mem_slab       = &tx_mem_slab,
		.block_size     = TX_BLOCK_SIZE,
		.timeout        = 2000,
	};

	int ret = i2s_configure(i2s_dev, I2S_DIR_TX, &tx_cfg);
	if (ret < 0) {
		LOG_ERR("I2S TX configure failed: %d", ret);
		return ret;
	}

	struct i2s_config rx_cfg = {
		.word_size      = 16,
		.channels       = 2,
		.format         = I2S_FMT_DATA_FORMAT_I2S,
		.options        = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
		.frame_clk_freq = SAMPLE_RATE,
		.mem_slab       = &rx_mem_slab,
		.block_size     = RX_BLOCK_SIZE,
		.timeout        = 2000,
	};

	ret = i2s_configure(i2s_dev, I2S_DIR_RX, &rx_cfg);
	if (ret < 0) {
		LOG_ERR("I2S RX configure failed: %d", ret);
		return ret;
	}

	LOG_INF("I2S configured (TX + RX, %d Hz, 16-bit stereo)", SAMPLE_RATE);
	return 0;
}

/* ------------------------------------------------------------------ */
/*  LittleFS mount                                                      */
/* ------------------------------------------------------------------ */

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);

static struct fs_mount_t lfs_mnt = {
	.type       = FS_LITTLEFS,
	.fs_data    = &storage,
	.storage_dev = (void *)FIXED_PARTITION_ID(storage_partition),
	.mnt_point  = "/lfs",
};

static bool lfs_mounted = false;

static int init_lfs(void)
{
	int ret = fs_mount(&lfs_mnt);
	if (ret < 0) {
		LOG_ERR("LittleFS mount failed: %d", ret);
		return ret;
	}
	lfs_mounted = true;
	
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Shell: bincat command (used by ultra_fast_extract.py)              */
/*                                                                      */
/*  Usage: bincat /lfs/audio_000.raw                                   */
/*  Output: BINSTART\n<hex lines>\nBINEND\n                            */
/* ------------------------------------------------------------------ */

#define BINCAT_CHUNK 256   /* bytes per hex line output */

static int cmd_bincat(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "Usage: bincat <path>");
		return -EINVAL;
	}

	struct fs_file_t file;
	fs_file_t_init(&file);

	int ret = fs_open(&file, argv[1], FS_O_READ);
	if (ret < 0) {
		shell_error(sh, "Cannot open %s: %d", argv[1], ret);
		return ret;
	}

	static uint8_t chunk[BINCAT_CHUNK];

	shell_print(sh, "BINSTART");

	ssize_t bytes_read;
	while ((bytes_read = fs_read(&file, chunk, sizeof(chunk))) > 0) {
		/* Print each byte as 2 hex digits, no spaces, then newline */
		for (ssize_t i = 0; i < bytes_read; i++) {
			shell_fprintf(sh, SHELL_NORMAL, "%02x", chunk[i]);
		}
		shell_fprintf(sh, SHELL_NORMAL, "\n");
	}

	shell_print(sh, "BINEND");

	fs_close(&file);
	return 0;
}

/* Also expose "fs ls" and "fs stat" via shell for convenience */
static int cmd_fs_ls(const struct shell *sh, size_t argc, char **argv)
{
	const char *path = (argc > 1) ? argv[1] : "/lfs";
	struct fs_dir_t dir;
	struct fs_dirent entry;

	fs_dir_t_init(&dir);

	int ret = fs_opendir(&dir, path);
	if (ret < 0) {
		shell_error(sh, "Cannot open dir %s: %d", path, ret);
		return ret;
	}

	while (true) {
		ret = fs_readdir(&dir, &entry);
		if (ret < 0 || entry.name[0] == '\0') break;
		shell_print(sh, "%8zu %s", entry.size, entry.name);
	}

	fs_closedir(&dir);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(fs_cmds,
	SHELL_CMD(ls, NULL, "List /lfs directory", cmd_fs_ls),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(bincat, NULL, "Binary dump file as hex", cmd_bincat);
SHELL_CMD_REGISTER(fs, &fs_cmds, "Filesystem commands", NULL);

/* ------------------------------------------------------------------ */
/*  Microphone recording to LittleFS                                    */
/* ------------------------------------------------------------------ */

/*
 * Queue a silence TX block — called inside the RX loop to keep TX fed.
 * Returns silently if the slab is full (it means TX is already full enough).
 */
static void feed_tx_silence(void)
{
	void *tx_block;
	if (k_mem_slab_alloc(&tx_mem_slab, &tx_block, K_NO_WAIT) == 0) {
		memset(tx_block, 0, TX_BLOCK_SIZE);
		if (i2s_write(i2s_dev, tx_block, TX_BLOCK_SIZE) < 0) {
			k_mem_slab_free(&tx_mem_slab, tx_block);
		}
	}
}

static int record_to_file(const char *path, uint32_t duration_ms)
{
	struct fs_file_t file;
	fs_file_t_init(&file);
	int ret;

	if (!lfs_mounted) {
		LOG_ERR("LittleFS not mounted — cannot record");
		return -ENODEV;
	}

	ret = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	if (ret < 0) {
		LOG_ERR("Cannot create %s: %d", path, ret);
		return ret;
	}

	LOG_INF("Recording %u ms to %s ...", duration_ms, path);

	/* ---- Step 1: Clean slate ---- */
	i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	k_msleep(20);

	/* ---- Step 2: Pre-fill TX with silence (fills the whole slab) ---- */
	for (int i = 0; i < NUM_TX_BLOCKS - 1; i++) {
		void *tx_block;
		if (k_mem_slab_alloc(&tx_mem_slab, &tx_block, K_MSEC(100)) == 0) {
			memset(tx_block, 0, TX_BLOCK_SIZE);
			if (i2s_write(i2s_dev, tx_block, TX_BLOCK_SIZE) < 0) {
				k_mem_slab_free(&tx_mem_slab, tx_block);
			}
		}
	}

	/* ---- Step 3: Start BOTH directions (shared clock requirement) ---- */
	ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("I2S BOTH start failed: %d", ret);
		fs_close(&file);
		fs_unlink(path);
		return ret;
	}
	LOG_INF("I2S TX+RX started — recording...");

	/* ---- Step 4: Activate ADC ---- */
	audio_codec_start_output(es7210_dev);
	k_msleep(50);

	/* ---- Step 5: Read loop ---- */
	int64_t end_time = k_uptime_get() + duration_ms;
	uint32_t blocks_read = 0;
	uint32_t max_amplitude = 0;
	size_t total_bytes = 0;

	while (k_uptime_get() < end_time) {

		/*
		 * Feed TX silence EVERY iteration.
		 * The ESP32-S3 I2S shares BCLK/LRCK between TX and RX.
		 * If TX DMA runs out of data the shared clock stalls,
		 * which causes -EIO on the next i2s_read.
		 * Feeding every loop iteration (not every 4th) ensures
		 * the TX FIFO never drains, eliminating the -5 error.
		 */
		feed_tx_silence();

		void *rx_block;
		size_t size;
		ret = i2s_read(i2s_dev, &rx_block, &size);

		if (ret == -EAGAIN) {
			k_yield();
			continue;
		}
		if (ret < 0) {
			LOG_ERR("i2s_read error: %d (restarting BOTH)", ret);
			/*
			 * On EIO: attempt to restart rather than abort.
			 * This recovers from a one-off TX underrun glitch.
			 */
			i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
			k_msleep(5);
			/* Refill TX */
			for (int i = 0; i < NUM_TX_BLOCKS - 1; i++) {
				feed_tx_silence();
			}
			i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_START);
			continue;
		}

		/* Write raw PCM to flash */
		ssize_t written = fs_write(&file, rx_block, size);
		if (written < 0) {
			LOG_ERR("fs_write failed: %d", (int)written);
			k_mem_slab_free(&rx_mem_slab, rx_block);
			break;
		}
		total_bytes += written;

		/* Track amplitude */
		int16_t *samples = (int16_t *)rx_block;
		uint32_t n = size / sizeof(int16_t);
		for (uint32_t i = 0; i < n; i++) {
			uint32_t amp = (uint32_t)abs((int)samples[i]);
			if (amp > max_amplitude) max_amplitude = amp;
		}

		k_mem_slab_free(&rx_mem_slab, rx_block);
		blocks_read++;

		if (blocks_read % 64 == 0) {
			LOG_INF("Recorded %zu bytes, peak %u", total_bytes, max_amplitude);
		}
	}

	/* ---- Step 6: Stop ---- */
	audio_codec_stop_output(es7210_dev);
	i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	fs_close(&file);

	LOG_INF("Recording done: %u blocks, %zu bytes, peak %u",
		blocks_read, total_bytes, max_amplitude);

	if (total_bytes == 0) {
		fs_unlink(path);
		return -EIO;
	}

	LOG_INF("Saved: %s (%zu bytes)", path, total_bytes);
	return 0;
}

/* Convenience: generate a unique filename audio_NNN.raw */
static void next_audio_filename(char *buf, size_t len)
{
	static uint32_t counter = 0;
	snprintf(buf, len, "/lfs/audio_%03u.raw", counter++);
}

/* ------------------------------------------------------------------ */
/*  Canon in D (optional)                                               */
/* ------------------------------------------------------------------ */

#ifdef TEST_AUDIO_AVAILABLE
static int play_test_audio(void)
{
	LOG_INF("Playing test.mp3...");

	struct i2s_config cfg = {
		.word_size      = 16,
		.channels       = 2,
		.format         = I2S_FMT_DATA_FORMAT_I2S,
		.options        = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
		.frame_clk_freq = TEST_AUDIO_SAMPLE_RATE,  /* Use 48kHz from test audio */
		.mem_slab       = &tx_mem_slab,
		.block_size     = TX_BLOCK_SIZE,
		.timeout        = 2000,
	};

	i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
	int ret = i2s_configure(i2s_dev, I2S_DIR_TX, &cfg);
	if (ret < 0) return ret;

	const int16_t *src = test_audio_data;
	size_t total = TEST_AUDIO_SAMPLES;
	size_t samp_per_block = TX_BLOCK_SIZE / sizeof(int16_t);
	size_t cur = 0;
	bool started = false;

	LOG_INF("📊 Total samples: %d, Sample rate: %dHz, Duration: ~%dms", 
		total, TEST_AUDIO_SAMPLE_RATE, (total / 2) * 1000 / TEST_AUDIO_SAMPLE_RATE);

	while (cur < total) {
		size_t n = MIN(samp_per_block, total - cur);
		void *tx_block;
		ret = k_mem_slab_alloc(&tx_mem_slab, &tx_block, K_MSEC(500));
		if (ret < 0) break;
		memcpy(tx_block, &src[cur], n * sizeof(int16_t));
		if (n < samp_per_block) {
			memset((int16_t *)tx_block + n, 0,
			       (samp_per_block - n) * sizeof(int16_t));
		}
		ret = i2s_write(i2s_dev, tx_block, TX_BLOCK_SIZE);
		if (ret < 0) {
			k_mem_slab_free(&tx_mem_slab, tx_block);
			break;
		}
		if (!started) {
			i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
			started = true;
			LOG_INF("🎵 Test audio playback started!");
		}
		cur += n;
		
		/* Progress indicator */
		if (cur % (total / 10) == 0) {
			int progress = (cur * 100) / total;
			LOG_INF("🎶 Playback progress: %d%%", progress);
		}
	}

	i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	k_sleep(K_MSEC(500));
	i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);

	/* Restore 16 kHz for microphone */
	cfg.frame_clk_freq = SAMPLE_RATE;
	i2s_configure(i2s_dev, I2S_DIR_TX, &cfg);
	
	LOG_INF("🎵 Test audio playback completed!");
	return 0;
}
#endif

/* ------------------------------------------------------------------ */
/*  Audio init                                                          */
/* ------------------------------------------------------------------ */

static int init_audio(void)
{
	int ret;

	ret = init_pa_control();
	if (ret < 0) return ret;

	ret = init_mute_control();
	if (ret < 0) return ret;

	ret = init_es8311();
	if (ret < 0) return ret;

	ret = init_es7210();
	if (ret < 0) return ret;

	ret = init_i2s();
	if (ret < 0) return ret;

	return 0;
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
	LOG_INF("=== ESP32-S3-BOX-3 Audio Demo (ES8311 + ES7210) ===");

	/* Mount LittleFS first so recording is available throughout */
	init_lfs();

	int ret = init_audio();
	if (ret < 0) {
		LOG_ERR("Audio init failed: %d", ret);
		return ret;
	}

	LOG_INF("Audio system ready — ES8311 speaker + ES7210 microphone");

#ifdef TEST_AUDIO_AVAILABLE
	LOG_INF("Playing test.mp3...");
	play_test_audio();
	k_sleep(K_SECONDS(3));
#endif

	/* ---- Record microphone and save to LittleFS ---- */
	LOG_INF("Now recording microphone — speak into the mic!");
	k_sleep(K_SECONDS(1));

	char rec_path[32];
	next_audio_filename(rec_path, sizeof(rec_path));

	ret = record_to_file(rec_path, REC_DURATION_MS);
	if (ret < 0) {
		LOG_ERR("Recording failed: %d", ret);
	} else {
		LOG_INF("Recording saved. Run ultra_fast_extract.py to retrieve it.");
	}

	LOG_INF("Audio demo complete - both speaker and microphone tested!");
	LOG_INF("Shell is active. Use 'fs ls /lfs' and 'bincat /lfs/audio_000.raw'");

	/* Stop after first cycle as requested */
	LOG_INF("Demo completed successfully. Application will now idle.");
	LOG_INF("Available shell commands:");
	LOG_INF("  fs ls /lfs          - List recorded files");
	LOG_INF("  bincat <filename>   - Extract file as hex for ultra_fast_extract.py");
	
	/* Keep shell active but don't continue cycling */
	while (1) {
		k_sleep(K_SECONDS(60));
		LOG_INF("Application idle - shell remains active");
	}

	return 0;
}