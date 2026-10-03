/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/aud/mpipe_aud.h>
#include <zephyr/mpipe/aud/mpipe_aud_i2s_src.h>
#include <zephyr/mpipe/aud/mpipe_aud_i2s_codec_sink.h>
#include <zephyr/mpipe/base/mpipe_caps_filter.h>

__nocache struct k_mem_slab test_slab;

static struct mpipe pipe;
static struct mpipe_aud_i2s_src src;
static struct mpipe_caps_filter caps_filter;
static struct mpipe_aud_i2s_codec_sink sink;

static void negotiate_and_check(uint32_t rate, uint32_t width)
{
	struct mpipe_structure caps;
	uint32_t got_rate = 0;
	uint32_t got_width = 0;

	/* The source pool clears the app slab on stop, so re-provide it each run. */
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&src,
					       MPIPE_PROP_AUD_SRC_SLAB_PTR, &test_slab,
					       MPIPE_PROP_LIST_END));
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&sink,
					       MPIPE_PROP_AUD_SINK_SLAB_PTR, &test_slab,
					       MPIPE_PROP_LIST_END));

	zassert_ok(mpipe_structure_init_fields(&caps, MPIPE_MEDIA_AUDIO_PCM, MPIPE_CAPS_SAMPLE_RATE,
					       MPIPE_TYPE_UINT, rate, MPIPE_CAPS_BITWIDTH,
					       MPIPE_TYPE_UINT, width, MPIPE_CAPS_NUM_OF_CHANNEL,
					       MPIPE_TYPE_UINT, 2, MPIPE_CAPS_FRAME_INTERVAL,
					       MPIPE_TYPE_UINT, 10000, MPIPE_CAPS_END));
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&caps_filter,
					       MPIPE_PROP_BASE_CAPS_FILTER_CAPS, &caps,
					       MPIPE_PROP_LIST_END));

	zassert_equal(mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_PAUSED),
		      0, "negotiation of %u/%u failed", rate, width);

	zassert_ok(mpipe_aud_caps_get_uint(&src.aud_src.src.src_pad.caps, MPIPE_CAPS_SAMPLE_RATE,
					   &got_rate));
	zassert_ok(mpipe_aud_caps_get_uint(&src.aud_src.src.src_pad.caps, MPIPE_CAPS_BITWIDTH,
					   &got_width));
	zassert_equal(got_rate, rate, "negotiated rate %u, wanted %u", got_rate, rate);
	zassert_equal(got_width, width, "negotiated width %u, wanted %u", got_width, width);

	(void)mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_READY);
}

ZTEST(mpipe_caps_combo, test_16b48k_and_32b96k)
{
	zassert_ok(mpipe_pipeline_init(&pipe, 0));
	zassert_ok(mpipe_aud_i2s_src_init(&src, 1, DEVICE_DT_GET(DT_ALIAS(i2s_codec_rx))));
	zassert_ok(mpipe_caps_filter_init(&caps_filter, 2));
	zassert_ok(mpipe_aud_i2s_codec_sink_init(&sink, 3));

	zassert_ok(mpipe_element_link((struct mpipe_element *)&src,
				      (struct mpipe_element *)&caps_filter,
				      (struct mpipe_element *)&sink, NULL));
	zassert_ok(mpipe_bin_add((struct mpipe_bin *)&pipe, (struct mpipe_element *)&src,
				 (struct mpipe_element *)&caps_filter,
				 (struct mpipe_element *)&sink, NULL));

	negotiate_and_check(48000, 16);
	negotiate_and_check(96000, 32);
}

__nocache struct k_mem_slab nc_slab;
static struct mpipe nc_pipe;
static struct mpipe_aud_i2s_src nc_src;
static struct mpipe_caps_filter nc_caps_filter;
static struct mpipe_aud_i2s_codec_sink nc_sink;

/* An I2S amplifier has no codec to configure: the I2S link alone decides the format. */
ZTEST(mpipe_caps_combo, test_sink_without_codec)
{
	struct mpipe_structure caps;
	uint32_t got_rate = 0;

	zassert_ok(mpipe_pipeline_init(&nc_pipe, 0));
	zassert_ok(mpipe_aud_i2s_src_init(&nc_src, 1, DEVICE_DT_GET(DT_ALIAS(i2s_codec_rx))));
	zassert_ok(mpipe_caps_filter_init(&nc_caps_filter, 2));
	zassert_ok(mpipe_aud_i2s_codec_sink_init(&nc_sink, 3));

	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&nc_sink,
					       MPIPE_PROP_AUD_SINK_CODEC_DEVICE, NULL,
					       MPIPE_PROP_AUD_SINK_SLAB_PTR, &nc_slab,
					       MPIPE_PROP_LIST_END));
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&nc_src,
					       MPIPE_PROP_AUD_SRC_SLAB_PTR, &nc_slab,
					       MPIPE_PROP_LIST_END));
	zassert_is_null(nc_sink.codec_dev);

	zassert_ok(mpipe_structure_init_fields(&caps, MPIPE_MEDIA_AUDIO_PCM, MPIPE_CAPS_SAMPLE_RATE,
					       MPIPE_TYPE_UINT, 16000, MPIPE_CAPS_BITWIDTH,
					       MPIPE_TYPE_UINT, 16, MPIPE_CAPS_NUM_OF_CHANNEL,
					       MPIPE_TYPE_UINT, 2, MPIPE_CAPS_FRAME_INTERVAL,
					       MPIPE_TYPE_UINT, 10000, MPIPE_CAPS_END));
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&nc_caps_filter,
					       MPIPE_PROP_BASE_CAPS_FILTER_CAPS, &caps,
					       MPIPE_PROP_LIST_END));

	zassert_ok(mpipe_element_link((struct mpipe_element *)&nc_src,
				      (struct mpipe_element *)&nc_caps_filter,
				      (struct mpipe_element *)&nc_sink, NULL));
	zassert_ok(mpipe_bin_add((struct mpipe_bin *)&nc_pipe, (struct mpipe_element *)&nc_src,
				 (struct mpipe_element *)&nc_caps_filter,
				 (struct mpipe_element *)&nc_sink, NULL));

	zassert_equal(mpipe_element_set_state((struct mpipe_element *)&nc_pipe,
					      MPIPE_STATE_PAUSED),
		      0, "negotiation without a codec failed");
	zassert_ok(mpipe_aud_caps_get_uint(&nc_src.aud_src.src.src_pad.caps, MPIPE_CAPS_SAMPLE_RATE,
					   &got_rate));
	zassert_equal(got_rate, 16000, "negotiated rate %u, wanted 16000", got_rate);

	(void)mpipe_element_set_state((struct mpipe_element *)&nc_pipe, MPIPE_STATE_READY);
}

ZTEST_SUITE(mpipe_caps_combo, NULL, NULL, NULL, NULL, NULL);
