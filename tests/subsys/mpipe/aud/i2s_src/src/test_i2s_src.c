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

#define PREPARE_TEST_BLOCK_SIZE  1920
#define PREPARE_TEST_BLOCK_COUNT 5

K_MEM_SLAB_DEFINE_STATIC(prepare_test_slab, PREPARE_TEST_BLOCK_SIZE, PREPARE_TEST_BLOCK_COUNT, 4);

struct prepare_probe_i2s_data {
	struct i2s_config config;
	uint32_t configure_count;
	uint32_t prepare_count;
};

static int prepare_probe_i2s_configure(const struct device *dev, enum i2s_dir dir,
				       const struct i2s_config *config)
{
	struct prepare_probe_i2s_data *data = dev->data;

	ARG_UNUSED(dir);
	data->config = *config;
	data->configure_count++;
	return 0;
}

static const struct i2s_config *prepare_probe_i2s_config_get(const struct device *dev,
							     enum i2s_dir dir)
{
	struct prepare_probe_i2s_data *data = dev->data;

	ARG_UNUSED(dir);
	return &data->config;
}

static int prepare_probe_i2s_read(const struct device *dev, void **mem_block, size_t *size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mem_block);
	ARG_UNUSED(size);
	return -ENOSYS;
}

static int prepare_probe_i2s_write(const struct device *dev, void *mem_block, size_t size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mem_block);
	ARG_UNUSED(size);
	return -ENOSYS;
}

static int prepare_probe_i2s_trigger(const struct device *dev, enum i2s_dir dir,
				     enum i2s_trigger_cmd cmd)
{
	struct prepare_probe_i2s_data *data = dev->data;

	ARG_UNUSED(dir);
	if (cmd == I2S_TRIGGER_PREPARE) {
		data->prepare_count++;
	}
	return 0;
}

static DEVICE_API(i2s, prepare_probe_i2s_api) = {
	.configure = prepare_probe_i2s_configure,
	.config_get = prepare_probe_i2s_config_get,
	.read = prepare_probe_i2s_read,
	.write = prepare_probe_i2s_write,
	.trigger = prepare_probe_i2s_trigger,
};

static struct prepare_probe_i2s_data prepare_probe_i2s_data;

DEVICE_DEFINE(prepare_probe_i2s, "prepare_probe_i2s", NULL, NULL, &prepare_probe_i2s_data, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &prepare_probe_i2s_api);

/* The teardown case needs its own graph: the objects above are already linked. */
__nocache struct k_mem_slab td_slab;
static struct mpipe td_pipe;
static struct mpipe_aud_i2s_src td_src;
static struct mpipe_aud_i2s_codec_sink td_sink;
static struct mpipe_caps_filter td_caps_filter;

ZTEST(mpipe_aud_i2s_src, test_enum_caps_nonempty)
{
	struct mpipe_aud_i2s_src local;
	struct mpipe_structure out;
	int ret;

	zassert_ok(mpipe_aud_i2s_src_init(&local, 1, DEVICE_DT_GET(DT_ALIAS(i2s_codec_rx))));

	ret = mpipe_pad_enum_caps(&local.aud_src.src.src_pad, 0, NULL, &out);
	zassert_ok(ret, "enum_caps index 0 returned %d", ret);

	mpipe_structure_clear(&out);
}

ZTEST(mpipe_aud_i2s_src, test_sink_does_not_prepare_a_healthy_stream)
{
	struct mpipe_aud_i2s_codec_sink local_sink;
	struct mpipe_structure caps;

	memset(&prepare_probe_i2s_data, 0, sizeof(prepare_probe_i2s_data));
	zassert_ok(mpipe_aud_i2s_codec_sink_init(&local_sink, 1));
	local_sink.i2s_dev = DEVICE_GET(prepare_probe_i2s);
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&local_sink,
					       MPIPE_PROP_AUD_SINK_SLAB_PTR, &prepare_test_slab,
					       MPIPE_PROP_LIST_END));
	zassert_ok(mpipe_structure_init_fields(
		&caps, MPIPE_MEDIA_AUDIO_PCM, MPIPE_CAPS_SAMPLE_RATE, MPIPE_TYPE_UINT, 48000,
		MPIPE_CAPS_BITWIDTH, MPIPE_TYPE_UINT, 16, MPIPE_CAPS_NUM_OF_CHANNEL,
		MPIPE_TYPE_UINT, 2, MPIPE_CAPS_FRAME_INTERVAL, MPIPE_TYPE_UINT, 10000,
		MPIPE_CAPS_INTERLEAVED, MPIPE_TYPE_BOOLEAN, true, MPIPE_CAPS_END));

	zassert_ok(local_sink.sink.set_caps(&local_sink.sink, &caps));
	zassert_equal(prepare_probe_i2s_data.configure_count, 1U);
	zassert_equal(prepare_probe_i2s_data.prepare_count, 0U,
		      "a normal configure issued PREPARE outside I2S_STATE_ERROR");
}

/* READY->PAUSED prepares I2S; only PLAYING may start the hardware streams. */
ZTEST(mpipe_aud_i2s_src, test_pipeline_starts_i2s_only_while_playing)
{
	struct mpipe_structure caps;

	zassert_ok(mpipe_pipeline_init(&pipe, 0));
	zassert_ok(mpipe_aud_i2s_src_init(&src, 1, DEVICE_DT_GET(DT_ALIAS(i2s_codec_rx))));
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
		      MPIPE_STATE_CHANGE_SUCCESS, "RX was not stopped and restarted across pause");

	(void)mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_READY);
}

/*
 * Tearing a streaming graph down releases the pool's backing store while the
 * source thread may still be inside i2s_read(). Without the -EPIPE guard in
 * mpipe_aud_i2s_src_acquire_buffer() the block that comes back matches nothing
 * and is freed into a slab that no longer owns it, which panics the kernel.
 *
 * The sleep is what makes this different from the test above: it lets the
 * source thread get a block in flight before the teardown starts.
 */
ZTEST(mpipe_aud_i2s_src, test_teardown_while_streaming)
{
	struct mpipe_structure caps;

	zassert_ok(mpipe_pipeline_init(&td_pipe, 0));
	zassert_ok(mpipe_aud_i2s_src_init(&td_src, 1, DEVICE_DT_GET(DT_ALIAS(i2s_codec_rx))));
	zassert_ok(mpipe_caps_filter_init(&td_caps_filter, 2));
	zassert_ok(mpipe_aud_i2s_codec_sink_init(&td_sink, 3));

	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&td_src,
					       MPIPE_PROP_AUD_SRC_SLAB_PTR, &td_slab,
					       MPIPE_PROP_LIST_END));
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&td_sink,
					       MPIPE_PROP_AUD_SINK_SLAB_PTR, &td_slab,
					       MPIPE_PROP_LIST_END));

	zassert_ok(mpipe_structure_init_fields(
		&caps, MPIPE_MEDIA_AUDIO_PCM, MPIPE_CAPS_FRAME_INTERVAL, MPIPE_TYPE_UINT, 10000,
		MPIPE_CAPS_NUM_OF_CHANNEL, MPIPE_TYPE_UINT, 2, MPIPE_CAPS_END));
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&td_caps_filter,
					       MPIPE_PROP_BASE_CAPS_FILTER_CAPS, &caps,
					       MPIPE_PROP_LIST_END));

	zassert_ok(mpipe_element_link((struct mpipe_element *)&td_src,
				      (struct mpipe_element *)&td_caps_filter,
				      (struct mpipe_element *)&td_sink, NULL));
	zassert_ok(mpipe_bin_add((struct mpipe_bin *)&td_pipe, (struct mpipe_element *)&td_src,
				 (struct mpipe_element *)&td_caps_filter,
				 (struct mpipe_element *)&td_sink, NULL));

	zassert_equal(mpipe_element_set_state((struct mpipe_element *)&td_pipe, MPIPE_STATE_PAUSED),
		      MPIPE_STATE_CHANGE_SUCCESS, "negotiation to PAUSED failed");
	zassert_equal(
		mpipe_element_set_state((struct mpipe_element *)&td_pipe, MPIPE_STATE_PLAYING),
		MPIPE_STATE_CHANGE_SUCCESS);

	/* Let the source thread take a block out of the pool. */
	k_sleep(K_MSEC(50));

	zassert_equal(mpipe_element_set_state((struct mpipe_element *)&td_pipe, MPIPE_STATE_READY),
		      MPIPE_STATE_CHANGE_SUCCESS, "teardown from PLAYING did not complete");
}

ZTEST_SUITE(mpipe_aud_i2s_src, NULL, NULL, NULL, NULL, NULL);
