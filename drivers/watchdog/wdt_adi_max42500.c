/*
 * Copyright (c) 2025 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max42500_watchdog

#include <zephyr/sys/util_macro.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_adi_max42500, CONFIG_WDT_LOG_LEVEL);

#define MAX42500_REG_ID 0x00

#define MAX42500_REG_VMON          0x03
#define MAX42500_REG_VMON_VMPD_BIT 7

#define MAX42500_REG_RSTMAP          0x04
#define MAX42500_REG_RSTMAP_PARM_BIT 7

#define MAX42500_REG_STATOV 0x05
#define MAX42500_REG_STATUV 0x06

#define MAX42500_REG_VIN1  0x08
#define MAX42500_REG_OVUV1 0x11

#define MAX42500_REG_WDSTAT 0x26

#define MAX42500_REG_WDCDIV           0x27
#define MAX42500_REG_WDCDIV_SWW_BIT   6
#define MAX42500_REG_WDCDIV_WDIV_MASK BIT_MASK(6)

#define MAX42500_REG_WDCFG1 0x28

#define MAX42500_REG_WDCFG2          0x29
#define MAX42500_REG_WDCFG2_WDEN_BIT 3

#define MAX42500_REG_WDKEY  0x2A
#define MAX42500_REG_WDLOCK 0x2B

struct wdt_max42500_monitor_cfg {
	uint8_t reg;
	bool uvov_trigger_reset;

	/* IN6 and IN7 are differential and use finer UV/OV limits, but no nominal
	 * voltage
	 */
	union {
		struct {
			uint8_t nominal_voltage;
			uint8_t ovuv_limit;
		} single_ended;

		struct {
			uint8_t uv_limit;
			uint8_t ov_limit;
		} differential;
	};
};

enum wdt_max42500_mode {
	WDT_MAX42500_MODE_DEFAULT = 0,
	WDT_MAX42500_MODE_SWW,
	WDT_MAX42500_MODE_CHALLENGE_RESPONSE,
};

struct wdt_max42500_dev_config {
	struct i2c_dt_spec i2c;
	const struct wdt_max42500_monitor_cfg *monitor_cfgs;
	uint8_t monitor_cfgs_len;
	bool enable_vmpd;
};

struct wdt_max42500_dev_data {
	bool timeout_installed;
	uint8_t wdiv;
	enum wdt_max42500_mode mode;
};

static int wdt_max42500_is_enabled(const struct device *dev)
{
	const struct wdt_max42500_dev_config *cfg = dev->config;
	int r;
	uint8_t wdcfg2;

	r = i2c_reg_read_byte_dt(&cfg->i2c, MAX42500_REG_WDCFG2, &wdcfg2);
	if (r < 0) {
		LOG_ERR("Error reading WDCFG2");
		return r;
	}

	return wdcfg2 & BIT(MAX42500_REG_WDCFG2_WDEN_BIT);
}

static int wdt_max42500_set_enable(const struct device *dev, bool enabled)
{

	const struct wdt_max42500_dev_config *cfg = dev->config;
	int r;

	r = i2c_reg_update_byte_dt(&cfg->i2c, MAX42500_REG_WDCFG2,
				   BIT(MAX42500_REG_WDCFG2_WDEN_BIT),
				   enabled ? BIT(MAX42500_REG_WDCFG2_WDEN_BIT) : 0);
	if (r < 0) {
		LOG_ERR("Error updating WDCFG2 WDEN bit");
		return r;
	}

	return 0;
}

static int wdt_max42500_config_single_ended_monitor(const struct device *dev,
						    const struct wdt_max42500_monitor_cfg *m_cfg)
{
	const struct wdt_max42500_dev_config *cfg = dev->config;
	int r;

	r = i2c_reg_write_byte_dt(&cfg->i2c, MAX42500_REG_VIN1 + m_cfg->reg - 1,
				  m_cfg->single_ended.nominal_voltage);
	if (r < 0) {
		LOG_ERR("Failed to set monitor %d nominal voltage", m_cfg->reg);
		return r;
	}

	/* OV/UV is packed in the union, so just use the address of the ov_limit field */
	r = i2c_reg_write_byte_dt(&cfg->i2c, MAX42500_REG_OVUV1 + m_cfg->reg - 1,
				  m_cfg->single_ended.ovuv_limit);
	if (r < 0) {
		LOG_ERR("Failed to set monitor %d ovuv voltage", m_cfg->reg);
		return r;
	}

	return 0;
}

static int wdt_max42500_config_differential_monitor(const struct device *dev,
						    const struct wdt_max42500_monitor_cfg *m_cfg)
{
	return -ENOTSUP;
}

static int wdt_max42500_config_monitor(const struct device *dev,
				       const struct wdt_max42500_monitor_cfg *m_cfg)
{
	switch (m_cfg->reg) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		return wdt_max42500_config_single_ended_monitor(dev, m_cfg);
	case 6:
	case 7:
		return wdt_max42500_config_differential_monitor(dev, m_cfg);
	default:
		return -EINVAL;
	}
}

static int wdt_max42500_set_up_voltage_monitors(const struct device *dev)
{
	const struct wdt_max42500_dev_config *cfg = dev->config;
	int r;
	uint8_t rstmap = 0, vmon = 0;
	uint8_t uv_stat, ov_stat;

	if (!cfg->monitor_cfgs_len) {
		return 0;
	}

	/* RSTMAP resets to OTP values, so fetch the current value to avoid clearing PARM bit
	 * set in OTP
	 */
	r = i2c_reg_read_byte_dt(&cfg->i2c, MAX42500_REG_RSTMAP, &rstmap);
	if (r < 0) {
		LOG_ERR("Failed to read RSTMAP register");
		return r;
	}

	r = i2c_reg_read_byte_dt(&cfg->i2c, MAX42500_REG_VMON, &vmon);
	if (r < 0) {
		LOG_ERR("Failed to read VMON register");
		return r;
	}

	LOG_DBG("Reset map on start: %0x, VMON on start: %0x", rstmap, vmon);

	rstmap &= BIT(MAX42500_REG_RSTMAP_PARM_BIT);

	WRITE_BIT(vmon, MAX42500_REG_VMON_VMPD_BIT, cfg->enable_vmpd);

	for (uint8_t i = 0; i < cfg->monitor_cfgs_len; i++) {
		const struct wdt_max42500_monitor_cfg *m_cfg = &cfg->monitor_cfgs[i];

		WRITE_BIT(vmon, m_cfg->reg - 1, 0);
		WRITE_BIT(rstmap, m_cfg->reg - 1, 0);

		if (m_cfg->uvov_trigger_reset) {
			rstmap |= BIT(m_cfg->reg - 1);
		}
	}

	LOG_DBG("Setting VMON to %x", vmon);
	r = i2c_reg_write_byte_dt(&cfg->i2c, MAX42500_REG_VMON, vmon);
	if (r < 0) {
		LOG_ERR("Failed to write VMON");
		return r;
	}

	LOG_DBG("Setting reset map to %0x", rstmap);

	r = i2c_reg_write_byte_dt(&cfg->i2c, MAX42500_REG_RSTMAP, rstmap);
	if (r < 0) {
		LOG_ERR("Failed to write RSTMAP");
		return r;
	}

	for (uint8_t i = 0; i < cfg->monitor_cfgs_len; i++) {
		const struct wdt_max42500_monitor_cfg *m_cfg = &cfg->monitor_cfgs[i];

		LOG_DBG("Setting monitor %d", m_cfg->reg);
		r = wdt_max42500_config_monitor(dev, m_cfg);
		if (r < 0) {
			LOG_ERR("Failed to configure monitor %d (%d)", m_cfg->reg, r);
			return r;
		}

		WRITE_BIT(vmon, m_cfg->reg - 1, 1);
		WRITE_BIT(rstmap, m_cfg->reg - 1, m_cfg->uvov_trigger_reset);
	}

	LOG_DBG("Setting VMON to %x", vmon);
	r = i2c_reg_write_byte_dt(&cfg->i2c, MAX42500_REG_VMON, vmon);
	if (r < 0) {
		LOG_ERR("Failed to write VMON");
		return r;
	}

	k_usleep(50);

	r = i2c_reg_read_byte_dt(&cfg->i2c, MAX42500_REG_STATOV, &ov_stat);
	if (r < 0) {
		LOG_ERR("Failed to read OV status register");
		return r;
	}

	r = i2c_reg_read_byte_dt(&cfg->i2c, MAX42500_REG_STATUV, &uv_stat);
	if (r < 0) {
		LOG_ERR("Failed to read UV status register");
		return r;
	}

	LOG_DBG("UV: %x, OV: %x", uv_stat, ov_stat);

	LOG_DBG("Setting reset map to %0x", rstmap);

	r = i2c_reg_write_byte_dt(&cfg->i2c, MAX42500_REG_RSTMAP, rstmap);
	if (r < 0) {
		LOG_ERR("Failed to write RSTMAP");
		return r;
	}

	r = i2c_reg_read_byte_dt(&cfg->i2c, MAX42500_REG_STATOV, &ov_stat);
	if (r < 0) {
		LOG_ERR("Failed to read OV status register");
		return r;
	}

	r = i2c_reg_read_byte_dt(&cfg->i2c, MAX42500_REG_STATUV, &uv_stat);
	if (r < 0) {
		LOG_ERR("Failed to read UV status register");
		return r;
	}

	LOG_DBG("UV: %x, OV: %x", uv_stat, ov_stat);

	return r;
}

static int wdt_max42500_setup(const struct device *dev, uint8_t options)
{
	struct wdt_max42500_dev_data *data = dev->data;
	int r;

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		LOG_ERR("Pause in sleep not supported");
		return -ENOTSUP;
	}

	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		LOG_ERR("Pause when halted by debugger not supported");
		return -ENOTSUP;
	}

	r = wdt_max42500_set_up_voltage_monitors(dev);
	if (r < 0) {
		LOG_ERR("Failed to set up voltage monitors (%d)", r);
		return r;
	}

	if (data->timeout_installed) {
		r = wdt_max42500_set_enable(dev, true);
		if (r < 0) {
			LOG_ERR("Failed to enable the watchdog");
			return r;
		}
	}

	return 0;
}

static int wdt_max42500_disable(const struct device *dev)
{
	int r;

	r = wdt_max42500_is_enabled(dev);
	if (r < 0) {
		LOG_ERR("Error fetching if in enabled");
		return r;
	} else if (r == 0) {
		/* Already disabled, don't try to disable again */
		return 0;
	}

	return wdt_max42500_set_enable(dev, false);
}

static inline uint32_t wdt_max42500_t_wdclk_for_wdiv(uint32_t wdiv)
{
	return (wdiv + 1) * 25 * 8;
}

static uint32_t wdt_max42500_timeout_to_period(uint32_t t_wdclk, uint32_t timeout_ms)
{
	uint32_t timeout_us = timeout_ms * 1000;
	uint32_t divisor = t_wdclk * 8;
	uint32_t divided = DIV_ROUND_UP(timeout_us, divisor);

	if (divided > 0) {
		divided -= 1;
	}
	return divided;
}

static int wdt_max42500_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct wdt_max42500_dev_data *data = dev->data;
	const struct wdt_max42500_dev_config *dev_cfg = dev->config;
	int r;
	uint8_t wdcfg1;
	uint32_t open, close, t_wdclk;

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		LOG_ERR("Only SoC reset supported");
		return -ENOTSUP;
	}

	if (cfg->window.max == 0) {
		LOG_ERR("Upper limit timeout out of range");
		return -EINVAL;
	}

	if (cfg->callback) {
		LOG_ERR("Callback not currently supported");
		return -ENOTSUP;
	}

	t_wdclk = wdt_max42500_t_wdclk_for_wdiv(data->wdiv);

	open = wdt_max42500_timeout_to_period(t_wdclk, cfg->window.min);
	close = wdt_max42500_timeout_to_period(t_wdclk, cfg->window.max);

	if (open > BIT_MASK(4) || close > BIT_MASK(4) || close <= open) {
		LOG_ERR("Invalid window for given wdiv %d (%d - %d)", data->wdiv, open, close);
		return -EINVAL;
	}

	LOG_DBG("For window %d-%d setting open/closed to %d-%d", cfg->window.min, cfg->window.max,
		open, close);

	wdcfg1 = (close << 4) | open;
	r = i2c_reg_write_byte_dt(&dev_cfg->i2c, MAX42500_REG_WDCFG1, wdcfg1);
	if (r < 0) {
		LOG_ERR("Failed to set the watchdog open/close window");
		return r;
	}

	data->timeout_installed = true;

	return 0;
}

static inline uint8_t wdt_max42500_challenge_response(uint8_t key)
{
	uint8_t lfsr = key;
	uint8_t bit = ((lfsr >> 7) ^ (lfsr >> 5) ^ (lfsr >> 4) ^ (lfsr >> 3)) & 1;

	lfsr = (lfsr << 1) | bit;

	return lfsr;
}

static int wdt_max42500_get_challenge_response(const struct device *dev, uint8_t *dest_wdkey)
{
	const struct wdt_max42500_dev_config *cfg = dev->config;
	int r;
	uint8_t curr_wdkey;

	r = i2c_reg_read_byte_dt(&cfg->i2c, MAX42500_REG_WDKEY, &curr_wdkey);
	if (r < 0) {
		LOG_ERR("Failed to read current WDKEY");
		return r;
	}

	LOG_DBG("Existing key %0x", curr_wdkey);

	*dest_wdkey = wdt_max42500_challenge_response(curr_wdkey) + 1;

	return 0;
}

static int wdt_max42500_feed(const struct device *dev, int channel_id)
{
	const struct wdt_max42500_dev_config *cfg = dev->config;
	struct wdt_max42500_dev_data *data = dev->data;
	int r;
	uint8_t wdkey = 0, stat;

	if (!data->timeout_installed) {
		LOG_ERR("No valid timeout installed");
		return -EINVAL;
	}

	r = i2c_reg_read_byte_dt(&cfg->i2c, MAX42500_REG_WDSTAT, &stat);
	if (r < 0) {
		LOG_ERR("Failed to read the wd status (%d)", r);
		return r;
	}

	LOG_DBG("Status %0x", stat);

	if (data->mode == WDT_MAX42500_MODE_CHALLENGE_RESPONSE) {
		r = wdt_max42500_get_challenge_response(dev, &wdkey);
		if (r < 0) {
			LOG_ERR("Failed to get new challenge/response WDKEY value");
			return r;
		}

		LOG_DBG("Fetched the new key %0x!", wdkey);
	}

	r = i2c_reg_write_byte_dt(&cfg->i2c, MAX42500_REG_WDKEY, wdkey);
	if (r < 0) {
		LOG_ERR("Failed to write the new WDKEY value to feed the watchdog");
		return r;
	}

	LOG_DBG("Fed by writing to WDKEY");

	r = i2c_reg_read_byte_dt(&cfg->i2c, MAX42500_REG_WDSTAT, &stat);
	if (r < 0) {
		LOG_ERR("Failed to read the wd status (%d)", r);
		return r;
	}

	LOG_DBG("Status %0x", stat);

	return 0;
}

static DEVICE_API(wdt, wdt_max42500_api) = {
	.setup = wdt_max42500_setup,
	.disable = wdt_max42500_disable,
	.install_timeout = wdt_max42500_install_timeout,
	.feed = wdt_max42500_feed,
};

static int wdt_max42500_init(const struct device *dev)
{
	const struct wdt_max42500_dev_config *cfg = dev->config;
	struct wdt_max42500_dev_data *data = dev->data;
	int r;
	uint8_t id, cdiv;
	bool update_cdiv = false;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus device is not ready");
		return -ENODEV;
	}

	r = i2c_reg_read_byte_dt(&cfg->i2c, MAX42500_REG_ID, &id);
	if (r < 0) {
		LOG_ERR("Failed to fetch the device ID");
		return r;
	}

	LOG_INF("MAX42500 Device: %d rev %d", id & 0xF, id >> 4);

	r = i2c_reg_read_byte_dt(&cfg->i2c, MAX42500_REG_WDCDIV, &cdiv);
	if (r < 0) {
		LOG_ERR("Failed to fetch the device WDCDIV");
		return r;
	}

	if (data->wdiv == UINT8_MAX) {
		data->wdiv = cdiv & MAX42500_REG_WDCDIV_WDIV_MASK;
		LOG_INF("Fetched device WDIV: %d", data->wdiv);
	} else {
		cdiv = (cdiv & ~MAX42500_REG_WDCDIV_WDIV_MASK) | data->wdiv;
		update_cdiv = true;
		LOG_INF("New CDIV with updated WDIV: %d", cdiv);
	}

	if (data->mode == WDT_MAX42500_MODE_DEFAULT) {
		data->mode = (cdiv & BIT(MAX42500_REG_WDCDIV_SWW_BIT))
				     ? WDT_MAX42500_MODE_SWW
				     : WDT_MAX42500_MODE_CHALLENGE_RESPONSE;
		LOG_INF("Fetched device SWW mode: %d", data->mode);
	} else {
		WRITE_BIT(cdiv, MAX42500_REG_WDCDIV_SWW_BIT, data->mode == WDT_MAX42500_MODE_SWW);
		update_cdiv = true;
		LOG_INF("New CDIV with updated mode: %d", cdiv);
	}

	if (update_cdiv) {
		r = i2c_reg_write_byte_dt(&cfg->i2c, MAX42500_REG_WDCDIV, cdiv);
		if (r < 0) {
			LOG_ERR("Failed to update the device WDCDIV");
			return r;
		}
	}

	r = wdt_max42500_disable(dev);
	if (r < 0) {
		LOG_ERR("Failed to disable the watchdog on init (%d)", r);
	}

	return r;
}

#define MAX42500_MONITOR_OVUV(n) ((DT_PROP(n, ov_limit) << 4) | (DT_PROP(n, uv_limit) & 0xF))

#define MAX42500_MONITOR_CFG(n)                                                                    \
	{.reg = DT_PROP(n, reg),                                                                   \
	 .uvov_trigger_reset = DT_PROP(n, uvov_trigger_reset),                                     \
	 COND_CODE_1(UTIL_OR(IS_EQ(DT_PROP(n, reg), 6), IS_EQ(DT_PROP(n, reg), 7)), (              \
			.differential = {                                                          \
				.uv_limit = DT_PROP(n, uv_limit),                                  \
				.ov_limit = DT_PROP(n, ov_limit),                                  \
			},                                                                         \
		), ({                                                                              \
			.single_ended = {                                                          \
				.nominal_voltage = DT_PROP(n, nominal_voltage),                    \
				.ovuv_limit = MAX42500_MONITOR_OVUV(n),                            \
			},                                                                         \
		})) }

#define MAX42500_MONITOR_CFG_BUILD_ASSERT(n)                                                       \
	BUILD_ASSERT(DT_PROP(n, reg) > 0 && DT_PROP(n, reg) <= 7,                                  \
		     "Monitor `reg` must be in range of 1-7 inclusive");

/* clang-format off */

#define WDT_MAX42500_INST(n)                                                                       \
	DT_INST_FOREACH_CHILD(n, MAX42500_MONITOR_CFG_BUILD_ASSERT)                                \
	static const struct wdt_max42500_monitor_cfg monitor_cfgs_##n[] = {                        \
		DT_INST_FOREACH_CHILD_SEP(n, MAX42500_MONITOR_CFG, (,))};                          \
	static const struct wdt_max42500_dev_config wdt_max42500_cfg_##n = {                       \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.monitor_cfgs = monitor_cfgs_##n,                                                  \
		.monitor_cfgs_len = ARRAY_SIZE(monitor_cfgs_##n),                                  \
		.enable_vmpd = DT_INST_PROP(n, enable_vmpd),                                       \
	};                                                                                         \
	static struct wdt_max42500_dev_data wdt_max42500_data_##n = {                              \
		.wdiv = DT_INST_PROP_OR(n, wdiv, UINT8_MAX),                                       \
		.mode = DT_INST_ENUM_IDX_OR(n, mode, WDT_MAX42500_MODE_DEFAULT),                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, wdt_max42500_init, NULL, &wdt_max42500_data_##n,                  \
			      &wdt_max42500_cfg_##n, POST_KERNEL,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_max42500_api);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(WDT_MAX42500_INST)
