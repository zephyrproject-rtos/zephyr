/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raydium_rm692c9

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(rm692c9, CONFIG_DISPLAY_LOG_LEVEL);

#define RM692C9_TEON         0x35
#define RM692C9_SET_DSI_MODE 0xC2
#define RM692C9_WRMAUCCTR    0xFE
/*
 * These commands are taken from NXP's MCUXpresso SDK.
 * Additional documentation is added where possible, but the
 * Manufacture command set pages are not described in the datasheet
 */
static const struct {
	uint8_t cmd;
	uint8_t param;
} rm692c9_init_setting[] = {
	{.cmd = RM692C9_WRMAUCCTR, .param = 0x00},
	{.cmd = RM692C9_SET_DSI_MODE, .param = 0x08},
	{.cmd = RM692C9_TEON, .param = 0x00},
};

struct rm692c9_config {
	const struct device *mipi_dsi;
	uint8_t channel;
	uint8_t num_of_lanes;
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec bl_gpio;
};

static int rm692c9_init(const struct device *dev)
{
	const struct rm692c9_config *config = dev->config;
	struct mipi_dsi_device mdev = {0};
	int ret;
	uint32_t i;
	uint8_t cmd, param;

	/* Attach to MIPI DSI host */
	mdev.data_lanes = config->num_of_lanes;

	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}

	if (config->reset_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}

		/*
		 * Power to the display has been enabled via the regulator fixed api during
		 * regulator init. Per datasheet, we must wait at least 10ms before
		 * starting reset sequence after power on.
		 */
		k_sleep(K_MSEC(10));
		/* Start reset sequence */
		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret < 0) {
			LOG_ERR("Could not pull reset low (%d)", ret);
			return ret;
		}
		/* Per datasheet, reset low pulse width should be at least 10usec */
		k_sleep(K_USEC(30));
		ret = gpio_pin_set_dt(&config->reset_gpio, 1);
		if (ret < 0) {
			LOG_ERR("Could not pull reset high (%d)", ret);
			return ret;
		}
		/*
		 * It is necessary to wait at least 120msec after releasing reset,
		 * before sending additional commands. This delay can be 5msec
		 * if we are certain the display module is in SLEEP IN state,
		 * but this is not guaranteed (for example, with a warm reset)
		 */
		k_sleep(K_MSEC(150));
	}

	if (config->bl_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->bl_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure bl GPIO (%d)", ret);
			return ret;
		}
	}

	/* Write initialization settings for display  */
	for (i = 0; i < ARRAY_SIZE(rm692c9_init_setting); i++) {
		cmd = rm692c9_init_setting[i].cmd;
		param = rm692c9_init_setting[i].param;
		ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel, cmd, &param, 1);
		if (ret < 0) {
			return ret;
		}
	}

	k_sleep(K_MSEC(80));
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel, MIPI_DCS_SOFT_RESET, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* Delay 80 ms before enter DSI mode */
	k_sleep(K_MSEC(80));

	/* Set DSI mode */
	param = 0x0B;
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel, RM692C9_SET_DSI_MODE, &param,
				 1);
	if (ret < 0) {
		return ret;
	}

	/* Set Brightness. */
	param = 0xFF;
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel, MIPI_DCS_SET_DISPLAY_BRIGHTNESS,
				 &param, 1);
	if (ret < 0) {
		return ret;
	}

	/* Delay 50 ms before exiting sleep mode */
	k_sleep(K_MSEC(50));
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel, MIPI_DCS_EXIT_SLEEP_MODE, NULL,
				 0);
	if (ret < 0) {
		return ret;
	}

	/*
	 * We must wait 5 ms after exiting sleep mode before sending additional
	 * commands. If we intend to enter sleep mode, we must delay
	 * 120 ms before sending that command. To be safe, delay 150ms
	 */
	k_sleep(K_MSEC(150));

	/* Now, enable display */
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel, MIPI_DCS_SET_DISPLAY_ON, NULL,
				 0);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(100));

	ret = mipi_dsi_detach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}

	return ret;
}

static int rm692c9_blanking_off(const struct device *dev)
{
	const struct rm692c9_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 1);
	} else {
		return -ENOTSUP;
	}
}

static int rm692c9_blanking_on(const struct device *dev)
{
	const struct rm692c9_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 0);
	} else {
		return -ENOTSUP;
	}
}

static int rm692c9_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	ARG_UNUSED(dev);

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static const struct display_driver_api rm692c9_api = {
	.blanking_on = rm692c9_blanking_on,
	.blanking_off = rm692c9_blanking_off,
	.set_orientation = rm692c9_set_orientation,
};

#define RM692C9_PANEL(id)                                                                          \
	static const struct rm692c9_config rm692c9_config_##id = {                                 \
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(id)),                                        \
		.channel = DT_INST_REG_ADDR(id),                                                   \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(id, reset_gpios, {0}),                      \
		.bl_gpio = GPIO_DT_SPEC_INST_GET_OR(id, bl_gpios, {0}),                            \
		.num_of_lanes = DT_INST_PROP_BY_IDX(id, data_lanes, 0),                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, &rm692c9_init, NULL, NULL, &rm692c9_config_##id, POST_KERNEL,    \
			      CONFIG_DISPLAY_INIT_PRIORITY, &rm692c9_api);

DT_INST_FOREACH_STATUS_OKAY(RM692C9_PANEL)
