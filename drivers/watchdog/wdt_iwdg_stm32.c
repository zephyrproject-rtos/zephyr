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
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
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

#ifdef CONFIG_IWDG_STM32_EARLY_WAKEUP

void iwdg_stm32_isr(const struct device *dev)
{
	struct iwdg_stm32_data *data = dev->data;
	IWDG_TypeDef *iwdg = ((const struct iwdg_stm32_config *)dev->config)->instance;

	if (LL_IWDG_IsEnabledIT_EWI(iwdg) && LL_IWDG_IsActiveFlag_EWIF(iwdg)) {
		LL_IWDG_ClearFlag_EWIF(iwdg);
		if (data->callback != NULL) {
			data->callback(dev, 0);
		}
	}
}

static void iwdg_stm32_irq_config(const struct device *dev)
{
	IWDG_TypeDef *idg = ((const struct iwdg_stm32_config *)dev->config)->instance;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), iwdg_stm32_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));

	LL_IWDG_ClearFlag_EWIF(idg);

	while (LL_IWDG_IsActiveFlag_EWU(idg)) {
	}
	LL_IWDG_EnableIT_EWI(idg);
}

#endif /* CONFIG_IWDG_STM32_EARLY_WAKEUP */

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
	const struct iwdg_stm32_config *cfg = dev->config;
	struct iwdg_stm32_data *data = dev->data;
	uint32_t tickstart;

	/* Deactivate running when debugger is attached. */
	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
#if defined(CONFIG_SOC_SERIES_STM32WB0X)
		/* STM32WB0 watchdog does not support halt by debugger */
		return -ENOTSUP;
#elif defined(CONFIG_SOC_SERIES_STM32F0X)
		LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_DBGMCU);
#elif defined(CONFIG_SOC_SERIES_STM32C0X) || defined(CONFIG_SOC_SERIES_STM32G0X)
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_DBGMCU);
#elif defined(CONFIG_SOC_SERIES_STM32L0X)
		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_DBGMCU);
#elif defined(CONFIG_SOC_SERIES_STM32H7X)
		LL_DBGMCU_APB4_GRP1_FreezePeriph(LL_DBGMCU_APB4_GRP1_IWDG1_STOP);
#elif defined(CONFIG_SOC_SERIES_STM32H7RSX)
		LL_DBGMCU_APB4_GRP1_FreezePeriph(LL_DBGMCU_APB4_GRP1_IWDG_STOP);
#elif defined(CONFIG_SOC_SERIES_STM32MP2X)
		LL_DBGMCU_APB3_GRP1_FreezePeriph(LL_DBGMCU_APB3_GRP1_IWDG4_STOP);
#else
		LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_IWDG_STOP);
#endif
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		return -ENOTSUP;
	}

	/* Enable the IWDG now and write IWDG registers at the same time */
	LL_IWDG_Enable(cfg->instance);
	LL_IWDG_EnableWriteAccess(cfg->instance);
	/* Write the prescaler and reload counter to the IWDG registers*/
	LL_IWDG_SetPrescaler(cfg->instance, data->prescaler);
	LL_IWDG_SetReloadCounter(cfg->instance, data->reload);

	tickstart = k_uptime_get_32();

#ifdef CONFIG_IWDG_STM32_EARLY_WAKEUP
	if (data->reload < 2U) {
		/* Early wake-up is not possible if counter is
		 * reloaded with value 1 as the system will be
		 * reset immediately when the counter decrements.
		 * (Note: reload=1 is NOT RECOMMENDED per RefMan)
		 */
		return -EINVAL;
	}

	/* If Kconfig is higher than reload, set early wake-up
	 * as high as possible (= reload - 1); otherwise, set
	 * to the requested value.
	 */
	uint32_t ewi_time = MIN(CONFIG_IWDG_STM32_EWI_TIME, data->reload);

	LL_IWDG_SetEwiTime(cfg->instance, ewi_time);

	iwdg_stm32_irq_config(dev);
#endif /* CONFIG_IWDG_STM32_EARLY_WAKEUP */

	/* Wait for the update operation completed */
	while (LL_IWDG_IsReady(cfg->instance) == 0) {
		if ((k_uptime_get_32() - tickstart) > IWDG_SR_UPDATE_TIMEOUT) {
			return -ENODEV;
		}
	}

	/* Reload counter just before leaving */
	LL_IWDG_ReloadCounter(cfg->instance);

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
	struct iwdg_stm32_data *data = dev->data;
	uint32_t timeout = config->window.max * USEC_PER_MSEC;
	uint32_t prescaler = 0U;
	uint32_t reload = 0U;

	if (config->callback != NULL) {
		if (IS_ENABLED(CONFIG_IWDG_STM32_EARLY_WAKEUP)) {
			data->callback = config->callback;
		} else {
			return -ENOTSUP;
		}
	}
	if (data->reload) {
		/* Timeout has already been configured */
		return -ENOMEM;
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
	const struct iwdg_stm32_config *cfg = dev->config;

	ARG_UNUSED(channel_id);
	LL_IWDG_ReloadCounter(cfg->instance);

	return 0;
}

static DEVICE_API(wdt, iwdg_stm32_api) = {
	.setup = iwdg_stm32_setup,
	.disable = iwdg_stm32_disable,
	.install_timeout = iwdg_stm32_install_timeout,
	.feed = iwdg_stm32_feed,
};

static int iwdg_stm32_init(const struct device *dev)
{
/* Enable watchdog clock if needed */
#if DT_INST_NODE_HAS_PROP(0, clocks)
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct stm32_pclken clk_cfg = STM32_CLOCK_INFO(0, DT_DRV_INST(0));
	int err = clock_control_on(clk, (clock_control_subsys_t)&clk_cfg);

	if (err < 0) {
		return err;
	}
#if defined(CONFIG_SOC_SERIES_STM32WB0X)
	/**
	 * On STM32WB0, application must wait two slow clock cycles
	 * before accessing the IWDG IP after turning on the WDGEN
	 * bit in RCC registers. However, there is no register that
	 * can be polled for this event.
	 * To work around this limitation, force the IWDG to go
	 * through a reset cycle, which also takes two slow clock
	 * cycles, but can polled on (bit WDGRSTF of RCC_CIFR).
	 */

	/* Clear bit beforehand to avoid early exit of polling loop */
	LL_RCC_ClearFlag_WDGRSTREL();

	/* Place IWDG under reset, then release the reset */
	LL_APB0_GRP1_ForceReset(LL_APB0_GRP1_PERIPH_WDG);
	LL_APB0_GRP1_ReleaseReset(LL_APB0_GRP1_PERIPH_WDG);
	while (!LL_RCC_IsActiveFlag_WDGRSTREL()) {
		/* Wait for IWDG reset release event,
		 * which takes two slow clock cycles
		 */
	}

	/* Clear WDRSTF bit after polling completes */
	LL_RCC_ClearFlag_WDGRSTREL();
#endif /* defined(CONFIG_SOC_SERIES_STM32WB0X) */
#endif /* DT_INST_NODE_HAS_PROP(0, clocks) */

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

static const struct iwdg_stm32_config iwdg_stm32_dev_cfg = {
	.instance = (IWDG_TypeDef *)DT_INST_REG_ADDR(0),
};

static struct iwdg_stm32_data iwdg_stm32_dev_data = {
	.callback = NULL,
};

DEVICE_DT_INST_DEFINE(0, iwdg_stm32_init, NULL,
		    &iwdg_stm32_dev_data, &iwdg_stm32_dev_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &iwdg_stm32_api);
