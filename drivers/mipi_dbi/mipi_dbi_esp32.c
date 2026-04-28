/*
 * Copyright (c) 2025 Chen Xingyu <hi@xingrz.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_lcd_cam_mipi_dbi

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <soc/gdma_channel.h>
#include <esp_clk_tree.h>
#include <esp_heap_caps.h>
#include <esp_memory_utils.h>
#include <hal/lcd_hal.h>
#include <hal/lcd_ll.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mipi_dbi_esp32_lcd, CONFIG_MIPI_DBI_LOG_LEVEL);

/* Always 0 since we only have one */
#define LCD_BUS_ID (0)

#define LCD_PERIPH_CLOCK_PRE_SCALE (2)

#define LCD_DMA_TRANSFER_TIMEOUT_MS (1000)

struct mipi_dbi_esp32_device_config {
	uint32_t pclk_freq_hz;
	bool pclk_idle_low;
	bool pclk_active_neg;
	bool dc_idle_level;
	bool dc_cmd_level;
	bool dc_dummy_level;
	bool dc_data_level;
};

struct mipi_dbi_esp32_config {
	soc_periph_lcd_clk_src_t clock_source;
	int irq_source;
	int irq_priority;
	int irq_flags;
	const struct device *dma_dev;
	const struct mipi_dbi_esp32_device_config *devices;
	uint16_t num_devices;
	const struct gpio_dt_spec *cs_gpios;
	uint16_t num_cs_gpios;
	struct gpio_dt_spec reset_gpio;
	uint8_t tx_dma_channel;
};

struct mipi_dbi_esp32_data {
	lcd_hal_context_t hal;
	size_t resolution_hz;
	const struct mipi_dbi_config *current_dbi_cfg;
	struct k_mutex lock;
	struct k_sem dma_sem;
};

static const struct mipi_dbi_esp32_device_config *
mipi_dbi_esp32_get_device_config(const struct mipi_dbi_esp32_config *cfg,
				 const struct mipi_dbi_config *dbi_cfg)
{
	uint16_t index = dbi_cfg->config.slave;

	if (index >= cfg->num_devices) {
		LOG_ERR("Invalid device index %d", index);
		return NULL;
	}

	return &cfg->devices[index];
}

static void mipi_dbi_esp32_set_cs(const struct device *dev, const struct mipi_dbi_config *dbi_cfg,
				  bool active)
{
	const struct mipi_dbi_esp32_config *cfg = dev->config;

	if (dbi_cfg == NULL) {
		return;
	}

	if (dbi_cfg->config.slave >= cfg->num_cs_gpios) {
		return;
	}

	gpio_pin_set_dt(&cfg->cs_gpios[dbi_cfg->config.slave], active);
}

static int mipi_dbi_esp32_switch_device(const struct device *dev,
					const struct mipi_dbi_config *next_dbi_cfg)
{
	const struct mipi_dbi_esp32_config *cfg = dev->config;
	struct mipi_dbi_esp32_data *data = dev->data;
	lcd_hal_context_t *hal = &data->hal;
	const struct mipi_dbi_config *curr_dbi_cfg = data->current_dbi_cfg;
	const struct mipi_dbi_esp32_device_config *pcfg;
	uint32_t pclk_prescale;

	if (curr_dbi_cfg != NULL && curr_dbi_cfg->config.slave == next_dbi_cfg->config.slave) {
		return 0; /* Already selected */
	}

	switch (next_dbi_cfg->mode) {
	case MIPI_DBI_MODE_6800_BUS_16_BIT:
	case MIPI_DBI_MODE_8080_BUS_16_BIT:
		lcd_ll_set_data_width(hal->dev, 16);
		break;
	case MIPI_DBI_MODE_6800_BUS_8_BIT:
	case MIPI_DBI_MODE_8080_BUS_8_BIT:
		lcd_ll_set_data_width(hal->dev, 8);
		break;
	default:
		LOG_ERR("MIPI DBI mode %u is not supported.", next_dbi_cfg->mode);
		return -ENOTSUP;
	}

	pcfg = mipi_dbi_esp32_get_device_config(cfg, next_dbi_cfg);
	if (pcfg == NULL) {
		return -EINVAL;
	}

	if (pcfg->pclk_freq_hz == 0U) {
		LOG_ERR("Invalid PCLK frequency for device %u", next_dbi_cfg->config.slave);
		return -EINVAL;
	}

	pclk_prescale = DIV_ROUND_UP(data->resolution_hz, pcfg->pclk_freq_hz);
	if (pclk_prescale == 0 || pclk_prescale > LCD_LL_PCLK_DIV_MAX) {
		LOG_ERR("Unsupported PCLK frequency %" PRIu32 " Hz", pcfg->pclk_freq_hz);
		return -EINVAL;
	}

	lcd_ll_set_pixel_clock_prescale(hal->dev, pclk_prescale);
	lcd_ll_set_clock_idle_level(hal->dev, pcfg->pclk_idle_low);
	lcd_ll_set_pixel_clock_edge(hal->dev, pcfg->pclk_active_neg);
	lcd_ll_set_dc_level(hal->dev, pcfg->dc_idle_level, pcfg->dc_cmd_level, pcfg->dc_dummy_level,
			    pcfg->dc_data_level);

	mipi_dbi_esp32_set_cs(dev, curr_dbi_cfg, false);
	mipi_dbi_esp32_set_cs(dev, next_dbi_cfg, true);

	data->current_dbi_cfg = next_dbi_cfg;

	return 0;
}

static int mipi_dbi_esp32_dma_start(const struct device *dev, const uint8_t *buffer, size_t len)
{
	const struct mipi_dbi_esp32_config *cfg = dev->config;
	struct dma_status dma_status = {0};
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	int ret;

	if (len == 0U) {
		return 0;
	}

	ret = dma_get_status(cfg->dma_dev, cfg->tx_dma_channel, &dma_status);
	if (ret < 0) {
		LOG_ERR("Unable to get DMA status (%d)", ret);
		return ret;
	}

	if (dma_status.busy) {
		return -EBUSY;
	}

	dma_blk.block_size = len;
	dma_blk.source_address = (uint32_t)buffer;

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.dma_slot = ESP_GDMA_TRIG_PERIPH_LCD0;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;

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

static int mipi_dbi_esp32_transfer(const struct device *dev, const struct mipi_dbi_config *dbi_cfg,
				   bool send_cmd, uint8_t cmd, const uint8_t *buffer, size_t len)
{
	const struct mipi_dbi_esp32_config *cfg = dev->config;
	struct mipi_dbi_esp32_data *data = dev->data;
	lcd_hal_context_t *hal = &data->hal;
	uint8_t *tmp_buffer = NULL;
	int ret;

	ret = mipi_dbi_esp32_switch_device(dev, dbi_cfg);
	if (ret < 0) {
		return ret;
	}

	if (len > 0 && buffer == NULL) {
		return -EINVAL;
	}

	if (len > 0 && !esp_ptr_dma_capable(buffer)) {
		tmp_buffer = k_malloc(len);
		if (tmp_buffer == NULL) {
			LOG_ERR("Failed to allocate DMA capable buffer (%zu bytes)", len);
			return -ENOMEM;
		}

		memcpy(tmp_buffer, buffer, len);
	}

	lcd_ll_clear_interrupt_status(hal->dev, LCD_LL_EVENT_TRANS_DONE);
	while (k_sem_take(&data->dma_sem, K_NO_WAIT) == 0) {
	}

	lcd_ll_enable_interrupt(hal->dev, LCD_LL_EVENT_TRANS_DONE, true);

	if (send_cmd) {
		lcd_ll_set_command(hal->dev, 8, cmd);
	}

	lcd_ll_set_phase_cycles(hal->dev, send_cmd ? 1 : 0, 0, len > 0 ? 1 : 0);
	lcd_ll_set_blank_cycles(hal->dev, 1, 1);
	lcd_ll_fifo_reset(hal->dev);

	if (len > 0) {
		ret = mipi_dbi_esp32_dma_start(dev, tmp_buffer ? tmp_buffer : buffer, len);
		if (ret < 0) {
			goto free;
		}
	}

	lcd_ll_start(hal->dev);

	ret = k_sem_take(&data->dma_sem, K_MSEC(LCD_DMA_TRANSFER_TIMEOUT_MS));
	if (ret < 0) {
		LOG_ERR("Timed out waiting for transfer done");
		ret = -ETIMEDOUT;
	}

free:
	lcd_ll_enable_interrupt(hal->dev, LCD_LL_EVENT_TRANS_DONE, false);
	if (ret < 0 && len > 0) {
		dma_stop(cfg->dma_dev, cfg->tx_dma_channel);
	}
	k_free(tmp_buffer);

	return ret;
}

static int mipi_dbi_esp32_command_write(const struct device *dev,
					const struct mipi_dbi_config *dbi_cfg, uint8_t cmd,
					const uint8_t *data, size_t len)
{
	struct mipi_dbi_esp32_data *drv_data = dev->data;
	int ret;

	k_mutex_lock(&drv_data->lock, K_FOREVER);
	ret = mipi_dbi_esp32_transfer(dev, dbi_cfg, true, cmd, data, len);
	k_mutex_unlock(&drv_data->lock);

	return ret;
}

static int mipi_dbi_esp32_write_display(const struct device *dev,
					const struct mipi_dbi_config *dbi_cfg,
					const uint8_t *framebuf,
					struct display_buffer_descriptor *desc,
					enum display_pixel_format pixfmt)
{
	struct mipi_dbi_esp32_data *drv_data = dev->data;
	int ret;

	if (pixfmt != PIXEL_FORMAT_RGB_565 && pixfmt != PIXEL_FORMAT_RGB_565X) {
		return -ENOTSUP;
	}

	if (framebuf == NULL || desc->buf_size == 0U) {
		return -EINVAL;
	}

	k_mutex_lock(&drv_data->lock, K_FOREVER);
	ret = mipi_dbi_esp32_transfer(dev, dbi_cfg, false, 0x00, framebuf, desc->buf_size);
	k_mutex_unlock(&drv_data->lock);

	return ret;
}

static int mipi_dbi_esp32_reset(const struct device *dev, k_timeout_t delay)
{
	const struct mipi_dbi_esp32_config *cfg = dev->config;
	int ret;

	if (cfg->reset_gpio.port == NULL) {
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&cfg->reset_gpio, 1);
	if (ret < 0) {
		return ret;
	}
	k_sleep(delay);
	return gpio_pin_set_dt(&cfg->reset_gpio, 0);
}

static DEVICE_API(mipi_dbi, mipi_dbi_esp32_api) = {
	.command_write = mipi_dbi_esp32_command_write,
	.write_display = mipi_dbi_esp32_write_display,
	.reset = mipi_dbi_esp32_reset,
};

static void IRAM_ATTR mipi_dbi_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct mipi_dbi_esp32_data *data = dev->data;
	uint32_t intr_status;

	intr_status = lcd_ll_get_interrupt_status(data->hal.dev);
	if (intr_status & LCD_LL_EVENT_TRANS_DONE) {
		lcd_ll_clear_interrupt_status(data->hal.dev, LCD_LL_EVENT_TRANS_DONE);
		lcd_ll_enable_interrupt(data->hal.dev, LCD_LL_EVENT_TRANS_DONE, false);
		k_sem_give(&data->dma_sem);
	}
}

static int mipi_dbi_esp32_init(const struct device *dev)
{
	const struct mipi_dbi_esp32_config *cfg = dev->config;
	struct mipi_dbi_esp32_data *data = dev->data;
	const struct gpio_dt_spec *cs_gpio;
	lcd_hal_context_t *hal = &data->hal;
	uint32_t clock_source_freq_hz;
	int ret;

	k_mutex_init(&data->lock);
	k_sem_init(&data->dma_sem, 0, 1);

	if (cfg->dma_dev == NULL) {
		LOG_ERR("DMA device not configured");
		return -ENODEV;
	}

	if (!device_is_ready(cfg->dma_dev)) {
		LOG_ERR("DMA device not ready");
		return -ENODEV;
	}

	for (cs_gpio = cfg->cs_gpios; cs_gpio < &cfg->cs_gpios[cfg->num_cs_gpios]; cs_gpio++) {
		if (!gpio_is_ready_dt(cs_gpio)) {
			LOG_ERR("CS GPIO port %s pin %d is not ready", cs_gpio->port->name,
				cs_gpio->pin);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(cs_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure CS GPIO (%d)", ret);
			return ret;
		}
	}

	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("Reset GPIO port %s pin %d is not ready",
				cfg->reset_gpio.port->name, cfg->reset_gpio.pin);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO (%d)", ret);
			return ret;
		}
	}

	lcd_hal_init(hal, LCD_BUS_ID);
	lcd_ll_enable_clock(hal->dev, true);

	/* Select periph clock */

	ret = esp_clk_tree_src_get_freq_hz(
		cfg->clock_source, ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX, &clock_source_freq_hz);
	if (ret != ESP_OK) {
		LOG_ERR("Failed to get clock source frequency (%d)", ret);
		return -EINVAL;
	}

	lcd_ll_select_clk_src(hal->dev, cfg->clock_source);
	lcd_ll_set_group_clock_coeff(hal->dev, LCD_PERIPH_CLOCK_PRE_SCALE, 0, 0);

	data->resolution_hz = clock_source_freq_hz / LCD_PERIPH_CLOCK_PRE_SCALE;

	lcd_ll_reset(hal->dev);
	lcd_ll_fifo_reset(hal->dev);

	/* Set up interrupts */

	ret = esp_intr_alloc_intrstatus(
		cfg->irq_source,
		ESP_PRIO_TO_FLAGS(cfg->irq_priority) | ESP_INT_FLAGS_CHECK(cfg->irq_flags),
		(uint32_t)lcd_ll_get_interrupt_status_reg(hal->dev), LCD_LL_EVENT_TRANS_DONE,
		(intr_handler_t)mipi_dbi_esp32_isr, (void *)dev, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to allocate interrupt (%d)", ret);
		return ret;
	}

	lcd_ll_enable_interrupt(hal->dev, LCD_LL_EVENT_TRANS_DONE, false);
	lcd_ll_clear_interrupt_status(hal->dev, LCD_LL_EVENT_TRANS_DONE);

	/* Set parameters */

	lcd_ll_enable_rgb_mode(hal->dev, false);
	lcd_ll_enable_rgb_yuv_convert(hal->dev, false);
	lcd_ll_enable_output_always_on(hal->dev, true);

	return 0;
}

#define MIPI_DBI_ESP32_INIT_DEVICE(node_id)                                                        \
	{                                                                                          \
		.pclk_freq_hz = DT_PROP(node_id, mipi_max_frequency),                              \
		.pclk_idle_low = !DT_PROP_OR(node_id, mipi_cpol, true),                            \
		.pclk_active_neg = DT_PROP_OR(node_id, mipi_cpha, false),                          \
		.dc_idle_level = DT_PROP_OR(node_id, dc_idle_level, 0),                            \
		.dc_cmd_level = DT_PROP_OR(node_id, dc_cmd_level, 0),                              \
		.dc_dummy_level = DT_PROP_OR(node_id, dc_dummy_level, 0),                          \
		.dc_data_level = DT_PROP_OR(node_id, dc_data_level, 1),                            \
	}

static const struct mipi_dbi_esp32_device_config mipi_dbi_esp32_devices[] = {
	DT_INST_FOREACH_CHILD_SEP(0, MIPI_DBI_ESP32_INIT_DEVICE, (,))};

static const struct gpio_dt_spec mipi_dbi_esp32_cs_gpios[] = {
#if DT_INST_NODE_HAS_PROP(0, cs_gpios)
	DT_INST_FOREACH_PROP_ELEM_SEP(0, cs_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,)),
#endif
};

static const struct mipi_dbi_esp32_config mipi_dbi_esp32_config = {
	.clock_source = LCD_CLK_SRC_DEFAULT,
	.irq_source = DT_IRQ_BY_IDX(DT_INST_PARENT(0), 0, irq),
	.irq_priority = DT_IRQ_BY_IDX(DT_INST_PARENT(0), 0, priority),
	.irq_flags = DT_IRQ_BY_IDX(DT_INST_PARENT(0), 0, flags),
	.dma_dev = DEVICE_DT_GET_OR_NULL(DT_DMAS_CTLR_BY_NAME(DT_INST_PARENT(0), tx)),
	.tx_dma_channel = DT_DMAS_CELL_BY_NAME(DT_INST_PARENT(0), tx, channel),
	.devices = mipi_dbi_esp32_devices,
	.num_devices = ARRAY_SIZE(mipi_dbi_esp32_devices),
	.cs_gpios = mipi_dbi_esp32_cs_gpios,
	.num_cs_gpios = ARRAY_SIZE(mipi_dbi_esp32_cs_gpios),
	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(0, reset_gpios, {NULL}),
};

static struct mipi_dbi_esp32_data mipi_dbi_esp32_data = {0};

DEVICE_DT_INST_DEFINE(0, mipi_dbi_esp32_init, NULL, &mipi_dbi_esp32_data, &mipi_dbi_esp32_config,
		      POST_KERNEL, CONFIG_MIPI_DBI_INIT_PRIORITY, &mipi_dbi_esp32_api);
