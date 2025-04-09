/*
 * Copyright (c) 2025 Texas Instruments
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

LOG_MODULE_REGISTER(can_mcan_platform, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT bosch_m_can

struct mcan_platform_config {
	mm_reg_t base;
	mm_reg_t mrba;
	mem_addr_t mram;
	uint32_t clock_frequency;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
};

static int mcan_platform_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcan_platform_config *config = mcan_config->custom;

	return can_mcan_sys_read_reg(config->base, reg, val);
}

static int mcan_platform_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcan_platform_config *config = mcan_config->custom;

	return can_mcan_sys_write_reg(config->base, reg, val);
}

static int mcan_platform_read_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcan_platform_config *config = mcan_config->custom;

	return can_mcan_sys_read_mram(config->mram, offset, dst, len);
}

static int mcan_platform_write_mram(const struct device *dev, uint16_t offset, const void *src,
				    size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcan_platform_config *config = mcan_config->custom;

	return can_mcan_sys_write_mram(config->mram, offset, src, len);
}

static int mcan_platform_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcan_platform_config *config = mcan_config->custom;

	return can_mcan_sys_clear_mram(config->mram, offset, len);
}

static int mcan_platform_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcan_platform_config *config = mcan_config->custom;

	if (config->clock_dev != NULL) {
		return clock_control_get_rate(config->clock_dev, config->clock_subsys, rate);
	}

	if (config->clock_frequency != 0U) {
		*rate = config->clock_frequency;
		return 0;
	}

	return -ENOSYS;
}

static int mcan_platform_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct mcan_platform_config *config = mcan_config->custom;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	if (config->clock_dev != NULL) {
		if (!device_is_ready(config->clock_dev)) {
			LOG_ERR("clock control device not ready");
			return -ENODEV;
		}

		err = clock_control_on(config->clock_dev, config->clock_subsys);
		if (err != 0) {
			LOG_ERR("failed to enable clock (err %d)", err);
			return -EINVAL;
		}
	}

	err = can_mcan_configure_mram(dev, config->mrba, config->mram);
	if (err != 0) {
		LOG_ERR("failed to configure mcan (err %d)", err);
		return err;
	}

	err = can_mcan_init(dev);
	if (err != 0) {
		LOG_ERR("failed to initialize mcan (err %d)", err);
		return err;
	}

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(can, mcan_platform_driver_api) = {
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
	.get_core_clock = mcan_platform_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif /* CONFIG_CAN_FD_MODE */
};

static const struct can_mcan_ops mcan_platform_ops = {
	.read_reg = mcan_platform_read_reg,
	.write_reg = mcan_platform_write_reg,
	.read_mram = mcan_platform_read_mram,
	.write_mram = mcan_platform_write_mram,
	.clear_mram = mcan_platform_clear_mram,
};

#define MCAN_PLATFORM_CLK_CONFIG(n)                                                                \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks), (                                            \
			.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                        \
			.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, cclk),      \
			.clock_frequency = 0                                                       \
		), (                                                                               \
			.clock_dev = NULL,                                                         \
			.clock_subsys = NULL,                                                      \
			.clock_frequency = DT_INST_PROP_OR(n, clock_frequency, 0)                  \
		)                                                                                  \
	)

#define MCAN_PLATFORM_INIT(n)                                                                      \
	static void mcan_platform_irq_config_##n(const struct device *dev)                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, int0, irq),                                     \
			    DT_INST_IRQ_BY_NAME(n, int0, priority), can_mcan_line_0_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQ_BY_NAME(n, int0, irq));                                     \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, int1, irq),                                     \
			    DT_INST_IRQ_BY_NAME(n, int1, priority), can_mcan_line_1_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQ_BY_NAME(n, int1, irq));                                     \
	}                                                                                          \
                                                                                                   \
	CAN_MCAN_DT_INST_BUILD_ASSERT_MRAM_CFG(n);                                                 \
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(n, mcan_platform_cbs_##n);                               \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct mcan_platform_config mcan_platform_config_##n = {                      \
		.base = CAN_MCAN_DT_INST_MCAN_ADDR(n),                                             \
		.mrba = CAN_MCAN_DT_INST_MRBA(n),                                                  \
		.mram = CAN_MCAN_DT_INST_MRAM_ADDR(n),                                             \
		.irq_config_func = mcan_platform_irq_config_##n,                                   \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		MCAN_PLATFORM_CLK_CONFIG(n),                                                       \
	};                                                                                         \
                                                                                                   \
	static const struct can_mcan_config can_mcan_config_##n = CAN_MCAN_DT_CONFIG_INST_GET(     \
		n, &mcan_platform_config_##n, &mcan_platform_ops, &mcan_platform_cbs_##n);         \
                                                                                                   \
	static struct can_mcan_data can_mcan_data_##n = CAN_MCAN_DATA_INITIALIZER(NULL);           \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(n, mcan_platform_init, NULL, &can_mcan_data_##n,                 \
				  &can_mcan_config_##n, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,     \
				  &mcan_platform_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCAN_PLATFORM_INIT)
