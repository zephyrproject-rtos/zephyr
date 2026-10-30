/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tcpwm_pwm

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/dt-bindings/pwm/pwm_ifx_tcpwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#else
#include <infineon_kconfig.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#endif

#ifdef CONFIG_PWM_EVENT
#include <zephyr/drivers/pwm/pwm_utils.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#endif /* CONFIG_PWM_EVENT */

LOG_MODULE_REGISTER(pwm_ifx_tcpwm_grp, CONFIG_PWM_LOG_LEVEL);

#define TCPWM_CTRL           0x00U
#define TCPWM_STATUS         0x04U
#define TCPWM_COUNTER        0x08U
#define TCPWM_CC0            0x10U
#define TCPWM_CC0_BUFF       0x14U
#define TCPWM_PERIOD         0x20U
#define TCPWM_PERIOD_BUFF    0x24U
#define TCPWM_LINE_SEL       0x28U
#define TCPWM_DT             0x30U
#define TCPWM_TR_CMD         0x40U
#define TCPWM_TR_IN_SEL0     0x44U
#define TCPWM_TR_IN_SEL1     0x48U
#define TCPWM_TR_IN_EDGE_SEL 0x4CU
#define TCPWM_TR_PWM_CTRL    0x50U
#define TCPWM_TR_OUT_SEL     0x54U
#define TCPWM_INTR           0x70U
#define TCPWM_INTR_MASK      0x78U
#define TCPWM_INTR_MASKED    0x7CU

#define TCPWM_CTRL_AUTO_RELOAD_CC0    BIT(0)
#define TCPWM_CTRL_AUTO_RELOAD_PERIOD BIT(2)
#define TCPWM_CTRL_PWM_DISABLE_MODE   GENMASK(13, 12)
#define TCPWM_CTRL_QUAD_ENCODING_MODE GENMASK(21, 20)
#define TCPWM_CTRL_MODE               GENMASK(26, 24)
#define TCPWM_CTRL_MODE_PWM           4U
#define TCPWM_CTRL_ENABLED            BIT(31)
#define TCPWM_STATUS_RUNNING          BIT(15)
#define TCPWM_TR_CMD_CAPTURE0         BIT(0)
#define TCPWM_TR_CMD_START            BIT(4)
#define TCPWM_TR_IN_SEL0_COUNT_SEL    GENMASK(13, 8)
#define TCPWM_TR_IN_ONE               1U
#define TCPWM_TR_IN_EDGE_SEL_COUNT    GENMASK(3, 2)
#define TCPWM_TR_IN_EDGE_NO_EDGE_DET  3U
#define TCPWM_TR_PWM_CTRL_CC0_MATCH   GENMASK(1, 0)
#define TCPWM_TR_PWM_CTRL_OVERFLOW    GENMASK(3, 2)
#define TCPWM_TR_PWM_CTRL_UNDERFLOW   GENMASK(5, 4)
#define TCPWM_TR_PWM_CTRL_CC1_MATCH   GENMASK(7, 6)
#define TCPWM_TR_PWM_SET              0U
#define TCPWM_TR_PWM_CLEAR            1U
#define TCPWM_TR_PWM_NO_CHANGE        3U
#define TCPWM_LINE_SEL_OUT_SEL        GENMASK(2, 0)
#define TCPWM_LINE_SEL_COMPL_OUT_SEL  GENMASK(6, 4)
#define TCPWM_LINE_SEL_PWM            2U
#define TCPWM_LINE_SEL_PWM_INV        3U
#define TCPWM_INTR_TC                 BIT(0)
#define TCPWM_INTR_CC0                BIT(1)

struct ifx_tcpwm_grp_pwm_config {
	mm_reg_t base;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
	const struct device *clk_dev;
	struct ifx_clk_peri clk;
	uint32_t freq;
#else
	struct ifx_cat1_clock clock;
	en_clk_dst_t clk_dst;
#endif
	uint32_t max_cycles;
#ifdef CONFIG_PWM_EVENT
	void (*irq_config)(void);
#endif /* CONFIG_PWM_EVENT */
};

#ifdef CONFIG_PWM_EVENT
struct ifx_tcpwm_grp_pwm_data {
	sys_slist_t event_callbacks;
	struct k_spinlock lock;
};
#endif /* CONFIG_PWM_EVENT */

static int ifx_tcpwm_grp_pwm_set_cycles(const struct device *dev, uint32_t channel,
					uint32_t period_cycles, uint32_t pulse_cycles,
					pwm_flags_t flags)
{
	const struct ifx_tcpwm_grp_pwm_config *config = dev->config;
	mm_reg_t base = config->base;
	uint32_t ctrl;

	ARG_UNUSED(channel);

	if ((period_cycles > config->max_cycles) || (pulse_cycles > config->max_cycles)) {
		LOG_ERR("Period %u or pulse %u cycles exceed the counter width", period_cycles,
			pulse_cycles);
		return -EINVAL;
	}

	ctrl = sys_read32(base + TCPWM_CTRL) &
	       ~(TCPWM_CTRL_QUAD_ENCODING_MODE | TCPWM_CTRL_PWM_DISABLE_MODE);
	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED) {
		ctrl |= TCPWM_CTRL_QUAD_ENCODING_MODE;
	}
	ctrl |= FIELD_PREP(TCPWM_CTRL_PWM_DISABLE_MODE, flags >> PWM_IFX_TCPWM_OUTPUT_POS);
	sys_write32(ctrl, base + TCPWM_CTRL);

	if ((period_cycles != 0U) && (pulse_cycles != 0U) &&
	    ((sys_read32(base + TCPWM_STATUS) & TCPWM_STATUS_RUNNING) == 0U)) {
		sys_write32(period_cycles - 1U, base + TCPWM_PERIOD);
		sys_write32(pulse_cycles, base + TCPWM_CC0);
	}

	if (period_cycles == 0U) {
		sys_write32(0U, base + TCPWM_PERIOD_BUFF);
		sys_write32(0U, base + TCPWM_CC0_BUFF);
	} else {
		sys_write32(period_cycles - 1U, base + TCPWM_PERIOD_BUFF);
		sys_write32(pulse_cycles, base + TCPWM_CC0_BUFF);
	}

	sys_write32(TCPWM_TR_CMD_CAPTURE0, base + TCPWM_TR_CMD);
	sys_write32(ctrl | TCPWM_CTRL_ENABLED, base + TCPWM_CTRL);
	sys_write32(TCPWM_TR_CMD_START, base + TCPWM_TR_CMD);

	return 0;
}

#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
static int ifx_tcpwm_grp_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
						uint64_t *cycles)
{
	const struct ifx_tcpwm_grp_pwm_config *config = dev->config;
	uint32_t rate;
	int ret;

	ARG_UNUSED(channel);

	ret = clock_control_get_rate(config->clk_dev, (clock_control_subsys_t)&config->clk, &rate);
	if (ret < 0) {
		return ret;
	}

	*cycles = rate;

	return 0;
}

static int ifx_tcpwm_grp_pwm_clock_init(const struct ifx_tcpwm_grp_pwm_config *config)
{
	uint32_t rate = config->freq;

	return clock_control_set_rate(config->clk_dev, (clock_control_subsys_t)&config->clk, &rate);
}
#else
static int ifx_tcpwm_grp_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
						uint64_t *cycles)
{
	const struct ifx_tcpwm_grp_pwm_config *config = dev->config;

	ARG_UNUSED(channel);

	*cycles = ifx_cat1_utils_peri_pclk_get_frequency(config->clk_dst, &config->clock);

	return 0;
}

static int ifx_tcpwm_grp_pwm_clock_init(const struct ifx_tcpwm_grp_pwm_config *config)
{

	if (ifx_cat1_utils_peri_pclk_assign_divider(config->clk_dst, &config->clock) !=
	    CY_RSLT_SUCCESS) {
		return -EIO;
	}

	return 0;
}
#endif

#ifdef CONFIG_PWM_EVENT
static void ifx_tcpwm_grp_pwm_isr(const struct device *dev)
{
	const struct ifx_tcpwm_grp_pwm_config *config = dev->config;
	struct ifx_tcpwm_grp_pwm_data *data = dev->data;
	mm_reg_t base = config->base;
	uint32_t pending;

	pending = sys_read32(base + TCPWM_INTR_MASKED);
	sys_write32(pending, base + TCPWM_INTR);
	(void)sys_read32(base + TCPWM_INTR);

	if ((pending & TCPWM_INTR_TC) != 0U) {
		pwm_fire_event_callbacks(&data->event_callbacks, dev, 0U, PWM_EVENT_TYPE_PERIOD);
	}

	if ((pending & TCPWM_INTR_CC0) != 0U) {
		pwm_fire_event_callbacks(&data->event_callbacks, dev, 0U,
					 PWM_EVENT_TYPE_COMPARE_CAPTURE);
	}
}

static int ifx_tcpwm_grp_pwm_manage_event_callback(const struct device *dev,
						   struct pwm_event_callback *callback, bool set)
{
	const struct ifx_tcpwm_grp_pwm_config *config = dev->config;
	struct ifx_tcpwm_grp_pwm_data *data = dev->data;
	mm_reg_t base = config->base;
	struct pwm_event_callback *cb;
	k_spinlock_key_t key;
	uint32_t mask = 0U;
	int ret;

	ret = pwm_manage_event_callback(&data->event_callbacks, callback, set);
	if (ret < 0) {
		return ret;
	}

	key = k_spin_lock(&data->lock);

	SYS_SLIST_FOR_EACH_CONTAINER(&data->event_callbacks, cb, node) {
		if ((cb->event_mask & PWM_EVENT_TYPE_PERIOD) != 0U) {
			mask |= TCPWM_INTR_TC;
		}
		if ((cb->event_mask & PWM_EVENT_TYPE_COMPARE_CAPTURE) != 0U) {
			mask |= TCPWM_INTR_CC0;
		}
	}

	sys_write32(mask, base + TCPWM_INTR);
	(void)sys_read32(base + TCPWM_INTR);
	sys_write32(mask, base + TCPWM_INTR_MASK);

	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif /* CONFIG_PWM_EVENT */

static int ifx_tcpwm_grp_pwm_init(const struct device *dev)
{
	const struct ifx_tcpwm_grp_pwm_config *config = dev->config;
	mm_reg_t base = config->base;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = ifx_tcpwm_grp_pwm_clock_init(config);
	if (ret < 0) {
		return ret;
	}

	sys_write32(TCPWM_CTRL_AUTO_RELOAD_CC0 | TCPWM_CTRL_AUTO_RELOAD_PERIOD |
		    FIELD_PREP(TCPWM_CTRL_MODE, TCPWM_CTRL_MODE_PWM),
		    base + TCPWM_CTRL);
	sys_write32(0U, base + TCPWM_DT);
	sys_write32(0U, base + TCPWM_COUNTER);
	sys_write32(FIELD_PREP(TCPWM_TR_PWM_CTRL_CC0_MATCH, TCPWM_TR_PWM_CLEAR) |
		    FIELD_PREP(TCPWM_TR_PWM_CTRL_OVERFLOW, TCPWM_TR_PWM_SET) |
		    FIELD_PREP(TCPWM_TR_PWM_CTRL_UNDERFLOW, TCPWM_TR_PWM_NO_CHANGE) |
		    FIELD_PREP(TCPWM_TR_PWM_CTRL_CC1_MATCH, TCPWM_TR_PWM_NO_CHANGE),
		    base + TCPWM_TR_PWM_CTRL);
	sys_write32(0U, base + TCPWM_CC0);
	sys_write32(0U, base + TCPWM_CC0_BUFF);
	sys_write32(0U, base + TCPWM_PERIOD);
	sys_write32(0U, base + TCPWM_PERIOD_BUFF);
	sys_write32(FIELD_PREP(TCPWM_TR_IN_SEL0_COUNT_SEL, TCPWM_TR_IN_ONE),
		    base + TCPWM_TR_IN_SEL0);
	sys_write32(0U, base + TCPWM_TR_IN_SEL1);
	sys_write32(FIELD_PREP(TCPWM_TR_IN_EDGE_SEL_COUNT, TCPWM_TR_IN_EDGE_NO_EDGE_DET),
		    base + TCPWM_TR_IN_EDGE_SEL);
	sys_write32(0U, base + TCPWM_TR_OUT_SEL);
	sys_write32(0U, base + TCPWM_INTR_MASK);
	sys_write32(FIELD_PREP(TCPWM_LINE_SEL_OUT_SEL, TCPWM_LINE_SEL_PWM) |
		    FIELD_PREP(TCPWM_LINE_SEL_COMPL_OUT_SEL, TCPWM_LINE_SEL_PWM_INV),
		    base + TCPWM_LINE_SEL);

#ifdef CONFIG_PWM_EVENT
	config->irq_config();
#endif /* CONFIG_PWM_EVENT */

	return 0;
}

static DEVICE_API(pwm, ifx_tcpwm_grp_pwm_api) = {
	.set_cycles = ifx_tcpwm_grp_pwm_set_cycles,
	.get_cycles_per_sec = ifx_tcpwm_grp_pwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_EVENT
	.manage_event_callback = ifx_tcpwm_grp_pwm_manage_event_callback,
#endif /* CONFIG_PWM_EVENT */
};

#define IFX_TCPWM_GRP_PWM_IRQ_CONFIG(n)                                                            \
	static struct ifx_tcpwm_grp_pwm_data ifx_tcpwm_grp_pwm_data_##n;                           \
                                                                                                   \
	static void ifx_tcpwm_grp_pwm_irq_config_##n(void)                                         \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)), DT_IRQ(DT_INST_PARENT(n), priority),       \
			    ifx_tcpwm_grp_pwm_isr, DEVICE_DT_INST_GET(n), 0);                      \
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));                                            \
	}

#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
#define IFX_TCPWM_GRP_PWM_CLOCK_INIT(n)                                                            \
	.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(n))),                               \
	.clk = {                                                                                   \
		.div_ord = DT_DEP_ORD(DT_INST_CLOCKS_CTLR(n)),                                     \
		.rootclk_id = DT_PROP(DT_INST_PARENT(n), clk_dst),                                 \
	},                                                                                         \
	.freq = DT_PROP(DT_INST_PARENT(n), clock_frequency),
#else
#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define IFX_TCPWM_GRP_PWM_CLOCK_BLOCK(n)                                                           \
	IFX_CAT1_PERIPHERAL_GROUP_ADJUST(DT_PROP_BY_IDX(DT_INST_CLOCKS_CTLR(n), peri_group, 0),    \
					 DT_PROP_BY_IDX(DT_INST_CLOCKS_CTLR(n), peri_group, 1),    \
					 DT_PROP(DT_INST_CLOCKS_CTLR(n), div_type))
#else
#define IFX_TCPWM_GRP_PWM_CLOCK_BLOCK(n)                                                           \
	IFX_CAT1_PERIPHERAL_GROUP_ADJUST(DT_PROP_BY_IDX(DT_INST_CLOCKS_CTLR(n), peri_group, 1),    \
					 DT_PROP(DT_INST_CLOCKS_CTLR(n), div_type))
#endif
#define IFX_TCPWM_GRP_PWM_CLOCK_INIT(n)                                                            \
	.clock = {                                                                                 \
		.block = IFX_TCPWM_GRP_PWM_CLOCK_BLOCK(n),                                         \
		.channel = DT_PROP(DT_INST_CLOCKS_CTLR(n), channel),                               \
	},                                                                                         \
	.clk_dst = DT_PROP(DT_INST_PARENT(n), clk_dst),
#endif

#define IFX_TCPWM_GRP_PWM_INIT(n)                                                                  \
	IF_ENABLED(CONFIG_PWM_EVENT, (IFX_TCPWM_GRP_PWM_IRQ_CONFIG(n)))                            \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct ifx_tcpwm_grp_pwm_config ifx_tcpwm_grp_pwm_config_##n = {              \
		.base = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		IFX_TCPWM_GRP_PWM_CLOCK_INIT(n)                                                    \
		.max_cycles = BIT64_MASK(DT_PROP(DT_INST_PARENT(n), resolution)),                  \
		IF_ENABLED(CONFIG_PWM_EVENT,                                                       \
			   (.irq_config = ifx_tcpwm_grp_pwm_irq_config_##n,))                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_tcpwm_grp_pwm_init, NULL,                                     \
			      COND_CODE_1(CONFIG_PWM_EVENT, (&ifx_tcpwm_grp_pwm_data_##n),         \
					  (NULL)),                                                 \
			      &ifx_tcpwm_grp_pwm_config_##n, POST_KERNEL,                          \
			      CONFIG_PWM_INIT_PRIORITY, &ifx_tcpwm_grp_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_TCPWM_GRP_PWM_INIT)
