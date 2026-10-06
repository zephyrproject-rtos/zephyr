/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_dpi_display

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/cache.h>
#include <zephyr/sys/util.h>

#include <esp_clk_tree.h>
#include <esp_private/esp_clk_tree_common.h>
#include <esp_attr.h>
#include <esp_private/cache_utils.h>
#include <hal/dma_types.h>
#include <hal/lcd_hal.h>
#include <hal/lcd_ll.h>
#include <hal/hal_utils.h>
#include <soc/clk_tree_defs.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_dpi, CONFIG_DISPLAY_LOG_LEVEL);

#define LCD_BUS_ID 0
#define ESP32_DPI_CLK_SRC LCD_CLK_SRC_PLL160M
#define LCD_PERIPH_CLOCK_PRE_SCALE 2
#define DMA_DESC_CHUNK_SIZE DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED
#define FB_ALIGNMENT 64
#define ESP32_DPI_NUM_BOUNCE_BUFS 2

struct esp32_dpi_config {
	uint16_t width;
	uint16_t height;
	uint32_t pclk_hz;
	uint32_t hsync_len;
	uint32_t hback_porch;
	uint32_t hfront_porch;
	uint32_t vsync_len;
	uint32_t vback_porch;
	uint32_t vfront_porch;
	bool pclk_active_neg;
	uint8_t data_width;
	uint8_t dma_burst_size;
	const struct device *dma_dev;
	uint8_t tx_dma_channel;
	size_t bounce_buf_size;
	size_t bounce_stride;
	uint8_t *bounce_storage;
	const struct pinctrl_dev_config *pinctrl;
};

struct esp32_dpi_data {
	const struct device *dev;
	lcd_hal_context_t hal;
	uint8_t *frame_buffer;
	size_t frame_buffer_size;
	uint8_t *bounce_bufs[ESP32_DPI_NUM_BOUNCE_BUFS];
	size_t bounce_buf_size;
	size_t bounce_pos;
	uint8_t next_bb;
	atomic_t running;
	uint32_t src_clk_hz;
	enum display_pixel_format current_pixel_format;
	struct k_mutex lock;
};

static void esp32_dpi_fill_bounce_from_fb(struct esp32_dpi_data *data, uint8_t *bounce_buf)
{
	memcpy(bounce_buf, data->frame_buffer + data->bounce_pos, data->bounce_buf_size);
	sys_cache_data_flush_range(bounce_buf, data->bounce_buf_size);
}

static void IRAM_ATTR esp32_dpi_advance_bounce_pos(struct esp32_dpi_data *data)
{
	data->bounce_pos += data->bounce_buf_size;
	if (data->bounce_pos >= data->frame_buffer_size) {
		data->bounce_pos = 0;
	}
}

static void esp32_dpi_prefill_bounce_buffers(struct esp32_dpi_data *data)
{
	data->bounce_pos = 0;
	data->next_bb = 0;
	for (size_t i = 0; i < ESP32_DPI_NUM_BOUNCE_BUFS; i++) {
		esp32_dpi_fill_bounce_from_fb(data, data->bounce_bufs[i]);
		esp32_dpi_advance_bounce_pos(data);
	}
}

/*
 * Runs in the GDMA interrupt, which stays enabled while the cache is off for
 * a flash operation. Only touch DRAM state then, and skip the copy from the
 * PSRAM framebuffer while still keeping the position in step with the scan.
 */
static void IRAM_ATTR esp32_dpi_dma_done(const struct device *dma_dev, void *user_data,
					 uint32_t channel, int status)
{
	struct esp32_dpi_data *data = user_data;
	uint8_t *bounce_buf;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	if (atomic_get(&data->running) == 0 || status < 0) {
		return;
	}

	bounce_buf = data->bounce_bufs[data->next_bb];
	data->next_bb = (data->next_bb + 1U) % ESP32_DPI_NUM_BOUNCE_BUFS;

	if (spi_flash_cache_enabled()) {
		esp32_dpi_fill_bounce_from_fb(data, bounce_buf);
	}

	esp32_dpi_advance_bounce_pos(data);
}

static void esp32_dpi_flush_fb_region(uint8_t *fb, size_t fb_size, uint16_t width,
				   uint16_t x, uint16_t y, size_t line_len, uint16_t height)
{
	size_t line_size = width * 2U;
	uintptr_t start = (uintptr_t)(fb + y * line_size + x * 2U);
	uintptr_t end = start + line_len * height;
	size_t line_bytes = sys_cache_data_line_size_get();
	uintptr_t flush_start;
	size_t flush_size;

	if (line_bytes == 0) {
		sys_cache_data_flush_range(fb, fb_size);
		return;
	}

	flush_start = start & ~(line_bytes - 1);
	end = ROUND_UP(end, line_bytes);

	if (end > (uintptr_t)(fb + fb_size)) {
		end = (uintptr_t)(fb + fb_size);
	}

	flush_size = end - flush_start;
	if (flush_size > 0) {
		sys_cache_data_flush_range((void *)flush_start, flush_size);
	}
}

static int esp32_dpi_dma_start(const struct esp32_dpi_config *cfg, struct esp32_dpi_data *data)
{
	struct dma_block_config blk[ESP32_DPI_NUM_BOUNCE_BUFS] = {0};
	struct dma_config dma_cfg = {0};
	int ret;

	for (size_t i = 0; i < ESP32_DPI_NUM_BOUNCE_BUFS; i++) {
		blk[i].source_address = (uint32_t)data->bounce_bufs[i];
		blk[i].block_size = data->bounce_buf_size;
		if (i + 1 < ESP32_DPI_NUM_BOUNCE_BUFS) {
			blk[i].next_block = &blk[i + 1];
		}
	}

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.dma_slot = ESP_GDMA_TRIG_PERIPH_LCD0;
	dma_cfg.cyclic = 1;
	dma_cfg.block_count = ESP32_DPI_NUM_BOUNCE_BUFS;
	dma_cfg.head_block = blk;
	dma_cfg.source_burst_length = cfg->dma_burst_size;
	dma_cfg.dest_burst_length = cfg->dma_burst_size;
	dma_cfg.dma_callback = esp32_dpi_dma_done;
	dma_cfg.user_data = data;

	ret = dma_config(cfg->dma_dev, cfg->tx_dma_channel, &dma_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure DMA channel %u (%d)", cfg->tx_dma_channel, ret);
		return ret;
	}

	ret = dma_start(cfg->dma_dev, cfg->tx_dma_channel);
	if (ret < 0) {
		LOG_ERR("Failed to start DMA channel %u (%d)", cfg->tx_dma_channel, ret);
		return ret;
	}

	return 0;
}

static int esp32_dpi_start_transmission(const struct esp32_dpi_config *cfg,
					struct esp32_dpi_data *data)
{
	lcd_hal_context_t *hal = &data->hal;
	int ret;

	atomic_set(&data->running, 0);
	(void)dma_stop(cfg->dma_dev, cfg->tx_dma_channel);

	lcd_ll_stop(hal->dev);
	lcd_ll_reset(hal->dev);
	lcd_ll_fifo_reset(hal->dev);

	esp32_dpi_prefill_bounce_buffers(data);

	ret = esp32_dpi_dma_start(cfg, data);
	if (ret < 0) {
		return ret;
	}

	atomic_set(&data->running, 1);

	k_busy_wait(1);
	lcd_ll_start(hal->dev);

	return 0;
}

static int esp32_dpi_configure_lcd(const struct device *dev)
{
	const struct esp32_dpi_config *cfg = dev->config;
	struct esp32_dpi_data *data = dev->data;
	lcd_hal_context_t *hal = &data->hal;
	hal_utils_clk_div_t lcd_clk_div = {};
	uint32_t active_width;
	uint32_t pclk_hz;

	if (esp_clk_tree_enable_src(ESP32_DPI_CLK_SRC, true) != ESP_OK) {
		return -EIO;
	}

	data->src_clk_hz = 0;
	if (esp_clk_tree_src_get_freq_hz(ESP32_DPI_CLK_SRC, ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX,
				       &data->src_clk_hz) != ESP_OK) {
		return -EINVAL;
	}

	lcd_ll_select_clk_src(hal->dev, ESP32_DPI_CLK_SRC);
	lcd_ll_set_group_clock_coeff(hal->dev, LCD_PERIPH_CLOCK_PRE_SCALE, 0, 0);

	data->src_clk_hz /= LCD_PERIPH_CLOCK_PRE_SCALE;
	pclk_hz = lcd_hal_cal_pclk_freq(hal, data->src_clk_hz, cfg->pclk_hz, &lcd_clk_div);
	LOG_INF("Pixel clock %u Hz (requested %u Hz)", pclk_hz, cfg->pclk_hz);
	lcd_ll_set_group_clock_coeff(hal->dev, lcd_clk_div.integer, lcd_clk_div.denominator,
				     lcd_clk_div.numerator);

	lcd_ll_set_clock_idle_level(hal->dev, false);
	lcd_ll_set_pixel_clock_edge(hal->dev, cfg->pclk_active_neg);
	lcd_ll_enable_rgb_mode(hal->dev, true);
	lcd_ll_set_dma_read_stride(hal->dev, cfg->data_width);
	lcd_ll_set_data_wire_width(hal->dev, cfg->data_width);
	lcd_ll_enable_color_convert(hal->dev, false);
	lcd_ll_set_phase_cycles(hal->dev, 0, 0, 1);
	lcd_ll_enable_output_always_on(hal->dev, true);
	lcd_ll_set_idle_level(hal->dev, true, true, false);
	lcd_ll_set_blank_cycles(hal->dev, 1, 1);

	active_width = cfg->width * 16 / cfg->data_width;
	lcd_ll_set_horizontal_timing(hal->dev, cfg->hsync_len, cfg->hback_porch, active_width,
				     cfg->hfront_porch);
	lcd_ll_set_vertical_timing(hal->dev, cfg->vsync_len, cfg->vback_porch, cfg->height,
				   cfg->vfront_porch);
	lcd_ll_enable_output_hsync_in_porch_region(hal->dev, true);
	lcd_ll_set_hsync_position(hal->dev, 0);
	lcd_ll_enable_auto_next_frame(hal->dev, true);

	return 0;
}

static int esp32_dpi_write(const struct device *dev, const uint16_t x, const uint16_t y,
		      const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct esp32_dpi_config *cfg = dev->config;
	struct esp32_dpi_data *data = dev->data;
	uint8_t *fb = data->frame_buffer;
	const uint8_t *src = buf;
	size_t line_len = desc->width * 2U;
	size_t row;

	if (desc->pitch != desc->width || desc->width > cfg->width || desc->height > cfg->height ||
	    x + desc->width > cfg->width || y + desc->height > cfg->height) {
		return -EINVAL;
	}

	if (desc->buf_size < (size_t)desc->pitch * desc->height * 2U) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	for (row = 0; row < desc->height; row++) {
		memcpy(fb + ((y + row) * cfg->width + x) * 2U, src, line_len);
		src += line_len;
	}

	esp32_dpi_flush_fb_region(fb, data->frame_buffer_size, cfg->width, x, y, line_len,
			       desc->height);
	k_mutex_unlock(&data->lock);

	return 0;
}

static void *esp32_dpi_get_framebuffer(const struct device *dev)
{
	struct esp32_dpi_data *data = dev->data;

	return data->frame_buffer;
}

static int esp32_dpi_blanking_on(const struct device *dev)
{
	struct esp32_dpi_data *data = dev->data;
	const struct esp32_dpi_config *cfg = dev->config;

	atomic_set(&data->running, 0);
	(void)dma_stop(cfg->dma_dev, cfg->tx_dma_channel);
	lcd_ll_stop(data->hal.dev);

	return 0;
}

static int esp32_dpi_blanking_off(const struct device *dev)
{
	return esp32_dpi_start_transmission(dev->config, dev->data);
}

static void esp32_dpi_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct esp32_dpi_config *cfg = dev->config;
	struct esp32_dpi_data *data = dev->data;

	memset(caps, 0, sizeof(*caps));
	caps->x_resolution = cfg->width;
	caps->y_resolution = cfg->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	caps->current_pixel_format = data->current_pixel_format;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int esp32_dpi_set_pixel_format(const struct device *dev, const enum display_pixel_format fmt)
{
	struct esp32_dpi_data *data = dev->data;

	if (fmt != PIXEL_FORMAT_RGB_565) {
		return -ENOTSUP;
	}

	data->current_pixel_format = fmt;

	return 0;
}

static int esp32_dpi_init(const struct device *dev)
{
	const struct esp32_dpi_config *cfg = dev->config;
	struct esp32_dpi_data *data = dev->data;
	int ret;

	if (cfg->dma_dev == NULL || !device_is_ready(cfg->dma_dev)) {
		LOG_ERR("DMA device not ready");
		return -ENODEV;
	}

	data->dev = dev;

	ret = pinctrl_apply_state(cfg->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	k_mutex_init(&data->lock);
	data->current_pixel_format = PIXEL_FORMAT_RGB_565;
	data->frame_buffer_size = cfg->width * cfg->height * 2U;

	data->frame_buffer = shared_multi_heap_aligned_alloc(SMH_REG_ATTR_EXTERNAL, FB_ALIGNMENT,
							 data->frame_buffer_size);
	if (data->frame_buffer == NULL) {
		LOG_ERR("Failed to allocate %zu byte framebuffer", data->frame_buffer_size);
		return -ENOMEM;
	}

	memset(data->frame_buffer, 0, data->frame_buffer_size);
	sys_cache_data_flush_range(data->frame_buffer, data->frame_buffer_size);

	data->bounce_buf_size = cfg->bounce_buf_size;
	for (size_t i = 0; i < ESP32_DPI_NUM_BOUNCE_BUFS; i++) {
		data->bounce_bufs[i] = cfg->bounce_storage + i * cfg->bounce_stride;
	}

	lcd_ll_enable_bus_clock(LCD_BUS_ID, true);
	lcd_ll_reset_register(LCD_BUS_ID);
	lcd_hal_init(&data->hal, LCD_BUS_ID);
	lcd_ll_enable_clock(data->hal.dev, true);
	lcd_ll_mem_set_low_power_mode(data->hal.dev, LCD_LL_MEM_LP_MODE_SHUT_DOWN);
	lcd_ll_mem_power_by_pmu(data->hal.dev);
	lcd_ll_enable_trans_buffer(data->hal.dev, true);
	lcd_ll_reset(data->hal.dev);
	lcd_ll_fifo_reset(data->hal.dev);

	ret = esp32_dpi_configure_lcd(dev);
	if (ret < 0) {
		goto err_free_fb;
	}

	ret = esp32_dpi_start_transmission(cfg, data);
	if (ret < 0) {
		goto err_free_fb;
	}

	return 0;

err_free_fb:
	shared_multi_heap_free(data->frame_buffer);
	data->frame_buffer = NULL;

	return ret;
}

static DEVICE_API(display, esp32_dpi_api) = {
	.blanking_on = esp32_dpi_blanking_on,
	.blanking_off = esp32_dpi_blanking_off,
	.write = esp32_dpi_write,
	.get_framebuffer = esp32_dpi_get_framebuffer,
	.get_capabilities = esp32_dpi_get_capabilities,
	.set_pixel_format = esp32_dpi_set_pixel_format,
};

#define ESP32_DPI_LCD_CAM_NODE(n) DT_INST_PARENT(n)

#define ESP32_DPI_BOUNCE_LINES CONFIG_DISPLAY_ESP32_DPI_BOUNCE_BUFFER_LINES

#define ESP32_DPI_BOUNCE_BUF_SIZE(n)                                                               \
	((size_t)(DT_INST_PROP(n, width) * ESP32_DPI_BOUNCE_LINES * 2U))

#define ESP32_DPI_BOUNCE_STRIDE(n) ROUND_UP(ESP32_DPI_BOUNCE_BUF_SIZE(n), FB_ALIGNMENT)

#define ESP32_DPI_DEFINE(n)                                                                        \
	BUILD_ASSERT((DT_INST_PROP(n, height) % ESP32_DPI_BOUNCE_LINES) == 0, \
		     "Panel height must be a multiple of the bounce buffer lines"); \
	BUILD_ASSERT(ESP32_DPI_NUM_BOUNCE_BUFS * \
		     DIV_ROUND_UP(ESP32_DPI_BOUNCE_BUF_SIZE(n), DMA_DESC_CHUNK_SIZE) <= \
		     CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM, \
		     "Bounce buffers exceed CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM"); \
	PINCTRL_DT_DEFINE(ESP32_DPI_LCD_CAM_NODE(n));                                              \
	static uint8_t esp32_dpi_bounce_##n[ESP32_DPI_NUM_BOUNCE_BUFS * \
					    ESP32_DPI_BOUNCE_STRIDE(n)] \
		__aligned(FB_ALIGNMENT); \
	static const struct esp32_dpi_config esp32_dpi_config_##n = {                              \
		.width = DT_INST_PROP(n, width),                                                   \
		.height = DT_INST_PROP(n, height),                                                 \
		.pclk_hz = DT_INST_PROP(n, pclk_frequency),                                        \
		.hsync_len = DT_INST_PROP(n, hsync_len),                                           \
		.hback_porch = DT_INST_PROP(n, hback_porch),                                       \
		.hfront_porch = DT_INST_PROP(n, hfront_porch),                                     \
		.vsync_len = DT_INST_PROP(n, vsync_len),                                           \
		.vback_porch = DT_INST_PROP(n, vback_porch),                                       \
		.vfront_porch = DT_INST_PROP(n, vfront_porch),                                     \
		.pclk_active_neg = DT_INST_PROP(n, pclk_active_negative),                          \
		.data_width = DT_INST_PROP_OR(n, data_width, 16),                                  \
		.dma_burst_size = DT_INST_PROP_OR(n, dma_burst_size, 64),                          \
		.dma_dev = DEVICE_DT_GET(DT_DMAS_CTLR_BY_NAME(ESP32_DPI_LCD_CAM_NODE(n), tx)),     \
		.tx_dma_channel = DT_DMAS_CELL_BY_NAME(ESP32_DPI_LCD_CAM_NODE(n), tx, channel),    \
		.bounce_buf_size = ESP32_DPI_BOUNCE_BUF_SIZE(n),                                   \
		.bounce_stride = ESP32_DPI_BOUNCE_STRIDE(n), \
		.bounce_storage = esp32_dpi_bounce_##n, \
		.pinctrl = PINCTRL_DT_DEV_CONFIG_GET(ESP32_DPI_LCD_CAM_NODE(n)),                   \
	};                                                                                         \
	static struct esp32_dpi_data esp32_dpi_data_##n;                                           \
	DEVICE_DT_INST_DEFINE(n, esp32_dpi_init, NULL, &esp32_dpi_data_##n, &esp32_dpi_config_##n, \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &esp32_dpi_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_DPI_DEFINE)
