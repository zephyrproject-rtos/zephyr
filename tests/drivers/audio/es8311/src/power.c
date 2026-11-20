/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/audio/codec.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator/fake.h>
#include <zephyr/ztest.h>

#include <emul_es8311.h>

DEFINE_FFF_GLOBALS;

#define ES8311_REG_DAC_MUTE   0x31U
#define ES8311_REG_DAC_VOLUME 0x32U
#define ES8311_DAC_MUTE_ON    0x60U
#define ES8311_DAC_MUTE_OFF   0x00U

static const struct device *const codec = DEVICE_DT_GET(DT_NODELABEL(codec_power));
static const struct device *const gpio = DEVICE_DT_GET(DT_NODELABEL(codec_power_gpio));
static const struct emul *const emul = EMUL_DT_GET(DT_NODELABEL(codec_power));

struct pa_observation {
	struct gpio_callback callback;
	bool enabled;
	int32_t read_result;
	uint8_t dac_mute;
};

static void pa_enabled(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	static const struct i2c_dt_spec bus = I2C_DT_SPEC_GET(DT_NODELABEL(codec_power));
	struct pa_observation *observation = CONTAINER_OF(cb, struct pa_observation, callback);

	ARG_UNUSED(pins);
	if (gpio_emul_output_get(port, 0) == 1) {
		observation->enabled = true;
		observation->read_result =
			i2c_reg_read_byte_dt(&bus, ES8311_REG_DAC_MUTE, &observation->dac_mute);
	}
}

static struct audio_codec_cfg config(void)
{
	return (struct audio_codec_cfg){
		.mclk_freq = 48000U * 256U,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_PLAYBACK_CAPTURE,
		.dai_cfg.i2s = {
			.word_size = 16U,
			.channels = 2U,
			.frame_clk_freq = 48000U,
			.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET,
		},
	};
}

static void before(void *fixture)
{
	struct audio_codec_cfg cfg = config();

	ARG_UNUSED(fixture);
	zassert_true(device_is_ready(codec));
	emul_es8311_reset(emul);
	zassert_ok(audio_codec_stop(codec, AUDIO_DAI_DIR_TXRX));
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					  (audio_property_value_t){.mute = false}));
	zassert_ok(audio_codec_configure(codec, &cfg));
}

ZTEST(es8311_power, test_supply_enabled_and_amplifier_stopped)
{
	const struct device *const supply = DEVICE_DT_GET(DT_NODELABEL(codec_supply));

	zassert_true(regulator_is_enabled(supply));
	zassert_equal(gpio_emul_output_get(gpio, 0), 0);
}

ZTEST(es8311_power, test_amplifier_follows_playback_only)
{
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX));
	zassert_equal(gpio_emul_output_get(gpio, 0), 0);
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_TX));
	zassert_equal(gpio_emul_output_get(gpio, 0), 1);
	zassert_ok(audio_codec_stop(codec, AUDIO_DAI_DIR_TX));
	zassert_equal(gpio_emul_output_get(gpio, 0), 0);
	zassert_ok(audio_codec_stop(codec, AUDIO_DAI_DIR_RX));
}

ZTEST(es8311_power, test_reconfigure_disables_amplifier)
{
	struct audio_codec_cfg cfg = config();

	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_TX));
	zassert_equal(gpio_emul_output_get(gpio, 0), 1);
	zassert_ok(audio_codec_configure(codec, &cfg));
	zassert_equal(gpio_emul_output_get(gpio, 0), 0);
}

ZTEST(es8311_power, test_amplifier_enabled_while_dac_muted)
{
	struct pa_observation observation = {0};
	int ret;

	gpio_init_callback(&observation.callback, pa_enabled, BIT(0));
	zassert_ok(gpio_add_callback(gpio, &observation.callback));
	ret = audio_codec_start(codec, AUDIO_DAI_DIR_TX);
	zassert_ok(gpio_remove_callback(gpio, &observation.callback));
	zassert_ok(ret);
	zassert_true(observation.enabled);
	zassert_ok(observation.read_result);
	zassert_equal(observation.dac_mute, ES8311_DAC_MUTE_ON,
		      "Amplifier enabled while DAC was unmuted");
}

ZTEST(es8311_power, test_failed_start_keeps_amplifier_off)
{
	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_VOLUME);
	zassert_equal(audio_codec_start(codec, AUDIO_DAI_DIR_TX), -EIO);
	zassert_equal(gpio_emul_output_get(gpio, 0), 0);
}

ZTEST(es8311_power, test_failed_start_unmute_disables_amplifier)
{
	emul_es8311_fail_write_landed_value(emul, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF);
	zassert_equal(audio_codec_start(codec, AUDIO_DAI_DIR_TX), -EIO);
	zassert_equal(gpio_emul_output_get(gpio, 0), 0);
	emul_es8311_fail_write_landed(emul, -1);
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_TX));
	zassert_equal(gpio_emul_output_get(gpio, 0), 1);
}

ZTEST(es8311_power, test_failed_stop_disables_amplifier)
{
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_TX));
	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_MUTE);
	zassert_equal(audio_codec_stop(codec, AUDIO_DAI_DIR_TX), -EIO);
	zassert_equal(gpio_emul_output_get(gpio, 0), 0);
}

ZTEST(es8311_power, test_failed_property_unmute_disables_amplifier)
{
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_TX));
	emul_es8311_fail_write_landed_value(emul, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF);
	zassert_equal(audio_codec_apply_properties(codec), -EIO);
	zassert_equal(gpio_emul_output_get(gpio, 0), 0);
	emul_es8311_fail_write_to(emul, -1);
	emul_es8311_fail_write_landed(emul, -1);
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_TX));
	zassert_equal(gpio_emul_output_get(gpio, 0), 1);
}

ZTEST(es8311_power, test_failed_init_releases_supply)
{
	const struct device *const failed = DEVICE_DT_GET(DT_NODELABEL(codec_power_failed));
	const struct device *const supply = DEVICE_DT_GET(DT_NODELABEL(codec_failed_supply));
	const struct emul *const failed_emul = EMUL_DT_GET(DT_NODELABEL(codec_power_failed));

	emul_es8311_set_chip_id(failed_emul, 0x00, 0x00);
	zassert_equal(device_init(failed), -ENODEV);
	zassert_false(device_is_ready(failed));
	zassert_equal(regulator_fake_enable_fake.call_count, 1U);
	zassert_equal_ptr(regulator_fake_enable_fake.arg0_val, supply);
	zassert_equal(regulator_fake_disable_fake.call_count, 1U);
	zassert_equal_ptr(regulator_fake_disable_fake.arg0_val, supply);
	zassert_false(regulator_is_enabled(supply));
}

ZTEST_SUITE(es8311_power, NULL, NULL, before, NULL, NULL);
