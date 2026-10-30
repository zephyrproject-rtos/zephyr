/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rh850_rlin3_uart

#include <errno.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

/* RLIN3 registers, 8-bit unless noted */
#define RLIN3_LWBR   0x01
#define RLIN3_LBRP01 0x02 /* 16-bit */
#define RLIN3_LMD    0x08
#define RLIN3_LBFC   0x09
#define RLIN3_LEDE   0x0d
#define RLIN3_LCUC   0x0e
#define RLIN3_LMST   0x11
#define RLIN3_LST    0x12
#define RLIN3_LUOER  0x20
#define RLIN3_LUOR1  0x21
#define RLIN3_LUTDR  0x24 /* 16-bit */
#define RLIN3_LURDR  0x26 /* 16-bit */

#define LMD_UART   0x01
#define LCUC_OM0   BIT(0)
#define LMST_OMM0  BIT(0)
#define LST_UTS    BIT(4)
#define LUOER_UTOE BIT(0)
#define LUOER_UROE BIT(1)

/* LWBR.NSPB = 0: 16 samples per bit, prescaler 1/1 */
#define RLIN3_SAMPLES 16U

#define EIC_EIRF BIT(12)

struct uart_rh850_rlin3_config {
	mem_addr_t base;
	/* EIRF of the UR1 interrupt is set when a byte arrives in LURDR */
	mem_addr_t rx_eic;
	uint32_t clock_freq;
	uint32_t baudrate;
};

struct uart_rh850_rlin3_data {
	struct k_spinlock lock;
};

static int uart_rh850_rlin3_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_rh850_rlin3_config *cfg = dev->config;
	struct uart_rh850_rlin3_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint16_t eic = sys_read16(cfg->rx_eic);
	int ret = -1;

	if ((eic & EIC_EIRF) != 0U) {
		sys_write16(eic & ~EIC_EIRF, cfg->rx_eic);
		*c = (unsigned char)sys_read16(cfg->base + RLIN3_LURDR);
		ret = 0;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static void uart_rh850_rlin3_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_rh850_rlin3_config *cfg = dev->config;
	struct uart_rh850_rlin3_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	sys_write8(c, cfg->base + RLIN3_LUTDR);
	while ((sys_read8(cfg->base + RLIN3_LST) & LST_UTS) != 0U) {
	}

	k_spin_unlock(&data->lock, key);
}

/* UART mode, 8N1, LSB first, no error detection */
static int uart_rh850_rlin3_init(const struct device *dev)
{
	const struct uart_rh850_rlin3_config *cfg = dev->config;
	uint32_t brp = DIV_ROUND_CLOSEST(cfg->clock_freq, RLIN3_SAMPLES * cfg->baudrate);

	if ((brp == 0U) || (brp > (UINT16_MAX + 1U))) {
		return -EINVAL;
	}

	sys_write8(0, cfg->base + RLIN3_LCUC);
	while ((sys_read8(cfg->base + RLIN3_LMST) & LMST_OMM0) != 0U) {
	}

	sys_write8(0, cfg->base + RLIN3_LWBR);
	sys_write16((uint16_t)(brp - 1U), cfg->base + RLIN3_LBRP01);
	sys_write8(LMD_UART, cfg->base + RLIN3_LMD);
	sys_write8(0, cfg->base + RLIN3_LBFC);
	sys_write8(0, cfg->base + RLIN3_LEDE);
	sys_write8(0, cfg->base + RLIN3_LUOR1);

	sys_write8(LCUC_OM0, cfg->base + RLIN3_LCUC);
	while ((sys_read8(cfg->base + RLIN3_LMST) & LMST_OMM0) == 0U) {
	}
	sys_write8(LUOER_UTOE | LUOER_UROE, cfg->base + RLIN3_LUOER);

	return 0;
}

static DEVICE_API(uart, uart_rh850_rlin3_driver_api) = {
	.poll_in = uart_rh850_rlin3_poll_in,
	.poll_out = uart_rh850_rlin3_poll_out,
};

#define RLIN3_RX_IRQ(n) DT_INST_IRQ_BY_NAME(n, rx, irq)

#define RLIN3_RX_EIC(n)                                                                            \
	((RLIN3_RX_IRQ(n) < 32 ? DT_REG_ADDR_BY_NAME(DT_INST_IRQ_INTC_BY_NAME(n, rx), intc1)       \
			       : DT_REG_ADDR_BY_NAME(DT_INST_IRQ_INTC_BY_NAME(n, rx), intc2)) +    \
	 2 * RLIN3_RX_IRQ(n))

#define UART_RH850_RLIN3_INIT(n)                                                                   \
	static struct uart_rh850_rlin3_data uart_rh850_rlin3_data_##n;                             \
	static const struct uart_rh850_rlin3_config uart_rh850_rlin3_config_##n = {                \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.rx_eic = RLIN3_RX_EIC(n),                                                         \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
		.baudrate = DT_INST_PROP(n, current_speed),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, uart_rh850_rlin3_init, NULL, &uart_rh850_rlin3_data_##n,          \
			      &uart_rh850_rlin3_config_##n, PRE_KERNEL_1,                          \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_rh850_rlin3_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RH850_RLIN3_INIT)
