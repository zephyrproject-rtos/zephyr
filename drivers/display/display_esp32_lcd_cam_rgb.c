/*
 * Copyright (c) 2026 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include <zephyr/sys/util.h>

#include <soc/interrupts.h>
#include <esp_attr.h>
#include <esp_system.h>
#include <hal/lcd_ll.h>
#include <hal/lcd_hal.h>
#include <hal/gdma_ll.h>
#include <hal/dma_types.h>
#include <esp_clk_tree.h>
#include <esp_rom_sys.h>
#include <esp_private/esp_clk_tree_common.h>

#define DT_DRV_COMPAT espressif_esp32_lcd_cam_rgb

LOG_MODULE_REGISTER(display_esp32_lcd_cam_rgb, CONFIG_DISPLAY_LOG_LEVEL);

#define ESP32_LCD_CAM_RGB_BUS_ID        0
/* Each bounce buffer is one DMA descriptor (<= 4095 bytes) and a multiple of the 64 byte burst */
#define ESP32_LCD_CAM_RGB_BB_CHUNK_MAX  4032U
#define ESP32_LCD_CAM_RGB_BB_CHUNK_MIN  256U
#define ESP32_LCD_CAM_RGB_BB_COUNT_MIN  4U
#define ESP32_LCD_CAM_RGB_BB_COUNT_MAX  8U
#define ESP32_LCD_CAM_RGB_BB_TOTAL_SIZE CONFIG_DISPLAY_ESP32_LCD_CAM_RGB_BOUNCE_BUFFER_SIZE

/* Restart skip bytes for the GDMA ring buffer - inherited from esp-idf */
#define ESP32_LCD_CAM_RGB_RESTART_SKIP_BYTES ((LCD_LL_FIFO_DEPTH + 1U) * 2U)

/* FIFO prime delay in microseconds - inherited from esp-idf */
#define ESP32_LCD_CAM_RGB_FIFO_PRIME_US 1U

#define ESP32_LCD_CAM_RGB_INTR_FLAGS                                                               \
	(ESP_INTR_FLAG_INTRDISABLED | ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_LOWMED |                \
	 ESP_INTR_FLAG_IRAM)

struct esp32_lcd_cam_rgb_timing {
	uint16_t width;
	uint16_t height;
	uint8_t data_bus_width;
	uint32_t clock_frequency;
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
};

struct esp32_lcd_cam_rgb_ctx {
	lcd_cam_dev_t *dev;
	const struct device *dma_dev;
	uint32_t dma_channel;
	intr_handle_t intr_handle;
	struct dma_block_config dma_blocks[ESP32_LCD_CAM_RGB_BB_COUNT_MAX];
	uint8_t *bb;
	size_t bb_capacity;
	size_t chunk_size;
	uint32_t chunk_count;
	uint32_t bb_count;
	uint32_t next_fill;
	uint32_t fill_buf;
	volatile uint32_t done_count;
	struct esp32_lcd_cam_rgb_timing timing;
	uint8_t *fb;
	size_t fb_capacity;
	size_t fb_size;
	size_t bytes_per_pixel;
	struct k_mutex lock;
	bool initialized;
	bool hw_ready;
};

struct esp32_lcd_cam_rgb_config {
	const struct device *panel;
	struct esp32_lcd_cam_rgb_timing timing;
};

static int esp32_lcd_cam_rgb_bytes_per_pixel(const struct esp32_lcd_cam_rgb_timing *timing)
{
	if (timing->data_bus_width <= 16U) {
		return 2;
	}

	if (timing->data_bus_width <= 24U) {
		return 3;
	}

	return -EINVAL;
}

static void IRAM_ATTR esp32_lcd_cam_rgb_prefill(struct esp32_lcd_cam_rgb_ctx *ctx)
{
	memcpy(ctx->bb, ctx->fb, ctx->bb_count * ctx->chunk_size);
	ctx->fill_buf = 0U;
	ctx->next_fill = ctx->bb_count % ctx->chunk_count;
	ctx->done_count = 0U;
}

static void IRAM_ATTR esp32_lcd_cam_rgb_dma_done(const struct device *dma_dev, void *user_data,
						 uint32_t channel, int status)
{
	const struct device *dev = user_data;
	struct esp32_lcd_cam_rgb_ctx *ctx = dev->data;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	if (status < 0) {
		return;
	}

	/* The buffer that just drained gets the chunk that is bb_count chunks ahead */
	memcpy(ctx->bb + ctx->fill_buf * ctx->chunk_size,
	       ctx->fb + ctx->next_fill * ctx->chunk_size, ctx->chunk_size);
	ctx->fill_buf = (ctx->fill_buf + 1U) % ctx->bb_count;
	ctx->next_fill = (ctx->next_fill + 1U) % ctx->chunk_count;
	ctx->done_count++;
}

static void IRAM_ATTR esp32_lcd_cam_rgb_vsync_isr(void *arg)
{
	const struct device *dev = arg;
	struct esp32_lcd_cam_rgb_ctx *ctx = dev->data;
	uint32_t st = lcd_ll_get_interrupt_status(ctx->dev);
	unsigned int key;

	lcd_ll_clear_interrupt_status(ctx->dev, st);

	if (!(st & LCD_LL_EVENT_RGB)) {
		return;
	}

	key = irq_lock();

	/* A whole frame must have drained exactly chunk_count buffers, else DMA and LCD are out of
	 * step
	 */
	if (ctx->done_count != ctx->chunk_count) {
		esp32_lcd_cam_rgb_prefill(ctx);
		lcd_ll_fifo_reset(ctx->dev);
		dma_esp32_gdma_restart_channel(ctx->dma_dev, ctx->dma_channel);
		gdma_ll_tx_clear_interrupt_status(GDMA_LL_GET_HW(0), ctx->dma_channel / 2U,
						  UINT32_MAX);
	}
	ctx->done_count = 0U;

	irq_unlock(key);
}

/* Buffer i must always hold chunk i mod count, so count has to divide the chunks per frame */
static int esp32_lcd_cam_rgb_pick_bounce(size_t fb_size, size_t *chunk_out, uint32_t *count_out)
{
	for (uint32_t count = ESP32_LCD_CAM_RGB_BB_COUNT_MIN;
	     count <= ESP32_LCD_CAM_RGB_BB_COUNT_MAX; count++) {
		size_t limit = MIN(ESP32_LCD_CAM_RGB_BB_CHUNK_MAX,
				   ESP32_LCD_CAM_RGB_BB_TOTAL_SIZE / count);

		for (size_t chunk = ROUND_DOWN(limit, 64U); chunk >= ESP32_LCD_CAM_RGB_BB_CHUNK_MIN;
		     chunk -= 64U) {
			if (((fb_size % chunk) == 0U) && (((fb_size / chunk) % count) == 0U)) {
				*chunk_out = chunk;
				*count_out = count;
				return 0;
			}
		}
	}

	return -EINVAL;
}

static int esp32_lcd_cam_rgb_configure_dma(const struct device *dev)
{
	struct esp32_lcd_cam_rgb_ctx *ctx = dev->data;
	struct dma_config dma_cfg = {0};
	size_t chunk_size;
	uint32_t bb_count;
	int ret;

	ret = esp32_lcd_cam_rgb_pick_bounce(ctx->fb_size, &chunk_size, &bb_count);
	if ((ret < 0) || (ctx->bb_capacity < bb_count * chunk_size)) {
		LOG_ERR("No bounce buffer layout fits a %u byte frame", (uint32_t)ctx->fb_size);
		return -EINVAL;
	}

	ctx->chunk_size = chunk_size;
	ctx->chunk_count = ctx->fb_size / chunk_size;
	ctx->bb_count = bb_count;

	for (uint32_t i = 0U; i < bb_count; i++) {
		ctx->dma_blocks[i] = (struct dma_block_config){
			.source_address = (uint32_t)(uintptr_t)(ctx->bb + i * chunk_size),
			.block_size = chunk_size,
			.next_block = (i + 1U < bb_count) ? &ctx->dma_blocks[i + 1U] : NULL,
		};
	}

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.dma_slot = ESP_GDMA_TRIG_PERIPH_LCD0;
	dma_cfg.complete_callback_en = 1U;
	dma_cfg.cyclic = 1U;
	dma_cfg.block_count = bb_count;
	dma_cfg.head_block = &ctx->dma_blocks[0];
	dma_cfg.dma_callback = esp32_lcd_cam_rgb_dma_done;
	dma_cfg.user_data = (void *)dev;
	dma_cfg.source_burst_length = 64U;
	dma_cfg.dest_burst_length = 64U;

	ret = dma_config(ctx->dma_dev, ctx->dma_channel, &dma_cfg);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int esp32_lcd_cam_rgb_hw_prepare(struct esp32_lcd_cam_rgb_ctx *ctx)
{
	uint32_t src_clk_hz;
	hal_utils_clk_div_t clk_div = {0};
	uint32_t actual_pclk_hz;
	esp_err_t err;

	if (ctx->hw_ready) {
		return 0;
	}

	if (ctx->timing.clock_frequency == 0U) {
		LOG_ERR("LCD pixel clock must be non-zero");
		return -EINVAL;
	}

	err = esp_clk_tree_enable_src((soc_module_clk_t)LCD_CLK_SRC_PLL160M, true);
	if (err != ESP_OK) {
		return -EIO;
	}
	err = esp_clk_tree_src_get_freq_hz((soc_module_clk_t)LCD_CLK_SRC_PLL160M,
					   ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &src_clk_hz);
	if (err != ESP_OK) {
		return -EIO;
	}

	lcd_ll_enable_bus_clock(ESP32_LCD_CAM_RGB_BUS_ID, true);
	lcd_ll_reset_register(ESP32_LCD_CAM_RGB_BUS_ID);
	ctx->dev = LCD_LL_GET_HW(ESP32_LCD_CAM_RGB_BUS_ID);
	if (ctx->dev == NULL) {
		return -ENODEV;
	}
	lcd_ll_enable_clock(ctx->dev, true);
	lcd_ll_reset(ctx->dev);
	lcd_ll_fifo_reset(ctx->dev);
	lcd_ll_select_clk_src(ctx->dev, LCD_CLK_SRC_PLL160M);
	actual_pclk_hz = lcd_hal_cal_pclk_freq(
		(lcd_hal_context_t *)&(lcd_hal_context_t){
			.dev = ctx->dev,
		},
		src_clk_hz, ctx->timing.clock_frequency, &clk_div);
	if (actual_pclk_hz == 0U) {
		return -EINVAL;
	}
	lcd_ll_set_group_clock_coeff(ctx->dev, clk_div.integer, clk_div.denominator,
				     clk_div.numerator);
	lcd_ll_set_pixel_clock_edge(ctx->dev, !ctx->timing.pixelclk_active);
	lcd_ll_set_clock_idle_level(ctx->dev, false);
	lcd_ll_set_data_wire_width(ctx->dev, ctx->timing.data_bus_width);
	lcd_ll_set_dma_read_stride(ctx->dev, ctx->timing.data_bus_width);
	lcd_ll_set_horizontal_timing(ctx->dev, ctx->timing.hsync_len, ctx->timing.hback_porch,
				     ctx->timing.width, ctx->timing.hfront_porch);
	lcd_ll_set_vertical_timing(ctx->dev, ctx->timing.vsync_len, ctx->timing.vback_porch,
				   ctx->timing.height, ctx->timing.vfront_porch);
	lcd_ll_set_idle_level(ctx->dev, !ctx->timing.hsync_active, !ctx->timing.vsync_active,
			      !ctx->timing.de_active);
	lcd_ll_set_phase_cycles(ctx->dev, 0, 0, 1);
	lcd_ll_set_blank_cycles(ctx->dev, 1, 1);
	lcd_ll_enable_output_hsync_in_porch_region(ctx->dev, true);
	lcd_ll_set_hsync_position(ctx->dev, 0);
	lcd_ll_enable_rgb_mode(ctx->dev, true);
	lcd_ll_enable_auto_next_frame(ctx->dev, true);
	lcd_ll_enable_color_convert(ctx->dev, false);
	lcd_ll_enable_output_always_on(ctx->dev, true);
	lcd_ll_enable_trans_buffer(ctx->dev, false);

	ctx->hw_ready = true;
	return 0;
}

static int esp32_lcd_cam_rgb_init(const struct device *dev)
{
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;
	struct esp32_lcd_cam_rgb_ctx *ctx = dev->data;
	uint64_t fb_size64;
	int bpp;
	int ret;

	k_mutex_init(&ctx->lock);

	if ((cfg->timing.width == 0U) || (cfg->timing.height == 0U)) {
		return -EINVAL;
	}

	bpp = esp32_lcd_cam_rgb_bytes_per_pixel(&cfg->timing);
	if (bpp < 0) {
		return bpp;
	}

	fb_size64 = (uint64_t)cfg->timing.width * (uint64_t)cfg->timing.height * (uint64_t)bpp;
	if (fb_size64 > SIZE_MAX) {
		return -EOVERFLOW;
	}

	if (ctx->fb == NULL) {
		return -ENOMEM;
	}

	if ((size_t)fb_size64 > ctx->fb_capacity) {
		LOG_ERR("Framebuffer capacity too small: need %u have %u", (uint32_t)fb_size64,
			(uint32_t)ctx->fb_capacity);
		return -ENOMEM;
	}

	/* The panel is initialized first and must be ready before the stream starts */
	if ((cfg->panel != NULL) && !device_is_ready(cfg->panel)) {
		LOG_ERR("Panel controller is not ready");
		return -ENODEV;
	}

	ctx->timing = cfg->timing;
	ctx->fb_size = (size_t)fb_size64;
	ctx->bytes_per_pixel = (size_t)bpp;

	if (!device_is_ready(ctx->dma_dev)) {
		LOG_ERR("LCD_CAM DMA device is not ready");
		return -ENODEV;
	}

	memset(ctx->fb, 0, (size_t)fb_size64);

	ret = esp32_lcd_cam_rgb_hw_prepare(ctx);
	if (ret < 0) {
		printk("Failed to prepare LCD_CAM RGB hardware: %d\n", ret);
		return ret;
	}

	ret = esp32_lcd_cam_rgb_configure_dma(dev);
	if (ret < 0) {
		printk("Failed to configure LCD_CAM RGB DMA: %d\n", ret);
		return ret;
	}

	sys_cache_data_flush_range(ctx->fb, ctx->fb_size);

	lcd_ll_stop(ctx->dev);
	lcd_ll_reset(ctx->dev);
	lcd_ll_fifo_reset(ctx->dev);

	lcd_ll_clear_interrupt_status(ctx->dev, UINT32_MAX);
	ret = esp_intr_alloc_intrstatus(ETS_LCD_CAM_INTR_SOURCE, ESP32_LCD_CAM_RGB_INTR_FLAGS,
					(uint32_t)lcd_ll_get_interrupt_status_reg(ctx->dev),
					LCD_LL_EVENT_RGB, esp32_lcd_cam_rgb_vsync_isr, (void *)dev,
					&ctx->intr_handle);
	if (ret != 0) {
		printk("Failed to install LCD_CAM RGB VSYNC interrupt: %d\n", ret);
		return -EINVAL;
	}
	lcd_ll_enable_interrupt(ctx->dev, LCD_LL_EVENT_RGB, true);
	esp_intr_enable(ctx->intr_handle);

	ret = dma_esp32_gdma_config_restart(ctx->dma_dev, ctx->dma_channel,
					    ESP32_LCD_CAM_RGB_RESTART_SKIP_BYTES);
	if (ret < 0) {
		printk("Failed to configure LCD_CAM RGB restart node: %d\n", ret);
		return ret;
	}

	esp32_lcd_cam_rgb_prefill(ctx);

	ret = dma_start(ctx->dma_dev, ctx->dma_channel);
	if (ret < 0) {
		printk("Failed to start LCD_CAM RGB DMA: %d\n", ret);
		return ret;
	}
	/* Let DMA push the first words into the LCD FIFO before the LCD engine
	 * starts pulling from it.
	 */
	k_busy_wait(ESP32_LCD_CAM_RGB_FIFO_PRIME_US);
	lcd_ll_start(ctx->dev);

	ctx->initialized = true;

	LOG_INF("ESP32 LCD_CAM RGB backend ready: %ux%u @ %u Hz (%u-bit)", cfg->timing.width,
		cfg->timing.height, cfg->timing.clock_frequency, cfg->timing.data_bus_width);
	return 0;
}

static int esp32_lcd_cam_rgb_write(const struct device *dev, const uint16_t x, const uint16_t y,
				   const struct display_buffer_descriptor *desc, const void *buf)
{
	struct esp32_lcd_cam_rgb_ctx *ctx = dev->data;
	size_t dst_stride;
	size_t src_stride;
	size_t row_copy;
	uint8_t *dst;
	const uint8_t *src;
	int ret;

	if (!ctx->initialized) {
		printk("LCD_CAM RGB device not initialized\n");
		return -ENODEV;
	}

	if ((desc == NULL) || (buf == NULL)) {
		printk("Invalid descriptor or buffer\n");
		return -EINVAL;
	}

	if ((desc->width == 0U) || (desc->height == 0U) || (desc->pitch < desc->width)) {
		printk("Invalid descriptor dimensions: width=%u, height=%u, pitch=%u\n",
		       desc->width, desc->height, desc->pitch);
		return -EINVAL;
	}

	if (((uint32_t)x + desc->width > ctx->timing.width) ||
	    ((uint32_t)y + desc->height > ctx->timing.height)) {
		printk("Write operation exceeds framebuffer bounds: x=%u, y=%u, width=%u, "
		       "height=%u\n",
		       x, y, desc->width, desc->height);
		return -EINVAL;
	}

	src_stride = (size_t)desc->pitch * ctx->bytes_per_pixel;
	row_copy = (size_t)desc->width * ctx->bytes_per_pixel;
	if (desc->buf_size < (src_stride * desc->height)) {
		printk("Buffer size too small for the given descriptor: buf_size=%u, required=%u\n",
		       desc->buf_size, (uint32_t)(src_stride * desc->height));
		return -EMSGSIZE;
	}

	dst_stride = (size_t)ctx->timing.width * ctx->bytes_per_pixel;
	dst = ctx->fb + ((size_t)y * dst_stride) + ((size_t)x * ctx->bytes_per_pixel);
	src = buf;

	ret = k_mutex_lock(&ctx->lock, K_FOREVER);
	if (ret != 0) {
		printk("Failed to acquire mutex lock (error %d)\n", ret);
		return ret;
	}

	for (uint16_t row = 0U; row < desc->height; row++) {
		memcpy(dst, src, row_copy);
		dst += dst_stride;
		src += src_stride;
	}

	k_mutex_unlock(&ctx->lock);
	return 0;
}

static int esp32_lcd_cam_rgb_blanking_on(const struct device *dev)
{
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;

	if (cfg->panel == NULL) {
		return -ENOSYS;
	}

	return display_blanking_on(cfg->panel);
}

static int esp32_lcd_cam_rgb_blanking_off(const struct device *dev)
{
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;

	if (cfg->panel == NULL) {
		return -ENOSYS;
	}

	return display_blanking_off(cfg->panel);
}

static enum display_pixel_format esp32_lcd_cam_rgb_get_pixel_format(const struct device *dev)
{
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;

	return (cfg->timing.data_bus_width <= 16U) ? PIXEL_FORMAT_RGB_565 : PIXEL_FORMAT_RGB_888;
}

static void esp32_lcd_cam_rgb_get_capabilities(const struct device *dev,
					       struct display_capabilities *caps)
{
	const struct esp32_lcd_cam_rgb_config *cfg = dev->config;
	const enum display_pixel_format fmt = esp32_lcd_cam_rgb_get_pixel_format(dev);

	memset(caps, 0, sizeof(*caps));
	caps->x_resolution = cfg->timing.width;
	caps->y_resolution = cfg->timing.height;
	caps->supported_pixel_formats = fmt;
	caps->current_pixel_format = fmt;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int esp32_lcd_cam_rgb_set_pixel_format(const struct device *dev,
					      const enum display_pixel_format fmt)
{
	return (fmt == esp32_lcd_cam_rgb_get_pixel_format(dev)) ? 0 : -ENOTSUP;
}

static int esp32_lcd_cam_rgb_set_orientation(const struct device *dev,
					     const enum display_orientation orientation)
{
	ARG_UNUSED(dev);

	return (orientation == DISPLAY_ORIENTATION_NORMAL) ? 0 : -ENOTSUP;
}

static DEVICE_API(display, esp32_lcd_cam_rgb_api) = {
	.blanking_on = esp32_lcd_cam_rgb_blanking_on,
	.blanking_off = esp32_lcd_cam_rgb_blanking_off,
	.write = esp32_lcd_cam_rgb_write,
	.get_capabilities = esp32_lcd_cam_rgb_get_capabilities,
	.set_pixel_format = esp32_lcd_cam_rgb_set_pixel_format,
	.set_orientation = esp32_lcd_cam_rgb_set_orientation,
};

#define ESP32_LCD_CAM_RGB_BPP(inst) ((DT_INST_PROP(inst, data_bus_width) <= 16) ? 2U : 3U)

#define ESP32_LCD_CAM_RGB_FB_SIZE(inst)                                                            \
	((size_t)DT_INST_PROP(inst, width) * (size_t)DT_INST_PROP(inst, height) *                  \
	 (size_t)ESP32_LCD_CAM_RGB_BPP(inst))

#define ESP32_LCD_CAM_RGB_TIMING(inst) DT_INST_CHILD(inst, display_timings)

#define ESP32_LCD_CAM_RGB_INST_DEFINE(inst)                                                        \
	BUILD_ASSERT(DT_INST_PROP(inst, data_bus_width) <= 24,                                     \
		     "lcd_cam_rgb: data-bus-width > 24 is not supported");                         \
	static __aligned(64) uint8_t __attribute__((section(                                       \
		".ext_ram.bss"))) esp32_lcd_cam_rgb_fb_##inst[ESP32_LCD_CAM_RGB_FB_SIZE(inst)];    \
	static __aligned(64) uint8_t esp32_lcd_cam_rgb_bb_##inst[ESP32_LCD_CAM_RGB_BB_TOTAL_SIZE]; \
	static const struct esp32_lcd_cam_rgb_config esp32_lcd_cam_rgb_cfg_##inst = {              \
		.panel = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, display_controller)),         \
		.timing =                                                                          \
			{                                                                          \
				.width = DT_INST_PROP(inst, width),                                \
				.height = DT_INST_PROP(inst, height),                              \
				.data_bus_width = DT_INST_PROP(inst, data_bus_width),              \
				.clock_frequency = DT_PROP_OR(ESP32_LCD_CAM_RGB_TIMING(inst),      \
							      clock_frequency, 0),                 \
				.hsync_len = DT_PROP(ESP32_LCD_CAM_RGB_TIMING(inst), hsync_len),   \
				.hback_porch =                                                     \
					DT_PROP(ESP32_LCD_CAM_RGB_TIMING(inst), hback_porch),      \
				.hfront_porch =                                                    \
					DT_PROP(ESP32_LCD_CAM_RGB_TIMING(inst), hfront_porch),     \
				.vsync_len = DT_PROP(ESP32_LCD_CAM_RGB_TIMING(inst), vsync_len),   \
				.vback_porch =                                                     \
					DT_PROP(ESP32_LCD_CAM_RGB_TIMING(inst), vback_porch),      \
				.vfront_porch =                                                    \
					DT_PROP(ESP32_LCD_CAM_RGB_TIMING(inst), vfront_porch),     \
				.hsync_active =                                                    \
					DT_PROP(ESP32_LCD_CAM_RGB_TIMING(inst), hsync_active),     \
				.vsync_active =                                                    \
					DT_PROP(ESP32_LCD_CAM_RGB_TIMING(inst), vsync_active),     \
				.de_active = DT_PROP(ESP32_LCD_CAM_RGB_TIMING(inst), de_active),   \
				.pixelclk_active =                                                 \
					DT_PROP(ESP32_LCD_CAM_RGB_TIMING(inst), pixelclk_active),  \
			},                                                                         \
	};                                                                                         \
	static struct esp32_lcd_cam_rgb_ctx esp32_lcd_cam_rgb_data_##inst = {                      \
		.fb = esp32_lcd_cam_rgb_fb_##inst,                                                 \
		.fb_capacity = ESP32_LCD_CAM_RGB_FB_SIZE(inst),                                    \
		.bb = esp32_lcd_cam_rgb_bb_##inst,                                                 \
		.bb_capacity = sizeof(esp32_lcd_cam_rgb_bb_##inst),                                \
		.dma_dev = DEVICE_DT_GET(DT_DMAS_CTLR_BY_NAME(DT_PARENT(DT_DRV_INST(inst)), tx)),  \
		.dma_channel = DT_DMAS_CELL_BY_NAME(DT_PARENT(DT_DRV_INST(inst)), tx, channel),    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, esp32_lcd_cam_rgb_init, NULL, &esp32_lcd_cam_rgb_data_##inst,  \
			      &esp32_lcd_cam_rgb_cfg_##inst, POST_KERNEL,                          \
			      CONFIG_DISPLAY_ESP32_LCD_CAM_RGB_INIT_PRIORITY,                      \
			      &esp32_lcd_cam_rgb_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_LCD_CAM_RGB_INST_DEFINE)
