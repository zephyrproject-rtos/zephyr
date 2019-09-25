/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <watchdog.h>
#include <soc.h>
#include <errno.h>
#include <assert.h>
#include <clock_control/stm32_clock_control.h>
#include <drivers/clock_control.h>

#include "wdt_wwdg_stm32.h"

#define WWDG_INTERNAL_DIVIDER   4096U
#define WWDG_RESET_LIMIT    WWDG_COUNTER_MIN
#define WWDG_COUNTER_MIN    0x40
#define WWDG_COUNTER_MAX    0x7f

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
 *     - prescaler: the programmable divider with valid values of 1, 2, 4 or 8
 *
 * The minimum timeout is calculated with:
 *  - counter = 0x40
 *  - prescaler = 1
 * The maximim timeout is calculated with:
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
#define WWDG_TIMEOUT_ERROR_MARGIN   (100 * USEC_PER_MSEC)
#define IS_WWDG_TIMEOUT(__TIMEOUT_GOLDEN__, __TIMEOUT__)  \
	(ABS_DIFF_UINT(__TIMEOUT_GOLDEN__, __TIMEOUT__) < \
	 WWDG_TIMEOUT_ERROR_MARGIN)

static void wwdg_stm32_irq_config(struct device *dev);

static u32_t wwdg_stm32_get_pclk(struct device *dev)
{
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	const struct wwdg_stm32_config *cfg = WWDG_STM32_CFG(dev);
	u32_t pclk_rate;

	__ASSERT_NO_MSG(clk);

	clock_control_get_rate(clk, (clock_control_subsys_t *) &cfg->pclken,
			       &pclk_rate);

	return pclk_rate;
}

/**
 * @brief Calculates the timeout in microseconds.
 *
 * @param dev Pointer to device structure.
 * @param prescaler The prescaler value.
 * @param counter The counter value.
 * @return The timeout calculated in microseconds.
 */
static u32_t wwdg_stm32_get_timeout(struct device *dev, u32_t prescaler,
				    u32_t counter)
{
	u32_t divider = WWDG_INTERNAL_DIVIDER * (1 << (prescaler >> 7));
	float f_wwdg = wwdg_stm32_get_pclk(dev) / divider;

	return USEC_PER_SEC * (((counter & 0x3F) + 1) / f_wwdg);
}

/**
 * @brief Calculates prescaler & counter values.
 *
 * @param dev Pointer to device structure.
 * @param timeout Timeout value in microseconds.
 * @param prescaler Pointer to prescaler value.
 * @param counter Pointer to counter value.
 */
static void wwdg_stm32_convert_timeout(struct device *dev, u32_t timeout,
				       u32_t *prescaler,
				       u32_t *counter)
{
	u32_t clock_freq = wwdg_stm32_get_pclk(dev);
	u8_t divider = 0U;
	u8_t shift = 3U;

	/* Convert timeout to seconds. */
	float timeout_s = (float)timeout / USEC_PER_SEC;
	float wwdg_freq;

	*prescaler = 0;
	*counter = 0;

	for (divider = 8; divider >= 1; divider >>= 1) {
		wwdg_freq = ((float)clock_freq) / WWDG_INTERNAL_DIVIDER / divider;
		/* +1 to ceil the result, which may lose from truncation */
		*counter = (u32_t)(timeout_s * wwdg_freq + 1) - 1;
		*counter |= WWDG_RESET_LIMIT;
		*prescaler = shift << 7;

		if (*counter <= WWDG_COUNTER_MAX) {
			break;
		}

		shift--;
	}
}

static int wwdg_stm32_setup(struct device *dev, u8_t options)
{
	WWDG_TypeDef *wwdg = WWDG_STM32_STRUCT(dev);

	/* Deactivate running when debugger is attached. */
	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
#if defined(CONFIG_SOC_SERIES_STM32F0X)
		LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_DBGMCU);
#elif defined(CONFIG_SOC_SERIES_STM32L0X)
		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_DBGMCU);
#endif
		LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_WWDG_STOP);
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		return -ENOTSUP;
	}

	/* Enable the WWDG */
	LL_WWDG_Enable(wwdg);

	return 0;
}

static int wwdg_stm32_disable(struct device *dev)
{
	/* watchdog cannot be stopped once started unless SOC gets a reset */
	ARG_UNUSED(dev);

	return -EPERM;
}

static int wwdg_stm32_install_timeout(struct device *dev,
				      const struct wdt_timeout_cfg *config)
{
	struct wwdg_stm32_data *data = WWDG_STM32_DATA(dev);
	WWDG_TypeDef *wwdg = WWDG_STM32_STRUCT(dev);
	u32_t timeout = config->window.max * USEC_PER_MSEC;
	u32_t calculated_timeout;
	u32_t prescaler = 0U;
	u32_t counter = 0U;

	if (config->callback != NULL) {
		data->callback = config->callback;
	}

	wwdg_stm32_convert_timeout(dev, timeout, &prescaler, &counter);

	calculated_timeout = wwdg_stm32_get_timeout(dev, prescaler, counter);
	if (!(IS_WWDG_PRESCALER(prescaler) && IS_WWDG_COUNTER(counter) &&
	      IS_WWDG_TIMEOUT(timeout, calculated_timeout))) {
		/* One of the parameters provided is invalid */
		return -EINVAL;
	}

	data->counter = counter;

	/* Configure WWDG */
	/* Set the programmable prescaler */
	LL_WWDG_SetPrescaler(wwdg, prescaler);

	/* Set window the same as the counter to be able to feed the WWDG almost
	 * immediately
	 */
	LL_WWDG_SetWindow(wwdg, counter);
	LL_WWDG_SetCounter(wwdg, counter);

	return 0;
}

static int wwdg_stm32_feed(struct device *dev, int channel_id)
{
	WWDG_TypeDef *wwdg = WWDG_STM32_STRUCT(dev);
	struct wwdg_stm32_data *data = WWDG_STM32_DATA(dev);

	ARG_UNUSED(channel_id);
	LL_WWDG_SetCounter(wwdg, data->counter);

	return 0;
}

void wwdg_stm32_isr(void *arg)
{
	struct device *const dev = (struct device *)arg;
	struct wwdg_stm32_data *data = WWDG_STM32_DATA(dev);
	WWDG_TypeDef *wwdg = WWDG_STM32_STRUCT(dev);

	if (LL_WWDG_IsEnabledIT_EWKUP(wwdg)) {
		if (LL_WWDG_IsActiveFlag_EWKUP(wwdg)) {
			LL_WWDG_ClearFlag_EWKUP(wwdg);
			data->callback(dev, 0);
		}
	}
}

static const struct wdt_driver_api wwdg_stm32_api = {
	.setup = wwdg_stm32_setup,
	.disable = wwdg_stm32_disable,
	.install_timeout = wwdg_stm32_install_timeout,
	.feed = wwdg_stm32_feed,
};

static int wwdg_stm32_init(struct device *dev)
{
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	const struct wwdg_stm32_config *cfg = WWDG_STM32_CFG(dev);

	__ASSERT_NO_MSG(clk);

	clock_control_on(clk, (clock_control_subsys_t *) &cfg->pclken);

	wwdg_stm32_irq_config(dev);

	return 0;
}

static struct wwdg_stm32_data wwdg_stm32_dev_data = {
	.counter = WWDG_RESET_LIMIT,
	.callback = NULL
};

static struct wwdg_stm32_config wwdg_stm32_dev_config = {
	.pclken = {
		.enr = DT_WWDT_0_CLOCK_BITS,
		.bus = DT_WWDT_0_CLOCK_BUS
	},
	.Instance = (WWDG_TypeDef *)DT_WWDT_0_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(wwdg_stm32, DT_WWDT_0_NAME,
		    wwdg_stm32_init, &wwdg_stm32_dev_data, &wwdg_stm32_dev_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wwdg_stm32_api);

static void wwdg_stm32_irq_config(struct device *dev)
{
	WWDG_TypeDef *wwdg = WWDG_STM32_STRUCT(dev);

	IRQ_CONNECT(DT_WWDT_0_IRQ, DT_WWDT_0_IRQ_PRI,
		    wwdg_stm32_isr, DEVICE_GET(wwdg_stm32), 0);
	irq_enable(DT_WWDT_0_IRQ);
	LL_WWDG_EnableIT_EWKUP(wwdg);
}
