/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <src/core/mp_pixel_format.h>

#include "mp_zvid.h"
#include "mp_zvid_object.h"

LOG_MODULE_REGISTER(mp_zvid_object, CONFIG_LIBMP_LOG_LEVEL);

int mp_zvid_object_set_property(struct mp_zvid_object *zvid_obj, uint32_t key, const void *val)
{
	switch (key) {
	default:
		if (IN_RANGE(key, VIDEO_CID_BASE, VIDEO_CID_LASTP1) ||
		    IN_RANGE(key, VIDEO_CID_CODEC_CLASS_BASE, VIDEO_CID_JPEG_COMPRESSION_QUALITY) ||
		    key > VIDEO_CID_PRIVATE_BASE) {
			struct video_control ctrl = {.id = key, .val = (int32_t)(uintptr_t)val};

			return video_set_ctrl(zvid_obj->vdev, &ctrl);
		}
	}

	return -ENOTSUP;
}

int mp_zvid_object_get_property(struct mp_zvid_object *zvid_obj, uint32_t key, void *val)
{
	int ret;

	switch (key) {
	default:
		if (IN_RANGE(key, VIDEO_CID_BASE, VIDEO_CID_LASTP1) ||
		    IN_RANGE(key, VIDEO_CID_CODEC_CLASS_BASE, VIDEO_CID_JPEG_COMPRESSION_QUALITY) ||
		    key > VIDEO_CID_PRIVATE_BASE) {
			struct video_control ctrl = {.id = key};

			ret = video_get_ctrl(zvid_obj->vdev, &ctrl);
			if (ret < 0) {
				return ret;
			}

			*(int32_t *)val = ctrl.val;

			return 0;
		}
	}

	return -ENOTSUP;
}

static void append_frmrates_to_structure(const struct device *vdev, struct video_format *fmt,
					 struct mp_structure *caps_item)
{
	struct mp_value *frmrates = mp_value_new(MP_TYPE_LIST, NULL);
	struct mp_value *frmrate = NULL;
	struct video_frmival_enum fie = {0};

	fie.format = fmt;
	while (video_enum_frmival(vdev, &fie) == 0) {
		switch (fie.type) {
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			frmrate = mp_value_new(MP_TYPE_UINT_FRACTION, fie.discrete.denominator,
					       fie.discrete.numerator);

			mp_value_list_append(frmrates, frmrate);
			break;
		case VIDEO_FRMIVAL_TYPE_STEPWISE:
			frmrate = mp_value_new(
				MP_TYPE_UINT_FRACTION_RANGE, fie.stepwise.max.denominator,
				fie.stepwise.max.numerator, fie.stepwise.min.denominator,
				fie.stepwise.min.numerator, fie.stepwise.step.denominator,
				fie.stepwise.step.numerator);

			mp_structure_append(caps_item, "framerate", frmrate);

			break;
		default:
			break;
		}
		fie.index++;
	}

	if (!mp_value_list_is_empty(frmrates)) {
		mp_structure_append(caps_item, "framerate", frmrates);
	}
}

struct mp_caps *mp_zvid_object_get_caps(struct mp_zvid_object *zvid_obj)
{
	struct mp_caps *caps = mp_caps_new(NULL);
	struct mp_structure *caps_item = NULL;
	struct video_caps vcaps = {.type = zvid_obj->type};
	struct video_format fmt = {.type = zvid_obj->type};
	enum mp_pixel_format mp_fmt;

	if (video_get_caps(zvid_obj->vdev, &vcaps)) {
		LOG_WRN("Unable to retrieve device's capabilities");
		return NULL;
	}

	/* Set buffer pool's min_buffers and alignment */
	MP_BUFFER_POOL(&zvid_obj->pool)->config.min_buffers = vcaps.min_vbuf_count;
	MP_BUFFER_POOL(&zvid_obj->pool)->config.align = vcaps.buf_align;

	for (uint8_t i = 0; vcaps.format_caps[i].pixelformat != 0; i++) {
		mp_fmt = zvid2mp_pixfmt(vcaps.format_caps[i].pixelformat);
		if (mp_fmt == MP_PIXEL_FORMAT_UNKNOWN) {
			continue;
		}

		/*
		 * TODO: Only supports video/x-raw for now. Should detect other media types,
		 * e.g. video/x-bayer, video/x-h264, image/jpeg, etc.
		 */
		fmt.pixelformat = vcaps.format_caps[i].pixelformat;

		/* TODO: Add support for MP_TYPE_UINT_RANGE and use it here */
		caps_item = mp_structure_new(
			"video/x-raw", "format", MP_TYPE_UINT, mp_fmt, "width", MP_TYPE_UINT_RANGE,
			vcaps.format_caps[i].width_min, vcaps.format_caps[i].width_max,
			vcaps.format_caps[i].width_step, "height", MP_TYPE_UINT_RANGE,
			vcaps.format_caps[i].height_min, vcaps.format_caps[i].height_max,
			vcaps.format_caps[i].height_step, NULL);

		/*
		 * Only query the frame interval for the min frame size
		 */
		fmt.width = vcaps.format_caps[i].width_min;
		fmt.height = vcaps.format_caps[i].height_min;

		append_frmrates_to_structure(zvid_obj->vdev, &fmt, caps_item);
		mp_caps_append(caps, caps_item);
	}

	return caps;
}

bool mp_zvid_object_set_caps(struct mp_zvid_object *zvid_obj, struct mp_caps *caps)
{
	struct video_format_cap fcaps = {0};
	struct video_format fmt;
	struct video_frmival frmival;
	struct mp_structure *first_structure = mp_caps_get_structure(caps, 0);
	struct mp_value *frmrate = mp_structure_get_value(first_structure, "framerate");

	if (!mp_caps_is_fixed(caps)) {
		return false;
	}

	/* Set format */
	get_zvid_fmt_from_structure(first_structure, &fcaps);

	fmt.type = zvid_obj->type;
	fmt.pixelformat = fcaps.pixelformat;
	fmt.width = fcaps.width_min;
	fmt.height = fcaps.height_min;
	if (video_set_format(zvid_obj->vdev, &fmt)) {
		LOG_ERR("Unable to set format");
		return false;
	}

	/* Set buffer pool size */
	MP_BUFFER_POOL(&zvid_obj->pool)->config.size = fmt.size;

	/* Set frame rate only if the element's caps support it */
	struct mp_caps *objcaps = mp_zvid_object_get_caps(zvid_obj);

	first_structure = mp_caps_get_structure(objcaps, 0);
	if (frmrate != NULL && mp_structure_get_value(first_structure, "framerate") != NULL) {
		mp_caps_unref(objcaps);
		frmival.numerator = mp_value_get_fraction_denominator(frmrate);
		frmival.denominator = mp_value_get_fraction_numerator(frmrate);
		if (video_set_frmival(zvid_obj->vdev, &frmival)) {
			LOG_ERR("Unable to set frame interval");
			return false;
		}
	}

	return true;
}

bool mp_zvid_object_decide_allocation(struct mp_zvid_object *zvid_obj, struct mp_query *query)
{
	struct mp_buffer_pool *query_pool = mp_query_get_pool(query);
	struct mp_buffer_pool_config *pool_config = &MP_BUFFER_POOL(&zvid_obj->pool)->config;
	struct mp_buffer_pool_config *qpc = NULL;

	if (query_pool == NULL) {
		qpc = mp_query_get_pool_config(query);
	} else {
		qpc = &query_pool->config;
	}

	/* Always use its own pool, just negotiatie the configs */
	if (qpc != NULL) {
		/* Decide min buffers */
		if (qpc->min_buffers > pool_config->min_buffers) {
			pool_config->min_buffers = qpc->min_buffers;
		}

		/* Decide alignment */
		int align = lcm(qpc->align, pool_config->align);

		if (align == -1) {
			return false;
		} else if (align == 0 && qpc->align != 0) {
			pool_config->align = qpc->align;
		} else if (align != 0) {
			pool_config->align = align;
		} else {
			return true;
		}
	}

	return true;
}
