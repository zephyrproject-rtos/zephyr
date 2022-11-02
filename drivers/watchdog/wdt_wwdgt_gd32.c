/*
 * Copyright (c) 2022 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_wwdgt

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>

#include <gd32_wwdgt.h>

LOG_MODULE_REGISTER(wdt_wwdgt_gd32, CONFIG_WDT_LOG_LEVEL);

#define WWDGT_PRESCALER_EXP_MAX (3U)
#define WWDGT_COUNTER_MIN	(0x40U)
#define WWDGT_COUNTER_MAX	(0x7fU)
#define WWDGT_INTERNAL_DIVIDER	(4096ULL)

struct gd32_wwdgt_config {
	uint16_t clkid;
	struct reset_dt_spec reset;
};

/* mutable driver data */
struct gd32_wwdgt_data {
	/* counter update value*/
	uint8_t counter;
	/* user defined callback */
	wdt_callback_t callback;
};

static void gd32_wwdgt_irq_config(const struct device *dev);

/**
 * @param timeout Timeout value in milliseconds.
 * @param exp exponent part of prescaler
 *
 * @return ticks count calculated by this formula.
 *
 *  timeout = pclk * INTERNAL_DIVIDER * (2^prescaler_exp) * (count + 1)
 *  transform as
 *  count = (timeout * pclk / INTERNAL_DIVIDER * (2^prescaler_exp) ) - 1
 *
 *  and add WWDGT_COUNTER_MIN to this as a offset value.
 */
static inline uint32_t gd32_wwdgt_calc_ticks(const struct device *dev,
					    uint32_t timeout, uint32_t exp)
{
	const struct gd32_wwdgt_config *config = dev->config;
	uint32_t pclk;

	(void)clock_control_get_rate(GD32_CLOCK_CONTROLLER,
				       (clock_control_subsys_t *)&config->clkid,
				       &pclk);

	return ((timeout * pclk)
		/ (WWDGT_INTERNAL_DIVIDER * (1 << exp) * MSEC_PER_SEC) - 1)
		+ WWDGT_COUNTER_MIN;
}

/**
 * @brief Calculates WWDGT config value from timeout window.
 *
 * @param win Pointer to timeout window struct.
 * @param counter Pointer to the storage of counter value.
 * @param wval Pointer to the storage of window value.
 * @param prescaler Pointer to the storage of prescaler value.
 *
 * @return 0 on success, -EINVAL if the window-max is out of range
 */
static int gd32_wwdgt_calc_window(const struct device *dev,
				  const struct wdt_window *win,
				  uint32_t *counter, uint32_t *wval,
				  uint32_t *prescaler)
{
	for (uint32_t shift = 0U; shift <= WWDGT_PRESCALER_EXP_MAX; shift++) {
		uint32_t max_count = gd32_wwdgt_calc_ticks(dev, win->max, shift);

		if (max_count <= WWDGT_COUNTER_MAX) {
			*counter = max_count;
			*prescaler = CFG_PSC(shift);
			if (win->min == 0U) {
				*wval = max_count;
			} else {
				*wval = gd32_wwdgt_calc_ticks(dev, win->min, shift);
			}

			return 0;
		}
	}

	return -EINVAL;
}

static int gd32_wwdgt_setup(const struct device *dev, uint8_t options)
{
	ARG_UNUSED(dev);

	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
#if CONFIG_GD32_DBG_SUPPORT
		dbg_periph_enable(DBG_WWDGT_HOLD);
#else
		LOG_ERR("Debug support not enabled");
		return -ENOTSUP;
#endif
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		LOG_ERR("WDT_OPT_PAUSE_IN_SLEEP not supported");
		return -ENOTSUP;
	}

	wwdgt_enable();
	wwdgt_flag_clear();
	wwdgt_interrupt_enable();

	return 0;
}

static int gd32_wwdgt_disable(const struct device *dev)
{
	/* watchdog cannot be stopped once started */
	ARG_UNUSED(dev);

	return -EPERM;
}

static int gd32_wwdgt_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *config)
{
	uint32_t prescaler = 0U;
	uint32_t counter = 0U;
	uint32_t window = 0U;
	struct gd32_wwdgt_data *data = dev->data;

	if (config->window.max == 0U) {
		LOG_ERR("window.max must be non-zero");
		return -EINVAL;
	}

	if (gd32_wwdgt_calc_window(dev, &config->window, &counter, &window,
				   &prescaler) != 0) {
		LOG_ERR("window.max in out of range");
		return -EINVAL;
	}

	data->callback = config->callback;
	data->counter = counter;

	wwdgt_config(counter, window, prescaler);

	return 0;
}

static int gd32_wwdgt_feed(const struct device *dev, int channel_id)
{
	struct gd32_wwdgt_data *data = dev->data;

	ARG_UNUSED(channel_id);

	wwdgt_counter_update(data->counter);

	return 0;
}

static void gd32_wwdgt_isr(const struct device *dev)
{
	struct gd32_wwdgt_data *data = dev->data;

	if (wwdgt_flag_get() != 0) {
		wwdgt_flag_clear();

		if (data->callback != NULL) {
			data->callback(dev, 0);
		}
	}
}

static void gd32_wwdgt_irq_config(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), gd32_wwdgt_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

static const struct wdt_driver_api wwdgt_gd32_api = {
	.setup = gd32_wwdgt_setup,
	.disable = gd32_wwdgt_disable,
	.install_timeout = gd32_wwdgt_install_timeout,
	.feed = gd32_wwdgt_feed,
};

static int gd32_wwdgt_init(const struct device *dev)
{
	const struct gd32_wwdgt_config *config = dev->config;

	(void)clock_control_on(GD32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t *)&config->clkid);
	(void)reset_line_toggle_dt(&config->reset);
	gd32_wwdgt_irq_config(dev);

	return 0;
}

static const struct gd32_wwdgt_config wwdgt_cfg = {
	.clkid = DT_INST_CLOCKS_CELL(0, id),
	.reset = RESET_DT_SPEC_INST_GET(0),
};

static struct gd32_wwdgt_data wwdgt_data = {
	.counter = WWDGT_COUNTER_MIN,
	.callback = NULL
};

DEVICE_DT_INST_DEFINE(0, gd32_wwdgt_init, NULL, &wwdgt_data, &wwdgt_cfg, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wwdgt_gd32_api);
