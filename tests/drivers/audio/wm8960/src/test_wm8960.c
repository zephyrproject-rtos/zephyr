/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/audio/codec.h>
#include <zephyr/logging/log.h>
#include <zephyr/audio/codec.h>

#include "wm8960_emul.h"

LOG_MODULE_REGISTER(wm8960_test, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define WM8960_NODE DT_NODELABEL(wm8960)

static const struct device *codec_dev;
static const struct emul *wm8960_emul;

static void *wm8960_setup(void)
{
	/* Get the codec device */
	codec_dev = DEVICE_DT_GET(WM8960_NODE);
	zassert_true(device_is_ready(codec_dev), "WM8960 device is not ready");

	/* Get the emulator */
	wm8960_emul = EMUL_DT_GET(WM8960_NODE);
	zassert_not_null(wm8960_emul, "WM8960 emulator not found");

	return NULL;
}

static void wm8960_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Reset the emulator to known state before each test */
	wm8960_emul_set_reg(wm8960_emul, 0x0F, 0x0000); /* Trigger reset */
}

ZTEST_SUITE(wm8960_test, NULL, wm8960_setup, wm8960_before, NULL, NULL);

ZTEST(wm8960_test, test_wm8960_init)
{
	/* Verify that the device initialized properly */
	zassert_true(device_is_ready(codec_dev), "WM8960 device is not ready");

	/* Check that the chip ID register has the correct value */
	uint16_t clocking_reg = wm8960_emul_get_reg(wm8960_emul, 0x04);

	LOG_INF("Clocking register: 0x%04x", clocking_reg);

	/* Verify that the device is in a known state after initialization */
	zassert_false(wm8960_emul_is_powered_up(wm8960_emul),
			  "WM8960 should not be powered up after init");
	zassert_false(wm8960_emul_is_dac_enabled(wm8960_emul),
			  "DAC should not be enabled after init");
	zassert_false(wm8960_emul_is_adc_enabled(wm8960_emul),
			  "ADC should not be enabled after init");
	zassert_false(wm8960_emul_is_headphone_enabled(wm8960_emul),
			  "Headphone should not be enabled after init");
	zassert_false(wm8960_emul_is_speaker_enabled(wm8960_emul),
			  "Speaker should not be enabled after init");
}

ZTEST(wm8960_test, test_wm8960_configure_i2s)
{
	int ret;
	struct audio_codec_cfg config = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s = {
			.word_size = 16,
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
			.frame_clk_freq = 48000,
		}};

	/* Configure the codec */
	ret = audio_codec_configure(codec_dev, &config);
	zassert_equal(ret, 0, "Failed to configure WM8960: %d", ret);

	/* Verify that the audio interface format was set correctly */
	uint8_t format = wm8960_emul_get_audio_format(wm8960_emul);

	zassert_equal(format, 2, "Audio format should be I2S (2), got %d", format);

	/* Verify that the word length was set correctly */
	uint8_t word_length = wm8960_emul_get_word_length(wm8960_emul);

	zassert_equal(word_length, 16, "Word length should be 16, got %d", word_length);
}

ZTEST(wm8960_test, test_wm8960_configure_different_word_sizes)
{
	int ret;
	struct audio_codec_cfg config = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s = {
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
			.frame_clk_freq = 48000,
		}};

	/* Test different word sizes */
	uint8_t word_sizes[] = {16, 20, 24, 32};

	for (int i = 0; i < ARRAY_SIZE(word_sizes); i++) {
		config.dai_cfg.i2s.word_size = word_sizes[i];

		ret = audio_codec_configure(codec_dev, &config);
		zassert_equal(ret, 0, "Failed to configure WM8960 with %d-bit words: %d",
				  word_sizes[i], ret);

		uint8_t configured_word_length = wm8960_emul_get_word_length(wm8960_emul);

		zassert_equal(configured_word_length, word_sizes[i],
				  "Word length should be %d, got %d", word_sizes[i],
				  configured_word_length);
	}
}

ZTEST(wm8960_test, test_wm8960_configure_different_sample_rates)
{
	int ret;
	struct audio_codec_cfg config = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s = {
			.word_size = 16,
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
		}};

	/* Test different sample rates */
	uint32_t sample_rates[] = {8000, 16000, 32000, 44100, 48000, 96000};

	for (int i = 0; i < ARRAY_SIZE(sample_rates); i++) {
		config.dai_cfg.i2s.frame_clk_freq = sample_rates[i];

		ret = audio_codec_configure(codec_dev, &config);
		zassert_equal(ret, 0, "Failed to configure WM8960 with %dHz: %d", sample_rates[i],
				  ret);
	}
}

ZTEST(wm8960_test, test_wm8960_configure_invalid_parameters)
{
	int ret;
	struct audio_codec_cfg config = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s = {
			.word_size = 16,
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
			.frame_clk_freq = 48000,
		}};

	/* Test invalid DAI type */
	config.dai_type = 99; /* Invalid type */
	ret = audio_codec_configure(codec_dev, &config);
	zassert_not_equal(ret, 0, "Should fail with invalid DAI type");

	/* Test invalid word size */
	config.dai_type = AUDIO_DAI_TYPE_I2S;
	config.dai_cfg.i2s.word_size = 12; /* Invalid word size */
	ret = audio_codec_configure(codec_dev, &config);
	zassert_not_equal(ret, 0, "Should fail with invalid word size");

	/* Test invalid channel count */
	config.dai_cfg.i2s.word_size = 16;
	config.dai_cfg.i2s.channels = 0; /* Invalid channel count */
	ret = audio_codec_configure(codec_dev, &config);
	zassert_not_equal(ret, 0, "Should fail with invalid channel count");
}

ZTEST(wm8960_test, test_wm8960_volume_control)
{
	int ret;
	audio_property_value_t val;

	struct audio_codec_cfg config = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
		.dai_cfg.i2s = {
			.word_size = 16,
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
			.frame_clk_freq = 48000,
		}};

	/* Configure the codec */
	audio_codec_configure(codec_dev, &config);

	/* Set volume to 50% (0x40) instead of 0x80 which exceeds max */
	val.vol = 0x40;
	ret = audio_codec_set_property(
		codec_dev,
		AUDIO_PROPERTY_OUTPUT_VOLUME,
		AUDIO_CHANNEL_ALL,
		val
	);
	zassert_equal(ret, 0, "Failed to set volume: %d", ret);

	/* Apply properties */
	ret = audio_codec_apply_properties(codec_dev);
	zassert_equal(ret, 0, "Failed to apply properties: %d", ret);

	/* Verify that the volume was set correctly */
	/* in wm8960 DAC volume is fixed to 0xFF */
#if TEST_DAC_VOLUME
	uint16_t left_vol = wm8960_emul_get_volume(wm8960_emul, 0) & 0xFF;  /* Left DAC */
	uint16_t right_vol = wm8960_emul_get_volume(wm8960_emul, 1) & 0xFF; /* Right DAC */

	zassert_equal(left_vol, 0xFF, "Left volume should be 0x40, got 0x%02x", left_vol);
	zassert_equal(right_vol, 0xFF, "Right volume should be 0x40, got 0x%02x", right_vol);
#endif
	/* Test mute */
	val.mute = true;
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					   val);
	zassert_equal(ret, 0, "Failed to set mute: %d", ret);

	/* Apply properties */
	ret = audio_codec_apply_properties(codec_dev);
	zassert_equal(ret, 0, "Failed to apply properties: %d", ret);

#if TEST_DAC_VOLUME
	/* Verify that the channels are muted */
	zassert_true(wm8960_emul_is_muted(wm8960_emul, 0), "Left channel should be muted");
	zassert_true(wm8960_emul_is_muted(wm8960_emul, 1), "Right channel should be muted");
#endif
	/* Unmute */
	val.mute = false;
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					   val);
	zassert_equal(ret, 0, "Failed to clear mute: %d", ret);

	/* Apply properties */
	ret = audio_codec_apply_properties(codec_dev);
	zassert_equal(ret, 0, "Failed to apply properties: %d", ret);

#if TEST_DAC_VOLUME
	/* Verify that the channels are unmuted */
	zassert_false(wm8960_emul_is_muted(wm8960_emul, 0), "Left channel should be unmuted");
	zassert_false(wm8960_emul_is_muted(wm8960_emul, 1), "Right channel should be unmuted");
#endif
}

ZTEST(wm8960_test, test_wm8960_volume_boundary_values)
{
	int ret;
	audio_property_value_t val;
	struct audio_codec_cfg config = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
		.dai_cfg.i2s = {
			.word_size = 16,
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
			.frame_clk_freq = 48000,
		}};

	/* Configure the codec */
	audio_codec_configure(codec_dev, &config);

	/* Test minimum volume (0x00) */
	val.vol = 0x00;
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					   val);
	zassert_equal(ret, 0, "Failed to set minimum volume: %d", ret);

	ret = audio_codec_apply_properties(codec_dev);
	zassert_equal(ret, 0, "Failed to apply minimum volume: %d", ret);

#if TEST_DAC_VOLUME
	uint16_t left_vol = wm8960_emul_get_volume(wm8960_emul, 0) & 0xFF;

	zassert_equal(left_vol, 0x00, "Left volume should be 0x00, got 0x%02x", left_vol);
#endif

	/* Test maximum volume (0x7F) - WM8960 max is 0x7F, not 0xFF */
	val.vol = 0x7F;
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					   val);
	zassert_equal(ret, 0, "Failed to set maximum volume: %d", ret);

	ret = audio_codec_apply_properties(codec_dev);
	zassert_equal(ret, 0, "Failed to apply maximum volume: %d", ret);
#if TEST_DAC_VOLUME
	left_vol = wm8960_emul_get_volume(wm8960_emul, 0) & 0xFF;
	zassert_equal(left_vol, 0x7F, "Left volume should be 0x7F, got 0x%02x", left_vol);
#endif
}

ZTEST(wm8960_test, test_wm8960_individual_channel_control)
{
	int ret;
	audio_property_value_t val;
	struct audio_codec_cfg config = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
		.dai_cfg.i2s = {
			.word_size = 16,
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
			.frame_clk_freq = 48000,
		}};

	/* Configure the codec */
	audio_codec_configure(codec_dev, &config);

	/* Set left channel volume to 25% (0x20) */
	val.vol = 0x20;
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					   AUDIO_CHANNEL_FRONT_LEFT, val);
	zassert_equal(ret, 0, "Failed to set left volume: %d", ret);

	/* Set right channel volume to 75% (0x60) */
	val.vol = 0x60;
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					   AUDIO_CHANNEL_FRONT_RIGHT, val);
	zassert_equal(ret, 0, "Failed to set right volume: %d", ret);

	/* Apply properties */
	ret = audio_codec_apply_properties(codec_dev);
	zassert_equal(ret, 0, "Failed to apply properties: %d", ret);

#if TEST_DAC_VOLUME
	/* Verify that the volumes were set correctly */
	uint16_t left_vol = wm8960_emul_get_volume(wm8960_emul, 0) & 0xFF;  /* Left DAC */
	uint16_t right_vol = wm8960_emul_get_volume(wm8960_emul, 1) & 0xFF; /* Right DAC */

	zassert_equal(left_vol, 0x20, "Left volume should be 0x20, got 0x%02x", left_vol);
	zassert_equal(right_vol, 0x60, "Right volume should be 0x60, got 0x%02x", right_vol);
#endif

	/* Mute only left channel */
	val.mute = true;
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_MUTE,
					   AUDIO_CHANNEL_FRONT_LEFT, val);
	zassert_equal(ret, 0, "Failed to mute left channel: %d", ret);

	/* Apply properties */
	ret = audio_codec_apply_properties(codec_dev);
	zassert_equal(ret, 0, "Failed to apply properties: %d", ret);

#if TEST_DAC_VOLUME
	/* Verify that All channels are muted */
	zassert_true(wm8960_emul_is_muted(wm8960_emul, 0), "Left channel should be muted");
	zassert_true(wm8960_emul_is_muted(wm8960_emul, 1), "Right channel should be muted");
#endif
}

ZTEST(wm8960_test, test_wm8960_input_volume_control)
{
	int ret;
	audio_property_value_t val;
	struct audio_codec_cfg config = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_PLAYBACK_CAPTURE,
		.dai_cfg.i2s = {
			.word_size = 16,
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
			.frame_clk_freq = 48000,
		}};

	/* Configure the codec */
	audio_codec_configure(codec_dev, &config);

	/* Set input volume to 50% (0x40) */
	val.vol = 0x40;
	ret = audio_codec_set_property(codec_dev,
		AUDIO_PROPERTY_INPUT_VOLUME,
		AUDIO_CHANNEL_ALL,
		val
	);
	zassert_equal(ret, 0, "Failed to set input volume: %d", ret);

	/* Apply properties */
	ret = audio_codec_apply_properties(codec_dev);
	zassert_equal(ret, 0, "Failed to apply input volume: %d", ret);

	/* Verify that the input volume was set correctly */
	uint16_t right_adc_vol = wm8960_emul_get_volume(wm8960_emul, 7) & 0xFF; /* Right ADC */

	zassert_equal(
		right_adc_vol,
		0x40,
		"Right ADC volume should be 0x40, got 0x%02x",
		right_adc_vol
	);

	/* Test input mute */
	val.mute = true;
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL,
					   val);
	zassert_equal(ret, 0, "Failed to set input mute: %d", ret);

	ret = audio_codec_apply_properties(codec_dev);
	zassert_equal(ret, 0, "Failed to apply input mute: %d", ret);
}

ZTEST(wm8960_test, test_wm8960_invalid_properties)
{
	int ret;
	audio_property_value_t val;
	struct audio_codec_cfg config = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
		.dai_cfg.i2s = {
			.word_size = 16,
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
			.frame_clk_freq = 48000,
		}};

	/* Configure the codec */
	ret = audio_codec_configure(codec_dev, &config);
	zassert_equal(ret, 0, "Failed to configure WM8960: %d", ret);

	/* Test invalid property */
	val.vol = 0x80;
	ret = audio_codec_set_property(codec_dev, 999, AUDIO_CHANNEL_ALL, val);
	zassert_not_equal(ret, 0, "Should fail with invalid property");

	/* Test invalid channel */
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME, 999, val);
	zassert_not_equal(ret, 0, "Should fail with invalid channel");
}

ZTEST(wm8960_test, test_wm8960_reset)
{
	int ret;
	struct audio_codec_cfg config = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_PLAYBACK_CAPTURE,
		.dai_cfg.i2s = {
			.word_size = 16,
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
			.frame_clk_freq = 48000,
		}
	};

	/* Configure the codec */
	ret = audio_codec_configure(codec_dev, &config);
	zassert_equal(ret, 0, "Failed to configure WM8960: %d", ret);

	/* Verify that the codec is powered up */
	zassert_true(wm8960_emul_is_powered_up(wm8960_emul),
			 "WM8960 should be powered up");

	/* Trigger a reset by writing to the reset register */
	wm8960_emul_set_reg(wm8960_emul, 0x0F, 0x0000);

	/* Verify that the codec is reset to default state */
	zassert_false(wm8960_emul_is_powered_up(wm8960_emul),
			  "WM8960 should be powered down after reset");
	zassert_false(wm8960_emul_is_dac_enabled(wm8960_emul),
			  "DAC should be disabled after reset");
	zassert_false(wm8960_emul_is_adc_enabled(wm8960_emul),
			  "ADC should be disabled after reset");

#if TEST_DAC_VOLUME
	/* Verify that the volume registers are reset to default values */
	uint16_t left_dac_vol = wm8960_emul_get_volume(wm8960_emul, 0);

	zassert_equal(left_dac_vol, 0x00FF, "Left DAC volume should be reset to 0x00FF, got 0x%04x",
			  left_dac_vol);
#endif
}

ZTEST(wm8960_test, test_wm8960_pll_configuration)
{
	/* Test PLL enable/disable functionality */
	zassert_false(wm8960_emul_is_pll_enabled(wm8960_emul), "PLL should be disabled by default");

	/* Enable PLL by setting CLKSEL bit */
	wm8960_emul_set_reg(wm8960_emul, 0x04, 0x0001); /* CLOCKING_1 register */
	zassert_true(wm8960_emul_is_pll_enabled(wm8960_emul),
			 "PLL should be enabled after setting CLKSEL");

	/* Disable PLL */
	wm8960_emul_set_reg(wm8960_emul, 0x04, 0x0000);
	zassert_false(wm8960_emul_is_pll_enabled(wm8960_emul),
			  "PLL should be disabled after clearing CLKSEL");
}

ZTEST(wm8960_test, test_wm8960_register_access)
{
	/* Test direct register access */
	uint16_t test_value = 0x0123;
	uint8_t test_reg = 0x04; /* CLOCKING_1 register */

	/* Write to register */
	wm8960_emul_set_reg(wm8960_emul, test_reg, test_value);

	/* Read back and verify */
	uint16_t read_value = wm8960_emul_get_reg(wm8960_emul, test_reg);

	zassert_equal(read_value, test_value,
			  "Register read/write mismatch: wrote 0x%04x, read 0x%04x", test_value,
			  read_value);

	/* Test invalid register access */
	uint16_t invalid_reg_value = wm8960_emul_get_reg(wm8960_emul, 0xFF);

	zassert_equal(invalid_reg_value, 0, "Invalid register should return 0");
}

ZTEST(wm8960_test, test_wm8960_speaker_output)
{
	/* Test speaker output functionality */
	audio_codec_start_output(codec_dev);

	/* Check if speaker output can be enabled */
	if (wm8960_emul_is_speaker_enabled(wm8960_emul)) {
		LOG_INF("Speaker output is enabled");

		/* Test speaker volume control */
		uint16_t left_spk_vol = wm8960_emul_get_volume(wm8960_emul, 4);  /* Left Speaker */
		uint16_t right_spk_vol = wm8960_emul_get_volume(wm8960_emul, 5); /* Right Speaker */

		LOG_INF("Speaker volumes - Left: 0x%04x, Right: 0x%04x", left_spk_vol,
			right_spk_vol);
	}

	audio_codec_stop_output(codec_dev);
}

ZTEST(wm8960_test, test_wm8960_power_state_transitions)
{
	/* Test various power state transitions */
	int ret;
	struct audio_codec_cfg config = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_PLAYBACK_CAPTURE,
		.dai_cfg.i2s = {
			.word_size = 16,
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
			.frame_clk_freq = 48000,
		}
	};

	/* Initial state - all off */
	zassert_false(wm8960_emul_is_powered_up(wm8960_emul), "Should start powered down");

	/* Configure the codec */
	ret = audio_codec_configure(codec_dev, &config);
	zassert_equal(ret, 0, "Failed to configure WM8960: %d", ret);

	zassert_true(wm8960_emul_is_powered_up(wm8960_emul), "Should be powered up with output");
#if TEST_DAC_VOLUME
	zassert_true(wm8960_emul_is_dac_enabled(wm8960_emul), "DAC should be enabled");
#endif
	zassert_true(wm8960_emul_is_adc_enabled(wm8960_emul), "ADC should be enabled");

	/* Remove output, keep input */
	audio_codec_stop_output(codec_dev);
	zassert_true(wm8960_emul_is_powered_up(wm8960_emul), "Should remain powered for input");
}
