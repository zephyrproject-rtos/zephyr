/*
 * Copyright(c) 2026, Realtek Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_core_wdt

#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>

#ifdef CONFIG_SOC_SERIES_RTL87X2G
#include <rtl_wdt.h>
#elif CONFIG_SOC_SERIES_RTL8752H
#include <rtl876x_wdg.h>
#include <vector_table.h>
#include <rtl876x_nvic.h>
#endif

LOG_MODULE_REGISTER(wdt_core_bee, CONFIG_WDT_LOG_LEVEL);

#define CORE_WDT_INITIAL_TIMEOUT DT_INST_PROP(0, initial_timeout_ms)

struct core_wdt_bee_data {
	/*
	 * Note: Core WDT callback is only triggered in INTERRUPT_CPU mode
	 * (corresponding to Zephyr's WDT_FLAG_RESET_NONE flag).
	 * In other reset modes, the system will reset directly and no ISR generated.
	 */
	wdt_callback_t callback;
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
	ARG_UNUSED(dev);

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) || (options & WDT_OPT_PAUSE_HALTED_BY_DBG)) {
		return -ENOTSUP;
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

#ifdef CONFIG_SOC_SERIES_RTL8752H
	T_WDG_MODE wdt_mode;
#elif CONFIG_SOC_SERIES_RTL87X2G
	WDTMode_TypeDef wdt_mode;
#endif

	if (config->window.min != 0U || config->window.max == 0U) {
		return -EINVAL;
	}

	switch (config->flags & WDT_FLAG_RESET_MASK) {
	case WDT_FLAG_RESET_NONE:
		/*
		 * INTERRUPT_CPU mode: Generates an interrupt upon timeout.
		 * The registered callback will be executed in the ISR.
		 */
		wdt_mode = INTERRUPT_CPU;
		data->callback = config->callback;
		break;
	case WDT_FLAG_RESET_CPU_CORE:
		wdt_mode = RESET_ALL_EXCEPT_AON;
		break;
	case WDT_FLAG_RESET_SOC:
		wdt_mode = RESET_ALL;
		break;
	default:
		LOG_ERR("Unsupported watchdog config flag: %d", config->flags);
		return -EINVAL;
	}

	bool ret = WDT_Start(config->window.max, wdt_mode);

	if (ret != true) {
		LOG_ERR("Realtek Bee Core WDT Start failed");
		return -EINVAL;
	}

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

#ifdef CONFIG_SOC_SERIES_RTL8752H

#define BEE_CORE_WDT_INIT(n)                                                                       \
	static struct core_wdt_bee_data core_wdt_bee_data_##n;                                     \
	static const struct core_wdt_bee_config core_wdt_bee_config_##n = {};                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, core_wdt_bee_init, NULL, &core_wdt_bee_data_##n,                  \
			      &core_wdt_bee_config_##n, POST_KERNEL,                               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &core_wdt_bee_api);

#else /* CONFIG_SOC_SERIES_RTL8752H */

#define BEE_CORE_WDT_INIT(n)                                                                       \
	static struct core_wdt_bee_data core_wdt_bee_data_##n;                                     \
                                                                                                   \
	static void core_wdt_bee_cfg_func_##n(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), core_wdt_bee_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct core_wdt_bee_config core_wdt_bee_config_##n = {                        \
		.irq_num = DT_INST_IRQN(n),                                                        \
		.cfg_func = core_wdt_bee_cfg_func_##n,                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, core_wdt_bee_init, NULL, &core_wdt_bee_data_##n,                  \
			      &core_wdt_bee_config_##n, POST_KERNEL,                               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &core_wdt_bee_api);

#endif /* CONFIG_SOC_SERIES_RTL8752H */

DT_INST_FOREACH_STATUS_OKAY(BEE_CORE_WDT_INIT)
