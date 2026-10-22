/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ethernet_phy_fixed_link

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/phy.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_mii_fixed_link, CONFIG_PHY_LOG_LEVEL);

#include "phy_mii.h"

#define ANY_RESET_GPIO   DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)

struct phy_mii_fixed_dev_config {
	enum phy_link_speed fixed_speed;
#if ANY_RESET_GPIO
	const struct gpio_dt_spec reset_gpio;
	uint32_t reset_assert_duration_us;
	uint32_t reset_deassertion_timeout_ms;
#endif /* ANY_RESET_GPIO */
};

static int phy_mii_fixed_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	const struct phy_mii_fixed_dev_config *const cfg = dev->config;

	state->speed = cfg->fixed_speed;
	state->is_up = true;

	return 0;
}

static int phy_mii_fixed_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	const struct phy_mii_fixed_dev_config *const cfg = dev->config;
	struct phy_link_state state;

	if (cb == NULL) {
		return 0;
	}

	state.speed = cfg->fixed_speed;
	state.is_up = true;

	cb(dev, &state, user_data);

	return 0;
}

#if ANY_RESET_GPIO
static int phy_mii_fixed_init(const struct device *dev)
{
	const struct phy_mii_fixed_dev_config *const cfg = dev->config;
	int ret;

	if (gpio_is_ready_dt(&cfg->reset_gpio)) {
		/* Issue a hard reset */
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure RST pin (%d)", ret);
			return ret;
		}

		/* assertion time */
		k_busy_wait(cfg->reset_assert_duration_us);

		ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
		if (ret < 0) {
			LOG_ERR("Failed to de-assert RST pin (%d)", ret);
			return ret;
		}

		k_msleep(cfg->reset_deassertion_timeout_ms);
	}

	return 0;
}
#endif /* ANY_RESET_GPIO */

static DEVICE_API(ethphy, phy_mii_fixed_driver_api) = {
	.get_link = phy_mii_fixed_get_link_state,
	.link_cb_set = phy_mii_fixed_link_cb_set,
};

#if ANY_RESET_GPIO
#define RESET_GPIO(n)                                                                              \
	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                               \
	.reset_assert_duration_us = DT_INST_PROP_OR(n, reset_assert_duration_us, 0),               \
	.reset_deassertion_timeout_ms = DT_INST_PROP_OR(n, reset_deassertion_timeout_ms, 0),
#else
#define RESET_GPIO(n)
#endif /* ANY_RESET_GPIO */

#define PHY_MII_FIXED_INIT(n)                                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, reset_gpios), (&phy_mii_fixed_init), (NULL))

#define PHY_MII_FIXED_DEVICE(n)                                                                    \
	BUILD_ASSERT((DT_INST_PROP_LEN(n, default_speeds) == 1) &&                                 \
		     (PHY_INST_GENERATE_DEFAULT_SPEEDS(n) != 0),                                   \
		     "Exactly one valid default speed must be configured");                        \
                                                                                                   \
	static const struct phy_mii_fixed_dev_config phy_mii_fixed_dev_config_##n = {              \
		.fixed_speed = PHY_INST_GENERATE_DEFAULT_SPEEDS(n),                                \
		RESET_GPIO(n)                                                                      \
	};                                                                                         \
												   \
DEVICE_DT_INST_DEFINE(n, PHY_MII_FIXED_INIT(n), NULL, NULL, &phy_mii_fixed_dev_config_##n,         \
		      POST_KERNEL, CONFIG_PHY_INIT_PRIORITY, &phy_mii_fixed_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PHY_MII_FIXED_DEVICE)
