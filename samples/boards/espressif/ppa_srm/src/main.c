/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/cache.h>

#include "esp_err.h"
#include "driver/ppa.h"

#define SCREEN_W 1024
#define SCREEN_H 600
#define BPP      3 /* RGB888 */

#define SRC_W 128
#define SRC_H 128
#define OUT_W 256
#define OUT_H 256

static const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static uint8_t *screen;
static uint8_t *src;
static uint8_t *dst;

static void put_pixel(uint8_t *buf, uint32_t pitch_px, uint32_t x, uint32_t y, uint8_t r, uint8_t g,
		      uint8_t b)
{
	uint8_t *p = buf + (y * pitch_px + x) * BPP;

	p[0] = r;
	p[1] = g;
	p[2] = b;
}

static void fill_rect(uint8_t *buf, uint32_t pitch_px, uint32_t x0, uint32_t y0, uint32_t w,
		      uint32_t h, uint8_t r, uint8_t g, uint8_t b)
{
	for (uint32_t y = 0; y < h; y++) {
		for (uint32_t x = 0; x < w; x++) {
			put_pixel(buf, pitch_px, x0 + x, y0 + y, r, g, b);
		}
	}
}

static void blit(uint8_t *img, uint32_t src_pitch, uint32_t copy_w, uint32_t copy_h, uint32_t dx,
		 uint32_t dy)
{
	for (uint32_t y = 0; y < copy_h; y++) {
		uint8_t *s = img + y * src_pitch * BPP;
		uint8_t *d = screen + ((dy + y) * SCREEN_W + dx) * BPP;

		memcpy(d, s, copy_w * BPP);
	}
}

static void draw_source(void)
{
	uint32_t hw = SRC_W / 2;
	uint32_t hh = SRC_H / 2;

	fill_rect(src, SRC_W, 0, 0, hw, hh, 0xFF, 0x00, 0x00);   /* top-left red */
	fill_rect(src, SRC_W, hw, 0, hw, hh, 0x00, 0xFF, 0x00);  /* top-right green */
	fill_rect(src, SRC_W, 0, hh, hw, hh, 0x00, 0x00, 0xFF);  /* bottom-left blue */
	fill_rect(src, SRC_W, hw, hh, hw, hh, 0xFF, 0xFF, 0x00); /* bottom-right yellow */

	for (uint32_t i = 0; i < SRC_W && i < SRC_H; i++) {
		put_pixel(src, SRC_W, i, i, 0xFF, 0xFF, 0xFF);
	}
}

static int run_srm(ppa_client_handle_t client, float scale, ppa_srm_rotation_angle_t angle,
		   bool mirror_x, bool mirror_y, uint32_t *out_w, uint32_t *out_h)
{
	ppa_srm_oper_config_t cfg = {0};

	cfg.in.buffer = src;
	cfg.in.pic_w = SRC_W;
	cfg.in.pic_h = SRC_H;
	cfg.in.block_w = SRC_W;
	cfg.in.block_h = SRC_H;
	cfg.in.srm_cm = PPA_SRM_COLOR_MODE_RGB888;

	cfg.out.buffer = dst;
	cfg.out.buffer_size = OUT_W * OUT_H * BPP;
	cfg.out.pic_w = OUT_W;
	cfg.out.pic_h = OUT_H;
	cfg.out.srm_cm = PPA_SRM_COLOR_MODE_RGB888;

	cfg.scale_x = scale;
	cfg.scale_y = scale;
	cfg.rotation_angle = angle;
	cfg.mirror_x = mirror_x;
	cfg.mirror_y = mirror_y;
	cfg.mode = PPA_TRANS_MODE_BLOCKING;

	uint32_t w = (uint32_t)(SRC_W * scale);
	uint32_t h = (uint32_t)(SRC_H * scale);

	if (angle == PPA_SRM_ROTATION_ANGLE_90 || angle == PPA_SRM_ROTATION_ANGLE_270) {
		uint32_t t = w;

		w = h;
		h = t;
	}
	*out_w = w;
	*out_h = h;

	memset(dst, 0, OUT_W * OUT_H * BPP);
	sys_cache_data_flush_range(dst, OUT_W * OUT_H * BPP);
	sys_cache_data_flush_range(src, SRC_W * SRC_H * BPP);

	esp_err_t err = ppa_do_scale_rotate_mirror(client, &cfg);

	if (err != ESP_OK) {
		printk("SRM failed: %d\n", err);
		return -1;
	}

	sys_cache_data_invd_range(dst, OUT_W * OUT_H * BPP);

	return 0;
}

static void present(void)
{
	struct display_buffer_descriptor d = {
		.buf_size = SCREEN_W * SCREEN_H * BPP,
		.width = SCREEN_W,
		.height = SCREEN_H,
		.pitch = SCREEN_W,
	};

	sys_cache_data_flush_range(screen, SCREEN_W * SCREEN_H * BPP);
	display_write(display_dev, 0, 0, &d, screen);
}

int main(void)
{
	if (!device_is_ready(display_dev)) {
		printk("Display not ready\n");
		return 0;
	}

	screen = shared_multi_heap_aligned_alloc(SMH_REG_ATTR_EXTERNAL, 64,
						 SCREEN_W * SCREEN_H * BPP);
	src = shared_multi_heap_aligned_alloc(SMH_REG_ATTR_EXTERNAL, 64, SRC_W * SRC_H * BPP);
	dst = shared_multi_heap_aligned_alloc(SMH_REG_ATTR_EXTERNAL, 64, OUT_W * OUT_H * BPP);

	if (screen == NULL || src == NULL || dst == NULL) {
		printk("Buffer alloc failed\n");
		shared_multi_heap_free(screen);
		shared_multi_heap_free(src);
		shared_multi_heap_free(dst);
		return 0;
	}

	draw_source();

	ppa_client_config_t ccfg = {
		.oper_type = PPA_OPERATION_SRM,
		.data_burst_length = PPA_DATA_BURST_LENGTH_128,
	};
	ppa_client_handle_t client;

	if (ppa_register_client(&ccfg, &client) != ESP_OK) {
		printk("PPA client register failed\n");
		return 0;
	}

	struct {
		const char *name;
		float scale;
		ppa_srm_rotation_angle_t angle;
		bool mx;
		bool my;
	} steps[] = {
		{"1x identity", 1.0f, PPA_SRM_ROTATION_ANGLE_0, false, false},
		{"2x scale", 2.0f, PPA_SRM_ROTATION_ANGLE_0, false, false},
		{"1x rotate 90", 1.0f, PPA_SRM_ROTATION_ANGLE_90, false, false},
		{"1x rotate 180", 1.0f, PPA_SRM_ROTATION_ANGLE_180, false, false},
		{"1x mirror-x", 1.0f, PPA_SRM_ROTATION_ANGLE_0, true, false},
	};

	printk("PPA SRM: source is red/green/blue/yellow quadrants + white diagonal\n");

	int step = 0;

	while (1) {
		int i = step % ARRAY_SIZE(steps);
		uint32_t ow, oh;

		memset(screen, 0x20, SCREEN_W * SCREEN_H * BPP);

		blit(src, SRC_W, SRC_W, SRC_H, 40, 40);

		if (run_srm(client, steps[i].scale, steps[i].angle, steps[i].mx, steps[i].my, &ow,
			    &oh) == 0) {
			blit(dst, OUT_W, ow, oh, 300, 40);
			printk("step %d: %s -> out %ux%u\n", i, steps[i].name, ow, oh);
		}

		present();
		step++;
		k_sleep(K_SECONDS(3));
	}

	return 0;
}
