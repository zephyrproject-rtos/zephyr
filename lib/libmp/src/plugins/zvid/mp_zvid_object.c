/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <src/core/mp_caps.h>

#include "mp_zvid_object.h"
#include "mp_zvid_property.h"

LOG_MODULE_REGISTER(mp_zvid_object, CONFIG_LIBMP_LOG_LEVEL);

static int set_dimension_fields(struct mp_structure *structure, uint8_t key, uint32_t *min,
				uint32_t *max, uint16_t *step)
{
	const struct mp_value *value = mp_structure_get_value(structure, key);

	if (value == NULL) {
		return -EINVAL;
	}

	if (value->type == MP_TYPE_UINT_RANGE) {
		*min = mp_value_get_uint_range_min(value);
		*max = mp_value_get_uint_range_max(value);
		*step = (uint16_t)mp_value_get_uint_range_step(value);
	} else if (value->type == MP_TYPE_UINT) {
		*min = mp_value_get_uint(value);
		*max = *min;
		*step = 0;
	} else {
		return -EINVAL;
	}

	return 0;
}

int mp_structure_to_vfc(struct mp_structure *structure, struct video_format_cap *vfc)
{
	int ret;
	struct mp_value *value;

	/* Get pixel format field */
	value = mp_structure_get_value(structure, MP_CAPS_PIXEL_FORMAT);
	if (value == NULL) {
		return -EINVAL;
	}
	if (value->type == MP_TYPE_UINT) {
		vfc->pixelformat = mp_value_get_uint(value);
	} else if (value->type == MP_TYPE_LIST) {
		/* Format may be of MP_TYPE_LIST due to the intersection with a list type but it is
		 * actually a single-value list, so take the 1st item in the list
		 */
		vfc->pixelformat = mp_value_get_uint(mp_value_list_get(value, 0));
	} else {
		return -EINVAL;
	}

	/* Get width fields */
	ret = set_dimension_fields(structure, MP_CAPS_IMAGE_WIDTH, &vfc->width_min, &vfc->width_max,
				   &vfc->width_step);
	if (ret < 0) {
		return ret;
	}

	/* Get height fields */
	return set_dimension_fields(structure, MP_CAPS_IMAGE_HEIGHT, &vfc->height_min,
				    &vfc->height_max, &vfc->height_step);
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

			mp_structure_append(caps_item, MP_CAPS_FRAME_RATE, frmrate);

			break;
		default:
			break;
		}
		fie.index++;
	}

	if (!mp_value_list_is_empty(frmrates)) {
		mp_structure_append(caps_item, MP_CAPS_FRAME_RATE, frmrates);
	}
}

struct mp_caps *mp_zvid_object_get_caps(struct mp_zvid_object *zvid_obj)
{
	int ret;
	struct mp_caps *caps = mp_caps_new(MP_MEDIA_END);
	struct mp_structure *caps_item = NULL;
	struct video_caps vcaps = {.type = zvid_obj->type};
	struct video_format fmt = {.type = zvid_obj->type};
	struct video_rect rect;
	uint32_t crop_w = UINT32_MAX;
	uint32_t crop_h = UINT32_MAX;
	uint32_t comp_min_w = UINT32_MAX;
	uint32_t comp_min_h = UINT32_MAX;
	uint32_t comp_max_w = 0;
	uint32_t comp_max_h = 0;
	struct video_selection sel = {
		.type = zvid_obj->type,
		.target = VIDEO_SEL_TGT_CROP,
	};

	/* Get caps */
	if (video_get_caps(zvid_obj->vdev, &vcaps)) {
		LOG_WRN("Unable to retrieve device's capabilities");
		return NULL;
	}

	/* Get crop selection */
	ret = video_get_selection(zvid_obj->vdev, &sel);
	if (ret == 0) {
		crop_w = sel.rect.width;
		crop_h = sel.rect.height;
	}

	/* Get compose selection upper-bound */
	sel.target = VIDEO_SEL_TGT_COMPOSE_BOUND;
	ret = video_get_selection(zvid_obj->vdev, &sel);
	if (ret == 0) {
		comp_max_w = sel.rect.width + sel.rect.left;
		comp_max_h = sel.rect.height + sel.rect.top;
	}

	/* Memorize the current compose selection */
	sel.target = VIDEO_SEL_TGT_COMPOSE;
	ret = video_get_selection(zvid_obj->vdev, &sel);
	if (ret == 0) {
		rect = sel.rect;
	}

	/* Probe the compose selection lower-bound */
	sel.target = VIDEO_SEL_TGT_COMPOSE;
	sel.rect = (struct video_rect){.top = 0, .left = 0, .width = 1, .height = 1};
	ret = video_set_selection(zvid_obj->vdev, &sel);
	if (ret == 0) {
		comp_min_w = sel.rect.width + sel.rect.left;
		comp_min_h = sel.rect.height + sel.rect.top;
	}

	/* Set back the original compose selection */
	sel.rect = rect;
	video_set_selection(zvid_obj->vdev, &sel);

	/* Set buffer pool's min_buffers and alignment */
	MP_BUFFER_POOL(&zvid_obj->pool)->config.min_buffers = vcaps.min_vbuf_count;
	MP_BUFFER_POOL(&zvid_obj->pool)->config.align = vcaps.buf_align;

	for (uint8_t i = 0; vcaps.format_caps[i].pixelformat != 0; i++) {
		caps_item = mp_structure_new(
			MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_UINT,
			vcaps.format_caps[i].pixelformat, MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT_RANGE,
			min3(vcaps.format_caps[i].width_min, crop_w, comp_min_w),
			max(vcaps.format_caps[i].width_max, comp_max_w),
			vcaps.format_caps[i].width_step, MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT_RANGE,
			min3(vcaps.format_caps[i].height_min, crop_h, comp_min_h),
			max(vcaps.format_caps[i].height_max, comp_max_h),
			vcaps.format_caps[i].height_step, MP_CAPS_END);

		/* Get frame rate */
		fmt.pixelformat = vcaps.format_caps[i].pixelformat;
		fmt.width = vcaps.format_caps[i].width_min;
		fmt.height = vcaps.format_caps[i].height_min;
		append_frmrates_to_structure(zvid_obj->vdev, &fmt, caps_item);

		mp_caps_append(caps, caps_item);
	}

	return caps;
}

bool mp_zvid_object_set_caps(struct mp_zvid_object *zvid_obj, struct mp_caps *caps)
{
	struct video_format_cap vfc = {0};
	struct video_format fmt;
	struct video_frmival frmival;
	struct mp_structure *first_structure = mp_caps_get_structure(caps, 0);
	struct mp_value *frmrate = mp_structure_get_value(first_structure, MP_CAPS_FRAME_RATE);

	if (!mp_caps_is_fixed(caps)) {
		return false;
	}

	/* Set format */
	if (mp_structure_to_vfc(first_structure, &vfc) < 0) {
		return false;
	}

	fmt.type = zvid_obj->type;
	fmt.pixelformat = vfc.pixelformat;
	fmt.width = vfc.width_min;
	fmt.height = vfc.height_min;
	if (video_set_compose_format(zvid_obj->vdev, &fmt)) {
		LOG_ERR("Unable to set format");
		return false;
	}

	/* Set buffer pool size */
	MP_BUFFER_POOL(&zvid_obj->pool)->config.size = fmt.size;

	/* Set frame rate only if the element's caps support it */
	struct mp_caps *objcaps = mp_zvid_object_get_caps(zvid_obj);

	first_structure = mp_caps_get_structure(objcaps, 0);
	if (frmrate != NULL &&
	    mp_structure_get_value(first_structure, MP_CAPS_FRAME_RATE) != NULL) {
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

int mp_zvid_object_set_property(struct mp_zvid_object *zvid_obj, uint32_t key, const void *val,
				struct mp_caps **pad_caps)
{
	switch (key) {
	case PROP_ZVID_DEVICE:
	case PROP_ZVID_CROP:
		if (key == PROP_ZVID_DEVICE) {
			zvid_obj->vdev = val;
		} else {
			zvid_obj->crop = *(struct video_rect *)val;

			/* Set crop selection target to HW */
			struct video_selection sel = {
				.type = zvid_obj->type,
				.target = VIDEO_SEL_TGT_CROP,
				.rect = zvid_obj->crop,
			};

			video_set_selection(zvid_obj->vdev, &sel);
		}

		/* Get caps from driver and update the pad's caps */
		if (pad_caps != NULL) {
			struct mp_caps *new_caps = mp_zvid_object_get_caps(zvid_obj);

			mp_caps_replace(pad_caps, new_caps);
			mp_caps_unref(new_caps);
		}

		return 0;
	default:
		if (IN_RANGE(key, VIDEO_CID_BASE, VIDEO_CID_LASTP1) ||
		    IN_RANGE(key, VIDEO_CID_CODEC_CLASS_BASE, VIDEO_CID_JPEG_COMPRESSION_QUALITY) ||
		    key > VIDEO_CID_PRIVATE_BASE) {
			struct video_control ctrl = {.id = key, .val = (int32_t)(uintptr_t)val};

			return video_set_ctrl(zvid_obj->vdev, &ctrl);
		}

		return -ENOTSUP;
	}
}

int mp_zvid_object_get_property(struct mp_zvid_object *zvid_obj, uint32_t key, void *val)
{
	int ret;

	switch (key) {
	case PROP_ZVID_DEVICE:
		*(const struct device **)val = zvid_obj->vdev;
		return 0;
	case PROP_ZVID_CROP:
		*(struct video_rect *)val = zvid_obj->crop;
		return 0;
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

		return -ENOTSUP;
	}
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
		int align = sys_lcm(qpc->align, pool_config->align);

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
