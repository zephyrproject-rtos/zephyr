/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(board_control, CONFIG_LOG_DEFAULT_LEVEL);

#if defined(CONFIG_ETH_PHY_DRIVER)

#define PHY_RESET_ASSERT_MS   200
#define PHY_RESET_DEASSERT_MS 35

#define PHY_RESET_GPIO_PORT   DEVICE_DT_GET(DT_NODELABEL(ioport7))
#define PHY_RESET_GPIO_PIN    11
#define PHY_RESET_GPIO_FLAGS  GPIO_ACTIVE_LOW

static int phy_init(void)
{
	int ret;

	if (!device_is_ready(PHY_RESET_GPIO_PORT)) {
		LOG_DBG("PHY reset GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure(PHY_RESET_GPIO_PORT,
				 PHY_RESET_GPIO_PIN,
				 GPIO_OUTPUT | PHY_RESET_GPIO_FLAGS);
	if (ret) {
		LOG_DBG("Failed to configure PHY reset GPIO (%d)", ret);
		return ret;
	}

	/* Assert reset */
	gpio_pin_set(PHY_RESET_GPIO_PORT, PHY_RESET_GPIO_PIN, 1);
	k_msleep(PHY_RESET_ASSERT_MS);

	/* Deassert reset */
	gpio_pin_set(PHY_RESET_GPIO_PORT, PHY_RESET_GPIO_PIN, 0);
	k_msleep(PHY_RESET_DEASSERT_MS);

	return 0;
}

SYS_INIT(phy_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
