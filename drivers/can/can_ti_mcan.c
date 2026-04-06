/*
 * Copyright (c) 2026 Texas Instruments
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

#ifdef CONFIG_CLOCK_CONTROL_TISCI
#include <zephyr/drivers/clock_control/tisci_clock_control.h>
#endif

LOG_MODULE_REGISTER(can_ti_mcan, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT ti_mcan

#define DEV_CFG_(dev)  ((const struct can_mcan_config *)dev->config)
#define DEV_DATA_(dev) ((struct can_mcan_data *)dev->data)
#define DEV_CFG(dev)   ((const struct ti_mcan_config *)DEV_CFG_(dev)->custom)
#define DEV_DATA(dev)  ((struct ti_mcan_data *)DEV_DATA_(dev)->custom)

#define DEV_M_CAN(dev) DEVICE_MMIO_NAMED_GET(dev, m_can)
#define DEV_MRBA(dev)  (DEVICE_MMIO_NAMED_GET(dev, message_ram))
#define DEV_MRAM(dev)  (DEV_MRBA(dev) + DEV_CFG_(dev)->mram_offsets[CAN_MCAN_MRAM_CFG_OFFSET])

struct ti_mcan_config {
	DEVICE_MMIO_NAMED_ROM(m_can);
	DEVICE_MMIO_NAMED_ROM(message_ram);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(void);
	const struct pinctrl_dev_config *pincfg;
};

struct ti_mcan_data {
	DEVICE_MMIO_NAMED_RAM(m_can);
	DEVICE_MMIO_NAMED_RAM(message_ram);
};

static int ti_mcan_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	return can_mcan_sys_read_reg(DEV_M_CAN(dev), reg, val);
}

static int ti_mcan_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	return can_mcan_sys_write_reg(DEV_M_CAN(dev), reg, val);
}

static int ti_mcan_read_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	return can_mcan_sys_read_mram(DEV_MRAM(dev), offset, dst, len);
}

static int ti_mcan_write_mram(const struct device *dev, uint16_t offset, const void *src,
			      size_t len)
{
	return can_mcan_sys_write_mram(DEV_MRAM(dev), offset, src, len);
}

static int ti_mcan_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	return can_mcan_sys_clear_mram(DEV_MRAM(dev), offset, len);
}

static int ti_mcan_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct ti_mcan_config *config = DEV_CFG(dev);

	return clock_control_get_rate(config->clock_dev, config->clock_subsys, rate);
}

static int ti_mcan_init(const struct device *dev)
{
	const struct ti_mcan_config *config = DEV_CFG(dev);
	int err;

	DEVICE_MMIO_NAMED_MAP(dev, m_can, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, message_ram, K_MEM_CACHE_NONE);

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err != 0) {
		LOG_ERR("failed to enable clock (err %d)", err);
		return -EINVAL;
	}

	err = can_mcan_configure_mram(dev, DEV_MRBA(dev), DEV_MRAM(dev));
	if (err != 0) {
		LOG_ERR("failed to configure mcan (err %d)", err);
		return err;
	}

	err = can_mcan_init(dev);
	if (err != 0) {
		LOG_ERR("failed to initialize mcan (err %d)", err);
		return err;
	}

	config->irq_config_func();

	return 0;
}

static DEVICE_API(can, ti_mcan_driver_api) = {
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
	.get_core_clock = ti_mcan_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif /* CONFIG_CAN_FD_MODE */
};

static const struct can_mcan_ops ti_mcan_ops = {
	.read_reg = ti_mcan_read_reg,
	.write_reg = ti_mcan_write_reg,
	.read_mram = ti_mcan_read_mram,
	.write_mram = ti_mcan_write_mram,
	.clear_mram = ti_mcan_clear_mram,
};

#define TI_MCAN_DEFINE_CLK_SUBSYS(n)                                                               \
	COND_CODE_1(CONFIG_CLOCK_CONTROL_TISCI, (                                                  \
		static struct tisci_clock_config tisci_fclk_##n =                                  \
			TISCI_GET_CLOCK_DETAILS_BY_INST(n);                                        \
		static const clock_control_subsys_t ti_mcan_clk_subsys_##n =                       \
			&tisci_fclk_##n;                                                           \
	), (COND_CODE_1(CONFIG_CLOCK_CONTROL_ARM_SCMI,                                             \
		(static const clock_control_subsys_t ti_mcan_clk_subsys_##n =                      \
			(clock_control_subsys_t)DT_INST_PHA(n, clocks, name);                      \
	), (BUILD_ASSERT(0, "Unsupported clock controller");))))

#define TI_MCAN_INIT(n)                                                                            \
	TI_MCAN_DEFINE_CLK_SUBSYS(n);                                                              \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	CAN_MCAN_DT_INST_BUILD_ASSERT_MRAM_CFG(n);                                                 \
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(n, ti_mcan_cbs_##n);                                     \
                                                                                                   \
	static void ti_mcan_irq_config_func_##n(void)                                              \
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
	static const struct ti_mcan_config ti_mcan_config_##n = {                                  \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(m_can, DT_DRV_INST(n)),                         \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(message_ram, DT_DRV_INST(n)),                   \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = ti_mcan_clk_subsys_##n,                                            \
		.irq_config_func = ti_mcan_irq_config_func_##n,                                    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
	};                                                                                         \
                                                                                                   \
	static const struct can_mcan_config can_mcan_config_##n = CAN_MCAN_DT_CONFIG_INST_GET(     \
		n, &ti_mcan_config_##n, &ti_mcan_ops, &ti_mcan_cbs_##n);                           \
                                                                                                   \
	static struct ti_mcan_data ti_mcan_data_##n;                                               \
                                                                                                   \
	static struct can_mcan_data can_mcan_data_##n =                                            \
		CAN_MCAN_DATA_INITIALIZER(&ti_mcan_data_##n);                                      \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(n, ti_mcan_init, NULL, &can_mcan_data_##n,                       \
				  &can_mcan_config_##n, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,     \
				  &ti_mcan_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TI_MCAN_INIT)
