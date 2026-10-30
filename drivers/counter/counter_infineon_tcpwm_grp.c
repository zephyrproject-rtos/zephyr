/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tcpwm_counter

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#else
#include <infineon_kconfig.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#endif

#define TCPWM_CTRL           0x00U
#define TCPWM_STATUS         0x04U
#define TCPWM_COUNTER        0x08U
#define TCPWM_CC0            0x10U
#define TCPWM_PERIOD         0x20U
#define TCPWM_DT             0x30U
#define TCPWM_TR_CMD         0x40U
#define TCPWM_TR_IN_SEL0     0x44U
#define TCPWM_TR_IN_SEL1     0x48U
#define TCPWM_TR_IN_EDGE_SEL 0x4CU
#define TCPWM_TR_OUT_SEL     0x54U
#define TCPWM_INTR           0x70U
#define TCPWM_INTR_SET       0x74U
#define TCPWM_INTR_MASK      0x78U
#define TCPWM_INTR_MASKED    0x7CU

#define TCPWM_CTRL_ENABLED           BIT(31)
#define TCPWM_STATUS_RUNNING         BIT(15)
#define TCPWM_TR_CMD_START           BIT(4)
#define TCPWM_TR_IN_SEL0_COUNT_SEL   GENMASK(13, 8)
#define TCPWM_TR_IN_ONE              1U
#define TCPWM_TR_IN_EDGE_SEL_NO_EDGE 0xFFFU
#define TCPWM_INTR_TC                BIT(0)
#define TCPWM_INTR_CC0               BIT(1)

struct ifx_tcpwm_grp_counter_config {
	struct counter_config_info info;
#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
	struct ifx_clk_peri clk;
	const struct device *clk_dev;
#else
	struct ifx_cat1_clock clock;
	en_clk_dst_t clk_dst;
#endif
	void (*irq_config)(void);
	mm_reg_t base;
};

struct ifx_tcpwm_grp_counter_data {
	counter_alarm_callback_t alarm_cb;
	struct k_spinlock lock;
	counter_top_callback_t top_cb;
	void *alarm_user_data;
	void *top_user_data;
	uint32_t guard;
#ifdef CONFIG_PM_DEVICE
	bool was_running;
#endif
};

static void tcpwm_irq_set(const struct device *dev, uint32_t intr, bool enable)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;
	struct ifx_tcpwm_grp_counter_data *data = dev->data;
	mm_reg_t base = config->base;
	k_spinlock_key_t key;
	uint32_t mask;

	key = k_spin_lock(&data->lock);
	mask = sys_read32(base + TCPWM_INTR_MASK);

	if (enable) {
		sys_write32(intr & ~mask, base + TCPWM_INTR);
		mask |= intr;
	} else {
		mask &= ~intr;
	}

	sys_write32(mask, base + TCPWM_INTR_MASK);
	k_spin_unlock(&data->lock, key);
}

static uint32_t ticks_add(uint32_t val, uint32_t delta, uint32_t top)
{
	if (likely((top & (top + 1U)) == 0U)) {
		return (val + delta) & top;
	}

	return (delta <= (top - val)) ? (val + delta) : (delta - (top - val) - 1U);
}

static uint32_t ticks_sub(uint32_t val, uint32_t old, uint32_t top)
{
	if (likely((top & (top + 1U)) == 0U)) {
		return (val - old) & top;
	}

	return (val >= old) ? (val - old) : (val + top + 1U - old);
}

static void ifx_tcpwm_grp_counter_isr(const struct device *dev)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;
	struct ifx_tcpwm_grp_counter_data *data = dev->data;
	mm_reg_t base = config->base;
	uint32_t pending;

	pending = sys_read32(base + TCPWM_INTR_MASKED);
	sys_write32(pending, base + TCPWM_INTR);
	(void)sys_read32(base + TCPWM_INTR);

	if (((pending & TCPWM_INTR_CC0) != 0U) && (data->alarm_cb != NULL)) {
		tcpwm_irq_set(dev, TCPWM_INTR_CC0, false);
		data->alarm_cb(dev, 0U, sys_read32(base + TCPWM_COUNTER), data->alarm_user_data);
	}

	if (((pending & TCPWM_INTR_TC) != 0U) && (data->top_cb != NULL)) {
		data->top_cb(dev, data->top_user_data);
	}
}

static int ifx_tcpwm_grp_counter_start(const struct device *dev)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;
	mm_reg_t base = config->base;

	sys_write32(TCPWM_CTRL_ENABLED, base + TCPWM_CTRL);
	sys_write32(TCPWM_TR_CMD_START, base + TCPWM_TR_CMD);

	return 0;
}

static int ifx_tcpwm_grp_counter_stop(const struct device *dev)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;

	sys_write32(0U, config->base + TCPWM_CTRL);

	return 0;
}

static int ifx_tcpwm_grp_counter_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;

	*ticks = sys_read32(config->base + TCPWM_COUNTER);

	return 0;
}

static int ifx_tcpwm_grp_counter_set_top_value(const struct device *dev,
					       const struct counter_top_cfg *cfg)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;
	struct ifx_tcpwm_grp_counter_data *data = dev->data;
	mm_reg_t base = config->base;
	int ret = 0;

	if (cfg->ticks > config->info.max_top_value) {
		return -ENOTSUP;
	}

	data->top_cb = cfg->callback;
	data->top_user_data = cfg->user_data;
	sys_write32(cfg->ticks, base + TCPWM_PERIOD);

	if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) == 0U) {
		sys_write32(0U, base + TCPWM_COUNTER);
	} else if (sys_read32(base + TCPWM_COUNTER) >= cfg->ticks) {
		ret = -ETIME;
		if ((cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) != 0U) {
			sys_write32(0U, base + TCPWM_COUNTER);
		}
	}

	tcpwm_irq_set(dev, TCPWM_INTR_TC, cfg->callback != NULL);

	return ret;
}

static uint32_t ifx_tcpwm_grp_counter_get_top_value(const struct device *dev)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;

	return sys_read32(config->base + TCPWM_PERIOD);
}

static int ifx_tcpwm_grp_counter_set_alarm(const struct device *dev, uint8_t chan_id,
					   const struct counter_alarm_cfg *alarm_cfg)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;
	struct ifx_tcpwm_grp_counter_data *data = dev->data;
	mm_reg_t base = config->base;
	uint32_t top = sys_read32(base + TCPWM_PERIOD);
	uint32_t ticks = alarm_cfg->ticks;
	bool absolute = (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0U;
	uint32_t max_rel;
	bool irq_on_late;
	uint32_t now;

	ARG_UNUSED(chan_id);

	data->alarm_cb = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;

	if (ticks > top) {
		return -EINVAL;
	}

	if (absolute) {
		max_rel = top - data->guard;
		irq_on_late = (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) != 0U;
	} else {
		irq_on_late = ticks < (top / 2U);
		max_rel = irq_on_late ? (top / 2U) : top;
		ticks = ticks_add(sys_read32(base + TCPWM_COUNTER), ticks, top);
	}

	now = sys_read32(base + TCPWM_COUNTER);

	if ((absolute && (ticks < now)) || (ticks_sub(ticks - 1U, now, top) > max_rel)) {
		if (irq_on_late) {
			tcpwm_irq_set(dev, TCPWM_INTR_CC0, true);
			sys_write32(TCPWM_INTR_CC0, base + TCPWM_INTR_SET);
		}

		return absolute ? -ETIME : 0;
	}

	sys_write32(ticks, base + TCPWM_CC0);
	tcpwm_irq_set(dev, TCPWM_INTR_CC0, true);

	return 0;
}

static int ifx_tcpwm_grp_counter_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);

	tcpwm_irq_set(dev, TCPWM_INTR_CC0, false);

	return 0;
}

static uint32_t ifx_tcpwm_grp_counter_get_pending_int(const struct device *dev)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;

	return sys_read32(config->base + TCPWM_INTR_MASKED) & (TCPWM_INTR_TC | TCPWM_INTR_CC0);
}

static uint32_t ifx_tcpwm_grp_counter_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct ifx_tcpwm_grp_counter_data *data = dev->data;

	ARG_UNUSED(flags);

	return data->guard;
}

static int ifx_tcpwm_grp_counter_set_guard_period(const struct device *dev, uint32_t guard,
						  uint32_t flags)
{
	struct ifx_tcpwm_grp_counter_data *data = dev->data;

	ARG_UNUSED(flags);
	__ASSERT_NO_MSG(guard < ifx_tcpwm_grp_counter_get_top_value(dev));

	data->guard = guard;

	return 0;
}

#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
static uint32_t ifx_tcpwm_grp_counter_get_freq(const struct device *dev)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;
	uint32_t rate;
	int ret;

	ret = clock_control_get_rate(config->clk_dev, (clock_control_subsys_t)&config->clk, &rate);
	if (ret < 0) {
		return 0U;
	}

	return rate;
}

static int ifx_tcpwm_grp_counter_clock_init(const struct device *dev)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;
	uint32_t rate = config->info.freq;

	return clock_control_set_rate(config->clk_dev, (clock_control_subsys_t)&config->clk, &rate);
}
#else
static uint32_t ifx_tcpwm_grp_counter_get_freq(const struct device *dev)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;

	return ifx_cat1_utils_peri_pclk_get_frequency(config->clk_dst, &config->clock);
}

static int ifx_tcpwm_grp_counter_clock_init(const struct device *dev)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;

	if (ifx_cat1_utils_peri_pclk_assign_divider(config->clk_dst, &config->clock) !=
	    CY_RSLT_SUCCESS) {
		return -EIO;
	}

	return 0;
}
#endif

static int ifx_tcpwm_grp_counter_init(const struct device *dev)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;
	mm_reg_t base = config->base;
	int ret;

	sys_write32(0U, base + TCPWM_CTRL);
	sys_write32(0U, base + TCPWM_INTR_MASK);
	sys_write32(0U, base + TCPWM_DT);
	sys_write32(FIELD_PREP(TCPWM_TR_IN_SEL0_COUNT_SEL, TCPWM_TR_IN_ONE),
		    base + TCPWM_TR_IN_SEL0);
	sys_write32(0U, base + TCPWM_TR_IN_SEL1);
	sys_write32(TCPWM_TR_IN_EDGE_SEL_NO_EDGE, base + TCPWM_TR_IN_EDGE_SEL);
	sys_write32(0U, base + TCPWM_TR_OUT_SEL);
	sys_write32(config->info.max_top_value, base + TCPWM_PERIOD);
	sys_write32(0U, base + TCPWM_COUNTER);

	ret = ifx_tcpwm_grp_counter_clock_init(dev);
	if (ret < 0) {
		return ret;
	}

	sys_write32(TCPWM_CTRL_ENABLED, base + TCPWM_CTRL);
	config->irq_config();

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int ifx_tcpwm_grp_counter_pm_action(const struct device *dev,
					   enum pm_device_action action)
{
	const struct ifx_tcpwm_grp_counter_config *config = dev->config;
	struct ifx_tcpwm_grp_counter_data *data = dev->data;
	mm_reg_t base = config->base;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		data->was_running =
			(sys_read32(base + TCPWM_STATUS) & TCPWM_STATUS_RUNNING) != 0U;
		sys_write32(0U, base + TCPWM_CTRL);
		break;
	case PM_DEVICE_ACTION_RESUME:
		sys_write32(TCPWM_CTRL_ENABLED, base + TCPWM_CTRL);
		if (data->was_running) {
			sys_write32(TCPWM_TR_CMD_START, base + TCPWM_TR_CMD);
		}
		break;
#if defined(CONFIG_PM_S2RAM) || defined(CONFIG_PM_DEVICE_POWER_DOMAIN)
	case PM_DEVICE_ACTION_TURN_ON:
		return ifx_tcpwm_grp_counter_init(dev);
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static DEVICE_API(counter, ifx_tcpwm_grp_counter_api) = {
	.start = ifx_tcpwm_grp_counter_start,
	.stop = ifx_tcpwm_grp_counter_stop,
	.get_value = ifx_tcpwm_grp_counter_get_value,
	.set_alarm = ifx_tcpwm_grp_counter_set_alarm,
	.cancel_alarm = ifx_tcpwm_grp_counter_cancel_alarm,
	.set_top_value = ifx_tcpwm_grp_counter_set_top_value,
	.get_pending_int = ifx_tcpwm_grp_counter_get_pending_int,
	.get_top_value = ifx_tcpwm_grp_counter_get_top_value,
	.get_freq = ifx_tcpwm_grp_counter_get_freq,
	.get_guard_period = ifx_tcpwm_grp_counter_get_guard_period,
	.set_guard_period = ifx_tcpwm_grp_counter_set_guard_period,
};

#define TCPWM_GRP_MAX_TOP(n)                                                                       \
	((DT_PROP(DT_INST_PARENT(n), resolution) == 32) ? UINT32_MAX : UINT16_MAX)

#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
#define TCPWM_GRP_CLOCK_INIT(n)                                                                    \
	.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(n))),                               \
	.clk = {                                                                                   \
		.div_ord = DT_DEP_ORD(DT_INST_CLOCKS_CTLR(n)),                                     \
		.rootclk_id = DT_PROP(DT_INST_PARENT(n), clk_dst),                                 \
	},
#define TCPWM_GRP_CLOCK_ASSERT(n)                                                                  \
	BUILD_ASSERT(DT_NODE_HAS_PROP(DT_INST_PARENT(n), clock_frequency),                         \
		     "parent must have clock-frequency");
#else
#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define TCPWM_GRP_CLOCK_BLOCK(n)                                                                   \
	IFX_CAT1_PERIPHERAL_GROUP_ADJUST(DT_PROP_BY_IDX(DT_INST_CLOCKS_CTLR(n), peri_group, 0),    \
					 DT_PROP_BY_IDX(DT_INST_CLOCKS_CTLR(n), peri_group, 1),    \
					 DT_PROP(DT_INST_CLOCKS_CTLR(n), div_type))
#else
#define TCPWM_GRP_CLOCK_BLOCK(n)                                                                   \
	IFX_CAT1_PERIPHERAL_GROUP_ADJUST(DT_PROP_BY_IDX(DT_INST_CLOCKS_CTLR(n), peri_group, 1),    \
					 DT_PROP(DT_INST_CLOCKS_CTLR(n), div_type))
#endif
#define TCPWM_GRP_CLOCK_INIT(n)                                                                    \
	.clock = {                                                                                 \
		.block = TCPWM_GRP_CLOCK_BLOCK(n),                                                 \
		.channel = DT_PROP(DT_INST_CLOCKS_CTLR(n), channel),                               \
	},                                                                                         \
	.clk_dst = DT_PROP(DT_INST_PARENT(n), clk_dst),
#define TCPWM_GRP_CLOCK_ASSERT(n)
#endif

#define IFX_TCPWM_GRP_COUNTER_INIT(n)                                                              \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_INST_PARENT(n), infineon_tcpwm),                        \
		     "parent must be infineon,tcpwm");                                             \
	TCPWM_GRP_CLOCK_ASSERT(n)                                                                  \
	BUILD_ASSERT(DT_IRQ_HAS_IDX(DT_INST_PARENT(n), 0), "parent must have an interrupt");       \
                                                                                                   \
	static void ifx_tcpwm_grp_counter_irq_config_##n(void)                                     \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)), DT_IRQ(DT_INST_PARENT(n), priority),       \
			    ifx_tcpwm_grp_counter_isr, DEVICE_DT_INST_GET(n), 0);                  \
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));                                            \
	}                                                                                          \
                                                                                                   \
	static struct ifx_tcpwm_grp_counter_data ifx_tcpwm_grp_counter_data_##n;                   \
                                                                                                   \
	static const struct ifx_tcpwm_grp_counter_config ifx_tcpwm_grp_counter_config_##n = {      \
		.info = {                                                                          \
			.max_top_value = TCPWM_GRP_MAX_TOP(n),                                     \
			.freq = DT_PROP_OR(DT_INST_PARENT(n), clock_frequency, 0),                 \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,                                     \
			.channels = 1U,                                                            \
		},                                                                                 \
		.base = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
		TCPWM_GRP_CLOCK_INIT(n)                                                            \
		.irq_config = ifx_tcpwm_grp_counter_irq_config_##n,                                \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, ifx_tcpwm_grp_counter_pm_action);                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_tcpwm_grp_counter_init, PM_DEVICE_DT_INST_GET(n),             \
			      &ifx_tcpwm_grp_counter_data_##n,                                     \
			      &ifx_tcpwm_grp_counter_config_##n, POST_KERNEL,                      \
			      CONFIG_COUNTER_INIT_PRIORITY, &ifx_tcpwm_grp_counter_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_TCPWM_GRP_COUNTER_INIT)
