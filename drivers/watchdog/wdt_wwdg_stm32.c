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
#include <zephyr/sys/clock.h>

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
	(ABS_DIFF_UINT(__TIMEOUT__, __TIMEOUT_GOLDEN__) < \
	 WWDG_TIMEOUT_ERROR_MARGIN(__TIMEOUT_GOLDEN__))

static void wwdg_stm32_irq_config(const struct device *dev);

/**
 * @brief Get the WWDG bus clock rate.
 *
 * @param dev Pointer to device structure.
 * @param pclk Pointer to the clock rate in Hz.
 * @retval 0 On success.
 * @retval -EIO The clock rate could not be read.
 */
static int wwdg_stm32_get_pclk(const struct device *dev, uint32_t *pclk)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct wwdg_stm32_config *cfg = WWDG_STM32_CFG(dev);

	if ((clock_control_get_rate(clk, (clock_control_subsys_t)&cfg->pclken, pclk) < 0) ||
	    (*pclk == 0U)) {
		LOG_ERR("Failed call clock_control_get_rate");
		return -EIO;
	}

	return 0;
}

/**
 * @brief Calculates the timeout in microseconds.
 *
 * @param pclk The WWDG bus clock rate in Hz.
 * @param prescaler_exp The prescaler exponent value(Base 2).
 * @param counter The counter value.
 * @return The timeout calculated in microseconds.
 */
static uint32_t wwdg_stm32_get_timeout(uint32_t pclk,
				       uint32_t prescaler_exp,
				       uint32_t counter)
{
	uint32_t ticks = (counter & 0x3FU) + 1U;
	uint64_t divider = WWDG_INTERNAL_DIVIDER << prescaler_exp;
	uint64_t num = (uint64_t)ticks * divider * (uint64_t)USEC_PER_SEC;
	uint64_t time_us64 = num / (uint64_t)pclk;

	__ASSERT_NO_MSG(time_us64 <= UINT32_MAX);

	return (uint32_t)time_us64;
}

/**
 * @brief Converts a duration to a number of WWDG ticks, rounded up.
 *
 * @param pclk The WWDG bus clock rate in Hz.
 * @param time_us The duration in microseconds.
 * @param prescaler_exp The prescaler exponent value(Base 2).
 * @return The number of ticks.
 */
static uint64_t wwdg_stm32_us_to_ticks(uint32_t pclk, uint32_t time_us,
				       uint32_t prescaler_exp)
{
	uint64_t divider = (uint64_t)WWDG_INTERNAL_DIVIDER << prescaler_exp;
	uint64_t num = (uint64_t)time_us * (uint64_t)pclk;
	uint64_t den = (uint64_t)USEC_PER_SEC * divider;

	return DIV_ROUND_UP(num, den);
}

/**
 * @brief Calculates prescaler & counter values.
 *
 * The smallest prescaler that can count @p timeout is selected. The
 * counter expires after (counter - WWDG_COUNTER_MIN + 1) ticks.
 *
 * @param pclk The WWDG bus clock rate in Hz.
 * @param timeout Timeout value in microseconds.
 * @param prescaler_exp Pointer to prescaler exponent value(Base 2).
 * @param counter Pointer to counter value.
 */
static void wwdg_stm32_convert_timeout(uint32_t pclk,
				       uint32_t timeout,
				       uint32_t *prescaler_exp,
				       uint32_t *counter)
{
	for (uint32_t psc_exp = 0U; psc_exp <= WWDG_PRESCALER_EXPONENT_MAX; psc_exp++) {
		uint64_t ticks = wwdg_stm32_us_to_ticks(pclk, timeout, psc_exp);

		if (IN_RANGE(ticks, 1U, WWDG_COUNTER_MAX - WWDG_COUNTER_MIN + 1U)) {
			*counter = WWDG_COUNTER_MIN + (uint32_t)ticks - 1U;
			*prescaler_exp = psc_exp;
			return;
		}
	}

	/* timeout zero or longer than wwdg can provide, set to max possible value */
	*counter = WWDG_COUNTER_MAX;
	*prescaler_exp = WWDG_PRESCALER_EXPONENT_MAX;
}

/**
 * @brief Compute the window value
 *
 * Once the WWDG counter is reloaded with @p counter after a feed, it
 * cannot be fed again until it has reached the window value. Feeding
 * earlier generates a reset. The number of ticks is rounded up.
 *
 * A feed does not reset the prescaler, so the first tick after a feed
 * comes up to one tick early. The window can open up to one tick before
 * @p min_timeout rounded up to whole ticks, and a feed is only
 * guaranteed to be accepted once that rounded value has elapsed.
 *
 * @param pclk The WWDG bus clock rate in Hz.
 * @param min_timeout Minimum time between two feeds in microseconds.
 * @param prescaler_exp The prescaler exponent value(Base 2).
 * @param counter The counter value.
 * @param window Pointer to the window value.
 * @retval 0 On success.
 * @retval -EINVAL @p min_timeout leaves no tick to feed before the reset.
 */
static int wwdg_stm32_convert_window(uint32_t pclk,
				     uint32_t min_timeout,
				     uint32_t prescaler_exp,
				     uint32_t counter,
				     uint32_t *window)
{
	uint64_t ticks = wwdg_stm32_us_to_ticks(pclk, min_timeout, prescaler_exp);

	/* A feed is accepted once the counter equals the window, so 0x40 still
	 * leaves a one-tick window.
	 */
	if (ticks > (counter - WWDG_COUNTER_MIN)) {
		return -EINVAL;
	}

	*window = counter - (uint32_t)ticks;

	return 0;
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

#if defined(CONFIG_SOC_SERIES_STM32H7X) || defined(CONFIG_SOC_SERIES_STM32MP2X)
		LL_DBGMCU_APB3_GRP1_FreezePeriph(LL_DBGMCU_APB3_GRP1_WWDG1_STOP);
#elif defined(CONFIG_SOC_SERIES_STM32C5X)
		LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_WWDG_STOP);
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
	uint32_t calculated_timeout;
	uint32_t prescaler_exp = 0U;
	uint32_t counter = 0U;
	uint32_t window;
	uint32_t pclk;
	int ret;

	/* Reloading the counter above a reduced window would reset the MCU */
	if (LL_WWDG_IsEnabled(wwdg) != 0U) {
		return -EBUSY;
	}

	if (config->window.max > (UINT32_MAX / USEC_PER_MSEC)) {
		return -EINVAL;
	}

	if (config->window.min > config->window.max) {
		return -EINVAL;
	}

	const uint32_t timeout = config->window.max * USEC_PER_MSEC;
	const uint32_t window_start = config->window.min * USEC_PER_MSEC;

	ret = wwdg_stm32_get_pclk(dev, &pclk);
	if (ret < 0) {
		return ret;
	}

	if (config->callback != NULL) {
		data->callback = config->callback;
	}

	wwdg_stm32_convert_timeout(pclk, timeout, &prescaler_exp, &counter);
	calculated_timeout = wwdg_stm32_get_timeout(pclk, prescaler_exp, counter);

	LOG_DBG("prescaler: %d", (1 << prescaler_exp));
	LOG_DBG("Desired WDT: %d us", timeout);
	LOG_DBG("Set WDT:     %d us", calculated_timeout);

	/* Rounding up to a whole tick or clamping to the longest timeout can
	 * exceed the tolerated error.
	 */
	if (!IS_WWDG_TIMEOUT(timeout, calculated_timeout)) {
		return -EINVAL;
	}

	/* A zero minimum keeps the window equal to the counter, so the WWDG can
	 * be fed immediately.
	 */
	ret = wwdg_stm32_convert_window(pclk, window_start, prescaler_exp, counter, &window);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Desired window: %d us", window_start);
	LOG_DBG("Set window:     %d (counter %d)", window, counter);

	data->counter = counter;

	/* Configure WWDG */
	/* Set the programmable prescaler */
	LL_WWDG_SetPrescaler(wwdg,
		(prescaler_exp << WWDG_PRESCALER_POS) & WWDG_PRESCALER_MASK);

	LL_WWDG_SetWindow(wwdg, window);
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
			if (data->callback != NULL) {
				data->callback(dev, 0);
			}
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
	.pclken = STM32_DT_INST_CLOCK_INFO(0),
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
