/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#define DT_DRV_COMPAT realtek_rts5912_uart

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/types.h>

#include <zephyr/init.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>

#include <reg/reg_uart.h>

LOG_MODULE_REGISTER(uart_rts5912, CONFIG_UART_LOG_LEVEL);

#define IIR_MSTAT 0x00 /* modem status interrupt  */
#define IIR_NIP   0x01 /* no interrupt pending    */
#define IIR_THRE  0x02 /* transmit holding register empty interrupt */
#define IIR_RBRF  0x04 /* receiver buffer register full interrupt */
#define IIR_LS    0x06 /* receiver line status interrupt */
#define IIR_MASK  0x07 /* interrupt id bits mask  */
#define IIR_ID    0x06 /* interrupt ID mask without NIP */

#define FCR_FIFO    0x01 /* enable XMIT and RCVR FIFO */
#define FCR_RCVRCLR 0x02 /* clear RCVR FIFO */
#define FCR_XMITCLR 0x04 /* clear XMIT FIFO */

#define FCR_MODE0 0x00 /* set receiver in mode 0 */
#define FCR_MODE1 0x08 /* set receiver in mode 1 */

/* RCVR FIFO interrupt levels: trigger interrupt with this bytes in FIFO */
#define FCR_FIFO_1  0x00 /* 1 byte in RCVR FIFO */
#define FCR_FIFO_4  0x40 /* 4 bytes in RCVR FIFO */
#define FCR_FIFO_8  0x80 /* 8 bytes in RCVR FIFO */
#define FCR_FIFO_14 0xC0 /* 14 bytes in RCVR FIFO */

/* constants for line control register */
#define LCR_CS5   0x00 /* 5 bits data size */
#define LCR_CS6   0x01 /* 6 bits data size */
#define LCR_CS7   0x02 /* 7 bits data size */
#define LCR_CS8   0x03 /* 8 bits data size */
#define LCR_2_STB 0x04 /* 2 stop bits */
#define LCR_1_STB 0x00 /* 1 stop bit */
#define LCR_PEN   0x08 /* parity enable */
#define LCR_PDIS  0x00 /* parity disable */
#define LCR_EPS   0x10 /* even parity select */
#define LCR_SP    0x20 /* stick parity select */
#define LCR_SBRK  0x40 /* break control bit */
#define LCR_DLAB  0x80 /* divisor latch access enable */

#define RECEIVER_LINE_STATUS 0x06

#define IIRC(dev) (((struct uart_rts5912_dev_data *)(dev)->data)->iir_cache)

/* device config */
struct uart_rts5912_device_config {
	UART_Type *regs;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	uint32_t clk_grp;
	uint32_t clk_idx;
	uint32_t clk_src;
	uint32_t clk_div;
};

/** Device data structure */
struct uart_rts5912_dev_data {
	struct uart_config uart_config;
	struct k_spinlock lock;
	uint8_t fcr_cache;
	uint8_t iir_cache;
};

static int uart_rts5912_set_baud_rate(const struct device *dev, uint32_t baud_rate)
{
	const struct uart_rts5912_device_config *const dev_cfg = dev->config;
	struct uart_rts5912_dev_data *dev_data = dev->data;
	UART_Type *regs = dev_cfg->regs;

	struct rts5912_sccon_subsys sccon_subsys = {
		.clk_grp = dev_cfg->clk_grp,
		.clk_idx = dev_cfg->clk_idx,
		.clk_src = dev_cfg->clk_src,
		.clk_div = dev_cfg->clk_div,
	};
	uint32_t uart_clk_freq;
	uint32_t divisor;
	uint8_t lcr_cache;
	int rc;

	rc = clock_control_get_rate(dev_cfg->clk_dev, &sccon_subsys, &uart_clk_freq);
	if (rc != 0) {
		return rc;
	}

	if ((baud_rate != 0U) && (uart_clk_freq != 0U)) {
		/*
		 * calculate baud rate divisor. a variant of
		 * (uint32_t)(dev_cfg->sys_clk_freq / (16.0 * baud_rate) + 0.5)
		 */
		divisor = ((uart_clk_freq + (baud_rate << 3)) / baud_rate) >> 4;

		/* set the DLAB to access the baud rate divisor registers */
		lcr_cache = regs->LCR;
		regs->LCR = UART_LCR_DLAB_Msk | lcr_cache;
		regs->DLL = (uint8_t)(divisor & 0xff);
		regs->DLH = (uint8_t)((divisor >> 8) & 0xff);

		/* restore the DLAB to access the baud rate divisor registers */
		regs->LCR = lcr_cache;

		dev_data->uart_config.baudrate = baud_rate;
	}

	return 0;
}

static int uart_rts5912_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_rts5912_device_config *const dev_cfg = dev->config;
	struct uart_rts5912_dev_data *dev_data = dev->data;
	UART_Type *regs = dev_cfg->regs;

	int ret = -1;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if ((regs->LSR & UART_LSR_DR_Msk) != 0) {
		*c = regs->RBR;
		ret = 0;
	}

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

static void uart_rts5912_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_rts5912_device_config *const dev_cfg = dev->config;
	struct uart_rts5912_dev_data *dev_data = dev->data;
	UART_Type *regs = dev_cfg->regs;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	while ((regs->LSR & UART_LSR_THRE_Msk) == 0)
		;

	regs->THR = c;

	k_spin_unlock(&dev_data->lock, key);
}

static int uart_rts5912_err_check(const struct device *dev)
{
	const struct uart_rts5912_device_config *const dev_cfg = dev->config;
	UART_Type *regs = dev_cfg->regs;
	uint32_t lsr, iir;
	int err = 0;

	iir = regs->IIR;
	if (((iir & UART_IIR_IID_Msk) >> UART_IIR_IID_Pos) == RECEIVER_LINE_STATUS) {
		lsr = regs->LSR;
		if (lsr & UART_LSR_RFE_Msk) {
			if (lsr & UART_LSR_FE_Msk) {
				err |= UART_ERROR_FRAMING;
			}
			if (lsr & UART_LSR_PE_Msk) {
				err |= UART_ERROR_PARITY;
			}
		}
		if (lsr & UART_LSR_OE_Msk) {
			err |= UART_ERROR_OVERRUN;
		}
		if (lsr & UART_LSR_BI_Msk) {
			err |= UART_BREAK;
		}
	}

	return err;
}

static int uart_rts5912_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_rts5912_device_config *const dev_cfg = dev->config;
	struct uart_rts5912_dev_data *dev_data = dev->data;
	UART_Type *regs = dev_cfg->regs;

	int ret = 0;
	uint32_t lcr_cache;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	ARG_UNUSED(dev_data);

	dev_data->fcr_cache = 0U;
	dev_data->iir_cache = 0U;

	ret = uart_rts5912_set_baud_rate(dev, cfg->baudrate);
	if (ret != 0) {
		return ret;
	}

	/* Local structure to hold temporary values */
	struct uart_config uart_cfg;

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		uart_cfg.data_bits = LCR_CS5;
		break;
	case UART_CFG_DATA_BITS_6:
		uart_cfg.data_bits = LCR_CS6;
		break;
	case UART_CFG_DATA_BITS_7:
		uart_cfg.data_bits = LCR_CS7;
		break;
	case UART_CFG_DATA_BITS_8:
		uart_cfg.data_bits = LCR_CS8;
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		uart_cfg.stop_bits = LCR_1_STB;
		break;
	case UART_CFG_STOP_BITS_2:
		uart_cfg.stop_bits = LCR_2_STB;
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		uart_cfg.parity = LCR_PDIS;
		break;
	case UART_CFG_PARITY_EVEN:
		uart_cfg.parity = LCR_EPS;
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	dev_data->uart_config = *cfg;

	/* data bits, stop bits, parity, clear DLAB */
	regs->LCR = uart_cfg.data_bits | uart_cfg.stop_bits | uart_cfg.parity;

	/*
	 * Program FIFO: enabled, mode 0
	 * generate the interrupt at 4th byte
	 * Clear TX and RX FIFO
	 */
	dev_data->fcr_cache = FCR_FIFO | FCR_MODE0 | FCR_FIFO_4 | FCR_RCVRCLR | FCR_XMITCLR;
	regs->FCR = dev_data->fcr_cache;

	/* clear the port */
	lcr_cache = regs->LCR;
	regs->LCR = UART_LCR_DLAB_Msk | lcr_cache;
	regs->LCR = lcr_cache;

	/* disable interrupts  */
	regs->IER = 0;

out:
	k_spin_unlock(&dev_data->lock, key);
	return ret;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_rts5912_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_rts5912_dev_data *data = dev->data;

	cfg->baudrate = data->uart_config.baudrate;
	cfg->parity = data->uart_config.parity;
	cfg->stop_bits = data->uart_config.stop_bits;
	cfg->data_bits = data->uart_config.data_bits;
	cfg->flow_ctrl = data->uart_config.flow_ctrl;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static DEVICE_API(uart, rts5912_uart_driver_api) = {
	.poll_in = uart_rts5912_poll_in,
	.poll_out = uart_rts5912_poll_out,
	.err_check = uart_rts5912_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_rts5912_configure,
	.config_get = uart_rts5912_config_get,
#endif
};

static int rts5912_uart_init(const struct device *dev)
{
	const struct uart_rts5912_device_config *const dev_cfg = dev->config;
	struct uart_rts5912_dev_data *dev_data = dev->data;
	struct rts5912_sccon_subsys sccon_subsys = {
		.clk_grp = dev_cfg->clk_grp,
		.clk_idx = dev_cfg->clk_idx,
		.clk_src = dev_cfg->clk_src,
		.clk_div = dev_cfg->clk_div,
	};

	int rc;

	if (!device_is_ready(dev_cfg->clk_dev)) {
		return -ENODEV;
	}

	rc = clock_control_on(dev_cfg->clk_dev, (clock_control_subsys_t)&sccon_subsys);
	if (rc != 0) {
		return rc;
	}

	rc = uart_rts5912_configure(dev, &dev_data->uart_config);
	if (rc != 0) {
		return rc;
	}

	rc = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);

	return 0;
}

#define DEV_CONFIG_REG_INIT(n) .regs = (UART_Type *)(DT_INST_REG_ADDR(n)),

#define DEV_CONFIG_CLK_DEV_INIT(n)                                                                 \
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                          \
	.clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(uart##n), uart##n, clk_grp),                \
	.clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(uart##n), uart##n, clk_idx),

#define DEV_CONFIG_IRQ_FUNC_INIT(n)
#define UART_RTS5912_IRQ_FUNC_DECLARE(n)
#define UART_RTS5912_IRQ_FUNC_DEFINE(n)

#define DEV_DATA_FLOW_CTRL(n) DT_INST_PROP_OR(n, hw_flow_control, UART_CFG_FLOW_CTRL_NONE)

#define UART_RTS5912_DEVICE_INIT(n)                                                                \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	UART_RTS5912_IRQ_FUNC_DECLARE(n);                                                          \
                                                                                                   \
	static const struct uart_rts5912_device_config uart_rts5912_dev_cfg_##n = {                \
		DEV_CONFIG_REG_INIT(n) DEV_CONFIG_IRQ_FUNC_INIT(n) DEV_CONFIG_CLK_DEV_INIT(n)      \
			.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                 \
	};                                                                                         \
	static struct uart_rts5912_dev_data uart_rts5912_dev_data_##n = {                          \
		.uart_config.baudrate = DT_INST_PROP_OR(n, current_speed, 0),                      \
		.uart_config.parity = UART_CFG_PARITY_NONE,                                        \
		.uart_config.stop_bits = UART_CFG_STOP_BITS_1,                                     \
		.uart_config.data_bits = UART_CFG_DATA_BITS_8,                                     \
		.uart_config.flow_ctrl = DEV_DATA_FLOW_CTRL(n),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &rts5912_uart_init, NULL, &uart_rts5912_dev_data_##n,             \
			      &uart_rts5912_dev_cfg_##n, PRE_KERNEL_1,                             \
			      CONFIG_SERIAL_INIT_PRIORITY, &rts5912_uart_driver_api);              \
	UART_RTS5912_IRQ_FUNC_DEFINE(n)

DT_INST_FOREACH_STATUS_OKAY(UART_RTS5912_DEVICE_INIT)
