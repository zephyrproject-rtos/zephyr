/*
 * Copyright 2023 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>

#include <u8g2_zephyr.h>
#include <u8g2.h>

struct _u8g2_zephyr_struct {
	u8g2_t u8g2;
	u8x8_display_info_t display_info;
	enum display_pixel_format pixel_format;
	enum display_screen_info screen_info;
	const struct device *dev;
	uint8_t buf[0];
};

typedef struct _u8g2_zephyr_struct u8g2_zephyr_t;

static inline uint8_t byte_reverse(uint8_t b)
{
	b = (b & 0xf0) >> 4 | (b & 0x0f) << 4;
	b = (b & 0xcc) >> 2 | (b & 0x33) << 2;
	b = (b & 0xaa) >> 1 | (b & 0x55) << 1;
	return b;
}

static int u8x8_Zephyr_DrawTiles(const u8g2_zephyr_t *u8x8_z, uint16_t tx, uint16_t ty,
				 uint8_t tile_cnt, uint8_t *tile_ptr)
{
	const bool need_reverse = ((u8x8_z->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0);
	struct display_buffer_descriptor desc;

	if (need_reverse) {
		for (uint32_t i = 0; i < tile_cnt * 8; i++) {
			tile_ptr[i] = byte_reverse(tile_ptr[i]);
		}
	}

	desc.buf_size = tile_cnt * 8;
	desc.width = tile_cnt * 8;
	desc.height = 8;
	desc.pitch = tile_cnt * 8;

	if (!(u8x8_z->pixel_format == PIXEL_FORMAT_MONO10) ||
	    (u8x8_z->pixel_format == PIXEL_FORMAT_MONO01)) {
		return -ENOTSUP;
	}

	if (!(u8x8_z->screen_info & SCREEN_INFO_MONO_VTILED)) {
		return -ENOTSUP;
	}

	return display_write(u8x8_z->dev, tx * 8, ty * 8, &desc, tile_ptr);
}

static uint8_t u8x8_zephyr_framebuffer_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	const u8g2_zephyr_t *u8x8_z =
		CONTAINER_OF(CONTAINER_OF(u8x8, u8g2_t, u8x8), u8g2_zephyr_t, u8g2);
	u8g2_uint_t x, y, c;
	uint8_t *ptr;

	switch (msg) {
	case U8X8_MSG_DISPLAY_SETUP_MEMORY:
		u8x8_d_helper_display_setup_memory(u8x8, &u8x8_z->display_info);
		break;
	case U8X8_MSG_DISPLAY_INIT:
		u8x8_d_helper_display_init(u8x8);
		break;
	case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
		break;
	case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
		break;
	case U8X8_MSG_DISPLAY_DRAW_TILE:
		x = ((u8x8_tile_t *)arg_ptr)->x_pos;
		y = ((u8x8_tile_t *)arg_ptr)->y_pos;
		c = ((u8x8_tile_t *)arg_ptr)->cnt;
		ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;
		do {
			if (u8x8_Zephyr_DrawTiles(u8x8_z, x, y, c, ptr)) {
				return 1;
			}
			x += c;
			arg_int--;
		} while (arg_int > 0);
		break;
	default:
		return 0;
	}

	return 1;
}

static u8g2_zephyr_t *u8g2_SetupZephyr_internal(const struct device *dev, const u8g2_cb_t *u8g2_cb)
{
	struct display_capabilities caps;
	u8g2_zephyr_t *u8x8_z;
	size_t buf_size = sizeof(u8g2_zephyr_t);

	display_get_capabilities(dev, &caps);

	if (u8g2_cb) {
		buf_size += (caps.x_resolution * caps.y_resolution / 8);
	}

	u8x8_z = k_malloc(buf_size);
	if (u8x8_z == NULL) {
		return NULL;
	}

	u8x8_z->dev = dev;
	u8x8_z->pixel_format = caps.current_pixel_format;
	u8x8_z->screen_info = caps.screen_info;
	u8x8_z->display_info.tile_width = (caps.x_resolution + 7) / 8;
	u8x8_z->display_info.tile_height = (caps.y_resolution + 7) / 8;
	u8x8_z->display_info.pixel_width = caps.x_resolution;
	u8x8_z->display_info.pixel_height = caps.y_resolution;

	u8x8_SetupDefaults(u8g2_GetU8x8(&u8x8_z->u8g2));

	u8g2_GetU8x8(&u8x8_z->u8g2)->display_cb = u8x8_zephyr_framebuffer_cb;

	u8x8_SetupMemory(u8g2_GetU8x8(&u8x8_z->u8g2));

	if (u8g2_cb) {
		u8g2_SetupBuffer(&u8x8_z->u8g2, u8x8_z->buf, u8x8_z->display_info.tile_height,
				 u8g2_ll_hvline_vertical_top_lsb, u8g2_cb);
	}

	return u8x8_z;
}

u8g2_t *u8g2_SetupZephyr(const struct device *dev, const u8g2_cb_t *u8g2_cb)
{
	u8g2_zephyr_t *u8g2_z;

	u8g2_z = u8g2_SetupZephyr_internal(dev, u8g2_cb);
	if (u8g2_z == NULL) {
		return NULL;
	}
	return &u8g2_z->u8g2;
}

u8x8_t *u8x8_SetupZephyr(const struct device *dev)
{
	u8g2_t *u8g2;

	u8g2 = u8g2_SetupZephyr(dev, NULL);
	if (u8g2 == NULL) {
		return NULL;
	}
	return u8g2_GetU8x8(u8g2);
}

void u8g2_TeardownZephyr(u8g2_t *u8g2)
{
	u8g2_zephyr_t *u8x8_z = CONTAINER_OF(u8g2, u8g2_zephyr_t, u8g2);

	k_free(u8x8_z);
}

void u8x8_TeardownZephyr(u8x8_t *u8x8)
{
	u8g2_t *u8g2 = CONTAINER_OF(u8x8, u8g2_t, u8x8);

	u8g2_TeardownZephyr(u8g2);
}
