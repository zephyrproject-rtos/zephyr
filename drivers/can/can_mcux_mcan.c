/*
 * Copyright (c) 2021 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_mcan.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(can_mcux_mcan, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_lpc_mcan

/* Message RAM Base Address register */
#define MCUX_MCAN_MRBA	 0x200
#define MCUX_MCAN_MRBA_BA GENMASK(31, 16)

struct mcux_mcan_config {
	mm_reg_t base;
	mem_addr_t mram;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
};

static int mcux_mcan_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcux_mcan_config *mcux_config = mcan_config->custom;

	return can_mcan_sys_read_reg(mcux_config->base, reg, val);
}

static int mcux_mcan_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcux_mcan_config *mcux_config = mcan_config->custom;

	return can_mcan_sys_write_reg(mcux_config->base, reg, val);
}

static int mcux_mcan_read_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcux_mcan_config *mcux_config = mcan_config->custom;

	return can_mcan_sys_read_mram(mcux_config->mram, offset, dst, len);
}

static int mcux_mcan_write_mram(const struct device *dev, uint16_t offset, const void *src,
				size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcux_mcan_config *mcux_config = mcan_config->custom;

	return can_mcan_sys_write_mram(mcux_config->mram, offset, src, len);
}

static int mcux_mcan_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcux_mcan_config *mcux_config = mcan_config->custom;

	return can_mcan_sys_clear_mram(mcux_config->mram, offset, len);
}

static int mcux_mcan_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcux_mcan_config *mcux_config = mcan_config->custom;

	return clock_control_get_rate(mcux_config->clock_dev, mcux_config->clock_subsys,
				      rate);
}

static int mcux_mcan_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcux_mcan_config *mcux_config = mcan_config->custom;
	const uintptr_t mrba = mcux_config->mram & MCUX_MCAN_MRBA_BA;
	int err;

	if (!device_is_ready(mcux_config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = pinctrl_apply_state(mcux_config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	err = clock_control_on(mcux_config->clock_dev, mcux_config->clock_subsys);
	if (err) {
		LOG_ERR("failed to enable clock (err %d)", err);
		return -EINVAL;
	}

	err = can_mcan_write_reg(dev, MCUX_MCAN_MRBA, (uint32_t)mrba);
	if (err != 0) {
		return -EIO;
	}

	err = can_mcan_configure_mram(dev, mrba, mcux_config->mram);
	if (err != 0) {
		return -EIO;
	}

	err = can_mcan_init(dev);
	if (err) {
		LOG_ERR("failed to initialize mcan (err %d)", err);
		return err;
	}

	mcux_config->irq_config_func(dev);

	return 0;
}

static const struct can_driver_api mcux_mcan_driver_api = {
	.get_capabilities = can_mcan_get_capabilities,
	.start = can_mcan_start,
	.stop = can_mcan_stop,
	.set_mode = can_mcan_set_mode,
	.set_timing = can_mcan_set_timing,
	.send = can_mcan_send,
	.add_rx_filter = can_mcan_add_rx_filter,
	.remove_rx_filter = can_mcan_remove_rx_filter,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_mcan_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.get_state = can_mcan_get_state,
	.set_state_change_callback = can_mcan_set_state_change_callback,
	.get_core_clock = mcux_mcan_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	/*
	 * MCUX MCAN timing limits are specified in the "Nominal bit timing and
	 * prescaler register (NBTP)" table in the SoC reference manual.
	 *
	 * Note that the values here are the "physical" timing limits, whereas
	 * the register field limits are physical values minus 1 (which is
	 * handled by the register assignments in the common MCAN driver code).
	 *
	 * Beware that at least some SoC reference manuals contain a bug
	 * regarding the minimum values for nominal phase segments. Valid
	 * register values are 1 and up.
	 */
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	/*
	 * MCUX MCAN data timing limits are specified in the "Data bit timing
	 * and prescaler register (DBTP)" table in the SoC reference manual.
	 *
	 * Note that the values here are the "physical" timing limits, whereas
	 * the register field limits are physical values minus 1 (which is
	 * handled by the register assignments in the common MCAN driver code).
	 *
	 * Beware that at least some SoC reference manuals contain a bug
	 * regarding the maximum value for data phase segment 2. Valid register
	 * values are 0 to 31.
	 */
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif /* CONFIG_CAN_FD_MODE */
};

static const struct can_mcan_ops mcux_mcan_ops = {
	.read_reg = mcux_mcan_read_reg,
	.write_reg = mcux_mcan_write_reg,
	.read_mram = mcux_mcan_read_mram,
	.write_mram = mcux_mcan_write_mram,
	.clear_mram = mcux_mcan_clear_mram,
};

#define MCUX_MCAN_INIT(n)						\
	CAN_MCAN_DT_INST_BUILD_ASSERT_MRAM_CFG(n);			\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static void mcux_mcan_irq_config_##n(const struct device *dev); \
									\
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(n, mcux_mcan_cbs_##n);	\
	CAN_MCAN_DT_INST_MRAM_DEFINE(n, mcux_mcan_mram_##n);	\
									\
	static const struct mcux_mcan_config mcux_mcan_config_##n = {	\
		.base = CAN_MCAN_DT_INST_MCAN_ADDR(n),			\
		.mram = (mem_addr_t)POINTER_TO_UINT(&mcux_mcan_mram_##n), \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys = (clock_control_subsys_t)		\
			DT_INST_CLOCKS_CELL(n, name),			\
		.irq_config_func = mcux_mcan_irq_config_##n,		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
									\
	static const struct can_mcan_config can_mcan_config_##n =	\
		CAN_MCAN_DT_CONFIG_INST_GET(n, &mcux_mcan_config_##n,	\
					    &mcux_mcan_ops,		\
					    &mcux_mcan_cbs_##n);	\
									\
	static struct can_mcan_data can_mcan_data_##n =			\
		CAN_MCAN_DATA_INITIALIZER(NULL);			\
									\
	CAN_DEVICE_DT_INST_DEFINE(n, mcux_mcan_init, NULL,		\
				  &can_mcan_data_##n,			\
				  &can_mcan_config_##n,			\
				  POST_KERNEL,				\
				  CONFIG_CAN_INIT_PRIORITY,		\
				  &mcux_mcan_driver_api);		\
									\
	static void mcux_mcan_irq_config_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, int0, irq),		\
			    DT_INST_IRQ_BY_NAME(n, int0, priority),	\
			    can_mcan_line_0_isr,			\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQ_BY_NAME(n, int0, irq));		\
									\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, int1, irq),		\
			    DT_INST_IRQ_BY_NAME(n, int1, priority),	\
			    can_mcan_line_1_isr,			\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQ_BY_NAME(n, int1, irq));		\
	}

DT_INST_FOREACH_STATUS_OKAY(MCUX_MCAN_INIT)
