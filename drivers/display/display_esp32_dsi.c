/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp_dsi_display

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/cache.h>

#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <hal/mipi_dsi_brg_ll.h>
#include <hal/dw_gdma_ll.h>
#include <soc/interrupts.h>
#include <esp_intr_alloc.h>

#include "display_esp32_dsi.h"

LOG_MODULE_REGISTER(display_esp32_dsi, CONFIG_DISPLAY_LOG_LEVEL);

#define DISPLAY_ESP32_DSI_GDMA_CLK_TIMEOUT_US 1000

struct display_esp32_dsi_config {
	const struct device *panel;
	uint8_t dma_channel;
	uint8_t irq_source;
	uint8_t irq_priority;
	uint8_t irq_flags;
	uint16_t width;
	uint16_t height;
	uint8_t pixel_format;
};

struct display_esp32_dsi_data {
	const struct device *dev;
	uint8_t dma_channel;
	uint8_t *fb[CONFIG_DISPLAY_ESP32_DSI_FB_NUM];
	uint32_t fb_size;
	uint8_t fb_count;
	uint8_t bytes_per_pixel;
	uint8_t draw_fb;
	uint8_t last_fb;
	bool have_last_fb;
	bool fb_seeded;
	bool started;
	uint32_t writers;
	void *prev_buf;
	intr_handle_t dma_intr;
	struct k_spinlock lock;
	atomic_t dma_err_count;
	atomic_t frame_count;
	uint8_t active_fb;
	int8_t pending_fb;
	uint8_t *ext_fb;
	struct k_sem frame_sem;
	display_event_cb_t event_cb;
	void *event_user_data;
	uint32_t event_mask;
	dw_gdma_link_list_item_t lli[CONFIG_DISPLAY_ESP32_DSI_FB_NUM + 1]
		__aligned(DW_GDMA_LL_LINK_LIST_ALIGNMENT);
};

static int display_esp32_dsi_writer_enter(struct display_esp32_dsi_data *data)
{
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (!data->started) {
		k_spin_unlock(&data->lock, key);
		return -EAGAIN;
	}

	data->writers++;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static void display_esp32_dsi_writer_exit(struct display_esp32_dsi_data *data)
{
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->writers--;
	k_spin_unlock(&data->lock, key);

	k_sem_give(&data->frame_sem);
}

static int display_esp32_dsi_flip(struct display_esp32_dsi_data *data, uint32_t index)
{
	k_spinlock_key_t key;

	if (index >= data->fb_count) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	data->pending_fb = (int8_t)index;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int8_t display_esp32_dsi_index_of(struct display_esp32_dsi_data *data, const void *buf)
{
	for (uint8_t i = 0; i < data->fb_count; i++) {
		if (buf == data->fb[i]) {
			return (int8_t)i;
		}
	}

	return -1;
}

static void display_esp32_dsi_present(struct display_esp32_dsi_data *data, int8_t index)
{
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->pending_fb = index;
	k_spin_unlock(&data->lock, key);
}

static int display_esp32_dsi_wait_buffer_free(struct display_esp32_dsi_data *data, const void *buf,
					      k_timeout_t timeout)
{
	int8_t index = -1;

	for (uint8_t i = 0; i < data->fb_count; i++) {
		if (buf == data->fb[i]) {
			index = (int8_t)i;
			break;
		}
	}

	if (index < 0 && buf == data->ext_fb) {
		index = (int8_t)data->fb_count;
	}

	if (index < 0) {
		return -EINVAL;
	}

	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (true) {
		k_spinlock_key_t key = k_spin_lock(&data->lock);
		bool busy = (index == (int8_t)data->active_fb) || (index == data->pending_fb);

		k_spin_unlock(&data->lock, key);

		if (!busy) {
			break;
		}

		/* A give from an earlier frame only costs an extra iteration,
		 * since the locked check above decides when the buffer is free.
		 */
		if (k_sem_take(&data->frame_sem, sys_timepoint_timeout(end)) != 0 &&
		    sys_timepoint_expired(end)) {
			return -EAGAIN;
		}
	}

	return 0;
}

static void display_esp32_dsi_dma_isr(void *arg)
{
	struct display_esp32_dsi_data *data = arg;
	dw_gdma_dev_t *dma = DW_GDMA_LL_GET_HW(0);
	uint8_t ch = data->dma_channel;

	uint32_t status = dw_gdma_ll_channel_get_intr_status(dma, ch);

	dw_gdma_ll_channel_clear_intr(dma, ch, status);

	if (status & DW_GDMA_LL_CHANNEL_EVENT_DMA_TFR_DONE) {
		k_spinlock_key_t key = k_spin_lock(&data->lock);
		display_event_cb_t cb = data->event_cb;
		uint32_t mask = data->event_mask;
		void *user_data = data->event_user_data;
		int8_t pending = data->pending_fb;
		uint8_t shown = data->active_fb;
		bool started = data->started;
		bool skip_flip = false;

		k_spin_unlock(&data->lock, key);

		if (!started) {
			return;
		}

		if (cb != NULL) {
			struct display_event_data evt = {
				.info = {.buffer_id = (int)shown},
			};

			if (mask & DISPLAY_EVENT_VSYNC) {
				(void)cb(data->dev, DISPLAY_EVENT_VSYNC, &evt, user_data);
			}
			/* Only frame done gates the flip, since presenting the
			 * pending buffer is what this driver does by default for
			 * that event.
			 */
			if (mask & DISPLAY_EVENT_FRAME_DONE) {
				skip_flip = cb(data->dev, DISPLAY_EVENT_FRAME_DONE, &evt,
					       user_data) == DISPLAY_EVENT_RESULT_HANDLED;
			}
		}

		key = k_spin_lock(&data->lock);
		if (!skip_flip && pending >= 0 && data->pending_fb == pending) {
			data->active_fb = (uint8_t)pending;
			data->pending_fb = -1;
		}
		shown = data->active_fb;
		k_spin_unlock(&data->lock, key);

		dw_gdma_link_list_item_t *lli = &data->lli[shown];

		dw_gdma_ll_lli_set_block_markers(lli, true, true, true);
		sys_cache_data_flush_range(lli, sizeof(*lli));
		dw_gdma_ll_channel_set_link_list_head_addr(dma, ch, (uint32_t)lli);
		dw_gdma_ll_channel_enable(dma, ch, true);

		atomic_inc(&data->frame_count);

		k_sem_give(&data->frame_sem);
	}

	if (status & DW_GDMA_LL_CHANNEL_EVENT_SHADOWREG_OR_LLI_INVALID_ERR) {
		atomic_inc(&data->dma_err_count);
	}
}

static int display_esp32_dsi_dma_setup(const struct device *dev)
{
	const struct display_esp32_dsi_config *config = dev->config;
	struct display_esp32_dsi_data *data = dev->data;
	dw_gdma_dev_t *dma = DW_GDMA_LL_GET_HW(0);
	uint8_t ch = data->dma_channel;

	dw_gdma_ll_enable_bus_clock(0, true);
	if (!WAIT_FOR(dw_gdma_ll_is_bus_clock_enabled(0), DISPLAY_ESP32_DSI_GDMA_CLK_TIMEOUT_US,
		      k_busy_wait(1))) {
		LOG_ERR("GDMA bus clock did not come up");
		return -ETIMEDOUT;
	}
	dw_gdma_ll_reset(dma);

	dw_gdma_ll_enable_controller(dma, true);
	dw_gdma_ll_enable_intr_global(dma, true);

	dw_gdma_ll_channel_enable(dma, ch, false);

	dw_gdma_ll_channel_set_trans_flow(dma, ch, DW_GDMA_ROLE_MEM, DW_GDMA_ROLE_PERIPH_DSI,
					  DW_GDMA_FLOW_CTRL_SELF);

	dw_gdma_ll_channel_set_src_multi_block_type(dma, ch, DW_GDMA_BLOCK_TRANSFER_LIST);
	dw_gdma_ll_channel_set_dst_multi_block_type(dma, ch, DW_GDMA_BLOCK_TRANSFER_LIST);

	dw_gdma_ll_channel_set_src_handshake_interface(dma, ch, DW_GDMA_HANDSHAKE_HW);
	dw_gdma_ll_channel_set_dst_handshake_interface(dma, ch, DW_GDMA_HANDSHAKE_HW);
	dw_gdma_ll_channel_set_dst_handshake_periph(dma, ch, DW_GDMA_ROLE_PERIPH_DSI);

	dw_gdma_ll_channel_set_src_outstanding_limit(dma, ch, 5);
	dw_gdma_ll_channel_set_dst_outstanding_limit(dma, ch, 2);

	dw_gdma_ll_channel_set_priority(dma, ch, 1);

	for (uint8_t i = 0; i < data->fb_count; i++) {
		dw_gdma_link_list_item_t *lli = &data->lli[i];

		memset(lli, 0, sizeof(*lli));

		dw_gdma_ll_lli_set_src_addr(lli, (uint32_t)data->fb[i]);
		dw_gdma_ll_lli_set_src_master_port(lli, (intptr_t)data->fb[i]);
		dw_gdma_ll_lli_set_src_burst_mode(lli, DW_GDMA_BURST_MODE_INCREMENT);
		dw_gdma_ll_lli_set_src_trans_width(lli, DW_GDMA_TRANS_WIDTH_64);
		dw_gdma_ll_lli_set_src_burst_items(lli, DW_GDMA_BURST_ITEMS_512);
		dw_gdma_ll_lli_set_src_burst_len(lli, 16);

		dw_gdma_ll_lli_set_dst_addr(lli, MIPI_DSI_BRG_MEM_BASE);
		dw_gdma_ll_lli_set_dst_master_port(lli, MIPI_DSI_BRG_MEM_BASE);
		dw_gdma_ll_lli_set_dst_burst_mode(lli, DW_GDMA_BURST_MODE_FIXED);
		dw_gdma_ll_lli_set_dst_trans_width(lli, DW_GDMA_TRANS_WIDTH_64);
		dw_gdma_ll_lli_set_dst_burst_items(lli, DW_GDMA_BURST_ITEMS_256);
		dw_gdma_ll_lli_set_dst_burst_len(lli, 16);

		dw_gdma_ll_lli_set_trans_block_size(lli, data->fb_size / 8);

		dw_gdma_ll_lli_set_block_markers(lli, true, true, true);
		dw_gdma_ll_lli_set_next_item_addr(lli, 0);
		dw_gdma_ll_lli_set_link_list_master_port(lli, DW_GDMA_LL_MASTER_PORT_MEMORY);
	}

	/* The spare item is re-pointed at a caller-owned frame when one is
	 * presented, so it starts as a copy of the first framebuffer's item.
	 */
	memcpy(&data->lli[data->fb_count], &data->lli[0], sizeof(data->lli[0]));

	sys_cache_data_flush_range(data->lli, sizeof(data->lli[0]) * (data->fb_count + 1));

	data->active_fb = 0;
	data->pending_fb = -1;
	data->ext_fb = NULL;

	dw_gdma_ll_channel_enable_intr_generation(dma, ch, UINT32_MAX, true);
	dw_gdma_ll_channel_enable_intr_propagation(
		dma, ch,
		DW_GDMA_LL_CHANNEL_EVENT_DMA_TFR_DONE |
			DW_GDMA_LL_CHANNEL_EVENT_SHADOWREG_OR_LLI_INVALID_ERR,
		true);

	if (data->dma_intr == NULL) {
		int err = esp_intr_alloc_intrstatus(
			config->irq_source,
			ESP_INTR_FLAG_SHARED | ESP_PRIO_TO_FLAGS(config->irq_priority) |
				ESP_INT_FLAGS_CHECK(config->irq_flags),
			(uint32_t)dw_gdma_ll_get_intr_status_reg(dma),
			DW_GDMA_LL_CHANNEL_EVENT_MASK(ch), display_esp32_dsi_dma_isr, data,
			&data->dma_intr);
		if (err != 0) {
			LOG_ERR("Failed to allocate DMA interrupt (%d)", err);
			dw_gdma_ll_channel_enable(dma, ch, false);
			return -EIO;
		}
	}

	dw_gdma_ll_channel_set_link_list_head_addr(dma, ch, (uint32_t)&data->lli[data->active_fb]);
	dw_gdma_ll_channel_set_link_list_master_port(dma, ch, DW_GDMA_LL_MASTER_PORT_MEMORY);
	dw_gdma_ll_channel_enable(dma, ch, true);

	LOG_INF("DMA streaming started: %u fb(s), size=%u", data->fb_count, data->fb_size);

	return 0;
}

int display_esp32_dsi_start(const struct device *dev, uint32_t bits_per_pixel)
{
	struct display_esp32_dsi_data *data = dev->data;
	const struct display_esp32_dsi_config *config = dev->config;

	if (data->started) {
		return 0;
	}

	/* The panel reports the format it was attached with, while the
	 * capabilities come from this node. They describe the same pixels, so
	 * refuse to stream rather than stride the framebuffer one way and
	 * report it another.
	 */
	if (bits_per_pixel != DISPLAY_BITS_PER_PIXEL(config->pixel_format)) {
		LOG_ERR("Panel uses %u bpp, the controller is configured for %u", bits_per_pixel,
			DISPLAY_BITS_PER_PIXEL(config->pixel_format));
		return -EINVAL;
	}

	data->bytes_per_pixel = bits_per_pixel / 8;
	data->fb_size = config->width * config->height * data->bytes_per_pixel;
	data->fb_size = ROUND_UP(data->fb_size, CONFIG_ESP32_CACHE_L2_LINE_SIZE);
	data->fb_count = CONFIG_DISPLAY_ESP32_DSI_FB_NUM;

	for (uint8_t i = 0; i < data->fb_count; i++) {
		data->fb[i] = shared_multi_heap_aligned_alloc(
			SMH_REG_ATTR_EXTERNAL, CONFIG_ESP32_CACHE_L2_LINE_SIZE, data->fb_size);
		if (data->fb[i] == NULL) {
			LOG_ERR("Failed to allocate framebuffer %u (%u bytes)", i, data->fb_size);
			for (uint8_t j = 0; j < i; j++) {
				shared_multi_heap_free(data->fb[j]);
				data->fb[j] = NULL;
			}
			data->fb_count = 0;
			return -ENOMEM;
		}
		memset(data->fb[i], 0, data->fb_size);
		sys_cache_data_flush_range(data->fb[i], data->fb_size);
	}

	int err = display_esp32_dsi_dma_setup(dev);

	if (err != 0) {
		for (uint8_t i = 0; i < data->fb_count; i++) {
			shared_multi_heap_free(data->fb[i]);
			data->fb[i] = NULL;
		}
		data->fb_count = 0;
		return err;
	}

	data->draw_fb = (data->fb_count > 1) ? 1 : 0;
	data->started = true;

	return 0;
}

int display_esp32_dsi_stop(const struct device *dev)
{
	struct display_esp32_dsi_data *data = dev->data;
	dw_gdma_dev_t *dma = DW_GDMA_LL_GET_HW(0);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (!data->started) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	/* Retire the device before the hardware is touched, so a caller that
	 * has not entered yet is turned away and the handler returns early if
	 * it runs on the other core.
	 */
	data->started = false;
	data->event_cb = NULL;
	data->event_mask = 0;
	data->event_user_data = NULL;
	k_spin_unlock(&data->lock, key);

	/* Silence the source before the abort. Aborting only guarantees the
	 * channel is idle, not that a handler already latched on the other
	 * core has finished re-arming it.
	 */
	if (data->dma_intr != NULL) {
		esp_intr_disable(data->dma_intr);
	}

	dw_gdma_ll_channel_enable_intr_propagation(
		dma, data->dma_channel,
		DW_GDMA_LL_CHANNEL_EVENT_DMA_TFR_DONE |
			DW_GDMA_LL_CHANNEL_EVENT_SHADOWREG_OR_LLI_INVALID_ERR,
		false);
	dw_gdma_ll_channel_abort(dma, data->dma_channel);

	if (data->dma_intr != NULL) {
		esp_intr_free(data->dma_intr);
		data->dma_intr = NULL;
	}

	/* Release a writer blocked on a frame that will never be presented,
	 * then wait for every one of them to leave before the buffers they
	 * are still copying into are handed back to the heap.
	 */
	k_sem_give(&data->frame_sem);

	while (true) {
		key = k_spin_lock(&data->lock);
		bool busy = data->writers > 0;

		k_spin_unlock(&data->lock, key);

		if (!busy) {
			break;
		}

		k_sleep(K_MSEC(1));
	}

	for (uint8_t i = 0; i < data->fb_count; i++) {
		if (data->fb[i] != NULL) {
			shared_multi_heap_free(data->fb[i]);
			data->fb[i] = NULL;
		}
	}

	data->fb_count = 0;
	data->have_last_fb = false;
	data->fb_seeded = false;
	data->prev_buf = NULL;

	return 0;
}

static int display_esp32_dsi_write_locked(const struct device *dev, const uint16_t x,
					  const uint16_t y,
					  const struct display_buffer_descriptor *desc,
					  const void *buf)
{
	const struct display_esp32_dsi_config *config = dev->config;
	struct display_esp32_dsi_data *data = dev->data;
	uint16_t write_w = desc->width;
	uint16_t write_h = desc->height;
	uint8_t bpp = data->bytes_per_pixel;
	uint32_t src_pitch = desc->pitch * bpp;
	uint32_t dst_pitch = config->width * bpp;
	bool multi = data->fb_count > 1;

	if (buf == NULL) {
		return -EINVAL;
	}

	if ((uint32_t)x + write_w > config->width || (uint32_t)y + write_h > config->height) {
		LOG_ERR("Write %ux%u at (%u,%u) exceeds %ux%u panel", write_w, write_h, x, y,
			config->width, config->height);
		return -EINVAL;
	}

	if (desc->pitch < write_w) {
		LOG_ERR("Pitch %u is smaller than the width %u", desc->pitch, write_w);
		return -EINVAL;
	}

	if ((size_t)src_pitch * write_h > desc->buf_size) {
		LOG_ERR("Buffer of %u bytes is too small for %ux%u", desc->buf_size, write_w,
			write_h);
		return -EINVAL;
	}

	if (x == 0 && y == 0 && write_w == config->width && write_h == config->height &&
	    desc->pitch == config->width) {
		int8_t index = display_esp32_dsi_index_of(data, buf);

		/* A caller that renders into its own full-size frame can have it
		 * scanned out directly, by pointing the spare link list item at
		 * it instead of copying it into a driver framebuffer.
		 */
		if (index < 0 && desc->buf_size >= data->fb_size &&
		    ((uintptr_t)buf % CONFIG_ESP32_CACHE_L2_LINE_SIZE) == 0) {
			dw_gdma_link_list_item_t *lli = &data->lli[data->fb_count];

			sys_cache_data_flush_range((void *)buf, data->fb_size);

			dw_gdma_ll_lli_set_src_addr(lli, (uint32_t)buf);
			dw_gdma_ll_lli_set_src_master_port(lli, (intptr_t)buf);
			sys_cache_data_flush_range(lli, sizeof(*lli));

			data->ext_fb = (uint8_t *)buf;
			index = (int8_t)data->fb_count;
		}

		if (index >= 0) {
			/* Write the buffer back before it is published, so the
			 * frame the DMA picks up is never the stale one still
			 * sitting in the cache.
			 */
			sys_cache_data_flush_range((void *)buf, data->fb_size);
			display_esp32_dsi_present(data, index);

			if (multi && data->prev_buf != NULL && data->prev_buf != buf) {
				int err = display_esp32_dsi_wait_buffer_free(data, data->prev_buf,
									     K_MSEC(100));

				if (err != 0) {
					LOG_DBG("Timed out waiting for buffer %p", data->prev_buf);
					return err;
				}
			}
			data->prev_buf = (void *)buf;

			return 0;
		}
	}

	uint8_t *fb = data->fb[data->draw_fb];

	if (fb == NULL) {
		return -EAGAIN;
	}

	if (multi && !data->fb_seeded) {
		if (data->have_last_fb && data->last_fb != data->draw_fb) {
			memcpy(fb, data->fb[data->last_fb], data->fb_size);
			sys_cache_data_flush_range(fb, data->fb_size);
		}
		data->fb_seeded = true;
	}

	for (uint16_t row = 0; row < write_h; row++) {
		uint32_t dst_offset = (y + row) * dst_pitch + (uint32_t)x * bpp;
		uint32_t src_offset = row * src_pitch;

		memcpy(fb + dst_offset, (const uint8_t *)buf + src_offset, (size_t)write_w * bpp);
	}

	sys_cache_data_flush_range(fb + (uint32_t)y * dst_pitch, (size_t)write_h * dst_pitch);

	if (multi && !desc->frame_incomplete) {
		display_esp32_dsi_flip(data, data->draw_fb);
		data->last_fb = data->draw_fb;
		data->have_last_fb = true;
		data->draw_fb = (data->draw_fb + 1) % data->fb_count;
		data->fb_seeded = false;
	}

	return 0;
}

static int display_esp32_dsi_write(const struct device *dev, const uint16_t x, const uint16_t y,
				   const struct display_buffer_descriptor *desc, const void *buf)
{
	struct display_esp32_dsi_data *data = dev->data;
	int err = display_esp32_dsi_writer_enter(data);

	if (err != 0) {
		return err;
	}

	err = display_esp32_dsi_write_locked(dev, x, y, desc, buf);

	display_esp32_dsi_writer_exit(data);

	return err;
}

static int display_esp32_dsi_read_locked(const struct device *dev, const uint16_t x,
					 const uint16_t y,
					 const struct display_buffer_descriptor *desc, void *buf)
{
	const struct display_esp32_dsi_config *config = dev->config;
	struct display_esp32_dsi_data *data = dev->data;
	uint8_t bpp = data->bytes_per_pixel;
	uint32_t src_pitch = config->width * bpp;
	uint32_t dst_pitch = desc->pitch * bpp;
	k_spinlock_key_t key;
	const uint8_t *fb;

	if (buf == NULL) {
		return -EINVAL;
	}

	if ((uint32_t)x + desc->width > config->width ||
	    (uint32_t)y + desc->height > config->height) {
		LOG_ERR("Read %ux%u at (%u,%u) exceeds %ux%u panel", desc->width, desc->height, x,
			y, config->width, config->height);
		return -EINVAL;
	}

	if (desc->pitch < desc->width) {
		LOG_ERR("Pitch %u is smaller than the width %u", desc->pitch, desc->width);
		return -EINVAL;
	}

	if ((size_t)dst_pitch * desc->height > desc->buf_size) {
		LOG_ERR("Buffer of %u bytes is too small for %ux%u", desc->buf_size, desc->width,
			desc->height);
		return -EINVAL;
	}

	/* Read back what is on screen rather than the buffer being drawn. The
	 * frame on screen may be a caller-owned one, which lives outside the
	 * driver's framebuffer array.
	 */
	key = k_spin_lock(&data->lock);
	fb = (data->active_fb < data->fb_count) ? data->fb[data->active_fb] : data->ext_fb;
	k_spin_unlock(&data->lock, key);

	if (fb == NULL) {
		return -EAGAIN;
	}

	for (uint16_t row = 0; row < desc->height; row++) {
		memcpy((uint8_t *)buf + (size_t)row * dst_pitch,
		       fb + (size_t)(y + row) * src_pitch + (size_t)x * bpp,
		       (size_t)desc->width * bpp);
	}

	return 0;
}

static int display_esp32_dsi_read(const struct device *dev, const uint16_t x, const uint16_t y,
				  const struct display_buffer_descriptor *desc, void *buf)
{
	struct display_esp32_dsi_data *data = dev->data;
	int err = display_esp32_dsi_writer_enter(data);

	if (err != 0) {
		return err;
	}

	err = display_esp32_dsi_read_locked(dev, x, y, desc, buf);

	display_esp32_dsi_writer_exit(data);

	return err;
}

static int display_esp32_dsi_blanking_on(const struct device *dev)
{
	const struct display_esp32_dsi_config *config = dev->config;

	if (config->panel == NULL) {
		return -ENOSYS;
	}

	if (!device_is_ready(config->panel)) {
		return -ENODEV;
	}

	return display_blanking_on(config->panel);
}

static int display_esp32_dsi_blanking_off(const struct device *dev)
{
	const struct display_esp32_dsi_config *config = dev->config;

	if (config->panel == NULL) {
		return -ENOSYS;
	}

	if (!device_is_ready(config->panel)) {
		return -ENODEV;
	}

	return display_blanking_off(config->panel);
}

static void *display_esp32_dsi_get_framebuffer(const struct device *dev)
{
	struct display_esp32_dsi_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	void *fb = data->started ? data->fb[data->draw_fb] : NULL;

	k_spin_unlock(&data->lock, key);

	return fb;
}

void *display_esp32_dsi_get_framebuffer_by_index(const struct device *dev, uint32_t index)
{
	struct display_esp32_dsi_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	void *fb = (index < data->fb_count) ? data->fb[index] : NULL;

	k_spin_unlock(&data->lock, key);

	return fb;
}

static void display_esp32_dsi_get_capabilities(const struct device *dev,
					       struct display_capabilities *capabilities)
{
	const struct display_esp32_dsi_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->supported_pixel_formats = config->pixel_format;
	capabilities->current_pixel_format = config->pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int display_esp32_dsi_set_pixel_format(const struct device *dev,
					      const enum display_pixel_format pixel_format)
{
	const struct display_esp32_dsi_config *config = dev->config;

	if (pixel_format == config->pixel_format) {
		return 0;
	}

	return -ENOTSUP;
}

static int display_esp32_dsi_set_orientation(const struct device *dev,
					     const enum display_orientation orientation)
{
	const struct display_esp32_dsi_config *config = dev->config;

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}

	/* The scanout streams the framebuffer as it is laid out, so a rotation
	 * can only be done by a panel that supports one itself.
	 */
	if (config->panel == NULL) {
		return -ENOTSUP;
	}

	if (!device_is_ready(config->panel)) {
		return -ENODEV;
	}

	return display_set_orientation(config->panel, orientation);
}

static int display_esp32_dsi_register_event_cb(const struct device *dev, display_event_cb_t cb,
					       void *user_data, uint32_t event_mask, bool in_isr,
					       uint32_t *out_reg_handle)
{
	struct display_esp32_dsi_data *data = dev->data;
	k_spinlock_key_t key;

	if (out_reg_handle == NULL) {
		return -EINVAL;
	}
	if (!in_isr) {
		return -ENOTSUP;
	}
	if (event_mask & ~(DISPLAY_EVENT_VSYNC | DISPLAY_EVENT_FRAME_DONE)) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);
	if (data->event_cb != NULL) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	data->event_user_data = user_data;
	data->event_mask = event_mask;
	data->event_cb = cb;
	k_spin_unlock(&data->lock, key);

	*out_reg_handle = 1;

	return 0;
}

static int display_esp32_dsi_unregister_event_cb(const struct device *dev, uint32_t reg_handle)
{
	struct display_esp32_dsi_data *data = dev->data;
	k_spinlock_key_t key;

	if (reg_handle != 1) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (data->event_cb == NULL) {
		k_spin_unlock(&data->lock, key);
		return -EPERM;
	}
	data->event_cb = NULL;
	data->event_mask = 0;
	data->event_user_data = NULL;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static DEVICE_API(display, display_esp32_dsi_api) = {
	.blanking_on = display_esp32_dsi_blanking_on,
	.blanking_off = display_esp32_dsi_blanking_off,
	.write = display_esp32_dsi_write,
	.read = display_esp32_dsi_read,
	.get_framebuffer = display_esp32_dsi_get_framebuffer,
	.get_capabilities = display_esp32_dsi_get_capabilities,
	.set_pixel_format = display_esp32_dsi_set_pixel_format,
	.set_orientation = display_esp32_dsi_set_orientation,
	.register_event_cb = display_esp32_dsi_register_event_cb,
	.unregister_event_cb = display_esp32_dsi_unregister_event_cb,
};

static int display_esp32_dsi_init(const struct device *dev)
{
	const struct display_esp32_dsi_config *config = dev->config;
	struct display_esp32_dsi_data *data = dev->data;

	data->dev = dev;
	data->dma_channel = config->dma_channel;
	data->pending_fb = -1;
	k_sem_init(&data->frame_sem, 0, K_SEM_MAX_LIMIT);

	return 0;
}

/* The panel sits on the DSI host this controller owns. It is resolved here
 * only so blanking can be forwarded to it; the panel still initializes after
 * this controller, which starts the scanout.
 */
#define DISPLAY_ESP32_DSI_PANEL_OF(node) DEVICE_DT_GET(node),

/* The panels hang off the DSI host, which is this controller's child, so they
 * are reached one level below rather than as siblings. The host is matched by
 * compatible so the lookup does not depend on a board node label.
 */
#define DISPLAY_ESP32_DSI_HOST_PANELS(node, fn)                                                    \
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, espressif_esp_mipi_dsi),                               \
		   (DT_FOREACH_CHILD_STATUS_OKAY(node, fn)))

#define DISPLAY_ESP32_DSI_PANEL(inst)                                                              \
	DT_FOREACH_CHILD_STATUS_OKAY_VARGS(DT_DRV_INST(inst), DISPLAY_ESP32_DSI_HOST_PANELS,       \
					   DISPLAY_ESP32_DSI_PANEL_OF)

#define DISPLAY_ESP32_DSI_DEVICE(inst)                                                             \
	static struct display_esp32_dsi_data display_esp32_dsi_data_##inst;                        \
	static const struct device *const display_esp32_dsi_panels_##inst[] = {                    \
		DISPLAY_ESP32_DSI_PANEL(inst)};                                                    \
	static const struct display_esp32_dsi_config display_esp32_dsi_config_##inst = {           \
		.dma_channel = DT_INST_PROP(inst, dma_channel),                                    \
		.irq_source = DT_INST_IRQ_BY_IDX(inst, 0, irq),                                    \
		.irq_priority = DT_INST_IRQ_BY_IDX(inst, 0, priority),                             \
		.irq_flags = DT_INST_IRQ_BY_IDX(inst, 0, flags),                                   \
		.panel = ARRAY_SIZE(display_esp32_dsi_panels_##inst)                               \
				 ? display_esp32_dsi_panels_##inst[0]                              \
				 : NULL,                                                           \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
		.pixel_format = DT_INST_PROP(inst, pixel_format),                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, display_esp32_dsi_init, NULL, &display_esp32_dsi_data_##inst,  \
			      &display_esp32_dsi_config_##inst, POST_KERNEL,                       \
			      CONFIG_DISPLAY_ESP32_DSI_INIT_PRIORITY, &display_esp32_dsi_api);

DT_INST_FOREACH_STATUS_OKAY(DISPLAY_ESP32_DSI_DEVICE)
