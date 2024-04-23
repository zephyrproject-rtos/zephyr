/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 * Copyright (c) 2018 qianfan Zhao
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_watchdog

#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_iwdg.h>
#include <stm32_ll_system.h>
#include <errno.h>

#include "wdt_iwdg_stm32.h"

#define IWDG_PRESCALER_MIN	(4U)

#if defined(LL_IWDG_PRESCALER_1024)
#define IWDG_PRESCALER_MAX (1024U)
#else
#define IWDG_PRESCALER_MAX (256U)
#endif

#define IWDG_RELOAD_MIN		(0x0000U)
#define IWDG_RELOAD_MAX		(0x0FFFU)

/* Minimum timeout in microseconds. */
#define IWDG_TIMEOUT_MIN	(IWDG_PRESCALER_MIN * (IWDG_RELOAD_MIN + 1U) \
				 * USEC_PER_SEC / LSI_VALUE)

/* Maximum timeout in microseconds. */
#define IWDG_TIMEOUT_MAX	((uint64_t)IWDG_PRESCALER_MAX * \
				 (IWDG_RELOAD_MAX + 1U) * \
				 USEC_PER_SEC / LSI_VALUE)

#define IS_IWDG_TIMEOUT(__TIMEOUT__)		\
	(((__TIMEOUT__) >= IWDG_TIMEOUT_MIN) &&	\
	 ((__TIMEOUT__) <= IWDG_TIMEOUT_MAX))

/*
 * Status register needs 5 LSI clock cycles divided by prescaler to be updated.
 * With highest prescaler and considering clock variation, we will wait
 * maximum 6 cycles (48 ms at 32 kHz) for register update.
 */
#define IWDG_SR_UPDATE_TIMEOUT	(6U * IWDG_PRESCALER_MAX * \
				 MSEC_PER_SEC / LSI_VALUE)

/**
 * @brief Calculates prescaler & reload values.
 *
 * @param timeout Timeout value in microseconds.
 * @param prescaler Pointer to prescaler value.
 * @param reload Pointer to reload value.
 */
static void iwdg_stm32_convert_timeout(uint32_t timeout,
				       uint32_t *prescaler,
				       uint32_t *reload)
{
	uint16_t divider = 4U;
	uint8_t shift = 0U;

	/* Convert timeout to LSI clock ticks. */
	uint32_t ticks = (uint64_t)timeout * LSI_VALUE / USEC_PER_SEC;

	while ((ticks / divider) > IWDG_RELOAD_MAX) {
		shift++;
		divider = 4U << shift;
	}

	/*
	 * Value of the 'shift' variable corresponds to the
	 * defines of LL_IWDG_PRESCALER_XX type.
	 */
	*prescaler = shift;
	*reload = (uint32_t)(ticks / divider) - 1U;
}

static int iwdg_stm32_setup(const struct device *dev, uint8_t options)
{
	struct iwdg_stm32_data *data = IWDG_STM32_DATA(dev);
	IWDG_TypeDef *iwdg = IWDG_STM32_STRUCT(dev);
	uint32_t tickstart;

	/* Deactivate running when debugger is attached. */
	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
#if defined(CONFIG_SOC_SERIES_STM32F0X)
		LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_DBGMCU);
#elif defined(CONFIG_SOC_SERIES_STM32C0X) || defined(CONFIG_SOC_SERIES_STM32G0X)
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_DBGMCU);
#elif defined(CONFIG_SOC_SERIES_STM32L0X)
		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_DBGMCU);
#endif
#if defined(CONFIG_SOC_SERIES_STM32H7X)
		LL_DBGMCU_APB4_GRP1_FreezePeriph(LL_DBGMCU_APB4_GRP1_IWDG1_STOP);
#else
		LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_IWDG_STOP);
#endif
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		return -ENOTSUP;
	}

	/* Enable the IWDG now and write IWDG registers at the same time */
	LL_IWDG_Enable(iwdg);
	LL_IWDG_EnableWriteAccess(iwdg);
	/* Write the prescaler and reload counter to the IWDG registers*/
	LL_IWDG_SetPrescaler(iwdg, data->prescaler);
	LL_IWDG_SetReloadCounter(iwdg, data->reload);

	tickstart = k_uptime_get_32();

	/* Wait for the update operation completed */
	while (LL_IWDG_IsReady(iwdg) == 0) {
		if ((k_uptime_get_32() - tickstart) > IWDG_SR_UPDATE_TIMEOUT) {
			return -ENODEV;
		}
	}

	/* Reload counter just before leaving */
	LL_IWDG_ReloadCounter(iwdg);

	return 0;
}

static int iwdg_stm32_disable(const struct device *dev)
{
	/* watchdog cannot be stopped once started */
	ARG_UNUSED(dev);

	return -EPERM;
}

static int iwdg_stm32_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *config)
{
	struct iwdg_stm32_data *data = IWDG_STM32_DATA(dev);
	uint32_t timeout = config->window.max * USEC_PER_MSEC;
	uint32_t prescaler = 0U;
	uint32_t reload = 0U;

	if (config->callback != NULL) {
		return -ENOTSUP;
	}

	/* Calculating parameters to be applied later, on setup */
	iwdg_stm32_convert_timeout(timeout, &prescaler, &reload);

	if (!(IS_IWDG_TIMEOUT(timeout) && IS_IWDG_PRESCALER(prescaler) &&
	    IS_IWDG_RELOAD(reload))) {
		/* One of the parameters provided is invalid */
		return -EINVAL;
	}

	/* Store the calculated values to write in the iwdg registers */
	data->prescaler = prescaler;
	data->reload = reload;

	/* Do not enable and update the iwdg here but during wdt_setup() */
	return 0;
}

static int iwdg_stm32_feed(const struct device *dev, int channel_id)
{
	IWDG_TypeDef *iwdg = IWDG_STM32_STRUCT(dev);

	ARG_UNUSED(channel_id);
	LL_IWDG_ReloadCounter(iwdg);

	return 0;
}

static const struct wdt_driver_api iwdg_stm32_api = {
	.setup = iwdg_stm32_setup,
	.disable = iwdg_stm32_disable,
	.install_timeout = iwdg_stm32_install_timeout,
	.feed = iwdg_stm32_feed,
};

static int iwdg_stm32_init(const struct device *dev)
{
#ifndef CONFIG_WDT_DISABLE_AT_BOOT
	struct wdt_timeout_cfg config = {
		.window.max = CONFIG_IWDG_STM32_INITIAL_TIMEOUT
	};
	/* Watchdog should be configured and started by `wdt_setup`*/
	iwdg_stm32_install_timeout(dev, &config);
	iwdg_stm32_setup(dev, 0); /* no option specified */
#endif

	/*
	 * The ST production value for the option bytes where WDG_SW bit is
	 * present is 0x00FF55AA, namely the Software watchdog mode is
	 * enabled by default.
	 * If the IWDG is started by either hardware option or software access,
	 * the LSI oscillator is forced ON and cannot be disabled.
	 *
	 * t_IWDG(ms) = t_LSI(ms) x 4 x 2^(IWDG_PR[2:0]) x (IWDG_RLR[11:0] + 1)
	 */

	return 0;
}

static struct iwdg_stm32_data iwdg_stm32_dev_data = {
	.Instance = (IWDG_TypeDef *)DT_INST_REG_ADDR(0)
};

DEVICE_DT_INST_DEFINE(0, iwdg_stm32_init, NULL,
		    &iwdg_stm32_dev_data, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &iwdg_stm32_api);
