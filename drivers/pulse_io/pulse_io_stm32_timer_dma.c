/*
 * Copyright (c) 2026 Dent IoT
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_timer_pulse_io

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/pulse_io.h>
#include <zephyr/dt-bindings/dma/stm32_dma.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <stm32_ll_tim.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_gpio.h>

LOG_MODULE_REGISTER(pulse_io_stm32, CONFIG_PULSE_IO_LOG_LEVEL);

/* Default per-channel DMA word buffer, sized for a modest WS2812 chain. */
#define PULSE_IO_STM32_DEFAULT_BUFSZ 4096

struct pulse_io_stm32_config {
	TIM_TypeDef *timer;
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	uint32_t prescaler;
	const struct device *dma_dev;
	uint32_t dma_channel;
	uint32_t dma_slot;
	struct gpio_dt_spec gpio;
	volatile uint32_t *bsrr;
	uint16_t mask;
	size_t buf_size;
};

static inline uint32_t set_mask(const struct pulse_io_stm32_config *config)
{
	return (uint32_t)config->mask;
}

static inline uint32_t reset_mask(const struct pulse_io_stm32_config *config)
{
	return (uint32_t)config->mask << 16;
}



struct pulse_io_stm32_data {
	struct k_sem done;
	int result;
	bool configured;
	bool in_use;
	bool busy;
	enum pulse_io_dir dir;
	uint32_t cell_period_ticks;
	uint32_t resolution_hz;
	bool idle_high;
	uint32_t *buf;
	struct dma_block_config block;
	struct dma_config dma_cfg;
};

static int pulse_io_stm32_get_capabilities(const struct device *dev,
					   struct pulse_io_caps *caps)
{
	ARG_UNUSED(dev);

	*caps = (struct pulse_io_caps){
		.tx_min_chunk_symbols = 1U,
		.tx_max_streaming = SIZE_MAX,
		.modes = PULSE_IO_MODE_CELL,
		.min_tick_ns = 100U,
		.max_tick_ns = 1000000U,
		.max_duration_ticks = 0xFFFFU,
		.tx_loop_max = 0U,
		.tx_channel_mask = BIT(0),
		.rx_channel_mask = 0U,
		.cell_duty_bits = 16U,
		.num_channels = 1U,
		.supports_tx = true,
		.supports_rx = false,
	};

	return 0;
}

static int pulse_io_stm32_channel_get(const struct device *dev,
				      uint8_t channel_idx,
				      struct pulse_io_channel **chan)
{
	struct pulse_io_stm32_data *data = dev->data;

	if (channel_idx != 0U) {
		return -ENODEV;
	}

	if (data->in_use) {
		return -EBUSY;
	}

	data->in_use = true;
	*chan = (struct pulse_io_channel *)(uintptr_t)1U;

	return 0;
}

static int pulse_io_stm32_channel_release(const struct device *dev,
					 struct pulse_io_channel *chan)
{
	struct pulse_io_stm32_data *data = dev->data;

	ARG_UNUSED(chan);

	if (!data->in_use) {
		return -EINVAL;
	}

	data->in_use = false;
	data->configured = false;

	return 0;
}

static int pulse_io_stm32_channel_configure(const struct device *dev,
					    struct pulse_io_channel *chan,
					    const struct pulse_io_config *cfg)
{
	const struct pulse_io_stm32_config *config = dev->config;
	struct pulse_io_stm32_data *data = dev->data;
	TIM_TypeDef *timer = config->timer;
	const struct device *clk;
	uint32_t tim_clk;
	uint32_t arr;
	int ret;

	ARG_UNUSED(chan);

	if (cfg == NULL) {
		return -EINVAL;
	}

	if (cfg->dir != PULSE_IO_DIR_TX) {
		return -ENOTSUP;
	}

	if (cfg->mode != PULSE_IO_MODE_CELL) {
		return -ENOTSUP;
	}

	if (cfg->cell_period_ticks == 0U || cfg->cell_period_ticks > 0xFFFFU) {
		return -EINVAL;
	}

	if (cfg->resolution_hz == 0U) {
		return -EINVAL;
	}

	clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (config->pclk_len < 2U) {
		LOG_ERR("timer clock source not specified");
		return -EINVAL;
	}

	ret = clock_control_get_rate(clk, (clock_control_subsys_t)&config->pclken[1],
				     &tim_clk);
	if (ret < 0) {
		LOG_ERR("timer clock rate get failed (%d)", ret);
		return ret;
	}

	/* The timer update-DMA request fires every (PSC+1)*(ARR+1) timer
	 * ticks, so the per-tick frequency is tim_clk / ((PSC+1)*(ARR+1)).
	 * Solve for ARR given the requested resolution and the configured
	 * prescaler.
	 */
	uint64_t tick_freq = (uint64_t)tim_clk /
			     (uint64_t)(config->prescaler + 1U);
	arr = (uint32_t)(tick_freq / cfg->resolution_hz);

	if (arr == 0U) {
		arr = 1U;
	}
	if (arr > 0xFFFFU) {
		LOG_ERR("timer auto-reload %u out of range for %u Hz (increase "
			"st,prescaler)", arr, cfg->resolution_hz);
		return -EINVAL;
	}

	LL_TIM_SetPrescaler(timer, config->prescaler);
	LL_TIM_SetAutoReload(timer, arr);
	if (IS_TIM_COUNTER_MODE_SELECT_INSTANCE(timer)) {
		LL_TIM_SetCounterMode(timer, LL_TIM_COUNTERMODE_UP);
	}
	if (IS_TIM_CLOCK_DIVISION_INSTANCE(timer)) {
		LL_TIM_SetClockDivision(timer, LL_TIM_CLOCKDIVISION_DIV1);
	}
#ifdef IS_TIM_REPETITION_COUNTER_INSTANCE
	if (IS_TIM_REPETITION_COUNTER_INSTANCE(timer)) {
		LL_TIM_SetRepetitionCounter(timer, 0U);
	}
#endif
	LL_TIM_EnableARRPreload(timer);
	/* Generate an update event to immediately load PSC/ARR. */
	LL_TIM_GenerateEvent_UPDATE(timer);

	data->cell_period_ticks = cfg->cell_period_ticks;
	data->resolution_hz = (uint32_t)(tick_freq / arr);
	data->idle_high = cfg->idle_high;
	data->dir = cfg->dir;
	data->configured = true;

	LOG_DBG("configured: tim_clk=%u presc=%u arr=%u -> %u Hz",
		tim_clk, config->prescaler, arr, data->resolution_hz);

	return 0;
}

static void pulse_io_stm32_dma_callback(const struct device *dma_dev,
					void *user_data, uint32_t channel,
					int status)
{
	const struct device *dev = (const struct device *) user_data;
	const struct pulse_io_stm32_config *config = dev->config;
	struct pulse_io_stm32_data *data = dev->data;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	if (status != 0) {
		data->result = -EIO;
	} else {
		data->result = 0;
	}

	/* Stop the timer and its update-DMA request. */
	LL_TIM_DisableDMAReq_UPDATE(config->timer);
	LL_TIM_DisableCounter(config->timer);
	data->busy = false;

	/* Return the line to the idle level. */
	if (data->idle_high) {
		*config->bsrr = set_mask(config);
	} else {
		*config->bsrr = reset_mask(config);
	}

	k_sem_give(&data->done);
}

/* Expand a CELL-mode stream into BSRR set/reset words. Returns the number
 * of words written, or 0 on overflow.
 */
static size_t pulse_io_stm32_encode(const struct device *dev,
				    const struct pulse_cell *cells, size_t count,
				    uint32_t *buf, size_t cap)
{
	const struct pulse_io_stm32_config *config = dev->config;
	struct pulse_io_stm32_data *data = dev->data;
	size_t n = 0U;
	uint32_t period = data->cell_period_ticks;
	uint32_t set = set_mask(config);
	uint32_t rst = reset_mask(config);

	for (size_t i = 0U; i < count; i++) {
		uint32_t duty = cells[i].duty;
		uint32_t low;

		if (duty > period) {
			duty = period;
		}
		low = period - duty;

		/* A whole cell (duty + low) must fit; report overflow if not. */
		if (n + period > cap) {
			LOG_ERR("DMA buffer overflow at cell %zu "
				"(need more buffer-size)", i);
			return 0U;
		}

		for (uint32_t t = 0U; t < duty; t++) {
			buf[n++] = set;
		}
		for (uint32_t t = 0U; t < low; t++) {
			buf[n++] = rst;
		}
	}

	return n;
}

static int pulse_io_stm32_transmit_sync(const struct device *dev,
					struct pulse_io_channel *chan,
					const struct pulse_io_tx_req *req,
					k_timeout_t timeout)
{
	const struct pulse_io_stm32_config *config = dev->config;
	struct pulse_io_stm32_data *data = dev->data;
	size_t words;
	int ret;

	ARG_UNUSED(chan);

	if (!data->configured || req == NULL || req->cells == NULL ||
	    req->count == 0U) {
		return -EINVAL;
	}

	if (req->loop_count != 0U) {
		return -ENOTSUP;
	}

	words = pulse_io_stm32_encode(dev, req->cells, req->count,
				     data->buf, config->buf_size);
	if (words == 0U) {
		return -ENOMEM;
	}

	k_sem_reset(&data->done);

	/* Arm the DMA transfer: memory -> BSRR, 32-bit, memory increment,
	 * peripheral fixed. The DMA is triggered by the timer update event.
	 */
	memset(&data->block, 0, sizeof(data->block));
	data->block.source_address = (uint32_t)(uintptr_t)data->buf;
	data->block.dest_address   = (uint32_t)(uintptr_t)config->bsrr;
	data->block.block_size = words * sizeof(uint32_t);
	data->block.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	data->block.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	memset(&data->dma_cfg, 0, sizeof(data->dma_cfg));
	data->dma_cfg.dma_slot = config->dma_slot;
	data->dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	data->dma_cfg.source_data_size = sizeof(uint32_t);
	data->dma_cfg.dest_data_size = sizeof(uint32_t);
	data->dma_cfg.source_burst_length = sizeof(uint32_t);
	data->dma_cfg.dest_burst_length = sizeof(uint32_t);
	data->dma_cfg.block_count = 1U;
	data->dma_cfg.head_block = &data->block;
	data->dma_cfg.dma_callback = pulse_io_stm32_dma_callback;
	data->dma_cfg.user_data = (void *)dev;

	ret = dma_config(config->dma_dev, config->dma_channel, &data->dma_cfg);
	if (ret != 0) {
		LOG_ERR("dma_config failed (%d)", ret);
		return ret;
	}

	ret = dma_start(config->dma_dev, config->dma_channel);
	if (ret != 0) {
		LOG_ERR("dma_start failed (%d)", ret);
		return ret;
	}

	/* Enable the timer update-DMA request and start the counter; the
	 * first update event triggers the first DMA word.
	 */
	data->busy = true;
	LL_TIM_SetCounter(config->timer, 0U);
	LL_TIM_EnableDMAReq_UPDATE(config->timer);
	LL_TIM_EnableCounter(config->timer);

	ret = k_sem_take(&data->done, timeout);
	if (ret != 0) {
		(void)dma_stop(config->dma_dev, config->dma_channel);
		LL_TIM_DisableDMAReq_UPDATE(config->timer);
		LL_TIM_DisableCounter(config->timer);
		data->busy = false;
		return -ETIMEDOUT;
	}

	return data->result;
}

static int pulse_io_stm32_receive_sync(const struct device *dev,
				      struct pulse_io_channel *chan,
				      const struct pulse_io_rx_req *req,
				      size_t *count, k_timeout_t timeout)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);
	ARG_UNUSED(req);
	ARG_UNUSED(count);
	ARG_UNUSED(timeout);

	return -ENOTSUP;
}

static int pulse_io_stm32_stop(const struct device *dev,
			      struct pulse_io_channel *chan)
{
	const struct pulse_io_stm32_config *config = dev->config;
	struct pulse_io_stm32_data *data = dev->data;

	ARG_UNUSED(chan);

	if (data->busy) {
		(void)dma_stop(config->dma_dev, config->dma_channel);
		LL_TIM_DisableDMAReq_UPDATE(config->timer);
		LL_TIM_DisableCounter(config->timer);
		data->busy = false;
		k_sem_reset(&data->done);
	}

	return 0;
}

static DEVICE_API(pulse_io, pulse_io_stm32_api) = {
	.get_capabilities = pulse_io_stm32_get_capabilities,
	.channel_get = pulse_io_stm32_channel_get,
	.channel_release = pulse_io_stm32_channel_release,
	.channel_configure = pulse_io_stm32_channel_configure,
	.transmit_sync = pulse_io_stm32_transmit_sync,
	.receive_sync = pulse_io_stm32_receive_sync,
	.stop = pulse_io_stm32_stop,
};

static int pulse_io_stm32_init(const struct device *dev)
{
	const struct pulse_io_stm32_config *config = dev->config;
	struct pulse_io_stm32_data *data = dev->data;
	const struct device *clk;
	int ret;

	k_sem_init(&data->done, 0, 1);

	/* The static buffer was assigned at build time; nothing else to do. */

	if (!device_is_ready(config->dma_dev)) {
		LOG_ERR("DMA device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->gpio)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->gpio, GPIO_OUTPUT);
	if (ret != 0) {
		LOG_ERR("GPIO pin configure failed (%d)", ret);
		return ret;
	}

	/* Enable the timer clock. */
	clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(clk, (clock_control_subsys_t)&config->pclken[0]);
	if (ret < 0) {
		LOG_ERR("timer clock enable failed (%d)", ret);
		return ret;
	}

	if (config->pclk_len > 1U) {
		ret = clock_control_configure(clk,
			(clock_control_subsys_t)&config->pclken[1], NULL);
		if (ret != 0) {
			LOG_ERR("timer clock source config failed (%d)", ret);
			return ret;
		}
	}

	/* Park the line at idle (low) before any transfer. */
	*config->bsrr = reset_mask(config);

	return 0;
}

#define PULSE_IO_STM32_INIT(inst)                                                   \
	static const struct stm32_pclken                                            \
		pulse_io_stm32_pclken_##inst[DT_NUM_CLOCKS(                         \
			DT_PHANDLE(DT_DRV_INST(inst), timer))] =                    \
			STM32_DT_CLOCKS(DT_PHANDLE(DT_DRV_INST(inst), timer));      \
\
	static uint32_t pulse_io_stm32_buf_##inst                                   \
		[DT_INST_PROP_OR(inst, buffer_size, PULSE_IO_STM32_DEFAULT_BUFSZ)]; \
                                                                                    \
	static struct pulse_io_stm32_data pulse_io_stm32_data_##inst = {            \
		.buf = pulse_io_stm32_buf_##inst,                                   \
	};                                                                          \
\
	static const struct pulse_io_stm32_config pulse_io_stm32_cfg_##inst = {     \
		.timer = (TIM_TypeDef *)DT_REG_ADDR(                                \
			DT_PHANDLE(DT_DRV_INST(inst), timer)),                      \
		.pclken = pulse_io_stm32_pclken_##inst,                             \
		.pclk_len = DT_NUM_CLOCKS(DT_PHANDLE(DT_DRV_INST(inst), timer)),    \
		.prescaler = DT_INST_PROP_OR(inst, st_prescaler, 0),                \
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR(inst)),                  \
		.dma_channel = DT_INST_DMAS_CELL_BY_NAME(inst, tx, channel),        \
		.dma_slot = DT_INST_DMAS_CELL_BY_NAME(inst, tx, slot),              \
		.gpio = GPIO_DT_SPEC_INST_GET(inst, gpios),                         \
		.bsrr = (volatile uint32_t *)                                       \
			&((GPIO_TypeDef *)DT_REG_ADDR(DT_GPIO_CTLR(                 \
				DT_DRV_INST(inst), gpios)))->BSRR,                  \
		.mask = BIT(DT_GPIO_PIN(DT_DRV_INST(inst), gpios)),                 \
		.buf_size = DT_INST_PROP_OR(inst, buffer_size,                      \
					    PULSE_IO_STM32_DEFAULT_BUFSZ),          \
	};                                                                          \
\
	DEVICE_DT_INST_DEFINE(inst, pulse_io_stm32_init, NULL,                      \
			      &pulse_io_stm32_data_##inst,                          \
			      &pulse_io_stm32_cfg_##inst, POST_KERNEL,              \
			      CONFIG_PULSE_IO_INIT_PRIORITY,                        \
			      &pulse_io_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(PULSE_IO_STM32_INIT)
