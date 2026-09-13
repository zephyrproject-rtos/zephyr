/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include "can_mcan.h"
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

#include <infineon_kconfig.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>

#include <cy_canfd.h>
#include <cy_syslib.h>

LOG_MODULE_REGISTER(can_infineon, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT infineon_can

struct can_infineon_data {
	struct ifx_cat1_clock clock;
};

struct can_infineon_config {
	mm_reg_t base;
	mem_addr_t mrba;
	mem_addr_t mram;
	void (*config_irq)(void);
	const struct pinctrl_dev_config *pcfg;
	const struct device *ctrl_dev;
	en_clk_dst_t clk_dst;
	uint32_t clk_div;
};

/*
 * Wrapper config — defined here (before DT_DRV_COMPAT switch) so the
 * channel driver can access the parent wrapper's config.
 */
struct canfd_ifx_ctrl_config {
	CANFD_Type *base;
#ifdef CONFIG_CAN_RX_TIMESTAMP
	bool timestamp_counter;
#endif /* CONFIG_CAN_RX_TIMESTAMP */
	bool ecc_enabled;
};

static int can_infineon_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_infineon_config *infineon_cfg = mcan_cfg->custom;

	return can_mcan_sys_read_reg(infineon_cfg->base, reg, val);
}

static int can_infineon_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_infineon_config *infineon_cfg = mcan_cfg->custom;

	return can_mcan_sys_write_reg(infineon_cfg->base, reg, val);
}

static int can_infineon_read_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_infineon_config *infineon_cfg = mcan_cfg->custom;

	return can_mcan_sys_read_mram(infineon_cfg->mram, offset, dst, len);
}

static int can_infineon_write_mram(const struct device *dev, uint16_t offset, const void *src,
				   size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_infineon_config *infineon_cfg = mcan_cfg->custom;

	return can_mcan_sys_write_mram(infineon_cfg->mram, offset, src, len);
}

static int can_infineon_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_infineon_config *infineon_cfg = mcan_cfg->custom;

	return can_mcan_sys_clear_mram(infineon_cfg->mram, offset, len);
}

static int can_infineon_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_infineon_config *infineon_cfg = mcan_cfg->custom;
	struct can_mcan_data *mcan_data = dev->data;
	struct can_infineon_data *data = mcan_data->custom;

	*rate = ifx_cat1_utils_peri_pclk_get_frequency(infineon_cfg->clk_dst, &data->clock);

	return 0;
}

/*
 * Rebuild only the M_CAN core: the Message RAM layout, the controller and -
 * when enabled - the external timestamp source and the IRQ lines.  This is the
 * state lost when the CAN power domain is gated (DeepSleep) or power-cycled
 * (DeepSleep-RAM).  The clock divider and pin routing sit in the retained
 * PERI/IOSS domains and are left untouched.
 */
static int can_infineon_reinit_core(const struct device *dev, bool reconnect_irq)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_infineon_config *infineon_cfg = mcan_cfg->custom;
	int ret;

	ret = can_mcan_configure_mram(dev, infineon_cfg->mrba, infineon_cfg->mram);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret != 0) {
		LOG_ERR("can_mcan_init failed (%d)", ret);
		return ret;
	}

#ifdef CONFIG_CAN_RX_TIMESTAMP
	/*
	 * If the parent controller has the shared timestamp counter enabled,
	 * select the external timestamp source (TSS = 2) in the M_CAN core.
	 * This must be done after can_mcan_init() which sets TSS = 1.
	 */
	const struct canfd_ifx_ctrl_config *ctrl_cfg = infineon_cfg->ctrl_dev->config;

	if (ctrl_cfg->timestamp_counter) {
		ret = can_mcan_write_reg(dev, CAN_MCAN_TSCC, FIELD_PREP(CAN_MCAN_TSCC_TSS, 2U));
		if (ret != 0) {
			return ret;
		}
	}
#endif /* CONFIG_CAN_RX_TIMESTAMP */

	if (reconnect_irq && (infineon_cfg->config_irq != NULL)) {
		infineon_cfg->config_irq();
	}

	return 0;
}

/*
 * Apply the pin routing and program the CAN functional-clock divider.  Done in
 * the driver because the central clock-control driver is absent on the CM55/SRF
 * build, as in the other PDL drivers.  The divider is disabled before its value
 * changes so it latches cleanly.  Shared by cold init and the DS-RAM rebuild.
 */
static int can_infineon_setup_clock(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_infineon_config *infineon_cfg = mcan_cfg->custom;
	struct can_mcan_data *mcan_data = dev->data;
	struct can_infineon_data *data = mcan_data->custom;
	cy_rslt_t result;
	int ret;

	ret = pinctrl_apply_state(infineon_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("CAN pinctrl setup failed (%d)", ret);
		return ret;
	}

	result = ifx_cat1_utils_peri_pclk_assign_divider(infineon_cfg->clk_dst, &data->clock);
	if (result != CY_RSLT_SUCCESS) {
		LOG_ERR("CAN clock assign failed (%d)", (int)result);
		return -EIO;
	}

	(void)ifx_cat1_utils_peri_pclk_disable_divider(infineon_cfg->clk_dst, &data->clock);

	/*
	 * The 16.5-bit and 24.5-bit dividers are fractional and the integer
	 * set_divider() rejects them, so use set_frac_divider() with a zero
	 * fractional part.  Mirrors the SCB and clock-control drivers.
	 */
	if (ifx_cat1_utils_peri_is_fract_div(&data->clock)) {
		result = ifx_cat1_utils_peri_pclk_set_frac_divider(
			infineon_cfg->clk_dst, &data->clock, infineon_cfg->clk_div - 1U, 0U);
	} else {
		result = ifx_cat1_utils_peri_pclk_set_divider(infineon_cfg->clk_dst, &data->clock,
							      infineon_cfg->clk_div - 1U);
	}
	if (result != CY_RSLT_SUCCESS) {
		LOG_ERR("CAN clock divider set failed (%d)", (int)result);
		return -EIO;
	}
	(void)ifx_cat1_utils_peri_pclk_enable_divider(infineon_cfg->clk_dst, &data->clock);

	return 0;
}

static int can_infineon_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_infineon_config *infineon_cfg = mcan_cfg->custom;
	int ret;

	/* Ensure the parent controller (MRAM + channel clocks) is ready */
	if (!device_is_ready(infineon_cfg->ctrl_dev)) {
		LOG_ERR_DEVICE_NOT_READY(infineon_cfg->ctrl_dev);
		return -ENODEV;
	}

	ret = can_infineon_setup_clock(dev);
	if (ret != 0) {
		return ret;
	}

	return can_infineon_reinit_core(dev, true);
}

#ifdef CONFIG_PM_DEVICE
static int can_infineon_pm_action(const struct device *dev, enum pm_device_action action)
{
	uint32_t reg;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* Refuse mid-transmit; gating the core clock would abort the frame. */
		ret = can_infineon_read_reg(dev, CAN_MCAN_CCCR, &reg);
		if (ret != 0) {
			return ret;
		}
		if ((reg & CAN_MCAN_CCCR_INIT) == 0U) {
			ret = can_infineon_read_reg(dev, CAN_MCAN_TXBRP, &reg);
			if (ret != 0) {
				return ret;
			}
			if ((reg & CAN_MCAN_TXBRP_TRP) != 0U) {
				return -EBUSY;
			}
		}
		break;
	case PM_DEVICE_ACTION_RESUME:
		/*
		 * DeepSleep gates the CAN domain, so the M_CAN returns at reset
		 * defaults.  Rebuild the core only - the clock divider and pins are
		 * retained - after checking the power-gate witness: ILE is set solely
		 * by can_mcan_init() and never by start/stop, so ILE == 0 means the
		 * domain was gated.  The wrapper re-powers the Message RAM first.
		 */
		ret = can_infineon_read_reg(dev, CAN_MCAN_ILE, &reg);
		if (ret != 0) {
			return ret;
		}
		if (reg == 0U) {
			return can_infineon_reinit_core(dev, true);
		}
		break;
#if defined(CONFIG_PM_S2RAM) || defined(CONFIG_PM_DEVICE_POWER_DOMAIN)
	case PM_DEVICE_ACTION_TURN_ON:
		/*
		 * Power was lost: re-run the full channel init (pinctrl, clock
		 * divider, Message RAM and M_CAN core).  Unlike RESUME the PERI/IOSS
		 * domains are gone, so the clock and pins are reprogrammed too.  Runs
		 * on the eager warm-boot resume path, pre-scheduler with IRQs masked.
		 */
		return can_infineon_init(dev);
#endif /* CONFIG_PM_S2RAM || CONFIG_PM_DEVICE_POWER_DOMAIN */
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(can, can_infineon_driver_api) = {
	.get_capabilities = can_mcan_get_capabilities,
	.start = can_mcan_start,
	.stop = can_mcan_stop,
	.set_mode = can_mcan_set_mode,
	.set_timing = can_mcan_set_timing,
	.send = can_mcan_send,
	.add_rx_filter = can_mcan_add_rx_filter,
	.remove_rx_filter = can_mcan_remove_rx_filter,
	.get_state = can_mcan_get_state,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_mcan_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE*/
	.get_core_clock = can_infineon_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.set_state_change_callback = can_mcan_set_state_change_callback,
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif
};

static const struct can_mcan_ops can_infineon_ops = {
	.read_reg = can_infineon_read_reg,
	.write_reg = can_infineon_write_reg,
	.read_mram = can_infineon_read_mram,
	.write_mram = can_infineon_write_mram,
	.clear_mram = can_infineon_clear_mram,
};

#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define CAN_PERI_CLOCK_INIT(n)                                                                     \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                         \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 0),                 \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),                 \
			DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                             \
		.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                            \
	},
#else
#define CAN_PERI_CLOCK_INIT(n)                                                                     \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                         \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),                 \
			DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                             \
		.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                            \
	},
#endif

#define CAN_INFINEON_MCAN_INIT(n)                                                                  \
	CAN_MCAN_DT_INST_BUILD_ASSERT_MRAM_CFG(n);                                                 \
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_ELEMENTS_SIZE(n) <= CAN_MCAN_DT_INST_MRAM_SIZE(n),      \
		     "Insufficient Message RAM size to hold elements");                            \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(n, can_infineon_cbs_##n);                                \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, can_infineon_pm_action);                                       \
                                                                                                   \
	static void infineon_mcan_irq_config_##n(void)                                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, int0, irq),                                     \
			    DT_INST_IRQ_BY_NAME(n, int0, priority), can_mcan_line_0_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQ_BY_NAME(n, int0, irq));                                     \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, int1, irq),                                     \
			    DT_INST_IRQ_BY_NAME(n, int1, priority), can_mcan_line_1_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQ_BY_NAME(n, int1, irq));                                     \
	}                                                                                          \
                                                                                                   \
	static struct can_infineon_data can_infineon_data_##n = {CAN_PERI_CLOCK_INIT(n)};          \
                                                                                                   \
	static const struct can_infineon_config can_infineon_cfg_##n = {                           \
		.base = CAN_MCAN_DT_INST_MCAN_ADDR(n),                                             \
		.mrba = CAN_MCAN_DT_INST_MRBA(n),                                                  \
		.mram = CAN_MCAN_DT_INST_MRAM_ADDR(n),                                             \
		.config_irq = infineon_mcan_irq_config_##n,                                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.ctrl_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                      \
		.clk_dst = DT_INST_PROP(n, clk_dst),                                               \
		.clk_div = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_div),                          \
	};                                                                                         \
                                                                                                   \
	static const struct can_mcan_config can_mcan_cfg_##n = CAN_MCAN_DT_CONFIG_INST_GET(        \
		n, &can_infineon_cfg_##n, &can_infineon_ops, &can_infineon_cbs_##n);               \
                                                                                                   \
	CAN_MCAN_DATA_DEFINE(can_mcan_data_##n, &can_infineon_data_##n);                           \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(n, can_infineon_init, PM_DEVICE_DT_INST_GET(n),                  \
				  &can_mcan_data_##n, &can_mcan_cfg_##n, POST_KERNEL,              \
				  CONFIG_CAN_INIT_PRIORITY, &can_infineon_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_INFINEON_MCAN_INIT);

/*
 * Infineon CAN FD controller wrapper.
 *
 * The Infineon CAN implementation wraps the individual BOSCH M_CAN channels in a higher
 * level block.  The wrapper manages powering on the shared Message RAM and configures
 * other optional features, such as timestamp-counter and ECC.
 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT infineon_canfd_controller

static int canfd_ifx_ctrl_init(const struct device *dev)
{
	const struct canfd_ifx_ctrl_config *cfg = dev->config;
	uint32_t ctl;

	/* Power on MRAM (clear CTL.MRAM_OFF) */
	ctl = cfg->base->CTL;
	ctl &= ~CANFD_CTL_MRAM_OFF_Msk;
	cfg->base->CTL = ctl;

	/*
	 * Wait for MRAM power-up. Explicitly use Cy_SysLib_DelayUs() instead of
	 * k_busy_wait(), as it needs to be called on DS-RAM wake in idle thread.
	 */
	Cy_SysLib_DelayUs(6U);

#ifdef CONFIG_CAN_RX_TIMESTAMP
	/* Enable the shared timestamp counter if configured */
	if (cfg->timestamp_counter) {
		cfg->base->TS_CTL = CANFD_TS_CTL_ENABLED_Msk;
	}
#endif /* CONFIG_CAN_RX_TIMESTAMP */

	/* Enable ECC if configured */
	if (cfg->ecc_enabled) {
		cfg->base->ECC_CTL = CANFD_ECC_CTL_ECC_EN_Msk;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int canfd_ifx_ctrl_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct canfd_ifx_ctrl_config *cfg = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* Nothing to gate; the channels' suspend handlers quiesce traffic. */
		break;
	case PM_DEVICE_ACTION_RESUME:
		/*
		 * DeepSleep gates the wrapper, so the Message RAM returns powered
		 * down (CTL.MRAM_OFF) and the timestamp/ECC config is lost.  Rebuild
		 * only when actually gated; runs before the channels' RESUME.
		 */
		if ((cfg->base->CTL & CANFD_CTL_MRAM_OFF_Msk) != 0U) {
			return canfd_ifx_ctrl_init(dev);
		}
		break;
#if defined(CONFIG_PM_S2RAM) || defined(CONFIG_PM_DEVICE_POWER_DOMAIN)
	case PM_DEVICE_ACTION_TURN_ON:
		/*
		 * Power was lost: re-power the shared Message RAM and re-apply the
		 * timestamp/ECC config.  Runs before the channels' TURN_ON so the
		 * Message RAM is live by the time a channel rebuilds.
		 */
		return canfd_ifx_ctrl_init(dev);
#endif /* CONFIG_PM_S2RAM || CONFIG_PM_DEVICE_POWER_DOMAIN */
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

#define CANFD_IFX_CTRL_INIT(n)                                                                     \
	PM_DEVICE_DT_INST_DEFINE(n, canfd_ifx_ctrl_pm_action);                                     \
                                                                                                   \
	static const struct canfd_ifx_ctrl_config canfd_ifx_ctrl_cfg_##n = {                       \
		.base = (CANFD_Type *)DT_INST_REG_ADDR_BY_NAME(n, wrapper),                        \
		.ecc_enabled = DT_INST_PROP(n, ecc_enabled),                                       \
		IF_ENABLED(CONFIG_CAN_RX_TIMESTAMP,                                                \
			   (.timestamp_counter = DT_INST_PROP(n, shared_timestamp_counter),)) };   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, canfd_ifx_ctrl_init, PM_DEVICE_DT_INST_GET(n), NULL,              \
			      &canfd_ifx_ctrl_cfg_##n, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(CANFD_IFX_CTRL_INIT)
