/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <devicetree/gpio.h>
#include <power/x86_non_dsx.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_SYS_PM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(power);

/* Power sequencing GPIOs */
static struct gpio_config power_seq_gpios[] = {
	{
		POWER_SEQ_GPIO(PCH_EC_SLP_SUS_L),
	},
	{
		POWER_SEQ_GPIO(PCH_EC_SLP_S0_L),
	},
	{
		POWER_SEQ_GPIO(VR_EC_EC_RSMRST_ODL),
	},
	{
		POWER_SEQ_GPIO(VR_EC_ALL_SYS_PWRGD),
	},
#if POWER_SEQ_GPIO_PRESENT(VR_EC_DSW_PWROK)
	{
		POWER_SEQ_GPIO(VR_EC_DSW_PWROK),
	},
#endif
	{
		POWER_SEQ_GPIO(EC_PCH_RSMRST_L),
	},
#if POWER_SEQ_GPIO_PRESENT(EC_PCH_DSW_PWROK)
	{
		POWER_SEQ_GPIO(EC_PCH_DSW_PWROK),
	},
#endif
#if POWER_SEQ_GPIO_PRESENT(EC_PCH_SYS_PWROK)
	{
		POWER_SEQ_GPIO(EC_PCH_SYS_PWROK),
	},
#endif
#if POWER_SEQ_GPIO_PRESENT(EC_VR_PPVAR_VCCIN)
	{
		POWER_SEQ_GPIO(EC_VR_PPVAR_VCCIN),
	},
#endif
	{
		POWER_SEQ_GPIO(EC_PCH_PWR_BTN_ODL),
	},
#if POWER_SEQ_GPIO_PRESENT(EC_VR_EN_PP5000_A)
	{
		POWER_SEQ_GPIO(EC_VR_EN_PP5000_A),
	},
#endif
	{
		POWER_SEQ_GPIO(EC_VR_EN_PP3300_A),
	},
};

static void powseq_gpio_init(void)
{
	struct gpio_config *gpio;
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(power_seq_gpios); i++) {
		gpio = &power_seq_gpios[i];

		LOG_DBG("Configuring GPIO: net_name=%s, port_name=%s, \
			pin=0x%x, flag=0x%x", gpio->net_name, gpio->port_name,
			gpio->pin, gpio->flags);

		/* Get GPIO binding */
		gpio->port = device_get_binding(gpio->port_name);
		if (!gpio->port) {
			ret = -EINVAL;
			break;
		}

		/* Configure the GPIO */
		ret = gpio_pin_configure(gpio->port, gpio->pin, gpio->flags);
		if (ret != 0)
			break;
	}

	if (ret == 0)
		LOG_INF("Configuring GPIO complete");
	else
		LOG_ERR("Configuring GPIO failed: net_name=%s, port_name=%s, \
			pin=0x%x, flag=0x%x", gpio->net_name, gpio->port_name,
			gpio->pin, gpio->flags);
}

void pwrseq_thread(void *p1, void *p2, void *p3)
{
	powseq_gpio_init();

	while (true) {
	}
}
