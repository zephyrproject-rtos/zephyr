/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Live classification: the camera stream is shown while a neural network
 * classifies the frames on a second branch of the pipeline.
 *
 *   vid_src -> caps_filter -> tee -> queue_d -> [vid_trans] -> disp_sink
 *                                -> queue_nn -> nn_converter -> nn_runner
 *                                               -> nn_decoder -> app_sink
 */

#include <string.h>

#include <zephyr/drivers/video.h>
#include <zephyr/video/controls.h>
#include <zephyr/logging/log.h>
#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/base/mpipe_app_sink.h>
#include <zephyr/mpipe/base/mpipe_caps_filter.h>
#include <zephyr/mpipe/base/mpipe_queue.h>
#include <zephyr/mpipe/base/mpipe_tee.h>
#include <zephyr/mpipe/disp/mpipe_disp_sink.h>
#include <zephyr/mpipe/vid/mpipe_vid_src.h>
#include <zephyr/mpipe/utils/mpipe_player.h>
#include <zephyr/mpipe/nn/mpipe_nn_converter.h>
#include <zephyr/mpipe/nn/mpipe_nn_decoder.h>
#include <zephyr/mpipe/nn/mpipe_nn_runner.h>
#if DT_HAS_CHOSEN(zephyr_videotrans)
#include <zephyr/mpipe/vid/mpipe_vid_transform.h>
#endif
#if !defined(CONFIG_MPIPE_NN_BACKEND_TFLM)
#include <zephyr/mpipe/nn/mpipe_nn_backend_fake.h>
#endif
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* Element IDs (values are arbitrary; only uniqueness within the pipeline matters) */
enum {
	PIPE_ID,
	VID_SRC_ID,
	CAPS_FILTER_ID,
	TEE_ID,
	QUEUE_D_ID,
	VID_TRANS_ID,
	DISP_SINK_ID,
	QUEUE_NN_ID,
	NN_CONVERTER_ID,
	NN_RUNNER_ID,
	NN_DECODER_ID,
	APP_SINK_ID,
};

static struct mpipe pipe;
static struct mpipe_vid_src vid_src;
static struct mpipe_caps_filter caps_filter;
static struct mpipe_tee tee;
static struct mpipe_queue queue_d;
static struct mpipe_queue queue_nn;
#if DT_HAS_CHOSEN(zephyr_videotrans)
static struct mpipe_vid_transform vid_trans;
#endif
static struct mpipe_disp_sink disp_sink;
static struct mpipe_nn_converter nn_converter;
static struct mpipe_nn_runner nn_runner;
static struct mpipe_nn_decoder nn_decoder;
static struct mpipe_app_sink app_sink;
static struct mpipe_player player;

#if defined(CONFIG_MPIPE_NN_BACKEND_TFLM)

static const uint8_t model_data[] __aligned(16) = {
#include "nn_model.inc"
};

static uint8_t arena[CONFIG_SAMPLE_NN_ARENA_SIZE] __aligned(16);

/* Class order of the person detection model */
static const char *const labels[] = {"no person", "person"};

#else /* fake backend */

static const struct mpipe_nn_fake_model fake_model_cfg = {
	.in_w = 96,
	.in_h = 96,
	.channels = 1,
	.type = MPIPE_NN_TENSOR_TYPE_INT8,
	.num_classes = 2,
};

static uint8_t arena[64];

static const char *const labels[] = {"class 0", "class 1"};

#endif /* CONFIG_MPIPE_NN_BACKEND_TFLM */

/* Runs in the inference branch thread: report the classification of one frame */
static void results_cb(const struct net_buf *buf, void *user_data)
{
	const struct mpipe_nn_results *res =
		(const struct mpipe_nn_results *)(const void *)buf->data;

	ARG_UNUSED(user_data);

	if (mpipe_buffer_get_meta(buf)->bytes_used < sizeof(*res) || res->count == 0U) {
		return;
	}

	LOG_INF("Frame %u: %s (score %d%%)", res->seq,
		(res->cls[0].class_id < ARRAY_SIZE(labels)) ? labels[res->cls[0].class_id] : "?",
		(int)(res->cls[0].score * 100.0f));
}

int main(void)
{
	static const struct mpipe_app_sink_cb cb = {.fn = results_cb, .user_data = NULL};
#if defined(CONFIG_MPIPE_NN_BACKEND_TFLM)
	static const struct mpipe_nn_model model = {.data = model_data, .size = sizeof(model_data)};
#else
	static const struct mpipe_nn_model model = {.data = (const uint8_t *)&fake_model_cfg,
						    .size = sizeof(fake_model_cfg)};
#endif
	static const struct mpipe_nn_arena arena_desc = {.data = arena, .size = sizeof(arena)};
	static const uint8_t nn_queue_size = 1;
	static const enum mpipe_base_queue_leak nn_queue_leak = MPIPE_BASE_QUEUE_LEAK_OLDEST;
	static const uint8_t display_queue_size = 1;
	static const int nn_thread_priority = K_LOWEST_APPLICATION_THREAD_PRIO;
	int ret;

	ret = mpipe_pipeline_init(&pipe, PIPE_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_vid_src_init(&vid_src, VID_SRC_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_caps_filter_init(&caps_filter, CAPS_FILTER_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_tee_init(&tee, TEE_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_queue_init(&queue_d, QUEUE_D_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_queue_init(&queue_nn, QUEUE_NN_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_disp_sink_init(&disp_sink, DISP_SINK_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_nn_converter_init(&nn_converter, NN_CONVERTER_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_nn_runner_init(&nn_runner, NN_RUNNER_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_nn_decoder_init(&nn_decoder, NN_DECODER_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_app_sink_init(&app_sink, APP_SINK_ID);
	if (ret < 0) {
		goto err;
	}

	struct video_rect __maybe_unused crop = {
		CONFIG_VIDEO_SOURCE_CROP_LEFT, CONFIG_VIDEO_SOURCE_CROP_TOP,
		CONFIG_VIDEO_SOURCE_CROP_WIDTH, CONFIG_VIDEO_SOURCE_CROP_HEIGHT};

	/* clang-format off */
	ret = mpipe_object_set_properties((struct mpipe_object *)&vid_src,
		COND_CODE_0(CONFIG_PROP_NUM_BUFS, (),
			    (MPIPE_PROP_SRC_NUM_BUFS, CONFIG_PROP_NUM_BUFS,))
		COND_CODE_0(CONFIG_VIDEO_SOURCE_CROP_WIDTH, (), (MPIPE_PROP_VID_CROP, &crop,))
		IF_ENABLED(CONFIG_VIDEO_CTRL_HFLIP, (VIDEO_CID_HFLIP, CONFIG_VIDEO_CTRL_HFLIP,))
		IF_ENABLED(CONFIG_VIDEO_CTRL_VFLIP, (VIDEO_CID_VFLIP, CONFIG_VIDEO_CTRL_VFLIP,))
		MPIPE_PROP_LIST_END);
	/* clang-format on */
	if (ret < 0) {
		goto err;
	}

	/* clang-format off */
	struct mpipe_structure caps;
	struct mpipe_value pixfmt;

	ret = mpipe_structure_init_fields(&caps, MPIPE_MEDIA_VIDEO,
		COND_CODE_0(CONFIG_VIDEO_FRAME_WIDTH,
			(), (MPIPE_CAPS_IMAGE_WIDTH, MPIPE_TYPE_UINT, CONFIG_VIDEO_FRAME_WIDTH,))
		COND_CODE_0(CONFIG_VIDEO_FRAME_HEIGHT,
			(), (MPIPE_CAPS_IMAGE_HEIGHT, MPIPE_TYPE_UINT, CONFIG_VIDEO_FRAME_HEIGHT,))
		MPIPE_CAPS_END);
	/* clang-format on */
	if (ret != 0) {
		goto err;
	}

	if (strcmp(CONFIG_VIDEO_PIXEL_FORMAT, "") != 0) {
		ret = mpipe_value_set(&pixfmt, MPIPE_TYPE_UINT,
				      VIDEO_FOURCC_FROM_STR(CONFIG_VIDEO_PIXEL_FORMAT));
		if (ret < 0) {
			goto err;
		}

		ret = mpipe_structure_append_value(&caps, MPIPE_CAPS_PIXEL_FORMAT, &pixfmt);
		if (ret < 0) {
			goto err;
		}
	}

	ret = mpipe_object_set_properties((struct mpipe_object *)&caps_filter,
					  MPIPE_PROP_BASE_CAPS_FILTER_CAPS, &caps,
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	/*
	 * The inference branch queue: one slot dropping the oldest, its thread at the
	 * lowest application priority, so a slow model never starves capture or display
	 */
	struct mpipe_object *queue_nn_obj = (struct mpipe_object *)&queue_nn;

	ret = mpipe_object_set_properties(queue_nn_obj, MPIPE_PROP_BASE_QUEUE_SIZE, &nn_queue_size,
					  MPIPE_PROP_BASE_QUEUE_LEAK, &nn_queue_leak,
					  MPIPE_PROP_BASE_QUEUE_THREAD_PRIORITY,
					  &nn_thread_priority, MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	/* The display branch queue: one slot, so the camera pool stays within the CSI's four */
	ret = mpipe_object_set_properties((struct mpipe_object *)&queue_d,
					  MPIPE_PROP_BASE_QUEUE_SIZE, &display_queue_size,
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

#ifdef CONFIG_SAMPLE_NN_INPUT_SHIFT
	static const int32_t zero_point = CONFIG_SAMPLE_NN_INPUT_ZERO_POINT;

	ret = mpipe_object_set_properties((struct mpipe_object *)&nn_converter,
					  MPIPE_PROP_NN_CONVERTER_ZERO_POINT, &zero_point,
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}
#endif

	ret = mpipe_object_set_properties((struct mpipe_object *)&nn_runner, MPIPE_PROP_NN_MODEL,
					  &model, MPIPE_PROP_NN_ARENA, &arena_desc,
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	ret = mpipe_object_set_properties((struct mpipe_object *)&app_sink,
					  MPIPE_PROP_BASE_APP_SINK_CB, &cb, MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

#if DT_HAS_CHOSEN(zephyr_videotrans)
	ret = mpipe_vid_transform_init(&vid_trans, VID_TRANS_ID);
	if (ret < 0) {
		goto err;
	}

	/* clang-format off */
	ret = mpipe_object_set_properties((struct mpipe_object *)&vid_trans,
		COND_CODE_0(CONFIG_VIDEO_ROTATION_ANGLE,
			(), (VIDEO_CID_ROTATE, CONFIG_VIDEO_ROTATION_ANGLE,)) MPIPE_PROP_LIST_END);
	/* clang-format on */
	if (ret < 0) {
		goto err;
	}
#endif

	/* clang-format off */
	/* Add elements to the pipeline - order does not matter */
	ret = mpipe_bin_add((struct mpipe_bin *)&pipe,
			(struct mpipe_element *)&vid_src,
			(struct mpipe_element *)&caps_filter,
			(struct mpipe_element *)&tee,
			(struct mpipe_element *)&queue_d,
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_videotrans),
				   ((struct mpipe_element *)&vid_trans,))
			(struct mpipe_element *)&disp_sink,
			(struct mpipe_element *)&queue_nn,
			(struct mpipe_element *)&nn_converter,
			(struct mpipe_element *)&nn_runner,
			(struct mpipe_element *)&nn_decoder,
			(struct mpipe_element *)&app_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto err;
	}

	/* Branch 1: camera to display */
	ret = mpipe_element_link((struct mpipe_element *)&vid_src,
			(struct mpipe_element *)&caps_filter,
			(struct mpipe_element *)&tee,
			(struct mpipe_element *)&queue_d,
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_videotrans),
				   ((struct mpipe_element *)&vid_trans,))
			(struct mpipe_element *)&disp_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link branch 1 (%d)", ret);
		goto err;
	}

	/* Branch 2: tee (2nd src pad) to the classifier */
	ret = mpipe_element_link((struct mpipe_element *)&tee,
			(struct mpipe_element *)&queue_nn,
			(struct mpipe_element *)&nn_converter,
			(struct mpipe_element *)&nn_runner,
			(struct mpipe_element *)&nn_decoder,
			(struct mpipe_element *)&app_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link branch 2 (%d)", ret);
		goto err;
	}
	/* clang-format on */

	LOG_INF("Pipeline linked.");

	ret = mpipe_player_init(&player, &pipe);
	if (ret != 0) {
		LOG_ERR("Failed to init player (%d)", ret);
		goto err;
	}

	(void)mpipe_player_play(&player);
	(void)mpipe_player_wait_quit(&player);
	(void)mpipe_player_deinit(&player);

	LOG_INF("Done.");

	return 0;

err:
	LOG_ERR("Aborting sample");
	return 0;
}
