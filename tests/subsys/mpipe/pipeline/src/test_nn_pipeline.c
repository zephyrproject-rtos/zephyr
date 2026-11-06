/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/ztest.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/base/mpipe_app_sink.h>
#include <zephyr/mpipe/base/mpipe_app_src.h>
#include <zephyr/mpipe/nn/mpipe_nn_backend_fake.h>
#include <zephyr/mpipe/nn/mpipe_nn_converter.h>
#include <zephyr/mpipe/nn/mpipe_nn_decoder.h>
#include <zephyr/mpipe/nn/mpipe_nn_runner.h>

ZBUS_MSG_SUBSCRIBER_DEFINE(test_nn_pipeline_sub);

/* Element IDs (values are arbitrary; only uniqueness within the pipeline matters) */
enum {
	PIPE_ID,
	SRC_ID,
	CONVERTER_ID,
	RUNNER_ID,
	DECODER_ID,
	SINK_ID,
};

/* Number of images the application pushes before EOS */
#define TEST_BUFS_NUM 10

/* The fake model: a 16x16 single-channel int8 classifier with 4 classes */
#define TEST_DIM         16
#define TEST_NUM_CLASSES 4

/* The fake detector: a scored box list of three rows, the second overlapping the first */
#define TEST_NUM_BOXES 3

static uint8_t test_image[TEST_DIM * TEST_DIM];

static struct {
	atomic_t count;
	struct mpipe_nn_results last;
} cb_state;

/* Runs in the pipeline thread: copy the results snapshot out */
static void test_results_cb(const struct net_buf *buf, void *user_data)
{
	ARG_UNUSED(user_data);

	if (mpipe_buffer_get_meta(buf)->bytes_used >= sizeof(struct mpipe_nn_results)) {
		cb_state.last = *(const struct mpipe_nn_results *)(const void *)buf->data;
		atomic_inc(&cb_state.count);
	}
}

struct test_nn_pipeline_fixture {
	struct mpipe pipeline;
	struct mpipe_app_src app_src;
	struct mpipe_nn_converter converter;
	struct mpipe_nn_runner runner;
	struct mpipe_nn_decoder decoder;
	struct mpipe_app_sink app_sink;
	struct zbus_channel *bus;
};

static void *nn_pipeline_suite_setup(void)
{
	static struct test_nn_pipeline_fixture fixture;

	/* A known image: the fake inference derives the winning class from it */
	for (size_t i = 0; i < sizeof(test_image); i++) {
		test_image[i] = (uint8_t)i;
	}

	return &fixture;
}

/* app_src(grey 16x16 image) -> nn_converter -> nn_runner(fake backend) -> nn_decoder -> app_sink */
static void nn_pipeline_before(void *f)
{
	struct test_nn_pipeline_fixture *fix = f;
	static const struct mpipe_nn_fake_model fake_model_cfg = {
		.in_w = TEST_DIM,
		.in_h = TEST_DIM,
		.channels = 1,
		.type = MPIPE_NN_TENSOR_TYPE_INT8,
		.num_classes = TEST_NUM_CLASSES,
	};
	static const struct mpipe_nn_model model = {
		.data = (const uint8_t *)&fake_model_cfg,
		.size = sizeof(fake_model_cfg),
	};
	static uint8_t test_arena[64];
	static const struct mpipe_nn_arena arena = {
		.data = test_arena,
		.size = sizeof(test_arena),
	};
	static const struct mpipe_app_sink_cb cb = {
		.fn = test_results_cb,
		.user_data = NULL,
	};
	struct mpipe_structure caps;

	memset(fix, 0, sizeof(*fix));
	atomic_set(&cb_state.count, 0);
	memset(&cb_state.last, 0, sizeof(cb_state.last));

	zassert_ok(mpipe_pipeline_init(&fix->pipeline, PIPE_ID));
	zassert_ok(mpipe_app_src_init(&fix->app_src, SRC_ID));
	zassert_ok(mpipe_nn_converter_init(&fix->converter, CONVERTER_ID));
	zassert_ok(mpipe_nn_runner_init(&fix->runner, RUNNER_ID));
	zassert_ok(mpipe_nn_decoder_init(&fix->decoder, DECODER_ID));
	zassert_ok(mpipe_app_sink_init(&fix->app_sink, SINK_ID));

	/* The application pushes grey 16x16 frames */
	zassert_ok(mpipe_structure_init_fields(
			   &caps, MPIPE_MEDIA_VIDEO, MPIPE_CAPS_PIXEL_FORMAT, MPIPE_TYPE_UINT,
			   VIDEO_PIX_FMT_GREY, MPIPE_CAPS_IMAGE_WIDTH, MPIPE_TYPE_UINT, TEST_DIM,
			   MPIPE_CAPS_IMAGE_HEIGHT, MPIPE_TYPE_UINT, TEST_DIM, MPIPE_CAPS_END),
		   "Failed to init app_src caps");
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&fix->app_src,
					       MPIPE_PROP_BASE_APP_SRC_CAPS, &caps,
					       MPIPE_PROP_LIST_END),
		   "Failed to set app_src caps");

	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&fix->runner,
					       MPIPE_PROP_NN_MODEL, &model, MPIPE_PROP_NN_ARENA,
					       &arena, MPIPE_PROP_LIST_END),
		   "Failed to set runner properties");

	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&fix->app_sink,
					       MPIPE_PROP_BASE_APP_SINK_CB, &cb,
					       MPIPE_PROP_LIST_END),
		   "Failed to set app_sink callback");

	zassert_ok(mpipe_bin_add((struct mpipe_bin *)&fix->pipeline,
				 (struct mpipe_element *)&fix->app_src,
				 (struct mpipe_element *)&fix->converter,
				 (struct mpipe_element *)&fix->runner,
				 (struct mpipe_element *)&fix->decoder,
				 (struct mpipe_element *)&fix->app_sink, NULL),
		   "Failed to add elements");
	zassert_ok(mpipe_element_link((struct mpipe_element *)&fix->app_src,
				      (struct mpipe_element *)&fix->converter,
				      (struct mpipe_element *)&fix->runner,
				      (struct mpipe_element *)&fix->decoder,
				      (struct mpipe_element *)&fix->app_sink, NULL),
		   "Failed to link elements");

	fix->bus = mpipe_element_get_bus_chan((struct mpipe_element *)&fix->pipeline);
	zassert_ok(zbus_chan_add_obs(fix->bus, &test_nn_pipeline_sub, K_FOREVER),
		   "Failed to add observer to pipeline channel");
}

static void nn_pipeline_after(void *f)
{
	struct test_nn_pipeline_fixture *fix = f;

	/* A failed assertion may have left the pipeline streaming */
	(void)mpipe_element_set_state((struct mpipe_element *)&fix->pipeline, MPIPE_STATE_READY);
	(void)zbus_chan_rm_obs(fix->bus, &test_nn_pipeline_sub, K_FOREVER);

	/* The single-instance fake backend must be free for the next test */
	mpipe_nn_runner_unload(&fix->runner);
}

ZTEST_SUITE(test_nn_pipeline, NULL, nn_pipeline_suite_setup, nn_pipeline_before, nn_pipeline_after,
	    NULL);

/* Push the images and EOS, then wait for the EOS message and check it is the only one */
static void nn_pipeline_stream(struct test_nn_pipeline_fixture *fix, int run)
{
	const struct zbus_channel *chan;
	struct mpipe_message msg;

	atomic_set(&cb_state.count, 0);

	zassert_ok(mpipe_element_set_state((struct mpipe_element *)&fix->pipeline,
					   MPIPE_STATE_PLAYING),
		   "run %d failed to start PLAYING", run);

	for (int i = 0; i < TEST_BUFS_NUM; i++) {
		zassert_ok(mpipe_app_src_push(&fix->app_src, test_image, sizeof(test_image),
					      K_SECONDS(1)),
			   "run %d failed to push image %d", run, i);
	}
	zassert_ok(mpipe_app_src_eos(&fix->app_src, K_SECONDS(1)), "run %d failed to push EOS",
		   run);

	zassert_ok(zbus_sub_wait_msg(&test_nn_pipeline_sub, &chan, &msg, K_FOREVER),
		   "run %d timed out waiting for a pipeline message", run);
	zassert_equal(msg.type, MPIPE_MESSAGE_EOS, "run %d: expected EOS, got %d", run, msg.type);

	/* Exactly one EOS per run: a second would mean the aggregation leaked */
	zassert_equal(zbus_sub_wait_msg(&test_nn_pipeline_sub, &chan, &msg, K_MSEC(50)), -ENOMSG,
		      "run %d produced more than one message", run);

	zassert_equal(atomic_get(&cb_state.count), TEST_BUFS_NUM,
		      "run %d: expected %d results, got %ld", run, TEST_BUFS_NUM,
		      atomic_get(&cb_state.count));
}

/*
 * Negotiation settles on the model's input tensor, every image yields one
 * deterministic classification and EOS reaches the bus, on every run of a replay
 */
ZTEST_F(test_nn_pipeline, test_pipeline_nn_classification)
{
	uint32_t sum = 0;
	uint16_t expected_class;

	/* Raw bytes in the tensor; the fake backend crowns class (sum of bytes) mod num_classes */
	for (size_t i = 0; i < sizeof(test_image); i++) {
		sum += test_image[i];
	}
	expected_class = (uint16_t)(sum % TEST_NUM_CLASSES);

	/* A replay is where state left behind by the previous run shows up */
	for (int run = 0; run < 3; run++) {
		nn_pipeline_stream(fixture, run);

		/* Every image produced one result, in order, numbered from one */
		zassert_equal(cb_state.last.seq, TEST_BUFS_NUM, "run %d: expected seq %d, got %u",
			      run, TEST_BUFS_NUM, cb_state.last.seq);
		zassert_equal(cb_state.last.type, MPIPE_NN_RESULTS_CLASSIFICATION,
			      "run %d: wrong results type", run);
		zassert_equal(cb_state.last.count, TEST_NUM_CLASSES,
			      "run %d: expected %d ranked classes, got %u", run, TEST_NUM_CLASSES,
			      cb_state.last.count);
		zassert_equal(cb_state.last.cls[0].class_id, expected_class,
			      "run %d: expected winning class %u, got %u", run, expected_class,
			      cb_state.last.cls[0].class_id);
		zassert_true(cb_state.last.cls[0].score > 0.9f,
			     "run %d: winning score not dominant", run);

		zassert_ok(mpipe_element_set_state((struct mpipe_element *)&fixture->pipeline,
						   MPIPE_STATE_READY),
			   "run %d failed to return to READY", run);
	}
}

/*
 * The fake detector emits a scored box list: the decoder ranks the rows, drops
 * the one overlapping a better box and hands the coordinates back dequantized
 */
ZTEST_F(test_nn_pipeline, test_pipeline_nn_detection_nms)
{
	static const struct mpipe_nn_fake_model fake_detector_cfg = {
		.in_w = TEST_DIM,
		.in_h = TEST_DIM,
		.channels = 1,
		.type = MPIPE_NN_TENSOR_TYPE_INT8,
		.num_boxes = TEST_NUM_BOXES,
		.box_list = 1,
	};
	static const struct mpipe_nn_model model = {
		.data = (const uint8_t *)&fake_detector_cfg,
		.size = sizeof(fake_detector_cfg),
	};
	static const uint8_t decode_type = MPIPE_NN_RESULTS_DETECTION;
	const struct mpipe_nn_box *box;

	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&fixture->runner,
					       MPIPE_PROP_NN_MODEL, &model, MPIPE_PROP_LIST_END),
		   "Failed to set the detector model");
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&fixture->decoder,
					       MPIPE_PROP_NN_DECODER_TYPE, &decode_type,
					       MPIPE_PROP_LIST_END),
		   "Failed to set the decode type");

	nn_pipeline_stream(fixture, 0);

	zassert_equal(cb_state.last.type, MPIPE_NN_RESULTS_DETECTION, "Wrong results type");

	/* Three rows above the threshold, the duplicate of the best one suppressed */
	zassert_equal(cb_state.last.count, TEST_NUM_BOXES - 1, "Expected %d boxes, got %u",
		      TEST_NUM_BOXES - 1, cb_state.last.count);

	box = &cb_state.last.boxes[0];
	zassert_true(box->score > 0.85f, "Best box score %d%% not the highest",
		     (int)(box->score * 100.0f));
	zassert_true(box->score > cb_state.last.boxes[1].score, "Boxes not ranked by score");
	zassert_equal(box->class_id, 1, "Best box class %u, expected 1", box->class_id);
	zassert_within(box->ymin, 0.25f, 0.01f, "Best box top %d%% not dequantized",
		       (int)(box->ymin * 100.0f));
	zassert_within(box->ymax, 0.75f, 0.01f, "Best box bottom %d%% not dequantized",
		       (int)(box->ymax * 100.0f));

	box = &cb_state.last.boxes[1];
	zassert_within(box->score, 0.7f, 0.02f, "Second box score %d%%, expected 70%%",
		       (int)(box->score * 100.0f));
	zassert_within(box->ymin, 0.8f, 0.01f, "Second box not the lower band one");

	zassert_ok(mpipe_element_set_state((struct mpipe_element *)&fixture->pipeline,
					   MPIPE_STATE_READY),
		   "Failed to return to READY");
}
