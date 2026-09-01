/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/mpipe_pipeline.h>
#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/aud/mpipe_aud.h>
#include <zephyr/mpipe/aud/mpipe_aud_src.h>
#include <zephyr/mpipe/aud/mpipe_aud_i2s_src.h>
#include <zephyr/mpipe/aud/mpipe_aud_i2s_codec_sink.h>
#include <zephyr/mpipe/base/mpipe_caps_filter.h>

/* The audio buffer pool initializes this slab during negotiation. */
__nocache struct k_mem_slab test_slab;

static struct mpipe pipe;
static struct mpipe_aud_i2s_src src;
static struct mpipe_aud_i2s_codec_sink sink;
static struct mpipe_caps_filter caps_filter;

ZTEST(mpipe_aud_i2s_src, test_enum_caps_nonempty)
{
	struct mpipe_aud_i2s_src local;
	struct mpipe_structure out;
	int ret;

	zassert_ok(mpipe_aud_i2s_src_init(&local, 1));

	ret = mpipe_pad_enum_caps(&local.aud_src.src.src_pad, 0, NULL, &out);
	zassert_ok(ret, "enum_caps index 0 returned %d", ret);

	mpipe_structure_clear(&out);
}

/* READY->PAUSED prepares I2S; only PLAYING may start the hardware streams. */
ZTEST(mpipe_aud_i2s_src, test_pipeline_starts_i2s_only_while_playing)
{
	struct mpipe_structure caps;

	zassert_ok(mpipe_pipeline_init(&pipe, 0));
	zassert_ok(mpipe_aud_i2s_src_init(&src, 1));
	zassert_ok(mpipe_caps_filter_init(&caps_filter, 2));
	zassert_ok(mpipe_aud_i2s_codec_sink_init(&sink, 3));

	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&src,
					       MPIPE_PROP_AUD_SRC_SLAB_PTR, &test_slab,
					       MPIPE_PROP_LIST_END));
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&sink,
					       MPIPE_PROP_AUD_SINK_SLAB_PTR, &test_slab,
					       MPIPE_PROP_LIST_END));

	zassert_ok(mpipe_structure_init_fields(
		&caps, MPIPE_MEDIA_AUDIO_PCM, MPIPE_CAPS_FRAME_INTERVAL, MPIPE_TYPE_UINT, 10000,
		MPIPE_CAPS_NUM_OF_CHANNEL, MPIPE_TYPE_UINT, 2, MPIPE_CAPS_END));
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&caps_filter,
					       MPIPE_PROP_BASE_CAPS_FILTER_CAPS, &caps,
					       MPIPE_PROP_LIST_END));

	zassert_ok(mpipe_element_link((struct mpipe_element *)&src,
				      (struct mpipe_element *)&caps_filter,
				      (struct mpipe_element *)&sink, NULL));
	zassert_ok(mpipe_bin_add((struct mpipe_bin *)&pipe, (struct mpipe_element *)&src,
				 (struct mpipe_element *)&caps_filter,
				 (struct mpipe_element *)&sink, NULL));

	zassert_equal(mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_PAUSED),
		      MPIPE_STATE_CHANGE_SUCCESS, "negotiation to PAUSED failed");
	zassert_false(sink.started, "TX started before the pipeline reached PLAYING");
	zassert_equal(mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_PLAYING),
		      MPIPE_STATE_CHANGE_SUCCESS);
	zassert_not_ok(i2s_trigger(src.pool.aud_dev, I2S_DIR_RX, I2S_TRIGGER_START),
		       "RX was not running after the pipeline reached PLAYING");
	zassert_true(sink.started, "sync-clocked capture left the TX clock stopped");
	zassert_equal(sink.count, CONFIG_MPIPE_AUD_I2S_CODEC_SINK_SILENCE_PRIME_COUNT,
		      "TX did not retain the configured silence prime");
	zassert_equal(mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_PAUSED),
		      MPIPE_STATE_CHANGE_SUCCESS);
	zassert_equal(mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_PLAYING),
		      MPIPE_STATE_CHANGE_SUCCESS,
		      "RX was not stopped and restarted across pause");

	(void)mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_READY);
}

ZTEST_SUITE(mpipe_aud_i2s_src, NULL, NULL, NULL, NULL, NULL);
