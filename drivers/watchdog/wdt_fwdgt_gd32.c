/*
 * Copyright (c) 2022 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_fwdgt

#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>

#include <gd32_fwdgt.h>

LOG_MODULE_REGISTER(wdt_fwdgt_gd32, CONFIG_WDT_LOG_LEVEL);

#define FWDGT_RELOAD_MAX    (0xFFFU)
#define FWDGT_PRESCALER_MAX (256U)

#if defined(CONFIG_GD32_HAS_IRC_32K)
#define RCU_IRC_LOW_SPEED RCU_IRC32K
#elif defined(CONFIG_GD32_HAS_IRC_40K)
#define RCU_IRC_LOW_SPEED RCU_IRC40K
#else
#error IRC frequency was not configured
#endif

#define IS_VALID_FWDGT_PRESCALER(psc)                                          \
	(((psc) == FWDGT_PSC_DIV4) || ((psc) == FWDGT_PSC_DIV8) ||             \
	 ((psc) == FWDGT_PSC_DIV16) || ((psc) == FWDGT_PSC_DIV32) ||           \
	 ((psc) == FWDGT_PSC_DIV64) || ((psc) == FWDGT_PSC_DIV128) ||          \
	 ((psc) == FWDGT_PSC_DIV256))

#define FWDGT_INITIAL_TIMEOUT DT_INST_PROP(0, initial_timeout_ms)

#if (FWDGT_INITIAL_TIMEOUT <= 0)
#error Must be initial-timeout > 0
#elif (FWDGT_INITIAL_TIMEOUT >                                                 \
	(FWDGT_PRESCALER_MAX * FWDGT_RELOAD_MAX * MSEC_PER_SEC /               \
	CONFIG_GD32_LOW_SPEED_IRC_FREQUENCY))
#error Must be initial-timeout <= (256 * 4095 * 1000 / GD32_LOW_SPEED_IRC_FREQUENCY)
#endif

/**
 * @brief Calculates FWDGT config value from timeout.
 *
 * @param timeout Timeout value in milliseconds.
 * @param prescaler Pointer to the storage of prescaler value.
 * @param reload Pointer to the storage of reload value.
 *
 * @return 0 on success, -EINVAL if the timeout is out of range
 */
static int gd32_fwdgt_calc_timeout(uint32_t timeout, uint32_t *prescaler,
				   uint32_t *reload)
{
	uint16_t divider = 4U;
	uint8_t shift = 0U;
	uint32_t ticks = (uint64_t)CONFIG_GD32_LOW_SPEED_IRC_FREQUENCY *
			 timeout / MSEC_PER_SEC;

	while ((ticks / divider) > FWDGT_RELOAD_MAX) {
		shift++;
		divider = 4U << shift;
	}

	if (!IS_VALID_FWDGT_PRESCALER(PSC_PSC(shift)) || timeout == 0U) {
		return -EINVAL;
	}

	/* convert the 'shift' to prescaler value */
	*prescaler = PSC_PSC(shift);
	*reload = (ticks / divider) - 1U;

	return 0;
}

static int gd32_fwdgt_setup(const struct device *dev, uint8_t options)
{
	ARG_UNUSED(dev);

	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) != 0U) {
#if CONFIG_GD32_DBG_SUPPORT
		dbg_periph_enable(DBG_FWDGT_HOLD);
#else
		LOG_ERR("Debug support not enabled");
		return -ENOTSUP;
#endif
	}

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0U) {
		LOG_ERR("WDT_OPT_PAUSE_IN_SLEEP not supported");
		return -ENOTSUP;
	}

	fwdgt_enable();

	return 0;
}

static int gd32_fwdgt_disable(const struct device *dev)
{
	/* watchdog cannot be stopped once started */
	ARG_UNUSED(dev);

	return -EPERM;
}

static int gd32_fwdgt_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *config)
{
	uint32_t prescaler = 0U;
	uint32_t reload = 0U;
	ErrStatus errstat = ERROR;

	/* Callback is not supported by FWDGT */
	if (config->callback != NULL) {
		LOG_ERR("callback not supported by FWDGT");
		return -ENOTSUP;
	}

	/* Calculate prescaler and reload value from timeout value */
	if (gd32_fwdgt_calc_timeout(config->window.max, &prescaler,
				    &reload) != 0) {
		LOG_ERR("window max is out of range");
		return -EINVAL;
	}

	/* Configure and run FWDGT */
	fwdgt_write_enable();
	errstat = fwdgt_config(reload, prescaler);
	if (errstat != SUCCESS) {
		LOG_ERR("fwdgt_config() failed: %d", errstat);
		return -EINVAL;
	}
	fwdgt_write_disable();

	return 0;
}

static int gd32_fwdgt_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(channel_id);

	fwdgt_counter_reload();

	return 0;
}

static const struct wdt_driver_api fwdgt_gd32_api = {
	.setup = gd32_fwdgt_setup,
	.disable = gd32_fwdgt_disable,
	.install_timeout = gd32_fwdgt_install_timeout,
	.feed = gd32_fwdgt_feed,
};

static int gd32_fwdgt_init(const struct device *dev)
{
	int ret = 0;

	/* Turn on and wait stabilize system clock oscillator. */
	rcu_osci_on(RCU_IRC_LOW_SPEED);
	while (!rcu_osci_stab_wait(RCU_IRC_LOW_SPEED)) {
	}

#if !defined(CONFIG_WDT_DISABLE_AT_BOOT)
	const struct wdt_timeout_cfg config = {
		.window.max = FWDGT_INITIAL_TIMEOUT
	};

	ret = gd32_fwdgt_install_timeout(dev, &config);
#endif

	return ret;
}

DEVICE_DT_INST_DEFINE(0, gd32_fwdgt_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fwdgt_gd32_api);
