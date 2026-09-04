/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_bl61x_can

#include "can_sja1000.h"

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include <zephyr/dt-bindings/clock/bflb_clock_common.h>

#include <soc.h>
#include <bflb_soc.h>
#include <glb_reg.h>

LOG_MODULE_REGISTER(can_bflb_bl61x, CONFIG_CAN_LOG_LEVEL);

/*
 * The BL61x ISO11898 controller is an NXP SJA1000 compatible PeliCAN core. Its
 * 8-bit registers are mapped on a 32-bit word stride and it is clocked from the
 * shared UART peripheral clock domain. CAN TX/RX are routed to GPIO pads through
 * the UART signal multiplexer (handled by pinctrl), and the AHB clock is gated
 * by GLB_CGEN_CFG1.
 */

/*
 * ISO11898 AHB clock gate bit position within GLB_CGEN_CFG1. Matches the
 * vendor SDK PERIPHERAL_CLOCK_CAN_ENABLE() macro (BL616/BL628), which sets
 * bit 26 of GLB_CGEN_CFG1; no named symbol exists in the HAL register headers.
 */
#define CAN_BFLB_CGEN1_CAN_POS       26U
/** SJA1000 8-bit registers are mapped on a 32-bit word stride. */
#define CAN_BFLB_REG_STRIDE          sizeof(uint32_t)
/** Mask selecting a single 8-bit SJA1000 register value. */
#define CAN_BFLB_REG_MASK            0xFFU
/** Minimum CAN bitrate supported by the controller, in bits per second. */
#define CAN_BFLB_MIN_BITRATE         25000U
/** Fixed prescaler applied internally by the SJA1000 bit timing logic. */
#define CAN_BFLB_SJA1000_CLK_DIVIDER 2U

struct can_bflb_config {
	mem_addr_t base;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(const struct device *dev);
};

static uint8_t can_bflb_read_reg(const struct device *dev, uint8_t reg)
{
	const struct can_sja1000_config *sja1000_config = dev->config;
	const struct can_bflb_config *bflb_config = sja1000_config->custom;
	mem_addr_t addr = bflb_config->base + (reg * CAN_BFLB_REG_STRIDE);

	return (uint8_t)(sys_read32(addr) & CAN_BFLB_REG_MASK);
}

static void can_bflb_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct can_sja1000_config *sja1000_config = dev->config;
	const struct can_bflb_config *bflb_config = sja1000_config->custom;
	mem_addr_t addr = bflb_config->base + (reg * CAN_BFLB_REG_STRIDE);

	sys_write32((uint32_t)val & CAN_BFLB_REG_MASK, addr);
}

static int can_bflb_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct device *clock_dev = DEVICE_DT_GET_ANY(bflb_clock_controller);
	uint32_t uart_div;
	uint32_t bclk;
	int err;

	ARG_UNUSED(dev);

	if (!device_is_ready(clock_dev)) {
		return -ENODEV;
	}

	/*
	 * The controller is fed by the UART peripheral clock, which the clock
	 * controller sources from BCLK at boot; this reads the live UART divider
	 * but assumes that BCLK source selection (it does not inspect the HBN
	 * UART clock select). As with the NXP SJA1000, the bit timing logic
	 * divides this input clock by 2 internally, so the effective core clock
	 * used by the timing calculation is halved.
	 */
	uart_div = sys_read32(GLB_BASE + GLB_UART_CFG0_OFFSET);
	uart_div = (uart_div & GLB_UART_CLK_DIV_MSK) >> GLB_UART_CLK_DIV_POS;

	err = clock_control_get_rate(clock_dev, (void *)BFLB_CLKID_CLK_BCLK, &bclk);
	if (err < 0) {
		return err;
	}

	*rate = (bclk / (uart_div + 1U)) / CAN_BFLB_SJA1000_CLK_DIVIDER;

	return 0;
}

static int can_bflb_init(const struct device *dev)
{
	const struct can_sja1000_config *sja1000_config = dev->config;
	const struct can_bflb_config *bflb_config = sja1000_config->custom;
	uint32_t regval;
	int err;

	err = pinctrl_apply_state(bflb_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		LOG_ERR("failed to configure CAN pins (err %d)", err);
		return err;
	}

	/* Enable the shared UART peripheral clock feeding the controller */
	regval = sys_read32(GLB_BASE + GLB_UART_CFG0_OFFSET);
	regval |= GLB_UART_CLK_EN_MSK;
	sys_write32(regval, GLB_BASE + GLB_UART_CFG0_OFFSET);

	/* Ungate the ISO11898 AHB clock */
	regval = sys_read32(GLB_BASE + GLB_CGEN_CFG1_OFFSET);
	regval |= BIT(CAN_BFLB_CGEN1_CAN_POS);
	sys_write32(regval, GLB_BASE + GLB_CGEN_CFG1_OFFSET);

	err = can_sja1000_init(dev);
	if (err != 0) {
		LOG_ERR("failed to initialize controller (err %d)", err);
		return err;
	}

	bflb_config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(can, can_bflb_driver_api) = {
	.get_capabilities = can_sja1000_get_capabilities,
	.start = can_sja1000_start,
	.stop = can_sja1000_stop,
	.set_mode = can_sja1000_set_mode,
	.set_timing = can_sja1000_set_timing,
	.send = can_sja1000_send,
	.add_rx_filter = can_sja1000_add_rx_filter,
	.remove_rx_filter = can_sja1000_remove_rx_filter,
	.get_state = can_sja1000_get_state,
	.set_state_change_callback = can_sja1000_set_state_change_callback,
	.get_core_clock = can_bflb_get_core_clock,
	.get_max_filters = can_sja1000_get_max_filters,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_sja1000_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.timing_min = CAN_SJA1000_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_SJA1000_TIMING_MAX_INITIALIZER,
};

#define CAN_BFLB_INIT(inst)                                                                        \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static void can_bflb_irq_config_##inst(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), can_sja1000_isr,      \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
                                                                                                   \
	static const struct can_bflb_config can_bflb_config_##inst = {                             \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.irq_config_func = can_bflb_irq_config_##inst,                                     \
	};                                                                                         \
                                                                                                   \
	static const struct can_sja1000_config can_sja1000_config_##inst =                         \
		CAN_SJA1000_DT_CONFIG_INST_GET(inst, &can_bflb_config_##inst, can_bflb_read_reg,   \
					       can_bflb_write_reg, CAN_SJA1000_OCR_OCMODE_NORMAL,  \
					       CAN_SJA1000_CDR_CLOCK_OFF, CAN_BFLB_MIN_BITRATE);   \
                                                                                                   \
	static struct can_sja1000_data can_sja1000_data_##inst =                                   \
		CAN_SJA1000_DATA_INITIALIZER(NULL);                                                \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, can_bflb_init, NULL, &can_sja1000_data_##inst,             \
				  &can_sja1000_config_##inst, POST_KERNEL,                         \
				  CONFIG_CAN_INIT_PRIORITY, &can_bflb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_BFLB_INIT)
