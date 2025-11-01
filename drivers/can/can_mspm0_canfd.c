/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_mcan.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/logging/log.h>

/* Driverlib includes */
#include <ti/driverlib/dl_mcan.h>

LOG_MODULE_REGISTER(can_mspm0_canfd, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT ti_mspm0_canfd

#define CAN_MSPM0_CLK_SRC_PLLCLK1 1

struct can_mspm0_canfd_config {
	uint32_t ti_canfd_base;
	const struct mspm0_sys_clock *clock_subsys;

	const DL_MCAN_ClockConfig clock_cfg;
	mm_reg_t mcan_base;
	mem_addr_t mram;

	void (*irq_cfg_func)(void);
	const struct pinctrl_dev_config *pinctrl;
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
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(ckm));
	uint32_t clock_rate;
	int ret;

	ret = clock_control_get_rate(clk_dev, (struct mspm0_sys_clock *)config->clock_subsys,
				     &clock_rate);
	if (ret < 0) {
		return ret;
	}

	if (config->clock_cfg.divider != DL_MCAN_FCLK_DIV_1) {
		*rate = clock_rate / (config->clock_cfg.divider >>  MCAN_CLKDIV_RATIO_OFS);
	} else {
		*rate = clock_rate;
	}

	return ret;
}

static int can_mspm0_canfd_clock_enable(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mspm0_canfd_config *config = mcan_cfg->custom;
	DL_MCAN_RevisionId revid_MCAN0;

	DL_MCAN_setClockConfig((MCAN_Regs *)config->ti_canfd_base,
			       (DL_MCAN_ClockConfig *)&config->clock_cfg);

	/* Get MCANSS Revision ID. */
	do {
		DL_MCAN_enableModuleClock((MCAN_Regs *)config->ti_canfd_base);
		DL_MCAN_getRevisionId((MCAN_Regs *)config->ti_canfd_base, &revid_MCAN0);
	} while ((uint32_t)revid_MCAN0.scheme == 0x00);

	return 0;
}

static int can_mspm0_canfd_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mspm0_canfd_config *config = mcan_cfg->custom;
	int ret = 0;

	LOG_DBG("Initializing %s", dev->name);

	/* Init GPIO */
	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("MSPM0 CAN pinctrl error (%d)", ret);
		return ret;
	}

	/* Init power */
	DL_MCAN_reset((MCAN_Regs *)config->ti_canfd_base);
	DL_MCAN_enablePower((MCAN_Regs *)config->ti_canfd_base);
	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);

	can_mspm0_canfd_clock_enable(dev);

	while (false == DL_MCAN_isMemInitDone((MCAN_Regs *)config->ti_canfd_base)) {
		/* Wait for Memory initialization to be completed. */
	}

	ret = can_mcan_configure_mram(dev, 0x8000, config->mram);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret != 0) {
		return ret;
	}

	DL_MCAN_clearInterruptStatus((MCAN_Regs *)config->ti_canfd_base,
				     (DL_MCAN_MSP_INTERRUPT_LINE0 | DL_MCAN_MSP_INTERRUPT_LINE1));
	DL_MCAN_enableInterrupt((MCAN_Regs *)config->ti_canfd_base,
				(DL_MCAN_MSP_INTERRUPT_LINE0 | DL_MCAN_MSP_INTERRUPT_LINE1));
	config->irq_cfg_func();

	return ret;
}

static void can_mspm0_canfd_isr(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mspm0_canfd_config *config = mcan_cfg->custom;

	switch (DL_MCAN_getPendingInterrupt((MCAN_Regs *)config->ti_canfd_base)) {
	case DL_MCAN_IIDX_LINE0:
		can_mcan_line_0_isr(dev);
		((MCAN_Regs *)config->ti_canfd_base)
			->MCANSS.TI_WRAPPER.PROCESSORS.MCANSS_REGS.MCANSS_EOI =
			DL_MCAN_INTR_SRC_MCAN_LINE_0;
		break;
	case DL_MCAN_IIDX_LINE1:
		can_mcan_line_1_isr(dev);
		((MCAN_Regs *)config->ti_canfd_base)
			->MCANSS.TI_WRAPPER.PROCESSORS.MCANSS_REGS.MCANSS_EOI =
			DL_MCAN_INTR_SRC_MCAN_LINE_1;
		break;
	default:
		break;
	}
}

static const struct can_driver_api can_mspm0_canfd_driver_api = {
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

#define can_mspm0_canfd_IRQ_CFG_FUNCTION(index)							\
	static void config_can_##index(void)							\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority),			\
			    can_mspm0_canfd_isr, DEVICE_DT_INST_GET(index), 0);			\
		irq_enable(DT_INST_IRQN(index));						\
	}

#define can_mspm0_canfd_CFG_INST(index)								\
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_ELEMENTS_SIZE(index) <=				\
			     CAN_MCAN_DT_INST_MRAM_SIZE(index),					\
		     "Insufficient Message RAM size");						\
												\
	PINCTRL_DT_INST_DEFINE(index);								\
	CAN_MCAN_CALLBACKS_DEFINE(can_mspm0_canfd_cbs_##index,					\
				  CAN_MCAN_DT_INST_MRAM_TX_BUFFER_ELEMENTS(index),		\
				  CONFIG_CAN_MAX_STD_ID_FILTER, CONFIG_CAN_MAX_EXT_ID_FILTER);	\
												\
	static const struct mspm0_sys_clock mspm0_can_sys_clock##index =			\
		MSPM0_CLOCK_SUBSYS_FN(index);							\
												\
	static const struct can_mspm0_canfd_config can_mspm0_canfd_cfg_##index = {		\
		.ti_canfd_base = DT_REG_ADDR_BY_NAME(DT_DRV_INST(index), ti_canfd),		\
		.clock_subsys = &mspm0_can_sys_clock##index,					\
		.clock_cfg =									\
			{									\
				.clockSel = DT_PROP_OR(DT_DRV_INST(index),			\
					ti_canclk_source, CAN_MSPM0_CLK_SRC_PLLCLK1) ==		\
					CAN_MSPM0_CLK_SRC_PLLCLK1 ? DL_MCAN_FCLK_SYSPLLCLK1	\
									: DL_MCAN_FCLK_HFCLK,	\
				.divider = DT_PROP(DT_DRV_INST(index), ti_divider),		\
			},									\
		.mcan_base = CAN_MCAN_DT_INST_MCAN_ADDR(index),					\
		.mram = CAN_MCAN_DT_INST_MRAM_ADDR(index),					\
		.irq_cfg_func = config_can_##index,						\
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(index),				\
	};											\
												\
	static const struct can_mcan_config can_mcan_cfg_##index = CAN_MCAN_DT_CONFIG_INST_GET(	\
		index, &can_mspm0_canfd_cfg_##index, &can_mspm0_canfd_ops,			\
		&can_mspm0_canfd_cbs_##index);

#define can_mspm0_canfd_DATA_INST(index)							\
	static struct can_mcan_data can_mcan_data_##index = CAN_MCAN_DATA_INITIALIZER(NULL);

#define can_mspm0_canfd_DEVICE_INST(index)							\
	CAN_DEVICE_DT_INST_DEFINE(index, can_mspm0_canfd_init, NULL, &can_mcan_data_##index,	\
				  &can_mcan_cfg_##index, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,	\
				  &can_mspm0_canfd_driver_api);

#define can_mspm0_canfd_INST(index)								\
	can_mspm0_canfd_IRQ_CFG_FUNCTION(index) can_mspm0_canfd_CFG_INST(index)			\
		can_mspm0_canfd_DATA_INST(index) can_mspm0_canfd_DEVICE_INST(index)

DT_INST_FOREACH_STATUS_OKAY(can_mspm0_canfd_INST)
