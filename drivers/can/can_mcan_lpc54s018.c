/*
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
 * Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_mcan

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_mcan.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <soc.h>

LOG_MODULE_REGISTER(can_mcan_lpc54s018, CONFIG_CAN_LOG_LEVEL);

/* LPC54S018 specific MCAN definitions */
#define LPC_MCAN_MAX_FILTERS        16
#define LPC_MCAN_MAX_STD_FILTERS    LPC_MCAN_MAX_FILTERS
#define LPC_MCAN_MAX_EXT_FILTERS    8

/* MCAN register layout for LPC54S018 */
struct lpc_mcan_regs {
	uint32_t CREL;          /* Core Release Register */
	uint32_t ENDN;          /* Endian Register */
	uint32_t RESERVED1[1];
	uint32_t DBTP;          /* Data Bit Timing & Prescaler */
	uint32_t TEST;          /* Test Register */
	uint32_t RWD;           /* RAM Watchdog */
	uint32_t CCCR;          /* CC Control Register */
	uint32_t NBTP;          /* Nominal Bit Timing & Prescaler */
	uint32_t TSCC;          /* Timestamp Counter Configuration */
	uint32_t TSCV;          /* Timestamp Counter Value */
	uint32_t TOCC;          /* Timeout Counter Configuration */
	uint32_t TOCV;          /* Timeout Counter Value */
	uint32_t RESERVED2[4];
	uint32_t ECR;           /* Error Counter Register */
	uint32_t PSR;           /* Protocol Status Register */
	uint32_t TDCR;          /* Transmitter Delay Compensation */
	uint32_t RESERVED3[1];
	uint32_t IR;            /* Interrupt Register */
	uint32_t IE;            /* Interrupt Enable */
	uint32_t ILS;           /* Interrupt Line Select */
	uint32_t ILE;           /* Interrupt Line Enable */
	uint32_t RESERVED4[8];
	uint32_t GFC;           /* Global Filter Configuration */
	uint32_t SIDFC;         /* Standard ID Filter Configuration */
	uint32_t XIDFC;         /* Extended ID Filter Configuration */
	uint32_t RESERVED5[1];
	uint32_t XIDAM;         /* Extended ID AND Mask */
	uint32_t HPMS;          /* High Priority Message Status */
	uint32_t NDAT1;         /* New Data 1 */
	uint32_t NDAT2;         /* New Data 2 */
	uint32_t RXF0C;         /* Rx FIFO 0 Configuration */
	uint32_t RXF0S;         /* Rx FIFO 0 Status */
	uint32_t RXF0A;         /* Rx FIFO 0 Acknowledge */
	uint32_t RXBC;          /* Rx Buffer Configuration */
	uint32_t RXF1C;         /* Rx FIFO 1 Configuration */
	uint32_t RXF1S;         /* Rx FIFO 1 Status */
	uint32_t RXF1A;         /* Rx FIFO 1 Acknowledge */
	uint32_t RXESC;         /* Rx Buffer/FIFO Element Size Configuration */
	uint32_t TXBC;          /* Tx Buffer Configuration */
	uint32_t TXFQS;         /* Tx FIFO/Queue Status */
	uint32_t TXESC;         /* Tx Buffer Element Size Configuration */
	uint32_t TXBRP;         /* Tx Buffer Request Pending */
	uint32_t TXBAR;         /* Tx Buffer Add Request */
	uint32_t TXBCR;         /* Tx Buffer Cancellation Request */
	uint32_t TXBTO;         /* Tx Buffer Transmission Occurred */
	uint32_t TXBCF;         /* Tx Buffer Cancellation Finished */
	uint32_t TXBTIE;        /* Tx Buffer Transmission Interrupt Enable */
	uint32_t TXBCIE;        /* Tx Buffer Cancellation Finished Interrupt Enable */
	uint32_t RESERVED6[2];
	uint32_t TXEFC;         /* Tx Event FIFO Configuration */
	uint32_t TXEFS;         /* Tx Event FIFO Status */
	uint32_t TXEFA;         /* Tx Event FIFO Acknowledge */
};

/* Message RAM layout */
struct lpc_mcan_msg_ram {
	/* Standard ID filters */
	uint32_t std_filter[LPC_MCAN_MAX_STD_FILTERS];
	/* Extended ID filters */
	uint32_t ext_filter[LPC_MCAN_MAX_EXT_FILTERS * 2];
	/* RX FIFO 0 */
	uint8_t rx_fifo0[16 * 72];  /* 16 elements, max 72 bytes each */
	/* RX FIFO 1 */
	uint8_t rx_fifo1[16 * 72];  /* 16 elements, max 72 bytes each */
	/* TX Event FIFO */
	uint32_t tx_event_fifo[16 * 2];  /* 16 elements, 8 bytes each */
	/* TX buffers */
	uint8_t tx_buffer[16 * 72];  /* 16 elements, max 72 bytes each */
} __aligned(4);

struct can_mcan_lpc54s018_config {
	struct can_mcan_config mcan_cfg;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
};

struct can_mcan_lpc54s018_data {
	struct can_mcan_data mcan_data;
	struct lpc_mcan_msg_ram msg_ram;
};

static int can_mcan_lpc54s018_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_lpc54s018_config *config = dev->config;
	struct lpc_mcan_regs *regs = (struct lpc_mcan_regs *)config->mcan_cfg.base;

	/* Convert MCAN register offset to LPC register */
	*val = *(volatile uint32_t *)((uint8_t *)regs + reg);
	
	return 0;
}

static int can_mcan_lpc54s018_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_lpc54s018_config *config = dev->config;
	struct lpc_mcan_regs *regs = (struct lpc_mcan_regs *)config->mcan_cfg.base;

	/* Convert MCAN register offset to LPC register */
	*(volatile uint32_t *)((uint8_t *)regs + reg) = val;
	
	return 0;
}

static int can_mcan_lpc54s018_read_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	struct can_mcan_lpc54s018_data *data = dev->data;
	
	if (offset + len > sizeof(struct lpc_mcan_msg_ram)) {
		return -EINVAL;
	}
	
	memcpy(dst, (uint8_t *)&data->msg_ram + offset, len);
	
	return 0;
}

static int can_mcan_lpc54s018_write_mram(const struct device *dev, uint16_t offset, const void *src, size_t len)
{
	struct can_mcan_lpc54s018_data *data = dev->data;
	
	if (offset + len > sizeof(struct lpc_mcan_msg_ram)) {
		return -EINVAL;
	}
	
	memcpy((uint8_t *)&data->msg_ram + offset, src, len);
	
	return 0;
}

static int can_mcan_lpc54s018_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	struct can_mcan_lpc54s018_data *data = dev->data;
	
	if (offset + len > sizeof(struct lpc_mcan_msg_ram)) {
		return -EINVAL;
	}
	
	memset((uint8_t *)&data->msg_ram + offset, 0, len);
	
	return 0;
}

static int can_mcan_lpc54s018_init(const struct device *dev)
{
	const struct can_mcan_lpc54s018_config *config = dev->config;
	struct can_mcan_lpc54s018_data *data = dev->data;
	int ret;

	/* Configure pins */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to configure CAN pins");
		return ret;
	}

	/* Enable clock */
	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to enable CAN clock");
		return ret;
	}

	/* Configure interrupts */
	config->irq_config_func(dev);

	/* Initialize MCAN */
	ret = can_mcan_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize MCAN");
		return ret;
	}

	/* Configure message RAM */
	ret = can_mcan_configure_mram(dev, 
				      (uintptr_t)&data->msg_ram,
				      (uintptr_t)&data->msg_ram);
	if (ret < 0) {
		LOG_ERR("Failed to configure message RAM");
		return ret;
	}

	LOG_INF("LPC54S018 MCAN initialized");
	return 0;
}

static const struct can_mcan_ops can_mcan_lpc54s018_ops = {
	.read_reg = can_mcan_lpc54s018_read_reg,
	.write_reg = can_mcan_lpc54s018_write_reg,
	.read_mram = can_mcan_lpc54s018_read_mram,
	.write_mram = can_mcan_lpc54s018_write_mram,
	.clear_mram = can_mcan_lpc54s018_clear_mram,
};

static const struct can_driver_api can_mcan_lpc54s018_driver_api = {
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
#endif
	.get_core_clock = can_mcan_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.set_state_change_callback = can_mcan_set_state_change_callback,
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif
};

#define CAN_MCAN_LPC54S018_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);							\
											\
	static void can_mcan_lpc54s018_irq_config_##n(const struct device *dev);	\
											\
	static const struct can_mcan_lpc54s018_config can_mcan_lpc54s018_config_##n = {	\
		.mcan_cfg = {								\
			.ops = &can_mcan_lpc54s018_ops,					\
			.base = DT_INST_REG_ADDR(n),					\
			.max_filters = LPC_MCAN_MAX_FILTERS,				\
			.max_std_filters = LPC_MCAN_MAX_STD_FILTERS,			\
			.max_ext_filters = LPC_MCAN_MAX_EXT_FILTERS,			\
			.msg_ram_size = sizeof(struct lpc_mcan_msg_ram),		\
		},									\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
		.irq_config_func = can_mcan_lpc54s018_irq_config_##n,			\
	};										\
											\
	static struct can_mcan_lpc54s018_data can_mcan_lpc54s018_data_##n = {		\
		.mcan_data = {								\
			.std_filter_cnt = 0,						\
			.ext_filter_cnt = 0,						\
		},									\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, can_mcan_lpc54s018_init, NULL,			\
			      &can_mcan_lpc54s018_data_##n,				\
			      &can_mcan_lpc54s018_config_##n,				\
			      POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,			\
			      &can_mcan_lpc54s018_driver_api);				\
											\
	static void can_mcan_lpc54s018_irq_config_##n(const struct device *dev)	\
	{										\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq),				\
			    DT_INST_IRQ_BY_IDX(n, 0, priority),			\
			    can_mcan_line_0_isr,					\
			    DEVICE_DT_INST_GET(n), 0);					\
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));				\
											\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 1, irq),				\
			    DT_INST_IRQ_BY_IDX(n, 1, priority),			\
			    can_mcan_line_1_isr,					\
			    DEVICE_DT_INST_GET(n), 0);					\
		irq_enable(DT_INST_IRQ_BY_IDX(n, 1, irq));				\
	}

DT_INST_FOREACH_STATUS_OKAY(CAN_MCAN_LPC54S018_INIT)