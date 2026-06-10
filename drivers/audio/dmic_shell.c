/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/sys/base64.h>

#include "dmic_capture.h"
#include "pcm_level.h"

/* Default DMIC configuration */
#define DMIC_SHELL_DEF_SAMPLE_RATE 16000U
#define DMIC_SHELL_DEF_CHANNELS    1U
#define DMIC_SHELL_DEF_PCM_WIDTH   16U

/* PDM clock constraints applied to every capture */
#define DMIC_SHELL_CLK_MIN    1000000U
#define DMIC_SHELL_CLK_MAX    3500000U
#define DMIC_SHELL_CLK_DC_MIN 40U
#define DMIC_SHELL_CLK_DC_MAX 60U

/* Milliseconds of audio per read cycle; must divide 1000 so BLOCKS_PER_SEC is exact. */
#define DMIC_SHELL_BLOCK_DURATION_MS 50U
#define DMIC_SHELL_BLOCKS_PER_SEC    (1000U / DMIC_SHELL_BLOCK_DURATION_MS)

/* Timeout for a single dmic_read(), in milliseconds */
#define DMIC_SHELL_READ_TIMEOUT_MS 2000

/* Number of capture buffers in the pool */
#define DMIC_SHELL_BLOCK_COUNT CONFIG_AUDIO_DMIC_SHELL_BLOCK_COUNT

/* The shell drives a single capture at a time; the busy flag enforces it across commands. */
static struct dmic_capture dmic_shell_cap;
static atomic_t dmic_shell_busy;

/* "dmic read" default number of blocks to capture */
#define DMIC_SHELL_READ_DEF_COUNT 5U

/* Level-meter bar geometry, in characters */
#define DMIC_SHELL_VU_BAR_MIN       4
#define DMIC_SHELL_VU_BAR_MAX       40
/* Fixed chars per channel line around the bar, used to size the bar to the terminal width. */
#define DMIC_SHELL_VU_LINE_OVERHEAD 20

/* Up to two interleaved channels (mono or stereo) */
#define DMIC_SHELL_VU_MAX_CHANNELS 2

/* Background level-meter worker thread */
#define DMIC_SHELL_VU_STACK_SIZE      CONFIG_AUDIO_DMIC_SHELL_VU_STACK_SIZE
/* dmic_read() timeout while metering: short so a key press stops promptly */
#define DMIC_SHELL_VU_READ_TIMEOUT_MS 200

/* The meter spans this dBFS window; truly silent input shows "-inf". */
#define DMIC_SHELL_VU_FLOOR_DBFS_T  (-600) /* -60.0 dBFS */
/* Conventional digital-audio colour zones, in tenths of a dB. */
#define DMIC_SHELL_VU_GREEN_DBFS_T  (-180) /* green below -18 dBFS */
#define DMIC_SHELL_VU_YELLOW_DBFS_T (-60)  /* yellow -18..-6, red above -6 dBFS */

/*
 * Peak-hold ballistics: after a new peak the '|' tick holds flat for PEAK_HOLD_MS, then falls at
 * HOLD_DECAY_T tenths of a dB per block to the floor; the clip '!' stays lit for CLIP_HOLD_MS.
 */
#define DMIC_SHELL_VU_HOLD_BLOCKS                                                                  \
	DIV_ROUND_UP(CONFIG_AUDIO_DMIC_SHELL_VU_PEAK_HOLD_MS, DMIC_SHELL_BLOCK_DURATION_MS)
#define DMIC_SHELL_VU_HOLD_DECAY_T 5
#define DMIC_SHELL_VU_CLIP_HOLD_BLOCKS                                                             \
	DIV_ROUND_UP(CONFIG_AUDIO_DMIC_SHELL_VU_CLIP_HOLD_MS, DMIC_SHELL_BLOCK_DURATION_MS)

/* "dmic dump" defaults and limits */
#define DMIC_SHELL_DUMP_DEF_SECONDS 2U
#define DMIC_SHELL_DUMP_MAX_SECONDS 10U
/* Encode 48 source bytes (= 64 base64 chars) per line */
#define DMIC_SHELL_DUMP_CHUNK       48U
#define DMIC_SHELL_DUMP_B64         ((DMIC_SHELL_DUMP_CHUNK / 3U) * 4U)

static int parse_audio_params(const struct shell *sh, size_t argc, char *argv[], size_t first,
			      uint32_t *rate, uint8_t *channels, uint8_t *pcm_width)
{
	int err = 0;

	if (argc > first) {
		unsigned long val = shell_strtoul(argv[first], 0, &err);

		if (err != 0) {
			shell_error(sh, "Invalid rate '%s'", argv[first]);
			return -EINVAL;
		}
		*rate = (uint32_t)val;
	}
	if (argc > first + 1) {
		unsigned long val = shell_strtoul(argv[first + 1], 0, &err);

		if (err != 0 || val < 1UL || val > 2UL) {
			shell_error(sh, "Channels must be 1 or 2");
			return -EINVAL;
		}
		*channels = (uint8_t)val;
	}
	if (argc > first + 2) {
		unsigned long val = shell_strtoul(argv[first + 2], 0, &err);

		if (err != 0 || (val != 8UL && val != 16UL && val != 24UL && val != 32UL)) {
			shell_error(sh, "PCM width must be 8, 16, 24 or 32");
			return -EINVAL;
		}
		*pcm_width = (uint8_t)val;
	}

	return 0;
}

static bool device_is_dmic(const struct device *dev)
{
	return DEVICE_API_IS(dmic, dev);
}

static const struct device *dmic_shell_device(const struct shell *sh, const char *name)
{
	const struct device *dev = shell_device_get_binding(name);

	if (dev == NULL) {
		shell_error(sh, "DMIC device '%s' not found", name);
		return NULL;
	}
	if (!device_is_dmic(dev)) {
		shell_error(sh, "'%s' is not a DMIC device", name);
		return NULL;
	}

	return dev;
}

/*
 * Acquire the shared capture, configure and start it. On success the caller owns the running
 * capture and must call dmic_shell_capture_stop(); on failure diagnostics are printed here.
 */
static int dmic_shell_capture_start(const struct shell *sh, const struct device *dev,
				    uint32_t sample_rate, uint8_t channels, uint8_t pcm_width)
{
	struct dmic_capture_cfg cfg = {
		.sample_rate = sample_rate,
		.channels = channels,
		.pcm_width = pcm_width,
		.block_duration_ms = DMIC_SHELL_BLOCK_DURATION_MS,
		.block_count = DMIC_SHELL_BLOCK_COUNT,
		.min_pdm_clk_freq = DMIC_SHELL_CLK_MIN,
		.max_pdm_clk_freq = DMIC_SHELL_CLK_MAX,
		.min_pdm_clk_dc = DMIC_SHELL_CLK_DC_MIN,
		.max_pdm_clk_dc = DMIC_SHELL_CLK_DC_MAX,
	};
	int ret;

	if (!atomic_cas(&dmic_shell_busy, 0, 1)) {
		shell_error(sh, "A DMIC capture is already in progress");
		return -EBUSY;
	}

	ret = dmic_capture_start(&dmic_shell_cap, dev, &cfg);
	if (ret < 0) {
		atomic_clear(&dmic_shell_busy);
		switch (ret) {
		case -ENOMEM: {
			size_t need = ROUND_UP(dmic_shell_cap.block_size, sizeof(void *)) *
				      DMIC_SHELL_BLOCK_COUNT;

			shell_error(
				sh,
				"Out of heap for %u x %u-byte capture buffers (~%zu bytes); "
				"increase CONFIG_HEAP_MEM_POOL_ADD_SIZE_AUDIO_DMIC_SHELL to fit",
				DMIC_SHELL_BLOCK_COUNT, (unsigned int)dmic_shell_cap.block_size,
				need);
			break;
		}
		case -EINVAL:
			shell_error(
				sh,
				"Invalid capture parameters; check the rate, channels or width");
			break;
		default:
			shell_error(sh, "Failed to start DMIC: %d", ret);
			break;
		}
		return ret;
	}

	return 0;
}

/* Stop the running capture and release the busy flag. */
static void dmic_shell_capture_stop(void)
{
	dmic_capture_stop(&dmic_shell_cap);
	atomic_clear(&dmic_shell_busy);
}

/* Format a dBFS value (tenths of a dB) as e.g. "-23.5" or "-inf". */
static void db_fmt(char *buf, size_t buf_size, int32_t tenths)
{
	if (tenths == INT32_MIN) {
		(void)snprintf(buf, buf_size, "-inf");
	} else if (tenths < 0) {
		uint32_t a = (uint32_t)(-tenths);

		(void)snprintf(buf, buf_size, "-%u.%u", a / 10U, a % 10U);
	} else {
		(void)snprintf(buf, buf_size, "%u.%u", (uint32_t)tenths / 10U,
			       (uint32_t)tenths % 10U);
	}
}

/* Colour for one bar cell: dBFS zone for filled cells and the peak tick, dim for empty cells. */
static enum shell_vt100_color bar_cell_color(int i, int filled, int marker, int green_end,
					     int yellow_end)
{
	if (i >= filled && i != marker) {
		return SHELL_NORMAL; /* empty cell */
	}
	if (i < green_end) {
		return SHELL_INFO;
	}
	if (i < yellow_end) {
		return SHELL_WARNING;
	}
	return SHELL_ERROR;
}

static void render_meter_bar(const struct shell *sh, int filled, int marker, int bar_width)
{
	int span = 0 - DMIC_SHELL_VU_FLOOR_DBFS_T;
	int green_end =
		bar_width * (DMIC_SHELL_VU_GREEN_DBFS_T - DMIC_SHELL_VU_FLOOR_DBFS_T) / span;
	int yellow_end =
		bar_width * (DMIC_SHELL_VU_YELLOW_DBFS_T - DMIC_SHELL_VU_FLOOR_DBFS_T) / span;
	int i = 0;

	while (i < bar_width) {
		enum shell_vt100_color color =
			bar_cell_color(i, filled, marker, green_end, yellow_end);
		char run[DMIC_SHELL_VU_BAR_MAX + 1];
		int len = 0;

		while (i < bar_width &&
		       bar_cell_color(i, filled, marker, green_end, yellow_end) == color) {
			char cell;

			if (i == marker) {
				cell = '|';
			} else if (i < filled) {
				cell = '#';
			} else {
				cell = '-';
			}
			run[len++] = cell;
			i++;
		}
		run[len] = '\0';
		shell_fprintf(sh, color, "%s", run);
	}
}

#define DMIC_READ_HELP                                                                             \
	SHELL_HELP("Read audio blocks and print peak levels per channel",                          \
		   "<device> [<count> [<rate_hz> [<channels> [<pcm_width>]]]]")

static int cmd_read(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	uint32_t count = DMIC_SHELL_READ_DEF_COUNT;
	uint32_t rate = DMIC_SHELL_DEF_SAMPLE_RATE;
	uint8_t channels = DMIC_SHELL_DEF_CHANNELS;
	uint8_t pcm_width = DMIC_SHELL_DEF_PCM_WIDTH;
	int ret;

	dev = dmic_shell_device(sh, argv[1]);
	if (dev == NULL) {
		return -ENODEV;
	}

	if (argc >= 3) {
		int err = 0;

		count = (uint32_t)shell_strtoul(argv[2], 0, &err);
		if (err != 0) {
			shell_error(sh, "Invalid count '%s'", argv[2]);
			return -EINVAL;
		}
		if (count == 0U) {
			return 0;
		}
	}

	ret = parse_audio_params(sh, argc, argv, 3, &rate, &channels, &pcm_width);
	if (ret < 0) {
		return ret;
	}

	ret = dmic_shell_capture_start(sh, dev, rate, channels, pcm_width);
	if (ret < 0) {
		return ret;
	}

	shell_print(sh, "Reading %u blocks at %u Hz, %u ch, %u-bit:", count, rate, channels,
		    pcm_width);

	for (uint32_t i = 0; i < count; i++) {
		void *buf;
		size_t size;

		ret = dmic_capture_read(&dmic_shell_cap, &buf, &size, DMIC_SHELL_READ_TIMEOUT_MS);
		if (ret < 0) {
			shell_error(sh, "Block %u: read failed: %d", i, ret);
			break;
		}

		shell_fprintf(sh, SHELL_NORMAL, "  block %u (%u bytes):", i, (unsigned int)size);
		for (uint8_t ch = 0; ch < channels; ch++) {
			struct pcm_level_meas m;
			int pct;

			if (pcm_level_analyze(buf, size, pcm_width, channels, ch, &m) < 0) {
				shell_fprintf(sh, SHELL_ERROR, "  CH%u analyze error", ch);
				continue;
			}
			pct = (m.peak_n * 100 + PCM_LEVEL_FS_N / 2) / PCM_LEVEL_FS_N;
			shell_fprintf(sh, SHELL_NORMAL, "  CH%u peak=%3d%%", ch, MIN(pct, 100));
		}
		shell_fprintf(sh, SHELL_NORMAL, "\n");

		dmic_capture_free_block(&dmic_shell_cap, buf);
	}

	dmic_shell_capture_stop();
	return ret < 0 ? ret : 0;
}

struct dmic_vu_ctx {
	struct k_thread thread;
	const struct shell *sh;
	uint8_t channels;
	uint8_t pcm_width;
	int bar_width;
	uint32_t frames; /* rendered frames; used only to manage in-place redraw */
	/* Per-channel meter state carried across blocks. */
	int32_t hold_tenths[DMIC_SHELL_VU_MAX_CHANNELS];  /* decaying peak-hold, dBFS */
	uint32_t hold_blocks[DMIC_SHELL_VU_MAX_CHANNELS]; /* blocks left holding the peak flat */
	uint32_t clip_blocks[DMIC_SHELL_VU_MAX_CHANNELS]; /* blocks left showing the clip '!' */
	bool thread_started; /* thread object has been created at least once; needs a join */
	atomic_t running;
	atomic_t stop;
};

static K_THREAD_STACK_DEFINE(dmic_vu_stack, DMIC_SHELL_VU_STACK_SIZE);
static struct dmic_vu_ctx dmic_vu_ctx_data;

/*
 * Redraw one meter frame in place, one line per channel: "CHx [###|----] <peak> dBFS [!]" where
 * '#' is the current peak, '|' the decaying peak-hold tick, and '!' a recent-clip flag.
 */
static void render_frame(struct dmic_vu_ctx *ctx, const void *buf, size_t size)
{
	const struct shell *sh = ctx->sh;
	int span = 0 - DMIC_SHELL_VU_FLOOR_DBFS_T;

	/* After the first frame, rewind the cursor over the previous lines to overwrite them. */
	if (ctx->frames > 0U) {
		shell_fprintf(sh, SHELL_NORMAL, "\033[%uA", ctx->channels);
	}

	for (uint8_t ch = 0; ch < ctx->channels; ch++) {
		struct pcm_level_meas m;
		int32_t peak_t;
		int filled, marker;
		char peak_s[16];

		pcm_level_analyze(buf, size, ctx->pcm_width, ctx->channels, ch, &m);

		peak_t = pcm_level_power_dbfs_tenths((uint64_t)m.peak_n * (uint64_t)m.peak_n);

		/* Peak hold: jump to a new peak, hold for HOLD_BLOCKS, then decay to -inf. */
		if (peak_t != INT32_MIN && peak_t >= ctx->hold_tenths[ch]) {
			ctx->hold_tenths[ch] = peak_t;
			ctx->hold_blocks[ch] = DMIC_SHELL_VU_HOLD_BLOCKS;
		} else if (ctx->hold_blocks[ch] > 0U) {
			ctx->hold_blocks[ch]--;
		} else if (ctx->hold_tenths[ch] != INT32_MIN) {
			ctx->hold_tenths[ch] -= DMIC_SHELL_VU_HOLD_DECAY_T;
			if (ctx->hold_tenths[ch] < DMIC_SHELL_VU_FLOOR_DBFS_T) {
				ctx->hold_tenths[ch] = INT32_MIN;
			}
		} else {
			/* no peak hold active */
		}

		/* Clip hold: re-arm on a clipped sample, else count down so '!' clears after
		 * CLIP_HOLD_BLOCKS quiet blocks.
		 */
		if (m.clips > 0U) {
			ctx->clip_blocks[ch] = DMIC_SHELL_VU_CLIP_HOLD_BLOCKS;
		} else if (ctx->clip_blocks[ch] > 0U) {
			ctx->clip_blocks[ch]--;
		} else {
			/* no clip indicator active */
		}

		/* Map the current peak to the fill and the peak-hold to the '|' tick. */
		if (peak_t == INT32_MIN) {
			filled = 0;
		} else {
			filled = (int)(((int64_t)peak_t - DMIC_SHELL_VU_FLOOR_DBFS_T) *
				       ctx->bar_width / span);
			filled = CLAMP(filled, 0, ctx->bar_width);
		}
		if (ctx->hold_tenths[ch] == INT32_MIN) {
			marker = -1;
		} else {
			marker =
				(int)(((int64_t)ctx->hold_tenths[ch] - DMIC_SHELL_VU_FLOOR_DBFS_T) *
				      ctx->bar_width / span);
			marker = CLAMP(marker, 0, ctx->bar_width - 1);
		}

		db_fmt(peak_s, sizeof(peak_s), peak_t);

		shell_fprintf(sh, SHELL_NORMAL, "\r\033[2KCH%u [", ch);
		render_meter_bar(sh, filled, marker, ctx->bar_width);
		shell_fprintf(sh, SHELL_NORMAL, "] %6s dBFS", peak_s);
		/* A red '!' flags that the channel has clipped recently. */
		if (ctx->clip_blocks[ch] > 0U) {
			shell_fprintf(sh, SHELL_ERROR, " !");
		}
		shell_fprintf(sh, SHELL_NORMAL, "\n");
	}

	ctx->frames++;
}

/* Any byte received while the meter runs stops it. */
static void dmic_vu_bypass(const struct shell *sh, uint8_t *data, size_t len, void *user_data)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	ARG_UNUSED(user_data);

	atomic_set(&dmic_vu_ctx_data.stop, 1);
}

static void dmic_vu_thread(void *p1, void *p2, void *p3)
{
	struct dmic_vu_ctx *ctx = &dmic_vu_ctx_data;
	bool warned = false;
	int ret = 0;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (!atomic_get(&ctx->stop)) {
		void *buf;
		size_t size;

		ret = dmic_capture_read(&dmic_shell_cap, &buf, &size,
					DMIC_SHELL_VU_READ_TIMEOUT_MS);
		if (ret == -EAGAIN) {
			/* No block in time; warn once, keep trying, stay stoppable. */
			if (!warned) {
				shell_warn(ctx->sh, "No audio data yet (read timed out) - check "
						    "the DMIC clock and wiring");
				warned = true;
			}
			continue;
		}
		if (ret < 0) {
			shell_error(ctx->sh, "Read failed: %d", ret);
			break;
		}
		warned = false;

		if (!atomic_get(&ctx->stop)) {
			render_frame(ctx, buf, size);
		}
		dmic_capture_free_block(&dmic_shell_cap, buf);
	}

	dmic_shell_capture_stop();
	shell_set_bypass(ctx->sh, NULL, NULL);
	shell_fprintf(ctx->sh, SHELL_NORMAL, "\r\033[2K");
	shell_print(ctx->sh, "Level meter stopped");
	atomic_clear(&ctx->running);
}

#define DMIC_VU_HELP                                                                               \
	SHELL_HELP("Show a live microphone level meter", "<device> [<rate_hz> [<channels> "        \
							 "[<pcm_width>]]]")

static int cmd_vu(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	uint32_t rate = DMIC_SHELL_DEF_SAMPLE_RATE;
	uint8_t channels = DMIC_SHELL_DEF_CHANNELS;
	uint8_t pcm_width = DMIC_SHELL_DEF_PCM_WIDTH;
	int ret;

	dev = dmic_shell_device(sh, argv[1]);
	if (dev == NULL) {
		return -ENODEV;
	}

	ret = parse_audio_params(sh, argc, argv, 2, &rate, &channels, &pcm_width);
	if (ret < 0) {
		return ret;
	}

	if (!atomic_cas(&dmic_vu_ctx_data.running, 0, 1)) {
		shell_error(sh, "Level meter is already running");
		return -EBUSY;
	}

	/*
	 * Winning the CAS means a previous run has already cleared "running" and is in its final
	 * unwind. Join it so the thread is fully torn down before we reuse the shared thread object
	 * and stack below; otherwise the create could race a not-quite-dead thread.
	 */
	if (dmic_vu_ctx_data.thread_started) {
		k_thread_join(&dmic_vu_ctx_data.thread, K_FOREVER);
	}

	/* Size the bar to the configured terminal width (capped for readout alignment). */
	dmic_vu_ctx_data.bar_width =
		CLAMP(CONFIG_SHELL_DEFAULT_TERMINAL_WIDTH - DMIC_SHELL_VU_LINE_OVERHEAD,
		      DMIC_SHELL_VU_BAR_MIN, DMIC_SHELL_VU_BAR_MAX);

	ret = dmic_shell_capture_start(sh, dev, rate, channels, pcm_width);
	if (ret < 0) {
		atomic_clear(&dmic_vu_ctx_data.running);
		return ret;
	}

	dmic_vu_ctx_data.sh = sh;
	dmic_vu_ctx_data.channels = channels;
	dmic_vu_ctx_data.pcm_width = pcm_width;
	dmic_vu_ctx_data.frames = 0U;
	for (uint8_t ch = 0; ch < channels; ch++) {
		dmic_vu_ctx_data.hold_tenths[ch] = INT32_MIN;
		dmic_vu_ctx_data.hold_blocks[ch] = 0U;
		dmic_vu_ctx_data.clip_blocks[ch] = 0U;
	}
	atomic_clear(&dmic_vu_ctx_data.stop);

	shell_print(sh, "Starting level meter at %u Hz, %u ch, %u-bit (press any key to stop)",
		    rate, channels, pcm_width);

	/* Bypass first so a keypress can stop the meter even during start-up. */
	shell_set_bypass(sh, dmic_vu_bypass, NULL);
	k_thread_create(&dmic_vu_ctx_data.thread, dmic_vu_stack,
			K_THREAD_STACK_SIZEOF(dmic_vu_stack), dmic_vu_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	k_thread_name_set(&dmic_vu_ctx_data.thread, "dmic_vu");
	dmic_vu_ctx_data.thread_started = true;

	return 0;
}

/* base64-encode one chunk (<= DMIC_SHELL_DUMP_CHUNK bytes) and print it as a line. */
static int dump_line(const struct shell *sh, const uint8_t *src, size_t len)
{
	uint8_t line[DMIC_SHELL_DUMP_B64 + 1];
	size_t olen;
	int ret;

	ret = base64_encode(line, sizeof(line), &olen, src, len);
	if (ret < 0) {
		return ret;
	}
	line[olen] = '\0';
	shell_print(sh, "%s", line);

	return 0;
}

/*
 * Stream `src` as base64, one DMIC_SHELL_DUMP_CHUNK-byte line at a time. A partial chunk is kept
 * in @carry and prepended to the next block so output stays 3-byte aligned until flushed.
 */
static int dump_emit(const struct shell *sh, const uint8_t *src, size_t slen, uint8_t *carry,
		     size_t *carry_len)
{
	int ret;

	if (*carry_len != 0U) {
		size_t take = MIN(DMIC_SHELL_DUMP_CHUNK - *carry_len, slen);

		memcpy(carry + *carry_len, src, take);
		*carry_len += take;
		src += take;
		slen -= take;

		if (*carry_len < DMIC_SHELL_DUMP_CHUNK) {
			return 0;
		}

		ret = dump_line(sh, carry, *carry_len);
		if (ret < 0) {
			return ret;
		}
		*carry_len = 0U;
	}

	while (slen >= DMIC_SHELL_DUMP_CHUNK) {
		ret = dump_line(sh, src, DMIC_SHELL_DUMP_CHUNK);
		if (ret < 0) {
			return ret;
		}
		src += DMIC_SHELL_DUMP_CHUNK;
		slen -= DMIC_SHELL_DUMP_CHUNK;
	}

	if (slen != 0U) {
		memcpy(carry, src, slen);
		*carry_len = slen;
	}

	return 0;
}

static int dump_flush(const struct shell *sh, const uint8_t *carry, size_t carry_len)
{
	return (carry_len != 0U) ? dump_line(sh, carry, carry_len) : 0;
}

static void dump_print_instructions(const struct shell *sh, uint32_t rate, uint8_t channels,
				    uint8_t pcm_width)
{
	/* ffmpeg raw PCM format token; 8-bit has no endianness suffix. */
	const char *fmt;

	if (pcm_width == 8U) {
		fmt = "s8";
	} else if (pcm_width == 16U) {
		fmt = "s16le";
	} else if (pcm_width == 24U) {
		fmt = "s24le";
	} else {
		fmt = "s32le";
	}

	shell_print(sh, "# --- on your computer ---");
	shell_print(sh, "# Log the serial session to a file (copy-paste drops lines), then:");
	shell_print(sh, "#   sed -n '/BEGIN PCM base64/,/END PCM base64/p' session.log \\");
	shell_print(sh, "#     | grep '^[A-Za-z0-9+/=]\\+$' | base64 -d > audio.raw");
	shell_print(sh, "# Play it:");
	shell_print(sh, "#   ffplay -f %s -ar %u -ac %u audio.raw", fmt, rate, channels);
	shell_print(sh, "# or convert it to a .wav:");
	shell_print(sh, "#   sox -r %u -e signed -b %u -c %u audio.raw audio.wav", rate, pcm_width,
		    channels);
}

#define DMIC_DUMP_HELP                                                                             \
	SHELL_HELP("Dump captured PCM as base64 for offline playback",                             \
		   "<device> [<seconds> [<rate_hz> [<channels> [<pcm_width>]]]]")

static int cmd_dump(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	uint32_t seconds = DMIC_SHELL_DUMP_DEF_SECONDS;
	uint32_t rate = DMIC_SHELL_DEF_SAMPLE_RATE;
	uint8_t channels = DMIC_SHELL_DEF_CHANNELS;
	uint8_t pcm_width = DMIC_SHELL_DEF_PCM_WIDTH;
	uint8_t carry[DMIC_SHELL_DUMP_CHUNK];
	size_t carry_len = 0U;
	uint32_t blocks;
	int ret;

	dev = dmic_shell_device(sh, argv[1]);
	if (dev == NULL) {
		return -ENODEV;
	}

	if (argc >= 3) {
		int err = 0;

		seconds = (uint32_t)shell_strtoul(argv[2], 0, &err);
		if (err != 0 || seconds == 0U || seconds > DMIC_SHELL_DUMP_MAX_SECONDS) {
			shell_error(sh, "Duration must be in the range 1-%u seconds",
				    DMIC_SHELL_DUMP_MAX_SECONDS);
			return -EINVAL;
		}
	}
	ret = parse_audio_params(sh, argc, argv, 3, &rate, &channels, &pcm_width);
	if (ret < 0) {
		return ret;
	}

	ret = dmic_shell_capture_start(sh, dev, rate, channels, pcm_width);
	if (ret < 0) {
		return ret;
	}

	blocks = seconds * DMIC_SHELL_BLOCKS_PER_SEC;

	shell_print(sh, "# --- BEGIN PCM base64 (%us, %u Hz, %u ch, %u-bit signed LE) ---", seconds,
		    rate, channels, pcm_width);

	for (uint32_t i = 0; i < blocks; i++) {
		void *buf;
		size_t size;

		ret = dmic_capture_read(&dmic_shell_cap, &buf, &size, DMIC_SHELL_READ_TIMEOUT_MS);
		if (ret < 0) {
			shell_error(sh, "Block %u: read failed: %d", i, ret);
			break;
		}

		ret = dump_emit(sh, buf, size, carry, &carry_len);
		dmic_capture_free_block(&dmic_shell_cap, buf);
		if (ret < 0) {
			shell_error(sh, "base64 encode failed: %d", ret);
			break;
		}
	}

	if (ret == 0) {
		ret = dump_flush(sh, carry, carry_len);
	}

	shell_print(sh, "# --- END PCM base64 ---");

	if (ret == 0) {
		dump_print_instructions(sh, rate, channels, pcm_width);
	}

	dmic_shell_capture_stop();
	return ret < 0 ? ret : 0;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_dmic);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_dmic,
	SHELL_CMD_ARG(read, &dsub_device_name, DMIC_READ_HELP, cmd_read, 2, 4),
	SHELL_CMD_ARG(vu,   &dsub_device_name, DMIC_VU_HELP,   cmd_vu,   2, 3),
	SHELL_CMD_ARG(dump, &dsub_device_name, DMIC_DUMP_HELP, cmd_dump, 2, 4),
	SHELL_SUBCMD_SET_END
);
/* clang-format on */

SHELL_CMD_REGISTER(dmic, &sub_dmic, "Digital Microphone commands", NULL);
