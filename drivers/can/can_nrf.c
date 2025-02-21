/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_can

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_mcan.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#ifdef CONFIG_SOC_NRF54H20_GPD
#include <nrf/gpd.h>
#endif

/* nRF CAN wrapper offsets */
#define CAN_TASKS_START	  offsetof(NRF_CAN_Type, TASKS_START)
#define CAN_EVENTS_CORE_0 offsetof(NRF_CAN_Type, EVENTS_CORE[0])
#define CAN_EVENTS_CORE_1 offsetof(NRF_CAN_Type, EVENTS_CORE[1])
#define CAN_INTEN	  offsetof(NRF_CAN_Type, INTEN)

struct can_nrf_config {
	uint32_t wrapper;
	uint32_t mcan;
	uint32_t mrba;
	uint32_t mram;
	const struct device *auxpll;
	const struct device *hsfll;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_configure)(void);
	uint16_t irq;
};

static void can_nrf_irq_handler(const struct device *dev)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_nrf_config *config = mcan_config->custom;

	if (sys_read32(config->wrapper + CAN_EVENTS_CORE_0) == 1U) {
		sys_write32(0U, config->wrapper + CAN_EVENTS_CORE_0);
		can_mcan_line_0_isr(dev);
	}

	if (sys_read32(config->wrapper + CAN_EVENTS_CORE_1) == 1U) {
		sys_write32(0U, config->wrapper + CAN_EVENTS_CORE_1);
		can_mcan_line_1_isr(dev);
	}
}

static int can_nrf_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_nrf_config *config = mcan_config->custom;

	return clock_control_get_rate(config->auxpll, NULL, rate);
}

static DEVICE_API(can, can_nrf_api) = {
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
	.get_core_clock = can_nrf_get_core_clock,
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

static int can_nrf_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_nrf_config *config = mcan_config->custom;

	return can_mcan_sys_read_reg(config->mcan, reg, val);
}

static int can_nrf_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_nrf_config *config = mcan_config->custom;

	return can_mcan_sys_write_reg(config->mcan, reg, val);
}

static int can_nrf_read_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_nrf_config *config = mcan_config->custom;

	return can_mcan_sys_read_mram(config->mram, offset, dst, len);
}

static int can_nrf_write_mram(const struct device *dev, uint16_t offset, const void *src,
			      size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_nrf_config *config = mcan_config->custom;

	return can_mcan_sys_write_mram(config->mram, offset, src, len);
}

static int can_nrf_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_nrf_config *config = mcan_config->custom;

	return can_mcan_sys_clear_mram(config->mram, offset, len);
}

static const struct can_mcan_ops can_mcan_nrf_ops = {
	.read_reg = can_nrf_read_reg,
	.write_reg = can_nrf_write_reg,
	.read_mram = can_nrf_read_mram,
	.write_mram = can_nrf_write_mram,
	.clear_mram = can_nrf_clear_mram,
};

static int configure_hsfll(const struct device *dev, bool on)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_nrf_config *config = mcan_config->custom;
	struct nrf_clock_spec spec = { 0 };

	/* If CAN is on, HSFLL frequency >= AUXPLL frequency */
	if (on) {
		int ret;

		ret = clock_control_get_rate(dev, NULL, &spec.frequency);
		if (ret < 0) {
			return ret;
		}
	}

	return nrf_clock_control_request_sync(config->hsfll, &spec, K_FOREVER);
}

static int can_nrf_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_nrf_config *config = mcan_config->custom;
	int ret;

	if (!device_is_ready(config->auxpll) || !device_is_ready(config->hsfll)) {
		return -ENODEV;
	}

	ret = configure_hsfll(dev, true);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_on(config->auxpll, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}


	sys_write32(0U, config->wrapper + CAN_EVENTS_CORE_0);
	sys_write32(0U, config->wrapper + CAN_EVENTS_CORE_1);
	sys_write32(CAN_INTEN_CORE0_Msk | CAN_INTEN_CORE1_Msk, config->wrapper + CAN_INTEN);
	sys_write32(1U, config->wrapper + CAN_TASKS_START);

#ifdef CONFIG_SOC_NRF54H20_GPD
	ret = nrf_gpd_retain_pins_set(config->pcfg, false);
	if (ret < 0) {
		return ret;
	}
#endif

	config->irq_configure();

	ret = can_mcan_configure_mram(dev, config->mrba, config->mram);
	if (ret < 0) {
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#define CAN_NRF_DEFINE(n)                                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static inline void can_nrf_irq_configure##n(void)                                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), can_nrf_irq_handler,        \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct can_nrf_config can_nrf_config##n = {                                   \
		.wrapper = DT_INST_REG_ADDR_BY_NAME(n, wrapper),                                   \
		.mcan = CAN_MCAN_DT_INST_MCAN_ADDR(n),                                             \
		.mrba = CAN_MCAN_DT_INST_MRBA(n),                                                  \
		.mram = CAN_MCAN_DT_INST_MRAM_ADDR(n),                                             \
		.auxpll = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, auxpll)),                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.hsfll = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, hsfll)),                     \
		.irq = DT_INST_IRQN(n),                                                            \
		.irq_configure = can_nrf_irq_configure##n,                                         \
	};                                                                                         \
                                                                                                   \
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(n, can_mcan_nrf_cbs##n);                                 \
                                                                                                   \
	static const struct can_mcan_config can_mcan_nrf_config##n = CAN_MCAN_DT_CONFIG_INST_GET(  \
		n, &can_nrf_config##n, &can_mcan_nrf_ops, &can_mcan_nrf_cbs##n);                   \
                                                                                                   \
	static struct can_mcan_data can_mcan_nrf_data##n = CAN_MCAN_DATA_INITIALIZER(NULL);        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, can_nrf_init, NULL, &can_mcan_nrf_data##n,                        \
			      &can_mcan_nrf_config##n, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,      \
			      &can_nrf_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_NRF_DEFINE)
