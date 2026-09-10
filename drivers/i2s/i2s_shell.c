/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/linker/section_tags.h>

#define I2S_TONE_START_HELP                                                                        \
	SHELL_HELP("Stream a generated test tone over I2S TX",                                     \
		   "<i2s-device> [frequency_hz] [sample_rate] [bits:16|24|32]")

#define I2S_TONE_STOP_HELP SHELL_HELP("Stop streaming the test tone", "")

#define I2S_TONE_INFO_HELP SHELL_HELP("Show I2S tone stream status", "")

#define I2S_SHELL_CHANNELS    2
#define I2S_SHELL_FRAMES      CONFIG_I2S_SHELL_BLOCK_FRAMES
#define I2S_SHELL_BLOCK_BYTES (I2S_SHELL_FRAMES * I2S_SHELL_CHANNELS * sizeof(int16_t))
#define I2S_SHELL_BLOCK_COUNT CONFIG_I2S_SHELL_BLOCK_COUNT

#define I2S_SHELL_DEFAULT_RATE 48000
#define I2S_SHELL_DEFAULT_FREQ 440
#define I2S_SHELL_DEFAULT_BITS 16
#define I2S_SHELL_AMPLITUDE    16384 /* ~ -6 dBFS */

/* Sine lookup table size and 16.16 fixed-point phase accumulator geometry. */
#define I2S_SHELL_SINE_POINTS     256
#define I2S_SHELL_SINE_MASK       (I2S_SHELL_SINE_POINTS - 1)
#define I2S_SHELL_PHASE_FRAC_BITS 16

/* Blocks queued before the first I2S_TRIGGER_START, so DMA cannot underrun. */
#define I2S_SHELL_PRIME_BLOCKS 2

/* I2S TX timeout and slab-allocation wait, in milliseconds. */
#define I2S_SHELL_TX_TIMEOUT_MS    200
#define I2S_SHELL_ALLOC_TIMEOUT_MS 200

K_MEM_SLAB_DEFINE_IN_SECT_STATIC(i2s_shell_tx_slab, __nocache, I2S_SHELL_BLOCK_BYTES,
				 I2S_SHELL_BLOCK_COUNT, 4);

K_THREAD_STACK_DEFINE(i2s_shell_stack, CONFIG_I2S_SHELL_STACK_SIZE);

struct i2s_shell_stream {
	struct k_thread thread;
	k_tid_t tid;
	const struct device *dev;
	const struct shell *sh;
	uint32_t rate;
	uint32_t freq;
	uint32_t phase;
	uint32_t phase_inc;
	uint8_t bits;         /* 16, 24 or 32 */
	uint8_t sample_bytes; /* 2 for 16-bit, 4 for 24/32-bit */
	volatile bool running;
};

static struct i2s_shell_stream tone_stream;

static const int16_t sine_table[I2S_SHELL_SINE_POINTS] = {
	     0,    402,    804,   1205,   1606,   2006,   2404,   2801,
	  3196,   3590,   3981,   4370,   4756,   5139,   5520,   5897,
	  6270,   6639,   7005,   7366,   7723,   8076,   8423,   8765,
	  9102,   9434,   9760,  10080,  10394,  10702,  11003,  11297,
	 11585,  11866,  12140,  12406,  12665,  12916,  13160,  13395,
	 13623,  13842,  14053,  14256,  14449,  14635,  14811,  14978,
	 15137,  15286,  15426,  15557,  15679,  15791,  15893,  15986,
	 16069,  16143,  16207,  16261,  16305,  16340,  16364,  16379,
	 16384,  16379,  16364,  16340,  16305,  16261,  16207,  16143,
	 16069,  15986,  15893,  15791,  15679,  15557,  15426,  15286,
	 15137,  14978,  14811,  14635,  14449,  14256,  14053,  13842,
	 13623,  13395,  13160,  12916,  12665,  12406,  12140,  11866,
	 11585,  11297,  11003,  10702,  10394,  10080,   9760,   9434,
	  9102,   8765,   8423,   8076,   7723,   7366,   7005,   6639,
	  6270,   5897,   5520,   5139,   4756,   4370,   3981,   3590,
	  3196,   2801,   2404,   2006,   1606,   1205,    804,    402,
	     0,   -402,   -804,  -1205,  -1606,  -2006,  -2404,  -2801,
	 -3196,  -3590,  -3981,  -4370,  -4756,  -5139,  -5520,  -5897,
	 -6270,  -6639,  -7005,  -7366,  -7723,  -8076,  -8423,  -8765,
	 -9102,  -9434,  -9760, -10080, -10394, -10702, -11003, -11297,
	-11585, -11866, -12140, -12406, -12665, -12916, -13160, -13395,
	-13623, -13842, -14053, -14256, -14449, -14635, -14811, -14978,
	-15137, -15286, -15426, -15557, -15679, -15791, -15893, -15986,
	-16069, -16143, -16207, -16261, -16305, -16340, -16364, -16379,
	-16384, -16379, -16364, -16340, -16305, -16261, -16207, -16143,
	-16069, -15986, -15893, -15791, -15679, -15557, -15426, -15286,
	-15137, -14978, -14811, -14635, -14449, -14256, -14053, -13842,
	-13623, -13395, -13160, -12916, -12665, -12406, -12140, -11866,
	-11585, -11297, -11003, -10702, -10394, -10080,  -9760,  -9434,
	 -9102,  -8765,  -8423,  -8076,  -7723,  -7366,  -7005,  -6639,
	 -6270,  -5897,  -5520,  -5139,  -4756,  -4370,  -3981,  -3590,
	 -3196,  -2801,  -2404,  -2006,  -1606,  -1205,   -804,   -402,
};

static bool i2s_device_check(const struct device *dev)
{
	return DEVICE_API_IS(i2s, dev);
}

static void i2s_shell_set_tone(uint32_t freq, uint32_t rate)
{
	tone_stream.freq = freq;
	tone_stream.rate = rate;
	tone_stream.phase_inc = (uint32_t)(((uint64_t)freq << I2S_SHELL_PHASE_FRAC_BITS) *
					   I2S_SHELL_SINE_POINTS / rate);
}

static void i2s_shell_fill_block(void *dst)
{
	uint32_t frames = I2S_SHELL_BLOCK_BYTES / (I2S_SHELL_CHANNELS * tone_stream.sample_bytes);
	uint32_t shift = tone_stream.bits - 16U; /* 0/8/16 for 16/24/32-bit */
	int16_t *out16 = dst;
	int32_t *out32 = dst;
	uint32_t idx;
	int32_t frac;
	int32_t s0;
	int32_t s1;
	int16_t sample;
	uint32_t f;

	for (f = 0; f < frames; f++) {
		idx = (tone_stream.phase >> I2S_SHELL_PHASE_FRAC_BITS) & I2S_SHELL_SINE_MASK;
		frac = tone_stream.phase & ((1U << I2S_SHELL_PHASE_FRAC_BITS) - 1U);
		s0 = sine_table[idx];
		s1 = sine_table[(idx + 1U) & I2S_SHELL_SINE_MASK];
		sample = (int16_t)(s0 + (((s1 - s0) * frac) >> I2S_SHELL_PHASE_FRAC_BITS));

		tone_stream.phase += tone_stream.phase_inc;

		if (tone_stream.sample_bytes == sizeof(int16_t)) {
			*out16++ = sample;
			*out16++ = sample;
		} else {
			*out32++ = (int32_t)sample << shift;
			*out32++ = (int32_t)sample << shift;
		}
	}
}

static void i2s_shell_feeder(void *p1, void *p2, void *p3)
{
	void *block;
	int ret;
	unsigned int primed = 0;
	bool started = false;
	bool had_error = false;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (tone_stream.running) {
		ret = k_mem_slab_alloc(&i2s_shell_tx_slab, &block,
				       K_MSEC(I2S_SHELL_ALLOC_TIMEOUT_MS));
		if (ret != 0) {
			continue;
		}

		i2s_shell_fill_block(block);

		ret = i2s_write(tone_stream.dev, block, I2S_SHELL_BLOCK_BYTES);
		if (ret < 0) {
			k_mem_slab_free(&i2s_shell_tx_slab, block);
			shell_warn(tone_stream.sh, "i2s_write failed (%d)", ret);
			had_error = true;
			break;
		}

		if (!started) {
			primed++;
		}

		if (!started && (primed >= I2S_SHELL_PRIME_BLOCKS)) {
			ret = i2s_trigger(tone_stream.dev, I2S_DIR_TX, I2S_TRIGGER_START);
			if (ret < 0) {
				shell_warn(tone_stream.sh, "i2s START failed (%d)", ret);
				had_error = true;
				break;
			}
			started = true;
		}
	}

	if (had_error) {
		if (i2s_trigger(tone_stream.dev, I2S_DIR_TX, I2S_TRIGGER_DROP) < 0) {
			(void)i2s_trigger(tone_stream.dev, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
		}
	} else {
		(void)i2s_trigger(tone_stream.dev, I2S_DIR_TX,
				  started ? I2S_TRIGGER_DRAIN : I2S_TRIGGER_DROP);
	}

	tone_stream.running = false;
}

static int cmd_tone_start(const struct shell *sh, size_t argc, char *argv[])
{
	struct i2s_config cfg;
	const struct device *dev;
	uint32_t freq = I2S_SHELL_DEFAULT_FREQ;
	uint32_t rate = I2S_SHELL_DEFAULT_RATE;
	uint32_t bits = I2S_SHELL_DEFAULT_BITS;
	int ret;

	if (tone_stream.running) {
		shell_error(sh, "Already streaming on %s; 'i2s tone stop' first",
			    tone_stream.dev->name);
		return -EBUSY;
	}

	dev = shell_device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "I2S device '%s' not found", argv[1]);
		return -ENODEV;
	}
	if (!i2s_device_check(dev)) {
		shell_error(sh, "%s is not an I2S device", dev->name);
		return -EINVAL;
	}

	if (argc > 2) {
		freq = strtoul(argv[2], NULL, 0);
	}
	if (argc > 3) {
		rate = strtoul(argv[3], NULL, 0);
	}
	if (argc > 4) {
		bits = strtoul(argv[4], NULL, 0);
	}
	if (freq == 0U || rate == 0U) {
		shell_error(sh, "frequency and rate must be > 0");
		return -EINVAL;
	}
	if (bits != 16U && bits != 24U && bits != 32U) {
		shell_error(sh, "bits must be 16, 24 or 32");
		return -EINVAL;
	}

	tone_stream.bits = (uint8_t)bits;
	tone_stream.sample_bytes = (bits <= 16U) ? sizeof(int16_t) : sizeof(int32_t);

	cfg.word_size = (uint8_t)bits;
	cfg.channels = I2S_SHELL_CHANNELS;
	cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	cfg.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
	cfg.frame_clk_freq = rate;
	cfg.mem_slab = &i2s_shell_tx_slab;
	cfg.block_size = I2S_SHELL_BLOCK_BYTES;
	cfg.timeout = I2S_SHELL_TX_TIMEOUT_MS;

	ret = i2s_configure(dev, I2S_DIR_TX, &cfg);
	if (ret < 0) {
		shell_error(sh, "i2s_configure failed (%d)", ret);
		return ret;
	}

	tone_stream.phase = 0;
	i2s_shell_set_tone(freq, rate);
	tone_stream.dev = dev;
	tone_stream.sh = sh;
	tone_stream.running = true;

	tone_stream.tid = k_thread_create(&tone_stream.thread, i2s_shell_stack,
					  K_THREAD_STACK_SIZEOF(i2s_shell_stack), i2s_shell_feeder,
					  NULL, NULL, NULL, CONFIG_I2S_SHELL_THREAD_PRIO, 0,
					  K_NO_WAIT);
	k_thread_name_set(tone_stream.tid, "i2s_tone");

	shell_print(sh, "Streaming %u Hz tone on %s @ %u Hz, %u-bit stereo", freq, dev->name, rate,
		    bits);
	return 0;
}

static int cmd_tone_stop(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!tone_stream.running && tone_stream.tid == NULL) {
		shell_warn(sh, "Not streaming");
		return 0;
	}

	tone_stream.running = false;
	if (tone_stream.tid != NULL) {
		k_thread_join(&tone_stream.thread, K_FOREVER);
		tone_stream.tid = NULL;
	}

	shell_print(sh, "Stopped");
	return 0;
}

static int cmd_tone_info(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t frames;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "state    : %s", tone_stream.running ? "streaming" : "idle");

	if (!tone_stream.running) {
		return 0;
	}

	shell_print(sh, "i2s dev  : %s", tone_stream.dev->name);
	shell_print(sh, "tone     : %u Hz", tone_stream.freq);
	shell_print(sh, "rate     : %u Hz", tone_stream.rate);
	frames = I2S_SHELL_BLOCK_BYTES / (I2S_SHELL_CHANNELS * tone_stream.sample_bytes);
	shell_print(sh, "format   : %u-bit stereo, %u-frame blocks x %u", tone_stream.bits,
		    frames, I2S_SHELL_BLOCK_COUNT);

	return 0;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, i2s_device_check);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_i2s_tone,
	SHELL_CMD_ARG(start, &dsub_device_name, I2S_TONE_START_HELP, cmd_tone_start, 2, 3),
	SHELL_CMD_ARG(stop, NULL, I2S_TONE_STOP_HELP, cmd_tone_stop, 1, 0),
	SHELL_CMD_ARG(info, NULL, I2S_TONE_INFO_HELP, cmd_tone_info, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_i2s,
	SHELL_CMD(tone, &sub_i2s_tone, "Generated test-tone streaming commands", NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(i2s, &sub_i2s, "I2S commands", NULL);
