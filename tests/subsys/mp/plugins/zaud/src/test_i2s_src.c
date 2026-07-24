/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_structure.h>
#include <zephyr/mp/core/mp_value.h>
#include <zephyr/mp/zaud/mp_zaud.h>
#include <zephyr/mp/zaud/mp_zaud_i2s_src.h>
#include <zephyr/mp/zaud/mp_zaud_property.h>

#define TEST_I2S_SRC_ID  1
#define TEST_BLOCK_SIZE  1024
#define TEST_BLOCK_COUNT 4

K_MEM_SLAB_DEFINE_STATIC(test_i2s_slab, TEST_BLOCK_SIZE, TEST_BLOCK_COUNT, 4);

static const struct device *const i2s_rx_dev = DEVICE_DT_GET(DT_ALIAS(i2s_codec_rx));

/*
 * A codec that records how it was configured. The dummy codec keeps its
 * configuration private, and what matters here is which route the source asks
 * for, so use a local fake instead of adding a test hook to a driver.
 */
struct fake_codec_data {
	struct audio_codec_cfg cfg;
	bool configured;
};

static struct fake_codec_data fake_codec_data;

static int fake_codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct fake_codec_data *data = dev->data;

	if (cfg == NULL) {
		return -EINVAL;
	}

	data->cfg = *cfg;
	data->configured = true;

	return 0;
}

static void fake_codec_start_output(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void fake_codec_stop_output(const struct device *dev)
{
	ARG_UNUSED(dev);
}

/* Deliberately narrower than the I2S receiver so intersection is observable. */
static int fake_codec_get_caps(const struct device *dev, struct audio_caps *caps)
{
	ARG_UNUSED(dev);

	if (caps == NULL) {
		return -EINVAL;
	}

	memset(caps, 0, sizeof(*caps));
	caps->min_total_channels = 2U;
	caps->max_total_channels = 2U;
	caps->supported_sample_rates = AUDIO_SAMPLE_RATE_48000;
	caps->supported_bit_widths = AUDIO_BIT_WIDTH_16;
	caps->min_num_buffers = 2U;
	caps->min_frame_interval = 1000U;
	caps->max_frame_interval = 100000U;
	caps->interleaved = true;

	return 0;
}

static DEVICE_API(audio_codec, fake_codec_api) = {
	.configure = fake_codec_configure,
	.start_output = fake_codec_start_output,
	.stop_output = fake_codec_stop_output,
	.get_caps = fake_codec_get_caps,
};

static int fake_codec_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

DEVICE_DEFINE(fake_codec, "fake_codec", fake_codec_init, NULL, &fake_codec_data, NULL, POST_KERNEL,
	      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &fake_codec_api);

static const struct device *const fake_codec_dev = DEVICE_GET(fake_codec);

static struct mp_zaud_i2s_src i2s_src;

static void i2s_src_before(void *f)
{
	ARG_UNUSED(f);

	memset(&i2s_src, 0, sizeof(i2s_src));
	memset(&fake_codec_data, 0, sizeof(fake_codec_data));
	MP_ELEMENT_INIT(&i2s_src, mp_zaud_i2s_src_init, TEST_I2S_SRC_ID);
}

ZTEST(zaud_i2s_src, test_binds_the_i2s_receiver_from_devicetree)
{
	const struct device *dev = NULL;

	zassert_true(device_is_ready(i2s_rx_dev), "I2S receiver not ready");

	zassert_ok(mp_object_get_properties((struct mp_object *)&i2s_src, PROP_ZAUD_SRC_DEVICE,
					    &dev, PROP_LIST_END));
	zassert_equal(dev, i2s_rx_dev, "element did not bind the i2s-codec-rx alias");
}

ZTEST(zaud_i2s_src, test_slab_property_round_trips)
{
	struct k_mem_slab *slab = NULL;

	zassert_ok(mp_object_set_properties((struct mp_object *)&i2s_src, PROP_ZAUD_SRC_SLAB_PTR,
					    &test_i2s_slab, PROP_LIST_END));
	zassert_ok(mp_object_get_properties((struct mp_object *)&i2s_src, PROP_ZAUD_SRC_SLAB_PTR,
					    &slab, PROP_LIST_END));
	zassert_equal(slab, &test_i2s_slab, "memory slab property did not round-trip");
}

/*
 * The element must advertise what the I2S receiver reports for the receive
 * direction, which is what lets it be negotiated against the rest of a
 * pipeline.
 */
ZTEST(zaud_i2s_src, test_advertises_receiver_capabilities)
{
	struct mp_caps *caps = i2s_src.zaud_src.src.srcpad.caps;
	struct mp_structure *structure;
	struct audio_caps hw_caps;

	zassert_not_null(caps, "source advertised no capabilities");

	structure = mp_caps_get_structure(caps, 0);
	zassert_not_null(structure, "capabilities carry no structure");

	zassert_ok(i2s_get_caps(i2s_rx_dev, &hw_caps, I2S_DIR_RX));

	zassert_not_null(mp_structure_get_value(structure, MP_CAPS_SAMPLE_RATE),
			 "no sample rate advertised");
	zassert_not_null(mp_structure_get_value(structure, MP_CAPS_BITWIDTH),
			 "no bit width advertised");
	zassert_not_null(mp_structure_get_value(structure, MP_CAPS_NUM_OF_CHANNEL),
			 "no channel count advertised");
	zassert_not_null(mp_structure_get_value(structure, MP_CAPS_BUFFER_COUNT),
			 "no buffer count advertised");
}

/*
 * get_audio_caps() must be bound to the RX direction. A source that queried
 * I2S_DIR_TX would negotiate against the transmitter's capabilities and then
 * fail to configure, so check the two agree.
 */
ZTEST(zaud_i2s_src, test_queries_the_receive_direction)
{
	struct audio_caps rx_caps;

	zassert_not_null(i2s_src.zaud_src.get_audio_caps, "get_audio_caps not bound");

	zassert_ok(i2s_src.zaud_src.get_audio_caps(i2s_rx_dev, &rx_caps),
		   "get_audio_caps failed on the receiver");

	zassert_ok(i2s_get_caps(i2s_rx_dev, &rx_caps, I2S_DIR_RX),
		   "receiver does not support I2S_DIR_RX");
}

ZTEST(zaud_i2s_src, test_set_caps_without_a_slab_is_rejected)
{
	struct mp_caps *caps = mp_caps_new(
		MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_UINT, 16000, MP_CAPS_BITWIDTH,
		MP_TYPE_UINT, 16, MP_CAPS_NUM_OF_CHANNEL, MP_TYPE_UINT, 2, MP_CAPS_FRAME_INTERVAL,
		MP_TYPE_UINT, 10000, MP_CAPS_END);

	zassert_not_null(caps);

	/* No slab was set, so configuring the receiver must fail cleanly. */
	zassert_not_equal(i2s_src.zaud_src.src.set_caps(&i2s_src.zaud_src.src, caps), 0,
			  "set_caps accepted a missing memory slab");

	mp_caps_unref(caps);
}

/*
 * A capture codec is optional. When one is attached the source must only offer
 * formats the codec also supports, otherwise negotiation can settle on
 * something the codec cannot capture.
 */
ZTEST(zaud_i2s_src, test_codec_narrows_the_advertised_capabilities)
{
	struct audio_caps i2s_caps;
	struct audio_caps codec_caps;
	struct mp_structure *structure;
	struct mp_value *rates;

	zassert_ok(i2s_get_caps(i2s_rx_dev, &i2s_caps, I2S_DIR_RX));
	zassert_ok(audio_codec_get_caps(fake_codec_dev, &codec_caps));

	/* The codec must be the narrower of the two or there is nothing to test. */
	zassert_not_equal(i2s_caps.supported_sample_rates, codec_caps.supported_sample_rates,
			  "receiver and codec advertise the same rates, nothing to narrow");

	zassert_ok(mp_object_set_properties((struct mp_object *)&i2s_src,
					    PROP_ZAUD_SRC_CODEC_DEVICE, fake_codec_dev,
					    PROP_LIST_END));

	structure = mp_caps_get_structure(i2s_src.zaud_src.src.srcpad.caps, 0);
	zassert_not_null(structure, "no caps after attaching a codec");

	rates = mp_structure_get_value(structure, MP_CAPS_SAMPLE_RATE);
	zassert_not_null(rates, "no sample rates after attaching a codec");

	/* The codec supports one rate, so that is all the source may offer. */
	zassert_equal(mp_value_list_get_size(rates), 1, "expected the codec's single rate, got %zu",
		      mp_value_list_get_size(rates));
	zassert_equal(mp_value_get_uint(mp_value_list_get(rates, 0)), 48000,
		      "advertised rate is not the one the codec supports");
}

ZTEST(zaud_i2s_src, test_codec_property_round_trips)
{
	const struct device *read_back = NULL;

	zassert_ok(mp_object_set_properties((struct mp_object *)&i2s_src,
					    PROP_ZAUD_SRC_CODEC_DEVICE, fake_codec_dev,
					    PROP_LIST_END));
	zassert_ok(mp_object_get_properties((struct mp_object *)&i2s_src,
					    PROP_ZAUD_SRC_CODEC_DEVICE, &read_back, PROP_LIST_END));
	zassert_equal(read_back, fake_codec_dev, "codec property did not round-trip");
}

ZTEST(zaud_i2s_src, test_clock_role_round_trips_and_rejects_junk)
{
	enum mp_zaud_i2s_codec_clk_role role;

	zassert_ok(mp_object_set_properties((struct mp_object *)&i2s_src, PROP_ZAUD_SRC_CLK_ROLE,
					    (void *)MP_ZAUD_I2S_TARGET_CODEC_CONTROLLER,
					    PROP_LIST_END));
	zassert_ok(mp_object_get_properties((struct mp_object *)&i2s_src, PROP_ZAUD_SRC_CLK_ROLE,
					    &role, PROP_LIST_END));
	zassert_equal(role, MP_ZAUD_I2S_TARGET_CODEC_CONTROLLER, "clock role did not round-trip");

	zassert_not_equal(mp_object_set_properties((struct mp_object *)&i2s_src,
						   PROP_ZAUD_SRC_CLK_ROLE, (void *)0xBADU,
						   PROP_LIST_END),
			  0, "an invalid clock role was accepted");
}

/*
 * The capture codec has to be configured for capture. Asking for playback would
 * leave the ADC untouched on codecs that treat the routes as mutually exclusive
 * - wm8904, wm8962 and da7212 all do - and the source would capture nothing.
 */
ZTEST(zaud_i2s_src, test_codec_is_configured_for_capture)
{
	struct mp_caps *caps;

	zassert_ok(mp_object_set_properties((struct mp_object *)&i2s_src, PROP_ZAUD_SRC_SLAB_PTR,
					    &test_i2s_slab, PROP_LIST_END));
	zassert_ok(mp_object_set_properties((struct mp_object *)&i2s_src,
					    PROP_ZAUD_SRC_CODEC_DEVICE, fake_codec_dev,
					    PROP_LIST_END));

	caps = mp_caps_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_UINT, 48000,
			   MP_CAPS_BITWIDTH, MP_TYPE_UINT, 16, MP_CAPS_NUM_OF_CHANNEL, MP_TYPE_UINT,
			   2, MP_CAPS_FRAME_INTERVAL, MP_TYPE_UINT, 10000, MP_CAPS_END);
	zassert_not_null(caps);

	zassert_ok(i2s_src.zaud_src.src.set_caps(&i2s_src.zaud_src.src, caps),
		   "set_caps failed with a capture codec attached");

	zassert_true(fake_codec_data.configured, "capture codec was never configured");
	zassert_equal(fake_codec_data.cfg.dai_route, AUDIO_ROUTE_CAPTURE,
		      "capture codec was not configured for capture");
	zassert_equal(fake_codec_data.cfg.dai_cfg.i2s.frame_clk_freq, 48000,
		      "codec was configured with the wrong sample rate");

	mp_caps_unref(caps);
}

ZTEST_SUITE(zaud_i2s_src, NULL, NULL, i2s_src_before, NULL, NULL);
