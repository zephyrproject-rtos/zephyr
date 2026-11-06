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
#include <zephyr/irq.h>
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
#define IFX_SCB_INTR_CAUSE		0xE00U
#define IFX_SCB_INTR_TX			0xF80U
#define IFX_SCB_INTR_TX_MASK		0xF88U
#define IFX_SCB_INTR_RX			0xFC0U
#define IFX_SCB_INTR_RX_MASK		0xFC8U

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
#define IFX_SCB_FIFO_TRIGGER_MSK	GENMASK(7, 0)
#define IFX_SCB_FIFO_SR_VALID		BIT(15)
#define IFX_SCB_FIFO_SIZE		128U
#define IFX_SCB_TX_TRIGGER_LEVEL	(IFX_SCB_FIFO_SIZE / 2U)

#define IFX_SCB_INTR_CAUSE_TX		BIT(2)
#define IFX_SCB_INTR_CAUSE_RX		BIT(3)

#define IFX_SCB_INTR_TX_TRIGGER		BIT(0)
#define IFX_SCB_INTR_RX_NOT_EMPTY	BIT(2)
#define IFX_SCB_INTR_RX_OVERFLOW	BIT(5)
#define IFX_SCB_INTR_RX_FRAME_ERROR	BIT(8)
#define IFX_SCB_INTR_RX_PARITY_ERROR	BIT(9)
#define IFX_SCB_INTR_RX_ERRORS                                                 \
	(IFX_SCB_INTR_RX_OVERFLOW | IFX_SCB_INTR_RX_FRAME_ERROR |              \
	 IFX_SCB_INTR_RX_PARITY_ERROR)

#define IFX_SCB_UART_OVS		16U

struct ifx_scb_uart_config {
	mem_addr_t base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	struct ifx_clk_peri clk;
	struct uart_config uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config)(void);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

struct ifx_scb_uart_data {
	struct k_spinlock lock;
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct uart_config uart_cfg;
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;
	void *cb_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
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

static ALWAYS_INLINE void ifx_scb_clear_intr(const struct device *dev,
					     uint32_t reg, uint32_t intr)
{
	ifx_scb_write(dev, reg, intr);
	(void)ifx_scb_read(dev, reg);
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
	uint32_t tx_fifo;
	int ret;

	if ((ifx_scb_uart_data_width(ucfg->data_bits, &width) != 0) ||
	    (ifx_scb_uart_stop_bits(ucfg->stop_bits, &stop) != 0) ||
	    (ifx_scb_uart_parity(ucfg->parity, &parity) != 0) ||
	    (ucfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE)) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	ctrl = ifx_scb_read(dev, IFX_SCB_CTRL);
	ifx_scb_write(dev, IFX_SCB_CTRL, ctrl & ~IFX_SCB_CTRL_ENABLED);

	ret = ifx_scb_uart_set_clock(dev, ucfg->baudrate);
	if (ret != 0) {
		ifx_scb_write(dev, IFX_SCB_CTRL, ctrl);
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

	tx_fifo = FIELD_PREP(IFX_SCB_FIFO_TRIGGER_MSK,
			     IFX_SCB_TX_TRIGGER_LEVEL);
	ifx_scb_write(dev, IFX_SCB_TX_FIFO_CTRL, IFX_SCB_FIFO_CLEAR | tx_fifo);
	ifx_scb_write(dev, IFX_SCB_TX_FIFO_CTRL, tx_fifo);
	ifx_scb_write(dev, IFX_SCB_RX_FIFO_CTRL, IFX_SCB_FIFO_CLEAR);
	ifx_scb_write(dev, IFX_SCB_RX_FIFO_CTRL, 0U);

	ifx_scb_rmw(dev, IFX_SCB_CTRL, 0U, IFX_SCB_CTRL_ENABLED);

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	data->uart_cfg = *ucfg;
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

out:
	k_spin_unlock(&data->lock, key);

	return ret;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int ifx_scb_uart_config_get(const struct device *dev,
				   struct uart_config *ucfg)
{
	struct ifx_scb_uart_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		*ucfg = data->uart_cfg;
	}

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

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

static int ifx_scb_uart_err_check(const struct device *dev)
{
	uint32_t intr = ifx_scb_read(dev, IFX_SCB_INTR_RX) &
			IFX_SCB_INTR_RX_ERRORS;
	int err = 0;

	if ((intr & IFX_SCB_INTR_RX_OVERFLOW) != 0U) {
		err |= UART_ERROR_OVERRUN;
	}

	if ((intr & IFX_SCB_INTR_RX_FRAME_ERROR) != 0U) {
		err |= UART_ERROR_FRAMING;
	}

	if ((intr & IFX_SCB_INTR_RX_PARITY_ERROR) != 0U) {
		err |= UART_ERROR_PARITY;
	}

	if (intr != 0U) {
		ifx_scb_clear_intr(dev, IFX_SCB_INTR_RX, intr);
	}

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static ALWAYS_INLINE uint32_t ifx_scb_tx_used(const struct device *dev)
{
	return ifx_scb_read(dev, IFX_SCB_TX_FIFO_STATUS) &
	       IFX_SCB_FIFO_USED_MSK;
}

static int ifx_scb_uart_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data, int size)
{
	struct ifx_scb_uart_data *data = dev->data;
	int n = 0;

	K_SPINLOCK(&data->lock) {
		uint32_t used = ifx_scb_tx_used(dev);

		while ((n < size) && (used < IFX_SCB_FIFO_SIZE)) {
			ifx_scb_write(dev, IFX_SCB_TX_FIFO_WR, tx_data[n]);
			n++;
			used++;
		}
	}

	return n;
}

static int ifx_scb_uart_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	int n = 0;

	while ((n < size) &&
	       ((ifx_scb_read(dev, IFX_SCB_RX_FIFO_STATUS) &
		 IFX_SCB_FIFO_USED_MSK) != 0U)) {
		rx_data[n] = (uint8_t)ifx_scb_read(dev, IFX_SCB_RX_FIFO_RD);
		n++;
	}

	return n;
}

static void ifx_scb_uart_irq_enable(const struct device *dev, uint32_t intr,
				    uint32_t mask, uint32_t bits)
{
	struct ifx_scb_uart_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		ifx_scb_clear_intr(dev, intr, bits);
		ifx_scb_rmw(dev, mask, 0U, bits);
	}
}

static void ifx_scb_uart_irq_disable(const struct device *dev, uint32_t mask,
				     uint32_t bits)
{
	struct ifx_scb_uart_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		ifx_scb_rmw(dev, mask, bits, 0U);
	}
}

static void ifx_scb_uart_irq_tx_enable(const struct device *dev)
{
	ifx_scb_uart_irq_enable(dev, IFX_SCB_INTR_TX, IFX_SCB_INTR_TX_MASK,
				IFX_SCB_INTR_TX_TRIGGER);
}

static void ifx_scb_uart_irq_tx_disable(const struct device *dev)
{
	ifx_scb_uart_irq_disable(dev, IFX_SCB_INTR_TX_MASK,
				 IFX_SCB_INTR_TX_TRIGGER);
}

static int ifx_scb_uart_irq_tx_ready(const struct device *dev)
{
	uint32_t mask = ifx_scb_read(dev, IFX_SCB_INTR_TX_MASK);

	return (((mask & IFX_SCB_INTR_TX_TRIGGER) != 0U) &&
		(ifx_scb_tx_used(dev) < IFX_SCB_FIFO_SIZE)) ? 1 : 0;
}

static int ifx_scb_uart_irq_tx_complete(const struct device *dev)
{
	uint32_t status = ifx_scb_read(dev, IFX_SCB_TX_FIFO_STATUS);

	return (((status & IFX_SCB_FIFO_USED_MSK) == 0U) &&
		((status & IFX_SCB_FIFO_SR_VALID) == 0U)) ? 1 : 0;
}

static void ifx_scb_uart_irq_rx_enable(const struct device *dev)
{
	ifx_scb_uart_irq_enable(dev, IFX_SCB_INTR_RX, IFX_SCB_INTR_RX_MASK,
				IFX_SCB_INTR_RX_NOT_EMPTY);
}

static void ifx_scb_uart_irq_rx_disable(const struct device *dev)
{
	ifx_scb_uart_irq_disable(dev, IFX_SCB_INTR_RX_MASK,
				 IFX_SCB_INTR_RX_NOT_EMPTY);
}

static int ifx_scb_uart_irq_rx_ready(const struct device *dev)
{
	uint32_t used = ifx_scb_read(dev, IFX_SCB_RX_FIFO_STATUS) &
			IFX_SCB_FIFO_USED_MSK;

	return (used != 0U) ? 1 : 0;
}

static void ifx_scb_uart_irq_err_enable(const struct device *dev)
{
	ifx_scb_uart_irq_enable(dev, IFX_SCB_INTR_RX, IFX_SCB_INTR_RX_MASK,
				IFX_SCB_INTR_RX_ERRORS);
}

static void ifx_scb_uart_irq_err_disable(const struct device *dev)
{
	ifx_scb_uart_irq_disable(dev, IFX_SCB_INTR_RX_MASK,
				 IFX_SCB_INTR_RX_ERRORS);
}

static int ifx_scb_uart_irq_is_pending(const struct device *dev)
{
	uint32_t cause = ifx_scb_read(dev, IFX_SCB_INTR_CAUSE);

	return ((cause & (IFX_SCB_INTR_CAUSE_TX | IFX_SCB_INTR_CAUSE_RX)) !=
		0U) ? 1 : 0;
}

static void ifx_scb_uart_irq_update(const struct device *dev)
{
	ifx_scb_clear_intr(dev, IFX_SCB_INTR_TX, IFX_SCB_INTR_TX_TRIGGER);
	ifx_scb_clear_intr(dev, IFX_SCB_INTR_RX, IFX_SCB_INTR_RX_NOT_EMPTY);
}

static void ifx_scb_uart_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb,
					  void *cb_data)
{
	struct ifx_scb_uart_data *data = dev->data;

	data->cb = cb;
	data->cb_data = cb_data;
}

static void ifx_scb_uart_isr(const struct device *dev)
{
	struct ifx_scb_uart_data *data = dev->data;

	if (data->cb != NULL) {
		data->cb(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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

	ret = ifx_scb_uart_configure(dev, &cfg->uart_cfg);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config();
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

static DEVICE_API(uart, ifx_scb_uart_api) = {
	.poll_in = ifx_scb_uart_poll_in,
	.poll_out = ifx_scb_uart_poll_out,
	.err_check = ifx_scb_uart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = ifx_scb_uart_configure,
	.config_get = ifx_scb_uart_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = ifx_scb_uart_fifo_fill,
	.fifo_read = ifx_scb_uart_fifo_read,
	.irq_tx_enable = ifx_scb_uart_irq_tx_enable,
	.irq_tx_disable = ifx_scb_uart_irq_tx_disable,
	.irq_tx_ready = ifx_scb_uart_irq_tx_ready,
	.irq_tx_complete = ifx_scb_uart_irq_tx_complete,
	.irq_rx_enable = ifx_scb_uart_irq_rx_enable,
	.irq_rx_disable = ifx_scb_uart_irq_rx_disable,
	.irq_rx_ready = ifx_scb_uart_irq_rx_ready,
	.irq_err_enable = ifx_scb_uart_irq_err_enable,
	.irq_err_disable = ifx_scb_uart_irq_err_disable,
	.irq_is_pending = ifx_scb_uart_irq_is_pending,
	.irq_update = ifx_scb_uart_irq_update,
	.irq_callback_set = ifx_scb_uart_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define IFX_SCB_UART_IRQ_CONFIG(n)                                             \
	static void ifx_scb_uart_irq_config_##n(void)                          \
	{                                                                      \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),         \
			    ifx_scb_uart_isr, DEVICE_DT_INST_GET(n), 0);       \
		irq_enable(DT_INST_IRQN(n));                                   \
	}

#define IFX_SCB_UART_IRQ_INIT(n)                                               \
	.irq_config = ifx_scb_uart_irq_config_##n,
#else
#define IFX_SCB_UART_IRQ_CONFIG(n)
#define IFX_SCB_UART_IRQ_INIT(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define IFX_SCB_UART_INIT(n)                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                             \
	IFX_SCB_UART_IRQ_CONFIG(n)                                             \
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
		IFX_SCB_UART_IRQ_INIT(n)                                       \
	};                                                                     \
                                                                               \
	static struct ifx_scb_uart_data ifx_scb_uart_data_##n;                 \
                                                                               \
	DEVICE_DT_INST_DEFINE(n, ifx_scb_uart_init, NULL,                      \
			      &ifx_scb_uart_data_##n,                          \
			      &ifx_scb_uart_config_##n, PRE_KERNEL_1,          \
			      CONFIG_SERIAL_INIT_PRIORITY, &ifx_scb_uart_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_SCB_UART_INIT)
