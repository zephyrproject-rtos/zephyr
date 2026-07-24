/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/video.h>
#include <zephyr/sys/util.h>

#include <zephyr/mp/core/mp_buffer.h>

#include <zephyr/mp/zvid/mp_zvid_convert.h>
#include <zephyr/mp/zvid/mp_zvid_convert_table.h>

static int convert_nv12_to_rgb565(struct mp_zvid_convert *conv, const struct net_buf *in,
				  struct net_buf *out)
{
	const uint8_t *y_plane = in->data;
	const uint8_t *uv_plane = in->data + ((uint32_t)conv->width * (uint32_t)conv->height);

	conv->impl.nv12_to_rgb565(conv->width, conv->height, y_plane, uv_plane,
				  (uint16_t *)out->data);
	return 0;
}

static int convert_xrgb_to_argb(struct mp_zvid_convert *conv, const struct net_buf *in,
				struct net_buf *out)
{
	uint32_t pixels = (uint32_t)conv->width * (uint32_t)conv->height;
	const uint8_t *src = in->data;
	uint8_t *dst = out->data;

	for (uint32_t i = 0; i < pixels; i++) {
		dst[0] = 0xFF; /* A */
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = src[3];
		src += 4;
		dst += 4;
	}
	return 0;
}

static int convert_argb_to_xrgb(struct mp_zvid_convert *conv, const struct net_buf *in,
				struct net_buf *out)
{
	uint32_t pixels = (uint32_t)conv->width * (uint32_t)conv->height;
	const uint8_t *src = in->data;
	uint8_t *dst = out->data;

	for (uint32_t i = 0; i < pixels; i++) {
		dst[0] = 0xFF; /* X */
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = src[3];
		src += 4;
		dst += 4;
	}
	return 0;
}

const struct mp_zvid_convert_desc mp_zvid_convert_descs[] = {
	{VIDEO_PIX_FMT_NV12, VIDEO_PIX_FMT_RGB565, convert_nv12_to_rgb565},
	{VIDEO_PIX_FMT_XRGB32, VIDEO_PIX_FMT_ARGB32, convert_xrgb_to_argb},
	{VIDEO_PIX_FMT_ARGB32, VIDEO_PIX_FMT_XRGB32, convert_argb_to_xrgb},
};

const size_t mp_zvid_convert_descs_len = ARRAY_SIZE(mp_zvid_convert_descs);
