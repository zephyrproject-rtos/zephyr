/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_crsas_ma2_watchdog

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wdt_crsas_ma2, CONFIG_WDT_LOG_LEVEL);

#define WDT_WCS_OFFSET     0x000
#define WDT_WOR_OFFSET     0x008
#define WDT_WRR_OFFSET     0x000
#define ARM_WDT_FRAME_SIZE 0x1000

#define WCS_EN  0
#define WCS_WS0 1
#define WCS_WS1 2

struct wdt_crsas_ma2_config {
	uintptr_t control_base;
	uintptr_t refresh_base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint8_t ws0_irq_num;
	uint8_t ws1_irq_num;
};

struct wdt_crsas_ma2_data {
	wdt_callback_t callback;
	uint32_t timeout_value;
	bool enabled;
	uint8_t timeout_flags;
};

static void wdt_crsas_ma2_ws0_isr(const struct device *dev)
{
	const struct wdt_crsas_ma2_config *cfg = dev->config;
	struct wdt_crsas_ma2_data *data = dev->data;

	if (data->timeout_flags == WDT_FLAG_RESET_NONE) {
		/* feed the watchdog ourselves to prevent any reset */
		sys_write32(0x1, cfg->refresh_base + WDT_WRR_OFFSET);
	}

	if (data->callback) {
		data->callback(dev, 0);
	} else {
		LOG_WRN_ONCE("WDT: WS0 interrupt, no callback so expect reset");
	}

	irq_disable(cfg->ws0_irq_num);
}

static void wdt_crsas_ma2_ws1_isr(const struct device *dev)
{
	const struct wdt_crsas_ma2_config *cfg = dev->config;

	/* If this ISR is reached, then the watchdog reset pin wasn't
	 * connected to a core or system reset. All we can do is
	 * log that it's occurring and panic.
	 */
	LOG_ERR("WDT: WS1 Reset Signal Asserted!");

	/* disable interrupt or else it will keep firing */
	irq_disable(cfg->ws1_irq_num);

	k_panic();
}

static int wdt_crsas_ma2_disable(const struct device *dev)
{
	const struct wdt_crsas_ma2_config *cfg = dev->config;
	struct wdt_crsas_ma2_data *data = dev->data;

	if (!data->enabled) {
		return -EFAULT;
	}

	/* Disable & Stop the watchdog timer */
	sys_clear_bit(cfg->control_base + WDT_WCS_OFFSET, WCS_EN);
	data->enabled = false;
	/* API says timeout must be installed again before next setup */
	data->timeout_value = 0;

	return 0;
}

static int wdt_crsas_ma2_install_timeout(const struct device *dev,
					 const struct wdt_timeout_cfg *timeout)
{
	const struct wdt_crsas_ma2_config *cfg = dev->config;
	struct wdt_crsas_ma2_data *data = dev->data;
	uint32_t clk_freq;
	int rc;

	if (data->enabled) {
		rc = -EBUSY;
		goto out;
	}

	/* we don't support window, so min must be 0.
	 * we don't support a timeout of 0
	 */
	if (timeout->window.min != 0U || timeout->window.max == 0U) {
		rc = -EINVAL;
		goto out;
	}

	/* if no callback is provided, it doesn't really make sense to
	 * configure for RESET_NONE. we would just silently feed the
	 * watchdog and just waste power.
	 */
	if (timeout->flags == WDT_FLAG_RESET_NONE && timeout->callback == NULL) {
		rc = -ENOTSUP;
		goto out;
	}

	if (timeout->flags == WDT_FLAG_RESET_CPU_CORE) {
		rc = -ENOTSUP;
		goto out;
	}

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock control device is not ready yet");
		rc = -EINVAL;
		goto out;
	}

	/* get counter frequency */
	rc = clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &clk_freq);
	if (rc != 0) {
		LOG_ERR("unable to get clock frequency");
		rc = -EINVAL;
		goto out;
	}

	/* Check if the timeout can be represented. Must be 32-bits
	 * since that's the size of the Watchdog Offset register (WOR).
	 *
	 * When the watchdog is started, the WOR is added to the current
	 * value of the 64-bit System Counter to set the value of the
	 * 64-bit Watchdog Compare Value register (WCV). When the comparison
	 * is matched, the first interrupt fires and the WCV is reloaded
	 * again the same way. When a second match occurs, the second
	 * interrupt fires, or possibly a reset occurs without the interrupt.
	 * The type of reset (CORE, SOC, etc) is system dependent.
	 */
	uint64_t timeout_ticks = (uint64_t)clk_freq * timeout->window.max / 1000;

	if (timeout_ticks > UINT32_MAX) {
		LOG_ERR("timeout window.max is too large");
		rc = -EINVAL;
		goto out;
	}

	data->callback = timeout->callback;
	data->timeout_flags = timeout->flags;

	/* Do the "install" by save the ticks to data. We only write to the
	 * Watchdog Offset register (WOR) on setup, since the match register
	 * is updated as soon as the WOR is written.
	 */
	data->timeout_value = timeout_ticks;

out:
	return rc;
}

static int wdt_crsas_ma2_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_crsas_ma2_config *cfg = dev->config;
	struct wdt_crsas_ma2_data *data = dev->data;

	/* we don't support any options currently */
	if (options != 0) {
		return -ENOTSUP;
	}

	if (data->enabled) {
		return -EBUSY;
	}

	if (data->timeout_value == 0) {
		return -ENOTSUP;
	}

	/* Set WOR and enable */
	sys_write32(data->timeout_value, cfg->control_base + WDT_WOR_OFFSET);
	sys_set_bit(cfg->control_base + WDT_WCS_OFFSET, WCS_EN);
	data->enabled = true;

	return 0;
}

static int wdt_crsas_ma2_feed(const struct device *dev, int channel_id)
{
	const struct wdt_crsas_ma2_config *cfg = dev->config;
	struct wdt_crsas_ma2_data *data = dev->data;

	/* channel_id must be 0 */
	__ASSERT_NO_MSG(channel_id == 0);

	if (!data->enabled) {
		return -EINVAL;
	}

	if (data->timeout_value == 0) {
		return -EINVAL;
	}

	/* Writing any value to WRR in the refresh frame resets the wdt */
	sys_write32(0x1, cfg->refresh_base + WDT_WRR_OFFSET);

	return 0;
}

static int wdt_crsas_ma2_init(const struct device *dev)
{
	const struct wdt_crsas_ma2_config *cfg = dev->config;

	/* Make sure watchdog starts disabled */
	sys_write32(0, cfg->control_base + WDT_WCS_OFFSET);

	/* Configure and enable IRQ(s) */
	cfg->irq_config_func(dev);

	return 0;
}

static DEVICE_API(wdt, wdt_crsas_ma2_api) = {
	.setup = wdt_crsas_ma2_setup,
	.disable = wdt_crsas_ma2_disable,
	.install_timeout = wdt_crsas_ma2_install_timeout,
	.feed = wdt_crsas_ma2_feed,
};

#define GET_REFRESH_BASE(n)                                                                        \
	COND_CODE_1(DT_INST_REG_HAS_IDX(n, 1), \
	(DT_INST_REG_ADDR_BY_IDX(n, 1)), \
	(DT_INST_REG_ADDR_BY_IDX(n, 0) + ARM_WDT_FRAME_SIZE))

#define WDT_CRSAS_MA2_INIT(n)                                                                      \
	static void wdt_crsas_ma2_irq_config_##n(const struct device *dev)                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq), DT_INST_IRQ_BY_IDX(n, 0, priority),     \
			    wdt_crsas_ma2_ws0_isr, DEVICE_DT_INST_GET(n), 0);                      \
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));                                         \
                                                                                                   \
		/* connect irq index 1 if it exists */                                             \
		COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 1), (                                  \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 1, irq),                            \
		DT_INST_IRQ_BY_IDX(n, 1, priority),                       \
		wdt_crsas_ma2_ws1_isr, DEVICE_DT_INST_GET(n), 0);         \
		irq_enable(DT_INST_IRQ_BY_IDX(n, 1, irq));                            \
		), (/* IRQ index idx not defined */))        \
	}                                                                                          \
                                                                                                   \
	static const struct wdt_crsas_ma2_config wdt_crsas_ma2_cfg_##n = {                         \
		.control_base = DT_INST_REG_ADDR_BY_IDX(n, 0),                                     \
		.refresh_base = GET_REFRESH_BASE(n),                                               \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)(DT_INST_CLOCKS_CELL(n, clkid)),           \
		.irq_config_func = wdt_crsas_ma2_irq_config_##n,                                   \
		.ws0_irq_num = DT_INST_IRQ_BY_IDX(n, 0, irq),                                      \
		.ws1_irq_num =                                                                     \
			COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 1),                     \
					(DT_INST_IRQ_BY_IDX(n, 1, irq)), 0),     \
	};                                                                                         \
                                                                                                   \
	static struct wdt_crsas_ma2_data wdt_crsas_ma2_data_##n;                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, wdt_crsas_ma2_init, NULL, &wdt_crsas_ma2_data_##n,                \
			      &wdt_crsas_ma2_cfg_##n, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_crsas_ma2_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_CRSAS_MA2_INIT)
