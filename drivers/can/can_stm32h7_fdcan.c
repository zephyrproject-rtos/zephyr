/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2022 Blue Clover
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_mcan.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <stm32_ll_rcc.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(can_stm32h7, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32h7_fdcan

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with a domain clock */
#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define STM32H7_FDCAN_DOMAIN_CLOCK_SUPPORT 1
#else
#define STM32H7_FDCAN_DOMAIN_CLOCK_SUPPORT 0
#endif

#define VOS0_MAX_FREQ	MHZ(125)

struct can_stm32h7_config {
	mm_reg_t base;
	mem_addr_t mrba;
	mem_addr_t mram;
	void (*config_irq)(void);
	const struct pinctrl_dev_config *pcfg;
	size_t pclk_len;
	const struct stm32_pclken *pclken;
	uint8_t clock_divider;
};

static int can_stm32h7_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32h7_config *stm32h7_cfg = mcan_cfg->custom;

	return can_mcan_sys_read_reg(stm32h7_cfg->base, reg, val);
}

static int can_stm32h7_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32h7_config *stm32h7_cfg = mcan_cfg->custom;

	return can_mcan_sys_write_reg(stm32h7_cfg->base, reg, val);
}

static int can_stm32h7_read_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32h7_config *stm32h7_cfg = mcan_cfg->custom;

	return can_mcan_sys_read_mram(stm32h7_cfg->mram, offset, dst, len);
}

static int can_stm32h7_write_mram(const struct device *dev, uint16_t offset, const void *src,
				size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32h7_config *stm32h7_cfg = mcan_cfg->custom;

	return can_mcan_sys_write_mram(stm32h7_cfg->mram, offset, src, len);
}

static int can_stm32h7_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32h7_config *stm32h7_cfg = mcan_cfg->custom;

	return can_mcan_sys_clear_mram(stm32h7_cfg->mram, offset, len);
}

static int can_stm32h7_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const uint32_t rate_tmp = LL_RCC_GetFDCANClockFreq(LL_RCC_FDCAN_CLKSOURCE);
	uint32_t cdiv;

	ARG_UNUSED(dev);

	if (rate_tmp == LL_RCC_PERIPH_FREQUENCY_NO) {
		LOG_ERR("Can't read core clock");
		return -EIO;
	}

	cdiv = FIELD_GET(FDCANCCU_CCFG_CDIV, FDCAN_CCU->CCFG);
	if (cdiv == 0U) {
		*rate = rate_tmp;
	} else {
		*rate = rate_tmp / (cdiv << 1U);
	}

	return 0;
}

static int can_stm32h7_clock_enable(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32h7_config *stm32h7_cfg = mcan_cfg->custom;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	uint32_t fdcan_clock = 0xffffffff;
	int ret;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (IS_ENABLED(STM32H7_FDCAN_DOMAIN_CLOCK_SUPPORT) && (stm32h7_cfg->pclk_len > 1)) {
		ret = clock_control_configure(clk,
				(clock_control_subsys_t)&stm32h7_cfg->pclken[1],
				NULL);
		if (ret < 0) {
			LOG_ERR("Could not select can_stm32fd domain clock");
			return ret;
		}

		/* Check if clock has correct range according to chosen regulator voltage
		 * scaling (Table 62 of RM0399 Rev 4).
		 * There is no need to test HSE case, since it's value is in range of
		 * 4 to 50 MHz (please refer to CubeMX clock control).
		 */
		ret = clock_control_get_rate(clk,
			(clock_control_subsys_t)&stm32h7_cfg->pclken[1], &fdcan_clock);
		if (ret != 0) {
			LOG_ERR("failure getting clock rate");
			return ret;
		}

		if (fdcan_clock > VOS0_MAX_FREQ) {
			LOG_ERR("FDCAN Clock source %d exceeds max allowed %d",
					fdcan_clock, VOS0_MAX_FREQ);
			return -ENODEV;
		}
	}

	ret = clock_control_on(clk, (clock_control_subsys_t)&stm32h7_cfg->pclken[0]);
	if (ret != 0) {
		LOG_ERR("failure enabling clock");
		return ret;
	}

	if (stm32h7_cfg->clock_divider != 0U) {
		can_mcan_enable_configuration_change(dev);

		FDCAN_CCU->CCFG = FDCANCCU_CCFG_BCC |
			FIELD_PREP(FDCANCCU_CCFG_CDIV, stm32h7_cfg->clock_divider >> 1U);
	}

	return 0;
}

static int can_stm32h7_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32h7_config *stm32h7_cfg = mcan_cfg->custom;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(stm32h7_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("CAN pinctrl setup failed (%d)", ret);
		return ret;
	}

	ret = can_stm32h7_clock_enable(dev);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_configure_mram(dev, stm32h7_cfg->mrba, stm32h7_cfg->mram);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret != 0) {
		return ret;
	}

	stm32h7_cfg->config_irq();

	return 0;
}

static const struct can_driver_api can_stm32h7_driver_api = {
	.get_capabilities = can_mcan_get_capabilities,
	.start = can_mcan_start,
	.stop = can_mcan_stop,
	.set_mode = can_mcan_set_mode,
	.set_timing = can_mcan_set_timing,
	.send = can_mcan_send,
	.add_rx_filter = can_mcan_add_rx_filter,
	.remove_rx_filter = can_mcan_remove_rx_filter,
	.get_state = can_mcan_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_mcan_recover,
#endif
	.get_core_clock = can_stm32h7_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.set_state_change_callback = can_mcan_set_state_change_callback,
	/* Timing limits are per the STM32H7 Reference Manual (RM0433 Rev 7),
	 * section 56.5.7, FDCAN nominal bit timing and prescaler register
	 * (FDCAN_NBTP).
	 *
	 * Beware that the reference manual contains a bug regarding the minimum
	 * values for nominal phase segments. Valid register values are 1 and up.
	 */
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	/* Data timing limits are per the STM32H7 Reference Manual
	 * (RM0433 Rev 7), section 56.5.3, FDCAN data bit timing and prescaler
	 * register (FDCAN_DBTP).
	 */
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif
};

static const struct can_mcan_ops can_stm32h7_ops = {
	.read_reg = can_stm32h7_read_reg,
	.write_reg = can_stm32h7_write_reg,
	.read_mram = can_stm32h7_read_mram,
	.write_mram = can_stm32h7_write_mram,
	.clear_mram = can_stm32h7_clear_mram,
};

#define CAN_STM32H7_MCAN_INIT(n)					    \
	CAN_MCAN_DT_INST_BUILD_ASSERT_MRAM_CFG(n);			    \
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_ELEMENTS_SIZE(n) <=		    \
		     CAN_MCAN_DT_INST_MRAM_SIZE(n),			    \
		     "Insufficient Message RAM size to hold elements");	    \
									    \
	static void stm32h7_mcan_irq_config_##n(void);			    \
									    \
	PINCTRL_DT_INST_DEFINE(n);					    \
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(n, can_stm32h7_cbs_##n);	    \
									    \
	static const struct stm32_pclken can_stm32h7_pclken_##n[] =	    \
					STM32_DT_INST_CLOCKS(n);	    \
									    \
	static const struct can_stm32h7_config can_stm32h7_cfg_##n = {	    \
		.base = CAN_MCAN_DT_INST_MCAN_ADDR(n),			    \
		.mrba = CAN_MCAN_DT_INST_MRBA(n),			    \
		.mram = CAN_MCAN_DT_INST_MRAM_ADDR(n),			    \
		.config_irq = stm32h7_mcan_irq_config_##n,		    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		    \
		.pclken = can_stm32h7_pclken_##n,			    \
		.pclk_len = DT_INST_NUM_CLOCKS(n),			    \
		.clock_divider = DT_INST_PROP_OR(n, clk_divider, 0)	    \
	};								    \
									    \
	static const struct can_mcan_config can_mcan_cfg_##n =		    \
		CAN_MCAN_DT_CONFIG_INST_GET(n, &can_stm32h7_cfg_##n,	    \
					    &can_stm32h7_ops,		    \
					    &can_stm32h7_cbs_##n);	    \
									    \
	static struct can_mcan_data can_mcan_data_##n =			    \
		CAN_MCAN_DATA_INITIALIZER(NULL);			    \
									    \
	CAN_DEVICE_DT_INST_DEFINE(n, can_stm32h7_init, NULL,		    \
				  &can_mcan_data_##n,			    \
				  &can_mcan_cfg_##n,			    \
				  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,    \
				  &can_stm32h7_driver_api);		    \
									    \
	static void stm32h7_mcan_irq_config_##n(void)			    \
	{								    \
		LOG_DBG("Enable CAN inst" #n " IRQ");			    \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, int0, irq),		    \
			DT_INST_IRQ_BY_NAME(n, int0, priority),		    \
			can_mcan_line_0_isr, DEVICE_DT_INST_GET(n), 0);	    \
		irq_enable(DT_INST_IRQ_BY_NAME(n, int0, irq));		    \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, int1, irq),		    \
			DT_INST_IRQ_BY_NAME(n, int1, priority),		    \
			can_mcan_line_1_isr, DEVICE_DT_INST_GET(n), 0);	    \
		irq_enable(DT_INST_IRQ_BY_NAME(n, int1, irq));		    \
	}

DT_INST_FOREACH_STATUS_OKAY(CAN_STM32H7_MCAN_INIT)
