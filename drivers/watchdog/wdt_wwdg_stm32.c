/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_window_watchdog

#include <zephyr/drivers/watchdog.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_wwdg.h>
#include <stm32_ll_system.h>
#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>

#include "wdt_wwdg_stm32.h"

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_wwdg_stm32);

#define WWDG_INTERNAL_DIVIDER   4096U
#define WWDG_RESET_LIMIT    WWDG_COUNTER_MIN
#define WWDG_COUNTER_MIN    0x40
#define WWDG_COUNTER_MAX    0x7f

#if defined WWDG_CFR_WDGTB_Pos
#define WWDG_PRESCALER_POS   WWDG_CFR_WDGTB_Pos
#define WWDG_PRESCALER_MASK  WWDG_CFR_WDGTB_Msk
#else
#error "WWDG CFR WDGTB position not defined for soc"
#endif

/*
 * additionally to the internal divider, the clock is divided by a
 * programmable prescaler.
 */
#if defined(LL_WWDG_PRESCALER_128)
#define WWDG_PRESCALER_EXPONENT_MAX 7 /* 2^7 = 128 */
#elif defined(LL_WWDG_PRESCALER_8)
#define WWDG_PRESCALER_EXPONENT_MAX 3 /* 2^3 = 8 */
#endif

/* The timeout of the WWDG in milliseconds is calculated by the below formula:
 *
 * t_WWDG = 1000 * ((counter & 0x3F) + 1) / f_WWDG (ms)
 *
 * where:
 *  - t_WWDG: WWDG timeout
 *  - counter: a value in [0x40, 0x7F] representing the cycles before timeout.
 *             Giving the counter a value below 0x40, will result in an
 *             immediate system reset. A reset is produced when the counter
 *             rolls over from 0x40 to 0x3F.
 *  - f_WWDG: the frequency of the WWDG clock. This can be calculated by the
 *            below formula:
 *    f_WWDG = f_PCLK / (4096 * prescaler) (Hz)
 *    where:
 *     - f_PCLK: the clock frequency of the system
 *     - 4096: the constant internal divider
 *     - prescaler: the programmable divider with valid values of 1, 2, 4 or 8,
 *                  and for some series additionally 16, 32, 64 and 128
 *
 * The minimum timeout is calculated with:
 *  - counter = 0x40
 *  - prescaler = 1
 * The maximum timeout is calculated with:
 *  - counter = 0x7F
 *  - prescaler = 8
 *
 * E.g. for f_PCLK = 2MHz
 *  t_WWDG_min = 1000 * ((0x40 & 0x3F) + 1) / (2000000 / (4096 * 1))
 *             = 2.048 ms
 *  t_WWDG_max = 1000 * ((0x7F & 0x3F) + 1) / (2000000 / (4096 * 8))
 *             = 1048.576 ms
 */

#define ABS_DIFF_UINT(a, b)  ((a) > (b) ? (a) - (b) : (b) - (a))
#define WWDG_TIMEOUT_ERROR_MARGIN(__TIMEOUT__)   (__TIMEOUT__ / 10)
#define IS_WWDG_TIMEOUT(__TIMEOUT_GOLDEN__, __TIMEOUT__)  \
	(__TIMEOUT__ - __TIMEOUT_GOLDEN__) < \
	WWDG_TIMEOUT_ERROR_MARGIN(__TIMEOUT_GOLDEN__)

static void wwdg_stm32_irq_config(const struct device *dev);

static uint32_t wwdg_stm32_get_pclk(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct wwdg_stm32_config *cfg = WWDG_STM32_CFG(dev);
	uint32_t pclk_rate;

	if (clock_control_get_rate(clk, (clock_control_subsys_t) &cfg->pclken,
			       &pclk_rate) < 0) {
		LOG_ERR("Failed call clock_control_get_rate");
		return -EIO;
	}

	return pclk_rate;
}

/**
 * @brief Calculates the timeout in microseconds.
 *
 * @param dev Pointer to device structure.
 * @param prescaler_exp The prescaler exponent value(Base 2).
 * @param counter The counter value.
 * @return The timeout calculated in microseconds.
 */
static uint32_t wwdg_stm32_get_timeout(const struct device *dev,
				       uint32_t prescaler_exp,
				       uint32_t counter)
{
	uint32_t divider = WWDG_INTERNAL_DIVIDER * (1 << prescaler_exp);
	float f_wwdg = (float)wwdg_stm32_get_pclk(dev) / divider;

	return USEC_PER_SEC * (((counter & 0x3F) + 1) / f_wwdg);
}

/**
 * @brief Calculates prescaler & counter values.
 *
 * @param dev Pointer to device structure.
 * @param timeout Timeout value in microseconds.
 * @param prescaler_exp Pointer to prescaler exponent value(Base 2).
 * @param counter Pointer to counter value.
 */
static void wwdg_stm32_convert_timeout(const struct device *dev,
				       uint32_t timeout,
				       uint32_t *prescaler_exp,
				       uint32_t *counter)
{
	uint32_t clock_freq = wwdg_stm32_get_pclk(dev);

	/* Convert timeout to seconds. */
	float timeout_s = (float)timeout / USEC_PER_SEC;
	float wwdg_freq;

	*prescaler_exp = 0U;
	*counter = 0;

	for (*prescaler_exp = 0; *prescaler_exp <= WWDG_PRESCALER_EXPONENT_MAX;
	     (*prescaler_exp)++) {
		wwdg_freq = ((float)clock_freq) / WWDG_INTERNAL_DIVIDER
			     / (1 << *prescaler_exp);
		/* +1 to ceil the result, which may lose from truncation */
		*counter = (uint32_t)(timeout_s * wwdg_freq + 1) - 1;
		*counter += WWDG_RESET_LIMIT;

		if (*counter <= WWDG_COUNTER_MAX) {
			return;
		}
	}

	/* timeout longer than wwdg can provide, set to max possible value */
	*counter = WWDG_COUNTER_MAX;
	*prescaler_exp = WWDG_PRESCALER_EXPONENT_MAX;
}

static int wwdg_stm32_setup(const struct device *dev, uint8_t options)
{
	WWDG_TypeDef *wwdg = WWDG_STM32_STRUCT(dev);

	/* Deactivate running when debugger is attached. */
	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
#if defined(CONFIG_SOC_SERIES_STM32F0X)
		LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_DBGMCU);
#elif defined(CONFIG_SOC_SERIES_STM32L0X)
		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_DBGMCU);
#elif defined(CONFIG_SOC_SERIES_STM32C0X) || defined(CONFIG_SOC_SERIES_STM32G0X)
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_DBGMCU);
#endif
#if defined(CONFIG_SOC_SERIES_STM32H7X)
		LL_DBGMCU_APB3_GRP1_FreezePeriph(LL_DBGMCU_APB3_GRP1_WWDG1_STOP);
#elif defined(CONFIG_SOC_SERIES_STM32MP1X)
		LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_WWDG1_STOP);
#else
		LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_WWDG_STOP);
#endif /* CONFIG_SOC_SERIES_STM32H7X */
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		return -ENOTSUP;
	}

	/* Ensure that Early Wakeup Interrupt Flag is cleared */
	LL_WWDG_ClearFlag_EWKUP(wwdg);

	/* Enable the WWDG */
	LL_WWDG_Enable(wwdg);

	return 0;
}

static int wwdg_stm32_disable(const struct device *dev)
{
	/* watchdog cannot be stopped once started unless SOC gets a reset */
	ARG_UNUSED(dev);

	return -EPERM;
}

static int wwdg_stm32_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *config)
{
	struct wwdg_stm32_data *data = WWDG_STM32_DATA(dev);
	WWDG_TypeDef *wwdg = WWDG_STM32_STRUCT(dev);
	uint32_t timeout = config->window.max * USEC_PER_MSEC;
	uint32_t calculated_timeout;
	uint32_t prescaler_exp = 0U;
	uint32_t counter = 0U;

	if (config->callback != NULL) {
		data->callback = config->callback;
	}

	wwdg_stm32_convert_timeout(dev, timeout, &prescaler_exp, &counter);
	calculated_timeout = wwdg_stm32_get_timeout(dev, prescaler_exp, counter);

	LOG_DBG("prescaler: %d", (1 << prescaler_exp));
	LOG_DBG("Desired WDT: %d us", timeout);
	LOG_DBG("Set WDT:     %d us", calculated_timeout);

	if (!(IS_WWDG_COUNTER(counter) &&
	      IS_WWDG_TIMEOUT(timeout, calculated_timeout))) {
		/* One of the parameters provided is invalid */
		return -EINVAL;
	}

	data->counter = counter;

	/* Configure WWDG */
	/* Set the programmable prescaler */
	LL_WWDG_SetPrescaler(wwdg,
		(prescaler_exp << WWDG_PRESCALER_POS) & WWDG_PRESCALER_MASK);

	/* Set window the same as the counter to be able to feed the WWDG almost
	 * immediately
	 */
	LL_WWDG_SetWindow(wwdg, counter);
	LL_WWDG_SetCounter(wwdg, counter);

	return 0;
}

static int wwdg_stm32_feed(const struct device *dev, int channel_id)
{
	WWDG_TypeDef *wwdg = WWDG_STM32_STRUCT(dev);
	struct wwdg_stm32_data *data = WWDG_STM32_DATA(dev);

	ARG_UNUSED(channel_id);
	LL_WWDG_SetCounter(wwdg, data->counter);

	return 0;
}

void wwdg_stm32_isr(const struct device *dev)
{
	struct wwdg_stm32_data *data = WWDG_STM32_DATA(dev);
	WWDG_TypeDef *wwdg = WWDG_STM32_STRUCT(dev);

	if (LL_WWDG_IsEnabledIT_EWKUP(wwdg)) {
		if (LL_WWDG_IsActiveFlag_EWKUP(wwdg)) {
			LL_WWDG_ClearFlag_EWKUP(wwdg);
			data->callback(dev, 0);
		}
	}
}

static DEVICE_API(wdt, wwdg_stm32_api) = {
	.setup = wwdg_stm32_setup,
	.disable = wwdg_stm32_disable,
	.install_timeout = wwdg_stm32_install_timeout,
	.feed = wwdg_stm32_feed,
};

static int wwdg_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct wwdg_stm32_config *cfg = WWDG_STM32_CFG(dev);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken) != 0) {
		LOG_ERR("clock control on failed");
		return -EIO;
	}

	/* Enable IRQ, especially EWKUP, once the peripheral is clocked */
	wwdg_stm32_irq_config(dev);

	return 0;
}

static struct wwdg_stm32_data wwdg_stm32_dev_data = {
	.counter = WWDG_RESET_LIMIT,
	.callback = NULL
};

static struct wwdg_stm32_config wwdg_stm32_dev_config = {
	.pclken = {
		.enr = DT_INST_CLOCKS_CELL(0, bits),
		.bus = DT_INST_CLOCKS_CELL(0, bus)
	},
	.Instance = (WWDG_TypeDef *)DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, wwdg_stm32_init, NULL,
		    &wwdg_stm32_dev_data, &wwdg_stm32_dev_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wwdg_stm32_api);

static void wwdg_stm32_irq_config(const struct device *dev)
{
	WWDG_TypeDef *wwdg = WWDG_STM32_STRUCT(dev);

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    wwdg_stm32_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	LL_WWDG_ClearFlag_EWKUP(wwdg);
	LL_WWDG_EnableIT_EWKUP(wwdg);
}
