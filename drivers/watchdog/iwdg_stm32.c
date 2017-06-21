/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <watchdog.h>
#include <soc.h>
#include <errno.h>
#include <assert.h>

#include "iwdg_stm32.h"

/* Minimal timeout in microseconds. */
#define IWDG_TIMEOUT_MIN	100
/* Maximal timeout in microseconds. */
#define IWDG_TIMEOUT_MAX	26214400

#define IS_IWDG_TIMEOUT(__TIMEOUT__)		\
	(((__TIMEOUT__) >= IWDG_TIMEOUT_MIN) &&	\
	 ((__TIMEOUT__) <= IWDG_TIMEOUT_MAX))

/*
 * Status register need 5 RC LSI divided by prescaler clock to be updated.
 * With higher prescaler (256U), and according to HSI variation,
 * we need to wait at least 6 cycles so 48 ms.
 */

#define IWDG_DEFAULT_TIMEOUT	48u

/**
 * @brief Calculates prescaler & reload values.
 *
 * @param timeout Timeout value in microseconds.
 * @param prescaler Pointer to prescaler value.
 * @param reload Pointer to reload value.
 */
static void iwdg_stm32_convert_timeout(u32_t timeout,
				       u32_t *prescaler,
				       u32_t *reload)
{
	assert(IS_IWDG_TIMEOUT(timeout));

	u16_t divider = 0;
	u8_t shift = 0;

	/* Convert timeout to seconds. */
	float m_timeout = (float)timeout / 1000000 * LSI_VALUE;

	do {
		divider = 4 << shift;
		shift++;
	} while ((m_timeout / divider) > 0xFFF);

	/*
	 * Value of the 'shift' variable corresponds to the
	 * defines of LL_IWDG_PRESCALER_XX type.
	 */
	*prescaler = --shift;
	*reload = (uint32_t)(m_timeout / divider) - 1;
}

static void iwdg_stm32_enable(struct device *dev)
{
	IWDG_TypeDef *iwdg = IWDG_STM32_STRUCT(dev);

	LL_IWDG_Enable(iwdg);
}

static void iwdg_stm32_disable(struct device *dev)
{
	/* watchdog cannot be stopped once started */
	ARG_UNUSED(dev);
}

static int iwdg_stm32_set_config(struct device *dev,
				 struct wdt_config *config)
{
	IWDG_TypeDef *iwdg = IWDG_STM32_STRUCT(dev);
	u32_t timeout = config->timeout;
	u32_t prescaler = 0;
	u32_t reload = 0;
	u32_t tickstart;

	assert(IS_IWDG_TIMEOUT(timeout));

	iwdg_stm32_convert_timeout(timeout, &prescaler, &reload);

	assert(IS_IWDG_PRESCALER(prescaler));
	assert(IS_IWDG_RELOAD(reload));

	LL_IWDG_EnableWriteAccess(iwdg);

	LL_IWDG_SetPrescaler(iwdg, prescaler);
	LL_IWDG_SetReloadCounter(iwdg, reload);

#if defined(CONFIG_SOC_SERIES_STM32F3X) || defined(CONFIG_SOC_SERIES_STM32L4X)
	/* Neither STM32F1X nor STM32F4 series supports window option. */
	LL_IWDG_SetWindow(iwdg, 0x0FFF);
#endif

	tickstart = k_uptime_get_32();

	while (LL_IWDG_IsReady(iwdg) == 0) {
		if ((k_uptime_get_32() - tickstart) > IWDG_DEFAULT_TIMEOUT) {
			return -ENODEV;
		}
	}

	LL_IWDG_ReloadCounter(iwdg);

	return 0;
}

static void iwdg_stm32_get_config(struct device *dev,
				  struct wdt_config *config)
{
	IWDG_TypeDef *iwdg = IWDG_STM32_STRUCT(dev);

	u32_t prescaler = LL_IWDG_GetPrescaler(iwdg);
	u32_t reload = LL_IWDG_GetReloadCounter(iwdg);

	/* Timeout given in microseconds. */
	config->timeout = (u32_t)((4 << prescaler) * (reload + 1)
				  * (1000000 / LSI_VALUE));
}

static void iwdg_stm32_reload(struct device *dev)
{
	IWDG_TypeDef *iwdg = IWDG_STM32_STRUCT(dev);

	LL_IWDG_ReloadCounter(iwdg);
}

static const struct wdt_driver_api iwdg_stm32_api = {
	.enable = iwdg_stm32_enable,
	.disable = iwdg_stm32_disable,
	.get_config = iwdg_stm32_get_config,
	.set_config = iwdg_stm32_set_config,
	.reload = iwdg_stm32_reload,
};

static int iwdg_stm32_init(struct device *dev)
{
	IWDG_TypeDef *iwdg = IWDG_STM32_STRUCT(dev);
	struct wdt_config config;

	config.timeout = CONFIG_IWDG_STM32_TIMEOUT;

	LL_IWDG_Enable(iwdg);

	iwdg_stm32_set_config(dev, &config);

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
	.Instance = IWDG
};

DEVICE_AND_API_INIT(iwdg_stm32, CONFIG_IWDG_STM32_DEVICE_NAME,
		    iwdg_stm32_init, &iwdg_stm32_dev_data, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &iwdg_stm32_api);

