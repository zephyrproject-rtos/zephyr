/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_scb_uart

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#define IFX_SCB_CTRL			0x000U
#define IFX_SCB_UART_CTRL		0x040U
#define IFX_SCB_UART_TX_CTRL		0x044U
#define IFX_SCB_UART_RX_CTRL		0x048U
#define IFX_SCB_UART_FLOW_CTRL		0x050U
#define IFX_SCB_TX_CTRL			0x200U
#define IFX_SCB_TX_FIFO_CTRL		0x204U
#define IFX_SCB_TX_FIFO_STATUS		0x208U
#define IFX_SCB_TX_FIFO_WR		0x240U
#define IFX_SCB_RX_CTRL			0x300U
#define IFX_SCB_RX_FIFO_CTRL		0x304U
#define IFX_SCB_RX_FIFO_STATUS		0x308U
#define IFX_SCB_RX_FIFO_RD		0x340U

#define IFX_SCB_CTRL_OVS_MSK		GENMASK(3, 0)
#define IFX_SCB_CTRL_MODE_MSK		GENMASK(25, 24)
#define IFX_SCB_CTRL_MODE_UART		2U
#define IFX_SCB_CTRL_ENABLED		BIT(31)

#define IFX_SCB_UART_STOP_BITS_1	1U
#define IFX_SCB_UART_STOP_BITS_1_5	2U
#define IFX_SCB_UART_STOP_BITS_2	3U
#define IFX_SCB_UART_PARITY_ODD		BIT(4)
#define IFX_SCB_UART_PARITY_EN		BIT(5)
#define IFX_SCB_UART_FRAME_MSK		GENMASK(5, 0)

#define IFX_SCB_DATA_WIDTH_MSK		GENMASK(4, 0)
#define IFX_SCB_DATA_WIDTH_5		4U
#define IFX_SCB_DATA_WIDTH_6		5U
#define IFX_SCB_DATA_WIDTH_7		6U
#define IFX_SCB_DATA_WIDTH_8		7U

#define IFX_SCB_FIFO_CLEAR		BIT(16)
#define IFX_SCB_FIFO_USED_MSK		GENMASK(8, 0)
#define IFX_SCB_FIFO_SIZE		128U

#define IFX_SCB_UART_OVS		16U

struct ifx_scb_uart_config {
	mem_addr_t base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	struct ifx_clk_peri clk;
	struct uart_config uart_cfg;
};

struct ifx_scb_uart_data {
	struct k_spinlock lock;
};

static ALWAYS_INLINE uint32_t ifx_scb_read(const struct device *dev,
					    uint32_t reg)
{
	const struct ifx_scb_uart_config *cfg = dev->config;

	return sys_read32(cfg->base + reg);
}

static ALWAYS_INLINE void ifx_scb_write(const struct device *dev,
					uint32_t reg, uint32_t val)
{
	const struct ifx_scb_uart_config *cfg = dev->config;

	sys_write32(val, cfg->base + reg);
}

static ALWAYS_INLINE void ifx_scb_rmw(const struct device *dev, uint32_t reg,
				      uint32_t clr, uint32_t set)
{
	uint32_t val = ifx_scb_read(dev, reg);

	val &= ~clr;
	val |= set;
	ifx_scb_write(dev, reg, val);
}

static ALWAYS_INLINE int ifx_scb_uart_set_clock(const struct device *dev,
						uint32_t baud)
{
	const struct ifx_scb_uart_config *cfg = dev->config;
	uint32_t rate = IFX_SCB_UART_OVS * baud;

	return clock_control_set_rate(cfg->clk_dev,
				      (clock_control_subsys_t)&cfg->clk, &rate);
}

static ALWAYS_INLINE int ifx_scb_uart_data_width(uint8_t data_bits,
						 uint32_t *width)
{
	switch (data_bits) {
	case UART_CFG_DATA_BITS_5:
		*width = IFX_SCB_DATA_WIDTH_5;
		return 0;
	case UART_CFG_DATA_BITS_6:
		*width = IFX_SCB_DATA_WIDTH_6;
		return 0;
	case UART_CFG_DATA_BITS_7:
		*width = IFX_SCB_DATA_WIDTH_7;
		return 0;
	case UART_CFG_DATA_BITS_8:
		*width = IFX_SCB_DATA_WIDTH_8;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static ALWAYS_INLINE int ifx_scb_uart_stop_bits(uint8_t stop_bits,
						uint32_t *stop)
{
	switch (stop_bits) {
	case UART_CFG_STOP_BITS_1:
		*stop = IFX_SCB_UART_STOP_BITS_1;
		return 0;
	case UART_CFG_STOP_BITS_1_5:
		*stop = IFX_SCB_UART_STOP_BITS_1_5;
		return 0;
	case UART_CFG_STOP_BITS_2:
		*stop = IFX_SCB_UART_STOP_BITS_2;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static ALWAYS_INLINE int ifx_scb_uart_parity(uint8_t parity, uint32_t *par)
{
	switch (parity) {
	case UART_CFG_PARITY_NONE:
		*par = 0U;
		return 0;
	case UART_CFG_PARITY_ODD:
		*par = IFX_SCB_UART_PARITY_EN | IFX_SCB_UART_PARITY_ODD;
		return 0;
	case UART_CFG_PARITY_EVEN:
		*par = IFX_SCB_UART_PARITY_EN;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int ifx_scb_uart_configure(const struct device *dev,
				  const struct uart_config *ucfg)
{
	struct ifx_scb_uart_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t parity;
	uint32_t width;
	uint32_t stop;
	uint32_t ctrl;
	int ret;

	if ((ifx_scb_uart_data_width(ucfg->data_bits, &width) != 0) ||
	    (ifx_scb_uart_stop_bits(ucfg->stop_bits, &stop) != 0) ||
	    (ifx_scb_uart_parity(ucfg->parity, &parity) != 0) ||
	    (ucfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE)) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	ifx_scb_rmw(dev, IFX_SCB_CTRL, IFX_SCB_CTRL_ENABLED, 0U);

	ret = ifx_scb_uart_set_clock(dev, ucfg->baudrate);
	if (ret != 0) {
		goto out;
	}

	ctrl = FIELD_PREP(IFX_SCB_CTRL_OVS_MSK, IFX_SCB_UART_OVS - 1U) |
	       FIELD_PREP(IFX_SCB_CTRL_MODE_MSK, IFX_SCB_CTRL_MODE_UART);
	ifx_scb_write(dev, IFX_SCB_CTRL, ctrl);
	ifx_scb_write(dev, IFX_SCB_UART_CTRL, 0U);

	ifx_scb_rmw(dev, IFX_SCB_UART_TX_CTRL, IFX_SCB_UART_FRAME_MSK,
		    stop | parity);
	ifx_scb_rmw(dev, IFX_SCB_UART_RX_CTRL, IFX_SCB_UART_FRAME_MSK,
		    stop | parity);
	ifx_scb_write(dev, IFX_SCB_UART_FLOW_CTRL, 0U);

	ifx_scb_write(dev, IFX_SCB_TX_CTRL,
		      FIELD_PREP(IFX_SCB_DATA_WIDTH_MSK, width));
	ifx_scb_write(dev, IFX_SCB_RX_CTRL,
		      FIELD_PREP(IFX_SCB_DATA_WIDTH_MSK, width));

	ifx_scb_write(dev, IFX_SCB_TX_FIFO_CTRL, IFX_SCB_FIFO_CLEAR);
	ifx_scb_write(dev, IFX_SCB_TX_FIFO_CTRL, 0U);
	ifx_scb_write(dev, IFX_SCB_RX_FIFO_CTRL, IFX_SCB_FIFO_CLEAR);
	ifx_scb_write(dev, IFX_SCB_RX_FIFO_CTRL, 0U);

	ifx_scb_rmw(dev, IFX_SCB_CTRL, 0U, IFX_SCB_CTRL_ENABLED);

out:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ifx_scb_uart_poll_in(const struct device *dev, unsigned char *c)
{
	uint32_t used = ifx_scb_read(dev, IFX_SCB_RX_FIFO_STATUS) &
			IFX_SCB_FIFO_USED_MSK;

	if (used == 0U) {
		return -1;
	}

	*c = (unsigned char)ifx_scb_read(dev, IFX_SCB_RX_FIFO_RD);

	return 0;
}

static void ifx_scb_uart_poll_out(const struct device *dev, unsigned char c)
{
	struct ifx_scb_uart_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	while ((ifx_scb_read(dev, IFX_SCB_TX_FIFO_STATUS) &
		IFX_SCB_FIFO_USED_MSK) >= IFX_SCB_FIFO_SIZE) {
	}

	ifx_scb_write(dev, IFX_SCB_TX_FIFO_WR, c);

	k_spin_unlock(&data->lock, key);
}

static int ifx_scb_uart_init(const struct device *dev)
{
	const struct ifx_scb_uart_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->clk_dev)) {
		return -ENODEV;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	return ifx_scb_uart_configure(dev, &cfg->uart_cfg);
}

static DEVICE_API(uart, ifx_scb_uart_api) = {
	.poll_in = ifx_scb_uart_poll_in,
	.poll_out = ifx_scb_uart_poll_out,
};

#define IFX_SCB_UART_INIT(n)                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                             \
                                                                               \
	static const struct ifx_scb_uart_config ifx_scb_uart_config_##n = {    \
		.base = DT_INST_REG_ADDR(n),                                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                     \
		.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(n))),   \
		.clk = IFX_CLK_PERI_DT_INST_SPEC_GET(n),                       \
		.uart_cfg = {                                                  \
			.baudrate = DT_INST_PROP(n, current_speed),            \
			.parity = DT_INST_ENUM_IDX_OR(                         \
				n, parity, UART_CFG_PARITY_NONE),              \
			.stop_bits = DT_INST_ENUM_IDX_OR(                      \
				n, stop_bits, UART_CFG_STOP_BITS_1),           \
			.data_bits = DT_INST_ENUM_IDX_OR(                      \
				n, data_bits, UART_CFG_DATA_BITS_8),           \
			.flow_ctrl = DT_INST_PROP(n, hw_flow_control)          \
				     ? UART_CFG_FLOW_CTRL_RTS_CTS              \
				     : UART_CFG_FLOW_CTRL_NONE,                \
		},                                                             \
	};                                                                     \
                                                                               \
	static struct ifx_scb_uart_data ifx_scb_uart_data_##n;                 \
                                                                               \
	DEVICE_DT_INST_DEFINE(n, ifx_scb_uart_init, NULL,                      \
			      &ifx_scb_uart_data_##n,                          \
			      &ifx_scb_uart_config_##n, PRE_KERNEL_1,          \
			      CONFIG_SERIAL_INIT_PRIORITY, &ifx_scb_uart_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_SCB_UART_INIT)
