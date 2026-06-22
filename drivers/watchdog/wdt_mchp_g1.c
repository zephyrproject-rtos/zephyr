/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stdio.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

#define DT_DRV_COMPAT microchip_wdt_g1

LOG_MODULE_REGISTER(wdt_mchp_g1, CONFIG_WDT_LOG_LEVEL);

#define WDT_LOCK_TIMEOUT              K_MSEC(10)
#define MAX_INSTALLABLE_TIMEOUT_COUNT (DT_PROP(DT_NODELABEL(wdt), max_installable_timeout_count))
#define MAX_TIMEOUT_WINDOW            (DT_PROP(DT_NODELABEL(wdt), max_timeout_window))
#define MAX_TIMEOUT_WINDOW_MODE       (DT_PROP(DT_NODELABEL(wdt), max_timeout_window_mode))
#define MIN_WINDOW_LIMIT              (DT_PROP(DT_NODELABEL(wdt), min_window_limit))
#define WDT_FLAG_ONLY_ONE_TIMEOUT_VALUE_SUPPORTED                                                  \
	(DT_PROP(DT_NODELABEL(wdog), only_one_timeout_val_supported_flag))

/* Either of these bits will be set in case watchdog is turned on */
#define WDT_ENABLED_BITS_POS (WDT_CTRLA_ENABLE(1) | WDT_CTRLA_ALWAYSON(1))
#define UINT32_WIDTH         32U
#define TIMER_FREQ_HZ        1024U
#define MS_PER_SEC           1000U

/* This macro takes an integer input `n` and calculates the period value by
 * left-shifting the number 8 by `n` positions. The result of this macro is 8 * 2^n.
 */
#define PERIOD_VALUE(n)  (8 << n)
#define TIMEOUT_VALUE_US 1000
#define DELAY_US         2

typedef enum {
	NORMAL_MODE = 0,
	WINDOW_MODE = 1,
} wdt_mode_t;

struct wdt_mchp_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
};

struct wdt_mchp_channel_data {
	struct wdt_window window;
};

struct wdt_mchp_dev_data {
	wdt_callback_t callback;
	bool interrupt_enabled;
	bool window_mode;
	uint8_t installed_timeout_cnt;
	struct wdt_mchp_channel_data channel_data[MAX_INSTALLABLE_TIMEOUT_COUNT];
	struct k_mutex lock;
};

struct wdt_mchp_dev_cfg {
	wdt_registers_t *regs;
	void (*irq_config_func)(const struct device *dev);
	struct wdt_mchp_clock wdt_clock;
};

static inline bool wdt_is_enabled(const wdt_registers_t *regs)
{
	/* the significance of WDT_ENABLED_BITS_POS is that it
	 * denotes the always on bit and the enable bit
	 */
	return ((regs->WDT_CTRLA & WDT_ENABLED_BITS_POS) != 0) ? true : false;
}

/*
 * Function to wait for the watchdog timer synchronization.
 *
 * This function waits until the synchronization busy bit in the watchdog timer
 * control register is cleared, indicating that the synchronization is complete.
 */
static void wdt_sync_wait(const wdt_registers_t *regs)
{
	if (WAIT_FOR((regs->WDT_SYNCBUSY == 0), TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for WDT_SYNCBUSY to clear");
	}
}
/*
 * Function to enable or disable watchdog peripheral
 *
 * It won't be able to disable if the always on bit is turned on
 */

static int wdt_enable(wdt_registers_t *regs, bool enable)
{
	int ret = 0;

	if (enable != 0) {
		regs->WDT_CTRLA |= WDT_CTRLA_ENABLE(1);
	} else {
		/* check always on bit here */
		if (0 == (regs->WDT_CTRLA & WDT_CTRLA_ALWAYSON(1))) {
			regs->WDT_CTRLA &= ~WDT_CTRLA_ENABLE(1);
		} else {
			LOG_ERR("watchdog disable not supported when always on bit is enabled");
			ret = -ENOTSUP;
		}
	}
	wdt_sync_wait(regs);
	LOG_DBG("ctrl reg = 0x%x\n", regs->WDT_CTRLA);

	return ret;
}

/*
 * Function to get the period index for a given timeout value.
 *
 * This function calculates the number of clock cycles required for the given
 * timeout value in milliseconds. It then determines the appropriate period
 * index based on the number of clock cycles. The function returns the period
 * index.
 */
static uint32_t wdt_get_period_idx(uint32_t timeout_ms)
{
	uint32_t next_period;
	uint32_t cycles;

	/* Calculate number of clock cycles @ TIMER_FREQ_HZ input clock */
	cycles = (timeout_ms * TIMER_FREQ_HZ) / MS_PER_SEC;

	/* Minimum wdt period is 8 clock cycles (register value 0) */
	if (cycles <= MIN_WINDOW_LIMIT) {
		return 0;
	}

	/* Round up to next period and calculate the register value
	 * This produces a value that is the smallest power of two
	 * greater than or equal to cycles
	 */
	next_period = BIT64(UINT32_WIDTH) >> __builtin_clz(cycles - 1);

	return find_msb_set(next_period >> 4);
}

/*
 * Function to get the watchdog timer timeout values.
 *
 * This function calculates the minimum and maximum timeout values for the
 * watchdog timer based on the input parameters. It calculates the appropriate
 * period indices for the window closed time and the maximum timeout value. The
 * function returns the calculated timeout values.
 */
static struct wdt_mchp_channel_data wdt_get_timeout_val(uint32_t window_closed_time,
							uint32_t timeout_max)
{
	struct wdt_mchp_channel_data new_timeout = {0};
	uint8_t window_index = wdt_get_period_idx(window_closed_time);
	uint8_t per_index = wdt_get_period_idx(timeout_max - window_closed_time);

	new_timeout.window.min = (window_closed_time ? PERIOD_VALUE(window_index) : 0);
	new_timeout.window.max =
		(window_closed_time ? (PERIOD_VALUE(per_index) + PERIOD_VALUE(window_index))
				    : PERIOD_VALUE(per_index));

	return new_timeout;
}

static int wdt_reset_type_set(uint8_t flag)
{
	int ret_val = 0;

	switch (flag) {
	case WDT_FLAG_RESET_NONE:
		ret_val = -ENOTSUP;
		break;

	case WDT_FLAG_RESET_CPU_CORE:
	case WDT_FLAG_RESET_SOC:
		break;

	default:
		ret_val = -EINVAL;
		break;
	}

	return ret_val;
}

/*
 * Function to validate the watchdog timer window configuration.
 *
 * This function checks the validity of the minimum and maximum timeout values
 * for the watchdog timer window mode. It performs various checks to ensure that
 * the timeout values are within the allowed range and that the window
 * configuration is valid. If any of the checks fail, the function returns
 * failure.
 */
static int wdt_validate_window(uint32_t timeout_min, uint32_t timeout_max)
{
	/* Check whether the timeout max is greater
	 * than the maximum possible value in window mode
	 */
	if ((timeout_max >= MAX_TIMEOUT_WINDOW_MODE) && (timeout_min != 0)) {
		LOG_ERR("invalid timeout values");
		return -EINVAL;
	}

	/* check whether the timeout max given is zero */
	if (timeout_max == 0) {
		LOG_ERR("invalid timeout values %d", __LINE__);
		return -EINVAL;
	}

	/* Check whether the timeout min is not
	 * less than the minimum possible window
	 */
	if ((timeout_min < PERIOD_VALUE(0)) && (timeout_min != 0)) {
		LOG_ERR("invalid timeout values %d", __LINE__);
		return -EINVAL;
	}

	/* Ensure that a window is available) */
	if (timeout_min > (timeout_max >> 1)) {
		LOG_ERR("invalid timeout values %d", __LINE__);
		return -EINVAL;
	}

	/* this will ensure that the timeout range is within
	 * the limit for both normal mode and window mode
	 */
	if ((timeout_max - timeout_min) > MAX_TIMEOUT_WINDOW) {
		LOG_ERR("invalid timeout values %d", __LINE__);
		return -EINVAL;
	}

	return 0;
}

static inline int wdt_interrupt_enable(wdt_registers_t *regs)
{
	ARG_UNUSED(regs);
	return -ENOTSUP;
}

static inline int wdt_interrupt_flag_clear(wdt_registers_t *regs)
{
	ARG_UNUSED(regs);
	return -ENOTSUP;
}

static void wdt_window_enable(wdt_registers_t *regs, bool enable)
{
	if (enable != 0) {
		regs->WDT_CTRLA |= WDT_CTRLA_WEN(1);
	} else {
		regs->WDT_CTRLA &= ~WDT_CTRLA_WEN(1);
	}
	wdt_sync_wait(regs);
}

static struct wdt_mchp_channel_data
wdt_set_timeout(wdt_registers_t *regs, uint32_t window_closed_time, uint32_t timeout_max)
{
	struct wdt_mchp_channel_data set_timeout = {0};
	uint8_t window = wdt_get_period_idx(window_closed_time);

	/* The difference is taken as the total time of WDT
	 * defined by the CONFIG.window + CONFIG.per register value
	 */
	uint8_t per = wdt_get_period_idx(timeout_max - window_closed_time);

	LOG_DBG("window = %d : 0x%x per = %d : 0x%x\n\r", window, WDT_CONFIG_WINDOW(window), per,
		WDT_CONFIG_PER(per));
	set_timeout.window.min = window_closed_time ? PERIOD_VALUE(window) : 0;

	/* Based on the mode the window mode or normal mode, timeout max is returned */
	set_timeout.window.max = (window_closed_time ? (PERIOD_VALUE(per) + PERIOD_VALUE(window))
						     : PERIOD_VALUE(per));
	regs->WDT_CONFIG = WDT_CONFIG_WINDOW(window) | WDT_CONFIG_PER(per);
	wdt_sync_wait(regs);
	LOG_DBG("wdt_config = 0x%x\n\r", regs->WDT_CONFIG);

	return set_timeout;
}

static int wdt_apply_options(wdt_registers_t *regs, uint8_t options)
{
	/* WDT_OPT_PAUSE_HALTED_BY_DBG is supported by default by the peripheral */
	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0) {
		LOG_ERR("unsupported option selected %s", __func__);
		return -ENOTSUP;
	}

	return 0;
}

static void wdt_mchp_isr(const struct device *wdt_dev)
{
	struct wdt_mchp_dev_data *mchp_wdt_data = wdt_dev->data;
	const struct wdt_mchp_dev_cfg *const mchp_wdt_cfg = wdt_dev->config;

	wdt_interrupt_flag_clear(mchp_wdt_cfg->regs);
	if (mchp_wdt_data->callback != NULL) {
		mchp_wdt_data->callback(wdt_dev, 0);
	}
}

static int wdt_mchp_setup(const struct device *wdt_dev, uint8_t options)
{
	struct wdt_mchp_dev_data *mchp_wdt_data = wdt_dev->data;
	const struct wdt_mchp_dev_cfg *const mchp_wdt_cfg = wdt_dev->config;
	wdt_registers_t *regs = mchp_wdt_cfg->regs;
	int ret;

	k_mutex_lock(&mchp_wdt_data->lock, WDT_LOCK_TIMEOUT);
	if (wdt_is_enabled(regs) == true) {
		k_mutex_unlock(&mchp_wdt_data->lock);
		LOG_ERR("Watchdog already setup");
		return -EBUSY;
	}

	if (mchp_wdt_data->installed_timeout_cnt == 0) {
		k_mutex_unlock(&mchp_wdt_data->lock);
		LOG_ERR("No valid timeout installed");
		return -EINVAL;
	}
	ret = wdt_apply_options(regs, options);
	if (ret < 0) {
		k_mutex_unlock(&mchp_wdt_data->lock);
		LOG_ERR("ret val apply = %d", ret);
		return ret;
	}
	ret = wdt_enable(regs, true);
	if (ret < 0) {
		k_mutex_unlock(&mchp_wdt_data->lock);
		LOG_ERR("wdt_enable failed %d", ret);
		return ret;
	}
	LOG_DBG("watchdog enabled : 0x%x\n\r", wdt_is_enabled(regs));
	k_mutex_unlock(&mchp_wdt_data->lock);

	return 0;
}

static int wdt_mchp_disable(const struct device *wdt_dev)
{
	struct wdt_mchp_dev_data *mchp_wdt_data = wdt_dev->data;
	const struct wdt_mchp_dev_cfg *const mchp_wdt_cfg = wdt_dev->config;
	wdt_registers_t *regs = mchp_wdt_cfg->regs;
	int ret;
	uint32_t irq_key;

	irq_key = irq_lock();
	mchp_wdt_data->installed_timeout_cnt = 0;

	/* if watchdog is not enabled, then return fault */
	if (wdt_is_enabled(regs) == false) {
		irq_unlock(irq_key);
		LOG_ERR("wdg is already disabled");
		return -EFAULT;
	}
	ret = wdt_enable(regs, false);
	if (ret < 0) {
		irq_unlock(irq_key);
		LOG_ERR("wdg was not disabled = %d", ret);
		return -EPERM;
	}
	irq_unlock(irq_key);

	return 0;
}

static int wdt_mchp_install_timeout(const struct device *wdt_dev, const struct wdt_timeout_cfg *cfg)
{
	struct wdt_mchp_dev_data *mchp_wdt_data = wdt_dev->data;
	const struct wdt_mchp_dev_cfg *const mchp_wdt_cfg = wdt_dev->config;
	wdt_registers_t *regs = mchp_wdt_cfg->regs;
	struct wdt_mchp_channel_data *channel_data = mchp_wdt_data->channel_data;
	struct wdt_mchp_channel_data actual_set_timeout = {0};
	int ret = 0;
	uint32_t irq_key = 0;

	k_mutex_lock(&mchp_wdt_data->lock, WDT_LOCK_TIMEOUT);
	mchp_wdt_data->callback = cfg->callback;
	mchp_wdt_data->window_mode = (((cfg->window.min) > 0) ? true : false);
	mchp_wdt_data->interrupt_enabled = ((mchp_wdt_data->callback != NULL) ? true : false);

	/* CONFIG is enable protected, error out if already enabled */
	if (wdt_is_enabled(regs) != 0) {
		k_mutex_unlock(&mchp_wdt_data->lock);
		LOG_ERR("Watchdog already setup");
		return -EBUSY;
	}

#if defined(WDT_FLAG_ONLY_ONE_TIMEOUT_VALUE_SUPPORTED)
	struct wdt_mchp_channel_data new_set_timeout = {0};

	/* Check whether the new timeout is different from the already existing timeout */
	if (mchp_wdt_data->installed_timeout_cnt != 0) {
		new_set_timeout = wdt_get_timeout_val(cfg->window.min, cfg->window.max);
		if ((new_set_timeout.window.min != channel_data[0].window.min) ||
		    (new_set_timeout.window.max != channel_data[0].window.max)) {
			k_mutex_unlock(&mchp_wdt_data->lock);
			LOG_ERR("invalid timeout val");
			return -EINVAL;
		}
	}
#endif /* WDT_FLAG_ONLY_ONE_TIMEOUT_VALUE_SUPPORTED */

	/* if more installable timeouts are not available the count will be
	 * MAX_INSTALLABLE_TIMEOUT_COUNT
	 */
	if (mchp_wdt_data->installed_timeout_cnt == MAX_INSTALLABLE_TIMEOUT_COUNT) {
		k_mutex_unlock(&mchp_wdt_data->lock);
		LOG_ERR("No more timeouts available");
		return -ENOMEM;
	}

	/* Set the behaviour of the watchdog peripheral based on the flags supplied */
	ret = wdt_reset_type_set(cfg->flags);
	if (ret < 0) {
		k_mutex_unlock(&mchp_wdt_data->lock);
		LOG_ERR("error in setting reset type %d", ret);
		return ret;
	}

	/* validate the timeout window to be in the range available for the peripheral */
	ret = wdt_validate_window(cfg->window.min, cfg->window.max);
	if (ret < 0) {
		k_mutex_unlock(&mchp_wdt_data->lock);
		LOG_ERR("timeout out of range");
		return -EINVAL;
	}

	/* register the provided callback and enable the interrupt */
	if (mchp_wdt_data->interrupt_enabled != 0) {
		mchp_wdt_data->callback = cfg->callback;
		ret = wdt_interrupt_enable(regs);
		if (ret < 0) {
			k_mutex_unlock(&mchp_wdt_data->lock);
			LOG_ERR("Interrupt is not supported for this peripeheral");
			return -ENOTSUP;
		}
	}

	if (mchp_wdt_data->window_mode == WINDOW_MODE) {
		wdt_window_enable(regs, true);
	} else { /* Normal Mode */
		wdt_window_enable(regs, false);
	}
	actual_set_timeout = wdt_set_timeout(regs, cfg->window.min, cfg->window.max);

	/* Update the channel_data structure with the window parameters of each channel */
	channel_data[mchp_wdt_data->installed_timeout_cnt].window.max =
		actual_set_timeout.window.max;
	channel_data[mchp_wdt_data->installed_timeout_cnt].window.min =
		actual_set_timeout.window.min;

	LOG_ERR("Rounded off timeout min to %d\nRounded off timeout max to %d",
		actual_set_timeout.window.min, actual_set_timeout.window.max);

	/* this will return the channel id and then increment the
	 * count which will then be used for the next channel.
	 */
	irq_key = irq_lock();
	ret = (mchp_wdt_data->installed_timeout_cnt)++;
	irq_unlock(irq_key);
	k_mutex_unlock(&mchp_wdt_data->lock);

	return ret;
}

static int wdt_mchp_feed(const struct device *wdt_dev, int channel_id)
{
	struct wdt_mchp_dev_data *mchp_wdt_data = wdt_dev->data;
	const struct wdt_mchp_dev_cfg *const mchp_wdt_cfg = wdt_dev->config;
	wdt_registers_t *regs = mchp_wdt_cfg->regs;

	if (wdt_is_enabled(regs) == false) {
		LOG_ERR("Watchdog not setup");
		return -EINVAL;
	}
	if ((channel_id < 0) || (channel_id >= (mchp_wdt_data->installed_timeout_cnt))) {
		LOG_ERR("Invalid channel selected");
		return -EINVAL;
	}
	if (mchp_wdt_data->installed_timeout_cnt == 0) {
		LOG_ERR("No valid timeout installed");
		return -EINVAL;
	}

	/* Lock mutex only if feed called from a thread */
	if (false == k_is_in_isr()) {
		k_mutex_lock(&mchp_wdt_data->lock, WDT_LOCK_TIMEOUT);
	}
	regs->WDT_CLEAR = WDT_CLEAR_CLEAR_KEY_Val;

	if (false == k_is_in_isr()) {
		k_mutex_unlock(&mchp_wdt_data->lock);
	}

	return 0;
}

static int wdt_mchp_init(const struct device *wdt_dev)
{
	struct wdt_mchp_dev_data *mchp_wdt_data = wdt_dev->data;
	const struct wdt_mchp_dev_cfg *const mchp_wdt_cfg = wdt_dev->config;
	int ret_val = 0;

	k_mutex_init(&mchp_wdt_data->lock);

#if defined(CONFIG_WDT_DISABLE_AT_BOOT)
	ret_val = wdt_mchp_disable(wdt_dev);
	if (ret_val < 0) {
		LOG_ERR("Watchdog could not be disabled on startup");
		return -EPERM;
	}
#endif /* CONFIG_WDT_DISABLE_AT_BOOT */

	ret_val = clock_control_on(mchp_wdt_cfg->wdt_clock.clock_dev,
				   mchp_wdt_cfg->wdt_clock.mclk_sys);
	if ((ret_val < 0) && (ret_val != -EALREADY)) {
		LOG_ERR("Clock control on failed for MCLK %d", ret_val);
		return ret_val;
	}

	mchp_wdt_data->installed_timeout_cnt = 0;
	mchp_wdt_cfg->irq_config_func(wdt_dev);

	return 0;
}

static DEVICE_API(wdt, wdt_mchp_api) = {
	.setup = wdt_mchp_setup,
	.disable = wdt_mchp_disable,
	.install_timeout = wdt_mchp_install_timeout,
	.feed = wdt_mchp_feed,
};

#define WDT_MCHP_IRQ_HANDLER(n)                                                                    \
	static void wdt_mchp_irq_config_##n(const struct device *wdt_dev)                          \
	{                                                                                          \
		MCHP_WDT_IRQ_CONNECT(n, 0);                                                        \
	}

#define MCHP_WDT_IRQ_CONNECT(n, m)                                                                 \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq), DT_INST_IRQ_BY_IDX(n, m, priority),     \
			    wdt_mchp_isr, DEVICE_DT_INST_GET(n), 0);                               \
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));                                         \
	} while (false);

#define WDT_MCHP_IRQ_HANDLER_DECL(n)                                                               \
	static void wdt_mchp_irq_config_##n(const struct device *wdt_dev)

#define WDT_MCHP_CONFIG_DEFN(n)                                                                    \
	static const struct wdt_mchp_dev_cfg wdt_mchp_config_##n = {                               \
		.regs = (wdt_registers_t *)DT_INST_REG_ADDR(n),                                    \
		.irq_config_func = wdt_mchp_irq_config_##n,                                        \
		.wdt_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                         \
		.wdt_clock.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem))}

#define WDT_MCHP_DEVICE_INIT(n)                                                                    \
	WDT_MCHP_IRQ_HANDLER_DECL(n);                                                              \
	WDT_MCHP_CONFIG_DEFN(n);                                                                   \
	static struct wdt_mchp_dev_data wdt_mchp_data_##n;                                         \
	DEVICE_DT_INST_DEFINE(n, wdt_mchp_init, NULL, &wdt_mchp_data_##n, &wdt_mchp_config_##n,    \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_mchp_api);    \
	WDT_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(WDT_MCHP_DEVICE_INIT)
