/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_itim32

/**
 * @file
 * @brief Nuvoton NPCM internal 32-bit timer (ITIM32) counter driver
 *
 * The ITIM32 counts down from the value in its counter register. When it
 * reaches zero it sets the timeout status, raises an interrupt and reloads the
 * counter register value. The driver therefore exposes a down-counting counter
 * whose top value is the reload value, with a callback on every reload. The
 * timer has no compare register, so no alarm channel is provided.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(counter_npcm_itim32, CONFIG_COUNTER_LOG_LEVEL);

/* ITIM32 registers, offsets from the module base */
#define NPCM_ITPRE   0x01U /* Prescaler, divides the input clock by PRE_8 + 1 */
#define NPCM_ITCTS   0x04U /* Control and status */
#define NPCM_ITCNT32 0x08U /* Counter, counts down to zero then reloads */

/* ITCTS fields */
#define NPCM_ITCTS_ITEN   7 /* Module enable; reads back the actual state */
#define NPCM_ITCTS_CKSEL  4 /* Input clock: 0 APB2 clock, 1 32 kHz LFCLK */
#define NPCM_ITCTS_TO_WUE 3 /* Wake-up event on timeout, LFCLK only */
#define NPCM_ITCTS_TO_IE  2 /* Interrupt on timeout */
#define NPCM_ITCTS_TO_STS 0 /* Timeout status, write 1 to clear */

#define NPCM_ITIM32_MAX_PRESCALER 256U
#define NPCM_ITIM32_LFCLK_FREQ    32768U

struct counter_npcm_itim32_config {
	struct counter_config_info info;
	mm_reg_t base;
	const struct device *clk_dev;
	uint32_t clk_id;
	/* Divider of the input clock, 1 to 256 */
	uint16_t prescaler;
	void (*irq_config_func)(void);
};

struct counter_npcm_itim32_data {
	struct k_spinlock lock;
	/* Counter frequency after the prescaler */
	uint32_t freq;
	/* Reload value: the counter counts down from it to zero */
	uint32_t top;
	counter_top_callback_t top_callback;
	void *top_user_data;
};

static uint8_t itim32_read_ctrl(const struct device *dev)
{
	const struct counter_npcm_itim32_config *config = dev->config;

	return sys_read8(config->base + NPCM_ITCTS);
}

static void itim32_write_ctrl(const struct device *dev, uint8_t val)
{
	const struct counter_npcm_itim32_config *config = dev->config;

	sys_write8(val, config->base + NPCM_ITCTS);
}

static bool itim32_is_enabled(const struct device *dev)
{
	return IS_BIT_SET(itim32_read_ctrl(dev), NPCM_ITCTS_ITEN);
}

static void itim32_clear_timeout(const struct device *dev)
{
	/* Writing 1 clears TO_STS; the other bits are written back unchanged */
	itim32_write_ctrl(dev, itim32_read_ctrl(dev) | BIT(NPCM_ITCTS_TO_STS));
}

/* Disable the module. The hardware resets the counter and clears TO_STS. */
static void itim32_disable(const struct device *dev)
{
	itim32_write_ctrl(dev, itim32_read_ctrl(dev) & ~BIT(NPCM_ITCTS_ITEN));
	while (itim32_is_enabled(dev)) {
	}
}

/* Load the reload value and enable the module; it must be disabled */
static void itim32_enable(const struct device *dev, uint32_t top)
{
	const struct counter_npcm_itim32_config *config = dev->config;

	sys_write32(top, config->base + NPCM_ITCNT32);
	itim32_write_ctrl(dev, itim32_read_ctrl(dev) | BIT(NPCM_ITCTS_ITEN));
	while (!itim32_is_enabled(dev)) {
	}
}

static int counter_npcm_itim32_start(const struct device *dev)
{
	struct counter_npcm_itim32_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (!itim32_is_enabled(dev)) {
		itim32_enable(dev, data->top);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_npcm_itim32_stop(const struct device *dev)
{
	struct counter_npcm_itim32_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	itim32_disable(dev);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_npcm_itim32_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_npcm_itim32_config *config = dev->config;
	uint32_t prev;
	uint32_t cnt = sys_read32(config->base + NPCM_ITCNT32);

	/*
	 * The counter runs in the input clock domain; read it until two
	 * consecutive reads agree. While disabled it reads the reload value.
	 */
	do {
		prev = cnt;
		cnt = sys_read32(config->base + NPCM_ITCNT32);
	} while (cnt != prev);

	*ticks = cnt;

	return 0;
}

static int counter_npcm_itim32_set_top_value(const struct device *dev,
					     const struct counter_top_cfg *cfg)
{
	const struct counter_npcm_itim32_config *config = dev->config;
	struct counter_npcm_itim32_data *data = dev->data;
	k_spinlock_key_t key;
	bool running;

	/* The reload value can only be changed while the module is disabled */
	if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) != 0U) {
		return -ENOTSUP;
	}

	if ((cfg->ticks == 0U) || (cfg->ticks > config->info.max_top_value)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	running = itim32_is_enabled(dev);
	if (running) {
		itim32_disable(dev);
	}

	data->top = cfg->ticks;
	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (running) {
		itim32_enable(dev, data->top);
	} else {
		sys_write32(data->top, config->base + NPCM_ITCNT32);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static uint32_t counter_npcm_itim32_get_top_value(const struct device *dev)
{
	struct counter_npcm_itim32_data *data = dev->data;

	return data->top;
}

static uint32_t counter_npcm_itim32_get_pending_int(const struct device *dev)
{
	return IS_BIT_SET(itim32_read_ctrl(dev), NPCM_ITCTS_TO_STS) ? 1U : 0U;
}

static uint32_t counter_npcm_itim32_get_freq(const struct device *dev)
{
	struct counter_npcm_itim32_data *data = dev->data;

	return data->freq;
}

static void counter_npcm_itim32_isr(const struct device *dev)
{
	struct counter_npcm_itim32_data *data = dev->data;
	counter_top_callback_t callback;
	void *user_data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	itim32_clear_timeout(dev);
	callback = data->top_callback;
	user_data = data->top_user_data;

	k_spin_unlock(&data->lock, key);

	if (callback != NULL) {
		callback(dev, user_data);
	}
}

static int counter_npcm_itim32_init(const struct device *dev)
{
	const struct counter_npcm_itim32_config *config = dev->config;
	struct counter_npcm_itim32_data *data = dev->data;
	uint32_t rate;
	uint8_t itcts;
	int ret;

	if ((config->prescaler < 1U) || (config->prescaler > NPCM_ITIM32_MAX_PRESCALER)) {
		return -EINVAL;
	}

	if (!device_is_ready(config->clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clk_dev, (clock_control_subsys_t)config->clk_id);
	if (ret < 0) {
		LOG_ERR("turn on ITIM32 clock failed %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(config->clk_dev, (clock_control_subsys_t)config->clk_id,
				     &rate);
	if (ret < 0) {
		LOG_ERR("get ITIM32 clock rate failed %d", ret);
		return ret;
	}

	itim32_disable(dev);

	sys_write8((uint8_t)(config->prescaler - 1U), config->base + NPCM_ITPRE);

	/* Interrupt and wake-up event on timeout; clear a stale timeout */
	itcts = BIT(NPCM_ITCTS_TO_WUE) | BIT(NPCM_ITCTS_TO_IE) | BIT(NPCM_ITCTS_TO_STS);
	/* Use the input clock that the clock controller reports for the module */
	if (rate == NPCM_ITIM32_LFCLK_FREQ) {
		itcts |= BIT(NPCM_ITCTS_CKSEL);
	}
	itim32_write_ctrl(dev, itcts);

	data->freq = rate / config->prescaler;
	data->top = config->info.max_top_value;
	sys_write32(data->top, config->base + NPCM_ITCNT32);

	config->irq_config_func();

	return 0;
}

static DEVICE_API(counter, counter_npcm_itim32_api) = {
	.start = counter_npcm_itim32_start,
	.stop = counter_npcm_itim32_stop,
	.get_value = counter_npcm_itim32_get_value,
	.set_top_value = counter_npcm_itim32_set_top_value,
	.get_pending_int = counter_npcm_itim32_get_pending_int,
	.get_top_value = counter_npcm_itim32_get_top_value,
	.get_freq = counter_npcm_itim32_get_freq,
};

#define COUNTER_NPCM_ITIM32_INIT(n)                                                                \
	static void counter_npcm_itim32_irq_config_##n(void)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), counter_npcm_itim32_isr,    \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct counter_npcm_itim32_config counter_npcm_itim32_config_##n = {          \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.flags = 0,                                                        \
				.channels = 0,                                                     \
			},                                                                         \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                  \
		.clk_id = DT_INST_PHA(n, clocks, clk_id),                                          \
		.prescaler = DT_INST_PROP(n, prescaler),                                           \
		.irq_config_func = counter_npcm_itim32_irq_config_##n,                             \
	};                                                                                         \
                                                                                                   \
	static struct counter_npcm_itim32_data counter_npcm_itim32_data_##n;                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, counter_npcm_itim32_init, NULL, &counter_npcm_itim32_data_##n,    \
			      &counter_npcm_itim32_config_##n, POST_KERNEL,                        \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_npcm_itim32_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_NPCM_ITIM32_INIT)
