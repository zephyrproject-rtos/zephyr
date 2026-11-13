/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_lp_i2s

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <soc/soc_caps.h>
#include <soc/lp_gpio_sig_map.h>
#include <hal/lp_i2s_hal.h>
#include <hal/lp_i2s_ll.h>
#include <hal/rtc_io_hal.h>
#include <hal/rtc_io_ll.h>
#include <hal/rtc_io_periph.h>
#include <esp_clk_tree.h>
#include <esp_private/periph_ctrl.h>

#include <errno.h>

LOG_MODULE_REGISTER(i2s_esp32_lp, CONFIG_I2S_LOG_LEVEL);

#define I2S_ESP32_LP_INTR_MASK                                                                     \
	(LP_I2S_LL_EVENT_RX_MEM_THRESHOLD_INT | LP_I2S_LL_EVENT_RX_HUNG_INT_INT |                  \
	 LP_I2S_LL_EVENT_RX_FIFOMEM_UDF_INT)

#define I2S_ESP32_LP_CLK_DIV_MIN 2U
#define I2S_ESP32_LP_CLK_DIV_MAX 256U
#define I2S_ESP32_LP_BCK_DIV_MAX 64U

struct i2s_esp32_lp_item {
	void *buffer;
	size_t size;
};

struct i2s_esp32_lp_config {
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	int bck_pin;
	int ws_pin;
	int sd_pin;
	int irq_source;
	int irq_priority;
	int irq_flags;
};

struct i2s_esp32_lp_data {
	lp_i2s_hal_context_t hal;
	struct i2s_config cfg;
	enum i2s_state state;
	bool configured;
	void *mem_block;
	size_t offset;
	struct k_msgq rx_queue;
	struct i2s_esp32_lp_item rx_items[CONFIG_I2S_ESP32_LP_RX_BLOCK_COUNT];
};

static void i2s_esp32_lp_fail(const struct device *dev)
{
	struct i2s_esp32_lp_data *data = dev->data;
	lp_i2s_dev_t *hw = data->hal.dev;

	lp_i2s_ll_rx_stop(hw);
	lp_i2s_ll_rx_enable_interrupt(hw, I2S_ESP32_LP_INTR_MASK, false);
	if (data->mem_block != NULL) {
		k_mem_slab_free(data->cfg.mem_slab, data->mem_block);
		data->mem_block = NULL;
		data->offset = 0U;
	}
	data->state = I2S_STATE_ERROR;
}

static void i2s_esp32_lp_queue_drop(struct i2s_esp32_lp_data *data)
{
	struct i2s_esp32_lp_item item;

	while (k_msgq_get(&data->rx_queue, &item, K_NO_WAIT) == 0) {
		k_mem_slab_free(data->cfg.mem_slab, item.buffer);
	}
}

/* Copy FIFO words into the current slab block. Several interrupts fill one block when the
 * block is larger than the peripheral FIFO threshold.
 */
static void i2s_esp32_lp_drain(const struct device *dev)
{
	struct i2s_esp32_lp_data *data = dev->data;
	lp_i2s_dev_t *hw = data->hal.dev;

	if ((data->state != I2S_STATE_RUNNING) && (data->state != I2S_STATE_STOPPING)) {
		return;
	}

	for (;;) {
		uint32_t avail;
		size_t space;
		size_t count;
		struct i2s_esp32_lp_item item;
		int ret;

		if (data->mem_block == NULL) {
			i2s_esp32_lp_fail(dev);
			return;
		}

		avail = lp_i2s_ll_get_rx_mem_fifo_cnt(hw) & ~(uint32_t)3U;
		if (avail == 0U) {
			return;
		}

		space = data->cfg.block_size - data->offset;
		count = MIN(avail, space) & ~(size_t)3U;
		if (count == 0U) {
			return;
		}

		lp_i2s_ll_read_buffer(hw, (uint8_t *)data->mem_block + data->offset, count);
		data->offset += count;
		if (data->offset != data->cfg.block_size) {
			continue;
		}

		item.buffer = data->mem_block;
		item.size = data->offset;
		data->mem_block = NULL;
		data->offset = 0U;
		if (k_msgq_put(&data->rx_queue, &item, K_NO_WAIT) != 0) {
			k_mem_slab_free(data->cfg.mem_slab, item.buffer);
			i2s_esp32_lp_fail(dev);
			return;
		}

		if (data->state == I2S_STATE_STOPPING) {
			lp_i2s_ll_rx_stop(hw);
			lp_i2s_ll_rx_enable_interrupt(hw, I2S_ESP32_LP_INTR_MASK, false);
			data->state = I2S_STATE_READY;
			return;
		}

		ret = k_mem_slab_alloc(data->cfg.mem_slab, &data->mem_block, K_NO_WAIT);
		if (ret != 0) {
			i2s_esp32_lp_fail(dev);
			return;
		}
	}
}

static void i2s_esp32_lp_isr(void *arg)
{
	const struct device *dev = arg;
	struct i2s_esp32_lp_data *data = dev->data;
	lp_i2s_dev_t *hw = data->hal.dev;
	uint32_t status;

	status = lp_i2s_ll_rx_get_interrupt_status(hw, false);
	if ((status & (LP_I2S_LL_EVENT_RX_HUNG_INT_INT | LP_I2S_LL_EVENT_RX_FIFOMEM_UDF_INT)) !=
	    0U) {
		lp_i2s_ll_rx_clear_interrupt_status(hw, status);
		i2s_esp32_lp_fail(dev);
		return;
	}

	if ((status & LP_I2S_LL_EVENT_RX_MEM_THRESHOLD_INT) != 0U) {
		i2s_esp32_lp_drain(dev);
		lp_i2s_ll_rx_clear_interrupt_status(hw, LP_I2S_LL_EVENT_RX_MEM_THRESHOLD_INT);
	}
}

static int i2s_esp32_lp_config_pin(int pin, bool output)
{
	int rtc_io;

	if ((pin < 0) || (pin >= SOC_GPIO_PIN_COUNT) || (rtc_io_num_map[pin] < 0)) {
		return -EINVAL;
	}

	rtc_io = rtc_io_num_map[pin];
#if SOC_LP_IO_CLOCK_IS_INDEPENDENT
	rtcio_ll_enable_io_clock(true);
#endif
	rtcio_hal_function_select(rtc_io, RTCIO_LL_FUNC_RTC);
	rtcio_hal_set_direction(rtc_io,
				output ? RTC_GPIO_MODE_OUTPUT_ONLY : RTC_GPIO_MODE_INPUT_ONLY);
	rtcio_hal_iomux_func_sel(rtc_io, RTCIO_LL_PIN_FUNC);
	return rtc_io;
}

static int i2s_esp32_lp_route_pins(const struct device *dev, bool controller, bool bck_inv)
{
	const struct i2s_esp32_lp_config *cfg = dev->config;
	int bck;
	int ws;
	int sd;

	bck = i2s_esp32_lp_config_pin(cfg->bck_pin, controller);
	ws = i2s_esp32_lp_config_pin(cfg->ws_pin, controller);
	sd = i2s_esp32_lp_config_pin(cfg->sd_pin, false);
	if ((bck < 0) || (ws < 0) || (sd < 0)) {
		return -EINVAL;
	}

	if (controller) {
		rtcio_ll_set_output_signal_matrix_source(bck, LP_I2S_I_BCK_PAD_OUT_IDX, bck_inv);
		rtcio_ll_set_output_signal_matrix_source(ws, LP_I2S_I_WS_PAD_OUT_IDX, false);
	} else {
		rtcio_ll_set_input_signal_matrix_source(bck, LP_I2S_I_BCK_PAD_IN_IDX, bck_inv);
		rtcio_ll_set_input_signal_matrix_source(ws, LP_I2S_I_WS_PAD_IN_IDX, false);
	}
	rtcio_ll_set_input_signal_matrix_source(sd, LP_I2S_I_SD_PAD_IN_IDX, false);
	return 0;
}

static void i2s_esp32_lp_fraction_clear(void)
{
	LPPERI.lp_i2s_rxclk_div_xyz.lp_i2s_rx_clkm_div_yn1 = 0;
	LPPERI.lp_i2s_rxclk_div_xyz.lp_i2s_rx_clkm_div_x = 0;
	LPPERI.lp_i2s_rxclk_div_xyz.lp_i2s_rx_clkm_div_z = 0;
	LPPERI.lp_i2s_rxclk_div_xyz.lp_i2s_rx_clkm_div_y = 1;
}

static int i2s_esp32_lp_set_controller_clock(lp_i2s_dev_t *hw, uint32_t bclk)
{
	uint32_t src = 0U;
	uint32_t best_n = 0U;
	uint32_t best_bck = 0U;
	uint32_t best_err = UINT32_MAX;
	bool exact = false;
	int err;

	err = esp_clk_tree_src_get_freq_hz((soc_module_clk_t)LP_I2S_CLK_SRC_XTAL_D2,
					   ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &src);
	if ((err != 0) || (src == 0U) || (bclk == 0U) || (bclk > src)) {
		return -EINVAL;
	}

	for (uint32_t div = I2S_ESP32_LP_CLK_DIV_MIN; (div <= I2S_ESP32_LP_CLK_DIV_MAX) && !exact;
	     div++) {
		for (uint32_t bck = 1U; bck <= I2S_ESP32_LP_BCK_DIV_MAX; bck++) {
			uint32_t got = (uint32_t)(src / ((uint64_t)div * bck));
			uint32_t diff = (got > bclk) ? (got - bclk) : (bclk - got);

			if (diff < best_err) {
				best_err = diff;
				best_n = div;
				best_bck = bck;
				exact = (diff == 0U);
			}
		}
	}

	if ((best_n == 0U) || (best_err > (bclk / 100U))) {
		LOG_ERR("BCLK %u Hz is not reachable from %u Hz", bclk, src);
		return -EINVAL;
	}

	PERIPH_RCC_ATOMIC()
	{
		lp_i2s_ll_select_rx_clk_source(0, LP_I2S_CLK_SRC_XTAL_D2);
		lp_i2s_ll_clk_source_div_num(0, best_n);
		i2s_esp32_lp_fraction_clear();
	}
	lp_i2s_ll_rx_set_bck_div_num(hw, best_bck);
	return 0;
}

static int i2s_esp32_lp_set_target_clock(lp_i2s_dev_t *hw)
{
	PERIPH_RCC_ATOMIC()
	{
		lp_i2s_ll_select_rx_clk_source(0, LP_I2S_CLK_SRC_XTAL_D2);
		lp_i2s_ll_clk_source_div_num(0, 2);
		lp_i2s_ll_rx_set_raw_clk_div(0, 0, 1);
	}
	lp_i2s_ll_rx_set_bck_div_num(hw, 1);
	return 0;
}

static uint8_t i2s_esp32_lp_channels(const struct i2s_config *cfg)
{
	i2s_fmt_t fmt = cfg->format & I2S_FMT_DATA_FORMAT_MASK;

	if (fmt == I2S_FMT_DATA_FORMAT_I2S) {
		return 2U;
	}

	return cfg->channels;
}

static int i2s_esp32_lp_apply_format(lp_i2s_dev_t *hw, const struct i2s_config *cfg)
{
	i2s_fmt_t fmt = cfg->format & I2S_FMT_DATA_FORMAT_MASK;
	uint8_t channels = i2s_esp32_lp_channels(cfg);
	bool msb_shift;
	lp_i2s_ll_rx_ws_pol_t ws_pol;
	lp_i2s_ll_rx_alignment_t align;
	bool frame_inv;

	switch (fmt) {
	case I2S_FMT_DATA_FORMAT_I2S:
		msb_shift = true;
		ws_pol = LP_I2S_LL_RX_WS_POL_NEGATIVE;
		align = LP_I2S_LL_RX_LEFT_ALIGN;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		msb_shift = false;
		ws_pol = LP_I2S_LL_RX_WS_POL_POSITIVE;
		align = LP_I2S_LL_RX_LEFT_ALIGN;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		msb_shift = false;
		ws_pol = LP_I2S_LL_RX_WS_POL_POSITIVE;
		align = LP_I2S_LL_RX_RIGHT_ALIGN;
		break;
	default:
		return -EINVAL;
	}

	frame_inv = (cfg->format & I2S_FMT_FRAME_CLK_INV) != 0U;
	if (frame_inv) {
		ws_pol = (ws_pol == LP_I2S_LL_RX_WS_POL_NEGATIVE) ? LP_I2S_LL_RX_WS_POL_POSITIVE
								  : LP_I2S_LL_RX_WS_POL_NEGATIVE;
	}

	lp_i2s_ll_rx_enable_tdm(hw);
	lp_i2s_ll_rx_enable_24_fill(hw, false);
	lp_i2s_ll_rx_set_sample_bit(hw, cfg->word_size, cfg->word_size);
	lp_i2s_ll_rx_set_half_sample_bits(hw, cfg->word_size);
	lp_i2s_ll_rx_set_ws_width(hw, cfg->word_size);
	lp_i2s_ll_rx_enable_msb_shift(hw, msb_shift);
	lp_i2s_ll_rx_set_ws_pol(hw, ws_pol);
	lp_i2s_ll_rx_set_alignment_mode(hw, align);
	lp_i2s_ll_rx_set_bit_order(hw, ((cfg->format & I2S_FMT_DATA_ORDER_LSB) != 0U)
					       ? LP_I2S_LL_RX_BIT_ORDER_SMALL_ENDIAN
					       : LP_I2S_LL_RX_BIT_ORDER_BIG_ENDIAN);
	lp_i2s_ll_rx_set_endian(hw, LP_I2S_LL_RX_SMALL_ENDIAN);
	lp_i2s_ll_rx_enable_mono_mode(hw, channels == 1U);
	lp_i2s_ll_rx_select_std_chan(hw, (channels == 1U) ? I2S_STD_SLOT_LEFT : I2S_STD_SLOT_BOTH);
	return 0;
}

static int i2s_esp32_lp_check_config(const struct i2s_config *cfg, uint8_t *channels,
				     size_t *frame_bytes)
{
	i2s_fmt_t fmt;
	bool bit_target;
	bool frame_target;
	uint8_t used;
	size_t frame;

	if (cfg == NULL) {
		return -EINVAL;
	}

	fmt = cfg->format & I2S_FMT_DATA_FORMAT_MASK;
	if ((fmt == I2S_FMT_DATA_FORMAT_PCM_SHORT) || (fmt == I2S_FMT_DATA_FORMAT_PCM_LONG)) {
		return -EINVAL;
	}
	if ((fmt != I2S_FMT_DATA_FORMAT_I2S) && (fmt != I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED) &&
	    (fmt != I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED)) {
		return -EINVAL;
	}
	if ((cfg->word_size != 8U) && (cfg->word_size != 16U)) {
		return -EINVAL;
	}
	if (((cfg->options & I2S_OPT_BIT_CLK_GATED) != 0U) ||
	    ((cfg->options & I2S_OPT_PINGPONG) != 0U) ||
	    ((cfg->options & I2S_OPT_LOOPBACK) != 0U)) {
		return -EINVAL;
	}

	bit_target = (cfg->options & I2S_OPT_BIT_CLK_TARGET) != 0U;
	frame_target = (cfg->options & I2S_OPT_FRAME_CLK_TARGET) != 0U;
	if (bit_target != frame_target) {
		return -EINVAL;
	}

	used = i2s_esp32_lp_channels(cfg);
	if ((used != 1U) && (used != 2U)) {
		return -EINVAL;
	}

	frame = (size_t)used * (cfg->word_size / 8U);
	if ((cfg->mem_slab == NULL) || (cfg->block_size == 0U) || ((cfg->block_size % 4U) != 0U) ||
	    ((cfg->block_size % frame) != 0U) ||
	    (cfg->block_size != cfg->mem_slab->info.block_size) ||
	    (cfg->mem_slab->info.num_blocks < 2U) || (cfg->frame_clk_freq == 0U)) {
		return -EINVAL;
	}

	*channels = used;
	*frame_bytes = frame;
	return 0;
}

static int i2s_esp32_lp_configure(const struct device *dev, enum i2s_dir dir,
				  const struct i2s_config *cfg)
{
	struct i2s_esp32_lp_data *data = dev->data;
	lp_i2s_dev_t *hw = data->hal.dev;
	uint8_t channels;
	size_t frame_bytes;
	bool controller;
	uint32_t chunk;
	uint32_t words;
	unsigned int key;
	int ret;

	if (dir != I2S_DIR_RX) {
		return -EINVAL;
	}

	key = irq_lock();
	if ((data->state != I2S_STATE_NOT_READY) && (data->state != I2S_STATE_READY)) {
		irq_unlock(key);
		return -EIO;
	}
	irq_unlock(key);

	if ((cfg != NULL) && (cfg->frame_clk_freq == 0U)) {
		key = irq_lock();
		if (data->configured) {
			i2s_esp32_lp_queue_drop(data);
		}
		data->configured = false;
		data->state = I2S_STATE_NOT_READY;
		irq_unlock(key);
		return 0;
	}

	ret = i2s_esp32_lp_check_config(cfg, &channels, &frame_bytes);
	if (ret != 0) {
		return ret;
	}
	ARG_UNUSED(frame_bytes);

	controller = (cfg->options & I2S_OPT_BIT_CLK_TARGET) == 0U;
	if (controller) {
		ret = i2s_esp32_lp_set_controller_clock(hw, cfg->frame_clk_freq * channels *
								    cfg->word_size);
	} else {
		ret = i2s_esp32_lp_set_target_clock(hw);
	}
	if (ret != 0) {
		return ret;
	}

	ret = i2s_esp32_lp_route_pins(dev, controller, (cfg->format & I2S_FMT_BIT_CLK_INV) != 0U);
	if (ret != 0) {
		return ret;
	}

	ret = i2s_esp32_lp_apply_format(hw, cfg);
	if (ret != 0) {
		return ret;
	}

	lp_i2s_ll_set_rx_master_slave_mode(hw, !controller);
	chunk = MIN(cfg->block_size, (size_t)LP_I2S_LL_RX_MEM_THRESH_BYTES_MAX);
	words = chunk / 4U;
	if (words > 0U) {
		words--;
	}
	lp_i2s_ll_set_rx_mem_threshold(hw, words);

	key = irq_lock();
	if ((data->state != I2S_STATE_NOT_READY) && (data->state != I2S_STATE_READY)) {
		irq_unlock(key);
		return -EIO;
	}
	if (data->configured) {
		i2s_esp32_lp_queue_drop(data);
	}
	data->cfg = *cfg;
	data->configured = true;
	data->state = I2S_STATE_READY;
	irq_unlock(key);
	return 0;
}

static const struct i2s_config *i2s_esp32_lp_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_esp32_lp_data *data = dev->data;

	if ((dir != I2S_DIR_RX) || !data->configured) {
		return NULL;
	}

	return &data->cfg;
}

static int i2s_esp32_lp_trigger(const struct device *dev, enum i2s_dir dir,
				enum i2s_trigger_cmd cmd)
{
	struct i2s_esp32_lp_data *data = dev->data;
	lp_i2s_dev_t *hw = data->hal.dev;
	unsigned int key;
	void *block = NULL;
	int ret = 0;

	if (dir != I2S_DIR_RX) {
		return -EINVAL;
	}

	if ((cmd == I2S_TRIGGER_START) && !data->configured) {
		return -EIO;
	}

	if (cmd == I2S_TRIGGER_START) {
		ret = k_mem_slab_alloc(data->cfg.mem_slab, &block, K_NO_WAIT);
		if (ret != 0) {
			return -ENOMEM;
		}
	}

	key = irq_lock();
	if (data->state == I2S_STATE_NOT_READY) {
		irq_unlock(key);
		ret = -EIO;
		goto out_free;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (data->state != I2S_STATE_READY) {
			ret = -EIO;
			break;
		}
		data->mem_block = block;
		block = NULL;
		data->offset = 0U;
		data->state = I2S_STATE_RUNNING;
		lp_i2s_ll_rx_reset(hw);
		lp_i2s_ll_rx_reset_fifo(hw);
		lp_i2s_ll_rx_reset_fifo_mem(hw);
		lp_i2s_ll_rx_clear_interrupt_status(hw, I2S_ESP32_LP_INTR_MASK);
		lp_i2s_ll_rx_enable_interrupt(hw, I2S_ESP32_LP_INTR_MASK, true);
		irq_unlock(key);
		lp_i2s_ll_rx_start(hw);
		return 0;
	case I2S_TRIGGER_STOP:
	case I2S_TRIGGER_DRAIN:
		if (data->state != I2S_STATE_RUNNING) {
			ret = -EIO;
			break;
		}
		data->state = I2S_STATE_STOPPING;
		break;
	case I2S_TRIGGER_DROP:
		lp_i2s_ll_rx_stop(hw);
		lp_i2s_ll_rx_enable_interrupt(hw, I2S_ESP32_LP_INTR_MASK, false);
		if (data->mem_block != NULL) {
			k_mem_slab_free(data->cfg.mem_slab, data->mem_block);
			data->mem_block = NULL;
			data->offset = 0U;
		}
		if (data->configured) {
			i2s_esp32_lp_queue_drop(data);
			data->state = I2S_STATE_READY;
		}
		break;
	case I2S_TRIGGER_PREPARE:
		if (data->state != I2S_STATE_ERROR) {
			ret = -EIO;
			break;
		}
		data->state = I2S_STATE_READY;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	irq_unlock(key);
out_free:
	if (block != NULL) {
		k_mem_slab_free(data->cfg.mem_slab, block);
	}
	return ret;
}

static int i2s_esp32_lp_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct i2s_esp32_lp_data *data = dev->data;
	struct i2s_esp32_lp_item item;
	k_timeout_t wait;
	int err;

	if (!data->configured || (data->state == I2S_STATE_NOT_READY)) {
		return -EIO;
	}

	if ((data->state == I2S_STATE_ERROR) || (data->cfg.timeout == 0)) {
		wait = K_NO_WAIT;
	} else {
		wait = SYS_TIMEOUT_MS(data->cfg.timeout);
	}

	err = k_msgq_get(&data->rx_queue, &item, wait);
	if (err == 0) {
		*mem_block = item.buffer;
		*size = item.size;
		return 0;
	}

	if ((data->state == I2S_STATE_ERROR) || (data->state == I2S_STATE_NOT_READY)) {
		return -EIO;
	}
	if (data->cfg.timeout == 0) {
		return -EBUSY;
	}
	return -EAGAIN;
}

static int i2s_esp32_lp_write(const struct device *dev, void *mem_block, size_t size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mem_block);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

static int i2s_esp32_lp_init(const struct device *dev)
{
	const struct i2s_esp32_lp_config *cfg = dev->config;
	struct i2s_esp32_lp_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret != 0) {
		LOG_ERR("failed to enable LP I2S clock (%d)", ret);
		return ret;
	}

	k_msgq_init(&data->rx_queue, (char *)data->rx_items, sizeof(struct i2s_esp32_lp_item),
		    CONFIG_I2S_ESP32_LP_RX_BLOCK_COUNT);

	lp_i2s_hal_init(&data->hal, 0);
	lp_i2s_ll_enable_mem(0, true);
	lp_i2s_ll_clk_gate_en(data->hal.dev, true);
	lp_i2s_ll_rx_mem_clk_gate_en(data->hal.dev, true);
	lp_i2s_ll_rx_reg_clk_gate_en(data->hal.dev, true);
	lp_i2s_ll_rx_reset(data->hal.dev);
	lp_i2s_ll_rx_reset_fifo(data->hal.dev);
	lp_i2s_ll_rx_reset_fifo_mem(data->hal.dev);
	lp_i2s_ll_set_rx_stop_mode(data->hal.dev, LP_I2S_LL_RX_STOP_MODE_START_CLEAR);
	data->state = I2S_STATE_NOT_READY;

	ret = esp_intr_alloc(cfg->irq_source,
			     ESP_PRIO_TO_FLAGS(cfg->irq_priority) |
				     ESP_INT_FLAGS_CHECK(cfg->irq_flags),
			     i2s_esp32_lp_isr, (void *)dev, NULL);
	if (ret != 0) {
		LOG_ERR("failed to allocate LP I2S interrupt (%d)", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(i2s, i2s_esp32_lp_driver_api) = {
	.configure = i2s_esp32_lp_configure,
	.config_get = i2s_esp32_lp_config_get,
	.trigger = i2s_esp32_lp_trigger,
	.read = i2s_esp32_lp_read,
	.write = i2s_esp32_lp_write,
};

#define I2S_ESP32_LP_INIT(n)                                                                       \
	static struct i2s_esp32_lp_data i2s_esp32_lp_data_##n;                                     \
	static const struct i2s_esp32_lp_config i2s_esp32_lp_cfg_##n = {                           \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, offset),            \
		.bck_pin = DT_INST_PROP(n, bck_pin),                                               \
		.ws_pin = DT_INST_PROP(n, ws_pin),                                                 \
		.sd_pin = DT_INST_PROP(n, sd_pin),                                                 \
		.irq_source = DT_INST_IRQ_BY_IDX(n, 0, irq),                                       \
		.irq_priority = DT_INST_IRQ_BY_IDX(n, 0, priority),                                \
		.irq_flags = DT_INST_IRQ_BY_IDX(n, 0, flags),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, i2s_esp32_lp_init, NULL, &i2s_esp32_lp_data_##n,                  \
			      &i2s_esp32_lp_cfg_##n, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,        \
			      &i2s_esp32_lp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2S_ESP32_LP_INIT)
