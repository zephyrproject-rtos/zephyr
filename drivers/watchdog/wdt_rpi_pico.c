/*
 * Copyright (c) 2022, Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_watchdog

#include <hardware/watchdog.h>
#include <hardware/ticks.h>
#include <hardware/structs/psm.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_rpi_pico, CONFIG_WDT_LOG_LEVEL);

#ifdef CONFIG_SOC_RP2040
/* Maximum watchdog time is halved due to errata RP2040-E1 */
#define RPI_PICO_WDT_TIME_MULTIPLICATION_FACTOR 2
#else
#define RPI_PICO_WDT_TIME_MULTIPLICATION_FACTOR 1
#endif
#define RPI_PICO_MAX_WDT_TIME (0xffffff / RPI_PICO_WDT_TIME_MULTIPLICATION_FACTOR)

/* Watchdog requires a 1MHz clock source, divided from the crystal oscillator */
#define RPI_PICO_CLK_REF_FREQ_WDT_TICK_DIVISOR 1000000

struct wdt_rpi_pico_data {
	uint8_t reset_type;
	uint32_t load;
	bool enabled;
};

struct wdt_rpi_pico_config {
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
};

static int wdt_rpi_pico_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_rpi_pico_config *config = dev->config;
	struct wdt_rpi_pico_data *data = dev->data;
	uint32_t ref_clk;
	int err;

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) == 1) {
		return -ENOTSUP;
	}

	hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);

	psm_hw->wdsel = 0;

	/* TODO: Handle individual core reset when SMP support for RP2040 is added */
	if (data->reset_type == WDT_FLAG_RESET_SOC) {
		hw_set_bits(&psm_hw->wdsel, PSM_WDSEL_BITS);
	} else if (data->reset_type == WDT_FLAG_RESET_CPU_CORE) {
		hw_set_bits(&psm_hw->wdsel, PSM_WDSEL_PROC0_BITS);
	}

	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0) {
		hw_clear_bits(&watchdog_hw->ctrl,
			      (WATCHDOG_CTRL_PAUSE_JTAG_BITS | WATCHDOG_CTRL_PAUSE_DBG0_BITS |
			       WATCHDOG_CTRL_PAUSE_DBG1_BITS));
	} else {
		hw_set_bits(&watchdog_hw->ctrl,
			    (WATCHDOG_CTRL_PAUSE_JTAG_BITS | WATCHDOG_CTRL_PAUSE_DBG0_BITS |
			     WATCHDOG_CTRL_PAUSE_DBG1_BITS));
	}

	watchdog_hw->load = data->load;

	/* Zero out the scratch registers so that the module reboots at the
	 * default program counter
	 */
	watchdog_hw->scratch[4] = 0;
	watchdog_hw->scratch[5] = 0;
	watchdog_hw->scratch[6] = 0;
	watchdog_hw->scratch[7] = 0;

	hw_set_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);

	data->enabled = true;

	err = clock_control_on(config->clk_dev, config->clk_id);
	if (err < 0) {
		return err;
	}

	err = clock_control_get_rate(config->clk_dev, config->clk_id, &ref_clk);
	if (err < 0) {
		return err;
	}

#ifdef CONFIG_SOC_RP2040
	watchdog_hw->tick = (ref_clk / RPI_PICO_CLK_REF_FREQ_WDT_TICK_DIVISOR) |
			    WATCHDOG_TICK_ENABLE_BITS;
#else
	ticks_hw->ticks[TICK_WATCHDOG].cycles = ref_clk / RPI_PICO_CLK_REF_FREQ_WDT_TICK_DIVISOR;
	ticks_hw->ticks[TICK_WATCHDOG].ctrl = TICKS_WATCHDOG_CTRL_ENABLE_BITS;
#endif

	return 0;
}

static int wdt_rpi_pico_disable(const struct device *dev)
{
	struct wdt_rpi_pico_data *data = dev->data;

	if (data->enabled == false) {
		return -EFAULT;
	}

	hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);

	data->enabled = false;

	return 0;
}

static int wdt_rpi_pico_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct wdt_rpi_pico_data *data = dev->data;

	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	} else if (cfg->window.max * USEC_PER_MSEC > RPI_PICO_MAX_WDT_TIME) {
		return -EINVAL;
	} else if (cfg->callback != NULL) {
		return -ENOTSUP;
	} else if ((cfg->flags & WDT_FLAG_RESET_MASK) == WDT_FLAG_RESET_NONE) {
		/* The RP2040 does technically support this mode, but requires
		 * a program counter and stack pointer value to be set,
		 * therefore do not allow configuring in this mode
		 */
		return -EINVAL;
	}

	data->load = (cfg->window.max * USEC_PER_MSEC * RPI_PICO_WDT_TIME_MULTIPLICATION_FACTOR);
	data->reset_type = (cfg->flags & WDT_FLAG_RESET_MASK);

	return 0;
}

static int wdt_rpi_pico_feed(const struct device *dev, int channel_id)
{
	struct wdt_rpi_pico_data *data = dev->data;

	if (channel_id != 0) {
		/* There is only one input to the watchdog */
		return -EINVAL;
	}

	if (data->enabled == false) {
		/* Watchdog is not running so does not need to be fed */
		return -EINVAL;
	}

	watchdog_hw->load = data->load;

	return 0;
}

static int wdt_rpi_pico_init(const struct device *dev)
{
#ifndef CONFIG_WDT_DISABLE_AT_BOOT
	return wdt_rpi_pico_setup(dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
#endif

	return 0;
}

static DEVICE_API(wdt, wdt_rpi_pico_driver_api) = {
	.setup = wdt_rpi_pico_setup,
	.disable = wdt_rpi_pico_disable,
	.install_timeout = wdt_rpi_pico_install_timeout,
	.feed = wdt_rpi_pico_feed,
};

#define WDT_RPI_PICO_WDT_DEVICE(idx)                                                               \
	static const struct wdt_rpi_pico_config wdt_##idx##_config = {                             \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),				   \
		.clk_id = (clock_control_subsys_t)DT_INST_PHA_BY_IDX(idx, clocks, 0, clk_id),	   \
	};                                                                                         \
	static struct wdt_rpi_pico_data wdt_##idx##_data = {                                       \
		.reset_type = WDT_FLAG_RESET_SOC,                                                  \
		.load = (CONFIG_WDT_RPI_PICO_INITIAL_TIMEOUT *                                     \
			 RPI_PICO_WDT_TIME_MULTIPLICATION_FACTOR),                                 \
		.enabled = false                                                                   \
	};                                                                                         \
	DEVICE_DT_DEFINE(DT_NODELABEL(wdt##idx), wdt_rpi_pico_init, NULL, &wdt_##idx##_data,       \
			 &wdt_##idx##_config, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,    \
			 &wdt_rpi_pico_driver_api)

DT_INST_FOREACH_STATUS_OKAY(WDT_RPI_PICO_WDT_DEVICE);
