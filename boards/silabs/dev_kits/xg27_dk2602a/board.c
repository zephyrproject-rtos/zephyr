/*
 * Copyright (c) 2021 Sateesh Kotapati
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_SOC_GECKO_DEV_INIT
#include "em_cmu.h"
#endif


LOG_MODULE_REGISTER(dev_kit, CONFIG_BOARD_XG27_DK2602A_LOG_LEVEL);

static int dev_kit_init_clocks(void);

void board_late_init_hook(void)
{
	int ret;

#ifdef CONFIG_SOC_GECKO_DEV_INIT
	dev_kit_init_clocks();
#endif
	static struct gpio_dt_spec wake_up_gpio_dev =
		GPIO_DT_SPEC_GET(DT_NODELABEL(wake_up_trigger), gpios);


	if (!gpio_is_ready_dt(&wake_up_gpio_dev)) {
		LOG_ERR("Wake-up GPIO device was not found!");
	}
	ret = gpio_pin_configure_dt(&wake_up_gpio_dev, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure wake-up GPIO device!");
	}
}

#ifdef CONFIG_SOC_GECKO_DEV_INIT
static int dev_kit_init_clocks(void)
{
	CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_HFRCODPLL);
#if defined(_CMU_EM01GRPACLKCTRL_MASK)
	CMU_ClockSelectSet(cmuClock_EM01GRPACLK, cmuSelect_HFRCODPLL);
#endif
#if defined(_CMU_EM01GRPBCLKCTRL_MASK)
	CMU_ClockSelectSet(cmuClock_EM01GRPBCLK, cmuSelect_HFRCODPLL);
#endif
	CMU_ClockSelectSet(cmuClock_EM23GRPACLK, cmuSelect_LFRCO);
#if defined(RTCC_PRESENT)
	CMU_ClockSelectSet(cmuClock_RTCC, cmuSelect_LFRCO);
#endif
	CMU_ClockSelectSet(cmuClock_WDOG0, cmuSelect_LFRCO);

	return 0;
}
#endif
