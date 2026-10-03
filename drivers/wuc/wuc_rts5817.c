/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5817_wuc

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/dt-bindings/wuc/wuc_rts5817.h>
#include <zephyr/drivers/wuc/rts5817_wuc.h>

#include "dlink_sys_reg.h"
#include "dlink_ldo_reg.h"

LOG_MODULE_REGISTER(rts5817_wuc, CONFIG_WUC_LOG_LEVEL);

#define WKUP_MODE_OFFSET(id) (((id) - 1) * 2 + 8)
#define WKUP_MODE_MASK(id)   (3ul << WKUP_MODE_OFFSET(id))

/* map: RTS5817_WKUP_SRC_* → exit enable bit position (1-based) */
#define EXIT_EN_BIT(id) (id)

/*
 * Mapping: wakeup source ID → bit position in EXIT_FLAG:
 *   RTS5817_WKUP_SRC_GPIO_AL2    (id=1) → bit 0
 *   RTS5817_WKUP_SRC_GPIO_AL1    (id=2) → bit 1
 *   RTS5817_WKUP_SRC_GPIO_AL0    (id=3) → bit 2
 *   RTS5817_WKUP_SRC_SENSOR_GPIO (id=4) → bit 3
 *   RTS5817_WKUP_SRC_SENSOR_CS   (id=5) → bit 4
 *   RTS5817_WKUP_SRC_GPI_WAKE2   (id=6) → bit 5
 *   RTS5817_WKUP_SRC_GPI_WAKE1   (id=7) → bit 6
 */
#define EXIT_FLAG_BIT(id) ((id) - 1)

struct rts5817_wkup_src_cfg {
	uint8_t id;
	uint8_t wkup_mode;
	uint32_t wkup_time;
	uint8_t substate_id;
};

struct rts5817_wakeup_event_t {
	rts5817_wakeup_event_cb_t cb;
	void *data;
};

struct rts5817_wuc_config {
	const struct device *syscon_sys;
	const struct device *syscon_ldo;
	struct rts5817_wkup_src_cfg wkup_src_cfg_list[RTS5817_WKUP_SRC_MAX_NUM];
};

struct rts5817_wuc_data {
	struct rts5817_wakeup_event_t wkup_evt[RTS5817_WKUP_SRC_MAX_NUM];
};

static void rts5817_set_gpio_deglitch(const struct device *syscon_sys, int id, uint8_t wkup_mode,
				      bool enable)
{
	uint32_t deglitch_cfg = 0;
	uint32_t deglitch_msk = 0;

	switch (id) {
	case RTS5817_WKUP_SRC_GPIO_AL0:
		deglitch_cfg = RC_DEGLITCH_GPIO_AL0_EN;
		if (wkup_mode == RTS5817_WAKEUP_MODE_FALLING_EDGE ||
		    wkup_mode == RTS5817_WAKEUP_MODE_LOW_LEVEL) {
			deglitch_cfg |= RC_DEGLITCH_EDGE_GPIO_AL0;
		}
		deglitch_msk = RC_DEGLITCH_GPIO_AL0_EN | RC_DEGLITCH_EDGE_GPIO_AL0;
		break;
	case RTS5817_WKUP_SRC_GPIO_AL1:
		deglitch_cfg = RC_DEGLITCH_GPIO_AL1_EN;
		if (wkup_mode == RTS5817_WAKEUP_MODE_FALLING_EDGE ||
		    wkup_mode == RTS5817_WAKEUP_MODE_LOW_LEVEL) {
			deglitch_cfg |= RC_DEGLITCH_EDGE_GPIO_AL1;
		}
		deglitch_msk = RC_DEGLITCH_GPIO_AL1_EN | RC_DEGLITCH_EDGE_GPIO_AL1;
		break;
	case RTS5817_WKUP_SRC_GPIO_AL2:
		deglitch_cfg = RC_DEGLITCH_GPIO_AL2_EN;
		if (wkup_mode == RTS5817_WAKEUP_MODE_FALLING_EDGE ||
		    wkup_mode == RTS5817_WAKEUP_MODE_LOW_LEVEL) {
			deglitch_cfg |= RC_DEGLITCH_EDGE_GPIO_AL2;
		}
		deglitch_msk = RC_DEGLITCH_GPIO_AL2_EN | RC_DEGLITCH_EDGE_GPIO_AL2;
		break;
	case RTS5817_WKUP_SRC_SENSOR_GPIO:
		deglitch_cfg = RC_DEGLITCH_GPIO_SSOR_EN;
		if (wkup_mode == RTS5817_WAKEUP_MODE_FALLING_EDGE ||
		    wkup_mode == RTS5817_WAKEUP_MODE_LOW_LEVEL) {
			deglitch_cfg |= RC_DEGLITCH_EDGE_GPIO_SSOR;
		}
		deglitch_msk = RC_DEGLITCH_GPIO_SSOR_EN | RC_DEGLITCH_EDGE_GPIO_SSOR;
		break;
	case RTS5817_WKUP_SRC_SENSOR_CS:
		deglitch_cfg = RC_DEGLITCH_GPIO_SNRCS_EN;
		if (wkup_mode == RTS5817_WAKEUP_MODE_FALLING_EDGE ||
		    wkup_mode == RTS5817_WAKEUP_MODE_LOW_LEVEL) {
			deglitch_cfg |= RC_DEGLITCH_EDGE_GPIO_SNRCS;
		}
		deglitch_msk = RC_DEGLITCH_GPIO_SNRCS_EN | RC_DEGLITCH_EDGE_GPIO_SNRCS;
		break;
	case RTS5817_WKUP_SRC_GPI_WAKE1:
		deglitch_cfg = RC_DEGLITCH_GPIO_WAKE1_EN;
		if (wkup_mode == RTS5817_WAKEUP_MODE_FALLING_EDGE ||
		    wkup_mode == RTS5817_WAKEUP_MODE_LOW_LEVEL) {
			deglitch_cfg |= RC_DEGLITCH_EDGE_GPIO_WAKE1;
		}
		deglitch_msk = RC_DEGLITCH_GPIO_WAKE1_EN | RC_DEGLITCH_EDGE_GPIO_WAKE1;
		break;
	case RTS5817_WKUP_SRC_GPI_WAKE2:
		deglitch_cfg = RC_DEGLITCH_GPIO_WAKE2_EN;
		if (wkup_mode == RTS5817_WAKEUP_MODE_FALLING_EDGE ||
		    wkup_mode == RTS5817_WAKEUP_MODE_LOW_LEVEL) {
			deglitch_cfg |= RC_DEGLITCH_EDGE_GPIO_WAKE2;
		}
		deglitch_msk = RC_DEGLITCH_GPIO_WAKE2_EN | RC_DEGLITCH_EDGE_GPIO_WAKE2;
		break;
	default:
		return;
	}

	if (enable) {
		syscon_update_bits(syscon_sys, R_RC_DEGLITCH_CFG, deglitch_msk, deglitch_cfg);
	} else {
		syscon_update_bits(syscon_sys, R_RC_DEGLITCH_CFG, deglitch_msk, 0);
	}
}

static inline uint32_t substate_to_reg(uint8_t dt_substate_id)
{
	return dt_substate_id == RTS5817_S2RAM_SUB_ID_SUSPEND ? R_SUSPEND_IN_OUT_CTRL
							      : R_SLEEP_IN_OUT_CTRL;
}

static int rts5817_wuc_enable_wakeup_source(const struct device *dev, uint32_t id)
{
	const struct rts5817_wuc_config *config = dev->config;
	const struct device *syscon_sys = config->syscon_sys;
	const struct rts5817_wkup_src_cfg *cfg;
	uint32_t reg_off;
	uint32_t mask;
	uint32_t val;

	cfg = &config->wkup_src_cfg_list[id];

	switch (id) {
	case RTS5817_WKUP_SRC_GPIO_AL2:
	case RTS5817_WKUP_SRC_GPIO_AL1:
	case RTS5817_WKUP_SRC_GPIO_AL0:
	case RTS5817_WKUP_SRC_SENSOR_GPIO:
	case RTS5817_WKUP_SRC_SENSOR_CS:
	case RTS5817_WKUP_SRC_GPI_WAKE2:
	case RTS5817_WKUP_SRC_GPI_WAKE1:
		reg_off = substate_to_reg(cfg->substate_id);
		mask = WKUP_MODE_MASK(id) | BIT(EXIT_EN_BIT(id));
		val = (cfg->wkup_mode << WKUP_MODE_OFFSET(id)) | BIT(EXIT_EN_BIT(id));

		syscon_update_bits(syscon_sys, reg_off, mask, val);
		rts5817_set_gpio_deglitch(syscon_sys, (int)id, cfg->wkup_mode, true);

		LOG_DBG("Wakeup src %u enabled (mode %u, substate %u)", id, cfg->wkup_mode,
			cfg->substate_id);
		return 0;
	case RTS5817_WKUP_SRC_RC_TIMER:
		val = cfg->wkup_time * 400U; /* ms → counter */

		syscon_update_bits(syscon_sys, R_RC_TIMER_CFG, RC_COUNTER_MASK,
				   val << RC_COUNTER_OFFSET);
		syscon_update_bits(syscon_sys, R_RC_TIMER_CFG, RC_TIMER_CLR_MASK,
				   RC_TIMER_CLR_MASK);
		syscon_update_bits(syscon_sys, R_RC_TIMER_CFG, RC_TIMER_EN_MASK, RC_TIMER_EN_MASK);
		LOG_DBG("RC timer wakeup enabled, timeout %u ms", cfg->wkup_time);
		return 0;
	case RTS5817_WKUP_SRC_USB_HOST:
	case RTS5817_WKUP_SRC_OCP:
		/* These wakeup sources are always enabled and cannot be disabled by software. */
		LOG_DBG("Wakeup src %u is always enabled", id);
		return 0;
	default:
		return -EINVAL;
	}
}

static int rts5817_wuc_disable_wakeup_source(const struct device *dev, uint32_t id)
{
	const struct rts5817_wuc_config *config = dev->config;
	const struct device *syscon_sys = config->syscon_sys;
	const struct rts5817_wkup_src_cfg *cfg;
	uint32_t reg_off;

	cfg = &config->wkup_src_cfg_list[id];

	switch (id) {
	case RTS5817_WKUP_SRC_GPIO_AL2:
	case RTS5817_WKUP_SRC_GPIO_AL1:
	case RTS5817_WKUP_SRC_GPIO_AL0:
	case RTS5817_WKUP_SRC_SENSOR_GPIO:
	case RTS5817_WKUP_SRC_SENSOR_CS:
	case RTS5817_WKUP_SRC_GPI_WAKE2:
	case RTS5817_WKUP_SRC_GPI_WAKE1:
		reg_off = substate_to_reg(cfg->substate_id);

		syscon_update_bits(syscon_sys, reg_off, BIT(EXIT_EN_BIT(id)), 0);
		rts5817_set_gpio_deglitch(syscon_sys, (int)id, cfg->wkup_mode, false);
		return 0;
	case RTS5817_WKUP_SRC_RC_TIMER:
		syscon_update_bits(syscon_sys, R_RC_TIMER_CFG, RC_TIMER_EN_MASK, 0);
		return 0;
	case RTS5817_WKUP_SRC_USB_HOST:
	case RTS5817_WKUP_SRC_OCP:
		/* These wakeup sources are always enabled and cannot be disabled by software. */
		LOG_DBG("Wakeup src %u is always enabled and cannot be disabled", id);
		return 0;
	default:
		return -EINVAL;
	}
}

static int rts5817_wuc_check_triggered(const struct device *dev, uint32_t id)
{
	const struct rts5817_wuc_config *config = dev->config;
	const struct device *syscon_sys = config->syscon_sys;
	const struct device *syscon_ldo = config->syscon_ldo;
	uint32_t bit;
	uint32_t suspend_reg;
	uint32_t sleep_reg;
	uint32_t reg;

	switch (id) {
	case RTS5817_WKUP_SRC_RC_TIMER:
		syscon_read_reg(syscon_sys, R_RC_TIMER_CFG, &reg);
		return (reg & RC_TIMER_WAKEUP_FLAG_MASK) ? 1 : 0;
	case RTS5817_WKUP_SRC_GPIO_AL2:
	case RTS5817_WKUP_SRC_GPIO_AL1:
	case RTS5817_WKUP_SRC_GPIO_AL0:
	case RTS5817_WKUP_SRC_SENSOR_GPIO:
	case RTS5817_WKUP_SRC_SENSOR_CS:
	case RTS5817_WKUP_SRC_GPI_WAKE2:
	case RTS5817_WKUP_SRC_GPI_WAKE1:
		/*
		 * Check EXIT_FLAG for both suspend and sleep offsets.
		 * We check both offsets since we don't know which substate was
		 * active when the wakeup occurred.
		 */
		syscon_read_reg(syscon_sys, R_EXIT_FLAG, &reg);
		bit = EXIT_FLAG_BIT(id);
		suspend_reg = (reg >> SUSPEND_EXIT_FLAG_OFFSET) & SLEEP_EXIT_FLAG_MASK;
		sleep_reg = (reg >> SLEEP_EXIT_FLAG_OFFSET) & SLEEP_EXIT_FLAG_MASK;

		if ((suspend_reg >> bit) & 1) {
			return 1;
		}
		if ((sleep_reg >> bit) & 1) {
			return 1;
		}
		return 0;
	case RTS5817_WKUP_SRC_USB_HOST:
		/* USB host wakeup doesn't have a dedicated wakeup flag. Instead, it sets the exit
		 * flag bits like a GPIO wakeup, but it can be distinguished by checking that no
		 * GPIO wakeup flags are set.
		 */
		syscon_read_reg(syscon_sys, R_EXIT_FLAG, &reg);
		suspend_reg = (reg >> SUSPEND_EXIT_FLAG_OFFSET) & SLEEP_EXIT_FLAG_MASK;
		sleep_reg = (reg >> SLEEP_EXIT_FLAG_OFFSET) & SLEEP_EXIT_FLAG_MASK;
		if (suspend_reg == 0 && sleep_reg == 0) {
			return 1;
		}
		return 0;
	case RTS5817_WKUP_SRC_OCP:
		/* OCP wakeup doesn't have a dedicated wakeup flag. Instead, it can be detected by
		 * checking the OCP status bit in the AL_DUMMY1 register.
		 */
		syscon_read_reg(syscon_ldo, R_LDO_TOP_STATUS, &reg);
		if (reg & (OC_POW_SVA_MASK | OC_POW_SVIO_MASK)) {
			return 1;
		}
		return 0;
	default:
		return -EINVAL;
	}
}

static int rts5817_wuc_clear_triggered(const struct device *dev, uint32_t id)
{
	const struct rts5817_wuc_config *config = dev->config;
	const struct device *syscon_sys = config->syscon_sys;

	ARG_UNUSED(id);

	syscon_update_bits(syscon_sys, R_EXIT_FLAG, PAD_EXIT_FLAG_CLR_PRE_MASK,
			   PAD_EXIT_FLAG_CLR_PRE_MASK);
	syscon_update_bits(syscon_sys, R_RC_TIMER_CFG, RC_TIMER_CLR_MASK, RC_TIMER_CLR_MASK);

	return 0;
}

static DEVICE_API(wuc, rts5817_wuc_api) = {
	.enable = rts5817_wuc_enable_wakeup_source,
	.disable = rts5817_wuc_disable_wakeup_source,
	.triggered = rts5817_wuc_check_triggered,
	.clear = rts5817_wuc_clear_triggered,
};

int rts_register_wakeup_event_callback(int id, rts5817_wakeup_event_cb_t cb, void *data)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct rts5817_wuc_data *wuc_data = dev->data;

	if (id >= RTS5817_WKUP_SRC_MAX_NUM || id == RTS5817_WKUP_SRC_NONE) {
		return -EINVAL;
	}

	wuc_data->wkup_evt[id].cb = cb;
	wuc_data->wkup_evt[id].data = data;

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int rts5817_wuc_resume(const struct device *dev)
{
	struct rts5817_wuc_data *wuc_data = dev->data;

	for (int i = 0; i < RTS5817_WKUP_SRC_MAX_NUM; i++) {
		if (wuc_data->wkup_evt[i].cb && rts5817_wuc_check_triggered(dev, i) > 0) {
			wuc_data->wkup_evt[i].cb(i, wuc_data->wkup_evt[i].data);
		}
	}

	return 0;
}

static int rts5817_wuc_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return rts5817_wuc_resume(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		return 0;
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

#define WKUP_SRC_INIT(node_id)                                                                     \
	[DT_PROP(node_id, id)] = {                                                                 \
		.id = DT_PROP(node_id, id),                                                        \
		.wkup_mode = DT_PROP_OR(node_id, mode, 0),                                         \
		.wkup_time = DT_PROP_OR(node_id, time, 0),                                         \
		.substate_id = DT_PROP(node_id, substate_id),                                      \
	},

static const struct rts5817_wuc_config wuc_cfg = {
	.syscon_sys = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(0), syscon_sys)),
	.syscon_ldo = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(0), syscon_ldo)),
	.wkup_src_cfg_list = {DT_FOREACH_CHILD(DT_DRV_INST(0), WKUP_SRC_INIT)},
};

static struct rts5817_wuc_data wuc_data;

PM_DEVICE_DT_INST_DEFINE(0, rts5817_wuc_pm_action);

DEVICE_DT_INST_DEFINE(0, NULL, PM_DEVICE_DT_INST_GET(0), &wuc_data, &wuc_cfg, PRE_KERNEL_1,
		      WUC_INIT_PRIORITY, &rts5817_wuc_api);
