/*
 * Copyright (c) 2024 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>

const struct device *rx_dev = DEVICE_DT_GET(DT_NODELABEL(test_video_emul_rx));
const struct device *imager_dev = DEVICE_DT_GET(DT_NODELABEL(test_video_emul_imager));

ZTEST(video_common, test_video_device)
{
	zexpect_true(device_is_ready(rx_dev));
	zexpect_true(device_is_ready(imager_dev));

	zexpect_ok(video_stream_start(imager_dev, VIDEO_BUF_TYPE_OUTPUT));
	zexpect_ok(video_stream_stop(imager_dev, VIDEO_BUF_TYPE_OUTPUT));

	zexpect_ok(video_stream_start(rx_dev, VIDEO_BUF_TYPE_OUTPUT));
	zexpect_ok(video_stream_stop(rx_dev, VIDEO_BUF_TYPE_OUTPUT));
}

ZTEST(video_common, test_video_format)
{
	struct video_caps caps = {0};
	struct video_format fmt = {0};

	zexpect_ok(video_get_caps(imager_dev, &caps));

	/* Test all the formats listed in the caps, the min and max values */
	for (size_t i = 0; caps.format_caps[i].pixelformat != 0; i++) {
		fmt.pixelformat = caps.format_caps[i].pixelformat;

		fmt.height = caps.format_caps[i].height_min;
		fmt.width = caps.format_caps[i].width_min;
		zexpect_ok(video_set_format(imager_dev, &fmt));
		zexpect_ok(video_get_format(imager_dev, &fmt));
		zexpect_equal(fmt.pixelformat, caps.format_caps[i].pixelformat);
		zexpect_equal(fmt.width, caps.format_caps[i].width_min);
		zexpect_equal(fmt.height, caps.format_caps[i].height_min);

		fmt.height = caps.format_caps[i].height_max;
		fmt.width = caps.format_caps[i].width_min;
		zexpect_ok(video_set_format(imager_dev, &fmt));
		zexpect_ok(video_get_format(imager_dev, &fmt));
		zexpect_equal(fmt.pixelformat, caps.format_caps[i].pixelformat);
		zexpect_equal(fmt.width, caps.format_caps[i].width_max);
		zexpect_equal(fmt.height, caps.format_caps[i].height_min);

		fmt.height = caps.format_caps[i].height_min;
		fmt.width = caps.format_caps[i].width_max;
		zexpect_ok(video_set_format(imager_dev, &fmt));
		zexpect_ok(video_get_format(imager_dev, &fmt));
		zexpect_equal(fmt.pixelformat, caps.format_caps[i].pixelformat);
		zexpect_equal(fmt.width, caps.format_caps[i].width_min);
		zexpect_equal(fmt.height, caps.format_caps[i].height_max);

		fmt.height = caps.format_caps[i].height_max;
		fmt.width = caps.format_caps[i].width_max;
		zexpect_ok(video_set_format(imager_dev, &fmt));
		zexpect_ok(video_get_format(imager_dev, &fmt));
		zexpect_equal(fmt.pixelformat, caps.format_caps[i].pixelformat);
		zexpect_equal(fmt.width, caps.format_caps[i].width_max);
		zexpect_equal(fmt.height, caps.format_caps[i].height_max);
	}

	fmt.pixelformat = 0x00000000;
	zexpect_not_ok(video_set_format(imager_dev, &fmt));
	zexpect_ok(video_get_format(imager_dev, &fmt));
	zexpect_not_equal(fmt.pixelformat, 0x00000000, "should not store wrong formats");
}

ZTEST(video_common, test_video_frmival)
{
	struct video_format fmt;
	struct video_frmival_enum fie = {.format = &fmt};

	/* Pick the current format for testing the frame interval enumeration */
	zexpect_ok(video_get_format(imager_dev, &fmt));

	/* Do a first enumeration of frame intervals, expected to work */
	zexpect_ok(video_enum_frmival(imager_dev, &fie));
	zexpect_equal(fie.index, 0, "fie's index should not increment on its own");

	/* Test that every value of the frame interval enumerator can be applied */
	for (fie.index = 0; video_enum_frmival(imager_dev, &fie) == 0; fie.index++) {
		struct video_frmival q, a;
		uint32_t min, max, step;

		zexpect_equal_ptr(fie.format, &fmt, "the format should not be changed");
		zexpect_true(fie.type == VIDEO_FRMIVAL_TYPE_STEPWISE ||
			     fie.type == VIDEO_FRMIVAL_TYPE_DISCRETE);

		switch (fie.type) {
		case VIDEO_FRMIVAL_TYPE_STEPWISE:
			/* Get everthing under the same denominator */
			q.denominator = fie.stepwise.min.denominator *
					fie.stepwise.max.denominator *
					fie.stepwise.step.denominator;
			min = fie.stepwise.max.denominator * fie.stepwise.step.denominator *
			      fie.stepwise.min.numerator;
			max = fie.stepwise.min.denominator * fie.stepwise.step.denominator *
			      fie.stepwise.max.numerator;
			step = fie.stepwise.min.denominator * fie.stepwise.max.denominator *
			       fie.stepwise.step.numerator;

			/* Test every supported frame interval */
			for (q.numerator = min; q.numerator <= max; q.numerator += step) {
				zexpect_ok(video_set_frmival(imager_dev, &q));
				zexpect_ok(video_get_frmival(imager_dev, &a));
				zexpect_equal(video_frmival_nsec(&q), video_frmival_nsec(&a),
					      "query %u/%u (%u nsec) answer %u/%u (%u nsec, sw)",
					      q.numerator, q.denominator, video_frmival_nsec(&q),
					      a.numerator, a.denominator, video_frmival_nsec(&a));
			}
			break;
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			/* There is just one frame interval to test */
			memcpy(&q, &fie.discrete, sizeof(q));
			zexpect_ok(video_set_frmival(imager_dev, &q));
			zexpect_ok(video_get_frmival(imager_dev, &a));

			zexpect_equal(video_frmival_nsec(&fie.discrete), video_frmival_nsec(&a),
				      "query %u/%u (%u nsec) answer %u/%u (%u nsec, discrete)",
				      q.numerator, q.denominator, video_frmival_nsec(&q),
				      a.numerator, a.denominator, video_frmival_nsec(&a));
			break;
		}
	}
}

ZTEST(video_common, test_video_ctrl)
{
	struct video_control ctrl = {.id = VIDEO_CID_PRIVATE_BASE + 0x01, .val = 30};

	/* Emulated vendor specific control, expected to be supported by all imagers */
	zexpect_ok(video_set_ctrl(imager_dev, &ctrl));
	ctrl.val = 0;
	zexpect_ok(video_get_ctrl(imager_dev, &ctrl));
	zexpect_equal(ctrl.val, 30);
}

ZTEST(video_common, test_video_vbuf)
{
	struct video_caps caps;
	struct video_format fmt;
	struct video_buffer *vbuf = NULL;
	enum video_buf_type type = VIDEO_BUF_TYPE_OUTPUT;

	/* Get a list of supported format */
	caps.type = type;
	zexpect_ok(video_get_caps(rx_dev, &caps));

	/* Pick set first format, just to use something supported */
	fmt.pixelformat = caps.format_caps[0].pixelformat;
	fmt.width = caps.format_caps[0].width_max;
	fmt.height = caps.format_caps[0].height_max;
	fmt.type = type;
	zexpect_ok(video_set_format(rx_dev, &fmt));

	/* Allocate a buffer, assuming prj.conf gives enough memory for it */
	vbuf = video_buffer_alloc(fmt.pitch * fmt.height, K_NO_WAIT);
	zexpect_not_null(vbuf);

	/* Start the virtual hardware */
	zexpect_ok(video_stream_start(rx_dev, type));

	vbuf->type = type;

	/* Enqueue a first buffer */
	zexpect_ok(video_enqueue(rx_dev, vbuf));

	/* Receive the completed buffer */
	zexpect_ok(video_dequeue(rx_dev, &vbuf, K_FOREVER));
	zexpect_not_null(vbuf);
	zexpect_equal(vbuf->bytesused, vbuf->size);

	/* Enqueue back the same buffer */
	zexpect_ok(video_enqueue(rx_dev, vbuf));

	/* Process the remaining buffers */
	zexpect_ok(video_flush(rx_dev, false));

	/* Expect the buffer to immediately be available */
	zexpect_ok(video_dequeue(rx_dev, &vbuf, K_FOREVER));
	zexpect_not_null(vbuf);
	zexpect_equal(vbuf->bytesused, vbuf->size);

	/* Nothing left in the queue, possible to stop */
	zexpect_ok(video_stream_stop(rx_dev, type));

	/* Nothing tested, but this should not crash */
	video_buffer_release(vbuf);
}

ZTEST_SUITE(video_emul, NULL, NULL, NULL, NULL, NULL);
