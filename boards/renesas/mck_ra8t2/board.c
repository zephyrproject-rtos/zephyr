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
static int phy_init(void)
{
	const struct gpio_dt_spec phy_reset_gpios =
		GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), phy_reset_gpios);
	const uint16_t phy_reset_assert = DT_PROP_OR(DT_PATH(zephyr_user), phy_reset_assert_ms, 0);
	const uint16_t phy_reset_deassert =
		DT_PROP_OR(DT_PATH(zephyr_user), phy_reset_deassert_ms, 0);

	if (!gpio_is_ready_dt(&phy_reset_gpios)) {
		LOG_DBG("PHY reset pin is not set");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&phy_reset_gpios, GPIO_OUTPUT)) {
		LOG_DBG("Failed to hardware reset PHY");
		return -EIO;
	}

	/* Issue a hardware reset */
	if (phy_reset_assert > 0 && phy_reset_deassert > 0) {
		gpio_pin_set_dt(&phy_reset_gpios, 1);
		k_msleep(phy_reset_assert);

		gpio_pin_set_dt(&phy_reset_gpios, 0);
		k_msleep(phy_reset_deassert);
	}

	return 0;
}

SYS_INIT(phy_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
