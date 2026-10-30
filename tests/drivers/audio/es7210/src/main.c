/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT everest_es7210

#include <string.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/audio/es7210.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/ztest.h>

/* This models control transactions only, not analog or serial audio behavior. */
struct es7210_emul_data {
	uint8_t regs[0x4d];
	uint32_t transfers;
	uint32_t fail_at;
	bool fail_all;
};

static int transfer(const struct emul *emulator, struct i2c_msg *msgs, int count, int addr)
{
	struct es7210_emul_data *data = emulator->data;

	data->transfers++;
	if ((addr == 0x42) || data->fail_all || (data->fail_at == data->transfers)) {
		return -EIO;
	}
	if ((count == 2) && (msgs[0].len == 1U) && (msgs[1].len == 1U) &&
	    ((msgs[0].flags & I2C_MSG_READ) == 0U) && ((msgs[1].flags & I2C_MSG_READ) != 0U) &&
	    (msgs[0].buf[0] < sizeof(data->regs))) {
		msgs[1].buf[0] = data->regs[msgs[0].buf[0]];
		return 0;
	}
	if ((count != 1) || ((msgs[0].flags & I2C_MSG_READ) != 0U) || (msgs[0].len != 2U) ||
	    (msgs[0].buf[0] >= sizeof(data->regs))) {
		return -EIO;
	}
	if ((msgs[0].buf[0] == 0U) && (msgs[0].buf[1] == 0xffU)) {
		memset(data->regs, 0, sizeof(data->regs));
	}
	data->regs[msgs[0].buf[0]] = msgs[0].buf[1];
	return 0;
}

static int emul_init(const struct emul *emulator, const struct device *parent)
{
	ARG_UNUSED(emulator);
	ARG_UNUSED(parent);
	return 0;
}

static const struct i2c_emul_api emul_api = {
	.transfer = transfer,
};

#define ES7210_EMUL_DEFINE(inst)                                                                   \
	static struct es7210_emul_data emul_data_##inst;                                           \
	EMUL_DT_INST_DEFINE(inst, emul_init, &emul_data_##inst, NULL, &emul_api, NULL);

DT_INST_FOREACH_STATUS_OKAY(ES7210_EMUL_DEFINE)

static const struct device *codec(uint8_t pair)
{
	return pair == 0U ? DEVICE_DT_GET(DT_NODELABEL(adc12)) : DEVICE_DT_GET(DT_NODELABEL(adc34));
}

static struct es7210_emul_data *registers(uint8_t pair)
{
	const struct emul *emulator =
		pair == 0U ? EMUL_DT_GET(DT_NODELABEL(adc12)) : EMUL_DT_GET(DT_NODELABEL(adc34));

	return emulator->data;
}

static struct audio_codec_cfg configuration(void)
{
	return (struct audio_codec_cfg){
		.mclk_freq = 4096000U,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_CAPTURE,
		.dai_cfg.i2s = {
			.word_size = 16U,
			.channels = 2U,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.frame_clk_freq = 16000U,
		},
	};
}

static void prepare(uint8_t pair)
{
	struct es7210_emul_data *data = registers(pair);
	struct audio_codec_cfg cfg = configuration();

	data->fail_all = false;
	data->fail_at = 0U;
	zassert_true(device_is_ready(codec(pair)));
	zassert_ok(audio_codec_stop(codec(pair), AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_configure(codec(pair), &cfg));
}

static void assert_stopped(uint8_t pair)
{
	struct es7210_emul_data *data = registers(pair);

	zassert_equal(data->regs[0x14], 3U);
	zassert_equal(data->regs[0x15], 3U);
	zassert_equal(data->regs[0x4b], 0xffU);
	zassert_equal(data->regs[0x4c], 0xffU);
	zassert_equal(data->regs[0x40], 0xc0U);
	zassert_equal(data->regs[0x01], 0x7fU);
}

ZTEST(es7210, test_missing_device)
{
	zassert_false(device_is_ready(DEVICE_DT_GET(DT_NODELABEL(adc_absent))));
}

ZTEST(es7210, test_pairs_formats_and_restart)
{
	static const uint8_t widths[] = {16U, 24U, 32U};
	static const uint8_t expected[] = {0x60U, 0x00U, 0x80U};

	for (uint8_t pair = 0U; pair < 2U; pair++) {
		struct es7210_emul_data *data = registers(pair);
		struct audio_codec_cfg cfg = configuration();

		prepare(pair);
		for (size_t i = 0; i < ARRAY_SIZE(widths); i++) {
			cfg.dai_cfg.i2s.word_size = widths[i];
			zassert_ok(audio_codec_configure(codec(pair), &cfg));
			zassert_equal(data->regs[0x11], expected[i]);
			zassert_equal(data->regs[0x12], pair == 0U ? 0x20U : 0x10U);
			zassert_equal(data->regs[0x41], pair == 0U ? 0x70U : 0x00U);
			assert_stopped(pair);
			zassert_ok(audio_codec_start(codec(pair), AUDIO_DAI_DIR_RX));
			zassert_equal(data->regs[0x01], pair == 0U ? 0x34U : 0x2aU);
			zassert_equal(data->regs[0x4b + pair], 0U);
			zassert_equal(data->regs[0x4c - pair], 0xffU);
			zassert_equal(data->regs[0x43 + pair * 2], 0x10U);
			zassert_equal(data->regs[0x44 + pair * 2], 0x10U);
			zassert_equal(audio_codec_configure(codec(pair), &cfg), -EBUSY);
			uint32_t transfers = data->transfers;

			zassert_ok(audio_codec_start(codec(pair), AUDIO_DAI_DIR_RX));
			zassert_equal(data->transfers, transfers);
			zassert_ok(audio_codec_stop(codec(pair), AUDIO_DAI_DIR_RX));
			assert_stopped(pair);
			zassert_ok(audio_codec_start(codec(pair), AUDIO_DAI_DIR_RX));
			zassert_ok(audio_codec_stop(codec(pair), AUDIO_DAI_DIR_RX));
		}
	}
}

ZTEST(es7210, test_clocks)
{
	static const struct {
		uint32_t mclk;
		uint32_t rate;
		uint8_t divider;
	} cases[] = {
		{4096000U, 8000U, 0x81U},   {4096000U, 16000U, 0xc1U},  {11289600U, 22050U, 0x81U},
		{12288000U, 24000U, 0x81U}, {16384000U, 32000U, 0x81U}, {11289600U, 44100U, 0xc1U},
		{12288000U, 48000U, 0xc1U},
	};
	struct audio_codec_cfg cfg = configuration();
	struct es7210_emul_data *data = registers(0U);

	prepare(0U);
	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		cfg.mclk_freq = cases[i].mclk;
		cfg.dai_cfg.i2s.frame_clk_freq = cases[i].rate;
		zassert_ok(audio_codec_configure(codec(0U), &cfg));
		zassert_equal(data->regs[0x02], cases[i].divider);
		zassert_equal(data->regs[0x07], 0x20U);
	}
}

ZTEST(es7210, test_unsupported_configuration_is_untouched)
{
	struct audio_codec_cfg cfg = configuration();
	struct es7210_emul_data *data = registers(0U);
	uint32_t transfers;

	prepare(0U);
	transfers = data->transfers;
	zassert_equal(audio_codec_configure(codec(0U), NULL), -EINVAL);
	for (uint8_t i = 0U; i < 9U; i++) {
		cfg = configuration();
		switch (i) {
		case 0U:
			cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
			break;
		case 1U:
			cfg.dai_type = AUDIO_DAI_TYPE_PCM;
			break;
		case 2U:
			cfg.dai_cfg.i2s.channels = 1U;
			break;
		case 3U:
			cfg.dai_cfg.i2s.channels = 4U;
			break;
		case 4U:
			cfg.dai_cfg.i2s.word_size = 15U;
			break;
		case 5U:
			cfg.dai_cfg.i2s.format |= I2S_FMT_DATA_ORDER_LSB;
			break;
		case 6U:
			cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET;
			break;
		case 7U:
			cfg.dai_cfg.i2s.frame_clk_freq = 96000U;
			break;
		case 8U:
			cfg.mclk_freq = 4000000U;
			break;
		default:
			zassert_unreachable();
		}
		zassert_equal(audio_codec_configure(codec(0U), &cfg), -ENOTSUP);
	}
	zassert_equal(audio_codec_start(codec(0U), AUDIO_DAI_DIR_TX), -ENOTSUP);
	zassert_equal(audio_codec_stop(codec(0U), AUDIO_DAI_DIR_TXRX), -ENOTSUP);
	audio_codec_start_output(codec(0U));
	audio_codec_stop_output(codec(0U));
	zassert_equal(data->transfers, transfers);
	zassert_ok(audio_codec_start(codec(0U), AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_stop(codec(0U), AUDIO_DAI_DIR_RX));
}

ZTEST(es7210, test_gain_and_mute)
{
	for (uint8_t pair = 0U; pair < 2U; pair++) {
		struct es7210_emul_data *data = registers(pair);
		audio_property_value_t value = {.vol = 30};

		prepare(pair);
		zassert_ok(audio_codec_set_property(codec(pair), AUDIO_PROPERTY_INPUT_VOLUME,
						    AUDIO_CHANNEL_FRONT_LEFT, value));
		zassert_equal(data->regs[0x43 + 2 * pair], 0x1aU);
		value.vol = 36;
		zassert_ok(audio_codec_set_property(codec(pair), AUDIO_PROPERTY_INPUT_VOLUME,
						    AUDIO_CHANNEL_FRONT_RIGHT, value));
		value.mute = true;
		zassert_ok(audio_codec_set_property(codec(pair), AUDIO_PROPERTY_INPUT_MUTE,
						    AUDIO_CHANNEL_FRONT_RIGHT, value));
		zassert_equal(data->regs[0x15 - pair], 3U);
		zassert_ok(audio_codec_start(codec(pair), AUDIO_DAI_DIR_RX));
		zassert_equal(data->regs[0x43 + 2 * pair], 0x1aU);
		zassert_equal(data->regs[0x44 + 2 * pair], 0x1dU);
		zassert_equal(data->regs[0x15 - pair], 2U);
		zassert_ok(audio_codec_set_property(codec(pair), AUDIO_PROPERTY_INPUT_MUTE,
						    AUDIO_CHANNEL_ALL, value));
		zassert_equal(data->regs[0x15 - pair], 3U);
		value.mute = false;
		zassert_ok(audio_codec_set_property(codec(pair), AUDIO_PROPERTY_INPUT_MUTE,
						    AUDIO_CHANNEL_FRONT_RIGHT, value));
		zassert_equal(data->regs[0x15 - pair], 1U);
		zassert_ok(audio_codec_stop(codec(pair), AUDIO_DAI_DIR_RX));
		zassert_ok(audio_codec_start(codec(pair), AUDIO_DAI_DIR_RX));
		zassert_equal(data->regs[0x15 - pair], 1U);
		zassert_ok(audio_codec_apply_properties(codec(pair)));
		value.vol = 1;
		zassert_equal(audio_codec_set_property(codec(pair), AUDIO_PROPERTY_INPUT_VOLUME,
						       AUDIO_CHANNEL_ALL, value),
			      -EINVAL);
		value.vol = -3;
		zassert_equal(audio_codec_set_property(codec(pair), AUDIO_PROPERTY_INPUT_VOLUME,
						       AUDIO_CHANNEL_ALL, value),
			      -EINVAL);
		zassert_equal(audio_codec_set_property(codec(pair), AUDIO_PROPERTY_OUTPUT_VOLUME,
						       AUDIO_CHANNEL_ALL, value),
			      -ENOTSUP);
		zassert_equal(audio_codec_set_property(codec(pair), AUDIO_PROPERTY_INPUT_MUTE,
						       AUDIO_CHANNEL_LFE, value),
			      -ENOTSUP);
		zassert_ok(audio_codec_stop(codec(pair), AUDIO_DAI_DIR_RX));
	}
}

ZTEST(es7210, test_configure_and_start_error_cleanup)
{
	struct es7210_emul_data *data = registers(0U);
	struct audio_codec_cfg cfg = configuration();
	uint32_t count;

	prepare(0U);
	count = data->transfers;
	zassert_ok(audio_codec_configure(codec(0U), &cfg));
	count = data->transfers - count;
	/* Fail each configure transaction, including its shutdown writes. */
	for (uint32_t i = 1U; i <= count; i++) {
		data->fail_at = data->transfers + i;
		zassert_equal(audio_codec_configure(codec(0U), &cfg), -EIO);
		assert_stopped(0U);
		zassert_equal(audio_codec_start(codec(0U), AUDIO_DAI_DIR_RX), -EIO);
		data->fail_at = 0U;
		zassert_ok(audio_codec_configure(codec(0U), &cfg));
	}
	count = data->transfers;
	zassert_ok(audio_codec_start(codec(0U), AUDIO_DAI_DIR_RX));
	count = data->transfers - count;
	zassert_ok(audio_codec_stop(codec(0U), AUDIO_DAI_DIR_RX));
	for (uint32_t i = 1U; i <= count; i++) {
		data->fail_at = data->transfers + i;
		zassert_equal(audio_codec_start(codec(0U), AUDIO_DAI_DIR_RX), -EIO);
		assert_stopped(0U);
		zassert_equal(audio_codec_apply_properties(codec(0U)), -EIO);
		data->fail_at = 0U;
		zassert_ok(audio_codec_configure(codec(0U), &cfg));
	}
}

ZTEST(es7210, test_property_stop_and_persistent_errors)
{
	struct es7210_emul_data *data = registers(0U);
	audio_property_value_t value = {.vol = 18};
	struct audio_codec_cfg cfg = configuration();

	for (uint8_t i = 1U; i <= 2U; i++) {
		prepare(0U);
		data->fail_at = data->transfers + i;
		zassert_equal(audio_codec_set_property(codec(0U), AUDIO_PROPERTY_INPUT_VOLUME,
						       AUDIO_CHANNEL_ALL, value),
			      -EIO);
		assert_stopped(0U);
		zassert_equal(audio_codec_start(codec(0U), AUDIO_DAI_DIR_RX), -EIO);
	}
	for (uint8_t i = 1U; i <= 6U; i++) {
		prepare(0U);
		zassert_ok(audio_codec_start(codec(0U), AUDIO_DAI_DIR_RX));
		uint32_t before = data->transfers;

		data->fail_at = before + i;
		zassert_equal(audio_codec_stop(codec(0U), AUDIO_DAI_DIR_RX), -EIO);
		zassert_equal(data->transfers, before + 6U);
		zassert_equal(audio_codec_start(codec(0U), AUDIO_DAI_DIR_RX), -EIO);
	}
	prepare(0U);
	data->fail_all = true;
	zassert_equal(audio_codec_configure(codec(0U), &cfg), -EIO);
	zassert_equal(audio_codec_start(codec(0U), AUDIO_DAI_DIR_RX), -EIO);
	zassert_equal(audio_codec_set_property(codec(0U), AUDIO_PROPERTY_INPUT_VOLUME,
					       AUDIO_CHANNEL_ALL, value),
		      -EIO);
	data->fail_all = false;
	zassert_ok(audio_codec_configure(codec(0U), &cfg));
}

static struct audio_codec_es7210_dai_cfg serial_configuration(void)
{
	return (struct audio_codec_es7210_dai_cfg){
		.mclk_freq = 4096000U,
		.sample_rate = 16000U,
		.format = AUDIO_CODEC_ES7210_I2S,
		.word_size = 16U,
		.slot_width = 16U,
		.channels = 4U,
	};
}

ZTEST(es7210, test_serial_formats_and_controller)
{
	static const uint8_t formats[] = {0x60U, 0x61U, 0x63U, 0x73U};
	static const uint8_t outputs[] = {0x22U, 0x22U, 0x21U, 0x21U};
	struct audio_codec_es7210_dai_cfg cfg = serial_configuration();
	struct es7210_emul_data *data = registers(0U);

	prepare(0U);
	for (uint8_t i = 0U; i < ARRAY_SIZE(formats); i++) {
		cfg.format = i;
		zassert_ok(audio_codec_es7210_configure(codec(0U), &cfg));
		zassert_equal(data->regs[0x11], formats[i]);
		zassert_equal(data->regs[0x12], outputs[i]);
		zassert_ok(audio_codec_start(codec(0U), AUDIO_DAI_DIR_RX));
		zassert_equal(data->regs[0x4b], 0U);
		zassert_equal(data->regs[0x4c], 0U);
		zassert_equal(data->regs[0x01], 0x20U);
		zassert_ok(audio_codec_stop(codec(0U), AUDIO_DAI_DIR_RX));
	}
	cfg.format = AUDIO_CODEC_ES7210_I2S;
	cfg.clock_controller = true;
	cfg.invert_bclk = true;
	cfg.invert_lrck = true;
	zassert_ok(audio_codec_es7210_configure(codec(0U), &cfg));
	zassert_equal(data->regs[0x03], 4U);
	zassert_equal(data->regs[0x04], 1U);
	zassert_equal(data->regs[0x05], 0U);
	zassert_equal(data->regs[0x08], 0x1dU);
	zassert_equal(data->regs[0x11], 0x70U);
	zassert_ok(audio_codec_start(codec(0U), AUDIO_DAI_DIR_RX));
	zassert_equal(data->regs[0x00], 1U);
	zassert_equal(data->regs[0x01], 0U);
	zassert_equal(audio_codec_es7210_configure(codec(0U), &cfg), -EBUSY);
	zassert_ok(audio_codec_stop(codec(0U), AUDIO_DAI_DIR_RX));
	cfg.word_size = 24U;
	cfg.slot_width = 24U;
	cfg.mclk_freq = 6144000U;
	zassert_ok(audio_codec_es7210_configure(codec(0U), &cfg));
	zassert_equal(data->regs[0x03], 4U);
	zassert_equal(data->regs[0x02], 3U);
	zassert_equal(data->regs[0x04], 1U);
	zassert_equal(data->regs[0x05], 0x80U);
	cfg.format = AUDIO_CODEC_ES7210_DSP_A;
	zassert_equal(audio_codec_es7210_configure(codec(0U), &cfg), -ENOTSUP);
	cfg = serial_configuration();
	zassert_equal(audio_codec_es7210_configure(codec(1U), &cfg), -ENOTSUP);
	cfg.word_size = 20U;
	cfg.slot_width = 32U;
	zassert_equal(audio_codec_es7210_configure(codec(0U), &cfg), -ENOTSUP);
}

ZTEST(es7210, test_extended_clocks_and_word_lengths)
{
	static const uint32_t rates[] = {
		8000U,  11025U, 12000U, 16000U, 22050U, 24000U,
		32000U, 44100U, 48000U, 64000U, 88200U, 96000U,
	};
	static const uint8_t widths[] = {16U, 18U, 20U, 24U, 32U};
	static const uint8_t words[] = {0x60U, 0x40U, 0x20U, 0U, 0x80U};
	static const uint32_t usb_rates[] = {8000U, 12000U, 16000U, 24000U, 32000U, 48000U, 96000U};
	static const uint8_t usb_mainclk[] = {0x5eU, 0x54U, 0x0aU, 0x4aU, 0x05U, 0x45U, 0x45U};
	static const uint8_t usb_osr[] = {40U, 40U, 30U, 40U, 30U, 40U, 40U};
	struct audio_codec_es7210_dai_cfg cfg = serial_configuration();
	struct es7210_emul_data *data = registers(0U);

	prepare(0U);
	cfg.channels = 2U;
	for (size_t i = 0; i < ARRAY_SIZE(rates); i++) {
		cfg.sample_rate = rates[i];
		cfg.mclk_freq = rates[i] * 256U;
		zassert_ok(audio_codec_es7210_configure(codec(0U), &cfg));
		zassert_equal(data->regs[0x02], rates[i] > 48000U ? 0x81U : 0xc1U);
		zassert_equal(data->regs[0x07], 32U);
		zassert_equal(data->regs[0x08], rates[i] > 48000U ? 0x16U : 0x14U);
	}
	cfg.sample_rate = 16000U;
	cfg.mclk_freq = 6144000U;
	for (size_t i = 0; i < ARRAY_SIZE(widths); i++) {
		cfg.word_size = widths[i];
		cfg.slot_width = 32U;
		zassert_ok(audio_codec_es7210_configure(codec(0U), &cfg));
		zassert_equal(data->regs[0x11], words[i]);
	}
	for (size_t i = 0; i < ARRAY_SIZE(usb_rates); i++) {
		cfg.sample_rate = usb_rates[i];
		cfg.mclk_freq = 19200000U;
		zassert_ok(audio_codec_es7210_configure(codec(0U), &cfg));
		zassert_equal(data->regs[0x02], usb_mainclk[i]);
		zassert_equal(data->regs[0x07], usb_osr[i]);
	}
	cfg.sample_rate = 64000U;
	zassert_equal(audio_codec_es7210_configure(codec(0U), &cfg), -ENOTSUP);
	cfg.sample_rate = 0U;
	zassert_equal(audio_codec_es7210_configure(codec(0U), &cfg), -ENOTSUP);
	cfg = serial_configuration();
	cfg.channels = 2U;
	cfg.word_size = 24U;
	cfg.slot_width = 24U;
	cfg.clock_controller = true;
	zassert_equal(audio_codec_es7210_configure(codec(0U), &cfg), -ENOTSUP);
}

ZTEST(es7210, test_processing_routing_gain_and_alc)
{
	struct audio_codec_es7210_dai_cfg cfg = serial_configuration();
	struct audio_codec_es7210_processing proc;
	struct es7210_emul_data *data = registers(0U);
	audio_property_value_t value = {.mute = true};

	prepare(0U);
	zassert_ok(audio_codec_es7210_configure(codec(0U), &cfg));
	zassert_ok(audio_codec_es7210_get_processing(codec(0U), &proc));
	proc.mic_mask = 0x0bU;
	proc.channel[0].pga_gain = 69;
	proc.channel[1].pga_gain = 75;
	proc.channel[2].digital_gain = -191;
	proc.channel[3].digital_gain = 64;
	proc.channel[0].digital_gain = -20;
	proc.channel[0].invert = true;
	proc.channel[3].invert = true;
	proc.pair[0].route = AUDIO_CODEC_ES7210_ROUTE_SWAP;
	proc.pair[1].route = AUDIO_CODEC_ES7210_ROUTE_LEFT;
	proc.pair[0].hpf_coefficient1 = 13U;
	proc.pair[0].hpf_coefficient2 = 13U;
	proc.pair[0].freeze_offset = true;
	proc.pair[0].alc_channels = 1U;
	proc.pair[0].alc_min_level = -181;
	proc.pair[0].alc_max_level = -85;
	proc.pair[1].alc_channels = 2U;
	proc.alc_ramp_cycles = 65536U;
	proc.gain_ramp_cycles = 256U;
	zassert_ok(audio_codec_es7210_set_processing(codec(0U), &proc));
	zassert_equal(data->regs[0x43], 0x1cU);
	zassert_equal(data->regs[0x44], 0x1eU);
	zassert_equal(data->regs[0x45], 0U);
	zassert_equal(data->regs[0x46], 0x10U);
	zassert_equal(data->regs[0x1b], 255U);
	zassert_equal(data->regs[0x1c], 0U);
	zassert_equal(data->regs[0x1d], 191U);
	zassert_equal(data->regs[0x1e], 171U);
	zassert_equal(data->regs[0x23], 0x6dU);
	zassert_equal(data->regs[0x22], 0xcdU);
	zassert_equal(data->regs[0x21], 0xaaU);
	zassert_equal(data->regs[0x20], 0x4aU);
	zassert_equal(data->regs[0x19], 0xb3U);
	zassert_equal(data->regs[0x16], 9U);
	zassert_equal(data->regs[0x17], 15U);
	zassert_equal(data->regs[0x1a], 0x70U);
	zassert_ok(audio_codec_start(codec(0U), AUDIO_DAI_DIR_RX));
	zassert_equal(data->regs[0x4b], 0U);
	zassert_equal(data->regs[0x4c], 0U);
	zassert_equal(data->regs[0x14], 1U);
	zassert_equal(audio_codec_es7210_set_processing(codec(0U), &proc), -EBUSY);
	zassert_ok(audio_codec_set_property(codec(0U), AUDIO_PROPERTY_INPUT_MUTE,
					    (audio_channel_t)AUDIO_CODEC_ES7210_MIC4, value));
	zassert_equal(data->regs[0x14], 3U);
	zassert_ok(audio_codec_stop(codec(0U), AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_start(codec(0U), AUDIO_DAI_DIR_RX));
	zassert_equal(data->regs[0x14], 3U);
	zassert_equal(data->regs[0x22], 0xcdU);
	zassert_ok(audio_codec_stop(codec(0U), AUDIO_DAI_DIR_RX));
	value.vol = 33;
	zassert_ok(audio_codec_set_property(codec(0U), AUDIO_PROPERTY_INPUT_VOLUME,
					    (audio_channel_t)AUDIO_CODEC_ES7210_MIC3, value));
	zassert_equal(data->regs[0x45], 11U);
	zassert_ok(audio_codec_es7210_get_processing(codec(0U), &proc));
	zassert_equal(proc.channel[2].pga_gain, 66);
}

ZTEST(es7210, test_digital_microphone_and_validation)
{
	struct audio_codec_es7210_dai_cfg cfg = serial_configuration();
	struct audio_codec_es7210_processing proc;
	struct audio_codec_es7210_processing invalid;
	struct es7210_emul_data *data = registers(0U);

	prepare(0U);
	zassert_ok(audio_codec_es7210_configure(codec(0U), &cfg));
	zassert_ok(audio_codec_es7210_get_processing(codec(0U), &proc));
	proc.pair[0].digital_mic = true;
	proc.pair[0].digital_mic_gain = 42U;
	proc.pair[1].digital_mic = true;
	proc.pair[1].digital_mic_gain = 18U;
	zassert_ok(audio_codec_es7210_set_processing(codec(0U), &proc));
	zassert_equal(data->regs[0x10], 0xdfU);
	zassert_equal(data->regs[0x43], 0U);
	zassert_equal(data->regs[0x46], 0U);
	uint32_t transfers = data->transfers;

	for (uint8_t i = 0U; i < 12U; i++) {
		invalid = proc;
		switch (i) {
		case 0:
			invalid.mic_mask = 0U;
			break;
		case 1:
			invalid.mic_mask = 16U;
			break;
		case 2:
			invalid.channel[0].pga_gain = 68;
			break;
		case 3:
			invalid.channel[0].digital_gain = -192;
			break;
		case 4:
			invalid.channel[0].digital_gain = 65;
			break;
		case 5:
			invalid.pair[0].hpf_coefficient1 = 32U;
			break;
		case 6:
			invalid.pair[0].digital_mic_gain = 7U;
			break;
		case 7:
			invalid.pair[0].alc_min_level = -100;
			break;
		case 8:
			invalid.pair[0].alc_min_level = -60;
			invalid.pair[0].alc_max_level = -120;
			break;
		case 9:
			invalid.alc_ramp_cycles = 3U;
			break;
		case 10:
			invalid.gain_ramp_cycles = 2U;
			break;
		case 11:
			invalid.pair[0].alc_channels = 4U;
			break;
		default:
			zassert_unreachable();
		}
		zassert_equal(audio_codec_es7210_set_processing(codec(0U), &invalid), -EINVAL);
	}
	zassert_equal(data->transfers, transfers);
}

ZTEST(es7210, test_processing_and_status_errors)
{
	struct audio_codec_es7210_processing proc;
	struct es7210_emul_data *data = registers(0U);
	struct audio_codec_es7210_status status;
	uint32_t count;

	prepare(0U);
	zassert_ok(audio_codec_es7210_get_processing(codec(0U), &proc));
	count = data->transfers;
	zassert_ok(audio_codec_es7210_set_processing(codec(0U), &proc));
	count = data->transfers - count;
	for (uint32_t i = 1U; i <= count; i++) {
		data->fail_at = data->transfers + i;
		zassert_equal(audio_codec_es7210_set_processing(codec(0U), &proc), -EIO);
		assert_stopped(0U);
		zassert_equal(audio_codec_es7210_get_processing(codec(0U), &proc), -EIO);
		prepare(0U);
	}
	data->regs[0x3f] = 0x13U;
	data->regs[0x0b] = 0xc2U;
	zassert_ok(audio_codec_es7210_get_status(codec(0U), &status));
	zassert_equal(status.revision, 0x13U);
	zassert_equal(status.state, 2U);
	zassert_true(status.configured);
	zassert_false(status.running);
	for (uint8_t i = 1U; i <= 2U; i++) {
		status.state = 99U;
		data->fail_at = data->transfers + i;
		zassert_equal(audio_codec_es7210_get_status(codec(0U), &status), -EIO);
		zassert_equal(status.state, 99U);
	}
	data->fail_at = 0U;
	zassert_equal(audio_codec_es7210_get_processing(NULL, &proc), -EINVAL);
	zassert_equal(audio_codec_es7210_set_processing(codec(0U), NULL), -EINVAL);
	zassert_equal(audio_codec_es7210_get_status(codec(0U), NULL), -EINVAL);
	zassert_equal(audio_codec_es7210_configure(NULL, NULL), -EINVAL);
	zassert_equal(audio_codec_es7210_configure(codec(0U), NULL), -EINVAL);
}

ZTEST(es7210, test_single_input_keeps_pair_powered)
{
	for (uint8_t pair = 0U; pair < 2U; pair++) {
		struct audio_codec_es7210_processing proc;
		struct es7210_emul_data *data = registers(pair);

		prepare(pair);
		for (uint8_t input = 0U; input < 2U; input++) {
			zassert_ok(audio_codec_es7210_get_processing(codec(pair), &proc));
			proc.mic_mask = BIT(pair * 2U + input);
			zassert_ok(audio_codec_es7210_set_processing(codec(pair), &proc));
			zassert_ok(audio_codec_start(codec(pair), AUDIO_DAI_DIR_RX));
			zassert_equal(data->regs[0x4b + pair], 0U);
			zassert_equal(data->regs[0x4c - pair], 0xffU);
			zassert_equal(data->regs[0x15 - pair], input == 0U ? 2U : 1U);
			zassert_equal(data->regs[0x43 + pair * 2U + input], 0x10U);
			zassert_equal(data->regs[0x43 + pair * 2U + (1U - input)], 0U);
			zassert_ok(audio_codec_stop(codec(pair), AUDIO_DAI_DIR_RX));
		}
	}
}

ZTEST(es7210, test_low_voltage_constraints)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(adc_low_voltage));
	struct audio_codec_cfg cfg = configuration();
	struct audio_codec_es7210_processing proc;
	audio_property_value_t value = {.vol = 18};

	zassert_true(device_is_ready(dev));
	zassert_ok(audio_codec_stop(dev, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_configure(dev, &cfg));
	zassert_ok(audio_codec_es7210_get_processing(dev, &proc));
	zassert_equal(proc.channel[0].pga_gain, 42);
	zassert_equal(audio_codec_set_property(dev, AUDIO_PROPERTY_INPUT_VOLUME, AUDIO_CHANNEL_ALL,
					       value),
		      -EINVAL);
	proc.channel[0].pga_gain = 36;
	zassert_equal(audio_codec_es7210_set_processing(dev, &proc), -EINVAL);
	cfg.dai_cfg.i2s.frame_clk_freq = 32000U;
	cfg.mclk_freq = 8192000U;
	zassert_equal(audio_codec_configure(dev, &cfg), -ENOTSUP);
}

ZTEST_SUITE(es7210, NULL, NULL, NULL, NULL, NULL);
