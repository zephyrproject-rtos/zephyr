/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_lcd_cam_rgb

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/sys/util.h>

#include <esp_attr.h>
#include <esp_clk_tree.h>
#include <esp_memory_utils.h>
#include <hal/dma_types.h>
#include <hal/gdma_ll.h>
#include <hal/lcd_ll.h>

LOG_MODULE_REGISTER(display_esp32_rgb, CONFIG_DISPLAY_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(espressif_esp32_lcd_cam_mipi_dbi) == 0,
	     "The LCD-CAM LCD block drives either an RGB or a MIPI DBI panel, not both");

/* RGB565 on a 16-bit bus */
#define RGB_BYTES_PER_PIXEL 2U

/*
 * The panel is fed from two bounce buffers in internal RAM, sent in a loop by
 * the DMA and refilled from the framebuffer while it sends the other one.
 * Reading the framebuffer from PSRAM directly, the DMA falls behind whenever
 * the CPU also uses PSRAM, and the picture breaks up.
 *
 * Each bounce buffer holds up to RGB_BOUNCE_LINES_MAX lines, as many as make a
 * frame a whole number of loops over both buffers, so every frame starts at the
 * top of the first one.
 */
#define RGB_BOUNCE_LINES_MAX 10U
#define RGB_BOUNCE_LINES_MIN 4U

/* ESP-IDF keeps the pixel clock prescaler at 2 or more on this block */
#define RGB_PCLK_PRESCALE 2U

/* Fixed denominator of the fractional LCD clock divider */
#define RGB_CLK_DIV_A (LCD_LL_CLK_FRAC_DIV_AB_MAX - 1U)

#define RGB_FB_NUM CONFIG_DISPLAY_ESP32_RGB_FB_NUM

/* How long a write waits for the panel to leave the framebuffer it draws into */
#define RGB_FLIP_TIMEOUT_MS 200

/*
 * The DMA moves PSRAM data in 64-byte blocks, so memory-to-memory copies into
 * and out of the framebuffers are split into aligned pieces of at most one
 * descriptor each.
 */
#define RGB_DMA_ALIGN       64U
#define RGB_DMA_CHUNK       ROUND_DOWN(DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED, RGB_DMA_ALIGN)
#define RGB_DMA_MAX_CHUNKS  CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM
#define RGB_DMA_TIMEOUT_MS  100

/*
 * The panel comes first: the LCD channel and the refills of the bounce buffers
 * get the highest DMA priority, the copies into the framebuffers the lowest, so
 * that these give way when PSRAM bandwidth runs short.
 */
#define RGB_DMA_PRIO_PANEL GDMA_LL_CHANNEL_MAX_PRIORITY
#define RGB_DMA_PRIO_WRITE 0U

struct display_esp32_rgb_config {
	const struct device *dma_dev;
	uint8_t dma_channel;
	int irq_source;
	int irq_priority;
	int irq_flags;
	/* Optional memory-to-memory channel pairs, NULL to copy with the CPU */
	const struct device *refill_dev;
	uint8_t refill_channel;
	const struct device *write_dev;
	uint8_t write_channel;
	uint8_t *bounce;
	/* Bitmap of the rows the frame being drawn has fully written */
	uint32_t *rows_done;
	uint16_t width;
	uint16_t height;
	uint32_t pclk_hz;
	uint16_t hsync_len;
	uint16_t hback_porch;
	uint16_t hfront_porch;
	uint16_t vsync_len;
	uint16_t vback_porch;
	uint16_t vfront_porch;
	bool hsync_active;
	bool vsync_active;
	bool de_active;
	bool pclk_active_neg;
};

struct display_esp32_rgb_data {
	uint8_t *fb[RGB_FB_NUM];
	/* Copies of the config used from the DMA interrupt */
	uint8_t *bounce;
	size_t pitch;
	uint16_t height;
	uint16_t bounce_lines;
	const struct device *refill_dev;
	uint8_t refill_channel;
	/* A refill copy is in flight, the next one falls back to the CPU */
	volatile bool refill_busy;
	struct k_sem written;
	/* Next framebuffer line to send, and bounce buffer the DMA sends next */
	uint16_t next_line;
	uint8_t drained;
	/* Framebuffers on the panel, completed and waiting for the next frame
	 * (-1 when none), being drawn, and last completed.
	 */
	int8_t front;
	int8_t pending;
	int8_t back;
	int8_t last;
	struct k_sem flipped;
};

/* Starts copying size bytes with a memory-to-memory channel pair */
static int IRAM_ATTR display_esp32_rgb_m2m(const struct device *dma_dev, uint8_t channel,
					   uint32_t priority, dma_callback_t callback,
					   void *user_data, const uint8_t *src, uint8_t *dst,
					   size_t size)
{
	struct dma_block_config blocks[RGB_DMA_MAX_CHUNKS] = {0};
	struct dma_config cfg = {0};
	size_t count = DIV_ROUND_UP(size, RGB_DMA_CHUNK);
	int ret;

	__ASSERT_NO_MSG(count <= ARRAY_SIZE(blocks));

	for (size_t i = 0; i < count; i++) {
		size_t offset = i * RGB_DMA_CHUNK;

		blocks[i].source_address = (uint32_t)(src + offset);
		blocks[i].dest_address = (uint32_t)(dst + offset);
		blocks[i].block_size = MIN(RGB_DMA_CHUNK, size - offset);
		blocks[i].next_block = (i + 1U < count) ? &blocks[i + 1U] : NULL;
	}

	cfg.channel_direction = MEMORY_TO_MEMORY;
	cfg.channel_priority = priority;
	cfg.source_burst_length = RGB_DMA_ALIGN;
	cfg.dest_burst_length = RGB_DMA_ALIGN;
	cfg.block_count = count;
	cfg.head_block = &blocks[0];
	cfg.dma_callback = callback;
	cfg.user_data = user_data;

	ret = dma_config(dma_dev, channel, &cfg);
	if (ret == 0) {
		ret = dma_start(dma_dev, channel);
	}

	return ret;
}

/* The receiving half of a memory-to-memory pair reports the end of a copy */
static inline bool display_esp32_rgb_copied(uint32_t channel, int status)
{
	return (channel % 2U) == 0U && status != DMA_STATUS_BLOCK;
}

static void IRAM_ATTR display_esp32_rgb_refilled(const struct device *dma_dev, void *user_data,
						 uint32_t channel, int status)
{
	struct display_esp32_rgb_data *data = user_data;

	ARG_UNUSED(dma_dev);

	if (display_esp32_rgb_copied(channel, status)) {
		data->refill_busy = false;
	}
}

/* Copies the next lines to send into a bounce buffer, starting the next frame
 * from the latest completed framebuffer once the current one is all sent.
 */
static void IRAM_ATTR display_esp32_rgb_fill(struct display_esp32_rgb_data *data, uint8_t bounce)
{
	uint8_t *dst = data->bounce + bounce * data->bounce_lines * data->pitch;
	uint16_t left = data->bounce_lines;

	while (left > 0U) {
		uint16_t lines;
		const uint8_t *src;

		if (data->next_line == data->height) {
			data->next_line = 0U;
			if (data->pending >= 0) {
				data->front = data->pending;
				data->pending = -1;
				k_sem_give(&data->flipped);
			}
		}

		lines = MIN(left, data->height - data->next_line);
		src = data->fb[data->front] + data->next_line * data->pitch;

		/* A whole bounce buffer goes by DMA, unless the last one is still in flight */
		if (lines == data->bounce_lines && data->refill_dev != NULL && !data->refill_busy) {
			data->refill_busy = true;
			if (display_esp32_rgb_m2m(data->refill_dev, data->refill_channel,
						  RGB_DMA_PRIO_PANEL, display_esp32_rgb_refilled,
						  data, src, dst, lines * data->pitch) != 0) {
				data->refill_busy = false;
				memcpy(dst, src, lines * data->pitch);
			}
		} else {
			memcpy(dst, src, lines * data->pitch);
		}

		dst += lines * data->pitch;
		data->next_line += lines;
		left -= lines;
	}
}

static void IRAM_ATTR display_esp32_rgb_dma_done(const struct device *dma_dev, void *user_data,
						 uint32_t channel, int status)
{
	struct display_esp32_rgb_data *data = user_data;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(status);

	/* The DMA went on with the other bounce buffer, refill the one it left */
	display_esp32_rgb_fill(data, data->drained);
	data->drained ^= 1U;
}

/* Fills both bounce buffers by CPU from the top of a frame */
static void display_esp32_rgb_prime(struct display_esp32_rgb_data *data)
{
	/* The first fill starts a frame, with the framebuffer waiting if any */
	data->next_line = data->height;
	data->drained = 0U;
	data->refill_busy = true;
	display_esp32_rgb_fill(data, 0U);
	display_esp32_rgb_fill(data, 1U);
	data->refill_busy = false;
}

/*
 * When a frame starts, the DMA is at the top of the first bounce buffer and
 * both buffers hold the first lines of the frame. Refills follow the DMA one
 * interrupt at a time, and interrupts held off for longer than a bounce buffer
 * takes to send, as during a flash erase, make them fall behind. The DMA never
 * stops, so filling both buffers again during the back porch puts them back in
 * step. A late VSYNC interrupt can do that at the wrong time, and the next one
 * fixes it.
 */
static void display_esp32_rgb_vsync(void *arg)
{
	const struct device *dev = arg;
	struct display_esp32_rgb_data *data = dev->data;
	unsigned int key;

	lcd_ll_clear_interrupt_status(LCD_LL_GET_HW(0), LCD_LL_EVENT_VSYNC_END);

	if (data->next_line == 2U * data->bounce_lines && data->drained == 0U) {
		return;
	}

	key = irq_lock();
	/* A copy still in flight would land over the lines filled now */
	if (data->refill_dev != NULL) {
		(void)dma_stop(data->refill_dev, data->refill_channel);
	}
	display_esp32_rgb_prime(data);
	irq_unlock(key);
}

static bool display_esp32_rgb_row_done(const uint32_t *rows_done, uint16_t row)
{
	return (rows_done[row / 32U] & BIT(row % 32U)) != 0U;
}

static void display_esp32_rgb_written(const struct device *dma_dev, void *user_data,
				      uint32_t channel, int status)
{
	struct display_esp32_rgb_data *data = user_data;

	ARG_UNUSED(dma_dev);

	if (display_esp32_rgb_copied(channel, status)) {
		k_sem_give(&data->written);
	}
}

/* Copies into a framebuffer by DMA, the calling thread waiting meanwhile */
static int display_esp32_rgb_dma_write(const struct display_esp32_rgb_config *config,
				       struct display_esp32_rgb_data *data, const uint8_t *src,
				       uint8_t *dst, size_t size)
{
	while (size > 0U) {
		size_t len = MIN(size, RGB_DMA_MAX_CHUNKS * RGB_DMA_CHUNK);
		int ret;

		k_sem_reset(&data->written);
		ret = display_esp32_rgb_m2m(config->write_dev, config->write_channel,
					    RGB_DMA_PRIO_WRITE, display_esp32_rgb_written, data,
					    src, dst, len);
		if (ret == 0 && k_sem_take(&data->written, K_MSEC(RGB_DMA_TIMEOUT_MS)) != 0) {
			ret = -ETIMEDOUT;
		}

		if (ret != 0) {
			LOG_ERR("DMA copy into the framebuffer failed (%d)", ret);
			return ret;
		}

		src += len;
		dst += len;
		size -= len;
	}

	return 0;
}

/* Copies rows of the last completed frame into the frame being drawn */
static int display_esp32_rgb_copy_rows(const struct display_esp32_rgb_config *config,
				       struct display_esp32_rgb_data *data, uint16_t row,
				       uint16_t count)
{
	const uint8_t *src = data->fb[data->last] + (size_t)row * data->pitch;
	uint8_t *dst = data->fb[data->back] + (size_t)row * data->pitch;
	size_t size = (size_t)count * data->pitch;

	if (config->write_dev != NULL && (data->pitch % RGB_DMA_ALIGN) == 0U) {
		return display_esp32_rgb_dma_write(config, data, src, dst, size);
	}

	memcpy(dst, src, size);

	return 0;
}

/* Brings the rows in [first, end) the frame being drawn has not written yet up to date
 * from the last frame, a run of consecutive rows at a time.
 */
static int display_esp32_rgb_seed(const struct display_esp32_rgb_config *config,
				  struct display_esp32_rgb_data *data, uint16_t first, uint16_t end)
{
	uint32_t start = first;

	for (uint32_t row = first; row <= end; row++) {
		int ret;

		if (row < end && !display_esp32_rgb_row_done(config->rows_done, row)) {
			continue;
		}

		if (row > start) {
			ret = display_esp32_rgb_copy_rows(config, data, start, row - start);
			if (ret < 0) {
				return ret;
			}
		}

		start = row + 1U;
	}

	return 0;
}

/* Hands the completed back framebuffer to the panel and picks the next one */
static int display_esp32_rgb_present(const struct display_esp32_rgb_config *config,
				     struct display_esp32_rgb_data *data)
{
	unsigned int key;
	int8_t dropped;
	int ret;

	ret = display_esp32_rgb_seed(config, data, 0U, config->height);
	if (ret < 0) {
		return ret;
	}

	memset(config->rows_done, 0, DIV_ROUND_UP(config->height, 32U) * sizeof(uint32_t));

	key = irq_lock();
	dropped = data->pending;
	data->pending = data->back;
	data->last = data->back;
	if (RGB_FB_NUM == 2) {
		/* Drawn into again once the panel has moved to the other one */
		data->back = data->front;
		k_sem_reset(&data->flipped);
	} else {
		/* The framebuffer neither on the panel nor waiting for it */
		data->back = (dropped >= 0) ? dropped : (3 - data->front - data->pending);
	}
	irq_unlock(key);

	return 0;
}

static int display_esp32_rgb_write(const struct device *dev, const uint16_t x, const uint16_t y,
				   const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct display_esp32_rgb_config *config = dev->config;
	struct display_esp32_rgb_data *data = dev->data;
	size_t src_pitch = (size_t)desc->pitch * RGB_BYTES_PER_PIXEL;
	size_t row_size = (size_t)desc->width * RGB_BYTES_PER_PIXEL;
	bool full_rows = (x == 0U && desc->width == config->width);
	const uint8_t *src = buf;
	uint8_t *dst;

	if (desc->width == 0U || desc->height == 0U) {
		return 0;
	}

	if ((uint32_t)x + desc->width > config->width ||
	    (uint32_t)y + desc->height > config->height) {
		LOG_ERR("Write %ux%u at (%u,%u) exceeds %ux%u display", desc->width, desc->height,
			x, y, config->width, config->height);
		return -EINVAL;
	}

	if (desc->pitch < desc->width ||
	    (desc->height - 1U) * src_pitch + row_size > desc->buf_size) {
		LOG_ERR("Buffer too small for %ux%u with pitch %u", desc->width, desc->height,
			desc->pitch);
		return -EINVAL;
	}

	/* With two framebuffers, the one to draw into stays on the panel until the
	 * frame completed in the other is shown.
	 */
	if (RGB_FB_NUM == 2 && data->back == data->front &&
	    k_sem_take(&data->flipped, K_MSEC(RGB_FLIP_TIMEOUT_MS)) != 0) {
		LOG_ERR("Timed out waiting for the panel to show the last frame");
		return -ETIMEDOUT;
	}

	dst = data->fb[data->back] + (size_t)y * data->pitch + (size_t)x * RGB_BYTES_PER_PIXEL;

	if (RGB_FB_NUM > 1) {
		/* Keep the rest of the rows the frame only partly redraws */
		if (!full_rows && src != dst) {
			int ret = display_esp32_rgb_seed(config, data, y, y + desc->height);

			if (ret < 0) {
				return ret;
			}
		}

		for (uint16_t row = y; row < y + desc->height; row++) {
			config->rows_done[row / 32U] |= BIT(row % 32U);
		}
	}

	/* Nothing to copy when the caller drew into the framebuffer itself. Contiguous
	 * whole rows from DMA-capable memory go by DMA when a channel pair is set.
	 */
	if (src != dst && config->write_dev != NULL && full_rows && src_pitch == data->pitch &&
	    (data->pitch % RGB_DMA_ALIGN) == 0U && ((uintptr_t)src % sizeof(uint32_t)) == 0U &&
	    esp_ptr_dma_capable(src)) {
		int ret = display_esp32_rgb_dma_write(config, data, src, dst,
						      desc->height * data->pitch);

		if (ret < 0) {
			return ret;
		}
	} else if (src != dst) {
		for (uint16_t row = 0; row < desc->height; row++) {
			memcpy(dst + row * data->pitch, src + row * src_pitch, row_size);
		}
	}

	if (RGB_FB_NUM > 1 && !desc->frame_incomplete) {
		return display_esp32_rgb_present(config, data);
	}

	return 0;
}

static void *display_esp32_rgb_get_framebuffer(const struct device *dev)
{
	struct display_esp32_rgb_data *data = dev->data;

	return data->fb[data->back];
}

static void display_esp32_rgb_get_capabilities(const struct device *dev,
					       struct display_capabilities *caps)
{
	const struct display_esp32_rgb_config *config = dev->config;

	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	caps->current_pixel_format = PIXEL_FORMAT_RGB_565;
}

static int display_esp32_rgb_set_pixel_format(const struct device *dev,
					      const enum display_pixel_format pixel_format)
{
	ARG_UNUSED(dev);

	return (pixel_format == PIXEL_FORMAT_RGB_565) ? 0 : -ENOTSUP;
}

static DEVICE_API(display, display_esp32_rgb_api) = {
	.write = display_esp32_rgb_write,
	.get_framebuffer = display_esp32_rgb_get_framebuffer,
	.get_capabilities = display_esp32_rgb_get_capabilities,
	.set_pixel_format = display_esp32_rgb_set_pixel_format,
};

static int display_esp32_rgb_init(const struct device *dev)
{
	const struct display_esp32_rgb_config *config = dev->config;
	struct display_esp32_rgb_data *data = dev->data;
	lcd_cam_dev_t *lcd = LCD_LL_GET_HW(0);
	size_t pitch = (size_t)config->width * RGB_BYTES_PER_PIXEL;
	size_t fb_size = pitch * config->height;
	struct dma_block_config dma_blocks[2] = {0};
	struct dma_config dma_cfg = {0};
	uint32_t src_hz;
	uint32_t lcd_hz;
	uint32_t div_num;
	uint32_t div_b;
	int ret;

	if (!device_is_ready(config->dma_dev)) {
		LOG_ERR("DMA controller not ready");
		return -ENODEV;
	}

	ret = esp_clk_tree_src_get_freq_hz((soc_module_clk_t)LCD_CLK_SRC_DEFAULT,
					   ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX, &src_hz);
	if (ret != ESP_OK) {
		LOG_ERR("Failed to get LCD clock source frequency (%d)", ret);
		return -EINVAL;
	}

	/* LCD clock = source / (div_num + div_b / RGB_CLK_DIV_A) */
	lcd_hz = config->pclk_hz * RGB_PCLK_PRESCALE;
	div_num = src_hz / lcd_hz;
	div_b = DIV_ROUND_CLOSEST((uint64_t)(src_hz % lcd_hz) * RGB_CLK_DIV_A, lcd_hz);
	if (div_b == RGB_CLK_DIV_A) {
		div_num++;
		div_b = 0U;
	}

	if (div_num < 2U || div_num > LCD_LL_CLK_FRAC_DIV_N_MAX) {
		LOG_ERR("Pixel clock %u Hz is out of range", config->pclk_hz);
		return -EINVAL;
	}

	data->bounce_lines = RGB_BOUNCE_LINES_MAX;
	while (config->height % (2U * data->bounce_lines) != 0U) {
		if (--data->bounce_lines < RGB_BOUNCE_LINES_MIN) {
			LOG_ERR("Height %u needs an even divisor from %u to %u", config->height,
				2U * RGB_BOUNCE_LINES_MIN, 2U * RGB_BOUNCE_LINES_MAX);
			return -ENOTSUP;
		}
	}

	if ((config->refill_dev != NULL && !device_is_ready(config->refill_dev)) ||
	    (config->write_dev != NULL && !device_is_ready(config->write_dev))) {
		LOG_ERR("Copy DMA controller not ready");
		return -ENODEV;
	}

	for (int i = 0; i < RGB_FB_NUM; i++) {
		data->fb[i] = shared_multi_heap_aligned_alloc(SMH_REG_ATTR_EXTERNAL, RGB_DMA_ALIGN,
							      fb_size);
		if (data->fb[i] == NULL) {
			LOG_ERR("Failed to allocate %zu byte framebuffer", fb_size);
			ret = -ENOMEM;
			goto free_fb;
		}

		memset(data->fb[i], 0, fb_size);
	}

	data->bounce = config->bounce;
	data->pitch = pitch;
	data->height = config->height;
	/* DMA copies need rows that start on a PSRAM block */
	data->refill_dev = ((pitch % RGB_DMA_ALIGN) == 0U) ? config->refill_dev : NULL;
	data->refill_channel = config->refill_channel;
	data->front = 0;
	data->pending = -1;
	data->back = RGB_FB_NUM - 1;
	data->last = 0;
	k_sem_init(&data->flipped, 0, 1);
	k_sem_init(&data->written, 0, 1);

	/* Both bounce buffers, sent in a loop */
	for (int i = 0; i < ARRAY_SIZE(dma_blocks); i++) {
		dma_blocks[i].source_address = (uint32_t)(config->bounce +
							  i * data->bounce_lines * pitch);
		dma_blocks[i].block_size = data->bounce_lines * pitch;
	}
	dma_blocks[0].next_block = &dma_blocks[1];

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.channel_priority = RGB_DMA_PRIO_PANEL;
	dma_cfg.dma_slot = ESP_GDMA_TRIG_PERIPH_LCD0;
	dma_cfg.block_count = ARRAY_SIZE(dma_blocks);
	dma_cfg.head_block = &dma_blocks[0];
	dma_cfg.cyclic = 1;
	dma_cfg.dma_callback = display_esp32_rgb_dma_done;
	dma_cfg.user_data = data;

	ret = dma_config(config->dma_dev, config->dma_channel, &dma_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure DMA channel %u (%d)", config->dma_channel, ret);
		goto free_fb;
	}

	lcd_ll_enable_clock(lcd, true);
	lcd_ll_select_clk_src(lcd, LCD_CLK_SRC_DEFAULT);
	lcd_ll_reset(lcd);
	lcd_ll_fifo_reset(lcd);

	lcd_ll_set_group_clock_coeff(lcd, div_num, div_b != 0U ? RGB_CLK_DIV_A : 0U, div_b);
	lcd_ll_set_pixel_clock_prescale(lcd, RGB_PCLK_PRESCALE);
	lcd_ll_set_clock_idle_level(lcd, false);
	lcd_ll_set_pixel_clock_edge(lcd, config->pclk_active_neg);

	lcd_ll_enable_rgb_mode(lcd, true);
	lcd_ll_enable_color_convert(lcd, false);
	lcd_ll_set_dma_read_stride(lcd, 16);
	lcd_ll_set_data_wire_width(lcd, 16);
	lcd_ll_set_phase_cycles(lcd, 0, 0, 1);
	lcd_ll_enable_output_always_on(lcd, true);
	lcd_ll_set_idle_level(lcd, !config->hsync_active, !config->vsync_active,
			      !config->de_active);
	lcd_ll_set_blank_cycles(lcd, 1, 1);
	lcd_ll_set_horizontal_timing(lcd, config->hsync_len, config->hback_porch, config->width,
				     config->hfront_porch);
	lcd_ll_set_vertical_timing(lcd, config->vsync_len, config->vback_porch, config->height,
				   config->vfront_porch);
	lcd_ll_enable_output_hsync_in_porch_region(lcd, true);
	lcd_ll_set_hsync_position(lcd, 0);
	/* The panel is refreshed continuously, frame after frame */
	lcd_ll_enable_auto_next_frame(lcd, true);

	ret = esp_intr_alloc_intrstatus(config->irq_source,
					ESP_PRIO_TO_FLAGS(config->irq_priority) |
						ESP_INT_FLAGS_CHECK(config->irq_flags),
					(uint32_t)lcd_ll_get_interrupt_status_reg(lcd),
					LCD_LL_EVENT_VSYNC_END,
					(intr_handler_t)display_esp32_rgb_vsync, (void *)dev, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to allocate interrupt (%d)", ret);
		goto free_fb;
	}

	/* Both bounce buffers are full before the panel DMA starts */
	display_esp32_rgb_prime(data);

	ret = dma_start(config->dma_dev, config->dma_channel);
	if (ret < 0) {
		LOG_ERR("Failed to start DMA channel %u (%d)", config->dma_channel, ret);
		goto free_fb;
	}

	lcd_ll_clear_interrupt_status(lcd, LCD_LL_EVENT_VSYNC_END);
	lcd_ll_enable_interrupt(lcd, LCD_LL_EVENT_VSYNC_END, true);
	lcd_ll_start(lcd);

	LOG_DBG("%ux%u RGB565, pixel clock %u Hz", config->width, config->height,
		(uint32_t)((uint64_t)src_hz * RGB_CLK_DIV_A /
			   ((uint64_t)div_num * RGB_CLK_DIV_A + div_b) / RGB_PCLK_PRESCALE));

	return 0;

free_fb:
	for (int i = 0; i < RGB_FB_NUM; i++) {
		shared_multi_heap_free(data->fb[i]);
		data->fb[i] = NULL;
	}

	return ret;
}

#define DISPLAY_ESP32_RGB_TIMINGS(n) DT_INST_CHILD(n, display_timings)

/* Optional memory-to-memory pair named in dma-names, as its receiving channel */
#define DISPLAY_ESP32_RGB_M2M_DEV(n, name)                                                         \
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(n, name),                                                \
		    (DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, name))), (NULL))
#define DISPLAY_ESP32_RGB_M2M_CHANNEL(n, name)                                                     \
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(n, name),                                                \
		    (ROUND_DOWN(DT_INST_DMAS_CELL_BY_NAME(n, name, channel), 2)), (0))

#define DISPLAY_ESP32_RGB_DEFINE(n)                                                                \
	BUILD_ASSERT(DT_NODE_HAS_PROP(DISPLAY_ESP32_RGB_TIMINGS(n), clock_frequency),              \
		     "display-timings needs a clock-frequency, the pixel clock");                  \
                                                                                                   \
	static uint8_t __aligned(4) display_esp32_rgb_bounce_##n[2 * RGB_BOUNCE_LINES_MAX *        \
								 DT_INST_PROP(n, width) *          \
								 RGB_BYTES_PER_PIXEL];             \
	static uint32_t display_esp32_rgb_rows_##n[DIV_ROUND_UP(DT_INST_PROP(n, height), 32)];     \
                                                                                                   \
	static const struct display_esp32_rgb_config display_esp32_rgb_config_##n = {              \
		.dma_dev = DEVICE_DT_GET(DT_DMAS_CTLR_BY_NAME(DT_INST_PARENT(n), tx)),            \
		.dma_channel = DT_DMAS_CELL_BY_NAME(DT_INST_PARENT(n), tx, channel),               \
		.irq_source = DT_IRQ_BY_IDX(DT_INST_PARENT(n), 0, irq),                            \
		.irq_priority = DT_IRQ_BY_IDX(DT_INST_PARENT(n), 0, priority),                     \
		.irq_flags = DT_IRQ_BY_IDX(DT_INST_PARENT(n), 0, flags),                           \
		.refill_dev = DISPLAY_ESP32_RGB_M2M_DEV(n, refill),                                \
		.refill_channel = DISPLAY_ESP32_RGB_M2M_CHANNEL(n, refill),                        \
		.write_dev = DISPLAY_ESP32_RGB_M2M_DEV(n, write),                                  \
		.write_channel = DISPLAY_ESP32_RGB_M2M_CHANNEL(n, write),                          \
		.bounce = display_esp32_rgb_bounce_##n,                                            \
		.rows_done = display_esp32_rgb_rows_##n,                                           \
		.width = DT_INST_PROP(n, width),                                                   \
		.height = DT_INST_PROP(n, height),                                                 \
		.pclk_hz = DT_PROP(DISPLAY_ESP32_RGB_TIMINGS(n), clock_frequency),                 \
		.hsync_len = DT_PROP(DISPLAY_ESP32_RGB_TIMINGS(n), hsync_len),                     \
		.hback_porch = DT_PROP(DISPLAY_ESP32_RGB_TIMINGS(n), hback_porch),                 \
		.hfront_porch = DT_PROP(DISPLAY_ESP32_RGB_TIMINGS(n), hfront_porch),               \
		.vsync_len = DT_PROP(DISPLAY_ESP32_RGB_TIMINGS(n), vsync_len),                     \
		.vback_porch = DT_PROP(DISPLAY_ESP32_RGB_TIMINGS(n), vback_porch),                 \
		.vfront_porch = DT_PROP(DISPLAY_ESP32_RGB_TIMINGS(n), vfront_porch),               \
		.hsync_active = DT_PROP(DISPLAY_ESP32_RGB_TIMINGS(n), hsync_active),               \
		.vsync_active = DT_PROP(DISPLAY_ESP32_RGB_TIMINGS(n), vsync_active),               \
		.de_active = DT_PROP(DISPLAY_ESP32_RGB_TIMINGS(n), de_active),                     \
		.pclk_active_neg = !DT_PROP(DISPLAY_ESP32_RGB_TIMINGS(n), pixelclk_active),        \
	};                                                                                         \
                                                                                                   \
	static struct display_esp32_rgb_data display_esp32_rgb_data_##n;                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, display_esp32_rgb_init, NULL, &display_esp32_rgb_data_##n,        \
			      &display_esp32_rgb_config_##n, POST_KERNEL,                          \
			      CONFIG_DISPLAY_INIT_PRIORITY, &display_esp32_rgb_api);

DT_INST_FOREACH_STATUS_OKAY(DISPLAY_ESP32_RGB_DEFINE)
