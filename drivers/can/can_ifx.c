/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_mcan.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/clock_control/infineon_peri_clock.h>

#include <cy_sysclk.h>

LOG_MODULE_REGISTER(can_infineon, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT infineon_cat1_can

struct can_infineon_config {
	mm_reg_t base;
	mem_addr_t mrba;
	mem_addr_t mram;
	void (*config_irq)(void);
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock;
	uint32_t clock_frequency;
	struct infineon_sys_clock clk_info;
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

	if (rate == NULL) {
		return -EINVAL;
	}

	*rate = infineon_cfg->clock_frequency;

	return 0;
}

static inline int can_infineon_clock_enable(const struct device *dev)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_infineon_config *cfg = mcan_config->custom;

	return clock_control_set_rate(cfg->clock, (void *)&cfg->clk_info,
				      (void *)&cfg->clock_frequency);
}

static int can_infineon_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_infineon_config *infineon_cfg = mcan_cfg->custom;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(infineon_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("CAN pinctrl setup failed (%d)", ret);
		return ret;
	}

	ret = can_infineon_clock_enable(dev);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_configure_mram(dev, infineon_cfg->mrba, infineon_cfg->mram);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret != 0) {
		return ret;
	}

	infineon_cfg->config_irq();

	return 0;
}

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

#define CAN_CLK_INFO(n)								\
        .clk_info = {								\
                .root_clk_id  = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, root_clk_id),	\
                .divider_type = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, divider_type),	\
                .divider_inst = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, divider_inst),	\
        },

#define CAN_INFINEON_MCAN_INIT(n)					    \
	CAN_MCAN_DT_INST_BUILD_ASSERT_MRAM_CFG(n);			    \
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_ELEMENTS_SIZE(n) <=		    \
		     CAN_MCAN_DT_INST_MRAM_SIZE(n),			    \
		     "Insufficient Message RAM size to hold elements");	    \
									    \
	static void infineon_mcan_irq_config_##n(void);			    \
									    \
	PINCTRL_DT_INST_DEFINE(n);					    \
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(n, can_infineon_cbs_##n);	    \
									    \
	static const struct can_infineon_config can_infineon_cfg_##n = {	    \
		.base = CAN_MCAN_DT_INST_MCAN_ADDR(n),			    \
		.mrba = CAN_MCAN_DT_INST_MRBA(n),			    \
		.mram = CAN_MCAN_DT_INST_MRAM_ADDR(n),			    \
		.config_irq = infineon_mcan_irq_config_##n,		    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		    \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		    \
		.clock_frequency = DT_INST_PROP(n, clock_frequency),	    \
		CAN_CLK_INFO(n)						    \
	};								    \
									    \
	static const struct can_mcan_config can_mcan_cfg_##n =		    \
		CAN_MCAN_DT_CONFIG_INST_GET(n, &can_infineon_cfg_##n,	    \
					    &can_infineon_ops,		    \
					    &can_infineon_cbs_##n);	    \
									    \
	static struct can_mcan_data can_mcan_data_##n =			    \
		CAN_MCAN_DATA_INITIALIZER(NULL);			    \
									    \
	CAN_DEVICE_DT_INST_DEFINE(n, can_infineon_init, NULL,		    \
				  &can_mcan_data_##n,			    \
				  &can_mcan_cfg_##n,			    \
				  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,    \
				  &can_infineon_driver_api);		    \
									    \
	static void infineon_mcan_irq_config_##n(void)			    \
	{								    \
		enable_sys_int(DT_INST_PROP_BY_IDX(n, system_interrupts, 0),\
			DT_INST_PROP_BY_IDX(n, system_interrupts, 1), 	    \
			(void (*)(const void *))(void *)can_mcan_line_0_isr,\
			DEVICE_DT_INST_GET(n)); 	    \
									    \
		enable_sys_int(DT_INST_PROP_BY_IDX(n, system_interrupts, 2),\
			DT_INST_PROP_BY_IDX(n, system_interrupts, 3),	    \
			(void (*)(const void *))(void *)can_mcan_line_1_isr,\
			DEVICE_DT_INST_GET(n)); 	    \
	}

DT_INST_FOREACH_STATUS_OKAY(CAN_INFINEON_MCAN_INIT);
