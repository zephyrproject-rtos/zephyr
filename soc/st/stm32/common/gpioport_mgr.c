/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_gpio

#include <cmsis_core.h>
#include <stm32_ll_gpio.h>

#include <stm32_hsem.h>
#include <stm32_gpio_shared.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/__assert.h>

/**
 * @brief Array containing pointers to each GPIO port.
 *
 * Entries will be NULL if the GPIO port is not enabled.
 */
static const struct device *const gpio_ports[] = {
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioa)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiob)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioc)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiod)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioe)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiof)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiog)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioh)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioi)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioj)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiok)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiol)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiom)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpion)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioo)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiop)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioq)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpior)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpios)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiot)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiou)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiov)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiow)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiox)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioy)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioz)),
};

const struct device *stm32_gpioport_get(uint32_t port_index)
{
	const struct device *dev;

	if (port_index >= ARRAY_SIZE(gpio_ports)) {
		return NULL;
	}

	dev = gpio_ports[port_index];

	return device_is_ready(dev) ? dev : NULL;
}

static int stm32_gpioport_pm_action(const struct device *dev,
				    enum pm_device_action action)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct gpio_stm32_config *cfg = dev->config;
	clock_control_subsys_t subsys = (void *)&cfg->pclken;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return clock_control_on(clk, subsys);
	case PM_DEVICE_ACTION_SUSPEND:
		return clock_control_off(clk, subsys);
	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_TURN_ON:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

__maybe_unused static int stm32_gpioport_init(const struct device *dev)
{
#if (defined(PWR_CR2_IOSV) || defined(PWR_SVMCR_IO2SV)) && \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpiog))
	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	/* Port G[15:2] requires external power supply */
	/* Cf: L4/L5/U3 RM, Chapter "Independent I/O supply rail" */
#ifdef CONFIG_SOC_SERIES_STM32U3X
	LL_PWR_EnableVDDIO2();
#else
	LL_PWR_EnableVddIO2();
#endif
	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);
#endif

	return pm_device_driver_init(dev, stm32_gpioport_pm_action);
}

#if defined(CONFIG_GPIO)
#define GPIO_PORT_INIT_PRIORITY CONFIG_GPIO_INIT_PRIORITY
#else /* CONFIG_GPIO */
/*
 * Use the default value of CONFIG_GPIO_INIT_PRIORITY
 * as init priority if the GPIO subsystem is not enabled.
 */
#define GPIO_PORT_INIT_PRIORITY CONFIG_KERNEL_INIT_PRIORITY_DEFAULT
#endif /* CONFIG_GPIO */

#define GPIO_PORT_DEVICE_INIT(__node, __suffix, __base_addr, __port)		\
	static const struct gpio_stm32_config gpio_stm32_cfg_## __suffix = {	\
		IF_ENABLED(IS_ENABLED(CONFIG_GPIO_STM32), (			\
		.common = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(16U),	\
		},))								\
		.base = (void *)__base_addr,					\
		.port = __port,							\
		IF_ENABLED(DT_NODE_HAS_PROP(__node, clocks),			\
			   (.pclken = STM32_CLOCK_INFO(0, __node),))		\
	};									\
										\
	static struct gpio_stm32_data gpio_stm32_data_## __suffix;		\
										\
	PM_DEVICE_DT_DEFINE(__node, stm32_gpioport_pm_action);			\
										\
	DEVICE_DT_DEFINE(__node,						\
			 COND_CODE_1(DT_NODE_HAS_PROP(__node, clocks),		\
				     (stm32_gpioport_init),			\
				     (NULL)),					\
			 PM_DEVICE_DT_GET(__node),				\
			 &gpio_stm32_data_## __suffix,				\
			 &gpio_stm32_cfg_## __suffix,				\
			 PRE_KERNEL_1,						\
			 GPIO_PORT_INIT_PRIORITY,				\
			 COND_CODE_1(IS_ENABLED(CONFIG_GPIO_STM32),		\
				(&gpio_stm32_driver), (NULL)))

#define GPIO_PORT_DEVICE_INIT_STM32(__suffix, __SUFFIX)				\
	GPIO_PORT_DEVICE_INIT(DT_NODELABEL(gpio##__suffix),			\
			 __suffix,						\
			 DT_REG_ADDR(DT_NODELABEL(gpio##__suffix)),		\
			 STM32_PORT##__SUFFIX)

#define GPIO_PORT_DEVICE_INIT_STM32_IF_OKAY(__suffix, __SUFFIX)			\
	IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio##__suffix)),	\
		   (GPIO_PORT_DEVICE_INIT_STM32(__suffix, __SUFFIX)))

#define DEVICE_INIT_IF_OKAY(idx, __suffix)				\
	GPIO_PORT_DEVICE_INIT_STM32_IF_OKAY(__suffix,			\
		GET_ARG_N(UTIL_INC(idx), STM32_GPIO_PORTS_LIST_UPR))

FOR_EACH_IDX(DEVICE_INIT_IF_OKAY, (;), STM32_GPIO_PORTS_LIST_LWR);
