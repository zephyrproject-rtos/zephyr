/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/mpipe/erpc/mpipe_erpc.h>
#include <zephyr/mpipe/vid/mpipe_vid_transform_client.h>
#include <zephyr/mpipe/vid/mpipe_vid_transform_client_erpc.h>

#include "c_mpipe_vid_transform_client.h"
#include "mpipe_vid_transform_common.h"

LOG_MODULE_REGISTER(mpipe_vid_transform_client_erpc, CONFIG_MPIPE_LOG_LEVEL);

/* The direction crosses the wire as its numeric value */
BUILD_ASSERT((int)MPIPE_RPC_PAD_UNKNOWN == (int)MPIPE_PAD_UNKNOWN);
BUILD_ASSERT((int)MPIPE_RPC_PAD_SRC == (int)MPIPE_PAD_SRC);
BUILD_ASSERT((int)MPIPE_RPC_PAD_SINK == (int)MPIPE_PAD_SINK);

static void mpipe_vid_transform_client_erpc_vfc_to_rpc(const struct video_format_cap *vfc,
						       mpipe_rpc_video_format_cap *rpc)
{
	rpc->pixelformat = vfc->pixelformat;
	rpc->width_min = vfc->width_min;
	rpc->width_max = vfc->width_max;
	rpc->height_min = vfc->height_min;
	rpc->height_max = vfc->height_max;
	rpc->width_step = vfc->width_step;
	rpc->height_step = vfc->height_step;
}

static void mpipe_vid_transform_client_erpc_rpc_to_vfc(const mpipe_rpc_video_format_cap *rpc,
						       struct video_format_cap *vfc)
{
	vfc->pixelformat = rpc->pixelformat;
	vfc->width_min = rpc->width_min;
	vfc->width_max = rpc->width_max;
	vfc->height_min = rpc->height_min;
	vfc->height_max = rpc->height_max;
	vfc->width_step = rpc->width_step;
	vfc->height_step = rpc->height_step;
}

static int32_t mpipe_vid_transform_client_erpc_get_buf_caps(enum mpipe_pad_direction direction,
							    uint8_t *min_buffers,
							    uint16_t *buf_align)
{
	return get_buf_caps_rpc((mpipe_rpc_pad_direction)direction, min_buffers, buf_align);
}

static int32_t mpipe_vid_transform_client_erpc_get_format_caps(enum mpipe_pad_direction direction,
							       uint8_t ind,
							       struct video_format_cap *vfc)
{
	mpipe_rpc_video_format_cap rpc;
	int32_t ret;

	ret = get_format_caps_rpc((mpipe_rpc_pad_direction)direction, ind, &rpc);
	if (ret != 0) {
		return ret;
	}

	mpipe_vid_transform_client_erpc_rpc_to_vfc(&rpc, vfc);

	return 0;
}

static int32_t mpipe_vid_transform_client_erpc_set_format(struct video_format *fmt)
{
	mpipe_rpc_video_format rpc = {
		.type = (uint8_t)fmt->type,
		.pixelformat = fmt->pixelformat,
		.width = fmt->width,
		.height = fmt->height,
		.pitch = fmt->pitch,
		.size = fmt->size,
	};
	int32_t ret;

	ret = set_format_rpc(&rpc);
	if (ret != 0) {
		return ret;
	}

	/* The server fills in what the format implies, the pitch and size above all */
	fmt->pixelformat = rpc.pixelformat;
	fmt->width = rpc.width;
	fmt->height = rpc.height;
	fmt->pitch = rpc.pitch;
	fmt->size = rpc.size;

	return 0;
}

static int32_t mpipe_vid_transform_client_erpc_transform_cap(enum mpipe_pad_direction direction,
							     uint16_t ind,
							     const struct video_format_cap *vfc,
							     struct video_format_cap *other_vfc)
{
	mpipe_rpc_video_format_cap rpc, other_rpc;
	int32_t ret;

	mpipe_vid_transform_client_erpc_vfc_to_rpc(vfc, &rpc);

	ret = transform_cap_rpc((mpipe_rpc_pad_direction)direction, ind, &rpc, &other_rpc);
	if (ret != 0) {
		return ret;
	}

	mpipe_vid_transform_client_erpc_rpc_to_vfc(&other_rpc, other_vfc);

	return 0;
}

static int mpipe_vid_transform_client_erpc_init_rpc(void)
{
	erpc_client_t client = mpipe_erpc_client_init();

	if (client == NULL) {
		return -EIO;
	}

	initMpipeVidTransform_client(client);

	return 0;
}

int mpipe_vid_transform_client_erpc_init(struct mpipe_vid_transform_client *vtc, uint8_t id)
{
	__ASSERT_NO_MSG(vtc != NULL);

	vtc->transform.init_rpc = mpipe_vid_transform_client_erpc_init_rpc;
	vtc->transform.process_fn_rpc = process_fn_rpc;

	vtc->get_buf_caps_rpc = mpipe_vid_transform_client_erpc_get_buf_caps;
	vtc->get_format_caps_rpc = mpipe_vid_transform_client_erpc_get_format_caps;
	vtc->set_format_rpc = mpipe_vid_transform_client_erpc_set_format;
	vtc->transform_cap_rpc = mpipe_vid_transform_client_erpc_transform_cap;

	return mpipe_vid_transform_client_init(vtc, id);
}
