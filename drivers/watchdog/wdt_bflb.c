/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_wdt

#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wdt_bflb, CONFIG_WDT_LOG_LEVEL);

#include <common_defines.h>
#include <bouffalolab/common/timer_reg.h>

/* WDG clock: 32K oscillator. With divider=31 => ~1032 Hz (32768/32).
 * We use divider 0 => 32768 Hz for maximum resolution.
 */
#define WDT_CLK_FREQ  32768U
#define WDT_CLK_DIV   0U
#define WDT_MAX_TICKS UINT16_MAX

#define TIMER_WDT_WFAR_KEY	0xBABA
#define TIMER_WDT_WSAR_KEY	0xEB10
#define TIMER_CLKSRC_32K	1U

struct wdt_bflb_config {
	uintptr_t base;
	void (*irq_config)(void);
};

struct wdt_bflb_data {
	wdt_callback_t callback;
	uint16_t match_value;
	bool reset_mode;
	bool configured;
};

static inline void wdt_bflb_unlock(uintptr_t base)
{
	sys_write32(TIMER_WDT_WFAR_KEY, base + TIMER_WFAR_OFFSET);
	sys_write32(TIMER_WDT_WSAR_KEY, base + TIMER_WSAR_OFFSET);
}

static int wdt_bflb_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_bflb_config *cfg = dev->config;
	struct wdt_bflb_data *data = dev->data;
	uint32_t tmp;

	if (!data->configured) {
		LOG_ERR("No timeout installed");
		return -EINVAL;
	}

	if (options & (WDT_OPT_PAUSE_IN_SLEEP | WDT_OPT_PAUSE_HALTED_BY_DBG)) {
		LOG_DBG("Pause options not supported, ignoring");
	}

	/* Disable WDG first */
	wdt_bflb_unlock(cfg->base);
	tmp = sys_read32(cfg->base + TIMER_WMER_OFFSET);
	tmp &= ~TIMER_WE;
	sys_write32(tmp, cfg->base + TIMER_WMER_OFFSET);

	/* Set clock source to 32K oscillator */
	tmp = sys_read32(cfg->base + TIMER_TCCR_OFFSET);
	tmp &= ~TIMER_CS_WDT_MASK;
	tmp |= (TIMER_CLKSRC_32K << TIMER_CS_WDT_SHIFT);
	sys_write32(tmp, cfg->base + TIMER_TCCR_OFFSET);

	/* Set clock divider */
	tmp = sys_read32(cfg->base + TIMER_TCDR_OFFSET);
	tmp &= ~TIMER_WCDR_MASK;
	tmp |= (WDT_CLK_DIV << TIMER_WCDR_SHIFT);
	sys_write32(tmp, cfg->base + TIMER_TCDR_OFFSET);

	/* Set match value */
	wdt_bflb_unlock(cfg->base);
	tmp = sys_read32(cfg->base + TIMER_WMR_OFFSET);
	tmp &= ~TIMER_WMR_MASK;
	tmp |= (data->match_value << TIMER_WMR_SHIFT);
	sys_write32(tmp, cfg->base + TIMER_WMR_OFFSET);

	/* Set mode: WRIE=1 for reset, WRIE=0 for interrupt */
	wdt_bflb_unlock(cfg->base);
	tmp = sys_read32(cfg->base + TIMER_WMER_OFFSET);
	if (data->reset_mode) {
		tmp |= TIMER_WRIE;
	} else {
		tmp &= ~TIMER_WRIE;
	}
	sys_write32(tmp, cfg->base + TIMER_WMER_OFFSET);

	/* Clear WDG interrupt */
	wdt_bflb_unlock(cfg->base);
	sys_write32(TIMER_WICLR, cfg->base + TIMER_WICR_OFFSET);

	/* Reset counter */
	wdt_bflb_unlock(cfg->base);
	sys_write32(TIMER_WCR, cfg->base + TIMER_WCR_OFFSET);

	/* Enable WDG */
	wdt_bflb_unlock(cfg->base);
	tmp = sys_read32(cfg->base + TIMER_WMER_OFFSET);
	tmp |= TIMER_WE;
	sys_write32(tmp, cfg->base + TIMER_WMER_OFFSET);

	return 0;
}

static int wdt_bflb_disable(const struct device *dev)
{
	const struct wdt_bflb_config *cfg = dev->config;
	uint32_t tmp;

	wdt_bflb_unlock(cfg->base);
	tmp = sys_read32(cfg->base + TIMER_WMER_OFFSET);
	tmp &= ~TIMER_WE;
	sys_write32(tmp, cfg->base + TIMER_WMER_OFFSET);

	return 0;
}

static int wdt_bflb_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *timeout)
{
	struct wdt_bflb_data *data = dev->data;
	uint32_t ticks;

	if (timeout->window.min != 0) {
		LOG_ERR("Window watchdog not supported");
		return -EINVAL;
	}

	if (timeout->window.max == 0) {
		LOG_ERR("Timeout must be > 0");
		return -EINVAL;
	}

	/* Convert ms to ticks: ticks = ms * freq / 1000 / (div+1) */
	ticks = (uint64_t)timeout->window.max * WDT_CLK_FREQ / 1000U / (WDT_CLK_DIV + 1U);

	if (ticks > WDT_MAX_TICKS) {
		LOG_ERR("Timeout too large (max %u ms)",
			(uint32_t)((uint64_t)WDT_MAX_TICKS * 1000U * (WDT_CLK_DIV + 1U) /
				   WDT_CLK_FREQ));
		return -EINVAL;
	}

	if (ticks == 0) {
		ticks = 1;
	}

	data->match_value = (uint16_t)ticks;
	data->callback = timeout->callback;
	data->reset_mode = (timeout->callback == NULL);
	data->configured = true;

	/* Single channel - always return 0 */
	return 0;
}

static int wdt_bflb_feed(const struct device *dev, int channel_id)
{
	const struct wdt_bflb_config *cfg = dev->config;

	if (channel_id != 0) {
		return -EINVAL;
	}

	wdt_bflb_unlock(cfg->base);
	sys_write32(TIMER_WCR, cfg->base + TIMER_WCR_OFFSET);

	return 0;
}

static void wdt_bflb_isr(const struct device *dev)
{
	const struct wdt_bflb_config *cfg = dev->config;
	struct wdt_bflb_data *data = dev->data;
	uint32_t tmp;

	/* Clear interrupt */
	wdt_bflb_unlock(cfg->base);
	sys_write32(TIMER_WICLR, cfg->base + TIMER_WICR_OFFSET);

	if (data->callback) {
		data->callback(dev, 0);
	}

	/* Switch to reset mode so the next expiry causes a SoC reset.
	 * Bouffalo Lab WDT is single-stage: either interrupt or reset, not both.
	 * To emulate two-stage behavior (interrupt then reset), we switch
	 * WRIE to 1 after the callback has had its chance to feed.
	 */
	wdt_bflb_unlock(cfg->base);
	tmp = sys_read32(cfg->base + TIMER_WMER_OFFSET);
	tmp |= TIMER_WRIE;
	sys_write32(tmp, cfg->base + TIMER_WMER_OFFSET);
}

static int wdt_bflb_init(const struct device *dev)
{
	const struct wdt_bflb_config *cfg = dev->config;

	cfg->irq_config();

	return 0;
}

static DEVICE_API(wdt, wdt_bflb_api) = {
	.setup = wdt_bflb_setup,
	.disable = wdt_bflb_disable,
	.install_timeout = wdt_bflb_install_timeout,
	.feed = wdt_bflb_feed,
};

#define WDT_BFLB_INIT(n)                                                                           \
	static void wdt_bflb_irq_config_##n(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), wdt_bflb_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static struct wdt_bflb_data wdt_bflb_data_##n;                                             \
                                                                                                   \
	static const struct wdt_bflb_config wdt_bflb_config_##n = {                                \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config = wdt_bflb_irq_config_##n,                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, wdt_bflb_init, NULL, &wdt_bflb_data_##n, &wdt_bflb_config_##n,    \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wdt_bflb_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_BFLB_INIT)
