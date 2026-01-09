/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_sdl_dc

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>

#include <nsi_tracing.h>
#include <string.h>
#include <stdlib.h>
#include <soc.h>
#include <zephyr/sys/byteorder.h>
#include "display_sdl_bottom.h"
#include "cmdline.h"

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_sdl);

static uint32_t sdl_display_zoom_pct;

enum sdl_display_op {
	SDL_WRITE,
	SDL_BLANKING_OFF,
	SDL_BLANKING_ON,
};

struct sdl_display_write {
	uint16_t x;
	uint16_t y;
	const struct display_buffer_descriptor *desc;
};

struct sdl_display_task {
	enum sdl_display_op op;
	union {
		struct sdl_display_write write;
	};
};

struct sdl_display_config {
	uint16_t height;
	uint16_t width;
	const char *title;
};

struct sdl_display_data {
	void *window;
	void *renderer;
	void *mutex;
	void *texture;
	void *read_texture;
	void *background_texture;
	void *round_disp_mask;
	bool display_on;
	enum display_pixel_format current_pixel_format;
	uint8_t *buf;
	uint8_t *read_buf;
	double angle;
	struct k_thread sdl_thread;

	K_KERNEL_STACK_MEMBER(sdl_thread_stack, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
	struct k_msgq *task_msgq;
	struct k_sem task_sem;
	struct k_mutex task_mutex;
};

static inline uint32_t mono_pixel_order(uint32_t order)
{
	if (IS_ENABLED(CONFIG_SDL_DISPLAY_MONO_MSB_FIRST)) {
		return BIT(7 - order);
	} else {
		return BIT(order);
	}
}

static void exec_sdl_task(const struct device *dev, const struct sdl_display_task *task)
{
	struct sdl_display_data *disp_data = dev->data;

	switch (task->op) {
	case SDL_WRITE: {
		struct sdl_display_write_params write_params = {
			.height = task->write.desc->height,
			.width = task->write.desc->width,
			.x = task->write.x,
			.y = task->write.y,
			.renderer = disp_data->renderer,
			.mutex = disp_data->mutex,
			.texture = disp_data->texture,
			.background_texture = disp_data->background_texture,
			.buf = disp_data->buf,
			.display_on = disp_data->display_on,
			.frame_incomplete = task->write.desc->frame_incomplete,
			.color_tint = CONFIG_SDL_DISPLAY_COLOR_TINT,
			.round_disp_mask = disp_data->round_disp_mask,
			.angle = disp_data->angle,
		};
		sdl_display_write_bottom(&write_params);
		break;
	}
	case SDL_BLANKING_OFF: {
		struct sdl_display_blanking_off_params blanking_off_params = {
			.renderer = disp_data->renderer,
			.texture = disp_data->texture,
			.background_texture = disp_data->background_texture,
			.color_tint = CONFIG_SDL_DISPLAY_COLOR_TINT,
			.round_disp_mask = disp_data->round_disp_mask,
			.angle = disp_data->angle,
		};
		sdl_display_blanking_off_bottom(&blanking_off_params);
		break;
	}
	case SDL_BLANKING_ON:
		sdl_display_blanking_on_bottom(disp_data->renderer);
		break;
	default:
		LOG_ERR("Unknown SDL task");
		break;
	}
}

static void sdl_task_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct sdl_display_data *disp_data = dev->data;
	const struct sdl_display_config *config = dev->config;
	struct sdl_display_task task;
	bool use_accelerator =
		IS_ENABLED(CONFIG_SDL_DISPLAY_USE_HARDWARE_ACCELERATOR);

	if (sdl_display_zoom_pct == UINT32_MAX) {
		sdl_display_zoom_pct = CONFIG_SDL_DISPLAY_ZOOM_PCT;
	}

	struct sdl_display_init_params init_params = {
		.height = config->height,
		.width = config->width,
		.zoom_pct = sdl_display_zoom_pct,
		.use_accelerator = use_accelerator,
		.window = &disp_data->window,
		.window_user_data = dev,
		.title = config->title,
		.renderer = &disp_data->renderer,
		.mutex = &disp_data->mutex,
		.texture = &disp_data->texture,
		.read_texture = &disp_data->read_texture,
		.background_texture = &disp_data->background_texture,
		.transparency_grid_color1 = CONFIG_SDL_DISPLAY_TRANSPARENCY_GRID_CELL_COLOR_1,
		.transparency_grid_color2 = CONFIG_SDL_DISPLAY_TRANSPARENCY_GRID_CELL_COLOR_2,
		.transparency_grid_cell_size = CONFIG_SDL_DISPLAY_TRANSPARENCY_GRID_CELL_SIZE,
		.round_disp_mask = COND_CODE_1(CONFIG_SDL_DISPLAY_ROUNDED_MASK,
						(&disp_data->round_disp_mask), NULL),
		.mask_color = CONFIG_SDL_DISPLAY_ROUNDED_MASK_COLOR,
		.angle = 0.0,
		};

	int rc = sdl_display_init_bottom(&init_params);

	k_sem_give(&disp_data->task_sem);

	if (rc != 0) {
		nsi_print_error_and_exit("Failed to create SDL display");
		return;
	}

	disp_data->display_on = false;

	while (1) {
		k_msgq_get(disp_data->task_msgq, &task, K_FOREVER);
		exec_sdl_task(dev, &task);
		k_sem_give(&disp_data->task_sem);
	}
}

static int sdl_display_init(const struct device *dev)
{
	struct sdl_display_data *disp_data = dev->data;

	LOG_DBG("Initializing display driver");

	disp_data->current_pixel_format =
#if defined(CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_RGB_888)
		PIXEL_FORMAT_RGB_888
#elif defined(CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_MONO01)
		PIXEL_FORMAT_MONO01
#elif defined(CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_MONO10)
		PIXEL_FORMAT_MONO10
#elif defined(CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_RGB_565)
		PIXEL_FORMAT_RGB_565
#elif defined(CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_RGB_565X)
		PIXEL_FORMAT_RGB_565X
#elif defined(CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_L_8)
		PIXEL_FORMAT_L_8
#elif defined(CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_AL_88)
		PIXEL_FORMAT_AL_88
#else  /* SDL_DISPLAY_DEFAULT_PIXEL_FORMAT */
		PIXEL_FORMAT_ARGB_8888
#endif /* SDL_DISPLAY_DEFAULT_PIXEL_FORMAT */
		;

	k_sem_init(&disp_data->task_sem, 0, 1);
	k_mutex_init(&disp_data->task_mutex);
	k_thread_create(&disp_data->sdl_thread, disp_data->sdl_thread_stack,
			K_KERNEL_STACK_SIZEOF(disp_data->sdl_thread_stack),
			sdl_task_thread, (void *)dev, NULL, NULL,
			CONFIG_SDL_DISPLAY_THREAD_PRIORITY, 0, K_NO_WAIT);
	/* Ensure task thread has performed the init */
	k_sem_take(&disp_data->task_sem, K_FOREVER);

	return 0;
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * PIXEL_FORMAT_ARGB_8888	Bbbbbbbb Gggggggg Rrrrrrrr Aaaaaaaa
 * into
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Aaaaaaaa
 *
 * Hence, simple copy is enough
 */
static void sdl_display_write_argb8888(void *disp_buf,
		const struct display_buffer_descriptor *desc, const void *buf)
{
	__ASSERT((desc->pitch * 4U * desc->height) <= desc->buf_size,
			"Input buffer too small");

	memcpy(disp_buf, buf, desc->pitch * 4U * desc->height);
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * PIXEL_FORMAT_RGB_888		Bbbbbbbb Gggggggg Rrrrrrrr Bbbbbbbb
 * into
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Ffffffff
 */
static void sdl_display_write_rgb888(uint8_t *disp_buf,
		const struct display_buffer_descriptor *desc, const void *buf)
{
	uint32_t w_idx;
	uint32_t h_idx;
	uint32_t pixel;
	const uint8_t *byte_ptr;

	__ASSERT((desc->pitch * 3U * desc->height) <= desc->buf_size,
			"Input buffer too small");

	for (h_idx = 0U; h_idx < desc->height; ++h_idx) {
		for (w_idx = 0U; w_idx < desc->width; ++w_idx) {
			byte_ptr = (const uint8_t *)buf +
				((h_idx * desc->pitch) + w_idx) * 3U;
			pixel = *(byte_ptr + 2) << 16;		/* R */
			pixel |= *(byte_ptr + 1) << 8;		/* G */
			pixel |= *byte_ptr;			/* B */
			*((uint32_t *)disp_buf) = sys_cpu_to_le32(pixel | 0xFF000000);
			disp_buf += 4;
		}
	}
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * PIXEL_FORMAT_AL_88		Gggggggg Aaaaaaaa Gggggggg Aaaaaaaa
 * into
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Aaaaaaaa
 *
 * G value is applied to all R/G/B value for BGRA32
 */
static void sdl_display_write_al88(uint8_t *disp_buf,
		const struct display_buffer_descriptor *desc, const void *buf)
{
	uint32_t w_idx;
	uint32_t h_idx;
	uint32_t pixel;
	const uint8_t *byte_ptr;

	__ASSERT((desc->pitch * 2U * desc->height) <= desc->buf_size,
			"Input buffer too small");

	for (h_idx = 0U; h_idx < desc->height; ++h_idx) {
		for (w_idx = 0U; w_idx < desc->width; ++w_idx) {
			byte_ptr = (const uint8_t *)buf +
				((h_idx * desc->pitch) + w_idx) * 2U;
			pixel = *(byte_ptr + 1) << 24;	/* A */
			pixel |= *(byte_ptr) << 16;	/* R */
			pixel |= *(byte_ptr) << 8;	/* G */
			pixel |= *(byte_ptr);		/* B */
			*((uint32_t *)disp_buf) = sys_cpu_to_le32(pixel);
			disp_buf += 4;
		}
	}
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * PIXEL_FORMAT_RGB_565		gggBbbbb RrrrrGgg gggBbbbb RrrrrGgg
 * into
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Ffffffff
 */
static void sdl_display_write_rgb565(uint8_t *disp_buf,
		const struct display_buffer_descriptor *desc, const void *buf)
{
	uint32_t w_idx;
	uint32_t h_idx;
	uint32_t pixel;
	const uint16_t *pix_ptr;
	uint16_t rgb565;

	__ASSERT((desc->pitch * 2U * desc->height) <= desc->buf_size,
			"Input buffer too small");

	for (h_idx = 0U; h_idx < desc->height; ++h_idx) {
		for (w_idx = 0U; w_idx < desc->width; ++w_idx) {
			pix_ptr = (const uint16_t *)buf +
				((h_idx * desc->pitch) + w_idx);
			rgb565 = sys_le16_to_cpu(*pix_ptr);
			pixel = (((rgb565 >> 11) & 0x1F) * 255 / 31) << 16;	/* R */
			pixel |= (((rgb565 >> 5) & 0x3F) * 255 / 63) << 8;	/* G */
			pixel |= (rgb565 & 0x1F) * 255 / 31;			/* B */
			*((uint32_t *)disp_buf) = sys_cpu_to_le32(pixel | 0xFF000000);
			disp_buf += 4;
		}
	}
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * PIXEL_FORMAT_RGB_565X	RrrrrGgg gggBbbbb RrrrrGgg gggBbbbb
 * into
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Ffffffff
 */
static void sdl_display_write_rgb565x(uint8_t *disp_buf,
				      const struct display_buffer_descriptor *desc, const void *buf)
{
	uint32_t w_idx;
	uint32_t h_idx;
	uint32_t pixel;
	const uint16_t *pix_ptr;
	uint16_t rgb565;

	__ASSERT((desc->pitch * 2U * desc->height) <= desc->buf_size,
			"Input buffer too small");

	for (h_idx = 0U; h_idx < desc->height; ++h_idx) {
		for (w_idx = 0U; w_idx < desc->width; ++w_idx) {
			pix_ptr = (const uint16_t *)buf +
				((h_idx * desc->pitch) + w_idx);
			/*
			 * Perform be16_to_cpu here in order to swap it so that handling
			 * is same as for rgb565
			 */
			rgb565 = sys_be16_to_cpu(*pix_ptr);
			pixel = (((rgb565 >> 11) & 0x1F) * 255 / 31) << 16;	/* R */
			pixel |= (((rgb565 >> 5) & 0x3F) * 255 / 63) << 8;	/* G */
			pixel |= (rgb565 & 0x1F) * 255 / 31;			/* B */
			*((uint32_t *)disp_buf) = sys_cpu_to_le32(pixel | 0xFF000000);
			disp_buf += 4;
		}
	}
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * PIXEL_FORMAT_MONO(01/10)	MMMMMMMM MMMMMMMM MMMMMMMM MMMMMMMM
 * into
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Ffffffff
 */
static void sdl_display_write_mono(uint8_t *disp_buf, const struct display_buffer_descriptor *desc,
				   const void *buf, const bool one_is_black)
{
	const uint32_t pixel_on = one_is_black ? 0U : 0x00FFFFFF;
	uint32_t w_idx;
	uint32_t h_idx;
	bool pixel;
	const uint8_t *byte_ptr;

	__ASSERT((desc->pitch * desc->height) <= (desc->buf_size * 8U), "Input buffer too small");

	for (h_idx = 0U; h_idx < desc->height; ++h_idx) {
		for (w_idx = 0U; w_idx < desc->width; ++w_idx) {
			byte_ptr = buf;

			if (IS_ENABLED(CONFIG_SDL_DISPLAY_MONO_VTILED)) {
				byte_ptr += ((h_idx / 8U) * DIV_ROUND_UP(desc->pitch, 1U)) + w_idx;
				pixel = !!(*byte_ptr & mono_pixel_order(h_idx % 8U));
			} else {
				byte_ptr += (h_idx * DIV_ROUND_UP(desc->pitch, 8U)) + (w_idx / 8U);
				pixel = !!(*byte_ptr & mono_pixel_order(w_idx % 8U));
			}

			*((uint32_t *)disp_buf) =
				sys_cpu_to_le32((pixel ? pixel_on : ~pixel_on) | 0xFF000000);
			disp_buf += 4;
		}
	}
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * PIXEL_FORMAT_L_8		Gggggggg Gggggggg Gggggggg Gggggggg
 * into
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Ffffffff
 *
 * G value is applied to all R/G/B value for BGRA32
 */
static void sdl_display_write_l8(uint8_t *disp_buf, const struct display_buffer_descriptor *desc,
				 const void *buf)
{
	uint32_t w_idx;
	uint32_t h_idx;
	uint32_t pixel;
	const uint8_t *byte_ptr;

	__ASSERT((desc->pitch * desc->height) <= desc->buf_size, "Input buffer too small");

	for (h_idx = 0U; h_idx < desc->height; ++h_idx) {
		for (w_idx = 0U; w_idx < desc->width; ++w_idx) {
			byte_ptr = (const uint8_t *)buf + ((h_idx * desc->pitch) + w_idx);
			pixel = *byte_ptr << 16;	/* R */
			pixel |= *byte_ptr << 8;	/* G */
			pixel |= *byte_ptr;		/* B */
			*((uint32_t *)disp_buf) = sys_cpu_to_le32(pixel | 0xFF000000);
			disp_buf += 4;
		}
	}
}

static int sdl_display_write(const struct device *dev, const uint16_t x,
			     const uint16_t y,
			     const struct display_buffer_descriptor *desc,
			     const void *buf)
{
	const struct sdl_display_config *config = dev->config;
	struct sdl_display_data *disp_data = dev->data;
	struct sdl_display_task task = {
		.op = SDL_WRITE,
		.write = {
			.x = x,
			.y = y,
			.desc = desc,
		},
	};

	LOG_DBG("Writing %dx%d (w,h) bitmap @ %dx%d (x,y)", desc->width,
			desc->height, x, y);

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT(desc->pitch <= config->width,
		"Pitch in descriptor is larger than screen size");
	__ASSERT(desc->height <= config->height,
		"Height in descriptor is larger than screen size");
	__ASSERT(x + desc->width <= config->width,
		 "Writing outside screen boundaries in horizontal direction");
	__ASSERT(y + desc->height <= config->height,
		 "Writing outside screen boundaries in vertical direction");

	if (desc->width > desc->pitch ||
	    x + desc->width > config->width ||
	    y + desc->height > config->height) {
		return -EINVAL;
	}

	k_mutex_lock(&disp_data->task_mutex, K_FOREVER);
	if (disp_data->current_pixel_format == PIXEL_FORMAT_ARGB_8888) {
		sdl_display_write_argb8888(disp_data->buf, desc, buf);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_RGB_888) {
		sdl_display_write_rgb888(disp_data->buf, desc, buf);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_MONO10) {
		sdl_display_write_mono(disp_data->buf, desc, buf, true);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_MONO01) {
		sdl_display_write_mono(disp_data->buf, desc, buf, false);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_RGB_565) {
		sdl_display_write_rgb565(disp_data->buf, desc, buf);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_RGB_565X) {
		sdl_display_write_rgb565x(disp_data->buf, desc, buf);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_L_8) {
		sdl_display_write_l8(disp_data->buf, desc, buf);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_AL_88) {
		sdl_display_write_al88(disp_data->buf, desc, buf);
	}

	if (k_current_get() == &disp_data->sdl_thread) {
		exec_sdl_task(dev, &task);
	} else {
		k_msgq_put(disp_data->task_msgq, &task, K_FOREVER);
		k_sem_take(&disp_data->task_sem, K_FOREVER);
	}

	k_mutex_unlock(&disp_data->task_mutex);

	return 0;
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Aaaaaaaa
 * into
 * PIXEL_FORMAT_ARGB_8888	Bbbbbbbb Gggggggg Rrrrrrrr Aaaaaaaa
 *
 * Hence, simple copy is enough
 */
static void sdl_display_read_argb8888(const uint8_t *read_buf,
				      const struct display_buffer_descriptor *desc, void *buf)
{
	__ASSERT((desc->pitch * 4U * desc->height) <= desc->buf_size, "Read buffer is too small");

	memcpy(buf, read_buf, desc->pitch * 4U * desc->height);
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Ffffffff
 * into
 * PIXEL_FORMAT_RGB_888		Bbbbbbbb Gggggggg Rrrrrrrr Bbbbbbbb
 */
static void sdl_display_read_rgb888(const uint8_t *read_buf,
				    const struct display_buffer_descriptor *desc, void *buf)
{
	uint32_t w_idx;
	uint32_t h_idx;
	uint8_t *buf8;
	const uint32_t *pix_ptr;

	__ASSERT((desc->pitch * 3U * desc->height) <= desc->buf_size, "Read buffer is too small");

	for (h_idx = 0U; h_idx < desc->height; ++h_idx) {
		buf8 = ((uint8_t *)buf) + desc->pitch * 3U * h_idx;

		for (w_idx = 0U; w_idx < desc->width; ++w_idx) {
			pix_ptr = (const uint32_t *)read_buf + ((h_idx * desc->pitch) + w_idx);
			pix_ptr = sys_le32_to_cpu(pix_ptr);
			*buf8 = (*pix_ptr & 0xFF);		/* B */
			buf8 += 1;
			*buf8 = (*pix_ptr & 0xFF00) >> 8;	/* G */
			buf8 += 1;
			*buf8 = (*pix_ptr & 0xFF0000) >> 16;	/* R */
			buf8 += 1;
		}
	}
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Ffffffff
 * into
 * PIXEL_FORMAT_RGB_565		gggBbbbb RrrrrGgg gggBbbbb RrrrrGgg
 */
static void sdl_display_read_rgb565(const uint8_t *read_buf,
				    const struct display_buffer_descriptor *desc, void *buf)
{
	uint32_t w_idx;
	uint32_t h_idx;
	uint16_t pixel;
	uint16_t *buf16;
	const uint32_t *pix_ptr;

	__ASSERT((desc->pitch * 2U * desc->height) <= desc->buf_size, "Read buffer is too small");

	for (h_idx = 0U; h_idx < desc->height; ++h_idx) {
		buf16 = (void *)(((uint8_t *)buf) + desc->pitch * 2U * h_idx);

		for (w_idx = 0U; w_idx < desc->width; ++w_idx) {
			pix_ptr = (const uint32_t *)read_buf + ((h_idx * desc->pitch) + w_idx);
			pix_ptr = sys_le32_to_cpu(pix_ptr);
			pixel = (*pix_ptr & 0xF80000) >> 8;	/* R */
			pixel |= (*pix_ptr & 0x00FC00) >> 5;	/* G */
			pixel |= (*pix_ptr & 0x0000F8) >> 3;	/* B */
			*buf16 = sys_le16_to_cpu(pixel);
			buf16 += 1;
		}
	}
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Ffffffff
 * into
 * PIXEL_FORMAT_RGB_565X	RrrrrGgg gggBbbbb RrrrrGgg gggBbbbb
 */
static void sdl_display_read_rgb565x(const uint8_t *read_buf,
				     const struct display_buffer_descriptor *desc, void *buf)
{
	uint32_t w_idx;
	uint32_t h_idx;
	uint16_t pixel;
	uint16_t *buf16;
	const uint32_t *pix_ptr;

	__ASSERT((desc->pitch * 2U * desc->height) <= desc->buf_size, "Read buffer is too small");

	for (h_idx = 0U; h_idx < desc->height; ++h_idx) {
		buf16 = (void *)(((uint8_t *)buf) + desc->pitch * 2U * h_idx);

		for (w_idx = 0U; w_idx < desc->width; ++w_idx) {
			pix_ptr = (const uint32_t *)read_buf + ((h_idx * desc->pitch) + w_idx);
			pix_ptr = sys_le32_to_cpu(pix_ptr);
			pixel = (*pix_ptr & 0xF80000) >> 8;	/* R */
			pixel |= (*pix_ptr & 0x00FC00) >> 5;	/* G */
			pixel |= (*pix_ptr & 0x0000F8) >> 3;	/* B */
			/*
			 * Perform cpu_to_be16 here in order to swap it so that handling
			 * is same as for rgb565
			 */
			*buf16 = sys_cpu_to_be16(pixel);
			buf16 += 1;
		}
	}
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Ffffffff
 * into
 * PIXEL_FORMAT_MONO(01/10)	MMMMMMMM MMMMMMMM MMMMMMMM MMMMMMMM
 */
static void sdl_display_read_mono(const uint8_t *read_buf,
				  const struct display_buffer_descriptor *desc, void *buf,
				  const bool one_is_black)
{
	const uint32_t pixel_on = one_is_black ? 0xFF000000 : 0xFFFFFFFF;
	uint32_t w_idx;
	uint32_t h_idx;
	uint8_t bits;
	const uint32_t *pix_ptr;
	uint8_t *buf8;

	__ASSERT((desc->pitch * desc->height) <= (desc->buf_size * 8U), "Read buffer is too small");

	for (h_idx = 0U; h_idx < desc->height; ++h_idx) {
		for (w_idx = 0U; w_idx < desc->width; ++w_idx) {
			pix_ptr = (const uint32_t *)read_buf + h_idx * desc->pitch + w_idx;
			pix_ptr = sys_cpu_to_le32(pix_ptr);
			buf8 = buf;

			if (IS_ENABLED(CONFIG_SDL_DISPLAY_MONO_VTILED)) {
				buf8 += (h_idx / 8U) * DIV_ROUND_UP(desc->pitch, 1U) + (w_idx);
				bits = mono_pixel_order(h_idx % 8U);
			} else {
				buf8 += (h_idx)*DIV_ROUND_UP(desc->pitch, 8U) + (w_idx / 8U);
				bits = mono_pixel_order(w_idx % 8U);
			}

			if (*pix_ptr == pixel_on) {
				*buf8 |= bits;
			} else {
				*buf8 &= ~bits;
			}
		}
	}
}

/*
 * Convert from			Byte 0   Byte 1   Byte 2   Byte 3
 *				7......0 15.....8 23....16 31....24
 * SDL_PIXELFORMAT_BGRA32	Bbbbbbbb Gggggggg Rrrrrrrr Ffffffff
 * into
 * PIXEL_FORMAT_L_8		Gggggggg Gggggggg Gggggggg Gggggggg
 *
 * G value is taken from the B component of SDL_PIXELFORMAT_BGRA32
 */
static void sdl_display_read_l8(const uint8_t *read_buf,
				const struct display_buffer_descriptor *desc, void *buf)
{
	uint32_t w_idx;
	uint32_t h_idx;
	uint8_t *buf8;
	const uint32_t *pix_ptr;

	__ASSERT((desc->pitch * desc->height) <= desc->buf_size, "Read buffer is too small");

	for (h_idx = 0U; h_idx < desc->height; ++h_idx) {
		buf8 = ((uint8_t *)buf) + desc->pitch * h_idx;

		for (w_idx = 0U; w_idx < desc->width; ++w_idx) {
			pix_ptr = (const uint32_t *)read_buf + ((h_idx * desc->pitch) + w_idx);
			pix_ptr = sys_le32_to_cpu(pix_ptr);
			*buf8 = *pix_ptr & 0xFF;
			buf8 += 1;
		}
	}
}

static void sdl_display_read_al88(const uint8_t *read_buf,
				const struct display_buffer_descriptor *desc, void *buf)
{
	uint32_t w_idx;
	uint32_t h_idx;
	uint8_t *buf8;
	const uint32_t *pix_ptr;

	__ASSERT((desc->pitch * 2U * desc->height) <= desc->buf_size, "Read buffer is too small");

	for (h_idx = 0U; h_idx < desc->height; ++h_idx) {
		buf8 = ((uint8_t *)buf) + desc->pitch * 2U * h_idx;

		for (w_idx = 0U; w_idx < desc->width; ++w_idx) {
			pix_ptr = (const uint32_t *)read_buf + ((h_idx * desc->pitch) + w_idx);
			pix_ptr = sys_le32_to_cpu(pix_ptr);
			*buf8 = (*pix_ptr & 0xFF);
			buf8 += 1;
			*buf8 = (*pix_ptr & 0xFF000000) >> 24;
			buf8 += 1;
		}
	}
}

static int sdl_display_read(const struct device *dev, const uint16_t x, const uint16_t y,
			    const struct display_buffer_descriptor *desc, void *buf)
{
	struct sdl_display_data *disp_data = dev->data;
	int err;

	LOG_DBG("Reading %dx%d (w,h) bitmap @ %dx%d (x,y)", desc->width,
			desc->height, x, y);

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");

	k_mutex_lock(&disp_data->task_mutex, K_FOREVER);
	memset(disp_data->read_buf, 0, desc->pitch * desc->height * 4);

	struct sdl_display_read_params read_params = {
		.height = desc->height,
		.width = desc->width,
		.x = x,
		.y = y,
		.renderer = disp_data->renderer,
		.buf = disp_data->read_buf,
		.pitch = desc->pitch,
		.mutex = disp_data->mutex,
		.texture = disp_data->texture,
		.read_texture = disp_data->read_texture,
		.angle = disp_data->angle,
	};

	err = sdl_display_read_bottom(&read_params);

	if (err) {
		k_mutex_unlock(&disp_data->task_mutex);
		return err;
	}

	if (disp_data->current_pixel_format == PIXEL_FORMAT_ARGB_8888) {
		sdl_display_read_argb8888(disp_data->read_buf, desc, buf);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_RGB_888) {
		sdl_display_read_rgb888(disp_data->read_buf, desc, buf);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_MONO10) {
		sdl_display_read_mono(disp_data->read_buf, desc, buf, true);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_MONO01) {
		sdl_display_read_mono(disp_data->read_buf, desc, buf, false);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_RGB_565) {
		sdl_display_read_rgb565(disp_data->read_buf, desc, buf);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_RGB_565X) {
		sdl_display_read_rgb565x(disp_data->read_buf, desc, buf);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_L_8) {
		sdl_display_read_l8(disp_data->read_buf, desc, buf);
	} else if (disp_data->current_pixel_format == PIXEL_FORMAT_AL_88) {
		sdl_display_read_al88(disp_data->read_buf, desc, buf);
	}
	k_mutex_unlock(&disp_data->task_mutex);

	return 0;
}

static int sdl_display_clear(const struct device *dev)
{
	const struct sdl_display_config *config = dev->config;
	struct sdl_display_data *disp_data = dev->data;
	uint8_t bgcolor = 0x00u;
	size_t size;

	LOG_DBG("Clearing display screen to black");

	switch (disp_data->current_pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		size = config->width * config->height * 4U;
		break;
	case PIXEL_FORMAT_RGB_888:
		size = config->width * config->height * 3U;
		break;
	case PIXEL_FORMAT_MONO10:
		size = config->height * DIV_ROUND_UP(config->width,
						NUM_BITS(uint8_t));
		break;
	case PIXEL_FORMAT_MONO01:
		size = config->height * DIV_ROUND_UP(config->width,
						NUM_BITS(uint8_t));
		bgcolor = 0xFFu;
		break;
	case PIXEL_FORMAT_RGB_565:
	case PIXEL_FORMAT_RGB_565X:
		size = config->width * config->height * 2U;
		break;
	case PIXEL_FORMAT_AL_88:
		size = config->width * config->height * 2U;
		break;
	default:
		__ASSERT_MSG_INFO("Pixel format not supported");
		return -EINVAL;
	}
	LOG_DBG("size: %zu, bgcolor: %hhu", size, bgcolor);
	k_mutex_lock(&disp_data->task_mutex, K_FOREVER);
	memset(disp_data->buf, bgcolor, size);
	k_mutex_unlock(&disp_data->task_mutex);

	return 0;
}

static int sdl_display_blanking_off(const struct device *dev)
{
	struct sdl_display_data *disp_data = dev->data;
	struct sdl_display_task task = {
		.op = SDL_BLANKING_OFF,
	};

	LOG_DBG("Turning display blacking off");
	disp_data->display_on = true;
	k_mutex_lock(&disp_data->task_mutex, K_FOREVER);

	if (k_current_get() == &disp_data->sdl_thread) {
		exec_sdl_task(dev, &task);
	} else {
		k_msgq_put(disp_data->task_msgq, &task, K_FOREVER);
		k_sem_take(&disp_data->task_sem, K_FOREVER);
	}

	k_mutex_unlock(&disp_data->task_mutex);

	return 0;
}

static int sdl_display_blanking_on(const struct device *dev)
{
	struct sdl_display_data *disp_data = dev->data;
	struct sdl_display_task task = {
		.op = SDL_BLANKING_ON,
	};

	LOG_DBG("Turning display blanking on");
	disp_data->display_on = false;
	k_mutex_lock(&disp_data->task_mutex, K_FOREVER);

	if (k_current_get() == &disp_data->sdl_thread) {
		exec_sdl_task(dev, &task);
	} else {
		k_msgq_put(disp_data->task_msgq, &task, K_FOREVER);
		k_sem_take(&disp_data->task_sem, K_FOREVER);
	}

	k_mutex_unlock(&disp_data->task_mutex);

	return 0;
}

static void sdl_display_get_capabilities(
	const struct device *dev, struct display_capabilities *capabilities)
{
	const struct sdl_display_config *config = dev->config;
	struct sdl_display_data *disp_data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_ARGB_8888 |
		PIXEL_FORMAT_RGB_888 |
		PIXEL_FORMAT_MONO01 |
		PIXEL_FORMAT_MONO10 |
		PIXEL_FORMAT_RGB_565 |
		PIXEL_FORMAT_RGB_565X |
		PIXEL_FORMAT_L_8 |
		PIXEL_FORMAT_AL_88;
	capabilities->current_pixel_format = disp_data->current_pixel_format;
	capabilities->screen_info =
		(IS_ENABLED(CONFIG_SDL_DISPLAY_MONO_VTILED) ? SCREEN_INFO_MONO_VTILED : 0) |
		(IS_ENABLED(CONFIG_SDL_DISPLAY_MONO_MSB_FIRST) ? SCREEN_INFO_MONO_MSB_FIRST : 0);
}

static int sdl_display_set_pixel_format(const struct device *dev,
		const enum display_pixel_format pixel_format)
{
	struct sdl_display_data *disp_data = dev->data;

	switch (pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
	case PIXEL_FORMAT_RGB_888:
	case PIXEL_FORMAT_MONO01:
	case PIXEL_FORMAT_MONO10:
	case PIXEL_FORMAT_RGB_565:
	case PIXEL_FORMAT_RGB_565X:
	case PIXEL_FORMAT_L_8:
	case PIXEL_FORMAT_AL_88:
		disp_data->current_pixel_format = pixel_format;
		return 0;
	default:
		LOG_ERR("Pixel format not supported");
		return -ENOTSUP;
	}
}

static int sdl_display_set_orientation(const struct device *dev,
				       const enum display_orientation orientation)
{
	struct sdl_display_data *disp_data = dev->data;

	switch (orientation) {
	case DISPLAY_ORIENTATION_ROTATED_90:
		disp_data->angle = 90.0;
		break;
	case DISPLAY_ORIENTATION_ROTATED_180:
		disp_data->angle = 180.0;
		break;
	case DISPLAY_ORIENTATION_ROTATED_270:
		disp_data->angle = 270.0;
		break;
	default:
		disp_data->angle = 0.0;
		break;
	}

	LOG_DBG("Display orientation set to %d", orientation);
	return 0;
}

static void sdl_display_cleanup(struct sdl_display_data *disp_data)
{
	struct sdl_display_cleanup_params cleanup_params = {
		.window = &disp_data->window,
		.renderer = &disp_data->renderer,
		.mutex = &disp_data->mutex,
		.texture = &disp_data->texture,
		.read_texture = &disp_data->read_texture,
		.background_texture = &disp_data->background_texture,
		.round_disp_mask = &disp_data->round_disp_mask,
	};
	sdl_display_cleanup_bottom(&cleanup_params);
}

static DEVICE_API(display, sdl_display_api) = {
	.blanking_on = sdl_display_blanking_on,
	.blanking_off = sdl_display_blanking_off,
	.write = sdl_display_write,
	.read = sdl_display_read,
	.clear = sdl_display_clear,
	.get_capabilities = sdl_display_get_capabilities,
	.set_pixel_format = sdl_display_set_pixel_format,
	.set_orientation = sdl_display_set_orientation,
};

#define DISPLAY_SDL_DEFINE(n)                                                                      \
	static const struct sdl_display_config sdl_config_##n = {                                  \
		.height = DT_INST_PROP(n, height),                                                 \
		.width = DT_INST_PROP(n, width),                                                   \
		.title = DT_INST_PROP_OR(n, title, "Zephyr Display"),                              \
	};                                                                                         \
                                                                                                   \
	static uint8_t sdl_buf_##n[4 * DT_INST_PROP(n, height) * DT_INST_PROP(n, width)];          \
	static uint8_t sdl_read_buf_##n[4 * DT_INST_PROP(n, height) * DT_INST_PROP(n, width)];     \
	K_MSGQ_DEFINE(sdl_task_msgq_##n, sizeof(struct sdl_display_task), 1, 4);                   \
	static struct sdl_display_data sdl_data_##n = {                                            \
		.buf = sdl_buf_##n,                                                                \
		.read_buf = sdl_read_buf_##n,                                                      \
		.task_msgq = &sdl_task_msgq_##n,                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &sdl_display_init, NULL, &sdl_data_##n, &sdl_config_##n,          \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &sdl_display_api);        \
                                                                                                   \
	static void sdl_display_cleanup_##n(void)                                                  \
	{                                                                                          \
		sdl_display_cleanup(&sdl_data_##n);                                                \
	}                                                                                          \
                                                                                                   \
	NATIVE_TASK(sdl_display_cleanup_##n, ON_EXIT, 1);

DT_INST_FOREACH_STATUS_OKAY(DISPLAY_SDL_DEFINE)

static void display_sdl_options(void)
{
	static struct args_struct_t sdl_display_options[] = {
		{ .option = "display_zoom_pct",
		  .name = "pct",
		  .type = 'u',
		  .dest = (void *)&sdl_display_zoom_pct,
		  .descript = "Display zoom percentage (100 == 1:1 scale), "
			      "by default " STRINGIFY(CONFIG_SDL_DISPLAY_ZOOM_PCT)
			      " = CONFIG_SDL_DISPLAY_ZOOM_PCT"
		},
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(sdl_display_options);
}

NATIVE_TASK(display_sdl_options, PRE_BOOT_1, 1);
