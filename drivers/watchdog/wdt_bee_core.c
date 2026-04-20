/*
 * Copyright(c) 2026, Realtek Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_core_wdt

#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <rtl_wdt.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <rtl876x_wdg.h>
#include <vector_table.h>
#include <rtl876x_nvic.h>
#else
#error "Unsupported Realtek SoC series"
#endif

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "add exactly one realtek bee core wdt node to the devicetree");

LOG_MODULE_REGISTER(wdt_core_bee, CONFIG_WDT_LOG_LEVEL);

struct core_wdt_bee_data {
	/*
	 * Note: Core WDT callback is only triggered in INTERRUPT_CPU mode
	 * (corresponding to Zephyr's WDT_FLAG_RESET_NONE flag).
	 * In other reset modes, the system will reset directly and no ISR generated.
	 */
	wdt_callback_t callback;
	uint32_t timeout;
#ifdef CONFIG_SOC_SERIES_RTL8752H
	T_WDG_MODE wdt_mode;
#elif CONFIG_SOC_SERIES_RTL87X2G
	WDTMode_TypeDef wdt_mode;
#endif
};

struct core_wdt_bee_config {
	uint32_t irq_num;
	void (*cfg_func)(void);
};

#ifdef CONFIG_SOC_SERIES_RTL8752H
static void core_wdt_bee_isr(void)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct core_wdt_bee_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, 0);
	}
}
#else
static void core_wdt_bee_isr(const struct device *dev)
{
	struct core_wdt_bee_data *data = dev->data;

#ifdef CONFIG_SOC_SERIES_RTL87X2G
	WDT_Disable();
#endif

	if (data->callback) {
		data->callback(dev, 0);
	}
}
#endif

static int core_wdt_bee_setup(const struct device *dev, uint8_t options)
{
	struct core_wdt_bee_data *data = dev->data;

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) || (options & WDT_OPT_PAUSE_HALTED_BY_DBG)) {
		return -ENOTSUP;
	}

	bool ret = WDT_Start(data->timeout, data->wdt_mode);

	if (ret != true) {
		LOG_ERR("Realtek Bee Core WDT Start failed");
		return -EINVAL;
	}

	return 0;
}

static int core_wdt_bee_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_SOC_SERIES_RTL87X2G
	WDT_Disable();
#elif CONFIG_SOC_SERIES_RTL8752H
	WDG_Disable();
#endif

	return 0;
}

static int core_wdt_bee_install_timeout(const struct device *dev,
					const struct wdt_timeout_cfg *config)
{
	struct core_wdt_bee_data *data = dev->data;

	if (config->window.min != 0U || config->window.max == 0U) {
		return -EINVAL;
	}

	switch (config->flags & WDT_FLAG_RESET_MASK) {
	case WDT_FLAG_RESET_NONE:
		/*
		 * INTERRUPT_CPU mode: Generates an interrupt upon timeout.
		 * The registered callback will be executed in the ISR.
		 */
		data->wdt_mode = INTERRUPT_CPU;
		data->callback = config->callback;
		break;
	case WDT_FLAG_RESET_CPU_CORE:
		data->wdt_mode = RESET_ALL_EXCEPT_AON;
		data->callback = NULL;
		break;
	case WDT_FLAG_RESET_SOC:
		data->wdt_mode = RESET_ALL;
		data->callback = NULL;
		break;
	default:
		LOG_ERR("Unsupported watchdog config flag: %d", config->flags);
		return -EINVAL;
	}

	data->timeout = config->window.max;

	return 0;
}

static int core_wdt_bee_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

#ifdef CONFIG_SOC_SERIES_RTL87X2G
	WDT_Kick();
#elif CONFIG_SOC_SERIES_RTL8752H
	WDG_Restart();
#endif

	return 0;
}

static DEVICE_API(wdt, core_wdt_bee_api) = {
	.setup = core_wdt_bee_setup,
	.disable = core_wdt_bee_disable,
	.install_timeout = core_wdt_bee_install_timeout,
	.feed = core_wdt_bee_feed,
};

static int core_wdt_bee_init(const struct device *dev)
{
#ifndef CONFIG_SOC_SERIES_RTL8752H
	const struct core_wdt_bee_config *config = dev->config;
#endif

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		core_wdt_bee_disable(dev);
	}

#ifdef CONFIG_SOC_SERIES_RTL8752H
	WDG_ClockEnable();
	/* RTL8752H: Directly update vector table for secondary interrupt */
	RamVectorTableUpdate(WDT_VECTORn, (IRQ_Fun)core_wdt_bee_isr);

	NVIC_InitTypeDef nvic_init_struct;

	nvic_init_struct.NVIC_IRQChannel = WDT_IRQn;
	nvic_init_struct.NVIC_IRQChannelCmd = (FunctionalState)ENABLE;
	nvic_init_struct.NVIC_IRQChannelPriority = 0;
	NVIC_Init(&nvic_init_struct);
#else
	NVIC_ClearPendingIRQ(config->irq_num);
	config->cfg_func();
#endif

	return 0;
}

static struct core_wdt_bee_data core_wdt_bee_dev_data;

#ifdef CONFIG_SOC_SERIES_RTL87X2G

static void core_wdt_bee_cfg_func(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), core_wdt_bee_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

static const struct core_wdt_bee_config core_wdt_bee_dev_config = {
	.irq_num = DT_INST_IRQN(0),
	.cfg_func = core_wdt_bee_cfg_func,
};

#elif CONFIG_SOC_SERIES_RTL8752H

static const struct core_wdt_bee_config core_wdt_bee_dev_config;

#endif

DEVICE_DT_INST_DEFINE(0, core_wdt_bee_init, NULL, &core_wdt_bee_dev_data, &core_wdt_bee_dev_config,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &core_wdt_bee_api);
