/*
 * Copyright (c) 2026 Gustavo Reis
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_lcd_cam_rgb

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/dt-bindings/display/panel.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/sys/util.h>

#include <esp_clk_tree.h>
#include <esp_intr_alloc.h>
#include <esp_memory_utils.h>
#include <hal/dma_types.h>
#include <hal/gdma_channel.h>
#include <hal/gdma_ll.h>
#include <hal/lcd_hal.h>
#include <hal/lcd_ll.h>
#include <soc/interrupts.h>

LOG_MODULE_REGISTER(display_esp32_lcd_cam_rgb, CONFIG_DISPLAY_LOG_LEVEL);

#define ESP32_LCD_CAM_RGB_BUS_ID           (0)
#define ESP32_LCD_CAM_RGB_ALIGNMENT        (64U)
#define ESP32_LCD_CAM_RGB_DMA_CHUNK_SIZE   (4032U)
#define ESP32_LCD_CAM_RGB_CACHE_CHUNK_SIZE (4096U)
#define ESP32_LCD_CAM_RGB_FRAME_WAIT_MS    (100U)

#define ESP32_LCD_CAM_RGB_FB_COUNT CONFIG_DISPLAY_ESP32_LCD_CAM_RGB_FB_NUM
#define ESP32_LCD_CAM_RGB_BB_LINES CONFIG_DISPLAY_ESP32_LCD_CAM_RGB_BOUNCE_BUFFER_LINES

struct esp32_lcd_cam_rgb_config {
	const struct device *dma_clock_dev;
	clock_control_subsys_t dma_clock_subsys;
	soc_periph_lcd_clk_src_t clock_source;
	int irq_source;
	int irq_priority;
	int irq_flags;
	uint8_t dma_channel;
	uint16_t width;
	uint16_t height;
	uint32_t pixel_format;
	uint8_t data_bus_width;
	uint32_t pclk_freq_hz;
	uint16_t hsync_len;
	uint16_t hback_porch;
	uint16_t hfront_porch;
	uint16_t vsync_len;
	uint16_t vback_porch;
	uint16_t vfront_porch;
	bool hsync_active;
	bool vsync_active;
	bool de_active;
	bool pixelclk_active;
	bool pclk_idle_high;
	bool de_mode_only;
	bool invert_byte_order;
	bool invert_bit_order;
	struct gpio_dt_spec backlight_gpio;
	struct gpio_dt_spec disp_gpio;
	struct gpio_dt_spec reset_gpio;
};

struct esp32_lcd_cam_rgb_data {
	const struct device *dev;
	lcd_hal_context_t hal;
	uint8_t *fb[ESP32_LCD_CAM_RGB_FB_COUNT];
#if ESP32_LCD_CAM_RGB_BB_LINES == 0
	dma_descriptor_t *fb_desc_pool;
	dma_descriptor_t *fb_descs[ESP32_LCD_CAM_RGB_FB_COUNT];
	uint32_t descs_per_fb;
#endif
	size_t fb_size;
	size_t fb_alloc_size;
	uint8_t dma_channel;
	uint8_t bytes_per_pixel;
	uint8_t active_fb;
	uint8_t draw_fb;
	int8_t pending_fb;
	bool frame_dirty;
	uint16_t dirty_x1;
	uint16_t dirty_y1;
	uint16_t dirty_x2;
	uint16_t dirty_y2;
	struct k_mutex write_lock;
	struct k_spinlock lock;
	struct k_sem vsync_sem;
	display_event_cb_t event_cb;
	void *event_user_data;
	uint32_t event_mask;
#if ESP32_LCD_CAM_RGB_BB_LINES > 0
	uint8_t *bb[2];
	dma_descriptor_t *bb_descs[2];
	uint32_t descs_per_bb;
	size_t bb_size;
	size_t bb_read_offset;
	intr_handle_t dma_intr;
#endif
};

static void esp32_lcd_cam_rgb_flush_cache(void *addr, size_t size)
{
	uint8_t *ptr = (uint8_t *)addr;
	size_t remaining = size;

	if (!esp_ptr_external_ram(addr)) {
		return;
	}

	while (remaining > 0U) {
		size_t chunk = MIN(remaining, ESP32_LCD_CAM_RGB_CACHE_CHUNK_SIZE);

		sys_cache_data_flush_range(ptr, chunk);
		ptr += chunk;
		remaining -= chunk;
	}
}

static void esp32_lcd_cam_rgb_build_desc_chain(dma_descriptor_t *descs, uint32_t num_descs,
					       uint8_t *buf, size_t total_bytes,
					       dma_descriptor_t *loop_target, bool mark_eof)
{
	size_t remaining = total_bytes;
	uint8_t *ptr = buf;

	for (uint32_t i = 0; i < num_descs; i++) {
		uint32_t chunk = (remaining > ESP32_LCD_CAM_RGB_DMA_CHUNK_SIZE)
					 ? ESP32_LCD_CAM_RGB_DMA_CHUNK_SIZE
					 : (uint32_t)remaining;
		bool is_last = (i == (num_descs - 1U));

		memset(&descs[i], 0, sizeof(dma_descriptor_t));
		descs[i].dw0.size = chunk;
		descs[i].dw0.length = chunk;
		descs[i].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
		descs[i].dw0.suc_eof = (is_last && mark_eof) ? 1U : 0U;
		descs[i].buffer = ptr;
		descs[i].next = is_last ? loop_target : &descs[i + 1U];

		ptr += chunk;
		remaining -= chunk;
	}

	esp32_lcd_cam_rgb_flush_cache(descs, num_descs * sizeof(dma_descriptor_t));
}

#if ESP32_LCD_CAM_RGB_BB_LINES > 0
static void IRAM_ATTR esp32_lcd_cam_rgb_fill_bb(struct esp32_lcd_cam_rgb_data *data, uint8_t bb_idx)
{
	size_t remaining = data->fb_size - data->bb_read_offset;
	size_t copy_len = MIN(data->bb_size, remaining);
	const uint8_t *src = data->fb[data->active_fb] + data->bb_read_offset;

	memcpy(data->bb[bb_idx], src, copy_len);
	data->bb_read_offset += copy_len;

	if (copy_len < data->bb_size) {
		size_t tail = data->bb_size - copy_len;

		if (data->pending_fb >= 0) {
			data->active_fb = (uint8_t)data->pending_fb;
			data->pending_fb = -1;
		}
		memcpy(data->bb[bb_idx] + copy_len, data->fb[data->active_fb], tail);
		data->bb_read_offset = tail;
	} else if (data->bb_read_offset >= data->fb_size) {
		data->bb_read_offset = 0U;
		if (data->pending_fb >= 0) {
			data->active_fb = (uint8_t)data->pending_fb;
			data->pending_fb = -1;
		}
	}
}

static void IRAM_ATTR esp32_lcd_cam_rgb_dma_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct esp32_lcd_cam_rgb_data *data = dev->data;
	gdma_dev_t *gdma = GDMA_LL_GET_HW(0);
	uint32_t status = gdma_ll_tx_get_interrupt_status(gdma, data->dma_channel, false);

	gdma_ll_tx_clear_interrupt_status(gdma, data->dma_channel, status);

	if ((status & GDMA_LL_EVENT_TX_EOF) != 0U) {
		uint32_t eof_addr = gdma_ll_tx_get_eof_desc_addr(gdma, data->dma_channel);
		dma_descriptor_t *bb0_last = &data->bb_descs[0][data->descs_per_bb - 1U];
		uint8_t finished_bb = (eof_addr == (uint32_t)bb0_last) ? 0U : 1U;
		k_spinlock_key_t key = k_spin_lock(&data->lock);

		esp32_lcd_cam_rgb_fill_bb(data, finished_bb);
		k_spin_unlock(&data->lock, key);
	}
}
#endif /* ESP32_LCD_CAM_RGB_BB_LINES > 0 */

static void IRAM_ATTR esp32_lcd_cam_rgb_vsync_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct esp32_lcd_cam_rgb_data *data = dev->data;
	lcd_hal_context_t *hal = &data->hal;
	uint32_t intr_status = lcd_ll_get_interrupt_status(hal->dev);

	lcd_ll_clear_interrupt_status(hal->dev, intr_status);

	if ((intr_status & LCD_LL_EVENT_VSYNC_END) == 0U) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

#if ESP32_LCD_CAM_RGB_BB_LINES == 0
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;
	gdma_dev_t *gdma = GDMA_LL_GET_HW(0);

	if (data->pending_fb >= 0) {
		uint8_t prev_fb = data->active_fb;

		data->active_fb = (uint8_t)data->pending_fb;
		data->pending_fb = -1;
		data->fb_descs[prev_fb][data->descs_per_fb - 1U].next = &data->fb_descs[prev_fb][0];
	}

	if (IS_ENABLED(CONFIG_DISPLAY_ESP32_LCD_CAM_RGB_RESTART_IN_VSYNC)) {
		dma_descriptor_t *head = &data->fb_descs[data->active_fb][0];

		lcd_ll_stop(hal->dev);
		gdma_ll_tx_stop(gdma, cfg->dma_channel);
		gdma_ll_tx_reset_channel(gdma, cfg->dma_channel);
		lcd_ll_fifo_reset(hal->dev);
		gdma_ll_tx_set_desc_addr(gdma, cfg->dma_channel, (uint32_t)head);
		gdma_ll_tx_start(gdma, cfg->dma_channel);
		lcd_ll_start(hal->dev);
	}
#endif

	display_event_cb_t cb = data->event_cb;
	uint32_t mask = data->event_mask;
	void *user_data = data->event_user_data;
	uint8_t shown = data->active_fb;

	k_spin_unlock(&data->lock, key);

	if (cb != NULL) {
		struct display_event_data evt = {
			.info = {.buffer_id = (int)shown},
		};

		if ((mask & DISPLAY_EVENT_VSYNC) != 0U) {
			(void)cb(dev, DISPLAY_EVENT_VSYNC, &evt, user_data);
		}
		if ((mask & DISPLAY_EVENT_FRAME_DONE) != 0U) {
			(void)cb(dev, DISPLAY_EVENT_FRAME_DONE, &evt, user_data);
		}
	}

	k_sem_give(&data->vsync_sem);
}

static int esp32_lcd_cam_rgb_wait_fb_free(struct esp32_lcd_cam_rgb_data *data, uint8_t fb_idx,
					  k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (true) {
		k_spinlock_key_t key = k_spin_lock(&data->lock);
		bool busy = (fb_idx == data->active_fb) || ((int8_t)fb_idx == data->pending_fb);

		k_spin_unlock(&data->lock, key);

		if (!busy) {
			return 0;
		}

		if (k_sem_take(&data->vsync_sem, sys_timepoint_timeout(end)) != 0 &&
		    sys_timepoint_expired(end)) {
			return -EAGAIN;
		}
	}
}

static void esp32_lcd_cam_rgb_copy_rect(uint8_t *dst_fb, const uint8_t *src_fb, uint16_t x,
					uint16_t y, uint16_t w, uint16_t h, uint32_t pitch)
{
	if (x == 0U && (uint32_t)w == pitch) {
		uint32_t offset = (uint32_t)y * pitch;
		size_t bytes = (size_t)h * pitch;

		memcpy(dst_fb + offset, src_fb + offset, bytes);
		esp32_lcd_cam_rgb_flush_cache(dst_fb + offset, bytes);
		return;
	}

	for (uint16_t row = 0; row < h; row++) {
		uint32_t offset = (uint32_t)(y + row) * pitch + (uint32_t)x;

		memcpy(dst_fb + offset, src_fb + offset, w);
		esp32_lcd_cam_rgb_flush_cache(dst_fb + offset, w);
	}
}

static int esp32_lcd_cam_rgb_write(const struct device *dev, const uint16_t x, const uint16_t y,
				   const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;
	struct esp32_lcd_cam_rgb_data *data = dev->data;
	uint16_t write_w = desc->width;
	uint16_t write_h = desc->height;
	uint8_t bpp = data->bytes_per_pixel;
	uint32_t src_pitch = desc->pitch * bpp;
	uint32_t dst_pitch = cfg->width * bpp;
	bool multi = (ESP32_LCD_CAM_RGB_FB_COUNT > 1);
	int ret = 0;

	if (buf == NULL || desc == NULL) {
		return -EINVAL;
	}

	if ((uint32_t)x + write_w > cfg->width || (uint32_t)y + write_h > cfg->height) {
		LOG_ERR("Write %ux%u at (%u,%u) exceeds %ux%u panel", write_w, write_h, x, y,
			cfg->width, cfg->height);
		return -EINVAL;
	}

	if (desc->pitch < write_w) {
		LOG_ERR("Pitch %u is smaller than width %u", desc->pitch, write_w);
		return -EINVAL;
	}

	if ((size_t)src_pitch * write_h > desc->buf_size) {
		LOG_ERR("Buffer of %u bytes is too small for %ux%u", desc->buf_size, write_w,
			write_h);
		return -EINVAL;
	}

	k_mutex_lock(&data->write_lock, K_FOREVER);

	if (multi) {
		(void)esp32_lcd_cam_rgb_wait_fb_free(data, data->draw_fb,
						     K_MSEC(ESP32_LCD_CAM_RGB_FRAME_WAIT_MS));
		if (!data->frame_dirty) {
			data->dirty_x1 = x;
			data->dirty_y1 = y;
			data->dirty_x2 = x + write_w;
			data->dirty_y2 = y + write_h;
			data->frame_dirty = true;
		} else {
			data->dirty_x1 = MIN(data->dirty_x1, x);
			data->dirty_y1 = MIN(data->dirty_y1, y);
			data->dirty_x2 = MAX(data->dirty_x2, (uint16_t)(x + write_w));
			data->dirty_y2 = MAX(data->dirty_y2, (uint16_t)(y + write_h));
		}
	}

	uint8_t *fb = data->fb[data->draw_fb];

	if (buf != fb) {
		if (x == 0U && write_w == cfg->width && desc->pitch == cfg->width) {
			uint32_t dst_offset = (uint32_t)y * dst_pitch;
			size_t copy_bytes = (size_t)write_h * dst_pitch;

			memcpy(fb + dst_offset, buf, copy_bytes);
			esp32_lcd_cam_rgb_flush_cache(fb + dst_offset, copy_bytes);
		} else {
			size_t row_bytes = (size_t)write_w * bpp;

			for (uint16_t row = 0; row < write_h; row++) {
				uint32_t dst_offset =
					(uint32_t)(y + row) * dst_pitch + (uint32_t)x * bpp;
				uint32_t src_offset = (uint32_t)row * src_pitch;

				memcpy(fb + dst_offset, (const uint8_t *)buf + src_offset,
				       row_bytes);
				esp32_lcd_cam_rgb_flush_cache(fb + dst_offset, row_bytes);
			}
		}
	} else {
		esp32_lcd_cam_rgb_flush_cache(fb + (uint32_t)y * dst_pitch,
					      (size_t)write_h * dst_pitch);
	}

	if (multi && !desc->frame_incomplete) {
		uint8_t completed_fb = data->draw_fb;
		uint8_t next_fb = (data->draw_fb + 1U) % ESP32_LCD_CAM_RGB_FB_COUNT;
		k_spinlock_key_t key = k_spin_lock(&data->lock);

		k_sem_reset(&data->vsync_sem);
		data->pending_fb = (int8_t)completed_fb;
#if ESP32_LCD_CAM_RGB_BB_LINES == 0
		data->fb_descs[data->active_fb][data->descs_per_fb - 1U].next =
			&data->fb_descs[completed_fb][0];
		data->fb_descs[completed_fb][data->descs_per_fb - 1U].next =
			&data->fb_descs[completed_fb][0];
#endif
		k_spin_unlock(&data->lock, key);

		if (esp32_lcd_cam_rgb_wait_fb_free(data, next_fb,
						   K_MSEC(ESP32_LCD_CAM_RGB_FRAME_WAIT_MS)) == 0 &&
		    data->frame_dirty) {
			uint16_t byte_x = data->dirty_x1 * bpp;
			uint16_t byte_w = (data->dirty_x2 - data->dirty_x1) * bpp;
			uint16_t copy_h = data->dirty_y2 - data->dirty_y1;

			esp32_lcd_cam_rgb_copy_rect(data->fb[next_fb], data->fb[completed_fb],
						    byte_x, data->dirty_y1, byte_w, copy_h,
						    dst_pitch);
		}

		data->draw_fb = next_fb;
		data->frame_dirty = false;
	}

	k_mutex_unlock(&data->write_lock);

	return ret;
}

static int esp32_lcd_cam_rgb_read(const struct device *dev, const uint16_t x, const uint16_t y,
				  const struct display_buffer_descriptor *desc, void *buf)
{
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;
	struct esp32_lcd_cam_rgb_data *data = dev->data;
	uint8_t bpp = data->bytes_per_pixel;
	uint32_t src_pitch = cfg->width * bpp;
	uint32_t dst_pitch = desc->pitch * bpp;

	if (buf == NULL || desc == NULL) {
		return -EINVAL;
	}

	if ((uint32_t)x + desc->width > cfg->width || (uint32_t)y + desc->height > cfg->height) {
		return -EINVAL;
	}

	if (desc->pitch < desc->width || (size_t)dst_pitch * desc->height > desc->buf_size) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	const uint8_t *fb = data->fb[data->active_fb];

	k_spin_unlock(&data->lock, key);

	for (uint16_t row = 0; row < desc->height; row++) {
		memcpy((uint8_t *)buf + (size_t)row * dst_pitch,
		       fb + (size_t)(y + row) * src_pitch + (size_t)x * bpp,
		       (size_t)desc->width * bpp);
	}

	return 0;
}

static void *esp32_lcd_cam_rgb_get_framebuffer(const struct device *dev)
{
	struct esp32_lcd_cam_rgb_data *data = dev->data;

	return data->fb[data->draw_fb];
}

static int esp32_lcd_cam_rgb_blanking_on(const struct device *dev)
{
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;
	int ret = 0;

	if (cfg->backlight_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight_gpio, 0);
		if (ret < 0) {
			return ret;
		}
	}

	if (cfg->disp_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->disp_gpio, 0);
	}

	return ret;
}

static int esp32_lcd_cam_rgb_blanking_off(const struct device *dev)
{
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;
	int ret = 0;

	if (cfg->disp_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->disp_gpio, 1);
		if (ret < 0) {
			return ret;
		}
	}

	if (cfg->backlight_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight_gpio, 1);
	}

	return ret;
}

static void esp32_lcd_cam_rgb_get_capabilities(const struct device *dev,
					       struct display_capabilities *capabilities)
{
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;

	memset(capabilities, 0, sizeof(*capabilities));
	capabilities->x_resolution = cfg->width;
	capabilities->y_resolution = cfg->height;
	capabilities->supported_pixel_formats = cfg->pixel_format;
	capabilities->current_pixel_format = cfg->pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	capabilities->supported_events = DISPLAY_EVENT_VSYNC | DISPLAY_EVENT_FRAME_DONE;
}

static int esp32_lcd_cam_rgb_set_pixel_format(const struct device *dev,
					      const enum display_pixel_format pixel_format)
{
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;

	if (pixel_format == cfg->pixel_format) {
		return 0;
	}

	return -ENOTSUP;
}

static int esp32_lcd_cam_rgb_set_orientation(const struct device *dev,
					     const enum display_orientation orientation)
{
	ARG_UNUSED(dev);

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}

	return -ENOTSUP;
}

static int esp32_lcd_cam_rgb_register_event_cb(const struct device *dev, display_event_cb_t cb,
					       void *user_data, uint32_t event_mask, bool in_isr,
					       uint32_t *out_reg_handle)
{
	struct esp32_lcd_cam_rgb_data *data = dev->data;
	k_spinlock_key_t key;

	if (out_reg_handle == NULL || cb == NULL) {
		return -EINVAL;
	}

	if (!in_isr) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);
	if (data->event_cb != NULL) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->event_cb = cb;
	data->event_user_data = user_data;
	data->event_mask = event_mask;
	k_spin_unlock(&data->lock, key);

	*out_reg_handle = 1U;

	return 0;
}

static int esp32_lcd_cam_rgb_unregister_event_cb(const struct device *dev, uint32_t reg_handle)
{
	struct esp32_lcd_cam_rgb_data *data = dev->data;
	k_spinlock_key_t key;

	if (reg_handle != 1U) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (data->event_cb == NULL) {
		k_spin_unlock(&data->lock, key);
		return -EPERM;
	}

	data->event_cb = NULL;
	data->event_user_data = NULL;
	data->event_mask = 0U;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int esp32_lcd_cam_rgb_init(const struct device *dev)
{
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;
	struct esp32_lcd_cam_rgb_data *data = dev->data;
	lcd_hal_context_t *hal = &data->hal;
	gdma_dev_t *gdma = GDMA_LL_GET_HW(0);
	hal_utils_clk_div_t lcd_clk_div = {0};
	uint32_t clk_src_freq_hz = 0;
	uint32_t actual_pclk_hz;
	int ret;

	data->dev = dev;
	data->dma_channel = cfg->dma_channel;
	data->active_fb = 0U;
	data->draw_fb = (ESP32_LCD_CAM_RGB_FB_COUNT > 1) ? 1U : 0U;
	data->pending_fb = -1;
	data->frame_dirty = false;

	k_mutex_init(&data->write_lock);
	k_sem_init(&data->vsync_sem, 0, 1);

	switch (cfg->pixel_format) {
	case PANEL_PIXEL_FORMAT_RGB_565:
	case PANEL_PIXEL_FORMAT_RGB_565X:
		data->bytes_per_pixel = 2U;
		break;
	case PANEL_PIXEL_FORMAT_L_8:
		data->bytes_per_pixel = 1U;
		break;
	default:
		LOG_ERR("Unsupported pixel format 0x%x", cfg->pixel_format);
		return -ENOTSUP;
	}

	data->fb_size = (size_t)cfg->width * cfg->height * data->bytes_per_pixel;
	data->fb_alloc_size = ROUND_UP(data->fb_size, ESP32_LCD_CAM_RGB_ALIGNMENT);

	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("Reset GPIO device not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
		k_msleep(10);
		gpio_pin_set_dt(&cfg->reset_gpio, 0);
		k_msleep(20);
	}

	if (cfg->disp_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->disp_gpio)) {
			LOG_ERR("DISP GPIO device not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->disp_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	if (cfg->backlight_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->backlight_gpio)) {
			LOG_ERR("Backlight GPIO device not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->backlight_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	/* Allocate full-screen framebuffers in external PSRAM */
	for (uint8_t i = 0; i < ESP32_LCD_CAM_RGB_FB_COUNT; i++) {
		data->fb[i] = shared_multi_heap_aligned_alloc(
			SMH_REG_ATTR_EXTERNAL, ESP32_LCD_CAM_RGB_ALIGNMENT, data->fb_alloc_size);
		if (data->fb[i] == NULL) {
			LOG_ERR("Failed to allocate PSRAM framebuffer %u (%zu bytes)", i,
				data->fb_alloc_size);
			for (uint8_t j = 0; j < i; j++) {
				shared_multi_heap_free(data->fb[j]);
				data->fb[j] = NULL;
			}
			return -ENOMEM;
		}

		memset(data->fb[i], 0, data->fb_alloc_size);
		esp32_lcd_cam_rgb_flush_cache(data->fb[i], data->fb_alloc_size);

#if ESP32_LCD_CAM_RGB_BB_LINES == 0
		data->fb_descs[i] = &data->fb_desc_pool[i * data->descs_per_fb];
		esp32_lcd_cam_rgb_build_desc_chain(data->fb_descs[i], data->descs_per_fb,
						   data->fb[i], data->fb_size,
						   &data->fb_descs[i][0], true);
#endif
	}

#if ESP32_LCD_CAM_RGB_BB_LINES > 0
	data->bb_size = (size_t)cfg->width * ESP32_LCD_CAM_RGB_BB_LINES * data->bytes_per_pixel;
	memset(data->bb[0], 0, data->bb_size);
	memset(data->bb[1], 0, data->bb_size);

	esp32_lcd_cam_rgb_build_desc_chain(data->bb_descs[0], data->descs_per_bb, data->bb[0],
					   data->bb_size, &data->bb_descs[1][0], true);
	esp32_lcd_cam_rgb_build_desc_chain(data->bb_descs[1], data->descs_per_bb, data->bb[1],
					   data->bb_size, &data->bb_descs[0][0], true);
	data->bb_read_offset = 0U;
	esp32_lcd_cam_rgb_fill_bb(data, 0U);
	esp32_lcd_cam_rgb_fill_bb(data, 1U);
#endif

	/* Initialize GDMA peripheral clock if not already running */
	if (!device_is_ready(cfg->dma_clock_dev)) {
		LOG_ERR("DMA clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_status(cfg->dma_clock_dev, cfg->dma_clock_subsys) !=
	    CLOCK_CONTROL_STATUS_ON) {
		ret = clock_control_on(cfg->dma_clock_dev, cfg->dma_clock_subsys);
		if (ret < 0 && ret != -EALREADY) {
			LOG_ERR("Failed to enable GDMA clock (%d)", ret);
			return ret;
		}
		gdma_ll_reset_register(0);
		gdma_ll_force_enable_reg_clock(gdma, true);
	}

	gdma_ll_tx_stop(gdma, cfg->dma_channel);
	gdma_ll_tx_reset_channel(gdma, cfg->dma_channel);
	gdma_ll_tx_connect_to_periph(gdma, cfg->dma_channel, SOC_GDMA_TRIG_PERIPH_LCD0);
	gdma_ll_tx_enable_data_burst(gdma, cfg->dma_channel, true);
	gdma_ll_tx_enable_descriptor_burst(gdma, cfg->dma_channel, true);
	gdma_ll_tx_set_burst_size(gdma, cfg->dma_channel, GDMA_LL_MAX_BURST_SIZE_PSRAM);
	gdma_ll_tx_enable_owner_check(gdma, cfg->dma_channel, false);
	gdma_ll_tx_enable_auto_write_back(gdma, cfg->dma_channel, false);
	gdma_ll_tx_set_priority(gdma, cfg->dma_channel, GDMA_LL_CHANNEL_MAX_PRIORITY);
	gdma_ll_tx_enable_interrupt(gdma, cfg->dma_channel, GDMA_LL_TX_EVENT_MASK, false);
	gdma_ll_tx_clear_interrupt_status(gdma, cfg->dma_channel, GDMA_LL_TX_EVENT_MASK);

#if ESP32_LCD_CAM_RGB_BB_LINES > 0
	gdma_ll_tx_set_eof_mode(gdma, cfg->dma_channel, 1U);
	gdma_ll_tx_enable_interrupt(gdma, cfg->dma_channel, GDMA_LL_EVENT_TX_EOF, true);

	ret = esp_intr_alloc_intrstatus(
		ETS_DMA_OUT_CH0_INTR_SOURCE + cfg->dma_channel,
		ESP_PRIO_TO_FLAGS(cfg->irq_priority) | ESP_INT_FLAGS_CHECK(cfg->irq_flags),
		(uint32_t)gdma_ll_tx_get_interrupt_status_reg(gdma, cfg->dma_channel),
		GDMA_LL_EVENT_TX_EOF, (intr_handler_t)esp32_lcd_cam_rgb_dma_isr, (void *)dev,
		&data->dma_intr);
	if (ret == -ENODEV) {
		ret = esp_intr_alloc_intrstatus(
			ETS_DMA_OUT_CH0_INTR_SOURCE + cfg->dma_channel,
			ESP_PRIO_TO_FLAGS(cfg->irq_priority) | ESP_INTR_FLAG_SHARED |
				ESP_INTR_FLAG_IRAM,
			(uint32_t)gdma_ll_tx_get_interrupt_status_reg(gdma, cfg->dma_channel),
			GDMA_LL_EVENT_TX_EOF, (intr_handler_t)esp32_lcd_cam_rgb_dma_isr,
			(void *)dev, &data->dma_intr);
	}
	if (ret != 0) {
		LOG_ERR("Failed to allocate GDMA TX interrupt (%d)", ret);
		return ret;
	}
#endif

	/* Initialize LCD_CAM peripheral in RGB mode */
	lcd_ll_enable_bus_clock(ESP32_LCD_CAM_RGB_BUS_ID, true);
	lcd_hal_init(hal, ESP32_LCD_CAM_RGB_BUS_ID);
	lcd_ll_enable_clock(hal->dev, true);

	ret = esp_clk_tree_src_get_freq_hz(
		cfg->clock_source, ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX, &clk_src_freq_hz);
	if (ret != ESP_OK) {
		LOG_ERR("Failed to query LCD clock source frequency (%d)", ret);
		return -EINVAL;
	}

	lcd_ll_select_clk_src(hal->dev, cfg->clock_source);
	actual_pclk_hz =
		lcd_hal_cal_pclk_freq(hal, clk_src_freq_hz, cfg->pclk_freq_hz, &lcd_clk_div);
	lcd_ll_set_group_clock_coeff(hal->dev, lcd_clk_div.integer, lcd_clk_div.denominator,
				     lcd_clk_div.numerator);

	lcd_ll_set_clock_idle_level(hal->dev, cfg->pclk_idle_high);
	lcd_ll_set_pixel_clock_edge(hal->dev, cfg->pixelclk_active);
	lcd_ll_enable_rgb_mode(hal->dev, true);
	lcd_ll_enable_auto_next_frame(hal->dev, true);
	lcd_ll_enable_output_hsync_in_porch_region(hal->dev, !cfg->de_mode_only);
	lcd_ll_set_hsync_position(hal->dev, 0U);
	lcd_ll_set_horizontal_timing(hal->dev, cfg->hsync_len, cfg->hback_porch, cfg->width,
				     cfg->hfront_porch);
	lcd_ll_set_vertical_timing(hal->dev, cfg->vsync_len, cfg->vback_porch, cfg->height,
				   cfg->vfront_porch);
	lcd_ll_set_idle_level(hal->dev, !cfg->hsync_active, !cfg->vsync_active, !cfg->de_active);
	lcd_ll_set_delay_ticks(hal->dev, 0U, 0U, 0U);
	lcd_ll_set_data_delay_ticks(hal->dev, 0U);
	lcd_ll_set_dma_read_stride(hal->dev, cfg->data_bus_width);
	lcd_ll_set_data_wire_width(hal->dev, cfg->data_bus_width);
	lcd_ll_set_phase_cycles(hal->dev, 0U, 0U, 1U);
	lcd_ll_set_blank_cycles(hal->dev, 1U, 1U);
	lcd_ll_enable_color_convert(hal->dev, false);
	lcd_ll_swap_dma_data_byte_order(hal->dev, cfg->invert_byte_order);
	lcd_ll_reverse_dma_data_bit_order(hal->dev, cfg->invert_bit_order);
	lcd_ll_enable_swizzle(hal->dev, false);

	lcd_ll_reset(hal->dev);
	lcd_ll_fifo_reset(hal->dev);

	/* Set up VSYNC end interrupt */
	lcd_ll_enable_interrupt(hal->dev, LCD_LL_EVENT_RGB, false);
	lcd_ll_clear_interrupt_status(hal->dev, LCD_LL_EVENT_RGB);

	ret = esp_intr_alloc_intrstatus(
		cfg->irq_source,
		ESP_PRIO_TO_FLAGS(cfg->irq_priority) | ESP_INT_FLAGS_CHECK(cfg->irq_flags),
		(uint32_t)lcd_ll_get_interrupt_status_reg(hal->dev), LCD_LL_EVENT_RGB,
		(intr_handler_t)esp32_lcd_cam_rgb_vsync_isr, (void *)dev, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to allocate LCD VSYNC interrupt (%d)", ret);
		return ret;
	}

#if ESP32_LCD_CAM_RGB_BB_LINES > 0
	gdma_ll_tx_set_desc_addr(gdma, cfg->dma_channel, (uint32_t)&data->bb_descs[0][0]);
#else
	gdma_ll_tx_set_desc_addr(gdma, cfg->dma_channel,
				 (uint32_t)&data->fb_descs[data->active_fb][0]);
#endif
	gdma_ll_tx_start(gdma, cfg->dma_channel);

	/* Allow GDMA to pre-fill the LCD async FIFO before starting dot-clock output */
	k_busy_wait(2);

	lcd_ll_clear_interrupt_status(hal->dev, LCD_LL_EVENT_RGB);
	lcd_ll_enable_interrupt(hal->dev, LCD_LL_EVENT_RGB, true);
	lcd_ll_start(hal->dev);

	if (cfg->disp_gpio.port != NULL) {
		(void)gpio_pin_set_dt(&cfg->disp_gpio, 1);
	}
	if (cfg->backlight_gpio.port != NULL) {
		(void)gpio_pin_set_dt(&cfg->backlight_gpio, 1);
	}

	LOG_INF("ESP32 LCD-CAM RGB initialized: %ux%u (%u FB in PSRAM, %u BB lines, PCLK %u Hz)",
		cfg->width, cfg->height, ESP32_LCD_CAM_RGB_FB_COUNT, ESP32_LCD_CAM_RGB_BB_LINES,
		actual_pclk_hz);

	return 0;
}

static DEVICE_API(display, esp32_lcd_cam_rgb_api) = {
	.blanking_on = esp32_lcd_cam_rgb_blanking_on,
	.blanking_off = esp32_lcd_cam_rgb_blanking_off,
	.write = esp32_lcd_cam_rgb_write,
	.read = esp32_lcd_cam_rgb_read,
	.get_framebuffer = esp32_lcd_cam_rgb_get_framebuffer,
	.get_capabilities = esp32_lcd_cam_rgb_get_capabilities,
	.set_pixel_format = esp32_lcd_cam_rgb_set_pixel_format,
	.set_orientation = esp32_lcd_cam_rgb_set_orientation,
	.register_event_cb = esp32_lcd_cam_rgb_register_event_cb,
	.unregister_event_cb = esp32_lcd_cam_rgb_unregister_event_cb,
};

#define ESP32_LCD_CAM_RGB_TIMINGS(inst) DT_INST_CHILD(inst, display_timings)

#define ESP32_LCD_CAM_RGB_BPP(inst)                                                                \
	((DT_INST_PROP(inst, pixel_format) == PANEL_PIXEL_FORMAT_L_8) ? 1U : 2U)

#define ESP32_LCD_CAM_RGB_FB_BYTES(inst)                                                           \
	(DT_INST_PROP(inst, width) * DT_INST_PROP(inst, height) * ESP32_LCD_CAM_RGB_BPP(inst))

#define ESP32_LCD_CAM_RGB_FB_DESCS_NUM(inst)                                                       \
	DIV_ROUND_UP(ESP32_LCD_CAM_RGB_FB_BYTES(inst), ESP32_LCD_CAM_RGB_DMA_CHUNK_SIZE)

#define ESP32_LCD_CAM_RGB_BB_BYTES(inst)                                                           \
	(DT_INST_PROP(inst, width) * ESP32_LCD_CAM_RGB_BB_LINES * ESP32_LCD_CAM_RGB_BPP(inst))

#define ESP32_LCD_CAM_RGB_BB_DESCS_NUM(inst)                                                       \
	DIV_ROUND_UP(MAX(ESP32_LCD_CAM_RGB_BB_BYTES(inst), 1U), ESP32_LCD_CAM_RGB_DMA_CHUNK_SIZE)

#if ESP32_LCD_CAM_RGB_BB_LINES > 0
#define ESP32_LCD_CAM_RGB_DEFINE_DESCS(inst)                                                       \
	static uint8_t esp32_rgb_bb0_buf_##inst[ESP32_LCD_CAM_RGB_BB_BYTES(inst)]                  \
		__aligned(ESP32_LCD_CAM_RGB_ALIGNMENT);                                            \
	static uint8_t esp32_rgb_bb1_buf_##inst[ESP32_LCD_CAM_RGB_BB_BYTES(inst)]                  \
		__aligned(ESP32_LCD_CAM_RGB_ALIGNMENT);                                            \
	static dma_descriptor_t esp32_rgb_bb0_descs_##inst[ESP32_LCD_CAM_RGB_BB_DESCS_NUM(inst)]   \
		__aligned(4);                                                                      \
	static dma_descriptor_t esp32_rgb_bb1_descs_##inst[ESP32_LCD_CAM_RGB_BB_DESCS_NUM(inst)]   \
		__aligned(4);
#define ESP32_LCD_CAM_RGB_DATA_DESC_INIT(inst)                                                     \
	.bb = {esp32_rgb_bb0_buf_##inst, esp32_rgb_bb1_buf_##inst},                                \
	.bb_descs = {esp32_rgb_bb0_descs_##inst, esp32_rgb_bb1_descs_##inst},                      \
	.descs_per_bb = ESP32_LCD_CAM_RGB_BB_DESCS_NUM(inst),
#else
#define ESP32_LCD_CAM_RGB_DEFINE_DESCS(inst)                                                       \
	static dma_descriptor_t esp32_rgb_fb_descs_##inst[ESP32_LCD_CAM_RGB_FB_COUNT *             \
							  ESP32_LCD_CAM_RGB_FB_DESCS_NUM(inst)]    \
		__aligned(4);
#define ESP32_LCD_CAM_RGB_DATA_DESC_INIT(inst)                                                     \
	.fb_desc_pool = esp32_rgb_fb_descs_##inst,                                                 \
	.descs_per_fb = ESP32_LCD_CAM_RGB_FB_DESCS_NUM(inst),
#endif

#define ESP32_LCD_CAM_RGB_INIT(inst)                                                               \
	ESP32_LCD_CAM_RGB_DEFINE_DESCS(inst)                                                       \
	static struct esp32_lcd_cam_rgb_data esp32_lcd_cam_rgb_data_##inst = {                     \
		ESP32_LCD_CAM_RGB_DATA_DESC_INIT(inst)};                                           \
	static const struct esp32_lcd_cam_rgb_config esp32_lcd_cam_rgb_config_##inst = {           \
		.dma_clock_dev = DEVICE_DT_GET(                                                    \
			DT_CLOCKS_CTLR(DT_DMAS_CTLR_BY_NAME(DT_INST_PARENT(inst), tx))),           \
		.dma_clock_subsys = (clock_control_subsys_t)DT_CLOCKS_CELL(                        \
			DT_DMAS_CTLR_BY_NAME(DT_INST_PARENT(inst), tx), offset),                   \
		.clock_source = LCD_CLK_SRC_DEFAULT,                                               \
		.irq_source = DT_IRQ_BY_IDX(DT_INST_PARENT(inst), 0, irq),                         \
		.irq_priority = DT_IRQ_BY_IDX(DT_INST_PARENT(inst), 0, priority),                  \
		.irq_flags = DT_IRQ_BY_IDX(DT_INST_PARENT(inst), 0, flags),                        \
		.dma_channel = DT_DMAS_CELL_BY_NAME(DT_INST_PARENT(inst), tx, channel) / 2U,       \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
		.pixel_format = DT_INST_PROP(inst, pixel_format),                                  \
		.data_bus_width = DT_INST_PROP_OR(inst, data_bus_width, 16),                       \
		.pclk_freq_hz = DT_PROP(ESP32_LCD_CAM_RGB_TIMINGS(inst), clock_frequency),         \
		.hsync_len = DT_PROP(ESP32_LCD_CAM_RGB_TIMINGS(inst), hsync_len),                  \
		.hback_porch = DT_PROP(ESP32_LCD_CAM_RGB_TIMINGS(inst), hback_porch),              \
		.hfront_porch = DT_PROP(ESP32_LCD_CAM_RGB_TIMINGS(inst), hfront_porch),            \
		.vsync_len = DT_PROP(ESP32_LCD_CAM_RGB_TIMINGS(inst), vsync_len),                  \
		.vback_porch = DT_PROP(ESP32_LCD_CAM_RGB_TIMINGS(inst), vback_porch),              \
		.vfront_porch = DT_PROP(ESP32_LCD_CAM_RGB_TIMINGS(inst), vfront_porch),            \
		.hsync_active = DT_PROP(ESP32_LCD_CAM_RGB_TIMINGS(inst), hsync_active),            \
		.vsync_active = DT_PROP(ESP32_LCD_CAM_RGB_TIMINGS(inst), vsync_active),            \
		.de_active = DT_PROP(ESP32_LCD_CAM_RGB_TIMINGS(inst), de_active),                  \
		.pixelclk_active = DT_PROP(ESP32_LCD_CAM_RGB_TIMINGS(inst), pixelclk_active),      \
		.pclk_idle_high = DT_INST_PROP(inst, pclk_idle_high),                              \
		.de_mode_only = DT_INST_PROP(inst, de_mode_only),                                  \
		.invert_byte_order = DT_INST_PROP(inst, invert_byte_order),                        \
		.invert_bit_order = DT_INST_PROP(inst, invert_bit_order),                          \
		.backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, backlight_gpios, {0}),            \
		.disp_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, disp_gpios, {0}),                      \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, esp32_lcd_cam_rgb_init, NULL, &esp32_lcd_cam_rgb_data_##inst,  \
			      &esp32_lcd_cam_rgb_config_##inst, POST_KERNEL,                       \
			      CONFIG_DISPLAY_INIT_PRIORITY, &esp32_lcd_cam_rgb_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_LCD_CAM_RGB_INIT)
