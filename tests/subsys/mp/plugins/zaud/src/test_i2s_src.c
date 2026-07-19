/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

static struct mp_zaud_i2s_src i2s_src;

static void i2s_src_before(void *f)
{
	ARG_UNUSED(f);

	memset(&i2s_src, 0, sizeof(i2s_src));
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

ZTEST_SUITE(zaud_i2s_src, NULL, NULL, i2s_src_before, NULL, NULL);
