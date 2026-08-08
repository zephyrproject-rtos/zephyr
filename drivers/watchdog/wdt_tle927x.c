/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Deere & Company
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tle927x_wdt

#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mfd/tle927x.h>

#define TLE927X_WD_CLOSED_WINDOW_PCT 72U

#define WDT_TLE927X_PRIMARY_CHANNEL 0

#define WDT_TLE927X_TIMEOUT_INSTALLED BIT(0)
#define WDT_TLE927X_SETUP_DONE        BIT(1)

struct wdt_tle927x_config {
	const struct device *parent;
	uint8_t wd_ctrl_flags;
};

struct wdt_tle927x_data {
	uint8_t wd_ctrl;
	uint8_t flags;
};

static int wdt_tle927x_trigger(const struct device *dev)
{
	const struct wdt_tle927x_config *config = dev->config;
	struct wdt_tle927x_data *data = dev->data;
	uint8_t wd_ctrl = data->wd_ctrl & ~TLE927X_WD_CTRL_CHECKSUM;

	if ((sys_count_bits(&wd_ctrl, sizeof(wd_ctrl)) & 1U) != 0U) {
		wd_ctrl |= TLE927X_WD_CTRL_CHECKSUM;
	}

	return mfd_tle927x_write_reg(config->parent, TLE927X_REG_WD_CTRL, wd_ctrl);
}

static int wdt_tle927x_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const uint16_t period_ms[] = {10U, 20U, 50U, 100U, 200U, 500U, 1000U};
	const struct wdt_tle927x_config *config = dev->config;
	struct wdt_tle927x_data *data = dev->data;
	uint8_t wd_ctrl, wd_timer;

	if ((data->flags & WDT_TLE927X_SETUP_DONE) != 0U) {
		return -EBUSY;
	}

	if ((data->flags & WDT_TLE927X_TIMEOUT_INSTALLED) != 0U) {
		return -ENOMEM;
	}

	if ((cfg == NULL) || (cfg->window.min >= cfg->window.max)) {
		return -EINVAL;
	}

	if ((cfg->callback != NULL) || ((cfg->flags & WDT_FLAG_RESET_MASK) != WDT_FLAG_RESET_SOC)) {
		return -ENOTSUP;
	}

	for (wd_timer = 0U; wd_timer < ARRAY_SIZE(period_ms); wd_timer++) {
		if (cfg->window.max == period_ms[wd_timer]) {
			break;
		}
	}

	if (wd_timer == ARRAY_SIZE(period_ms)) {
		return -EINVAL;
	}

	if ((cfg->window.min != 0U) &&
	    ((cfg->window.min * 100U) < (cfg->window.max * TLE927X_WD_CLOSED_WINDOW_PCT))) {
		return -EINVAL;
	}

	wd_ctrl = FIELD_PREP(TLE927X_WD_CTRL_WD_TIMER, wd_timer) | config->wd_ctrl_flags;

	if (cfg->window.min != 0U) {
		wd_ctrl |= TLE927X_WD_CTRL_WD_WIN;
	}

	data->wd_ctrl = wd_ctrl;
	data->flags |= WDT_TLE927X_TIMEOUT_INSTALLED;

	return WDT_TLE927X_PRIMARY_CHANNEL;
}

static int wdt_tle927x_setup(const struct device *dev, uint8_t options)
{
	struct wdt_tle927x_data *data = dev->data;
	int ret;

	if (options != 0U) {
		return -ENOTSUP;
	}

	if ((data->flags & WDT_TLE927X_TIMEOUT_INSTALLED) == 0U) {
		return -EINVAL;
	}

	if ((data->flags & WDT_TLE927X_SETUP_DONE) != 0U) {
		return -EBUSY;
	}

	ret = wdt_tle927x_trigger(dev);
	if (ret != 0) {
		return ret;
	}

	data->flags |= WDT_TLE927X_SETUP_DONE;

	return 0;
}

static int wdt_tle927x_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -EPERM;
}

static int wdt_tle927x_feed(const struct device *dev, int channel_id)
{
	struct wdt_tle927x_data *data = dev->data;

	if ((channel_id != WDT_TLE927X_PRIMARY_CHANNEL) ||
	    ((data->flags & WDT_TLE927X_SETUP_DONE) == 0U)) {
		return -EINVAL;
	}

	return wdt_tle927x_trigger(dev);
}

static DEVICE_API(wdt, wdt_tle927x_api) = {
	.setup = wdt_tle927x_setup,
	.disable = wdt_tle927x_disable,
	.install_timeout = wdt_tle927x_install_timeout,
	.feed = wdt_tle927x_feed,
};

static int wdt_tle927x_init(const struct device *dev)
{
	const struct wdt_tle927x_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		return -ENODEV;
	}

	return 0;
}

#define WDT_TLE927X_DEFINE(inst)                                                                   \
	static struct wdt_tle927x_data wdt_tle927x_data_##inst;                                    \
                                                                                                   \
	static const struct wdt_tle927x_config wdt_tle927x_config_##inst = {                       \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
		.wd_ctrl_flags =                                                                   \
			(DT_INST_PROP(inst, max_3_resets) ? TLE927X_WD_CTRL_MAX_3_RST : 0U) |      \
			TLE927X_WD_CTRL_WD_EN_WK_BUS,                                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, wdt_tle927x_init, NULL, &wdt_tle927x_data_##inst,              \
			      &wdt_tle927x_config_##inst, POST_KERNEL,                             \
			      CONFIG_WDT_TLE927X_INIT_PRIORITY, &wdt_tle927x_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_TLE927X_DEFINE)
