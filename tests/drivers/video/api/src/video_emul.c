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

	zexpect_ok(video_stream_start(imager_dev));
	zexpect_ok(video_stream_stop(imager_dev));

	zexpect_ok(video_stream_start(rx_dev));
	zexpect_ok(video_stream_stop(rx_dev));
}

ZTEST(video_common, test_video_format)
{
	struct video_caps caps = {0};
	struct video_format fmt = {0};

	zexpect_ok(video_get_caps(imager_dev, VIDEO_EP_OUT, &caps));

	/* Test all the formats listed in the caps, the min and max values */
	for (size_t i = 0; caps.format_caps[i].pixelformat != 0; i++) {
		fmt.pixelformat = caps.format_caps[i].pixelformat;

		fmt.height = caps.format_caps[i].height_min;
		fmt.width = caps.format_caps[i].width_min;
		zexpect_ok(video_set_format(imager_dev, VIDEO_EP_OUT, &fmt));
		zexpect_ok(video_get_format(imager_dev, VIDEO_EP_OUT, &fmt));
		zexpect_equal(fmt.pixelformat, caps.format_caps[i].pixelformat);
		zexpect_equal(fmt.width, caps.format_caps[i].width_min);
		zexpect_equal(fmt.height, caps.format_caps[i].height_min);

		fmt.height = caps.format_caps[i].height_max;
		fmt.width = caps.format_caps[i].width_min;
		zexpect_ok(video_set_format(imager_dev, VIDEO_EP_OUT, &fmt));
		zexpect_ok(video_get_format(imager_dev, VIDEO_EP_OUT, &fmt));
		zexpect_equal(fmt.pixelformat, caps.format_caps[i].pixelformat);
		zexpect_equal(fmt.width, caps.format_caps[i].width_max);
		zexpect_equal(fmt.height, caps.format_caps[i].height_min);

		fmt.height = caps.format_caps[i].height_min;
		fmt.width = caps.format_caps[i].width_max;
		zexpect_ok(video_set_format(imager_dev, VIDEO_EP_OUT, &fmt));
		zexpect_ok(video_get_format(imager_dev, VIDEO_EP_OUT, &fmt));
		zexpect_equal(fmt.pixelformat, caps.format_caps[i].pixelformat);
		zexpect_equal(fmt.width, caps.format_caps[i].width_min);
		zexpect_equal(fmt.height, caps.format_caps[i].height_max);

		fmt.height = caps.format_caps[i].height_max;
		fmt.width = caps.format_caps[i].width_max;
		zexpect_ok(video_set_format(imager_dev, VIDEO_EP_OUT, &fmt));
		zexpect_ok(video_get_format(imager_dev, VIDEO_EP_OUT, &fmt));
		zexpect_equal(fmt.pixelformat, caps.format_caps[i].pixelformat);
		zexpect_equal(fmt.width, caps.format_caps[i].width_max);
		zexpect_equal(fmt.height, caps.format_caps[i].height_max);
	}

	fmt.pixelformat = 0x00000000;
	zexpect_not_ok(video_set_format(imager_dev, VIDEO_EP_OUT, &fmt));
	zexpect_ok(video_get_format(imager_dev, VIDEO_EP_OUT, &fmt));
	zexpect_not_equal(fmt.pixelformat, 0x00000000, "should not store wrong formats");
}

ZTEST(video_common, test_video_frmival)
{
	struct video_format fmt;
	struct video_frmival_enum fie = {.format = &fmt};

	/* Pick the current format for testing the frame interval enumeration */
	zexpect_ok(video_get_format(imager_dev, VIDEO_EP_OUT, &fmt));

	/* Do a first enumeration of frame intervals, expected to work */
	zexpect_ok(video_enum_frmival(imager_dev, VIDEO_EP_OUT, &fie));
	zexpect_equal(fie.index, 1, "fie's index should increment by one at every iteration");

	/* Test that every value of the frame interval enumerator can be applied */
	do {
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
				zexpect_ok(video_set_frmival(imager_dev, VIDEO_EP_OUT, &q));
				zexpect_ok(video_get_frmival(imager_dev, VIDEO_EP_OUT, &a));
				zexpect_equal(video_frmival_nsec(&q), video_frmival_nsec(&a));
			}
			break;
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			/* There is just one frame interval to test */
			zexpect_ok(video_set_frmival(imager_dev, VIDEO_EP_OUT, &fie.discrete));
			zexpect_ok(video_get_frmival(imager_dev, VIDEO_EP_OUT, &a));

			zexpect_equal(video_frmival_nsec(&fie.discrete), video_frmival_nsec(&a));
			break;
		}
	} while (video_enum_frmival(imager_dev, VIDEO_EP_OUT, &fie) == 0);
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

	/* Get a list of supported format */
	zexpect_ok(video_get_caps(rx_dev, VIDEO_EP_OUT, &caps));

	/* Pick set first format, just to use something supported */
	fmt.pixelformat = caps.format_caps[0].pixelformat;
	fmt.width = caps.format_caps[0].width_max;
	fmt.height = caps.format_caps[0].height_max;
	fmt.pitch = fmt.width * 2;
	zexpect_ok(video_set_format(rx_dev, VIDEO_EP_OUT, &fmt));

	/* Allocate a buffer, assuming prj.conf gives enough memory for it */
	vbuf = video_buffer_alloc(fmt.pitch * fmt.height, K_NO_WAIT);
	zexpect_not_null(vbuf);

	/* Start the virtual hardware */
	zexpect_ok(video_stream_start(rx_dev));

	/* Enqueue a first buffer */
	zexpect_ok(video_enqueue(rx_dev, VIDEO_EP_OUT, vbuf));

	/* Receive the completed buffer */
	zexpect_ok(video_dequeue(rx_dev, VIDEO_EP_OUT, &vbuf, K_FOREVER));
	zexpect_not_null(vbuf);
	zexpect_equal(vbuf->bytesused, vbuf->size);

	/* Enqueue back the same buffer */
	zexpect_ok(video_enqueue(rx_dev, VIDEO_EP_OUT, vbuf));

	/* Process the remaining buffers */
	zexpect_ok(video_flush(rx_dev, VIDEO_EP_OUT, false));

	/* Expect the buffer to immediately be available */
	zexpect_ok(video_dequeue(rx_dev, VIDEO_EP_OUT, &vbuf, K_FOREVER));
	zexpect_not_null(vbuf);
	zexpect_equal(vbuf->bytesused, vbuf->size);

	/* Nothing left in the queue, possible to stop */
	zexpect_ok(video_stream_stop(rx_dev));

	/* Nothing tested, but this should not crash */
	video_buffer_release(vbuf);
}

ZTEST_SUITE(video_emul, NULL, NULL, NULL, NULL, NULL);
