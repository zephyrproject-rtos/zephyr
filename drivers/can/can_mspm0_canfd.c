/*
 * Copyright (c) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>

#include <zephyr/drivers/can.h>
#include "can_mcan.h"
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_REGISTER(can_mspm0_canfd, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT ti_mspm0_canfd

/* MCANSS register offsets and bitfields from ti_canfd base — verified from hw_mcan.h */

/* PWREN @ +0x6800 */
#define MSPM0_MCANSS_PWREN_OFFSET        0x6800U
#define MSPM0_MCANSS_PWREN_KEY           0x26000000U
#define MSPM0_MCANSS_PWREN_ENABLE        0x00000001U

/* RSTCTL @ +0x6804 */
#define MSPM0_MCANSS_RSTCTL_OFFSET       0x6804U
#define MSPM0_MCANSS_RSTCTL_KEY          0xB1000000U
#define MSPM0_MCANSS_RSTCTL_STKYCLR      0x00000002U
#define MSPM0_MCANSS_RSTCTL_ASSERT       0x00000001U

/* PID @ +0x7200: scheme field in bits [31:30]; non-zero when clock is stable */
#define MSPM0_MCANSS_PID_OFFSET          0x7200U
#define MSPM0_MCANSS_PID_SCHEME_SHIFT    30U

/* STAT @ +0x7208 */
#define MSPM0_MCANSS_STAT_OFFSET         0x7208U
#define MSPM0_MCANSS_STAT_MEMINIT        BIT(1)

/* EOI @ +0x7220: write line number to clear the aggregated CPU interrupt */
#define MSPM0_MCANSS_EOI_OFFSET          0x7220U
#define MSPM0_MCAN_EOI_LINE0             0x01U
#define MSPM0_MCAN_EOI_LINE1             0x02U

/* CPU_INT registers @ +0x7820 */
#define MSPM0_MCAN_IIDX_OFFSET           0x7820U
#define MSPM0_MCAN_IIDX_LINE0            0x01U
#define MSPM0_MCAN_IIDX_LINE1            0x02U

#define MSPM0_MCAN_IMASK_OFFSET          0x7828U
#define MSPM0_MCAN_INT_LINE0             BIT(0)
#define MSPM0_MCAN_INT_LINE1             BIT(1)

#define MSPM0_MCAN_ICLR_OFFSET           0x7848U

/* CLKEN @ +0x7900 */
#define MSPM0_MCANSS_CLKEN_OFFSET        0x7900U
#define MSPM0_MCANSS_CLK_ENABLE          0x00000001U

/* CLKDIV @ +0x7904: 0=÷1, 1=÷2, 2=÷4 */
#define MSPM0_MCANSS_CLKDIV_OFFSET       0x7904U

#define MSPM0_MCAN_CLOCK_TIMEOUT_US      1000U
#define MSPM0_MCAN_MEMINIT_TIMEOUT_US    1000U
#define MSPM0_MCAN_POWER_STARTUP_DELAY_US 1000U

/* CLKDIV register value: divider 1→0, 2→1, 4→2 */
#define MCAN_DT_CLK_DIV_REG(inst) \
	((DT_INST_PROP(inst, ti_divider) == 4) ? 2 : \
	 (DT_INST_PROP(inst, ti_divider) == 2) ? 1 : 0)

struct can_mspm0_canfd_config {
	mm_reg_t ti_canfd_base;
	const struct device *clock_dev;
	const struct mspm0_sys_clock *clock_subsys;
	mm_reg_t mcan_base;
	mem_addr_t mram;
	uintptr_t mrba;
	uint32_t clk_div;
	const struct pinctrl_dev_config *pinctrl;
	void (*irq_cfg_func)(void);
};

static int can_mspm0_canfd_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_mspm0_canfd_config *config = mcan_config->custom;

	return can_mcan_sys_read_reg(config->mcan_base, reg, val);
}

static int can_mspm0_canfd_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_mspm0_canfd_config *config = mcan_config->custom;

	return can_mcan_sys_write_reg(config->mcan_base, reg, val);
}

static int can_mspm0_canfd_read_mram(const struct device *dev, uint16_t offset, void *dst,
				     size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_mspm0_canfd_config *config = mcan_config->custom;

	return can_mcan_sys_read_mram(config->mram, offset, dst, len);
}

static int can_mspm0_canfd_write_mram(const struct device *dev, uint16_t offset, const void *src,
				      size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_mspm0_canfd_config *config = mcan_config->custom;

	return can_mcan_sys_write_mram(config->mram, offset, src, len);
}

static int can_mspm0_canfd_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_mspm0_canfd_config *config = mcan_config->custom;

	return can_mcan_sys_clear_mram(config->mram, offset, len);
}

static int can_mspm0_canfd_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_mspm0_canfd_config *config = mcan_config->custom;
	uint32_t clock_rate;
	int ret;

	ret = clock_control_get_rate(config->clock_dev,
				     (struct mspm0_sys_clock *)config->clock_subsys,
				     &clock_rate);
	if (ret < 0) {
		return ret;
	}

	*rate = clock_rate >> config->clk_div;

	return 0;
}

static int can_mspm0_canfd_clock_enable(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mspm0_canfd_config *config = mcan_cfg->custom;
	mm_reg_t base = config->ti_canfd_base;

	sys_write32(config->clk_div, base + MSPM0_MCANSS_CLKDIV_OFFSET);
	sys_write32(MSPM0_MCANSS_CLK_ENABLE, base + MSPM0_MCANSS_CLKEN_OFFSET);

	/* The revision ID will be invalid until the clock domain is
	 * fully stabilized.
	 */
	if (!WAIT_FOR(((sys_read32(base + MSPM0_MCANSS_PID_OFFSET) >>
			MSPM0_MCANSS_PID_SCHEME_SHIFT) != 0U),
		      MSPM0_MCAN_CLOCK_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("MSPM0 MCAN clock stabilization failed");
		return -ENODEV;
	}

	return 0;
}

static int can_mspm0_canfd_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mspm0_canfd_config *config = mcan_cfg->custom;
	mm_reg_t base = config->ti_canfd_base;
	int ret;

	LOG_DBG("Initializing %s", dev->name);

	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("MSPM0 MCAN pinctrl error (%d)", ret);
		return ret;
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev,
			       (clock_control_subsys_t)config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("failed to enable CANCLK (err %d)", ret);
		return ret;
	}

	sys_write32(MSPM0_MCANSS_RSTCTL_KEY | MSPM0_MCANSS_RSTCTL_STKYCLR |
		    MSPM0_MCANSS_RSTCTL_ASSERT,
		    base + MSPM0_MCANSS_RSTCTL_OFFSET);
	sys_write32(MSPM0_MCANSS_PWREN_KEY | MSPM0_MCANSS_PWREN_ENABLE,
		    base + MSPM0_MCANSS_PWREN_OFFSET);
	k_busy_wait(MSPM0_MCAN_POWER_STARTUP_DELAY_US);

	ret = can_mspm0_canfd_clock_enable(dev);
	if (ret != 0) {
		return ret;
	}

	/* Wait for Memory initialization to be completed. */
	if (!WAIT_FOR((sys_read32(base + MSPM0_MCANSS_STAT_OFFSET) &
		       MSPM0_MCANSS_STAT_MEMINIT) != 0U,
		      MSPM0_MCAN_MEMINIT_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("MSPM0 MCAN memory init failed");
		return -ENODEV;
	}

	ret = can_mcan_configure_mram(dev, config->mrba, config->mram);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret != 0) {
		return ret;
	}

	sys_write32(MSPM0_MCAN_INT_LINE0 | MSPM0_MCAN_INT_LINE1,
		    base + MSPM0_MCAN_ICLR_OFFSET);
	sys_write32(MSPM0_MCAN_INT_LINE0 | MSPM0_MCAN_INT_LINE1,
		    base + MSPM0_MCAN_IMASK_OFFSET);
	config->irq_cfg_func();

	return 0;
}

static void can_mspm0_canfd_isr(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mspm0_canfd_config *config = mcan_cfg->custom;
	mm_reg_t base = config->ti_canfd_base;

	/* MCANSS muxes LINE0 and LINE1 onto a single CPU IRQ via IIDX.
	 * Loop up to the number of lines so both can be drained per entry.
	 */
	for (int i = 0; i < 2; i++) {
		switch (sys_read32(base + MSPM0_MCAN_IIDX_OFFSET)) {
		case MSPM0_MCAN_IIDX_LINE0:
			can_mcan_line_0_isr(dev);
			sys_write32(MSPM0_MCAN_EOI_LINE0, base + MSPM0_MCANSS_EOI_OFFSET);
			break;
		case MSPM0_MCAN_IIDX_LINE1:
			can_mcan_line_1_isr(dev);
			sys_write32(MSPM0_MCAN_EOI_LINE1, base + MSPM0_MCANSS_EOI_OFFSET);
			break;
		default:
			return;
		}
	}
}

static DEVICE_API(can, can_mspm0_canfd_driver_api) = {
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
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.get_core_clock = can_mspm0_canfd_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.set_state_change_callback = can_mcan_set_state_change_callback,
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif /* CONFIG_CAN_FD_MODE */
};

static const struct can_mcan_ops can_mspm0_canfd_ops = {
	.read_reg = can_mspm0_canfd_read_reg,
	.write_reg = can_mspm0_canfd_write_reg,
	.read_mram = can_mspm0_canfd_read_mram,
	.write_mram = can_mspm0_canfd_write_mram,
	.clear_mram = can_mspm0_canfd_clear_mram,
};

#define CAN_MSPM0_CANFD_INIT(inst)								\
	static void can_mspm0_canfd_irq_cfg_##inst(void)					\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(inst),							\
			    DT_INST_IRQ(inst, priority),					\
			    can_mspm0_canfd_isr,						\
			    DEVICE_DT_INST_GET(inst), 0);					\
		irq_enable(DT_INST_IRQN(inst));							\
	}											\
												\
	static const struct mspm0_sys_clock can_mspm0_canfd_sys_clock_##inst =			\
		MSPM0_CLOCK_SUBSYS_FN(inst);							\
												\
	PINCTRL_DT_INST_DEFINE(inst);								\
												\
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(inst, can_mspm0_canfd_cbs_##inst);			\
												\
	CAN_MCAN_DT_INST_BUILD_ASSERT_MRAM_CFG(inst);						\
												\
	static const struct can_mspm0_canfd_config can_mspm0_canfd_cfg_##inst = {		\
		.ti_canfd_base = DT_REG_ADDR_BY_NAME(DT_DRV_INST(inst), ti_canfd),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),				\
		.clock_subsys = &can_mspm0_canfd_sys_clock_##inst,				\
		.mcan_base = CAN_MCAN_DT_INST_MCAN_ADDR(inst),					\
		.mram = CAN_MCAN_DT_INST_MRAM_ADDR(inst),					\
		.mrba = CAN_MCAN_DT_INST_MRBA(inst),						\
		.clk_div = MCAN_DT_CLK_DIV_REG(inst),						\
		.irq_cfg_func = can_mspm0_canfd_irq_cfg_##inst,					\
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
	};											\
												\
	CAN_MCAN_DATA_DEFINE(can_mcan_data_##inst, NULL);                                       \
												\
	static const struct can_mcan_config can_mcan_cfg_##inst =				\
		CAN_MCAN_DT_CONFIG_INST_GET(inst,						\
					    &can_mspm0_canfd_cfg_##inst,			\
					    &can_mspm0_canfd_ops,				\
					    &can_mspm0_canfd_cbs_##inst);			\
												\
	CAN_DEVICE_DT_INST_DEFINE(inst,								\
				  can_mspm0_canfd_init, NULL,					\
				  &can_mcan_data_##inst, &can_mcan_cfg_##inst,			\
				  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,			\
				  &can_mspm0_canfd_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_MSPM0_CANFD_INIT)
