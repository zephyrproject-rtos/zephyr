/*
 * Copyright (C) 2024-2025, Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam4l_watchdog

/**
 * @brief Watchdog (WDT) Driver for Atmel SAM4L MCUs
 *
 * Note:
 * - SAM4L watchdog has a fuse bit to automatically enable the watchdog at boot.
 *   It should be enabled to keep compatibility with SAM family.
 * - Since the MCU boots with WDT enabled, the CONFIG_WDT_DISABLE_AT_BOOT
 *   is set default at boot and watchdog module is disabled in the MCU for
 *   systems that don't need watchdog functionality.
 * - If the application needs to use the watchdog in the system, then
 *   CONFIG_WDT_DISABLE_AT_BOOT must be unset in the app's config file
 */

#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/irq.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_sam4l);

#define WDT_FIRST_KEY		  0x55ul
#define WDT_SECOND_KEY		  0xAAul

enum wdt_sam4l_clock_src {
	WDT_SAM4L_CLK_SRC_RCSYS   = 0,
	WDT_SAM4L_CLK_SRC_OSC32K  = 1,
};

struct wdt_sam4l_dev_cfg {
	Wdt *regs;
	void (*irq_cfg_func)(const struct device *dev);
	const struct atmel_sam_pmc_config clock_cfg;
	enum wdt_sam4l_clock_src clock_src;
	bool lock;
};

struct wdt_sam4l_dev_data {
	wdt_callback_t cb;
	uint32_t flags;
};

/**
 * @brief Sets the Watchdog Timer Control register to the @a ctrl value.
 *
 * @param ctrl  Value to set the WatchDog Timer Control register to.
 */
static void wdt_sam4l_set_ctrl(const struct wdt_sam4l_dev_cfg *cfg,
			       const uint32_t ctrl)
{
	Wdt *const wdt = cfg->regs;
	volatile uint32_t delay;

	/* Calculate delay for internal synchronization, see 45.1.3 WDT errata */
	if (cfg->clock_src == WDT_SAM4L_CLK_SRC_OSC32K) {
		delay = DIV_ROUND_UP(SOC_ATMEL_SAM_MCK_FREQ_HZ * 2, SOC_ATMEL_SAM_RC32K_NOMINAL_HZ);
	} else {
		delay = DIV_ROUND_UP(SOC_ATMEL_SAM_MCK_FREQ_HZ * 2, SOC_ATMEL_SAM_RCSYS_NOMINAL_HZ);
	}
	/* ~8 cycles for one while loop */
	delay >>= 3;
	while (delay--) {
	}
	wdt->CTRL = WDT_CTRL_KEY(WDT_FIRST_KEY)
		  | ctrl;
	wdt->CTRL = WDT_CTRL_KEY(WDT_SECOND_KEY)
		  | ctrl;
}

/**
 * @brief Calculate timeout scale factor based on a input in milliseconds
 *
 * timeout(ms) = (2pow(scale + 1) * 1000) / wdt_clk
 *
 * @param cfg  Configuration
 * @param time Timeout value in milliseconds
 */
static int wdt_sam4l_calc_timeout(const struct wdt_sam4l_dev_cfg *cfg,
				  uint32_t time)
{
	uint32_t wdt_clk = cfg->clock_src == WDT_SAM4L_CLK_SRC_OSC32K
			 ? SOC_ATMEL_SAM_RC32K_NOMINAL_HZ
			 : SOC_ATMEL_SAM_RCSYS_NOMINAL_HZ;

	for (int scale = 7; scale <= 31; ++scale) {
		uint32_t timeout = (BIT64(scale + 1) * 1000ull) / wdt_clk;

		if (time <= timeout) {
			return scale;
		}
	}

	return -EINVAL;
}

/**
 * @brief Calculate both the Banned and Prescale Select timeout to be installed
 * in the watchdog timer.
 *
 * The config->min value will define the banned timeout. The prescaler timeout
 * then should be defined by the interval of (config->max - config-min).
 *
 * @param config Timeout Window configuration value in milliseconds.
 * @param tban   Pointer to the banned timeout
 * @param psel   Pointer to the timeout perscaller selection
 */
static int wdt_sam4l_convert_timeout(const struct wdt_sam4l_dev_cfg *cfg,
				     const struct wdt_timeout_cfg *config,
				     uint32_t *tban, uint32_t *psel)
{
	int scale;

	if (config->window.max < config->window.min || config->window.max == 0) {
		LOG_ERR("The watchdog window should have a valid period");
		return -EINVAL;
	}

	scale = wdt_sam4l_calc_timeout(cfg, config->window.min);
	if (scale < 0) {
		LOG_ERR("Window minimal value is too big");
		return -EINVAL;
	}
	*tban = scale;

	scale = wdt_sam4l_calc_timeout(cfg, config->window.max - config->window.min);
	if (scale < 0) {
		LOG_ERR("Window maximal value is too big");
		return -EINVAL;
	}
	*psel = scale;

	return 0;
}

static int wdt_sam4l_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_sam4l_dev_cfg *cfg = dev->config;
	Wdt *const wdt = cfg->regs;
	volatile uint32_t reg;

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		LOG_ERR("Pause in Sleep is an invalid option");
		return -ENOTSUP;
	}

	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		LOG_ERR("Pause on CPU halted by debug is an invalid option");
		return -ENOTSUP;
	}

	reg = wdt->CTRL;

	if (reg & WDT_CTRL_EN) {
		LOG_ERR("Watchdog is running and can not be changed");
		return -EPERM;
	}

	if (reg & WDT_CTRL_SFV) {
		LOG_ERR("Watchdog is locked and can not be changed");
		return -EPERM;
	}

	if (!(reg & WDT_CTRL_PSEL_Msk)) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	reg = wdt->CTRL
	    | (cfg->lock ? WDT_CTRL_SFV : 0)
	    | WDT_CTRL_EN;

	wdt_sam4l_set_ctrl(cfg, reg);
	while ((wdt->CTRL & WDT_CTRL_EN) != WDT_CTRL_EN) {
	}

	return 0;
}

static int wdt_sam4l_disable(const struct device *dev)
{
	const struct wdt_sam4l_dev_cfg *cfg = dev->config;
	Wdt *const wdt = cfg->regs;

	if (wdt->CTRL & WDT_CTRL_SFV) {
		LOG_ERR("Watchdog is already locked");
		return -EPERM;
	}

	wdt_sam4l_set_ctrl(cfg, wdt->CTRL & ~WDT_CTRL_EN);
	while (wdt->CTRL & WDT_CTRL_EN) {
	}

	wdt_sam4l_set_ctrl(cfg, wdt->CTRL & ~WDT_CTRL_CEN);
	while (wdt->CTRL & WDT_CTRL_CEN) {
	}

	return 0;
}

static int wdt_sam4l_install_timeout(const struct device *dev,
				     const struct wdt_timeout_cfg *config)
{
	const struct wdt_sam4l_dev_cfg *cfg = dev->config;
	struct wdt_sam4l_dev_data *const data = dev->data;
	Wdt *const wdt = cfg->regs;
	volatile uint32_t reg;
	uint32_t tban, psel;

	if (wdt->CTRL & WDT_CTRL_MODE) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	if (config->flags & WDT_FLAG_RESET_CPU_CORE) {
		LOG_ERR("The SAM4L watchdog does not support reset CPU core");
		return -ENOTSUP;
	}

	if (wdt_sam4l_convert_timeout(cfg, config, &tban, &psel)) {
		return -EINVAL;
	}

	reg = wdt->CTRL;

	if (reg & WDT_CTRL_EN) {
		LOG_ERR("Watchdog is running and can not be changed");
		return -EPERM;
	}

	if (reg & WDT_CTRL_SFV) {
		LOG_ERR("Watchdog is locked and can not be changed");
		return -EPERM;
	}

	data->cb = config->callback;
	data->flags = config->flags;
	reg = (cfg->clock_src == WDT_SAM4L_CLK_SRC_OSC32K ? WDT_CTRL_CSSEL : 0)
	    | (config->callback ? WDT_CTRL_IM : 0)
	    | (config->window.min ? WDT_CTRL_MODE : 0)
	    | (config->window.min ? WDT_CTRL_TBAN(tban) : 0)
	    | WDT_CTRL_PSEL(psel);
	wdt_sam4l_set_ctrl(cfg, reg);

	wdt_sam4l_set_ctrl(cfg, wdt->CTRL | WDT_CTRL_CEN);
	while (!(wdt->CTRL & WDT_CTRL_CEN)) {
	}

	return 0;
}

static int wdt_sam4l_feed(const struct device *dev, int channel_id)
{
	const struct wdt_sam4l_dev_cfg *cfg = dev->config;
	Wdt *const wdt = cfg->regs;

	ARG_UNUSED(channel_id);

	while ((wdt->SR & WDT_SR_CLEARED) != WDT_SR_CLEARED) {
	}
	wdt->CLR = WDT_CLR_WDTCLR
		 | WDT_CLR_KEY(WDT_FIRST_KEY);
	wdt->CLR = WDT_CLR_WDTCLR
		 | WDT_CLR_KEY(WDT_SECOND_KEY);

	return 0;
}

/* If the callback blocks or ISR do not clear flags the system will trigger a
 * CPU reset on the next watchdog timeout, see 20.5.3 Interrupt Mode
 */
static void wdt_sam4l_isr(const struct device *dev)
{
	const struct wdt_sam4l_dev_cfg *cfg = dev->config;
	struct wdt_sam4l_dev_data *const data = dev->data;
	Wdt *const wdt = cfg->regs;

	wdt->ICR = WDT_ICR_WINT;
	data->cb(dev, 0);
}

static DEVICE_API(wdt, wdt_sam4l_driver_api) = {
	.setup = wdt_sam4l_setup,
	.disable = wdt_sam4l_disable,
	.install_timeout = wdt_sam4l_install_timeout,
	.feed = wdt_sam4l_feed,
};

static int wdt_sam4l_init(const struct device *dev)
{
	const struct wdt_sam4l_dev_cfg *const cfg = dev->config;
	Wdt *const wdt = cfg->regs;

	/* Enable WDT clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clock_cfg);

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		wdt_sam4l_disable(dev);
		return 0;
	}

	wdt->IDR = WDT_IDR_MASK;
	cfg->irq_cfg_func(dev);
	wdt->IER = WDT_IER_WINT;

	return 0;
}

#define WDT_SAM4L_INIT(n)						\
	static void wdt##n##_sam4l_irq_cfg(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    wdt_sam4l_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}								\
									\
	static const struct wdt_sam4l_dev_cfg wdt##n##_sam4l_cfg = {	\
		.regs = (Wdt *)DT_INST_REG_ADDR(n),			\
		.irq_cfg_func = wdt##n##_sam4l_irq_cfg,			\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),		\
		.clock_src = DT_INST_ENUM_IDX(n, clk_source),		\
		.lock = DT_INST_PROP(n, lock_mode),			\
	};								\
									\
	static struct wdt_sam4l_dev_data wdt##n##_sam4l_data;		\
									\
	DEVICE_DT_INST_DEFINE(n, wdt_sam4l_init,			\
			    NULL,					\
			    &wdt##n##_sam4l_data,			\
			    &wdt##n##_sam4l_cfg,			\
			    PRE_KERNEL_1,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &wdt_sam4l_driver_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_SAM4L_INIT)
