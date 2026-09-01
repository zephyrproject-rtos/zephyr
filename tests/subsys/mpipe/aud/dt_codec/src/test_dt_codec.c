/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>

#include <zephyr/mpipe/aud/mpipe_aud_i2s_src.h>
#include <zephyr/mpipe/aud/mpipe_aud_i2s_codec_sink.h>

static struct mpipe_aud_i2s_codec_sink sink;
static struct mpipe_aud_i2s_src src;

/* The I2S nodes point at codec1 via a phandle, not the default audio_codec. */
ZTEST(mpipe_dt_codec, test_sink_codec_from_i2s_node)
{
	zassert_ok(mpipe_aud_i2s_codec_sink_init(&sink, 1));
	zassert_equal(sink.codec_dev, DEVICE_DT_GET(DT_NODELABEL(codec1)),
		      "sink codec not resolved from the I2S node phandle");
}

ZTEST(mpipe_dt_codec, test_src_codec_from_i2s_node)
{
	zassert_ok(mpipe_aud_i2s_src_init(&src, 1));
	zassert_equal(src.codec_dev, DEVICE_DT_GET(DT_NODELABEL(codec1)),
		      "source codec not resolved from the I2S node phandle");
}

ZTEST_SUITE(mpipe_dt_codec, NULL, NULL, NULL, NULL, NULL);
