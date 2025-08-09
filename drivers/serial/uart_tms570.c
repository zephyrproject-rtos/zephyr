/*
 * Copyright (c) 2025 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/arch/arm/cortex_a_r/sys_io.h>

#define DT_DRV_COMPAT ti_tms570_uart

#define VCLK_FREQUENCY DT_PROP(DT_NODELABEL(clk_vclk), clock_frequency)

#define TMS570_GCR0		(0x00)
#define TMS570_GCR1		(0x04)
#define TMS570_GCR2		(0x08)
#define TMS570_SETINT		(0x0c)
#define TMS570_CLEARINT		(0x10)
#define TMS570_SETINTLVL	(0x14)
#define TMS570_CLEARINTLVL	(0x18)
#define TMS570_FLR		(0x1c)
#define TMS570_INTVECT0		(0x20)
#define TMS570_INTVECT1		(0x24)
#define TMS570_FORMAT		(0x28)
#define TMS570_BRS		(0x2c)
#define TMS570_ED		(0x30)
#define TMS570_RD		(0x34)
#define TMS570_TD		(0x38)
#define TMS570_PIO0		(0x3c)
#define TMS570_PIO1		(0x40)
#define TMS570_PIO2		(0x44)
#define TMS570_PIO3		(0x48)
#define TMS570_PIO4		(0x4c)
#define TMS570_PIO5		(0x50)
#define TMS570_PIO6		(0x54)
#define TMS570_PIO7		(0x58)
#define TMS570_PIO8		(0x5c)
#define TMS570_IODFTCTRL	(0x90)

#define TMS570_SCI_REG_GCR0		(dev_cfg->base_addr + TMS570_GCR0)
#define TMS570_SCI_REG_GCR1		(dev_cfg->base_addr + TMS570_GCR1)
#define TMS570_SCI_REG_CLEARINT		(dev_cfg->base_addr + TMS570_CLEARINT)
#define TMS570_SCI_REG_CLEARINTLVL	(dev_cfg->base_addr + TMS570_CLEARINTLVL)
#define TMS570_SCI_REG_FORMAT		(dev_cfg->base_addr + TMS570_FORMAT)
#define TMS570_SCI_REG_BRS		(dev_cfg->base_addr + TMS570_BRS)
#define TMS570_SCI_REG_FUNC		(dev_cfg->base_addr + TMS570_PIO0)
#define TMS570_SCI_REG_PIO8		(dev_cfg->base_addr + TMS570_PIO8)
#define TMS570_SCI_REG_RD		(dev_cfg->base_addr + TMS570_RD)
#define TMS570_SCI_REG_TD		(dev_cfg->base_addr + TMS570_TD)
#define TMS570_SCI_REG_FLR		(dev_cfg->base_addr + TMS570_FLR)

#define GCR1_TXENA		(1 << 25)
#define GCR1_RXENA		(1 << 24)
#define GCR1_CONT		(1 << 17)
#define GCR1_LOOPBACK		(1 << 16)
#define GCR1_STOP_EXT_FRAME	(1 << 13)
#define GCR1_HGEN_CTRL		(1 << 12)
#define GCR1_CTYPE		(1 << 11)
#define GCR1_MBUF_MODE		(1 << 10)
#define GCR1_ADAPT		(1 << 9)
#define GCR1_SLEEP		(1 << 8)
#define GCR1_SWnRST		(1 << 7)
#define GCR1_LIN_MODE		(1 << 6)
#define GCR1_CLOCK		(1 << 5)
#define GCR1_STOP_BIT_1		(0 << 4)
#define GCR1_STOP_BIT_2		(1 << 4)
#define GCR1_PARITY_ENA		(1 << 2)
#define GCR1_PARITY_ODD		((0 << 3) | GCR1_PARITY_ENA)
#define GCR1_PARITY_EVEN	((1 << 3) | GCR1_PARITY_ENA)
#define GCR1_PARITY_NONE	(0)
#define GCR1_TIMING_MODE_SYNC	(0 << 1)
#define GCR1_TIMING_MODE_ASYNC	(1 << 1)
#define GCR1_COMM_MODE		(1 << 0)

#define FLR_RX_RDY (1 << 9)
#define FLR_TX_RDY (1 << 8)

#define FORMAT_CHARS_IN_FRAME(x)	(((x - 1) & 0x7) << 16)
#define FORMAT_BITS_PER_CHAR(x)		(((x - 1) & 0x7))
#define FORMAT_8_BIT_1_CHAR		(FORMAT_CHARS_IN_FRAME(1) | FORMAT_BITS_PER_CHAR(8))

#define PIO_TX_EN (1 << 2)
#define PIO_RX_EN (1 << 1)

/* Device data structure */
struct uart_tms570_dev_cfg_t {
	const uint32_t base_addr;	/* Register base address */
	const uint32_t baud_rate;	/* Baud rate */
	const struct pinctrl_dev_config *pincfg;
};


static void uart_tms570_poll_out(const struct device *dev, uint8_t c)
{
	const struct uart_tms570_dev_cfg_t *dev_cfg =
		(const struct uart_tms570_dev_cfg_t *) dev->config;

	while ((sys_read32(TMS570_SCI_REG_FLR) & 0x00000100U) == 0U) {
		/* wait */
	}

	sys_write32(c, TMS570_SCI_REG_TD);
}

static int uart_tms570_poll_in(const struct device *dev, uint8_t *c)
{
	const struct uart_tms570_dev_cfg_t *dev_cfg =
		(const struct uart_tms570_dev_cfg_t *) dev->config;
	uint32_t flags;

	flags = sys_read32(TMS570_SCI_REG_FLR);
	if ((flags & FLR_RX_RDY) != 0) {
		*c = (uint8_t)sys_read32(TMS570_SCI_REG_RD);
		return 0;
	} else {
		return -1;
	}
}

static int uart_tms570_init(const struct device *dev)
{
	const struct uart_tms570_dev_cfg_t *dev_cfg =
		(const struct uart_tms570_dev_cfg_t *) dev->config;

	/* reset SCI */
	sys_write32(0, TMS570_SCI_REG_GCR0);
	sys_write32(1, TMS570_SCI_REG_GCR0);

	/* enable and set up uart  */
	sys_write32(GCR1_TXENA | GCR1_RXENA | /* enable both tx and rx */
		    GCR1_CLOCK |              /* internal clock (device has no clock pin) */
		    GCR1_STOP_BIT_1 |
		    GCR1_PARITY_NONE |
		    GCR1_TIMING_MODE_ASYNC,
		    TMS570_SCI_REG_GCR1);

	/* make pins SCI mode */
	sys_write32(PIO_TX_EN | PIO_RX_EN, TMS570_SCI_REG_FUNC);
	pinctrl_apply_state(dev_cfg->pincfg, PINCTRL_STATE_DEFAULT);

	/* baudrate */
	sys_write32(VCLK_FREQUENCY / ((dev_cfg->baud_rate - 1) * 16), TMS570_SCI_REG_BRS);

	/* we want 8 bit per char and 1 char per frame  */
	sys_write32(FORMAT_8_BIT_1_CHAR, TMS570_SCI_REG_FORMAT);

	/* start */
	sys_write32(sys_read32(TMS570_SCI_REG_GCR1) | GCR1_SWnRST, TMS570_SCI_REG_GCR1);

	return 0;
}

static const struct uart_driver_api uart_tms570_driver_api = {
	.poll_in = uart_tms570_poll_in,
	.poll_out = uart_tms570_poll_out,
	.err_check = NULL,
};

#define TMS570_UART_INIT(idx)									\
	PINCTRL_DT_INST_DEFINE(idx);								\
	static struct uart_tms570_dev_cfg_t tms570__uart##idx##_cfg = {				\
		.base_addr = DT_INST_REG_ADDR(idx),						\
		.baud_rate = DT_INST_PROP(idx, current_speed),					\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),					\
	};											\
	DEVICE_DT_INST_DEFINE(idx, &uart_tms570_init, NULL, NULL,				\
			      &tms570__uart##idx##_cfg, PRE_KERNEL_1,				\
			      CONFIG_SERIAL_INIT_PRIORITY,					\
			      &uart_tms570_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMS570_UART_INIT)
